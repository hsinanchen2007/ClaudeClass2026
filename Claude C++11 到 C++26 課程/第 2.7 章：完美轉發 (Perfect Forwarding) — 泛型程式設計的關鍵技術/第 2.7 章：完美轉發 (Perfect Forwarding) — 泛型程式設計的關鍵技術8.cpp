// =============================================================================
//  完美轉發陷阱 (3)：`0` 與 `NULL` 轉發後不再是「空指標」
// =============================================================================
//
// 【主題資訊 Information】
//   template<typename T> void wrapper(T&& arg);        // forwarding reference
//   標準版本：forwarding reference（C++11）、nullptr / std::nullptr_t（C++11）
//   標頭檔  ：<utility>（std::forward）、<cstddef>（NULL、std::nullptr_t）
//   關鍵規則：0 與 NULL 是「整數」，只有在「直接初始化指標」這個上下文中
//             才會被當成 null pointer constant。一旦被模板推導成整數型別，
//             這個特殊身分就永久消失了。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 target(0) 可以，wrapper(0) 卻不行？】
//   `0` 的型別自始至終都是 int，它從來不是指標。
//   C++ 保留了一條相容 C 的規則：整數常數 0（null pointer constant）
//   在「需要指標」的上下文中可以隱式轉換成空指標。關鍵是這條規則的觸發條件是
//   「目標型別已知是指標」：
//       target(0);      // 參數型別已知是 int* → 觸發轉換 → OK
//       wrapper(0);     // T 由引數推導 → T = int → 之後想轉回 int* 已太遲
//   在 wrapper 內部，arg 的型別已經被「定案」成 int&&。
//   呼叫 target(std::forward<T>(arg)) 時傳的是一個 int 型別的表達式，
//   而「int 表達式轉 int*」只對常數 0 成立，對執行期的 int 值不成立
//   → 編譯錯誤。轉發過程把「這是字面常數 0」這個編譯期資訊弄丟了。
//
// 【2. NULL 為什麼「可能」錯而不是「一定」錯？】
//   NULL 是巨集，標準只要求它展開成一個 null pointer constant，
//   實作可以選 0、0L、((void*)0)（僅限 C）或編譯器內建值。
//   → 推導出來的 T 因實作而異，所以錯誤訊息也因平台而異。
//   本機實測（GCC 15.2.0 / libstdc++）：decltype(NULL) 是 long、
//   sizeof(NULL) 是 8。因此 wrapper(NULL) 會推導出 T = long，
//   接著 long → int* 轉換失敗。
//   「因實作而異」正是 NULL 最糟的地方：同一份程式碼換個編譯器，
//   錯誤訊息、甚至是否觸發多載歧義都可能改變。
//
// 【3. nullptr 為什麼就沒事？】
//   nullptr 不是整數，它是型別為 std::nullptr_t 的「真正的值」。
//   推導後 T = std::nullptr_t，這個型別本身可隱式轉換成任何指標型別，
//   而且轉發前後型別完全一致 → 完美轉發成立。
//   這正是 C++11 引入 nullptr 的主因之一：讓「空指標」擁有自己的型別，
//   而不是寄生在整數 0 身上，泛型程式碼才能安全地轉發它。
//
// 【概念補充 Concept Deep Dive】
//   ▸ reference collapsing 是完美轉發的引擎：
//       template<typename T> void wrapper(T&& arg) 中，
//       - 傳 lvalue（X 型別）→ T = X&  → T&& = X& && → 摺疊為 X&
//       - 傳 rvalue（X 型別）→ T = X   → T&& = X&&
//     「只要有一個 &，結果就是 &」。T 帶不帶 & 就是「原引數是不是 lvalue」
//     的唯一載體，std::forward<T> 靠讀 T 才能把值類別還原。
//   ▸ std::forward<T> 為什麼一定要顯式寫 <T>？
//       arg 在函式內是具名變數 → 它本身永遠是 lvalue，看 arg 拿不到原值類別。
//       資訊只存在 T 裡，所以必須寫 std::forward<T>(arg)。
//       forward 的參數型別刻意寫成 std::remove_reference_t<T>&，
//       讓 T 落在 non-deduced context，寫 std::forward(arg) 直接編譯不過。
//   ▸ 值得注意的是：本檔的失敗點與「值類別」無關。
//     0 / NULL 的問題出在「型別」被定案得太早，
//     連 reference collapsing 都還沒派上用場就已經錯了。
//
// 【注意事項 Pay Attention】
//   1. 泛型程式碼一律用 nullptr，不要用 0 或 NULL —— 這不只是風格問題，
//      是「能不能通過編譯」的問題。
//   2. NULL 的定義是實作定義（implementation-defined），
//      推導結果不可攜；本機為 long（本機實測）。
//   3. std::nullptr_t 可轉成任何指標型別，但不能轉成整數型別。
//   4. 多載決議上：f(int) 與 f(char*) 並存時，f(NULL) 可能歧義或選錯，
//      f(nullptr) 則明確選到指標版本。
//   5. 印出指標值時，空指標在 libstdc++ 下印 0；非空指標的位址
//      每次執行都不同（ASLR），不可寫死進預期輸出。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】0 / NULL / nullptr 與完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `target(0)` 能編譯，為什麼 `wrapper(0)` 不能？
//     答：0 的型別是 int，只有在「目標型別已知是指標」時才會被當成
//         null pointer constant 轉換。target 的參數型別已知 → 轉換成立；
//         wrapper 要先推導 T，T 被定案成 int 之後，內部再也轉不回 int*。
//     追問：那 wrapper 內部改寫成 target(nullptr) 不就好了？
//         → 那就不叫轉發了，等於寫死行為、丟掉呼叫端傳進來的引數。
//
// 🔥 Q2. C++11 為什麼要引入 nullptr？只是為了好看嗎？
//     答：不是。核心理由是「讓空指標有自己的型別 std::nullptr_t」。
//         有了獨立型別，模板推導與多載決議才能正確識別它；
//         0 寄生在 int 身上，一進泛型程式碼就退化成整數。
//     追問：std::nullptr_t 可以轉成 int 嗎？→ 不行，只能轉成指標型別。
//
// ⚠️ 陷阱. NULL 已經是「空指標巨集」了，轉發它應該安全 —— 對嗎？
//     答：不對。NULL 只是展開成某個 null pointer constant 的巨集，
//         它展開後仍是「整數常數」，一樣會被推導成整數型別。
//         而且展開成什麼是實作定義：本機實測 decltype(NULL) 為 long，
//         換平台可能是 int，錯誤訊息與多載行為都會跟著變。
//     為什麼會錯：把 NULL 想成「指標型別的特殊值」，
//         實際上它從來沒有指標型別，只是一個長得像指標的整數。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstddef>
#include <utility>
#include <type_traits>

void target(int* ptr) {
    std::cout << "收到指標: " << ptr << "\n";
}

template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】包裝 C 風格函式庫的「選用輸出參數」
//   情境：C API 常見的簽名長這樣——最後一個參數是「選用的輸出指標，
//         不需要就傳 NULL」。例如：
//             int db_query(const char* sql, int* out_rows);   // out_rows 可為 NULL
//         團隊想寫一個 C++ 泛型轉發層統一加上 log／重試／計時。
//   問題：呼叫端沿用 C 的習慣寫 call_with_log("...", NULL) → 直接編譯不過，
//         而且錯誤訊息深埋在模板實例化裡，非常難讀。
//   對策：(1) 泛型層一律要求呼叫端傳 nullptr；
//         (2) 用 static_assert 在轉發層就給出人話錯誤訊息，
//             不要讓使用者去讀 C++ 模板實例化的天書。
// -----------------------------------------------------------------------------
int db_query(const char* sql, int* out_rows) {
    if (out_rows) *out_rows = 42;                 // 呼叫端想知道筆數才會傳非空指標
    std::cout << "  [db] 執行: " << sql
              << " (out_rows " << (out_rows ? "已回填" : "略過") << ")\n";
    return 0;
}

template<typename P>
int call_with_log(const char* sql, P&& out_rows) {
    // 若有人沿用 C 習慣傳了 0 或 NULL，在這裡就攔下來給出可讀的訊息。
    static_assert(!std::is_integral_v<std::remove_reference_t<P>>,
                  "選用輸出參數請傳 nullptr，不要傳 0 或 NULL："
                  "整數型別無法被轉發成指標。");
    std::cout << "  [log] 進入查詢\n";
    int rc = db_query(sql, std::forward<P>(out_rows));
    std::cout << "  [log] 離開查詢 rc=" << rc << "\n";
    return rc;
}

int main() {
    std::cout << "=== 直接呼叫：目標型別已知，0/NULL 都能轉成空指標 ===\n";
    target(0);            // OK：目標型別已知是 int* → 0 視為 null pointer constant
    target(NULL);         // OK：NULL 展開後仍是 null pointer constant
    target(nullptr);      // OK：std::nullptr_t → int*

    std::cout << "\n=== 經過完美轉發：只有 nullptr 活得下來 ===\n";
    // wrapper(0);        // ❌ 錯誤！T 推導為 int，int 表達式不能轉為 int*
    // wrapper(NULL);     // ❌ 本機推導為 long（本機實測），long 不能轉為 int*
    wrapper(nullptr);     // ✅ T 推導為 std::nullptr_t，型別全程保持一致

    std::cout << "\n=== NULL 在本機的真實身分（本機實測）===\n";
    std::cout << "sizeof(NULL) = " << sizeof(NULL)
              << "，decltype(NULL) 是 "
              << (std::is_same_v<decltype(NULL), long> ? "long" : "非 long")
              << "（GCC 15.2.0 / libstdc++；其他實作可能不同）\n";

    std::cout << "\n=== 日常實務：包裝 C API 的選用輸出參數 ===\n";
    int rows = 0;
    call_with_log("SELECT * FROM orders", &rows);
    std::cout << "  取得筆數 = " << rows << "\n";

    std::cout << "\n  不需要筆數時傳 nullptr：\n";
    call_with_log("DELETE FROM tmp", nullptr);

    // call_with_log("SELECT 1", NULL);
    // ❌ 會觸發 static_assert，訊息是人話而不是模板實例化天書。

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術8.cpp" -o fwd08

// === 預期輸出 ===
// (待實跑填入)
