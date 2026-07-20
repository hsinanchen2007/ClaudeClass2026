/*
 * std::minmax：一次取得兩值或列表的最小與最大
 * ==============================================
 * 兩參數版回 pair<const T&, const T&>，只比較一次；等價時 first 指第一參數，
 * second 指第二參數。initializer_list 版回 pair<T,T>，比較約 3N/2 次，通常比
 * 各跑一次 min/max 少比較。
 *
 * 兩參數版有 temporary reference 生命週期風險：即使寫 `auto [lo, hi]`，pair 的元素
 * 型別仍是 reference，並不會自動變成值。若引數是 temporary，應改用回傳值的
 * initializer_list overload、先建立具名物件，或明確複製結果。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1491. Average Salary Excluding the Minimum and Maximum Salary（去掉最低與最高薪資後的平均值）
// 題目：輸入至少三筆互異 salary，排除一筆最低與一筆最高後求平均；例如
// [4000,3000,1000,2000] 回 2500。
// 為何使用本章主題：逐筆以 minmax 更新目前上下界，同時累加總額；本寫法展示兩值
// minmax 的 reference pair 需立即複製，無需先完整排序。
// 思路：1. 驗證至少三筆；2. 線性累加 total 並更新 low/high；3. 扣除兩極值；4. 除以 N-2。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為薪資筆數。
// 易錯點：只排除各一筆極值；總和使用 long long；兩值 minmax 回 reference，不可回傳區域參考。
// -----------------------------------------------------------------------------
double leetcode_average_salary(const std::vector<int>& salary) {
    if (salary.size() < 3U) {
        throw std::invalid_argument("salary needs at least three entries");
    }
    long long total = 0;
    int low = salary.front();
    int high = salary.front();
    for (int value : salary) {
        total += value;
        const auto bounds = std::minmax(low, value);
        low = bounds.first;
        const auto upper = std::minmax(high, value);
        high = upper.second;
    }
    return static_cast<double>(total - low - high) /
           static_cast<double>(salary.size() - 2U);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】不定順序區間端點正規化
// 情境：UI 拖曳或外部 API 可能以任意順序傳入兩個端點，後端要統一成 [low,high]。
// 為何使用本章主題：std::minmax 一次比較即可取得兩端，比先判斷再 swap 更直接；
// 函式立即複製成 value pair，避免 reference pair 逸出。
// 設計：1. 對 endpoint_a 與 endpoint_b 求 minmax；2. 將 bounds.first/second 複製回傳。
// 成本：時間 O(1)、額外空間 O(1)，只需一次比較。
// 上線注意：若端點有非法 sentinel 或單位不同，必須先驗證；不可直接回 std::minmax(local...) 的參考 pair。
// -----------------------------------------------------------------------------
std::pair<int, int> practical_normalize_interval(int endpoint_a,
                                                 int endpoint_b) {
    const auto bounds = std::minmax(endpoint_a, endpoint_b);
    return {bounds.first, bounds.second};
}

int main() {
    const auto bounds = std::minmax({8, 1, 6, 3});
    assert(bounds.first == 1 && bounds.second == 8);

    // initializer_list overload 回傳 pair<int, int>，因此 temporary 不會留下懸空 reference。
    const auto [temporary_low, temporary_high] = std::minmax({1, 2});
    assert(temporary_low == 1 && temporary_high == 2);
    assert((practical_normalize_interval(20, 5) == std::pair{5, 20}));
    assert(leetcode_average_salary({4000, 3000, 1000, 2000}) == 2500.0);
    assert(leetcode_average_salary({1000, 2000, 3000}) == 2000.0);

    std::cout << "minmax：邊界、LC1491、區間正規化測試通過\n";
}

/*
 * 注意：本例已用 long long 加總；若輸入域可能超過 long long，仍須再定義溢位政策。
 * 面試：minmax_element 的 tie 規則與 min/max_element 不完全相同，請看下一章。
 * 練習：以 minmax 正規化矩形的 x/y 端點，再判斷點是否位於矩形內。
 *
 * 【LeetCode 數值安全】
 * salary 加總若 N 或單值很大應使用 long long。浮點比較測試一般用 tolerance；
 * 本例結果恰為可精確表示的整數值，才直接用 ==。
 *
 * 【實務生命週期】
 * normalize_interval 將 reference pair 立即複製成 value pair，因此參數離開函式後
 * 結果仍有效。不要回傳 std::minmax(local_a,local_b) 的 reference pair。
 *
 * 易錯陷阱：`const auto [lo,hi]=std::minmax(1,2)` 仍解構一個元素型別為 reference 的
 * pair；`auto` 不會替 pair 內的 reference 做 deep copy。本例改用 `std::minmax({1,2})`
 * 的 initializer_list overload，它回傳真正的 value pair。
 * 面試時可說 minmax 對兩值只需一次比較，對列表採成對比較減少總比較數。
 * LeetCode 平均薪資若有重複 min/max，題目仍只排除各一筆，不能 remove 所有同值。
 * 練習：用 long long 重寫總和，並以 tolerance 測試非整數平均。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'minmax.cpp' -o '/tmp/codex_cpp_C_Algorithm_min_max_minmax' && '/tmp/codex_cpp_C_Algorithm_min_max_minmax'
//
// === 預期輸出（節錄）===
// minmax：邊界、LC1491、區間正規化測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
