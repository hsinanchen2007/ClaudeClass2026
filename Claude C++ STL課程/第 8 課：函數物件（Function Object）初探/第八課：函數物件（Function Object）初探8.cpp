// =============================================================================
//  第八課：函數物件 8  —  邏輯類函數物件，以及「短路求值」的消失
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<functional>
//   三個邏輯函數物件（全部是 C++98）：
//       std::logical_and<T>   a && b     二元
//       std::logical_or<T>    a || b     二元
//       std::logical_not<T>   !a         一元
//   透明版本 std::logical_and<> 等：**C++14**。
//   C++14 起 operator() 是 constexpr。
//   ★ 最重要的一點：這三個是**普通的函式呼叫**，
//     因此 **完全沒有短路求值（short-circuit evaluation）**。
//
// 【詳細解釋 Explanation】
//
// 【1. 核心差異：運算子有短路，函數呼叫沒有】
//   寫成運算子時：
//       if (p != nullptr && p->value > 0)      // p 為 null 時，右邊根本不會被求值
//   這是語言層級的規則：內建的 && 與 || 是**唯二**保證
//   「左運算元先求值，且結果足以決定答案時右運算元不求值」的運算子。
//
//   但 std::logical_and 是一個**函式**：
//       std::logical_and<bool>{}(p != nullptr, p->value > 0)
//   函式呼叫的規則是「所有引數都要先求值完，才進入函式本體」。
//   所以 p->value 一定會被求值——p 是 null 的話就直接是未定義行為。
//   短路是運算子的性質，不是 && 這個「概念」的性質；
//   一旦包裝成函數就沒了。
//
// 【2. 這也是為什麼「重載 && 和 ||」被視為極糟的做法】
//   C++ 允許你為自訂型別重載 operator&& 與 operator||，
//   但重載之後它們就變成普通的函式呼叫，**短路特性一併消失**。
//   使用者看到 a && b 的寫法，卻得到函式呼叫的求值語意，
//   這是最惡劣的一種「語法騙人」。
//   （C++17 起連求值順序都有規定了，但短路依然不會回來。）
//   實務準則：除非在寫表達式模板（expression template）這類函式庫，
//   否則永遠不要重載 && 與 ||。
//
// 【3. 那這三個 functor 到底拿來做什麼】
//   老實說，日常程式碼幾乎用不到它們。它們的存在理由是「完備性」——
//   標準把每個運算子都包裝成 functor，好讓演算法能收。
//   少數真正合理的用途：
//       * 對兩個 bool 序列做逐元素運算：
//         std::transform(a.begin(), a.end(), b.begin(), out.begin(),
//                        std::logical_and<>());
//       * std::logical_not 當述詞否定用（但 C++17 起有更好的 std::not_fn）
//   在這些場合，兩邊本來就都已經是算好的值，沒有短路的必要。
//
// 【4. 要「組合述詞」該用什麼】
//   常見需求是「同時滿足 A 和 B」的述詞。**不要**用 logical_and，
//   正解是直接寫 lambda：
//       auto both = [&](const T& x) { return isA(x) && isB(x); };   // 有短路
//   這樣寫的 && 是運算子，短路仍然有效。
//   否定述詞則有標準工具：
//       std::not_fn(pred)         // C++17，取代已移除的 std::not1 / not2
//   （std::not1 / std::not2 在 C++17 被棄用、C++20 正式移除。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 短路求值不只是效能優化，它是**正確性**的一部分
//     if (i < v.size() && v[i] > 0)          // 沒有短路就會越界
//     if (p && p->next)                       // 沒有短路就會解參考 null
//     這些寫法完全依賴短路來保證安全。
//     把它們改寫成 std::logical_and 的形式會直接變成未定義行為。
//
// (B) 為什麼函式呼叫不能短路
//     因為引數求值發生在「進入函式之前」。函式收到的是**值**，
//     它無從得知那些值是怎麼算出來的，也就沒有機會「跳過」某個計算。
//     要有短路就必須延遲求值——把運算元包成 lambda 之類的可呼叫物件，
//     由函式決定要不要呼叫。這正是惰性求值（lazy evaluation）的核心。
//
// (C) 引數求值順序：C++17 之前連順序都不保證
//     f(g(), h()) 中 g() 與 h() 誰先執行，C++17 之前是**未指定**的
//     （不是 UB，但每個編譯器可以不同）。
//     C++17 收緊了部分規則，但一般函式引數之間仍然沒有順序保證，
//     只保證不交錯（indeterminately sequenced）。
//     所以 std::logical_and(f(), g()) 連「f 先跑」都不能假設。
//
// (D) C++17 的 std::not_fn 取代了 not1 / not2
//     舊的 std::not1 需要述詞提供 argument_type 這類 typedef，
//     lambda 沒有這些，所以對 lambda 根本不能用。
//     std::not_fn 沒有這個限制：
//         auto isOdd = std::not_fn([](int x){ return x % 2 == 0; });
//
// 【注意事項 Pay Attention】
//   1. **std::logical_and / logical_or 沒有短路求值**——
//      兩個引數一定都會被求值。
//   2. 因此絕不可用它們取代 `p && p->f()` 這類依賴短路保證安全的寫法。
//   3. 要組合述詞請直接寫 lambda（裡面的 && 仍是運算子，有短路）。
//   4. 不要重載 operator&& / operator||，重載後短路就消失了。
//   5. 否定述詞請用 C++17 的 std::not_fn；
//      std::not1 / std::not2 已於 C++17 棄用、C++20 移除。
//   6. 函式引數的求值順序在一般情況下不保證，別依賴它。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】邏輯函數物件與短路求值
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::logical_and<bool>{}(a, b) 和 a && b 有什麼差別？
//     答：最關鍵的差別是**短路求值**。a && b 是運算子，
//         a 為 false 時 b 完全不會被求值；
//         std::logical_and 是**函式**，函式呼叫的規則是
//         「所有引數先求值完才進入函式本體」，所以 b 一定會被求值。
//         這代表 std::logical_and(p != nullptr, p->value > 0)
//         在 p 為 null 時會直接解參考空指標——未定義行為。
//     追問：為什麼函式做不到短路？
//         → 因為引數在進入函式前就算完了，函式收到的只是**值**，
//           沒有機會跳過任何計算。要短路就必須延遲求值，
//           把運算元包成 lambda 交給函式自己決定要不要呼叫。
//
// 🔥 Q2. 為什麼「不要重載 operator&& 和 operator||」是公認的準則？
//     答：因為重載之後它們就變成普通的函式呼叫，**短路特性消失**。
//         使用者看到 a && b 這個熟悉的寫法，會理所當然假設有短路，
//         但實際上兩邊都會被求值。這是最惡劣的一種「語法騙人」——
//         程式碼看起來對，語意卻完全不同，而且沒有任何警告。
//     追問：那有沒有合理需要重載它們的情況？
//         → 表達式模板（expression template）這類函式庫，
//           它們本來就不在「立即求值」的模型裡，
//           重載 && 是為了建構運算樹而不是求值。
//           一般應用程式碼沒有理由這麼做。
//
// ⚠️ 陷阱. 「我要組合兩個述詞成『同時滿足』，
//         寫成 std::logical_and<bool>{}(isValid(x), isActive(x)) 很優雅吧？」
//     答：不優雅，而且可能有 bug。兩個問題：
//         (1) **沒有短路**：isActive(x) 一定會被呼叫，
//             即使 isValid(x) 已經是 false。若 isActive 假設
//             「只在資料有效時才會被呼叫」（例如它會解參考某個指標），
//             這裡就是未定義行為。
//         (2) 兩次函式呼叫的**順序不保證**——一般函式引數之間
//             沒有求值順序保證，不能假設 isValid 先跑。
//         正解就是直接寫 lambda：
//             auto both = [](const T& x) { return isValid(x) && isActive(x); };
//         裡面的 && 是運算子，短路照常有效，也更好讀。
//     為什麼會錯：把 std::logical_and 當成「&& 的函式版本」，
//         以為兩者只是寫法不同、語意相同。
//         實際上「運算子 → 函數」這個包裝過程**會遺失短路語意**，
//         而短路在 C++ 裡常常不只是優化，是保證不越界、不解參考 null 的
//         正確性機制。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：std::logical_and / logical_or / logical_not 在真實程式碼裡幾乎沒人用，
//   它們存在的意義是「把每個運算子都補齊成 functor」的完備性，
//   而不是解決什麼實際問題。LeetCode 的解法一律直接寫 && 和 ||
//   （而且必須這樣寫，因為很多題目依賴短路來避免越界，
//     例如 `i < n && nums[i] == target`）。
//   硬掛一題只會示範一個沒人該學的寫法。
//   本檔真正的價值是「短路求值會在包裝成函數時消失」這個觀念，
//   下面用實務範例呈現它造成的真實 bug。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】表單驗證：組合多個檢查規則
//   情境：使用者註冊表單要檢查多個條件——欄位存在、格式正確、長度足夠。
//         這些檢查有**相依性**：必須先確認欄位存在，才能檢查它的格式。
//   為什麼用到本主題：這正是「短路是正確性機制」的教科書案例。
//     用 lambda 裡的 && 可以安全地依序檢查；
//     若改用 std::logical_and 把兩個檢查的**結果**傳進去，
//     第二個檢查會在欄位不存在時照樣執行 —— 直接讀到不存在的欄位。
//     下面把兩種寫法並列，並用「呼叫次數」證明短路確實消失了。
// -----------------------------------------------------------------------------
struct FormField {
    bool        present;      // 使用者有沒有填這個欄位
    std::string value;
};

// 檢查次數計數器：用來證明短路有沒有生效（可重現，不是計時）
struct CheckCounter {
    int formatChecks = 0;
};

// 這個檢查「假設欄位一定存在」——欄位不存在時讀 value 是沒有意義的
bool looksLikeEmail(const FormField& f, CheckCounter& c) {
    ++c.formatChecks;
    return f.value.find('@') != std::string::npos
        && f.value.find('.') != std::string::npos;
}

// ✓ 正解：lambda 裡的 && 是運算子，短路有效
bool validateWithShortCircuit(const FormField& f, CheckCounter& c) {
    return f.present && looksLikeEmail(f, c);
}

// ✗ 錯誤示範：兩個引數都會被求值，短路消失
bool validateWithLogicalAnd(const FormField& f, CheckCounter& c) {
    return std::logical_and<bool>{}(f.present, looksLikeEmail(f, c));
}

int main() {
    std::cout << "=== 邏輯類函數物件 ===" << std::endl;

    // logical_and<T>：邏輯 AND
    std::logical_and<bool> land;
    std::cout << "logical_and: true && false = " << (land(true, false) ? "true" : "false") << std::endl;
    std::cout << "logical_and: true && true = " << (land(true, true) ? "true" : "false") << std::endl;

    // logical_or<T>：邏輯 OR
    std::logical_or<bool> lor;
    std::cout << "logical_or: true || false = " << (lor(true, false) ? "true" : "false") << std::endl;
    std::cout << "logical_or: false || false = " << (lor(false, false) ? "true" : "false") << std::endl;

    // logical_not<T>：邏輯 NOT
    std::logical_not<bool> lnot;
    std::cout << "logical_not: !true = " << (lnot(true) ? "true" : "false") << std::endl;
    std::cout << "logical_not: !false = " << (lnot(false) ? "true" : "false") << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 關鍵差異：短路求值在包裝成函數時消失了 ===" << std::endl;
    {
        // 用計數器證明「右邊有沒有被求值」——可重現，不是計時
        int calls = 0;
        auto sideEffect = [&calls]() { ++calls; return true; };

        calls = 0;
        bool r1 = (false && sideEffect());          // 運算子：短路，右邊不執行
        std::cout << "false && sideEffect()              → 右邊被呼叫 "
                  << calls << " 次（結果 " << std::boolalpha << r1 << "）" << std::endl;

        calls = 0;
        bool r2 = std::logical_and<bool>{}(false, sideEffect());  // 函式：兩邊都求值
        std::cout << "logical_and{}(false, sideEffect()) → 右邊被呼叫 "
                  << calls << " 次（結果 " << r2 << "）" << std::endl;

        calls = 0;
        bool r3 = (true || sideEffect());           // 運算子：短路
        std::cout << "true || sideEffect()               → 右邊被呼叫 "
                  << calls << " 次（結果 " << r3 << "）" << std::endl;

        calls = 0;
        bool r4 = std::logical_or<bool>{}(true, sideEffect());
        std::cout << "logical_or{}(true, sideEffect())   → 右邊被呼叫 "
                  << calls << " 次（結果 " << r4 << "）" << std::endl;

        std::cout << "→ 短路是**運算子**的性質，不是 && 這個概念的性質。" << std::endl;
        std::cout << "  函式呼叫的規則是「所有引數先求值完才進函式本體」。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 短路常常是「正確性」而非「效能」 ===" << std::endl;
    {
        std::vector<int> v = {10, 20, 30};
        std::size_t i = 5;                       // 故意越界

        // 這種寫法完全依賴短路來避免越界
        bool safe = (i < v.size() && v[i] > 0);
        std::cout << "i=5, v.size()=3" << std::endl;
        std::cout << "  (i < v.size() && v[i] > 0) = " << std::boolalpha << safe
                  << "  ← 短路讓 v[i] 根本沒被求值" << std::endl;
        std::cout << "  若改寫成 std::logical_and{}(i < v.size(), v[i] > 0)，"
                  << std::endl;
        std::cout << "  v[i] 會被求值 → 越界存取 → 未定義行為。" << std::endl;
        std::cout << "  （所以這裡刻意不執行那個版本。）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 組合述詞的正解：直接寫 lambda ===" << std::endl;
    {
        std::vector<int> nums = {1, 4, 6, 9, 12, 15, 18, 21, 24};

        // ✓ 正解：lambda 內的 && 是運算子，短路有效
        auto evenAndBig = [](int x) { return x % 2 == 0 && x > 10; };
        auto n1 = std::count_if(nums.begin(), nums.end(), evenAndBig);
        std::cout << "同時是偶數且大於 10 的個數: " << n1 << std::endl;

        // C++17 的 std::not_fn 取代已移除的 std::not1
        auto notEven = std::not_fn([](int x) { return x % 2 == 0; });
        auto n2 = std::count_if(nums.begin(), nums.end(), notEven);
        std::cout << "奇數的個數（用 std::not_fn，C++17）: " << n2 << std::endl;
        std::cout << "→ std::not1 / std::not2 已於 C++17 棄用、C++20 移除，"
                  << std::endl;
        std::cout << "  而且它們對 lambda 根本不能用（需要 argument_type typedef）。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 這三個 functor 真正合理的用途：逐元素運算 ===" << std::endl;
    {
        // 兩邊本來就都是算好的值，沒有短路的必要
        std::vector<bool> hasAccount = {true,  true,  false, true,  false};
        std::vector<bool> isVerified = {true,  false, true,  true,  false};
        std::vector<bool> canLogin(hasAccount.size());

        std::transform(hasAccount.begin(), hasAccount.end(), isVerified.begin(),
                       canLogin.begin(), std::logical_and<>());

        std::cout << "有帳號: ";
        for (bool b : hasAccount) std::cout << b << " ";
        std::cout << std::endl;
        std::cout << "已驗證: ";
        for (bool b : isVerified) std::cout << b << " ";
        std::cout << std::endl;
        std::cout << "可登入: ";
        for (bool b : canLogin) std::cout << b << " ";
        std::cout << "  ← 兩個序列逐元素 AND" << std::endl;
        std::cout << "→ 這種場合兩邊本來就都是算好的值，短路無從發生，"
                  << std::endl;
        std::cout << "  所以用 logical_and 完全合理。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：表單驗證的相依檢查 ===" << std::endl;
    {
        const std::vector<FormField> fields = {
            {true,  "alice@example.com"},   // 有填，格式對
            {true,  "not-an-email"},        // 有填，格式錯
            {false, ""},                    // 沒填 —— 不該去檢查格式
            {true,  "bob@example.com"},     // 有填，格式對
        };

        CheckCounter c1, c2;
        std::cout << "四筆資料，其中第 3 筆使用者根本沒填：" << std::endl;

        std::cout << "  ✓ 短路版 (f.present && looksLikeEmail(f)):  ";
        for (const auto& f : fields)
            std::cout << (validateWithShortCircuit(f, c1) ? "O" : "X") << " ";
        std::cout << " → 格式檢查被呼叫 " << c1.formatChecks << " 次" << std::endl;

        std::cout << "  ✗ logical_and 版:                          ";
        for (const auto& f : fields)
            std::cout << (validateWithLogicalAnd(f, c2) ? "O" : "X") << " ";
        std::cout << " → 格式檢查被呼叫 " << c2.formatChecks << " 次" << std::endl;

        std::cout << "  結果相同，但呼叫次數差 "
                  << (c2.formatChecks - c1.formatChecks) << " 次。" << std::endl;
        std::cout << "→ 本例的 looksLikeEmail 讀空字串只是浪費；" << std::endl;
        std::cout << "  但若它改成解參考指標、或存取不存在的 map key，" << std::endl;
        std::cout << "  這多出來的那次呼叫就是一個真正的 bug。" << std::endl;
        std::cout << "  組合述詞請一律寫 lambda，讓 && 保持是運算子。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探8.cpp -o demo8
//
// ⚠️ 本檔用到 std::not_fn（C++17）與透明 functor std::logical_and<>（C++14），
//    所以最低需要 -std=c++17。

// === 預期輸出 ===
// === 邏輯類函數物件 ===
// logical_and: true && false = false
// logical_and: true && true = true
// logical_or: true || false = true
// logical_or: false || false = false
// logical_not: !true = false
// logical_not: !false = true
//
// === 關鍵差異：短路求值在包裝成函數時消失了 ===
// false && sideEffect()              → 右邊被呼叫 0 次（結果 false）
// logical_and{}(false, sideEffect()) → 右邊被呼叫 1 次（結果 false）
// true || sideEffect()               → 右邊被呼叫 0 次（結果 true）
// logical_or{}(true, sideEffect())   → 右邊被呼叫 1 次（結果 true）
// → 短路是**運算子**的性質，不是 && 這個概念的性質。
//   函式呼叫的規則是「所有引數先求值完才進函式本體」。
//
// === 短路常常是「正確性」而非「效能」 ===
// i=5, v.size()=3
//   (i < v.size() && v[i] > 0) = false  ← 短路讓 v[i] 根本沒被求值
//   若改寫成 std::logical_and{}(i < v.size(), v[i] > 0)，
//   v[i] 會被求值 → 越界存取 → 未定義行為。
//   （所以這裡刻意不執行那個版本。）
//
// === 組合述詞的正解：直接寫 lambda ===
// 同時是偶數且大於 10 的個數: 3
// 奇數的個數（用 std::not_fn，C++17）: 4
// → std::not1 / std::not2 已於 C++17 棄用、C++20 移除，
//   而且它們對 lambda 根本不能用（需要 argument_type typedef）。
//
// === 這三個 functor 真正合理的用途：逐元素運算 ===
// 有帳號: true true false true false
// 已驗證: true false true true false
// 可登入: true false false true false   ← 兩個序列逐元素 AND
// → 這種場合兩邊本來就都是算好的值，短路無從發生，
//   所以用 logical_and 完全合理。
//
// === 日常實務：表單驗證的相依檢查 ===
// 四筆資料，其中第 3 筆使用者根本沒填：
//   ✓ 短路版 (f.present && looksLikeEmail(f)):  O X X O  → 格式檢查被呼叫 3 次
//   ✗ logical_and 版:                          O X X O  → 格式檢查被呼叫 4 次
//   結果相同，但呼叫次數差 1 次。
// → 本例的 looksLikeEmail 讀空字串只是浪費；
//   但若它改成解參考指標、或存取不存在的 map key，
//   這多出來的那次呼叫就是一個真正的 bug。
//   組合述詞請一律寫 lambda，讓 && 保持是運算子。
