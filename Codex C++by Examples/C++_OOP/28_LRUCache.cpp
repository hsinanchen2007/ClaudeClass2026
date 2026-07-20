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

// LeetCode 146 官方範例的完整 operation sequence。
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

// 實務案例：包成可表達 miss 的 cache API。真實 value 可能是 -1，optional 比 sentinel 好。
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
