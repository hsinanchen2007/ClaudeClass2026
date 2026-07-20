/*
 * std::transform：把每個輸入映射成輸出（unary / binary）
 * =====================================================
 * unary: transform(first,last,out,f)；binary: 再給 second_first 與 f(a,b)。
 * 時間 O(N)，不改容器 size。目的端需空間或 inserter；輸出可與第一輸入完全相同
 * 以原地轉換，但危險的部分重疊不可用。
 *
 * 不應依賴 transform 的 callable 呼叫順序或副作用來做 prefix sum；尤其 execution
 * policy 版本可平行/向量化。transform 適合彼此獨立的 element-wise mapping。
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 977. Squares of a Sorted Array（有序陣列的平方）
// 題目：輸入非遞減 nums，回傳每個值平方後的非遞減陣列；例如
// [-4,-1,0,3,10] 回 [0,1,9,16,100]。
// 為何使用本章主題：transform 獨立計算每項平方，再 sort 修復負數平方造成的順序；
// 這是 O(N log N) 教學版，正式最佳解應用雙指標做到 O(N)。
// 思路：1. 建立等長輸出；2. transform 每值為 value*value；3. 完整排序平方結果。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 為 nums 的元素數。
// 易錯點：平方可能 int 溢位；不能假設原升冪順序經平方後仍升冪。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_sorted_squares(const std::vector<int>& nums) {
    std::vector<int> result(nums.size());
    std::transform(nums.begin(), nums.end(), result.begin(),
                   [](int value) { return value * value; });
    std::sort(result.begin(), result.end());
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】批次含稅顯示價格換算
// 情境：輸入以 cents 表示的未稅價格及 tax_rate，要逐項乘上稅率並四捨五入成整數
// cents，來源價目表保持不變。
// 為何使用本章主題：transform 對每筆價格執行同一個無相依 mapping，目的大小已知，
// 比帶外部索引的手寫迴圈更直接。
// 設計：1. 建立等長 result；2. 將 cents 升為 double 後乘 1+tax_rate；3. lround 後轉回 int。
// 成本：時間 O(N)、額外空間 O(N)，N 為價格筆數。
// 上線注意：權威會計不可依賴 binary floating point；需整數比例/decimal、明確 rounding 與溢位檢查。
// -----------------------------------------------------------------------------
std::vector<int> practical_prices_with_tax(const std::vector<int>& cents,
                                           double tax_rate) {
    std::vector<int> result(cents.size());
    std::transform(cents.begin(), cents.end(), result.begin(),
                   [tax_rate](int price) {
                       return static_cast<int>(std::lround(
                           static_cast<double>(price) * (1.0 + tax_rate)));
                   });
    return result;
}

int main() {
    std::vector<int> values{1, 2, 3};
    std::transform(values.begin(), values.end(), values.begin(),
                   [](int value) { return value * 10; });
    assert((values == std::vector<int>{10, 20, 30}));

    const std::vector<int> left{1, 2, 3};
    const std::vector<int> right{4, 5, 6};
    std::vector<int> sums(3);
    std::transform(left.begin(), left.end(), right.begin(), sums.begin(),
                   std::plus<>{});
    assert((sums == std::vector<int>{5, 7, 9}));

    assert((leetcode_sorted_squares({-4, -1, 0, 3, 10}) ==
            std::vector<int>{0, 1, 9, 16, 100}));
    assert((practical_prices_with_tax({100, 250}, 0.10) ==
            std::vector<int>{110, 275}));
    std::cout << "transform：LeetCode 977 與實務含稅價格測試通過\n";
}

/*
 * 易錯陷阱：binary transform classic overload 不知道第二範圍長度，呼叫者要保證
 * 至少 N 格。平方可能 int overflow；LeetCode 約束若放寬就升 long long。
 *
 * 面試：transform vs for loop？語意明確、可組 execution policy，但複雜控制流用 loop
 * 更可讀。實務金額不應以 binary floating point 作權威會計；本例僅示範 mapping，
 * 正式系統採整數比例/decimal 與明確 rounding policy。
 * 練習：用雙指標把 LC977 最佳化為 O(N)，保留相同測試。
 * LeetCode 最佳解要從絕對值較大的兩端反向填入輸出，避免額外 sort。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'transform.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_transform' && '/tmp/codex_cpp_C_Algorithm_modifying_transform'
//
// === 預期輸出（節錄）===
// transform：LeetCode 977 與實務含稅價格測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
