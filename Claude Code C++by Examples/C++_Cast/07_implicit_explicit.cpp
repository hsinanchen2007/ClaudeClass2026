// =============================================================================
//  07_implicit_explicit.cpp  —  隱式轉換 與 explicit 關鍵字
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/language/implicit_conversion
//    https://en.cppreference.com/w/cpp/language/explicit
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、隱式轉換是什麼？                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  C++ 允許在某些情境下「不寫 cast」就把一型轉成另一型：
//
//   (a) 標準轉換：int → double、Derived* → Base*、T* → bool
//   (b) 使用者定義轉換：
//        - 只接受「單參數」的非 explicit 建構子（converting constructor）
//        - 非 explicit 的 conversion operator： operator T() const
//
//  例：
//      std::string s = "hello";   // const char* → std::string（隱式）
//      void f(std::string);
//      f("world");                 // 隱式建構 string
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、explicit 為什麼重要？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  少數情境隱式轉換很方便（例如 string）；但更多情境會悄悄製造 bug：
//
//      class Tank { public: Tank(int litres); ... };
//      void refill(Tank t);
//      refill(50);     // 你想要 Tank{50} 還是被解釋成「50 litres」？
//
//  在 Tank 的建構子前加 explicit，就強迫呼叫者寫 Tank{50}：
//
//      class Tank { public: explicit Tank(int litres); ... };
//      refill(50);          // ❌ 編譯錯
//      refill(Tank{50});    // ✅ 明寫意圖
//
//  C++11 起可對「轉換運算子」也加 explicit：
//
//      class Optional { public: explicit operator bool() const; };
//
//  如此 Optional 物件不會「不小心」被當 int / float 用，但仍然可以在
//  if (opt) 處被「明確」轉成 bool（語境轉換例外）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：未標 explicit 的問題演示
//   * Demo 2：加 explicit 後強制使用者明寫
//   * Demo 3：explicit 轉換運算子
// =============================================================================

/*
補充筆記：implicit_explicit
  - implicit_explicit 要先分清楚四種命名轉型：static_cast、dynamic_cast、const_cast、reinterpret_cast 各自解決不同問題。
  - static_cast 用於明確且語意合理的轉換，例如數值轉型、base/derived 已知方向；它不做執行期型別檢查。
  - dynamic_cast 用於 polymorphic base 上的安全向下轉型；失敗時 pointer 得到 nullptr，reference 會丟 std::bad_cast。
  - const_cast 只能調整 const/volatile 屬性；若原物件本來就是 const，移除 const 後修改是未定義行為。
  - reinterpret_cast 是低階重新解讀位元或位址，最容易違反 aliasing、alignment 和生命週期規則；能不用就不用。
  - C++ 風格轉型 (T)x 太模糊，可能偷偷做 const_cast 或 reinterpret_cast；教材應優先使用具名 cast 表達意圖。
  - implicit conversion 方便但可能讓函式呼叫選到非預期 overload；單參數建構子通常要考慮 explicit。
  - explicit 可防止 copy-initialization 的自動轉換，例如 T x = value，但 direct-initialization T x(value) 仍可使用。
  - 轉換運算子 operator T() 也可加 explicit，避免物件在 bool、int、string 等情境被偷偷轉掉。
*/
#include <iostream>
#include <string>

// ─── 沒加 explicit 的版本 ───
struct LooseTank {
    int litres;
    LooseTank(int v) : litres(v) {}            // 隱式建構（converting）
    operator int() const { return litres; }    // 隱式轉 int
};

// ─── 加上 explicit 的版本 ───
struct StrictTank {
    int litres;
    explicit StrictTank(int v) : litres(v) {}
    explicit operator int() const { return litres; }
    explicit operator bool() const { return litres > 0; }
};

static void refill(const LooseTank& t) {
    std::cout << "[refill] (loose)  litres=" << t.litres << '\n';
}
static void refillStrict(const StrictTank& t) {
    std::cout << "[refill] (strict) litres=" << t.litres << '\n';
}

// 前置宣告：附加範例
static void demo_unique_id_wrapper();
static void demo_optional_like_pattern();

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：LooseTank — 隱式轉換到處發生
    // ─────────────────────────────────────────────────────────
    refill(50);                  // 50 → LooseTank(50)，可能不是你想要的
    LooseTank a{20};
    int n = a;                    // LooseTank → int 隱式
    std::cout << "[Demo1] LooseTank to int = " << n << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：StrictTank — 必須明寫
    // ─────────────────────────────────────────────────────────
    // refillStrict(50);          // ❌ 編譯錯：no conversion
    refillStrict(StrictTank{30});

    StrictTank b{10};
    // int m = b;                  // ❌ 編譯錯
    int m = static_cast<int>(b);   // ✅ 明寫
    std::cout << "[Demo2] StrictTank to int = " << m << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：explicit operator bool()
    //   if / while / 三元 ?: 等「contextual conversion」會自動把
    //   explicit operator bool() 當作可用 — 但「賦值給 int」不會。
    // ─────────────────────────────────────────────────────────
    StrictTank c{5};
    if (c) std::cout << "[Demo3] tank c is non-empty (allowed in if)\n";
    // int e = c;                  // ❌ 編譯錯（不會自動轉 int）
    int e = static_cast<bool>(c);   // ✅ 明寫
    std::cout << "[Demo3] static_cast<bool>(c) = " << e << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼建構子應該加 explicit？
    //    A：除非「能像值字面量般合理隱式建構」（例如 std::string 從 char*），
    //       一律加 explicit。Effective Modern C++ 與 cpp core guidelines
    //       都建議「default explicit」。
    //
    //  Q2：explicit 對多參數建構子也有用嗎？
    //    A：C++11 起對「列表初始化」(brace init) 才生效；
    //         struct P { P(int, int); };
    //         P x = {1, 2};      // 隱式 list-init
    //         加 explicit 後 = {1,2} 寫法被禁，必須 P{1,2}。
    //
    //  Q3：explicit(condition) 是什麼？
    //    A：C++20 加的「conditional explicit」 — 用 SFINAE 條件決定要不要
    //       explicit。例如可以「只在某 trait 為 true 時才隱式」。多用於泛型
    //       wrapper（pair、tuple 內部就用了）。
    //
    demo_unique_id_wrapper();
    demo_optional_like_pattern();
    return 0;
}

// =============================================================================
//  附加 1：實用範例 — type-safe ID wrapper（避免不同種類 ID 互相混用）
// =============================================================================
//  工作上常見：UserId、OrderId、ProductId 全都是 int，但語意不同。
//  如果都用 int，那 lookup_user(orderId) 不會被編譯器擋下，造成嚴重 bug。
//  解法：把 ID 包成 explicit 的 struct，從此「不同 ID 不能互換」。
// =============================================================================
struct UserId  { int v; explicit UserId(int x)  : v(x) {} };
struct OrderId { int v; explicit OrderId(int x) : v(x) {} };
static void lookupUser(UserId id) {
    std::cout << "[id_wrapper] looking up user " << id.v << '\n';
}
static void demo_unique_id_wrapper() {
    UserId u{42};
    OrderId o{99};
    lookupUser(u);                  // ✅ 正確
    // lookupUser(o);               // ❌ 編譯錯：OrderId 不能隱式變 UserId
    // lookupUser(123);             // ❌ 編譯錯：explicit 擋下隱式 int → UserId
    lookupUser(UserId{123});        // ✅ 明寫
}

// =============================================================================
//  附加 2：實用範例 — Optional-like wrapper 的 explicit operator bool
// =============================================================================
//  自己的「可選值」型別常常會提供 operator bool 表示「有沒有值」。如果不加
//  explicit，可能會在 ostream <<、算術運算等情境被「不小心」轉成 bool 或 int。
//  加 explicit 後，if/while 等 contextual bool 仍可用，但「+ 1」這種誤用會被擋。
// =============================================================================
class MyOptional {
public:
    explicit MyOptional() : hasValue_(false), value_(0) {}
    MyOptional(int v) : hasValue_(true), value_(v) {}  // 注意：這裡允許隱式 — 看設計取捨

    explicit operator bool() const { return hasValue_; }
    int operator*() const { return value_; }
private:
    bool hasValue_;
    int value_;
};
static void demo_optional_like_pattern() {
    MyOptional a{42}, b{};
    if (a) std::cout << "[opt] a has value = " << *a << '\n';
    if (!b) std::cout << "[opt] b is empty\n";
    // int n = a;            // ❌ 編譯錯：explicit operator bool 不會隱式轉 int
    int n = static_cast<bool>(a);
    std::cout << "[opt] (bool)a = " << n << '\n';
}
