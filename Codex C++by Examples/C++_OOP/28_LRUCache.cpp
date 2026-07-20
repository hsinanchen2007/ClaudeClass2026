// ============================================================================
// 課題 28：綜合實戰 - LeetCode 146 LRU Cache
// ============================================================================
//
// LRU（Least Recently Used）要求 get/put 平均 O(1)：
//   * std::list 保存 MRU→LRU 順序；已知 iterator 的 erase/splice 是 O(1)。
//   * unordered_map 將 key 映射到 list iterator，O(1) 找 entry。
//
// 兩個 containers 必須維持 invariant：map 每一 iterator 都指向 list 中同 key；list 中
// 每個 key 恰有一份。capacity 滿時刪 list.back() 前，必須先從 map erase 同 key。
//
// 本檔整合 encapsulation、constructor invariant、custom class、STL iterator、RAII 與
// Rule of Zero。production cache 還要處理 thread safety、TTL、memory-bytes capacity、
// cache stampede、metrics；面試題只要求單執行緒 fixed entry count。
// ============================================================================

#include <cstddef>
#include <iostream>
#include <list>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace {

// 教材內的驗證在 release build 也必須生效。
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache（LRU 快取）
// 題目：以固定容量支援平均 O(1) get/put，命中與更新都刷新最近使用；官方容量 2 序列會依次淘汰 2、1。
// 為何使用本章主題：class 封裝 list+unordered_map 的跨容器 invariant，並以 constructor 保證容量為正。
// 思路：map 定位 list node；get/更新用 splice 移至前端；新 key 插前端並建索引；超量刪尾端與 map key。
// 複雜度：平均 get/put O(1)、hash 最壞 O(N)，空間 O(C)，C 為容量。
// 易錯點：get 也會改 recency；miss 用 optional 而非 -1；map 配置失敗時需回滾新 list node，並行操作需鎖。
// -----------------------------------------------------------------------------
class LRUCache {
public:
    explicit LRUCache(int capacity) : capacity_(validate(capacity)) {}

    // 【契約】nullopt 才表示 miss；所有 int（包含 -1）都是合法快取值。
    // cache hit 也會更新 recency，因此 get 不是 const operation。
    [[nodiscard]] std::optional<int> get(int key)
    {
        const auto found = index_.find(key);
        if (found == index_.end()) return std::nullopt;
        touch(found->second);
        return found->second->value;
    }

    void put(int key, int value)
    {
        const auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->value = value;
            touch(found->second);
            return;
        }

        entries_.push_front(Entry{key, value});
        index_[key] = entries_.begin();
        if (entries_.size() > capacity_) {
            const int evicted_key = entries_.back().key;
            index_.erase(evicted_key);
            entries_.pop_back();
        }
    }

    std::size_t size() const { return entries_.size(); }

private:
    struct Entry {
        int key;
        int value;
    };
    using Iterator = std::list<Entry>::iterator;

    static std::size_t validate(int capacity)
    {
        if (capacity <= 0) throw std::invalid_argument("capacity must be positive");
        return static_cast<std::size_t>(capacity);
    }

    void touch(Iterator entry)
    {
        entries_.splice(entries_.begin(), entries_, entry); // iterator 仍有效。
    }

    std::size_t capacity_;
    std::list<Entry> entries_; // front=MRU, back=LRU。
    std::unordered_map<int, Iterator> index_;
};

void basic_example()
{
    LRUCache cache(2);
    cache.put(1, 100);
    cache.put(2, 200);
    expect(cache.get(1) == std::optional<int>{100},
           "key 1 應命中並成為 MRU"); // key 1 變 MRU，key 2 變 LRU。
    cache.put(3, 300);           // 淘汰 key 2。
    expect(!cache.get(2).has_value(), "key 2 應已被淘汰");
    expect(cache.get(3) == std::optional<int>{300}, "key 3 應命中");
    std::cout << "[基礎] get 會 touch，put 超量淘汰 LRU\n";
}

void leetcode_146_example()
{
    LRUCache cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    expect(cache.get(1) == std::optional<int>{1}, "key 1 應命中");
    cache.put(3, 3);
    expect(!cache.get(2).has_value(), "key 2 應已被淘汰");
    cache.put(4, 4);
    expect(!cache.get(1).has_value(), "key 1 應已被淘汰");
    expect(cache.get(3) == std::optional<int>{3}, "key 3 應命中");
    expect(cache.get(4) == std::optional<int>{4}, "key 4 應命中");
    expect(cache.size() == 2U, "cache 大小不應超過容量");
    std::cout << "[LeetCode 146] official sequence 全部通過\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】模型參數量 metadata 快取
// 情境：按 model id 快取 parameter_millions；合法值可能是 -1，因此 miss 必須與資料值分開表達。
// 為何使用本章主題：ModelMetadataCache composition 重用 LRU 行為，optional<int> 比 sentinel API 保留完整 value domain。
// 設計：remember 委派 put；lookup 委派 get 並刷新 recency；容量滿時由核心 cache 淘汰最舊模型。
// 成本：平均查詢/寫入 O(1)、空間 O(C)，C 為模型快取容量；最壞 hash O(C)。
// 上線注意：真實容量常按 bytes 而非筆數；需 TTL、命中率 metrics、stampede 防護與 thread safety。
// -----------------------------------------------------------------------------
class ModelMetadataCache {
public:
    explicit ModelMetadataCache(int capacity) : cache_(capacity) {}
    void remember(int model_id, int parameter_millions)
    {
        cache_.put(model_id, parameter_millions);
    }
    std::optional<int> lookup(int model_id)
    {
        return cache_.get(model_id);
    }
private:
    LRUCache cache_;
};

void practical_example()
{
    ModelMetadataCache models(2);
    models.remember(7, 1'000);
    models.remember(8, 7'000);
    expect(models.lookup(7) == std::optional<int>{1'000}, "model 7 應命中");
    models.remember(9, 70'000); // model 8 是 LRU，被淘汰。
    expect(!models.lookup(8).has_value(), "model 8 應已被淘汰");

    ModelMetadataCache negative_values(1);
    negative_values.remember(42, -1);
    expect(negative_values.lookup(42) == std::optional<int>{-1},
           "合法值 -1 不可被誤判為 cache miss");
    expect(!negative_values.lookup(99).has_value(), "不存在的 model 才應回 nullopt");
    std::cout << "[實務] model metadata cache 正確回報 hit/miss\n";
}

void boundary_example()
{
    bool zero_capacity_rejected = false;
    try {
        const LRUCache invalid(0);
        static_cast<void>(invalid);
    } catch (const std::invalid_argument&) {
        zero_capacity_rejected = true;
    }
    expect(zero_capacity_rejected, "容量零必須被拒絕");

    LRUCache cache(2);
    cache.put(1, -1);
    cache.put(2, 20);
    expect(cache.get(1) == std::optional<int>{-1}, "核心 cache 必須保存合法值 -1");
    cache.put(1, 10); // 更新既有 key，也要讓 key 1 成為 MRU。
    cache.put(3, 30);
    expect(!cache.get(2).has_value(), "更新既有 key 後應淘汰真正的 LRU");
    expect(cache.get(1) == std::optional<int>{10}, "既有 key 的 value 應被更新");
    expect(cache.size() == 2U, "更新既有 key 不可增加 cache 大小");
}

int main()
{
    basic_example();
    leetcode_146_example();
    practical_example();
    boundary_example();
}

// 易錯與面試：list iterator 存入 map 後，splice 保持 iterator 有效；erase node 前必須先
// 清 map entry。容量 0、重複 put、get 更新 recency 都是 LRU 高頻邊界測試。
// 練習：改成 template<class Key,class Value>，讓 optional<Value> 契約適用其他 value type。
// 生命週期：map 保存 list iterators，splice 不使其失效；erase node 後對應 iterator 必須同步移除。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '28_LRUCache.cpp' -o '/tmp/codex_cpp_C_OOP_28_LRUCache' && '/tmp/codex_cpp_C_OOP_28_LRUCache'
//
// === 預期輸出（節錄）===
// [實務] model metadata cache 正確回報 hit/miss
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
