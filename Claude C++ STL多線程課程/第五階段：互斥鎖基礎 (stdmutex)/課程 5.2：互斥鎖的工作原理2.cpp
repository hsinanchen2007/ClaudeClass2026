// =============================================================================
//  課程 5.2：互斥鎖的工作原理2.cpp  —  CAS（Compare-And-Swap）與自旋鎖
// =============================================================================
//
// 【主題資訊 Information】
//   bool std::atomic<T>::compare_exchange_strong(T& expected, T desired,
//            std::memory_order order = std::memory_order_seq_cst) noexcept;  // C++11
//   bool std::atomic<T>::compare_exchange_weak  (T& expected, T desired,
//            std::memory_order order = std::memory_order_seq_cst) noexcept;  // C++11
//   標頭檔：<atomic>
//   語意（原子地執行）：
//       if (*this == expected) { *this = desired; return true;  }
//       else                   { expected = *this; return false; }
//   ⚠️ 注意 expected 是【傳參考】：失敗時它會被寫成「當時的實際值」。
//      這個設計不是多餘的，是 CAS 迴圈能寫得高效的關鍵（見下方第 3 點）。
//
// 【詳細解釋 Explanation】
//
// 【1. CAS 比 Test-And-Set 強在哪裡】
//   exchange（TAS）只能說「不管現在是什麼，都設成 X，順便告訴我原本是什麼」。
//   CAS 則能表達【條件式更新】：「只有在現在還是我以為的那個值時，才更新」。
//   這句「只有在……才」正是所有無鎖（lock-free）演算法的地基：
//       舊值 = 讀取當前狀態
//       新值 = 根據舊值算出來的結果        ← 這段可以任意複雜
//       if (CAS(舊值 → 新值) 成功) 完成
//       else 有人搶先改了，用新的當前值重算一次
//   TAS 做不到這件事，因為它無法「在計算期間確認狀態沒被別人改過」。
//
// 【2. strong vs weak：spurious failure（偽失敗）】
//   compare_exchange_weak 【允許】在值明明相等的情況下也回傳 false。
//   這叫 spurious failure（偽失敗），聽起來像 bug，其實是效能考量：
//     * 在 ARM / PowerPC 這類 LL/SC（Load-Linked / Store-Conditional）架構上，
//       CAS 是用一組 LDXR/STXR 指令實現的。中間只要發生 context switch、
//       中斷、甚至同一條快取行上的無關寫入，store-conditional 就會失敗。
//     * strong 版必須在內部自己加一層迴圈把偽失敗吃掉；
//       weak 版把這層迴圈交給呼叫端。
//   → 選用原則：
//       * 本來就寫在迴圈裡 → 用 weak（外層迴圈本來就會重試，不必付兩層代價）
//       * 只做「單次嘗試」、不重試 → 用 strong（否則會被偽失敗誤導）
//   在 x86-64 上兩者其實編出來的指令相同（都是 LOCK CMPXCHG），
//   差異只有在 LL/SC 架構上才看得出來。
//
// 【3. expected 為什麼要傳參考——本檔原始碼的那行 expected = false 是必要的】
//   CAS 失敗時會把 expected 覆寫成「當時的實際值」。
//   在本檔的鎖裡，失敗代表 locked 是 true，於是 expected 被寫成 true。
//   下一輪若不重設，就變成「拿 true 去比對 true」——
//   一旦持有者解鎖（locked 變 false），這個比對反而【不再相等】，
//   邏輯就完全反了。所以迴圈裡的 expected = false; 不是多餘的防禦，
//   而是【必要】的。這是 CAS 迴圈最經典的初學者陷阱之一。
//   對比典型的「讀-改-寫」CAS 迴圈，那裡就【不需要】手動重設，
//   因為失敗後拿到的最新值正好就是下一輪要用的基準（見本檔實務範例）。
//
// 【4. ABA 問題：CAS 唯一的根本弱點】
//   CAS 只比對「值」，不比對「歷史」。若一個值從 A 變成 B 又變回 A，
//   CAS 會認為「什麼都沒發生」而成功——但世界其實已經變了。
//   對本檔這種 bool 旗標無所謂（只有兩個狀態，且語意上就是「現在鎖著沒」）；
//   但對「指標」就致命：無鎖 stack 的節點被 pop 出去、釋放、
//   再配置回同一個位址 push 回來，CAS 看到指標相同就通過，
//   實際上串在後面的鏈結已經完全不同了。
//   常見解法：帶標籤的指標（tagged pointer，值 + 版本號一起 CAS）、
//   hazard pointer、epoch-based reclamation。
//
// 【概念補充 Concept Deep Dive】
//   * x86-64 上 CAS 對應 LOCK CMPXCHG。lock 前綴會鎖住快取行
//     （現代 CPU 用快取一致性協定實現，不是真的鎖匯流排），
//     成本大約幾十個 cycle，且是 full barrier。
//   * compare_exchange 有兩個 memory_order 參數的重載：
//     compare_exchange_strong(expected, desired, success_order, failure_order)。
//     失敗時的順序不可比成功時強，也不可以是 release/acq_rel
//     （失敗沒有寫入，release 無意義）。
//   * 為什麼「CAS 迴圈」不是「自旋鎖」：兩者長得很像，語意卻不同。
//     自旋鎖是「等別人放手，然後我獨佔一段時間」；
//     CAS 迴圈是「我自己重算一次再試」——期間沒有任何人被排除在外，
//     所以它是 lock-free 的（至少有一條執行緒必然在推進），
//     而自旋鎖不是（持有者被搶佔，全體停擺）。
//   * 本檔的 shared_counter 結果是【確定】的 200000：
//     所有 ++ 都在鎖內，沒有 data race。
//
// 【注意事項 Pay Attention】
//   1. expected 是【傳參考】，失敗後會被改寫。寫 CAS 迴圈時務必想清楚
//      下一輪的 expected 該是什麼——本檔的鎖必須手動重設回 false。
//   2. 迴圈中的 CAS 用 weak；單次嘗試用 strong。
//   3. CAS 只比對值，無法察覺 ABA。對指標做 CAS 時務必評估這個風險。
//   4. 「lock-free」不等於「wait-free」，也不等於「比較快」。
//      高競爭下 CAS 迴圈可能反覆失敗重試，反而比一把好的 mutex 慢。
//   5. 同 5.2 前一檔：應用程式請直接用 std::mutex / std::atomic，
//      不要自己造自旋鎖。本檔的 CASSpinlock 是教學用途。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】CAS（Compare-And-Swap）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. compare_exchange_weak 和 compare_exchange_strong 差在哪？各該用在哪？
//     答：weak 允許 spurious failure——值明明相等也可能回傳 false。
//         這是為了配合 ARM/PowerPC 的 LL/SC 指令（被中斷或 context switch
//         就會失敗），strong 得在內部多包一層迴圈把它吃掉。
//         所以：本來就寫在重試迴圈裡 → 用 weak（省掉多餘的內層迴圈）；
//         只嘗試一次不重試 → 用 strong（否則會被偽失敗騙到）。
//     追問：x86 上兩者有差嗎？→ 幾乎沒有，都編成 LOCK CMPXCHG；
//           差異只在 LL/SC 架構上才顯現。
//
// 🔥 Q2. 什麼是 ABA 問題？為什麼用 CAS 實作無鎖 stack 特別危險？
//     答：CAS 只比對值不比對歷史。指標從 A 變 B 再變回 A 時，
//         CAS 會誤判「沒人動過」。無鎖 stack 中，被 pop 走的節點可能被釋放後
//         又配置到同一位址並 push 回來，此時 head 指標值相同、
//         但它的 next 鏈結已經完全不同 → 串接錯誤、記憶體損毀。
//     追問：怎麼解？→ tagged pointer（值 + 版本號一起 CAS，
//           x86-64 可用 128-bit CMPXCHG16B）、hazard pointer，
//           或 epoch-based reclamation 延後釋放。
//
// ⚠️ 陷阱. 把本檔 lock() 迴圈裡的「expected = false;」刪掉，會怎樣？
//     答：會壞掉，而且壞的方向很反直覺。CAS 失敗時 expected 被寫成
//         當時的實際值（true）。下一輪就變成「期待 true」——
//         於是持有者解鎖、locked 變回 false 之後，CAS 反而【不再成功】；
//         要等到有【另一條】執行緒把它設成 true 的瞬間才會通過，
//         結果就是兩條執行緒可能同時進入臨界區段，互斥完全失效。
//     為什麼會錯：多數人把 expected 當成「唯讀的比較對象」，
//         沒注意到它是 T& —— 失敗時是【輸出參數】。
//         這也解釋了為什麼一般的 CAS 讀-改-寫迴圈不需要手動重設：
//         那裡失敗後拿到的最新值，正好就是下一輪計算要用的基準。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）本質都是
//   「多執行緒依指定順序輪流輸出」，用一個 atomic 狀態變數 + 自旋或
//   condition_variable 就能解，【用不到 CAS 的條件式更新語意】——
//   那些題目從來不需要「只有在值還是我以為的那個時才更新」。
//   同課「工作原理1」已用 1116 示範自旋等待模式；
//   這裡若再掛一題只是重複，反而模糊了 CAS 真正的用武之地
//   （無鎖資料結構、條件式更新）。故從缺，改以實務範例呈現 CAS 的正規用法。

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

class CASSpinlock {
private:
    std::atomic<bool> locked{false};

public:
    void lock() {
        bool expected = false;

        // compare_exchange_strong：
        // 如果 locked == expected(false)，則設為 true，返回 true
        // 如果 locked != expected，則 expected 被更新為 locked 的值，返回 false
        while (!locked.compare_exchange_strong(expected, true)) {
            expected = false;  // ⚠️ 必要！失敗時 expected 已被寫成 true，
                               //    不重設的話下一輪的比對邏輯會完全反過來
            // 繼續嘗試
        }
    }

    void unlock() {
        locked.store(false);
    }
};

CASSpinlock cas_lock;
int shared_counter = 0;

void worker() {
    for (int i = 0; i < 100000; ++i) {
        cas_lock.lock();
        ++shared_counter;
        cas_lock.unlock();
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】無鎖地記錄「尖峰延遲」（CAS 讀-改-寫迴圈的標準形式）
//   情境：API gateway 要記錄所有請求中最慢的那一筆延遲。
//         這件事無法用 fetch_add 之類的單一原子指令完成——
//         「取最大值」需要先讀舊值、比較、才決定要不要寫，
//         正是 CAS 的典型用途。
//   注意這裡的迴圈【不需要】手動重設 expected：
//         CAS 失敗時 observed 已被更新成當前實際值，
//         那正好就是下一輪比較要用的基準，語意剛好對。
// -----------------------------------------------------------------------------
class LatencyTracker {
private:
    std::atomic<long> peakMicros{0};
    std::atomic<long> totalMicros{0};
    std::atomic<long> samples{0};

public:
    void record(long micros) {
        // (1) 累加：單一原子指令就夠，不需要 CAS
        totalMicros.fetch_add(micros, std::memory_order_relaxed);
        samples.fetch_add(1, std::memory_order_relaxed);

        // (2) 取最大值：必須用 CAS 迴圈
        long observed = peakMicros.load(std::memory_order_relaxed);
        while (micros > observed) {
            // 只有在 peakMicros 仍等於我剛才讀到的 observed 時才寫入。
            // 若失敗，observed 會被自動更新成當前的真實值 → 直接進下一輪比較。
            if (peakMicros.compare_exchange_weak(observed, micros,
                                                 std::memory_order_relaxed)) {
                break;
            }
            // 這裡刻意不重設 observed —— 它已經是最新值了
        }
    }

    long peak()  const { return peakMicros.load(); }
    long count() const { return samples.load(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】只執行一次的初始化旗標（CAS 的「單次嘗試」用法）
//   情境：多條執行緒可能同時發現「快取失效、需要重建」，
//         但重建只能有一條執行緒去做，其餘的略過即可。
//   這裡是【單次嘗試】而非迴圈，所以必須用 compare_exchange_strong——
//         用 weak 的話，偽失敗會讓本該負責重建的那條執行緒誤以為
//         「別人已經在做了」，結果沒有任何人去重建。
//   （真實程式碼可用 std::call_once / std::once_flag，語意更清楚；
//     這裡是為了示範 strong 與 weak 的選用差異。）
// -----------------------------------------------------------------------------
class RebuildGuard {
private:
    std::atomic<bool> rebuilding{false};

public:
    // 回傳 true 表示「這條執行緒搶到了重建工作」
    bool tryClaim() {
        bool expected = false;
        // 單次嘗試，不重試 → 必須用 strong
        return rebuilding.compare_exchange_strong(expected, true);
    }

    void finish() { rebuilding.store(false); }
};

int main() {
    std::cout << "=== 課程示範: CASSpinlock 保護 counter ===" << std::endl;
    {
        std::thread t1(worker);
        std::thread t2(worker);

        t1.join();
        t2.join();

        std::cout << "counter = " << shared_counter
                  << "  (預期 200000；所有 ++ 都在鎖內，結果是確定的)" << std::endl;
    }

    std::cout << "\n=== compare_exchange 的 expected 是輸出參數 ===" << std::endl;
    {
        std::atomic<int> value{42};
        int expected = 7;                       // 故意猜錯
        const bool ok = value.compare_exchange_strong(expected, 100);
        std::cout << "拿 expected=7 去 CAS（實際值 42）→ 回傳 "
                  << (ok ? "true" : "false") << std::endl;
        std::cout << "失敗後 expected 被改寫成: " << expected
                  << "  (這就是為什麼寫 CAS 迴圈要想清楚下一輪的 expected)" << std::endl;
        std::cout << "atomic 的值維持不變: " << value.load() << std::endl;

        // 這次用正確的 expected
        const bool ok2 = value.compare_exchange_strong(expected, 100);
        std::cout << "用被改寫後的 expected=42 再 CAS 一次 → 回傳 "
                  << (ok2 ? "true" : "false")
                  << "，atomic 現在是 " << value.load() << std::endl;
    }

    std::cout << "\n=== 日常實務 1: 無鎖記錄尖峰延遲 ===" << std::endl;
    {
        LatencyTracker tracker;

        // 8 條執行緒各記錄 1000 筆，延遲值刻意做成可預測的集合：
        // 第 t 條執行緒送出的最大值是 (t + 1) * 1000
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&tracker, t]() {
                for (int i = 1; i <= 1000; ++i) {
                    tracker.record(static_cast<long>(t + 1) * i);
                }
            });
        }
        for (auto& th : workers) th.join();

        std::cout << "取樣筆數: " << tracker.count() << "  (預期 8000)" << std::endl;
        std::cout << "尖峰延遲: " << tracker.peak()
                  << " 微秒  (預期 8000 = 第 8 條執行緒的最大值)" << std::endl;
        std::cout << "說明: 「取最大值」無法用單一原子指令完成，"
                     "必須用 CAS 讀-改-寫迴圈" << std::endl;
    }

    std::cout << "\n=== 日常實務 2: 只有一條執行緒能搶到重建工作 ===" << std::endl;
    {
        RebuildGuard guard;
        std::atomic<int> winners{0};

        std::vector<std::thread> workers;
        for (int t = 0; t < 16; ++t) {
            workers.emplace_back([&guard, &winners]() {
                if (guard.tryClaim()) {
                    winners.fetch_add(1);
                    // 模擬重建工作
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    // 注意：這裡刻意不呼叫 finish()，
                    //       確保整場只會有一個贏家，讓輸出可驗證
                }
            });
        }
        for (auto& th : workers) th.join();

        std::cout << "16 條執行緒同時嘗試，搶到重建工作的數量: "
                  << winners.load() << "  (必須恰好是 1)" << std::endl;
        std::cout << "說明: 單次嘗試不重試，所以必須用 strong；"
                     "用 weak 會因偽失敗而可能沒人搶到" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.2：互斥鎖的工作原理2.cpp' -o cas_lock

// -----------------------------------------------------------------------------
// 【輸出但書】
//   本檔所有輸出都是確定的：
//     * counter：所有 ++ 都在鎖內完成，必為 200000。
//     * 尖峰延遲：8 條執行緒送出的最大值必為 8 * 1000 = 8000，
//       CAS 迴圈保證不會漏掉最大值（每次失敗都會拿到最新值重新比較）。
//     * 重建贏家：CAS 保證只有一條執行緒能把 false 換成 true，必為 1。
//   「哪一條執行緒搶到」與「各步驟耗時」則每次都不同，故不列入輸出。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: CASSpinlock 保護 counter ===
// counter = 200000  (預期 200000；所有 ++ 都在鎖內，結果是確定的)
//
// === compare_exchange 的 expected 是輸出參數 ===
// 拿 expected=7 去 CAS（實際值 42）→ 回傳 false
// 失敗後 expected 被改寫成: 42  (這就是為什麼寫 CAS 迴圈要想清楚下一輪的 expected)
// atomic 的值維持不變: 42
// 用被改寫後的 expected=42 再 CAS 一次 → 回傳 true，atomic 現在是 100
//
// === 日常實務 1: 無鎖記錄尖峰延遲 ===
// 取樣筆數: 8000  (預期 8000)
// 尖峰延遲: 8000 微秒  (預期 8000 = 第 8 條執行緒的最大值)
// 說明: 「取最大值」無法用單一原子指令完成，必須用 CAS 讀-改-寫迴圈
//
// === 日常實務 2: 只有一條執行緒能搶到重建工作 ===
// 16 條執行緒同時嘗試，搶到重建工作的數量: 1  (必須恰好是 1)
// 說明: 單次嘗試不重試，所以必須用 strong；用 weak 會因偽失敗而可能沒人搶到
