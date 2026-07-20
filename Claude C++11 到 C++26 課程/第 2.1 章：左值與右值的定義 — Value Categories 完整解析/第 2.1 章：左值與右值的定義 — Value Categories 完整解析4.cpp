// =============================================================================
//  第 2.1 章 -4  —  自己動手做「值類別偵測器」：decltype + 樣板偏特化
// =============================================================================
//
// 【主題資訊 Information】
//   語法：decltype(expr) 與 decltype((expr))  — 注意多一層括號，語意不同
//   標準版本：decltype 為 C++11；本檔的偵測器只用到 C++11 語法
//   複雜度：全部在編譯期完成，執行期零成本
//   標頭檔：<type_traits> <utility>
//
// 【詳細解釋 Explanation】
//
// 【1. decltype 的兩種模式 —— 為什麼要多包一層括號】
//   decltype 刻意設計成兩種行為，這是本檔能運作的關鍵：
//     (a) decltype(變數名)      → 回報「這個實體被宣告成什麼型別」
//                                 int x;  decltype(x)  就是 int
//     (b) decltype(任意表達式)  → 回報「這個表達式的型別 + 值類別」，
//                                 並用引用來編碼值類別：
//                                   lvalue  → T&
//                                   xvalue  → T&&
//                                   prvalue → T   （非引用）
//   而 (x) 已經不是「變數名」而是「表達式」，所以會走 (b)：
//     int x;
//     decltype(x)   → int      （宣告型別）
//     decltype((x)) → int&     （x 是 lvalue，所以編碼成 int&）
//   ★ 這個差異就是整個偵測器的原理：多寫一對括號，強制走「表達式」模式，
//     把肉眼看不見的值類別，變成型別系統裡看得見的 & 與 &&。
//
// 【2. 用樣板偏特化把「型別」翻譯回「值類別」】
//   拿到 decltype((expr)) 之後，只要判斷它是 T&、T&& 還是非引用即可：
//     value_category<T>    主樣板    → "prvalue"   （沒有引用）
//     value_category<T&>   偏特化    → "lvalue"
//     value_category<T&&>  偏特化    → "xvalue"
//   編譯器做多載解析時，偏特化比主樣板更「特殊」，會優先選中，
//   所以 int& 命中第二個、int&& 命中第三個、int 落回主樣板。
//   整個判斷在編譯期完成，執行期只是印出一個字串常數。
//
// 【概念補充 Concept Deep Dive】
//   ★ 引用折疊（reference collapsing）—— C++11，本檔的隱藏主角
//     C++ 不允許「引用的引用」直接出現在原始碼裡，但樣板推導與 typedef
//     可以間接產生它。標準規定了折疊規則，只有一條要背：
//         & + &  = &        （左值引用 + 左值引用 → 左值引用）
//         & + && = &
//         && + & = &
//         && + && = &&      ← 只有這一種會得到右值引用
//     一句話記法：「只要出現任何一個 &，結果就是 &；唯有 && 加 && 才是 &&」。
//     本機實測（std::is_same，g++ 15.2）四條規則全部驗證為 true，
//     程式最後一段會實際印出來。
//
//   ★ 這條規則為什麼重要：它是 std::forward 與「萬用引用」的全部機制。
//     template<class T> void f(T&& arg);
//       傳左值 int  → T 推導為 int&  → T&& = int& && = int&   → 綁左值
//       傳右值 int  → T 推導為 int   → T&& = int&&            → 綁右值
//     同一個 T&& 語法，因為折疊規則而能同時吃左值與右值 —— 這就是為什麼
//     它叫 forwarding reference（轉發引用），而不是普通的右值引用。
//     注意：只有「需要推導的 T&&」才是轉發引用；
//     std::vector<T>&& 或已知型別的 std::string&& 都是純右值引用，不折疊。
//
// 【注意事項 Pay Attention】
//   1. decltype(x) 與 decltype((x)) 不同，這是刻意的語言設計，不是陷阱。
//      寫回傳型別時尤其要小心：
//        decltype(auto) f() { int x = 1; return (x); } // 回傳 int& → 懸空引用！
//   2. 本偵測器把值類別編碼成「T& / T&& / T」，這正是 decltype 的規範，
//      不是巧合，也不是實作細節，任何合規編譯器結果都相同。
//   3. 偵測器回報的是「表達式的值類別」，不是「變數宣告的型別」。
//      int&& r = 42; 之後 PRINT(r) 會印 lvalue —— 這正確，因為 r 這個
//      表達式確實是 lvalue（見 -3 檔）。
//   4. 巨集參數用 (expr) 包起來很重要，否則含逗號的表達式會被當成兩個引數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decltype 與引用折疊
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. decltype(x) 和 decltype((x)) 有什麼差別？
//     答：decltype(x) 回報變數 x 的宣告型別（int）；decltype((x)) 把 (x)
//         當成表達式，回報型別 + 值類別，x 是 lvalue 所以得到 int&。
//         這正是「值類別偵測器」的原理：用引用把值類別編碼進型別系統。
//     追問：這在實務上什麼時候會咬人？
//         → decltype(auto) f() { int x = 1; return (x); }
//           多的那對括號讓回傳型別變成 int&，回傳了區域變數的引用 → 懸空。
//
// 🔥 Q2. 引用折疊有哪幾條規則？為什麼需要它？
//     答：& & → &、& && → &、&& & → &、&& && → &&。
//         只有「&& 加 &&」得到右值引用，其餘一律折疊成左值引用。
//         需要它是因為樣板推導會產生「引用的引用」，而 T&& 之所以能同時
//         接受左值與右值（forwarding reference），完全靠這條規則。
//     追問：那 std::vector<T>&& 是 forwarding reference 嗎？
//         → 不是。必須是「該函式樣板自己要推導的 T&&」才算。
//           std::vector<T>&& 的 T 不在頂層，是純右值引用，不會折疊。
//
// ⚠️ 陷阱. int&& r = 42; 之後，偵測器對 r 印出什麼？
//     答：印出 lvalue。因為偵測的是「r 這個表達式」的值類別，而具名變數
//         永遠是 lvalue；r 的『型別』雖然是 int&&，那是另一回事。
//     為什麼會錯：直覺會認為「宣告成右值引用 → 它就是右值」，把型別和
//         值類別混為一談。記住：型別看宣告，值類別看表達式怎麼被使用。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <type_traits>
#include <string>
#include <utility>

// 利用模板推導規則判斷值類別
// 原理：decltype((expr)) 會把值類別編碼成引用 —— lvalue→T&、xvalue→T&&、prvalue→T
template<typename T>
struct value_category {
    static constexpr const char* value = "prvalue";   // 主樣板：非引用
};

template<typename T>
struct value_category<T&> {
    static constexpr const char* value = "lvalue";    // 偏特化：左值引用
};

template<typename T>
struct value_category<T&&> {
    static constexpr const char* value = "xvalue";    // 偏特化：右值引用
};

// 巨集：印出表達式的值類別
// 注意 (expr) 外面那層括號 —— 少了它就會退化成 decltype(變數名) 模式
#define PRINT_VALUE_CATEGORY(expr) \
    std::cout << "  " << #expr << " is " \
              << value_category<decltype((expr))>::value << "\n"

std::string make_string() { return "temp"; }
std::string& get_ref(std::string& s) { return s; }
std::string&& get_rref(std::string& s) { return std::move(s); }  // 回傳 T&& → xvalue

int main() {
    int x = 42;
    int& r = x;
    std::string s = "Hello";

    std::cout << "=== 基本值類別偵測 ===\n";
    PRINT_VALUE_CATEGORY(x);              // lvalue
    PRINT_VALUE_CATEGORY(r);              // lvalue
    PRINT_VALUE_CATEGORY(42);             // prvalue
    PRINT_VALUE_CATEGORY(x + 1);          // prvalue
    PRINT_VALUE_CATEGORY(std::move(x));   // xvalue
    PRINT_VALUE_CATEGORY(make_string());  // prvalue
    PRINT_VALUE_CATEGORY(get_ref(s));     // lvalue
    PRINT_VALUE_CATEGORY(std::move(s));   // xvalue

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== 容易搞錯的幾個 ===\n";
    int&& rr = 42;
    PRINT_VALUE_CATEGORY(rr);             // lvalue！具名右值引用是左值
    PRINT_VALUE_CATEGORY("Hello");        // lvalue！字串字面值是左值
    PRINT_VALUE_CATEGORY(get_rref(s));    // xvalue（函式回傳 T&&）
    int arr[3] = {1, 2, 3};
    PRINT_VALUE_CATEGORY(arr[0]);         // lvalue（陣列元素）
    int y = 10;
    PRINT_VALUE_CATEGORY(++y);            // lvalue（回傳引用）
    PRINT_VALUE_CATEGORY(y++);            // prvalue（回傳舊值副本）

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== decltype(x) vs decltype((x)) ===\n";
    std::cout << std::boolalpha;
    std::cout << "  decltype(x)   是 int  ? "
              << std::is_same<decltype(x), int>::value  << "\n";
    std::cout << "  decltype((x)) 是 int& ? "
              << std::is_same<decltype((x)), int&>::value << "\n";
    std::cout << "  → 多一對括號就從『宣告型別』切換成『表達式型別 + 值類別』\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== 引用折疊規則（本機實測驗證）===\n";
    using L = int&;
    using R = int&&;
    std::cout << "  &  +  &  = &   : " << std::is_same<L&,  int&>::value  << "\n";
    std::cout << "  &  +  && = &   : " << std::is_same<L&&, int&>::value  << "\n";
    std::cout << "  && +  &  = &   : " << std::is_same<R&,  int&>::value  << "\n";
    std::cout << "  && +  && = &&  : " << std::is_same<R&&, int&&>::value << "\n";
    std::cout << "  → 只要出現任何一個 &，結果就是 &；唯有 && + && 才是 &&\n";
    std::cout << "  → 這條規則就是 forwarding reference（T&&）能同時吃左右值的原因\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.1 章：左值與右值的定義 — Value Categories 完整解析4.cpp" -o detector

// === 預期輸出 ===
// === 基本值類別偵測 ===
//   x is lvalue
//   r is lvalue
//   42 is prvalue
//   x + 1 is prvalue
//   std::move(x) is xvalue
//   make_string() is prvalue
//   get_ref(s) is lvalue
//   std::move(s) is xvalue
//
// === 容易搞錯的幾個 ===
//   rr is lvalue
//   "Hello" is lvalue
//   get_rref(s) is xvalue
//   arr[0] is lvalue
//   ++y is lvalue
//   y++ is prvalue
//
// === decltype(x) vs decltype((x)) ===
//   decltype(x)   是 int  ? true
//   decltype((x)) 是 int& ? true
//   → 多一對括號就從『宣告型別』切換成『表達式型別 + 值類別』
//
// === 引用折疊規則（本機實測驗證）===
//   &  +  &  = &   : true
//   &  +  && = &   : true
//   && +  &  = &   : true
//   && +  && = &&  : true
//   → 只要出現任何一個 &，結果就是 &；唯有 && + && 才是 &&
//   → 這條規則就是 forwarding reference（T&&）能同時吃左右值的原因
