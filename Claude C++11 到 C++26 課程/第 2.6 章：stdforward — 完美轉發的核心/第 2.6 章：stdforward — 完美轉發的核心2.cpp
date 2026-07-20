// =============================================================================
//  第 2.6 章：std::forward — 完美轉發的核心 (2)
//  主題：解法 — std::forward 與引用折疊（reference collapsing）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>
//     template<class T>
//     constexpr T&& forward(std::remove_reference_t<T>&  t) noexcept;  // (1) 接左值
//     template<class T>
//     constexpr T&& forward(std::remove_reference_t<T>&& t) noexcept;  // (2) 接右值
//
//   標準版本：C++11 引入；constexpr 自 C++14 起
//   複雜度  ：O(1)，純編譯期 static_cast，執行期零成本
//   ※ 實作相關：本機 libstdc++（g++ 15.2）即使在 -std=c++11 -pedantic-errors 下
//     也接受 std::forward 出現在常數運算式中；但標準上「保證 constexpr」是 C++14 起。
//     跨編譯器寫程式時請以 C++14 為準。
//
// 【詳細解釋 Explanation】
//
// 【1. std::forward 是「有條件的轉型」】
//   std::move  ：無條件把引數轉成右值（xvalue）—— 不管你原本是什麼。
//   std::forward：依照模板參數 T 的推導結果決定 —— 原本是左值就還原成左值，
//                 原本是右值才轉成右值。
//   所以 forward 的正式名稱是 conditional cast（有條件轉型）。
//   條件寫在哪裡？寫在 T 裡面。這就是為什麼一定要寫 std::forward<T>。
//
// 【2. forwarding reference 的推導規則】
//   對 template<class T> void wrapper(T&& arg)：
//       傳入 std::string 左值 → T 推導為 std::string&
//       傳入 std::string 右值 → T 推導為 std::string   （不帶 &）
//   注意左值那一列：T 自己就帶了一個 &。這是整套機制的關鍵，
//   因為「原本是不是左值」這個唯一的線索，就存在 T 帶不帶 & 上面。
//
// 【3. 引用折疊如何把 T&& 變成正確的型別】
//   把上面推導出的 T 代回參數宣告 T&&，會出現「引用的引用」，
//   由引用折疊規則（C++11 起，[dcl.ref]）化簡：
//       傳左值：T = string&  → T&& = string& &&  → 折疊為 string&   （綁得住左值）
//       傳右值：T = string   → T&& = string&&                        （綁得住右值）
//   同一個寫法 T&&，因此能同時吃下左值與右值 —— 這正是它被稱為
//   forwarding reference（轉發引用；Scott Meyers 舊稱 universal reference）的原因。
//
// 【4. forward 內部做了什麼】
//   概念上等價於：
//       template<class T>
//       constexpr T&& forward(std::remove_reference_t<T>& t) noexcept {
//           return static_cast<T&&>(t);
//       }
//   再套一次引用折疊看回傳型別：
//       T = string&  → 回傳 string& &&  → 折疊為 string&  → 結果是「左值」✅
//       T = string   → 回傳 string&&                       → 結果是「右值」✅
//   同一行 static_cast<T&&>(t)，因為 T 不同而產生兩種值類別。
//   整個「完美轉發」就是這麼一個編譯期的型別遊戲，執行期什麼事都沒發生。
//
// 【概念補充 Concept Deep Dive】
//
//   ■ 引用折疊的四條規則（只有這四條，背起來就通了）
//       T&  &   →  T&      （左值引用 的 左值引用 → 左值引用）
//       T&  &&  →  T&      （左值引用 的 右值引用 → 左值引用）
//       T&& &   →  T&      （右值引用 的 左值引用 → 左值引用）
//       T&& &&  →  T&&     （只有這一條保持右值引用）
//     一句話記法：「只要沾到一個 &，結果就是 &；唯有 && && 才是 &&。」
//     左值引用具有傳染性（infectious）。
//     本檔 main() 用 static_assert 把這四條規則實際驗證一次。
//
//     ※ 注意：你不能自己手寫 `int& &`，那是語法錯誤。引用折疊只會在
//       模板參數推導、typedef / using 別名、decltype 這些「間接」場合發生。
//
//   ■ 為什麼 std::forward 必須寫 <T>，而 std::move 不用？
//     因為兩者需要的資訊量不同：
//       std::move(x)      只需要知道 x 的型別 → 可以從引數推導 → 不必寫模板引數。
//       std::forward<T>(x) 需要知道「x 原本是左值還是右值」。
//     而在函式內部，x 是具名變數 → 運算式 x 恆為 lvalue，
//     從 x 本身已經「問不出」原始值類別了。唯一還記得這件事的是 T
//     （T 帶 & 代表原本是左值，不帶 & 代表原本是右值）。
//     所以 T 必須由你明確給出 —— 資訊只存在那裡。
//
//   ■ 為什麼參數型別要寫成 remove_reference_t<T>& 而不是 T&？
//     這是刻意的「防呆設計」。remove_reference_t<T> 是
//     typename remove_reference<T>::type，屬於 non-deduced context
//     （非推導語境）—— 編譯器無法從引數反推出 T（多個 T 會對到同一個結果）。
//     於是模板引數推導被「封死」，你不寫 <T> 就無法呼叫。
//     結果是：std::forward(x) 直接編譯失敗，而不是靜默地做錯事。
//     本機實測 g++ 15.2 錯誤訊息：
//         error: no matching function for call to 'forward(int&)'
//     ★ 標準庫用「不可推導」當作安全機制，逼你把意圖寫清楚。
//
// 【注意事項 Pay Attention】
//   1. std::forward<T>(arg) 中的 T，必須就是該函式模板推導出來的那個 T，
//      不要寫成 std::forward<decltype(arg)>(arg)（型別多帶一層 &，語意會不同）。
//      在 auto&& / C++20 的 auto 參數上才用 decltype(x)（見第 8 檔）。
//   2. 對「同一個參數」只能 forward 一次（見第 7 檔的陷阱說明）。
//   3. forward 只還原值類別，不會幫你加上或去掉 const。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move vs std::forward
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::move 和 std::forward 有什麼差別？
//     答：兩者都不搬動任何資料，都只是編譯期的 static_cast。差別在條件：
//         std::move    是「無條件」轉成右值 —— 不管引數原本是什麼。
//         std::forward 是「有條件」轉型 —— 依 T 是否帶 & 決定還原成左值或右值。
//         所以 move 用在「我確定要放棄這個物件」；
//         forward 用在「我只是中轉，不做決定」。
//     追問：那我可以用 std::forward 取代 std::move 嗎？
//         → 技術上 std::forward<T>(x)（T 為非引用）等價於 std::move(x)，
//           但語意會誤導讀者，而且失去 move 的自我說明性。Scott Meyers 的建議是：
//           對右值引用用 move，對 forwarding reference 用 forward。
//
// 🔥 Q2. 為什麼同一個寫法 T&& 既能綁左值又能綁右值？
//     答：因為傳左值時 T 被推導為 X&，代回去變成 X& &&，
//         由引用折疊化簡為 X& —— 參數實際上變成了左值引用。
//         傳右值時 T 推導為 X，參數就是 X&&。是推導＋折疊兩步合力的結果，
//         不是 && 本身有什麼特異功能。
//     追問：那 const T&& 呢？
//         → 不是 forwarding reference。加了 const 就喪失資格，只能綁右值。
//
// ⚠️ 陷阱. 為什麼一定要寫 std::forward<T>(x)，不能寫 std::forward(x)？
//     答：因為 forward 的參數型別是 remove_reference_t<T>&，屬於非推導語境，
//         編譯器無法反推 T，所以 std::forward(x) 根本無法通過多載決議。
//         本機實測錯誤訊息："no matching function for call to 'forward(int&)'"。
//         更根本的原因是：資訊只存在 T 裡，x 本身是具名變數、恆為左值，
//         就算能推導也推導不出「原本是右值」這件事。
//     為什麼會錯：多數人以為 forward 像 move 一樣「看引數就知道要做什麼」。
//         實際上 move 只需要型別（可推導），forward 需要值類別（不可推導）。
//         標準庫索性把推導封死，讓你在編譯期就被擋下來。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// 參數名刻意省略：只需顯示「哪個多載被選中」，不讀取內容（避免 -Wunused-parameter）。
void target(const std::string&) { std::cout << "  target(const T&) 左值版本\n"; }
void target(std::string&&)      { std::cout << "  target(T&&) 右值版本\n"; }

template<typename T>
void wrapper(T&& arg) {
    std::cout << "  轉發中...\n";
    target(std::forward<T>(arg));  // 完美轉發
}

// -----------------------------------------------------------------------------
// 用 static_assert 把「引用折疊四規則」直接編譯期驗證一次。
// 這些斷言若不成立，這個檔案根本編不過 —— 比印出來更有說服力。
// -----------------------------------------------------------------------------
namespace collapse_proof {
    using X   = std::string;
    using LR  = X&;    // 左值引用
    using RR  = X&&;   // 右值引用

    static_assert(std::is_same<LR&,  X&>::value,  "規則 1: &  &  -> &");
    static_assert(std::is_same<LR&&, X&>::value,  "規則 2: &  && -> &");
    static_assert(std::is_same<RR&,  X&>::value,  "規則 3: && &  -> &");
    static_assert(std::is_same<RR&&, X&&>::value, "規則 4: && && -> &&");
}

// 觀察 T 到底被推導成什麼：左值 → T = string&；右值 → T = string
template<typename T>
void show_deduction(T&& arg) {
    std::cout << "  T 是否為左值引用: "
              << (std::is_lvalue_reference<T>::value ? "是 → 原本傳入左值"
                                                     : "否 → 原本傳入右值")
              << "\n";
    target(std::forward<T>(arg));
}

int main() {
    std::string s = "Hello";

    std::cout << "傳入左值:\n";
    wrapper(s);                     // 左值 → 轉發為左值 ✅

    std::cout << "\n傳入右值:\n";
    wrapper(std::string("tmp"));    // 右值 → 轉發為右值 ✅

    std::cout << "\n傳入 std::move:\n";
    wrapper(std::move(s));          // 右值 → 轉發為右值 ✅
    s = "Hello";                    // std::move 後的 s 狀態未指定，重新賦值才繼續使用

    // ─────────────────────────────────────────────────────────
    // 觀察 T 的推導結果：這是引用折疊的「輸入」
    // ─────────────────────────────────────────────────────────
    std::cout << "\n=== T 被推導成什麼 ===\n";
    std::cout << "傳左值 s:\n";
    show_deduction(s);
    std::cout << "傳右值 string(\"tmp\"):\n";
    show_deduction(std::string("tmp"));

    std::cout << "\n=== 引用折疊四規則（已於編譯期以 static_assert 驗證）===\n";
    std::cout << "  &  &  -> &\n";
    std::cout << "  &  && -> &\n";
    std::cout << "  && &  -> &\n";
    std::cout << "  && && -> &&   ← 只有這條保持右值\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心2.cpp" -o fwd2

// === 預期輸出 ===
// 傳入左值:
//   轉發中...
//   target(const T&) 左值版本
//
// 傳入右值:
//   轉發中...
//   target(T&&) 右值版本
//
// 傳入 std::move:
//   轉發中...
//   target(T&&) 右值版本
//
// === T 被推導成什麼 ===
// 傳左值 s:
//   T 是否為左值引用: 是 → 原本傳入左值
//   target(const T&) 左值版本
// 傳右值 string("tmp"):
//   T 是否為左值引用: 否 → 原本傳入右值
//   target(T&&) 右值版本
//
// === 引用折疊四規則（已於編譯期以 static_assert 驗證）===
//   &  &  -> &
//   &  && -> &
//   && &  -> &
//   && && -> &&   ← 只有這條保持右值
