// =============================================================
// 19_hazard_pointers.cpp  --  Hazard pointers + lock-free Treiber stack
// =============================================================
//
// 本課目標:
//   1. 看到「無鎖容器」的核心難題:當你 pop 出一個 node 之後,
//      你還能不能 *安全 delete* 它?
//   2. 學會 Hazard Pointers (HP) —— Maged Michael 提出的、寫
//      lock-free 容器時最常見的記憶體回收方案。
//   3. 自己組一個 *可以刪除節點* 的 Treiber stack,作為前面
//      所有課的綜合應用 (atomic、CAS、release/acquire、
//      cache-line padding、thread_local)。
//
// 注意:這是 *教學版*,簡化過。生產環境請用 folly::HazPtr、
// concurrentqueue、cds 之類經過驗證的函式庫。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 19_hazard_pointers.cpp -o 19_hazard_pointers
//
// 執行方式:
//     ./19_hazard_pointers
// =============================================================
//
// 為什麼 lock-free pop 很危險?
//
// Treiber stack 的 pop 看起來這麼單純:
//     do {
//         old_head = head.load();
//         if (!old_head) return;
//         new_head = old_head->next;
//     } while (!head.compare_exchange_weak(old_head, new_head));
//     delete old_head;     // <<< 危險!
//
// 如果在我讀 old_head->next 的時候,*另一個執行緒* 已經 pop
// 走 old_head,並把它 delete 了 —— 我手上就是個野指標。
//
// 解法是 Hazard Pointer:每條執行緒有一個全域可見的「我正在
// 看 X」的 slot。我先把 old_head 寫進那個 slot,*再* 確認
// head 還是 old_head,再去讀 ->next。回收時看一遍所有 slot,
// 還沒在任何 slot 裡就可以安全 delete。
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Hazard Pointers (HP) + Treiber lock-free stack
// 前置課程: lesson 04, 11, 13, 16
// 觀念詞彙:
//   - hazard pointer  ── 「我正在看 X」的全域可見 slot
//   - retire list     ── 等待回收的 node 清單
//   - Treiber stack   ── 用 CAS 實作的 lock-free stack
//   - ABA problem     ── pop+push 同一指標但中間換過,CAS 看不出
// 新介紹 API:
//   std::atomic<void*>           hazard pointer slot 的型別
//   std::atomic<Node*>            head 指標,需 CAS 操作
//   compare_exchange_weak        Treiber pop 的核心
//   thread_local                 每個 thread 認領自己的 HP slot
// 何時使用:
//   - 寫 lock-free 容器,需要安全回收 popped node
//   - 比 RCU 適合「每次操作只持一個熱指標」的場景
// 何時不要用:
//   - 不需要 lock-free → 普通 mutex + std::stack 早已夠用
//   - thread 數量無上限 → HP 通常需要固定上限
//   - 教學以外 → 用 folly::HazPtr / cds / concurrentqueue
// 常見錯誤:
//   - 設 HP 後沒 *再次驗證* head 沒變 → race window
//   - 認為設了 HP 就解決一切 → 仍要看 ABA 等問題
//   - thread_local retire list 在 thread 結束時遺漏 → 寫一個 RAII drain
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── HP / ABA / 三大 reclamation 比較
// =============================================================
//
// 1. lock-free 結構的「誰來 free」難題
//    Treiber stack 的 pop:
//        do {
//            Node* h = head.load(acquire);          // (1) 讀 head
//            if (!h) return false;
//            Node* next = h->next;                  // (2) 讀 h->next  ← 危險!
//        } while (!head.CAS(h, next));
//        delete h;                                  // (3)
//    若 thread A 卡在 (2) 之前,thread B 已 pop 並 delete h → A 在 (2)
//    讀 h->next 是 use-after-free → UB。
//    解法核心:在讀 h 之前,先公告「我正在用 h」。這就是 hazard pointer。
//
// 2. HP 的協議
//    每條 thread 擁有一個 (或 K 個) thread_local hazard slot。
//    reader:
//        do {
//            h = head.load(acquire);                // (a) 讀
//            hp_slot = h;                           // (b) 公告
//        } while (h != head.load(acquire));         // (c) 重檢一次
//        // 從這裡開始 h 安全
//    writer (retire):
//        retired_list.push(h);                       // 不立刻 free
//        if (retired_list.size() > threshold)
//            scan_and_free();
//    scan_and_free:
//        snapshot = 所有 thread 的 hazard slots;
//        for each n in retired_list:
//            if (n not in snapshot) free(n);
//
//    重檢 (c) 是必要的:在 (a) 與 (b) 之間 writer 可能已經把 h 換掉並
//    開始 retire,reader 公告了一個過期指標 → 重讀一次保證公告的還是
//    當前的 head。
//
// 3. ABA 問題
//    pop 的 CAS:`head.CAS(old=h, new=next)`。若有人 pop 了 h,然後 push
//    了 *相同地址的新節點* (例如 free list 重用了那塊 memory),CAS 會
//    成功 ── 但 next 已不是當前 head 後面那個。stack 結構就壞了。
//    解法:
//      A. tagged pointer:把 ABA counter 塞進指標的 16 個高位 bit (x86-64
//         有 48-bit 虛擬位址)。每次 push counter++。
//      B. HP:配 HP 的同時延遲 free,h 永遠不會「被回收又重出現」── 所以
//         CAS 看到 h 就是 h。
//      C. RCU/epoch:同樣延遲回收。
//
// 4. 三大 reclamation 方案比較
//      Reference counting (atomic shared_ptr)
//        ❌ ref-count 變成 cache 熱點,reader 之間爭用
//        ✅ 概念簡單、自動
//
//      Hazard Pointers
//        ✅ Reader 路徑只 touch thread_local,不爭用
//        ❌ 每個 thread 的 hazard slots 需要管理;writer 要 scan
//        ❌ 上限固定 (K hazards / thread),不適合複雜結構
//
//      RCU / epoch
//        ✅ Reader 完全免費 (進 RCU read-side critical section 只是 inc
//           thread_local epoch counter)
//        ❌ 必須有 grace period 機制 (定期確定無 reader 在舊版本)
//        ❌ writer-heavy 時 retire 累積,記憶體膨脹
//
//    經驗法則:reader ≫ writer,單純結構 → HP;非常多 reader 且結構複雜 →
//    RCU;一般情況不講求極限效能 → shared_mutex 或 atomic shared_ptr。
//
// 5. 為什麼 ABA 不會在 mutex queue 出現
//    mutex 串列下,pop 的 read-h-and-check-next 整段在臨界區內,沒人能
//    在中間插隊;結構不可能「同地址但邏輯換人」。所以 ABA 是 lock-free
//    結構的特產。
//
// 6. retire threshold 怎麼選
//    過小:scan 太頻繁,效能下降。
//    過大:retire list 累積,記憶體浮高。
//    經驗法則:K_MAX_HAZARDS_PER_THREAD * NUM_THREADS * 2 左右開始。
//    folly::Hazptr 預設 ~1000。
//
// 7. thread 結束時的清理
//    thread_local retire list 在 thread 退出時必須被「捐」給某個全域 list,
//    讓其他 thread 接手 scan;否則這些節點就遺漏。實作可在 thread_local
//    物件的解構子做 ── 這也是為什麼正確的 HP 實作不簡單。folly::Hazptr
//    /folly::Synchronized 是工業界最成熟參考。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── HP 太底層,LC 不考。
//   → lesson 30 任一題目若你想做 lock-free 升級,本課的 HP 就是 reclamation
//     的標準解。例如 Q5 BoundedBlockingQueue 改成 lock-free Treiber stack,
//     就需要 HP 來解決「pop 中讀 next 卻被 free」的 use-after-free。
//
// 主要 API 對照 (cppreference):
//   - std::atomic<T*>                   https://en.cppreference.com/w/cpp/atomic/atomic
//   - compare_exchange_weak/strong      https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
//   - thread_local                      https://en.cppreference.com/w/cpp/language/storage_duration
//   - 提案:std::hazard_pointer (P2530) 已被 C++26 接受,標準預期 C++26
//                                       https://en.cppreference.com/w/cpp/memory/hazard_pointer
//
// 練習建議:
//   - 把本課的 Treiber stack 套到 lesson 30 Q5 BoundedBlockingQueue 的
//     生產/消費 pattern,試著做出 lock-free 版本。觀察 throughput 變化。
//   - 進階:userspace RCU (liburcu) 是另一種 reclamation 思路,適合
//     reader 比 writer 多很多的場景。
// =============================================================

/*
補充筆記：hazard_pointers
  - hazard_pointers 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - hazard_pointers 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Hazard pointer 與無鎖結構的安全記憶體回收
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Hazard pointer 解決什麼問題？
//     答：無鎖資料結構的「安全記憶體回收（SMR）」。thread A 剛把節點從結構移除、想
//         delete 它，但 thread B 手上可能還握著指向它的指標，直接 delete 就是 UB。
//         Hazard pointer 讓每個讀者把「我正在存取的指標」發布到一個全域可見的槽位；
//         回收者在真正 delete 前掃描所有槽位，若該指標仍被宣告，就先放進 retire list
//         延後處理。替代方案有 epoch-based reclamation、RCU、引用計數。
//     追問：成本在哪？（每次讀都要一次 store 加上必要的順序保證；回收時要掃描所有槽位，
//           因此通常累積到一定數量才批次回收）
//
// 🔥 Q2. 什麼是 ABA 問題？怎麼解？
//     答：thread 1 讀到指標值 A 準備 CAS；期間 thread 2 把 A 改成 B 再改回 A（中間
//         可能已釋放 A 並重新配置到同一位址）。thread 1 的 CAS 看到 A 就成功，誤以為
//         「沒人動過」，實際上結構已被破壞。解法：(1) tagged pointer／版本號，把指標
//         與遞增計數器一起比對；(2) hazard pointer；(3) epoch-based reclamation 或
//         RCU；(4) 根本上避免記憶體重用。
//     追問：為什麼 ABA 主要出現在指標而非計數器？（計數器的值回到原值通常無害，但
//           指標回到原值代表「同一位址、不同物件」，語意完全不同）
//
// Q3. Hazard pointer 為什麼順帶解決了 ABA？
//     答：因為它從根本上阻止了「被讀者持有的記憶體被釋放並重新配置」。只要位址不會
//         在讀者眼皮底下被重用，就不會出現「同一個位址其實已是另一個物件」的情況。
//     追問：那還需要版本號嗎？（若結構本身允許節點被合法地移除又重新插入同一個節點
//           物件，仍可能需要版本號來區分邏輯上的新舊）
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>
#include <array>
#include <cstddef>

constexpr std::size_t MAX_THREADS  = 64;
constexpr std::size_t CACHE_LINE   = 64;

// -------------------------------------------------------------
// HP slot:每條執行緒一個。寫者 (本執行緒) 用 release,讀者
// (回收 thread) 用 acquire。
// -------------------------------------------------------------
struct alignas(CACHE_LINE) HazardSlot {
    std::atomic<void*> ptr{nullptr};
};

static std::array<HazardSlot, MAX_THREADS> g_hp_slots;
static std::atomic<std::size_t>            g_next_slot{0};

// 每條執行緒在第一次用 HP 時,認領一個 slot
static thread_local std::size_t my_slot_idx = []{
    auto i = g_next_slot.fetch_add(1, std::memory_order_relaxed);
    if (i >= MAX_THREADS) {
        std::cerr << "exceeded MAX_THREADS\n";
        std::abort();
    }
    return i;
}();


// -------------------------------------------------------------
// 「待回收」清單:每條執行緒一份,thread_local。
// 達到一定大小才掃 HP slot 一次,把不在 hazard 的 node delete。
// -------------------------------------------------------------
template <typename T>
struct RetireList {
    std::vector<T*> nodes;

    void retire(T* p)
    {
        nodes.push_back(p);
        if (nodes.size() >= 32) scan();   // 攤銷:每 32 個掃一次
    }

    void scan()
    {
        // 1. 蒐集當下所有 thread 的 hazard pointer
        std::vector<void*> hazards;
        std::size_t n_slots = g_next_slot.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < n_slots; ++i) {
            void* p = g_hp_slots[i].ptr.load(std::memory_order_acquire);
            if (p) hazards.push_back(p);
        }

        // 2. 把不在 hazards 中的 node 釋放,留下還在 hazard 的
        std::vector<T*> still_pending;
        still_pending.reserve(nodes.size());
        for (T* p : nodes) {
            bool in_hazard = false;
            for (void* h : hazards) {
                if (h == p) { in_hazard = true; break; }
            }
            if (in_hazard) still_pending.push_back(p);
            else           delete p;
        }
        nodes = std::move(still_pending);
    }

    ~RetireList() { for (T* p : nodes) delete p; }   // 程式結束清掉
};


// -------------------------------------------------------------
// Lock-free Treiber stack,使用 HP 安全回收
// -------------------------------------------------------------
template <typename T>
class LockFreeStack {
    struct Node {
        T     value;
        Node* next;
    };

    std::atomic<Node*> head_{nullptr};

public:
    ~LockFreeStack()
    {
        // 析構時 (假設沒人在用),把剩下的鏈通通釋放。
        Node* p = head_.load(std::memory_order_acquire);
        while (p) {
            Node* n = p->next;
            delete p;
            p = n;
        }
    }

    void push(T v)
    {
        Node* node = new Node{std::move(v), nullptr};
        Node* cur  = head_.load(std::memory_order_relaxed);
        do {
            node->next = cur;
        } while (!head_.compare_exchange_weak(
                    cur, node,
                    std::memory_order_release,    // 成功:發布 node 內容
                    std::memory_order_relaxed));
    }

    bool try_pop(T& out)
    {
        auto& my_hp = g_hp_slots[my_slot_idx].ptr;

        Node* cur;
        while (true) {
            cur = head_.load(std::memory_order_acquire);
            if (!cur) return false;

            // ★ 設下 hazard:告訴所有人「我正在看 cur」
            my_hp.store(cur, std::memory_order_release);

            // 再次檢查 head 是否仍然是 cur。如果不是,代表
            // 在我們 store hazard 之前 head 就已經被換過了 ——
            // 重來。
            if (head_.load(std::memory_order_acquire) != cur) continue;

            // 現在 cur 在我們的 hazard slot 內了 —— 別的回收
            // thread 看到後就不會 delete 它,我們可以安全地讀 next。
            Node* next = cur->next;

            // 嘗試把 head 從 cur -> next
            if (head_.compare_exchange_weak(
                    cur, next,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                break;       // 成功 pop 出 cur
            }
            // CAS 失敗,代表別人插隊了。繼續。
        }

        out = std::move(cur->value);

        // 把 cur 從 hazard 移走 (我們不再讀它),並送進 retire。
        my_hp.store(nullptr, std::memory_order_release);

        thread_local RetireList<Node> rl;
        rl.retire(cur);
        return true;
    }
};


// =============================================================
// DEMO + Stress test
// =============================================================
int main()
{
    LockFreeStack<int> stack;

    constexpr int      THREADS_EACH = 4;
    constexpr long long N_PER_THREAD = 200'000;

    std::atomic<long long> popped_sum{0};

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> producers;
    for (int i = 0; i < THREADS_EACH; ++i) {
        producers.emplace_back([&, i]{
            for (long long j = 0; j < N_PER_THREAD; ++j) {
                // 每個 thread push 它自己的「id 區段」內的數字,
                // 這樣總和有可預測性
                stack.push(static_cast<int>(i * N_PER_THREAD + j));
            }
        });
    }

    std::vector<std::thread> consumers;
    std::atomic<long long> total_popped{0};
    long long expected_total = static_cast<long long>(THREADS_EACH) * N_PER_THREAD;

    for (int i = 0; i < THREADS_EACH; ++i) {
        consumers.emplace_back([&]{
            int v;
            long long local_sum   = 0;
            long long local_count = 0;
            while (total_popped.load(std::memory_order_relaxed) < expected_total) {
                if (stack.try_pop(v)) {
                    local_sum += v;
                    ++local_count;
                    total_popped.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::this_thread::yield();
                }
            }
            popped_sum.fetch_add(local_sum, std::memory_order_relaxed);
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();

    // 驗算:總和應該等於 0+1+2+...+(expected_total-1)
    long long expected_sum = expected_total * (expected_total - 1) / 2;

    std::cout << "[HP stack] " << expected_total << " push+pop in "
              << ms << " ms\n";
    std::cout << "[HP stack] sum = " << popped_sum.load()
              << "  expected = " << expected_sum
              << "  " << (popped_sum.load() == expected_sum ? "OK" : "MISMATCH!") << '\n';

    // ---------------------------------------------------------
    // 實戰 1: HP 保護的 stack 當 lock-free undo-stack
    // ---------------------------------------------------------
    // 應用場景: 多人協作編輯器, 多條 worker thread 並行對 doc
    // 做變更, 每次變更 push 一個 Command 進 undo stack; user 按
    // ctrl+Z 就 pop 一個。用 HP-stack 比加鎖 stack 在熱迴圈中
    // 約 2× 吞吐 (但記憶體開銷較大, 因為 retire 是延後)。
    //
    // 這裡用 small workload 示意, 不跑 benchmark。
    {
        std::cout << "\n[demo] HP stack as undo-stack (2 threads, 5 cmds)\n";
        LockFreeStack<int> undo;
        std::thread w1([&]{ for (int i = 1; i <= 5; ++i) undo.push(i * 10); });
        std::thread w2([&]{ for (int i = 1; i <= 5; ++i) undo.push(i * 100); });
        w1.join(); w2.join();

        int v;
        std::cout << "  pop order:";
        while (undo.try_pop(v)) std::cout << ' ' << v;
        std::cout << '\n';
    }

    // ---------------------------------------------------------
    // 實戰 2: HP slot 數量與 max concurrent readers 的關係
    // ---------------------------------------------------------
    // 教學重點: HP 的 slot 上限決定「同時間最多幾條 thread 能進
    // 入 critical section 讀 stack」。本課固定 MAX_HP_SLOTS, 超
    // 過會 assert 或 retire 掃不乾淨。生產環境通常用動態擴展。
    {
        std::cout << "\n[note] HP slot usage observation\n";
        std::cout << "  上面 stress test 共用了 ~" << (THREADS_EACH * 2)
                  << " 條 producer+consumer thread, 各佔 1 個 hazard slot\n";
        std::cout << "  → 生產系統 thread 數會動態變化, "
                  << "folly::HazPtr 採動態 grow 才安全\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：HP 跟 epoch-based reclamation (EBR) 的差別在哪?
    //    A：HP 每個 reader 在 hazard slot 上「明確公告當下正在讀哪個指標」,
    //       writer 掃 slot 才能釋放;粒度細、即時,但每筆 read 多兩次
    //       atomic store/load。EBR (crossbeam-epoch) 只是把 thread 的
    //       「epoch counter」 +1 進入 critical section、退出 -1,writer
    //       延後到「所有 thread 都離開該 epoch」才釋放;reader 路徑更便宜
    //       (一個 thread_local 寫),代價是釋放時機較粗 (整批延遲)。
    //
    //  Q2：為什麼設下 hazard 後還要再 load 一次 head_?
    //    A：在 load(head_) 與 store(hp_slot, h) 之間有窗口:writer 可能
    //       已經 pop 並 retire 掉 h。第二次 load 確認「我設下 hazard 時
    //       head_ 仍然是 h」,保證 retire-list scan 一定會看到我這格 → 不
    //       會被 free。這是經典的 publish-then-validate (announce-and-recheck)
    //       模式,所有 HP 與 RCU 都用得到。
    //
    //  Q3：自己寫一個正確的 HP 實作要處理哪些細節?為什麼工業界推薦用現成函式庫?
    //    A：(1) thread 結束時 retire list 必須「捐」給其他 thread 不能丟;
    //       (2) hazard slot 數量上限要動態擴展;(3) per-NUMA retire list 平衡;
    //       (4) ABA 問題的 corner case (HP 配 CAS 雖然解 free 但 sequence
    //       仍要小心);(5) scan threshold 調校。folly::Hazptr 與 cds 已被
    //       生產驗證,自己寫對的機率極低 ── 教學版懂原理就好,實戰用函式庫。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. Lock-free 的「正確性」分兩層:
//      (a) 演算法本身對嗎?(這層可以用 CAS 推理)
//      (b) 釋放記憶體安全嗎?(被別人 *正在讀* 的 node 不可
//          以 delete)
//    HP 解的是 (b)。沒有 HP 的「無鎖 pop」其實只是 *看似*
//    無鎖,實際上 use-after-free 隨時會炸。
//
// 2. HP 的成本:
//      - 每個讀操作都要兩次 atomic store/load (設 hazard +
//        確認指標仍有效)。
//      - 攤銷的 retire scan 是 O(N_threads × N_retired)。
//    比起拿一把 mutex,HP 通常 *更快* 而且 wait-free,但比
//    純 atomic CAS 慢一點。
//
// 3. 替代方案的階梯:
//      (a) HP (本課) ── 每個 thread 一個 slot,適合「每次操作
//          只持有一個熱指標」的場景。Treiber stack、Michael
//          queue 都用這個。
//      (b) Epoch-based reclamation (EBR、crossbeam-epoch) ──
//          更便宜的「per-operation overhead」,代價是有時候
//          回收得慢。
//      (c) RCU ── 寫者付出更高代價但讀者完全免費 (lesson 18
//          的 CoW 快照是它的特例)。
//      (d) 直接用 std::shared_ptr ── 最簡單,但 atomic
//          shared_ptr 在熱路徑上的效能未必夠。
//
// 4. 為什麼設 hazard 後還要再檢查 head?
//    因為 store hazard 與 load head 之間有個窗口:
//        load head -> 別人 pop+delete -> store hazard -> 讀 ->next
//    第二次 load head 確認「我設下 hazard 時,head 仍然是 cur」,
//    保證沒被誰偷走過。經典的 publish-then-validate 模式。
//
// 5. 一個常見錯誤:把 hazard slot 設成 thread_local 但沒有
//    暴露給其他 thread 看。HP 的本質就是要「讓回收 thread
//    能掃到我這格」,所以 slot 一定要是 *全域* 可見、固定
//    位址的東西。本課用一個固定大小的 g_hp_slots 陣列。
//
// 6. 教學程式 vs 生產程式:
//      生產等級的 HP 還要處理 thread join 後 slot 的回收、
//      多個 hazard per operation、跨 NUMA 的 retire list 平衡、
//      動態 max threads。folly::HazPtr 是個好的閱讀範本。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 19_hazard_pointers.cpp -o 19_hazard_pointers

// === 預期輸出 ===
// [HP stack] 800000 push+pop in 228 ms
// [HP stack] sum = 319999600000  expected = 319999600000  OK
//
// [demo] HP stack as undo-stack (2 threads, 5 cmds)
//   pop order: 500 400 300 200 100 50 40 30 20 10
//
// [note] HP slot usage observation
//   上面 stress test 共用了 ~8 條 producer+consumer thread, 各佔 1 個 hazard slot
//   → 生產系統 thread 數會動態變化, folly::HazPtr 採動態 grow 才安全
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
