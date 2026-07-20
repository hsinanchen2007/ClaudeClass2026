// =============================================================================
//  04_if_constexpr.cpp  —  if constexpr (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/if
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、是什麼？                                               │
//  └────────────────────────────────────────────────────────────┘
//
//      if constexpr (compile-time-bool-expr) { ... }
//      else { ... }
//
//  跟普通 if 的差別：分支條件是「編譯期常數」，編譯器會「靜態」選擇分支 —
//  另一條根本不會被編譯（不會檢查語法、不會 instantiate template）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼好用？                                           │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 之前要在 template 中「依型別不同走不同分支」，要用 SFINAE / tag
//  dispatch / template specialization：
//
//      template <class T>
//      typename std::enable_if<std::is_pointer<T>::value, void>::type
//      print(T t) { std::cout << *t; }
//
//      template <class T>
//      typename std::enable_if<!std::is_pointer<T>::value, void>::type
//      print(T t) { std::cout << t; }
//
//  C++17：
//
//      template <class T>
//      void print(T t) {
//          if constexpr (std::is_pointer_v<T>) std::cout << *t;
//          else                                std::cout << t;
//      }
//
//  好讀、且分支裡的程式碼「對另一種 T 不需要編得過」 — 編譯器只 instantiate
//  你「真的會走」的那條。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：根據型別印不同訊息
//   * Demo 2：遞迴展開 variadic — base case 用 if constexpr 終止
// =============================================================================

/*
補充筆記：if_constexpr
  - if_constexpr 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - if constexpr 在編譯期丟棄不成立分支，常用於 template 中依型別選擇實作。
  - 被丟棄分支仍要能通過基本語法解析，但可包含對該型別不成立的表達式。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】if constexpr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. if constexpr 與一般 if 差在哪？
//     答：條件必須是編譯期的 contextually-converted bool 常量；未選中的分支在該
//         模板實例化中不會被實例化（discarded statement），所以可以寫「對某些 T
//         根本無法編譯」的程式碼。一般 if 兩個分支都得對所有 T 合法。
//     追問：discarded branch 完全不檢查嗎？（不是，仍會做不依賴模板參數的語法／
//           語意檢查，只有依賴 T 的部分被豁免）
//
// 🔥 Q2. if constexpr 相對 SFINAE / tag dispatch 的優勢？
//     答：SFINAE（enable_if）要為每個重載手動維護互斥條件，重載一多就爆炸且錯誤
//         訊息極差；tag dispatch 要額外定義 tag 型別與轉發層。
//         if constexpr 把分支收攏在單一函式本體內，可讀性高、不需互斥。
//         三者都是編譯期決議，執行期無分支殘留。
//     追問：能取代所有 SFINAE 嗎？（不能，它無法影響重載決議／是否參與候選集，
//           那仍需 SFINAE 或 C++20 concepts）
//
// ⚠️ 陷阱. if constexpr 在「非模板」函式中還有不實例化的效果嗎？
//     答：沒有。豁免只對依賴模板參數的部分生效；在非模板函式中兩個分支都必須完整
//         合法，它只等於一個編譯期已知的 if，不能拿來藏「這型別根本編不過」的碼。
//     為什麼會錯：多數人把 if constexpr 記成「沒選到就不編譯」，忽略前提是
//         template instantiation。同理 discarded 分支裡寫 static_assert(false) 會
//         直接失敗，因為它不依賴 T；要寫成 static_assert(always_false_v<T>)。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>

template <class T>
void describe(T x) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "[describe] integer: " << x << '\n';
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "[describe] floating: " << x << '\n';
    } else if constexpr (std::is_pointer_v<T>) {
        std::cout << "[describe] pointer to: " << *x << '\n';
    } else {
        std::cout << "[describe] something: " << x << '\n';
    }
}

// 遞迴展開 variadic — terminate with if constexpr
template <class First, class... Rest>
void printAll(const First& f, const Rest&... rest) {
    std::cout << f;
    if constexpr (sizeof...(rest) > 0) {
        std::cout << ", ";
        printAll(rest...);          // 只在還有元素時遞迴 — 自然 terminate
    } else {
        std::cout << '\n';
    }
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：根據型別走不同分支
    // ─────────────────────────────────────────────────────────
    describe(42);                   // integer
    describe(3.14);                 // floating
    int  n = 7;
    describe(&n);                   // pointer
    describe(std::string{"hi"});    // something

    // ─────────────────────────────────────────────────────────
    // Demo 2：variadic 遞迴
    // ─────────────────────────────────────────────────────────
    printAll(1, 2.5, "hello", 'X', std::string{"world"});

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：跟普通 if 的「optimize away」差在哪？
    //    A：普通 if (constexpr_bool) 在 -O2 也會被 dead-code 消掉，但「另
    //       一條分支仍要編譯通過」。if constexpr 不要求 — 另一條只要在「該
    //       T 確實會走」的情況下才檢查語法。所以對「型別專屬」的程式碼
    //       (例如 *t 對非指標 T 不合法) 必須 if constexpr。
    //
    //  Q2：可以巢狀嗎？
    //    A：可以；常見 if/else if/else if/else 鏈處理多型別。注意 `_v` 後
    //       綴是 C++17 type traits 的 short alias（如 is_integral_v 等同
    //       is_integral<T>::value）。
    //
    //  Q3：跟 std::is_constant_evaluated() (C++20) 差別？
    //    A：if constexpr 是「依編譯期常數」分支；is_constant_evaluated() 是
    //       「依目前是否在編譯期 evaluate」分支。前者依「型別/常數」，後者
    //       依「上下文」 — 兩個目的不同。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：通用 toString — 不同型別走不同序列化
    //   工作上常見：debug print、log helper 對任意 T 友善
    // ─────────────────────────────────────────────────────────
    auto toString = [](auto&& x) {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(x);
        } else if constexpr (std::is_convertible_v<T, std::string>) {
            return std::string{x};                  // const char* / string
        } else {
            return std::string{"<unprintable>"};
        }
    };
    std::cout << "[Demo3] toString(42)      = " << toString(42)      << '\n';
    std::cout << "[Demo3] toString(3.14)    = " << toString(3.14)    << '\n';
    std::cout << "[Demo3] toString(\"hi\")   = " << toString("hi")    << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：safeDiv — 整數除法防 0，浮點除法直接除
    //   工作上常見：通用算術 helper 對 int/double 不同處理
    // ─────────────────────────────────────────────────────────
    auto safeDiv = [](auto a, auto b) {
        using T = decltype(a);
        if constexpr (std::is_integral_v<T>) {
            return b == 0 ? T{0} : a / b;            // 整數防 0
        } else {
            return a / b;                            // 浮點不防（會得 inf/nan）
        }
    };
    std::cout << "[Demo4] safeDiv(10, 0)   = " << safeDiv(10, 0)   << '\n';   // 0
    std::cout << "[Demo4] safeDiv(10, 3)   = " << safeDiv(10, 3)   << '\n';   // 3
    std::cout << "[Demo4] safeDiv(10.0, 4.0) = " << safeDiv(10.0, 4.0) << '\n'; // 2.5

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 04_if_constexpr.cpp -o 04_if_constexpr

// === 預期輸出 ===
// [describe] integer: 42
// [describe] floating: 3.14
// [describe] pointer to: 7
// [describe] something: hi
// 1, 2.5, hello, X, world
// [Demo3] toString(42)      = 42
// [Demo3] toString(3.14)    = 3.140000
// [Demo3] toString("hi")   = hi
// [Demo4] safeDiv(10, 0)   = 0
// [Demo4] safeDiv(10, 3)   = 3
// [Demo4] safeDiv(10.0, 4.0) = 2.5
