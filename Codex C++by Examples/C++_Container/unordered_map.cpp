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

// ----------------------------------------------------------------------------
// LeetCode 1：Two Sum
// ----------------------------------------------------------------------------
// 對目前值先查 complement，再插入目前索引，避免 target=6 時單一 3 自配。
// target-number 先提升為 int64_t 再相減；結果超出 int 範圍時，不可能存在於 int key
// 的 map，直接略過查找。若直接以 int 相減，INT_MIN/MAX 邊界可能 signed overflow。
// 平均 O(n) 時間、O(n) 空間；回傳找到的第一組索引。
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

// ----------------------------------------------------------------------------
// 實務：按服務聚合請求與錯誤數
// ----------------------------------------------------------------------------
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
