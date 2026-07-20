// =============================================================
// 11_memory_model.cpp  --  C++ 記憶體模型:memory_order_*
// =============================================================
//
// 本課目標:
//   1. 理解記憶體順序為何存在:編譯器與 CPU 為了效能會
//      *重新排序* 讀與寫,而其他執行緒可以 *觀察到* 這些
//      重新排序。
//   2. 學會你實際會用到的四種順序:
//        relaxed, acquire, release, seq_cst
//      (再加上 acq_rel —— RMW 用的組合,以及實際上等同
//       已死的 consume。)
//   3. 使用經典的「release/acquire 發布資料」模式 ——
//      無鎖設計的基石。
//   4. 在 *安全的場合* 使用 std::memory_order_relaxed,
//      看到一點點但確實存在的效能收益。
//   5. 培養健康的恐懼:寫錯記憶體順序會引入
//      *沒有任何測試能抓到的 bug*,在 x86 上尤其如此。
//      ARM 等弱記憶體序架構則會在生產環境抓到它們。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 11_memory_model.cpp -o 11_memory_model
//
// 執行方式:
//     ./11_memory_model
//
// 重要:這是唯一一課我會這樣說:讀完之後,*除非* 你有
// 量測過的理由 *和* 一位審核者,否則 *不要用* 你學到的
// 東西。預設值 std::memory_order_seq_cst 對你以後寫的
// 一切程式都是正確的,只是有時比必要的稍慢一點。
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     C++ 記憶體模型 ── memory_order_*,release/acquire
// 前置課程: lesson 04 (atomic)
// 觀念詞彙:
//   - memory order   ── 控制 atomic 操作前後的可見性順序
//   - synchronizes-with  ── release/acquire 配對所建立的順序關係
//   - relaxed        ── 只保證 atomicity,不保證任何順序
//   - acquire        ── load 之後的 ops 不可被重排到此 load 前
//   - release        ── store 之前的 ops 不可被重排到此 store 後
//   - seq_cst        ── 全 thread 統一全序,預設值,最強最慢
// 新介紹 API:
//   std::memory_order_relaxed
//   std::memory_order_acquire / release / acq_rel / consume
//   std::memory_order_seq_cst (預設)
//   .compare_exchange_strong/weak(expected, desired, on_succ, on_fail)
// 何時使用:
//   - 已測量過的熱點 + 配對清楚的 publish/subscribe
//   - 純獨立計數器 → relaxed 安全
// 何時不要用:
//   - 還沒理解 release/acquire → 用預設 seq_cst,正確優先
//   - 為了「看起來高效」就用 relaxed → 隨手寫 race
// 常見錯誤:
//   - 用 relaxed 來「發布」資料 → 沒有 happens-before
//   - 在 x86 上「看起來都對」就以為對 → ARM 上會炸
//   - compare_exchange 忘了第二個 (failure) order
//   - 信任 consume → 主流編譯器都當 acquire,別用
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── memory model 的核心心智圖
// =============================================================
//
// 1. 三層重排
//    (a) 編譯器重排:optimizer 為了好的 register 利用、減少 load,
//        會重排語句順序。-O0 也會做一點點;-O2 大量。
//    (b) CPU 微架構重排 (out-of-order execution):指令進管線後依資料
//        相依重排。x86 仍給「程式順序看得見」的假象 (TSO),ARM 不給。
//    (c) Memory subsystem 重排:store buffer、cache coherence 延遲。
//        x86 store→load 之間可重排 (StoreLoad reorder),這是 TSO 唯一弱點。
//    memory_order_* 同時對這三層下指令。
//
// 2. 五個常用 order 的決策樹
//    relaxed   ── 「我只要原子,不要任何順序保證」。獨立計數器、純統計。
//    acquire   ── 「我這個 load 之後的所有 ops,不能被重排到此 load 之前」。
//                 配對 release-store 看到資料才算「拿到」。
//    release   ── 「我這個 store 之前的所有 ops,不能被重排到此 store 之後」。
//                 把資料準備好後 store flag = ready。
//    acq_rel   ── RMW 操作 (fetch_add、CAS) 同時是 acquire+release。
//    seq_cst   ── 全 thread 看到 *全序*。預設,最強最慢。除非測過,別降級。
//
// 3. happens-before 的最小定義
//    A. 同一條 thread 內的程式順序就是 happens-before。
//    B. release-store 「synchronizes-with」 同變數上後續的 acquire-load。
//       (前提:acquire 看到的值剛好是該 release 寫入的)。
//    C. happens-before 是傳遞:若 A→B 且 B→C,則 A→C。
//    沒有 happens-before 的兩個非 atomic 存取若一個是寫 → data race → UB。
//    *所有同步原語* (mutex、cv、barrier、atomic with non-relaxed) 都是
//    在製造 happens-before 關係。
//
// 4. publish/subscribe 範例
//    Thread A:
//        data = compute_big_struct();           // (1) 普通寫
//        ready.store(true, release);            // (2) release-store
//    Thread B:
//        if (ready.load(acquire))               // (3) acquire-load
//            use(data);                          // (4) 普通讀
//    保證:若 (3) 看到 true,則 (1) happens-before (4) → (4) 安全讀 data。
//    若 (2)/(3) 改成 relaxed → (1) 與 (4) 之間沒 happens-before → race。
//
// 5. x86 vs ARM ── 為什麼測 x86 不夠
//    x86 是 TSO (Total Store Order):基本上「只有 StoreLoad 重排」。意思
//    是:你寫 release/acquire 跟寫 seq_cst,在 x86 上輸出的 asm 幾乎一樣
//    (差一個 mfence)。所以 x86 上「忘了寫 acquire」常常仍正確。
//    ARM、RISC-V、POWER 是「弱排序」:每對 load/store 都可能被重排。
//    在 x86 跑 1 億次都對的程式,搬到 ARM 上立刻爆 ── 這是 ARM Mac 上市
//    後 N 個 lock-free 函式庫被抓出 bug 的原因。
//
// 6. compare_exchange 的雙 order 參數
//    compare_exchange_weak(expected, desired, succ_order, fail_order)
//    成功時用 succ_order,失敗時用 fail_order。fail_order 不能比
//    succ_order 強,且不能是 release / acq_rel (失敗沒 store,沒意義)。
//    常用組合:succ=acq_rel, fail=acquire。
//
// 7. consume 為什麼別用
//    P0371 / 各家編譯器作者意見:consume 的「依賴鏈」語意太難分析,主流
//    實作 (gcc/clang) 直接降級成 acquire。所以「正確性沒輸」,但你不會
//    拿到設想的優化收益。標準正在重新設計 (P0735);現階段一律寫 acquire。
//
// 8. fence (lesson 29 對應)
//    std::atomic_thread_fence(release / acquire / seq_cst) 是 *獨立* 的
//    記憶體屏障,不附在任何 atomic op 上。用法:一連串 *普通* 寫入後,
//    收尾用一次 fence(release),省下每個寫都用 atomic store 的成本。
//    但 fence 比 release-store 更難用對 ── 90% 場景用 release-store 就好。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── memory order 屬於底層
//     實作議題,LC 全部用預設 seq_cst 都會過。
//   → 但本課是 *效能* 課:lesson 30 任一題若你想把熱迴圈內的 atomic
//     從 seq_cst 降到 acq_rel/relaxed,知識基礎都在這課。例如 Q4 H2O
//     裡 sem.release 預設是 seq_cst,實際上 acq_rel 就夠 ── 但這種
//     優化只有 ARM/POWER 才看得出差。
//
// 主要 API 對照 (cppreference):
//   - std::memory_order                 https://en.cppreference.com/w/cpp/atomic/memory_order
//   - happens-before / synchronizes-with https://en.cppreference.com/w/cpp/atomic/memory_order#Formal_description
//   - std::atomic_thread_fence          https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence
//   - std::atomic_signal_fence          https://en.cppreference.com/w/cpp/atomic/atomic_signal_fence
//
// 練習建議:
//   - 拿 lesson 16 SPSC ring buffer,把 head/tail 的 release/acquire
//     改成 seq_cst,測效能差距 (x86 上 0%,ARM 上 5-15%)。
//   - 把 lesson 30 Q5 BoundedBlockingQueue 換成 lesson 16 SPSC ring
//     (僅 1P+1C 場景),用 release/acquire 取代 mutex+cv,觀察 latency
//     從 µs 降到 ns。
// =============================================================

/*
補充筆記：std::memory_model
  - memory model 描述執行緒間何時能看見彼此的寫入。
  - happens-before 比「程式碼看起來先後」更重要，編譯器與 CPU 都可能重排。
  - atomic ordering 要和資料依賴一起推理，不能只背 acquire/release 名稱。
  - std::memory_model 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++ 記憶體模型與 memory_order
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 說明 relaxed / acquire / release / acq_rel / seq_cst 的語意
//     答：relaxed 只保證該操作本身是原子的，以及同一個原子變數上的 modification
//         order 一致性（所有 thread 對同一變數看到的修改順序一致）；不建立任何跨變數
//         的順序、不產生 synchronizes-with。release（用於 store）禁止「之前」的讀寫被
//         重排到它之後；acquire（用於 load）禁止「之後」的讀寫被重排到它之前。acq_rel
//         給 RMW 用，讀端 acquire、寫端 release。seq_cst 在 acquire/release 之上，額外
//         保證所有 seq_cst 操作存在單一全域總順序 S，且 S 與各物件的 modification
//         order、與 happens-before 一致。
//     追問：release/acquire 如何真正建立同步？（見 Q2）
//
// 🔥 Q2. release 與 acquire 是怎麼「配對」的？
//     答：當 acquire load「實際讀到了」那個 release store 寫入的值（或其 release
//         sequence 中的值），兩者才建立 synchronizes-with，進而 happens-before，於是
//         release 之前的所有寫入（包含非原子寫）對 acquire 之後可見。關鍵在「讀到了」
//         ——單獨用 release、或單獨用 acquire、或 acquire 讀到的是別人寫的值，都不成立
//         同步。它建立的是「同一個變數上一對特定操作」之間的關係，不是全域屏障。
//     追問：這個保證涵蓋非原子變數嗎？（涵蓋。這正是「用一個 atomic 旗標發布一整塊
//           普通資料」這個模式成立的理由）
//
// 🔥 Q3. seq_cst 比 acquire/release 多保證了什麼？
//     答：多的是單一全域總順序，實務上體現為阻止 StoreLoad 重排。經典反例
//         （store buffer / Dekker）：T1 做 x.store(1,release) 後 load y；T2 做
//         y.store(1,release) 後 load x。在 release/acquire 下「兩邊都讀到 0」是允許的
//         （兩個 store 還卡在各自 CPU 的 store buffer）。全部改成 seq_cst 後，因為存在
//         總順序，兩個 store 必有先後，兩邊同時讀到 0 就不可能發生。
//     追問：成本？（x86 上 seq_cst load 免費，store 需 MFENCE 或 XCHG；ARM 需 DMB）
//
// Q4. memory_order_consume 為什麼不建議用？
//     答：consume 原本想表達比 acquire 更弱、只沿資料依賴鏈傳遞順序，在 ARM/POWER 上
//         可省掉屏障。但標準對 dependency-ordered-before 的定義難以在編譯器中正確實作
//         （優化可能消除依賴），因此主流編譯器都直接把 consume 提升為 acquire，實質
//         沒有效益。面試答「知道它存在，實務一律用 acquire」即為正解。
//     追問：Linux kernel 的 rcu_dereference 怎麼處理？（自訂的編譯器屏障與依賴規則約定）
//
// ⚠️ 陷阱. 這段程式碼對嗎？
//     T1: data = 42; ready.store(true, relaxed);
//     T2: while (!ready.load(relaxed)) {}  assert(data == 42);
//     答：錯，assert 可能失敗。relaxed 只保證 ready 自身的原子性與 modification order
//         一致性，完全不建立 synchronizes-with，因此對非原子的 data 沒有任何可見性
//         保證；而 data 被兩個 thread 無同步地存取，本身就構成 data race → UB。正確
//         寫法是 store 用 release、load 用 acquire。
//     為什麼會錯：腦中的錯誤模型是「relaxed 只是不加屏障，值遲早會傳過去，頂多晚一點」。
//         實際上 relaxed 完全不排序其他變數的存取，編譯器也可自由重排那兩行。
//         更麻煩的是這在 x86 上幾乎測不出來：x86-TSO 不允許 StoreStore/LoadLoad 重排，
//         硬體恰好給了保證，但編譯器層級的重排仍可能發生，換到 ARM 立刻壞掉。
//         結論：記憶體序要靠推理與 code review 驗證，不能靠測試。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cassert>
#include <string>

// =============================================================
// PART 0 —— 誰會去重新排序什麼
//
// 兩層會對記憶體操作進行重排:
//
//   1. 編譯器。它會自由地重排、合併、消除、上提、下沉
//      讀與寫 —— 只要單一執行緒看不出差異就行。預設情況
//      下,編譯器並不在意「還有別的執行緒存在」這件事。
//
//   2. CPU。現代 CPU 會 out-of-order 執行、預取、用 store
//      buffer。x86 上能被觀察到的重排很溫和 (只有 StoreLoad
//      重排)。ARM 與 POWER 則很積極:獨立的 load、store
//      與兩者交互的順序,從另一顆核心看可能是任意的。
//
// 帶有正確 memory_order 的 std::atomic 會同時告訴 *兩層*
// 哪些重排在這個操作上 *不被允許*。這就是記憶體順序存在
// 的全部用途。
//
// =============================================================
// 你需要知道的四種順序
//
//   memory_order_relaxed
//       是 atomic,但與同一執行緒中的其他記憶體操作 *無序*。
//       只有當你只在意 *這個變數本身的值*、其他什麼都不在
//       意時才使用。經典範例:點擊計數器。
//
//   memory_order_acquire   (僅用於 load)
//       這個 load 之後,本執行緒中後續的 load 與 store 都
//       不能被重排到 acquire 之上。它與同一個 atomic 上的
//       release 配對,以接收發布者在它的 store 之前所做的
//       一切寫。
//       心智模型:「我在讀一個旗標。發布者在翻起旗標之前
//       做的事,現在對我都可見了」。
//
//   memory_order_release   (僅用於 store)
//       這個 store 之前,本執行緒中前面的 load 與 store 都
//       不能被重排到 release 之下。它與同一個 atomic 上的
//       acquire 配對。
//       心智模型:「我在翻起旗標。我在這個 store 之前做
//       的一切,將對任何用 acquire 讀到旗標的人可見」。
//
//   memory_order_seq_cst   (預設)
//       acquire + release,再加上跨所有執行緒、所有 seq_cst
//       操作的單一全域全序。最強、最簡單、最貴。
//
//   memory_order_acq_rel   (只用於 RMW —— fetch_add、
//                           exchange、compare_exchange_*)
//       對於同時需要 publish/subscribe 兩半的「讀-改-寫」
//       操作的自然選擇。
//
//   memory_order_consume
//       原本想成為 acquire 的廉價版,只負責「依存關係鏈」。
//       沒有任何主流編譯器把它實作好;每個標準函式庫都
//       把它升級成 acquire。直接無視。
// =============================================================


// =============================================================
// PART 1 —— 對「獨立」計數器,relaxed 是 OK 的
//
// Lesson 04 那個計數器並不在意任何 *其他* 記憶體操作的
// 順序 —— 沒有其他操作要協調。我們在 join 完執行緒後才去
// 看最後的值 (join 本身就是個完整的同步點,等同 seq_cst)。
// 所以這裡用 relaxed 是正確的。
//
// 比較時間:在 x86 上,這應該比預設的 seq_cst 略快一點點
// 但確實量得到;在 ARM 上,差距會大得多。
// =============================================================

struct Stopwatch {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    long long ns() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::steady_clock::now() - t0).count();
    }
};

static long long bench_counter_seq_cst(int threads, int iters)
{
    std::atomic<long long> c{0};
    Stopwatch sw;
    std::vector<std::thread> ts;
    ts.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        ts.emplace_back([&]{
            for (int j = 0; j < iters; ++j) {
                c.fetch_add(1);    // 預設:memory_order_seq_cst
            }
        });
    }
    for (auto& t : ts) t.join();
    auto ns = sw.ns();
    if (c.load() != 1LL * threads * iters) std::abort();   // 安全檢查
    return ns;
}

static long long bench_counter_relaxed(int threads, int iters)
{
    std::atomic<long long> c{0};
    Stopwatch sw;
    std::vector<std::thread> ts;
    ts.reserve(threads);
    for (int i = 0; i < threads; ++i) {
        ts.emplace_back([&]{
            for (int j = 0; j < iters; ++j) {
                // relaxed:仍然是 atomic,但這裡沒有任何
                // 記憶體屏障。在每個架構上都比較便宜。
                c.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : ts) t.join();
    auto ns = sw.ns();
    if (c.load() != 1LL * threads * iters) std::abort();
    return ns;
}


// =============================================================
// PART 2 —— RELEASE / ACQUIRE 模式
//
// C++ 中最重要的單一無鎖模式。它正是 std::mutex 內部
// unlock/lock 的運作方式,也是生產者把剛建好的值「發布」
// 給消費者所用的方式。
//
//   發布者 (Publisher):                 訂閱者 (Subscriber):
//   payload = build_message();           while (!ready.load(acquire)) {}
//   ready.store(true, release);          use(payload);   // 安全
//
// 為什麼這樣是正確的:
//   - `ready` 上的 release 說「在我之前的本執行緒寫,都
//     不可被重排到我之後」。所以填好 `payload` 的那些寫
//     都 happen-before `true` 這個 store。
//   - `ready` 上的 acquire 說「在我之後的本執行緒讀,都
//     不可被重排到我之前」。所以讀 `payload` 的那些讀都
//     happen-after「看到 true」這次 load。
//   - 合起來:「release 之前的所有東西,在對應的 acquire
//     之後都可見」。這在標準裡叫做「synchronizes-with」
//     關係。
//
// 為什麼把 `ready` 改成 relaxed 還 *不夠*:
//   - 兩邊都用 relaxed 時,編譯器或 CPU 可以把 payload
//     的寫排到 ready store 之後,或把 payload 的讀排到
//     ready load 之前。訂閱者可能會看到 ready=true 卻在
//     讀一個還沒寫完的 payload。
//   - 在 x86 上,你大概在測試裡永遠看不到這件事,因為
//     x86 不會重排這幾種特定情形。在 ARM 上絕對會。
//     不要相信你的筆電。
// =============================================================

namespace msgpassing {

struct Message {
    int    a = 0;
    int    b = 0;
    std::string text;
};

Message              payload;          // 普通的、非 atomic 的資料
std::atomic<bool>    ready{false};

void publisher()
{
    // 用普通、非 atomic 的寫操作把訊息填好。
    payload.a    = 7;
    payload.b    = 42;
    payload.text = "hello from another thread";

    // 發布。release 屏障保證上面那三個寫不會被重排到這個
    // store 之後。
    ready.store(true, std::memory_order_release);
}

void subscriber()
{
    // 自旋等待,直到看到發布為止。acquire 保證下面的讀都
    // 不會被重排到這個 load 之前。
    while (!ready.load(std::memory_order_acquire)) {
        // 在這個小示範裡用 tight loop 是沒關係的
    }

    // 安全:發布者在它的 release store 之前所做的每一個
    // 寫,現在對我們都是可見的。我們可以放心讀那些普通的
    // (非 atomic 的) 欄位。
    std::cout << "[subscriber] payload.a = " << payload.a
              << ", payload.b = "            << payload.b
              << ", text = \""               << payload.text << "\"\n";

    assert(payload.a == 7);
    assert(payload.b == 42);
}

} // namespace msgpassing


// =============================================================
// PART 3 —— 用 acquire/release 自製一個迷你「自旋鎖」
//
// std::mutex (在無爭用的快路徑之外) 最終就是建立在這個
// 模式上:一個旗標、lock 上的 acquire、unlock 上的
// release。看一次這個就能把「mutex 到底在底層做什麼」
// 變得不再神秘。
//
// 真實程式碼裡 *不要* 出貨自製的自旋鎖。std::mutex 在
// 爭用嚴重時更快,因為它知道如何呼叫核心去 *睡覺*,
// 而不是燒掉 CPU。
// =============================================================

class Spinlock {
public:
    void lock()
    {
        // 嘗試把 false -> true。exchange 回傳 *舊* 值。
        // 如果舊值就是 true,代表別人已經持有鎖;繼續
        // 自旋重試。
        //
        // 成功路徑上的 acquire 順序代表:在 lock() 之 *下*
        // 的任何記憶體操作都不能往 *上* 移。我們正要進入
        // 臨界區。
        while (flag_.exchange(true, std::memory_order_acquire)) {
            // 也可以視情況加: __builtin_ia32_pause() / std::this_thread::yield()
        }
    }

    void unlock()
    {
        // release 順序代表:在 unlock() 之 *上* 的任何記憶體
        // 操作都不能往 *下* 移。我們正要離開臨界區,且我們
        // 希望我們在裡面做的所有事,都對下一個取得鎖的人
        // 可見。
        flag_.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> flag_{false};
};


// =============================================================
// MAIN —— 跑各個 PART
// =============================================================
int main()
{
    // ---------- PART 1: counter, seq_cst vs relaxed ----------
    {
        constexpr int THREADS = 4;
        constexpr int ITERS   = 5'000'000;

        // 各跑兩次,取第二次,讓 cache 暖機。
        bench_counter_seq_cst (THREADS, ITERS);
        bench_counter_relaxed (THREADS, ITERS);

        auto seq = bench_counter_seq_cst(THREADS, ITERS);
        auto rlx = bench_counter_relaxed(THREADS, ITERS);

        std::cout << "[counter ] seq_cst = " << seq / 1'000'000 << " ms"
                  << "   relaxed = "         << rlx / 1'000'000 << " ms"
                  << "   speedup = "
                  << (static_cast<double>(seq) / static_cast<double>(rlx))
                  << "x\n";
    }

    // ---------- PART 2: message passing ----------
    {
        std::thread p(msgpassing::publisher);
        std::thread s(msgpassing::subscriber);
        p.join();
        s.join();
    }

    // ---------- PART 3: 用 atomics 組出來的 spinlock ----------
    {
        Spinlock         lock;
        long long        sum = 0;
        constexpr int    N   = 100'000;
        constexpr int    T   = 4;

        std::vector<std::thread> ts;
        for (int t = 0; t < T; ++t) {
            ts.emplace_back([&]{
                for (int i = 0; i < N; ++i) {
                    lock.lock();
                    sum += 1;            // 普通、非 atomic 的 int
                    lock.unlock();
                }
            });
        }
        for (auto& t : ts) t.join();

        std::cout << "[spinlock] sum = " << sum
                  << "   expected = " << (long long)T * N << '\n';
    }

    // ---------- 實戰 1: ready flag 發布 config ----------
    //
    // 應用場景: 啟動階段一個 worker 載入 config (網路、檔案、
    // DB), 完成後設 ready_=true; 其他 thread polling ready_,
    // true 後讀 config_。
    //
    // 為什麼要 release/acquire 而不是預設 seq_cst?
    // 預設也對, 只是 ARM 上每次 load 都要 fence。改成
    // ready_.store(release) + ready_.load(acquire) 是最小成本
    // 但仍保證「subscriber 讀到 ready_==true 之前, 一定看到
    // publisher 對 config_ 的所有寫入」。這是 publish/subscribe
    // 的標準食譜。
    {
        std::cout << "\n[demo] publish/subscribe via release/acquire\n";
        struct Config { int port = 0; std::string name; };
        Config            cfg;
        std::atomic<bool> ready{false};

        std::thread publisher([&]{
            // 假裝在載入
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            cfg.port = 8080;
            cfg.name = "main-service";
            ready.store(true, std::memory_order_release);   // publish
        });
        std::thread subscriber([&]{
            // 等到 ready 為 true ── acquire 保證之後 cfg 已可見
            while (!ready.load(std::memory_order_acquire))
                std::this_thread::yield();
            std::cout << "  got config: port=" << cfg.port
                      << " name=" << cfg.name << '\n';
        });
        publisher.join(); subscriber.join();
    }

    // ---------- 實戰 2: relaxed 計數器 (純統計用) ----------
    //
    // 應用場景: hot path 的事件計數 (例如「處理的 request 數」),
    // 只有「監測 thread」每隔 1 秒讀一次, 不會用這個計數做任何
    // 決策 (i.e., 它跟其他變數沒 happens-before 依賴)。
    //
    // 在這種「不需要與其他變數同步、只關心自己最後的數字
    // 對不對」的場景, memory_order_relaxed 就夠 ── 在 ARM 上
    // 可以省掉 dmb 指令, 計數器吞吐顯著提升。
    {
        std::cout << "\n[demo] relaxed event counter (純統計)\n";
        std::atomic<long long> events{0};
        std::vector<std::thread> ws;
        for (int t = 0; t < 4; ++t) {
            ws.emplace_back([&]{
                for (int i = 0; i < 100'000; ++i)
                    events.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& t : ws) t.join();
        // 最後讀一次 → join 完成後天然 happens-before 已建立
        std::cout << "  events = " << events.load() << " (預期 400000)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：release 跟 acquire 一定要配對嗎? 怎麼配?
    //    A：必須是「同一個 atomic 變數」上的 store(release) 配
    //       load(acquire)。release 端把它之前的所有寫入「打包」, acquire
    //       端讀到那個值之後, 看得到所有打包內容 (happens-before)。配
    //       錯變數 (release 寫 X、acquire 讀 Y) 完全沒同步保證, 是
    //       lock-free 程式碼最常見的隱藏 bug, TSan 才能抓出來。
    //
    //  Q2：seq_cst 比 acq_rel 多了什麼?
    //    A：acq_rel 只保證「單一變數的 happens-before」。seq_cst 額外
    //       保證「所有 seq_cst 操作有一個全 thread 一致的全域順序」
    //       (single total order)。這對 Dekker/Peterson 之類「兩個變數
    //       的 store 順序」必要。x86 上幾乎免費, ARM 要插 dmb ish, 較
    //       貴。預設用 seq_cst, 量過才下調。
    //
    //  Q3：memory_order_consume 是什麼? 為什麼幾乎沒人用?
    //    A：consume 原本想表達「跟著 dependency chain 流動的 acquire」,
    //       理論上更省。但實作太難 (要追蹤 data dependency), 所有主流
    //       編譯器都直接把 consume 升級成 acquire, 沒省到任何成本。
    //       P0735 已建議在標準裡 deprecated。實務上四種就夠: relaxed/
    //       acquire/release/seq_cst (加上 RMW 用的 acq_rel)。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 預設值是正確的。每個 std::atomic 操作的預設都是
//    memory_order_seq_cst。對你以後會寫的所有程式都是
//    安全的。把它調弱的唯一理由是 *量測過* 的效能需求。
//
// 2. 發布/訂閱的食譜。
//        publisher: write data; flag.store(true, release);
//        subscriber: while (!flag.load(acquire)) {}; read data;
//    把這個形狀背起來。每個無鎖的交接都是它的某種變形。
//
// 3. relaxed 只能用於「彼此獨立的變數」。
//    在以下場合使用 relaxed:
//      - 那個 atomic 值本身就是全部關注點 (一個計數器、
//        一個統計值、一個 UID 產生器)。
//      - 你不在意其他執行緒「何時」看到變化,只要在後續
//        join 或 seq_cst 操作之後計數正確即可。
//    *不要* 用 relaxed 去「發布」或「訂閱」任何東西。
//
// 4. acq_rel 用於 RMW。
//    fetch_add、exchange、compare_exchange_* 同時包含 load
//    與 store 兩半。當你需要「兩半都有順序」時,傳入
//    memory_order_acq_rel。
//
// 5. consume 是死的。把它當成「壞掉的 acquire」即可。
//
// 6. 在 x86 上測試是個陷阱。
//    x86 的 TSO 記憶體模型把你能寫出來的弱順序 bug 大
//    多數都遮起來。在你的 Intel/AMD 筆電上「沒事」的
//    程式碼,可能在 M 系列 Mac、ARM 伺服器、Android
//    裝置和嵌入式平台上會壞掉。請使用 thread sanitizer
//    (g++/clang: -fsanitize=thread),並讓懂規則的人逐一
//    審核你做的每個弱順序選擇。
//
// 7. compare-exchange:兩個順序,不是一個。
//        flag.compare_exchange_strong(expected, desired,
//             /*成功時 */ std::memory_order_acq_rel,
//             /*失敗時 */ std::memory_order_acquire);
//    很多人忘了「失敗時」這個參數。當交換失敗 (沒有寫
//    發生) 時,該操作其實只是個 load,所以失敗路徑上
//    不需要 release —— 但你仍然需要 acquire 來看到最新
//    的值。要嘛把這個參數寫對,要嘛就用單一參數的版本
//    (各處全部用 seq_cst —— 安全但較慢)。
//
// 8. 拿不定主意時,就用 mutex。std::mutex 速度快、人們
//    熟悉,而且不可能在順序上寫錯。「無鎖」不是目標;
//    「快又正確」才是。
// =============================================================
