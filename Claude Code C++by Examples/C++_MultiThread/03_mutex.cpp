// =============================================================
// 03_mutex.cpp  --  用 std::mutex 修好資料競爭
// =============================================================
//
// 本課目標:
//   1. 使用 std::mutex 讓 ++counter 在多執行緒下變得安全。
//   2. 學會 std::lock_guard —— 一個 RAII 包裝器,在建構子內
//      上鎖、在解構子內解鎖。這就是現代 C++ 中使用 mutex 的
//      慣用做法。
//   3. 看到結果現在 *永遠* 都精確等於 2,000,000,不論你執行
//      幾次,也不論你用什麼 -O 等級編譯。
//
// 編譯方式:
//     g++ -std=c++17 -O0 -pthread 03_mutex.cpp -o 03_mutex
//
// 執行方式:
//     ./03_mutex
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     用 mutex 把 critical section 保護起來
// 前置課程: lesson 01, 02
// 觀念詞彙:
//   - mutex (mutual exclusion) ── 同一時間只允許一個 thread 進入
//   - critical section          ── 持鎖期間執行的程式碼區段
//   - RAII                      ── 物件生命週期 = 資源生命週期
//   - exception safety          ── 拋例外時資源仍正確釋放
// 新介紹 API:
//   std::mutex                 最基本的互斥鎖
//   std::lock_guard<Mutex>     RAII 包裝:建構鎖、解構解鎖
//   m.lock() / m.unlock()      手動操作 (避免使用,沒有 RAII)
//   std::scoped_lock           (C++17) 同時鎖多把,避免死鎖
// 何時使用:
//   - 多執行緒讀寫同一份 *複合* 狀態 (vector、map、多個欄位)
//   - 操作不能分割成單一個 atomic
// 何時不要用:
//   - 單一變數的 +1、=1 → std::atomic (lesson 04) 更快
//   - 高 reads / 低 writes → std::shared_mutex (lesson 10)
// 常見錯誤:
//   - 手動 lock()/unlock() 而非 lock_guard → 例外時鎖不釋放
//   - 在 critical section 內做 I/O → 把多執行緒程式變回序列
//   - 漏鎖某個寫入路徑 → race 仍存在
//   - 鎖兩把以順序不一致 → deadlock (lesson 21)
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── mutex 內部與正確使用心智模型
// =============================================================
//
// 1. mutex 在 Linux 上是什麼
//    glibc 的 std::mutex (NPTL 的 pthread_mutex_t) 採 *futex* 機制。
//    futex = Fast Userspace muTEX。它有兩個層級:
//      - 無爭用時:純 user-space CAS (atomic compare_exchange) 標記
//        「已上鎖」,不進 kernel,成本約 ~15-25 ns。
//      - 有爭用時:CAS 失敗 → 進 syscall futex(FUTEX_WAIT),由 kernel
//        把 thread 放入等待佇列。被叫醒後 unlock 端執行 futex(FUTEX_WAKE)。
//        進出 kernel 的代價約 1-3 µs。
//
//    這個「樂觀 user-space 路徑 + 悲觀 kernel 路徑」設計就是為什麼
//    「lock 一把幾乎沒被別人搶的鎖」幾乎免費 ── 99% 案例不會踏進 kernel。
//
// 2. lock_guard / unique_lock / scoped_lock 差在哪
//    - std::lock_guard<M>     最便宜;只能 lock/unlock,不能 unlock 後再 lock。
//    - std::unique_lock<M>    可暫時 unlock、可 try_lock_for、可被 cv 用。
//                             成本多一個 bool 欄位記住「我有沒有 own」。
//    - std::scoped_lock<M...> C++17,可同時鎖多把,內含 std::lock 的
//                             deadlock-avoidance 演算法 (try-and-back-off)。
//    一般單把鎖選 lock_guard;cv 必須用 unique_lock;鎖兩把以上選
//    scoped_lock。
//
// 3. RAII 為什麼是非用不可
//    手動 m.lock()/unlock() 一旦中間 throw 例外,unlock 不會被呼叫,
//    鎖永遠不釋放 → 全程式卡死。lock_guard 在解構子裡呼叫 unlock,
//    例外路徑也保證執行 → C++ 多執行緒的「unlock 必然發生」靠 RAII。
//
// 4. mutex 的成本曲線 (rough numbers)
//      無爭用 lock+unlock     ~15-25 ns      (純 user-space)
//      有爭用 (進 futex)       ~1-3 µs       (進 kernel)
//      被搶 + context switch   ~5-10 µs      (排到別的核醒過來)
//    意涵:臨界區短於 1 µs 時,*別讓鎖變熱*── 一旦熱了 cache line
//    狂跳 (lesson 13 false sharing 的同型),效能會懸崖式下降。
//
// 5. critical section 內該做什麼、不該做什麼
//    該做:單純的 in-memory 改寫 (改 map、push 進 vector)。
//    不該做:I/O (write、send)、sleep、呼叫外部 callback、await future。
//    理由:鎖被你佔住的時間 = 全部其他 thread 的等待時間。把 I/O 拉到
//    鎖外、只在鎖內做最小修改,是寫高併發程式的核心原則。
//
// 6. mutex 不是 fence
//    一個常見誤解:「我有 mutex 保護就一切正確」。錯。mutex *只保護它
//    所屬臨界區內的存取*。如果你有兩條變數 X、Y,執行緒 A 在持鎖時改 X
//    與 Y,執行緒 B *不持鎖* 讀 Y,Y 的讀仍是 race。
//    lock 提供的同步是「lock A 的 release」 → 「lock A 的 acquire」之間
//    的 happens-before。沒進這個鎖的 thread 不在這條序裡。
//
// 7. mutex vs spinlock (前瞻 lesson 11/29)
//    spinlock 是「不睡,直接迴圈 retry」。臨界區極短 (~100 ns) 時 spinlock
//    比 mutex 快 (省了 syscall);但臨界區一長就把 CPU 燒光。一般選 mutex,
//    spinlock 留給 kernel-level 或極端低延遲場景。
//
// 8. priority inversion (常見面試陷阱)
//    高優先 thread A 等中優先 thread B 持有的鎖,但 OS 排到低優先 C
//    跑了 → A 被 C 「間接」卡住。Linux 有 PI futex 解,但 std::mutex 沒
//    暴露介面;若需要請用 pthread_mutexattr_setprotocol(PRIO_INHERIT)。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✓ LC 1279  Traffic Light Controlled Intersection
//     - 一把 mutex 保護「目前綠燈在哪條路」── 純 mutex 經典應用。
//     - 完整解答 → lesson 30 Q9
//   ✓ LC 1188  Design Bounded Blocking Queue
//     - mutex 保護 queue,搭配 cv 等待 (cv 在 lesson 05)。
//     - 完整解答 → lesson 30 Q5
//   ✓ LC 1226  Dining Philosophers
//     - 用 std::scoped_lock 一次拿兩根筷子,避免 deadlock。
//     - 完整解答 → lesson 30 Q7  (deadlock 預防專課:lesson 21)
//
// 主要 API 對照 (cppreference):
//   - std::mutex                        https://en.cppreference.com/w/cpp/thread/mutex
//   - std::mutex::lock / unlock         https://en.cppreference.com/w/cpp/thread/mutex/lock
//   - std::mutex::try_lock              https://en.cppreference.com/w/cpp/thread/mutex/try_lock
//   - std::lock_guard                   https://en.cppreference.com/w/cpp/thread/lock_guard
//   - std::unique_lock                  https://en.cppreference.com/w/cpp/thread/unique_lock
//   - std::scoped_lock (C++17)          https://en.cppreference.com/w/cpp/thread/scoped_lock
//
// 練習建議:
//   讀完本課,直接去 lesson 30 看 Q9 (純 mutex) 跟 Q5 (mutex + cv 組合)。
//   想深入「鎖多把怎麼避免 deadlock」 → lesson 21。
// =============================================================

/*
補充筆記：std::mutex
  - mutex 保護的是共享資料的不變式，不只是某一行程式。
  - lock_guard/scoped_lock 用 RAII 解鎖，比手動 lock/unlock 更不容易漏解鎖。
  - 多把鎖同時使用時要固定順序或用 scoped_lock 避免 deadlock。
  - std::mutex 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <thread>
#include <mutex>      // std::mutex, std::lock_guard
#include <vector>
#include <unordered_map>
#include <string>

// -------------------------------------------------------------
// 現在共享狀態變成兩塊一起出現:
//   - 我們要保護的資料 (`counter`)
//   - 守護它的 mutex     (`counter_mtx`)
//
// 命名慣例:用 mutex 所守護的資料來命名它。當你看到
// `counter_mtx`,就知道你必須先持有這把鎖才能去碰哪份資料。
// mutex 本身不會保護任何東西 —— 它只是執行緒之間的一個
// *承諾*:大家在動相關資料前都會先把它鎖起來。
// -------------------------------------------------------------
long long  counter = 0;
std::mutex counter_mtx;

// -------------------------------------------------------------
// 實戰範例: thread-safe 快取 (thread-safe cache)
// -------------------------------------------------------------
// 應用場景: 多條 worker thread 共用一份 unordered_map 當作小型
// 快取 (key→value, 例如 DNS 解析結果、symbol→price 查詢)。任何
// 對 map 的 insert / lookup 都必須持鎖, 因為 unordered_map *不是*
// thread-safe ── 同時 rehash 跟讀會 corrupt 整個 bucket chain。
//
// 重點:
//   - 持鎖時間越短越好 → 只在「找/塞」那一刻持鎖, 其他工作
//     (例如算 hash key 對應的 value) 在鎖外做。
//   - 回傳要 *拷貝出來* 而不是回傳 iterator/reference, 因為一
//     離開 lock_guard, map 隨時可能被改, 引用就懸空。
// -------------------------------------------------------------
class ThreadSafeCache {
    std::unordered_map<std::string, int> map_;
    std::mutex mtx_;
public:
    // 寫入: 持鎖更新
    void put(const std::string& k, int v) {
        std::lock_guard<std::mutex> lk(mtx_);
        map_[k] = v;
    }
    // 讀取: 持鎖查找, 拷貝回傳 (不是 reference!)
    bool get(const std::string& k, int& out) {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = map_.find(k);
        if (it == map_.end()) return false;
        out = it->second;        // 拷貝, 釋放鎖後 out 仍有效
        return true;
    }
    size_t size() {
        std::lock_guard<std::mutex> lk(mtx_);
        return map_.size();
    }
};

void demo_thread_safe_cache()
{
    std::cout << "\n[demo] thread-safe cache (mutex + unordered_map)\n";
    ThreadSafeCache cache;
    auto writer = [&](int base) {
        for (int i = 0; i < 1000; ++i)
            cache.put("key" + std::to_string(base * 1000 + i), base * 1000 + i);
    };
    std::thread w1(writer, 0), w2(writer, 1), w3(writer, 2);
    w1.join(); w2.join(); w3.join();
    std::cout << "  cache size = " << cache.size() << " (預期 3000)\n";
}

// =============================================================
// LeetCode 1279  Traffic Light Controlled Intersection
// 難度: medium (LC 標 medium, 但邏輯很簡單)
// =============================================================
// 題意: 一個十字路口, 兩條道路 A/B 互相垂直。一次只能有一條
//       道路放行 (因為共用同一個綠燈)。每台車到達時呼叫
//       carArrived(carId, roadId, ...), 若目前綠燈在自己的路
//       上就直接過, 不在自己路上要把燈轉過來再過。
//
// 解題思路:
//   - 共享狀態 = 「目前哪條路是綠燈」(一個 int road_in_green)
//   - 進入十字路口的整段邏輯 (檢查 + 轉燈 + 過車) 都要 *互斥*,
//     不然兩台不同方向的車會撞 → 用 std::mutex 包整段。
//   - 不需要 cv, 因為車不必「等」── 拿到鎖就直接決定怎麼過。
//
// 複雜度: 每筆 carArrived 是 O(1), 但會串行化 (mutex)。
// 邊界: roadId ∈ {1, 2}; 互相垂直, 任何時刻最多一條路綠燈。
//
// 完整解答在 lesson 30 Q9, 這裡示範核心 mutex 結構。
// =============================================================
class TrafficLight {
    std::mutex mtx_;
    int road_in_green_ = 1;   // 1 表示 road1 綠燈, 2 表示 road2 綠燈
public:
    void carArrived(int carId, int roadId) {
        std::lock_guard<std::mutex> lk(mtx_);     // 整段互斥
        if (road_in_green_ != roadId) {
            // 把綠燈轉過來 (在實際題目要呼叫 turnGreen(roadId))
            road_in_green_ = roadId;
            std::cout << "    light → road " << roadId
                      << " (car " << carId << ")\n";
        }
        // 過車 (在實際題目要呼叫 crossCar(carId))
        std::cout << "    car " << carId << " crosses road " << roadId << '\n';
    }
};

void demo_traffic_light()
{
    std::cout << "\n[demo] LC 1279 traffic light\n";
    TrafficLight tl;
    // 模擬 4 台車從兩條路交替到達
    std::thread t1([&]{ tl.carArrived(1, 1); tl.carArrived(3, 1); });
    std::thread t2([&]{ tl.carArrived(2, 2); tl.carArrived(4, 2); });
    t1.join(); t2.join();
}

void increment(int times)
{
    for (int i = 0; i < times; ++i) {

        // -----------------------------------------------------
        // std::lock_guard —— 本課最重要的類別。
        //
        // 建構子: 呼叫 counter_mtx.lock()   (若另一個執行緒
        //         已經持有,會等)
        // 解構子: 呼叫 counter_mtx.unlock() (在下面的 `}` 處
        //         自動執行,當 `lk` 離開作用域時)
        //
        // 為何要 RAII (Resource Acquisition Is Initialization)?
        //   - 你 *不可能* 忘記 unlock。
        //   - 如果被保護的程式碼丟出例外,堆疊會展開,
        //     `lk` 被銷毀,mutex 就被釋放。一組裸的
        //     lock()/unlock() 在丟例外時會洩漏鎖,讓整個
        //     程式陷入死鎖。
        //
        // 大括號 `{ ... }` 形成一個「臨界區 (critical section)」:
        // 持鎖的最小區域。盡可能讓 mutex 持有得越短越好 ——
        // 你持鎖時,所有想要這把鎖的執行緒都被擋住了。
        // -----------------------------------------------------
        {
            std::lock_guard<std::mutex> lk(counter_mtx);
            ++counter;        // 安全:同一時間只有一個執行緒在這裡
        }   // <-- lk 在這裡被銷毀,mutex 在這裡被釋放

        // 大括號 *外面* 的程式碼是在 *沒有* 持鎖的狀態下執行。
        // 這是好事:把不需要共享的工作放在臨界區外,讓其他
        // 執行緒不必等你。
    }
}

int main()
{
    const int N = 1'000'000;

    std::thread t1(increment, N);
    std::thread t2(increment, N);

    t1.join();
    t2.join();

    std::cout << "Expected counter = " << (2LL * N) << '\n';
    std::cout << "Actual   counter = " << counter   << '\n';
    std::cout << "Lost increments  = " << (2LL * N - counter) << '\n';

    // ---------------------------------------------------------
    // 兩個延伸示範: thread-safe cache + LC 1279
    // ---------------------------------------------------------
    demo_thread_safe_cache();
    demo_traffic_light();

    // ---------------------------------------------------------
    // 要記住的事
    //
    // 1. mutex 保護的是 *資料*,不是程式碼。每次從任何執行緒
    //    存取被保護的資料時,都要鎖同一把 mutex。
    //    只要有一次沒鎖,所有保護就毀了。
    //
    // 2. 鎖持有的時間越短越好。一個冗長的臨界區會把你的
    //    「多執行緒」程式變回單執行緒,還更慢。
    //
    // 3. *絕對不要* 在臨界區內做任何緩慢或會阻塞的事
    //    (檔案 I/O、網路呼叫、std::cout 輸出到終端機……)。
    //    先算好,再用最短的時間鎖起來把結果發布出去。
    //
    // 4. 優先使用 std::lock_guard (或 C++17 的 std::scoped_lock),
    //    不要自己手動 lock()/unlock()。RAII 不是可選項 ——
    //    這是唯一在例外發生時仍然安全的做法。
    //
    // 5. 如果你發現自己需要同時鎖兩把 mutex,請使用
    //    std::scoped_lock(m1, m2)。它會以不會死鎖的方式同時
    //    鎖住兩把 —— 這留待後面的課再講。
    // ---------------------------------------------------------

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：lock_guard 跟 unique_lock 該怎麼選?
    //    A：能用 lock_guard 就用 lock_guard ── 它最便宜, 只能 lock/
    //       unlock, 沒其他狀態。需要「中途暫時 unlock」、「try_lock_
    //       for」、或要餵給 condition_variable.wait() 時, 才必須用
    //       unique_lock (它多帶一個 bool 記錄擁有狀態, 體積稍大)。
    //       同時鎖兩把以上選 C++17 的 scoped_lock。
    //
    //  Q2：為什麼 std::mutex 在無爭用時幾乎免費 (~20ns), 一旦爭用就
    //       跳到 µs 級?
    //    A：Linux 上的 std::mutex 走 futex 機制。無爭用時純 user-space
    //       CAS 完成 (一條 lock cmpxchg), 不進 kernel; 一旦 CAS 失敗,
    //       才呼叫 futex(FUTEX_WAIT) 把 thread 放到 kernel 等待佇列。
    //       進出 kernel + context switch 大約 1-3 µs, 比 user-space
    //       慢 100 倍。所以「短臨界區 + 低爭用」是 mutex 表現最好的場景。
    //
    //  Q3：mutex 是 fence 嗎? 鎖住一邊就保證另一邊看到所有改動?
    //    A：mutex 提供的同步只在「同一把鎖」之間: lock A.release →
    //       lock A.acquire 之間有 happens-before。如果 thread B 沒持有
    //       同一把鎖去讀寫, 那讀寫仍是 race, mutex 救不了它。「mutex 保
    //       護的是資料, 不是程式碼」── 每個碰共享資料的路徑都要鎖同
    //       一把, 漏一個就全毀。
    //
    return 0;
}
