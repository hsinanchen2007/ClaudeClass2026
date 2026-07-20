// =============================================================================
//  第 12 課 (5)  —  空容器的 front() / back()：最高頻的面試陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   本檔主題：對 empty() 為 true 的 vector 呼叫 front() / back() 會發生什麼
//   標準規定：[sequence.reqmts] front()/back() 的前置條件是 !empty()。
//             違反 → 未定義行為（Undefined Behavior），**不擲出任何例外**
//   對照：v.at(0) 對空容器會擲出 std::out_of_range（這是唯一有保證的）
//   標頭檔：<vector>
//   相關函式：empty()（O(1)、noexcept）、size()（O(1)、noexcept）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼這是陷阱？因為直覺會給出錯誤答案】
// 學過 at() 之後，大部分人腦中會形成一條規則：
//       「STL 容器的存取函式，出錯時會擲出例外」
// 這條規則是**錯的**。正確的規則是：
//       「at() 是 vector 介面中唯一保證擲出例外的存取函式，其餘全部是 UB」
//
//   函式          空容器時的行為
//   ────────────────────────────────────────────────
//   v.at(0)       擲出 std::out_of_range  ← 唯一有保證的
//   v[0]          未定義行為
//   v.front()     未定義行為
//   v.back()      未定義行為
//   v.data()      合法，回傳一個不可解參考的指標（見第 6 檔）
//   v.pop_back()  未定義行為（空容器不能 pop）
//
// 【2. 為什麼標準不讓 front() 也做檢查？】
// 因為 front()/back() 的定位與 operator[] 相同：零成本的直接存取。
// 它們被大量用在「已知非空」的情境中（例如剛 push_back 完、
// 或在 while (!q.empty()) 迴圈內），若每次都檢查就是重複付費。
// 標準的態度一貫是：把檢查交給呼叫端，需要的人自己加。
//
// 而且 front() 沒有像 at() 那樣的「安全版本」可選 ——
// 標準**沒有**提供 at_front() 或 safe_front()。唯一的防線就是 !v.empty()。
//
// 【3. 空容器 front() 的底層機制】
// 空 vector 的 begin() == end()，兩者指向同一個位置
// （可能是 nullptr，也可能是一塊已配置但沒有元素的空間）。
// front() 等價於 *begin()，於是變成解參考一個「尾後指標」：
//
//     空 vector：
//     ┌┐
//     ││  ← 沒有任何元素
//     └┘
//      ▲
//   begin() == end()    ← front() 解參考這裡 = UB
//
// 【4. 本機實測：同一段程式碼的兩種結局】
// 環境 g++ 15.2.0，對空 vector 呼叫 front() 並印出：
//
//   建置方式              結果
//   ────────────────────────────────────────────────────────────────
//   g++ -std=c++17 (-O0)  hardening assertion 攔下 → abort（結束碼 134）
//   g++ -std=c++17 -O2    無檢查 → SIGSEGV 段錯誤（結束碼 139），
//                         而且緩衝區中尚未輸出的內容整批遺失
//
// 注意兩者的**失敗方式完全不同**（一個是受控 abort、一個是段錯誤），
// 而且都不是「擲出例外」，所以 try/catch 一個都攔不到。
// 這也是為什麼不能寫「空容器 front() 會 crash」這種話 ——
// 在別的最佳化等級 / 標準函式庫下，它甚至可能不 crash，只是讀到垃圾值。
//
// 【概念補充 Concept Deep Dive】
// 為什麼 try/catch 攔不住 UB？
// 例外是 C++ 語言層級的機制：throw 會展開堆疊、尋找匹配的 catch。
// 但 UB 造成的 abort 與 SIGSEGV 都不是 throw ——
//   * abort() 是直接送出 SIGABRT 終止行程，不做堆疊展開
//   * SIGSEGV 是 CPU 的記憶體保護單元觸發、由作業系統送出的訊號
// 兩者都繞過了 C++ 的例外機制。所以：
//       try { v.front(); } catch (...) { /* 永遠不會進來 */ }
// 這段程式碼給人虛假的安全感，實際上一點保護作用都沒有。
// **防禦 UB 的唯一方式是「不要讓它發生」，不是「試圖捕捉它」。**
//
// 【注意事項 Pay Attention】
// 1. 存取 front()/back() 前一律先確認 !v.empty()。這是無可取代的。
// 2. 用 !v.empty() 而不是 v.size() > 0：語意更直接，
//    而且對 std::forward_list 這種沒有 O(1) size() 的容器也適用。
// 3. try/catch 不能防禦 UB。不要寫出讓人誤以為安全的 catch 區塊。
// 4. 迴圈中要特別小心：while (!v.empty()) { ...; v.pop_back(); } 是安全的，
//    但若迴圈內同時有其他地方 pop，就可能在下一次 front() 時已經空了。
// 5. 開發階段開 -fsanitize=address 能在越界/解參考尾後指標當下就停下，
//    比事後除錯有效率得多。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】空容器的元素存取
// ───────────────────────────────────────────────────────────────────────────
// ⚠️ 陷阱. 對一個空的 std::vector 呼叫 v.front()，會發生什麼事？
//     答：未定義行為，**不會擲出任何例外**。
//         front() 的前置條件是 !empty()，違反時標準不做任何規範。
//         本機實測：-O0 被 hardening assertion 攔下 abort（134）；
//         -O2 沒有檢查，直接 SIGSEGV（139）。兩者都無法被 catch 攔下。
//     為什麼會錯：學過 at() 會擲 out_of_range 之後，
//         直覺推廣成「STL 存取出錯都會擲例外」。
//         事實正好相反：at() 是唯一的例外（雙關），其餘全部是 UB。
//
// 🔥 Q1. 那要怎麼安全地取得第一個元素？
//     答：呼叫前檢查 —— if (!v.empty()) { use(v.front()); }。
//         若「空」是正常的業務情況（不是錯誤），可以包成回傳
//         std::optional<T> 的函式，強迫呼叫端顯式處理空的情形（見第 9 檔）。
//     追問：為什麼偏好 !v.empty() 而不是 v.size() > 0？
//         → 語意更直接；且 empty() 對所有容器都保證 O(1)，
//           而 std::forward_list 根本沒有 size()。
//
// 🔥 Q2. 我把 v.front() 包在 try/catch(...) 裡面，這樣安全嗎？
//     答：完全不安全，而且更糟 —— 它製造了虛假的安全感。
//         UB 導致的 abort 與 SIGSEGV 都不經過 C++ 的例外機制
//         （一個是 SIGABRT、一個是 CPU 的記憶體保護訊號），
//         catch 攔不到任何東西。防禦 UB 的唯一方式是不讓它發生。
//     追問：那 catch(...) 到底能攔什麼？→ 只能攔真正被 throw 出來的 C++ 例外，
//         例如 at() 的 std::out_of_range、new 的 std::bad_alloc。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】批次作業結果彙總：正確處理「這一批完全沒有資料」
//   情境：每日結算程式讀入當天的交易金額，要產出「首筆、末筆、總額」報表。
//         有些日子（假日、系統維護日）根本沒有交易 —— 空資料是**正常情況**，
//         不是程式錯誤，因此不能讓它變成 crash，也不該擲例外。
//   為什麼是本主題：報表要的「首筆 / 末筆」正是 front() / back()，
//         而「今天沒有交易」正是空容器。這是實務中最容易踩到
//         空容器 UB 的地方 —— 開發時測試資料都不是空的，
//         上線後遇到第一個假日就炸了。
// -----------------------------------------------------------------------------
void printDailyReport(const std::string& date, const std::vector<double>& amounts) {
    std::cout << "[" << date << "] ";

    // 關鍵防線：先判空。少了這一行，遇到空資料就是未定義行為。
    if (amounts.empty()) {
        std::cout << "本日無交易（0 筆）— 首筆/末筆不適用，總額 0\n";
        return;
    }

    double total = 0.0;
    for (double a : amounts) total += a;

    std::cout << "筆數=" << amounts.size()
              << " 首筆=" << amounts.front()     // 已確認非空，安全
              << " 末筆=" << amounts.back()      // 已確認非空，安全
              << " 總額=" << total << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】工作佇列處理：迴圈中的 empty() 檢查為什麼不能省
//   情境：從待處理佇列逐一取出工作。這是 front() 最常見的使用場景，
//         也是「迴圈條件本身就是防線」的典型寫法。
// -----------------------------------------------------------------------------
void drainQueue(std::vector<std::string> queue) {
    int step = 0;
    // 迴圈條件 !queue.empty() 保證了迴圈內的 front() 一定合法
    while (!queue.empty()) {
        const std::string job = queue.front();   // 複製出來，不留參考
        queue.erase(queue.begin());              // 移除後原參考即失效
        std::cout << "  step " << ++step << " 處理: " << job
                  << "（剩餘 " << queue.size() << "）\n";
    }
    std::cout << "  佇列已清空，迴圈自然結束（不會誤觸 front()）\n";
}

int main() {
    std::cout << "=== 空容器的狀態 ===\n";
    std::vector<int> empty_vec;
    std::cout << "empty() = " << std::boolalpha << empty_vec.empty() << "\n";
    std::cout << "size()  = " << empty_vec.size() << "\n";

    std::cout << "\n=== 危險：以下呼叫是未定義行為（已註解，不執行）===\n";
    // std::cout << empty_vec.front() << "\n";   // UB：不會擲例外
    // std::cout << empty_vec.back()  << "\n";   // UB：不會擲例外
    // empty_vec.pop_back();                     // UB：空容器不能 pop
    std::cout << "front() / back() / pop_back() 對空容器皆為 UB，且無法被 catch 攔下\n";

    std::cout << "\n=== 安全做法一：先檢查 empty() ===\n";
    if (!empty_vec.empty()) {
        std::cout << "front = " << empty_vec.front() << "\n";
    } else {
        std::cout << "容器為空 → 略過存取（這是唯一可靠的防線）\n";
    }

    std::cout << "\n=== 安全做法二：at() 會擲出可捕捉的例外 ===\n";
    try {
        std::cout << empty_vec.at(0) << "\n";
    } catch (const std::out_of_range& e) {
        std::cout << "at(0) 擲出 std::out_of_range，成功捕捉\n";
        std::cout << "  e.what() = " << e.what() << "\n";
    }

    std::cout << "\n=== 對照：非空容器一切正常 ===\n";
    std::vector<int> v = {7, 8, 9};
    if (!v.empty()) {
        std::cout << "front = " << v.front() << ", back = " << v.back() << "\n";
    }

    std::cout << "\n=== 日常實務：每日結算報表（含無交易日）===\n";
    printDailyReport("2024-05-17", {1200.5, 880.0, 2310.25});
    printDailyReport("2024-05-18", {});              // 假日：空資料是正常情況
    printDailyReport("2024-05-19", {450.0});

    std::cout << "\n=== 日常實務：工作佇列逐一處理 ===\n";
    drainQueue({"resize-image", "send-mail", "update-index"});

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：vector 元素存取：operator[]、at、front、back5.cpp" -o access5
//
// ── 輸出但書 ────────────────────────────────────────────────────────────
// e.what() 的訊息文字屬實作定義（本機 GCC 15.2.0 / libstdc++）。標準只保證
// at() 越界會擲出 std::out_of_range，未規定訊息內容，請勿據此做字串判斷。

// === 預期輸出 ===
// === 空容器的狀態 ===
// empty() = true
// size()  = 0
//
// === 危險：以下呼叫是未定義行為（已註解，不執行）===
// front() / back() / pop_back() 對空容器皆為 UB，且無法被 catch 攔下
//
// === 安全做法一：先檢查 empty() ===
// 容器為空 → 略過存取（這是唯一可靠的防線）
//
// === 安全做法二：at() 會擲出可捕捉的例外 ===
// at(0) 擲出 std::out_of_range，成功捕捉
//   e.what() = vector::_M_range_check: __n (which is 0) >= this->size() (which is 0)
//
// === 對照：非空容器一切正常 ===
// front = 7, back = 9
//
// === 日常實務：每日結算報表（含無交易日）===
// [2024-05-17] 筆數=3 首筆=1200.5 末筆=2310.25 總額=4390.75
// [2024-05-18] 本日無交易（0 筆）— 首筆/末筆不適用，總額 0
// [2024-05-19] 筆數=1 首筆=450 末筆=450 總額=450
//
// === 日常實務：工作佇列逐一處理 ===
//   step 1 處理: resize-image（剩餘 2）
//   step 2 處理: send-mail（剩餘 1）
//   step 3 處理: update-index（剩餘 0）
//   佇列已清空，迴圈自然結束（不會誤觸 front()）
