// =============================================================================
//  第 19 課：vector 與原始陣列的互操作 2  —  &v[0] vs data()：空 vector 的分水嶺
// =============================================================================
//
// 【主題資訊 Information】
//   reference       operator[](size_type pos);         // 前置條件：pos < size()
//   const_reference operator[](size_type pos) const;   // 前置條件：pos < size()
//   T*              data() noexcept;                   // 無前置條件         (C++11)
//
//   標頭檔  : <vector>
//   關鍵差異: operator[] 有「pos < size()」的前置條件，違反即為未定義行為；
//             data() 沒有任何前置條件，對空 vector 呼叫也完全合法。
//   複雜度  : 兩者皆 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 &v[0] 在空 vector 上是 UB】
//   很多人的直覺是：「我只是取位址，又沒有真的讀值，應該沒事吧？」
//   問題出在求值順序——&v[0] 其實是兩個動作：
//       第一步：呼叫 v.operator[](0)，回傳 reference（型別是 int&）
//       第二步：對那個 reference 取位址
//   UB 發生在「第一步」。標準規定 operator[] 的前置條件是 pos < size()，
//   空 vector 的 size() 是 0，所以 v[0] 本身就已經違反契約了。
//   標準一旦說「未定義行為」，整段程式的行為就不再有任何保證——
//   不是「回傳一個垃圾值」，而是編譯器有權假設這件事不會發生並據此最佳化。
//
//   實務上在 libstdc++ 的 release 模式，&v[0] 對空 vector 常常「看起來能跑」
//   （因為 operator[] 只是做指標加法、沒有解參考）。但那是巧合，不是保證：
//   換一個標準函式庫、開啟 _GLIBCXX_ASSERTIONS、或編譯器換一種最佳化策略，
//   結果就可能完全不同。可能立刻中止、可能悄悄產生錯誤的位址、也可能沒事。
//
// 【2. 為什麼 data() 沒有這個問題】
//   data() 的規格是「回傳一個指標 p，使得 [p, p + size()) 是合法範圍」。
//   當 size() == 0 時，這個要求退化成「[p, p) 是合法的空範圍」——
//   任何值（包括 nullptr）都能滿足，所以標準不需要加任何前置條件。
//   代價是：這個指標「不保證可以解參考」。你可以拿它做比較、做指標算術
//   （加 0），但不能 *p。
//
// 【3. 空 vector 的 data() 到底回傳什麼】
//   標準「不規定」。實作可以回傳 nullptr，也可以回傳某個非空的哨兵位址。
//   本機 libstdc++ 15.2 實測：預設建構的空 vector，data() 回傳 nullptr。
//   但同樣是空的（size()==0）vector，如果先 reserve(4) 過，data() 就會回傳
//   一個已配置好的非空指標——同樣是 size()==0，指標卻不同。
//   這正說明了：不要對 data() 的值做任何假設，只能依賴 size() 判斷有沒有資料。
//
// 【4. 那 C API 收到 nullptr 會怎樣？】
//   這是真正該擔心的事。很多 C 函式沒有處理 nullptr，直接就解參考。
//   所以正確的互操作寫法是「呼叫端先擋」：
//       if (!v.empty()) c_api(v.data(), v.size());
//   或者在被呼叫端第一行就檢查 size == 0 直接返回（見本課第 3 個檔案）。
//   注意 memcpy 一類的函式即使 n == 0，傳 nullptr 給它在標準上仍是 UB，
//   雖然實務上幾乎所有實作都會直接返回。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「取位址不算存取」是錯的心智模型
//   C++ 的 UB 判定看的是「契約」不是「有沒有真的碰記憶體」。
//   operator[] 的契約寫明 pos < size()，違反了就結束，
//   後面你打算拿那個 reference 做什麼都無關緊要。
//   同理：*(p + n) 即使不讀取，只要 p + n 超出合法範圍加一，指標算術本身就是 UB。
//
// (B) at() 為什麼不能解決這件事
//   有人會想「那我用 &v.at(0) 總安全了吧」。at() 確實會做邊界檢查，
//   但檢查失敗時是丟 std::out_of_range 例外——你得到的是例外，不是可用的指標。
//   要「安全地取得起始指標」，答案永遠只有 data()。
//
// (C) 兩個空 vector 的 data() 可能相等也可能不等
//   標準沒有規定，所以「用 data() 是否為 nullptr 判斷 vector 是否為空」
//   是不可攜的寫法。判斷空請一律用 v.empty()（語意最清楚，且 O(1)）。
//
// 【注意事項 Pay Attention】
//   1. &v[0] 在空 vector 上是未定義行為，不是「回傳 nullptr」。
//   2. data() 在空 vector 上合法，但回傳值不保證可解參考，也不保證是 nullptr。
//   3. 判斷「有沒有元素」永遠用 empty() / size()，不要用 data() != nullptr。
//   4. 傳給 C API 前要考慮它能不能接受 nullptr + 0；不確定就自己先擋。
//   5. reserve() 過的空 vector，size() 仍是 0，但 data() 會是非空指標——
//      size 與指標是兩回事，見本課第 6 個檔案。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】&v[0] 與空 vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對一個空的 vector 寫 int* p = &v[0];，會發生什麼事？
//     答：這是未定義行為。&v[0] 會先呼叫 operator[](0)，而它的前置條件是
//         pos < size()，空 vector 的 size() 是 0，契約當場就被違反了。
//         標準不保證任何特定結果——可能看起來正常執行、可能中止、
//         也可能被最佳化成完全預期外的程式碼。正確寫法是 v.data()。
//     追問：那我改成 &v.at(0) 呢？
//         → 不是 UB 了，但會丟 std::out_of_range 例外，你依然拿不到指標。
//
// 🔥 Q2. 空 vector 的 data() 一定回傳 nullptr 嗎？
//     答：不一定，標準未規定。本機 libstdc++ 15.2 上，預設建構的空 vector
//         回傳 nullptr；但若先 reserve(4)，size() 仍是 0、data() 卻回傳
//         已配置的非空指標。所以「data() == nullptr」不能用來判斷是否為空，
//         判斷空請用 empty()。
//     追問：那 data() 的回傳值到底保證什麼？
//         → 只保證 [data(), data() + size()) 是合法範圍。size() 為 0 時
//           這是空範圍，因此該指標不保證可以解參考。
//
// ⚠️ 陷阱. 「我只是取位址又沒讀值，&v[0] 對空 vector 應該不會怎樣吧？」
//     答：錯。UB 在 v[0] 這一步就發生了，跟你後面有沒有解參考無關。
//         C++ 判定 UB 看的是「有沒有違反函式的前置條件」，
//         不是「有沒有實際碰到那塊記憶體」。
//     為什麼會錯：把 C++ 的 UB 想成「執行期的記憶體錯誤」，
//         但它其實是「編譯期的契約違反」。編譯器有權假設 UB 不會發生，
//         並依此刪除你認為一定會執行的檢查——這就是為什麼 UB 的後果
//         常常出現在跟原始碼看起來毫無關係的地方。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iostream>
#include <vector>

void c_library_function(const int* arr, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        std::cout << (i ? " " : "") << arr[i];
    }
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把資料庫查詢結果送進舊的 C 報表引擎
//   情境：查詢「今日訂單金額」，結果可能是 0 筆（今天還沒開單）。
//         報表引擎 report_render() 是 C 寫的，收 const int* + 筆數，
//         而且它「沒有」處理 nullptr——傳 nullptr 進去會直接解參考。
//   為何是本主題：這正是空 vector 互操作的真實痛點。
//         用 &v[0] 拿指標在 0 筆時是 UB；用 data() 合法但可能拿到 nullptr。
//         正解是「用 data() 取指標，並在呼叫端用 empty() 擋掉 0 筆」。
// -----------------------------------------------------------------------------
void report_render(const int* amounts, std::size_t n) {  // 假裝這是 C 報表引擎
    long long total = 0;
    for (std::size_t i = 0; i < n; ++i) total += amounts[i];
    std::cout << "  報表：" << n << " 筆，合計 " << total << " 元\n";
}

void renderDailyReport(const std::vector<int>& orderAmounts) {
    if (orderAmounts.empty()) {
        // 呼叫端先擋：不把 nullptr 交給不會檢查的 C 函式
        std::cout << "  報表：今日無訂單，跳過呼叫 C 引擎\n";
        return;
    }
    report_render(orderAmounts.data(), orderAmounts.size());
}

int main() {
    // -------------------------------------------------------------------------
    std::cout << "=== 非空 vector：&v[0] 與 data() 完全等價 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> v = {10, 20, 30, 40, 50};

    int* oldStyle = &v[0];      // C++03 風格（此處 v 非空，合法）
    int* newStyle = v.data();   // C++11 起的標準寫法

    std::cout << "兩者是否指向同一位址: " << (oldStyle == newStyle ? "是" : "否") << "\n";
    c_library_function(newStyle, v.size());

    // -------------------------------------------------------------------------
    std::cout << "\n=== 空 vector：兩者天差地別 ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> emptyVec;

    // int* bad = &emptyVec[0];
    //   ↑ 未定義行為！operator[](0) 的前置條件 pos < size() 已被違反。
    //     這一行沒有被註解掉的話，標準不保證任何結果——
    //     可能看似正常、可能中止、也可能被最佳化成非預期的行為。

    int* safe = emptyVec.data();   // 合法呼叫，沒有前置條件
    std::cout << "empty.data() 是否為 nullptr : "
              << (safe == nullptr ? "是" : "否")
              << "（libstdc++ 15.2 本機實測；標準未規定，不可依賴）\n";
    std::cout << "empty.size()                : " << emptyVec.size() << "\n";
    std::cout << "empty.empty()               : "
              << (emptyVec.empty() ? "true" : "false")
              << "（判斷是否為空請一律用這個）\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 同樣 size()==0，data() 卻可能不是 nullptr ===\n";
    // -------------------------------------------------------------------------
    std::vector<int> reserved;
    reserved.reserve(4);           // 配置了記憶體，但一個元素都還沒建構
    std::cout << "reserve(4) 後 size()      : " << reserved.size() << "\n";
    std::cout << "reserve(4) 後 capacity()  : " << reserved.capacity()
              << "（實作定義，libstdc++ 這裡剛好等於要求值）\n";
    std::cout << "reserve(4) 後 data()==null: "
              << (reserved.data() == nullptr ? "是" : "否") << "\n";
    std::cout << "→ 結論：size()==0 但 data() 非空，所以不能用 data() 判斷空不空\n";

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：訂單報表（0 筆 / 有資料兩種情境）===\n";
    // -------------------------------------------------------------------------
    std::vector<int> noOrders;                        // 今天還沒開單
    std::vector<int> todayOrders = {1200, 350, 4800}; // 有 3 筆

    std::cout << "情境 A（0 筆）:\n";
    renderDailyReport(noOrders);
    std::cout << "情境 B（3 筆）:\n";
    renderDailyReport(todayOrders);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第\ 19\ 課：vector\ 與原始陣列的互操作2.cpp -o interop2

// === 預期輸出 ===
// === 非空 vector：&v[0] 與 data() 完全等價 ===
// 兩者是否指向同一位址: 是
// 10 20 30 40 50
//
// === 空 vector：兩者天差地別 ===
// empty.data() 是否為 nullptr : 是（libstdc++ 15.2 本機實測；標準未規定，不可依賴）
// empty.size()                : 0
// empty.empty()               : true（判斷是否為空請一律用這個）
//
// === 同樣 size()==0，data() 卻可能不是 nullptr ===
// reserve(4) 後 size()      : 0
// reserve(4) 後 capacity()  : 4（實作定義，libstdc++ 這裡剛好等於要求值）
// reserve(4) 後 data()==null: 否
// → 結論：size()==0 但 data() 非空，所以不能用 data() 判斷空不空
//
// === 日常實務：訂單報表（0 筆 / 有資料兩種情境）===
// 情境 A（0 筆）:
//   報表：今日無訂單，跳過呼叫 C 引擎
// 情境 B（3 筆）:
//   報表：3 筆，合計 6350 元
