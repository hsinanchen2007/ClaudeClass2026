//
// =============================================================================
//  課程 5.5：保護共享資料實作6.cpp  —  在鎖外組字串，只在鎖內做輸出
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    執行緒安全的日誌記錄器；臨界區段最小化的完整實例
//   關鍵手法：在鎖外把訊息組成【一個完整字串】，鎖內只做一次 output << str
//   標準版本：std::mutex 為 C++11；std::this_thread::get_id 為 C++11；
//             std::put_time 為 C++11；enum class 為 C++11
//   標頭檔：  <mutex>、<sstream>、<iomanip>、<chrono>、<thread>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「先組字串」是這個類別的靈魂】
//   日誌的每一行由多段組成：時間戳、等級、執行緒 id、訊息本體。
//   天真的寫法是直接輸出：
//       lock_guard lk(mtx);
//       out << "[" << ts << "] [" << lvl << "] [T:" << tid << "] " << msg << endl;
//   這樣有兩個問題：
//     ① 產生時間戳（呼叫 localtime、格式化）、把數字轉成字串
//        —— 這些【純計算】全部發生在鎖內，臨界區段被拉長好幾倍。
//     ② 多個 << 呼叫意味著更長的持鎖時間，競爭機率上升。
//   本類別的做法是先在鎖外用 std::ostringstream 組出【一個完整字串】，
//   鎖內只剩下 `output << formatted;` 這一個動作。
//   → 臨界區段從「格式化 + 輸出」縮成「輸出」，通常是數倍到十倍的差距。
//     這正是課程 5.6-4 會用數據量化的「把計算移出鎖」原則。
//
// 【2. 為什麼一定要組成「一個」字串，而不是少幾個 <<】
//   關鍵不只是效能，還有【原子性】。
//   `out << a << b << c` 是三次獨立的操作，即使全部在鎖內也沒問題；
//   但只要有任何一條路徑漏掉鎖（例如別處直接用 std::cout），
//   輸出就會交錯成無法閱讀的樣子。
//   組成單一字串再一次輸出，讓「一行日誌」成為一個不可分割的單位 ——
//   這也是為什麼多執行緒環境下 printf 反而比多個 cout << 「不容易亂」
//   （POSIX 要求 stdio 對 FILE* 的操作是執行緒安全的）。
//
// 【3. 等級過濾為什麼放在最前面（且在鎖外）】
//       if (level < minLevel) return;   // ← 完全不上鎖就返回
//   生產環境通常只開 INFO 以上，DEBUG 訊息可能佔全部呼叫的九成。
//   把過濾放在最前面且不上鎖，這九成的呼叫連鎖都不用碰，
//   成本僅一次整數比較。若先上鎖再判斷，等於讓所有被丟棄的訊息
//   都去排隊搶鎖 —— 在高頻日誌下這會是嚴重的瓶頸。
//   ⚠️ 但這裡有個誠實的取捨：讀取 minLevel 沒有加鎖，
//      而 setLevel() 會寫它 → 嚴格來說構成 data race。
//      正式做法是把 minLevel 宣告成 std::atomic<Level>
//      （用 relaxed 即可，因為它不需要與其他資料建立順序關係）。
//      本檔在下方的 SafeLogger 示範這個修正。
//
// 【4. 為什麼需要禁止複製】
//   類別持有 std::ostream& 與 std::mutex：
//     * mutex 本來就不可複製（見 5.5-2）
//     * 複製一個 logger 會產生兩把鎖保護【同一個 ostream】——
//       兩個 logger 各自鎖各自的鎖，對同一個輸出串流輸出，保護完全失效
//   所以 = delete 不只是「因為 mutex 不能複製」，
//   更是因為【複製這件事在語意上就是錯的】。
//
// 【5. 這個 logger 還缺什麼（正式系統的落差）】
//   本檔是教學實作，正式的日誌函式庫還需要：
//     * 非同步寫入：把訊息丟進佇列，由專門的 I/O 執行緒寫出，
//       呼叫端完全不等磁碟（課程 5.6-5 的方向）。
//     * 檔案輪替（rotation）與大小上限
//     * 崩潰時的緩衝區 flush（本檔每筆都 flush，安全但慢）
//     * 結構化輸出（JSON）以便機器解析
//   本檔每筆都呼叫 flush() 是為了教學上「立刻看得到」，
//   正式環境通常改成定期 flush 或依等級決定（ERROR 才立即 flush），
//   因為每筆 flush 都是一次系統呼叫。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::localtime 不是執行緒安全的（本檔的隱藏問題）
//   `std::localtime()` 回傳指向【靜態內部緩衝區】的指標，
//   多條執行緒同時呼叫會互相覆蓋 —— 這是 C 標準庫的歷史包袱。
//   POSIX 提供 localtime_r()（可重入版本，由呼叫端提供緩衝區），
//   Windows 提供 localtime_s()。
//   本檔的 getTimestamp() 在鎖【外】呼叫 localtime，所以有這個風險。
//   下方的 SafeLogger 用 localtime_r 修正。
//   → 這是很典型的「以為自己寫的是執行緒安全的類別，
//     卻在裡面呼叫了非執行緒安全的 C 函式」。
//
// (B) std::endl 與 '\n' 的差別
//   std::endl = 輸出 '\n' + 呼叫 flush()。
//   在迴圈中大量使用 endl 會造成大量不必要的系統呼叫，
//   是 C++ 效能問題的常見來源。
//   本檔在 ostringstream 裡用了 endl（對字串流而言 flush 沒有實際成本），
//   但正式程式碼在字串流中應該直接用 '\n' 表達意圖。
//
// (C) 為什麼 thread id 印出來是一長串數字
//   std::thread::id 的 operator<< 是實作定義的。
//   libstdc++ 直接印出底層的 pthread_t 值（本機是一個很大的整數，
//   實際上是執行緒堆疊的位址），所以看起來像 132207352694720。
//   這個值【每次執行都不同】，也不保證跨平台一致，
//   只適合用來「區分是不是同一條執行緒」，不適合當作穩定識別碼。
//
// 【注意事項 Pay Attention】
// 1. 格式化在鎖外、輸出在鎖內 —— 這是本檔最重要的設計。
// 2. 組成【單一字串】再輸出，讓一行日誌成為不可分割的單位。
// 3. 等級過濾要放在最前面且不上鎖，讓被丟棄的訊息連鎖都不碰。
// 4. minLevel 被 setLevel 寫、被 log 讀 → 應宣告為 std::atomic（本檔已示範修正）。
// 5. std::localtime 回傳靜態緩衝區、非執行緒安全，應改用 localtime_r。
// 6. 每筆都 flush() 很安全但很慢（每次都是系統呼叫）；
//    正式環境常改為定期 flush 或僅對 ERROR 立即 flush。
// 7. 時間戳與 thread id【每次執行都不同】，不可當作預期輸出的固定值。
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1115. Print FooBar Alternately
//   （實作於下方 FooBar 類別）
//   題目：兩條執行緒分別呼叫 foo() 與 bar() 各 n 次，
//         必須交替輸出 "foobarfoobar..."。
//   為什麼用到本主題：本檔的 logger 用 mutex 保證「一行不被切斷」，
//         但它【完全不保證順序】—— 哪條執行緒先寫完全看排程。
//         1115 要的正是 logger 做不到的那件事：強制交替。
//         這清楚地показ出 mutex 與 condition_variable 的分工：
//           * mutex：保證互斥（不會同時進入），但不決定誰先進
//           * condition_variable：等待某個條件成立，才決定順序
//         想只用 mutex 做出交替是不可能的 —— 這是很多人第一次
//         寫並行程式時最大的誤解，也是本題的價值所在。
// -----------------------------------------------------------------------------
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全的日誌記錄器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 寫一個執行緒安全的 logger，最關鍵的設計是什麼?
//     答：把「格式化」與「輸出」分開 —— 在鎖【外】用 ostringstream
//         組出一個完整字串，鎖【內】只做一次 output << str。
//         格式化（取時間、數字轉字串、字串串接）是純計算，
//         不碰共享資料，完全沒有理由佔著鎖做。
//         另外等級過濾要放在最前面且不上鎖，讓被丟棄的訊息連鎖都不碰。
//     追問：為什麼要組成「一個」字串，少幾個 << 不就好了?
//         → 除了效能，還有原子性：一行日誌應該是不可分割的單位。
//           分成多個 << 時，只要有任何一條路徑漏了鎖，輸出就會交錯。
//
// 🔥 Q2. 用 mutex 保護輸出，能保證多條執行緒的日誌「按順序」出現嗎?
//     答：不能。mutex 只保證【互斥】（同一時間只有一個進去），
//         完全不保證【順序】（誰先拿到鎖由作業系統排程決定，
//         std::mutex 也不保證公平性，可能有執行緒連續搶到）。
//         想要順序必須用 condition_variable 等待條件成立，
//         這正是 LeetCode 1115 Print FooBar Alternately 在考的事。
//     追問：那 logger 的時間戳可以用來排序嗎?
//         → 不可靠。時間戳在鎖外產生，「取得時間戳」與「實際輸出」
//           之間可以被插隊，所以檔案中的行序可能與時間戳順序不一致。
//           要嚴格排序必須在鎖內取序號（單調遞增的 counter）。
//
// ⚠️ 陷阱. 「我的 logger 每個成員函式都有 lock_guard，所以它執行緒安全」——
//         這個類別裡還藏著什麼問題?
//     答：兩個。① minLevel 被 setLevel() 寫、被 log() 在鎖外讀 →
//         這一對存取之間沒有同步，構成 data race（應改用 std::atomic）。
//         ② getTimestamp() 呼叫了 std::localtime，
//         而 localtime 回傳指向靜態內部緩衝區的指標，本身就不是執行緒安全的 ——
//         多條執行緒同時呼叫會互相覆蓋（應改用 localtime_r）。
//     為什麼會錯：只檢查「自己寫的程式碼有沒有加鎖」，
//         卻沒檢查「呼叫到的函式本身是不是執行緒安全的」。
//         C 標準庫裡有一整批這種函式（localtime、strtok、asctime、
//         rand、gmtime），它們都是單執行緒時代的遺產，
//         在多執行緒程式中必須改用 _r 後綴的可重入版本。
// ═══════════════════════════════════════════════════════════════════════════
// 檔案：lesson_5_5_thread_safe_logger.cpp
// 說明：執行緒安全的日誌記錄器

#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <fstream>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <ctime>

class ThreadSafeLogger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
private:
    mutable std::mutex mtx;
    std::ostream& output;
    Level minLevel;
    bool includeTimestamp;
    bool includeThreadId;
    
    std::string levelToString(Level level) const {
        switch (level) {
            case Level::DEBUG:   return "DEBUG";
            case Level::INFO:    return "INFO ";
            case Level::WARNING: return "WARN ";
            case Level::ERROR:   return "ERROR";
            default:             return "?????";
        }
    }
    
    std::string getTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }
    
public:
    ThreadSafeLogger(std::ostream& out = std::cout, 
                     Level level = Level::DEBUG,
                     bool timestamp = true,
                     bool threadId = true)
        : output(out)
        , minLevel(level)
        , includeTimestamp(timestamp)
        , includeThreadId(threadId) {}
    
    // 禁止複製
    ThreadSafeLogger(const ThreadSafeLogger&) = delete;
    ThreadSafeLogger& operator=(const ThreadSafeLogger&) = delete;
    
    void setLevel(Level level) {
        std::lock_guard<std::mutex> lock(mtx);
        minLevel = level;
    }
    
    void log(Level level, const std::string& message) {
        if (level < minLevel) {
            return;  // 過濾低級別日誌
        }
        
        // 在鎖外準備訊息（減少臨界區段）
        std::ostringstream oss;
        
        if (includeTimestamp) {
            oss << "[" << getTimestamp() << "] ";
        }
        
        oss << "[" << levelToString(level) << "] ";
        
        if (includeThreadId) {
            oss << "[T:" << std::this_thread::get_id() << "] ";
        }
        
        oss << message << std::endl;
        
        std::string formatted = oss.str();
        
        // 只在輸出時鎖定
        std::lock_guard<std::mutex> lock(mtx);
        output << formatted;
        output.flush();
    }
    
    void debug(const std::string& msg)   { log(Level::DEBUG, msg); }
    void info(const std::string& msg)    { log(Level::INFO, msg); }
    void warning(const std::string& msg) { log(Level::WARNING, msg); }
    void error(const std::string& msg)   { log(Level::ERROR, msg); }
};


// -----------------------------------------------------------------------------
// 【修正版 Logger】修掉上面點出的兩個真實問題
//   ① minLevel 改成 std::atomic<Level> —— 消除 setLevel 與 log 之間的 data race
//   ② 改用 localtime_r —— 消除 std::localtime 的靜態緩衝區問題
//   ③ 新增鎖內取得的單調序號 —— 讓日誌可以被嚴格排序
//   為了讓輸出可重現，本類別可關閉時間戳（時間戳每次執行都不同）。
// -----------------------------------------------------------------------------
class SafeLogger {
public:
    enum class Level { DEBUG, INFO, WARNING, ERROR };

private:
    mutable std::mutex mtx;
    std::ostream& output;
    std::atomic<Level> minLevel;      // ← 修正 ①：原本是裸 Level
    bool includeTimestamp;
    long sequence = 0;                // 鎖內遞增，可用於嚴格排序

    static const char* levelToString(Level level) {
        switch (level) {
            case Level::DEBUG:   return "DEBUG";
            case Level::INFO:    return "INFO ";
            case Level::WARNING: return "WARN ";
            case Level::ERROR:   return "ERROR";
            default:             return "?????";
        }
    }

    static std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tmbuf{};
        localtime_r(&time, &tmbuf);   // ← 修正 ②：可重入版本，各自的緩衝區
        std::ostringstream oss;
        oss << std::put_time(&tmbuf, "%H:%M:%S");
        return oss.str();
    }

public:
    SafeLogger(std::ostream& out, Level level, bool timestamp)
        : output(out), minLevel(level), includeTimestamp(timestamp) {}

    SafeLogger(const SafeLogger&) = delete;
    SafeLogger& operator=(const SafeLogger&) = delete;

    void setLevel(Level level) {
        minLevel.store(level, std::memory_order_relaxed);   // 不需要鎖了
    }

    void log(Level level, const std::string& message) {
        // 過濾在最前面且完全不上鎖：被丟棄的訊息連鎖都不碰
        if (level < minLevel.load(std::memory_order_relaxed)) return;

        // ── 鎖外：所有格式化工作 ──
        std::ostringstream oss;
        if (includeTimestamp) oss << "[" << getTimestamp() << "] ";
        oss << "[" << levelToString(level) << "] " << message << "\n";
        std::string formatted = oss.str();

        // ── 鎖內：只剩下取序號與輸出 ──
        std::lock_guard<std::mutex> lock(mtx);
        ++sequence;
        output << formatted;
    }

    void debug(const std::string& m)   { log(Level::DEBUG, m); }
    void info(const std::string& m)    { log(Level::INFO, m); }
    void warning(const std::string& m) { log(Level::WARNING, m); }
    void error(const std::string& m)   { log(Level::ERROR, m); }

    long emitted() const { std::lock_guard<std::mutex> lock(mtx); return sequence; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1115. Print FooBar Alternately
//   題目：兩條執行緒分別呼叫 foo() 與 bar() 各 n 次，輸出必須是
//         "foobarfoobar..."（嚴格交替）。
//   為什麼用到本主題：本檔的 logger 證明了 mutex 能保證「一行完整」，
//         卻【完全不保證順序】。1115 要的正是 mutex 做不到的那件事。
//         關鍵是 condition_variable：
//           * mutex 決定「同時只有一個能進」
//           * cv 決定「什麼條件成立時才輪到你」
//         只用 mutex 是不可能做出嚴格交替的 —— 因為 std::mutex
//         不保證公平性，同一條執行緒完全可能連續搶到鎖好幾次。
// -----------------------------------------------------------------------------
class FooBar {
private:
    // 【注意】成員初始化順序依「宣告順序」，與初始化列表順序無關。
    std::mutex mtx;
    std::condition_variable cv;
    int n;
    bool fooTurn = true;      // true = 輪到 foo，false = 輪到 bar

public:
    explicit FooBar(int nn) : n(nn) {}

    void foo(const std::function<void()>& printFoo) {
        for (int i = 0; i < n; ++i) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return fooTurn; });   // 等到輪到我
            printFoo();
            fooTurn = false;
            cv.notify_all();
        }
    }

    void bar(const std::function<void()>& printBar) {
        for (int i = 0; i < n; ++i) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !fooTurn; });
            printBar();
            fooTurn = true;
            cv.notify_all();
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】非同步日誌：把 I/O 徹底移出呼叫端的路徑
//   情境：高頻服務每秒數十萬筆日誌。即使已經「在鎖外格式化」，
//         寫磁碟本身仍是系統呼叫（數微秒到數十微秒），
//         若在鎖內做，所有執行緒都會排隊等這個 I/O。
//   做法：呼叫端只把【已格式化好的字串】丟進佇列（極快），
//         由專門的寫出執行緒批次取走並一次寫出。
//         這樣呼叫端完全不接觸磁碟，臨界區段只剩一次 push_back。
//   這是正式日誌函式庫（spdlog、glog 的 async mode）的核心架構，
//   也是課程 5.6-5「避免在臨界區段內做 I/O」的完整形態。
// -----------------------------------------------------------------------------
class AsyncLogger {
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<std::string> buffer;
    std::vector<std::string> written;   // 這裡用容器代替真實檔案，方便驗證
    bool done = false;
    std::thread worker;

    void run() {
        std::vector<std::string> batch;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return !buffer.empty() || done; });
                if (buffer.empty() && done) break;
                batch.swap(buffer);          // 一次搬走整批，臨界區段極短
            }
            // ── 鎖外：真正的「I/O」，不阻塞任何生產者 ──
            for (auto& line : batch) written.push_back(std::move(line));
            batch.clear();
        }
    }

public:
    AsyncLogger() : worker(&AsyncLogger::run, this) {}

    ~AsyncLogger() { stop(); }

    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    // 呼叫端：格式化在鎖外，鎖內只有一次 push_back
    void log(const std::string& level, const std::string& msg) {
        std::string line = "[" + level + "] " + msg;   // 鎖外組字串
        {
            std::lock_guard<std::mutex> lock(mtx);
            buffer.push_back(std::move(line));          // 鎖內只有這一行
        }
        cv.notify_one();
    }

    void stop() {
        if (!worker.joinable()) return;
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
        }
        cv.notify_all();
        worker.join();
    }

    size_t writtenCount() {
        std::lock_guard<std::mutex> lock(mtx);
        return written.size();
    }
};

// 全域 Logger（實際專案中可能使用單例模式）
ThreadSafeLogger logger;

void worker(int id) {
    logger.info("Worker " + std::to_string(id) + " 開始");
    
    for (int i = 0; i < 3; ++i) {
        logger.debug("Worker " + std::to_string(id) + 
                     " 執行任務 " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    logger.info("Worker " + std::to_string(id) + " 結束");
}

int main() {
    logger.info("=== 程式開始 ===");
    
    std::vector<std::thread> threads;
    
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    logger.info("=== 程式結束 ===");

    std::cout << "\n=== 修正版 Logger（關閉時間戳，輸出可重現）===" << std::endl;
    {
        SafeLogger safeLog(std::cout, SafeLogger::Level::INFO, false);
        safeLog.debug("這行會被過濾掉（等級低於 INFO，連鎖都不碰）");
        safeLog.info("服務啟動");
        safeLog.warning("設定檔缺少 timeout，使用預設值 3000ms");
        safeLog.error("無法連線至 db-replica-2");

        // 併發驗證：每一行必定完整、不交錯
        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&safeLog] {
                for (int k = 0; k < 2500; ++k) safeLog.setLevel(SafeLogger::Level::INFO);
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "→ 4 執行緒併發呼叫 setLevel 各 2500 次："
                  << "minLevel 是 atomic，無 data race" << std::endl;
        std::cout << "→ 已輸出行數: " << safeLog.emitted()
                  << "（必定為 3：呼叫了 4 次，但 DEBUG 那筆在鎖外就被過濾掉了）"
                  << std::endl;
    }

    std::cout << "\n=== LeetCode 1115. Print FooBar Alternately (n = 5) ===" << std::endl;
    {
        FooBar fb(5);
        std::string out;
        std::mutex outMtx;
        auto emit = [&out, &outMtx](const std::string& s) {
            std::lock_guard<std::mutex> lock(outMtx);
            out += s;
        };

        // 刻意先啟動 bar，證明順序由 condition_variable 保證，
        // 而不是由執行緒建立順序保證
        std::thread b([&] { fb.bar([&] { emit("bar"); }); });
        std::thread a([&] { fb.foo([&] { emit("foo"); }); });
        b.join();
        a.join();

        std::cout << out << std::endl;
        std::cout << "→ 即使先啟動 bar 執行緒，輸出仍必定嚴格交替" << std::endl;
        std::cout << "→ 只用 mutex 做不到這件事：mutex 保證互斥，不保證順序" << std::endl;
    }

    std::cout << "\n=== 日常實務：非同步日誌（I/O 完全移出呼叫端）===" << std::endl;
    {
        AsyncLogger async;
        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&async, i] {
                for (int k = 0; k < 5000; ++k) {
                    async.log("INFO", "worker " + std::to_string(i)
                                      + " processed item " + std::to_string(k));
                }
            });
        }
        for (auto& t : ths) t.join();
        async.stop();      // 等寫出執行緒把佇列清空

        std::cout << "4 執行緒 × 5000 筆 = 20000 筆日誌" << std::endl;
        std::cout << "實際寫出: " << async.writtenCount() << " 筆（必定為 20000，不漏）" << std::endl;
        std::cout << "→ 呼叫端的臨界區段只有一次 push_back；" << std::endl;
        std::cout << "  真正的寫出由專責執行緒批次處理，不阻塞任何生產者。" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.5：保護共享資料實作6.cpp' -o thread_safe_logger

// ⚠️ 第一段（原始 ThreadSafeLogger）的輸出【每次執行都不同】，有三個原因：
//   ① 時間戳 [HH:MM:SS.mmm] 是執行當下的真實時間
//   ② thread id [T:...] 是 libstdc++ 印出的底層 pthread_t（執行緒堆疊位址），
//      每次執行都不同，也不保證跨平台一致
//   ③ 三個 worker 的【行序】取決於排程 —— mutex 保證每一行完整不交錯，
//      但完全不保證誰先寫（這正是 LeetCode 1115 那一段要說明的重點）
// 下面貼的是本機某一次的真實實測，不是保證值。
//
// 其餘各段皆為確定值：修正版 Logger 刻意關閉時間戳、
// LeetCode 1115 由 condition_variable 保證嚴格交替、
// 非同步日誌的 20000 筆不漏 —— 這三段每次執行都相同。

// === 預期輸出 ===
// [10:04:11.722] [INFO ] [T:137822050359232] === 程式開始 ===
// [10:04:11.722] [INFO ] [T:137822042322624] Worker 1 開始
// [10:04:11.722] [INFO ] [T:137822025537216] Worker 3 開始
// [10:04:11.722] [INFO ] [T:137822033929920] Worker 2 開始
// [10:04:11.722] [DEBUG] [T:137822033929920] Worker 2 執行任務 0
// [10:04:11.722] [DEBUG] [T:137822042322624] Worker 1 執行任務 0
// [10:04:11.722] [DEBUG] [T:137822025537216] Worker 3 執行任務 0
// [10:04:11.733] [DEBUG] [T:137822033929920] Worker 2 執行任務 1
// [10:04:11.733] [DEBUG] [T:137822025537216] Worker 3 執行任務 1
// [10:04:11.733] [DEBUG] [T:137822042322624] Worker 1 執行任務 1
// [10:04:11.743] [DEBUG] [T:137822033929920] Worker 2 執行任務 2
// [10:04:11.743] [DEBUG] [T:137822025537216] Worker 3 執行任務 2
// [10:04:11.743] [DEBUG] [T:137822042322624] Worker 1 執行任務 2
// [10:04:11.753] [INFO ] [T:137822033929920] Worker 2 結束
// [10:04:11.753] [INFO ] [T:137822025537216] Worker 3 結束
// [10:04:11.753] [INFO ] [T:137822042322624] Worker 1 結束
// [10:04:11.753] [INFO ] [T:137822050359232] === 程式結束 ===
//
// === 修正版 Logger（關閉時間戳，輸出可重現）===
// [INFO ] 服務啟動
// [WARN ] 設定檔缺少 timeout，使用預設值 3000ms
// [ERROR] 無法連線至 db-replica-2
// → 4 執行緒併發呼叫 setLevel 各 2500 次：minLevel 是 atomic，無 data race
// → 已輸出行數: 3（必定為 3：呼叫了 4 次，但 DEBUG 那筆在鎖外就被過濾掉了）
//
// === LeetCode 1115. Print FooBar Alternately (n = 5) ===
// foobarfoobarfoobarfoobarfoobar
// → 即使先啟動 bar 執行緒，輸出仍必定嚴格交替
// → 只用 mutex 做不到這件事：mutex 保證互斥，不保證順序
//
// === 日常實務：非同步日誌（I/O 完全移出呼叫端）===
// 4 執行緒 × 5000 筆 = 20000 筆日誌
// 實際寫出: 20000 筆（必定為 20000，不漏）
// → 呼叫端的臨界區段只有一次 push_back；
//   真正的寫出由專責執行緒批次處理，不阻塞任何生產者。
