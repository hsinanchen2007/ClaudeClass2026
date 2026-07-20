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

// LeetCode 703：資料流中的第 K 大。
// 保留大小最多 k 的 min-heap，front 是目前第 k 大。
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

// 實務：監控只保留最高的 top-k latency，避免保存無限資料流。
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
