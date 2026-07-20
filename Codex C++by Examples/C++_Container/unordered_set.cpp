// ============================================================================
// unordered_set：hash-based 唯一值集合
// ============================================================================
// unordered_set 適合 membership、去重與 visited 集合；contains/find/insert/erase 平均
// O(1)、最差 O(n)。它不提供 lower_bound，也不保證遍歷順序。大量已知筆數先
// reserve；rehash 使 iterator 失效。元素視為 const key，不可原地改造成另一 hash。

#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
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
    std::unordered_set<std::string> features;
    features.reserve(4U);
    const auto [first, inserted] = features.insert("cuda");
    expect(inserted && *first == "cuda", "首次 insert 應成功");
    expect(!features.insert("cuda").second, "重複 insert 應失敗");
    expect(features.contains("cuda"), "contains 應找到既有元素");
    expect(!features.contains("opencl"), "contains 不應找到缺少元素");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 128. Longest Consecutive Sequence（最長連續序列）
// 題目：在未排序整數中找最長連續值序列長度；例如 [100,4,200,1,3,2] 回傳 4。
// 為何使用本章主題：unordered_set 去重並提供平均 O(1) membership，能避開整體排序。
// 思路：建 values 集合；只從沒有前驅的值起跑；向右查連續值；更新最長長度。
// 複雜度：平均時間 O(N)、最壞 O(N^2)，額外空間 O(N)，N 為輸入長度。
// 易錯點：重複值不應重算；INT_MIN/MAX 前後運算需短路防 signed overflow；hash 最壞並非線性保證。
// -----------------------------------------------------------------------------
std::size_t longest_consecutive(const std::vector<int>& numbers)
{
    const std::unordered_set<int> values(numbers.begin(), numbers.end());
    constexpr int min_value = std::numeric_limits<int>::min();
    constexpr int max_value = std::numeric_limits<int>::max();
    std::size_t best = 0U;
    for (const int value : values) {
        if (value != min_value && values.contains(value - 1)) {
            continue;
        }

        std::size_t length = 1U;
        int current = value;
        while (current != max_value) {
            const int next = current + 1;  // current != INT_MAX，故加法可表示。
            if (!values.contains(next)) {
                break;
            }
            ++length;
            current = next;
        }
        if (length > best) {
            best = length;
        }
    }
    return best;
}

void leetcode_demo()
{
    expect(longest_consecutive({100, 4, 200, 1, 3, 2}) == 4U,
           "一般連續序列答案錯誤");
    expect(longest_consecutive({0, 3, 7, 2, 5, 8, 4, 6, 0, 1}) == 9U,
           "含重複值的連續序列答案錯誤");
    expect(longest_consecutive({}) == 0U, "空輸入答案應為零");

    constexpr int min_value = std::numeric_limits<int>::min();
    constexpr int max_value = std::numeric_limits<int>::max();
    expect(longest_consecutive({min_value, min_value + 1,
                                max_value - 1, max_value}) == 2U,
           "INT_MIN/MAX 邊界序列答案錯誤");
    expect(longest_consecutive({min_value, max_value}) == 1U,
           "不可讓 INT_MIN 與 INT_MAX 因溢位被視為相鄰");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】冪等 API 的 request-id 去重
// 情境：服務收到可能重送的 req-42，首次接受處理，之後相同 request-id 必須拒絕以免重複扣款。
// 為何使用本章主題：unordered_set 只保留唯一 id，insert 回傳值可在一次平均 O(1) 操作中完成查重與登記。
// 設計：accept 嘗試 insert；inserted=true 代表新請求；false 代表已處理過。
// 成本：單次平均 O(1)、最壞 O(R)，空間 O(R)，R 為保留的 request-id 數。
// 上線注意：集合不可無界成長，需 TTL/持久化與跨節點一致性；id 要驗長度，查重與副作用需原子提交。
// -----------------------------------------------------------------------------
class RequestDeduplicator {
public:
    bool accept(const std::string& request_id)
    {
        return seen_.insert(request_id).second;
    }

private:
    std::unordered_set<std::string> seen_;
};

void practical_demo()
{
    RequestDeduplicator deduplicator;
    expect(deduplicator.accept("req-42"), "新 request-id 應被接受");
    expect(!deduplicator.accept("req-42"), "重複 request-id 應被拒絕");
    expect(deduplicator.accept("req-43"), "另一個 request-id 應被接受");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "unordered_set：membership、連續序列與 request 去重測試通過\n";
}

// 【常見陷阱】無界 request-id set 會持續吃記憶體；production 要 TTL/分片/持久化。
// 【邊界】對 signed int 做 value±1 前必須確認不是 INT_MIN/INT_MAX，否則是 UB。
// 【面試】為何最差 O(n)？許多 key 可能落同 bucket，退化成線性搜尋。
// 【練習】實作 LeetCode 349 Intersection of Two Arrays。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'unordered_set.cpp' -o '/tmp/codex_cpp_C_Container_unordered_set' && '/tmp/codex_cpp_C_Container_unordered_set'
//
// === 預期輸出（節錄）===
// unordered_set：membership、連續序列與 request 去重測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
