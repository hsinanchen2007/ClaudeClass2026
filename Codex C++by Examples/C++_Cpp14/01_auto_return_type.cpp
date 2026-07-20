/*
 * C++14 教科書：function auto return type deduction
 *
 * C++14 允許普通函式與 function template 只寫 auto，compiler 由 return expression 推導。
 * 所有可到達的 return 必須推導成同一型別；auto 採傳值推導，會去掉 top-level const/ref。
 * 若要精確保留 reference，使用同版新增的 decltype(auto)，但必須審查生命週期。
 *
 * 【何時使用】回傳 iterator、closure、複雜模板運算結果，且型別由實作顯然可知。
 * 公開 API 若回傳型別本身是契約，明寫型別通常更清楚，也能避免實作改動悄悄改 ABI。
 * 【遞迴】auto-return 遞迴函式必須先有可供推導的 return，否則遞迴 call 時尚未知型別。
 * 【面試題】`const int& f(); auto g(){return f();}` 的 g 回傳什麼？int（複製）。
 * 【練習】讓 practical_average 同時支援 vector<float> 且避免整數除法。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
auto twice(int value) { return value * 2; }

template <class Left, class Right>
auto plus_value(Left left, Right right) {
    return left + right;
}

void demo() {
    static_assert(std::is_same<decltype(twice(2)), int>::value,
                  "twice 應推導為 int");
    static_assert(std::is_same<decltype(plus_value(2, 0.5)), double>::value,
                  "混合運算應依 operator+ 推導 double");
    assert(twice(6) == 12);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列動態和）
// 題目：回傳每個位置以前的元素總和；例如 [1,2,3,4] 轉成 [1,3,6,10]。
// 為何使用本章主題：函式的 auto 由唯一的 vector<int> return expression 推導回傳型別，避免在簽章重複實作型別。
// 思路：1. 預留 nums 大小的容量；2. 逐項累加到 total；3. 將每一步的 total 放入 result。
// 複雜度：N 為元素數；時間 O(N)，輸出空間 O(N)、除輸出外額外空間 O(1)。
// 易錯點：多個 return 必須推導成同一型別；total 使用 int，輸入總和超出範圍時仍會溢位。
// -----------------------------------------------------------------------------
auto leetcode_running_sum(const std::vector<int>& nums) {
    std::vector<int> result;
    result.reserve(nums.size());
    int total = 0;
    for (const int value : nums) {
        total += value;
        result.push_back(total);
    }
    return result;
}

void leetcode_test() {
    assert(leetcode_running_sum({1, 2, 3, 4}) ==
           std::vector<int>({1, 3, 6, 10}));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】監控樣本平均值計算
// 情境：收集一批帶 name 與 double value 的監控樣本，產生單一平均值並拒絕空批次。
// 為何使用本章主題：auto 由最後的 double 除法推導回傳型別，讓私有 helper 可隨實作自然決定結果型別。
// 設計：1. 先拒絕空 vector；2. 以 0.0 和 lambda 累加每筆 value；3. 除以樣本數。
// 成本：N 為樣本數；時間 O(N)、額外空間 O(1)。
// 上線注意：公開 ABI 若需固定型別應明寫 double；還要定義 NaN、無限值與浮點累加誤差政策。
// -----------------------------------------------------------------------------
struct Sample {
    std::string name;
    double value;
};

auto practical_average(const std::vector<Sample>& samples) {
    if (samples.empty()) {
        throw std::invalid_argument("samples 不可為空");
    }
    const double total = std::accumulate(
        samples.begin(), samples.end(), 0.0,
        [](double sum, const Sample& sample) { return sum + sample.value; });
    return total / static_cast<double>(samples.size());
}

void practical_test() {
    const std::vector<Sample> samples{{"cpu", 40.0}, {"gpu", 80.0}};
    assert(practical_average(samples) == 60.0);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "C++14 auto return：running sum 與監控平均值測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_auto_return_type.cpp' -o '/tmp/codex_cpp_C_Cpp14_01_auto_return_type' && '/tmp/codex_cpp_C_Cpp14_01_auto_return_type'
//
// === 預期輸出（節錄）===
// C++14 auto return：running sum 與監控平均值測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
