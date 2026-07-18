// =============================================================
// 10_shared_mutex.cpp  --  以 std::shared_mutex 實作讀寫鎖
// =============================================================
//
// 本課目標:
//   1. 認得「許多讀者、偶爾才有寫者」的模式 (設定快取、
//      路由表、DNS 快取、每個請求都會查的字典......)。
//   2. 使用 std::shared_mutex (C++17),讓 *許多* 讀者能平行
//      執行,同時仍保證寫者擁有獨佔存取權。
//   3. 搭配對應的 RAII 包裝器:
//        std::shared_lock<std::shared_mutex>  讀者用
//        std::unique_lock<std::shared_mutex>  寫者用
//   4. 用 benchmark 證明加速確實存在,並學會 shared_mutex
//      在什麼情況下其實 *輸給* 普通 mutex。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 10_shared_mutex.cpp \
//         -o 10_shared_mutex
//
// 執行方式:
//     ./10_shared_mutex
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Reader/writer 鎖 ── std::shared_mutex
// 前置課程: lesson 03
// 觀念詞彙:
//   - shared_mutex   ── 多讀者可同時持有,寫者獨佔
//   - shared_lock    ── 讀者用的 RAII
//   - unique_lock    ── 寫者用的 RAII
//   - writer starvation ── 讀者不停進場,寫者永遠等不到
// 新介紹 API:
//   std::shared_mutex              C++17 的 reader/writer lock
//   std::shared_lock<Mutex>        讀者:可同時多人持有
//   std::unique_lock<Mutex>        寫者:獨佔
//   m.lock_shared() / unlock_shared()    讀者手動 (一般避免)
//   std::shared_timed_mutex        C++14,多 timed 方法
// 何時使用:
//   - 讀 *遠* 多於寫,且每次讀在持鎖期間做的事夠多
//   - 設定快取、路由表、字典查詢
// 何時不要用:
//   - 讀只是載一個 int → std::atomic 更快
//   - 寫頻繁 → 普通 mutex 反而較好
//   - 「讀大但持鎖時間極短」→ 收益不大
// 常見錯誤:
//   - 沒把 mutex 宣告為 mutable → const 成員無法 lock
//   - 想在 shared_lock 中升級為 unique_lock → 標準不支援
//   - 假設讀者完全免費 → reader 計數仍是 atomic 操作
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── shared_mutex 的工程細節
// =============================================================
//
// 1. 內部資料結構 (libstdc++ 為例)
//    一個 shared_mutex 至少包含:
//      - 一把內部 mutex (保護下面兩個欄位的修改)
//      - reader_count (atomic int)
//      - writer_waiting / writer_active (旗標)
//    lock_shared 流程:
//      ① 檢查沒 writer 在等/在持有 → atomic ++reader_count → 返回
//      ② 否則 cv.wait 直到 writer 釋放
//    lock 流程:
//      ① 設 writer_waiting=true,等 reader_count==0 且無其他 writer
//      ② 拿到 → 設 writer_active=true,清 writer_waiting
//
//    意思是:即使「只有讀者」的情況,reader 之間仍會去碰 reader_count
//    這個 atomic 變數 ── 它本身就是 cache-line 級的爭用源。8 個 reader
//    搶這條 line 反而比 8 條都搶 plain mutex 還慢過 (lesson 17/19 用
//    sharding / RCU 解這個)。
//
// 2. 公平性與 starvation 風險
//    主流實作 (libstdc++ glibc) 給 writer 優先:有 writer 在等時,後到
//    的 reader 不能進入,避免 writer 永遠被連續來的 reader 卡死。
//    但 *舊版 reader 仍能繼續*── 一旦讀很久 (例如持 lock 做 I/O),
//    writer 就被 hold 住。
//    Boost.Thread / pthread_rwlock 提供更多公平性選項;std 沒有。
//
// 3. 不能升級 (upgrade) 是有意設計
//    很多人想:「我先讀,確認需要寫再升級」── shared → unique。
//    標準不允許,因為兩條 thread 同時想升級就 deadlock (兩邊都持 reader
//    身份,等對方釋放才能變 writer)。
//    替代:先放 shared,再拿 unique;期間別人可能改了狀態,所以拿 unique
//    後要 *重檢一次條件*。或乾脆從頭直接 unique。
//
// 4. shared_mutex 不一定比 mutex 快
//    經驗法則:
//      - 純讀比例 ≥ 90% 且持鎖時間 ≥ 1 µs → shared_mutex 通常贏。
//      - 寫多 (≥ 20%) 或臨界區極短 (< 100 ns) → mutex 反而贏 (shared
//        本身的內部 atomic 開銷 > 互斥 mutex 的成本)。
//      - 高 reader 數 (≥ 16) → 考慮 lesson 18/19 RCU/HP 取代 shared_mutex。
//
// 5. shared_lock 的 RAII
//    std::shared_lock<std::shared_mutex> sl(m);
//      建構時 lock_shared,解構時 unlock_shared。
//    std::unique_lock 已存在,別與 shared_lock 混淆 ── 兩者不能轉換。
//
// 6. 與其他語言對照
//    Java     ReentrantReadWriteLock (有 fair / unfair 版,可升級降級)
//    .NET     ReaderWriterLockSlim
//    Go       sync.RWMutex
//    Rust     std::sync::RwLock (poisoning 機制特別)
//    各家公平性策略不同。Java fair 版會降低吞吐;Go 沒有 fair 設定。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應 RW lock 題* ── 真實系統很常用
//     (例如 routing table、config snapshot),但 LC 沒考。
//   → 若硬要找對應:把 lesson 30 Q5 (Bounded Blocking Queue) 的
//     interface 加一個 peek_back() / size() 純讀方法,兩個 reader 可
//     並行 ── 用 shared_mutex 比 mutex 快。
//   → 進階替代:lesson 18 (CoW snapshot) 與 lesson 19 (Hazard pointers)
//     是 reader 比 writer 多很多時更快的解法。
//
// 主要 API 對照 (cppreference):
//   - std::shared_mutex (C++17)         https://en.cppreference.com/w/cpp/thread/shared_mutex
//   - std::shared_timed_mutex (C++14)   https://en.cppreference.com/w/cpp/thread/shared_timed_mutex
//   - std::shared_lock                  https://en.cppreference.com/w/cpp/thread/shared_lock
//   - shared_lock 與 unique_lock 共存的 lock_shared / unlock_shared
//                                       https://en.cppreference.com/w/cpp/thread/shared_mutex/lock_shared
//
// 練習建議:
//   - 把 lesson 30 Q5 Bounded Blocking Queue 的 size() 改用 shared_mutex
//     的 shared_lock 來保護 reader (寫端 push/pop 仍要 unique_lock)。
//   - 進階:讀寫比例 ≥ 100:1 時 shared_mutex 仍嫌慢 → 看 lesson 18 用
//     atomic<shared_ptr> 做 CoW snapshot。
// =============================================================

/*
補充筆記：std::shared_mutex
  - shared_mutex 允許多讀單寫，適合讀多寫少的共享資料。
  - shared_lock 用於讀取，unique_lock 用於寫入；寫入仍必須獨占。
  - 讀寫鎖不一定比 mutex 快，寫入頻繁或臨界區很短時成本可能更高。
  - std::shared_mutex 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
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
#include <shared_mutex>      // std::shared_mutex, std::shared_lock
#include <vector>
#include <unordered_map>
#include <string>
#include <atomic>
#include <chrono>

// -------------------------------------------------------------
// 鎖的對照表
//
//   你持有的         其他讀者          其他寫者
//   shared_lock      可以拿到            被擋住
//   unique_lock      被擋住              被擋住
//
// "Shared" 是讀者用的字,"unique" 是寫者用的字。可以這樣
// 在腦中翻譯:「許多人能 *共享* 這把鎖當讀者;只有一人
// 能 *獨佔* 這把鎖當寫者」。
//
// 寫者用 std::lock_guard<std::shared_mutex> 也可以 (它一定
// 是獨佔上鎖),但慣例是用 unique_lock,讓「我在寫」這個
// 意圖在呼叫端就一目了然。
// -------------------------------------------------------------


// -------------------------------------------------------------
// 一個迷你的設定快取 (config cache),兩種實作。
// 兩者對外的 API 完全相同:
//     std::string get(const std::string& key) const;
//     void        set(const std::string& key, std::string val);
//
// 一個用 std::mutex (所有操作都序列化)。
// 另一個用 std::shared_mutex (讀者可平行)。
// -------------------------------------------------------------

class ConfigPlain {
public:
    std::string get(const std::string& key) const
    {
        std::lock_guard<std::mutex> lk(mtx_);          // 獨佔
        auto it = data_.find(key);
        return it == data_.end() ? std::string{} : it->second;
    }

    void set(const std::string& key, std::string val)
    {
        std::lock_guard<std::mutex> lk(mtx_);          // 獨佔
        data_[key] = std::move(val);
    }

private:
    mutable std::mutex                          mtx_;
    std::unordered_map<std::string, std::string> data_;
};

class ConfigShared {
public:
    std::string get(const std::string& key) const
    {
        // shared_lock = 「我是讀者」。
        // 多個執行緒可以同時持有 `mtx_` 的 shared_lock,
        // 只要沒有任何執行緒以 unique_lock 持有它即可。
        std::shared_lock<std::shared_mutex> lk(mtx_);
        auto it = data_.find(key);
        return it == data_.end() ? std::string{} : it->second;
    }

    void set(const std::string& key, std::string val)
    {
        // unique_lock = 「我是寫者,沒有人可以在裡面」。
        // 拿這把鎖會阻塞,直到所有現存讀者都釋放它們的
        // shared_lock,且沒有其他寫者持有它。
        std::unique_lock<std::shared_mutex> lk(mtx_);
        data_[key] = std::move(val);
    }

private:
    // shared_mutex 本身在 const 成員函式裡若要被
    // shared_lock / unique_lock 操作,必須宣告為 mutable
    // (因為它們呼叫的是 mutex 的非 const 方法) —— 這跟
    // 普通 std::mutex 在 const 方法裡的處理方式完全相同。
    mutable std::shared_mutex                   mtx_;
    std::unordered_map<std::string, std::string> data_;
};


// -------------------------------------------------------------
// 碼錶。
// -------------------------------------------------------------
struct Stopwatch {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    long long ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0).count();
    }
};


// -------------------------------------------------------------
// Benchmark。
//
// 工作負載形狀:
//   - 8 條讀者執行緒,每條做 READS_PER_THREAD 次查詢。
//   - 1 條寫者執行緒,偶爾做一次更新。
//
// 為了讓「讀者端的平行」能被觀察到,每次查詢也在持鎖
// 的狀態下做了一點 CPU 工作 —— 例如把符合某模式的所有
// value 長度加起來。沒有這點工作的話,鎖獲取的開銷會
// 主導每次呼叫的成本,差距就看不出來了。
// -------------------------------------------------------------
template <typename Cache>
long long run_benchmark(const char* label,
                        Cache& cache,
                        int reader_threads,
                        int reads_per_thread,
                        int writer_count)
{
    // 預先填充快取,讓讀有東西可讀。
    for (int i = 0; i < 100; ++i) {
        cache.set("key_" + std::to_string(i),
                  "value_for_key_" + std::to_string(i));
    }

    std::atomic<long long> total_len{0};
    std::atomic<bool>      stop_writer{false};

    Stopwatch sw;

    // ---- 啟動寫者 ----
    std::thread writer([&]{
        int n = 0;
        while (!stop_writer.load()) {
            cache.set("key_" + std::to_string(n % 100),
                      "updated_" + std::to_string(n));
            ++n;
            // 稍微 sleep 一下,讓寫相對於讀變得很 *稀少*
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
        (void)writer_count;
    });

    // ---- 啟動讀者 ----
    std::vector<std::thread> readers;
    readers.reserve(reader_threads);
    for (int t = 0; t < reader_threads; ++t) {
        readers.emplace_back([&, t]{
            long long local = 0;
            for (int i = 0; i < reads_per_thread; ++i) {
                int idx = (t * 7 + i) % 100;          // 偽隨機
                std::string v = cache.get("key_" + std::to_string(idx));

                // 一點 CPU 工作,模擬「拿到 value 後接著做點
                // 有意義的事」。這裡是在鎖外做的,但上面
                // get() 內部本來就在持鎖時做了類似的事。
                local += static_cast<long long>(v.size());
            }
            total_len.fetch_add(local, std::memory_order_relaxed);
        });
    }

    for (auto& r : readers) r.join();
    stop_writer.store(true);
    writer.join();

    auto ms = sw.ms();
    std::cout << "[" << label << "] " << ms << " ms"
              << "   total_len=" << total_len.load() << '\n';
    return ms;
}


int main()
{
    constexpr int READER_THREADS    = 8;
    constexpr int READS_PER_THREAD  = 200'000;

    std::cout << "Workload: " << READER_THREADS << " readers x "
              << READS_PER_THREAD << " reads, plus an occasional writer.\n\n";

    ConfigPlain  cp;
    ConfigShared cs;

    // 每組跑 3 次,平滑掉排程器造成的雜訊。
    for (int run = 1; run <= 3; ++run) {
        std::cout << "-- run " << run << " --\n";
        run_benchmark("std::mutex       ", cp,
                      READER_THREADS, READS_PER_THREAD, 1);
        run_benchmark("std::shared_mutex", cs,
                      READER_THREADS, READS_PER_THREAD, 1);
    }

    // ---------------------------------------------------------
    // 實戰範例: feature flag store (讀爆量, 寫稀少)
    // ---------------------------------------------------------
    // 應用場景: 服務的「功能開關」(feature flag), 例如
    //   "new_payment_flow_enabled" = true
    // 每個 request 進來都會查一次, 但 ops 只在每幾分鐘才改一次。
    // 這正是 shared_mutex 最甜的場景: 100% 讀路徑都可並行。
    //
    // 注意: 因為 *讀超快*, 也可以考慮 atomic<shared_ptr<map>>
    // 的 CoW 設計 (lesson 18), 完全不要鎖; 但 shared_mutex 是
    // 最直觀且零依賴的版本。
    // ---------------------------------------------------------
    {
        std::cout << "\n[demo] feature flag store\n";
        ConfigShared flags;
        flags.set("new_ui",   "true");
        flags.set("dark_mode","false");
        flags.set("v2_api",   "true");

        std::atomic<int> reads{0};
        std::atomic<bool> stop{false};

        // 6 條讀者, 每條狂查 flag
        std::vector<std::thread> rs;
        for (int t = 0; t < 6; ++t) {
            rs.emplace_back([&]{
                while (!stop.load()) {
                    flags.get("new_ui");
                    flags.get("dark_mode");
                    flags.get("v2_api");
                    reads.fetch_add(3, std::memory_order_relaxed);
                }
            });
        }
        // 1 條 ops thread, 偶爾切換一個 flag
        std::thread w([&]{
            for (int i = 0; i < 5; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                flags.set("dark_mode", (i & 1) ? "true" : "false");
            }
        });
        w.join();
        stop.store(true);
        for (auto& t : rs) t.join();
        std::cout << "  100 ms 內完成讀次數 ≈ " << reads.load() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：shared_mutex 永遠比 mutex 快嗎?
    //    A：不是。shared_mutex 內部要維護 reader 計數, 加解鎖比普通
    //       mutex 重 (約 2-3×)。只有當「並行 reader 多 + 臨界區夠長」
    //       才賺得回來; 短臨界區 (例如讀一個 int) 反而不如直接用
    //       atomic 或 mutex。本課 benchmark ~1.8× 加速是「中等臨界區
    //       + 高 reader」典型結果。
    //
    //  Q2：reader-preferring 跟 writer-preferring 是什麼?
    //    A：reader-preferring: 只要還有 reader 就讓新 reader 進場, 寫者
    //       可能餓死。writer-preferring: 一旦寫者來等, 後到的 reader
    //       要排在他後面, 讀寫公平但讀吞吐降低。POSIX 不規定, 各家實
    //       作不同 (glibc 是 writer-preferring 為主)。如果寫者罕見就
    //       不太重要, 寫者頻繁時要明確選擇實作。
    //
    //  Q3：shared_lock 跟 unique_lock<shared_mutex> 該配哪個?
    //    A：reader 路徑用 std::shared_lock<std::shared_mutex>, 它呼叫
    //       lock_shared/unlock_shared, 多 reader 並行。writer 路徑用
    //       std::unique_lock<std::shared_mutex> 或 std::lock_guard
    //       <std::shared_mutex>, 走 lock/unlock 獨佔。誤用 shared_lock
    //       去寫資料就還原成普通 race。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 什麼時候該用 shared_mutex。
//    讀 *遠遠* 多於寫,而且每次讀在持鎖期間做的工作多到
//    「平行讀」確實能省時間。
//
//    經典的吻合場景:
//      - 啟動時載入一次、之後每個請求都會讀的設定快取。
//      - 路由表、DNS 快取、feature flag 查詢。
//      - 偶爾才會重建一次的「以讀為主」的索引。
//
//    *不適合* 的場景:
//      - 「讀」其實只是載一個 int 或一根指標。shared_mutex
//        的簿記成本比讀本身還貴。改用 std::atomic。
//      - 寫操作很頻繁的工作負載。shared_mutex 在每個操作
//        上 *比* std::mutex 還慢;你只能靠「讀的並行性」
//        把它賺回來。
//
// 2. RAII 包裝器要對得起來。
//      讀者:std::shared_lock<std::shared_mutex>
//      寫者:std::unique_lock<std::shared_mutex>
//            (用 std::lock_guard 也行,但 unique_lock 讓
//             「我是寫者」這個意圖在閱讀程式碼時更清楚)
//    永遠不要自己呼叫 .lock_shared() / .unlock_shared()。
//    Lesson 03 說過的例外安全規則照樣適用:裸的
//    lock/unlock 在丟例外時會洩漏鎖。
//
// 3. const 正確性。
//    一個 `std::string get(...) const` 成員若要使用
//    shared_mutex,必須把 mutex 宣告為 `mutable`。
//    這跟 const 方法上使用 std::mutex 的模式完全一樣。
//
// 4. 升級鎖。C++17 的 shared_mutex *不* 支援把 shared_lock
//    原子升級為 unique_lock。標準慣用做法是:
//
//        // 1. 用 shared_lock 讀
//        // 2. 釋放它
//        // 3. 拿 unique_lock
//        // 4. *重新檢查* 條件 (在沒持鎖的空檔,狀態
//        //    可能已經改變)
//        // 5. 修改
//
//    一直有人在要求「可升級鎖」,但 C++ 標準一直沒加,
//    因為自然的使用情境往往隱含微妙的競爭。
//
// 5. 寫者餓死 (writer starvation)。如果讀者源源不絕地進來
//    而實作總是讓他們進場,寫者可能永遠都等不到。實際的
//    實作會用各種策略 (FIFO、寫者優先、公平排程),但
//    標準對此 *沒有規定*。如果寫的延遲很重要,請在實際
//    負載下量測。
//
// 6. 它仍然只是 *一把* 鎖。shared_mutex 並不是什麼魔法
//    並行。寫者仍然會擋下所有人。一個以讀為主、但偶爾
//    要重建一次的快取,在那次冗長的寫期間會把所有讀者
//    全部卡住。對延遲關鍵的路徑,改看 copy-on-write 或
//    read-copy-update (RCU) —— 後續的話題。
//
// 7. shared_timed_mutex (C++14) vs shared_mutex (C++17)。
//    概念相同,但 C++14 版本還提供 try_lock_for /
//    try_lock_until。優先用 shared_mutex (較小、較快),
//    只有真的需要 timed 方法時才退回到 shared_timed_mutex。
// =============================================================
