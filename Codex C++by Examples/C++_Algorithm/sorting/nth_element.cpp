/*
 * std::nth_element：只把第 n 個元素放到完整排序後應在的位置
 * ============================================================
 * 完成後 *nth 正確，且 [first,nth) 沒有任何元素大於 [nth,last)（依 comparator）；
 * 兩側內部都未排序。平均/標準複雜度為線性比較量級，額外空間通常小。
 * 需要 random-access iterator；nth 可等於 last，此時不應解參考。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode 215：Kth Largest Element；descending comparator 下第 k-1 個。
int leetcode_find_kth_largest(std::vector<int> nums, int k) {
    assert(k >= 1 && static_cast<std::size_t>(k) <= nums.size());
    const auto nth = nums.begin() + (k - 1);
    std::nth_element(nums.begin(), nth, nums.end(), std::greater<>{});
    return *nth;
}

// 實務：取得 latency p-quantile；函式接受 copy，避免破壞呼叫者觀測順序。
double practical_quantile(std::vector<double> samples, double quantile) {
    assert(!samples.empty());
    assert(quantile >= 0.0 && quantile <= 1.0);
    const std::size_t index = static_cast<std::size_t>(
        quantile * static_cast<double>(samples.size() - 1U));
    const auto nth = samples.begin() + static_cast<std::ptrdiff_t>(index);
    std::nth_element(samples.begin(), nth, samples.end());
    return *nth;
}

int main() {
    std::vector<int> values{7, 1, 9, 3, 5};
    const auto median = values.begin() + 2;
    std::nth_element(values.begin(), median, values.end());
    assert(*median == 5);
    assert(std::all_of(values.begin(), median,
                       [median](int value) { return value <= *median; }));
    assert(std::all_of(median + 1, values.end(),
                       [median](int value) { return value >= *median; }));

    assert(leetcode_find_kth_largest({3, 2, 1, 5, 6, 4}, 2) == 5);
    assert(practical_quantile({10.0, 30.0, 20.0, 40.0, 50.0}, 0.50) == 30.0);
    assert(practical_quantile({10.0, 20.0}, 1.0) == 20.0);

    std::cout << "nth_element：LC215、median 與 quantile 測試通過\n";
}

/*
 * 易錯陷阱：
 * - nth 左右不是排序完成；要 top-k 且前 k 也要有序，再 sort 前半或 partial_sort。
 * - percentile 定義很多（nearest-rank、插值等）；本實務例採 floor index，API 文件
 *   必須寫明，不可只叫 p95 卻不定義算法。
 * - 演算法原地重排；若原順序要保留就接受 copy 或另建 index vector。
 * - comparator 與 kth largest 的方向常寫反：greater 搭 k-1。
 *
 * 面試：quickselect 平均 O(N)，但工程標準函式的最壞保證與實作策略應依當版標準/
 * library 查證，不要背單一實作。輸入有 NaN 時浮點 `<` 不形成一般期待的排序，
 * 應先清洗或定義 total ordering。
 *
 * 邊界：nth==first 會選最小值；nth==last 合法但不可解參考，且沒有「第 N 個」元素。
 * duplicate 時第 K 值仍正確，但左右兩側可能含與 *nth 等價的元素，不能聲稱左側
 * 全部「嚴格小於」。
 *
 * 若同時需要多個 quantile，可重複 nth_element 但會多次掃描；大量 quantile 可考慮
 * 一次 sort，或使用 selection tree/approximate sketch，依 query 數與精度選擇。
 *
 * 練習：回傳偶數筆的統計 median（中間兩值平均），注意需找兩個 order statistic
 * 與避免整數相加溢位。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'nth_element.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_nth_element' && '/tmp/codex_cpp_C_Algorithm_sorting_nth_element'
//
// === 預期輸出（節錄）===
// nth_element：LC215、median 與 quantile 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
