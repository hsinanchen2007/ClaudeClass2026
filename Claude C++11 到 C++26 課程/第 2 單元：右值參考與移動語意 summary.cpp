// =============================================================================
//  第 2 單元：右值參考與移動語意 summary.cpp  —  完整規則書
// =============================================================================
//
// 【主題資訊 Information】
//   核心語法與標準版本（皆以 -pedantic-errors 實測驗證）：
//     右值參考 T&&                      C++11
//     std::move / std::forward          C++11（<utility>）
//     移動建構子 / 移動賦值運算子         C++11
//     noexcept                          C++11
//     Rule of Five                      C++11 起的慣例
//     保證的複製省略（guaranteed copy elision）C++17
//     value category 正式定義 gl/pr/x   C++11
//   標頭檔  ：<utility>（move/forward/swap）、<type_traits>（is_*_constructible）
//   複雜度  ：移動通常為 O(1)（搬指標）；深拷貝為 O(N)
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 移動語意要解決什麼：一次沒有必要的深拷貝】
//   C++11 之前，函式回傳容器、或把暫時物件放進 vector，
//   都可能發生「把整塊堆積資料複製一份，然後立刻把原本那份銷毀」。
//   複製的目的是「兩邊都要能用」，但暫時物件根本沒有下一個使用者 ——
//   那次複製純屬浪費。
//   移動語意的洞見是：如果來源即將消亡，就不必複製內容，
//   直接「把指標搬過來、把來源設成空」即可 —— O(N) 變成 O(1)。
//
// 【2. 右值參考 T&& 是什麼：一個「可以安全掏空」的標記】
//   T&& 只能綁定到右值（將亡值/純右值），而右值的共同特徵是「沒有人會再用它」。
//   因此當重載決議選中 T&& 版本時，編譯器等於在告訴你：
//   「這個物件的資源你可以拿走，沒人會發現。」
//   這就是為什麼移動建構子的參數是 T&& 而不是 const T& ——
//   const T& 無法修改來源，也就無法把來源設成空。
//
// 【3. 最重要的一條規則：有名字的右值參考是左值】
//       void f(std::string&& s) {   // s 的「型別」是 string&&
//           g(s);                   // 但 s 這個「表達式」是 lvalue → 呼叫 g(const&)
//           g(std::move(s));        // 要傳給 g(&&) 必須再 move 一次
//       }
//   為什麼要這樣設計？因為 s 有名字，在函式內可能被使用多次。
//   若它自動當右值，第一次呼叫就把它掏空了，第二行就會讀到空物件。
//   標準的選擇是「安全優先」：想放棄它，你必須明寫 std::move。
//
// 【4. std::move 什麼都沒有移動】
//   std::move 只是一個 static_cast<T&&>，是純粹的型別轉換，
//   編譯後不產生任何指令。真正搬東西的是「被它選中的那個移動建構子/賦值運算子」。
//   若型別沒有移動操作，std::move 會安靜地退回複製 —— 這是常見的效能幻覺來源。
//
// 【5. noexcept 為什麼值百倍效能】
//   std::vector 擴容時要把舊元素搬到新記憶體。
//   若移動建構子「可能拋例外」，搬到一半失敗就會兩邊都毀損、無法回復。
//   因此 vector 只在移動建構子標了 noexcept 時才敢用移動，否則退回複製
//   （這是 std::move_if_noexcept 的策略）。
//   結論：移動建構子與移動賦值「幾乎必須」標 noexcept，否則等於白寫。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 五種 value category 的完整關係
//         expression
//         ├── glvalue（有身分，可定位）
//         │   ├── lvalue  ：具名變數、回傳 T& 的函式呼叫、v[i]
//         │   └── xvalue  ：std::move(x)、回傳 T&& 的函式呼叫
//         └── rvalue（可被移動）
//             ├── xvalue  ：（同上，同時屬於 glvalue 與 rvalue）
//             └── prvalue ：字面值 42、a+b、回傳 T 的函式呼叫
//     兩個判準：「有沒有身分（identity）」與「可不可以被移動（movable）」。
//     xvalue 之所以特殊，正因為它兩者皆是 —— 有位址，但已被標記為可掏空。
//
// (B) 移動後的來源物件處於什麼狀態
//     標準規定：標準庫型別在被移動後處於「有效但未指定（valid but unspecified）」。
//     「有效」代表你可以安全地銷毀它、可以賦新值、可以呼叫無前提條件的操作
//     （例如 clear()、empty()）。
//     「未指定」代表你不能假設它的內容 —— 常見實作會讓 string 變空，
//     但那不是標準保證，不同實作／不同長度（SSO）都可能不同。
//     實務準則：移動後的物件，除了「重新賦值」或「銷毀」之外不要碰。
//
// (C) 為什麼成員的初始化順序是「宣告順序」
//     成員初始化列表的書寫順序不影響實際順序 —— 一律依成員在類別中的宣告順序。
//     這是為了讓解構順序（宣告順序的反序）永遠是確定的。
//     實務後果非常嚴重：
//         char*  m_data;
//         size_t m_len;
//         Foo(const char* s) : m_len(strlen(s)), m_data(new char[m_len+1]) {}
//     這裡 m_data 先被初始化，而它用到的 m_len 此時尚未初始化 ——
//     讀到不確定的值，配置出錯誤大小的緩衝區。
//     正確作法是讓宣告順序與依賴順序一致（本檔的類別即為此排列，請勿調換）。
//
// (D) Rule of Three / Five / Zero
//     Rule of Three（C++98）：需要自訂解構子、複製建構子、複製賦值其中之一，
//                             通常三個都要自訂。
//     Rule of Five（C++11）：再加上移動建構子與移動賦值。
//     Rule of Zero（現代首選）：讓成員自己管理資源（用 std::string、vector、
//                             unique_ptr），類別本身一個都不用寫 —— 最不容易出錯。
//     另有一條連動規則：自訂解構子或複製操作，會「抑制」移動操作的自動生成，
//     此時 std::move 會安靜退回複製（見本檔第 9、10 章）。
//
// 【注意事項 Pay Attention】
//   1. 有名字的右值參考是左值；要繼續傳遞其右值性必須再寫一次 std::move。
//   2. std::move 不移動任何東西，它只是 static_cast<T&&>，執行期零成本。
//   3. 移動建構子與移動賦值務必標 noexcept，否則 vector 擴容時會退回複製。
//   4. 被移動後的物件處於「有效但未指定」狀態，除了重新賦值或銷毀外不要使用；
//      不可假設它一定變成空字串或 0 —— 那是實作細節，非標準保證。
//   5. 成員初始化順序依「宣告順序」，與初始化列表的書寫順序無關。
//      本檔的類別成員刻意排成「長度在前、指標在後」，請勿調換（會造成未初始化讀取）。
//   6. 移動賦值必須處理自我賦值（self-assignment），否則可能先釋放再使用自己的資源。
//   7. 不要對 const 物件期待移動：const T&& 無法掏空來源，會安靜退回複製。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】右值參考與移動語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 到底做了什麼？
//     答：什麼都沒做。它是 static_cast<remove_reference_t<T>&&>，
//         純粹把表達式的 value category 轉成右值，編譯後不產生任何指令。
//         真正搬資源的是被它選中的移動建構子或移動賦值運算子。
//     追問：如果型別沒有移動建構子呢？→ 重載決議會退回 const T&（複製），
//         程式仍然正確，只是完全沒有加速 —— 這是最常見的「以為有優化」幻覺。
//
// 🔥 Q2. 為什麼「有名字的右值參考」是左值？
//     答：因為它有名字，在函式內可能被使用多次。若它自動當右值，
//         第一次使用就會把資源掏空，後續使用就讀到空物件。
//         標準選擇安全優先：要放棄它，必須明寫 std::move。
//     追問：那轉發函式怎麼辦？→ 用 std::forward<T>(arg)，
//         它會依模板推導出的 T 決定要不要轉成右值，這就是完美轉發。
//
// 🔥 Q3. 為什麼移動建構子一定要標 noexcept？
//     答：vector 擴容需要把舊元素搬到新緩衝區。若移動途中可能拋例外，
//         搬一半失敗就無法回復（舊的已被掏空、新的不完整），
//         因此 vector 只在移動操作標了 noexcept 時才使用移動，否則退回複製。
//     追問：怎麼驗證？→ 用 std::is_nothrow_move_constructible<T>::value 檢查；
//         本檔第 11 章有實測對照，兩者的擴容成本差距非常明顯。
//
// 🔥 Q4. 一個物件被 std::move 之後還能用嗎？
//     答：處於「有效但未指定」狀態。可以安全銷毀、可以重新賦值、
//         可以呼叫沒有前提條件的操作（如 clear()、empty()）。
//         但不可假設其內容 —— 讀取它的值是邏輯錯誤（雖然不是 UB）。
//     追問：那 std::string 被移動後一定是空的嗎？→ 不保證。
//         短字串因 SSO 可能是複製而非移動，內容甚至可能原封不動。
//
// ⚠️ 陷阱 1. return std::move(local); 會比 return local; 快嗎？
//     答：不會，反而更慢。return local; 可以觸發 NRVO（具名回傳值最佳化），
//         直接在呼叫端的記憶體建構，一次搬移都不做。
//         寫成 return std::move(local); 反而「阻止」了 NRVO，
//         強迫真的執行一次移動建構。
//     為什麼會錯：以為「移動一定比複製好，所以到處加 std::move」。
//         正確認知是：回傳區域變數時什麼都別加，讓編譯器做省略最佳化。
//
// ⚠️ 陷阱 2. 自訂了解構子之後，這個類別還有移動建構子嗎？
//     答：沒有。只要自訂了解構子（或任一複製操作），
//         編譯器就「不會」自動生成移動建構子與移動賦值。
//         此時 std::move 會安靜退回複製，效能優化完全落空。
//     為什麼會錯：以為「移動操作永遠會自動生成」。
//         但 is_move_constructible<T> 仍然回傳 true（因為 const T& 也接得住右值），
//         所以看起來像有移動 —— 要用 is_nothrow_move_constructible 才看得出真相。
//         需要移動時請明寫 = default（Rule of Five）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 本檔案是一本「迷你書」，涵蓋以下章節：
//   第 1 章：為什麼需要移動語意？— 從一個效能問題說起
//   第 2 章：左值與右值 — 你必須先搞懂的基礎
//   第 3 章：引用家族 — T&、const T&、T&&
//   第 4 章：右值引用的重要陷阱 — 有名字就是左值
//   第 5 章：深拷貝的代價 — 理解「為什麼慢」
//   第 6 章：移動建構子 — 偷取資源的藝術
//   第 7 章：移動賦值運算子 — 偷取 + 清理
//   第 8 章：統一賦值（Copy-and-Swap）— 一招打天下
//   第 9 章：Rule of Three → Five → Zero
//   第 10 章：std::move 的真相 — 它什麼都沒移動
//   第 11 章：noexcept — 一個關鍵字值百倍效能
//   第 12 章：std::forward 與完美轉發
//   第 13 章：實戰模式與最佳實踐
//   第 14 章：效能實測
//   第 15 章：常見錯誤與陷阱大全

#include <iostream>
#include <cstring>      // strlen, strcpy
#include <utility>      // std::move, std::swap, std::forward
#include <string>
#include <vector>
#include <memory>       // unique_ptr
#include <type_traits>  // is_move_constructible 等
#include <algorithm>    // std::copy
#include <chrono>
#include <functional>

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

void subtitle(const char* s) {
    cout << "--- " << s << " ---\n";
}


// ================================================================
//
//  第 1 章：為什麼需要移動語意？— 從一個效能問題說起
//
// ================================================================
// 想像你有一個類別管理一大塊動態記憶體（例如 100 萬個整數的陣列）。
// 每次拷貝這個物件，就要：
//   1. 配置一塊新的 100 萬個整數的記憶體
//   2. 把舊的 100 萬個整數一個一個複製過去
// 這非常慢！
//
// 但如果你知道某個物件「馬上就要被丟掉了」（例如一個臨時物件），
// 何必費力複製？直接「偷走」它的記憶體不就好了？
//
// 這就是移動語意的核心思想：
//   【如果來源馬上要消失，就偷它的資源，而不是複製】
//
// 問題是：C++ 怎麼知道一個物件「馬上要消失」？
// 答案就是：右值引用（Rvalue Reference）。

void chapter1() {
    title("第 1 章：為什麼需要移動語意？");

    // 先用 string 直觀感受「複製 vs 移動」的差異
    string original(100000, 'A');  // 一個 10 萬字元的字串

    // 複製：配置新記憶體 + 逐字元複製 → 慢
    auto t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        string copy = original;  // 每次都要 new + memcpy
        (void)copy;
    }
    auto t2 = chrono::high_resolution_clock::now();
    auto copy_ms = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

    // 移動：只偷指標 → 快
    auto t3 = chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        string temp = original;           // 先複製一份（公平比較）
        string moved = std::move(temp);   // 這步是 O(1) 的移動
        (void)moved;
    }
    auto t4 = chrono::high_resolution_clock::now();
    auto move_ms = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();

    cout << "  複製 10000 次 (10 萬字元): " << copy_ms << " ms\n";
    cout << "  複製+移動 10000 次:        " << move_ms << " ms\n";
    cout << "  （移動本身幾乎是零成本，差異主要來自避免第二次複製）\n\n";

    cout << "  【核心問題】如何讓 C++ 知道何時可以「偷」而非「複製」？\n";
    cout << "  【答案】右值引用 T&& — 用來標記「可以被偷走資源」的物件\n";
}


// ================================================================
//
//  第 2 章：左值與右值 — 你必須先搞懂的基礎
//
// ================================================================
// 在學右值引用之前，你必須先理解什麼是左值、什麼是右值。
//
// 【最簡單的判斷法】
//   能放在 = 左邊的 → 左值（有名字、有地址、可以取 &）
//   只能放在 = 右邊的 → 右值（臨時的、沒名字、不能取 &）
//
// 【左值範例】
//   int x = 42;        x 是左值（有名字 "x"、可以 &x）
//   int& ref = x;      ref 是左值
//   arr[0]             陣列元素是左值
//   *ptr               解引用是左值
//   "Hello"            字串字面值是左值（特殊！存在靜態區）
//   ++x                前置遞增回傳引用，是左值
//
// 【右值範例】
//   42                 字面值是右值
//   x + y              運算結果是右值（臨時的）
//   func()             回傳值（非引用）是右值
//   string("temp")     臨時物件是右值
//   x++                後置遞增回傳舊值副本，是右值

void chapter2() {
    title("第 2 章：左值與右值");

    int x = 42;

    // 左值：有名字、可以取地址
    subtitle("左值範例");
    cout << "  x 是左值，&x = " << &x << "\n";
    int& ref = x;
    cout << "  ref 是左值，&ref = " << &ref << " (和 &x 相同)\n";

    int arr[3] = {10, 20, 30};
    cout << "  arr[0] 是左值，&arr[0] = " << &arr[0] << "\n";

    // 前置 vs 後置遞增
    int y = 10;
    [[maybe_unused]] int& pre = ++y;   // ++y 回傳引用 → 左值 → 可以取地址
    // int& post = y++;  // ❌ y++ 回傳舊值副本 → 右值 → 不能綁到 T&
    cout << "  ++y 是左值（回傳引用），y++ 是右值（回傳副本）\n\n";

    // 右值：臨時的、不能取地址
    subtitle("右值範例");
    cout << "  42 是右值（字面值）\n";
    cout << "  x + 1 = " << (x + 1) << " 是右值（運算結果）\n";
    cout << "  string(\"temp\") 是右值（臨時物件）\n";
    // &42;              // ❌ 不能取右值的地址
    // &(x + 1);         // ❌ 不能取右值的地址
    // &string("temp");  // ❌ 不能取右值的地址

    cout << "\n  【記住】有名字 → 左值    沒名字 → 右值\n";
    cout << "  【記住】能 & 取址 → 左值    不能取址 → 右值\n";
}


// ================================================================
//
//  第 3 章：引用家族 — T&、const T&、T&&
//
// ================================================================
// C++ 有三種引用，各自有不同的綁定規則：
//
//   T&        左值引用       只能綁定左值
//   const T&  const 左值引用 可以綁定左值和右值（萬能！）
//   T&&       右值引用       只能綁定右值
//
// 為什麼 const T& 可以綁定右值？
//   因為 C++ 有一條特殊規則：const 引用可以延長臨時物件的生命週期。
//   這在 C++11 之前是唯一能「捕獲」臨時物件的方式。
//
// T&& 是 C++11 新增的。它的目的是：
//   讓你能區分「這個值是臨時的（可以偷）」和「這個值還要用（不能偷）」

void chapter3() {
    title("第 3 章：引用家族");

    int x = 42;

    // ---- T& 左值引用 ----
    subtitle("T& 左值引用 — 只能綁定左值");
    int& lref = x;          // ✅ 左值 → 左值引用
    cout << "  int& lref = x;        ✅ x 是左值\n";
    // int& bad = 42;        // ❌ 右值 → 不能綁到 T&
    cout << "  int& bad = 42;        ❌ 42 是右值（編譯錯誤）\n\n";

    // ---- const T& const 左值引用 ----
    subtitle("const T& — 萬能引用（左值右值都能綁）");
    const int& clref1 = x;  // ✅ 綁定左值
    const int& clref2 = 42; // ✅ 綁定右值（特殊規則！延長臨時物件生命週期）
    cout << "  const int& c1 = x;    ✅ 綁定左值\n";
    cout << "  const int& c2 = 42;   ✅ 綁定右值（延長生命週期）\n\n";

    // ---- T&& 右值引用 ----
    subtitle("T&& 右值引用 — 只能綁定右值");
    int&& rref = 42;         // ✅ 右值 → 右值引用
    // int&& bad = x;         // ❌ 左值 → 不能綁到 T&&
    cout << "  int&& rref = 42;      ✅ 42 是右值\n";
    cout << "  int&& bad = x;        ❌ x 是左值（編譯錯誤）\n";
    cout << "  int&& ok = move(x);   ✅ move(x) 把 x 轉成右值\n\n";

    // T&& 綁定後可以修改（和 const T& 不同！）
    rref = 100;
    cout << "  rref 綁定後可以修改：rref = " << rref << "\n";
    cout << "  而 const T& 綁定後不能修改\n\n";

    // 綁定規則總結
    subtitle("綁定規則總結表");
    cout << "  ┌─────────────┬───────────┬───────────┐\n";
    cout << "  │   引用類型   │  左值(x)  │ 右值(42) │\n";
    cout << "  ├─────────────┼───────────┼───────────┤\n";
    cout << "  │ T&          │    ✅     │    ❌     │\n";
    cout << "  │ const T&    │    ✅     │    ✅     │\n";
    cout << "  │ T&&         │    ❌     │    ✅     │\n";
    cout << "  └─────────────┴───────────┴───────────┘\n";

    (void)lref; (void)clref1; (void)clref2;
}


// ================================================================
//
//  第 4 章：右值引用的重要陷阱 — 有名字就是左值
//
// ================================================================
// 這是整個移動語意中最容易搞混的地方！
//
// int&& rref = 42;  // rref 綁定了右值 42
// 問題：rref 本身是左值還是右值？
// 答案：rref 是左值！因為它有名字 "rref"、可以取地址 &rref。
//
// 【關鍵規則】
//   右值引用「變數」本身是左值。
//   它的「型別」是 T&&，但它的「值類別」是左值。
//   型別和值類別是兩回事！

// 用來測試傳入的是左值還是右值
void detect(const string& s) { cout << "    收到左值: \"" << s << "\"\n"; }
void detect(string&& s)      { cout << "    收到右值: \"" << s << "\"\n"; }

// 函數參數 T&& 在函數內也是左值
void take_rvalue_ref(string&& s) {
    cout << "  在函數內，s 的型別是 string&&\n";
    cout << "  但 s 有名字 → s 是左值！\n";
    cout << "  直接傳 s:       ";
    detect(s);                   // ← 呼叫左值版本！
    cout << "  傳 move(s):     ";
    detect(std::move(s));        // ← 呼叫右值版本
}

void chapter4() {
    title("第 4 章：右值引用的重要陷阱 — 有名字就是左值");

    // 陷阱展示
    subtitle("右值引用變數本身是左值");
    int&& rref = 42;
    cout << "  rref 的型別是 int&&（右值引用）\n";
    cout << "  但 rref 有名字 → rref 是左值\n";
    cout << "  可以取地址：&rref = " << &rref << "\n\n";

    // 不能把左值綁到另一個右值引用
    // int&& rref2 = rref;         // ❌ rref 是左值！
    int&& rref2 = std::move(rref); // ✅ move 把它轉回右值
    cout << "  int&& rref2 = rref;         ❌ rref 是左值\n";
    cout << "  int&& rref2 = move(rref);   ✅ move 轉成右值\n\n";

    // 函數參數也一樣
    subtitle("函數參數 T&& 在函數內是左值");
    take_rvalue_ref(string("Hello"));
    cout << "\n";

    // 函數重載展示
    subtitle("函數重載：左值 vs 右值");
    string name = "Dragon";
    cout << "  傳左值:       ";
    detect(name);                     // 左值版本
    cout << "  傳臨時物件:   ";
    detect(string("Phoenix"));        // 右值版本
    cout << "  傳 move(name):";
    detect(std::move(name));          // 右值版本
    cout << "\n";

    cout << "  【鐵律】有名字 → 左值，不管它的型別是什麼\n";
    cout << "  【鐵律】要把左值「變回」右值，用 std::move\n";

    (void)rref2;
}


// ================================================================
//
//  第 5 章：深拷貝的代價 — 理解「為什麼慢」
//
// ================================================================
// 在進入移動建構子之前，我們先用一個簡單的類別來理解深拷貝有多貴。
//
// SimpleString 管理一個動態分配的 char 陣列：
//   建構：new char[n] + strcpy   → 配置 + 複製
//   拷貝：new char[n] + strcpy   → 又配置 + 又複製（深拷貝）
//   解構：delete[] data          → 釋放

// 這個類別我們會在後面的章節一步一步加上移動語意
class SimpleString {
    // ⚠️ 成員的初始化順序一律依「宣告順序」，與初始化列表的書寫順序無關。
    //    m_len 必須宣告在 m_data 前面，否則 new char[m_len + 1] 會讀到
    //    尚未初始化的 m_len，配置出錯誤大小的緩衝區（heap-buffer-overflow）。
    size_t m_len;
    char* m_data;
    static int s_copyCount;
    static int s_moveCount;

public:
    // ──── 一般建構 ────
    SimpleString(const char* str = "")
        : m_len(strlen(str)), m_data(new char[m_len + 1]) {
        strcpy(m_data, str);
    }

    // ──── 解構 ────
    ~SimpleString() {
        delete[] m_data;
    }

    // ──── 拷貝建構（深拷貝）────
    // 要做的事：1. 配置新記憶體  2. 複製所有資料
    // 時間複雜度：O(n)，n = 字串長度
    SimpleString(const SimpleString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        strcpy(m_data, other.m_data);
        ++s_copyCount;
    }

    // ──── 移動建構（偷資源）────
    // 要做的事：1. 偷指標  2. 歸零來源  3. 複製基本型別
    // 時間複雜度：O(1)，與資料大小無關！
    SimpleString(SimpleString&& other) noexcept
        : m_len(other.m_len)        // 步驟 1：複製基本型別（順序須與宣告順序一致）
        , m_data(other.m_data)      // 步驟 2：偷走指標
    {
        other.m_data = nullptr;     // 步驟 3：歸零來源（讓它安全解構）
        other.m_len = 0;
        ++s_moveCount;
    }

    // ──── swap ────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──── 統一賦值（Copy-and-Swap）────
    SimpleString& operator=(SimpleString other) noexcept {
        swap(other);
        return *this;
    }

    // ──── 存取 ────
    const char* c_str() const { return m_data ? m_data : "(null)"; }
    size_t length() const { return m_len; }

    static void resetCounters() { s_copyCount = s_moveCount = 0; }
    static void printCounters() {
        cout << "    拷貝: " << s_copyCount << " 次, 移動: " << s_moveCount << " 次\n";
    }
};
int SimpleString::s_copyCount = 0;
int SimpleString::s_moveCount = 0;

void chapter5() {
    title("第 5 章：深拷貝的代價");

    subtitle("拷貝 = 配置新記憶體 + 複製全部資料 = O(n)");
    {
        SimpleString a("Hello, this is a string with some content!");
        cout << "  a = \"" << a.c_str() << "\"\n";
        cout << "  a 佔用 " << (a.length() + 1) << " bytes 的堆積記憶體\n\n";

        SimpleString::resetCounters();
        SimpleString b = a;  // 深拷貝：new + strcpy
        cout << "  b = a（深拷貝）:\n";
        cout << "    b = \"" << b.c_str() << "\"\n";
        cout << "    a 和 b 各自擁有獨立的記憶體 ✅\n";
        SimpleString::printCounters();
    }
    cout << "\n";

    subtitle("如果拷貝 10000 次呢？");
    {
        SimpleString source(string(10000, 'X').c_str());
        SimpleString::resetCounters();

        auto t1 = chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; ++i) {
            SimpleString copy = source;  // 每次 new 10001 bytes + strcpy
            (void)copy;
        }
        auto t2 = chrono::high_resolution_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        cout << "  拷貝 10000 個 10000 字元的字串: " << ms << " ms\n";
        SimpleString::printCounters();
        cout << "  每次拷貝都要：new char[10001] + memcpy(10001 bytes)\n";
        cout << "  這就是我們需要移動語意的原因！\n";
    }
}


// ================================================================
//
//  第 6 章：移動建構子 — 偷取資源的藝術
//
// ================================================================
// 移動建構子的簽名：
//   ClassName(ClassName&& other) noexcept
//
// 移動三步驟（以管理 char* 的類別為例）：
//   步驟 1 — 偷取：m_data = other.m_data;       （偷走指標）
//   步驟 2 — 歸零：other.m_data = nullptr;       （讓來源安全解構）
//   步驟 3 — 複製基本型別：m_len = other.m_len;  （int/size_t 直接複製）
//
// 為什麼步驟 2 很重要？
//   如果不歸零，other 解構時會 delete 同一塊記憶體 → double free！
//   delete nullptr 是安全的（什麼都不做），所以設成 nullptr 就安全了。
//
// 移動後的 other 處於「有效但未指定」(valid but unspecified) 狀態：
//   ✅ 可以安全解構
//   ✅ 可以重新賦值
//   ❌ 不應該讀取它的值（因為已被偷走）

void chapter6() {
    title("第 6 章：移動建構子");

    subtitle("拷貝 vs 移動的對比");
    {
        SimpleString a("Dragon Sword");
        cout << "  a = \"" << a.c_str() << "\"\n\n";

        // 拷貝建構
        SimpleString::resetCounters();
        cout << "  【拷貝建構】SimpleString b = a;\n";
        SimpleString b = a;
        cout << "    b = \"" << b.c_str() << "\"\n";
        cout << "    a = \"" << a.c_str() << "\" (a 不受影響)\n";
        SimpleString::printCounters();
        cout << "\n";

        // 移動建構
        SimpleString::resetCounters();
        cout << "  【移動建構】SimpleString c = std::move(a);\n";
        SimpleString c = std::move(a);
        cout << "    c = \"" << c.c_str() << "\"\n";
        cout << "    a = \"" << a.c_str() << "\" (a 被掏空了！)\n";
        SimpleString::printCounters();
    }
    cout << "\n";

    subtitle("從臨時物件移動（最常見的場景）");
    {
        SimpleString::resetCounters();
        // 臨時物件是右值 → 自動觸發移動建構
        SimpleString d = SimpleString("Phoenix Staff");
        cout << "  d = \"" << d.c_str() << "\"\n";
        SimpleString::printCounters();
        cout << "  （臨時物件是右值，自動走移動路徑，可能被 RVO 優化掉）\n";
    }
    cout << "\n";

    subtitle("移動後可以重新賦值");
    {
        SimpleString a("First");
        SimpleString b = std::move(a);
        cout << "  移動後 a = \"" << a.c_str() << "\"\n";
        a = SimpleString("Reborn");  // 重新賦值是安全的
        cout << "  重新賦值後 a = \"" << a.c_str() << "\"\n";
    }
    cout << "\n";

    // 移動三步驟圖解
    subtitle("移動三步驟圖解");
    cout << "  移動前：\n";
    cout << "    this:   m_data → [空]        m_len = 0\n";
    cout << "    other:  m_data → [D|r|a|g|o|n|\\0]  m_len = 6\n\n";
    cout << "  步驟 1 偷取：this.m_data = other.m_data\n";
    cout << "    this:   m_data → [D|r|a|g|o|n|\\0]  ← 偷來的！\n";
    cout << "    other:  m_data → [D|r|a|g|o|n|\\0]  ← 還指著同一塊\n\n";
    cout << "  步驟 2 歸零：other.m_data = nullptr\n";
    cout << "    this:   m_data → [D|r|a|g|o|n|\\0]\n";
    cout << "    other:  m_data → nullptr  ← 安全了！\n\n";
    cout << "  步驟 3 複製基本型別：this.m_len = other.m_len; other.m_len = 0\n";
}


// ================================================================
//
//  第 7 章：移動賦值運算子 — 偷取 + 清理
//
// ================================================================
// 移動賦值和移動建構的差異：
//   移動建構：this 是全新的（不需要清理舊資料）
//   移動賦值：this 已經存在（需要先釋放舊資源！）
//
// 簽名：ClassName& operator=(ClassName&& other) noexcept
//
// 步驟：
//   1. 自我賦值檢查：if (this != &other)
//   2. 釋放自己的舊資源：delete[] m_data;
//   3. 偷取：m_data = other.m_data;
//   4. 歸零：other.m_data = nullptr;
//   5. 回傳 *this

// 為了教學，用一個追蹤所有操作的類別
class TrackedBuffer {
    // ⚠️ 成員的初始化順序一律依「宣告順序」，與初始化列表的書寫順序無關。
    //    m_len 必須宣告在 m_data 前面，否則 new char[m_len + 1] 會讀到
    //    尚未初始化的 m_len，配置出錯誤大小的緩衝區（heap-buffer-overflow）。
    size_t m_len;
    char* m_data;
public:
    TrackedBuffer(const char* str = "")
        : m_len(strlen(str)), m_data(new char[m_len + 1]) {
        strcpy(m_data, str);
        cout << "    [建構] \"" << m_data << "\"\n";
    }

    ~TrackedBuffer() {
        cout << "    [解構] \"" << (m_data ? m_data : "null") << "\"\n";
        delete[] m_data;
    }

    // 拷貝建構
    TrackedBuffer(const TrackedBuffer& o)
        : m_len(o.m_len), m_data(new char[m_len + 1]) {
        strcpy(m_data, o.m_data);
        cout << "    [拷貝建構💰] \"" << m_data << "\"\n";
    }

    // 移動建構
    TrackedBuffer(TrackedBuffer&& o) noexcept
        : m_len(o.m_len), m_data(o.m_data) {   // 順序須與宣告順序一致
        o.m_data = nullptr; o.m_len = 0;
        cout << "    [移動建構⚡] \"" << m_data << "\"\n";
    }

    // ★ 拷貝賦值
    TrackedBuffer& operator=(const TrackedBuffer& o) {
        cout << "    [拷貝賦值💰]\n";
        if (this != &o) {
            delete[] m_data;                     // 先釋放舊的
            m_len = o.m_len;
            m_data = new char[m_len + 1];        // 配置新的
            strcpy(m_data, o.m_data);            // 複製
        }
        return *this;
    }

    // ★ 移動賦值
    TrackedBuffer& operator=(TrackedBuffer&& o) noexcept {
        cout << "    [移動賦值⚡]\n";
        if (this != &o) {
            delete[] m_data;                     // 先釋放舊的
            m_data = o.m_data;  m_len = o.m_len; // 偷取
            o.m_data = nullptr; o.m_len = 0;     // 歸零
        }
        return *this;
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

void chapter7() {
    title("第 7 章：移動賦值運算子");

    subtitle("拷貝賦值 vs 移動賦值");
    {
        TrackedBuffer a("Alpha");
        TrackedBuffer b("Beta");
        cout << "\n  拷貝賦值 b = a:\n";
        b = a;
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        cout << "\n  移動賦值 b = move(a):\n";
        b = std::move(a);
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        cout << "\n  臨時物件賦值 b = TrackedBuffer(\"Gamma\"):\n";
        b = TrackedBuffer("Gamma");
        cout << "    b=\"" << b.c_str() << "\"\n";
    }
    cout << "\n";

    subtitle("觸發時機");
    cout << "  b = a;              → a 是左值 → 拷貝賦值\n";
    cout << "  b = move(a);        → move(a) 是右值 → 移動賦值\n";
    cout << "  b = func();         → 回傳值是右值 → 移動賦值\n";
    cout << "  b = Type(\"temp\");   → 臨時物件是右值 → 移動賦值\n";
}


// ================================================================
//
//  第 8 章：統一賦值（Copy-and-Swap）— 一招打天下
//
// ================================================================
// 前一章我們分別寫了拷貝賦值和移動賦值，要寫兩個函數。
// 有一個更優雅的方法：只寫一個 operator=，同時處理拷貝和移動。
//
// 技巧：把參數改成傳值（不是 const T& 也不是 T&&）
//
//   T& operator=(T other) {   // ← 傳值！
//       swap(*this, other);
//       return *this;
//   }
//
// 為什麼這樣就行了？
//   傳入左值 → other 由拷貝建構子建構 → swap → 走拷貝路徑
//   傳入右值 → other 由移動建構子建構 → swap → 走移動路徑
//
// 優點：
//   1. 一個函數搞定兩種賦值
//   2. 異常安全：如果 new 在拷貝建構階段失敗，原物件不受影響
//   3. 自動處理自我賦值（a = a 時 other 是 a 的拷貝，交換後仍正確）

void chapter8() {
    title("第 8 章：統一賦值（Copy-and-Swap）");

    // SimpleString 已經用了這個技巧
    subtitle("一個 operator= 同時處理拷貝和移動");
    {
        SimpleString a("Dragon");
        SimpleString b("Knight");

        cout << "  拷貝路徑（傳左值 → 拷貝建構 other → swap）:\n";
        SimpleString::resetCounters();
        b = a;
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
        SimpleString::printCounters();
        cout << "\n";

        cout << "  移動路徑（傳右值 → 移動建構 other → swap）:\n";
        SimpleString::resetCounters();
        b = std::move(a);
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
        SimpleString::printCounters();
        cout << "\n";

        cout << "  臨時物件路徑:\n";
        SimpleString::resetCounters();
        b = SimpleString("Phoenix");
        cout << "    b=\"" << b.c_str() << "\"\n";
        SimpleString::printCounters();
    }
    cout << "\n";

    subtitle("Copy-and-Swap 程式碼（回顧 SimpleString）");
    cout << "  void swap(SimpleString& other) noexcept {\n";
    cout << "      std::swap(m_data, other.m_data);\n";
    cout << "      std::swap(m_len, other.m_len);\n";
    cout << "  }\n\n";
    cout << "  SimpleString& operator=(SimpleString other) { // ← 傳值！\n";
    cout << "      swap(other);      // 交換內容\n";
    cout << "      return *this;     // other 解構時釋放舊資料\n";
    cout << "  }\n";
}


// ================================================================
//
//  第 9 章：Rule of Three → Five → Zero
//
// ================================================================

void chapter9() {
    title("第 9 章：Rule of Three → Five → Zero");

    subtitle("Rule of Three（C++98）");
    cout << "  如果類別需要自訂以下任何一個，三個都要寫：\n";
    cout << "    1. ~ClassName()                 解構子\n";
    cout << "    2. ClassName(const ClassName&)   拷貝建構\n";
    cout << "    3. operator=(const ClassName&)   拷貝賦值\n\n";
    cout << "  為什麼？需要解構子 → 表示管理了資源\n";
    cout << "  → 預設的逐成員複製（淺拷貝）不安全 → 要自訂拷貝\n\n";

    subtitle("Rule of Five（C++11）");
    cout << "  = Rule of Three + 移動建構 + 移動賦值：\n";
    cout << "    1. ~ClassName()                       解構子\n";
    cout << "    2. ClassName(const ClassName&)         拷貝建構\n";
    cout << "    3. operator=(const ClassName&)         拷貝賦值\n";
    cout << "    4. ClassName(ClassName&&) noexcept     移動建構  ← 新增\n";
    cout << "    5. operator=(ClassName&&) noexcept     移動賦值  ← 新增\n\n";
    cout << "  用統一賦值可以簡化成 4 個：\n";
    cout << "    解構 + 拷貝建構 + 移動建構 + operator=(T other)\n\n";

    subtitle("Rule of Zero（最佳實踐！）");
    cout << "  只用 RAII 成員（string, vector, unique_ptr...）\n";
    cout << "  → 什麼特殊函式都不用寫！編譯器全部自動生成正確版本\n\n";

    // Rule of Zero 範例
    struct SafeClass {
        string name;          // 自動管理
        vector<int> data;     // 自動管理
        // 不需要寫任何解構/拷貝/移動！
    };

    SafeClass s1;
    s1.name = "Hero";
    s1.data = {1, 2, 3};
    SafeClass s2 = s1;              // 安全拷貝
    SafeClass s3 = std::move(s1);   // 安全移動
    cout << "  SafeClass：只用 string + vector 成員\n";
    cout << "    s2.name = " << s2.name << "\n";
    cout << "    s3.name = " << s3.name << "\n";
    cout << "    s1.name = \"" << s1.name << "\" (已被移動)\n\n";

    // 自動生成規則
    subtitle("重要：自訂任何一個 → 移動不自動生成");
    cout << "  自訂了解構子 → 編譯器不自動生成移動建構/移動賦值\n";
    cout << "  自訂了拷貝建構 → 同上\n";
    cout << "  自訂了拷貝賦值 → 同上\n";
    cout << "  → 這就是為什麼 Rule of Five 說「要嘛全寫，要嘛全不寫」\n";

    // type_traits 驗證
    cout << "\n  用 type_traits 檢查：\n";
    cout << boolalpha;

    struct OnlyDestructor {
        ~OnlyDestructor() {}  // 自訂解構 → 移動不自動生成
    };
    cout << "    OnlyDestructor 可移動建構？ "
         << is_move_constructible_v<OnlyDestructor> << "\n";
    cout << "    OnlyDestructor nothrow？    "
         << is_nothrow_move_constructible_v<OnlyDestructor> << "\n";
    cout << "    （看似可以移動，但實際上退回拷貝，不是真正的移動）\n";
}


// ================================================================
//
//  第 10 章：std::move 的真相 — 它什麼都沒移動
//
// ================================================================

void chapter10() {
    title("第 10 章：std::move 的真相");

    subtitle("std::move 只是一個 cast，不做任何移動");
    cout << "  std::move(x)  ≡  static_cast<T&&>(x)\n\n";
    cout << "  它只是把左值「標記為」右值引用\n";
    cout << "  真正的移動發生在接收端的移動建構子或移動賦值中\n\n";

    subtitle("std::move 的實作（超級簡單）");
    cout << "  template<typename T>\n";
    cout << "  remove_reference_t<T>&& move(T&& arg) noexcept {\n";
    cout << "      return static_cast<remove_reference_t<T>&&>(arg);\n";
    cout << "  }\n\n";

    subtitle("三種 move 不會觸發移動的情況");

    // 情況 1：const 物件
    cout << "  1. const 物件 → move 後型別是 const T&& → 匹配 const T&（拷貝！）\n";
    {
        const string c = "World";
        string d = std::move(c);  // 拷貝！不是移動！
        cout << "     const string c = \"World\";\n";
        cout << "     string d = move(c);  // c 仍然是 \"" << c << "\" → 拷貝！\n\n";
    }

    // 情況 2：基本型別
    cout << "  2. 基本型別（int, double...）→ 沒有資源可搬，移動 = 複製\n";
    {
        int x = 42;
        [[maybe_unused]] int y = std::move(x);
        cout << "     int x = 42; int y = move(x);\n";
        cout << "     x 仍然 = " << x << " → 無差異\n\n";
    }

    // 情況 3：SSO 短字串
    cout << "  3. SSO 短字串 → string 的 Small String Optimization\n";
    cout << "     短字串存在物件內部（棧上），不在堆積上 → 移動 ≈ 複製\n\n";

    subtitle("unique_ptr — 只能 move 不能 copy");
    {
        auto p = make_unique<int>(42);
        cout << "  auto p = make_unique<int>(42);\n";
        cout << "  *p = " << *p << "\n";
        // auto q = p;  // ❌ unique_ptr 禁止複製
        auto q = std::move(p);  // ✅ 轉移所有權
        cout << "  auto q = move(p);  // p → nullptr, q 接管\n";
        cout << "  p = " << (p ? "非空" : "nullptr") << "\n";
        cout << "  *q = " << *q << "\n";
    }
}


// ================================================================
//
//  第 11 章：noexcept — 一個關鍵字值百倍效能
//
// ================================================================

void chapter11() {
    title("第 11 章：noexcept 的重要性");

    cout << "  移動建構子一定要加 noexcept！\n\n";
    cout << "  為什麼？因為 vector 擴容時需要搬移元素：\n";
    cout << "    有 noexcept → vector 使用移動（快！）\n";
    cout << "    沒 noexcept → vector 退回使用拷貝（慢！）\n\n";
    cout << "  原因：vector 需要「強異常安全保證」\n";
    cout << "  如果移動到一半拋出例外 → 原始資料已被破壞 → 無法恢復\n";
    cout << "  所以 vector 只在確定不會拋例外時才敢用移動\n\n";

    // 效能測試
    subtitle("效能測試：有 vs 沒有 noexcept");
    struct WithNoexcept {
        vector<int> data;
        WithNoexcept() : data(1000, 42) {}
        WithNoexcept(const WithNoexcept& o) : data(o.data) {}
        WithNoexcept(WithNoexcept&& o) noexcept : data(std::move(o.data)) {}
    };
    struct WithoutNoexcept {
        vector<int> data;
        WithoutNoexcept() : data(1000, 42) {}
        WithoutNoexcept(const WithoutNoexcept& o) : data(o.data) {}
        WithoutNoexcept(WithoutNoexcept&& o) : data(std::move(o.data)) {} // 沒 noexcept！
    };

    {
        auto t1 = chrono::high_resolution_clock::now();
        vector<WithNoexcept> v1;
        for (int i = 0; i < 50000; ++i) v1.emplace_back();
        auto t2 = chrono::high_resolution_clock::now();
        auto ms1 = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        auto t3 = chrono::high_resolution_clock::now();
        vector<WithoutNoexcept> v2;
        for (int i = 0; i < 50000; ++i) v2.emplace_back();
        auto t4 = chrono::high_resolution_clock::now();
        auto ms2 = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();

        cout << "  有 noexcept（擴容用移動）: " << ms1 << " ms\n";
        cout << "  沒 noexcept（擴容用拷貝）: " << ms2 << " ms\n";
        if (ms1 > 0)
            cout << "  差距約: " << (double)ms2 / ms1 << " 倍\n";
    }
}


// ================================================================
//
//  第 12 章：std::forward 與完美轉發
//
// ================================================================

void fwd_target(const string& /*s*/) { cout << "    → const T& 左值版本\n"; }  // 參數不具名：只看選中哪個重載
void fwd_target(string&& /*s*/)      { cout << "    → T&& 右值版本\n"; }       // 同上

// 錯誤：直接傳 arg → 永遠左值
template<typename T>
void wrapper_bad(T&& arg) {
    fwd_target(arg);  // arg 有名字 → 左值 → 永遠呼叫左值版本
}

// 正確：用 std::forward
template<typename T>
void wrapper_good(T&& arg) {
    fwd_target(std::forward<T>(arg));
}

// 泛型工廠
class Gadget {
    string name_;
public:
    Gadget(const string& n) : name_(n) { cout << "    Gadget(const T&) 拷貝\n"; }
    Gadget(string&& n) : name_(std::move(n)) { cout << "    Gadget(T&&) 移動\n"; }
};

template<typename T, typename... Args>
unique_ptr<T> my_make(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

void chapter12() {
    title("第 12 章：std::forward 與完美轉發");

    subtitle("問題：T&& 參數在函數內是左值");
    string s = "Hello";
    cout << "  不用 forward（錯誤）:\n";
    cout << "    傳左值:  "; wrapper_bad(s);
    cout << "    傳右值:  "; wrapper_bad(string("tmp"));
    cout << "    兩個都呼叫左值版本！❌\n\n";

    cout << "  用 forward（正確）:\n";
    cout << "    傳左值:  "; wrapper_good(s);
    cout << "    傳右值:  "; wrapper_good(string("tmp"));
    cout << "    正確區分！✅\n\n";

    subtitle("三種轉發方式對比");
    cout << "  ┌─────────────────────────┬───────────┬───────────┐\n";
    cout << "  │       轉發方式           │ 傳左值時  │ 傳右值時  │\n";
    cout << "  ├─────────────────────────┼───────────┼───────────┤\n";
    cout << "  │ target(arg)             │   左值    │   左值 ❌ │\n";
    cout << "  │ target(move(arg))       │   右值 ❌ │   右值    │\n";
    cout << "  │ target(forward<T>(arg)) │   左值 ✅ │   右值 ✅ │\n";
    cout << "  └─────────────────────────┴───────────┴───────────┘\n\n";

    subtitle("forward 的原理：引用折疊");
    cout << "  傳入左值 s → T 推導為 string& → T&& = string& (& + && = &)\n";
    cout << "    → forward<string&>(arg) = static_cast<string&>(arg) → 左值\n\n";
    cout << "  傳入右值 string(\"x\") → T = string → T&& = string&&\n";
    cout << "    → forward<string>(arg) = static_cast<string&&>(arg) → 右值\n\n";

    subtitle("實戰：泛型工廠函式");
    string name = "Sword";
    cout << "  傳左值:\n";
    auto g1 = my_make<Gadget>(name);
    cout << "  傳右值:\n";
    auto g2 = my_make<Gadget>(string("Shield"));
    cout << "\n";

    subtitle("emplace_back — 標準庫中最常見的完美轉發");
    cout << "  push_back(obj)          → 拷貝到容器\n";
    cout << "  push_back(move(obj))    → 移動到容器\n";
    cout << "  emplace_back(args...)   → 在容器內直接建構（零拷貝零移動！）\n";
    {
        vector<string> v;
        v.reserve(3);
        string x = "Alpha";
        cout << "\n";
        // emplace_back 用完美轉發把引數傳給 string 的建構子
        v.emplace_back("directly constructed");  // 直接在 vector 內部建構
        cout << "  emplace_back(\"directly constructed\") → 零拷貝零移動\n";
    }
}


// ================================================================
//
//  第 13 章：實戰模式與最佳實踐
//
// ================================================================

void chapter13() {
    title("第 13 章：實戰模式與最佳實踐");

    // 模式 1：建構子 pass-by-value + move
    subtitle("模式 1：建構子 pass-by-value + move（推薦）");
    cout << "  class Hero {\n";
    cout << "      string name_;\n";
    cout << "  public:\n";
    cout << "      Hero(string name) : name_(move(name)) {}\n";
    cout << "  };\n\n";
    cout << "  傳左值：拷貝到參數 → move 到成員（1 拷貝 + 1 移動）\n";
    cout << "  傳右值：移動到參數 → move 到成員（2 移動，零拷貝）\n\n";

    // 實際展示
    {
        struct Hero {
            string name_;
            Hero(string name) : name_(std::move(name)) {}
        };

        SimpleString::resetCounters();
        string n = "Arthur";
        Hero h1(n);                  // 左值：1 拷貝 + 1 移動
        Hero h2(string("Merlin"));   // 右值：1 移動（或 RVO）+ 1 移動
        cout << "  h1.name_ = " << h1.name_ << "\n";
        cout << "  h2.name_ = " << h2.name_ << "\n\n";
    }

    // 模式 2：swap 實現（3 次移動，0 次拷貝）
    subtitle("模式 2：用 std::swap 交換兩個物件");
    {
        string a = "Left", b = "Right";
        cout << "  swap 前：a=\"" << a << "\" b=\"" << b << "\"\n";
        // swap 內部：
        //   T tmp = move(a);   // 移動建構
        //   a = move(b);       // 移動賦值
        //   b = move(tmp);     // 移動賦值
        // 3 次移動，0 次拷貝！
        std::swap(a, b);
        cout << "  swap 後：a=\"" << a << "\" b=\"" << b << "\"\n";
        cout << "  內部是 3 次移動，0 次拷貝\n\n";
    }

    // 模式 3：容器操作
    subtitle("模式 3：容器操作");
    {
        vector<string> vec;
        vec.reserve(3);

        string s = "reusable";
        vec.push_back(s);              // 拷貝（s 還要用）
        vec.push_back(std::move(s));   // 移動（s 不再需要）
        vec.emplace_back("in-place");  // 原地建構（最快）
        cout << "  push_back(s)       → 拷貝\n";
        cout << "  push_back(move(s)) → 移動\n";
        cout << "  emplace_back(...)  → 原地建構（零拷貝零移動）\n\n";
    }

    // 模式 4：從容器提取元素
    subtitle("模式 4：從容器提取元素");
    {
        vector<string> vec = {"Alpha", "Beta", "Gamma"};
        string last = std::move(vec.back());  // 移動出來
        vec.pop_back();                       // 移除（解構空殻）
        cout << "  string last = move(vec.back()); vec.pop_back();\n";
        cout << "  last = \"" << last << "\"\n\n";
    }

    // 模式 5：unique_ptr 所有權轉移
    subtitle("模式 5：unique_ptr 所有權轉移");
    {
        auto p1 = make_unique<int>(42);
        auto p2 = std::move(p1);  // p1 → nullptr，p2 接管
        cout << "  unique_ptr 只能 move 不能 copy\n";
        cout << "  p1 = " << (p1 ? "非空" : "nullptr") << "\n";
        cout << "  *p2 = " << *p2 << "\n";
    }
}


// ================================================================
//
//  第 14 章：效能實測
//
// ================================================================

void chapter14() {
    title("第 14 章：效能實測");

    // 測試 1：string
    // ⚠️ 請先看清楚這兩個迴圈「各做了什麼」：
    //      迴圈 A：1 次深拷貝
    //      迴圈 B：1 次深拷貝 + 1 次移動
    //    迴圈 B 做的事嚴格多於 A，所以它「不可能」比 A 快。
    //    這個測試真正量到的是「在一次拷貝之上，額外多做一次移動的成本」。
    //    兩者耗時越接近，越能證明那次移動幾乎免費 —— 這才是本測試的結論。
    //    （要直接比「拷貝 vs 移動」，兩邊必須做同樣多的事；但每輪都得重新
    //      準備一個可被掏空的來源，那份準備成本又會混進測量結果。
    //      這正是微效能測試最容易失真之處，故本檔誠實標示自己在量什麼。）
    subtitle("測試 1：純拷貝 vs 拷貝+移動 (10000 字元)");
    {
        const int N = 2000000;
        string source(10000, 'x');

        auto t1 = chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            string copy = source; (void)copy;
        }
        auto t2 = chrono::high_resolution_clock::now();

        auto t3 = chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            string temp = source;
            string moved = std::move(temp); (void)moved;
        }
        auto t4 = chrono::high_resolution_clock::now();

        cout << "  純拷貝    : " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << " ms\n";
        cout << "  拷貝+移動 : " << chrono::duration_cast<chrono::milliseconds>(t4 - t3).count() << " ms\n";
        cout << "  → 後者多做一次移動，理論上必然略慢；差距很小即代表移動幾乎免費。\n";
        cout << "  → 這裡量的是「移動的邊際成本」，不是「移動 vs 拷貝的勝負」。\n\n";
    }

    // 測試 2：vector 不同大小
    subtitle("測試 2：vector<int> 拷貝+移動 vs 純拷貝");
    {
        auto bench = [](size_t size) {
            const int N = 5000;
            vector<int> source(size, 42);
            long long copy_us, move_us;
            {
                auto t = chrono::high_resolution_clock::now();
                for (int i = 0; i < N; ++i) { vector<int> c = source; (void)c; }
                copy_us = chrono::duration_cast<chrono::microseconds>(
                    chrono::high_resolution_clock::now() - t).count();
            }
            {
                auto t = chrono::high_resolution_clock::now();
                for (int i = 0; i < N; ++i) {
                    vector<int> tmp = source;
                    vector<int> m = std::move(tmp); (void)m;
                }
                move_us = chrono::duration_cast<chrono::microseconds>(
                    chrono::high_resolution_clock::now() - t).count();
            }
            cout << "  size=" << size
                 << "\t純拷貝: " << copy_us << " us"
                 << "\t拷貝+移動: " << move_us << " us";
            if (move_us > 0)
                cout << "\t比值: " << (double)copy_us / move_us << "x";
            cout << "\n";
            // 註：此比值是「純拷貝 ÷ (拷貝+移動)」，不是加速比。
            //     後者做的事更多，所以「理論上」比值應 ≤ 1、且越接近 1 代表移動越便宜。
            //     但實測在大 size 時偶爾會略大於 1（本機就出現過 ~1.08）——
            //     因為此時堆積配置、page fault 與快取效應遠大於那一次移動的成本，
            //     測量雜訊已把訊號淹沒。這本身就是個重要教訓：
            //     微效能測試的雜訊可能大於你要量的東西，看趨勢而非單次數值。
        };
        bench(100);
        bench(1000);
        bench(100000);
    }
    cout << "\n";

    // 測試 3：push_back vs emplace_back
    subtitle("測試 3：push_back 拷貝 vs move vs emplace_back");
    {
        const int N = 1000000;
        {
            auto t = chrono::high_resolution_clock::now();
            vector<string> v; v.reserve(N);
            for (int i = 0; i < N; ++i) {
                string s = "Hello World!!!!!";
                v.push_back(s);
            }
            cout << "  push_back(copy): "
                 << chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - t).count() << " ms\n";
        }
        {
            auto t = chrono::high_resolution_clock::now();
            vector<string> v; v.reserve(N);
            for (int i = 0; i < N; ++i) {
                string s = "Hello World!!!!!";
                v.push_back(std::move(s));
            }
            cout << "  push_back(move): "
                 << chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - t).count() << " ms\n";
        }
        {
            auto t = chrono::high_resolution_clock::now();
            vector<string> v; v.reserve(N);
            for (int i = 0; i < N; ++i) {
                v.emplace_back("Hello World!!!!!");
            }
            cout << "  emplace_back:    "
                 << chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - t).count() << " ms\n";
        }
    }
}


// ================================================================
//
//  第 15 章：常見錯誤與陷阱大全
//
// ================================================================

void chapter15() {
    title("第 15 章：常見錯誤與陷阱大全");

    // 陷阱 1
    subtitle("陷阱 1：move 後繼續使用物件的值");
    {
        string s = "Important";
        string t = std::move(s);
        cout << "  ❌ string s = \"Important\"; string t = move(s);\n";
        cout << "     s = \"" << s << "\"  ← 已被掏空，不應該讀取\n";
        cout << "  ✅ 可以重新賦值：s = \"New\";\n";
        s = "New";
        cout << "     s = \"" << s << "\"\n\n";
    }

    // 陷阱 2
    subtitle("陷阱 2：move const 物件 → 退化為拷貝");
    {
        const string c = "Immovable";
        string d = std::move(c);  // const string&& → 匹配 const string&（拷貝！）
        cout << "  const string c = \"Immovable\";\n";
        cout << "  string d = move(c);  // c 仍然是 \"" << c << "\"\n";
        cout << "  → const T&& 只能匹配 const T&（拷貝建構），不是移動！\n\n";
    }

    // 陷阱 3
    subtitle("陷阱 3：return std::move(local) 阻止 RVO");
    cout << "  ❌ string func() { string s = \"x\"; return move(s); }\n";
    cout << "     move 阻止了 NRVO 優化，反而更慢！\n";
    cout << "  ✅ string func() { string s = \"x\"; return s; }\n";
    cout << "     編譯器會自動做 NRVO（直接在呼叫者空間建構）\n\n";

    // 陷阱 4
    subtitle("陷阱 4：忘記 noexcept → vector 不用移動");
    cout << "  ❌ MyClass(MyClass&& o) { ... }           // 沒有 noexcept\n";
    cout << "  ✅ MyClass(MyClass&& o) noexcept { ... }   // 有 noexcept\n";
    cout << "  → 沒有 noexcept，vector 擴容時退回使用拷貝（慢很多）\n\n";

    // 陷阱 5
    subtitle("陷阱 5：右值引用變數本身是左值");
    cout << "  string&& r = string(\"temp\");\n";
    cout << "  func(r);        → 呼叫左值版本！（r 有名字 → 左值）\n";
    cout << "  func(move(r));  → 呼叫右值版本   （move 轉回右值）\n\n";

    // 陷阱 6
    subtitle("陷阱 6：move 基本型別沒有效果");
    {
        int x = 42;
        [[maybe_unused]] int y = std::move(x);
        cout << "  int x = 42; int y = move(x);\n";
        cout << "  x = " << x << " → int 沒有資源可搬，move 無效\n\n";
    }

    // 陷阱 7
    subtitle("陷阱 7：只寫解構不寫拷貝 → 淺拷貝 → 崩潰");
    cout << "  class Bad {\n";
    cout << "      int* data;\n";
    cout << "  public:\n";
    cout << "      Bad(int v) : data(new int(v)) {}\n";
    cout << "      ~Bad() { delete data; }   // 只寫了這個\n";
    cout << "  };\n";
    cout << "  Bad a(42); Bad b = a;  // 淺拷貝 → double free 💥\n";

    cout << "\n";
    subtitle("總結：安全使用移動語意的檢查清單");
    cout << "  ✅ 移動建構/賦值加 noexcept\n";
    cout << "  ✅ 移動後不再使用物件的值\n";
    cout << "  ✅ 不要 move const 物件\n";
    cout << "  ✅ 不要 return move(local)\n";
    cout << "  ✅ 不要 move 基本型別\n";
    cout << "  ✅ 用 emplace_back > push_back(move) > push_back(copy)\n";
    cout << "  ✅ 優先 Rule of Zero（只用 RAII 成員）\n";
    cout << "  ✅ 如果要 Rule of Five，用 Copy-and-Swap 簡化\n";
}


// ================================================================
//
//  第 16 章：LeetCode 實戰 + 日常實務範例
//
// ================================================================
//
// 說明：移動語意本身是「資源管理機制」，不是演算法。
//       LeetCode 沒有「考移動語意」的題目。
//       但下面這題的標準解法會反覆把子結果放進結果容器，
//       正是 move 與 emplace_back 能產生真實差異的場景，
//       故收錄；至於完美轉發則完全不屬於 LeetCode 範疇，不強加。

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 46. Permutations
//   題目：回傳陣列所有排列。
//   為什麼用到本主題：
//     回溯法會產生大量 vector<int> 子結果並收進 vector<vector<int>>。
//     每找到一組排列就 push_back(current) 會「複製」整個 vector；
//     改用 emplace_back / push_back(std::move(...)) 可避免複製。
//     這裡示範「什麼時候該 move、什麼時候不該」：
//       * current 之後還要繼續回溯使用 → 不能 move（會被掏空）
//       * 若是最後一次使用的臨時容器 → 才可以 move
//   複雜度：時間 O(n! * n)，空間 O(n!)（輸出）
// -----------------------------------------------------------------------------
void permuteBacktrack(vector<int>& nums, size_t depth,
                      vector<vector<int>>& out) {
    if (depth == nums.size()) {
        // ⚠️ 這裡「不可以」寫 out.push_back(std::move(nums))：
        //    nums 是回溯狀態，後面還要繼續交換使用，掏空它整個演算法就壞了。
        //    正確作法是複製一份目前狀態 —— 這是必要的複製，不是浪費。
        out.push_back(nums);
        return;
    }
    for (size_t i = depth; i < nums.size(); ++i) {
        std::swap(nums[depth], nums[i]);
        permuteBacktrack(nums, depth + 1, out);
        std::swap(nums[depth], nums[i]);   // 還原
    }
}

vector<vector<int>> permute(vector<int> nums) {
    vector<vector<int>> out;
    // 先算出 n! 並 reserve：避免過程中反覆擴容
    size_t total = 1;
    for (size_t i = 2; i <= nums.size(); ++i) total *= i;
    out.reserve(total);

    permuteBacktrack(nums, 0, out);
    return out;    // ← 直接 return，讓編譯器做 NRVO；千萬別寫 return std::move(out);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】訊息佇列 — 移動語意最典型的正當用途
//   情境：網路服務收到封包後放進處理佇列，工作執行緒再取出處理。
//         封包內容（payload）可能很大，全程都不該複製。
//   為什麼用到本主題：
//     (a) 生產端把封包「交出去」→ 用 std::move 移交所有權，O(1)
//     (b) 消費端把封包「取出來」→ 同樣用 move，避免複製
//     這是移動語意在真實系統中最常見、收益最明確的形式：
//     資料只有一個擁有者，隨著流程在各層之間移交。
// -----------------------------------------------------------------------------
struct Message {
    string      topic;
    string      payload;    // 可能很大
    long long   timestamp;

    Message(string t, string p, long long ts)
        : topic(std::move(t))        // 參數按值接收 + move：呼叫端可自由選擇複製或移動
        , payload(std::move(p))
        , timestamp(ts) {}
};

class MessageQueue {
    vector<Message> m_items;

public:
    // 接受右值：呼叫端必須明確 move 進來，語意清楚「所有權轉移」
    void push(Message&& msg) {
        m_items.push_back(std::move(msg));   // 有名字的右值參考是左值 → 必須再 move
    }

    // 取出最舊的一筆（移出後從容器移除）
    bool pop(Message& out) {
        if (m_items.empty()) return false;
        out = std::move(m_items.front());    // 移出，不複製
        m_items.erase(m_items.begin());
        return true;
    }

    size_t size() const { return m_items.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】為什麼「按值傳參 + std::move」是現代慣例
//   情境：建構子或 setter 要把外部字串存進成員。
//   三種寫法的取捨：
//     (a) const string& s → m_name = s;          左值：1 次複製；右值：1 次複製（浪費）
//     (b) string s        → m_name = move(s);    左值：1 次複製；右值：1 次移動（最佳平衡）
//     (c) 同時寫 const&/&& 兩個重載              最快，但參數一多就是組合爆炸
//   結論：除非在效能極端敏感的路徑，(b) 是可讀性與效能兼顧的預設選擇。
// -----------------------------------------------------------------------------
class UserProfile {
    string m_name;
    string m_email;

public:
    // 按值接收 + move：對左值與右值都給出合理成本
    UserProfile(string name, string email)
        : m_name(std::move(name)), m_email(std::move(email)) {}

    void setName(string name) { m_name = std::move(name); }

    const string& name()  const { return m_name; }
    const string& email() const { return m_email; }
};

void chapter16() {
    title("第 16 章：LeetCode 實戰 + 日常實務範例");

    subtitle("LeetCode 46. Permutations");
    {
        vector<vector<int>> all = permute({1, 2, 3});
        cout << "  [1,2,3] 的全排列（共 " << all.size() << " 組）:\n    ";
        for (size_t i = 0; i < all.size(); ++i) {
            cout << "[";
            for (size_t j = 0; j < all[i].size(); ++j)
                cout << all[i][j] << (j + 1 < all[i].size() ? "," : "");
            cout << "] ";
        }
        cout << "\n";
        cout << "  重點：回溯狀態 nums 之後還要用，所以只能複製、不能 move。\n";
        cout << "        「該不該 move」取決於這個物件之後還會不會被使用。\n";
        cout << "        回傳時直接 return out;（NRVO），不要寫 return std::move(out);\n";
    }

    subtitle("日常實務 1：訊息佇列（所有權移交）");
    {
        MessageQueue queue;

        // 建立一則較大的封包
        string bigPayload(64, 'X');
        Message m1("sensor/temp", bigPayload, 1721400000);      // 複製 bigPayload
        Message m2("sensor/humid", std::move(bigPayload), 1721400001); // 移動

        cout << "  移動後 bigPayload 的長度: " << bigPayload.size()
             << "（被移動後處於有效但未指定狀態；此處觀察到長度為 0，"
                "但這是實作行為，非標準保證）\n";

        queue.push(std::move(m1));
        queue.push(std::move(m2));
        cout << "  佇列目前筆數: " << queue.size() << "\n";

        Message got("", "", 0);
        while (queue.pop(got)) {
            cout << "    取出 topic=" << got.topic
                 << ", payload 長度=" << got.payload.size()
                 << ", ts=" << got.timestamp << "\n";
        }
        cout << "  全程只搬指標，payload 內容一次都沒有被複製。\n";
    }

    subtitle("日常實務 2：按值傳參 + std::move 的慣例");
    {
        string existingName = "Alice";                 // 左值：會被複製
        UserProfile u1(existingName, "alice@example.com");

        UserProfile u2(string("Bob"), "bob@example.com");  // 右值：會被移動

        cout << "  u1: " << u1.name() << " / " << u1.email() << "\n";
        cout << "  u2: " << u2.name() << " / " << u2.email() << "\n";
        cout << "  existingName 仍可用: \"" << existingName << "\"（傳左值只是複製）\n";

        u1.setName(std::move(existingName));            // 明確移交
        cout << "  改名後 u1: " << u1.name() << "\n";
        cout << "  重點：按值接收 + std::move，對左值與右值都給出合理成本，\n";
        cout << "        而且不必為每個參數寫 const&/&& 兩份重載。\n";
    }
}


// ================================================================
//
//  main：按順序執行所有章節
//
// ================================================================
int main() {
    cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    cout << "║  右值參考與移動語意 — 完整教學（15 章）                         ║\n";
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
    chapter16();

    cout << "\n";
    cout << "================================================================\n";
    cout << "  恭喜！你已經完整學完右值參考與移動語意的所有核心知識。\n";
    cout << "  建議順序複習：\n";
    cout << "    1. 左值/右值 → 2. 三種引用 → 3. 有名字=左值\n";
    cout << "    4. 移動建構/賦值 → 5. Copy-and-Swap → 6. Rule of 3/5/0\n";
    cout << "    7. std::move → 8. noexcept → 9. std::forward\n";
    cout << "    10. 實戰模式 → 11. 陷阱大全\n";
    cout << "================================================================\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 單元：右值參考與移動語意 summary.cpp" -o unit2_summary
//  ⚠️ 注意：下列輸出中所有耗時數字（ms / us）與由它們算出的比值，
//     都「每次執行都不同」—— 取決於當下的 CPU 負載、快取狀態與記憶體配置，
//     此處僅為某一次實際執行的樣本，不具重現性。
//
//     另請留意第 14 章各測試「量的是什麼」，以免誤讀：
//       * 測試 1 與測試 2 的第二個迴圈都是「拷貝 + 移動」，做的事嚴格多於
//         第一個迴圈的「純拷貝」。它們量的是「多做一次移動的邊際成本」，
//         不是「移動 vs 拷貝的勝負」。理論上比值應 ≤ 1、越接近 1 代表移動越便宜；
//         但大 size 時配置與快取雜訊可能讓比值略大於 1（本次 size=100000 即是），
//         那代表該次移動的成本已小到被測量雜訊淹沒。
//       * 測試 3（noexcept）才是真正的對照實驗：兩邊做同樣的事，
//         差別只在移動建構子有沒有標 noexcept，因此可直接比較快慢。
//     記憶體位址（若有）同樣每次執行都不同。

// === 預期輸出 ===
// ╔══════════════════════════════════════════════════════════════════╗
// ║  右值參考與移動語意 — 完整教學（15 章）                         ║
// ╚══════════════════════════════════════════════════════════════════╝
//
// ================================================================
//   第 1 章：為什麼需要移動語意？
// ================================================================
//
//   複製 10000 次 (10 萬字元): 18 ms
//   複製+移動 10000 次:        18 ms
//   （移動本身幾乎是零成本，差異主要來自避免第二次複製）
//
//   【核心問題】如何讓 C++ 知道何時可以「偷」而非「複製」？
//   【答案】右值引用 T&& — 用來標記「可以被偷走資源」的物件
//
// ================================================================
//   第 2 章：左值與右值
// ================================================================
//
// --- 左值範例 ---
//   x 是左值，&x = 0x7ffc8244afa0
//   ref 是左值，&ref = 0x7ffc8244afa0 (和 &x 相同)
//   arr[0] 是左值，&arr[0] = 0x7ffc8244afbc
//   ++y 是左值（回傳引用），y++ 是右值（回傳副本）
//
// --- 右值範例 ---
//   42 是右值（字面值）
//   x + 1 = 43 是右值（運算結果）
//   string("temp") 是右值（臨時物件）
//
//   【記住】有名字 → 左值    沒名字 → 右值
//   【記住】能 & 取址 → 左值    不能取址 → 右值
//
// ================================================================
//   第 3 章：引用家族
// ================================================================
//
// --- T& 左值引用 — 只能綁定左值 ---
//   int& lref = x;        ✅ x 是左值
//   int& bad = 42;        ❌ 42 是右值（編譯錯誤）
//
// --- const T& — 萬能引用（左值右值都能綁） ---
//   const int& c1 = x;    ✅ 綁定左值
//   const int& c2 = 42;   ✅ 綁定右值（延長生命週期）
//
// --- T&& 右值引用 — 只能綁定右值 ---
//   int&& rref = 42;      ✅ 42 是右值
//   int&& bad = x;        ❌ x 是左值（編譯錯誤）
//   int&& ok = move(x);   ✅ move(x) 把 x 轉成右值
//
//   rref 綁定後可以修改：rref = 100
//   而 const T& 綁定後不能修改
//
// --- 綁定規則總結表 ---
//   ┌─────────────┬───────────┬───────────┐
//   │   引用類型   │  左值(x)  │ 右值(42) │
//   ├─────────────┼───────────┼───────────┤
//   │ T&          │    ✅     │    ❌     │
//   │ const T&    │    ✅     │    ✅     │
//   │ T&&         │    ❌     │    ✅     │
//   └─────────────┴───────────┴───────────┘
//
// ================================================================
//   第 4 章：右值引用的重要陷阱 — 有名字就是左值
// ================================================================
//
// --- 右值引用變數本身是左值 ---
//   rref 的型別是 int&&（右值引用）
//   但 rref 有名字 → rref 是左值
//   可以取地址：&rref = 0x7ffc8244af3c
//
//   int&& rref2 = rref;         ❌ rref 是左值
//   int&& rref2 = move(rref);   ✅ move 轉成右值
//
// --- 函數參數 T&& 在函數內是左值 ---
//   在函數內，s 的型別是 string&&
//   但 s 有名字 → s 是左值！
//   直接傳 s:           收到左值: "Hello"
//   傳 move(s):         收到右值: "Hello"
//
// --- 函數重載：左值 vs 右值 ---
//   傳左值:           收到左值: "Dragon"
//   傳臨時物件:       收到右值: "Phoenix"
//   傳 move(name):    收到右值: "Dragon"
//
//   【鐵律】有名字 → 左值，不管它的型別是什麼
//   【鐵律】要把左值「變回」右值，用 std::move
//
// ================================================================
//   第 5 章：深拷貝的代價
// ================================================================
//
// --- 拷貝 = 配置新記憶體 + 複製全部資料 = O(n) ---
//   a = "Hello, this is a string with some content!"
//   a 佔用 43 bytes 的堆積記憶體
//
//   b = a（深拷貝）:
//     b = "Hello, this is a string with some content!"
//     a 和 b 各自擁有獨立的記憶體 ✅
//     拷貝: 1 次, 移動: 0 次
//
// --- 如果拷貝 10000 次呢？ ---
//   拷貝 10000 個 10000 字元的字串: 0 ms
//     拷貝: 10000 次, 移動: 0 次
//   每次拷貝都要：new char[10001] + memcpy(10001 bytes)
//   這就是我們需要移動語意的原因！
//
// ================================================================
//   第 6 章：移動建構子
// ================================================================
//
// --- 拷貝 vs 移動的對比 ---
//   a = "Dragon Sword"
//
//   【拷貝建構】SimpleString b = a;
//     b = "Dragon Sword"
//     a = "Dragon Sword" (a 不受影響)
//     拷貝: 1 次, 移動: 0 次
//
//   【移動建構】SimpleString c = std::move(a);
//     c = "Dragon Sword"
//     a = "(null)" (a 被掏空了！)
//     拷貝: 0 次, 移動: 1 次
//
// --- 從臨時物件移動（最常見的場景） ---
//   d = "Phoenix Staff"
//     拷貝: 0 次, 移動: 0 次
//   （臨時物件是右值，自動走移動路徑，可能被 RVO 優化掉）
//
// --- 移動後可以重新賦值 ---
//   移動後 a = "(null)"
//   重新賦值後 a = "Reborn"
//
// --- 移動三步驟圖解 ---
//   移動前：
//     this:   m_data → [空]        m_len = 0
//     other:  m_data → [D|r|a|g|o|n|\0]  m_len = 6
//
//   步驟 1 偷取：this.m_data = other.m_data
//     this:   m_data → [D|r|a|g|o|n|\0]  ← 偷來的！
//     other:  m_data → [D|r|a|g|o|n|\0]  ← 還指著同一塊
//
//   步驟 2 歸零：other.m_data = nullptr
//     this:   m_data → [D|r|a|g|o|n|\0]
//     other:  m_data → nullptr  ← 安全了！
//
//   步驟 3 複製基本型別：this.m_len = other.m_len; other.m_len = 0
//
// ================================================================
//   第 7 章：移動賦值運算子
// ================================================================
//
// --- 拷貝賦值 vs 移動賦值 ---
//     [建構] "Alpha"
//     [建構] "Beta"
//
//   拷貝賦值 b = a:
//     [拷貝賦值💰]
//     a="Alpha" b="Alpha"
//
//   移動賦值 b = move(a):
//     [移動賦值⚡]
//     a="(null)" b="Alpha"
//
//   臨時物件賦值 b = TrackedBuffer("Gamma"):
//     [建構] "Gamma"
//     [移動賦值⚡]
//     [解構] "null"
//     b="Gamma"
//     [解構] "Gamma"
//     [解構] "null"
//
// --- 觸發時機 ---
//   b = a;              → a 是左值 → 拷貝賦值
//   b = move(a);        → move(a) 是右值 → 移動賦值
//   b = func();         → 回傳值是右值 → 移動賦值
//   b = Type("temp");   → 臨時物件是右值 → 移動賦值
//
// ================================================================
//   第 8 章：統一賦值（Copy-and-Swap）
// ================================================================
//
// --- 一個 operator= 同時處理拷貝和移動 ---
//   拷貝路徑（傳左值 → 拷貝建構 other → swap）:
//     a="Dragon" b="Dragon"
//     拷貝: 1 次, 移動: 0 次
//
//   移動路徑（傳右值 → 移動建構 other → swap）:
//     a="(null)" b="Dragon"
//     拷貝: 0 次, 移動: 1 次
//
//   臨時物件路徑:
//     b="Phoenix"
//     拷貝: 0 次, 移動: 0 次
//
// --- Copy-and-Swap 程式碼（回顧 SimpleString） ---
//   void swap(SimpleString& other) noexcept {
//       std::swap(m_data, other.m_data);
//       std::swap(m_len, other.m_len);
//   }
//
//   SimpleString& operator=(SimpleString other) { // ← 傳值！
//       swap(other);      // 交換內容
//       return *this;     // other 解構時釋放舊資料
//   }
//
// ================================================================
//   第 9 章：Rule of Three → Five → Zero
// ================================================================
//
// --- Rule of Three（C++98） ---
//   如果類別需要自訂以下任何一個，三個都要寫：
//     1. ~ClassName()                 解構子
//     2. ClassName(const ClassName&)   拷貝建構
//     3. operator=(const ClassName&)   拷貝賦值
//
//   為什麼？需要解構子 → 表示管理了資源
//   → 預設的逐成員複製（淺拷貝）不安全 → 要自訂拷貝
//
// --- Rule of Five（C++11） ---
//   = Rule of Three + 移動建構 + 移動賦值：
//     1. ~ClassName()                       解構子
//     2. ClassName(const ClassName&)         拷貝建構
//     3. operator=(const ClassName&)         拷貝賦值
//     4. ClassName(ClassName&&) noexcept     移動建構  ← 新增
//     5. operator=(ClassName&&) noexcept     移動賦值  ← 新增
//
//   用統一賦值可以簡化成 4 個：
//     解構 + 拷貝建構 + 移動建構 + operator=(T other)
//
// --- Rule of Zero（最佳實踐！） ---
//   只用 RAII 成員（string, vector, unique_ptr...）
//   → 什麼特殊函式都不用寫！編譯器全部自動生成正確版本
//
//   SafeClass：只用 string + vector 成員
//     s2.name = Hero
//     s3.name = Hero
//     s1.name = "" (已被移動)
//
// --- 重要：自訂任何一個 → 移動不自動生成 ---
//   自訂了解構子 → 編譯器不自動生成移動建構/移動賦值
//   自訂了拷貝建構 → 同上
//   自訂了拷貝賦值 → 同上
//   → 這就是為什麼 Rule of Five 說「要嘛全寫，要嘛全不寫」
//
//   用 type_traits 檢查：
//     OnlyDestructor 可移動建構？ true
//     OnlyDestructor nothrow？    true
//     （看似可以移動，但實際上退回拷貝，不是真正的移動）
//
// ================================================================
//   第 10 章：std::move 的真相
// ================================================================
//
// --- std::move 只是一個 cast，不做任何移動 ---
//   std::move(x)  ≡  static_cast<T&&>(x)
//
//   它只是把左值「標記為」右值引用
//   真正的移動發生在接收端的移動建構子或移動賦值中
//
// --- std::move 的實作（超級簡單） ---
//   template<typename T>
//   remove_reference_t<T>&& move(T&& arg) noexcept {
//       return static_cast<remove_reference_t<T>&&>(arg);
//   }
//
// --- 三種 move 不會觸發移動的情況 ---
//   1. const 物件 → move 後型別是 const T&& → 匹配 const T&（拷貝！）
//      const string c = "World";
//      string d = move(c);  // c 仍然是 "World" → 拷貝！
//
//   2. 基本型別（int, double...）→ 沒有資源可搬，移動 = 複製
//      int x = 42; int y = move(x);
//      x 仍然 = 42 → 無差異
//
//   3. SSO 短字串 → string 的 Small String Optimization
//      短字串存在物件內部（棧上），不在堆積上 → 移動 ≈ 複製
//
// --- unique_ptr — 只能 move 不能 copy ---
//   auto p = make_unique<int>(42);
//   *p = 42
//   auto q = move(p);  // p → nullptr, q 接管
//   p = nullptr
//   *q = 42
//
// ================================================================
//   第 11 章：noexcept 的重要性
// ================================================================
//
//   移動建構子一定要加 noexcept！
//
//   為什麼？因為 vector 擴容時需要搬移元素：
//     有 noexcept → vector 使用移動（快！）
//     沒 noexcept → vector 退回使用拷貝（慢！）
//
//   原因：vector 需要「強異常安全保證」
//   如果移動到一半拋出例外 → 原始資料已被破壞 → 無法恢復
//   所以 vector 只在確定不會拋例外時才敢用移動
//
// --- 效能測試：有 vs 沒有 noexcept ---
//   有 noexcept（擴容用移動）: 278 ms
//   沒 noexcept（擴容用拷貝）: 317 ms
//   差距約: 1.14029 倍
//
// ================================================================
//   第 12 章：std::forward 與完美轉發
// ================================================================
//
// --- 問題：T&& 參數在函數內是左值 ---
//   不用 forward（錯誤）:
//     傳左值:      → const T& 左值版本
//     傳右值:      → const T& 左值版本
//     兩個都呼叫左值版本！❌
//
//   用 forward（正確）:
//     傳左值:      → const T& 左值版本
//     傳右值:      → T&& 右值版本
//     正確區分！✅
//
// --- 三種轉發方式對比 ---
//   ┌─────────────────────────┬───────────┬───────────┐
//   │       轉發方式           │ 傳左值時  │ 傳右值時  │
//   ├─────────────────────────┼───────────┼───────────┤
//   │ target(arg)             │   左值    │   左值 ❌ │
//   │ target(move(arg))       │   右值 ❌ │   右值    │
//   │ target(forward<T>(arg)) │   左值 ✅ │   右值 ✅ │
//   └─────────────────────────┴───────────┴───────────┘
//
// --- forward 的原理：引用折疊 ---
//   傳入左值 s → T 推導為 string& → T&& = string& (& + && = &)
//     → forward<string&>(arg) = static_cast<string&>(arg) → 左值
//
//   傳入右值 string("x") → T = string → T&& = string&&
//     → forward<string>(arg) = static_cast<string&&>(arg) → 右值
//
// --- 實戰：泛型工廠函式 ---
//   傳左值:
//     Gadget(const T&) 拷貝
//   傳右值:
//     Gadget(T&&) 移動
//
// --- emplace_back — 標準庫中最常見的完美轉發 ---
//   push_back(obj)          → 拷貝到容器
//   push_back(move(obj))    → 移動到容器
//   emplace_back(args...)   → 在容器內直接建構（零拷貝零移動！）
//
//   emplace_back("directly constructed") → 零拷貝零移動
//
// ================================================================
//   第 13 章：實戰模式與最佳實踐
// ================================================================
//
// --- 模式 1：建構子 pass-by-value + move（推薦） ---
//   class Hero {
//       string name_;
//   public:
//       Hero(string name) : name_(move(name)) {}
//   };
//
//   傳左值：拷貝到參數 → move 到成員（1 拷貝 + 1 移動）
//   傳右值：移動到參數 → move 到成員（2 移動，零拷貝）
//
//   h1.name_ = Arthur
//   h2.name_ = Merlin
//
// --- 模式 2：用 std::swap 交換兩個物件 ---
//   swap 前：a="Left" b="Right"
//   swap 後：a="Right" b="Left"
//   內部是 3 次移動，0 次拷貝
//
// --- 模式 3：容器操作 ---
//   push_back(s)       → 拷貝
//   push_back(move(s)) → 移動
//   emplace_back(...)  → 原地建構（零拷貝零移動）
//
// --- 模式 4：從容器提取元素 ---
//   string last = move(vec.back()); vec.pop_back();
//   last = "Gamma"
//
// --- 模式 5：unique_ptr 所有權轉移 ---
//   unique_ptr 只能 move 不能 copy
//   p1 = nullptr
//   *p2 = 42
//
// ================================================================
//   第 14 章：效能實測
// ================================================================
//
// --- 測試 1：純拷貝 vs 拷貝+移動 (10000 字元) ---
//   純拷貝    : 495 ms
//   拷貝+移動 : 627 ms
//   → 後者多做一次移動，理論上必然略慢；差距很小即代表移動幾乎免費。
//   → 這裡量的是「移動的邊際成本」，不是「移動 vs 拷貝的勝負」。
//
// --- 測試 2：vector<int> 拷貝+移動 vs 純拷貝 ---
//   size=100	純拷貝: 328 us	拷貝+移動: 512 us	比值: 0.640625x
//   size=1000	純拷貝: 654 us	拷貝+移動: 772 us	比值: 0.84715x
//   size=100000	純拷貝: 62489 us	拷貝+移動: 60922 us	比值: 1.02572x
//
// --- 測試 3：push_back 拷貝 vs move vs emplace_back ---
//   push_back(copy): 190 ms
//   push_back(move): 184 ms
//   emplace_back:    111 ms
//
// ================================================================
//   第 15 章：常見錯誤與陷阱大全
// ================================================================
//
// --- 陷阱 1：move 後繼續使用物件的值 ---
//   ❌ string s = "Important"; string t = move(s);
//      s = ""  ← 已被掏空，不應該讀取
//   ✅ 可以重新賦值：s = "New";
//      s = "New"
//
// --- 陷阱 2：move const 物件 → 退化為拷貝 ---
//   const string c = "Immovable";
//   string d = move(c);  // c 仍然是 "Immovable"
//   → const T&& 只能匹配 const T&（拷貝建構），不是移動！
//
// --- 陷阱 3：return std::move(local) 阻止 RVO ---
//   ❌ string func() { string s = "x"; return move(s); }
//      move 阻止了 NRVO 優化，反而更慢！
//   ✅ string func() { string s = "x"; return s; }
//      編譯器會自動做 NRVO（直接在呼叫者空間建構）
//
// --- 陷阱 4：忘記 noexcept → vector 不用移動 ---
//   ❌ MyClass(MyClass&& o) { ... }           // 沒有 noexcept
//   ✅ MyClass(MyClass&& o) noexcept { ... }   // 有 noexcept
//   → 沒有 noexcept，vector 擴容時退回使用拷貝（慢很多）
//
// --- 陷阱 5：右值引用變數本身是左值 ---
//   string&& r = string("temp");
//   func(r);        → 呼叫左值版本！（r 有名字 → 左值）
//   func(move(r));  → 呼叫右值版本   （move 轉回右值）
//
// --- 陷阱 6：move 基本型別沒有效果 ---
//   int x = 42; int y = move(x);
//   x = 42 → int 沒有資源可搬，move 無效
//
// --- 陷阱 7：只寫解構不寫拷貝 → 淺拷貝 → 崩潰 ---
//   class Bad {
//       int* data;
//   public:
//       Bad(int v) : data(new int(v)) {}
//       ~Bad() { delete data; }   // 只寫了這個
//   };
//   Bad a(42); Bad b = a;  // 淺拷貝 → double free 💥
//
// --- 總結：安全使用移動語意的檢查清單 ---
//   ✅ 移動建構/賦值加 noexcept
//   ✅ 移動後不再使用物件的值
//   ✅ 不要 move const 物件
//   ✅ 不要 return move(local)
//   ✅ 不要 move 基本型別
//   ✅ 用 emplace_back > push_back(move) > push_back(copy)
//   ✅ 優先 Rule of Zero（只用 RAII 成員）
//   ✅ 如果要 Rule of Five，用 Copy-and-Swap 簡化
//
// ================================================================
//   第 16 章：LeetCode 實戰 + 日常實務範例
// ================================================================
//
// --- LeetCode 46. Permutations ---
//   [1,2,3] 的全排列（共 6 組）:
//     [1,2,3] [1,3,2] [2,1,3] [2,3,1] [3,2,1] [3,1,2] 
//   重點：回溯狀態 nums 之後還要用，所以只能複製、不能 move。
//         「該不該 move」取決於這個物件之後還會不會被使用。
//         回傳時直接 return out;（NRVO），不要寫 return std::move(out);
// --- 日常實務 1：訊息佇列（所有權移交） ---
//   移動後 bigPayload 的長度: 0（被移動後處於有效但未指定狀態；此處觀察到長度為 0，但這是實作行為，非標準保證）
//   佇列目前筆數: 2
//     取出 topic=sensor/temp, payload 長度=64, ts=1721400000
//     取出 topic=sensor/humid, payload 長度=64, ts=1721400001
//   全程只搬指標，payload 內容一次都沒有被複製。
// --- 日常實務 2：按值傳參 + std::move 的慣例 ---
//   u1: Alice / alice@example.com
//   u2: Bob / bob@example.com
//   existingName 仍可用: "Alice"（傳左值只是複製）
//   改名後 u1: Alice
//   重點：按值接收 + std::move，對左值與右值都給出合理成本，
//         而且不必為每個參數寫 const&/&& 兩份重載。
//
// ================================================================
//   恭喜！你已經完整學完右值參考與移動語意的所有核心知識。
//   建議順序複習：
//     1. 左值/右值 → 2. 三種引用 → 3. 有名字=左值
//     4. 移動建構/賦值 → 5. Copy-and-Swap → 6. Rule of 3/5/0
//     7. std::move → 8. noexcept → 9. std::forward
//     10. 實戰模式 → 11. 陷阱大全
// ================================================================
