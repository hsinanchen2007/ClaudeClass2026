/*
 * std::push_heap：把「剛附加在尾端」的一個元素納入既有 heap
 * =============================================================
 * 正確順序：container.push_back(x)；std::push_heap(begin,end,comp)。
 * push_heap 不會替容器增加元素，只重排 [first,last)。前置條件是 [first,last-1)
 * 已是使用同一 comparator 的 heap，否則契約不成立。
 *
 * 複雜度 O(log N) 比較/交換。vector 的 push_back 可能 reallocate，使先前所有
 * iterator/reference 失效；呼叫 push_heap 時應重新取 begin/end。
 */

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 703. Kth Largest Element in a Stream（資料流中的第 K 大元素）
// 題目：以 k 與初始 nums 建立物件，每次 add(value) 後回傳目前資料流第 k 大；例如
// k=3、初始 [4,5,8,2]，依序加入 3、5、10 時回 4、5、5。
// 為何使用本章主題：只保留大小至多 K 的 min-heap；front 是保留集合最小值，也就是
// 全部已見元素的第 K 大，push_heap 可增量納入每個新值。
// 思路：1. 每筆先 push_back 並 push_heap；2. 超過 K 時 pop_heap 丟掉最小值；3. 以
// front 回目前門檻；4. wrapper 依序收集每次 add 的結果。
// 複雜度：每次 add 時間 O(log K)、物件空間 O(K)；wrapper 另用 O(A) 保存 A 次輸出。
// 易錯點：K 必須大於 0；min-heap 的所有 push/pop 都要使用 std::greater；front 在空 heap 不可讀。
// -----------------------------------------------------------------------------
class KthLargest {
public:
    KthLargest(std::size_t k, const std::vector<int>& nums) : k_(k) {
        assert(k_ > 0U);
        for (int value : nums) {
            static_cast<void>(add(value));
        }
    }

    int add(int value) {
        heap_.push_back(value);
        std::push_heap(heap_.begin(), heap_.end(), std::greater<>{});
        if (heap_.size() > k_) {
            std::pop_heap(heap_.begin(), heap_.end(), std::greater<>{});
            heap_.pop_back();
        }
        return heap_.front();
    }

private:
    std::size_t k_;
    std::vector<int> heap_;
};

std::vector<int> leetcode_kth_largest_results(
    std::size_t k, const std::vector<int>& initial,
    const std::vector<int>& additions) {
    KthLargest kth(k, initial);
    std::vector<int> result;
    result.reserve(additions.size());
    for (int value : additions) {
        result.push_back(kth.add(value));
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】監控延遲樣本 Top-K 保留
// 情境：服務持續產生 latency 樣本，只需保留最高 K 筆供事故診斷，避免完整保存所有
// 數值；回傳前再依高到低排序。
// 為何使用本章主題：固定大小 min-heap 以 O(K) 記憶體保存目前最大 K 筆；每個新值
// 只需與 heap 門檻競爭，比每次完整排序所有樣本省成本。
// 設計：1. 每筆加入 min-heap；2. size>K 時移除最小值；3. 串流結束後將 K 筆降冪排序。
// 成本：時間 O(N log K + K log K)、額外空間 O(K)，N 為樣本數。
// 上線注意：K=0 時目前實作會對空 heap 操作，呼叫端必須拒絕；多執行緒寫入需同步或分片歸併。
// -----------------------------------------------------------------------------
std::vector<int> practical_top_latencies(const std::vector<int>& samples,
                                         std::size_t k) {
    std::vector<int> heap;
    for (int value : samples) {
        heap.push_back(value);
        std::push_heap(heap.begin(), heap.end(), std::greater<>{});
        if (heap.size() > k) {
            std::pop_heap(heap.begin(), heap.end(), std::greater<>{});
            heap.pop_back();
        }
    }
    std::sort(heap.begin(), heap.end(), std::greater<>{});
    return heap;
}

int main() {
    std::vector<int> heap{8, 4, 6};
    std::make_heap(heap.begin(), heap.end());
    heap.push_back(10);
    std::push_heap(heap.begin(), heap.end());
    assert(heap.front() == 10);

    assert((leetcode_kth_largest_results(3U, {4, 5, 8, 2}, {3, 5, 10}) ==
            std::vector<int>{4, 5, 5}));

    assert((practical_top_latencies({12, 90, 30, 70, 50}, 3U) ==
            std::vector<int>{90, 70, 50}));
    std::cout << "push_heap：串流第 K 大與 top-k 監控測試通過\n";
}

/*
 * 面試陷阱：大小為 k 的 min-heap 為何求 top-k 最大值？因為 front 是保留集合
 * 中最小者，新值只需與這道門檻競爭。每筆 O(log k)、空間 O(k)。
 * 練習：加入 sequence number，讓相同 priority 的工作依到達順序處理。
 * LeetCode 測試透過 wrapper 一次驗證多筆串流輸出；實務監控則最後只為顯示而
 * 排序。熱路徑若不需完整排序，可以直接保留 heap，避免額外 O(k log k)。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'push_heap.cpp' -o '/tmp/codex_cpp_C_Algorithm_heap_push_heap' && '/tmp/codex_cpp_C_Algorithm_heap_push_heap'
//
// === 預期輸出（節錄）===
// push_heap：串流第 K 大與 top-k 監控測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
