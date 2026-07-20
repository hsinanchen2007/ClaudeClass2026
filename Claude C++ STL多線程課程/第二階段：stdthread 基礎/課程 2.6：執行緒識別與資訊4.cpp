// =============================================================================
//  課程 2.6：執行緒識別與資訊4.cpp  —  依核心數建立執行緒 + std::thread::id
// =============================================================================
//
// 【主題資訊 Information】
//   std::thread::id                              // C++11，可比較、可雜湊、可輸出
//   std::thread::id      std::thread::get_id() const noexcept;
//   std::thread::id      std::this_thread::get_id() noexcept;
//   static unsigned int  std::thread::hardware_concurrency() noexcept;
//   標頭檔：<thread>、<vector>
//   複雜度：get_id() 為 O(1)；建立執行緒是相對昂貴的作業（微秒級）。
//
// 【詳細解釋 Explanation】
//
// 【1. std::thread::id 是一個「不透明型別」，不是整數】
//   標準【沒有】規定 std::thread::id 的底層表示。它只保證：
//     * 可預設建構（代表「不指涉任何執行緒」）
//     * 支援 ==、!=、<、<=、>、>=（所以能放進 std::map 當 key）
//     * 特化了 std::hash（所以能放進 std::unordered_map）
//     * 支援 operator<<（輸出格式【由實作決定】）
//   它【不能】隱式轉成 int/long。想把它當數字用（例如取模分桶），
//   標準做法是 std::hash<std::thread::id>{}(id)，而不是 reinterpret_cast。
//   libstdc++ 的 operator<< 會印出底層 pthread_t 值，那是實作細節，
//   不同平台、不同執行都可能長得完全不一樣。
//
// 【2. 兩種取得 ID 的方式，語意完全不同】
//     std::this_thread::get_id()  → 在執行緒【內部】呼叫，取得「我是誰」
//     t.get_id()                  → 在執行緒【外部】呼叫，問「t 是誰」
//   關鍵差異：t.get_id() 對一個【非 joinable】的 thread 物件
//   （預設建構、已被 join()、已被 detach()、已被 move 走）
//   會回傳「預設建構的 id」，也就是 std::thread::id{}。
//   所以 t.get_id() != std::thread::id{} 是判斷 t 是否還關聯著執行緒的
//   一種寫法，雖然直接用 t.joinable() 更直觀。
//
// 【3. ID 會被【重複使用】——這是最容易踩的坑】
//   標準明說：一個執行緒結束後，它的 id 值【可以】被之後建立的執行緒重用。
//   所以「用 thread::id 當長期的物件識別碼」是錯的設計：
//   你在 map 裡用 id 當 key 記錄某執行緒的狀態，該執行緒結束後
//   新執行緒可能拿到同一個 id，於是讀到前一個執行緒殘留的資料。
//   要長期識別請自己配一個遞增的邏輯編號（本檔的實務範例就是這樣做）。
//
// 【4. 為什麼本檔把「印出」搬到主執行緒】
//   原始寫法是在每條 worker 裡直接 std::cout <<。這會有兩個問題：
//     (a) 多條執行緒的輸出【交錯順序不定】，每次執行結果都不一樣；
//     (b) 一個 cout << a << b << c 由多個獨立呼叫組成，
//         中間可能被別的執行緒插入，出現「半行混在一起」的畫面。
//   本檔改成：每條 worker 只把自己的資訊寫進【專屬的 vector 槽位】
//   （各執行緒寫不同索引，彼此不重疊，不需要鎖），
//   全部 join() 之後再由主執行緒依序印出——輸出就穩定可驗證了。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼各寫各的 vector 索引不需要鎖？
//     std::vector<T> 的不同元素是不同的物件，
//     只要 (1) 事先配置好、過程中不再改變大小（不會重新配置記憶體），
//     (2) 每條執行緒只碰自己那一格，就不存在對同一記憶體位置的
//     「至少一個是寫入」的並行存取，也就不是 data race。
//     ⚠️ 例外：std::vector<bool> 是位元壓縮特化，相鄰元素共用同一個 byte，
//     並行寫不同索引【會】造成 data race，必須改用 std::vector<char> 或加鎖。
//   * threads.emplace_back(lambda) 直接在容器內建構 std::thread，
//     避免先建再 move。std::thread 不可複製、只能移動。
//   * 迴圈 join()：每個 std::thread 都必須在解構前 join() 或 detach()，
//     否則解構函式會呼叫 std::terminate()。
//
// 【注意事項 Pay Attention】
//   1. std::thread::id 的輸出格式是【實作定義】的，不可寫死在測試裡比對。
//   2. 執行緒結束後 id【可被重用】，不可當作永久識別碼。
//   3. 不要在多條執行緒裡無保護地直接 cout，輸出會交錯。
//   4. hardware_concurrency() 可能回傳 0，本檔照慣例 fallback 到 2。
//   5. lambda 用 [i] 【值捕獲】迴圈變數；若寫成 [&i] 會捕獲到已離開作用域
//      的迴圈變數的參考 → dangling reference，是典型的 UB。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread::id 與依核心數建立執行緒
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread::id 可以直接轉成整數當識別碼嗎？
//     答：不行。它是不透明型別，標準沒規定底層表示，也沒提供轉整數的介面。
//         要當雜湊表 key 就直接用它（有 operator< 與 std::hash 特化）；
//         真的需要一個數字，用 std::hash<std::thread::id>{}(id)。
//     追問：那 cout << id 印出來的數字是什麼？→ 實作細節（libstdc++ 印
//           底層 pthread_t），格式與數值都不可依賴，不同平台完全不同。
//
// 🔥 Q2. 執行緒 A 結束後，新建立的執行緒 B 有沒有可能拿到跟 A 一樣的 id？
//     答：有。標準明確允許已結束執行緒的 id 被重複使用。
//         因此拿 thread::id 當「長期物件識別」是設計錯誤，
//         應該自己配一個單調遞增的邏輯 ID。
//     追問：那用 id 當 map key 記錄「目前正在執行的執行緒」的狀態可以嗎？
//           → 可以，但必須在執行緒結束時把該 entry 移除，
//             否則下一個重用同 id 的執行緒會讀到殘留資料。
//
// ⚠️ 陷阱. 16 條執行緒各自 std::cout << "執行緒 " << i << "\n";
//        為什麼有時會看到兩行文字疊在一起？
//     答：std::cout 本身有保證不會資料損毀（C++11 起對同步化的標準串流有
//         保護），但「一連串的 << 呼叫」不是一個不可分割的整體。
//         執行緒 A 印完 "執行緒 " 之後，B 完全可以插進來印它那段。
//     為什麼會錯：多數人把「cout 是 thread-safe」理解成「整行輸出是原子的」。
//         thread-safe 指的是「不會壞掉」，不是「不會交錯」。
//         要整行不被插斷，得自己加鎖，或把整行組成單一字串再一次輸出。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔主題是「查詢環境資訊 + 依核心數開執行緒」。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）固定使用
//   評測框架給定的執行緒數與呼叫順序，考的是【順序同步】，
//   跟「依硬體核心數決定開幾條」毫無關係。
//   相關的 yield / 同步解法安排在同課的其他檔案示範。

#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】為每條 worker 配一個穩定的「邏輯執行緒編號」，用於 log 前綴
//   情境：伺服器的 access log 需要標出是哪條 worker 處理的請求。
//         直接印 std::thread::id 有三個問題——
//           (1) 格式冗長（本機是十幾位數字），洗版；
//           (2) 跨平台格式不同，log 分析腳本難寫；
//           (3) 執行緒結束後 id 會被重用，事後追查會張冠李戴。
//         正解：建立執行緒時就發一個單調遞增的邏輯編號，log 只印這個編號。
// -----------------------------------------------------------------------------
struct WorkerRecord {
    unsigned int    logicalId = 0;   // 我們自己配的穩定編號
    std::thread::id systemId;        // 執行期給的 id（會被重用）
    std::string     logLine;         // 該 worker 產生的一行 log
};

std::string formatLogLine(unsigned int logicalId, const std::string& message) {
    std::ostringstream oss;
    oss << "[worker-" << logicalId << "] " << message;
    return oss.str();
}

int main() {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;   // 標準允許回傳 0，必須兜底

    std::cout << "=== 建立執行緒 ===" << std::endl;
    std::cout << "建立 " << numThreads << " 個執行緒" << std::endl;

    // 事先配置好槽位：之後不再改變 vector 大小，
    // 每條執行緒只寫自己那一格 → 沒有並行寫同一位置，不需要鎖。
    std::vector<WorkerRecord> records(numThreads);

    // ⚠️ 這個 barrier 是【必要】的，不是裝飾：
    //    若不擋住，先建立的執行緒可能在後面的還沒建立前就結束，
    //    它的 std::thread::id 會被【重複使用】給後來的執行緒，
    //    那麼下面「兩兩相異」的檢查就【不再確定】。
    //    讓所有執行緒都到齊後才取 id，才能保證它們同時存活 → id 必然互異。
    std::atomic<unsigned int> arrived{0};

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; ++i) {
        // [i, &records, &arrived, numThreads]：i 用【值捕獲】
        //（迴圈變數會消失，不可用參考捕獲，否則是 dangling reference）
        threads.emplace_back([i, &records, &arrived, numThreads]() {
            arrived.fetch_add(1, std::memory_order_acq_rel);
            while (arrived.load(std::memory_order_acquire) < numThreads) {
                std::this_thread::yield();   // 等其他執行緒到齊
            }

            records[i].logicalId = i;
            records[i].systemId  = std::this_thread::get_id();
            records[i].logLine   = formatLogLine(i, "handled request /api/health");
        });
    }

    for (auto& t : threads) {
        t.join();   // 每個 thread 都必須 join 或 detach，否則解構時 terminate
    }

    // 全部 join 完才輸出：順序固定，不會交錯
    std::cout << "\n=== 日常實務: 以邏輯編號輸出 log ===" << std::endl;
    for (const auto& r : records) {
        std::cout << r.logLine << std::endl;
    }

    // ── 驗證 std::thread::id 的性質（這些檢查結果是確定的） ──
    std::cout << "\n=== std::thread::id 性質驗證 ===" << std::endl;

    bool allDistinct = true;
    for (std::size_t a = 0; a < records.size() && allDistinct; ++a) {
        for (std::size_t b = a + 1; b < records.size(); ++b) {
            if (records[a].systemId == records[b].systemId) {
                allDistinct = false;
                break;
            }
        }
    }
    std::cout << "同時存活的 " << records.size()
              << " 條執行緒 id 兩兩相異: " << (allDistinct ? "是" : "否") << std::endl;

    // 主執行緒的 id 不會等於任何一條仍存活過的 worker 的 id
    const std::thread::id mainId = std::this_thread::get_id();
    bool mainDiffers = true;
    for (const auto& r : records) {
        if (r.systemId == mainId) { mainDiffers = false; break; }
    }
    std::cout << "主執行緒 id 與所有 worker 相異: " << (mainDiffers ? "是" : "否") << std::endl;

    // 預設建構的 id 代表「不指涉任何執行緒」
    std::thread empty;
    std::cout << "預設建構的 std::thread，get_id() == std::thread::id{}: "
              << (empty.get_id() == std::thread::id{} ? "是" : "否") << std::endl;
    std::cout << "預設建構的 std::thread，joinable(): "
              << (empty.joinable() ? "是" : "否") << std::endl;

    // 注意：實際的 id 數值【每次執行都不同】，且格式是實作定義的，
    // 因此這裡刻意不把它印出來當作預期輸出的一部分。

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 2.6：執行緒識別與資訊4.cpp' -o thread_id_pool

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 執行緒條數是【實作定義】的：本機 hardware_concurrency() = 16，
//      所以是 worker-0 ~ worker-15。換機器行數會不同。
//   2. 實際的 std::thread::id 數值每次執行都不同、格式也因平台而異，
//      本檔【刻意不印出】，只印可驗證的性質（是否兩兩相異）。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 建立執行緒 ===
// 建立 16 個執行緒
//
// === 日常實務: 以邏輯編號輸出 log ===
// [worker-0] handled request /api/health
// [worker-1] handled request /api/health
// [worker-2] handled request /api/health
// [worker-3] handled request /api/health
// [worker-4] handled request /api/health
// [worker-5] handled request /api/health
// [worker-6] handled request /api/health
// [worker-7] handled request /api/health
// [worker-8] handled request /api/health
// [worker-9] handled request /api/health
// [worker-10] handled request /api/health
// [worker-11] handled request /api/health
// [worker-12] handled request /api/health
// [worker-13] handled request /api/health
// [worker-14] handled request /api/health
// [worker-15] handled request /api/health
//
// === std::thread::id 性質驗證 ===
// 同時存活的 16 條執行緒 id 兩兩相異: 是
// 主執行緒 id 與所有 worker 相異: 是
// 預設建構的 std::thread，get_id() == std::thread::id{}: 是
// 預設建構的 std::thread，joinable(): 否
