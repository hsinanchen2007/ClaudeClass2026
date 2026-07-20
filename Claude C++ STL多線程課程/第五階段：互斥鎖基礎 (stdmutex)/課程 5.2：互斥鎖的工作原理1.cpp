// =============================================================================
//  課程 5.2：互斥鎖的工作原理1.cpp  —  自旋鎖（Spinlock）與 Test-And-Set
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T> T std::atomic<T>::exchange(
//       T desired, memory_order order = memory_order_seq_cst) noexcept;   // C++11
//       → 原子地把值設為 desired，並回傳【設定前】的舊值（Test-And-Set 的核心）
//   void std::atomic<T>::store(T desired, memory_order = seq_cst) noexcept;
//   標頭檔：<atomic>
//   複雜度：exchange 在 x86-64 上編成單一 XCHG 指令（隱含 lock 前綴，
//           自帶完整記憶體屏障）。
//   ⚠️ 定位：本檔的 SimpleSpinlock 是【教學用】，用來說明鎖的底層原理，
//            不是生產環境該用的東西（理由見「注意事項」）。
//
// 【詳細解釋 Explanation】
//
// 【1. 一把鎖最少需要什麼：一個「不可分割的讀-改-寫」】
//   最直覺的鎖會這樣寫：
//       while (locked) { }     // 等到沒人鎖
//       locked = true;         // 換我鎖
//   這是【錯的】，而且錯得很經典：兩條執行緒可能同時通過 while
//   （都看到 locked == false），然後都設成 true，於是兩個都以為自己拿到鎖。
//   問題在於「檢查」與「設定」是兩個獨立步驟，中間有空隙。
//   解法是把兩步合成一個【硬體層級不可分割】的動作：
//       while (locked.exchange(true)) { }
//   exchange 一次完成「寫入 true 並回傳舊值」。
//     * 舊值是 false → 之前沒人鎖，而且【現在鎖已經是我設的】→ 取得成功
//     * 舊值是 true  → 已經有人鎖著，我剛剛的寫入沒有改變任何事實 → 繼續轉
//   這個原語叫 Test-And-Set（TAS），是所有鎖的共同基礎。
//
// 【2. 為什麼「自旋」有時比「睡眠」快】
//   睡眠（讓出 CPU、進入阻塞、之後被喚醒）需要進核心：
//   一次 context switch 大約 1~10 微秒，還會清掉 CPU 快取與 TLB 的暖度。
//   如果臨界區段只有「++counter」這種等級（幾奈秒），
//   那麼「原地轉幾十圈等它放手」遠比「睡下去再被叫醒」便宜。
//   反過來，如果持鎖者要做 I/O 或長計算，自旋就是純粹燒 CPU——
//   而且燒的還是別的執行緒本來可以用來完成工作的 CPU。
//   → 判準：臨界區段長度 vs context switch 成本。
//     這也正是 std::mutex 採「先自旋一小段、失敗再睡」混合策略的原因。
//
// 【3. unlock 為什麼只要一個 store】
//   釋放鎖不需要 exchange——只有持有者會呼叫 unlock，沒有競爭問題。
//   一個 store(false) 就夠了。真正重要的是它的【記憶體順序】：
//   預設 seq_cst（或至少要 release）保證「臨界區段內的所有寫入」
//   都排在這個 store 之前，於是下一個拿到鎖的執行緒
//   （它的 exchange 是 acquire）必然看得到那些寫入。
//   若把 unlock 寫成 locked.store(false, memory_order_relaxed)，
//   鎖的互斥性還在，但【可見性保證消失】，臨界區段保護的資料可能讀到舊值。
//
// 【4. 這個實作的三個真實缺陷】
//   (a) 不公平：純 TAS 自旋沒有排隊，可能同一條執行緒連續搶到，
//       某條倒楣的執行緒被餓死（starvation）。
//   (b) 快取行乒乓（cache-line ping-pong）：每次 exchange 都是寫入，
//       會把該快取行搶成 Exclusive 狀態，讓其他核心的副本失效。
//       N 條執行緒空轉就是 N 個核心互相搶同一條快取行，
//       匯流排流量爆增，反而讓真正的持有者變慢。
//       業界標準改良是 TTAS（Test-and-Test-And-Set）：
//           while (true) {
//               while (locked.load(std::memory_order_relaxed)) { /* 只讀 */ }
//               if (!locked.exchange(true)) break;
//           }
//       先用【唯讀】的 load 自旋（快取行可停在 Shared 狀態，不搶所有權），
//       看起來有機會了才做一次昂貴的 exchange。
//   (c) 沒有 PAUSE：x86 有專門的 PAUSE 指令提示 CPU「這是自旋迴圈」，
//       可降低功耗並避免離開迴圈時的記憶體順序違規管線清空懲罰。
//
// 【概念補充 Concept Deep Dive】
//   * exchange 在 x86-64 上：XCHG 指令對記憶體操作時【自動帶 lock 語意】，
//     不需要顯式加 lock 前綴，且是 full barrier。
//     在 ARM 上則是 LDAXR/STLXR 這組 load-exclusive/store-exclusive 迴圈，
//     由硬體的 exclusive monitor 保證不可分割。
//   * std::atomic<bool> 幾乎必然是 lock-free 的，可用 is_lock_free() 驗證。
//     若某型別太大而非 lock-free，std::atomic 會退化成內部加鎖，
//     那時「用 atomic 實作 spinlock」就變成一件很荒謬的事。
//   * 為什麼真實世界仍然需要自旋鎖：作業系統核心裡不能睡覺的情境
//     （中斷處理常式、持有排程器鎖時）只能自旋。
//     使用者空間程式幾乎沒有理由自己寫自旋鎖——std::mutex 已經內建混合策略。
//   * 本檔的 counter 結果【是確定的】：所有 ++counter 都在鎖內完成，
//     不存在 data race，所以必然是 200000。
//     這與「無鎖版本」形成對照——後者是 UB，結果不可預測。
//
// 【注意事項 Pay Attention】
//   1. 【不要在應用程式裡自己寫自旋鎖】。std::mutex 在無競爭時的成本
//      已經很低（本機 x86-64 上是一次原子操作），而且有混合策略與
//      作業系統排程器配合；自製自旋鎖幾乎一定更差。
//   2. 自旋鎖【不可重入】：同一執行緒再 lock 一次會永遠自旋（自己等自己）。
//   3. 持有自旋鎖時【絕不可】做 I/O、配置記憶體、或任何可能阻塞的事。
//   4. 在超額訂閱（執行緒數 > 核心數）的情況下自旋鎖表現會急遽惡化：
//      持有者可能被搶佔下 CPU，而其他執行緒還在原地空轉等它。
//   5. 本檔的 lock() 沒有 yield/pause，是刻意保留最小形式以凸顯 TAS 原理；
//      真要用請至少改成 TTAS + PAUSE。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】自旋鎖與 Test-And-Set
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 lock() 要用 exchange，不能寫成
//        「while (locked) {} locked = true;」？
//     答：那個寫法把「檢查」與「設定」拆成兩步，中間有空隙。
//         兩條執行緒可能同時看到 locked == false，然後雙雙設成 true，
//         兩個都以為拿到鎖 —— 互斥完全失效。
//         exchange 把讀與寫合併成硬體層級不可分割的一步（Test-And-Set），
//         回傳的舊值就是「在我寫入之前它是不是已經被鎖住」的權威答案。
//     追問：那可以改用 compare_exchange 嗎？→ 可以，語意等價，
//           而且更容易擴充成 TTAS 或加入其他狀態。
//
// 🔥 Q2. 自旋鎖什麼時候比 std::mutex 快？什麼時候會慘敗？
//     答：臨界區段極短（奈秒級）且執行緒數不超過核心數時，
//         自旋可以省下 context switch 的微秒級成本。
//         反之，臨界區段長、或執行緒數超過核心數時會慘敗——
//         持有者可能被排程器搶佔下 CPU，其他執行緒卻還在空轉等它，
//         整台機器的 CPU 全部浪費在無意義的自旋上。
//     追問：std::mutex 內部怎麼處理？→ 混合策略：先自旋一小段
//           （賭它馬上會釋放），失敗才進 futex 睡眠。兩邊的好處都拿到。
//
// ⚠️ 陷阱. 「自旋鎖不進核心、沒有系統呼叫，所以一定比 mutex 快。」
//     答：不對。無競爭時 std::mutex 也不進核心——
//         glibc 的實作在 fast path 就只是一次原子操作，跟自旋鎖一樣快。
//         系統呼叫只在【真的要睡】的時候才發生。
//         所以自旋鎖省下來的東西，在無競爭時根本不存在；
//         而有競爭時自旋鎖的快取行乒乓與空轉反而可能更糟。
//     為什麼會錯：常見的錯誤模型是「mutex = 每次都要進核心」。
//         實際上 Linux 的 futex 設計就是為了讓「無競爭路徑完全在使用者空間」，
//         這也是 futex 名稱的由來（Fast Userspace muTEX）。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

class SimpleSpinlock {
private:
    std::atomic<bool> locked{false};

public:
    void lock() {
        // exchange 是原子操作：設定新值並返回舊值
        while (locked.exchange(true)) {
            // 如果舊值是 true，表示已被鎖定，繼續自旋
            // 這裡可以加入 yield 或 pause 來減少 CPU 消耗
        }
        // 如果舊值是 false，表示成功獲取鎖
    }

    void unlock() {
        locked.store(false);
    }
};

// -----------------------------------------------------------------------------
// 【改良版對照組】TTAS（Test-and-Test-And-Set）
//   差別只有一行：先用【唯讀】的 load 自旋，看起來有機會了才做一次 exchange。
//   效果：等待期間快取行可以停在 Shared 狀態，不會每輪都去搶所有權，
//         大幅減少多核心之間的快取行乒乓。
// -----------------------------------------------------------------------------
class TTASSpinlock {
private:
    std::atomic<bool> locked{false};

public:
    void lock() {
        for (;;) {
            // 第一層：唯讀自旋（便宜，不搶快取行所有權）
            while (locked.load(std::memory_order_relaxed)) {
                // 真實實作這裡會放 x86 的 PAUSE 指令
            }
            // 第二層：看起來有空了，才付出一次 exchange 的代價
            if (!locked.exchange(true, std::memory_order_acquire)) {
                return;
            }
        }
    }

    void unlock() {
        locked.store(false, std::memory_order_release);
    }
};

// 測試
SimpleSpinlock spinlock;
int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        spinlock.lock();
        ++counter;
        spinlock.unlock();
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1116. Print Zero Even Odd
//   題目：三條執行緒分別呼叫 zero()／even()／odd()，
//         必須協同輸出 "0102030405..."（zero 印 0，另兩條交替印奇偶數）。
//   為什麼用到本主題：這題的核心就是本檔的自旋等待模式——
//         用一個 atomic 狀態變數表示「現在輪到誰」，
//         沒輪到的執行緒原地自旋檢查。
//         它同時示範了自旋等待的正當前提：
//         每一輪的等待都極短（另一條執行緒只印一個數字就換手），
//         短到不值得付出 context switch 的代價。
//   ⚠️ 若每輪工作很長，這種寫法就會變成純燒 CPU，該換 condition_variable。
// -----------------------------------------------------------------------------
class ZeroEvenOdd {
private:
    int n;
    // 0 → 該印 zero；1 → 該印 odd；2 → 該印 even
    std::atomic<int> turn{0};
    std::atomic<int> current{1};   // 下一個要印的數字

public:
    explicit ZeroEvenOdd(int count) : n(count) {}

    void zero(const std::function<void(int)>& printNumber) {
        for (int i = 1; i <= n; ++i) {
            while (turn.load(std::memory_order_acquire) != 0) {
                std::this_thread::yield();
            }
            printNumber(0);
            // 下一個數字是奇數就交給 odd，偶數交給 even
            turn.store((current.load(std::memory_order_relaxed) % 2 == 1) ? 1 : 2,
                       std::memory_order_release);
        }
    }

    void odd(const std::function<void(int)>& printNumber) {
        for (int i = 1; i <= n; i += 2) {
            while (turn.load(std::memory_order_acquire) != 1) {
                std::this_thread::yield();
            }
            printNumber(current.load(std::memory_order_relaxed));
            current.fetch_add(1, std::memory_order_relaxed);
            turn.store(0, std::memory_order_release);
        }
    }

    void even(const std::function<void(int)>& printNumber) {
        for (int i = 2; i <= n; i += 2) {
            while (turn.load(std::memory_order_acquire) != 2) {
                std::this_thread::yield();
            }
            printNumber(current.load(std::memory_order_relaxed));
            current.fetch_add(1, std::memory_order_relaxed);
            turn.store(0, std::memory_order_release);
        }
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】高頻計數器：什麼時候該用鎖、什麼時候根本不該用鎖
//   情境：網路服務要統計「每秒處理了幾個封包」。這是典型的
//         「臨界區段短到極致（就是一個 ++）」的場景。
//   三種寫法的取捨：
//     (a) 自旋鎖保護普通 int  —— 能work，但每次都要一次 exchange + 一次 store
//     (b) std::mutex 保護     —— 無競爭時成本接近 (a)，有競爭時表現更好
//     (c) std::atomic<long> 直接 fetch_add —— 【正解】，一次原子加法就結束，
//         連鎖都不需要，也不可能忘記解鎖。
//   本例實測三者的正確性都一樣（結果必為 800000），
//   重點是讓讀者知道：能用 atomic 解決的，就不要拿鎖出來。
// -----------------------------------------------------------------------------
struct PacketStats {
    SimpleSpinlock   spinLock;
    long             spinGuarded = 0;

    TTASSpinlock     ttasLock;
    long             ttasGuarded = 0;

    std::atomic<long> atomicCounter{0};   // 正解：不需要鎖
};

long runCounterBenchmark(PacketStats& stats, int threads, int perThread,
                         const std::string& mode) {
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&stats, perThread, mode]() {
            for (int i = 0; i < perThread; ++i) {
                if (mode == "spin") {
                    stats.spinLock.lock();
                    ++stats.spinGuarded;
                    stats.spinLock.unlock();
                } else if (mode == "ttas") {
                    stats.ttasLock.lock();
                    ++stats.ttasGuarded;
                    stats.ttasLock.unlock();
                } else {
                    stats.atomicCounter.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& t : workers) t.join();

    if (mode == "spin") return stats.spinGuarded;
    if (mode == "ttas") return stats.ttasGuarded;
    return stats.atomicCounter.load();
}

int main() {
    std::cout << "=== 課程示範: SimpleSpinlock 保護 counter ===" << std::endl;
    {
        std::thread t1(increment);
        std::thread t2(increment);

        t1.join();
        t2.join();

        std::cout << "預期: 200000" << std::endl;
        std::cout << "實際: " << counter << std::endl;
        // 這個結果是【確定】的：所有 ++counter 都在鎖內，不存在 data race。
    }

    std::cout << "\n=== atomic<bool> 是否 lock-free ===" << std::endl;
    {
        std::atomic<bool> probe{false};
        // 若這裡是 false，用 atomic 實作自旋鎖就毫無意義（內部反而在加鎖）
        std::cout << "std::atomic<bool>::is_lock_free() = "
                  << (probe.is_lock_free() ? "true" : "false")
                  << "  (本機 x86-64；此為實作定義)" << std::endl;
    }

    std::cout << "\n=== LeetCode 1116. Print Zero Even Odd (n = 5) ===" << std::endl;
    {
        ZeroEvenOdd zeo(5);
        std::thread te([&zeo]() { zeo.even([](int x) { std::cout << x; }); });
        std::thread to([&zeo]() { zeo.odd ([](int x) { std::cout << x; }); });
        std::thread tz([&zeo]() { zeo.zero([](int x) { std::cout << x; }); });
        tz.join();
        to.join();
        te.join();
        std::cout << std::endl;
    }

    std::cout << "\n=== 日常實務: 高頻計數器的三種寫法 ===" << std::endl;
    {
        const int threads   = 8;
        const int perThread = 100000;
        const long expected = static_cast<long>(threads) * perThread;

        PacketStats stats;
        const long spinResult   = runCounterBenchmark(stats, threads, perThread, "spin");
        const long ttasResult   = runCounterBenchmark(stats, threads, perThread, "ttas");
        const long atomicResult = runCounterBenchmark(stats, threads, perThread, "atomic");

        std::cout << "預期值: " << expected << std::endl;
        std::cout << "SimpleSpinlock 保護 : " << spinResult
                  << (spinResult == expected ? "  ✓" : "  ✗") << std::endl;
        std::cout << "TTASSpinlock  保護 : " << ttasResult
                  << (ttasResult == expected ? "  ✓" : "  ✗") << std::endl;
        std::cout << "atomic fetch_add   : " << atomicResult
                  << (atomicResult == expected ? "  ✓" : "  ✗") << std::endl;
        std::cout << "結論: 三者都正確，但第三種連鎖都不需要——"
                     "能用 atomic 就不要用鎖" << std::endl;
        // 註：三種寫法的【耗時】差異取決於機器與當下負載，每次執行都不同，
        //     故本檔只驗證正確性，不列出時間數字。
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.2：互斥鎖的工作原理1.cpp' -o spinlock_concept

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. is_lock_free() 的結果是【實作定義】的。本機（x86-64 / g++ 15.2 /
//      libstdc++）為 true；在某些平台上 std::atomic<bool> 可能不是 lock-free。
//   2. 三種計數寫法的【執行時間】每次都不同，故刻意不輸出耗時，只驗證正確性。
//   3. 所有數值結果都是確定的：每一次 ++ 都在鎖內或由 atomic 完成，
//      不存在 data race，因此不會出現「少加」的情況。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: SimpleSpinlock 保護 counter ===
// 預期: 200000
// 實際: 200000
//
// === atomic<bool> 是否 lock-free ===
// std::atomic<bool>::is_lock_free() = true  (本機 x86-64；此為實作定義)
//
// === LeetCode 1116. Print Zero Even Odd (n = 5) ===
// 0102030405
//
// === 日常實務: 高頻計數器的三種寫法 ===
// 預期值: 800000
// SimpleSpinlock 保護 : 800000  ✓
// TTASSpinlock  保護 : 800000  ✓
// atomic fetch_add   : 800000  ✓
// 結論: 三者都正確，但第三種連鎖都不需要——能用 atomic 就不要用鎖
