// ============================================================================
// 課題 9：std::move_iterator - 解參考時產生 rvalue reference
// ============================================================================
//
// 一般 iterator 的 *it 是 T&；move_iterator 的 *it 是 T&&（概念上套 std::move）。
// 因此 copy algorithm 也能選到元素的 move constructor：
//   destination.insert(..., make_move_iterator(first), make_move_iterator(last));
//
// moved-from 物件仍「有效但值未指定」：可以析構、重新賦值、呼叫有明確前置條件的
// 操作；不能假設 string 一定變空。unique_ptr 移動後則規格明定為 nullptr。
//
// 易錯點：
//   * const T 無法真正移動資源，const T&& 通常只能呼叫 copy constructor。
//   * range 移動不是 transaction；中途配置失敗時來源可能已有部分元素被移動。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

void basic_example()
{
    std::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(10));
    source.push_back(std::make_unique<int>(20));

    std::vector<std::unique_ptr<int>> destination(
        std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
    assert(destination.size() == 2 && *destination[1] == 20);
    assert(source[0] == nullptr && source[1] == nullptr);
    std::cout << "[基礎] move_iterator transferred unique ownership\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 56. Merge Intervals（合併區間）
// 題目：合併所有重疊區間；例如 [[1,3],[2,6],[8,10],[15,18]] 得 [[1,6],[8,10],[15,18]]。
// 為何使用本章主題：演算法本身是排序掃描；本教學版在新區間出現時 move payload，展示 ownership 搬移而非解題必要技巧。
// 思路：按起點排序；逐區間判斷是否與末段分離；分離時 move 到結果；重疊時延伸末端。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 為區間數；不計回傳結果時排序可能仍需 O(log N) stack。
// 易錯點：每個區間必須有兩端且 start<=end；moved-from current 不可再依賴其值；vector<int> 很小，效益僅示意。
// -----------------------------------------------------------------------------
using Interval = std::vector<int>;

std::vector<Interval> merge_intervals(std::vector<Interval> intervals)
{
    std::sort(intervals.begin(), intervals.end());
    std::vector<Interval> merged;
    for (Interval& current : intervals) {
        if (merged.empty() || merged.back()[1] < current[0]) {
            merged.push_back(std::move(current));
        } else {
            merged.back()[1] = std::max(merged.back()[1], current[1]);
        }
    }
    return merged;
}

void leetcode_56_example()
{
    const auto result = merge_intervals({{1, 3}, {2, 6}, {8, 10}, {15, 18}});
    assert((result == std::vector<Interval>{{1, 6}, {8, 10}, {15, 18}}));
    std::cout << "[LeetCode 56] intervals merged while payloads move into output\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】將 staging 工作批次移交 consumer queue
// 情境：producer 完成含大型 payload 的一批 Job，要把整批 ownership 交給 queue 而不複製每個字串。
// 為何使用本章主題：move_iterator 令 range insert 看到 Job&&，比一般 iterator 複製 payload 更符合一次性交接。
// 設計：先依 staging 大小 reserve；以 move iterators 定義來源 range；insert 到 queue；清除有效但未指定的來源。
// 成本：時間 O(J)，額外配置由目的 vector 決定；payload 資源通常 O(1) 轉移，J 為工作數。
// 上線注意：搬移不是交易，中途失敗可能只搬一部分；不可假設 moved-from string 為空，並行交接需同步。
// -----------------------------------------------------------------------------
struct Job {
    int id{};
    std::string payload;
};

void practical_example()
{
    std::vector<Job> staging{{1, std::string(1024, 'A')}, {2, std::string(1024, 'B')}};
    std::vector<Job> queue;
    queue.reserve(staging.size());
    queue.insert(queue.end(), std::make_move_iterator(staging.begin()),
                 std::make_move_iterator(staging.end()));
    assert(queue.size() == 2 && queue[0].payload.size() == 1024);
    // 不 assert staging[i].payload.empty()：string moved-from 值未指定。
    staging.clear(); // 合法：moved-from objects 仍可析構。
    std::cout << "[實務] staging batch moved into work queue without payload copies\n";
}

int main()
{
    basic_example();
    leetcode_56_example();
    practical_example();
}

// 面試自問：std::move 為何本身不搬任何資料？move_iterator 又在哪一步觸發 move？
// 練習：替 Job 加 copy/move counter，比較普通 iterator 與 move_iterator 的建構次數。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_move_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_09_move_iterator' && '/tmp/codex_cpp_C_Iterator_09_move_iterator'
//
// === 預期輸出（節錄）===
// [實務] staging batch moved into work queue without payload copies
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
