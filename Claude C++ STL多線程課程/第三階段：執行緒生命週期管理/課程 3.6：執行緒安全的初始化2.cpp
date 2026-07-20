// =============================================================================
//  課程 3.6：執行緒安全的初始化2.cpp  —  std::call_once:只初始化一次的保證
// =============================================================================
//
// 【主題資訊 Information】
//   簽名      : template<class Callable, class... Args>
//               void call_once(std::once_flag& flag, Callable&& f, Args&&... args);
//   標準版本  : C++11
//   標頭檔    : <mutex>(once_flag 與 call_once 都在這裡,不在 <thread>)
//   複雜度    : 首次呼叫執行 f;之後每次呼叫只做一次快速檢查
//   例外      : f 拋出例外 → 例外傳播給呼叫者,且 flag「不會」被標記完成
//
// 【詳細解釋 Explanation】
//
// 【1. 這個問題有多容易寫錯】
// 「延遲初始化一個全域資源」看起來是最單純的需求,卻是多執行緒最經典的陷阱。
// 最直覺的寫法:
//
//     if (db == nullptr) {         // (1) 檢查
//         db = new Database();     // (2) 建立
//     }
//
// 兩條執行緒可能同時通過 (1),於是 (2) 執行兩次 —— 建立兩個 Database,
// 其中一個的指標被覆寫,直接記憶體洩漏;若 Database 建構子還有副作用
// (開檔、連線、註冊),後果更嚴重。這還只是「同時進入」的問題,
// 底下【2】會說明更隱蔽的那一半。
//
// 【2. 為什麼「加個 bool 旗標」的雙重檢查鎖定曾是經典錯誤】
// 很多人會寫出這個著名的 Double-Checked Locking Pattern:
//
//     if (!initialized) {              // 快速路徑,不上鎖
//         lock_guard g(mtx);
//         if (!initialized) {
//             db = new Database();
//             initialized = true;
//         }
//     }
//
// 在 C++11 之前,這段程式碼是「公認錯誤」的,原因有兩層:
//   (a) 語言層:C++98 根本沒有記憶體模型,連「兩個執行緒同時存取
//       一個變數」這件事都沒有定義,更別說推理正確性。
//   (b) 硬體/編譯器層:db = new Database() 實際上是三步 ——
//       配置記憶體、跑建構子、把位址寫進 db。編譯器與 CPU 都可能重排,
//       讓「db 已被賦值」對其他執行緒可見,但建構子還沒跑完。
//       另一條執行緒的快速路徑看到 initialized == true 就直接使用,
//       拿到的是「還沒建構完的物件」。
// C++11 之後可以用 std::atomic 搭配正確的 memory order 自己寫對,
// 但這需要對記憶體序有相當理解。call_once 把這一切封裝好了。
//
// 【3. call_once 到底保證什麼】
//   * 對同一個 once_flag,傳入的可呼叫物件「恰好成功執行一次」。
//   * 其他同時呼叫的執行緒會「阻塞等待」,直到那一次執行完成才返回
//     —— 不是「跳過就走」,而是真的等它做完。這點很關鍵:返回後
//     你可以安全地使用被初始化的資源。
//   * 記憶體序:那一次執行的完成 happens-before 所有其他 call_once
//     呼叫的返回。因此初始化過程中寫入的所有資料,對其他執行緒
//     都保證可見 —— 這正是【2】(b) 缺的那塊拼圖。
//
// 【4. 和 pthread_once 的關係】
// call_once 是 POSIX pthread_once 的 C++ 版本,但更強:
//   * pthread_once 的函式不能帶參數;call_once 可以完美轉發任意參數。
//   * pthread_once 的初始化函式若失敗沒有標準的重試語意;
//     call_once 明確規定「拋例外 ⇒ 不算完成 ⇒ 下一個執行緒可以再試」
//     (見本課第 6 個範例檔)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) once_flag 裡面是什麼
//   概念上是一個狀態機:未執行 / 執行中 / 已完成。libstdc++ 在 Linux 上
//   以 pthread_once 或內部的 futex 實作。重點是它「不是單純一個 bool」——
//   必須能表達「執行中」這個中間狀態,其他執行緒才知道該等待而不是跳過。
//
// (B) once_flag 不可複製、不可移動
//   標準明確刪除了它的複製與移動建構子。理由很直觀:旗標代表「某段程式
//   有沒有被執行過」這個唯一的事實,複製一份就會讓同一段初始化被執行兩次,
//   完全違背設計目的。也因此含有 once_flag 的類別預設也不可複製 ——
//   本課第 7 個範例檔會看到這件事的實際影響。
//
// (C) 首次之後的成本
//   初始化完成後,call_once 仍然要做一次原子讀取來確認狀態,再加上一次
//   函式呼叫。這比「什麼都不做」貴,但比每次都搶一把 mutex 便宜得多。
//   若這段程式在極熱的迴圈裡,考慮把結果快取到區域變數,而不是每圈都呼叫。
//
// (D) 和 function-local static(magic static)的取捨
//   C++11 起,函式內 static 區域變數的初始化本身就保證執行緒安全,
//   寫起來更短(見本課第 5 個範例檔)。本機實測 GCC 15.2.0 反組譯可見,
//   編譯器為它產生 __cxa_guard_acquire / __cxa_guard_release 呼叫與一個
//   guard 變數。兩者的差異在於:
//     * magic static:物件與初始化綁在一起,最簡潔,自動有解構。
//     * call_once   :初始化邏輯可以任意複雜、可帶參數、可跨多個物件,
//                     適合「初始化好幾樣東西」或「初始化目標不是單一物件」。
//
// 【注意事項 Pay Attention】
// 1. once_flag 必須有「至少和被保護資源一樣長」的生命週期,
//    而且必須是同一個物件 —— 每個執行緒各自帶一個區域 once_flag
//    等於完全沒有保護。
// 2. 別在初始化函式裡對「同一個」once_flag 再呼叫 call_once —— 死結。
// 3. 初始化函式拋例外 ⇒ 旗標不算完成,下一個執行緒會再試一次。
//    這是刻意設計的重試語意,不是 bug(見第 6 個範例檔)。
// 4. call_once 保證的是「初始化」的執行緒安全,不是「之後的使用」。
//    本例 db->query() 若會修改狀態,仍需要自己的同步。
// 5. 用裸指標 + new 的單例會在程式結束時漏掉解構;本例最後手動 delete,
//    但更好的做法是 unique_ptr 或直接用 magic static。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::call_once 與執行緒安全初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::call_once 除了「只執行一次」,還保證了什麼?
//     答：還保證兩件事。第一,其他同時呼叫的執行緒會「阻塞等到那一次執行
//         完成」才返回,而不是看到旗標已設就直接走人。第二是記憶體序:
//         那次執行的完成 happens-before 其他 call_once 的返回,
//         所以初始化寫入的資料對所有執行緒都保證可見。
//     追問：少了第二點會怎樣?
//         → 就會回到雙重檢查鎖定的經典 bug:別的執行緒看到「已初始化」,
//           拿到的卻是還沒建構完的物件,因為指標賦值可能先於建構子完成
//           而變得可見。
//
// 🔥 Q2. 為什麼 C++11 之前的雙重檢查鎖定被認為是錯的?
//     答：兩層原因。語言層面,C++98 沒有記憶體模型,無法定義多執行緒下
//         的資料存取;實作層面,new T() 包含「配置、建構、賦值指標」三步,
//         編譯器與 CPU 都可能重排,使得指標已可見但物件尚未建構完成。
//     追問：那 C++11 之後可以自己寫對嗎?
//         → 可以,用 std::atomic 搭配 release/acquire 就是正確的,
//           但需要對記憶體序有把握。call_once 或 magic static 更省事也更不易錯。
//
// ⚠️ 陷阱. 「call_once 已經保證執行緒安全了,所以之後多執行緒隨便用
//         那個物件都沒問題。」哪裡錯了?
//     答：call_once 保證的只有「初始化」這個動作的執行緒安全 ——
//         它確保物件被正確建立一次,而且大家都看得到完整的它。
//         但物件建好之後,如果多個執行緒同時呼叫會修改其內部狀態的方法,
//         那是另一個資料競爭,call_once 完全不管。
//     為什麼會錯：把「初始化安全」誤讀成「這個物件從此執行緒安全」。
//         這兩件事的作用範圍完全不同:前者只涵蓋建立的那一瞬間,
//         後者需要物件自己用 mutex/atomic 保護每個會改狀態的操作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 原始示範:用 call_once 保證 Database 只被建立一次
// -----------------------------------------------------------------------------
class Database {
public:
    Database() { say("  Database 初始化"); }
    void query() { say("  查詢中..."); }
};

Database* db = nullptr;
std::once_flag initFlag;

void initDatabase() {
    db = new Database();
}

void initAndUse() {
    std::call_once(initFlag, initDatabase);
    // 走到這裡時,db 保證已被完整建構,而且對本執行緒可見
    db->query();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】延遲載入設定檔:多執行緒同時要求,只讀一次磁碟
//   情境: 服務啟動後,第一個進來的請求才去讀設定檔(避免拖慢啟動);
//         但同一瞬間可能有數十個請求湧入,設定檔絕不能被讀好幾次
//         ——那不只浪費 I/O,若中途檔案被改寫,不同執行緒還會拿到不同設定。
//   為什麼用本主題: 這正是 call_once 的標準用例:昂貴、只該做一次、
//                   而且所有人都必須等它做完才能繼續。
// -----------------------------------------------------------------------------
struct AppConfig {
    std::string host;
    int         port = 0;
    int         loadCount = 0;      // 用來驗證「真的只載入了一次」
};

AppConfig      g_config;
std::once_flag g_configFlag;
int            g_diskReads = 0;     // 只在 call_once 內修改,不需額外同步

void loadConfigFromDisk() {
    ++g_diskReads;                  // 這行一輩子只會執行一次
    g_config.host = "10.0.0.7";
    g_config.port = 8443;
    g_config.loadCount = g_diskReads;
    say("    [設定] 從磁碟載入設定檔(這行只會出現一次)");
}

const AppConfig& config() {
    std::call_once(g_configFlag, loadConfigFromDisk);
    return g_config;
}

void requestHandler(int id) {
    const AppConfig& c = config();
    say("    [請求 " + std::to_string(id) + "] 連往 " + c.host + ":" +
        std::to_string(c.port));
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】call_once 可以帶參數(pthread_once 做不到的事)
//   情境: 初始化 log 系統時需要外部提供的參數(輸出路徑、等級)。
//         call_once 會完美轉發參數給初始化函式,不必為了傳參而借用全域變數。
//   為什麼用本主題: 展示【詳細解釋 4】提到的 call_once 相對 pthread_once
//                   的關鍵優勢。
// -----------------------------------------------------------------------------
struct Logger {
    std::string path;
    std::string level;
};

std::unique_ptr<Logger> g_logger;      // 用 unique_ptr,避免裸 new 漏掉解構
std::once_flag          g_loggerFlag;

void initLogger(const std::string& path, const std::string& level) {
    g_logger = std::make_unique<Logger>();
    g_logger->path  = path;
    g_logger->level = level;
    say("    [log] 初始化 logger → " + path + " (等級 " + level + ")");
}

Logger& logger(const std::string& path, const std::string& level) {
    // 參數會被完美轉發給 initLogger;只有「第一個到達的執行緒」的參數生效
    std::call_once(g_loggerFlag, initLogger, path, level);
    return *g_logger;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的五題並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   考的都是「多執行緒之間反覆交替執行的順序控制」,需要條件變數或號誌;
//   而 call_once 的語意是「某件事整個程式只做一次」,兩者性質不同,
//   套用進去只會讓解法變得奇怪。設計題(146 LRU Cache 等)則是單執行緒
//   資料結構題。本課主題與 call_once 最貼近的實際應用是「單例/延遲初始化」,
//   因此改以上面兩個真實情境示範;本課第 7 個範例檔會示範一個
//   真正用得上 LeetCode 146 的組合(執行緒安全單例 + LRU 快取)。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:三條執行緒同時初始化資料庫 ===" << std::endl;
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    std::thread t3(initAndUse);
    t1.join();
    t2.join();
    t3.join();
    std::cout << "  ↑「Database 初始化」只出現一次,「查詢中...」出現三次"
              << std::endl;

    std::cout << "\n=== 實務 1:8 條執行緒同時要求設定,只讀一次磁碟 ===" << std::endl;
    std::vector<std::thread> pool;
    for (int i = 1; i <= 8; ++i) pool.emplace_back(requestHandler, i);
    for (std::thread& t : pool) t.join();
    std::cout << "  實際磁碟讀取次數 = " << g_diskReads
              << "(8 條執行緒,只載入 1 次)" << std::endl;

    std::cout << "\n=== 實務 2:call_once 帶參數初始化 logger ===" << std::endl;
    std::thread l1([]() { logger("/var/log/app.log", "INFO"); });
    std::thread l2([]() { logger("/tmp/other.log", "DEBUG"); });
    l1.join();
    l2.join();
    std::cout << "  最終 logger 路徑 = " << g_logger->path
              << "  (只有先搶到的那組參數生效)" << std::endl;

    delete db;    // 裸指標單例必須手動釋放;更好的做法見【注意事項】5
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.6：執行緒安全的初始化2.cpp" -o once_basic

// 注意:以下為某一次實際執行的結果。
//   * 各執行緒之間的行序每次執行都可能不同(例如「請求 3」可能排在「請求 1」之前)。
//   * 「最終 logger 路徑」取決於哪條執行緒先搶到初始化 —— 兩種結果都合法,
//     這正是「只有第一個到達者的參數生效」的直接體現。
//   * 但關鍵不變量每次都成立:「Database 初始化」恰好一次、
//     磁碟讀取次數恰好為 1、「從磁碟載入設定檔」恰好一行。

// === 預期輸出 ===
// === 原始示範:三條執行緒同時初始化資料庫 ===
//   Database 初始化
//   查詢中...
//   查詢中...
//   查詢中...
//   ↑「Database 初始化」只出現一次,「查詢中...」出現三次
//
// === 實務 1:8 條執行緒同時要求設定,只讀一次磁碟 ===
//     [設定] 從磁碟載入設定檔(這行只會出現一次)
//     [請求 1] 連往 10.0.0.7:8443
//     [請求 4] 連往 10.0.0.7:8443
//     [請求 3] 連往 10.0.0.7:8443
//     [請求 6] 連往 10.0.0.7:8443
//     [請求 5] 連往 10.0.0.7:8443
//     [請求 2] 連往 10.0.0.7:8443
//     [請求 7] 連往 10.0.0.7:8443
//     [請求 8] 連往 10.0.0.7:8443
//   實際磁碟讀取次數 = 1(8 條執行緒,只載入 1 次)
//
// === 實務 2:call_once 帶參數初始化 logger ===
//     [log] 初始化 logger → /var/log/app.log (等級 INFO)
//   最終 logger 路徑 = /var/log/app.log  (只有先搶到的那組參數生效)
