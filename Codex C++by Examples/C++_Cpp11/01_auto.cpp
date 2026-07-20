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
// LeetCode 1：Two Sum。unordered_map 的 key/value 型別由容器清楚表達，
// 迴圈中的 auto 只消除冗長型別。平均 O(n)，最壞情況 O(n^2)，空間 O(n)。
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

// 【實務案例】監控樣本平均值：以 const auto& 走訪，避免逐筆複製含 string 的 Measurement。
namespace practical {
struct Measurement {
    std::string sensor;
    double value;
};

// 實務：監控資料通常不希望逐筆複製含 string 的結構，因此用 const auto&。
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
