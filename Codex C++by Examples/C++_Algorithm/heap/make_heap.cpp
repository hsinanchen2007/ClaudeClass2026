/*
 * std::make_heap：把 random-access 範圍原地建成 heap
 * ==================================================
 * 預設是 max-heap：front() 是最大元素。heap 只保證「父節點不小於子節點」，
 * 並非整體排序。vector 的底層排列可視為完全二元樹：子節點 2*i+1、2*i+2。
 *
 * make_heap(first,last,comp) 的複雜度是 O(N)，不是逐一 push 的 O(N log N)。
 * 它會重排元素，不改容器大小，也不保存 iterator。需要 random-access iterator，
 * 因此 vector/deque/array 可用，list 不可用。比較器必須是嚴格弱序；使用
 * greater<> 會得到 min-heap。
 */

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <vector>

// LeetCode 215：第 K 大元素。建 max-heap 後 pop k-1 次。
int leetcode_find_kth_largest(std::vector<int> nums, int k) {
    assert(k >= 1 && static_cast<std::size_t>(k) <= nums.size());
    std::make_heap(nums.begin(), nums.end());
    for (int removed = 1; removed < k; ++removed) {
        std::pop_heap(nums.begin(), nums.end());
        nums.pop_back();
    }
    return nums.front();
}

// 實務：一次載入待處理工作，建立 min-heap 讓最早 deadline 優先。
struct Job {
    int deadline;
    int id;
};

struct LaterDeadline {
    bool operator()(const Job& lhs, const Job& rhs) const {
        return lhs.deadline > rhs.deadline;
    }
};

int practical_earliest_job_id(std::vector<Job> jobs) {
    std::make_heap(jobs.begin(), jobs.end(), LaterDeadline{});
    return jobs.front().id;
}

int main() {
    std::vector<int> values{3, 1, 4, 1, 5, 9};
    std::make_heap(values.begin(), values.end());
    assert(values.front() == 9);
    assert(std::is_heap(values.begin(), values.end()));

    std::vector<int> min_values{3, 1, 4};
    std::make_heap(min_values.begin(), min_values.end(), std::greater<>{});
    assert(min_values.front() == 1);

    assert(leetcode_find_kth_largest({3, 2, 1, 5, 6, 4}, 2) == 5);
    assert(practical_earliest_job_id({{30, 3}, {10, 1}, {20, 2}}) == 1);

    std::cout << "make_heap：建堆、第 K 大、deadline 佇列測試通過\n";
}

/*
 * 面試：為何 bottom-up heapify 是 O(N)？大多數節點靠近葉端，只需很少下沉；
 * 各高度工作量加總為線性。常見錯誤是把 heap 當 sorted range，迭代輸出並不
 * 會得到排序結果。練習：改用大小為 k 的 min-heap 解第 K 大，分析空間 O(k)。
 *
 * 【LeetCode 選型】
 * LC215 若只查一次，nth_element 平均 O(N) 常更適合；本例選 heap 是為了學習
 * 連續 pop。若資料是 stream，無法一次 nth_element，大小 k min-heap 更合理。
 *
 * 【實務 comparator】
 * LaterDeadline 使用 lhs.deadline > rhs.deadline，使較小 deadline 成為 front。
 * 名稱用「較低優先」語意比 MinHeapCompare 更不易寫反。正式工作若 deadline
 * 相同，還要加入 sequence/id 作 tie-break，否則順序沒有穩定保證。
 * 易錯陷阱：make_heap 後直接 sort 會得到排序，但此後不能再假設仍是 heap；若要
 * 保持排程狀態，應透過 pop_heap 逐筆消費，或使用 priority_queue 封裝。
 */
