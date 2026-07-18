// =============================================================
// 04_atomic.cpp  --  以 std::atomic 進行無鎖 (lock-free) 計數
// =============================================================
//
// 本課目標:
//   1. 學會 std::atomic<T> —— 一種型別,其上的操作從每個
//      執行緒看來都是不可分割的。
//   2. 看到對於單純的計數器,atomic 比 mutex *既更簡單也更快*。
//   3. 理解 *什麼時候* 該用 atomic、什麼時候用 mutex。
//      提示:atomic 保護 *單一變數*;mutex 保護一段會碰到
//      多個彼此相關狀態的程式碼。
//   4. 對「記憶體順序 (memory ordering)」做第一次的初步認識
//      —— 為何 std::atomic 看似簡單卻有那麼多旋鈕,以及為何
//      預設值 (sequentially consistent) 對初學者就是正確選擇。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 04_atomic.cpp -o 04_atomic
//
// 執行方式:
//     ./04_atomic
//
// 比較兩種寫法的時間:
//     time ./03_mutex
//     time ./04_atomic
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     用 std::atomic 寫 lock-free 計數
// 前置課程: lesson 02, 03
// 觀念詞彙:
//   - atomic        ── 操作不可分割,所有 thread 看到的是「全有或全無」
//   - lock-free     ── 不需要核心介入、不會被 OS 暫停
//   - memory order  ── 控制操作前後的可見性順序 (進階,lesson 11)
//   - RMW           ── read-modify-write,如 fetch_add
// 新介紹 API:
//   std::atomic<T>             T 必須 trivially-copyable
//   .load() / .store(v)        讀 / 寫
//   .fetch_add(v) / .fetch_sub(v)   原子加減,回傳舊值
//   .exchange(v)                原子交換,回傳舊值
//   .compare_exchange_weak/strong  CAS,lock-free 的核心
//   ++a / a += k 等運算子       方便語法,內部呼叫 fetch_*
// 何時使用:
//   - 單一變數的 +1、旗標、版本號、小型統計
// 何時不要用:
//   - 多欄位要一起更新 (帳戶轉帳) → mutex
//   - 操作邏輯比一個 fetch_add 複雜 → mutex
// 常見錯誤:
//   - 把 atomic<T> 當「原本物件 + 一道鎖」用,卻仍分多步操作
//   - 自作主張用 memory_order_relaxed → race 又回來
//   - atomic<bool> 配 plain int 共享狀態 → 仍要 release/acquire
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── atomic 在硬體層真正做什麼
// =============================================================
//
// 1. 「lock-free」的真正含義
//    lock-free *不是* 「沒有同步成本」── 它是「不會用到 OS mutex /
//    futex」。硬體層仍要做事:x86 的 atomic RMW 用 LOCK 前綴,把整條
//    cache line 鎖到自己核上 (MESI Modified 狀態) 直到指令完成,期間
//    其他核的同 line 操作必須等。ARM 用 LL/SC (load-linked/store-
//    conditional) ── 樂觀做完事後 store 一次,失敗就重試。
//
//    所以「lock-free 的 atomic」也會在熱資料上排隊,只是排在硬體
//    coherence protocol,不是排在 OS scheduler。
//
// 2. is_lock_free vs is_always_lock_free
//    is_lock_free()           執行期判斷 (這個物件、這個對齊)
//    is_always_lock_free      編譯期常數 (這個 *型別* 在這個 ABI 永遠是)
//    在 x86-64 上:atomic<int>、atomic<long>、atomic<T*> 都 always lock-
//    free。atomic<__int128> 看 ABI;atomic<std::shared_ptr<T>> *不是* (見
//    lesson 18)。沒對齊或大過 64 bit 容易退化成有 mutex 的 fallback。
//
// 3. CAS (compare_exchange) 的兩個版本
//    compare_exchange_strong:即使 expected 與 actual 相等也保證成功
//                             (除非 race);loop 外用最直觀。
//    compare_exchange_weak:   允許 *spurious failure* (ARM LL/SC 重試
//                             失敗時直接回 false 不重試),適合放在迴圈
//                             內 (loop 自然會 retry)。
//    經典 pattern:
//        T expected = atomic.load(...);
//        do { /* compute new from expected */ }
//        while (!atomic.compare_exchange_weak(expected, new, ...));
//    expected 是 in/out 參數:失敗時 expected 會被寫成「現在的值」,
//    省掉一次 load。
//
// 4. 為什麼 fetch_add 比 mutex 快很多?
//    無爭用 fetch_add (x86 lock add):~5-15 ns
//    無爭用 mutex lock+unlock:        ~15-25 ns
//    爭用 fetch_add (8 thread 拼一條 line):會退化到 ~50-200 ns/op,
//        因為 cache line 在 8 個核之間反覆 invalidate (cache-line
//        ping-pong)。但仍 *沒有 syscall、沒有 context switch*。
//    爭用 mutex 一旦進 futex:~1-3 µs/op + context switch tax。
//    所以對「單變數 + 簡單 op」atomic 永遠是首選。對「多欄位需一致更新」
//    mutex 才合適。
//
// 5. atomic 不是 fence
//    `atomic<int> x;  x = 1;  some_other_var = 2;`
//    x 的 store 是原子的,但 x 與 some_other_var 之間的順序由 memory
//    order 決定。預設 memory_order_seq_cst → 全 thread 看到一致順序;
//    若你寫 memory_order_relaxed,編譯器與 CPU 都能任意重排兩個 store。
//    Lesson 11 詳細處理。
//
// 6. atomic<bool> 不能取代 mutex
//    `if (!flag) { do_init(); flag = true; }` 兩條 thread 都會進 do_init。
//    bool flag 不是「鎖」,只是個 bit。要互斥就用 mutex 或 call_once
//    (lesson 15)。
//
// 7. true sharing vs false sharing 都會痛
//    true sharing:兩條 thread 真的在改同一個 atomic 變數 → cache line
//                  ping-pong,沒救,只能設計上避免 (sharding,lesson 17)。
//    false sharing:兩個無關的 atomic 剛好落在同一條 cache line
//                   (~64 bytes),仍會 ping-pong → 加 alignas(64) padding
//                   分開 (lesson 13)。
//
// 8. atomic 與 trivially-copyable 的關係
//    std::atomic<T> 要求 T 是 trivially-copyable (memcpy 安全)。如果你
//    塞 std::string 進去:UB 或無法編譯。要塞 string 一定得用 mutex
//    或 atomic<shared_ptr<string>> (lesson 18 CoW pattern)。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✓ LC 1242  Web Crawler Multithreaded
//     - in_flight 計數用 atomic<int> 來做 termination detection。
//     - 完整解答 → lesson 30 Q8
//   ✓ LC 1188  Design Bounded Blocking Queue
//     - size() 若要快速查可用 atomic counter (但本課 30 Q5 是用 mutex+queue.size())。
//     - 完整解答 → lesson 30 Q5
//   → 多數 LC concurrency 題目都會混用 atomic counter (active worker、
//     completion flag) 與 mutex/cv,本課的 atomic 是基本零件。
//
// 主要 API 對照 (cppreference):
//   - std::atomic<T>                    https://en.cppreference.com/w/cpp/atomic/atomic
//   - std::atomic::load / store         https://en.cppreference.com/w/cpp/atomic/atomic/load
//   - std::atomic::fetch_add / sub      https://en.cppreference.com/w/cpp/atomic/atomic/fetch_add
//   - std::atomic::exchange             https://en.cppreference.com/w/cpp/atomic/atomic/exchange
//   - std::atomic::compare_exchange_*   https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
//   - std::atomic::is_lock_free         https://en.cppreference.com/w/cpp/atomic/atomic/is_lock_free
//
// 練習建議:
//   - 試著拿 lesson 30 Q5 的 BoundedBlockingQueue,把 size() 用 atomic
//     counter 取代「mutex + q_.size()」,觀察 size() 變得多便宜。
//   - 進階:讀 lesson 11 把 fetch_add 從預設 seq_cst 改成 relaxed,
//     測量加速。
// =============================================================

/*
補充筆記：std::atomic
  - atomic 適合單一變數的同步操作，不等於可保護多欄位不變式。
  - ++atomic 是原子 read-modify-write，但多個 atomic 組合仍可能有邏輯 race。
  - memory_order 只在你能證明 happens-before 時才手動調整；初學先用預設 sequentially consistent。
  - std::atomic 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <thread>
#include <atomic>     // std::atomic
#include <chrono>     // 用來計時兩種做法
#include <vector>

// -------------------------------------------------------------
// std::atomic<long long>
//
// 你可以這樣讀它:「一個 long long,可以被多個執行緒同時
// 讀、寫、遞增,且 *不會* 發生資料競爭」。
//
// 為何不需要 mutex 也能做到?
//   - 在 x86 上,編譯器會發出單一條 LOCK 前綴的 CPU 指令
//     (例如 `lock xadd`),硬體保證它在所有核心之間以
//     不可分割的方式執行。
//   - 在 ARM 上,則會展開成 load-linked / store-conditional
//     的迴圈。
//   - 兩種情況下,都不需要呼叫核心、也沒有任何執行緒被
//     送去睡覺 —— 這就是「無鎖 (lock-free)」的含義。
//
// 代價是 *一條* 帶有記憶體屏障的指令,而不是 std::mutex
// 在競爭嚴重時的數十條指令外加一次可能的上下文切換。
// -------------------------------------------------------------
std::atomic<long long> counter{0};

void increment(int times)
{
    for (int i = 0; i < times; ++i) {
        // 對 std::atomic 做 ++counter,實際上會呼叫
        // .fetch_add(1)。這是 *一次* 不可分割的「讀-改-寫」,
        // 不像 lesson 02 中的純 `++counter` 是三個分開的步驟。
        ++counter;
    }
}

// -------------------------------------------------------------
// 實戰範例 1: atomic<bool> 當 graceful-shutdown flag
// -------------------------------------------------------------
// 應用場景: 背景 worker thread 不斷處理工作, 直到主程式告訴
// 它「該下班了」。最簡單的訊號就是一個共享 bool, 主程式設
// done = true, worker 在迴圈頂端檢查。
//
// 為什麼要 atomic<bool> 而不是 plain bool?
//   - plain bool 寫 + 另一條 thread 讀 = data race = UB
//   - 編譯器看到 plain bool 在迴圈中沒被本 thread 改, 可能直接
//     hoist 出迴圈, while(!done) 變成 while(true) 永遠不停。
//   - atomic<bool> 保證 visibility + 編譯器不會 hoist。
//
// 注意: 這只是 *最簡單* 的 stop pattern; C++20 有更完整的
// std::jthread + stop_token (lesson 07) 與 std::stop_source。
// -------------------------------------------------------------
std::atomic<bool> shutdown_flag{false};

void background_worker()
{
    int loops = 0;
    while (!shutdown_flag.load()) {
        // 假裝做事
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++loops;
    }
    std::cout << "  worker stopped after " << loops << " loops\n";
}

void demo_atomic_shutdown()
{
    std::cout << "\n[demo] atomic<bool> 當 shutdown flag\n";
    shutdown_flag.store(false);
    std::thread w(background_worker);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    shutdown_flag.store(true);   // 發訊號
    w.join();
}

// -------------------------------------------------------------
// 實戰範例 2: 進度條 / 多 worker 共享計數
// -------------------------------------------------------------
// 應用場景: N 條 worker 一起處理 total 筆任務, 主執行緒每隔
// 一段時間印出進度。每完成一筆 worker 就 progress.fetch_add(1)。
// 主執行緒 progress.load() 算百分比, 不用持任何鎖。
//
// 這就是「分散更新, 集中讀」── atomic 在這個 pattern 上是
// 最佳工具 (mutex 反而會把所有 worker 序列化)。
// -------------------------------------------------------------
void chunk_worker(std::atomic<int>& progress, int items)
{
    for (int i = 0; i < items; ++i) {
        // 模擬處理一筆: 0.5 ms
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        progress.fetch_add(1, std::memory_order_relaxed);
    }
}

void demo_progress_counter()
{
    std::cout << "\n[demo] 進度條: 4 worker, 每人 50 筆\n";
    std::atomic<int> progress{0};
    const int per_worker = 50;
    const int total = per_worker * 4;

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back(chunk_worker, std::ref(progress), per_worker);

    // 主執行緒不持鎖, 直接 load 進度
    while (progress.load() < total) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        int p = progress.load();
        std::cout << "  進度 " << (p * 100 / total) << "% ("
                  << p << "/" << total << ")\n";
    }
    for (auto& t : workers) t.join();
    std::cout << "  完成: progress = " << progress.load() << '\n';
}

int main()
{
    const int N = 1'000'000;

    auto t_start = std::chrono::steady_clock::now();

    std::thread t1(increment, N);
    std::thread t2(increment, N);
    t1.join();
    t2.join();

    auto t_end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  t_end - t_start).count();

    // -----------------------------------------------------
    // 讀取一個 atomic。
    //
    // `counter.load()` 是明確且建議使用的寫法。
    // 你也可以直接寫 `counter`,它會隱式呼叫 .load(),但
    // 把 .load() 寫出來能讓未來閱讀程式碼的人一眼看出
    // 這裡有同步行為。
    // -----------------------------------------------------
    std::cout << "Expected counter = " << (2LL * N)        << '\n';
    std::cout << "Actual   counter = " << counter.load()   << '\n';
    std::cout << "Elapsed time     = " << ms << " ms\n";

    // ---------------------------------------------------------
    // 兩個延伸示範
    // ---------------------------------------------------------
    demo_atomic_shutdown();
    demo_progress_counter();

    // ---------------------------------------------------------
    // ATOMIC vs. MUTEX —— 該用哪一個
    //
    // 用 std::atomic 的時機:
    //   - 你保護的是 *單一* 變數 (一個計數器、一個旗標、
    //     一個指向新建物件的指標)。
    //   - 操作能納入下列其中之一:load、store、exchange、
    //     fetch_add、fetch_sub、compare_exchange。
    //
    // 用 std::mutex 的時機:
    //   - 你需要 *同時* 更新多個彼此相關的欄位,而且不能
    //     讓其他執行緒看到「更新到一半」的狀態。
    //     (例如 push 一個 vector 並同時更新 size,或是把錢
    //      從帳戶 A 轉到帳戶 B。)
    //   - 臨界區內呼叫的函式不在你掌控內,或可能丟例外。
    //
    // 經驗法則:如果你沒辦法把整個更新寫成單一個 atomic
    // 操作,那你就需要 mutex。
    // ---------------------------------------------------------

    // ---------------------------------------------------------
    // 補充:其他你實際會用到的 atomic 操作
    //
    //   std::atomic<bool>  done{false};
    //   done.store(true);             // 發布一個旗標
    //   while (!done.load()) {...}    // 自旋 / 輪詢
    //
    //   counter.fetch_add(5);         // atomic +=
    //   counter.fetch_sub(1);         // atomic -=
    //   counter.exchange(0);          // 交換,並回傳舊值
    //
    //   long long expected = 10;
    //   counter.compare_exchange_strong(expected, 11);
    //       // ^ 「若 counter == 10,把它設為 11;
    //       //    否則,把 `expected` 更新成目前的值。」
    //       //   這是無鎖演算法的基本構件。後面會再用到。
    // ---------------------------------------------------------

    // ---------------------------------------------------------
    // 關於記憶體順序的一個警告
    //
    // 每個 atomic 操作都有一個可選的第二參數,稱為記憶體
    // 順序 (memory order),例如:
    //
    //   counter.fetch_add(1, std::memory_order_relaxed);
    //
    // 預設是 std::memory_order_seq_cst (sequentially consistent)
    // —— 最強、也最容易推理的順序,同時也是最慢的。
    //
    // 初學者:*永遠* 使用預設值。只有在你認真讀過 C++ 的
    // 記憶體模型,並且 *量測過* 有效能問題之後,才去使用
    // 較弱的順序 (relaxed、acquire、release)。搞錯的話會
    // 重新引入沒有任何測試能抓到的競爭。
    // ---------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::atomic<T> 對任何 T 都 lock-free 嗎?
    //    A：不是。標準保證 std::atomic_flag 一定 lock-free, std::
    //       atomic<T> 對 T 是 1/2/4/8 byte 的整數/指標通常 lock-free
    //       (走 lock cmpxchg/xadd), 但 std::atomic<BigStruct> 內部
    //       會用一個隱藏的 mutex 模擬, 反而比直接用 mutex 還慢。執行
    //       期可用 .is_lock_free() 或編譯期 ATOMIC_xxx_LOCK_FREE 宏
    //       檢查。
    //
    //  Q2：memory_order_relaxed 真的「沒同步」嗎? 為什麼計數器可以用?
    //    A：relaxed 仍保證「對同一變數的修改順序是全 thread 一致」
    //       (modification order), 而且 RMW (fetch_add) 仍是不可分割
    //       的 atomic, 所以「lost update」不會發生。它「不」保證的
    //       是「這個寫入跟其他變數的寫入間誰先誰後」── 計數器只關心
    //       自己, 不依賴別的變數的順序, 所以可以用 relaxed 換點效能。
    //
    //  Q3：x86 上 seq_cst 跟 relaxed 量起來幾乎一樣, 那 memory order
    //       是不是都不重要?
    //    A：x86 是強序 (TSO), 即使你寫 relaxed, 硬體也接近 seq_cst,
    //       看不出差異。但 ARM / RISC-V / Apple Silicon 是弱序架構,
    //       relaxed 會省掉昂貴的 dmb 指令, 差異 2-5×; 反過來 seq_cst
    //       的 store 在 ARM 要插 full barrier, 在熱路徑成本明顯。在
    //       x86 量 benchmark「沒差」不能當作可移植性的依據。
    //
    return 0;
}
