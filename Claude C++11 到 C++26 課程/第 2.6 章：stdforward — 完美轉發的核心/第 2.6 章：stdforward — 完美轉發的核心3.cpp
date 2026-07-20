// =============================================================================
//  第 2.6 章：std::forward — 完美轉發的核心 (3)
//  主題：三種傳遞方式的正面對決 — arg / std::move / std::forward
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>
//     constexpr T&& forward(std::remove_reference_t<T>&  t) noexcept;  // (1)
//     constexpr T&& forward(std::remove_reference_t<T>&& t) noexcept;  // (2)
//     constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;     // 對照組
//
//   標準版本：兩者皆 C++11 引入；constexpr 自 C++14 起
//   複雜度  ：O(1)，皆為編譯期 static_cast，不產生執行期指令
//
// 【詳細解釋 Explanation】
//
// 【1. 一張表看完三種寫法】
//   在 template<class T> void demo(T&& arg) 內部：
//
//       寫法                     傳入左值時      傳入右值時      性質
//       ----------------------   ------------   ------------   ------------------
//       check(arg)               左值           左值 ❌        原封不動（但已失真）
//       check(std::move(arg))    右值 ❌        右值           無條件轉右值
//       check(std::forward<T>)   左值           右值           有條件（完美轉發）✅
//
//   兩個 ❌ 就是問題所在：前者讓右值退化成左值（該移動卻複製，效能損失）；
//   後者讓左值升格為右值（呼叫端的物件被偷走，正確性損失 —— 更嚴重）。
//
// 【2. 為什麼 check(arg) 永遠是左值】
//   arg 是具名變數，運算式 arg 的值類別恆為 lvalue，與它的型別無關。
//   詳見第 1 檔【3.】。
//
// 【3. 為什麼 check(std::move(arg)) 永遠是右值】
//   std::move 是無條件轉型，它根本不看 T：
//       template<class T>
//       constexpr std::remove_reference_t<T>&& move(T&& t) noexcept {
//           return static_cast<std::remove_reference_t<T>&&>(t);
//       }
//   注意回傳型別先 remove_reference 再加 &&，所以不管 T 推導成什麼，
//   結果一定是右值引用 —— 沒有任何條件判斷的餘地。
//
// 【4. 為什麼 std::forward<T> 兩種都對】
//   因為它把 T 原封不動地放回 static_cast<T&&>，讓引用折疊自己算：
//       T = X&（原本是左值）→ X& && → 折疊為 X&  → 左值
//       T = X （原本是右值）→ X&&                → 右值
//   條件判斷是「型別系統」做的，不是執行期的 if。
//
// 【概念補充 Concept Deep Dive】
//
//   ■ 為什麼「左值被誤當右值」比「右值被誤當左值」嚴重得多？
//     右值退化成左值：該移動變成複製 → 只是慢，程式仍然正確。
//     左值升格成右值：呼叫端「還要用」的物件被搬空 →
//       物件進入 valid but unspecified state（有效但未指定狀態）。
//     後者是正確性 bug，而且症狀常常在很遠的地方才爆出來，極難追。
//     這也是為什麼在泛型包裝函式裡濫用 std::move 特別危險。
//
//   ■ 「valid but unspecified」到底是什麼意思？
//     被移動後的標準庫物件仍然是「有效物件」：解構是安全的，
//     呼叫沒有前置條件的操作（clear()、size()、operator=）也是安全的。
//     但它的「值」是未指定的 —— 標準不保證它一定變成空字串。
//     所以絕不能寫「移動後一定會印出空字串」這種敘述；
//     常見實作對長字串多半留下空字串，對短字串（SSO）則不一定，
//     這是實作定義的行為。正確做法是：移動後先重新賦值，再繼續使用。
//     本檔 main() 只印出「是否為空」並附上這個但書，不宣稱固定結果。
//
// 【注意事項 Pay Attention】
//   1. 在 forwarding reference 上用 std::move，等於單方面替呼叫端做了
//      「放棄物件」的決定 —— 除非你是最後一站（sink function），否則不要這樣做。
//   2. std::forward<T> 的 T 要用函式模板推導出的 T，別自己另外編一個型別。
//   3. 這三種寫法的執行期成本完全相同（都是 0）；差別只在多載決議選到誰。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三種轉發方式與移動後狀態
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 在 template<class T> void f(T&& arg) 裡，直接寫 std::move(arg) 有什麼風險？
//     答：如果呼叫端傳的是左值，std::move 會把它無條件轉成右值，
//         下游可能真的把它搬走 —— 但呼叫端根本沒同意，它之後還要用那個物件。
//         這是正確性 bug，不是效能問題。在 forwarding reference 上，
//         正確做法一律是 std::forward<T>(arg)。
//     追問：那什麼時候該用 std::move？
//         → 當參數型別是「明確的右值引用」（如 void sink(std::string&& s)）、
//           或你是所有權鏈的最後一站時。這時呼叫端已經用 && 表明放棄所有權了。
//
// ⚠️ 陷阱. 「一個物件被 std::move 之後，它一定變成空的」—— 這句話對嗎？
//     答：不對。標準只保證移動後的物件處於 valid but unspecified state
//         （有效但未指定），不保證是空的、也不保證是任何特定值。
//         你可以安全地解構它、或重新賦值給它，但不可以「假設」它的內容。
//     為什麼會錯：多數人拿 std::string 做實驗，看到印出空字串就以為是規則。
//         那只是常見實作在該長度下的行為（短字串走 SSO 時甚至可能原封不動），
//         屬於實作定義，換一個標準庫或換一個字串長度就可能不同。
//
// ⚠️ 陷阱 2. 既然 forward 這麼萬用，是不是所有參數都該寫成 T&& + forward？
//     答：不是。forwarding reference 會「什麼都收」，包括你不想要的型別，
//         很容易在多載決議中壓過其他候選（例如壓過複製建構子），
//         導致奇怪的錯誤訊息。它適合「中轉層」（工廠、包裝器、emplace）；
//         如果函式只是要讀資料，const T& 更精確也更好讀。
//     為什麼會錯：把「萬用」當成「總是最好」。萬用的代價是精確度與錯誤訊息品質。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>

// 參數名刻意省略：只需顯示「哪個多載被選中」，不讀取內容（避免 -Wunused-parameter）。
void check(const std::string&) { std::cout << "  左值\n"; }
void check(std::string&&)      { std::cout << "  右值\n"; }

template<typename T>
void demo(T&& arg) {
    std::cout << "原樣傳遞 arg:       ";
    check(arg);                         // 永遠是左值

    std::cout << "std::move(arg):     ";
    check(std::move(arg));              // 永遠是右值

    std::cout << "std::forward<T>(arg):";
    check(std::forward<T>(arg));        // 保持原始值類別
}

// -----------------------------------------------------------------------------
// 示範「在 forwarding reference 上用 std::move」為什麼是正確性 bug：
// 呼叫端傳的是左值、之後還要用，卻被中轉層擅自搬走。
// -----------------------------------------------------------------------------
void sink(std::string&& s) {
    std::string stolen = std::move(s);  // 真的把資料搬走
    std::cout << "  sink 取走了: \"" << stolen << "\"\n";
}
void sink(const std::string& s) {
    std::cout << "  sink 只是讀取: \"" << s << "\"\n";
}

template<typename T>
void bad_relay(T&& arg) { sink(std::move(arg));         }  // ❌ 擅自替呼叫端做決定
template<typename T>
void good_relay(T&& arg) { sink(std::forward<T>(arg)); }   // ✅ 尊重呼叫端

int main() {
    std::string s = "Hello";

    std::cout << "=== 傳入左值 ===\n";
    demo(s);

    std::cout << "\n=== 傳入右值 ===\n";
    demo(std::string("tmp"));

    // ─────────────────────────────────────────────────────────
    // 對照：中轉層濫用 std::move 的後果
    // ─────────────────────────────────────────────────────────
    std::cout << "\n=== bad_relay：中轉層擅自 std::move ===\n";
    std::string owned = "我還要用這個字串";
    bad_relay(owned);
    // owned 現在處於 valid but unspecified state（有效但未指定）。
    // 標準「不保證」它變成空字串，因此這裡只印出是否為空，並標明這是實作相關結果。
    std::cout << "  呼叫端的 owned 是否為空: "
              << (owned.empty() ? "是" : "否")
              << "  ← 本機實測結果，標準不保證，屬實作定義\n";

    std::cout << "\n=== good_relay：正確使用 forward ===\n";
    std::string kept = "我還要用這個字串";
    good_relay(kept);
    std::cout << "  呼叫端的 kept 仍然完好: \"" << kept << "\"\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心3.cpp" -o fwd3

// （下方 owned 是否為空的那一行為本機實測值，屬實作定義，換編譯器／字串長度可能不同）

// === 預期輸出 ===
//
// === 傳入左值 ===
// 原樣傳遞 arg:         左值
// std::move(arg):       右值
// std::forward<T>(arg):  左值
//
// === 傳入右值 ===
// 原樣傳遞 arg:         左值
// std::move(arg):       右值
// std::forward<T>(arg):  右值
//
// === bad_relay：中轉層擅自 std::move ===
//   sink 取走了: "我還要用這個字串"
//   呼叫端的 owned 是否為空: 是  ← 本機實測結果，標準不保證，屬實作定義
//
// === good_relay：正確使用 forward ===
//   sink 只是讀取: "我還要用這個字串"
//   呼叫端的 kept 仍然完好: "我還要用這個字串"
