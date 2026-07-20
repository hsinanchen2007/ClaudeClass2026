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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 215. Kth Largest Element in an Array（陣列中的第 K 大元素）
// 題目：輸入未排序陣列 nums 與 k，回傳排序後第 k 大的值；例如
// [3,2,1,5,6,4]、k=2 回 5。
// 為何使用本章主題：make_heap 可在線性時間把完整輸入建成 max-heap，之後每次
// pop_heap 移除一個目前最大值，直到 heap front 成為第 k 大。
// 思路：1. 驗證 k 在 [1,N]；2. 建 max-heap；3. 執行 k-1 次 pop_heap 與 pop_back；
// 4. 回傳剩餘 heap 的 front。
// 複雜度：時間 O(N+K log N)、額外空間 O(N)，N 為 nums 數量；空間包含按值複製輸入。
// 易錯點：pop_heap 不會縮小 vector，必須再 pop_back；k 不可為 0 或大於 N。
// -----------------------------------------------------------------------------
int leetcode_find_kth_largest(std::vector<int> nums, int k) {
    assert(k >= 1 && static_cast<std::size_t>(k) <= nums.size());
    std::make_heap(nums.begin(), nums.end());
    for (int removed = 1; removed < k; ++removed) {
        std::pop_heap(nums.begin(), nums.end());
        nums.pop_back();
    }
    return nums.front();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】批次工作最早期限選取
// 情境：一次載入尚未排程的 Job，每筆有 deadline 與 id；要先取得截止時間最早的
// 工作 id，原輸入可由呼叫端繼續保留。
// 為何使用本章主題：make_heap 可從批次資料一次建 min-heap，成本低於逐筆排序；
// LaterDeadline 反轉預設順序，讓最小 deadline 位於 front。
// 設計：1. 比較器以較晚 deadline 為較低優先；2. 對 jobs 建 heap；3. 讀取 front.id。
// 成本：時間 O(N)、額外空間 O(N)，N 為工作數；額外空間來自按值接收 jobs。
// 上線注意：函式要求非空輸入；deadline 相同目前沒有穩定順序，正式排程應加入 sequence/id tie-break。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'make_heap.cpp' -o '/tmp/codex_cpp_C_Algorithm_heap_make_heap' && '/tmp/codex_cpp_C_Algorithm_heap_make_heap'
//
// === 預期輸出（節錄）===
// make_heap：建堆、第 K 大、deadline 佇列測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
