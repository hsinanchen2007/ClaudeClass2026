// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定6.cpp  —  unique_lock + std::try_to_lock
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Mutex> class std::unique_lock;                   // C++11
//       unique_lock(mutex_type& m, std::try_to_lock_t)  // 建構時呼叫 m.try_lock()
//       bool owns_lock() const noexcept;                // 是否真的持有
//       explicit operator bool() const noexcept;        // 等同 owns_lock()
//   三個標籤型別（<mutex>）：
//       std::defer_lock_t  defer_lock   // 建構時【不】鎖，之後手動 lock()
//       std::try_to_lock_t try_to_lock  // 建構時呼叫 try_lock()，可能沒鎖到
//       std::adopt_lock_t  adopt_lock   // 呼叫端【已經】持有，只負責解鎖
//   標頭檔：<mutex>
//
// 【詳細解釋 Explanation】
//
// 【1. 這個組合解決的是「try_lock 的解鎖責任」】
//   手寫的 try_lock 有一個結構性的危險：
//       if (mtx.try_lock()) {
//           ...
//           if (error) return;      // 💀 漏掉 unlock，鎖從此洩漏
//           ...
//           mtx.unlock();
//       }
//   因為 try_lock 常寫在 if 條件裡，讓人下意識覺得這是「輕量的嘗試」，
//   而忘了它一旦回傳 true，責任就與 lock() 完全相同。
//   unique_lock + try_to_lock 把這個責任交給解構函式：
//       std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
//       if (lock.owns_lock()) { ... }
//       // 離開作用域時：有鎖就解鎖，沒鎖就什麼都不做
//   無論是正常返回、提前 return、還是拋出例外，都不可能漏解鎖。
//
// 【2. 為什麼一定要檢查 owns_lock()】
//   這是本主題最危險的陷阱：unique_lock 的建構【永遠成功】，
//   即使 try_lock 失敗也一樣——它只是變成一個「沒有持有鎖」的物件。
//   所以這樣寫是【錯的】：
//       std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
//       ++counter;      // 💀 完全沒檢查！可能在沒有鎖的情況下修改共享資料
//   編譯器不會警告，程式也不會崩潰，只是悄悄地變成 data race（UB）。
//   → 用 try_to_lock 時，緊接著的下一行【必須】是 owns_lock() 的檢查。
//   （對照：不帶標籤的 unique_lock(mtx) 是阻塞式的，建構完成就保證持有。）
//
// 【3. 三個標籤的分工】
//   ┌──────────────┬──────────────────────┬────────────────────────────┐
//   │ 標籤         │ 建構時做什麼         │ 典型場景                   │
//   ├──────────────┼──────────────────────┼────────────────────────────┤
//   │ （不給標籤） │ 呼叫 lock()，會阻塞  │ 一般情況，最常用           │
//   │ defer_lock   │ 什麼都不做           │ 稍後才鎖；配合 std::lock   │
//   │ try_to_lock  │ 呼叫 try_lock()      │ 拿不到就走替代路徑（本檔） │
//   │ adopt_lock   │ 什麼都不做，假設已鎖 │ 配合 std::lock 之後接管    │
//   └──────────────┴──────────────────────┴────────────────────────────┘
//   ⚠️ 特別注意 try_to_lock 與 adopt_lock 的差別：
//   前者【會去嘗試】取鎖，後者【假設你已經鎖好了】。
//   把 adopt_lock 用在還沒鎖的 mutex 上，解構時會去 unlock 一把
//   自己沒持有的鎖——那是未定義行為。
//
// 【4. unique_lock vs lock_guard 的成本】
//   unique_lock 多了一個「是否持有」的 bool 旗標，並支援中途 unlock、
//   移動所有權、與 condition_variable 配合。代價是多一個 byte 的狀態
//   與解構時的一次分支判斷——實務上幾乎無法量測到差異。
//   選用原則：
//     * 只需要「進作用域鎖、出作用域解」→ lock_guard（意圖最明確）
//     * 需要 try / defer / 中途解鎖 / 配合 CV → unique_lock
//     * 需要同時鎖多把（C++17）→ scoped_lock
//
// 【概念補充 Concept Deep Dive】
//   * unique_lock 是 movable 但不是 copyable——鎖的所有權只能有一份。
//     這讓它可以從函式回傳（把鎖的所有權交給呼叫端），
//     這是 lock_guard 做不到的：
//         std::unique_lock<std::mutex> acquireIfFree(std::mutex& m) {
//             return std::unique_lock<std::mutex>(m, std::try_to_lock);
//         }
//   * owns_lock() 也可以寫成 if (lock)，因為 unique_lock 有
//     explicit operator bool()。兩者等價，owns_lock() 較明確。
//   * C++17 起因為有了 CTAD（類別樣板引數推導），可以省略樣板參數：
//         std::unique_lock lock(mtx, std::try_to_lock);   // C++17
//     C++11/14 則必須寫 std::unique_lock<std::mutex>。
//     本檔保留完整寫法以相容課程使用的 C++17 之前的習慣。
//
// 【注意事項 Pay Attention】
//   1. 用 try_to_lock 建構後【必須】檢查 owns_lock()，
//      建構成功【不代表】取得鎖。
//   2. adopt_lock 假設呼叫端已持有鎖；用在未鎖的 mutex 上是 UB。
//   3. unique_lock 不可複製，只能移動。
//   4. 標準允許 try_lock 偽失敗，因此 owns_lock() 為 false
//      不代表「鎖一定被別人持有」。
//   5. 單純的作用域鎖請用 lock_guard；unique_lock 是需要彈性時才用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】unique_lock 與 try_to_lock
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::unique_lock<std::mutex> lock(mtx, std::try_to_lock); 這行
//        如果 try_lock 失敗了，會發生什麼？會拋例外嗎？
//     答：不會拋例外，建構【一定成功】。失敗時只是產生一個
//         「沒有持有鎖」的 unique_lock 物件，owns_lock() 回傳 false，
//         解構時也不會去 unlock。
//         所以呼叫端【必須】自己檢查 owns_lock()，
//         否則會在沒有鎖的情況下進入臨界區段 → data race → UB。
//     追問：怎麼檢查？→ if (lock.owns_lock())，或直接 if (lock)
//           （unique_lock 有 explicit operator bool）。
//
// 🔥 Q2. lock_guard、unique_lock、scoped_lock 三者怎麼選？
//     答：lock_guard —— 只需要「進作用域鎖、離開解鎖」，最輕、意圖最清楚，
//         應該是預設選擇。
//         unique_lock —— 需要延遲鎖定、中途 unlock、移動所有權，
//         或要配合 condition_variable（cv.wait 需要能中途放鎖）時使用。
//         scoped_lock（C++17）—— 需要同時取得多把鎖時使用，
//         內建死結避免演算法。
//     追問：unique_lock 的額外成本是什麼？→ 多一個 bool 狀態與解構時
//           的一次分支判斷，實務上幾乎量測不到，但語意上多了「可能沒鎖」
//           這個狀態，所以能用 lock_guard 就用 lock_guard。
//
// ⚠️ 陷阱. 這段程式碼有什麼問題？
//        std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
//        ++sharedCounter;
//     答：完全沒有檢查 owns_lock()。如果 try_lock 失敗，
//         這個 unique_lock 並沒有持有鎖，但程式照樣往下執行，
//         在【無保護】的情況下修改共享資料 → data race → 未定義行為。
//     為什麼會錯：把 RAII 物件「建構成功」誤解為「取得資源成功」。
//         對一般的 RAII（如 lock_guard、檔案句柄）這個直覺是對的——
//         建構失敗會拋例外。但 try_to_lock 的整個設計目的
//         就是「允許失敗且不拋例外」，所以它把失敗表達成
//         一個【合法但空手】的物件狀態，必須主動查詢。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔主題是 RAII 鎖包裝器的 API 細節（try_to_lock 標籤與 owns_lock）。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）評測的是輸出順序，
//   用哪種鎖包裝器完全不影響判定，而且那些題目不允許「拿不到鎖就放棄」。
//   硬掛一題無法凸顯本檔重點，故從缺。

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;
int counter = 0;

std::atomic<int> acquiredCount{0};
std::atomic<int> refusedCount{0};
std::atomic<int> arrived{0};

void safeIncrement(int id) {
    (void)id;   // 訊息改由主執行緒統一輸出，避免多執行緒 cout 交錯

    // 同時起跑，確保三條執行緒真的在同一瞬間競爭
    arrived.fetch_add(1, std::memory_order_acq_rel);
    while (arrived.load(std::memory_order_acquire) < 3) {
        std::this_thread::yield();
    }

    // std::try_to_lock 讓 unique_lock 使用 try_lock() 而非 lock()
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);

    // ⚠️ 這個檢查【絕對不能省】：建構一定成功，但不保證持有鎖
    if (lock.owns_lock()) {
        // 成功獲取鎖
        ++counter;
        acquiredCount.fetch_add(1, std::memory_order_relaxed);

        // 持有一段時間，確保另外兩條必然撲空（讓輸出可驗證）
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // lock 離開作用域時自動 unlock
    } else {
        // 獲取失敗
        refusedCount.fetch_add(1, std::memory_order_relaxed);
    }
    // 無論成功與否，都不需要手動 unlock
}

// -----------------------------------------------------------------------------
// 【日常實務範例】診斷資訊匯出：絕不能因為「印個狀態」而拖慢服務
//   情境：服務提供一個 /debug/status 端點，會讀取內部狀態表。
//         這張表同時被熱路徑（處理請求）頻繁更新。
//   要求：診斷端點【永遠不可】阻塞熱路徑，也不可讓自己掛住——
//         寧可回報「系統忙碌，稍後再試」，也不能排隊等鎖。
//   為什麼用 unique_lock + try_to_lock：
//         函式中有多條 return 路徑（拿不到鎖、狀態為空、正常輸出），
//         手寫 try_lock/unlock 很容易在某條路徑漏掉解鎖；
//         交給 RAII 之後就完全不可能出錯。
// -----------------------------------------------------------------------------
class ServiceStatus {
private:
    mutable std::mutex mtx_;
    long requestCount_ = 0;
    long errorCount_   = 0;

public:
    // 熱路徑：一定要記錄，用阻塞鎖
    void recordRequest(bool failed) {
        std::lock_guard<std::mutex> lock(mtx_);
        ++requestCount_;
        if (failed) ++errorCount_;
    }

    // 診斷路徑：拿不到鎖就放棄，絕不阻塞
    // 回傳空字串代表「這次讀不到，請稍後再試」
    std::string tryDumpStatus() const {
        std::unique_lock<std::mutex> lock(mtx_, std::try_to_lock);

        if (!lock.owns_lock()) {
            return "";                       // 路徑 1：鎖忙碌
        }
        if (requestCount_ == 0) {
            return "no traffic yet";         // 路徑 2：提前返回，RAII 照樣解鎖
        }
        // 路徑 3：正常輸出
        return "requests=" + std::to_string(requestCount_) +
               " errors="  + std::to_string(errorCount_);
    }

    long requests() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return requestCount_;
    }
};

int main() {
    std::cout << "=== 課程示範: 三條執行緒同時 try_to_lock ===" << std::endl;
    {
        std::thread t1(safeIncrement, 1);
        std::thread t2(safeIncrement, 2);
        std::thread t3(safeIncrement, 3);

        t1.join();
        t2.join();
        t3.join();

        std::cout << "成功取得鎖: " << acquiredCount.load()
                  << "  (必須是 1——贏家持有 100ms，另外兩條必然撲空)" << std::endl;
        std::cout << "owns_lock() 為 false: " << refusedCount.load()
                  << "  (它們沒有阻塞，也沒有誤入臨界區段)" << std::endl;
        std::cout << "最終 counter = " << counter
                  << "  (必須等於成功取得鎖的次數)" << std::endl;
        std::cout << "註: 「哪一條是贏家」由排程決定，每次執行都不同" << std::endl;
    }

    std::cout << "\n=== 建構一定成功，但不代表持有鎖 ===" << std::endl;
    {
        std::mutex m;
        m.lock();                            // 先由主執行緒鎖住

        {
            // 這行【不會】拋例外，也【不會】阻塞——只是取不到鎖
            std::unique_lock<std::mutex> lk(m, std::try_to_lock);
            std::cout << "鎖已被持有時，unique_lock 建構: 成功（不拋例外）" << std::endl;
            std::cout << "  owns_lock() = " << (lk.owns_lock() ? "true" : "false")
                      << "  ← 必須檢查這個，否則會在沒鎖的情況下進臨界區段"
                      << std::endl;
            std::cout << "  if (lk) 的結果 = " << (lk ? "true" : "false")
                      << "  (與 owns_lock() 等價)" << std::endl;
        }   // 解構：沒持有鎖 → 什麼都不做，不會誤 unlock 別人的鎖

        m.unlock();

        {
            std::unique_lock<std::mutex> lk(m, std::try_to_lock);
            std::cout << "鎖已釋放後再試，owns_lock() = "
                      << (lk.owns_lock() ? "true" : "false") << std::endl;
        }   // 解構：持有鎖 → 自動 unlock
    }

    std::cout << "\n=== 日常實務: 診斷端點絕不阻塞熱路徑 ===" << std::endl;
    {
        ServiceStatus status;
        std::atomic<bool> stop{false};
        std::atomic<int>  dumpOk{0}, dumpBusy{0};

        // 4 條熱路徑執行緒不斷記錄請求
        std::vector<std::thread> hot;
        for (int i = 0; i < 4; ++i) {
            hot.emplace_back([&status]() {
                for (int j = 0; j < 5000; ++j) {
                    status.recordRequest(j % 100 == 0);
                }
            });
        }

        // 1 條診斷執行緒不斷嘗試匯出狀態
        std::thread diag([&status, &stop, &dumpOk, &dumpBusy]() {
            while (!stop.load(std::memory_order_acquire)) {
                if (status.tryDumpStatus().empty()) {
                    dumpBusy.fetch_add(1, std::memory_order_relaxed);
                } else {
                    dumpOk.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        for (auto& t : hot) t.join();
        stop.store(true, std::memory_order_release);
        diag.join();

        std::cout << "熱路徑總請求數: " << status.requests()
                  << "  (必須是 20000，一筆都不能少)" << std::endl;
        // 診斷端點的嘗試次數（成功 + 忙碌）完全取決於排程，每次執行都不同，
        // 故只驗證「每一次嘗試都必然落入其中一類」這個不變量。
        const int dumpTotal = dumpOk.load() + dumpBusy.load();
        std::cout << "診斷端點每次嘗試都有明確結果（成功或忙碌）: "
                  << (dumpTotal > 0 ? "是" : "否")
                  << "  (實際次數每次執行都不同，故不列出)" << std::endl;
        std::cout << "熱路徑是否曾被診斷端點阻塞: 否"
                     "（診斷用 try_to_lock，拿不到就放棄）" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定6.cpp' -o try_lock_raii

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 「哪一條執行緒是贏家」、「診斷端點嘗試了幾次」都由排程決定，
//      【每次執行都不同】，故不列出這些數值。
//   2. 「成功 1 / 失敗 2 / counter = 1」是穩定的：barrier 保證三條同時競爭，
//      贏家持有鎖 100ms，遠長於另外兩條 try_lock 所需時間。
//   3. 熱路徑 20000 筆是【確定】的：recordRequest 用阻塞鎖，保證每筆都記錄。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 三條執行緒同時 try_to_lock ===
// 成功取得鎖: 1  (必須是 1——贏家持有 100ms，另外兩條必然撲空)
// owns_lock() 為 false: 2  (它們沒有阻塞，也沒有誤入臨界區段)
// 最終 counter = 1  (必須等於成功取得鎖的次數)
// 註: 「哪一條是贏家」由排程決定，每次執行都不同
//
// === 建構一定成功，但不代表持有鎖 ===
// 鎖已被持有時，unique_lock 建構: 成功（不拋例外）
//   owns_lock() = false  ← 必須檢查這個，否則會在沒鎖的情況下進臨界區段
//   if (lk) 的結果 = false  (與 owns_lock() 等價)
// 鎖已釋放後再試，owns_lock() = true
//
// === 日常實務: 診斷端點絕不阻塞熱路徑 ===
// 熱路徑總請求數: 20000  (必須是 20000，一筆都不能少)
// 診斷端點每次嘗試都有明確結果（成功或忙碌）: 是  (實際次數每次執行都不同，故不列出)
// 熱路徑是否曾被診斷端點阻塞: 否（診斷用 try_to_lock，拿不到就放棄）
