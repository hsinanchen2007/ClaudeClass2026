// ============================================================================
// priority_queue：每次取出最高優先權元素的 heap adapter
// ============================================================================
// 預設 priority_queue<T> 是 max-heap：top() 為最大值。push/pop O(log n)，top O(1)，
// 建構自範圍通常 O(n)。底層預設 vector。第三個模板參數 Compare 的直覺容易反：
// std::greater<T> 會形成 min-heap，使最小值在 top。
//
// 它只保證 top，不保證底層完整排序；相同優先權的先後順序不穩定。若需要 FIFO
// tie-break，將 sequence number 一起放進比較鍵。

#include <cassert>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

void basic_demo()
{
    std::priority_queue<int> maximums;
    for (const int value : {4, 1, 9, 3}) {
        maximums.push(value);
    }
    assert(maximums.top() == 9);

    std::priority_queue<int, std::vector<int>, std::greater<int>> minimums;
    for (const int value : {4, 1, 9, 3}) {
        minimums.push(value);
    }
    assert(minimums.top() == 1);
}

// ----------------------------------------------------------------------------
// LeetCode 215：Kth Largest Element in an Array
// ----------------------------------------------------------------------------
// 維持最多 k 個值的 min-heap；top 是目前第 k 大。時間 O(n log k)、空間 O(k)。
int kth_largest(const std::vector<int>& numbers, std::size_t k)
{
    assert(k > 0U && k <= numbers.size());
    std::priority_queue<int, std::vector<int>, std::greater<int>> best;
    for (const int value : numbers) {
        best.push(value);
        if (best.size() > k) {
            best.pop();
        }
    }
    return best.top();
}

void leetcode_demo()
{
    assert(kth_largest({3, 2, 1, 5, 6, 4}, 2U) == 5);
    assert(kth_largest({3, 2, 3, 1, 2, 4, 5, 5, 6}, 4U) == 4);
}

// ----------------------------------------------------------------------------
// 實務：可重現的工作排程（高 priority 先，同分時較早 sequence 先）
// ----------------------------------------------------------------------------
struct Job {
    int priority;
    int sequence;
    std::string name;
};

struct LowerPriority {
    bool operator()(const Job& left, const Job& right) const
    {
        // priority 小者較晚；同 priority 時 sequence 大者較晚。
        return std::tie(left.priority, right.sequence) <
               std::tie(right.priority, left.sequence);
    }
};

void practical_demo()
{
    std::priority_queue<Job, std::vector<Job>, LowerPriority> jobs;
    jobs.push({5, 0, "normal-A"});
    jobs.push({9, 1, "urgent"});
    jobs.push({5, 2, "normal-B"});
    assert(jobs.top().name == "urgent");
    jobs.pop();
    assert(jobs.top().name == "normal-A");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "priority_queue：heap、Top-K 與穩定 tie-break 測試通過\n";
}

// 【陷阱】修改已在 heap 內物件的比較欄位會破壞 heap invariant；通常 push 新版本。
// 【面試】Top-K 為何用大小 k 的 min-heap，而非把 n 個元素全排序？
// 【練習】實作 LeetCode 703 Kth Largest in a Stream。
