// =============================================================
// 12_cpp20_sync.cpp  --  C++20 新同步原語:latch / barrier /
//                        counting_semaphore / atomic wait+notify
// =============================================================
//
// 本課目標:
//   1. 用 std::latch 表達「等 N 件事都做好之後再走」
//      (一次性的倒數計時器)。
//   2. 用 std::barrier 表達「N 個執行緒每一輪都要對齊」
//      (可重複使用的會合點)。
//   3. 用 std::counting_semaphore 表達「同時最多 N 個」
//      (限流 / 資源池)。
//   4. 用 std::atomic<T>::wait + notify 表達「值改變了再
//      叫醒我」—— 不需要 mutex+CV,直接由核心管理等待。
//   5. 知道在哪些情境下,以上每一個都比手刻 mutex+CV 更短、
//      更精確、更不容易寫錯。
//
// 編譯方式 (需要 C++20):
//     g++ -std=c++20 -O2 -pthread 12_cpp20_sync.cpp -o 12_cpp20_sync
//
// 執行方式:
//     ./12_cpp20_sync
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     C++20 新同步原語 ── latch / barrier / semaphore / atomic.wait
// 前置課程: lesson 03, 04, 05
// 觀念詞彙:
//   - latch              ── 一次性倒數計數器
//   - barrier            ── 可重複會合點
//   - counting_semaphore ── 計數信號量,限流
//   - completion fn      ── barrier 所有人到達時跑一次的收尾函式
// 新介紹 API:
//   std::latch(n)                     初始計數
//   l.count_down(k=1) / .wait() / .arrive_and_wait()
//   std::barrier(n, completion_fn)
//   b.arrive_and_wait()
//   std::counting_semaphore<MAX>(init)
//   sem.acquire() / .release(k=1) / .try_acquire()
//   std::binary_semaphore = counting_semaphore<1>
//   atomic.wait(old) / .notify_one() / .notify_all()
// 何時使用:
//   - 等 N 件事都好 → latch
//   - N 個 worker 每輪對齊 → barrier
//   - 同時最多 N 個 → semaphore
//   - 等一個 atomic 值改變 → atomic.wait
// 何時不要用:
//   - 複雜共享狀態的條件等待 → mutex+CV (lesson 05)
//   - 要重置 latch → 它是一次性的,改用 barrier
// 常見錯誤:
//   - latch 期望可重置 → 不能,要重新建立
//   - barrier 數量誤算少了一個 → 永遠卡住
//   - semaphore 忘了 release → 計數慢慢用光
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── 各 sync primitive 的選擇樹
// =============================================================
//
// 1. 四個新工具的核心區別
//      std::latch                ── 一次性。N 條 thread arrive,湊齊就放行。不能重置。
//      std::barrier              ── 可重複。每輪有 completion lambda。
//                                   適合「N thread × 多輪同步」。
//      std::counting_semaphore   ── 計數 token,acquire/release。適合限流。
//      std::binary_semaphore     ── 同上,容量為 1。可代替 mutex 在「跨 thread
//                                   解鎖」場景 (mutex 必須由 lock 者解)。
//      std::atomic<T>::wait/notify ── 對 atomic 變數做「值改變」的 futex 等待。
//                                   比 cv 輕,但只能等「值是否相等」。
//
// 2. 怎麼選 (典型問題 → 工具)
//      「等所有 worker 啟動完才開始 main」              → latch
//      「N round simulation,每輪所有人到齊才下一輪」    → barrier
//      「同時最多 K 條進臨界區」                        → counting_semaphore<K>
//      「跨 thread 通知一次性事件」                      → binary_semaphore
//      「我有個 atomic flag,變了想被叫醒」              → atomic.wait/notify
//      「複合條件 (queue 非空且未滿)」                   → cv (lesson 05)
//
// 3. latch 的 arrive_and_wait vs count_down
//      arrive_and_wait()  ── 計數 -1 並阻塞至歸零
//      count_down(n=1)    ── 計數 -n,*不阻塞* (常用於 main thread 通知)
//      wait()             ── 純等到歸零
//    典型:N 個 worker 的 main 想等它們開始,workers 在啟動最後 count_down,
//    main 用 wait()。或 main 用 count_down(N) 通知 N 個 worker 開始衝。
//
// 4. barrier 的 completion lambda
//    barrier 在 *最後一個 arrive 的 thread* 上執行 completion lambda,
//    然後才釋放所有人。常用於:
//      - 重置共享狀態 (例如把每輪統計清零)
//      - 印階段標題
//      - 在「全員都到」的特殊時刻做唯一一次的決策
//    在 lesson 30 的 H2O 題目就利用這點:湊齊 2H+1O 後 completion lambda
//    補回 semaphore 的 slot。
//
// 5. counting_semaphore 的範本參數是「最大值」
//    std::counting_semaphore<8> sem(3);
//      最大可累積到 8;初值是 3。release 超過 max → UB。
//    binary_semaphore 是 counting_semaphore<1> 的別名。
//
// 6. atomic::wait 是 cv 的「窮人版」也是「省人版」
//    底層是 futex (Linux)、WaitOnAddress (Windows)。比 cv 省一把 mutex,
//    省一次 user-kernel 切換。但只能等「值不等於 X」這個簡單條件,複合條件
//    (例如「不為空 *且* 未關閉」) 你只能模擬。一般低層工具/性能熱點才用。
//
// 7. semaphore 沒有 priority、沒有 fairness 保證
//    多個 acquirer 等同個 semaphore,實作不保證順序。要嚴格 FIFO 自己用
//    queue + cv 做。
//
// 8. 用 semaphore 做 mutex (跨 thread 解鎖)
//    mutex 規定 *誰 lock 誰 unlock*,不能 thread A 拿,thread B 解。
//    binary_semaphore 沒這限制 → 適合「producer 把資源餵進來,consumer
//    用完後 release」這類流程 (例如 lesson 30 Q1 print-in-order)。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目 (本課工具是 C++20 解法的最佳武器):
//   ✓ LC 1114  Print in Order
//     - 兩個 binary_semaphore 一次性閘門,直觀解。
//     - 完整解答 → lesson 30 Q1
//   ✓ LC 1115  Print FooBar Alternately
//     - 雙 binary_semaphore 互推 ── 比 cv 解短 50%。
//     - 完整解答 → lesson 30 Q2 (sem 版)
//   ✓ LC 1116  Print Zero Even Odd
//     - 三個 binary_semaphore 環狀,zero 推 odd/even,各推 zero。
//     - 完整解答 → lesson 30 Q3
//   ✓ LC 1117  Building H2O
//     - counting_semaphore 限類數量 + barrier 湊齊放行 + completion
//       lambda 補回 slot ── C++20 三件套合作的教科書範例。
//     - 完整解答 → lesson 30 Q4
//
// 主要 API 對照 (cppreference):
//   - std::latch                        https://en.cppreference.com/w/cpp/thread/latch
//   - std::barrier                      https://en.cppreference.com/w/cpp/thread/barrier
//   - std::counting_semaphore           https://en.cppreference.com/w/cpp/thread/counting_semaphore
//   - std::binary_semaphore             同上 (counting_semaphore<1> 別名)
//   - std::atomic<T>::wait / notify     https://en.cppreference.com/w/cpp/atomic/atomic/wait
//
// 練習建議:
//   讀完本課,馬上去 lesson 30 看 Q1/Q2/Q3/Q4 ── 你會發現有了 C++20
//   sync 工具,LC 1114-1117 全部都比 cv 解短一截。
// =============================================================

/*
補充筆記：cpp20_sync
  - cpp20_sync 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - cpp20_sync 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++20 同步原語：latch / barrier / semaphore / atomic wait
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++20 新增了哪些同步原語？各解決什麼問題？
//     答：std::latch 是一次性倒數門閂，N 個事件完成後放行，不可重複使用
//         （count_down / wait）。std::barrier 是可重複使用的會合點，N 個 thread 每輪
//         都要到齊才一起前進，並可指定每輪的 completion function，適合迭代式平行運算。
//         std::counting_semaphore<N> / binary_semaphore 用於限流與資源池。
//         std::atomic<T>::wait / notify_one / notify_all 提供「值改變才喚醒」的輕量
//         等待，不需要 mutex。
//     追問：latch 與 barrier 差在哪？（一次性 vs 可重用；barrier 還有每輪的完成回呼）
//
// 🔥 Q2. semaphore 與 condition_variable 差在哪？為什麼 semaphore 沒有 lost wakeup？
//     答：CV 沒有記憶——notify 時若無人等待，通知就消失，所以必須搭配 mutex 保護的
//         狀態變數與 predicate。semaphore 內部有「計數」，先 release 後 acquire 仍然
//         有效，計數會被保留，因此不需要外部 mutex 也不會遺失通知。代價是它只能表達
//         「可用資源數」，無法表達任意複雜的條件。
//     追問：binary_semaphore 可以當 mutex 用嗎？（可以表達互斥，但沒有「擁有者」概念，
//           可被別的 thread release，且不支援 RAII 慣例，語意上不建議混用）
//
// Q3. atomic<T>::wait/notify 與自己寫 spin 迴圈差在哪？
//     答：spin 迴圈持續佔用 CPU；wait 則在確認值未變後讓 thread 進入阻塞（實作上通常
//         走作業系統的等待機制，如 Linux futex），有等待者才需要被 notify 喚醒。它比
//         mutex+CV 輕，因為不需要額外的 mutex 與 predicate 狀態，但只能等待「某個原子
//         變數的值不再等於舊值」這種簡單條件。
//     追問：notify 沒有等待者時的成本？（實作通常可用內部旗標避免不必要的系統呼叫，
//           但這屬於實作細節，標準未規定）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <latch>           // std::latch
#include <barrier>         // std::barrier
#include <semaphore>       // std::counting_semaphore, std::binary_semaphore

// =============================================================
// PART 1 —— std::latch
//
// 概念:一個「一次性倒數計數器」。建構時給定初始值 N。
//   .count_down(k)   讓計數減 k (預設 1)。
//   .wait()          阻塞,直到計數歸零。
//   .arrive_and_wait()  count_down(1) 之後再 wait。
//
// 用 latch 取代 mutex+CV+counter 這個老樣板:
//   - 老寫法要一個 mutex、一個 CV、一個 int counter,還要
//     在「counter==0」時 notify_all。
//   - latch 用一行表達同樣的意思,而且無法寫錯
//     (沒有 lost wakeup、沒有 spurious wakeup 要處理)。
//
// 經典用途:多階段啟動、平行 benchmark 同步起跑、ParallelGo
// 風格的「等所有 worker 初始化完成」。
//
// 注意:latch 是 *一次性* 的。歸零之後不能重置 —— 要重複
// 使用就改用 barrier。
// =============================================================
static void demo_latch()
{
    std::cout << "PART 1: std::latch (一次性集合點)\n";

    constexpr int N = 4;
    std::latch ready(N);          // 初始計數 = 4

    std::vector<std::jthread> workers;
    for (int i = 0; i < N; ++i) {
        workers.emplace_back([i, &ready]{
            // 假裝每個 worker 初始化所需時間不同。
            std::this_thread::sleep_for(std::chrono::milliseconds(50 * (i + 1)));
            std::cout << "  [worker " << i << "] init done\n";
            ready.count_down();   // 我這份初始化完了
        });
    }

    std::cout << "  [main] waiting for all " << N << " workers to init...\n";
    ready.wait();                 // 阻塞,直到所有 worker 都 count_down
    std::cout << "  [main] all ready, proceeding\n\n";

    // jthread 在 vector 解構時自動 join
}


// =============================================================
// PART 2 —— std::barrier
//
// 概念:可 *重複使用* 的 N 路會合點。
//   .arrive_and_wait()  把計數減 1,並阻塞到全部 N 個都到達。
//                       到達後計數會自動重置,可進入下一輪。
//
// 還能傳入一個「completion function」,在所有人都到達、但
// 還沒被釋放前執行 —— 用來做 phase 之間的單執行緒收尾
// (例如:把每個 worker 的部分結果合併、印出本輪統計、
// 切換到下一個資料區段)。
//
// 經典用途:lock-step 模擬、平行迭代演算法、shader 風格的
// SIMT compute、需要「每一輪所有人對齊一次」的工作。
// =============================================================
static void demo_barrier()
{
    std::cout << "PART 2: std::barrier (可重複會合點)\n";

    constexpr int N = 3;
    int round = 0;

    std::barrier sync_point(
        N,
        [&round]() noexcept {
            // 所有 worker 抵達時 *只有一個執行緒* 會跑這個 lambda
            // (由實作決定是哪一個),且在大家被釋放前完成。
            ++round;
            std::cout << "  [barrier] >>> round " << round
                      << " complete, releasing all\n";
        }
    );

    std::vector<std::jthread> workers;
    for (int i = 0; i < N; ++i) {
        workers.emplace_back([i, &sync_point]{
            for (int r = 0; r < 3; ++r) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30 + i * 20));
                std::cout << "    [worker " << i << "] phase work for round "
                          << (r + 1) << " done\n";
                sync_point.arrive_and_wait();   // 所有人都到才繼續
            }
        });
    }

    // jthread join,然後印一個分隔
    workers.clear();
    std::cout << '\n';
}


// =============================================================
// PART 3 —— std::counting_semaphore
//
// 概念:一把帶有 *計數* 的鎖。
//   .acquire()       計數 -1,若已是 0 則等到有 release。
//   .release(k = 1)  計數 +k,可能喚醒等待者。
//   .try_acquire()   非阻塞嘗試。
//
// 模板參數是「最大計數」(編譯期常數)。常見用法:
//   std::counting_semaphore<3> sem(3);  // 同時最多 3 個進去
//
// 用 semaphore 取代 mutex+CV+counter 這個樣板,優勢:
//   - 不需要自己寫「if 還有空位才放進來」的邏輯。
//   - 沒有 race window;acquire/release 是 atomic 的。
//
// std::binary_semaphore = std::counting_semaphore<1>,作為
// 最小化的「mutex 替代品」(但拿與放可以發生在不同執行緒
// 上 —— 一般 mutex 不允許)。
//
// 經典用途:限流連線數、限制同時的下載數、實作有界生產者
// /消費者佇列、做為「ready 信號」(release 一次給對方)。
// =============================================================
static void demo_semaphore()
{
    std::cout << "PART 3: std::counting_semaphore (限流到 3)\n";

    constexpr int SLOTS = 3;
    std::counting_semaphore<SLOTS> sem(SLOTS);

    std::atomic<int> active{0};
    std::atomic<int> peak{0};

    std::vector<std::jthread> workers;
    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([i, &sem, &active, &peak]{
            sem.acquire();                       // 拿一個槽位
            int now = ++active;
            // 更新觀察到的最大同時在場數 (CAS 迴圈)
            int p = peak.load();
            while (now > p && !peak.compare_exchange_weak(p, now)) {}
            std::cout << "    [worker " << i << "] inside, active=" << now << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            --active;
            sem.release();                       // 把槽位還回去
        });
    }
    workers.clear();   // 等全部結束 (jthread 解構即 join)

    std::cout << "  [semaphore] peak concurrency observed = "
              << peak.load() << " (limit = " << SLOTS << ")\n\n";
}


// =============================================================
// PART 4 —— std::atomic<T>::wait / notify_one / notify_all
//
// C++20 給每個 std::atomic 加上了「等到值改變」的能力,
// 由作業系統的 futex (Linux)、ulock (macOS)、WaitOnAddress
// (Windows) 等核心級原語實作 —— 跟 std::mutex 內部用的是
// 同一套機制。
//
//   x.wait(old)        若 x.load() == old,則睡到值不再等於 old。
//                      支援可選 memory_order。
//   x.notify_one()     喚醒一個正在 .wait 的執行緒。
//   x.notify_all()     喚醒所有。
//
// 為什麼有時這比 mutex+CV 更好:
//   - 沒有 mutex 要管。如果你的「狀態」剛好就是一個 atomic
//     值 (旗標、計數、版本號),這寫法乾淨得多。
//   - 你不必準備 predicate;wait(old) 會幫你過濾假醒。
//   - 在 contention 低時甚至不需要進核心 (fast path)。
//
// 何時 *不要* 用:
//   - 共享狀態複雜 (vector、map、多個欄位)、需要在等待中同時
//     檢查多個條件 —— 那些情境 mutex+CV 仍然合適。
// =============================================================
static void demo_atomic_wait()
{
    std::cout << "PART 4: std::atomic<T>::wait + notify\n";

    std::atomic<int> stage{0};   // 0 = 未準備, 1 = go, 2 = 完成

    std::jthread worker([&stage]{
        std::cout << "  [worker] sleeping until stage != 0 (kernel-level wait)\n";
        stage.wait(0);                       // 阻塞,完全 0 CPU
        std::cout << "  [worker] woke up, stage = " << stage.load() << '\n';

        // 第二輪
        stage.wait(1);
        std::cout << "  [worker] woke up again, stage = " << stage.load() << '\n';
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    stage.store(1);
    stage.notify_one();                      // 喚醒第一次 wait

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    stage.store(2);
    stage.notify_one();                      // 喚醒第二次 wait

    // jthread 解構自動 join
}


// =============================================================
// 實戰範例: counting_semaphore 當 API rate limiter
// =============================================================
// 應用場景: 我要呼叫某個第三方 API, 但他規定「同時最多 3 個
// 連線」。如果我有 N 條 worker thread 都想呼叫, 必須做限流。
// counting_semaphore 是天生的工具:
//   acquire() 拿許可, 沒了就阻塞;
//   release() 用完還回去。
//
// 比起手刻「mutex + counter + cv」, 這個版本不用維護任何狀態,
// 一行程式就限到上限。也比 thread pool 簡單 ── thread pool 限
// 制「同時跑幾條 worker」, semaphore 限制「同時持有多少資源」,
// 兩個維度互補。
// =============================================================
static void demo_rate_limiter()
{
    std::cout << "PART 5 (extra): API rate limiter\n";
    constexpr int CONCURRENCY = 3;
    std::counting_semaphore<CONCURRENCY> sem(CONCURRENCY);
    std::atomic<int> inflight{0}, peak{0};

    auto call_api = [&](int who){
        sem.acquire();
        int cur = ++inflight;
        int p = peak.load();
        while (cur > p && !peak.compare_exchange_weak(p, cur)) {}
        std::cout << "    [req " << who << "] inflight=" << cur << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        --inflight;
        sem.release();
    };

    std::vector<std::jthread> reqs;
    for (int i = 0; i < 10; ++i) reqs.emplace_back(call_api, i);
    reqs.clear();   // 等全部完成
    std::cout << "  peak inflight = " << peak.load()
              << " (limit = " << CONCURRENCY << ")\n\n";
}

// =============================================================
// LeetCode 1117  Building H2O
// 難度: medium
// =============================================================
// 題意: H thread 與 O thread 不斷呼叫 hydrogen()/oxygen(),
//       每湊齊 2H + 1O 必須在「分子內」一起印, 再進下一輪。
//
// 解題思路 (semaphore + barrier 經典):
//   - sem_h(2): 一輪最多放 2 個 H 進場
//   - sem_o(1): 一輪最多放 1 個 O 進場
//   - barrier(3): 三個到齊才印出, 自動進下一輪
//
// 為什麼用 barrier 而不是 latch: H/O 是 *連續多輪* 的同步,
// 每輪結束 barrier 自動重置, latch 只能用一次。
//
// 完整解在 lesson 30 Q4, 這裡示範核心結構 (跑 2 輪 = 4H + 2O)。
// =============================================================
static void demo_h2o()
{
    std::cout << "PART 6 (extra): LC 1117 Building H2O\n";
    std::counting_semaphore<2> sem_h(2);
    std::counting_semaphore<1> sem_o(1);
    std::barrier              br(3);   // 3 = 2H+1O

    auto hydrogen = [&](int id){
        sem_h.acquire();
        std::cout << "    H" << id << ' ' << std::flush;
        br.arrive_and_wait();   // 等湊齊一個分子
        sem_h.release();
    };
    auto oxygen = [&](int id){
        sem_o.acquire();
        std::cout << "O" << id << ' ' << std::flush;
        br.arrive_and_wait();
        std::cout << "(molecule done)\n";   // 由 O thread 印
        sem_o.release();
    };

    // 2 輪 = 4H + 2O
    std::vector<std::jthread> ths;
    for (int i = 0; i < 4; ++i) ths.emplace_back(hydrogen, i);
    for (int i = 0; i < 2; ++i) ths.emplace_back(oxygen,   i);
    ths.clear();
    std::cout << '\n';
}


// =============================================================
// MAIN
// =============================================================
int main()
{
    demo_latch();
    demo_barrier();
    demo_semaphore();
    demo_atomic_wait();
    demo_rate_limiter();
    demo_h2o();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：latch 跟 barrier 的最大差別是什麼?
    //    A：latch 是「一次性」── count 倒數歸零後就死, 想再用必須建一
    //       個新的。barrier 是「可重複」── 每輪所有 thread 到齊後重置
    //       counter, 進入下一輪。fork-join (一次匯合) 用 latch; 迴圈式
    //       (每個 epoch 同步一次, 例如 SIMD 模擬、ML mini-batch) 用
    //       barrier。用錯就會打架: latch 拿來做 epoch 同步會壞。
    //
    //  Q2：counting_semaphore 跟 mutex 差在哪?
    //    A：mutex 是「最多 1 個持有者 + 必須同一 thread 釋放」, semaphore
    //       是「最多 N 個許可 + 任何 thread 都能 release」。所以
    //       semaphore 適合「限流」(例如最多 5 條 thread 同時呼叫 API)、
    //       「producer/consumer 槽位計數」、「跨 thread 訊號傳遞」。
    //       binary_semaphore (count=1) 像 mutex, 但允許 thread A lock、
    //       thread B unlock, 這是 mutex 嚴禁的。
    //
    //  Q3：std::atomic<T>::wait/notify 跟 condition_variable 差在哪?
    //    A：cv 必須搭配 mutex + predicate, 寫法繁瑣但語意清楚。atomic.
    //       wait 直接由 OS futex 管理, 沒 mutex, 條件就是「值跟我傳進
    //       去的不同了嗎」, 適合「單一變數變了再叫醒我」(例如 ref-
    //       count 歸零、旗標翻轉)。複雜條件還是 cv 好寫, 也避免假醒。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 選擇地圖。
//      想表達的事                                     用什麼
//      「等 N 個一次性事件都完成」                    std::latch
//      「N 個 worker 每輪都要對齊一次」               std::barrier
//      「同時最多 N 個進來」                          std::counting_semaphore
//      「等這個 atomic 值不再是 X」                   x.wait(X)
//      「複雜共享狀態的等待 + 條件」                  mutex + CV (lesson 05)
//
// 2. latch 一次性,barrier 可重用。寫錯這個會讓你的 barrier
//    第二輪卡住,或讓你的 latch 沒辦法重置。記住:l atch 是
//    L 開頭、像「Last Once」。
//
// 3. semaphore 的 acquire / release 可以發生在 *不同的*
//    執行緒上。這是它與 mutex 最大的差別,也讓它能用來做
//    「非對稱訊號 (一邊發、另一邊等)」。
//
// 4. atomic::wait 不是 std::condition_variable 的替代品。
//    它的條件只能是「這個 atomic 的值」。複雜條件仍然需要
//    mutex+CV 的 predicate 形式。
//
// 5. barrier 的 completion function 在所有人到達、但還沒
//    放行之前由 *單一* 執行緒執行。這是放「合併部分結果」、
//    「換 phase 用的資料區段」、「印 phase 統計」的好地方。
//
// 6. 這些原語底層大多是 futex / WaitOnAddress —— 與 mutex
//    用的是同一套 kernel 機制,所以效能上跟手刻的 mutex+CV
//    在同一個量級,不會更慢,寫起來只會更短。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 12_cpp20_sync.cpp -o 12_cpp20_sync

// === 預期輸出 (節錄) ===
// PART 1: std::latch (一次性集合點)
//   [main] waiting for all 4 workers to init...
//   [worker 0] init done
//   [worker 1] init done
//   [worker 2] init done
//   [worker 3] init done
//   [main] all ready, proceeding
//
// PART 2: std::barrier (可重複會合點)
//     [worker 0] phase work for round 1 done
//     [worker 1] phase work for round 1 done
//     [worker 2] phase work for round 1 done
//   [barrier] >>> round 1 complete, releasing all
//     [worker 0] phase work for round 2 done
//     [worker 1] phase work for round 2 done
//     [worker 2] phase work for round 2 done
//   [barrier] >>> round 2 complete, releasing all
//     [worker 0] phase work for round 3 done
//     [worker 1] phase work for round 3 done
//     [worker 2] phase work for round 3 done
// …（後略，完整輸出共 53 行）
