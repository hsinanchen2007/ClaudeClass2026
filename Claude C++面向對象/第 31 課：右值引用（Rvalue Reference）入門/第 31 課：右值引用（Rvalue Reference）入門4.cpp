// =============================================================================
//  第 31 課 -4  —  std::move 預覽：它只是 cast，以及 move 之後能做什麼
// =============================================================================
//
// 【主題資訊 Information】
//   簽名     ： template<class T>
//               constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;
//   實作本質 ： return static_cast<std::remove_reference_t<T>&&>(t);
//               —— 純粹是型別轉換，不搬移任何資料
//   標準版本 ： C++11（C++14 起為 constexpr）
//   標頭檔   ： <utility>
//   複雜度   ： O(1)，而且是編譯期的事，執行期沒有任何指令
//   move 後狀態： valid but unspecified（有效但未指定）
//
// 【詳細解釋 Explanation】
//
// 【1. std::move 這個名字取壞了】
//   它不移動任何東西。它做的事只有一件：把實參「重新標記」成右值，
//   好讓重載解析去選中吃 T&& 的那個版本。真正的搬移發生在接收端 ——
//   移動建構函數或移動賦值運算子被選中並執行的時候。
//   叫它 rvalue_cast 會誠實得多，但名字已經定了。
//   本檔第 2 段就是證據：process(std::move(a)) 選中了 [T&&] 版本，
//   但那個版本只是把字串印出來，什麼都沒搬 —— 所以 a 其實完好無缺。
//
// 【2. 「沒有人接手就什麼都不會發生」】
//   這是最常被忽略的一點：
//       process(std::move(s));        // 若 process 收的是 const T& → s 完好
//       std::string t = std::move(s); // 移動建構接手了 → s 才真的被搬空
//   所以「std::move 之後原物件一定變空」是錯的。
//   會不會變空，取決於接收端有沒有真的執行移動操作。
//
// 【3. move 之後的物件：valid but unspecified】
//   拆成兩半來記：
//     * 有效  ：物件還活著。解構一定安全；所有「沒有前置條件」的操作
//               都可以呼叫 —— 重新賦值、clear()、size()、empty()。
//               這些都不是未定義行為。
//     * 未指定：它的「內容」沒有任何保證。標準不要求它變成空字串、
//               不要求 size() 是 0、也不要求它維持原樣。
//   有前置條件的操作則不行：例如對已被搬空的容器呼叫 front() 或 pop_back()
//   （前置條件是「非空」）就可能是未定義行為。
//   實務守則一句話：move 之後，只重新賦值或讓它解構，不要讀它。
//
// 【4. 本檔為什麼不列印 move 後的內容】
//   原始版本印出 a 與 a.size()，並註明「通常是空字串，但不保證」。
//   註明是對的，但把觀察到的值寫進「預期輸出」仍然有害 ——
//   讀者會把它當成保證，而它不是。
//   本機 libstdc++ 實測（SSO 短字串與 heap 長字串都測過）move 後
//   size() 皆為 0，但這是實作細節。
//   本檔改成只驗證「標準真正保證的事」：move 之後重新賦值一定安全。
//   那才是你能寫進文件、也能依賴的行為。
//
// 【5. 右值引用變數本身是左值（本課的核心規則再一次登場）】
//   本檔第 5 段：
//       std::string&& rref = std::string("Omega");
//       process(rref);              → [const T&] 版本！因為 rref 有名字
//       process(std::move(rref));   → [T&&] 版本
//   同一個變數、兩種結果，差別只在有沒有再 cast 一次。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 std::move 是 noexcept、而且不做任何檢查
//   因為它只是 static_cast，不可能失敗。它甚至不要求 T 有移動操作 ——
//   對一個沒有移動建構函數的型別呼叫 std::move，程式照樣編譯通過，
//   只是接收端會安靜地選中拷貝版本。這是「沒有警告的效能陷阱」的來源。
//
// (B) const 會讓 move 靜默失效
//   const std::string cs = "x";
//   std::string t = std::move(cs);   // 編得過，但走的是「拷貝」
//   因為 std::move(cs) 的型別是 const std::string&&，
//   而移動建構函數收的是 std::string&&，接不了；
//   於是退回 const std::string&（拷貝建構）。沒有錯誤、沒有警告，只是變慢。
//
// (C) 不要對回傳值寫 return std::move(local);
//   那會把 local 變成 xvalue，使編譯器無法套用具名回傳值最佳化（NRVO），
//   從「完全不複製」退化成「一次移動」。正確寫法就是 return local;。
//   例外是「回傳成員變數」或「回傳參數」等 NRVO 本來就不適用的情形。
//
// 【注意事項 Pay Attention】
//   1. std::move 不移動任何東西，只是 cast。真正的動作在接收端。
//   2. move 之後不要讀原物件的內容；重新賦值或讓它解構才是正確用法。
//   3. const 物件無法被移動，會安靜退回拷貝 —— 該搬的東西不要宣告成 const。
//   4. 回傳區域變數時不要加 std::move，那會抑制比 move 更強的最佳化。
//   5. 教材中不要把「move 後觀察到的值」寫成預期輸出：那是實作細節，
//      不是標準保證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move 的真正語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 做了什麼？它會移動資料嗎？
//     答：不會。它的實作本質是 static_cast<remove_reference_t<T>&&>(t)，
//         一個位元組都沒搬。它只是把左值標記成右值，讓重載解析選中
//         移動建構或移動賦值。真正的搬移由接收端執行。
//         本檔第 2 段就是證明：process(std::move(a)) 進了 T&& 版本，
//         但那個函式只印字串沒搬東西，所以 a 完好如初。
//     追問：那對一個沒有移動建構函數的型別呼叫 std::move 會怎樣？
//         → 照樣編譯通過，接收端安靜地選中拷貝版本。
//           沒有錯誤也沒有警告 —— 這是典型的無聲效能陷阱。
//
// 🔥 Q2. std::move 之後，原物件處於什麼狀態？可以做什麼？
//     答：valid but unspecified。物件仍然有效（解構安全），
//         所有「沒有前置條件」的操作都能呼叫 —— 重新賦值、clear()、size()。
//         但它的內容沒有任何保證，所以不該去讀。
//         有前置條件的操作（例如對空容器呼叫 front()）則可能是未定義行為。
//     追問：那 std::vector 被 move 之後，size() 一定是 0 嗎？
//         → 標準沒有這樣要求。常見實作確實是 0（本機 libstdc++ 實測是 0），
//           但那是實作細節，不能當成可依賴的保證。
//
// ⚠️ 陷阱 1. 「回傳區域變數時加 std::move 可以少一次拷貝」——反而更慢。
//     答：return std::move(local); 把 local 變成 xvalue，
//         使編譯器無法套用具名回傳值最佳化（NRVO）。
//         本來可以「完全不複製」（直接建構在呼叫端），
//         現在退化成「一次移動」。正確寫法就是 return local;。
//     為什麼會錯：以為 move 一定比不 move 好。實際上 NRVO 比 move 更強 ——
//         它連移動都省掉了，而 std::move 反而擋住了它。
//
// ⚠️ 陷阱 2. 「把成員宣告成 const 比較安全」——會讓移動悄悄失效。
//     答：std::move(constObj) 的型別是 const T&&，
//         移動建構函數收的是 T&&，接不了，於是退回 const T& 的拷貝版本。
//         程式完全正確，只是每次都在做完整深拷貝，而且不會有任何提示。
//     為什麼會錯：把 const 當成純粹的保護修飾。
//         它是型別的一部分，會參與重載解析並改變最終選中的函式。
// ═══════════════════════════════════════════════════════════════════════════

// lesson31_move_preview.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -o lesson31d lesson31_move_preview.cpp

#include <iostream>
#include <string>
#include <utility>

void process(const std::string& s) {
    std::cout << "  [const T&] \"" << s << "\"\n";
}

void process(std::string&& s) {
    std::cout << "  [T&&]      \"" << s << "\"\n";
}

int main() {
    std::string a = "Alpha";
    std::string b = "Beta";

    std::cout << "1. 傳入左值：\n";
    process(a);                  // const T& 版本

    std::cout << "\n2. 傳入 std::move(a)：\n";
    process(std::move(a));       // T&& 版本

    std::cout << "\n3. move 後 a 的狀態：\n";
    // ⚠️ 這裡「刻意不列印」a 的內容或 size()。
    //    標準只保證 move 後的物件處於 valid but unspecified state ——
    //    內容沒有任何保證。把某次觀察到的值寫進預期輸出，
    //    等於把實作細節偽裝成標準保證。
    //    （附帶一提：本例的 process 收的是 T&&，函式內只是印字串、
    //      並未真的搬走 a 的資源，所以這裡 a 其實根本沒被動過。
    //      這正好說明「std::move 不等於已經移動」。）
    std::cout << "   （valid but unspecified：內容無保證，故不列印）\n";
    std::cout << "   能保證的是：無前置條件的操作都安全。\n";
    a.clear();                       // 無前置條件 → 保證安全
    std::cout << "   clear() 後 a.empty() = " << std::boolalpha << a.empty()
              << "  ← 這一步才是標準有保證的\n";

    std::cout << "\n4. 可以對 a 重新賦值：\n";
    a = "Gamma";                 // ✅ 重新賦值是安全的
    std::cout << "   a = \"" << a << "\"\n";

    std::cout << "\n5. 右值引用變數本身是左值：\n";
    std::string&& rref = std::string("Omega");
    process(rref);               // const T& 版本！因為 rref 是左值
    process(std::move(rref));    // T&& 版本

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 31 課：右值引用（Rvalue Reference）入門4.cpp" -o lesson31d

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：沒有位址、沒有執行緒 id、沒有耗時，重跑結果一致
//   （本機實測連跑 3 次逐位元組相同）。
// * 第 2 段 [T&&] 印出的仍是 "Alpha"，這不是錯誤而是本檔的重點：
//   std::move(a) 只把 a 標記成右值、讓重載解析選中 T&& 版本，
//   而那個版本只是印字串、沒有真的接手 a 的資源，所以 a 完好無缺。
//   「呼叫過 std::move 就等於已經被搬空」是錯的。
// * 第 3 段刻意不印 a 的內容與 size()：move 後是 valid but unspecified，
//   內容沒有任何標準保證。這裡只驗證標準真正保證的事 ——
//   clear() 這種無前置條件的操作一定安全，故 empty() 為 true 是可依賴的。
// * 第 5 段兩行同樣印 "Omega" 卻走不同重載：rref 有名字所以是左值 →
//   [const T&]；再 std::move(rref) 才變回右值 → [T&&]。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// 1. 傳入左值：
//   [const T&] "Alpha"
//
// 2. 傳入 std::move(a)：
//   [T&&]      "Alpha"
//
// 3. move 後 a 的狀態：
//    （valid but unspecified：內容無保證，故不列印）
//    能保證的是：無前置條件的操作都安全。
//    clear() 後 a.empty() = true  ← 這一步才是標準有保證的
//
// 4. 可以對 a 重新賦值：
//    a = "Gamma"
//
// 5. 右值引用變數本身是左值：
//   [const T&] "Omega"
//   [T&&]      "Omega"
