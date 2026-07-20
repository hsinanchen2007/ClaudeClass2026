/*
 * 第 25 章：模板編譯模型
 *
 * 一般函式可在 .cpp 定義後由 linker 找實作；模板必須先針對 T 產生具體函式，故實體化點
 * 通常需要看見完整定義。最常見做法是把模板定義放 header。另一做法是在 .cpp 明確實體化
 * 限定型別，再用 `extern template` 抑制其他 translation unit 重複實體化。
 *
 * 本檔是單一 translation unit，以下以 namespace 分區模擬 header 與 implementation。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace header_style {

template <typename T>
T clamp_value(T value, T low, T high) {
    return value < low ? low : (high < value ? high : value);
}

} // namespace header_style

namespace explicit_instantiation_style {

template <typename T>
class Accumulator {
public:
    void add(T value) { total_ += value; }
    T total() const { return total_; }

private:
    T total_{};
};

// 真實專案可把下列 explicit instantiation definition 放在唯一的 .cpp：
template class Accumulator<int>;
template class Accumulator<double>;

} // namespace explicit_instantiation_style

// LeetCode 35：Search Insert Position。模板定義完整可見，vector<int> 才能在此實體化。
template <typename T>
std::size_t leetcode_search_insert(const std::vector<T>& values, const T& target) {
    std::size_t left = 0;
    std::size_t right = values.size();
    while (left < right) {
        const std::size_t mid = left + (right - left) / 2;
        if (values[mid] < target) {
            left = mid + 1U;
        } else {
            right = mid;
        }
    }
    return left;
}

template <typename T>
struct Parser {
    static T parse(const std::string& text);
};

// 完整特化不再是待實體化的一般模板，可像普通函式一樣提供明確定義。
template <>
int Parser<int>::parse(const std::string& text) {
    return std::stoi(text);
}

// 【實務情境】固定支援型別以 explicit instantiation 集中產生機器碼。
void practical_compilation_boundary_test() {
    explicit_instantiation_style::Accumulator<int> requests;
    requests.add(10);
    requests.add(7);
    assert(requests.total() == 17);
    assert(Parser<int>::parse("42") == 42);
}

int main() {
    assert(header_style::clamp_value(15, 0, 10) == 10);
    assert(header_style::clamp_value(std::string{"m"}, std::string{"a"}, std::string{"z"}) == "m");

    const std::vector<int> values{1, 3, 5, 6};
    assert(leetcode_search_insert(values, 5) == 2U);
    assert(leetcode_search_insert(values, 2) == 1U);
    assert(leetcode_search_insert(values, 7) == 4U);

    practical_compilation_boundary_test();
    std::cout << "模板編譯模型測試完成\n";
}

/*
 * 【ODR】相同模板定義可出現在多個 translation unit，但必須 token-equivalent 並遵守 ODR。
 * 【陷阱】只把 template 宣告放 header、定義藏 .cpp，任意新 T 常會得到 linker undefined reference。
 * 【取捨】header-only 易用但增加編譯時間；explicit instantiation 降成本但只支援預先列出的型別。
 * 【面試】何時發生實體化？需要完整定義或 code generation 的使用點；部分語境只需宣告。
 * 【練習】拆成 accumulator.hpp/.cpp/main.cpp，使用 extern template class Accumulator<int> 驗證。
 */

/*
 * 【教科書補充：單一 translation unit 看不到的 ODR 邊界】
 * - 完整特化宣告必須在任何可能觸發 implicit instantiation 的使用點之前可見。
 * - extern template declaration 通常放 header，唯一 explicit-instantiation definition 放某個 .cpp。
 * - header 中定義非模板 specialization 仍要處理 inline/ODR，不能因它源自模板主題就自動多重定義安全。
 * - 本檔是單 TU 模型，只能解釋語法；真正 linker 行為需拆成至少 header + 兩個 source 驗證。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '25_template_compilation_model.cpp' -o '/tmp/codex_cpp_C_Template_25_template_compilation_model' && '/tmp/codex_cpp_C_Template_25_template_compilation_model'
//
// === 預期輸出（節錄）===
// 模板編譯模型測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
