/*
 * std::minmax_element：單趟取得最小/最大元素 iterator
 * ==================================================
 * 回 pair{min_iterator,max_iterator}。空範圍兩者皆 end。比較次數約 3N/2，少於
 * 分別掃兩次。tie 規則要記：最小值回第一個，最大值回最後一個等價元素。
 * 回傳 iterator 的生命與容器綁定，修改 vector 後需重新取得。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 908. Smallest Range I（最小差值 I）
// 題目：每個 nums[i] 可各自加上 [-k,k] 內的值，求可能的最小最大差；例如
// [0,10]、k=2 回 6。
// 為何使用本章主題：答案只依原始最小與最大值，minmax_element 可單趟取得兩端；
// 可縮短最多 2k，且結果不能小於 0。
// 思路：1. 找 low/high iterator；2. 計算原範圍 high-low；3. 扣除 2k；4. 與 0 取 max。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：輸入不可空；high-low-2*k 若約束放寬可能 int 溢位；答案需截在 0。
// -----------------------------------------------------------------------------
int leetcode_smallest_range(const std::vector<int>& nums, int k) {
    assert(!nums.empty());
    const auto [low, high] = std::minmax_element(nums.begin(), nums.end());
    return std::max(0, *high - *low - 2 * k);
}

struct Reading {
    int minute;
    double temperature;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】溫度序列極值發生時刻摘要
// 情境：監控批次含 minute 與 temperature，要單趟回最低溫與最高溫各自發生的分鐘。
// 為何使用本章主題：minmax_element 約以 3N/2 次比較同時找兩極，少於各掃一次；
// 回傳 iterator 可直接取得關聯的 minute 欄位。
// 設計：1. 以 temperature comparator 掃描 readings；2. 取得第一個最低與最後一個
// 最高 iterator；3. 回傳兩者 minute。
// 成本：時間 O(N)、額外空間 O(1)，N 為讀值筆數。
// 上線注意：輸入不可空且 temperature 應先拒絕 NaN；相同最高值會取最後一筆，需符合告警契約。
// -----------------------------------------------------------------------------
std::pair<int, int> practical_temperature_extreme_minutes(
    const std::vector<Reading>& readings) {
    assert(!readings.empty());
    const auto [low, high] = std::minmax_element(
        readings.begin(), readings.end(),
        [](const Reading& lhs, const Reading& rhs) {
            return lhs.temperature < rhs.temperature;
        });
    return {low->minute, high->minute};
}

int main() {
    const std::vector<int> values{4, 1, 9, 1, 9};
    const auto [low, high] = std::minmax_element(values.begin(), values.end());
    assert(low == values.begin() + 1);   // 第一個最小值
    assert(high == values.begin() + 4);  // 最後一個最大值

    assert(leetcode_smallest_range({1, 3, 6}, 3) == 0);
    assert(leetcode_smallest_range({0, 10}, 2) == 6);
    assert((practical_temperature_extreme_minutes(
                {{0, 20.5}, {5, 18.0}, {10, 25.0}}) ==
            std::pair{5, 10}));

    std::cout << "minmax_element：單趟雙極值與 LC908 測試通過\n";
}

/*
 * 面試：為何最大值回最後一個？這是標準指定的 tie 行為；若業務要最早最高點，
 * 可分別用 min_element/max_element，或自訂迴圈明確 tie 規則。
 * 練習：回傳極差與兩個 index，並處理空輸入為 optional。
 *
 * 【LeetCode 推導】
 * 所有數都可向彼此靠近最多 k，原始 range=max-min 可縮短最多 2k；結果不可能
 * 小於 0，所以取 max(0,range-2k)。只需極值，不需排序。
 *
 * 【實務資料品質】
 * 溫度 NaN 會破壞 comparator；先驗證 finite。相同高溫回最後一筆可能符合「最新
 * 警報」需求，但若要最早發生點，tie 契約必須改寫並加測試。
 * 易錯陷阱：空範圍回 end/end；先檢查再解參考。面試需能說出約 3N/2 比較。
 * LeetCode 計算 2*k 與 range 時，輸入若大應升 long long 防 signed overflow。
 * 練習：以 optional 回傳空讀值，並明確決定相同極值的時間 tie 規則。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'minmax_element.cpp' -o '/tmp/codex_cpp_C_Algorithm_min_max_minmax_element' && '/tmp/codex_cpp_C_Algorithm_min_max_minmax_element'
//
// === 預期輸出（節錄）===
// minmax_element：單趟雙極值與 LC908 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
