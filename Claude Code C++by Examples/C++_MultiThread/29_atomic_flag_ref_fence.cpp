// =====================================================================
// 29_atomic_flag_ref_fence.cpp
//
// 主題:剩下三個 <atomic> 中還沒上過的標準元件
//
//   A. std::atomic_flag           (C++11,唯一保證 lock-free 的 atomic)
//   B. std::atomic_ref<T>         (C++20,把任意 T 變數當 atomic 操作)
//   C. std::atomic_thread_fence   (獨立的記憶體屏障,不附在 atomic op 上)
//
// 為什麼這課很值得補:
//   - lesson 04 用了 std::atomic<T> 做 counter
//   - lesson 11 講了 memory_order_acquire / release
//   - 但 std::atomic_flag 是 *標準保證* 永遠 lock-free 的唯一型別
//     (std::atomic<int> 在小到中型機器幾乎一定 lock-free,但不是保證,
//      在某些奇怪 ABI / 嵌入式 / 不對齊情境會退化成有 mutex 的實作。)
//
//   - std::atomic_ref 解決一個很實際的問題:
//     「我手上的 buffer / 結構是 plain int / float (因為要餵 SIMD、
//      mmap、第三方 API),不能改成 std::atomic<int>,但我又想對某幾個
//      欄位做 atomic 操作。」  C++20 之前你只能轉指標 + 編譯器 builtin,
//      C++20 開始可以用 std::atomic_ref<T>(x) 暫時把它當 atomic。
//
//   - std::atomic_thread_fence 是「獨立的 release/acquire」。如果你
//     有一連串 *普通* 寫入 (非 atomic),想保證它們都對 reader 可見後,
//     才把一個 flag 標起來,可以省掉每個寫都用 atomic store 的成本,
//     最後用一次 thread_fence(release) 收尾。
//
// Build:  g++ -std=c++20 -O2 -pthread 29_atomic_flag_ref_fence.cpp -o 29_atomic_flag_ref_fence
// =====================================================================

// =====================================================================
// 課程資訊 (Class Info)
// =====================================================================
// 主題:     <atomic> 標頭中的三個補完元件 ── atomic_flag / atomic_ref /
//           atomic_thread_fence
// 前置課程: lesson 04 (std::atomic<T>) / lesson 11 (memory_order /
//           release-acquire / spinlock 簡介)
// 觀念詞彙:
//   - lock-free             ── *進展保證*:任一 thread 被搶佔時整體仍能前進。
//                              「不會退化成有 mutex 的實作」是它在 atomic 上的
//                              常見表現,不是定義(atomic_flag 的操作是 spec
//                              保證 lock-free,atomic<T> 多數平台是事實但非保證)
//                              ★ 注意「lock-free *操作*」≠「lock-free *演算法*」:
//                                 本檔下面用 atomic_flag 組出的 SpinLock,每個操作
//                                 都是 lock-free,但那個 spinlock 本身是 *blocking*
//                                 演算法(持鎖者被搶佔 → 全體停住)。見 lesson 17。
//   - test-and-set          ── 「讀舊值並設新值」的不可分割原子操作
//   - test-and-test-and-set ── 加上前置 test 減少 cache invalidation 的優化
//   - alignment requirement ── atomic_ref 對被 ref 物件的對齊要求
//   - memory fence / barrier── 獨立於 atomic op 之外的 happens-before 屏障
// 新介紹 API:
//   std::atomic_flag                    最小、保證 lock-free 的 atomic
//     - test_and_set(order)  → bool     舊值設為 true,回傳舊值
//     - clear(order)                    設為 false
//     - test(order) (C++20)  → bool     只讀,不改
//     - wait(old, order) / notify_one / notify_all (C++20)
//   std::atomic_ref<T>(x)               C++20,把 plain 變數暫時包成 atomic
//     - load / store / fetch_add / compare_exchange_*  與 atomic<T> 一致
//     - 必須符合 required_alignment 對齊要求
//   std::atomic_thread_fence(order)     獨立 barrier,不附在任何 op 上
// 何時使用:
//   - atomic_flag:極端低層、自製 spinlock、嵌入式 / kernel-level、需要
//                  「絕對保證 lock-free」的場合
//   - atomic_ref :buffer 必須是 plain 型別 (餵 SIMD / mmap / 第三方 API),
//                  又要對少數欄位做 atomic op
//   - thread_fence:批次大量 plain 寫入後再用一個 flag 對外公佈,省掉每個
//                  寫都用 atomic store 的成本
// 何時不要用:
//   - 一般用戶層程式:寫 std::atomic<T> + memory_order 即可,別碰這三個
//   - 自製 spinlock 99% 都該換成 std::mutex (除非 critical section <100 ns
//     且已 benchmark)
//   - atomic_thread_fence 寫錯極易,最好讓 release/acquire 跟著 store/load 走
// 常見錯誤:
//   - atomic_flag 沒給 acquire/release order → 預設 seq_cst,雖正確但較慢;
//     spinlock 場景明確指定 acquire/release
//   - atomic_ref 用完還繼續用「原來的 plain 變數」做普通讀寫 → race UB
//   - atomic_ref 對齊不足 → required_alignment 沒滿足,行為未定義
//   - thread_fence 配 op 用 relaxed 但忘記 fence,或 fence 放錯邊 → 失效
//   - 把 spinlock 用在 critical section 長 / 高負載場景 → 燒 CPU + 餓死
// =====================================================================

// =====================================================================
// 深入解析 (Deep Dive) ── atomic_flag / atomic_ref / fence 的精準定位
// =====================================================================
//
// 1. atomic_flag 為什麼是「最小的 atomic」
//    C++ spec 對 atomic_flag 的承諾:
//      - 無條件 lock-free。任何平台都不會退化成有 mutex 的實作。
//      - 大小至少能放進 1 byte (實作通常是 1 byte 或 word)。
//      - 只有 test_and_set / clear / (C++20) test / wait / notify。
//    比 atomic<bool> 強在哪:atomic<bool> *理論上* 可能有 mutex 實作
//    (儘管主流平台都 lock-free)。atomic_flag 是 *spec 保證*。
//    用途:極端低層 (嵌入式、kernel-level)、自製 spinlock、一次性 flag。
//
// 2. spinlock vs std::mutex
//    spinlock 用 atomic_flag 實作,核心:
//        while (flag.test_and_set(acquire)) { /* spin */ }
//    優點:臨界區極短 (< 100 ns) 時比 mutex 快 (省 syscall + futex)。
//    缺點:
//      - 臨界區一長就燒 CPU
//      - 沒有 fairness ── 高負載下某些 thread 可能餓死
//      - 沒有 priority handling ── 風險 priority inversion
//    經驗法則:
//      - 用戶層應用:99% 用 std::mutex,別寫 spinlock
//      - kernel / 中斷處理:不能睡,一定 spinlock
//      - 高頻交易/HFT 熱迴圈:仔細 benchmark 後才考慮
//
// 3. test-and-test-and-set 優化
//    純 test_and_set 每次都把 cache line 改成 Modified (寫操作),即使
//    沒拿到鎖也觸發 invalidate broadcast。改成:
//        while (flag.test_and_set(acquire)) {
//            while (flag.test(relaxed)) { _mm_pause(); }
//        }
//    內層只 read,cache line 留在 Shared 狀態,bus traffic 大降。
//    `_mm_pause()` (x86 PAUSE 指令) 給 hyperthread sibling 時間,並讓
//    branch predictor 知道我們在 spin。
//
// 4. atomic_ref (C++20) 解決什麼問題
//    場景:
//      - 你有個 plain `int data[N]`,要餵給 SIMD / OpenMP / 第三方 API,
//        所以它必須是 plain 不能是 atomic<int>。
//      - 但有時候你想對 *某幾個* 欄位做 atomic 操作 (例如統計計數)。
//    舊解法:用 atomic<int>* 強轉、或編譯器 builtin (__atomic_*),不便攜。
//    新解法:暫時把 plain int 包進 atomic_ref:
//        std::atomic_ref<int> r(data[i]);
//        r.fetch_add(1, relaxed);
//    要求:被 ref 的物件必須對齊到該型別的 required_alignment (atomic_ref<int>::
//    required_alignment 通常 4 byte)。
//
// 5. atomic_ref 的限制
//    - ref 必須與 *所有其他存取* 透過 atomic_ref 進行,否則 race。
//      意思是「原來那個 plain int」一旦被 atomic_ref 觸過,就不能再用普通
//      讀寫 ── 否則就是 data race UB。
//    - lifetime:ref 不持有資料所有權;data[i] 釋放後 ref 立刻 dangling。
//
// 6. atomic_thread_fence 的用途
//    一般 atomic store(release) 已建立 happens-before。fence 是 *獨立的*
//    barrier,不附在任何 atomic op 上。
//    場景:
//        for (int i = 0; i < N; ++i) data[i] = compute(i);   // 普通寫
//        std::atomic_thread_fence(release);                   // 收尾屏障
//        ready.store(1, relaxed);                              // 普通的 atomic store
//    讀者那側:
//        if (ready.load(relaxed)) {
//            std::atomic_thread_fence(acquire);
//            for (int i = 0; i < N; ++i) consume(data[i]);
//        }
//    為什麼 ready 用 relaxed 仍正確?因為 fence 提供了 release/acquire
//    語意。優勢:寫入主迴圈內不必每筆 atomic store。
//    但這個 pattern 容易寫錯 ── 99% 場景用 release-store 就好。
//
// 7. fence 的 visibility 模型 (進階)
//    `atomic_thread_fence(seq_cst)` 與 `atomic.store(seq_cst)` 不完全等價。
//    fence 影響「fence 同 thread 之前/之後的所有 atomic 與普通 op」;
//    op 上的 order 只影響「該 op 與其他 *配對* op」。
//    Herb Sutter 的 "atomic<> Weapons" talk 是這方面最清楚的講解。
//    建議:無需追求極限性能就用 atomic op 上的 order,別動 fence。
//
// 8. 三者該用哪個 ── 決策樹
//      只想要保證 lock-free 的 1 bit 標誌 → atomic_flag
//      要在 plain 變數上做 atomic op (不能改成 atomic<T>) → atomic_ref
//      想對「一連串 plain 寫」收尾建立 happens-before  → atomic_thread_fence
//    一般情境:都別用,寫普通的 std::atomic<T> + memory_order 就好。
// =====================================================================

// =====================================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =====================================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── 三件工具屬於底層補完,
//     LC 用普通 atomic<T> 都能過。
//   → 但本課的 atomic_flag 是「自製 spinlock」唯一保證 lock-free 的工具,
//     可作為 lesson 30 Q5/Q9 mutex 的替換實驗 (短臨界區下測試效能差)。
//
// 主要 API 對照 (cppreference):
//   - std::atomic_flag                  https://en.cppreference.com/w/cpp/atomic/atomic_flag
//   - atomic_flag::test_and_set         https://en.cppreference.com/w/cpp/atomic/atomic_flag/test_and_set
//   - atomic_flag::clear                https://en.cppreference.com/w/cpp/atomic/atomic_flag/clear
//   - atomic_flag::test (C++20)         https://en.cppreference.com/w/cpp/atomic/atomic_flag/test
//   - atomic_flag::wait / notify (C++20) https://en.cppreference.com/w/cpp/atomic/atomic_flag/wait
//   - std::atomic_ref<T> (C++20)        https://en.cppreference.com/w/cpp/atomic/atomic_ref
//   - std::atomic_thread_fence          https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence
//   - std::atomic_signal_fence          https://en.cppreference.com/w/cpp/atomic/atomic_signal_fence
//
// 練習建議:
//   - 把 lesson 30 Q9 Traffic Light 的 std::mutex 替換成本課的
//     atomic_flag spinlock,測量 30 輛車場景下的 latency 變化。注意:
//     臨界區一旦變長 spinlock 就反而更慢。
// =====================================================================

/*
補充筆記：atomic_flag_ref_fence
  - atomic_flag_ref_fence 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - atomic_flag_ref_fence 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】atomic_flag / atomic_ref / atomic_thread_fence
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::atomic_flag 有什麼特別之處？
//     答：它是標準「唯一保證永遠 lock-free」的原子型別，只有 test_and_set 與 clear
//         （C++20 再加 test 與 wait/notify）。std::atomic<int> 在常見平台上幾乎一定
//         lock-free，但那是實作性質而非標準保證，可用 is_lock_free() 查詢。因為介面
//         極簡且保證無鎖，atomic_flag 是手寫 spinlock 的標準材料。
//     追問：手寫 spinlock 要注意什麼？（上鎖用 test_and_set(acquire)、解鎖用
//           clear(release)；迴圈中先用 test() 唯讀輪詢再嘗試 test_and_set，避免持續
//           以獨佔狀態拉走 cache line，並加上 pause 指令）
//
// 🔥 Q2. std::atomic_ref<T> 解決什麼問題？
//     答：C++20 新增，讓你把「既有的普通變數」暫時當作原子物件來操作。它解決的實際
//         困境是：手上的 buffer 或結構必須是 plain int/float（要餵 SIMD、mmap、
//         第三方 C API），不能改成 std::atomic<int>，但某幾個欄位又需要原子存取。
//         注意在 atomic_ref 存活期間，該物件的所有存取都必須經由 atomic_ref，否則
//         仍是 data race；而且被引用的物件必須滿足其對齊要求。
//     追問：為什麼不能直接把整個陣列宣告成 atomic？（atomic<T> 的大小／對齊可能與 T
//           不同，也無法保證能與期望 plain T 佈局的外部 API 相容）
//
// Q3. atomic_thread_fence 與「在 atomic 操作上指定 memory_order」差在哪？
//     答：fence 是獨立的屏障，不附著於特定原子操作。典型用法是做一連串 relaxed 寫入
//         後，用一次 fence(release) 收尾，比每個寫入都用 release store 便宜。它的
//         順序保證涵蓋範圍更廣、擺放位置更彈性，代價是推理更難：同步關係不再由「哪個
//         變數的哪一對操作」清楚標示，review 時更容易看漏。
//     追問：fence 可以單獨使用嗎？（不行。仍要與另一端的 acquire fence 或 acquire
//           操作配對，並且中間要有實際被讀到的原子變數作為傳遞的媒介）
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// =====================================================================
// A. std::atomic_flag — 最小、最快、最受 spec 保證的 atomic
//
// API 只有這幾個 (C++11):
//   .test_and_set(memory_order)   ── 設成 true,回傳「先前」的值
//   .clear(memory_order)          ── 設成 false (release 語意)
// C++20 加上:
//   .test(memory_order)           ── 只讀,不修改
//   .wait(value) / .notify_one()  ── futex-style,等到值不是 value 才醒
//
// 注意 std::atomic_flag 沒有 .load() / .store() (因為 C++11 的設計
// 完全不允許讀);C++20 才補了 .test()。
//
// 主要用途:
//   1. 旗號型 spinlock (test_and_set 等於 try_lock,clear 等於 unlock)
//   2. 一次性 latch 或「我已經初始化過了」flag
//
// 重要陷阱:
//   - std::atomic_flag 必須用 ATOMIC_FLAG_INIT 或預設建構 (C++20 起
//     建構就保證 false)。
// =====================================================================

class SpinLock {
public:
    void lock() noexcept {
        // test_and_set 回傳「先前」的值。
        //   - 如果先前是 false → 現在已經幫我們設成 true,我們拿到了鎖。
        //   - 如果先前是 true  → 別人持有,我們繼續轉。
        //
        // memory_order_acquire 是必要的:它建立 happens-before,
        // 確保「拿到鎖之後讀到的資料」看到「上一個 unlock 之前寫入」。
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // C++20 的 .wait() 比純 busy-wait 友善,但 spinlock 的本意
            // 就是「臨界區極短」,所以這裡示範純 busy-wait 並讓出 CPU。
            //
            // .test() (只讀) 比 .test_and_set() 對 cache line 友善:
            // 後者每次都會把 line 寫成 modified 狀態,觸發 bus traffic;
            // 前者只是讀,讓 cache 留在 shared 狀態。這是經典的
            // "test-and-test-and-set" 優化。
            while (flag_.test(std::memory_order_relaxed)) {
                // 給 hyperthread 同伴一點時間,讓 OS 排程器知道我們
                // 在 spin。x86 上這通常被 lower 成 PAUSE 指令。
                std::this_thread::yield();
            }
        }
    }

    void unlock() noexcept {
        // memory_order_release 把上一段臨界區的所有寫入,在 flag 變 false
        // 之前 publish 出去。這是配對 lock 的 acquire 用的。
        flag_.clear(std::memory_order_release);
    }

private:
    // C++20 起,預設建構就保證是 clear 狀態。C++17 以前要寫
    // = ATOMIC_FLAG_INIT。
    std::atomic_flag flag_{};
};

void demo_atomic_flag_spinlock() {
    std::cout << "=== A. std::atomic_flag (spinlock) ===\n";

    SpinLock lk;
    long long counter = 0;
    constexpr int N = 8;
    constexpr int per_thread = 100'000;

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> ts;
    for (int i = 0; i < N; ++i) {
        ts.emplace_back([&]{
            for (int k = 0; k < per_thread; ++k) {
                lk.lock();
                ++counter;        // ← 用 plain int,不需要 atomic
                lk.unlock();
            }
        });
    }
    for (auto& t : ts) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    std::cout << "  " << N << " threads × " << per_thread
              << " 增量,counter = " << counter
              << " (預期 " << N * per_thread << ")"
              << ",耗時 " << ms << " ms\n";
    std::cout << "  ※ spinlock 在臨界區極短時最快,臨界區一變長就會"
                 "不停 burn CPU,改用 std::mutex 反而較好。\n\n";
}

// =====================================================================
// B. std::atomic_ref<T> — 把 *已存在* 的 T 暫時當 atomic 操作
//
// 用 std::atomic<T> 的限制:
//   - 你必須在宣告時就決定 T 是 atomic。
//   - 不能跟非 atomic 的 API / 資料對齊 (例如餵給 SIMD intrinsics、
//     寫入 mmap 的 binary layout、跟 C 結構互通)。
//
// std::atomic_ref<T>(x) 的精神:
//   - x 還是普通的 T (記憶體、layout 不變)
//   - 但只要你把對 x 的所有 *並行* 存取都「透過 atomic_ref」做,
//     就有 atomic 語意。
//
// 限制:
//   - x 必須對齊到 std::atomic_ref<T>::required_alignment。
//     (基本型別通常都自然對齊,但 struct 內的成員要小心 padding。)
//   - 在 atomic_ref 存活期間,*所有* 對 x 的存取都必須透過 atomic_ref;
//     不可以同時混用 plain 寫入 + atomic_ref 寫入,否則 UB。
// =====================================================================

void demo_atomic_ref() {
    std::cout << "=== B. std::atomic_ref<T> (C++20) ===\n";

    // 想像這個 vector 是「外部 API 給我的、不能改型別」的資料。
    // 但我又想多執行緒同時更新某幾個位置。
    alignas(std::atomic_ref<int>::required_alignment)
        std::vector<int> data(8, 0);

    constexpr int N = 4;
    constexpr int iters = 250'000;

    std::vector<std::thread> ts;
    for (int i = 0; i < N; ++i) {
        ts.emplace_back([&, slot = i % data.size()]{
            // 用 atomic_ref 把 data[slot] 暫時當 atomic 操作。
            // 注意 ref 是 view,不擁有資料,不能用 std::atomic<int> 取代。
            std::atomic_ref<int> ar(data[slot]);
            for (int k = 0; k < iters; ++k) {
                ar.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : ts) t.join();

    long long sum = 0;
    for (int v : data) sum += v;
    std::cout << "  4 threads × " << iters << " 各自 fetch_add 到 4 個 slot\n"
              << "  data = ";
    for (int v : data) std::cout << v << ' ';
    std::cout << "\n  sum  = " << sum
              << " (預期 " << static_cast<long long>(N) * iters << ")\n\n";
}

// =====================================================================
// C. std::atomic_thread_fence — 獨立的記憶體屏障
//
// 場景:
//   你有「N 個普通寫入」想 publish 給 reader,只用 *一個 atomic flag*
//   收尾。如果每個寫入都做 atomic store(release) 太貴,可以這樣寫:
//
//     /* 先寫一堆普通記憶體 */
//     std::atomic_thread_fence(std::memory_order_release);
//     ready.store(true, std::memory_order_relaxed);  // 不需要 release
//
//   讀者那邊配對:
//
//     while (!ready.load(std::memory_order_relaxed)) ;
//     std::atomic_thread_fence(std::memory_order_acquire);
//     /* 現在可以安全讀那一堆普通記憶體 */
//
// 為什麼可行:
//   thread_fence(release) + 之後的 atomic store(relaxed)
//   組合起來,語意上等同於 atomic store(release)。
//   thread_fence(acquire) + 之前的 atomic load(relaxed) 同理。
//
// 為什麼要 fence 不直接用 release/acquire store?
//   - 普通記憶體寫入沒「掛載點」可以放 release;只有 atomic 操作可以。
//   - 用 fence 可以「集中放一次」,而不是每筆 atomic 都付 release 的稅。
//   - 在 weak memory model 平台 (ARM、POWER) 這個差別才看得見;
//     x86 因為 TSO,普通 store 就是 release-ish,差別很小。
//
// 注意:thread_fence 跟 *atomic 變數* 有同步關係,不是「barrier 全部
// 記憶體」。你還是需要那個 atomic flag 來連兩邊。光寫 thread_fence
// 不會自己產生 happens-before。
// =====================================================================

namespace fence_demo {

constexpr int N = 1024;
int  payload[N];                 // ← 故意是普通 int 陣列,不是 atomic
std::atomic<bool> ready{false};

void producer() {
    // 一連串 *普通* 寫入。沒有任何同步開銷。
    for (int i = 0; i < N; ++i) {
        payload[i] = i * i;
    }

    // 把上面那 N 個 plain store 全部 publish 出去。
    // 這之後的 atomic store 就只需要 relaxed。
    std::atomic_thread_fence(std::memory_order_release);
    ready.store(true, std::memory_order_relaxed);
}

void consumer() {
    // 等到 producer 喊 ready (relaxed 即可)。
    while (!ready.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
    }

    // 收尾的 acquire fence 把上面那段 publish 「拉」進來。
    // 這之後讀 payload[] 就保證能看到 producer 寫入的最終值。
    std::atomic_thread_fence(std::memory_order_acquire);

    // 普通讀,不需要 atomic load。
    long long sum = 0;
    for (int i = 0; i < N; ++i) sum += payload[i];

    // 0² + 1² + ... + 1023² = 1023*1024*2047/6 = 357'389'824
    std::cout << "  consumer 看到 sum = " << sum
              << " (預期 357'389'824)\n";
}

} // namespace fence_demo

void demo_atomic_thread_fence() {
    std::cout << "=== C. std::atomic_thread_fence ===\n";
    fence_demo::ready.store(false);

    std::thread p(fence_demo::producer);
    std::thread c(fence_demo::consumer);
    p.join(); c.join();

    std::cout << "  ※ 如果把 release fence 拿掉,在 ARM/POWER 上"
                 "consumer 可能讀到 0 / 殘值;\n"
                 "    在 x86 上常常 *碰巧能跑*,但 spec 上仍然是 race。\n\n";
}

// ---------------------------------------------------------------------
// std::atomic_signal_fence (順便提一下,不單獨示範)
//
// 跟 atomic_thread_fence 的差別:
//   - thread_fence:跨 *執行緒* 的記憶體可見性
//   - signal_fence: 同一執行緒的 main code 跟 signal handler 之間
//                   (像 Unix signal handler / interrupt handler)
//
// 它 *不會* 產生 CPU memory barrier,只阻止編譯器 reorder。所以幾乎
// 0 成本,但只能用在「同 CPU、不跨 thread」的情境。我們在 user-space
// 多執行緒程式幾乎用不到,但寫 signal handler / lock-free 與 OS
// interrupt 互動時會看到。
// ---------------------------------------------------------------------

// =====================================================================
// 實戰範例 1: atomic_flag 當「first to fire」一次性 guard
// =====================================================================
// 應用場景: 多條 thread 可能同時偵測到「該觸發 shutdown」(例如
// 都看到同一個 fatal error)。我們只想 *第一個* 收到的觸發者真
// 的做 shutdown, 其他人就靜默離開。
//
// 用 atomic_flag.test_and_set() 是最便宜的「最先到者贏」工具:
//   - 第一個呼叫的 thread 看到回傳 false (原本沒設), 進入分支
//   - 後續呼叫者看到 true (已被設), 跳過分支
// 比 atomic<bool>.exchange(true) 還短, 而且 spec 保證 lock-free。
// =====================================================================
void demo_atomic_flag_once_guard() {
    std::cout << "=== Demo D (extra): atomic_flag 一次性 guard ===\n";
    std::atomic_flag fired = ATOMIC_FLAG_INIT;
    std::atomic<int> winners{0};

    auto on_error = [&](int who){
        if (!fired.test_and_set()) {
            // 第一個 → 真的執行 shutdown 邏輯
            ++winners;
            std::cout << "  [thread " << who
                      << "] 第一個觸發, 執行 shutdown\n";
        } else {
            std::cout << "  [thread " << who << "] 已被別人處理, 退出\n";
        }
    };
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i) ts.emplace_back(on_error, i);
    for (auto& t : ts) t.join();
    std::cout << "  總共 winner = " << winners.load()
              << " (預期 1)\n\n";
}

// =====================================================================
// 實戰範例 2: atomic_thread_fence 大量 plain write 後一次發布
// =====================================================================
// 應用場景: 序列化器/打包器把幾百個欄位寫進 buffer (純 memcpy),
// 最後一次設 ready_flag 通知對端「可以讀了」。對 ARM/POWER 等弱
// 序架構, 每個欄位都用 atomic store(release) 太貴; 用 thread_fence
// (release) + 一次 relaxed flag store 取代, 一次性 publish 整段。
//
// 這裡用 4 個欄位示意; 真實 protobuf / flatbuffer 序列化器內部
// 用的就是這個 pattern (有的還會展開 SIMD memcpy 之後接 fence)。
// =====================================================================
void demo_publish_struct_via_fence() {
    std::cout << "=== Demo E (extra): plain struct + fence publish ===\n";
    struct Packet { int id; long ts; int type; int crc; };
    static Packet pkt;          // 普通 struct, *不是* atomic
    static std::atomic<bool> ready{false};

    std::thread serializer([&]{
        pkt.id   = 7;
        pkt.ts   = 1717171717L;
        pkt.type = 42;
        pkt.crc  = 0xDEAD;
        std::atomic_thread_fence(std::memory_order_release);   // 一次發布
        ready.store(true, std::memory_order_relaxed);
    });
    std::thread reader([&]{
        while (!ready.load(std::memory_order_relaxed))
            std::this_thread::yield();
        std::atomic_thread_fence(std::memory_order_acquire);
        std::cout << "  reader 讀到 pkt.id=" << pkt.id
                  << " ts=" << pkt.ts
                  << " type=" << pkt.type
                  << " crc=0x" << std::hex << pkt.crc << std::dec << '\n';
    });
    serializer.join(); reader.join();
    std::cout << '\n';
}

int main() {
    demo_atomic_flag_spinlock();
    demo_atomic_ref();
    demo_atomic_thread_fence();
    demo_atomic_flag_once_guard();
    demo_publish_struct_via_fence();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：既然 std::atomic<bool> 在大多數平台都 lock-free,為什麼還需要
    //      std::atomic_flag?
    //    A：差別在「保證」二字。spec 對 atomic<bool> *不保證* lock-free
    //       ── 在某些奇怪 ABI、未對齊、嵌入式平台上 atomic<bool> 可能退化
    //       成有 mutex 的實作 (內部 splitlock)。atomic_flag 是 spec 唯一
    //       *無條件保證* lock-free 的型別,大小 ≥ 1 byte,API 只有
    //       test_and_set / clear / (C++20) test/wait/notify。寫 kernel
    //       module、驅動程式、嵌入式 RT 系統時這個保證才看得出價值。
    //
    //  Q2：fence 跟 memory barrier (CPU 指令) 是同一回事嗎?
    //    A：不完全是。std::atomic_thread_fence 是「C++ 抽象機器」層面的
    //       happens-before barrier,編譯器會視 ISA 翻成適當的 CPU 指令:
    //       x86 上 release/acquire fence 多半是 NOP (TSO 已保證 store-store
    //       與 load-load 順序),seq_cst fence 翻成 mfence;ARM/POWER 才會
    //       發 dmb/lwsync。另外有 std::atomic_signal_fence(只擋編譯器
    //       reorder、不發 CPU barrier),專給同 thread 的 signal handler
    //       與 main code 同步用,跨 thread 完全沒效果別搞混。
    //
    //  Q3：atomic_thread_fence 比每筆 release-store 省在哪?什麼時候別用?
    //    A：場景:一段迴圈寫一萬筆 plain 資料,最後用一個 flag 對外公佈。
    //       若每筆都用 atomic store(release) → 每筆付 release tax (ARM/POWER
    //       上要 dmb 指令)。改成「迴圈內全 plain 寫 → 最後一次 thread_fence
    //       (release) → flag store(relaxed)」可省下 N-1 次 release。但這
    //       pattern 易寫錯 (fence 位置、配對 acquire 都得對),99% 場景用
    //       release-store 就好;只在 profile 證實 release tax 是熱點時才
    //       上 fence。Herb Sutter 的 "atomic<> Weapons" 是進階參考。
    //
    return 0;
}

// =====================================================================
// 帶走的重點:
//
// 1. std::atomic_flag 是 *spec 保證* lock-free 的唯一 atomic 型別。
//    API 只有 test_and_set / clear (+ C++20 的 test/wait/notify)。
//    最常見用途是寫一顆極小的 spinlock。臨界區一長就改 std::mutex。
//
// 2. std::atomic_ref<T> (C++20) 把已經存在的、不是 std::atomic 的
//    變數暫時 *當 atomic 看*。受對齊限制,而且只要 atomic_ref 還活著
//    就不能再用 plain 存取那塊記憶體。專治「外部 API 給的記憶體」。
//
// 3. std::atomic_thread_fence(release/acquire) 是「不附 store/load
//    的記憶體屏障」。可以把一連串 plain 寫入用一次 release fence 收尾,
//    省掉每個寫入都付 release 稅。常跟 *一個* 收尾的 atomic flag 配對。
//
// 4. std::atomic_signal_fence 跟 thread_fence 同 API 但語意完全不同:
//    它只擋 *編譯器* reorder、不發 CPU barrier,專給 signal handler 用。
//
// 5. 這些是 <atomic> 中相對「補完」的元件,前面 lesson 04/11/16 已經
//    用 std::atomic<T> 與 memory_order 把主要使用情境講過了;這課把
//    最後三個剩下的 STL 元件補齊。
// =====================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 29_atomic_flag_ref_fence.cpp -o 29_atomic_flag_ref_fence

// === 預期輸出 ===
// === A. std::atomic_flag (spinlock) ===
//   8 threads × 100000 增量,counter = 800000 (預期 800000),耗時 24 ms
//   ※ spinlock 在臨界區極短時最快,臨界區一變長就會不停 burn CPU,改用 std::mutex 反而較好。
//
// === B. std::atomic_ref<T> (C++20) ===
//   4 threads × 250000 各自 fetch_add 到 4 個 slot
//   data = 250000 250000 250000 250000 0 0 0 0
//   sum  = 1000000 (預期 1000000)
//
// === C. std::atomic_thread_fence ===
//   consumer 看到 sum = 357389824 (預期 357'389'824)
//   ※ 如果把 release fence 拿掉,在 ARM/POWER 上consumer 可能讀到 0 / 殘值;
//     在 x86 上常常 *碰巧能跑*,但 spec 上仍然是 race。
//
// === Demo D (extra): atomic_flag 一次性 guard ===
//   [thread 0] 第一個觸發, 執行 shutdown
//   [thread 1] 已被別人處理, 退出
//   [thread   [thread 3] 已被別人處理, 退出
// 2] 已被別人處理, 退出
//   總共 winner = 1 (預期 1)
//
// === Demo E (extra): plain struct + fence publish ===
//   reader 讀到 pkt.id=7 ts=1717171717 type=42 crc=0xdead
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
