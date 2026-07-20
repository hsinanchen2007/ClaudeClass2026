/*
 * C++11 教科書：auto 型別推導
 *
 * 【核心觀念】
 * 1. auto 讓編譯器由 initializer 推導型別；它不是動態型別，結果仍在編譯期固定。
 * 2. 單寫 auto 近似模板的「傳值推導」：會去掉 top-level const 與 reference。
 * 3. 要修改原物件用 auto&；只讀且避免複製用 const auto&；auto&& 可接受左值或右值，
 *    但是否應 perfect-forward 還要搭配模板與 std::forward。
 * 4. auto 最適合 iterator、lambda 回傳物件與明顯 initializer；若型別承載商業語意，
 *    寫出 Money、Seconds 等型別通常比 auto 更清楚。
 *
 * 【複雜度與生命週期】auto 不改變演算法複雜度，也不延長參考對象生命週期。
 * 【常見陷阱】auto x = vector<bool>[0] 可能得到 proxy；auto list{1,2} 是
 * initializer_list<int>。`auto one{1}` 原始 C++11 wording 與 N3922 defect report
 * 的歷史結果不同；現代編譯器通常也在 C++11 模式套用該 DR 而推導成 int，不能只靠
 * `-std=` 旗標推測舊編譯器行為。
 * 【面試題】說明 auto、auto&、const auto& 推導 const/reference 的差異。
 * 【練習】把 practical::average 的迴圈改成 index 寫法，比較可讀性與越界風險。
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    const int original = 7;
    auto copied = original;              // int：top-level const 被移除
    const auto& viewed = original;       // const int&：不複製且不可修改
    auto&& forwarded_lvalue = copied;    // int&：左值使 auto&& collapse 成左值參考

    static_assert(std::is_same<decltype(copied), int>::value,
                  "auto 傳值推導應移除 top-level const");
    static_assert(std::is_same<decltype(viewed), const int&>::value,
                  "const auto& 應保留 const reference");
    static_assert(std::is_same<decltype(forwarded_lvalue), int&>::value,
                  "左值初始化 auto&& 時應 reference collapse 成 T&");

    forwarded_lvalue = 9;
    assert(copied == 9 && viewed == 7);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：輸入整數陣列 nums 與 target，找出和為 target 的兩個索引；例如 [2,7,11,15] 與 9 回傳 (0,1)。
// 為何使用本章主題：auto 讓補數與 unordered_map iterator 由初始化式推導，省去冗長型別但不改變雜湊解法。
// 思路：1. 逐項計算 target-nums[i]；2. 在已看過的值中查補數；3. 命中就回索引，否則記錄目前值。
// 複雜度：N 為元素數；平均時間 O(N)、雜湊最壞 O(N^2)，額外空間 O(N)。
// 易錯點：必須先查再插入才能避免同一元素配自己；索引轉 int 前也要符合題目範圍契約。
// -----------------------------------------------------------------------------
std::pair<int, int> two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> index_by_value;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        const auto wanted = target - nums[i];
        const auto found = index_by_value.find(wanted);
        if (found != index_by_value.end()) {
            return {found->second, static_cast<int>(i)};
        }
        index_by_value.emplace(nums[i], static_cast<int>(i));
    }
    throw std::invalid_argument("不存在答案");
}

void test() {
    const auto answer = two_sum({2, 7, 11, 15}, 9);
    assert((answer == std::pair<int, int>{0, 1}));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】監控樣本平均值彙整
// 情境：一批感測資料同時帶 sensor 字串與數值，要計算本批平均值並拒絕空批次。
// 為何使用本章主題：const auto& 走訪可由容器元素推導型別，且不複製每筆 Measurement 內的 string。
// 設計：1. 先驗證 rows 非空；2. 以 const auto& 累加 value；3. 用筆數換成 double 計算平均。
// 成本：N 為樣本數；時間 O(N)、額外空間 O(1)，並避免 N 次結構與字串複製。
// 上線注意：要定義 NaN/無限值與總和溢位政策；rows 只在呼叫期間借用，不能保存元素參考。
// -----------------------------------------------------------------------------
struct Measurement {
    std::string sensor;
    double value;
};

double average(const std::vector<Measurement>& rows) {
    if (rows.empty()) {
        throw std::invalid_argument("資料不可為空");
    }
    auto total = 0.0;
    for (const auto& row : rows) {
        total += row.value;
    }
    return total / static_cast<double>(rows.size());
}

void test() {
    const std::vector<Measurement> rows{{"cpu", 20.0}, {"gpu", 40.0}};
    assert(average(rows) == 30.0);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "auto：基礎、Two Sum、監控平均值測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_auto.cpp' -o '/tmp/codex_cpp_C_Cpp11_01_auto' && '/tmp/codex_cpp_C_Cpp11_01_auto'
//
// === 預期輸出（節錄）===
// auto：基礎、Two Sum、監控平均值測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
