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

// -----------------------------------------------------------------------------
// 【日常實務範例】固定支援型別的模板編譯邊界
// 情境：函式庫只承諾 int/double Accumulator 與 int Parser，要集中機器碼並避免任意型別連結失敗。
// 為何使用本章主題：唯一 implementation 區明確實體化 Accumulator<int/double>，Parser<int> 則提供
// 完整特化定義；相較全 header-only，可控制支援集合與重複實體化成本。
// 設計：主模板定義 add/total；列出 explicit instantiation；宣告 Parser 主模板並只定義 int 特化；測試邊界。
// 成本：Accumulator add/total 為 O(1)，parse 約 O(L)，L 是文字長度；主要取捨是 build time 與 binary size。
// 上線注意：header 需搭配 extern template 與特化宣告，且每個 definition 必須唯一並遵守 ODR/ABI 版本政策。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 35. Search Insert Position（搜尋插入位置）
// 題目：在升冪無重複陣列找 target，存在回索引，否則回應插入位置；[1,3,5,6] 查 2 得 1。
// 為何使用本章主題：函式模板定義在實體化點完整可見，編譯器才能為 vector<int> 產生程式碼；
// 演算法本身不依賴特殊編譯技巧，泛化後 T 只需提供一致的 `<` 排序。
// 思路：維護半開 [left,right)；middle 小於 target 時移左界；否則收右界，收斂點即 lower_bound。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 是 values 長度。
// 易錯點：輸入必須升冪；回傳 size_t 可表示尾端 N，不能用 -1 表示未找到。
// -----------------------------------------------------------------------------
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
