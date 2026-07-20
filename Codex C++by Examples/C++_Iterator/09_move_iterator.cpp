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

// LeetCode 56（Merge Intervals）的資料搬移版本：先排序再把結果 move 出去。
// vector<int> 本身很小，但 API 形狀可直接套到大型 payload。
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

struct Job {
    int id{};
    std::string payload;
};

// 實務：producer 完成一批 jobs 後，把整批 ownership 交給 consumer queue。
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
