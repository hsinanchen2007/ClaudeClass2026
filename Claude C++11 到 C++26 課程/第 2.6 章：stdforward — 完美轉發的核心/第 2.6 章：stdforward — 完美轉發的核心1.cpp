// =============================================================================
//  第 2.6 章：std::forward — 完美轉發的核心 (1)
//  主題：問題展示 — 為什麼「原樣傳遞」永遠做不到完美轉發
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
//             （C++11 版簽名寫作 typename std::remove_reference<T>::type&，且非 constexpr）
//   複雜度  ：O(1)。它只是一個 static_cast，編譯後不產生任何機器碼。
//
//   本檔「不使用」forward — 這一檔的任務是先把病看清楚，第 2 檔才給藥。
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼叫「完美轉發」】
//   包裝函式 wrapper 收下引數後再交給 target，希望 target 看到的東西
//   和「使用者直接呼叫 target」完全一樣 —— 型別一樣、const 一樣，
//   最關鍵的是「值類別（value category）」也要一樣：
//       使用者傳左值 → target 應選到 const T& 版本（該複製就複製）
//       使用者傳右值 → target 應選到 T&&     版本（該移動就移動）
//   只要有任何一項走樣，就不叫「完美」轉發。
//
// 【2. 第一種失敗：用 const T& 接】
//       template<typename T> void wrapper_v1(const T& arg) { target(arg); }
//   const T& 既能綁左值也能綁右值，看起來很萬用 —— 但它把資訊「壓平」了：
//   一旦綁上去，arg 的型別就是 const T&，「原本是不是右值」再也查不回來。
//   而且多了一層 const，連移動都做不了（移動建構子不吃 const）。
//
// 【3. 第二種失敗：用 T&& 接、卻直接把 arg 傳出去】
//       template<typename T> void wrapper_v2(T&& arg) { target(arg); }
//   T&& 這個「參數宣告」確實保住了資訊（見第 2 檔的引用折疊說明），
//   問題出在函式「內部」使用 arg 的那一刻：
//
//       ★ 具名的右值引用，本身是左值。★
//
//   arg 有名字、可以取位址、可以重複使用，因此運算式 `arg` 的值類別是
//   lvalue —— 即使它的型別寫作 std::string&&。所以 target(arg) 永遠選到
//   左值多載。這是初學者最常撞到的一面牆。
//
// 【4. 為什麼標準要這樣規定】
//   如果具名右值引用還算右值，那
//       void f(std::string&& s) { g(s); h(s); }
//   就會在 g(s) 把 s 搬走之後，讓 h(s) 拿到一個被掏空的物件 —— 而且完全看不出來。
//   「具名者為左值」這條規則強迫你必須明確寫出 std::move / std::forward，
//   把「我要放棄這個物件」變成程式碼裡看得見的一行字。
//
// 【概念補充 Concept Deep Dive】
//   型別（type）與值類別（value category）是兩個獨立的軸，初學者常把它們混為一談：
//
//       宣告                      arg 的「型別」       運算式 arg 的「值類別」
//       -----------------------   ------------------   ----------------------
//       std::string   arg         std::string          lvalue
//       std::string&  arg         std::string&         lvalue
//       std::string&& arg         std::string&&        lvalue  ← 關鍵！
//
//   最右欄才是多載決議（overload resolution）真正在看的東西。
//   「型別上是右值引用」不等於「用起來是右值」。std::forward 的全部工作，
//   就是把最右欄那一格改回它原本該有的樣子。
//
// 【注意事項 Pay Attention】
//   1. const T& 不是「萬用接法」，它會同時吃掉「右值資訊」與「可變性」。
//   2. 具名右值引用是左值 —— 這是標準規則，不是編譯器 bug。
//   3. 本檔的 wrapper_v1 / wrapper_v2 都能正常編譯執行。
//      它們不是語法錯誤，而是「語意上悄悄做錯事」，這種 bug 最難抓。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】具名右值引用的值類別
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. void f(std::string&& s) 裡面，運算式 s 是左值還是右值？
//     答：左值。s 的「型別」是 std::string&&，但它有名字、可取位址，
//         所以運算式 s 的「值類別」是 lvalue。要當右值用必須寫
//         std::move(s)（無條件）或 std::forward<T>(s)（有條件）。
//     追問：那為什麼標準要這樣設計？
//         → 若具名右值引用算右值，函式內連續用兩次就會靜默地用到被搬空的物件。
//           強制寫出 move/forward，等於讓「放棄所有權」這件事必須看得見。
//
// 🔥 Q2. T&& 什麼時候是 forwarding reference，什麼時候只是普通右值引用？
//     答：只有在「T 是這個函式自己推導出來的模板參數」、且參數形式恰好是裸 T&&
//         時，才是 forwarding reference。以下都「不是」：
//           template<class T> void f(std::vector<T>&& v)
//                              ← 形式不是裸 T&&，是普通右值引用
//           template<class T> struct Box { void push(T&& v); };
//                              ← T 在類別實體化時就固定了，push 沒有推導任何東西
//                                → 普通右值引用
//           template<class T> void f(const T&& x)
//                              ← 有 const，喪失資格
//     追問：怎麼快速自我驗證？
//         → 拿一個左值去呼叫它。forwarding reference 收得下；
//           普通右值引用會直接編譯失敗（本機實測 g++ 15.2 訊息：
//           "cannot bind rvalue reference of type 'std::vector<int>&&' to lvalue"）。
//
// ⚠️ 陷阱. wrapper_v2 用了 T&&，為什麼還是失敗？「我不是已經用了萬用引用嗎？」
//     答：T&& 只保住了「參數宣告」這一層，沒有保住「使用那一刻」的值類別。
//         參數型別對了，但 target(arg) 裡的 arg 是具名變數 → 一律 lvalue。
//     為什麼會錯：多數人腦中的模型是「型別對了，行為就會對」。
//         但多載決議看的是運算式的值類別，不是變數宣告時寫了什麼。
//         少了 std::forward<T>(arg) 這一步，資訊就在最後一公尺掉了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>

// 參數名刻意省略：這兩個多載只用來顯示「哪一個被選中」，不讀取內容。
// 省略名字可避免 -Wextra 的 -Wunused-parameter 警告。
void target(const std::string&) { std::cout << "  target(const T&) 左值版本\n"; }
void target(std::string&&)      { std::cout << "  target(T&&) 右值版本\n"; }

// 嘗試 1：用 const T& 轉發
template<typename T>
void wrapper_v1(const T& arg) {
    target(arg);  // arg 永遠是左值 → 永遠呼叫左值版本
}

// 嘗試 2：用 T&& 轉發
template<typename T>
void wrapper_v2(T&& arg) {
    target(arg);  // arg 有名字 → 還是左值 → 還是呼叫左值版本！
}

int main() {
    std::string s = "Hello";

    std::cout << "=== wrapper_v1（const T&）===\n";
    wrapper_v1(s);                   // 左值 → 左值版本 ✅
    wrapper_v1(std::string("tmp"));  // 右值 → 左值版本 ❌ 應該是右值版本

    std::cout << "\n=== wrapper_v2（T&&）===\n";
    wrapper_v2(s);                   // 左值 → 左值版本 ✅
    wrapper_v2(std::string("tmp"));  // 右值 → 左值版本 ❌ 還是錯！

    // ─────────────────────────────────────────────────────────
    // 課堂重點：兩次嘗試都「編譯成功、執行正常、結果錯誤」
    // ─────────────────────────────────────────────────────────
    std::cout << "\n=== 結論 ===\n";
    std::cout << "  const T&：右值資訊與可變性一起被吃掉\n";
    std::cout << "  T&& 但直接傳 arg：參數型別對了，但具名變數是左值\n";
    std::cout << "  → 解法是 std::forward<T>(arg)，見第 2 檔\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心1.cpp" -o fwd1

// === 預期輸出 ===
// === wrapper_v1（const T&）===
//   target(const T&) 左值版本
//   target(const T&) 左值版本
//
// === wrapper_v2（T&&）===
//   target(const T&) 左值版本
//   target(const T&) 左值版本
//
// === 結論 ===
//   const T&：右值資訊與可變性一起被吃掉
//   T&& 但直接傳 arg：參數型別對了，但具名變數是左值
//   → 解法是 std::forward<T>(arg)，見第 2 檔
