// =============================================================================
//  02_decltype.cpp  —  decltype 取得運算式型別 (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/decltype
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、decltype 是什麼？                                      │
//  └────────────────────────────────────────────────────────────┘
//
//  decltype(expr) 回傳「expr 的型別」 — 是個編譯期型別查詢工具，不會真的
//  evaluate expr。
//
//      int x = 0;
//      decltype(x) y = 1;          // y 是 int
//      decltype(x + 1.0) z = 2.0;  // z 是 double（運算式推導）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、decltype vs auto 的關鍵差別                           │
//  └────────────────────────────────────────────────────────────┘
//
//  auto 推導會「捨棄 reference / top-level const」（值語意）；
//  decltype 對「lvalue 運算式」會精確保留型別（含 reference 與 cv）。
//
//  例：
//      const int& cr = x;
//      auto a = cr;            // a 是 int（去掉 const、ref）
//      decltype(cr) b = cr;    // b 是 const int&（完全一樣）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、decltype 對「身份運算式」與「非身份運算式」不同        │
//  └────────────────────────────────────────────────────────────┘
//
//  「unparenthesized id-expression」(就是裸名字) → 直接用該變數的宣告型別
//  「其他 lvalue 運算式」 → 加上 reference
//
//      int x;
//      decltype(x)    a;        // int
//      decltype((x))  b = x;    // int& (因為 (x) 是 lvalue 運算式)
//      decltype(x+1) c;        // int (rvalue 運算式)
//
//  小技巧記憶：「裸名字 = 看宣告」「(x) = lvalue → ref」「rvalue = 值型別」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、最有用的場景                                           │
//  └────────────────────────────────────────────────────────────┘
//
//   1) 在 template 中表達「跟某參數同型別」
//        template <class T, class U>
//        auto add(T a, U b) -> decltype(a + b) { return a + b; }
//
//   2) 推導出複雜運算式的型別（不必手寫）
//        decltype(my_map.begin()->second) v;
//
//   3) decltype(auto) (C++14) — 同時要 auto 的便利、又要 decltype 的精確
//      （見 C++_Cpp14/02_decltype_auto.cpp）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：基本用法
//   * Demo 2：(x) 與 x 的差別
//   * Demo 3：trailing return 用 decltype 表達
// =============================================================================

/*
補充筆記：decltype
  - decltype 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - decltype(name) 取得宣告型別，decltype((expr)) 會依 value category 產生 reference，括號差異很重要。
  - decltype 常用在泛型程式中保留精確回傳型別，比 auto 更不會丟失 reference。
*/
#include <iostream>
#include <type_traits>
#include <vector>

// 用 trailing return type + decltype 表達「結果型別 = a + b」
template <class T, class U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本
    // ─────────────────────────────────────────────────────────
    int x = 10;
    const int cx = 20;
    const int& cr = x;

    decltype(x)  a = 1;            // int
    decltype(cx) b = 2;            // const int
    decltype(cr) c = x;            // const int&  (保留 ref + const)

    static_assert(std::is_same<decltype(a), int>::value, "");
    static_assert(std::is_same<decltype(b), const int>::value, "");
    static_assert(std::is_same<decltype(c), const int&>::value, "");
    std::cout << "[Demo1] static_assert all passed\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：x vs (x) — 後者多一個括號就會變 ref
    // ─────────────────────────────────────────────────────────
    int y = 5;
    decltype(y)   d = 0;         // int      （裸名字 → 看宣告）
    decltype((y)) e = y;         // int&     （(y) 是 lvalue 運算式 → 加 ref）

    static_assert(std::is_same<decltype(d), int>::value, "");
    static_assert(std::is_same<decltype(e), int&>::value, "");
    std::cout << "[Demo2] decltype(y) is int, decltype((y)) is int&\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：trailing return 中的 decltype
    // ─────────────────────────────────────────────────────────
    auto sum = add(1, 2.5);              // decltype(int + double) = double
    static_assert(std::is_same<decltype(sum), double>::value, "");
    std::cout << "[Demo3] add(1, 2.5) = " << sum
              << " (return type deduced as double)\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：decltype 會 evaluate 內部 expr 嗎？
    //    A：不會 — 是編譯期語意操作。decltype(throw_func()) 不會執行函式，
    //       只看回傳型別。
    //
    //  Q2：什麼時候 auto 不夠用、要 decltype？
    //    A：(1) 想精確保留 reference 與 cv（auto 會丟掉）
    //       (2) template return type 必須在 「函式簽名」級別表達同型別
    //       (3) 取「成員變數的型別」(decltype(obj.x)) 來宣告新變數
    //
    //  Q3：decltype(auto)（C++14）解決什麼？
    //    A：「我要 auto 的方便、但要 decltype 的精確 ref/const 保留」 —
    //       常用在「轉發 member access」的小函式中。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1672. Richest Customer Wealth (用 decltype 表達 sum 累加器)
    //   題意：accounts[i][j] 是第 i 客戶在第 j 銀行的存款，回最大總和。
    //   為什麼放這？示範 decltype 用「容器元素型別」推 sum 變數型別 —
    //   日後把 int 改成 long long 時不必動 sum 宣告。
    // ─────────────────────────────────────────────────────────
    std::vector<std::vector<int>> accounts{{1, 5, 2}, {3, 7, 9}, {4, 4, 4}};
    // 注意：decltype(accounts[0][0]) 是 int& 而不是 int（[] 回 lvalue ref），
    //       要當「累加值」要用 decay 把 ref 去掉，這裡直接用 value_type 較簡單：
    using AccElem = std::vector<int>::value_type;   // = int
    AccElem mx = 0;
    for (const auto& row : accounts) {
        int sumRow = 0;
        for (auto v : row) sumRow += v;
        if (sumRow > mx) mx = sumRow;
    }
    std::cout << "[LC1672] max wealth = " << mx << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 decltype 寫「容器元素型別 alias」
    //   工作上常見：寫泛型 helper 時需要「跟容器同型別」的暫存變數
    // ─────────────────────────────────────────────────────────
    std::vector<double> prices{1.5, 2.5, 3.5, 4.5};
    using Elem = decltype(prices)::value_type;     // = double
    Elem total{};                                   // 跟容器元素同型別的 0
    for (auto v : prices) total += v;
    static_assert(std::is_same<Elem, double>::value, "");
    std::cout << "[Demo4] total prices = " << total << " (Elem deduced as double)\n";

    return 0;
}
