// =============================================================
// 21_deadlock.cpp  --  Deadlock 預防的四種策略
// =============================================================
//
// 本課目標:
//   1. 明確指出最常見的 deadlock 原因:*兩條執行緒以相反順序
//      鎖兩把 mutex*。
//   2. 學會四種正確的修法,各有適用場景:
//        (a) std::scoped_lock(m1, m2) ── C++17 神器,最簡單。
//        (b) try_lock + 退讓重試 ── 適合無法用 scoped_lock 的
//            情境 (例如鎖數量在執行期才知道、或要設超時)。
//        (c) 全域鎖排序 (lock ordering) ── 大型系統的標準做法。
//        (d) 設計層級避開 ── 別讓不同程式碼路徑同時拿超過一把
//            鎖。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 21_deadlock.cpp -o 21_deadlock
//
// 執行方式:
//     ./21_deadlock
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Deadlock 預防 ── 四種正確修法
// 前置課程: lesson 03
// 觀念詞彙:
//   - deadlock          ── 互相等對方持有的鎖
//   - lock ordering     ── 全局約定鎖順序
//   - try_lock + backoff── 拿不到就退讓重試
//   - lock hierarchy    ── 階層化鎖 (上層永遠先鎖)
// 新介紹 API:
//   std::scoped_lock(m1, m2, ...)        C++17,原子地同時鎖多把
//   std::lock(m1, m2, ...)               同上的舊形式 (C++11),全鎖到才返回
//   std::try_lock(m1, m2, ...)           C++11 自由函式;一次嘗試,全成功回 -1,
//                                          其中第 k 把失敗就回 k 並把前面的全 unlock
//   m.try_lock() / m.try_lock_for(d)     非阻塞 / 計時鎖
//   std::unique_lock(m, std::try_to_lock)
//   std::adopt_lock                       「鎖已上,我來接手 RAII」
// 何時用哪個:
//   - 鎖數量編譯期已知 → std::scoped_lock(...)
//   - 鎖數量執行期才知道 → try_lock + 重試
//   - 整個專案的鎖管理 → 全域 lock ordering hierarchy
//   - 想根本避開 → 重新設計,只持 *一把* 鎖
// 常見錯誤:
//   - 兩條程式碼以不同順序鎖兩把 mutex → deadlock
//   - 持鎖時等別人完成 (future.get、send 訊號等) → 隱藏 deadlock
//   - try_lock 不退讓直接 spin → livelock
//   - 持鎖時呼叫 callback,callback 又拿同一把鎖 → recursive deadlock
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── deadlock 的形式論與工程做法
// =============================================================
//
// 1. Coffman 四條件 (deadlock 必須同時滿足)
//    A. 互斥 (mutual exclusion)        ── 資源不能共享
//    B. 持有並等待 (hold and wait)      ── 持有 R1 時去要 R2
//    C. 不可剝奪 (no preemption)        ── 別人不能搶走你持有的 R1
//    D. 環形等待 (circular wait)        ── A 等 B,B 等 C,C 等 A
//    打破任一條 = 不會 deadlock。實務上能改的多半是 B 和 D。
//
// 2. 三種對應策略:預防 / 避免 / 偵測
//    預防 (prevention) ── 設計上保證四條件之一不成立
//      - 一次抓所有鎖 (打破 B,scoped_lock 就是這個)
//      - 全域鎖排序 (打破 D)
//      - 完全免鎖 (打破 A,但通常不可行)
//
//    避免 (avoidance) ── 執行時根據資源圖預先檢查 (Banker's algorithm)。
//      工業上幾乎不用 ── 太貴,且需要事先知道資源需求。
//
//    偵測 (detection) ── 容許 deadlock 發生,定期掃資源圖,發現環就強制
//      解開 (例如 kill 一條 thread,或回滾交易)。
//      DBMS (PostgreSQL、Oracle) 用這個;一般應用不用。
//
//    所以 99% 的 C++ 應用程式只會用 *預防*。
//
// 3. scoped_lock 的演算法
//    std::lock(m1, m2, ...) (scoped_lock 內部用) 採用 try-and-back-off:
//      1. lock(m_a) 第一個。
//      2. try_lock(m_b)。失敗 → 釋放 m_a,小退讓 (yield),從頭再來。
//      3. 一直到全部 try_lock 都成功才結束。
//    沒有環形等待 (失敗就放掉,不持有等別人) → 打破條件 B。
//    這個策略理論上 livelock 可能 (一直互相退讓),但實務上 OS 排程不齊
//    很快就解;且每次重來時起點 mutex 不同,不容易卡同一個 pattern。
//
// 4. 全域鎖排序 (lock hierarchy)
//    給每把鎖一個編號,規定:任何 thread 拿鎖必須按編號 *遞增* 順序拿。
//    例:account[i] 鎖編號 = i。轉帳 from i to j ── 永遠先鎖 min(i,j),
//    再鎖 max(i,j)。這樣兩條 thread 同時轉 (i→j) 與 (j→i),都按相同
//    順序拿 → 不可能成環。
//    維護:大型專案會在 mutex 包裝類別內塞「層級值」,debug 編譯版檢查
//    「拿 L=5 時不能再拿 L=3」 (例:Boost.Thread 的 lockable_concept_traits)。
//
// 5. try_lock_for + timeout
//    某些場景願意「等不到就放棄」,而不是排序鎖:
//        if (m.try_lock_for(100ms)) { ... } else { /* fallback */ }
//    適合 UI / 互動式應用 ── 不能讓使用者卡死。lesson 28 的 timed_mutex
//    示範。
//
// 6. 隱藏 deadlock 的常見來源
//    A. 持鎖時呼叫 callback ── callback 拿同一把 / 別把鎖,拓樸不可預測。
//       原則:critical section 內 *只做純內部資料操作*,不對外。
//    B. 持鎖時 future.get() ── 等別人,別人可能也在等你的鎖。
//    C. 同一 thread 重入同一個 std::mutex (非 recursive) → UB,通常 deadlock。
//       要重入用 std::recursive_mutex,但這常是設計味道 (見 lesson 28)。
//    D. cv.wait 沒檢查條件 → 假醒後做了不該做的事,可能間接 deadlock。
//
// 7. livelock vs deadlock
//    deadlock:全部 thread 卡死,系統不前進。
//    livelock:thread 不停活動但永遠無進展 (例如兩條 try-lock 演算法
//             同步退讓)。CPU 100%、什麼工作都沒完成。
//    解法:加入隨機 backoff (例如 yield 隨機 ns 數)、或限制重試次數。
//
// 8. priority inversion (前面 lesson 03 提過,這裡擴展)
//    高優先 H 等鎖 m,m 由低優先 L 持有,中優先 M 在跑 → L 沒被排到,
//    H 跟著被卡 (透過 L)。Linux 的 PI futex 解;標準 std::mutex 沒提供。
//    NASA Mars Pathfinder 1997 年因此事故聞名 ── 排程器把 high-pri 任務
//    意外卡死,後來空中升級 patch。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✓ LC 1226  The Dining Philosophers
//     - 教科書經典:5 個哲學家共用 5 根筷子,每人需要兩根才能吃。
//       本課的 std::scoped_lock 一次拿兩根,內含 deadlock-avoidance 演算法。
//     - 完整解答 → lesson 30 Q7
//   → 任何「同時鎖兩把以上 mutex」的 LC 解都直接受惠於本課。lesson 30
//     Q5 BoundedBlockingQueue 雖然只有一把鎖,但若擴充成「多 producer
//     多 queue 跨轉」就會用到本課。
//
// 主要 API 對照 (cppreference):
//   - std::scoped_lock (C++17)          https://en.cppreference.com/w/cpp/thread/scoped_lock
//   - std::lock                         https://en.cppreference.com/w/cpp/thread/lock
//   - std::try_lock                     https://en.cppreference.com/w/cpp/thread/try_lock
//   - std::adopt_lock_t / adopt_lock    https://en.cppreference.com/w/cpp/thread/lock_tag
//   - std::defer_lock_t / defer_lock    同上
//   - std::try_to_lock_t / try_to_lock  同上
//
// 練習建議:
//   - 拿 lesson 30 Q7 的 DiningPhilosophers,把 std::scoped_lock 改成
//     方案 A (全域排序,先鎖編號小的) 與方案 C (奇偶不對稱),測一樣的
//     200 餐 × 5 哲學家,看 latency 與正確性。
// =============================================================

/*
補充筆記：std::deadlock
  - deadlock 通常來自多把鎖的取得順序不一致。
  - std::scoped_lock 可同時鎖多個 mutex，降低手寫順序錯誤。
  - 設計上減少共享狀態或縮小鎖範圍，比事後偵測 deadlock 更可靠。
  - std::deadlock 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <array>


// -------------------------------------------------------------
// 我們的玩具:銀行帳戶。每個帳戶有自己的 mutex 與餘額。
// 「轉帳」要同時鎖住來源與目的地,才能保證沒有人在「中間
// 狀態」看到帳戶資料 (例如:錢已從 A 扣掉但還沒進 B)。
// -------------------------------------------------------------
struct Account {
    std::mutex mtx;
    long long  balance = 0;
};


// =============================================================
// 反面教材 ── 經典的兩鎖 deadlock
//
// 兩個執行緒同時做:
//    Thread X: transfer_deadly(a, b, ...)
//    Thread Y: transfer_deadly(b, a, ...)
//
// X 鎖了 a.mtx 在等 b.mtx;Y 鎖了 b.mtx 在等 a.mtx。
// 永遠不會結束。
//
// 為了避免 demo 程式真的卡死,我們 *不會* 真的呼叫這個函式;
// 寫在這裡只是作為對照組。
// =============================================================
[[maybe_unused]]
static void transfer_deadly(Account& a, Account& b, long long amount)
{
    std::lock_guard<std::mutex> la(a.mtx);
    std::this_thread::sleep_for(std::chrono::microseconds(10));   // 讓 race window 變大
    std::lock_guard<std::mutex> lb(b.mtx);
    a.balance -= amount;
    b.balance += amount;
}


// =============================================================
// 修法 1 ── std::scoped_lock(m1, m2)
//
// C++17 引入 std::scoped_lock,可以同時 *原子地* 鎖多個 mutex,
// 內部用 deadlock-free 演算法 (基本上就是 try-back-off 的工業
// 版本)。鎖的順序 *無關緊要*,絕對不會死鎖。
//
// 適合場景:鎖的數量是編譯期可知 (固定 2、3 把 mutex),這是
// 99% 的情況。
// =============================================================
static void transfer_scoped(Account& a, Account& b, long long amount)
{
    std::scoped_lock lk(a.mtx, b.mtx);    // ★ 一行解決
    a.balance -= amount;
    b.balance += amount;
}


// =============================================================
// 修法 2 ── try_lock + 退讓重試
//
// 自己手動 try_lock 第二把,失敗就放開第一把退一步重試。
// 比 scoped_lock 慢、寫起來囉唆,但有兩個優點:
//   - 可以自訂退讓策略 (回傳「busy」、放棄等)。
//   - 可以加超時 (try_lock_for) 防止 livelock。
//   - 鎖數量在執行期才知道時 (例如「動態算出要鎖哪幾個帳戶」)
//     仍然能寫。
// =============================================================
static void transfer_try(Account& a, Account& b, long long amount)
{
    while (true) {
        std::unique_lock<std::mutex> la(a.mtx);
        if (std::unique_lock<std::mutex> lb(b.mtx, std::try_to_lock); lb.owns_lock()) {
            a.balance -= amount;
            b.balance += amount;
            return;
        }
        // 沒鎖到 b ── 立刻放開 a,讓對方先做 (避免活鎖,有時
        // 加一點隨機 backoff 更好)。
        la.unlock();
        std::this_thread::yield();
    }
}


// =============================================================
// 修法 2.5 ── std::try_lock(m1, m2, ...) 自由函式 (C++11)
//
// 與 std::lock 同家族,但 *只試一輪*,不會阻塞:
//
//   int r = std::try_lock(m1, m2, m3);
//
//   - 全部都鎖到:回傳 -1。所有 mutex 此刻已上鎖,你需要自己負責
//     釋放 (用 std::adopt_lock 把所有權交給 RAII 包裝)。
//   - 第 k 把 (0-based) 失敗:回傳 k。失敗 *之前* 已成功鎖到的
//     m_0 ... m_{k-1} 會被 try_lock 自動 unlock,你拿到的是「乾淨
//     失敗」── 所以 livelock 之外不會有泄漏。
//
// 與 try_to_lock_t (lesson 21 修法 2 用的) 的差別:
//   - std::unique_lock(m, std::try_to_lock) ── 一把 mutex,給你 RAII
//   - std::try_lock(m1, m2, ...)            ── 多把 mutex,要你自己包
//
// 實務優勢:鎖數量在編譯期已知 (兩三把) 但你想要 *自定義退讓策略*
// (例如重試次數上限、隨機 backoff、收集失敗率指標) 時最合身。
// scoped_lock 把退讓邏輯黑盒化,try_lock 把它讓給你。
// =============================================================
static void transfer_try_free_fn(Account& a, Account& b, long long amount)
{
    int spin = 0;
    while (true) {
        // 一次嘗試鎖兩把。回 -1 表示全成功。
        int failed_at = std::try_lock(a.mtx, b.mtx);
        if (failed_at == -1) {
            // 用 std::adopt_lock 把已上鎖的 mutex 接手給 RAII
            // ── lock_guard 解構時會 unlock,我們不必手動 unlock。
            std::lock_guard<std::mutex> la(a.mtx, std::adopt_lock);
            std::lock_guard<std::mutex> lb(b.mtx, std::adopt_lock);
            a.balance -= amount;
            b.balance += amount;
            return;
        }
        // 拿不到 (failed_at = 0 或 1)。前面成功的鎖已被 try_lock
        // 自動 unlock,我們直接退讓重試。加一點 spin 上限避免極端
        // 情況下的 livelock (本 demo 不會觸發,但 production code
        // 應有此防線)。
        if (++spin > 1000) std::this_thread::sleep_for(
                                std::chrono::microseconds(1));
        else                std::this_thread::yield();
    }
}


// =============================================================
// 修法 3 ── 全域鎖排序 (address-based ordering)
//
// 規則:任何時候若需要鎖兩把以上的 mutex,*必須以固定順序*
// 來鎖。最簡單的固定順序就是用記憶體位址比大小。
//
// 比 scoped_lock 還快一點 (兩次普通的 lock,沒有 try-back-off
// 的開銷),但要求 *每一個* 會跨多鎖的程式路徑都遵守同樣的
// 排序規則 ── 不遵守就退化回 deadlock。在大型 codebase 裡,
// 用 address 比大小的「無腦排序」最不容易寫錯。
// =============================================================
static void transfer_ordered(Account& a, Account& b, long long amount)
{
    Account* first  = (&a < &b) ? &a : &b;
    Account* second = (&a < &b) ? &b : &a;
    std::lock_guard<std::mutex> l1(first->mtx);
    std::lock_guard<std::mutex> l2(second->mtx);
    a.balance -= amount;
    b.balance += amount;
}


// =============================================================
// 公平比較 ── 跑同樣的工作負載,只換 transfer 實作
// =============================================================
template <typename Fn>
static long long run(const char* label, Fn&& transfer,
                     std::array<Account, 8>& accts, int per_thread)
{
    // 每次重置初始餘額
    for (auto& a : accts) {
        std::lock_guard<std::mutex> lk(a.mtx);
        a.balance = 1'000'000;
    }

    auto t0 = std::chrono::steady_clock::now();

    constexpr int THREADS = 8;
    std::vector<std::thread> ts;
    for (int i = 0; i < THREADS; ++i) {
        ts.emplace_back([&accts, per_thread, i, &transfer]{
            std::mt19937 rng(static_cast<unsigned>(i));
            for (int k = 0; k < per_thread; ++k) {
                int x = rng() % accts.size();
                int y = rng() % accts.size();
                if (x == y) continue;
                transfer(accts[x], accts[y], 1);
            }
        });
    }
    for (auto& t : ts) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    long long total = 0;
    for (auto& a : accts) {
        std::lock_guard<std::mutex> lk(a.mtx);
        total += a.balance;
    }
    std::cout << "  [" << label << "] " << ms << " ms"
              << "  total balance = " << total
              << "  (expected " << (8LL * 1'000'000) << ")\n";
    return ms;
}


int main()
{
    std::array<Account, 8> accts{};
    constexpr int PER_THREAD = 50'000;

    std::cout << "8 threads, " << PER_THREAD
              << " random transfers each, between 8 accounts.\n\n";

    // 不跑 transfer_deadly!跑了會 deadlock。
    run("scoped_lock(a, b)        ", transfer_scoped,        accts, PER_THREAD);
    run("try_lock + retry (member) ", transfer_try,           accts, PER_THREAD);
    run("std::try_lock free fn     ", transfer_try_free_fn,   accts, PER_THREAD);
    run("address-ordered locking   ", transfer_ordered,       accts, PER_THREAD);

    // =====================================================
    // LeetCode 1226  Dining Philosophers
    // 難度: medium
    // =====================================================
    // 題意: 5 個哲學家坐圓桌, 每人左右兩根筷子 = 5 把 mutex。
    //       每人 wantsToEat() 呼叫時必須拿到自己左右兩根才能
    //       吃, 吃完還回去。要避免 deadlock + starvation。
    //
    // 解題思路:
    //   - 經典 deadlock 範例 ── 每人都先拿左、再拿右, 5 人同時
    //     拿到左手筷後就環形等待 → deadlock。
    //   - 最簡單解法: std::scoped_lock(left, right) 一次拿兩根,
    //     底層 try-and-back-off 保證無 deadlock。
    //   - 公平性: scoped_lock 不保證 FIFO, 但實務上不會餓死。
    //
    // 完整解在 lesson 30 Q7, 這裡示範核心結構 (每人吃 2 次)。
    // =====================================================
    {
        std::cout << "\n--- LC 1226 Dining Philosophers ---\n";
        std::array<std::mutex, 5> forks;
        std::atomic<int> meals{0};
        auto philosopher = [&](int p){
            std::mutex& left  = forks[p];
            std::mutex& right = forks[(p + 1) % 5];
            for (int i = 0; i < 2; ++i) {
                std::scoped_lock lk(left, right);   // ★ 一次拿兩根, 不會 deadlock
                meals.fetch_add(1);
                std::cout << "  P" << p << " eats #" << (i+1) << '\n';
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        };
        std::vector<std::thread> ts;
        for (int p = 0; p < 5; ++p) ts.emplace_back(philosopher, p);
        for (auto& t : ts) t.join();
        std::cout << "  total meals = " << meals.load()
                  << " (預期 10, 無 deadlock)\n";
    }

    // =====================================================
    // 實戰範例: 配對交易 (matching engine) 中的兩鎖場景
    // =====================================================
    // 應用場景: 撮合系統中, 一筆「賣 BTC 換 USD」要同時鎖住
    // BTC orderbook 跟 USD orderbook, 兩者都更新。多執行緒中
    // 任意買賣對都可能出現 ── 必須避免 deadlock。
    //
    // scoped_lock(btc_book.mtx, usd_book.mtx) 是最自然的寫法,
    // 兩本書一起鎖, 更新後一起釋放。實際撮合引擎內部會更複雜
    // (orderbook 自身用 sharded queue + sequencer), 但「兩本書
    // 一起鎖」這個 building block 隨處可見。
    // =====================================================
    {
        std::cout << "\n[demo] matching engine 兩鎖 (BTC + USD)\n";
        struct Book { std::mutex mtx; long long bal = 1'000'000; };
        Book btc, usd;
        auto trade = [&](long long btc_amt, long long usd_amt){
            std::scoped_lock lk(btc.mtx, usd.mtx);
            btc.bal -= btc_amt;
            usd.bal += usd_amt;
        };
        std::vector<std::thread> ts;
        for (int i = 0; i < 4; ++i) ts.emplace_back([&]{
            for (int k = 0; k < 1000; ++k) trade(1, 30000);
        });
        for (auto& t : ts) t.join();
        std::cout << "  btc=" << btc.bal << " usd=" << usd.bal
                  << " (4 thread × 1000 trade, 無 deadlock)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::scoped_lock 內部到底用什麼演算法保證不會 deadlock?
    //    A：它用 std::lock(m1, m2, ...) 的 try-and-back-off 演算法:lock 第
    //       一個 → try_lock 第二、第三...,任一失敗就 unlock 已拿到的全部、
    //       讓出 CPU、從另一個 mutex 開始重來。沒有「持有 + 等待」的窗口
    //       → 打破 Coffman 條件 B。理論上有 livelock 機率 (兩條 thread
    //       一直互相退讓),實務上 OS 排程不齊很快就解開。
    //
    //  Q2：什麼是 lockstep 問題?和 deadlock 怎麼差別?
    //    A：lockstep (livelock) 是兩條 thread 不停活動但互相退讓,系統
    //       無進展、CPU 100%。deadlock 則是大家全卡死、零 CPU 使用。
    //       try-lock 演算法若不加隨機 backoff 容易出現 lockstep:兩條
    //       thread 同步退讓 → 同步重試 → 又同步退讓。對策:加入隨機
    //       sleep_for(rand_us) 或限制重試次數後改成 lock() 強拿。
    //
    //  Q3：持鎖時呼叫 future.get() 為什麼是 deadlock 隱形殺手?
    //    A：你以為只持有一把 mutex,但 future.get() 可能阻塞等另一條
    //       thread 完成,而那條 thread 又在等你的 mutex → 環形等待形成。
    //       原則:critical section 內*只做純內部資料操作*,不呼叫任何
    //       會阻塞的東西 (cv.wait、future.get、send、callback)。把這些
    //       blocking call 移到 lock 之外,或改成 try_X 模式。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. Deadlock 的 *單一* 必要條件就是「鎖被以不一致的順序拿
//    取」。任何能消除這個條件的策略都能消除 deadlock。
//
// 2. 選擇地圖:
//      鎖數量編譯期已知,寫一段程式碼   -> std::scoped_lock
//      鎖數量執行期才知道                -> try_lock + retry,
//                                           或 std::lock(...)
//      整個專案級別的鎖管理              -> 全域鎖排序 hierarchy
//                                           (Boost 有 strict_lock
//                                            可協助強制檢查)
//      根本不想處理鎖的問題               -> 重新設計,讓你的
//                                           程式路徑只拿 *一把* 鎖
//                                           (sharding、message
//                                            passing、actor model)
//
// 3. std::scoped_lock 內部其實用的是 std::lock(m1, m2, ...)
//    的演算法 ── 它會嘗試 lock(m1)、try_lock(m2),失敗就 unlock,
//    換順序試,直到成功。理論上是 deadlock-free,實際上偶爾
//    會看到一點 livelock 行為 (兩條執行緒一直互讓),但加一點
//    backoff 後在實際工作負載中很穩定。
//
// 4. 全域鎖排序的「序」可以是:
//      - 記憶體位址 (本課用的,絕對通用)
//      - 階層編號 (1.network, 2.cache, 3.disk)
//      - lock ID 字串字典序
//    重點是 *全程式碼一致*。Linux 核心的 lockdep 子系統會在
//    執行期記錄每條獲取順序,發現任何破壞排序的取鎖都會在
//    debug build 印出警告。
//
// 5. 死鎖偵測工具:
//      - ThreadSanitizer (lesson 14):會發出 "lock-order
//        inversion" 警告。
//      - Helgrind (Valgrind):較重但更詳細。
//      - Linux lockdep (核心模式),不適用 user-space。
//      - clang-tidy 的 thread-safety analysis (annotation 風格)。
//
// 6. 一個常被忘掉的 deadlock 來源:*在持鎖時等待別人完成*。
//    例如「持有 mutex 時 .get() 一個 future,而那個 future
//    要靠另一條執行緒填,而那條執行緒在等同一把 mutex」 ──
//    這也是 deadlock,但很難看出來。預防原則:critical section
//    內 *只做純運算與資料結構操作*,不呼叫會阻塞的東西。
// =============================================================
