// =============================================================================
//  第 2.7 章 範例 9  —  陷阱：完美轉發 static const 成員造成的連結錯誤
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（std::forward）
//   問題形式：
//       class Config { static const int MAX_SIZE = 100; };   // 只是宣告 + 初值
//       wrapper(Config::MAX_SIZE);   // T&& 推成 const int& → odr-use → 需要定義
//   本機 g++ 15.2 實測，缺少類別外定義時的錯誤（發生在連結期，不是編譯期）：
//       undefined reference to `Config::MAX_SIZE'
//   三種解法：類別外定義（C++11）／inline variable（C++17）／constexpr（C++17 隱含 inline）。
//
// 【詳細解釋 Explanation】
//
// 【1. 「宣告」與「定義」的差別，是本陷阱的全部】
//   class Config { static const int MAX_SIZE = 100; };
//   這一行做了兩件事：宣告有這個靜態成員、並給它一個編譯期常數值。
//   它「沒有」做的事是：在程式的資料區段裡真的配置一塊記憶體給它。
//   所以：
//     * 只需要「值」的場合 → 編譯器直接把 100 貼進程式碼，不需要記憶體
//     * 需要「位址」的場合 → 必須真的有一塊記憶體 → 需要定義
//   類別外那一行 const int Config::MAX_SIZE; 就是在補上「定義」。
//   注意初值不可以再寫一次，因為初值已經在類別內給過了。
//
// 【2. 什麼叫 odr-use（One Definition Rule use）】
//   粗略但實用的判準：**這個運算式需要它的位址嗎？**
//     target(Config::MAX_SIZE)  參數是 const int& → 參考需要位址 → odr-use ✓
//     wrapper(Config::MAX_SIZE) T&& 推成 const int& → 同樣需要位址 → odr-use ✓
//     takeByValue(Config::MAX_SIZE) 參數是 int → 只要值 → 不是 odr-use ✗
//   本機實測驗證過這個差異：傳值版本即使沒有類別外定義也能正常連結並印出 100，
//   而 const int& 版本會在連結期失敗。
//
// 【3. 為什麼完美轉發特別容易踩到】
//   轉發參考 T&& 遇到左值會推導成參考型別，而參考幾乎必然 odr-use。
//   也就是說：**把一個原本傳值傳得好好的常數改成用轉發器傳，就可能突然連結失敗。**
//   而且錯誤發生在連結期，訊息只有 undefined reference，不會指向任何一行程式碼，
//   對第一次遇到的人幾乎不可能從訊息猜出原因。
//
// 【4. 三種解法的比較】
//   (a) 類別外定義（C++11 起可用，本檔採用）
//       const int Config::MAX_SIZE;
//       缺點：必須放在「恰好一個」.cpp 檔裡，放進標頭檔會造成重複定義。
//   (b) inline variable（C++17）
//       static inline const int MAX_SIZE = 100;
//       可直接寫在標頭檔，連結器負責合併，是純標頭函式庫的正解。
//   (c) constexpr（C++17 起隱含 inline，最推薦）
//       static constexpr int MAX_SIZE = 100;
//       同時取得編譯期常數與 inline 兩個好處，不需要任何類別外定義。
//   結論：新程式碼一律寫 static constexpr，這個陷阱自然消失。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼 C++17 之前不能直接 inline？因為在 C++17 之前，
//       inline 只能用在函式上，變數沒有 inline 的概念。
//       C++17 的 inline variable 正是為了解決這類「純標頭函式庫」的痛點。
//   (B) 這個陷阱在 C++17 之前的標準函式庫裡也存在：
//       經典例子是把 std::numeric_limits<int>::max() 之類的常數
//       綁到 const& 參數上，在某些舊實作上會踩到同樣的問題。
//   (C) 錯誤發生在連結期而非編譯期，所以單獨用 -fsyntax-only 或
//       -c 只編譯不連結，都「看不到」這個錯誤。必須真的完成連結才會浮現。
//
// 【注意事項 Pay Attention】
//   1. 類別外定義只能出現在一個翻譯單元，寫進標頭檔會造成重複定義。
//   2. 類別外定義不可以重複寫初值（初值已在類別內給過）。
//   3. 這是連結期錯誤，-fsyntax-only 與 -c 都驗不出來——必須完整連結。
//   4. C++17 起用 static constexpr（隱含 inline）是最乾淨的解法。
//   5. std::min/std::max 的參數也是 const&，所以
//      std::max(Config::MAX_SIZE, x) 同樣會 odr-use，是常見的觸發點。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】static const 成員與 odr-use
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. static const int MAX_SIZE = 100; 寫在類別裡，為什麼有時還要類別外定義？
//     答：類別內的那一行只是「宣告 + 編譯期初值」，並沒有配置記憶體。
//         只需要值時（傳值、當陣列大小）編譯器直接把 100 貼進去，不需要定義；
//         一旦發生 odr-use——需要它的位址，例如綁定到 const int&——
//         就必須有真正的定義，否則連結期報 undefined reference。
//     追問：怎麼快速判斷有沒有 odr-use？
//         → 問「這個運算式需要它的位址嗎」。綁參考需要，傳值不需要。
//
// 🔥 Q2. 為什麼改用完美轉發之後，原本好好的程式碼突然連結失敗？
//     答：因為轉發參考 T&& 遇到左值會推導成 const int&，而參考需要位址，
//         於是從「不是 odr-use」變成「是 odr-use」。
//         原本傳值時編譯器只是把常數貼進去，改成轉發後就需要真正的記憶體了。
//
// ⚠️ 陷阱. 「編譯過了就代表沒問題」——為什麼這裡特別不成立？
//     答：這是連結期（linker）錯誤，不是編譯期錯誤。
//         每一個 .cpp 都能單獨編譯成功，g++ -c 也完全乾淨，
//         只有在最後把目的檔連結成執行檔時才爆出 undefined reference，
//         而且訊息不會指向任何一行原始碼。
//     為什麼會錯：大家把「編譯」當成單一階段，
//         但 C++ 的建置至少分成前處理、編譯、組譯、連結四步；
//         符號解析發生在最後一步，前面全部通過完全不代表符號存在。
//         這也是為什麼驗證版本相容性時要真的連結，不能只用 -fsyntax-only。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>   // std::forward

class Config {
public:
    // 類別內只是「宣告」並給初值，這還不是「定義」。
    static const int MAX_SIZE = 100;
};

// ⚠️ 只要有任何地方對 MAX_SIZE 取址（odr-use，例如綁定到 const int&），
//    就必須補上這個「類別外定義」，否則連結階段會出現：
//        undefined reference to `Config::MAX_SIZE'
//    注意初值已寫在類別內，這裡不可以再寫一次。
const int Config::MAX_SIZE;

void target(const int& n) {
    std::cout << "n = " << n << "\n";
}

template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}

// 對照：傳值不會 odr-use，即使沒有類別外定義也能連結成功
void takeByValue(int n) {
    std::cout << "  傳值 n = " << n << "（不需要位址 → 不是 odr-use）\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】現代解法：以 constexpr 定義服務設定上限
//   情境：專案裡的設定上限（連線數、緩衝區大小、重試次數）通常寫成
//     類別的靜態常數，並散布在各處被使用——有些傳值、有些綁 const&、
//     有些被丟進 std::min/std::max（參數就是 const&）。
//     只要有一處 odr-use 而忘了補定義，就會在連結期爆掉。
//   C++17 起用 static constexpr（隱含 inline）可以一次消滅這個問題，
//   而且可以安全地放在標頭檔裡被多個 .cpp 引用。
// -----------------------------------------------------------------------------
struct ServiceLimits {
    // ★ constexpr 自 C++17 起隱含 inline → 不需要任何類別外定義
    static constexpr int kMaxConnections = 512;
    static constexpr int kRetryBudget    = 3;
};

// 這些函式全部都會 odr-use（參數是參考），但 constexpr 版本完全不需補定義
void reportLimit(const char* name, const int& value) {
    std::cout << "    " << name << " = " << value << "\n";
}

template<typename T>
void forwardLimit(const char* name, T&& value) {
    // 轉發參考推成 const int& → odr-use
    reportLimit(name, std::forward<T>(value));
}

int main() {
    std::cout << "=== C++11 寫法：需要類別外定義 ===\n";

    // 綁定到 const int& 就需要取址，這已經是 odr-use，
    // 所以這一行同樣依賴上面那個類別外定義（不是「不需要定義」）。
    target(Config::MAX_SIZE);

    // 完美轉發把 T&& 推導成 const int&，一樣需要取址。
    // 若少了上面的類別外定義，這一行就會出現連結錯誤：
    //   undefined reference to `Config::MAX_SIZE'（本機實測）
    wrapper(Config::MAX_SIZE);

    std::cout << "\n=== 對照：傳值不是 odr-use ===\n";
    // 這一行即使刪掉類別外定義也能正常連結（本機已實測驗證）
    takeByValue(Config::MAX_SIZE);

    std::cout << "\n=== 日常實務：C++17 constexpr 解法（推薦）===\n";
    std::cout << "  constexpr 自 C++17 起隱含 inline，不需要類別外定義:\n";
    forwardLimit("kMaxConnections", ServiceLimits::kMaxConnections);
    forwardLimit("kRetryBudget",    ServiceLimits::kRetryBudget);
    std::cout << "  以上兩行都 odr-use 了，但完全不必補任何定義\n";

    // 其他解法：
    // C++17 的 inline variable：
    //   static inline const int MAX_SIZE = 100;
    // 或用 constexpr（C++17 起隱含 inline，最推薦）：
    //   static constexpr int MAX_SIZE = 100;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術9.cpp" -o odr_use

// 註：本檔未附 LeetCode 範例。odr-use 與連結期符號解析屬於建置系統層面的議題，
//     LeetCode 是單檔評測、不會顯現這個問題；硬套一題只會失真。
//
// 註：本檔示範的連結錯誤，只有在把 const int Config::MAX_SIZE; 這行
//     類別外定義刪掉之後才會出現。
//     現行程式碼已有該定義，故可正常編譯、連結並執行，下方輸出即為實際結果。

// === 預期輸出 ===
// === C++11 寫法：需要類別外定義 ===
// n = 100
// n = 100
//
// === 對照：傳值不是 odr-use ===
//   傳值 n = 100（不需要位址 → 不是 odr-use）
//
// === 日常實務：C++17 constexpr 解法（推薦）===
//   constexpr 自 C++17 起隱含 inline，不需要類別外定義:
//     kMaxConnections = 512
//     kRetryBudget = 3
//   以上兩行都 odr-use 了，但完全不必補任何定義
