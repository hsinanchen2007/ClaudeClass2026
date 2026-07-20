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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 912. Sort an Array（排序陣列）
// 題目：輸入整數陣列 nums，回傳非遞減排序結果；例如 [5,2,3,1] 回 [1,2,3,5]。
// 為何使用本章主題：先以 make_heap 建 max-heap，再由 sort_heap 反覆把最大值放到
// 尾端，形成 heap sort；這是符合題意的教學解，未宣稱優於直接 std::sort。
// 思路：1. 對 nums 建 max-heap；2. 將整個 heap 原地轉成升冪序列；3. 回傳排序副本。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 為 nums 數量；heap 操作原地，空間來自按值輸入。
// 易錯點：sort_heap 的前置條件是整段已為同 comparator 的 heap；完成後不再維持 heap。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_sort_array(std::vector<int> nums) {
    std::make_heap(nums.begin(), nums.end());
    std::sort_heap(nums.begin(), nums.end());
    return nums;
}

struct Metric {
    int value;
    int original_order;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】批次分數堆的升冪輸出
// 情境：評分資料已進入以 vector 保存的批次流程，結束時要輸出完整升冪分數清單；
// 相同分數的原始順序不屬於契約。
// 為何使用本章主題：資料可先維持為 heap，再以 sort_heap 完成排序；若原本只是普通
// vector，直接 std::sort 通常更清楚，本例專門展示 heap 的收尾階段。
// 設計：1. 對 scores 建 max-heap；2. 呼叫 sort_heap 逐步固定尾端最大值；3. 回傳升冪結果。
// 成本：時間 O(N log N)、額外空間 O(N)，N 為分數數量；按值參數保留呼叫端資料。
// 上線注意：heap sort 不穩定；若同分需保留先後順序應改 stable_sort，若排序後還要 enqueue 則須重建 heap。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'sort_heap.cpp' -o '/tmp/codex_cpp_C_Algorithm_heap_sort_heap' && '/tmp/codex_cpp_C_Algorithm_heap_sort_heap'
//
// === 預期輸出（節錄）===
// sort_heap：heap sort 與批次輸出測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
