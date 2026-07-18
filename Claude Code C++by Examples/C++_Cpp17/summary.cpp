/*
================================================================================
【C++_Cpp17/summary.cpp】

本目錄主題：C++17 常用語法（現代 C++ 的「成熟」版本）

常見重點（對應本目錄檔案命名）：
  - structured bindings（結構化繫結）
  - if/switch initializer（if (auto x = ...; cond)）
  - std::optional / std::variant / std::any
  - std::string_view
  - std::filesystem（本 repo 另有 Filesystem 目錄）
  - CTAD（class template argument deduction）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯（核心特性不做條件編譯）

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Cpp17/C++_Cpp17 summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Cpp17/C++_Cpp17 summary 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Cpp17/C++_Cpp17 summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <iostream>
#include <string>
#include <utility>

// C++17 才有：optional / variant / any / string_view
#if __cplusplus >= 201703L
#include <any>
#include <optional>
#include <string_view>
#include <variant>
#endif

// -----------------------------------------------------------------------------
// 【重點 1】structured bindings：把 tuple/pair/struct 拆成多個名字
// -----------------------------------------------------------------------------
static void demo_structured_bindings() {
    std::cout << "\n[demo_structured_bindings]\n";

    std::pair<int, std::string> p{1, "one"};
#if defined(__cpp_structured_bindings) && (__cpp_structured_bindings >= 201606L)
    auto [id, name] = p; // 複製出來
    std::cout << "  id=" << id << ", name=" << name << "\n";

    std::tuple<int, int, int> t{1, 2, 3};
    auto [a, b, c] = t;
    std::cout << "  tuple: " << a << "," << b << "," << c << "\n";
#else
    std::cout << "  (structured bindings 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 2】if initializer：把「查找」與「判斷」寫在同一行
// -----------------------------------------------------------------------------
static void demo_if_initializer() {
    std::cout << "\n[demo_if_initializer]\n";

#if __cplusplus >= 201703L
    std::vector<int> v{1, 2, 3};
    if (auto it = std::find(v.begin(), v.end(), 2); it != v.end()) {
        std::cout << "  found 2 at index=" << (it - v.begin()) << "\n";
    }
#else
    std::cout << "  (if initializer 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 3】std::optional：有/沒有值
// -----------------------------------------------------------------------------
#if __cplusplus >= 201703L
static std::optional<int> maybe_parse_int(std::string_view s) {
    try {
        return std::stoi(std::string(s));
    } catch (...) {
        return std::nullopt;
    }
}
#endif

static void demo_optional() {
    std::cout << "\n[demo_optional]\n";
#if __cplusplus >= 201703L
    for (std::string_view s : {"42", "xx"}) {
        if (auto v = maybe_parse_int(s)) {
            std::cout << "  " << s << " -> " << *v << "\n";
        } else {
            std::cout << "  " << s << " -> (no value)\n";
        }
    }
#else
    std::cout << "  (std::optional 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 4】std::variant + std::visit：型別安全 union
// -----------------------------------------------------------------------------
#if __cplusplus >= 201703L
template <class... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};
template <class... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;
#endif

static void demo_variant() {
    std::cout << "\n[demo_variant]\n";
#if __cplusplus >= 201703L
    using V = std::variant<int, double, std::string>;
    std::vector<V> xs = {1, 2.5, std::string("hi")};

    for (const auto& v : xs) {
        std::visit(Overloaded{
            [](int x) { std::cout << "  int: " << x << "\n"; },
            [](double d) { std::cout << "  double: " << d << "\n"; },
            [](const std::string& s) { std::cout << "  string: " << s << "\n"; },
        }, v);
    }
#else
    std::cout << "  (std::variant/std::visit 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 5】std::any：型別擦除（能裝任何型別，但取出要 any_cast）
// -----------------------------------------------------------------------------
static void demo_any() {
    std::cout << "\n[demo_any]\n";
#if __cplusplus >= 201703L
    std::any a = 123;
    std::cout << "  int=" << std::any_cast<int>(a) << "\n";

    a = std::string("hello");
    if (auto p = std::any_cast<std::string>(&a)) {
        std::cout << "  string=" << *p << "\n";
    }
#else
    std::cout << "  (std::any 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 6】std::string_view：不擁有的字串視圖（避免不必要拷貝）
// -----------------------------------------------------------------------------
#if __cplusplus >= 201703L
static size_t count_a(std::string_view s) {
    size_t cnt = 0;
    for (char c : s) if (c == 'a') ++cnt;
    return cnt;
}
#endif

static void demo_string_view() {
    std::cout << "\n[demo_string_view]\n";
#if __cplusplus >= 201703L
    std::string s = "abracadabra";
    std::string_view sv = s; // view 不拷貝
    std::cout << "  count_a=" << count_a(sv) << "\n";

    // ★ 注意：string_view 不能比原字串活得更久（否則變成懸空引用）
#else
    std::cout << "  (std::string_view 需要 C++17)\n";
#endif
}

int main() {
    demo_structured_bindings();
    demo_if_initializer();
    demo_optional();
    demo_variant();
    demo_any();
    demo_string_view();

    std::cout << "\n[done]\n";
    return 0;
}

