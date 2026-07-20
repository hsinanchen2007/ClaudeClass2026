// =============================================================
// 18_cow_snapshot.cpp  --  Copy-on-Write 原子快照,以及它的真實代價
// =============================================================
//
// 本課目標:
//   1. 認識 CoW 快照模式:把不可變資料藏在一個原子指標後面,
//      讀者完全不需要鎖;寫者複製、修改、原子換指標。
//   2. 看到 *理論* 與 *libstdc++ 實作* 之間有個現實的差距:
//      std::atomic<std::shared_ptr<T>> 在 libstdc++ 裡 *不是*
//      真的 lock-free —— 它內部用一把隱藏鎖把 shared_ptr 的
//      atomic 操作串成序列。所以 naive CoW 在重讀工作負載下
//      可能反而 *輸給* lesson 10 的 std::shared_mutex。
//   3. 學到「測量比直覺重要」:concept 對 != 實作快。
//   4. 知道下一步是什麼:要避開 atomic<shared_ptr> 的稅,
//      就要用更底層的技術 —— hazard pointers (lesson 19) 或
//      RCU。
//
// 編譯方式:
//     g++ -std=c++20 -O2 -pthread 18_cow_snapshot.cpp -o 18_cow_snapshot
//
// 執行方式:
//     ./18_cow_snapshot
// =============================================================
//
// CoW 模式 (概念上):
//
//   class CoW {
//       std::atomic<std::shared_ptr<const T>> snap_;
//       std::mutex                            write_mtx_;   // 寫者序列化
//   public:
//       T_view read() const { return snap_.load(); }     // 完全無鎖
//       void write(F mutator) {
//           std::lock_guard lk(write_mtx_);
//           auto next = std::make_shared<T>(*snap_.load());
//           mutator(*next);
//           snap_.store(std::move(next));                 // 原子發布
//       }
//   };
//
// 美妙之處 (在合適的硬體 + 函式庫上):
//   - 讀完全無同步,可以無限平行。
//   - 寫者只擋其他寫者,不擋讀者。
//   - 舊版本由仍持有 shared_ptr 的讀者「順便」回收。
//
// 函式庫稅 (libstdc++ / libc++):
//   - std::atomic<std::shared_ptr<T>>::is_lock_free() 通常回傳
//     false。內部實作是「全域或 hash 化的 spin lock」。
//   - 結果:每次 reader 的 .load() 其實都搶一把隱藏鎖,
//     讀者並不像理論上那樣完全平行。
//   - 在重讀情境下,這把隱藏鎖的爭用可能比 shared_mutex 還
//     差。
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     Copy-on-Write 原子快照,以及 libstdc++ 的函式庫稅
// 前置課程: lesson 04, 10
// 觀念詞彙:
//   - copy-on-write (CoW) ── 寫者複製整份資料、原子換指標
//   - immutable snapshot  ── 讀者拿到的指標指向不變的資料
//   - RCU                 ── read-copy-update,CoW 的核心精神
//   - grace period        ── 等所有讀者完成後再回收舊版本
// 新介紹 API:
//   std::shared_ptr<const T>             指向不可變資料的 shared_ptr
//   std::atomic<std::shared_ptr<T>>      C++20,原子持有 shared_ptr
//   .load() / .store(p) / .exchange(p)
//   std::make_shared<MapT>(*old)         複製整個容器
// 何時使用:
//   - 寫操作 *極* 罕見 (例如系統初始化 + 偶爾 reload)
//   - 想對外公開不可變的快照指標
// 何時不要用:
//   - libstdc++ 上想榨極限效能 → atomic<shared_ptr> 不是 lock-free
//   - 寫頻繁、資料量大 → 每次複製太貴
//   - 要追求理論最佳 CoW → 用 folly::HazPtr 或 liburcu
// 常見錯誤:
//   - 假設 atomic<shared_ptr> 是 lock-free → libstdc++ 不是
//   - 兩個 writer 沒有外層 mutex → lost update
//   - 老版本永遠不釋放 (有讀者一直握著) → 記憶體膨脹
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── CoW、RCU、shared_mutex 的取捨
// =============================================================
//
// 1. CoW (Copy-on-Write) 的核心
//    狀態用一個 immutable snapshot 持有 (例如 shared_ptr<const Map>)。
//      - reader:atomic_load(snapshot) 拿到當前版本,直接讀,*不需要鎖*。
//      - writer:複製整個 snapshot → 修改新版 → atomic_store 換上去。
//        (多個 writer 之間用 mutex 協調。)
//    優點:reader 路徑零鎖、零 cache invalidate (純讀者本地 cache);
//    缺點:每次寫都複製整個資料,大 map 寫成本爆炸。
//    適用場景:read ≫ write (1000:1 以上),且資料整體不大。
//
// 2. atomic<shared_ptr<T>> 在 libstdc++ 是「假 lock-free」
//    spec 上 atomic<shared_ptr> 提供原子 load/store。但 libstdc++ 內部
//    實作用 *splitlock 陣列* (一組固定全域 mutex,以指標 hash 分桶),
//    每次 atomic_load 都進去搶一把鎖 + 改 shared_ptr 的 ref-count。
//    導致純讀情境下,*reader 之間* 反而互相爭用 → 比 shared_mutex 更慢。
//    觀察到的數字 (lesson 18 demo):純讀 ~0.36×,反輸 shared_mutex。
//    MSVC 與 libc++ 也都用 lock 實作;這是 spec 沒法保證 lock-free 的原因。
//
// 3. 怎麼修 CoW 的效能
//    A. RCU (Read-Copy-Update,Linux kernel 用,userspace 看 liburcu):
//       reader 完全 lock-free 也不碰 ref-count,只用 grace period 確保
//       「沒人還在讀舊版」之後再 free。Linux kernel 大量使用。
//    B. Hazard Pointers (lesson 19):每個 reader 在 thread_local hazard
//       slot 公布「我正在讀這個指標」,writer 釋放前掃所有 hazard,
//       還在被讀就延後。
//    C. epoch-based reclamation (Crossbeam / folly Hazptr):類似 RCU 但
//       更輕量,適合 userspace。
//    全部目的:把 reader 路徑從「touch 共享 ref-count」變成「touch 自己
//    thread-local」。
//
// 4. CoW 適用 vs 不適用清單
//      適用:設定檔、路由表、白名單、UI 樹、JIT 機器碼快照、查詢 plan cache
//      不適用:逐筆插入的 hot map、queue、計數器、需要部分更新的大物件
//
// 5. lost update 的 writer 競爭
//    兩個 writer 都 *讀* 當前版本 → 各自修改 → 各自 store 新版本。
//    後寫的覆蓋了前寫的。CoW 不自動避免這個 ── writers 之間要外加一把
//    mutex,或用 CAS (compare_exchange) 重試:
//        do {
//            old = snapshot.load();
//            new = make_modified(old);
//        } while (!snapshot.compare_exchange_weak(old, new));
//
// 6. 記憶體膨脹風險
//    若有讀者一直握著舊版本不放,且寫入頻繁 → 多個版本同時存活。每版
//    都是完整 deep copy → 記憶體爆。
//    對策:strict reader timeout、或限制最多保留 K 個版本 (新版本到時
//    若舊版本仍活著,寫者轉成 spin-wait reader 釋放)。
//
// 7. 在 CPU level 為什麼 reader 這麼便宜 (理論上)
//    若 reader 真的 lock-free 拿 snapshot:它讀的是 *自己 cache 中的
//    snapshot 指標*。多個 reader 各持一份 → S 狀態,沒有 invalidate。
//    Writer 換指標時:atomic store → 把那條 line 變 M,broadcast invalidate
//    給所有 reader。下次 reader 再讀時 cache miss,重新拉一次 ── 但拉
//    一次比每次 reader 都 lock 便宜千倍。lesson 18 demo 的問題正是因為
//    libstdc++ 沒實現這個理想路徑。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── CoW 是工業情境,LC 沒考。
//   → 但 lesson 30 Q8 Web Crawler 的 visited set 若改成 CoW snapshot
//     (寫者複製整份,reader lock-free 看舊版),可讓 reader 完全免鎖。
//     不過本課 demo 顯示:libstdc++ 的 atomic<shared_ptr> *不是真的*
//     lock-free,所以實戰要 CoW 通常選 lesson 19 (Hazard Pointers) 或
//     userspace RCU。
//
// 主要 API 對照 (cppreference):
//   - std::shared_ptr<T>                https://en.cppreference.com/w/cpp/memory/shared_ptr
//   - std::atomic<std::shared_ptr<T>>   https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic2
//   - std::shared_mutex (對照)          https://en.cppreference.com/w/cpp/thread/shared_mutex
//
// 練習建議:
//   - 拿 lesson 10 shared_mutex 與本課 CoW 跑同個 reader 重 + writer 輕
//     的工作負載,觀察「concept 對 ≠ 實作快」。
//   - 進階:lesson 19 Hazard Pointers 是真實能跑得快的 CoW reclamation。
// =============================================================

/*
補充筆記：cow_snapshot
  - cow_snapshot 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - cow_snapshot 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Copy-on-Write 快照與 shared_ptr 的執行緒安全
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. CoW 快照如何做到讀端完全無鎖？代價是什麼？
//     答：資料以不可變物件持有，讀者透過一次原子載入取得「當下快照」的 shared_ptr，
//         之後完全不需要任何鎖，因為那份資料永不被修改。寫者則是「複製一份 → 修改
//         副本 → 原子換掉指標」。舊快照由引用計數自動回收，最後一個讀者離開時才釋放。
//         代價：每次寫都要整份複製，寫入昂貴、記憶體峰值為兩份，只適合讀遠多於寫。
//     追問：與 RCU 差在哪？（RCU 用 grace period 而非引用計數回收，讀端更便宜，但
//           回收較延遲）
//
// 🔥 Q2. std::shared_ptr 是 thread-safe 的嗎？
//     答：要分三層。控制區塊的引用計數是 thread-safe 的（原子增減），所以「多個 thread
//         各自持有指向同一物件的 shared_ptr 副本並各自複製／解構」是安全的。但
//         shared_ptr 物件「本身」不是：多個 thread 對同一個 shared_ptr 實體並行讀寫
//         （一個 reset、一個 read）仍是 data race。而被指向的物件完全不受保護。C++20
//         提供 std::atomic<std::shared_ptr<T>>，取代已棄用的 atomic_load/store 自由函式。
//     追問：引用計數為何遞增可以 relaxed，遞減卻需要 acq_rel？（遞增只需原子性；
//           遞減到 0 的那個 thread 必須看見其他 thread 對物件的所有寫入，才能安全解構）
//
// ⚠️ 陷阱. 用了 std::atomic<std::shared_ptr<T>> 就是 lock-free 的無鎖讀了嗎？
//     答：不一定。它是否 lock-free 取決於實作——實務上常見的做法是在內部用鎖把
//         shared_ptr 的原子操作串成序列，因此高並行讀取下可能反而輸給 shared_mutex。
//         應該用 is_lock_free() 查詢，並且實測比較，不要憑「概念上無鎖」下結論。
//     為什麼會錯：把「介面是 atomic」等同於「實作是 lock-free」。標準只保證操作的
//         原子性，是否用鎖實作是 implementation-defined；std::atomic_flag 才是標準
//         保證永遠 lock-free 的唯一型別。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <chrono>

// -------------------------------------------------------------
// 版本 1:CoW 設定快取
// -------------------------------------------------------------
class ConfigCoW {
public:
    using MapT = std::unordered_map<std::string, std::string>;

    ConfigCoW()
    {
        snapshot_.store(std::make_shared<const MapT>());
    }

    // 看似完全無鎖。實際上 .load() 在 libstdc++ 會走進
    // __atomic_shared_ptr 的內部 mutex (見上面討論)。
    std::string get(const std::string& key) const
    {
        std::shared_ptr<const MapT> snap = snapshot_.load();
        auto it = snap->find(key);
        return it == snap->end() ? std::string{} : it->second;
    }

    void set(const std::string& key, std::string val)
    {
        std::lock_guard<std::mutex> lk(write_mtx_);
        auto cur  = snapshot_.load();
        auto next = std::make_shared<MapT>(*cur);
        (*next)[key] = std::move(val);
        snapshot_.store(std::move(next));
    }

private:
    std::atomic<std::shared_ptr<const MapT>> snapshot_;
    std::mutex                               write_mtx_;
};


// -------------------------------------------------------------
// 版本 2:lesson 10 的 shared_mutex 版本,作為基準
// -------------------------------------------------------------
class ConfigShared {
public:
    using MapT = std::unordered_map<std::string, std::string>;

    std::string get(const std::string& key) const
    {
        std::shared_lock<std::shared_mutex> lk(mtx_);
        auto it = data_.find(key);
        return it == data_.end() ? std::string{} : it->second;
    }
    void set(const std::string& key, std::string val)
    {
        std::unique_lock<std::shared_mutex> lk(mtx_);
        data_[key] = std::move(val);
    }
private:
    mutable std::shared_mutex mtx_;
    MapT                      data_;
};


// =============================================================
// Benchmark 框架
// =============================================================
template <typename Cache>
static long long bench(const char* label, Cache& c,
                       int reader_threads, int reads_per_thread,
                       int writer_period_us)
{
    for (int i = 0; i < 100; ++i)
        c.set("key_" + std::to_string(i), "value_" + std::to_string(i));

    std::atomic<bool> stop_writer{false};
    std::atomic<long long> sum_lengths{0};

    auto t0 = std::chrono::steady_clock::now();

    std::thread writer;
    if (writer_period_us > 0) {
        writer = std::thread([&, writer_period_us]{
            int n = 0;
            while (!stop_writer.load(std::memory_order_relaxed)) {
                c.set("key_" + std::to_string(n % 100),
                      "updated_" + std::to_string(n));
                ++n;
                std::this_thread::sleep_for(
                    std::chrono::microseconds(writer_period_us));
            }
        });
    }

    std::vector<std::thread> readers;
    for (int t = 0; t < reader_threads; ++t) {
        readers.emplace_back([&, t]{
            long long local = 0;
            for (int i = 0; i < reads_per_thread; ++i) {
                int idx = (t * 7 + i) % 100;
                auto v = c.get("key_" + std::to_string(idx));
                local += static_cast<long long>(v.size());
            }
            sum_lengths.fetch_add(local, std::memory_order_relaxed);
        });
    }

    for (auto& r : readers) r.join();
    if (writer.joinable()) {
        stop_writer.store(true, std::memory_order_relaxed);
        writer.join();
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();
    std::cout << "[" << label << "] " << ms << " ms"
              << "   sum_lengths=" << sum_lengths.load() << '\n';
    return ms;
}

int main()
{
    std::cout << "is_lock_free? std::atomic<std::shared_ptr<int>>: "
              << std::atomic<std::shared_ptr<int>>{}.is_lock_free() << '\n';
    std::cout << "(0 代表 libstdc++ 用了內部隱藏鎖)\n\n";

    constexpr int READERS         = 8;
    constexpr int READS_PER_THREAD = 300'000;

    // ---- PHASE A: 純讀,無 writer ----
    std::cout << "PHASE A: pure reads, no writer\n";
    {
        ConfigShared cs;
        ConfigCoW    cc;
        auto a = bench("shared_mutex", cs, READERS, READS_PER_THREAD, 0);
        auto b = bench("CoW snapshot", cc, READERS, READS_PER_THREAD, 0);
        std::cout << "  ratio  = "
                  << (static_cast<double>(a) / static_cast<double>(b))
                  << "x  (CoW 比 shared_mutex 快多少;>1 才是 CoW 贏)\n\n";
    }

    // ---- PHASE B: 讀很多 + writer 很罕見 (每 10ms,即 100 Hz) ----
    std::cout << "PHASE B: many reads + a slow writer (every 10ms)\n";
    {
        ConfigShared cs;
        ConfigCoW    cc;
        auto a = bench("shared_mutex", cs, READERS, READS_PER_THREAD, 10'000);
        auto b = bench("CoW snapshot", cc, READERS, READS_PER_THREAD, 10'000);
        std::cout << "  ratio  = "
                  << (static_cast<double>(a) / static_cast<double>(b))
                  << "x\n\n";
    }

    // ---------------------------------------------------------
    // 實戰範例: routing table hot-reload
    // ---------------------------------------------------------
    // 應用場景: HTTP 反向代理服務, 啟動時載入 routing 規則, ops
    // 改設定後想「熱重載」, 而請求路徑不能被卡住。CoW 是天生
    // 的解法:
    //   - 1000 個 reader thread 同時查 routing, 各自拿一個 shared_
    //     ptr<RoutingTable> snapshot, 完全免鎖 (libstdc++ 上仍有
    //     隱藏鎖, 但概念上正確)。
    //   - 重載: 載完整新表, 一次 atomic store, 舊版本由 shared_ptr
    //     refcount 自動回收 (還在用的 reader 不會看到斷尾)。
    {
        std::cout << "[demo] routing table hot-reload\n";
        ConfigCoW routes;
        routes.set("/api/v1/users",  "service_a");
        routes.set("/api/v1/orders", "service_b");

        std::atomic<int> queries{0};
        std::atomic<bool> stop{false};

        // 3 條 reader 不斷查
        std::vector<std::thread> readers;
        for (int t = 0; t < 3; ++t) {
            readers.emplace_back([&]{
                while (!stop.load()) {
                    auto _ = routes.get("/api/v1/users");
                    queries.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        // ops thread: 1 ms 後 hot-reload 一次
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        routes.set("/api/v1/users", "service_a_v2");   // 新版本
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        stop.store(true);
        for (auto& r : readers) r.join();
        std::cout << "  " << queries.load() << " queries served during reload\n";
        std::cout << "  最終 /api/v1/users → " << routes.get("/api/v1/users") << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：CoW 的 deep copy 何時真的會發生?成本與資料大小的關係?
    //    A：每次 writer 進來都要 std::make_shared<MapT>(*cur) 整份複製。
    //       map 有 N 個 entry → O(N) 配置 + O(N) memcpy。1k entry ~10 µs、
    //       100k entry 可達 ms 級。所以 CoW 只適合「寫罕見」場景 (config、
    //       routing table、白名單,通常 1-100 Hz 寫頻率),不適合 hot map。
    //
    //  Q2：reader-heavy (千讀:一寫) 的場景,CoW 一定贏嗎?
    //    A：概念上是,但 libstdc++ 的 atomic<shared_ptr> 不是 lock-free
    //       (內部用 hash splitlock),純讀情境 reader 之間反而互相爭用,
    //       本課 PHASE A 就是 0.36× 反輸 shared_mutex。要拿到 CoW 概念
    //       上的勝利,得改用 hazard pointers (lesson 19) 或 RCU
    //       (liburcu) 把 reader 路徑換成 thread-local。
    //
    //  Q3：兩個 writer 同時跑會發生什麼?如何避免 lost update?
    //    A：兩 writer 都讀同一個 cur,各自 make_shared 並 store → 後寫覆
    //       蓋前寫。CoW 必須在 writer 端外加一把 mutex (本課就是 write_
    //       mtx_) 串行化寫者;或改用 compare_exchange loop 重試 (snapshot.
    //       compare_exchange_weak(cur, next))。reader 不受 writer mutex
    //       影響,所以 reader latency 仍然不會被寫者卡住。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. CoW *概念上* 是「重讀」場景的勝利方案:
//      - 讀者完全不協調 (拿到不可變指標後就自己讀)
//      - 寫者付高代價 (整體複製),換來讀完全免費
//      - 舊版本由 shared_ptr 引用計數自動回收
//
// 2. CoW 的 *libstdc++ 現實*:
//      std::atomic<std::shared_ptr<T>> 不是 lock-free。它內部
//      多半是:
//        - 一個 hash 化的 spin lock (對 ptr 做 hash,落到 N
//          個全域鎖之一)
//        - 或一個 mutex 在 atomic_shared_count_base 周圍
//      無論哪種,「讀者並不像理論那樣完全平行」。
//
// 3. 在這個函式庫稅之下,本課的 benchmark 結果通常會是:
//      PHASE A (純讀):shared_mutex 與 CoW 接近,或 shared_mutex
//        甚至更快 (因為 shared_mutex 內部的 reader-counter
//        可以更輕量地實作)。
//      PHASE B (讀+極慢寫):結果類似;少量寫並不會讓 CoW
//        翻盤,因為瓶頸在讀這側的隱藏鎖。
//      偶爾因為排程不同會看到 CoW 略勝,但這不是 CoW 在
//      「概念上應有的勝利」。
//
// 4. 真正能拿到 CoW 概念上的好處,需要繞開 std::atomic<
//    shared_ptr>:
//      (a) 用 raw 指標 + 「待回收清單」 + 「等所有讀者
//          結束」(grace period) —— 這就是 RCU。
//      (b) 用 hazard pointers (lesson 19) 保護指標。
//      (c) 用 `folly::FBVector` / `folly::Synchronized` /
//          folly::AtomicSharedPtr (Folly 自己重寫過的 lock-free
//          版本)。
//      (d) Userspace RCU (liburcu),Linux 上經得起考驗的
//          選擇。
//
// 5. 教學要點:這就是 *基準測試的價值*。憑直覺寫出來的
//    「優化」未必更快;放上去測之後常常還更慢。生產環境的
//    每一個性能宣稱都要有 *測量結果* 為證。
//
// 6. 何時 CoW 仍然是對的選擇 (即使在 libstdc++)?
//      - 寫者極度罕見 (例如系統初始化 + 偶爾 reload)。
//      - 你只有 1–2 個讀者,隱藏鎖的爭用不嚴重。
//      - 你需要「對外公開不可變的快照指標」這個語意,而非
//        為了極限性能。
//      - 程式還沒到那個瓶頸,簡單的 CoW 寫法就夠用,先求
//        對再求快。
// =============================================================
