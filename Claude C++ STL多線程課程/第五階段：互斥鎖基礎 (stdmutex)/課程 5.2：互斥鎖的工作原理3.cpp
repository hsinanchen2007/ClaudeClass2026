// =============================================================================
//  課程 5.2：互斥鎖的工作原理3.cpp  —  Futex：std::mutex 底下究竟是什麼
// =============================================================================
//
// 【主題資訊 Information】
//   Futex = Fast Userspace muTEX（Linux 專屬機制，非 C++ 標準的一部分）
//   系統呼叫：SYS_futex（futex(2)）；本檔只解釋概念，不直接呼叫它。
//   對應到 C++：std::mutex 在 Linux + libstdc++ 上最終走到
//               pthread_mutex_lock/unlock，而 glibc 的實作核心就是 futex。
//   標頭檔（本檔實際用到的）：<atomic>、<mutex>、<condition_variable>
//   關鍵數據（實作定義）：本機 x86-64 / glibc 上 sizeof(std::mutex) = 40 bytes
//                        —— 一把鎖就只是一個小結構，核心裡沒有對應物件。
//
// 【詳細解釋 Explanation】
//
// 【1. Futex 要解決的矛盾】
//   鎖的實作有兩個互相衝突的需求：
//     (a) 無競爭時要【極快】——不該為了「其實沒人跟我搶」付出進核心的代價；
//     (b) 有競爭時要【不燒 CPU】——等待者應該睡著，把 CPU 讓給別人。
//   純自旋鎖只滿足 (a)，純睡眠鎖（每次 lock 都進核心排隊）只滿足 (b)。
//   Futex 的洞見是：把「狀態」放在使用者空間的一個整數裡，
//   只有【真的需要睡覺或叫醒別人】時才進核心。
//   於是 95% 以上的 lock/unlock 完全不觸發系統呼叫。
//
// 【2. 三個狀態的設計為什麼是「2」而不是「1」】
//   最直覺的設計是兩個狀態（0 = 未鎖，1 = 已鎖）。問題出在 unlock：
//   解鎖者無從得知「到底有沒有人在睡覺等我」，
//   為了保險只能【每次都】呼叫 futex_wake 進核心——
//   那 (a) 的好處就沒了，無競爭的解鎖也要付系統呼叫的代價。
//   加入第三個狀態就解決了：
//       0 = 未鎖
//       1 = 已鎖，【沒有】等待者
//       2 = 已鎖，【有】等待者（contended）
//   於是 unlock 可以判斷：狀態是 1 → 直接寫回 0，不進核心；
//   狀態是 2 → 才需要 futex_wake。
//   這個「2」是整個設計的靈魂，也是面試最愛問的細節。
//
// 【3. futex_wait 的原子性檢查——避免 lost wakeup】
//   futex_wait(&state, expected) 的語意不是「無條件睡覺」，而是：
//       「進入核心後【再檢查一次】*addr 是否仍等於 expected，
//         是才睡；否則立刻返回。」
//   為什麼需要這個檢查？考慮這個時序：
//       執行緒 A：讀到 state == 2，決定要睡
//       執行緒 B：（此時）unlock，把 state 設 0 並 futex_wake
//       執行緒 A：（現在才）真的進入睡眠 → 沒有人會再叫醒它了
//   這叫 lost wakeup（喚醒遺失），會造成永久卡死。
//   核心在持有 futex 佇列鎖的狀態下重新比對 *addr，
//   讓「檢查」與「睡眠」變成不可分割的一步，這個競爭就被消除了。
//   同樣的道理也解釋了 condition_variable 為什麼一定要傳入 unique_lock：
//   「釋放鎖」與「開始等待」必須是原子的。
//
// 【4. std::mutex → futex 的完整路徑】
//       std::mutex::lock()
//         → pthread_mutex_lock()                     [glibc]
//           → 先做幾次使用者空間的 CAS（fast path，可能還會自旋幾輪）
//           → 失敗才 syscall(SYS_futex, FUTEX_WAIT, ...)   [進核心]
//       std::mutex::unlock()
//         → pthread_mutex_unlock()
//           → 使用者空間把 state 寫回 0
//           → 只有在偵測到有等待者時才 syscall(SYS_futex, FUTEX_WAKE, ...)
//   這解釋了一個常見的誤解：「用 mutex 就會進核心」——
//   在無競爭的情況下【完全不會】。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼 sizeof(std::mutex) 這麼小（本機 40 bytes）：
//     因為核心裡根本沒有「這把鎖」的物件。核心只在有人真的睡下去時，
//     用「該記憶體位址」當 key，把等待者掛進一個雜湊表裡的佇列。
//     鎖沒有競爭時，核心完全不知道它存在。
//     對比 Windows 的 CreateMutex（核心物件、有 handle、可跨行程）
//     就明顯笨重得多；Windows 對應的輕量機制是 SRWLOCK / CRITICAL_SECTION。
//   * futex 的 key 是「實體記憶體位址」，所以它也能用在共享記憶體上，
//     讓【跨行程】的鎖同樣享有 fast path（pthread 的 PTHREAD_PROCESS_SHARED）。
//   * 驗證方式：對本檔編譯出的程式跑
//       strace -f -e trace=futex ./futex_concept
//     可以看到：無競爭的區段【一個 futex 呼叫都沒有】，
//     而有競爭 / 使用 condition_variable 的區段才出現 FUTEX_WAIT / FUTEX_WAKE。
//     這是把「理論」變成「眼見為憑」最快的方法。
//   * 檔頭那段虛擬碼是【簡化】的教學版本。真實的 glibc 實作還要處理
//     robust mutex、priority inheritance、遞迴/錯誤檢查型別、
//     以及自適應自旋（PTHREAD_MUTEX_ADAPTIVE_NP），複雜得多。
//
// 【注意事項 Pay Attention】
//   1. Futex 是【Linux 專屬】。macOS 用 os_unfair_lock / ulock，
//      Windows 用 WaitOnAddress / SRWLOCK。概念相通，API 完全不同。
//   2. 檔頭虛擬碼不是可編譯的程式碼，也不是 glibc 的真實實作，
//      只是用來說明三狀態與 fast/slow path 的教學模型。
//   3. 【不要】自己直接呼叫 SYS_futex 來造鎖。正確寫法極難
//      （lost wakeup、ABA、優先權反轉、訊號處理都要顧），
//      std::mutex 與 std::condition_variable 已經做好了。
//   4. sizeof(std::mutex) 是【實作定義】的：本機 glibc x86-64 為 40 bytes，
//      在其他平台或標準函式庫實作上會不同。
//   5. 「無競爭不進核心」是 glibc 的實作特性，不是 C++ 標準的保證。
//      標準只規定行為語意，完全沒規定要怎麼實作。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Futex 與 mutex 的底層實作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::mutex 的 lock() 會不會進核心（系統呼叫）？
//     答：無競爭時【不會】。glibc 的 fast path 只是使用者空間的一次 CAS，
//         成本大約幾十個 cycle。只有在真的搶不到、必須睡覺時，
//         才會發出 SYS_futex 的 FUTEX_WAIT。
//         這正是 futex 名稱的由來：Fast Userspace muTEX。
//     追問：怎麼證明？→ strace -e trace=futex 跑一次就知道：
//           無競爭的迴圈跑一百萬次，futex 呼叫次數是 0。
//
// 🔥 Q2. futex 為什麼要有「2 = 有等待者」這個狀態？只用 0/1 不行嗎？
//     答：不行，會讓 unlock 失去 fast path。只有 0/1 的話，
//         解鎖者無法判斷有沒有人在睡，為了不讓等待者永久卡住，
//         只能每次 unlock 都呼叫 futex_wake 進核心。
//         引入「2」之後，unlock 看到 1 就知道沒人等，直接寫回 0 即可返回，
//         完全不需要系統呼叫。
//     追問：那什麼時候 state 會被設成 2？→ 當一條執行緒發現搶不到、
//           準備睡覺之前，它會先把狀態從 1 改成 2，
//           等於留下「這裡有人在等」的記號給未來的解鎖者。
//
// ⚠️ 陷阱. futex_wait(&state, 2) 就是「讓這條執行緒睡到被叫醒」，對吧？
//     答：不完全對，而且漏掉的那半正是它存在的理由。
//         futex_wait 進核心後會【再比對一次】*&state 是否仍等於 2，
//         不相等就立刻返回、根本不睡。
//     為什麼會錯：把它想成單純的 sleep，就無法解釋 lost wakeup 是怎麼被防住的。
//         若「檢查 state == 2」與「睡下去」之間有空隙，
//         解鎖者可能剛好在那個空隙完成 unlock + wake，
//         等待者接著才睡下 → 永遠沒有人會再叫醒它。
//         核心在持有佇列鎖的情況下重新比對，讓這兩步變成原子的。
//         這與 condition_variable 必須傳入 unique_lock 是同一個道理：
//         「放開鎖」和「開始等待」不可以被拆開。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔的主題是「作業系統如何實作互斥鎖」，屬於 OS / 系統程式設計範疇。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）全部是
//   應用層的同步順序題，沒有任何一題涉及鎖的底層實作機制，
//   評測環境也不可能觀察系統呼叫。硬掛一題毫無關聯，故從缺。

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/*
 * 這是 Futex 的概念說明，不是實際可編譯的程式碼
 *
 * Futex 的核心思想：
 *
 * struct futex_mutex {
 *     atomic<int> state;  // 0 = unlocked, 1 = locked (無等待者), 2 = locked (有等待者)
 * };
 *
 * void lock() {
 *     int expected = 0;
 *
 *     // Fast path：嘗試 0 → 1（無競爭）
 *     if (state.compare_exchange_strong(expected, 1)) {
 *         return;  // 成功獲取，無需系統呼叫
 *     }
 *
 *     // Slow path：有競爭
 *     while (true) {
 *         // 標記為「有等待者」
 *         if (expected == 2 || state.compare_exchange_strong(expected, 2)) {
 *             // 系統呼叫：等待 state 變化
 *             // ⚠️ futex_wait 會在核心內【再檢查一次】 state 是否仍為 2，
 *             //    不是的話立刻返回 —— 這就是 lost wakeup 的防線。
 *             futex_wait(&state, 2);
 *         }
 *
 *         expected = 0;
 *         if (state.compare_exchange_strong(expected, 2)) {
 *             return;  // 成功獲取
 *         }
 *     }
 * }
 *
 * void unlock() {
 *     // 如果 state 從 1 變成 0，無等待者 → 不需要進核心
 *     if (state.fetch_sub(1) == 1) {
 *         return;  // 無需喚醒
 *     }
 *
 *     // 有等待者，需要喚醒
 *     state.store(0);
 *     futex_wake(&state, 1);  // 喚醒一個等待者
 * }
 */

// -----------------------------------------------------------------------------
// 【教學用模型】用 atomic + condition_variable 模擬 futex 的三狀態設計
//   ⚠️ 這【不是】futex，也不是給生產環境用的鎖。
//      它的用途是讓「三個狀態各自代表什麼、什麼時候才需要叫醒別人」
//      這件事在可執行的程式碼中被看見——
//      特別是 unlock() 裡那個「state == 1 就不需要 notify」的分支，
//      正對應真實 futex 省下系統呼叫的地方。
// -----------------------------------------------------------------------------
class FutexStyleMutex {
public:
    enum State { Unlocked = 0, LockedNoWaiters = 1, LockedWithWaiters = 2 };

private:
    std::atomic<int>        state{Unlocked};
    std::mutex              waitMtx;      // 模擬核心的等待佇列
    std::condition_variable waitCv;

    // 統計：真正需要「進核心」（此模型中即 cv 等待／喚醒）的次數
    std::atomic<long> slowPathWaits{0};
    std::atomic<long> slowPathWakes{0};

public:
    void lock() {
        int expected = Unlocked;
        // ── Fast path：0 → 1，完全在使用者空間，不碰 cv ──
        if (state.compare_exchange_strong(expected, LockedNoWaiters)) {
            return;
        }

        // ── Slow path：有競爭 ──
        for (;;) {
            // 宣告「這裡有人在等」：1 → 2（若已是 2 就維持）
            expected = LockedNoWaiters;
            state.compare_exchange_strong(expected, LockedWithWaiters);

            {
                std::unique_lock<std::mutex> lk(waitMtx);
                // 對應 futex_wait 的核心內再檢查：
                // 進入等待【之前】在持有 waitMtx 的情況下確認狀態仍是 2，
                // 這就是防止 lost wakeup 的關鍵。
                if (state.load() == LockedWithWaiters) {
                    slowPathWaits.fetch_add(1, std::memory_order_relaxed);
                    waitCv.wait(lk, [this]() {
                        return state.load() != LockedWithWaiters;
                    });
                }
            }

            expected = Unlocked;
            // 醒來後重新搶；搶到就直接標成 2（保守作法：可能還有別人在等）
            if (state.compare_exchange_strong(expected, LockedWithWaiters)) {
                return;
            }
        }
    }

    void unlock() {
        // 若原本是 1（沒有等待者），fetch_sub 之後就是 0，直接返回——
        // 這一行就是 futex 省下 FUTEX_WAKE 系統呼叫的地方。
        if (state.fetch_sub(1) == LockedNoWaiters) {
            return;
        }

        // 原本是 2：有人在睡，必須叫醒
        state.store(Unlocked);
        {
            std::lock_guard<std::mutex> lk(waitMtx);
        }
        slowPathWakes.fetch_add(1, std::memory_order_relaxed);
        waitCv.notify_one();
    }

    long waits() const { return slowPathWaits.load(); }
    long wakes() const { return slowPathWakes.load(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】優雅關機（graceful shutdown）：該睡覺的時候就別自旋
//   情境：背景的批次處理執行緒平常在等待新工作。服務要關閉時，
//         主執行緒設定 shutdown 旗標並喚醒它。
//   為什麼放在這一課：這正是 futex slow path 的應用層對應物——
//         等待可能長達數秒，用 condition_variable 讓執行緒真正睡著
//         （底層就是 FUTEX_WAIT），CPU 使用率是 0；
//         若改用 while (!shutdown) {} 自旋，會白白燒掉一整顆核心。
//   ⚠️ 注意 wait() 一定要傳 predicate（或自己寫 while 迴圈重新檢查），
//      因為 condition_variable 允許 spurious wakeup（假性喚醒）。
// -----------------------------------------------------------------------------
class BackgroundWorker {
private:
    std::mutex              mtx;
    std::condition_variable cv;
    std::vector<std::string> queue;
    bool                    shutdown = false;
    int                     processed = 0;

public:
    void submit(const std::string& job) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push_back(job);
        }
        cv.notify_one();
    }

    void requestShutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            shutdown = true;
        }
        cv.notify_all();
    }

    void run() {
        for (;;) {
            std::unique_lock<std::mutex> lock(mtx);
            // predicate 版本的 wait：自動處理 spurious wakeup
            cv.wait(lock, [this]() { return !queue.empty() || shutdown; });

            if (!queue.empty()) {
                queue.pop_back();
                ++processed;
                continue;
            }
            if (shutdown) return;   // 佇列已清空且收到關機訊號
        }
    }

    int processedCount() {
        std::lock_guard<std::mutex> lock(mtx);
        return processed;
    }
};

int main() {
    std::cout << "=== Futex 概念 ===" << std::endl;
    std::cout << "Futex 是 Linux 實現高效互斥鎖的機制" << std::endl;
    std::cout << "無競爭時：純用戶空間原子操作，無系統呼叫" << std::endl;
    std::cout << "有競爭時：使用系統呼叫讓執行緒睡眠" << std::endl;

    std::cout << "\n=== 一把鎖有多大（實作定義）===" << std::endl;
    std::cout << "sizeof(std::mutex)              = " << sizeof(std::mutex) << " bytes" << std::endl;
    std::cout << "sizeof(std::condition_variable) = " << sizeof(std::condition_variable)
              << " bytes" << std::endl;
    std::cout << "說明: 鎖沒有競爭時，核心裡根本沒有對應的物件——" << std::endl;
    std::cout << "      核心只在有人真的睡下去時，才用該記憶體位址當 key 掛佇列" << std::endl;

    std::cout << "\n=== 三狀態模型: fast path 完全不需要喚醒 ===" << std::endl;
    {
        FutexStyleMutex fm;
        // 單執行緒連續 lock/unlock：永遠只走 0→1→0，不可能有等待者
        for (int i = 0; i < 100000; ++i) {
            fm.lock();
            fm.unlock();
        }
        std::cout << "單執行緒 lock/unlock 十萬次" << std::endl;
        std::cout << "  進入等待（相當於 FUTEX_WAIT）次數: " << fm.waits()
                  << "  (必須是 0)" << std::endl;
        std::cout << "  發出喚醒（相當於 FUTEX_WAKE）次數: " << fm.wakes()
                  << "  (必須是 0)" << std::endl;
        std::cout << "這就是 std::mutex 在無競爭時「不進核心」的原理" << std::endl;
    }

    std::cout << "\n=== 三狀態模型: 有競爭時才需要睡眠與喚醒 ===" << std::endl;
    {
        FutexStyleMutex fm;
        long shared = 0;

        std::vector<std::thread> workers;
        for (int t = 0; t < 4; ++t) {
            workers.emplace_back([&fm, &shared]() {
                for (int i = 0; i < 20000; ++i) {
                    fm.lock();
                    ++shared;
                    fm.unlock();
                }
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "4 執行緒各加 20000 次，結果: " << shared
                  << "  (預期 80000，互斥有效)" << std::endl;
        std::cout << "註: 這次實際發生幾次睡眠/喚醒完全取決於排程，"
                     "每次執行都不同，故不列為預期輸出" << std::endl;
    }

    std::cout << "\n=== 日常實務: 優雅關機（等待用睡眠，不用自旋）===" << std::endl;
    {
        BackgroundWorker worker;
        std::thread bg(&BackgroundWorker::run, &worker);

        for (int i = 0; i < 5; ++i) {
            worker.submit("job-" + std::to_string(i));
        }
        worker.requestShutdown();
        bg.join();

        std::cout << "送出 5 個工作後關機，已處理: " << worker.processedCount()
                  << "  (預期 5，關機前佇列會被清空)" << std::endl;
        std::cout << "等待期間 CPU 使用率為 0——底層就是 FUTEX_WAIT，"
                     "執行緒真的睡著了" << std::endl;
    }

    std::cout << "\n=== 想親眼看見系統呼叫？ ===" << std::endl;
    std::cout << "strace -f -e trace=futex ./futex_concept" << std::endl;
    std::cout << "會看到: 無競爭區段一個 futex 呼叫都沒有；"
                 "有競爭與 cv 等待才出現 FUTEX_WAIT / FUTEX_WAKE" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.2：互斥鎖的工作原理3.cpp' -o futex_concept
// 觀察系統呼叫: strace -f -e trace=futex ./futex_concept

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. sizeof(std::mutex) = 40、sizeof(std::condition_variable) = 48 是
//      【實作定義】的值（本機 x86-64 / glibc / libstdc++）。
//      換平台或換標準函式庫實作，這兩個數字都會改變。
//   2. 「有競爭時」區段實際發生幾次睡眠與喚醒完全取決於排程，
//      每次執行都不同，故刻意不輸出那兩個計數，只驗證互斥的正確性（80000）。
//   3. 「fast path 為 0 次」則是確定的：單執行緒不可能有等待者。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === Futex 概念 ===
// Futex 是 Linux 實現高效互斥鎖的機制
// 無競爭時：純用戶空間原子操作，無系統呼叫
// 有競爭時：使用系統呼叫讓執行緒睡眠
//
// === 一把鎖有多大（實作定義）===
// sizeof(std::mutex)              = 40 bytes
// sizeof(std::condition_variable) = 48 bytes
// 說明: 鎖沒有競爭時，核心裡根本沒有對應的物件——
//       核心只在有人真的睡下去時，才用該記憶體位址當 key 掛佇列
//
// === 三狀態模型: fast path 完全不需要喚醒 ===
// 單執行緒 lock/unlock 十萬次
//   進入等待（相當於 FUTEX_WAIT）次數: 0  (必須是 0)
//   發出喚醒（相當於 FUTEX_WAKE）次數: 0  (必須是 0)
// 這就是 std::mutex 在無競爭時「不進核心」的原理
//
// === 三狀態模型: 有競爭時才需要睡眠與喚醒 ===
// 4 執行緒各加 20000 次，結果: 80000  (預期 80000，互斥有效)
// 註: 這次實際發生幾次睡眠/喚醒完全取決於排程，每次執行都不同，故不列為預期輸出
//
// === 日常實務: 優雅關機（等待用睡眠，不用自旋）===
// 送出 5 個工作後關機，已處理: 5  (預期 5，關機前佇列會被清空)
// 等待期間 CPU 使用率為 0——底層就是 FUTEX_WAIT，執行緒真的睡著了
//
// === 想親眼看見系統呼叫？ ===
// strace -f -e trace=futex ./futex_concept
// 會看到: 無競爭區段一個 futex 呼叫都沒有；有競爭與 cv 等待才出現 FUTEX_WAIT / FUTEX_WAKE
