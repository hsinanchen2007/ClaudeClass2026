/*
================================================================================
【C++_Template/summary.cpp】

本目錄主題：Templates（泛型程式設計）

你應該掌握的主線：
  - function template：把「型別」變成參數
  - class template：把「型別」參數化做出泛型容器/工具
  - type deduction：模板推導的規則（與 auto 類似）
  - specialization：特化（必要時才用）
  - SFINAE / enable_if：用條件限制模板（C++20 concepts 更好，但此處以 C++17 為主）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯；C++20 concepts 僅提示

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Template/C++_Template summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Template/C++_Template summary 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Template/C++_Template summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// -----------------------------------------------------------------------------
// 【重點 1】function template：最常見的泛型函式
// -----------------------------------------------------------------------------
template <class T>
T my_max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

static void demo_function_template() {
    std::cout << "\n[demo_function_template]\n";
    std::cout << "  my_max(3,5)=" << my_max(3, 5) << "\n";
    std::cout << "  my_max(string)=" << my_max(std::string("a"), std::string("b")) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】class template：泛型型別
// -----------------------------------------------------------------------------
template <class T>
class Box {
public:
    explicit Box(T v) : v_(std::move(v)) {}
    const T& get() const { return v_; }
    void set(T v) { v_ = std::move(v); }
private:
    T v_;
};

static void demo_class_template() {
    std::cout << "\n[demo_class_template]\n";
    Box<int> b(42);
    std::cout << "  Box<int>=" << b.get() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】template specialization：必要時才用（通常代表你在處理例外規則）
// -----------------------------------------------------------------------------
template <class T>
struct TypeName { static const char* name() { return "unknown"; } };

template <>
struct TypeName<int> { static const char* name() { return "int"; } };

template <>
struct TypeName<std::string> { static const char* name() { return "std::string"; } };

static void demo_specialization() {
    std::cout << "\n[demo_specialization]\n";
    std::cout << "  TypeName<int>=" << TypeName<int>::name() << "\n";
    std::cout << "  TypeName<double>=" << TypeName<double>::name() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】SFINAE / enable_if：限制模板只在某條件成立時參與多載決議
// -----------------------------------------------------------------------------
// 範例：只允許整數型別呼叫
template <class T, class = std::enable_if_t<std::is_integral<T>::value>>
T add_one(T x) {
    return x + 1;
}

static void demo_enable_if() {
    std::cout << "\n[demo_enable_if]\n";
    std::cout << "  add_one(7)=" << add_one(7) << "\n";
    // add_one(3.14); // ❌ 不符合 is_integral
}

static void demo_cpp20_concepts_note() {
    std::cout << "\n[demo_cpp20_concepts_note]\n";
#if __cplusplus >= 202002L
    std::cout << "  C++20: concepts 可以用 requires/Concept 取代 enable_if，錯誤訊息更友善。\n";
#else
    std::cout << "  (C++17：用 enable_if/SFINAE 做限制；錯誤訊息較不友善)\n";
#endif
}

int main() {
    demo_function_template();
    demo_class_template();
    demo_specialization();
    demo_enable_if();
    demo_cpp20_concepts_note();

    std::cout << "\n[done]\n";
    return 0;
}

