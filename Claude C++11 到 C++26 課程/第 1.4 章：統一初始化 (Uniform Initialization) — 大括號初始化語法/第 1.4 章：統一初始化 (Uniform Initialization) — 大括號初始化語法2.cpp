// =============================================================================
//  第 1.4 章 範例 2  —  Most Vexing Parse：{} 如何消除「宣告 vs 建構」的歧義
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  T obj(Args());     // 可能被解析為「函式宣告」← Most Vexing Parse
//           T obj{Args{}};     // 一定是「物件建構」
//   標準：  Most Vexing Parse 自 C++98 就存在（源自 C 的宣告語法）；
//           {} 列表初始化是 C++11 引入的解法。
//   標頭檔：本主題屬語言核心語法，不需要任何標頭檔。
//   本檔驗證標準：g++ -std=c++11 -pedantic-errors -Wall -Wextra（本機 g++ 15.2.0）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼會有這個歧義？——「能宣告就當宣告」】
//   C++ 繼承了 C 的宣告語法，而 C 的宣告允許在參數名外面加上任意多層小括號：
//       int (x);          // 就是 int x;
//       void f(int (a));  // 就是 void f(int a);
//   於是當編譯器看到
//       Widget w(Timer());
//   它面對兩種讀法：
//       (a) 用一個暫時的 Timer 物件去建構 w             ← 人類想的
//       (b) 宣告一個叫 w 的「函式」：參數是「一個回傳 Timer 的函式」，
//           回傳型別是 Widget                            ← 編譯器選的
//   標準規定的消歧義規則是「**只要能解析成宣告，就必須解析成宣告**」
//   （[dcl.ambig.res]，俗稱 "if it can be a declaration, it is a declaration"）。
//   所以 (b) 勝出。注意參數 `Timer()` 依「參數型別調整」規則會從「函式型別」
//   自動退化成「函式指標」，因此 w 的真實型別是 `Widget(Timer(*)())`。
//   本檔用 static_assert 把這件事「證明」給你看，而不是只用嘴巴說。
//
// 【2. 為什麼 {} 就沒有這個問題？——它不是合法的宣告語法】
//   函式宣告的參數列**只能**用小括號包起來，語法上不存在「用大括號寫參數列」
//   這種東西。所以 `Widget w{Timer{}};` 根本無法被讀成宣告，歧義自然消失。
//   這就是 Effective Modern C++ Item 7 推薦「預設用 {}」的重要理由之一：
//   它讓「我要建構一個物件」這個意圖在語法層面就無法被誤讀。
//
// 【概念補充 Concept Deep Dive】
//   * 這個歧義發生在「語法分析階段」，早於多載解析與型別檢查。
//     所以它跟 Widget 有沒有那個建構子完全無關 —— 即使你根本沒寫
//     Widget(Timer)，`Widget w(Timer());` 一樣會被當成函式宣告而編譯通過，
//     然後在你真正想用 w 時才爆出莫名其妙的錯誤。這正是它「vexing」之處：
//     錯誤訊息離真正的錯誤很遠。
//   * 只要「參數位置的東西看起來像個型別」就會觸發。所以
//         Widget w(Timer{});   // OK：Timer{} 是明確的物件，不是型別
//         Widget w(Timer());   // 中招：Timer() 可讀成「回傳 Timer 的函式」
//     差別只在內層的括號形狀。
//   * 在**區塊作用域**中，g++ 會給 -Wvexing-parse 警告提醒你；但在
//     **檔案作用域**（如本檔的示範）不會警告，因為那裡宣告函式再正常不過。
//     這也是為什麼實務上這個 bug 常出現在檔案作用域或類別定義中而不被發現。
//
// 【注意事項 Pay Attention】
//   1. 不要把「加了 {} 就一定安全」推得太遠：{} 解決的是**語法歧義**，
//      但它另外引入了 initializer_list 建構子的**優先權**問題（見本章範例 1
//      與 summary.cpp）。兩個坑要分開記。
//   2. `Widget w();` 也是同一類陷阱：那是宣告一個無參數、回傳 Widget 的函式，
//      不是預設建構。要預設建構請寫 `Widget w;` 或 `Widget w{};`。
//   3. 修好 MVP 有三種寫法，安全性相同，挑一種一致使用即可：
//         Widget w1{Timer{}};   // 全 {}（推薦）
//         Widget w2(Timer{});   // 內層 {} 即可破除歧義
//         Widget w3{Timer()};   // 外層 {} 即可破除歧義
//      額外一種是 C++11 前的老寫法：Widget w4((Timer()));  // 多包一層小括號
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Most Vexing Parse
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `Widget w(Timer());` 這行到底做了什麼？
//     答：它宣告了一個名為 w 的**函式**，不是建構物件。w 的型別是
//         `Widget(Timer(*)())` —— 接收「一個回傳 Timer 的函式指標」、回傳 Widget。
//         因為標準規定「只要能解析成宣告就當宣告」。本檔用 static_assert
//         驗證 decltype(w) 確實是函式型別。
//     追問：那 `Widget w();` 呢？（同樣是函式宣告：無參數、回傳 Widget。
//         想預設建構要寫 `Widget w;` 或 `Widget w{};`）
//
// 🔥 Q2. 為什麼把小括號換成大括號就解決了？
//     答：函式宣告的參數列在語法上**只能**用小括號。`Widget w{Timer{}};`
//         沒有任何一種讀法能變成宣告，歧義在語法層就消失了。這是 C++11
//         列表初始化的主要動機之一。
//     追問：那 `Widget w(Timer{});` 為什麼也行？（歧義來自「參數看起來像型別」，
//         Timer{} 是明確的物件運算式，不可能是型別，所以內層改掉就夠了）
//
// ⚠️ 陷阱. 「我的類別根本沒有 Widget(Timer) 建構子，所以寫錯了編譯器一定會擋。」
//     答：不會。MVP 發生在**語法分析**階段，早於多載解析。就算你沒有那個
//         建構子，`Widget w(Timer());` 依然是一個合法的函式宣告、編譯通過；
//         直到你把 w 當物件用（例如 w.foo()）才會噴出看似無關的錯誤。
//     為什麼會錯：多數人以為「編譯器會先看我有沒有這個建構子，再決定怎麼解析」。
//         實際順序相反 —— 先決定這行是宣告還是定義，才輪到型別與多載。
//         所以錯誤訊息會出現在離兇手很遠的地方。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <vector>

// Most Vexing Parse 問題
class Timer
{
public:
    Timer() { std::cout << "Timer 建構\n"; }
};

class Widget
{
public:
    // 參數不具名：本示範只關心「有沒有被呼叫」，不使用參數值
    Widget(Timer) { std::cout << "Widget 建構\n"; }
};

// -----------------------------------------------------------------------------
// 【證據 A】把 MVP 放在檔案作用域，用 static_assert 證明它真的是「函式」
//   （放檔案作用域還有一個好處：g++ 不會發出 -Wvexing-parse 警告，
//     正好模擬「實務上這個 bug 為何常常沒被發現」）
// -----------------------------------------------------------------------------
Widget vexing(Timer());   // 看起來像建構 vexing 物件，其實是函式宣告！

static_assert(std::is_function<decltype(vexing)>::value,
              "Most Vexing Parse：vexing 竟然是函式型別");
static_assert(std::is_same<decltype(vexing), Widget(Timer(*)())>::value,
              "其真實型別是 Widget(Timer(*)())：參數退化成函式指標");

// -----------------------------------------------------------------------------
// 【日常實務範例】從輸入串流讀取整數建立 vector —— 業界最有名的 MVP 受害者
//   情境：解析一段以空白分隔的數值資料（設定檔欄位、感測器讀數、測試資料）。
//   正確寫法要用 {}；用 () 會變成函式宣告，且錯誤訊息完全看不出真因。
// -----------------------------------------------------------------------------
std::istringstream& sensorStream();   // 僅供下一行宣告使用，不需定義

// ↓ 這行是 Meyers 書中的經典案例：看起來是「用兩個 iterator 建 vector」，
//   實際上是宣告一個名為 parseReadings 的函式。
std::vector<int> parseReadings(std::istream_iterator<int>(sensorStream()),
                               std::istream_iterator<int>());

static_assert(std::is_function<decltype(parseReadings)>::value,
              "同樣中招：這是函式宣告，不是 vector 物件");

// 正確版本：用 {} 建構，語意明確
std::vector<int> parseReadingsFixed(std::istream& in)
{
    return std::vector<int>{std::istream_iterator<int>(in),
                            std::istream_iterator<int>()};
}

int main()
{
    std::cout << std::boolalpha;

    std::cout << "=== 證據：MVP 宣告出來的是函式，不是物件 ===\n";
    std::cout << "is_function<decltype(vexing)>       = "
              << std::is_function<decltype(vexing)>::value << "\n";
    std::cout << "型別是否為 Widget(Timer(*)())      = "
              << std::is_same<decltype(vexing), Widget(Timer(*)())>::value << "\n";
    std::cout << "is_function<decltype(parseReadings)> = "
              << std::is_function<decltype(parseReadings)>::value << "\n\n";

    // 這是宣告一個函式，還是建構一個物件？
    // Widget w(Timer());   // ← 被解析為函式宣告！（見上方檔案作用域的證據）
    //                      //   w 是函式，接收一個回傳 Timer 的函式指標，回傳 Widget

    std::cout << "=== 使用 {} 避免歧義（三種寫法都有效）===\n";

    std::cout << "Widget w1{Timer{}};\n";
    Widget w1{Timer{}};   // 明確是物件建構

    std::cout << "Widget w2(Timer{});\n";
    Widget w2(Timer{});   // 內層 {} 即可破除歧義

    std::cout << "Widget w3{Timer()};\n";
    Widget w3{Timer()};   // 外層 {} 即可破除歧義

    (void)w1; (void)w2; (void)w3;

    std::cout << "\n=== 日常實務：解析感測器讀數 ===\n";
    std::istringstream in("3 1 4 1 5 9 2 6");
    std::vector<int> readings = parseReadingsFixed(in);
    std::cout << "讀到 " << readings.size() << " 筆:";
    for (int v : readings) std::cout << ' ' << v;
    std::cout << "\n";

    return 0;
}

// 編譯: g++ -std=c++11 -pedantic-errors -Wall -Wextra "第 1.4 章：統一初始化 (Uniform Initialization) — 大括號初始化語法2.cpp" -o mvp_demo

// === 預期輸出 ===
// === 證據：MVP 宣告出來的是函式，不是物件 ===
// is_function<decltype(vexing)>       = true
// 型別是否為 Widget(Timer(*)())      = true
// is_function<decltype(parseReadings)> = true
//
// === 使用 {} 避免歧義（三種寫法都有效）===
// Widget w1{Timer{}};
// Timer 建構
// Widget 建構
// Widget w2(Timer{});
// Timer 建構
// Widget 建構
// Widget w3{Timer()};
// Timer 建構
// Widget 建構
//
// === 日常實務：解析感測器讀數 ===
// 讀到 8 筆: 3 1 4 1 5 9 2 6
