// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定2.cpp  —  有限次數重試（bounded retry）
// =============================================================================
//
// 【主題資訊 Information】
//   bool std::mutex::try_lock();                                    // C++11，<mutex>
//   本檔模式：for (attempt = 1..N) { if (try_lock()) 做事並返回;
//                                    sleep(backoff); }  逾次數則放棄
//   標頭檔：<mutex>、<thread>、<chrono>
//   對照工具：std::timed_mutex::try_lock_for()（C++11）——
//             以「時間」為上限而非「次數」，通常才是正解（見下方第 4 點）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要「有上限」的重試】
//   try_lock() 失敗後有三種可能的反應：
//     (a) 直接放棄  → 適合「這次不做也沒關係」（上一檔的最佳努力取樣）
//     (b) 無限重試  → 等同 lock()，但用自旋實作，通常是壞主意
//     (c) 有限重試  → 本檔：給它幾次機會，還是拿不到就走替代路徑
//   (c) 的價值在於【可預測的最壞延遲】。呼叫端可以計算出
//   「這個函式最多花 maxRetries × backoff 就一定會回來」，
//   這對有 SLA 要求的服務非常重要——寧可回傳「暫時無法服務」，
//   也不要讓請求無限期地掛著。
//
// 【2. 重試之間【必須】要有等待】
//   如果把 sleep_for 拿掉，變成緊密的重試迴圈：
//       for (int i = 0; i < 1000; ++i) { if (mtx.try_lock()) ... }
//   這在極短的時間內把 CPU 燒滿，而且——反直覺的地方來了——
//   它反而【降低】了成功機率：這條執行緒佔著 CPU 猛試，
//   持有鎖的那條執行緒可能因此更難被排到，鎖就更晚被釋放。
//   等待讓出 CPU，才是幫助持有者早點完成的方式。
//
// 【3. 固定間隔 vs 指數退避（exponential backoff）】
//   本檔用固定的 10ms 間隔，簡單好懂。真實系統多半用指數退避：
//       10ms → 20ms → 40ms → 80ms …（通常再加上隨機抖動 jitter）
//   理由：競爭輕微時前幾次就成功，退避不影響延遲；
//   競爭嚴重時間隔迅速拉長，避免大量執行緒以相同節奏反覆撞擊
//   （這種同步化的重試風暴叫 thundering herd）。
//   加入隨機 jitter 則是為了打散「所有人同時醒來再一起搶」的節拍。
//
// 【4. 老實說：這個模式多半應該用 timed_mutex 取代】
//   本檔的「次數 × 間隔」其實是在土炮一個逾時機制，而且土炮得不太好：
//     * 真正的等待時間 = N × backoff + N 次 try_lock 的成本，不精確；
//     * 每次 sleep 都是完整的 context switch，比直接睡到被喚醒更貴；
//     * 鎖在兩次嘗試【之間】被釋放又被別人搶走時，這次就白等了。
//   標準的正規做法是：
//       std::timed_mutex tm;
//       if (tm.try_lock_for(std::chrono::milliseconds(100))) { ... }
//   它內部會正確地睡眠等待（底層 futex 有逾時參數），
//   鎖一釋放就【立刻】被喚醒，而不是傻等到下一個 10ms 的取樣點。
//   → 本檔保留手寫版本，是為了讓「重試」的結構被看見；
//     但實務上請優先考慮 timed_mutex。
//
// 【概念補充 Concept Deep Dive】
//   * 這個模式與網路請求的重試在思想上完全一致：
//     有限次數 + 退避 + 最終放棄並回報失敗。差別只在於
//     這裡爭搶的是一把鎖，那裡爭搶的是遠端服務的容量。
//   * 「放棄」不是失敗處理的結束，而是開始。回傳 false 之後，
//     呼叫端必須有明確的替代方案：走降級路徑、丟進佇列稍後處理、
//     或回報給使用者。如果放棄之後沒有任何替代方案，
//     那從一開始就該用 lock() 老實地等。
//   * 本檔原始版本用 maxRetries = 5、間隔 10ms（總共約 50ms），
//     而持有者的工作恰好也是 50ms —— 兩者太接近，
//     導致「第二條執行緒到底會成功還是放棄」完全看排程，每次執行結果都不同。
//     本檔把兩個情境【刻意分開】並拉開參數差距，
//     讓「一定成功」與「一定放棄」都能穩定重現。
//
// 【注意事項 Pay Attention】
//   1. 重試迴圈中【必須】有等待，否則燒 CPU 且降低成功率。
//   2. 每次 try_lock() 成功後都要 unlock()；本檔的 return true 之前有解鎖，
//      但這種「在 if 裡面 return」的結構很容易寫錯，正式程式請用 RAII。
//   3. 放棄（回傳 false）之後【必須】有明確的替代路徑，否則等於吞掉錯誤。
//   4. 別把「次數 × 間隔」當成精確的逾時；要逾時語意請用 timed_mutex。
//   5. 高競爭場景請改用指數退避 + 隨機 jitter，避免重試風暴。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】有限重試模式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 重試迴圈裡如果不加 sleep，會發生什麼？
//     答：CPU 被燒滿，而且成功機率反而【下降】。
//         這條執行緒佔著 CPU 空轉，持有鎖的那條執行緒因此更難被排程到，
//         鎖就更晚釋放。等待（讓出 CPU）反而是幫助持有者早點完成、
//         讓自己早點拿到鎖的方式。
//     追問：那間隔要設多久？→ 沒有通用答案，取決於臨界區段的長度。
//           實務上用指數退避 + 隨機 jitter，讓它自己適應競爭程度，
//           而不是猜一個固定值。
//
// 🔥 Q2. 「try_lock 重試 N 次、每次間隔 T」和 timed_mutex 的
//        try_lock_for(N×T) 有什麼差別？
//     答：語意相近，但實作品質差很多。手寫版本只在每個取樣點檢查一次，
//         鎖若在兩次嘗試【之間】被釋放，這段空窗就白等了，
//         而且每次 sleep 都是一次完整的 context switch。
//         try_lock_for 底層是帶逾時的 futex 等待——執行緒真的睡著，
//         鎖一釋放就立刻被喚醒，延遲更低、CPU 更省。
//     追問：那什麼時候還是要手寫重試？→ 當「每次重試之間要做別的事」時
//           （例如檢查取消旗標、更新進度、處理其他佇列）。
//
// ⚠️ 陷阱. 「重試 5 次、每次 10ms，所以最多 50ms 一定會拿到鎖。」
//     答：兩個地方都錯。第一，50ms 是【放棄的時間上限】，不是
//         「一定拿得到」——時間到了拿不到就是拿不到，函式回傳 false。
//         第二，連 50ms 這個數字本身也不精確：sleep_for 只保證「至少」，
//         再加上 5 次 try_lock 的成本與排程延遲，實際總時間一定超過 50ms。
//     為什麼會錯：把「重試上限」誤讀成「成功保證」，
//         又把「至少睡 T」誤記成「剛好睡 T」。
//         真正的保證只有一個：這個函式【最終一定會返回】，
//         不會像 lock() 那樣無限期阻塞。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔的核心語意是「試幾次拿不到就【放棄】並走替代路徑」。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）要求
//   每一次輸出都必須完成，沒有任何一題允許放棄——
//   用了這個模式反而會漏輸出而答錯。故從缺。

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;
int shared_data = 0;

// 為了讓輸出可驗證，各次嘗試的訊息先收進 log，最後統一輸出。
std::mutex logMtx;
std::vector<std::string> attemptLog;

void logLine(const std::string& s) {
    std::lock_guard<std::mutex> lk(logMtx);
    attemptLog.push_back(s);
}

bool tryUpdateWithRetry(int id, int maxRetries) {
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        if (mtx.try_lock()) {
            // 成功獲取鎖
            logLine("執行緒 " + std::to_string(id) + "：取得鎖成功");

            ++shared_data;

            // 模擬一些工作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            mtx.unlock();
            return true;
        }

        // 等待一小段時間再重試（不可省略：緊密迴圈會燒 CPU 且降低成功率）
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    logLine("執行緒 " + std::to_string(id) + "：達到最大重試次數，放棄");
    return false;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】日誌檔輪替（log rotation）：拿不到鎖就先寫進暫存緩衝
//   情境：多條執行緒共用一個日誌檔。當某條執行緒正在做檔案輪替
//         （關檔、改名、開新檔，可能耗時數十毫秒）時，其他執行緒
//         【不應該】為了寫一行 log 而阻塞在鎖上——業務流程比日誌重要。
//   策略：try_lock 重試幾次；仍拿不到就把該行放進執行緒本地的暫存區，
//         下次成功取得鎖時一併寫入。這樣既不阻塞主流程，也不會遺失日誌。
//   ⚠️ 這個模式的關鍵是「放棄之後有替代方案」（暫存區），
//      而不是直接把日誌丟掉。沒有替代方案的放棄等於吞掉資料。
// -----------------------------------------------------------------------------
class RotatingLogger {
private:
    std::mutex fileMtx_;              // 保護「檔案」這個共享資源
    std::vector<std::string> written_;  // 模擬已寫入檔案的內容
    int rotations_ = 0;

public:
    // 嘗試把 pending 內的所有行寫出；成功則清空 pending
    bool tryFlush(std::vector<std::string>& pending, int maxRetries) {
        for (int attempt = 1; attempt <= maxRetries; ++attempt) {
            if (fileMtx_.try_lock()) {
                for (const auto& line : pending) {
                    written_.push_back(line);
                }
                pending.clear();
                fileMtx_.unlock();
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;   // 放棄：呼叫端的 pending 仍保有資料，下次再試
    }

    // 模擬耗時的檔案輪替（持有鎖較久）
    void rotate() {
        std::lock_guard<std::mutex> lk(fileMtx_);
        ++rotations_;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::size_t writtenCount() {
        std::lock_guard<std::mutex> lk(fileMtx_);
        return written_.size();
    }
};

int main() {
    std::cout << "=== 情境 A: 重試次數充足 → 兩條執行緒都會成功 ===" << std::endl;
    {
        shared_data = 0;
        attemptLog.clear();

        // 持有者做 50ms 的工作；重試上限 50 次 × 10ms = 500ms，遠遠足夠。
        // （原始課程範例是 5 次 × 10ms = 50ms，恰好與工作時間相同，
        //   導致第二條執行緒是成功還是放棄完全看排程，每次結果都不一樣。）
        std::thread t1(tryUpdateWithRetry, 1, 50);
        std::thread t2(tryUpdateWithRetry, 2, 50);

        t1.join();
        t2.join();

        std::cout << "最終 shared_data = " << shared_data
                  << "  (預期 2：兩條執行緒最終都取得鎖並完成更新)" << std::endl;
        std::cout << "註: 「誰先拿到、各重試了幾次」由排程決定，"
                     "每次執行都不同，故不列出明細" << std::endl;
    }

    std::cout << "\n=== 情境 B: 重試次數不足 → 必然放棄 ===" << std::endl;
    {
        shared_data = 0;
        attemptLog.clear();

        // 主執行緒先霸佔鎖 300ms，worker 只重試 3 次（約 30ms）→ 必然放棄
        mtx.lock();

        bool result = true;
        std::thread worker([&result]() {
            result = tryUpdateWithRetry(9, 3);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        mtx.unlock();
        worker.join();

        std::cout << "worker 回傳值 = " << (result ? "true" : "false")
                  << "  (預期 false：3 次 × 10ms 遠短於持有者的 300ms)" << std::endl;
        std::cout << "shared_data = " << shared_data
                  << "  (預期 0：放棄的執行緒沒有修改任何資料)" << std::endl;
        std::cout << "重點: 放棄是【可預測的】——呼叫端知道最壞情況多久會返回"
                  << std::endl;
    }

    std::cout << "\n=== 日常實務: 日誌輪替期間不阻塞業務執行緒 ===" << std::endl;
    {
        RotatingLogger logger;

        // 一條執行緒不斷做輪替（每次持有鎖 20ms）
        std::thread rotator([&logger]() {
            for (int i = 0; i < 5; ++i) {
                logger.rotate();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });

        // 4 條業務執行緒各要寫 50 行 log
        std::vector<std::thread> apps;
        for (int a = 0; a < 4; ++a) {
            apps.emplace_back([&logger, a]() {
                std::vector<std::string> pending;   // 執行緒本地暫存區
                for (int i = 0; i < 50; ++i) {
                    pending.push_back("app" + std::to_string(a) + "-line" + std::to_string(i));
                    // 拿不到鎖也沒關係，資料留在 pending，繼續跑業務邏輯
                    logger.tryFlush(pending, 3);
                }
                // 收工前務必把剩下的寫出去——這次要有耐心（重試上限拉高）
                while (!pending.empty()) {
                    logger.tryFlush(pending, 1000);
                }
            });
        }

        rotator.join();
        for (auto& t : apps) t.join();

        std::cout << "4 條業務執行緒各產生 50 行，共 200 行" << std::endl;
        std::cout << "實際寫入檔案的行數: " << logger.writtenCount()
                  << "  (必須是 200：放棄只是延後，不是丟棄)" << std::endl;
        std::cout << "業務執行緒從未阻塞在檔案鎖上: 是" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定2.cpp' -o retry_pattern

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 「誰先拿到鎖」「各重試幾次」「哪一行 log 何時被寫出」都由排程決定，
//      【每次執行都不同】，故本檔只輸出可穩定驗證的不變量。
//   2. 情境 A 的 shared_data = 2 是穩定的：重試預算（500ms）遠大於
//      持有者的工作時間（50ms）。
//   3. 情境 B 的 false / 0 是穩定的：重試預算（約 30ms）遠小於
//      主執行緒霸佔鎖的 300ms。
//   4. 日誌行數 200 是【確定】的：收工前的迴圈會一直重試到 pending 清空。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 情境 A: 重試次數充足 → 兩條執行緒都會成功 ===
// 最終 shared_data = 2  (預期 2：兩條執行緒最終都取得鎖並完成更新)
// 註: 「誰先拿到、各重試了幾次」由排程決定，每次執行都不同，故不列出明細
//
// === 情境 B: 重試次數不足 → 必然放棄 ===
// worker 回傳值 = false  (預期 false：3 次 × 10ms 遠短於持有者的 300ms)
// shared_data = 0  (預期 0：放棄的執行緒沒有修改任何資料)
// 重點: 放棄是【可預測的】——呼叫端知道最壞情況多久會返回
//
// === 日常實務: 日誌輪替期間不阻塞業務執行緒 ===
// 4 條業務執行緒各產生 50 行，共 200 行
// 實際寫入檔案的行數: 200  (必須是 200：放棄只是延後，不是丟棄)
// 業務執行緒從未阻塞在檔案鎖上: 是
