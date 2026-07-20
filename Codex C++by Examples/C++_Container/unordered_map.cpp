// ============================================================================
// unordered_map：hash table 的唯一鍵對照表
// ============================================================================
// unordered_map<Key,Value> 以 hash 選 bucket，再用 equality 找 key。find/insert/erase
// 平均 O(1)、最差 O(n)；無排序、遍歷順序不可依賴，rehash 後順序也可能改變。
// 若 key_equal(a,b) 為 true，hash(a) 必須等於 hash(b)，反向不必成立。
//
// load_factor = size / bucket_count。超過 max_load_factor 時可能 rehash；rehash 使
// 所有 iterator 失效，但未被 erase 的元素 reference/pointer 仍有效。reserve(n)
// 可在大量插入前預留足夠 bucket，減少 rehash。[]/at/find 語意與 map 相同。

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void expect(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

void basic_demo()
{
    std::unordered_map<std::string, int> counts;
    counts.reserve(8U);
    const std::vector<std::string> words{"gpu", "cpu", "gpu"};
    for (const std::string& word : words) {
        ++counts[word];  // 缺少 key 時插入 0。
    }
    expect(counts.at("gpu") == 2, "operator[] 計數錯誤");
    expect(counts.contains("cpu"), "contains 應找到既有 key");
    expect(counts.find("tpu") == counts.end(), "find 應回報缺少 key");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：找兩個不同索引使值相加為 target；例如 [2,7,11,15]、target=9 回傳 [0,1]。
// 為何使用本章主題：unordered_map 由已看過的值平均 O(1) 找索引，避免巢狀掃描的 O(N^2)。
// 思路：先以 int64_t 算 complement；查已看過的值；命中即回兩索引；未命中才插入目前值。
// 複雜度：平均時間 O(N)、最壞 O(N^2)，額外空間 O(N)，N 為輸入長度。
// 易錯點：必須先查再插以免元素自配；差值與索引轉 int 都要驗範圍；hash 最壞成本不是保證 O(1)。
// -----------------------------------------------------------------------------
std::vector<int> two_sum(const std::vector<int>& numbers, int target)
{
    std::unordered_map<int, std::size_t> index_by_value;
    index_by_value.reserve(numbers.size());
    constexpr auto min_int =
        static_cast<std::int64_t>(std::numeric_limits<int>::min());
    constexpr auto max_int =
        static_cast<std::int64_t>(std::numeric_limits<int>::max());

    for (std::size_t index = 0; index < numbers.size(); ++index) {
        const int value = numbers.at(index);
        const auto wide_complement = static_cast<std::int64_t>(target) -
                                     static_cast<std::int64_t>(value);
        if (wide_complement >= min_int && wide_complement <= max_int) {
            const int complement = static_cast<int>(wide_complement);
            if (const auto found = index_by_value.find(complement);
                found != index_by_value.end()) {
                if (found->second > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
                    index > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                    throw std::length_error("Two Sum index does not fit LeetCode int result");
                }
                return {static_cast<int>(found->second), static_cast<int>(index)};
            }
        }
        index_by_value.emplace(value, index);
    }
    return {};
}

void leetcode_demo()
{
    expect(two_sum({2, 7, 11, 15}, 9) == std::vector<int>{0, 1},
           "Two Sum 一般案例答案錯誤");
    expect(two_sum({3, 3}, 6) == std::vector<int>{0, 1},
           "Two Sum 重複值案例答案錯誤");

    constexpr int min_value = std::numeric_limits<int>::min();
    constexpr int max_value = std::numeric_limits<int>::max();
    expect(two_sum({min_value, max_value}, -1) == std::vector<int>{0, 1},
           "Two Sum 應支援 INT_MIN + INT_MAX");
    expect(two_sum({min_value, -1}, max_value).empty(),
           "超出 int 的正 complement 應安全略過");
    expect(two_sum({max_value, 1}, min_value).empty(),
           "超出 int 的負 complement 應安全略過");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】按服務聚合請求與錯誤指標
// 情境：事件流包含 service 與 failed，需產出每個服務的 request/error 計數供監控報表使用。
// 為何使用本章主題：unordered_map 適合依服務名稱高頻更新計數，不需要 map 的排序成本。
// 設計：預留 bucket；以 operator[] 取得或建立零值 Counters；累加 requests；失敗時再累加 errors。
// 成本：平均時間 O(E)、最壞 O(E^2)，額外空間 O(S)，E 為事件數、S 為不同服務數。
// 上線注意：計數器需防溢位；輸出若要求可重現需排序；並行聚合要分片或同步，服務名稱也應限制長度。
// -----------------------------------------------------------------------------
struct Counters {
    std::size_t requests{};
    std::size_t errors{};
};

struct Event {
    std::string service;
    bool failed;
};

std::unordered_map<std::string, Counters> aggregate(
    const std::vector<Event>& events)
{
    std::unordered_map<std::string, Counters> result;
    result.reserve(events.size());
    for (const Event& event : events) {
        Counters& counters = result[event.service];
        ++counters.requests;
        if (event.failed) {
            ++counters.errors;
        }
    }
    return result;
}

void practical_demo()
{
    const auto result = aggregate({{"api", false}, {"db", true}, {"api", true}});
    expect(result.at("api").requests == 2, "api request 聚合錯誤");
    expect(result.at("api").errors == 1, "api error 聚合錯誤");
    expect(result.at("db").errors == 1, "db error 聚合錯誤");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "unordered_map：hash 查找、Two Sum 與指標聚合測試通過\n";
}

// 【陷阱】不可把輸出順序拿來做 snapshot/測試契約；需要 deterministic output 就排序。
// 【陷阱】自訂 key 的 hash 若漏掉 equality 使用的欄位，可能效能差；若反過來違反
//         equal=>same hash，則查找行為錯誤。
// 【邊界】兩個 int 都合法，不代表其和或差也能用 int 表示；先提升再做算術。
// 【面試】reserve 對 iterator 有何影響？呼叫當下若 rehash，舊 iterator 立即失效。
// 【練習】為 aggregate 輸出依服務名稱排序的 vector 報表。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'unordered_map.cpp' -o '/tmp/codex_cpp_C_Container_unordered_map' && '/tmp/codex_cpp_C_Container_unordered_map'
//
// === 預期輸出（節錄）===
// unordered_map：hash 查找、Two Sum 與指標聚合測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
