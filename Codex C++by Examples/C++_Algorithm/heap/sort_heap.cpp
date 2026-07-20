/*
 * std::sort_heap：把 heap 原地轉為排序範圍
 * ==========================================
 * 預設 max-heap 經 sort_heap 後成升冪。前置條件是整個範圍已是同 comparator
 * 的 heap。完成後「不再是 heap」（除非元素碰巧），需要繼續 push/pop 時必須
 * 重新 make_heap。
 *
 * 複雜度 O(N log N)，額外空間 O(1)，不保證 stable。一般要排序直接 std::sort
 * 更清楚；sort_heap 適合資料本來就在 heap 中，最後要一次輸出排序結果。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

// LeetCode 912：Sort an Array，以 heap sort 完成。
std::vector<int> leetcode_sort_array(std::vector<int> nums) {
    std::make_heap(nums.begin(), nums.end());
    std::sort_heap(nums.begin(), nums.end());
    return nums;
}

struct Metric {
    int value;
    int original_order;
};

// 實務：批次 top-score 最後輸出升冪；特別示範 heap sort 不穩定，
// 相同 value 的 original_order 不應被程式依賴。
std::vector<int> practical_sorted_scores(std::vector<int> scores) {
    std::make_heap(scores.begin(), scores.end());
    std::sort_heap(scores.begin(), scores.end());
    return scores;
}

int main() {
    std::vector<int> heap{4, 1, 7, 3};
    std::make_heap(heap.begin(), heap.end());
    std::sort_heap(heap.begin(), heap.end());
    assert((heap == std::vector<int>{1, 3, 4, 7}));

    assert((leetcode_sort_array({5, 2, 3, 1}) ==
            std::vector<int>{1, 2, 3, 5}));
    assert((leetcode_sort_array({5, 1, 1, 2, 0, 0}) ==
            std::vector<int>{0, 0, 1, 1, 2, 5}));
    assert((practical_sorted_scores({90, 70, 100}) ==
            std::vector<int>{70, 90, 100}));

    std::cout << "sort_heap：heap sort 與批次輸出測試通過\n";
}

/*
 * 面試比較：heap sort 最壞 O(N log N)、O(1) 額外空間、不穩定；std::sort 通常
 * introsort，平均/最壞 O(N log N) 且常數通常更好；stable_sort 保序但需空間。
 * 練習：用 greater<> 建 min-heap，再觀察 sort_heap 的排序方向。
 *
 * 【LeetCode 測試策略】
 * 除一般案例外應測空陣列、單元素、重複值、已排序與反向排序。sort_heap 對空
 * heap 合法。不能只驗 is_sorted，也要驗結果仍是原輸入的 permutation。
 *
 * 【實務取捨】
 * sort_heap 的價值是「資料已經維持成 heap」時不必先重建；若只有普通 vector，
 * 直接 std::sort 可讀性更好。呼叫後若還要繼續 enqueue，先 make_heap 恢復契約。
 * 易錯陷阱：建 heap 用 greater<>、sort_heap 卻省略 comparator，前置條件不成立。
 * 面試回答排序方向時可用三元素手算，不要只靠「min/max heap」名稱猜測。
 * 實務若要求相同 key 保留原順序，sort_heap 不合適，應改 stable_sort。
 * LeetCode 大輸入還要評估遞迴限制；heap sort 本身可完全迭代完成。
 */
