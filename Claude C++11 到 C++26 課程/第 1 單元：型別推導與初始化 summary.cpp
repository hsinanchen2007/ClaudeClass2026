// =============================================================================
//  第 1 單元：型別推導與初始化 summary.cpp  —  auto / decltype / {} 的完整規則書
// =============================================================================
//
// 【主題資訊 Information】
//   本檔涵蓋的語法與其標準版本（皆以 -pedantic-errors 實測驗證）：
//     auto 變數推導                    C++11
//     decltype(expr)                   C++11
//     後置回傳型別 auto f() -> T       C++11
//     統一初始化 {} / initializer_list C++11
//     decltype(auto)                   C++14
//     generic lambda [](auto x){}      C++14
//     變數模板 template<class T> T v   C++14
//     auto x{42} 推導為 int（非 list） C++17（N3922 修正）
//     CTAD（vector v{1,2,3}）          C++17
//     [[maybe_unused]]                 C++17
//   標頭檔：<type_traits>（is_same/decay）、<initializer_list>
//   本檔宣告的標準：C++17（因為用到 CTAD、is_const_v 等 _v 別名）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要型別推導？不是「少打字」而已】
//   C++03 時代寫一個 map 迭代器要打：
//       std::map<std::string, std::vector<int>>::const_iterator it = m.begin();
//   這不只是冗長 — 它是「重複資訊」。型別已經完全由 m.begin() 決定了，
//   人再抄一次只會製造兩個問題：
//     (a) 抄錯 → 觸發隱式轉換，產生一次靜默的複製（const_iterator vs iterator）。
//     (b) 改動 m 的型別時，所有抄寫處都要跟著改 → 維護地獄。
//   auto 把「型別的唯一真實來源」交還給初始化表達式，這是 DRY 原則在型別層的實踐。
//
// 【2. auto 用的是「模板參數推導」規則，不是新規則】
//   這是理解 auto 的關鍵：auto 幾乎完全複用 template<class T> 的推導機制。
//       auto x = expr;        ≈  template<class T> void f(T  x);  f(expr);
//       auto& r = expr;       ≈  template<class T> void f(T& r);  f(expr);
//       auto&& u = expr;      ≈  template<class T> void f(T&& u); f(expr);  ← 轉發引用
//   因此 auto 會做兩件「看似奇怪、其實一致」的事：
//     (a) by-value 推導會剝掉 top-level const 與 reference（因為是「複製一份」）。
//     (b) 陣列與函式會 decay 成指標（因為傳值時陣列無法複製）。
//   唯一例外：auto 對 {} 會推導成 std::initializer_list，而模板推導不會（見第 11 章）。
//
// 【3. decltype 是完全不同的東西：它問「表達式的宣告型別」】
//   auto 問的是「如果我把它複製一份，型別是什麼」；
//   decltype 問的是「這個表達式在標準裡被標記成什麼型別」——不剝任何東西。
//   decltype 有兩條規則，順序不能顛倒：
//     規則 A：若運算元是「未加括號的識別字或成員存取」→ 就是它宣告時的型別。
//     規則 B：否則，看 value category：
//              prvalue → T      lvalue → T&      xvalue → T&&
//   括號陷阱就是規則 A/B 的交界：x 是識別字走 A，(x) 是表達式走 B。
//   int x; decltype(x) → int；decltype((x)) → int&（x 是 lvalue）。
//
// 【4. {} 的三個真正目的（不只是「另一種括號」）】
//   (a) 統一語法：所有初始化都能寫 {}，含聚合、容器、成員初始化。
//   (b) 禁止窄化轉換：int x{3.14}; 是編譯錯誤，而 int x = 3.14; 只是靜默截斷。
//       這是 {} 唯一「多做事」的地方，也是最值得用它的理由。
//   (c) 繞開 Most Vexing Parse：Widget w(); 被解析成函式宣告，Widget w{}; 不會。
//
// 【概念補充 Concept Deep Dive】
//
// (A) auto 剝 const 的精確界線：只剝 top-level
//       const int  ci = 42;   auto a = ci;    // a 是 int      ← top-level const 被剝
//       const int* p  = &ci;  auto b = p;     // b 是 const int* ← low-level const 保留
//     為什麼？因為 by-value 是「複製一份給你」，你自己那份要不要 const 由你決定；
//     但 p 指向的東西是不是 const，是被指物的性質，不能替別人決定。
//
// (B) decltype(auto) 補的是哪個洞（C++14）
//     完美轉發回傳值在 C++11 是做不到的：
//         template<class F, class... A>
//         auto call(F f, A&&... a) -> decltype(f(std::forward<A>(a)...))   // C++11 得寫兩次
//     C++14 的 decltype(auto) 讓「回傳型別 = 回傳表達式的 decltype」：
//         decltype(auto) call(F f, A&&... a) { return f(std::forward<A>(a)...); }
//     代價是它把括號陷阱一起繼承了：return x; 回傳 int，return (x); 回傳 int&。
//     後者若 x 是區域變數就是懸空引用。
//
// (C) initializer_list 的記憶體真相
//     編譯器為 {1,2,3} 產生一個「後備陣列（backing array）」，
//     initializer_list 只是持有 {const T* begin, size_t size} 的輕量 view。
//     後備陣列的生命週期 = 該 initializer_list 物件的生命週期。
//     所以「回傳 initializer_list」= 回傳指向已死陣列的 view，是懸空。
//     元素是 const 的，因此 initializer_list 的元素只能複製、無法移動 —
//     這正是 vector<unique_ptr<T>> v{make_unique<T>()...} 編譯不過的原因。
//
// (D) 為何 {} 讓 initializer_list 建構子「霸道地」勝出
//     重載決議對 {} 有一條特例：只要有任何 initializer_list 建構子可行，
//     它就優先於所有其他建構子，甚至優先於完全精確匹配的版本。
//         vector<int> v1(5, 10);  // 5 個 10
//         vector<int> v2{5, 10};  // 2 個元素 5, 10
//     這不是 bug，是刻意設計：讓 {} 的「列表語意」永遠可預測。
//     代價是你必須記住「要呼叫大小/容量類建構子就用 ()」。
//
// 【注意事項 Pay Attention】
//   1. auto 一定要有初始化式；auto x; 無法推導。
//   2. auto 會剝 top-level const 與 reference — 需要保留請明寫 const auto& / auto&。
//   3. decltype((x)) 與 decltype(x) 不同型別；多打一對括號會從 int 變 int&。
//   4. decltype(auto) 的 return (x); 若 x 是區域變數，回傳的引用在函式結束後即懸空，
//      後續使用是未定義行為（UB）——實際觀察到的現象不固定，不可當成穩定結果。
//   5. auto x = {1,2,3}; 得到 initializer_list<int>，不是 vector。
//   6. auto x{42}; 在 C++17 起是 int；C++11/14 是 initializer_list<int>（N3922 修正）。
//   7. 窄化檢查只有 {} 會做；= 與 () 都只會靜默截斷。
//   8. 本檔多處刻意宣告變數只為展示推導結果，故標記 [[maybe_unused]]（C++17）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】型別推導與初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto 和 decltype 的根本差別是什麼？
//     答：auto 用「模板參數推導」規則，語意是「複製一份會是什麼型別」，
//         所以會剝掉 top-level const 與 reference、陣列會 decay 成指標。
//         decltype 用「宣告型別查詢」規則，不剝任何東西，並且會保留 reference。
//         一句話：auto 問「複製後是什麼」，decltype 問「它本來被宣告成什麼」。
//     追問：那 decltype(auto) 是哪一種？→ 用 auto 的位置（不必寫出型別），
//         但套 decltype 的規則（完整保留 const/reference），C++14 引入。
//
// 🔥 Q2. decltype(x) 和 decltype((x)) 為什麼不同？
//     答：decltype 規則 A 只在運算元是「未加括號的識別字/成員存取」時適用，
//         此時回傳宣告型別 int。加了括號後 (x) 不再是識別字而是 lvalue 表達式，
//         走規則 B：lvalue → T&，得到 int&。
//     追問：這在哪裡會咬人？→ decltype(auto) f(){ int x=42; return (x); }
//         回傳型別變成 int&，指向已銷毀的區域變數，是懸空引用。
//
// 🔥 Q3. vector<int> v(5, 10) 和 vector<int> v{5, 10} 差在哪？
//     答：() 呼叫 (count, value) 建構子 → 5 個 10。
//         {} 因為 initializer_list 建構子優先規則 → 2 個元素 [5, 10]。
//         重載決議對 {} 的特例是「只要 initializer_list 版本可行就必勝」。
//     追問：那要怎麼強制用 (count,value)？→ 只能用 ()，這是刻意的設計取捨。
//
// ⚠️ 陷阱 1. 「用 {} 一律比較安全，所以全部改成 {}」對嗎？
//     答：不對。{} 在「防窄化」與「避開 Most Vexing Parse」上確實更安全，
//         但它會改變重載決議結果 — 對有 initializer_list 建構子的型別
//         （vector/map 等）語意完全不同，vector<int>(5,10) 改成 {5,10} 是行為改變。
//     為什麼會錯：把 {} 當成「語法比較新的括號」，以為只是寫法差異；
//         實際上 {} 是一條獨立的重載決議路徑，不是 () 的同義詞。
//
// ⚠️ 陷阱 2. auto a = ci;（ci 是 const int）之後改 a 會編譯錯誤嗎？
//     答：不會。a 的型別是 int，top-level const 在 by-value 推導時被剝掉了。
//         很多人以為「const 會傳染」而預期編譯錯誤。
//     為什麼會錯：把 const 想成附著在「值」上；實際上 top-level const 修飾的是
//         「那個變數本身」，複製出來的新變數是不是 const 由新的宣告決定。
//         但 low-level const（指標所指之物）會保留：auto b = p;（const int* p）
//         推導出 const int*，仍不能透過 b 改內容。
// ═══════════════════════════════════════════════════════════════════════════
//
// 本檔案是一本「迷你書」，涵蓋以下章節：
//   第 1 章：auto — 讓編譯器幫你推導型別
//   第 2 章：auto 的推導規則 — const、引用、指標、陣列怎麼處理
//   第 3 章：decltype — 查詢任何表達式的型別
//   第 4 章：decltype 的括號陷阱 — 加括號型別會改變！
//   第 5 章：decltype(auto) — 完美保留型別的自動推導
//   第 6 章：後置回傳型別 — auto func() -> decltype(...)
//   第 7 章：統一初始化 {} — 大括號初始化語法
//   第 8 章：{} 的超能力 — 防止窄化轉換
//   第 9 章：std::initializer_list — 大括號背後的機制
//   第 10 章：{} vs () — 建構子選擇的優先順序陷阱
//   第 11 章：auto + {} — 型別推導的特殊規則
//   第 12 章：Most Vexing Parse — () 的歧義與 {} 的解法
//   第 13 章：實戰模式與最佳實踐
//   第 14 章：常見錯誤與陷阱大全
//   第 15 章：LeetCode 實戰 + 日常實務範例（本次新增）

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <array>
#include <type_traits>
#include <initializer_list>
#include <algorithm>
#include <numeric>
#include <memory>

using namespace std;

// ================================================================
//  輔助工具
// ================================================================
void title(const char* ch) {
    cout << "\n";
    cout << "================================================================\n";
    cout << "  " << ch << "\n";
    cout << "================================================================\n\n";
}
void sub(const char* s) { cout << "--- " << s << " ---\n"; }

// 型別資訊檢查巨集
#define SHOW_TYPE(expr) \
    cout << "  " << #expr << ":\n" \
         << "    is_const:     " << is_const_v<remove_reference_t<decltype(expr)>> << "\n" \
         << "    is_reference: " << is_reference_v<decltype(expr)> << "\n" \
         << "    is_pointer:   " << is_pointer_v<decltype(expr)> << "\n\n"


// ================================================================
//
//  第 1 章：auto — 讓編譯器幫你推導型別
//
// ================================================================
// 【C++11 最重要的語法糖之一】
//
// auto 讓編譯器根據初始化表達式自動推導變數的型別。
// 你不需要寫冗長的型別名稱，編譯器會幫你算出來。
//
// 使用 auto 的前提：變數必須有初始化值（否則編譯器無法推導）
//   auto x = 42;    // ✅ int
//   auto x;         // ❌ 編譯錯誤！沒有初始化值
//
// 最大的好處：簡化複雜型別名稱，尤其是迭代器和模板型別

void chapter1() {
    title("第 1 章：auto — 讓編譯器幫你推導型別");

    sub("基本型別推導");
    auto i = 42;          // int
    auto d = 3.14;        // double
    auto f = 3.14f;       // float（因為 f 後綴）
    auto c = 'A';         // char
    auto b = true;        // bool
    auto ll = 100LL;      // long long（因為 LL 後綴）
    cout << "  auto i = 42;     → int,       i = " << i << "\n";
    cout << "  auto d = 3.14;   → double,    d = " << d << "\n";
    cout << "  auto f = 3.14f;  → float,     f = " << f << "\n";
    cout << "  auto c = 'A';    → char,       c = " << c << "\n";
    cout << "  auto b = true;   → bool,       b = " << boolalpha << b << "\n";
    cout << "  auto ll = 100LL; → long long, ll = " << ll << "\n\n";

    sub("字串型別推導（注意！）");
    [[maybe_unused]] auto str1 = "Hello";              // const char*（字串字面值）
    auto str2 = string("World");      // string
    cout << "  auto str1 = \"Hello\";         → const char* ← 不是 string！\n";
    cout << "  auto str2 = string(\"World\"); → string\n\n";

    sub("auto 最常見的用途：簡化迭代器");
    map<string, vector<int>> data;
    data["Alice"] = {90, 85, 88};
    data["Bob"] = {78, 82, 80};

    cout << "  // 沒有 auto（C++03 風格）：\n";
    cout << "  // map<string, vector<int>>::iterator it = data.begin();\n";
    cout << "  // 用 auto（C++11）：\n";
    cout << "  // auto it = data.begin();  ← 簡潔多了！\n\n";

    for (auto it = data.begin(); it != data.end(); ++it) {
        cout << "  " << it->first << " 的成績: ";
        for (auto score : it->second)
            cout << score << " ";
        cout << "\n";
    }
    cout << "\n";

    sub("auto 不能用的地方");
    cout << "  ❌ auto x;               沒有初始化值\n";
    cout << "  ❌ auto arr[5] = {...};   不能用於陣列宣告\n";
    cout << "  ❌ class C { auto v=1; }; 不能用於非靜態成員\n";
    cout << "  ❌ void func(auto x);     C++11/14 不能用於參數（C++20 才行）\n";
}


// ================================================================
//
//  第 2 章：auto 的推導規則 — const、引用、指標、陣列怎麼處理
//
// ================================================================
// 【核心規則】auto 推導時：
//   1. 忽略頂層 const（top-level const）
//   2. 忽略引用（reference）
//   3. 保留指標
//   4. 陣列退化為指標
//
// 如果要保留 const 或引用，需要明確寫出：
//   const auto x = ...;    保留 const
//   auto& x = ...;         保留引用
//   const auto& x = ...;   保留 const + 引用

void chapter2() {
    title("第 2 章：auto 的推導規則");

    int x = 100;
    const int cx = 200;
    int& rx = x;
    const int& crx = x;

    sub("規則 1：忽略頂層 const（因為是複製）");
    [[maybe_unused]] auto a1 = cx;    // int（忽略 const，因為 a1 是新的副本）
    [[maybe_unused]] auto a2 = crx;   // int（忽略 const 和引用）
    a1 = 999;        // ✅ a1 可以修改，它是 int
    cout << "  auto a1 = cx(const int);  → a1 是 int，可修改\n";
    cout << "  auto a2 = crx(const int&); → a2 是 int，可修改\n\n";

    sub("規則 2：忽略引用（因為是複製）");
    auto a3 = rx;    // int（忽略引用）
    a3 = 777;
    cout << "  auto a3 = rx(int&); → a3 是 int，修改 a3 不影響 x\n";
    cout << "  x = " << x << "（不變）, a3 = " << a3 << "\n\n";

    sub("明確保留 const 或引用");
    const auto ca = x;      // const int
    auto& ra = x;            // int&
    const auto& cra = x;     // const int&
    [[maybe_unused]] auto& ra2 = cx;          // const int&（底層 const 保留！）

    ra = 555;
    cout << "  auto& ra = x;        → int&，修改 ra 就是修改 x\n";
    cout << "  x = " << x << "\n";
    // ca = 1;   // ❌ const int 不能修改
    // cra = 1;  // ❌ const int& 不能修改
    cout << "  const auto ca = x;   → const int，不能修改\n";
    cout << "  const auto& cra = x; → const int&，不能修改\n";
    cout << "  auto& ra2 = cx;      → const int&（底層 const 保留！）\n\n";

    sub("規則 3：指標被保留");
    int* ptr = &x;
    auto p1 = ptr;     // int*
    auto* p2 = ptr;    // int*（明確寫 * 效果相同）
    cout << "  auto p = ptr;  → int*（指標保留）\n";
    cout << "  *p1 = " << *p1 << "\n\n";

    sub("規則 4：陣列退化為指標");
    int arr[5] = {10, 20, 30, 40, 50};
    auto arrAuto = arr;     // int*（退化為指標）
    auto& arrRef = arr;     // int(&)[5]（引用保留陣列型別）

    cout << "  auto arrAuto = arr;   → int*（退化，sizeof = " << sizeof(arrAuto) << "）\n";
    cout << "  auto& arrRef = arr;   → int(&)[5]（保留，sizeof = " << sizeof(arrRef) << "）\n\n";

    sub("推導規則總結表");
    cout << "  ┌──────────────────┬───────────────────┬─────────────┐\n";
    cout << "  │    原始型別       │ auto x = ...      │ 推導結果    │\n";
    cout << "  ├──────────────────┼───────────────────┼─────────────┤\n";
    cout << "  │ int              │ auto a = x;       │ int         │\n";
    cout << "  │ const int        │ auto a = cx;      │ int         │\n";
    cout << "  │ int&             │ auto a = rx;      │ int         │\n";
    cout << "  │ const int&       │ auto a = crx;     │ int         │\n";
    cout << "  │ int*             │ auto a = ptr;     │ int*        │\n";
    cout << "  │ int[5]           │ auto a = arr;     │ int*        │\n";
    cout << "  ├──────────────────┼───────────────────┼─────────────┤\n";
    cout << "  │ 要保留 const     │ const auto a = x; │ const int   │\n";
    cout << "  │ 要保留引用       │ auto& a = x;      │ int&        │\n";
    cout << "  │ 要保留陣列       │ auto& a = arr;    │ int(&)[5]   │\n";
    cout << "  └──────────────────┴───────────────────┴─────────────┘\n";

    (void)ca; (void)cra; (void)p2;
}


// ================================================================
//
//  第 3 章：decltype — 查詢任何表達式的型別
//
// ================================================================
// 【auto vs decltype 的根本差異】
//   auto：推導時忽略 const 和引用（因為是「複製」語意）
//   decltype：完整保留表達式的型別（const、引用全部保留）
//
// 【decltype 的兩條推導規則】
//   規則 A：decltype(變數名) → 該變數的宣告型別（原封不動）
//   規則 B：decltype(表達式) → 根據表達式的值類別：
//           lvalue → T&
//           prvalue → T
//           xvalue → T&&
//
// 【decltype 不會執行表達式！】
//   decltype(func()) x;  // func() 不會被呼叫，只是查詢回傳型別

int globalVal = 100;
int& getGlobalRef() { return globalVal; }
const int& getConstRef() { static int v = 200; return v; }
int getValue() { return 42; }

void chapter3() {
    title("第 3 章：decltype — 查詢任何表達式的型別");

    int x = 10;
    const int cx = 20;
    int& rx = x;
    [[maybe_unused]] const int& crx = x;

    sub("auto vs decltype 的關鍵差異");
    auto       autoVal = cx;       // int（忽略 const）
    [[maybe_unused]] decltype(cx) declVal = 100;    // const int（保留 const）

    [[maybe_unused]] auto       autoRef = rx;       // int（忽略引用）
    decltype(rx) declRef = x;      // int&（保留引用）

    cout << "  原始：const int cx = 20;\n";
    cout << "    auto a = cx;       → int（忽略 const）\n";
    cout << "    decltype(cx) d;    → const int（保留！）\n\n";

    cout << "  原始：int& rx = x;\n";
    cout << "    auto a = rx;       → int（忽略引用）\n";
    cout << "    decltype(rx) d = x; → int&（保留！）\n\n";

    // 驗證引用
    declRef = 222;
    cout << "  declRef = 222 後，x = " << x << "（證明 declRef 是引用）\n\n";

    sub("decltype 與函式回傳型別");
    decltype(getValue()) v1;               // int（回傳 int）
    decltype(getGlobalRef()) v2 = x;       // int&（回傳 int&）
    decltype(getConstRef()) v3 = x;        // const int&
    cout << "  decltype(getValue())     → int\n";
    cout << "  decltype(getGlobalRef()) → int&\n";
    cout << "  decltype(getConstRef())  → const int&\n";
    cout << "  注意：函式不會被實際呼叫！\n\n";

    sub("decltype 保留陣列型別");
    int arr[5] = {1, 2, 3, 4, 5};
    decltype(arr) arr2 = {10, 20, 30, 40, 50};  // int[5]
    auto arrAuto = arr;                          // int*（退化）
    cout << "  sizeof(arr)     = " << sizeof(arr) << "\n";
    cout << "  sizeof(arr2)    = " << sizeof(arr2) << "（decltype 保留陣列）\n";
    cout << "  sizeof(arrAuto) = " << sizeof(arrAuto) << "（auto 退化為指標）\n\n";

    sub("decltype 與容器元素");
    vector<int> vec = {1, 2, 3};
    decltype(vec[0]) elem = vec[0];  // int&（operator[] 回傳引用）
    elem = 100;
    cout << "  decltype(vec[0]) → int&\n";
    cout << "  修改 elem 後 vec[0] = " << vec[0] << "（因為是引用）\n\n";

    sub("decltype 定義型別別名");
    vector<pair<string, int>> records;
    using RecordType = decltype(records);        // vector<pair<string,int>>
    using IterType   = decltype(records.begin()); // vector 的 iterator
    // 用 static_assert 驗證別名真的等於那個冗長型別（編譯期檢查，零執行成本）
    static_assert(is_same_v<RecordType, vector<pair<string, int>>>,
                  "RecordType 應等於 vector<pair<string,int>>");
    static_assert(is_same_v<IterType, vector<pair<string, int>>::iterator>,
                  "IterType 應等於該 vector 的 iterator");
    cout << "  using RecordType = decltype(records); → 省去寫完整型別\n";
    cout << "  static_assert 已在編譯期驗證兩個別名的型別正確 ✅\n";

    (void)v1; (void)v2; (void)v3; (void)autoVal;
}


// ================================================================
//
//  第 4 章：decltype 的括號陷阱 — 加括號型別會改變！
//
// ================================================================

void chapter4() {
    title("第 4 章：decltype 的括號陷阱");

    int num = 42;

    sub("不加括號 vs 加括號");
    decltype(num) d1 = 0;      // int（規則 A：變數名 → 宣告型別）
    decltype((num)) d2 = num;  // int&（規則 B：(num) 是左值表達式 → T&）

    d2 = 999;
    cout << "  decltype(num)   → int   （變數名 → 宣告型別）\n";
    cout << "  decltype((num)) → int&  （左值表達式 → T&）\n";
    cout << "  d2 = 999 後，num = " << num << "（因為 d2 是引用！）\n\n";

    sub("為什麼加括號就變引用了？");
    cout << "  decltype 有兩條推導規則：\n";
    cout << "    規則 A：如果放的是「變數名」→ 回傳宣告型別\n";
    cout << "    規則 B：如果放的是「表達式」→ 根據值類別決定\n\n";
    cout << "  num   → 是變數名 → 走規則 A → int\n";
    cout << "  (num) → 加括號後變成表達式 → 走規則 B\n";
    cout << "        → (num) 是左值 → T& → int&\n\n";

    sub("這個陷阱在 return 時最危險");
    cout << "  decltype(auto) func() {\n";
    cout << "      int x = 42;\n";
    cout << "      return x;    → int  （安全，回傳值）\n";
    cout << "      return (x);  → int& （危險！回傳局部變數的引用 → 懸空！）\n";
    cout << "  }\n";

    sub("表達式的值類別決定 decltype 結果");
    int a = 5, b = 10;
    decltype(a + b) sum = 0;        // int（a+b 是 prvalue → T）
    decltype(a = b) ref = a;        // int&（a=b 回傳 a 的引用 → lvalue → T&）
    cout << "  decltype(a + b)  → int  （prvalue → T）\n";
    cout << "  decltype(a = b)  → int& （lvalue → T&）\n";
    cout << "  decltype(a++)    → int  （prvalue → T）\n";
    cout << "  decltype(++a)    → int& （lvalue → T&）\n";

    (void)d1; (void)sum; (void)ref;
}


// ================================================================
//
//  第 5 章：decltype(auto) — 完美保留型別的自動推導
//
// ================================================================
// C++14 新增。結合了 auto 的便利和 decltype 的精確。
//
// 【auto vs decltype(auto)】
//   auto x = expr;            用 auto 規則推導（忽略 const/引用）
//   decltype(auto) x = expr;  用 decltype 規則推導（完整保留）

int g_value = 100;

auto autoReturn()           { return g_value; }   // int（複製）
decltype(auto) decltypeReturn() { return g_value; } // int（變數名 → 宣告型別）

int& getRef() { return g_value; }
auto autoReturnRef()           { return getRef(); }   // int（auto 忽略引用！）
decltype(auto) decltypeReturnRef() { return getRef(); } // int&（保留引用！）

void chapter5() {
    title("第 5 章：decltype(auto) — 完美保留型別");

    sub("變數推導比較");
    int x = 10;
    const int cx = 20;
    int& rx = x;

    auto a1 = cx;                // int（忽略 const）
    [[maybe_unused]] decltype(auto) d1 = cx;      // const int（保留 const）

    auto a2 = rx;                // int（忽略引用）
    decltype(auto) d2 = rx;      // int&（保留引用）

    cout << "  auto a1 = cx;            → int (忽略 const)\n";
    cout << "  decltype(auto) d1 = cx;  → const int (保留!)\n\n";
    cout << "  auto a2 = rx;            → int (忽略引用)\n";
    cout << "  decltype(auto) d2 = rx;  → int& (保留!)\n\n";

    // 驗證
    d2 = 888;
    cout << "  d2 = 888 後，x = " << x << "（d2 是引用）\n\n";

    sub("函式回傳型別比較");
    cout << "  auto 回傳:            永遠是值（忽略引用）\n";
    cout << "  decltype(auto) 回傳:  保留引用！\n\n";

    g_value = 100;
    decltypeReturnRef() = 500;  // 直接修改全域變數！
    cout << "  decltypeReturnRef() = 500;\n";
    cout << "  g_value = " << g_value << "（成功修改！因為回傳 int&）\n\n";

    sub("括號陷阱在 decltype(auto) 中更危險！");
    int num = 42;
    [[maybe_unused]] decltype(auto) safe = num;    // int
    [[maybe_unused]] decltype(auto) danger = (num); // int& ← 加括號變引用！
    cout << "  decltype(auto) safe   = num;   → int\n";
    cout << "  decltype(auto) danger = (num); → int& ← 陷阱！\n";
    cout << "  在 return 中：return (x) 會回傳引用 → 可能懸空！\n\n";

    sub("泛型轉發函式");
    cout << "  template<typename F>\n";
    cout << "  auto callWithAuto(F f)           { return f(); } // 丟失引用\n";
    cout << "  decltype(auto) callPerfect(F f)  { return f(); } // 保留引用\n";

    (void)a1; (void)a2;
}


// ================================================================
//
//  第 6 章：後置回傳型別 — auto func() -> decltype(...)
//
// ================================================================

// C++11 的問題：回傳型別寫在函式名前，此時參數還不存在
// int add(int a, int b);  ← 如果不知道 a+b 的型別呢？

// C++11 解法：後置回傳型別
template<typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

// C++14 之後可以省略後置型別：
// template<typename T, typename U>
// auto add14(T a, U b) { return a + b; }  // auto 自動推導回傳型別

void chapter6() {
    title("第 6 章：後置回傳型別");

    sub("auto func() -> decltype(...)");
    auto r1 = add(1, 2);       // int
    auto r2 = add(1, 2.5);     // double（int + double = double）
    auto r3 = add(1.5f, 2.5);  // double（float + double = double）
    cout << "  add(1, 2)     = " << r1 << " (int)\n";
    cout << "  add(1, 2.5)   = " << r2 << " (double)\n";
    cout << "  add(1.5f, 2.5)= " << r3 << " (double)\n\n";

    cout << "  // C++11 語法：\n";
    cout << "  auto add(T a, U b) -> decltype(a + b) { return a + b; }\n\n";
    cout << "  // C++14 簡化：\n";
    cout << "  auto add(T a, U b) { return a + b; }  // 自動推導\n";
}


// ================================================================
//
//  第 7 章：統一初始化 {} — 大括號初始化語法
//
// ================================================================

void chapter7() {
    title("第 7 章：統一初始化 {} — 大括號語法");

    sub("C++11 之前的初始化方式（混亂）");
    cout << "  int x = 42;         // 複製初始化\n";
    cout << "  int x(42);          // 直接初始化\n";
    cout << "  int arr[] = {1,2};  // 聚合初始化（只有陣列能用 {}）\n";
    cout << "  每種型別有不同的初始化語法 → 混亂！\n\n";

    sub("C++11：{} 統一所有初始化語法");

    // 基本型別
    int i1{42};
    int i2 = {42};
    int i3{};         // 值初始化 → 0
    cout << "  int i1{42};   → " << i1 << "\n";
    cout << "  int i2 = {42}; → " << i2 << "\n";
    cout << "  int i3{};     → " << i3 << " （值初始化為 0）\n\n";

    // 陣列
    int arr1[5]{1, 2, 3, 4, 5};
    int arr2[5]{1, 2};            // 其餘為 0
    int arr3[5]{};                // 全部為 0
    cout << "  int arr[5]{1,2};   → ";
    for (int n : arr2) cout << n << " ";
    cout << "（其餘為 0）\n\n";

    // STL 容器
    vector<int> vec{1, 2, 3, 4, 5};
    map<string, int> scores{{"Alice", 95}, {"Bob", 87}};
    cout << "  vector<int> vec{1,2,3,4,5};  ← 直接初始化！\n";
    cout << "  map<string,int> m{{\"Alice\",95}, {\"Bob\",87}};\n\n";

    // 聚合類別
    struct Point { int x; int y; };
    Point p{10, 20};
    cout << "  struct Point { int x; int y; };\n";
    cout << "  Point p{10, 20};  → (" << p.x << ", " << p.y << ")\n\n";

    // 零初始化
    sub("零初始化 vs 預設初始化");
    struct Data { int x; double y; };
    Data d1;     // 預設初始化 → 值不確定！
    Data d2{};   // 值初始化 → x=0, y=0.0
    cout << "  Data d1;    → 成員值不確定（危險！）\n";
    cout << "  Data d2{};  → x=" << d2.x << ", y=" << d2.y << "（安全）\n";

    (void)arr1; (void)arr3; (void)d1;
}


// ================================================================
//
//  第 8 章：{} 的超能力 — 防止窄化轉換
//
// ================================================================

void chapter8() {
    title("第 8 章：{} 防止窄化轉換（Narrowing Conversion）");

    sub("什麼是窄化轉換？");
    cout << "  把大範圍的值塞進小範圍的型別，可能丟失資料：\n";
    cout << "    double → int     （丟失小數部分）\n";
    cout << "    int → char       （可能溢位）\n";
    cout << "    long long → int  （可能溢位）\n\n";

    sub("() 和 = 不會阻止窄化");
    double pi = 3.14159;
    int truncated1 = pi;        // ✅ 編譯通過，但丟失 0.14159
    int truncated2(pi);         // ✅ 編譯通過，但丟失 0.14159
    cout << "  int x = 3.14159;   → " << truncated1 << " （丟失小數，但編譯通過）\n";
    cout << "  int x(3.14159);    → " << truncated2 << " （同上）\n\n";

    sub("{} 會在編譯期阻止窄化！");
    // int truncated3{pi};      // ❌ 編譯錯誤！窄化轉換
    // int truncated4 = {pi};   // ❌ 編譯錯誤！窄化轉換
    cout << "  int x{3.14159};    → ❌ 編譯錯誤！\n";
    cout << "  int x = {3.14159}; → ❌ 編譯錯誤！\n\n";

    // 常量在範圍內則允許
    char c{65};   // ✅ 65 在 char 範圍內
    cout << "  char c{65};        → ✅ 65 在 char 範圍內，合法\n\n";

    [[maybe_unused]] int big = 1000;
    // char d{big};  // ❌ big 不是常量表達式，可能超範圍
    cout << "  int big = 1000;\n";
    cout << "  char d{big};       → ❌ 編譯錯誤（big 不是常量）\n\n";

    sub("窄化轉換完整列表");
    cout << "  ┌─────────────────────┬────────┬────────┐\n";
    cout << "  │ 轉換                │ () / = │   {}   │\n";
    cout << "  ├─────────────────────┼────────┼────────┤\n";
    cout << "  │ double → int       │   ✅   │   ❌   │\n";
    cout << "  │ float → int        │   ✅   │   ❌   │\n";
    cout << "  │ int → char         │   ✅   │ 看值   │\n";
    cout << "  │ long long → int    │   ✅   │ 看值   │\n";
    cout << "  │ int → unsigned     │   ✅   │ 看值   │\n";
    cout << "  │ 常量 65 → char     │   ✅   │   ✅   │\n";
    cout << "  └─────────────────────┴────────┴────────┘\n";
    cout << "  「看值」= 如果是常量且在目標範圍內則允許\n";

    (void)c;
}


// ================================================================
//
//  第 9 章：std::initializer_list — 大括號背後的機制
//
// ================================================================

// 當你寫 vector<int> v{1,2,3}; 時，{1,2,3} 到底是什麼？
// 答案：它會被轉成 std::initializer_list<int>

// 自製類別也可以接收 initializer_list
class NumberList {
    vector<int> data_;
public:
    // 一般建構子
    NumberList(size_t size, int value) : data_(size, value) {
        cout << "  NumberList(size=" << size << ", value=" << value << ")\n";
    }
    // initializer_list 建構子
    NumberList(initializer_list<int> init) : data_(init) {
        cout << "  NumberList(initializer_list, " << init.size() << " 個元素)\n";
    }
    void print() const {
        cout << "    → ";
        for (int n : data_) cout << n << " ";
        cout << "\n";
    }
};

// 函式也可以接收 initializer_list
int sum(initializer_list<int> values) {
    return accumulate(values.begin(), values.end(), 0);
}

void chapter9() {
    title("第 9 章：std::initializer_list");

    sub("initializer_list 的基本性質");
    initializer_list<int> list = {10, 20, 30, 40};
    cout << "  initializer_list<int> list = {10, 20, 30, 40};\n";
    cout << "  size = " << list.size() << "\n";
    cout << "  元素是 const 的（不能修改）\n";
    cout << "  底層是臨時陣列，複製 list 只複製指標（輕量！）\n\n";

    sub("自訂類別的 initializer_list 建構子");
    cout << "  NumberList n1{5, 10}:\n";
    NumberList n1{5, 10};     // initializer_list 版本！
    n1.print();

    cout << "  NumberList n2(5, 10):\n";
    NumberList n2(5, 10);     // 一般建構子 (size, value)
    n2.print();
    cout << "\n";

    sub("函式參數使用 initializer_list");
    cout << "  sum({1, 2, 3, 4, 5}) = " << sum({1, 2, 3, 4, 5}) << "\n\n";

    sub("標準容器都支援 initializer_list");
    cout << "  vector<int> v{1,2,3};           // 建構子\n";
    cout << "  v.assign({10, 20, 30});          // assign 方法\n";
    cout << "  v.insert(v.end(), {40, 50});     // insert 方法\n";
}


// ================================================================
//
//  第 10 章：{} vs () — 建構子選擇的優先順序陷阱
//
// ================================================================

void chapter10() {
    title("第 10 章：{} vs () — 建構子優先順序陷阱");

    sub("陷阱：{} 優先匹配 initializer_list 建構子！");
    cout << "  當類別同時有 initializer_list 建構子和一般建構子時，\n";
    cout << "  {} 會優先匹配 initializer_list 版本！\n\n";

    // vector 是最典型的例子
    sub("vector 的陷阱");
    vector<int> v1(5, 10);   // 5 個 10    → [10, 10, 10, 10, 10]
    vector<int> v2{5, 10};   // 兩個元素   → [5, 10]

    cout << "  vector<int> v1(5, 10);  → ";
    for (int n : v1) cout << n << " ";
    cout << "（5 個 10）\n";

    cout << "  vector<int> v2{5, 10};  → ";
    for (int n : v2) cout << n << " ";
    cout << "（兩個元素：5 和 10）\n\n";

    cout << "  ⚠️ 完全不同的結果！\n\n";

    sub("自訂類別也一樣");
    cout << "  NumberList n1{5, 10};   → initializer_list 版本\n";
    cout << "  NumberList n2(5, 10);   → 一般建構子 (size, value)\n\n";

    sub("規則總結");
    cout << "  1. {} 優先匹配 initializer_list 建構子\n";
    cout << "  2. 如果沒有 initializer_list 建構子 → 匹配一般建構子\n";
    cout << "  3. {} 空的情況 → 呼叫預設建構子（不是空 initializer_list）\n";
    cout << "  4. 想指定用一般建構子 → 用 ()\n\n";

    sub("建議");
    cout << "  容器用 {} 初始化元素值：vector<int> v{1,2,3};\n";
    cout << "  容器用 () 指定大小/填充：vector<int> v(100, 0);\n";
}


// ================================================================
//
//  第 11 章：auto + {} — 型別推導的特殊規則
//
// ================================================================

void chapter11() {
    title("第 11 章：auto + {} 的特殊規則");

    sub("auto + {} = initializer_list（C++11/14）");
    auto list1 = {1, 2, 3};  // initializer_list<int>
    cout << "  auto list = {1, 2, 3};  → initializer_list<int>\n";
    cout << "  size = " << list1.size() << "\n\n";

    // auto list2 = {1, 2.0};  // ❌ 型別不一致
    cout << "  auto list = {1, 2.0};   → ❌ 型別不一致（int vs double）\n\n";

    sub("C++17 的變化：auto x{42}");
    auto x1{42};    // C++17：int（不是 initializer_list）
    // C++11/14：initializer_list<int>
    cout << "  auto x{42};\n";
    cout << "    C++11/14 → initializer_list<int>（一個元素）\n";
    cout << "    C++17    → int（直接推導為 int）\n\n";

    sub("建議");
    cout << "  想要 initializer_list → auto list = {1,2,3};\n";
    cout << "  想要單一值           → auto x = 42;（用 = 不用 {}）\n";

    (void)x1;
}


// ================================================================
//
//  第 12 章：Most Vexing Parse — () 的歧義與 {} 的解法
//
// ================================================================

void chapter12() {
    title("第 12 章：Most Vexing Parse");

    sub("問題：() 可能被解析為函式宣告");
    cout << "  class Timer {};\n";
    cout << "  class Widget { Widget(Timer t) {} };\n\n";

    cout << "  Widget w(Timer());   // 這是什麼？\n";
    cout << "    ❌ 不是建構 Widget 物件！\n";
    cout << "    ✅ 被解析為「函式宣告」：\n";
    cout << "       w 是一個函式，接收「回傳 Timer 的函式指標」，回傳 Widget\n\n";

    sub("解法：用 {} 消除歧義");
    cout << "  Widget w1{Timer{}};  // ✅ 明確是物件建構\n";
    cout << "  Widget w2(Timer{}); // ✅ 也行（參數用 {}）\n";
    cout << "  Widget w3{Timer()};  // ✅ 也行（外層用 {}）\n\n";

    sub("其他常見的 Most Vexing Parse");
    cout << "  int x();     // 看起來像變數初始化？不！是函式宣告！\n";
    cout << "  int x{};     // ✅ 這才是變數初始化（x = 0）\n\n";

    // 實際示範
    // int x();   // 函式宣告
    int y{};      // 值初始化 → 0
    cout << "  int y{};  → y = " << y << "（值初始化為 0）\n";
}


// ================================================================
//
//  第 13 章：實戰模式與最佳實踐
//
// ================================================================

void chapter13() {
    title("第 13 章：實戰模式與最佳實踐");

    sub("模式 1：range-based for 搭配 auto");
    {
        vector<string> names = {"Alice", "Bob", "Charlie"};

        cout << "  // 唯讀遍歷：\n";
        cout << "  for (const auto& name : names) { ... }\n";
        for (const auto& name : names)
            cout << "    " << name << "\n";

        cout << "\n  // 修改遍歷：\n";
        cout << "  for (auto& name : names) { name += \"!\"; }\n";
        for (auto& name : names) name += "!";
        for (const auto& name : names)
            cout << "    " << name << "\n";
        cout << "\n";
    }

    sub("模式 2：auto 搭配 Lambda");
    {
        auto add = [](int a, int b) { return a + b; };
        auto greet = [](const string& name) { return "Hello, " + name; };
        cout << "  auto add = [](int a, int b) { return a + b; };\n";
        cout << "  add(3, 4) = " << add(3, 4) << "\n";
        cout << "  greet(\"World\") = " << greet("World") << "\n\n";
    }

    sub("模式 3：auto 搭配 make_unique / make_shared");
    {
        auto ptr = make_unique<vector<int>>(initializer_list<int>{1, 2, 3});
        cout << "  auto ptr = make_unique<vector<int>>({1,2,3});\n";
        cout << "  比寫 unique_ptr<vector<int>> ptr = ... 簡潔多了\n\n";
    }

    sub("模式 4：結構化綁定（C++17）搭配 auto");
    {
        map<string, int> scores = {{"Alice", 95}, {"Bob", 87}};
        cout << "  for (const auto& [name, score] : scores) { ... }\n";
        for (const auto& [name, score] : scores)
            cout << "    " << name << ": " << score << "\n";
        cout << "\n";
    }

    sub("模式 5：{} 初始化 STL 容器");
    {
        cout << "  vector<int> v{1,2,3};\n";
        cout << "  map<string,int> m{{\"a\",1},{\"b\",2}};\n";
        cout << "  pair<int,string> p{42, \"answer\"};\n";
        cout << "  array<int,3> a{10, 20, 30};\n\n";
    }

    sub("何時用 auto vs 明確型別？");
    cout << "  ✅ 用 auto：型別明顯、迭代器、Lambda、make_unique 回傳值\n";
    cout << "  ❌ 不用 auto：型別不明顯時（auto x = getValue() ← 什麼型別？）\n";
    cout << "  ❌ 不用 auto：需要特定型別轉換時（auto x = 0 而非 size_t x = 0）\n\n";

    sub("何時用 {} vs ()？");
    cout << "  ✅ 用 {}：初始化基本型別（防窄化）、聚合類別、想要安全\n";
    cout << "  ✅ 用 ()：指定容器大小如 vector(100, 0)、避免 initializer_list 搶匹配\n";
    cout << "  ✅ 用 {}：消除 Most Vexing Parse 歧義\n";
}


// ================================================================
//
//  第 14 章：常見錯誤與陷阱大全
//
// ================================================================

void chapter14() {
    title("第 14 章：常見錯誤與陷阱大全");

    sub("陷阱 1：auto 推導字串字面值為 const char*");
    cout << "  auto s = \"Hello\";  → const char*（不是 string！）\n";
    cout << "  解法：auto s = string(\"Hello\"); 或 using namespace std::literals;\n";
    cout << "         auto s = \"Hello\"s;  ← C++14 字串字面值後綴\n\n";

    sub("陷阱 2：auto 忽略 const 和引用");
    cout << "  const int cx = 42;\n";
    cout << "  auto a = cx;  → int（不是 const int！）\n";
    cout << "  解法：const auto a = cx; 或 auto& a = cx;\n\n";

    sub("陷阱 3：decltype 加括號變引用");
    cout << "  int x = 42;\n";
    cout << "  decltype(x)   → int\n";
    cout << "  decltype((x)) → int&  ← 加括號就變引用！\n\n";

    sub("陷阱 4：decltype(auto) return (x) 懸空引用");
    cout << "  decltype(auto) func() {\n";
    cout << "      int x = 42;\n";
    cout << "      return (x);  → int& → 回傳局部變數引用 → 💥 懸空！\n";
    cout << "      return x;    → int  → 安全 ✅\n";
    cout << "  }\n\n";

    sub("陷阱 5：vector{} vs vector()");
    cout << "  vector<int> v1(5, 10); → 5 個 10：[10,10,10,10,10]\n";
    cout << "  vector<int> v2{5, 10}; → 2 個元素：[5, 10]\n";
    cout << "  完全不同的結果！\n\n";

    sub("陷阱 6：auto + {} = initializer_list");
    cout << "  auto x = {1, 2, 3};  → initializer_list<int>（不是 vector！）\n";
    cout << "  auto x{42};          → C++17 前是 initializer_list，C++17 後是 int\n\n";

    sub("陷阱 7：initializer_list 建構子搶匹配");
    cout << "  類別同時有 initializer_list 和一般建構子時，\n";
    cout << "  {} 總是優先匹配 initializer_list 版本\n";
    cout << "  想用一般建構子 → 用 ()\n\n";

    sub("陷阱 8：窄化轉換只有 {} 會報錯");
    cout << "  int x = 3.14;    → 靜默截斷（沒有警告）\n";
    cout << "  int x{3.14};     → 編譯錯誤 ✅（{} 保護你）\n\n";

    cout << "=== 安全檢查清單 ===\n";
    cout << "  ✅ 用 auto 時注意型別是否符合預期\n";
    cout << "  ✅ 需要 const/引用時明確寫出 const auto& / auto&\n";
    cout << "  ✅ decltype 避免不必要的括號\n";
    cout << "  ✅ decltype(auto) return 語句不要加括號\n";
    cout << "  ✅ 容器大小用 ()，元素值用 {}\n";
    cout << "  ✅ 用 {} 防止窄化轉換\n";
}


// ================================================================
//
//  第 15 章：LeetCode 實戰 + 日常實務範例
//
// ================================================================
//
// 說明：型別推導本身是「語言機制」而非演算法主題，因此不存在
//       「考 auto 的 LeetCode 題」。但下列兩題的標準解法裡，
//       auto（迭代器/範圍 for）與 {}（建構回傳值）是真正會用到、
//       而且換成 C++03 寫法會明顯變醜的地方，故收錄。

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1. Two Sum
//   題目：給定陣列與 target，回傳兩個和為 target 的元素索引。
//   為什麼用到本主題：
//     (a) auto it = seen.find(need);  — 省掉 unordered_map<int,int>::iterator
//     (b) return {it->second, i};     — 用 {} 直接建構 vector<int> 回傳值，
//         不必寫 return vector<int>{...}；這是統一初始化最自然的用途。
// -----------------------------------------------------------------------------
vector<int> twoSum(const vector<int>& nums, int target) {
    unordered_map<int, int> seen;                  // value -> index
    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        int need = target - nums[i];
        auto it = seen.find(need);                 // auto：型別由 find 決定
        if (it != seen.end()) {
            return {it->second, i};                // {} 建構回傳值
        }
        seen[nums[i]] = i;
    }
    return {};                                     // {} 表示空 vector
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 56. Merge Intervals
//   題目：合併所有重疊區間。
//   為什麼用到本主題：
//     (a) const auto& iv : intervals — 範圍 for 搭配 const auto& 避免每圈複製
//         一個 vector<int>（改成 auto 會多一次堆積配置，是常見效能錯誤）。
//     (b) out.push_back({s, e}); — 用 {} 就地建構元素。
// -----------------------------------------------------------------------------
vector<vector<int>> mergeIntervals(vector<vector<int>> intervals) {
    if (intervals.empty()) return {};
    sort(intervals.begin(), intervals.end());

    vector<vector<int>> out;
    out.push_back(intervals.front());              // 先放第一段

    for (const auto& iv : intervals) {             // const auto& ← 不複製
        auto& last = out.back();                   // auto& ← 要就地改，不能複製
        if (iv[0] <= last[1]) {
            last[1] = max(last[1], iv[1]);         // 重疊 → 延長
        } else {
            out.push_back({iv[0], iv[1]});         // {} 建構新區間
        }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】載入設定檔並套用預設值
//   情境：服務啟動時讀 key=value 設定，缺少的欄位要套預設值。
//   為什麼用到本主題：
//     (a) 預設值表用 {} 初始化 map，一眼看得出內容。
//     (b) 走訪用 const auto&，避免每圈複製 std::string。
//     (c) 逾時秒數刻意用 int{} 檢查 — 若來源是浮點數會在編譯期就被擋下
//         （窄化轉換），這正是 {} 比 = 安全的地方。
// -----------------------------------------------------------------------------
map<string, string> loadConfig(const vector<string>& lines) {
    // {} 讓「預設設定」像資料表一樣可讀
    map<string, string> cfg{
        {"host",    "127.0.0.1"},
        {"port",    "8080"},
        {"timeout", "30"},
        {"debug",   "false"}
    };

    for (const auto& line : lines) {               // const auto& ← 不複製字串
        if (line.empty() || line[0] == '#') continue;   // 跳過註解
        auto eq = line.find('=');                  // auto → std::string::size_type
        if (eq == string::npos) continue;          // 格式錯誤就略過
        cfg[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return cfg;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】以 decltype 讓輔助函式自動跟隨容器型別
//   情境：統計 log 各等級出現次數，之後 log 結構若改型別，統計函式不必改。
//   為什麼用到本主題：
//     回傳型別寫 decltype(...)，型別的唯一來源是容器本身；
//     日後把 map 換成 unordered_map，這個函式一個字都不用動。
// -----------------------------------------------------------------------------
using LogCounter = map<string, int>;

// 後置回傳型別（C++11）：回傳型別由參數推導而來
auto countLogLevels(const vector<string>& lines) -> LogCounter {
    LogCounter counter;
    for (const auto& line : lines) {
        auto l = line.find('[');
        if (l == string::npos) continue;
        auto r = line.find(']', l + 1);
        if (r == string::npos) continue;
        ++counter[line.substr(l + 1, r - l - 1)];
    }
    return counter;
}

void chapter15() {
    title("第 15 章：LeetCode 實戰 + 日常實務範例");

    sub("LeetCode 1. Two Sum");
    {
        vector<int> nums{2, 7, 11, 15};
        auto ans = twoSum(nums, 9);
        cout << "  nums = [2,7,11,15], target = 9 → [";
        for (size_t i = 0; i < ans.size(); ++i)
            cout << ans[i] << (i + 1 < ans.size() ? "," : "");
        cout << "]\n\n";
    }

    sub("LeetCode 56. Merge Intervals");
    {
        vector<vector<int>> iv{{1, 3}, {2, 6}, {8, 10}, {15, 18}};
        auto merged = mergeIntervals(iv);
        cout << "  輸入 [[1,3],[2,6],[8,10],[15,18]]\n";
        cout << "  合併 → ";
        for (const auto& seg : merged)
            cout << "[" << seg[0] << "," << seg[1] << "] ";
        cout << "\n\n";
    }

    sub("日常實務 1：載入設定檔（缺項自動套預設）");
    {
        vector<string> file{
            "# 這行是註解",
            "host=192.168.1.50",
            "port=9090",
            "壞掉的行沒有等號"
        };
        auto cfg = loadConfig(file);
        for (const auto& kv : cfg)
            cout << "  " << kv.first << " = " << kv.second << "\n";
        cout << "  ← timeout/debug 未出現在檔案中，套用了預設值\n\n";
    }

    sub("日常實務 2：統計 log 等級（後置回傳型別）");
    {
        vector<string> logs{
            "2026-07-19 10:00:01 [INFO] service started",
            "2026-07-19 10:00:05 [WARN] cache miss rate high",
            "2026-07-19 10:00:07 [ERROR] db connect failed",
            "2026-07-19 10:00:09 [ERROR] db connect failed",
            "沒有等級標籤的一行"
        };
        auto counter = countLogLevels(logs);
        for (const auto& kv : counter)
            cout << "  " << kv.first << " : " << kv.second << " 次\n";
        cout << "\n";
    }

    sub("{} 防窄化：實務上真正救命的地方");
    cout << "  int port{8080};      ✅ 正常\n";
    cout << "  int port{8080.5};    ❌ 編譯錯誤 — 設定值誤植為浮點數，編譯期就抓到\n";
    cout << "  int port = 8080.5;   ⚠️ 靜默截斷成 8080，錯誤被藏起來直到上線\n";
}


// ================================================================
//
//  main：按順序執行所有章節
//
// ================================================================
int main() {
    cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    cout << "║  型別推導與初始化 — 完整教學（14 章）                           ║\n";
    cout << "╚══════════════════════════════════════════════════════════════════╝\n";

    chapter1();
    chapter2();
    chapter3();
    chapter4();
    chapter5();
    chapter6();
    chapter7();
    chapter8();
    chapter9();
    chapter10();
    chapter11();
    chapter12();
    chapter13();
    chapter14();
    chapter15();

    cout << "\n";
    cout << "================================================================\n";
    cout << "  恭喜！你已經完整學完型別推導與初始化的所有核心知識。\n";
    cout << "  建議順序複習：\n";
    cout << "    1. auto 基本用法 → 2. auto 推導規則（const/引用/陣列）\n";
    cout << "    3. decltype → 4. 括號陷阱 → 5. decltype(auto)\n";
    cout << "    6. 後置回傳型別\n";
    cout << "    7. {} 統一初始化 → 8. 防窄化 → 9. initializer_list\n";
    cout << "    10. {} vs () → 11. auto+{} → 12. Most Vexing Parse\n";
    cout << "    13. 實戰模式 → 14. 陷阱大全\n";
    cout << "    15. LeetCode 實戰 + 日常實務\n";
    cout << "================================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 1 單元：型別推導與初始化 summary.cpp" -o unit1_summary

// === 預期輸出 ===
// ╔══════════════════════════════════════════════════════════════════╗
// ║  型別推導與初始化 — 完整教學（14 章）                           ║
// ╚══════════════════════════════════════════════════════════════════╝
//
// ================================================================
//   第 1 章：auto — 讓編譯器幫你推導型別
// ================================================================
//
// --- 基本型別推導 ---
//   auto i = 42;     → int,       i = 42
//   auto d = 3.14;   → double,    d = 3.14
//   auto f = 3.14f;  → float,     f = 3.14
//   auto c = 'A';    → char,       c = A
//   auto b = true;   → bool,       b = true
//   auto ll = 100LL; → long long, ll = 100
//
// --- 字串型別推導（注意！） ---
//   auto str1 = "Hello";         → const char* ← 不是 string！
//   auto str2 = string("World"); → string
//
// --- auto 最常見的用途：簡化迭代器 ---
//   // 沒有 auto（C++03 風格）：
//   // map<string, vector<int>>::iterator it = data.begin();
//   // 用 auto（C++11）：
//   // auto it = data.begin();  ← 簡潔多了！
//
//   Alice 的成績: 90 85 88 
//   Bob 的成績: 78 82 80 
//
// --- auto 不能用的地方 ---
//   ❌ auto x;               沒有初始化值
//   ❌ auto arr[5] = {...};   不能用於陣列宣告
//   ❌ class C { auto v=1; }; 不能用於非靜態成員
//   ❌ void func(auto x);     C++11/14 不能用於參數（C++20 才行）
//
// ================================================================
//   第 2 章：auto 的推導規則
// ================================================================
//
// --- 規則 1：忽略頂層 const（因為是複製） ---
//   auto a1 = cx(const int);  → a1 是 int，可修改
//   auto a2 = crx(const int&); → a2 是 int，可修改
//
// --- 規則 2：忽略引用（因為是複製） ---
//   auto a3 = rx(int&); → a3 是 int，修改 a3 不影響 x
//   x = 100（不變）, a3 = 777
//
// --- 明確保留 const 或引用 ---
//   auto& ra = x;        → int&，修改 ra 就是修改 x
//   x = 555
//   const auto ca = x;   → const int，不能修改
//   const auto& cra = x; → const int&，不能修改
//   auto& ra2 = cx;      → const int&（底層 const 保留！）
//
// --- 規則 3：指標被保留 ---
//   auto p = ptr;  → int*（指標保留）
//   *p1 = 555
//
// --- 規則 4：陣列退化為指標 ---
//   auto arrAuto = arr;   → int*（退化，sizeof = 8）
//   auto& arrRef = arr;   → int(&)[5]（保留，sizeof = 20）
//
// --- 推導規則總結表 ---
//   ┌──────────────────┬───────────────────┬─────────────┐
//   │    原始型別       │ auto x = ...      │ 推導結果    │
//   ├──────────────────┼───────────────────┼─────────────┤
//   │ int              │ auto a = x;       │ int         │
//   │ const int        │ auto a = cx;      │ int         │
//   │ int&             │ auto a = rx;      │ int         │
//   │ const int&       │ auto a = crx;     │ int         │
//   │ int*             │ auto a = ptr;     │ int*        │
//   │ int[5]           │ auto a = arr;     │ int*        │
//   ├──────────────────┼───────────────────┼─────────────┤
//   │ 要保留 const     │ const auto a = x; │ const int   │
//   │ 要保留引用       │ auto& a = x;      │ int&        │
//   │ 要保留陣列       │ auto& a = arr;    │ int(&)[5]   │
//   └──────────────────┴───────────────────┴─────────────┘
//
// ================================================================
//   第 3 章：decltype — 查詢任何表達式的型別
// ================================================================
//
// --- auto vs decltype 的關鍵差異 ---
//   原始：const int cx = 20;
//     auto a = cx;       → int（忽略 const）
//     decltype(cx) d;    → const int（保留！）
//
//   原始：int& rx = x;
//     auto a = rx;       → int（忽略引用）
//     decltype(rx) d = x; → int&（保留！）
//
//   declRef = 222 後，x = 222（證明 declRef 是引用）
//
// --- decltype 與函式回傳型別 ---
//   decltype(getValue())     → int
//   decltype(getGlobalRef()) → int&
//   decltype(getConstRef())  → const int&
//   注意：函式不會被實際呼叫！
//
// --- decltype 保留陣列型別 ---
//   sizeof(arr)     = 20
//   sizeof(arr2)    = 20（decltype 保留陣列）
//   sizeof(arrAuto) = 8（auto 退化為指標）
//
// --- decltype 與容器元素 ---
//   decltype(vec[0]) → int&
//   修改 elem 後 vec[0] = 100（因為是引用）
//
// --- decltype 定義型別別名 ---
//   using RecordType = decltype(records); → 省去寫完整型別
//   static_assert 已在編譯期驗證兩個別名的型別正確 ✅
//
// ================================================================
//   第 4 章：decltype 的括號陷阱
// ================================================================
//
// --- 不加括號 vs 加括號 ---
//   decltype(num)   → int   （變數名 → 宣告型別）
//   decltype((num)) → int&  （左值表達式 → T&）
//   d2 = 999 後，num = 999（因為 d2 是引用！）
//
// --- 為什麼加括號就變引用了？ ---
//   decltype 有兩條推導規則：
//     規則 A：如果放的是「變數名」→ 回傳宣告型別
//     規則 B：如果放的是「表達式」→ 根據值類別決定
//
//   num   → 是變數名 → 走規則 A → int
//   (num) → 加括號後變成表達式 → 走規則 B
//         → (num) 是左值 → T& → int&
//
// --- 這個陷阱在 return 時最危險 ---
//   decltype(auto) func() {
//       int x = 42;
//       return x;    → int  （安全，回傳值）
//       return (x);  → int& （危險！回傳局部變數的引用 → 懸空！）
//   }
// --- 表達式的值類別決定 decltype 結果 ---
//   decltype(a + b)  → int  （prvalue → T）
//   decltype(a = b)  → int& （lvalue → T&）
//   decltype(a++)    → int  （prvalue → T）
//   decltype(++a)    → int& （lvalue → T&）
//
// ================================================================
//   第 5 章：decltype(auto) — 完美保留型別
// ================================================================
//
// --- 變數推導比較 ---
//   auto a1 = cx;            → int (忽略 const)
//   decltype(auto) d1 = cx;  → const int (保留!)
//
//   auto a2 = rx;            → int (忽略引用)
//   decltype(auto) d2 = rx;  → int& (保留!)
//
//   d2 = 888 後，x = 888（d2 是引用）
//
// --- 函式回傳型別比較 ---
//   auto 回傳:            永遠是值（忽略引用）
//   decltype(auto) 回傳:  保留引用！
//
//   decltypeReturnRef() = 500;
//   g_value = 500（成功修改！因為回傳 int&）
//
// --- 括號陷阱在 decltype(auto) 中更危險！ ---
//   decltype(auto) safe   = num;   → int
//   decltype(auto) danger = (num); → int& ← 陷阱！
//   在 return 中：return (x) 會回傳引用 → 可能懸空！
//
// --- 泛型轉發函式 ---
//   template<typename F>
//   auto callWithAuto(F f)           { return f(); } // 丟失引用
//   decltype(auto) callPerfect(F f)  { return f(); } // 保留引用
//
// ================================================================
//   第 6 章：後置回傳型別
// ================================================================
//
// --- auto func() -> decltype(...) ---
//   add(1, 2)     = 3 (int)
//   add(1, 2.5)   = 3.5 (double)
//   add(1.5f, 2.5)= 4 (double)
//
//   // C++11 語法：
//   auto add(T a, U b) -> decltype(a + b) { return a + b; }
//
//   // C++14 簡化：
//   auto add(T a, U b) { return a + b; }  // 自動推導
//
// ================================================================
//   第 7 章：統一初始化 {} — 大括號語法
// ================================================================
//
// --- C++11 之前的初始化方式（混亂） ---
//   int x = 42;         // 複製初始化
//   int x(42);          // 直接初始化
//   int arr[] = {1,2};  // 聚合初始化（只有陣列能用 {}）
//   每種型別有不同的初始化語法 → 混亂！
//
// --- C++11：{} 統一所有初始化語法 ---
//   int i1{42};   → 42
//   int i2 = {42}; → 42
//   int i3{};     → 0 （值初始化為 0）
//
//   int arr[5]{1,2};   → 1 2 0 0 0 （其餘為 0）
//
//   vector<int> vec{1,2,3,4,5};  ← 直接初始化！
//   map<string,int> m{{"Alice",95}, {"Bob",87}};
//
//   struct Point { int x; int y; };
//   Point p{10, 20};  → (10, 20)
//
// --- 零初始化 vs 預設初始化 ---
//   Data d1;    → 成員值不確定（危險！）
//   Data d2{};  → x=0, y=0（安全）
//
// ================================================================
//   第 8 章：{} 防止窄化轉換（Narrowing Conversion）
// ================================================================
//
// --- 什麼是窄化轉換？ ---
//   把大範圍的值塞進小範圍的型別，可能丟失資料：
//     double → int     （丟失小數部分）
//     int → char       （可能溢位）
//     long long → int  （可能溢位）
//
// --- () 和 = 不會阻止窄化 ---
//   int x = 3.14159;   → 3 （丟失小數，但編譯通過）
//   int x(3.14159);    → 3 （同上）
//
// --- {} 會在編譯期阻止窄化！ ---
//   int x{3.14159};    → ❌ 編譯錯誤！
//   int x = {3.14159}; → ❌ 編譯錯誤！
//
//   char c{65};        → ✅ 65 在 char 範圍內，合法
//
//   int big = 1000;
//   char d{big};       → ❌ 編譯錯誤（big 不是常量）
//
// --- 窄化轉換完整列表 ---
//   ┌─────────────────────┬────────┬────────┐
//   │ 轉換                │ () / = │   {}   │
//   ├─────────────────────┼────────┼────────┤
//   │ double → int       │   ✅   │   ❌   │
//   │ float → int        │   ✅   │   ❌   │
//   │ int → char         │   ✅   │ 看值   │
//   │ long long → int    │   ✅   │ 看值   │
//   │ int → unsigned     │   ✅   │ 看值   │
//   │ 常量 65 → char     │   ✅   │   ✅   │
//   └─────────────────────┴────────┴────────┘
//   「看值」= 如果是常量且在目標範圍內則允許
//
// ================================================================
//   第 9 章：std::initializer_list
// ================================================================
//
// --- initializer_list 的基本性質 ---
//   initializer_list<int> list = {10, 20, 30, 40};
//   size = 4
//   元素是 const 的（不能修改）
//   底層是臨時陣列，複製 list 只複製指標（輕量！）
//
// --- 自訂類別的 initializer_list 建構子 ---
//   NumberList n1{5, 10}:
//   NumberList(initializer_list, 2 個元素)
//     → 5 10 
//   NumberList n2(5, 10):
//   NumberList(size=5, value=10)
//     → 10 10 10 10 10 
//
// --- 函式參數使用 initializer_list ---
//   sum({1, 2, 3, 4, 5}) = 15
//
// --- 標準容器都支援 initializer_list ---
//   vector<int> v{1,2,3};           // 建構子
//   v.assign({10, 20, 30});          // assign 方法
//   v.insert(v.end(), {40, 50});     // insert 方法
//
// ================================================================
//   第 10 章：{} vs () — 建構子優先順序陷阱
// ================================================================
//
// --- 陷阱：{} 優先匹配 initializer_list 建構子！ ---
//   當類別同時有 initializer_list 建構子和一般建構子時，
//   {} 會優先匹配 initializer_list 版本！
//
// --- vector 的陷阱 ---
//   vector<int> v1(5, 10);  → 10 10 10 10 10 （5 個 10）
//   vector<int> v2{5, 10};  → 5 10 （兩個元素：5 和 10）
//
//   ⚠️ 完全不同的結果！
//
// --- 自訂類別也一樣 ---
//   NumberList n1{5, 10};   → initializer_list 版本
//   NumberList n2(5, 10);   → 一般建構子 (size, value)
//
// --- 規則總結 ---
//   1. {} 優先匹配 initializer_list 建構子
//   2. 如果沒有 initializer_list 建構子 → 匹配一般建構子
//   3. {} 空的情況 → 呼叫預設建構子（不是空 initializer_list）
//   4. 想指定用一般建構子 → 用 ()
//
// --- 建議 ---
//   容器用 {} 初始化元素值：vector<int> v{1,2,3};
//   容器用 () 指定大小/填充：vector<int> v(100, 0);
//
// ================================================================
//   第 11 章：auto + {} 的特殊規則
// ================================================================
//
// --- auto + {} = initializer_list（C++11/14） ---
//   auto list = {1, 2, 3};  → initializer_list<int>
//   size = 3
//
//   auto list = {1, 2.0};   → ❌ 型別不一致（int vs double）
//
// --- C++17 的變化：auto x{42} ---
//   auto x{42};
//     C++11/14 → initializer_list<int>（一個元素）
//     C++17    → int（直接推導為 int）
//
// --- 建議 ---
//   想要 initializer_list → auto list = {1,2,3};
//   想要單一值           → auto x = 42;（用 = 不用 {}）
//
// ================================================================
//   第 12 章：Most Vexing Parse
// ================================================================
//
// --- 問題：() 可能被解析為函式宣告 ---
//   class Timer {};
//   class Widget { Widget(Timer t) {} };
//
//   Widget w(Timer());   // 這是什麼？
//     ❌ 不是建構 Widget 物件！
//     ✅ 被解析為「函式宣告」：
//        w 是一個函式，接收「回傳 Timer 的函式指標」，回傳 Widget
//
// --- 解法：用 {} 消除歧義 ---
//   Widget w1{Timer{}};  // ✅ 明確是物件建構
//   Widget w2(Timer{}); // ✅ 也行（參數用 {}）
//   Widget w3{Timer()};  // ✅ 也行（外層用 {}）
//
// --- 其他常見的 Most Vexing Parse ---
//   int x();     // 看起來像變數初始化？不！是函式宣告！
//   int x{};     // ✅ 這才是變數初始化（x = 0）
//
//   int y{};  → y = 0（值初始化為 0）
//
// ================================================================
//   第 13 章：實戰模式與最佳實踐
// ================================================================
//
// --- 模式 1：range-based for 搭配 auto ---
//   // 唯讀遍歷：
//   for (const auto& name : names) { ... }
//     Alice
//     Bob
//     Charlie
//
//   // 修改遍歷：
//   for (auto& name : names) { name += "!"; }
//     Alice!
//     Bob!
//     Charlie!
//
// --- 模式 2：auto 搭配 Lambda ---
//   auto add = [](int a, int b) { return a + b; };
//   add(3, 4) = 7
//   greet("World") = Hello, World
//
// --- 模式 3：auto 搭配 make_unique / make_shared ---
//   auto ptr = make_unique<vector<int>>({1,2,3});
//   比寫 unique_ptr<vector<int>> ptr = ... 簡潔多了
//
// --- 模式 4：結構化綁定（C++17）搭配 auto ---
//   for (const auto& [name, score] : scores) { ... }
//     Alice: 95
//     Bob: 87
//
// --- 模式 5：{} 初始化 STL 容器 ---
//   vector<int> v{1,2,3};
//   map<string,int> m{{"a",1},{"b",2}};
//   pair<int,string> p{42, "answer"};
//   array<int,3> a{10, 20, 30};
//
// --- 何時用 auto vs 明確型別？ ---
//   ✅ 用 auto：型別明顯、迭代器、Lambda、make_unique 回傳值
//   ❌ 不用 auto：型別不明顯時（auto x = getValue() ← 什麼型別？）
//   ❌ 不用 auto：需要特定型別轉換時（auto x = 0 而非 size_t x = 0）
//
// --- 何時用 {} vs ()？ ---
//   ✅ 用 {}：初始化基本型別（防窄化）、聚合類別、想要安全
//   ✅ 用 ()：指定容器大小如 vector(100, 0)、避免 initializer_list 搶匹配
//   ✅ 用 {}：消除 Most Vexing Parse 歧義
//
// ================================================================
//   第 14 章：常見錯誤與陷阱大全
// ================================================================
//
// --- 陷阱 1：auto 推導字串字面值為 const char* ---
//   auto s = "Hello";  → const char*（不是 string！）
//   解法：auto s = string("Hello"); 或 using namespace std::literals;
//          auto s = "Hello"s;  ← C++14 字串字面值後綴
//
// --- 陷阱 2：auto 忽略 const 和引用 ---
//   const int cx = 42;
//   auto a = cx;  → int（不是 const int！）
//   解法：const auto a = cx; 或 auto& a = cx;
//
// --- 陷阱 3：decltype 加括號變引用 ---
//   int x = 42;
//   decltype(x)   → int
//   decltype((x)) → int&  ← 加括號就變引用！
//
// --- 陷阱 4：decltype(auto) return (x) 懸空引用 ---
//   decltype(auto) func() {
//       int x = 42;
//       return (x);  → int& → 回傳局部變數引用 → 💥 懸空！
//       return x;    → int  → 安全 ✅
//   }
//
// --- 陷阱 5：vector{} vs vector() ---
//   vector<int> v1(5, 10); → 5 個 10：[10,10,10,10,10]
//   vector<int> v2{5, 10}; → 2 個元素：[5, 10]
//   完全不同的結果！
//
// --- 陷阱 6：auto + {} = initializer_list ---
//   auto x = {1, 2, 3};  → initializer_list<int>（不是 vector！）
//   auto x{42};          → C++17 前是 initializer_list，C++17 後是 int
//
// --- 陷阱 7：initializer_list 建構子搶匹配 ---
//   類別同時有 initializer_list 和一般建構子時，
//   {} 總是優先匹配 initializer_list 版本
//   想用一般建構子 → 用 ()
//
// --- 陷阱 8：窄化轉換只有 {} 會報錯 ---
//   int x = 3.14;    → 靜默截斷（沒有警告）
//   int x{3.14};     → 編譯錯誤 ✅（{} 保護你）
//
// === 安全檢查清單 ===
//   ✅ 用 auto 時注意型別是否符合預期
//   ✅ 需要 const/引用時明確寫出 const auto& / auto&
//   ✅ decltype 避免不必要的括號
//   ✅ decltype(auto) return 語句不要加括號
//   ✅ 容器大小用 ()，元素值用 {}
//   ✅ 用 {} 防止窄化轉換
//
// ================================================================
//   第 15 章：LeetCode 實戰 + 日常實務範例
// ================================================================
//
// --- LeetCode 1. Two Sum ---
//   nums = [2,7,11,15], target = 9 → [0,1]
//
// --- LeetCode 56. Merge Intervals ---
//   輸入 [[1,3],[2,6],[8,10],[15,18]]
//   合併 → [1,6] [8,10] [15,18] 
//
// --- 日常實務 1：載入設定檔（缺項自動套預設） ---
//   debug = false
//   host = 192.168.1.50
//   port = 9090
//   timeout = 30
//   ← timeout/debug 未出現在檔案中，套用了預設值
//
// --- 日常實務 2：統計 log 等級（後置回傳型別） ---
//   ERROR : 2 次
//   INFO : 1 次
//   WARN : 1 次
//
// --- {} 防窄化：實務上真正救命的地方 ---
//   int port{8080};      ✅ 正常
//   int port{8080.5};    ❌ 編譯錯誤 — 設定值誤植為浮點數，編譯期就抓到
//   int port = 8080.5;   ⚠️ 靜默截斷成 8080，錯誤被藏起來直到上線
//
// ================================================================
//   恭喜！你已經完整學完型別推導與初始化的所有核心知識。
//   建議順序複習：
//     1. auto 基本用法 → 2. auto 推導規則（const/引用/陣列）
//     3. decltype → 4. 括號陷阱 → 5. decltype(auto)
//     6. 後置回傳型別
//     7. {} 統一初始化 → 8. 防窄化 → 9. initializer_list
//     10. {} vs () → 11. auto+{} → 12. Most Vexing Parse
//     13. 實戰模式 → 14. 陷阱大全
//     15. LeetCode 實戰 + 日常實務
// ================================================================
