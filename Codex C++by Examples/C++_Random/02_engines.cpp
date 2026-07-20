// ============================================================================
// 課題 2：Random engines、state 與 reproducibility
// ============================================================================
//
// Engine 是 deterministic state machine：給相同 type/seed，engine 原始輸出序列由標準
// 規範（例如 mt19937），可 copy state、serialize、discard。Distribution 再把 engine bits
// 映射成 range/shape；distribution 的具體映射序列未必跨 standard library 保證一致。
//
// mt19937 state 大、速度/統計品質適合 simulation，不是 CSPRNG。default_random_engine 的
// 具體型別 implementation-defined，不適合要求跨機 reproducibility；請明寫 engine type。
// Engine 不預設 thread-safe，每 thread 各一份或加 synchronization。
// ============================================================================

#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

// 測試條件與其中的呼叫在 -DNDEBUG 下仍會執行。
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

void basic_example()
{
    std::mt19937 first(2026U);
    std::mt19937 second(2026U);
    for (int index = 0; index < 100; ++index) {
        expect(first() == second(), "相同 seed 的原始序列應一致");
    }

    std::mt19937 original(7U);
    original.discard(10);
    std::mt19937 copy = original;
    expect(original() == copy(), "複製 engine 應保留相同 state");
    std::cout << "[基礎] explicit engine seed/state gives reproducible raw sequence\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 380. Insert Delete GetRandom O(1)（O(1) 插入、刪除與隨機取值）
// 題目：維護不重複整數集合，insert/remove 回報成功與否，getRandom 等機率回傳現有元素；所有操作要求平均 O(1)。
// 為何使用本章主題：vector 提供 O(1) 隨機 index，hash map 記錄位置；mt19937 作為物件成員避免全域 rand 狀態並可注入 seed。
// 思路：1. insert 將值放到 vector 尾端並記 index。2. remove 以尾值覆蓋洞。3. 更新尾值索引後 pop/erase。4. 均勻抽合法 index。
// 複雜度：三種操作平均時間 O(1)、集合空間 O(N)，N 為元素數；hash 最壞情況可退化。
// 易錯點：刪除最後元素時仍須維持 map/vector 一致；空集合不可建 [0,size-1]，配置例外也可能讓雙容器更新只完成一半。
// -----------------------------------------------------------------------------
class RandomizedSet {
public:
    explicit RandomizedSet(unsigned seed) : engine_(seed) {}
    bool insert(int value)
    {
        if (index_.count(value) != 0U) return false;
        index_[value] = values_.size();
        values_.push_back(value);
        return true;
    }
    bool remove(int value)
    {
        const auto found = index_.find(value);
        if (found == index_.end()) return false;
        const std::size_t position = found->second;
        values_.at(position) = values_.back();
        index_[values_.at(position)] = position;
        values_.pop_back();
        index_.erase(found);
        return true;
    }
    int getRandom()
    {
        // 【契約】空集合沒有可取樣元素，所有建置模式都丟 underflow_error。
        if (values_.empty()) {
            throw std::underflow_error("RandomizedSet 為空，無法隨機取樣");
        }
        std::uniform_int_distribution<std::size_t> pick(0U, values_.size() - 1U);
        return values_.at(pick(engine_));
    }
private:
    std::vector<int> values_;
    std::unordered_map<int, std::size_t> index_;
    std::mt19937 engine_;
};

void leetcode_380_example()
{
    RandomizedSet set(42U);

    // insert/remove 會修改集合，不能藏在 release 會移除的 assert 內。
    const bool inserted_one = set.insert(1);
    const bool removed_missing_two = set.remove(2);
    const bool inserted_two = set.insert(2);
    expect(inserted_one, "首次 insert(1) 應成功");
    expect(!removed_missing_two, "remove(2) 應回報元素不存在");
    expect(inserted_two, "首次 insert(2) 應成功");

    const int random = set.getRandom();
    expect(random == 1 || random == 2, "隨機結果必須屬於集合");

    const bool removed_one = set.remove(1);
    const bool inserted_duplicate_two = set.insert(2);
    expect(removed_one, "remove(1) 應成功");
    expect(!inserted_duplicate_two, "重複 insert(2) 應失敗");
    expect(set.getRandom() == 2, "單元素集合只能取到該元素");
    std::cout << "[LeetCode 380] member engine + O(1) vector/map operations\n";
}

void boundary_example()
{
    RandomizedSet set(1U);
    bool empty_rejected = false;
    try {
        static_cast<void>(set.getRandom());
    } catch (const std::underflow_error&) {
        empty_rejected = true;
    }
    expect(empty_rejected, "空 RandomizedSet 的 getRandom 必須被拒絕");

    const bool inserted = set.insert(-1);
    const bool removed = set.remove(-1);
    expect(inserted && removed, "最後一個元素應可安全移除");
    expect(!set.remove(-1), "已移除的元素不可再次移除");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可重播的重試成功模擬
// 情境：測試以每次 70% 成功率模擬最多 10 次請求，失敗報告要能靠 seed=99 重現相同嘗試次數。
// 為何使用本章主題：局部 mt19937 加 bernoulli_distribution 明確建模成功率；固定 seed 比依賴隱藏全域 state 容易重播。
// 設計：1. 由輸入 seed 建 engine。2. 建立 p=0.7 的 Bernoulli 分布。3. 失敗便遞增 attempts。4. 成功或達 10 次即回傳。
// 成本：最多 10 次 draw，時間 O(1)、空間為一個 engine state；若上限改為 K，時間 O(K)。
// 上線注意：需把 seed、工具鏈與呼叫順序一起記錄；這只是測試模型，不能取代真實 retry deadline、backoff 與錯誤分類。
// -----------------------------------------------------------------------------
int simulated_retry_count(unsigned seed)
{
    std::mt19937 engine(seed);
    std::bernoulli_distribution succeeds(0.7);
    int attempts = 1;
    while (!succeeds(engine) && attempts < 10) ++attempts;
    return attempts;
}

void practical_example()
{
    const int first = simulated_retry_count(99U);
    const int replay = simulated_retry_count(99U);
    expect(first == replay, "相同 seed 應重播相同 retry count");
    std::cout << "[實務] recorded seed 99 reproduces retry count=" << first << '\n';
}

int main()
{
    basic_example();
    leetcode_380_example();
    boundary_example();
    practical_example();
}

// 易錯與面試：engine 產生 deterministic raw sequence，distribution 才映射到目標 domain。
// 相同 seed 的跨標準庫 distribution 結果未必相同；需要長期 replay 時要記工具鏈/演算法。
// 練習：serialize mt19937 到 stringstream，再 restore 並驗下一個 raw output 相同。
// 生命週期：engine object 擁有完整 state；copy 會複製序列位置，temporary engine 每次重建會重播。

/*
 * 【教科書補充：RandomizedSet 是雙容器交易】
 * - vector 與 index map 必須維持一一對應；先改 map、後 push vector 時，配置失敗會留下壞索引。
 * - production insert/remove 應安排 commit 順序與 rollback guard，並對 allocator failure 做 fault injection。
 * - engine 決定狀態序列，distribution 把序列映射到目標範圍；不要用 `% size` 取代 distribution。
 * - engine seed 相同利於測試重現，不代表不同標準庫的 distribution 轉換結果 bitwise 相同。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_engines.cpp' -o '/tmp/codex_cpp_C_Random_02_engines' && '/tmp/codex_cpp_C_Random_02_engines'
//
// === 預期輸出（節錄）===
// [LeetCode 380] member engine + O(1) vector/map operations
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
