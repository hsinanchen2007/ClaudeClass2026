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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++17 總覽
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++17 主要新增了哪些語言與函式庫特性？
//     答：語言層：structured bindings、if constexpr、if/switch with init、
//         inline variables、CTAD、fold expressions、auto 非型別模板參數、
//         巢狀命名空間 namespace a::b::c、[[nodiscard]]/[[maybe_unused]]/
//         [[fallthrough]]、constexpr lambda、保證的 copy elision。
//         函式庫層：string_view、optional、variant、any、invoke/apply、byte、
//         filesystem、平行演算法（std::execution）、shared_mutex。
//     追問：哪些是純語言特性、不需 include？（structured bindings、if constexpr、
//           CTAD、fold expression；string_view/optional/variant/any/invoke 都要標頭）
//
// 🔥 Q2. fold expression 是什麼？四種形式？
//     答：對 parameter pack 以二元運算子摺疊，取代 C++11 的遞迴模板寫法。
//         一元右摺 (E op ...)、一元左摺 (... op E)、
//         二元右摺 (E op ... op init)、二元左摺 (init op ... op E)。
//         例如 (args + ...) 就是求和。
//     追問：空 pack 會怎樣？（一元摺疊對空 pack 只有 &&（true）、||（false）、
//           逗號（void()）三個運算子有定義，其餘是編譯錯誤 → 改用二元摺疊給初值）
//
// Q3. C++17 對 lambda 做了哪兩個增強？
//     答：① constexpr lambda：滿足 constexpr 條件時 operator() 隱式為 constexpr，
//            可在編譯期求值，也可顯式寫 [](int x) constexpr {...}。
//         ② [*this] 捕獲：以值複製整個 *this 到閉包，解決非同步／回呼中 this 懸垂。
//         （注意 generic lambda 是 C++14 的，別記到 C++17 頭上）
//
// ⚠️ 陷阱. C++17 的「保證 copy elision」保證了什麼？NRVO 也在內嗎？
//     答：以 prvalue 初始化同型別物件時（T t = T(); 或回傳 prvalue），標準保證不產生
//         臨時物件——這是語意規定而非最佳化許可，所以即使複製與移動建構子都被
//         delete，這種初始化仍然合法，工廠函式因此可以回傳不可移動的型別。
//     為什麼會錯：多數人把它擴大解讀成「C++17 起省略複製都被保證」，但 NRVO
//         （回傳具名區域變數）仍然只是可選的最佳化，標準並未保證。
//
// Q. C++17 為什麼要再加一組 to_chars / from_chars？stringstream 不夠嗎？
//     答：<charconv> 的賣點是「最快、且不受 locale 影響」：
//         ① 不看 locale——stringstream 與 strtod 會受目前 locale 影響（例如德文
//            locale 下小數點是逗號），在解析 JSON／設定檔時是災難；from_chars
//            永遠用 C locale 規則。
//         ② 不配置記憶體、不拋例外——寫進呼叫者給的 buffer，錯誤透過回傳的
//            to_chars_result{ptr, ec} 表達；緩衝區不足時 ec 是
//            std::errc::value_too_large（本機實測確認），不會拋例外。
//         ③ from_chars 回傳的 ptr 指向「解析停止的位置」，本機實測 "42abc" 解析出
//            42 並停在 "abc"，可用來做增量解析。
//     追問：浮點數版本有什麼要注意的？（介面是 C++17 的，但各家標準庫實作進度落後
//         很多——libstdc++ 要到 GCC 11 才完整支援浮點 from_chars/to_chars，寫可攜
//         程式碼前要先確認工具鏈版本。to_chars 的浮點輸出預設是「能 round-trip 回
//         原值的最短表示」，也可指定 chars_format::fixed/scientific/hex 與精度）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>

// C++17 才有：optional / variant / any / string_view
#if __cplusplus >= 201703L
#include <any>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
#include <algorithm>
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

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
//
// [demo_structured_bindings]
//   id=1, name=one
//   tuple: 1,2,3
//
// [demo_if_initializer]
//   found 2 at index=1
//
// [demo_optional]
//   42 -> 42
//   xx -> (no value)
//
// [demo_variant]
//   int: 1
//   double: 2.5
//   string: hi
//
// [demo_any]
//   int=123
//   string=hello
//
// [demo_string_view]
//   count_a=5
//
// [done]
