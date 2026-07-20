//
// =============================================================================
//  課程 5.6：互斥鎖的效能考量5.cpp  —  永遠不要在鎖內做 I/O
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    臨界區段內的 I/O；swap 技巧與「批次取出、鎖外處理」
//   關鍵手法：localLogs.swap(logs);   // O(1) 交換指標，臨界區段極短
//   標準版本：std::vector::swap 為 C++98 且是 noexcept（C++11 起明確標註）
//   標頭檔：  <mutex>、<sstream>、<vector>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//
// 【詳細解釋 Explanation】
//
// 【1. I/O 為什麼是臨界區段的頭號公敵】
//   成本的數量級對比（本機等級的粗略概念）：
//     * mutex 無競爭        ：十幾 ~ 二十幾 ns
//     * 寫入終端機（tty）    ：數 μs ~ 數十 μs（每次都是系統呼叫，且可能同步）
//     * 寫入檔案（有緩衝）   ：數百 ns ~ 數 μs
//     * 磁碟 flush / fsync   ：數百 μs ~ 數 ms
//     * 網路請求             ：數 ms ~ 數百 ms
//   把任何一項放進鎖內，都等於把鎖扣住成千上萬個「鎖操作」的時間。
//   更糟的是 I/O 可能【阻塞】：磁碟滿了、網路斷了、終端機被暫停
//   （使用者按了 Ctrl+S！），此時所有等這把鎖的執行緒【全部卡死】。
//   → 這不只是效能問題，是可用性問題。
//
// 【2. 三種寫法的演進】
//   * badLog   ：lock → cout << → unlock
//                所有執行緒的輸出完全序列化，且鎖被 I/O 的時間扣住。
//   * goodLog  ：lock → push_back → unlock，I/O 移到鎖外
//                臨界區段縮成一次 push_back（可能觸發 vector 擴容，見注意事項）。
//   * flushLogs：lock → swap → unlock，然後在鎖外一次輸出整批
//                臨界區段縮到【只有一次指標交換】—— O(1)，與資料量無關。
//
// 【3. swap 技巧為什麼是這裡的關鍵】
//       std::vector<std::string> localLogs;
//       {
//           std::lock_guard<std::mutex> lock(mtx);
//           localLogs.swap(logs);      // ← 臨界區段只有這一行
//       }
//       for (const auto& log : localLogs) { /* 鎖外做 I/O */ }
//   std::vector::swap 只交換三個指標（begin / end / capacity），
//   是 O(1) 且 noexcept 的操作 —— 不論容器裡有 10 筆還是 100 萬筆，
//   臨界區段的長度都一樣。
//   交換之後，logs 變成空的（新的訊息會進到這個空容器），
//   而 localLogs 拿到了全部舊資料，可以在鎖外慢慢處理。
//   → 這個「swap 出來再處理」的模式是所有生產者-消費者系統的基本功，
//     也是 5.5-6 的 AsyncLogger 與各種批次處理器的核心。
//
// 【4. 為什麼不是「複製出來再處理」】
//   `std::vector<std::string> localLogs = logs;` 是 O(n) 的深複製，
//   而且會複製每一個 std::string（各自都可能配置記憶體）——
//   這個成本仍然在鎖內。swap 是 O(1)，差距在資料量大時是數量級的。
//   （若必須保留原資料，才用複製；本例是「取走並清空」，swap 完美契合。）
//
// 【5. 本檔 main 的設計：為什麼 badLog 被註解掉】
//   main 中刻意保留 `//badLog(oss.str());` 與 `goodLog(oss.str());` 的對照，
//   讓讀者一眼看出兩種寫法的差別。
//   注意 goodLog 之後必須呼叫 flushLogs()，否則訊息只會留在容器裡不會輸出 ——
//   這是「非同步 / 批次」模式的固有取捨：
//   【效能換即時性】。若程式在 flush 之前崩潰，緩衝中的日誌就遺失了。
//   正式系統會用「定期 flush + 崩潰處理器中 flush + ERROR 等級立即 flush」
//   三者組合來平衡。
//
// 【概念補充 Concept Deep Dive】
//
// (A) goodLog 的臨界區段其實沒有想像中短
//   `logs.push_back(message)` 在鎖內做了兩件事：
//     ① 複製 std::string（可能配置記憶體 —— 而 malloc 內部自己也有鎖！）
//     ② vector 若容量不足會重新配置並搬移【所有】既有元素（O(n)）
//   所以偶爾會有一次 push_back 特別慢（攤銷 O(1) 不代表每次 O(1)）。
//   要進一步優化可以：先 reserve() 預留容量，
//   或改用 std::move(message) 避免字串複製。
//   → 這呼應 5.6-4 的教訓：「鎖內的隱藏慢動作」比想像中多。
//
// (B) 為什麼多執行緒直接 cout 不會 UB，卻仍然不該做
//   C++11 起標準保證對標準串流物件的並行格式化輸出不構成 data race
//   （libstdc++ 在 streambuf 層有內部鎖）。所以 badLog 即使不加自己的鎖
//   也不是 UB。但：
//     ① 輸出會交錯（多個 << 之間可被插入）
//     ② 那把內部鎖同樣會序列化所有執行緒
//   → 「不是 UB」與「效能可以接受」是兩件完全不同的事。
//
// (C) 這個模式的通用形式：double buffering
//   swap 技巧的本質是雙緩衝：一個緩衝區收集新資料（生產者用），
//   另一個緩衝區被處理（消費者用），定期交換角色。
//   同樣的模式出現在圖形渲染（front/back buffer）、
//   日誌收集、metrics 上報、以及 RCU（read-copy-update）。
//
// 【注意事項 Pay Attention】
// 1. 絕對不要在鎖內做 I/O —— 不只慢，還可能阻塞導致所有等待者卡死。
// 2. 用 swap（O(1)、noexcept）把資料取出，不要用複製（O(n)）。
// 3. push_back 在鎖內仍可能觸發記憶體配置與 vector 擴容 ——
//    可先 reserve()，或用 std::move 避免字串複製。
// 4. 批次模式是「效能換即時性」：flush 之前崩潰會遺失緩衝中的日誌。
// 5. 多執行緒直接 cout 不是 UB（標準有保證），但輸出會交錯、且一樣會序列化。
// 6. 本檔的 goodLog 必須搭配 flushLogs 才看得到輸出。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔主題是「把 I/O 移出臨界區段」的工程手法，沒有可判題的演算法；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都不涉及 I/O 與批次緩衝。硬湊只會失焦，故從缺，
//   改以下方兩個真實情境（監控指標批次上報、慢查詢日誌）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】臨界區段內的 I/O
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼絕對不能在鎖內做 I/O?
//     答：兩個理由。① 成本：一次 mutex 操作是十幾奈秒，
//         一次終端機輸出是數微秒、一次磁碟 flush 可到數毫秒 ——
//         等於把鎖扣住成千上萬個鎖操作的時間。
//         ② 可用性：I/O 可能【阻塞】（磁碟滿、網路斷、終端機被暫停），
//         此時所有等這把鎖的執行緒全部卡死。這不只是慢，是掛掉。
//     追問：那不得不在鎖內記錄狀態怎麼辦?
//         → 在鎖內只把資料 push 進容器（純記憶體操作），
//           真正的 I/O 交給鎖外或專責執行緒做。
//
// 🔥 Q2. flushLogs 為什麼用 swap 而不是複製?
//     答：std::vector::swap 只交換三個指標（begin/end/capacity），
//         是 O(1) 且 noexcept —— 不論容器裡有 10 筆還是 100 萬筆，
//         臨界區段長度都一樣。複製則是 O(n)，
//         而且要逐一複製每個 std::string（各自可能配置記憶體），
//         這個成本全部落在鎖內。
//     追問：swap 之後原本的容器變成什麼?
//         → 變成空的（拿到 localLogs 原本的空狀態），
//           新訊息會進到這個空容器，處理與收集互不干擾 ——
//           這就是雙緩衝 (double buffering) 的基本形式。
//
// ⚠️ 陷阱. 「我已經把 cout 移到鎖外了，臨界區段裡只剩 push_back，
//         應該夠短了吧?」——還可能有什麼問題?
//     答：push_back 在鎖內其實做了兩件可能很慢的事：
//         ① 複製 std::string —— 會呼叫 malloc，而 malloc 內部自己有鎖
//         ② vector 容量不足時重新配置並搬移所有既有元素（O(n)）
//         所以偶爾會有一次 push_back 特別慢。
//         攤銷 O(1) 說的是平均，不是每一次。
//     為什麼會錯：把「一行程式碼」等同於「很短的時間」。
//         判斷臨界區段長度要看【最壞情況】而不是平均：
//         任何可能觸發記憶體配置、系統呼叫或 O(n) 搬移的操作，
//         都要當成「可能很慢」來看待。
//         改善方式是預先 reserve()、用 std::move 避免複製，
//         或改成固定大小的環形緩衝區。
// ═══════════════════════════════════════════════════════════════════════════
// 檔案：lesson_5_6_avoid_io_in_lock.cpp
// 說明：避免在臨界區段內進行 I/O

#include <iostream>
#include <thread>
#include <mutex>
#include <sstream>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>

std::mutex mtx;
std::vector<std::string> logs;

// 💀 差的做法：在鎖內進行 I/O
void badLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << message << std::endl;  // 💀 I/O 在鎖內！
}

// ✓ 好的做法：只保護共享資料
void goodLog(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        logs.push_back(message);  // ✓ 只保護共享容器
    }
    // I/O 在鎖外（或由專門的執行緒處理）
}

// ✓ 更好的做法：批次輸出
void flushLogs() {
    std::vector<std::string> localLogs;
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        localLogs.swap(logs);  // 快速交換
    }
    
    // 在鎖外進行 I/O
    for (const auto& log : localLogs) {
        std::cout << log << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】監控指標批次上報（swap 技巧的標準用法）
//   情境：服務每處理一個請求就記錄一筆延遲樣本，每 10 秒上報一次。
//         若上報時鎖住整個容器做網路請求，所有工作執行緒都會卡住。
//   做法：上報執行緒用 swap 把整批樣本取走（臨界區段 O(1)），
//         然後在鎖外慢慢做網路上報。工作執行緒完全不受影響。
// -----------------------------------------------------------------------------
class MetricsBuffer {
private:
    std::mutex mtx;
    std::vector<long> samples;
    long reportedCount = 0;
    int flushRounds = 0;

public:
    MetricsBuffer() { samples.reserve(4096); }   // 預留容量，減少鎖內擴容

    void record(long latencyUs) {
        std::lock_guard<std::mutex> lock(mtx);
        samples.push_back(latencyUs);            // 純記憶體操作
    }

    // 取走整批（O(1)），回傳給呼叫端在鎖外處理
    std::vector<long> drain() {
        std::vector<long> out;
        out.reserve(4096);
        {
            std::lock_guard<std::mutex> lock(mtx);
            out.swap(samples);                   // ← 臨界區段只有這一行
            ++flushRounds;
        }
        return out;                              // 鎖外：呼叫端愛怎麼處理都行
    }

    void markReported(size_t n) {
        std::lock_guard<std::mutex> lock(mtx);
        reportedCount += static_cast<long>(n);
    }

    long reported() { std::lock_guard<std::mutex> lock(mtx); return reportedCount; }
    int rounds()    { std::lock_guard<std::mutex> lock(mtx); return flushRounds; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】慢查詢日誌：鎖內只判斷，格式化與輸出都在鎖外
//   情境：資料庫代理要記錄執行超過門檻的查詢。
//   常見錯誤：在鎖內組出完整的日誌字串（含 SQL 全文、參數、堆疊）
//         —— 字串串接會配置記憶體，而且多數查詢根本不慢，白做工。
//   正解：① 先在鎖外判斷是否超過門檻（不需要共享資料）
//         ② 超過才在鎖外組字串
//         ③ 鎖內只做一次 push_back（用 std::move 避免複製）
// -----------------------------------------------------------------------------
class SlowQueryLog {
private:
    std::mutex mtx;
    std::vector<std::string> entries;
    long thresholdUs;

public:
    explicit SlowQueryLog(long thresholdMicros) : thresholdUs(thresholdMicros) {
        entries.reserve(1024);
    }

    void maybeRecord(const std::string& sql, long elapsedUs) {
        // ① 門檻判斷在鎖外（thresholdUs 建構後不再修改，唯讀安全）
        if (elapsedUs < thresholdUs) return;

        // ② 格式化在鎖外
        std::ostringstream oss;
        oss << "[slow " << elapsedUs << "us] " << sql;
        std::string line = oss.str();

        // ③ 鎖內只有一次 push_back，且用 move 避免字串複製
        std::lock_guard<std::mutex> lock(mtx);
        entries.push_back(std::move(line));
    }

    std::vector<std::string> drain() {
        std::vector<std::string> out;
        {
            std::lock_guard<std::mutex> lock(mtx);
            out.swap(entries);
        }
        return out;
    }
};

int main() {
    const int numThreads = 4;
    const int logsPerThread = 5;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, logsPerThread]() {
            for (int i = 0; i < logsPerThread; ++i) {
                std::ostringstream oss;
                oss << "Thread " << t << " - Log " << i;
                
                //badLog(oss.str());  // 差的做法
                goodLog(oss.str());   // 好的做法
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    flushLogs();  // 輸出所有日誌
    std::cout << "→ 以上 20 行由 flushLogs() 在【鎖外】一次輸出；" << std::endl;
    std::cout << "  臨界區段只有一次 swap（O(1)，與資料量無關）。" << std::endl;
    std::cout << "  行的順序取決於各執行緒寫入 logs 的先後，每次執行都可能不同。" << std::endl;

    std::cout << "\n=== 日常實務 1：監控指標批次上報 ===" << std::endl;
    {
        MetricsBuffer buf;
        std::atomic<bool> stop{false};
        std::vector<std::thread> workers;

        // 4 條工作執行緒不斷記錄樣本
        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([&buf, &stop, i] {
                long k = 0;
                while (!stop.load(std::memory_order_relaxed)) {
                    buf.record((k++ % 500) + i);
                }
            });
        }

        // 上報執行緒：swap 取走整批，在鎖外「上報」
        std::thread reporter([&buf, &stop] {
            long total = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                std::vector<long> batch = buf.drain();      // 臨界區段 O(1)
                // ── 鎖外：模擬網路上報（此處只做加總）──
                long sum = 0;
                for (long v : batch) sum += v;
                if (sum == -1) std::cout << "";             // 防止被最佳化掉
                buf.markReported(batch.size());
                total += static_cast<long>(batch.size());
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            std::vector<long> tail = buf.drain();           // 收尾
            buf.markReported(tail.size());
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stop.store(true);
        for (auto& t : workers) t.join();
        reporter.join();

        std::cout << "4 條工作執行緒持續記錄 50 ms" << std::endl;
        std::cout << "共上報樣本數: " << buf.reported() << " 筆（每次執行都不同）" << std::endl;
        std::cout << "上報回合數  : " << buf.rounds() << " 次（每次執行都不同）" << std::endl;
        std::cout << "→ 上報過程完全不阻塞工作執行緒：" << std::endl;
        std::cout << "  鎖只在 swap 的那一瞬間被持有。" << std::endl;
    }

    std::cout << "\n=== 日常實務 2：慢查詢日誌（門檻判斷與格式化都在鎖外）===" << std::endl;
    {
        SlowQueryLog slowLog(1000);   // 門檻 1000 μs
        std::vector<std::thread> ths;

        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&slowLog, i] {
                for (int k = 0; k < 5000; ++k) {
                    long elapsed = ((i * 5000 + k) % 2000);   // 一半會超過門檻
                    slowLog.maybeRecord("SELECT * FROM orders WHERE id = "
                                        + std::to_string(k), elapsed);
                }
            });
        }
        for (auto& t : ths) t.join();

        std::vector<std::string> recorded = slowLog.drain();
        std::cout << "4 執行緒 × 5000 次查詢 = 20000 次" << std::endl;
        std::cout << "記錄下來的慢查詢: " << recorded.size()
                  << " 筆（必定為 10000，剛好一半超過門檻）" << std::endl;
        std::cout << "範例: " << (recorded.empty() ? "(無)" : recorded.front().substr(0, 40))
                  << "..." << std::endl;
        std::cout << "→ 另外那 10000 次連鎖都沒有碰到（門檻判斷在鎖外）" << std::endl;
    }
    
    return 0;
}
// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量5.cpp' -o avoid_io_in_lock

// ⚠️ 兩處內容【每次執行都不同】，下面貼的是本機某一次的真實實測：
//   (1) 開頭 20 行日誌的【順序】——取決於四條執行緒寫入 logs 的先後。
//       確定的是必定剛好 20 行（4 執行緒 × 5 筆），且每行完整不交錯。
//   (2)「共上報樣本數」與「上報回合數」——工作執行緒在 50 ms 內能記錄多少筆，
//       完全取決於 CPU 速度與排程（本次為 494623 筆 / 48 回合）。
//
// 「記錄下來的慢查詢: 10000 筆」則是確定值：
// 20000 次查詢中延遲對 2000 取模，剛好一半 ≥ 門檻 1000 μs。

// === 預期輸出 ===
// Thread 3 - Log 0
// Thread 2 - Log 0
// Thread 3 - Log 1
// Thread 2 - Log 1
// Thread 2 - Log 2
// Thread 1 - Log 0
// Thread 2 - Log 3
// Thread 2 - Log 4
// Thread 1 - Log 1
// Thread 1 - Log 2
// Thread 1 - Log 3
// Thread 1 - Log 4
// Thread 3 - Log 2
// Thread 3 - Log 3
// Thread 3 - Log 4
// Thread 0 - Log 0
// Thread 0 - Log 1
// Thread 0 - Log 2
// Thread 0 - Log 3
// Thread 0 - Log 4
// → 以上 20 行由 flushLogs() 在【鎖外】一次輸出；
//   臨界區段只有一次 swap（O(1)，與資料量無關）。
//   行的順序取決於各執行緒寫入 logs 的先後，每次執行都可能不同。
//
// === 日常實務 1：監控指標批次上報 ===
// 4 條工作執行緒持續記錄 50 ms
// 共上報樣本數: 481778 筆（每次執行都不同）
// 上報回合數  : 47 次（每次執行都不同）
// → 上報過程完全不阻塞工作執行緒：
//   鎖只在 swap 的那一瞬間被持有。
//
// === 日常實務 2：慢查詢日誌（門檻判斷與格式化都在鎖外）===
// 4 執行緒 × 5000 次查詢 = 20000 次
// 記錄下來的慢查詢: 10000 筆（必定為 10000，剛好一半超過門檻）
// 範例: [slow 1000us] SELECT * FROM orders WHERE...
// → 另外那 10000 次連鎖都沒有碰到（門檻判斷在鎖外）
