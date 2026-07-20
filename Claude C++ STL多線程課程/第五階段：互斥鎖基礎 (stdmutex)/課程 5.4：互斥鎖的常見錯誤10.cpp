// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤10.cpp  —  例外安全：那條看不見的離開路徑
// =============================================================================
//
// 【主題資訊 Information】
//   std::lock_guard（C++11，<mutex>）在 stack unwinding 期間的行為：
//       例外拋出 → 堆疊展開（stack unwinding）
//                → 作用域內所有【已完整建構】的自動物件依序解構
//                → lock_guard 的解構函式執行 → mutex 被解鎖
//   標準依據：解構函式在 stack unwinding 時必然被呼叫，這是語言保證。
//   相關：std::lock_guard 的解構函式是隱含 noexcept 的
//         （unlock() 不拋例外）——這一點至關重要，見「概念補充」。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼例外是最危險的那條路徑】
//   忘記 unlock 有三種成因，危險程度依序遞增：
//     (a) 純粹忘記寫 —— 程式碼審查看得出來
//     (b) 提前 return —— 認真讀還是找得到，因為 return 是可見的字元
//     (c) 【拋出例外】 —— 原始碼裡【沒有任何字元】標示這裡會跳走
//   第三種最致命，因為一段看起來完全線性的程式碼：
//       mtx.lock();
//       results.push_back(compute(x));    // 這一行可能拋例外
//       mtx.unlock();
//   push_back 可能因為重新配置記憶體而丟 std::bad_alloc，
//   compute() 內部也可能拋任何東西。一旦發生，unlock() 直接被跳過，
//   鎖永遠不會釋放——而且【編譯器不會給任何警告】。
//
// 【2. 哪些「看起來不會拋例外」的操作其實會】
//   這份清單比多數人以為的長很多：
//       v.push_back(x)      → 重新配置 → std::bad_alloc
//       s + "text"          → 配置字串 → std::bad_alloc
//       m[key] = v          → 節點配置 → std::bad_alloc
//       m.at(key)           → 查無 key → std::out_of_range
//       new T               → std::bad_alloc
//       std::stoi(s)        → std::invalid_argument / std::out_of_range
//       任何虛擬函式呼叫    → 實作可能拋出任何東西
//       任何樣板參數的操作  → 使用者提供的型別可能拋出
//   → 結論：除非函式標了 noexcept，否則【假設它會拋例外】才是安全的心態。
//     這也是為什麼「手寫 lock/unlock 只在極短且明顯 noexcept 的區段可接受」。
//
// 【3. stack unwinding 保證了什麼、不保證什麼】
//   【保證】：所有已【完整建構】的自動儲存期物件，
//             會依建構的相反順序呼叫解構函式。
//   【不保證】：
//     * 建構到一半就拋例外的物件不會被解構（它還不算存在）——
//       但已完成建構的成員會被解構；
//     * 動態配置的物件（裸 new）不會被自動釋放 → 這是記憶體洩漏的來源，
//       所以要用 unique_ptr / shared_ptr；
//     * 若在解構函式中【又】拋出例外，且當下正在處理另一個例外，
//       程式直接 std::terminate()。
//
// 【4. 例外安全的三個等級（Abrahams 保證）】
//   本檔的 safeOperation 屬於哪一級？
//     * 【基本保證】(basic)：不洩漏資源、物件仍處於有效狀態，
//       但內容可能已被部分修改。
//     * 【強保證】(strong)：要嘛完全成功，要嘛完全沒發生（交易語意）。
//     * 【不拋保證】(nothrow)：絕不失敗。
//   lock_guard 提供的是「鎖一定會被釋放」——這只達成【基本保證】。
//   ⚠️ 關鍵：如果例外拋出時共享資料已經被改到一半，
//   雖然鎖釋放了，但下一個執行緒會看到【不一致的資料】。
//   要達成強保證，必須自己做 commit-or-rollback（本檔實務範例示範）。
//   → 這是最容易被忽略的一點：RAII 解決的是【資源洩漏】，
//     不是【資料一致性】。兩者是不同的問題。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼 lock_guard 的解構函式必須是 noexcept：
//     若它在 stack unwinding 期間拋出例外，就會有兩個例外同時在飛，
//     C++ 直接呼叫 std::terminate()。所以 std::mutex::unlock()
//     被規定為不拋例外（它在正確使用下不可能失敗）。
//   * throw 之後、catch 之前，鎖在哪個時間點被釋放？
//     答：在 stack unwinding 走出 lock_guard 所在的作用域時，
//     也就是【進入 catch 區塊之前】。所以 catch 區塊裡可以安全地
//     再次取得同一把鎖，不會死結——本檔實測了這一點。
//   * -fno-exceptions 編譯的專案（部分嵌入式／遊戲引擎）呢？
//     那裡沒有例外，(c) 這條路徑消失，但 (a)(b) 仍在，
//     RAII 依然是正確做法。
//
// 【注意事項 Pay Attention】
//   1. 除非明確標記 noexcept，否則要假設任何函式都可能拋例外。
//   2. RAII 保證「鎖被釋放」，【不保證】「資料保持一致」——
//      後者需要自己做 rollback 或「先算好再一次寫入」。
//   3. 解構函式中絕不可讓例外逸出（會 terminate）。
//   4. 例外在【離開 lock_guard 的作用域時】就解鎖，
//      所以 catch 區塊可以安全地再取同一把鎖。
//   5. 裸 new 的物件不會因 stack unwinding 而釋放，請用智慧指標。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】例外安全與鎖
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 臨界區段裡拋出例外時，鎖在什麼時候被釋放？catch 區塊裡能再取
//        同一把鎖嗎？
//     答：在 stack unwinding 展開到 lock_guard 所在的作用域時就解鎖了，
//         也就是【進入 catch 區塊之前】。
//         因此 catch 區塊裡可以安全地再次取得同一把鎖，不會死結。
//         這是語言層級的保證：解構函式在堆疊展開時必然被呼叫。
//     追問：那為什麼 unlock() 必須是不拋例外的？
//           → 若解構時拋出例外，而當下正在處理另一個例外，
//             兩個例外同時在飛，C++ 會直接 std::terminate()。
//
// 🔥 Q2. 用了 lock_guard，我的函式就是例外安全的了嗎？
//     答：只達成【基本保證】——資源（鎖）不會洩漏、物件仍有效。
//         但如果例外拋出時共享資料已被修改到一半，
//         鎖雖然釋放了，下一個執行緒會讀到【不一致的中間狀態】。
//         要達成【強保證】（要嘛全成功、要嘛全沒發生），
//         必須自己設計：先在區域變數上算好完整結果，
//         最後用不會拋例外的操作一次寫入（commit）；或準備 rollback。
//     追問：怎麼讓「最後的寫入」保證不拋例外？
//           → 事先 reserve() 好容量、用 std::swap（通常 noexcept）、
//             或只賦值 POD 型別。關鍵是把所有可能拋例外的動作
//             （配置記憶體）挪到修改共享狀態【之前】。
//
// ⚠️ 陷阱. 這段程式碼看起來完全線性，沒有 return 也沒有 throw，
//        所以 unlock() 一定會執行，對嗎？
//            mtx.lock();
//            cache.push_back(computeValue(key));
//            mtx.unlock();
//     答：不對。push_back 可能因為容量不足而重新配置記憶體，
//         失敗時丟 std::bad_alloc；computeValue() 內部也可能拋出任何東西。
//         任一發生，unlock() 都會被完全跳過，鎖從此永遠不會釋放，
//         之後所有想取這把鎖的執行緒都永久阻塞。
//     為什麼會錯：把「原始碼裡看得到的控制流」當成「全部的控制流」。
//         例外是一條【隱形的分支】——它不需要任何關鍵字標示，
//         任何一次函式呼叫、任何一次記憶體配置都可能觸發。
//         這正是 RAII 存在的核心理由：把清理綁在物件生命週期上，
//         就不必去推理有哪些離開路徑。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   例外安全是 C++ 資源管理的核心議題，但 LeetCode 的並行題
//   （1114 / 1115 / 1116 / 1117 / 1195）的解法完全不涉及例外——
//   它們沒有記憶體配置失敗以外的錯誤路徑，評測也不會注入例外。
//   本檔改以「轉帳交易的 commit-or-rollback」實務範例呈現，
//   那才是例外安全真正被檢驗的場景。

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

void safeOperation(int value) {
    std::lock_guard<std::mutex> lock(mtx);  // ✓ RAII

    std::cout << "開始處理 " << value << std::endl;

    if (value < 0) {
        throw std::invalid_argument("值不能為負數");
        // ✓ 例外拋出時，lock_guard 解構，自動 unlock
    }

    std::cout << "處理完成" << std::endl;
}  // ✓ 正常返回時也會自動 unlock

// -----------------------------------------------------------------------------
// 【日常實務範例】帳戶轉帳：從「基本保證」升級到「強保證」
//   情境：從 A 帳戶轉錢到 B 帳戶，過程中要寫一筆交易紀錄。
//         寫紀錄需要配置記憶體（字串、vector），有可能拋出 std::bad_alloc；
//         另外還有業務規則檢查可能拋出例外。
//
//   ❌ 只有基本保證的寫法（badTransfer）：
//        扣款 → 寫紀錄（此處拋例外）→ 入帳
//        結果：鎖被正確釋放（lock_guard 的功勞），
//              但錢【已經從 A 扣掉、卻沒進 B】——憑空蒸發。
//              資料處於不一致狀態，這是比洩漏鎖更嚴重的問題。
//
//   ✅ 強保證的寫法（safeTransfer）：
//        把所有「可能拋例外」的動作（配置記憶體、驗證）
//        全部挪到【修改共享狀態之前】完成；
//        真正的修改則是一連串不會拋例外的簡單賦值（commit 階段）。
//        → 要嘛完全成功，要嘛完全沒發生。
//
//   這就是本檔最重要的觀念：RAII 管的是【資源】，
//   資料的一致性要靠設計，不會自動發生。
// -----------------------------------------------------------------------------
class Ledger {
private:
    std::mutex mtx_;
    long balanceA_ = 10000;
    long balanceB_ = 10000;
    std::vector<std::string> journal_;

    // 模擬「寫交易紀錄時發生錯誤」（真實世界可能是 bad_alloc 或磁碟錯誤）
    static std::string makeJournalEntry(long amount, bool simulateFailure) {
        if (simulateFailure) {
            throw std::runtime_error("寫入交易紀錄失敗（模擬 I/O 錯誤）");
        }
        return "transfer " + std::to_string(amount);
    }

public:
    // ❌ 基本保證：鎖會釋放，但資料可能不一致
    void badTransfer(long amount, bool simulateFailure) {
        std::lock_guard<std::mutex> lock(mtx_);

        balanceA_ -= amount;                                    // 先改了共享狀態
        journal_.push_back(makeJournalEntry(amount, simulateFailure));  // 💀 可能拋
        balanceB_ += amount;                                    // 拋了就走不到這裡
    }

    // ✅ 強保證：所有可能拋例外的動作都在修改共享狀態之前完成
    void safeTransfer(long amount, bool simulateFailure) {
        std::lock_guard<std::mutex> lock(mtx_);

        // ── 階段 1：驗證與準備（這裡可以自由拋例外，共享狀態還沒被碰過）──
        if (balanceA_ < amount) {
            throw std::invalid_argument("餘額不足");
        }
        std::string entry = makeJournalEntry(amount, simulateFailure);  // 可能拋
        journal_.reserve(journal_.size() + 1);   // 預先配置，讓下面的 push_back 不會拋

        // ── 階段 2：commit（以下全部是不會拋例外的操作）──
        balanceA_ -= amount;
        balanceB_ += amount;
        journal_.push_back(std::move(entry));    // 容量已保留 + move → 不會拋
    }

    long balanceA() { std::lock_guard<std::mutex> lk(mtx_); return balanceA_; }
    long balanceB() { std::lock_guard<std::mutex> lk(mtx_); return balanceB_; }
    long total()    { std::lock_guard<std::mutex> lk(mtx_); return balanceA_ + balanceB_; }
    std::size_t journalSize() { std::lock_guard<std::mutex> lk(mtx_); return journal_.size(); }
};

int main() {
    std::cout << "=== 課程示範: 例外路徑下 lock_guard 仍會解鎖 ===" << std::endl;
    try {
        safeOperation(10);
        safeOperation(-5);  // 拋出例外
    } catch (const std::exception& e) {
        std::cout << "捕獲例外：" << e.what() << std::endl;
    }

    // ✓ 鎖已被正確釋放
    std::cout << "嘗試再次操作..." << std::endl;
    safeOperation(20);  // ✓ 正常執行（若上面漏解鎖，這裡會永久阻塞）

    std::cout << "\n=== 鎖在進入 catch 之前就已釋放 ===" << std::endl;
    try {
        safeOperation(-1);
    } catch (const std::exception&) {
        // 在 catch 區塊【內】再取同一把鎖——不會死結
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "在 catch 區塊內成功取得同一把鎖: 是" << std::endl;
        std::cout << "說明: stack unwinding 走出 lock_guard 的作用域時就解鎖了，"
                     "早於進入 catch" << std::endl;
    }

    std::cout << "\n=== 日常實務: 基本保證 vs 強保證 ===" << std::endl;
    {
        // ❌ 基本保證：鎖沒洩漏，但錢不見了
        {
            Ledger ledger;
            std::cout << "[badTransfer] 轉帳前 A+B 總額: " << ledger.total() << std::endl;
            try {
                ledger.badTransfer(3000, true);   // 中途拋例外
            } catch (const std::exception& e) {
                std::cout << "[badTransfer] 捕獲例外: " << e.what() << std::endl;
            }
            std::cout << "[badTransfer] 轉帳後 A = " << ledger.balanceA()
                      << ", B = " << ledger.balanceB() << std::endl;
            std::cout << "[badTransfer] 總額變成: " << ledger.total()
                      << "  ← 少了 3000！鎖沒洩漏，但資料已不一致" << std::endl;
        }

        // ✅ 強保證：完全沒發生
        {
            Ledger ledger;
            std::cout << "\n[safeTransfer] 轉帳前 A+B 總額: " << ledger.total() << std::endl;
            try {
                ledger.safeTransfer(3000, true);  // 同樣中途拋例外
            } catch (const std::exception& e) {
                std::cout << "[safeTransfer] 捕獲例外: " << e.what() << std::endl;
            }
            std::cout << "[safeTransfer] 轉帳後 A = " << ledger.balanceA()
                      << ", B = " << ledger.balanceB() << std::endl;
            std::cout << "[safeTransfer] 總額仍是: " << ledger.total()
                      << "  ← 完全沒發生，交易語意正確" << std::endl;

            // 成功的路徑
            ledger.safeTransfer(3000, false);
            std::cout << "[safeTransfer] 正常轉帳後 A = " << ledger.balanceA()
                      << ", B = " << ledger.balanceB()
                      << ", 總額 = " << ledger.total() << std::endl;
            std::cout << "[safeTransfer] 交易紀錄筆數: " << ledger.journalSize()
                      << "  (只有成功的那筆)" << std::endl;
        }
    }

    std::cout << "\n=== 結論 ===" << std::endl;
    std::cout << "RAII 保證【鎖一定被釋放】（資源不洩漏）" << std::endl;
    std::cout << "但【資料一致性】要靠設計：把可能拋例外的動作"
                 "全部挪到修改共享狀態之前" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤10.cpp' -o exception_safe

// -----------------------------------------------------------------------------
// 【輸出但書】
//   本檔輸出完全確定：全部是單執行緒的例外路徑示範，
//   不依賴任何排程行為。
//   「在 catch 區塊內成功取得同一把鎖」這行若印不出來（程式卡住），
//   就代表例外路徑漏了解鎖——這正是本檔要證明不會發生的事。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 例外路徑下 lock_guard 仍會解鎖 ===
// 開始處理 10
// 處理完成
// 開始處理 -5
// 捕獲例外：值不能為負數
// 嘗試再次操作...
// 開始處理 20
// 處理完成
//
// === 鎖在進入 catch 之前就已釋放 ===
// 開始處理 -1
// 在 catch 區塊內成功取得同一把鎖: 是
// 說明: stack unwinding 走出 lock_guard 的作用域時就解鎖了，早於進入 catch
//
// === 日常實務: 基本保證 vs 強保證 ===
// [badTransfer] 轉帳前 A+B 總額: 20000
// [badTransfer] 捕獲例外: 寫入交易紀錄失敗（模擬 I/O 錯誤）
// [badTransfer] 轉帳後 A = 7000, B = 10000
// [badTransfer] 總額變成: 17000  ← 少了 3000！鎖沒洩漏，但資料已不一致
//
// [safeTransfer] 轉帳前 A+B 總額: 20000
// [safeTransfer] 捕獲例外: 寫入交易紀錄失敗（模擬 I/O 錯誤）
// [safeTransfer] 轉帳後 A = 10000, B = 10000
// [safeTransfer] 總額仍是: 20000  ← 完全沒發生，交易語意正確
// [safeTransfer] 正常轉帳後 A = 7000, B = 13000, 總額 = 20000
// [safeTransfer] 交易紀錄筆數: 1  (只有成功的那筆)
//
// === 結論 ===
// RAII 保證【鎖一定被釋放】（資源不洩漏）
// 但【資料一致性】要靠設計：把可能拋例外的動作全部挪到修改共享狀態之前
