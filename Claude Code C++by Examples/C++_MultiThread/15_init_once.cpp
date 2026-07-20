// =============================================================
// 15_init_once.cpp  --  std::call_once / Meyer's singleton /
//                       thread_local
// =============================================================
//
// 本課目標:
//   1. 用 std::call_once + std::once_flag 確保某段初始化在
//      多執行緒之下 *只執行一次*。
//   2. 用 C++11 的 function-local static (Meyer's singleton)
//      —— 這是現代 C++ 中寫 thread-safe singleton 最短、最
//      正確的方式。
//   3. 用 thread_local 為每個執行緒準備一份「自己的」變數
//      —— 高效能日誌 buffer、per-thread allocator、亂數種子
//      等場景的標準做法。
//   4. 知道為什麼 *不要* 自己手刻 double-checked locking
//      (DCL) —— 它在 C++11 之前是個有名的反例,C++11 之後
//      編譯器會幫你做到正確,所以根本沒必要自己寫。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 15_init_once.cpp -o 15_init_once
//
// 執行方式:
//     ./15_init_once
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     初始化只跑一次 ── call_once / Meyer's singleton / thread_local
// 前置課程: lesson 03
// 觀念詞彙:
//   - once-only initialization ── 在多 thread 下確保僅執行一次
//   - Meyer's singleton        ── function-local static,C++11+ 自動 thread-safe
//   - thread_local             ── 每條 thread 各自一份的儲存類別
//   - DCL (double-checked locking) ── 古老反例,C++11 之後不要自己寫
// 新介紹 API:
//   std::once_flag             配合 call_once 的旗標
//   std::call_once(flag, fn)   保證 fn 只跑一次,即使多 thread 同時呼叫
//   thread_local T x;          每條 thread 自己一份的變數
// 何時使用 (選哪一個):
//   - 全域單例 → Meyer's singleton (function-local static)
//   - 一段任意位置的 lazy init → std::call_once
//   - 「per-thread 工作 buffer / 隨機種子」 → thread_local
// 何時不要用:
//   - 自己手刻 DCL → 永遠錯,直接用 call_once 或 Meyer's
//   - thread_local 全域物件當 RAII 資源管理 → 生命週期等於 thread,
//     在 thread pool 中很可能比你預期久
// 常見錯誤:
//   - 一個 once_flag 給多個 init 用 → 第二、第三個都被吃掉
//   - call_once 內再呼叫 call_once 同一旗標 → deadlock
//   - 把 thread_local 想成「永遠便宜」→ 仍有 TLS slot lookup 開銷
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── 一次性初始化的歷史與正確解法
// =============================================================
//
// 1. 為什麼初始化會 race
//    Singleton 經典寫法:
//        Instance* getInstance() {
//            if (!instance) instance = new Instance();
//            return instance;
//        }
//    多 thread 同時進入 → 兩個 thread 都看到 instance==null → 各自 new
//    → 一個 leak,且後寫的覆蓋,兩條 thread 持有不同物件,語意完全錯。
//
// 2. DCLP (Double-Checked Locking) 為什麼長期錯
//    1990s 流行的「修法」:
//        if (!instance) {
//            lock(mu);
//            if (!instance)               // 第二檢查
//                instance = new Instance();
//            unlock(mu);
//        }
//    在當時的 C++ 沒有 memory model,*編譯器/CPU 可重排*:
//        instance = (Instance*)malloc(sizeof Instance);  // 1. 分配
//        instance->ctor();                                // 2. 呼叫建構
//    若 1 先完成 → 別的 thread 看見 instance != null → 跳過第二檢查 →
//    存取 *尚未建構* 的物件。Andrei Alexandrescu 那篇有名論文「The C++
//    and the Perils of Double-Checked Locking」就在講這件事。
//
//    C++11 起這條路才修好:
//        atomic<Instance*> instance;
//        if (auto* p = instance.load(acquire); !p) {
//            scoped_lock lk(mu);
//            p = instance.load(relaxed);
//            if (!p) { p = new Instance(); instance.store(p, release); }
//        }
//        return p;
//    但這已不簡單。下面有更好的選擇。
//
// 3. C++11 之後的三個正確解
//    A. Meyer's singleton (推薦)
//         Instance& getInstance() {
//             static Instance inst;   // C++11 起,此處初始化保證 thread-safe
//             return inst;
//         }
//       編譯器產生 *guard variable* (一個 atomic 旗標) + 一個內部 mutex,
//       第一次呼叫時 lock+建構+設旗,後續呼叫直接走 fast path 看旗標。
//       fast path 成本約 1-2 ns (atomic load),完全合理。
//
//    B. std::call_once + std::once_flag
//         once_flag flag;
//         std::call_once(flag, []{ /* init */ });
//       本質與 A 一樣 (guard + mutex),但給你 *non-static* 場景:每個物件
//       的 lazy init、可被傳入 lambda。靈活度比 A 高,寫法稍長。
//
//    C. 直接 eager initialize
//         static Instance inst;          // 在 namespace scope
//       global object 在 main 之前完成初始化,沒有 race window。但要小心
//       「static initialization order fiasco」── 跨 TU 的初始化順序未定義。
//
// 4. call_once 的內部
//    Linux 實作通常是 once_flag 內部含 atomic state (idle/running/done) +
//    futex。第一個進入的人:CAS state 變 running,執行 lambda,完成後 CAS
//    為 done,futex_wake 全部等待者。其他人:CAS 失敗 → futex_wait。
//    Lambda 若 throw 例外 → state 重置回 idle,下個來的人重新嘗試。
//    *call_once 不能遞迴呼叫同一個 flag* → deadlock。
//
// 5. thread_local 的成本
//    thread_local 變數每 thread 一份,放在 TLS (thread-local storage) 區。
//    存取:
//      - x86-64 Linux glibc: 透過 fs:[var@tpoff] 一條指令拿到位址,~1 ns。
//      - 動態載入的 .so 內的 thread_local: 走 __tls_get_addr,~5-10 ns,
//        因為要查 dynamic TLS table。
//    第一次存取會觸發 lazy 建構;有解構子的 thread_local 在 thread 結束時
//    自動解構 (LIFO 順序)。
//    用法:per-thread cache、per-thread random engine、per-thread arena。
//
// 6. 哪個該選
//    單例 (process 全域) → Meyer's singleton (A)。
//    每個物件的 lazy init → call_once + member once_flag (B)。
//    高頻熱點、不能多花 1 ns → 自己 atomic + memory_order (C++11 修好的 DCLP)。
//    每 thread 獨立狀態 → thread_local (不是初始化問題,是 *分割*)。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── 一次性初始化是工程議題,
//     LC 評分用例不需要這個。
//   → 但 lesson 30 Q8 Web Crawler 的 hostname extractor 若有快取,
//     就是 thread_local + call_once 的應用場景。
//
// 主要 API 對照 (cppreference):
//   - std::call_once                    https://en.cppreference.com/w/cpp/thread/call_once
//   - std::once_flag                    https://en.cppreference.com/w/cpp/thread/once_flag
//   - thread_local 儲存類別             https://en.cppreference.com/w/cpp/language/storage_duration
//   - Magic statics (static local var)  https://en.cppreference.com/w/cpp/language/storage_duration#Static_local_variables
//
// 練習建議:
//   - Singleton 寫法:用 Meyer's static (最簡)、用 call_once + once_flag
//     (member 場景)、用 atomic + DCLP (歷史對照)。比較三者的 fast path
//     成本。
//   - 進階:thread_local 的 random engine,讓 lesson 30 Q7 Dining
//     Philosophers 不用每個 lambda 自己 mt19937(id)。
// =============================================================

/*
補充筆記：init_once
  - init_once 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - init_once 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全的一次性初始化：magic static / call_once / DCLP
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 如何實作 thread-safe singleton？
//     答：最佳解是 Meyers singleton（magic static）：
//         static T& instance() { static T inst; return inst; }
//         C++11 起標準保證 function-local static 的初始化是 thread-safe 的——多個
//         thread 同時進入時只有一個執行初始化，其餘阻塞等待完成。次選是 std::call_once
//         + std::once_flag（適用於非 static 的情境）。不要手寫 double-checked locking。
//     追問：有什麼隱憂？（static destruction order fiasco：跨 TU 的解構相依可能失效；
//           必要時用永不解構的 leaky singleton）
//
// 🔥 Q2. DCLP 在 C++11 之前為什麼是錯的？現在呢？
//     答：舊寫法 if (!p) { lock; if (!p) p = new T; } 的問題在於 p = new T 包含三步：
//         配置記憶體、呼叫建構子、把位址寫入 p。編譯器／CPU 可以把「寫入 p」重排到
//         「建構子完成」之前，於是另一個 thread 在第一層檢查看到非空指標，卻拿到尚未
//         建構完成的物件。C++11 之前語言沒有記憶體模型，volatile 也無法修正。C++11
//         之後把 p 宣告為 std::atomic<T*>、第一層用 load(acquire)、寫入用 store(release)
//         即可正確——但既然 magic static 更短更快，實務上沒有理由自己寫 DCLP。
//     追問：用預設的 seq_cst 寫 DCLP 對嗎？（對，只是比 acquire/release 稍慢）
//
// 🔥 Q3. thread_local 有什麼陷阱？
//     答：每個 thread 各有一份獨立實體，首次使用時建構、thread 結束時解構。陷阱：
//         (1) 存取有間接成本；(2) detached thread 的解構時機不保證可靠；(3) 在動態
//         載入的函式庫中語意複雜；(4) thread pool 中「thread 不會結束」，導致
//         thread_local 狀態跨任務殘留、造成污染。
//     追問：在 thread pool 裡怎麼安全使用？（明確在任務開始或結束時重置，別依賴解構）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>
#include <chrono>
#include <random>

// =============================================================
// PART 1 —— std::call_once + std::once_flag
//
// 規格保證:給定一個 std::once_flag,std::call_once 會把它
// 對應的 callable 在 *所有執行緒中合計* 只執行一次,即使有
// 100 條執行緒同時呼叫 std::call_once 也一樣。
//
// 與其他寫法的比較:
//   - 用一個 std::atomic<bool>:容易寫錯成 DCL 的破版本。
//   - 用 std::mutex + bool:可行但每次呼叫都要鎖,熱路徑慢。
//   - std::call_once:第一次以後是無鎖快路徑,且實作正確。
//
// 適合場景:
//   - 載入設定檔、建立連線池、註冊全域 callback。
//   - 任何「需要懶初始化、也需要在多執行緒下安全」的東西,
//     而且初始化本身可能比較重 (做 I/O、配置大塊記憶體)。
// =============================================================
namespace once_demo {

struct Heavy {
    Heavy()
    {
        std::cout << "  [Heavy ctor] running on thread "
                  << std::this_thread::get_id() << '\n';
        // 模擬「初始化需要花時間」
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    int value = 12345;
};

std::once_flag heavy_flag;
Heavy*         heavy_ptr = nullptr;

void use_heavy(int who)
{
    std::call_once(heavy_flag, []{
        // 這個 lambda 在所有執行緒之間 *只跑一次*。
        // 即便初始化丟例外,旗標仍會被視為「未完成」,
        // 下一個呼叫者會重試 —— call_once 的另一個正確
        // 行為,自己手刻 atomic flag 很容易忽略這點。
        heavy_ptr = new Heavy();
    });
    std::cout << "    [user " << who << "] heavy.value = "
              << heavy_ptr->value << '\n';
}

} // namespace once_demo


// =============================================================
// PART 2 —— Meyer's singleton (function-local static)
//
// C++11 起,規格保證 *函式內的 static 物件* 的初始化是
// thread-safe 的:多個執行緒同時走進 instance(),只有一個
// 會跑建構子,其他人會等;之後第二次以後的呼叫,進入這個
// 函式不會再做同步檢查 (好的編譯器把它優化成 atomic 旗標 +
// fast path)。
//
// 這是現代 C++ 寫 singleton 的 *建議* 做法。比 std::call_once
// 更短,而且因為「物件本體就在 static」,連解構順序也由標準
// 規範。比 new 出來的「永不解構的 raw pointer」更乾淨。
//
// 警告:singleton 是個容易被濫用的設計。能避免就避免;真的
// 需要時用 Meyer's 形式。
// =============================================================
class Config {
public:
    static Config& instance()
    {
        static Config c;          // <<< 一句,thread-safe,正確
        return c;
    }

    int port = 8080;

private:
    Config()
    {
        std::cout << "  [Config ctor] singleton built on thread "
                  << std::this_thread::get_id() << '\n';
    }

    // 禁止複製 / 搬移 —— singleton 應該是唯一的。
    Config(const Config&)            = delete;
    Config& operator=(const Config&) = delete;
};


// =============================================================
// PART 3 —— thread_local
//
// 把儲存類別設成 thread_local 的變數,*每一個執行緒* 都會
// 有自己的一份。讀寫不需要同步,因為其他執行緒根本看不到
// 你這份。
//
// 適合場景:
//   - 高效能日誌庫的 per-thread buffer (避開鎖)
//   - per-thread 的隨機數產生器 (std::mt19937 不是 thread-safe,
//     直接做 thread_local 就解決)
//   - per-thread allocator pool
//   - 蒐集 per-thread 統計,最後再合併
//
// 注意事項:
//   - 建構在「每條執行緒第一次用到這個變數時」發生;解構
//     在執行緒結束時。所以 thread_local 物件的生命週期可能
//     比你直覺的更短。
//   - thread_local 不是免費的:每次存取會走過 TLS slot lookup
//     (在 Linux 上是 %fs / %gs 偏移,很快;但仍比 stack 慢)。
//   - thread_local 全域物件的 *初始化順序* 規則比一般全域物件
//     寬鬆,跨翻譯單元的相依要小心。
// =============================================================
thread_local int per_thread_counter = 0;

// 簡易示範:示意「per-thread 隨機種子」這個常見用法,但
// 為了不引入額外 headers,我們用 counter 本身代替。
static void thread_work(int who)
{
    for (int i = 0; i < 5; ++i) ++per_thread_counter;   // 各自累加
    std::cout << "    [thread " << who << "] per_thread_counter = "
              << per_thread_counter << '\n';
}


// =============================================================
// 補充:為什麼 *不要* 自己手刻 Double-Checked Locking
//
// 在 C++11 之前你會看到這種寫法:
//
//     static Heavy* p = nullptr;
//     static std::mutex m;
//     Heavy* get() {
//         if (!p) {                  // 第一次 check (沒鎖)
//             std::lock_guard<std::mutex> lk(m);
//             if (!p) p = new Heavy(); // 第二次 check (有鎖)
//         }
//         return p;
//     }
//
// 看起來對,實際上在沒有 release/acquire 的情況下是錯的:
// 編譯器或 CPU 可以把「設定 p」與「執行 Heavy 建構」這兩件
// 事的順序對調,讓另一個執行緒看到「p 已經非 null」但 *指
// 到的物件其實還沒建好*,然後就崩了。
//
// 修法:
//   - 用 std::call_once (PART 1)
//   - 用 Meyer's singleton (PART 2)
//   - 真的要手刻,p 必須是 std::atomic<Heavy*>,且 store 用
//     release、load 用 acquire。
//
// 但你已經有 PART 1 / PART 2 兩個更短、更安全的選項,根本
// 沒理由自己 DCL。
// =============================================================


// =============================================================
// MAIN
// =============================================================
int main()
{
    // ---- PART 1: std::call_once ----
    std::cout << "PART 1: std::call_once (Heavy ctor 應該只跑一次)\n";
    {
        std::vector<std::jthread> ts;
        for (int i = 0; i < 8; ++i) {
            ts.emplace_back([i]{ once_demo::use_heavy(i); });
        }
        // jthread 解構自動 join
    }
    std::cout << '\n';

    // ---- PART 2: Meyer's singleton ----
    std::cout << "PART 2: Meyer's singleton (Config ctor 應該只跑一次)\n";
    {
        std::vector<std::jthread> ts;
        std::atomic<long long> sum{0};
        for (int i = 0; i < 8; ++i) {
            ts.emplace_back([&sum]{
                sum += Config::instance().port;
            });
        }
        ts.clear();
        std::cout << "  sum = " << sum.load()
                  << "  (期望 8 * 8080 = " << (8 * 8080) << ")\n";
    }
    std::cout << '\n';

    // ---- PART 3: thread_local ----
    std::cout << "PART 3: thread_local (每條執行緒有自己的 counter)\n";
    {
        std::vector<std::jthread> ts;
        for (int i = 0; i < 4; ++i) {
            ts.emplace_back(thread_work, i);
        }
        ts.clear();
        std::cout << "  [main] per_thread_counter = "
                  << per_thread_counter
                  << "  (main 自己這一份從未被改過,所以是 0)\n";
    }

    // ---- 實戰 1: lazy 載入大型 lookup table (call_once) ----
    //
    // 應用場景: 服務啟動快, 但有個 1MB+ 的字典/路由表需要從檔
    // 案載入, 且只有部分請求路徑會用到。用 call_once 在第一次
    // 需要時 lazy 載入, 之後完全免費。
    {
        std::cout << "\nPART 4 (extra): lazy lookup table via call_once\n";
        static std::once_flag dict_flag;
        static std::vector<std::string> dict;

        auto get_dict = []() -> const std::vector<std::string>& {
            std::call_once(dict_flag, []{
                std::cout << "  [lazy] loading dict (only once!)\n";
                // 假裝在從檔案讀
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                dict = {"alpha", "beta", "gamma", "delta"};
            });
            return dict;
        };

        std::vector<std::jthread> ts;
        for (int i = 0; i < 4; ++i) {
            ts.emplace_back([&, i]{
                const auto& d = get_dict();
                std::cout << "  [thread " << i << "] dict[0] = " << d[0] << '\n';
            });
        }
        ts.clear();
    }

    // ---- 實戰 2: per-thread RNG (thread_local) ----
    //
    // 應用場景: 多 thread 各自需要隨機數 (例如蒙地卡羅模擬、
    // 隨機抽樣 retry)。共享一個 std::mt19937 + mutex 會嚴重序
    // 列化。每條 thread 自己一份 thread_local mt19937, 各自 seed,
    // 不需要任何鎖, 也避免 false sharing (TLS 各在自己 page)。
    {
        std::cout << "\nPART 5 (extra): per-thread RNG\n";
        auto roll = [](int who){
            // 第一次進來才建構, 之後同 thread 重複用同一個 rng
            thread_local std::mt19937 rng(std::random_device{}() + who);
            std::uniform_int_distribution<int> dist(1, 100);
            int sum = 0;
            for (int i = 0; i < 5; ++i) sum += dist(rng);
            std::cout << "  [thread " << who << "] sum of 5 rolls = "
                      << sum << '\n';
        };
        std::vector<std::jthread> ts;
        for (int i = 0; i < 4; ++i) ts.emplace_back(roll, i);
        ts.clear();
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 C++11 之後不必自己寫 double-checked locking (DCL)?
    //    A：C++11 規定「function-local static 變數的初始化是 thread-
    //       safe」(俗稱 Magic Statics), 編譯器會在背後幫你做 DCL +
    //       acquire/release fence。Meyer's singleton (static T&
    //       instance() { static T t; return t; }) 一行解決, 比手刻 DCL
    //       短、正確、零鎖快。手刻 DCL 在 C++11 前是有名的反例, 容易
    //       漏 fence。
    //
    //  Q2：std::call_once 跟 Meyer's singleton 該選哪個?
    //    A：要 init 的東西在 namespace/class scope 而不是函式內 (例如
    //       global cache、要 lazy init 的 plugin) → call_once + once_
    //       flag。要 init 的是「某個值」, 可以放進 function-local
    //       static → Meyer's singleton 更短。call_once 還支援例外重試
    //       (init 函式 throw, flag 不會標記完成, 下次再試一次), 這是
    //       Meyer's singleton 也有的特性。
    //
    //  Q3：thread_local 變數在哪初始化、哪解構?
    //    A：第一次該 thread 進入該變數的存取時, 才執行 ctor (lazy);
    //       thread 結束時 ctor 反序執行 dtor。所以放重物 (大 vector、
    //       db connection) 在 thread_local, 進場成本會在第一次存取打到。
    //       注意: detached thread 結束時 thread_local dtor 仍會跑, 但
    //       main() 結束會 SIGKILL 整個 process, 那條 thread 的 dtor 就
    //       不執行 ── 影響 thread_local 中包 file/socket 的清理。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 「這段初始化必須只執行一次」的選擇順序:
//      (a) 全域單例 / 跨函式生命週期 —— 用 Meyer's singleton
//          (function-local static)。這是 C++11+ 最短最正確
//          的寫法。
//      (b) 一段任意位置的「lazy init」程式碼 —— 用
//          std::call_once + std::once_flag。
//      (c) 自己用 atomic + acquire/release 手刻 —— 不要,
//          除非你能說明為何 (a)(b) 都不適合,且有 reviewer。
//
// 2. Meyer's singleton 的好處:
//      - 自動 thread-safe (C++11 規格)。
//      - 解構順序明確 (與其他 static 反向)。
//      - 沒有 raw new / delete,沒有 leak 風險。
//      - 寫起來只要兩行。
//
// 3. std::call_once 的常見錯誤:
//      - 同一個 once_flag 給不同初始化用 —— 只會跑第一個
//        被觸發的那個,其他通通被吃掉。
//      - 在被 call_once 包住的 lambda 裡再呼叫 std::call_once
//        於同一個 flag —— 會死鎖。換不同 flag 才行。
//      - 假設 lambda 丟例外時旗標已被「設」—— 不會,
//        call_once 會把該次視為失敗,下一個呼叫者重試。
//
// 4. thread_local 重點:
//      - 適合「每條執行緒一份、不需要共享」的東西。
//      - 寫入完全免同步;讀寫只看自己這份。
//      - 在執行緒結束時自動解構;在執行緒池中要注意,因為
//        worker 可能很久才結束,thread_local 物件可能比你
//        想像的活得更久。
//      - thread_local 全域變數的 *初始化* 在「該執行緒第一次
//        用到它」時發生;不要假設 main() 開始時就好了。
//
// 5. 不要把 thread_local 和「高效能無鎖」劃等號。它是「沒有
//    共享」,因此天然無同步;但跨執行緒合併資料時,你還是
//    需要 atomic 或 mutex 來把每份結果蒐集起來。
//
// 6. 雙重檢查鎖定 (DCL) 是個寫對也沒收益的東西:寫對的版本
//    跟 std::call_once 在第二次以後的速度幾乎一樣;寫錯的
//    版本會炸。你完全沒理由再去碰它。
// =============================================================
