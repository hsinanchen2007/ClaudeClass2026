// =============================================================================
//  第 2.1 章 -2  —  prvalue（純右值）：沒有身份、可被移動的表達式
// =============================================================================
//
// 【主題資訊 Information】
//   概念：prvalue（pure rvalue）是三個基本值類別中「無 identity」的那一個
//   標準版本：C++11 命名；★ C++17 大幅改寫了它的定義（見【2.】，非常重要）
//   判定工具：decltype((expr)) 推導為「非引用型別 T」→ 該表達式是 prvalue
//   標頭檔：本檔僅需 <iostream> <string>
//
// 【詳細解釋 Explanation】
//
// 【1. prvalue 是什麼：沒有儲存位置的「值本身」】
//   lvalue 回答的是「哪個物件」，prvalue 回答的是「什麼值」。
//     42          — 就是「四十二」這個值，沒有任何物件叫做 42
//     x + 1       — 運算的結果，還沒被存到任何有名字的地方
//     greet()     — 回傳非引用型別的函式呼叫
//   因為沒有可指認的儲存位置，所以不能取位址（&42 編譯錯誤），
//   而且因為沒有人會再用到它，編譯器可以放心讓它被「移動」— 這就是為什麼
//   prvalue 落在 rvalue 那一邊。
//
// 【2. ★ C++17 的關鍵改變：prvalue 不再「是」一個物件】
//   這是本章最容易被舊教材帶壞的地方，請務必分清楚兩個時期：
//
//   ── C++11 / C++14 的模型 ──
//     prvalue「就是一個臨時物件」。std::string("x") 會實際建構出一個臨時
//     物件，然後 std::string s = std::string("x"); 理論上要呼叫一次移動
//     （或拷貝）建構子。編譯器『被允許』省略它（copy elision），但那是
//     可選的最佳化 — 所以標準仍要求該建構子必須存在且可存取。
//
//   ── C++17 起的模型（guaranteed copy elision）──
//     prvalue 被重新定義成「一個還沒發生的初始化動作」，本身不是物件。
//     只有在真正需要一個物件時（例如綁定到引用、或取成員），它才會經由
//     temporary materialization conversion（臨時物件實體化轉換）
//     變成一個 xvalue，這時才誕生真正的臨時物件。
//     結果：std::string s = std::string("x"); 根本沒有「兩個物件」，
//     那個 prvalue 直接就地初始化 s。不是「省略了拷貝」，而是「從來沒有
//     第二個物件可以拷貝」。
//
//   為什麼這個差別是實質的、不只是措辭？因為它改變了「能不能編譯」：
//     struct NoCopyNoMove {
//         NoCopyNoMove() = default;
//         NoCopyNoMove(const NoCopyNoMove&) = delete;
//         NoCopyNoMove(NoCopyNoMove&&)      = delete;
//     };
//     NoCopyNoMove make() { return NoCopyNoMove{}; }
//     NoCopyNoMove x = make();
//   本機實測（g++ 15.2，加 -pedantic-errors）：
//     -std=c++11 / c++14 → error: use of deleted function ‘NoCopyNoMove(NoCopyNoMove&&)’
//     -std=c++17         → 編譯成功
//   同一份程式碼，換標準就從「不合法」變「合法」。這就是為什麼 C++17 之後
//   工廠函式可以回傳完全不可拷貝也不可移動的型別（如 std::mutex 的包裝、
//   std::atomic 的持有者），在 C++14 是做不到的。
//
// 【概念補充 Concept Deep Dive】
//   ★ prvalue 與 xvalue 都屬於 rvalue，差別在 identity：
//       prvalue：無 identity、可移動 → 「值」，還沒有落腳處
//       xvalue ：有 identity、可移動 → 「物件」，只是你聲明放棄它了
//     std::move(x) 是 xvalue 不是 prvalue，因為它指向的仍是 x 那個物件。
//
//   ★ materialization 什麼時候發生（C++17）：
//       const std::string& r = std::string("hi");
//       ↑ 引用需要一個真正的物件可綁 → prvalue 實體化成臨時物件（xvalue）
//         → 該臨時物件的生命期被延長到 r 的生命期結束
//       std::string("hi").size();
//       ↑ 呼叫成員函式需要 this 指標 → 同樣觸發實體化
//     反之 std::string s = std::string("hi"); 不需要中間物件，直接初始化 s。
//
//   ★ 承接 -1 檔的兩問題矩陣：
//       有身份 + 不可移動 = lvalue
//       有身份 + 可移動   = xvalue
//       無身份 + 可移動   = prvalue   ← 本檔主題
//     沒有「無身份 + 不可移動」這一格，所以基本類別只有三個。
//
// 【注意事項 Pay Attention】
//   1. 字串字面值 "Hello" 不是 prvalue，是 lvalue（見 -1 檔）。其他字面值
//      42 / 3.14 / true / nullptr 才是 prvalue。這是最常考的例外。
//   2. lambda 表達式 [](int a){...} 本身是 prvalue（它是個閉包物件的初始化）。
//   3. ++x 是 lvalue（回傳引用），x++ 是 prvalue（回傳舊值的副本）— 這也是
//      為什麼自訂型別的 x++ 通常比 ++x 慢：它必須多做一份副本。
//   4. C++17 之後請避免說「編譯器省略了拷貝」。正確說法是「prvalue 直接
//      初始化目標物件，中間本來就沒有第二個物件」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】prvalue 與 C++17 的實體化模型
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++17 的 guaranteed copy elision 到底保證了什麼？
//     答：保證「用 prvalue 初始化同型別物件時，不會產生中間物件」。因為
//         C++17 把 prvalue 重新定義成尚未發生的初始化，而不是一個臨時物件。
//         實務後果：回傳型別即使把拷貝與移動建構子都 = delete 掉，
//         T x = make(); 仍然合法（本機實測 C++14 失敗、C++17 成功）。
//     追問：那 NRVO（具名回傳值最佳化）也被保證了嗎？
//         → 沒有。return s; 其中 s 是具名區域變數時，s 是 lvalue 不是
//           prvalue，所以不在保證範圍內；編譯器通常會做，但標準只是「允許」。
//
// ⚠️ 陷阱. std::move(x) 是 prvalue 嗎？
//     答：不是，它是 xvalue。prvalue 的特徵是「沒有 identity」，但
//         std::move(x) 明確指向 x 這個既有物件，它有 identity。
//         std::move 只做一件事：把值類別從 lvalue 改成 xvalue，物件從頭到尾
//         都是同一個。
//     為什麼會錯：很多人記成「rvalue = 臨時物件」，於是把所有 rvalue 都當成
//         prvalue。實際上 rvalue 是 xvalue 與 prvalue 的聯集，
//         「臨時物件」只描述了 prvalue 實體化之後的那一半。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

std::string greet() {
    return "Hello";  // 回傳值是 prvalue
}

// ─────────────────────────────────────────────────────────────────────────────
// C++17 guaranteed copy elision 的證明：拷貝與移動建構子都被 delete，
// 在 C++11/C++14 下 make() 無法編譯；C++17 下完全合法。
// ─────────────────────────────────────────────────────────────────────────────
struct NoCopyNoMove {
    int tag;
    explicit NoCopyNoMove(int t) : tag(t) {}
    NoCopyNoMove(const NoCopyNoMove&) = delete;   // 不可拷貝
    NoCopyNoMove(NoCopyNoMove&&)      = delete;   // 也不可移動
};

NoCopyNoMove makeNoCopyNoMove() {
    return NoCopyNoMove{7};   // prvalue：C++17 直接就地初始化呼叫端的物件
}

int main() {
    // ===== 以下全部是 prvalue =====
    // （(void) 僅用來消除「此敘述無作用」的警告，不影響值類別的說明）

    (void)42;              // 整數字面值
    (void)3.14;            // 浮點字面值
    (void)true;            // 布林字面值
    (void)nullptr;         // 空指標字面值（注意：字串字面值不是！）

    int x = 10;
    (void)(2 + 3);         // 算術運算結果
    (void)(x > 0);         // 比較運算結果

    (void)greet();         // 回傳非參考型別的函式呼叫

    (void)int(42);         // 型別轉換
    (void)static_cast<double>(x);// 轉型結果

    // Lambda 表達式本身是 prvalue
    (void)[](int a) { return a * 2; };

    // 後置遞增/遞減的結果是 prvalue（返回的是舊值的副本）
    int y = 10;
    (void)y++;             // prvalue（不同於 ++y 是左值）

    // ─────────────────────────────────────────────────────────────────
    std::cout << "=== prvalue 不能取位址 ===\n";
    std::cout << "&42、&(x+1)、&greet() 全部都是編譯錯誤\n";
    std::cout << "原因：prvalue 沒有可指認的儲存位置（沒有 identity）\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== C++17 guaranteed copy elision ===\n";
    NoCopyNoMove obj = makeNoCopyNoMove();   // 拷貝/移動都被 delete 仍然合法
    std::cout << "NoCopyNoMove 的拷貝與移動建構子都 = delete\n";
    std::cout << "但 NoCopyNoMove obj = makeNoCopyNoMove(); 編譯成功，obj.tag = "
              << obj.tag << "\n";
    std::cout << "本機實測：-std=c++11 / c++14 會編譯失敗（use of deleted function）\n";
    std::cout << "          -std=c++17 成功 → prvalue 直接初始化 obj，中間沒有第二個物件\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== 什麼時候 prvalue 才會實體化成臨時物件 ===\n";
    const std::string& r = greet();   // 引用需要真正的物件 → 觸發實體化
    std::cout << "const string& r = greet();  → 需要物件可綁 → 實體化，"
                 "生命期延長到 r 結束\n";
    std::cout << "  r = \"" << r << "\"\n";
    std::cout << "string s = greet();         → 不需要中間物件 → 直接初始化 s\n";
    std::cout << "  greet().size() = " << greet().size()
              << "  → 呼叫成員函式需要 this → 也會實體化\n";

    // ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== 前置 vs 後置遞增的值類別差異 ===\n";
    int n = 5;
    std::cout << "++n 回傳引用 → lvalue → 可以寫 int& a = ++n;\n";
    int& a = ++n;
    std::cout << "  ++n 後 n = " << n << "，&a == &n ? "
              << std::boolalpha << (&a == &n) << "\n";
    std::cout << "n++ 回傳舊值副本 → prvalue → int& b = n++; 是編譯錯誤\n";
    std::cout << "  這也是自訂型別 n++ 通常比 ++n 慢的原因：得多做一份副本\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.1 章：左值與右值的定義 — Value Categories 完整解析2.cpp" -o prvalue_demo
// 注意: 本檔的 NoCopyNoMove 示範「必須」用 C++17 以上；
//       用 -std=c++14 -pedantic-errors 會如預期編譯失敗，那正是本檔要證明的重點。

// === 預期輸出 ===
// === prvalue 不能取位址 ===
// &42、&(x+1)、&greet() 全部都是編譯錯誤
// 原因：prvalue 沒有可指認的儲存位置（沒有 identity）
//
// === C++17 guaranteed copy elision ===
// NoCopyNoMove 的拷貝與移動建構子都 = delete
// 但 NoCopyNoMove obj = makeNoCopyNoMove(); 編譯成功，obj.tag = 7
// 本機實測：-std=c++11 / c++14 會編譯失敗（use of deleted function）
//           -std=c++17 成功 → prvalue 直接初始化 obj，中間沒有第二個物件
//
// === 什麼時候 prvalue 才會實體化成臨時物件 ===
// const string& r = greet();  → 需要物件可綁 → 實體化，生命期延長到 r 結束
//   r = "Hello"
// string s = greet();         → 不需要中間物件 → 直接初始化 s
//   greet().size() = 5  → 呼叫成員函式需要 this → 也會實體化
//
// === 前置 vs 後置遞增的值類別差異 ===
// ++n 回傳引用 → lvalue → 可以寫 int& a = ++n;
//   ++n 後 n = 6，&a == &n ? true
// n++ 回傳舊值副本 → prvalue → int& b = n++; 是編譯錯誤
//   這也是自訂型別 n++ 通常比 ++n 慢的原因：得多做一份副本
