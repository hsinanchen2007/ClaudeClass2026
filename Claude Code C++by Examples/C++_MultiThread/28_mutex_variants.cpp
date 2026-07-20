// =====================================================================
// 28_mutex_variants.cpp
//
// 主題:std::recursive_mutex / std::timed_mutex / std::recursive_timed_mutex
//
// 為什麼還需要這課:
//   前面 lesson 03/10/21 主要用 std::mutex / std::shared_mutex /
//   std::scoped_lock。但 STL 還有 *變體* mutex 是專為兩個情境設計的:
//
//     A. 同一個執行緒會 *再進入* 同一段臨界區
//        → std::recursive_mutex
//     B. 想設定 「我最多等 X 毫秒,等不到就放棄」
//        → std::timed_mutex / std::recursive_timed_mutex
//
//   兩個都是 std::mutex 的擴充。一般 std::mutex 的規則是:
//     - 不可重入:同一執行緒第二次 lock() 同一把 mutex 是 UB。
//     - 沒有 timeout:lock() 一定會「無限等」直到拿到。
//
// 主要 API:
//   std::recursive_mutex::lock() / unlock() / try_lock()
//       同一執行緒可以連 lock N 次,要 unlock N 次才真的釋放。
//
//   std::timed_mutex::try_lock_for(duration) → bool
//   std::timed_mutex::try_lock_until(time_point) → bool
//       timeout 到了還沒拿到就回 false,不會卡住。
//
//   std::recursive_timed_mutex
//       兩個都加。同一執行緒可重入 + try_lock_for/until。
//
// 重要陷阱:
//   1. recursive_mutex 是 *方便,但通常是設計味道*。
//      如果你必須用 recursive_mutex,常常代表你的 callstack 在
//      自己呼叫自己時不小心又回來抓同一把鎖 —— 重新切設計通常
//      可以避免。但 legacy code / callback 風格 API 有時躲不掉。
//
//   2. try_lock_for 的 duration 是 *最小* 等待時間。實作可以
//      因為 spurious wake 提前回來,所以你還是要檢查回傳值。
//
//   3. try_lock_for 不保證「精確到毫秒」。它使用 steady_clock,
//      timeout 會被 OS 排程粒度影響 (Linux 預設 ~1ms,看 HZ)。
//
// Build:  g++ -std=c++17 -O2 -pthread 28_mutex_variants.cpp -o 28_mutex_variants
// =====================================================================

// =====================================================================
// 課程資訊 (Class Info)
// =====================================================================
// 主題:     std::mutex 的三個標準變體 (recursive / timed / recursive_timed)
// 前置課程: lesson 03 (std::mutex + lock_guard) / lesson 21 (deadlock 與 scoped_lock)
// 觀念詞彙:
//   - reentrancy           ── 同一條 thread 可重複 lock 而不 deadlock
//   - lock_count           ── recursive_mutex 內部記錄重入次數的計數器
//   - timeout              ── 等不到鎖就放棄,改走 fallback 路徑
//   - deadline             ── 整體預算的「絕對時間點」(常見於 RPC)
//   - try_lock_for / until ── 相對 vs 絕對的 timeout 介面
// 新介紹 API:
//   std::recursive_mutex                 同一 thread 可重入的 mutex
//   std::timed_mutex                     可帶 timeout 的 mutex
//   std::recursive_timed_mutex           兩者合一
//   m.try_lock_for(duration)   → bool    從現在起最多等 d
//   m.try_lock_until(time_pt)  → bool    等到絕對時間點為止
//   std::lock_guard<M> lk(m, std::adopt_lock)  接管「呼叫者已 lock 過」的鎖
// 何時使用:
//   - recursive_mutex:legacy / 第三方 callback 強迫同 thread 重入時的務實解
//   - timed_mutex   :latency-critical 但 best-effort 的 reader、UI 主執行緒、
//                     有 SLA 的 RPC handler、watchdog
// 何時不要用:
//   - 一般專案 90% 場景:用 std::mutex 就好,別動變體
//   - 「想用 recursive_mutex」往往代表設計可以拆 → 先嘗試把 critical
//     section 抽出 private helper,public 方法 lock 後呼叫 private 不再 lock
//   - 把 try_lock_for 當精確 timer:它只保證「至少等這麼久」,精度受 OS
//     排程粒度限制 (Linux 預設 ~1 ms)
// 常見錯誤:
//   - recursive_mutex unlock 次數不對等 → 鎖永遠沒釋放
//   - try_lock_for 忘記檢查回傳值 → 沒拿到也照樣動 critical section (UB)
//   - timed_mutex 把 try_lock_for 拉太大 → 跟普通 lock() 沒差,失去 timeout
//     的意義
//   - 期待 std::shared_recursive_mutex:標準根本沒提供 (見 Deep Dive #6)
// =====================================================================

// =====================================================================
// 深入解析 (Deep Dive) ── mutex 變體的設計動機
// =====================================================================
//
// 1. 為什麼 std::mutex 不可重入?
//    重入 (recursive) lock 是「同一條 thread 多次 lock 同一把鎖」。
//    std::mutex *不允許*,理由:
//      - 簡單實作只用一個 atomic owner field + 一個 wait queue,size = 8 bytes
//      - 不需要計數,unlock 必然立刻釋放
//      - 性能極致 (~15-25 ns)
//    recursive_mutex 必須額外維護 lock_count,大小通常翻倍 + lock 慢
//    幾 ns。
//
// 2. recursive_mutex 是設計味道 (大多時候)
//    若你的程式 *必須* 用 recursive_mutex,典型來源:
//      A. callback 模式:函式 X 持鎖呼叫 callback,callback 又呼叫 X。
//      B. 物件方法呼叫自己的 public 方法:public 方法都加鎖,內部呼叫
//         同個物件的另一個 public 方法時撞到。
//      C. 遞迴遍歷樹/圖,每個節點都加鎖。
//    更乾淨的解:
//      A. callback 在鎖外呼叫
//      B. 把同步邏輯拉到 *private 函式*,public 方法 lock 之後呼叫 private
//         不再 lock
//      C. 整棵樹用一把外層鎖,遍歷不再每節點 lock
//    legacy / 第三方 callback API 真的躲不掉時,recursive_mutex 是務實解。
//
// 3. timed_mutex 的應用場景
//    A. UI 主執行緒:不能讓使用者卡死,「等 100ms 拿不到鎖就走 fallback」。
//    B. 有 SLA 的服務:request handler 設 deadline,逼近 timeout 時放棄。
//    C. deadlock 偵測:若有時 lock 異常久,記錄並 abort,留下 stack。
//    D. 測試:在 unit test 裡確保「對方應該 unlock 才對」── 等不到就 fail。
//
// 4. try_lock_for vs try_lock_until
//    try_lock_for(d)        ── 從現在等 d 時間
//    try_lock_until(tp)     ── 等到絕對 time_point
//    後者比前者好:你算 deadline 一次後傳給多個操作,每個自己檢查還剩
//    多少。常見 RPC 程式:整體 budget 200 ms,每個下游 (DB、cache、外部
//    API) 都用 try_lock_until(deadline)。
//
// 5. 內部實作 (NPTL)
//    pthread_mutex_t 透過 attr 設定 PTHREAD_MUTEX_RECURSIVE / TIMED 等。
//    std::recursive_mutex = pthread_mutex_t with RECURSIVE attr,內部多
//    一個 owner_tid + count。
//    std::timed_mutex     = pthread_mutex_t + futex 帶 timeout 的 syscall
//    路徑 (FUTEX_WAIT_BITSET 與 abs deadline)。
//    std::recursive_timed_mutex = 兩者合一。
//
// 6. 為什麼沒有 std::shared_recursive_mutex?
//    遞迴 + 讀寫鎖會引入「reader 持鎖中升級成 writer」的 deadlock 問題
//    (見 lesson 10),標準乾脆不提供。Boost.Thread 有
//    boost::shared_recursive_mutex,但用的人少且踩坑多。
//
// 7. 性能對照 (rough)
//      std::mutex                ~15-25 ns lock+unlock (uncontended)
//      std::recursive_mutex      ~20-30 ns
//      std::timed_mutex          ~20-30 ns (timeout 路徑沒走時)
//      std::shared_mutex (read)  ~30-50 ns (atomic counter)
//      std::shared_mutex (write) ~50-100 ns (要等 reader 全清空)
//    mutex 變體之間的差異對絕大多數應用可忽略。決策依據是 *語意*,不是
//    幾 ns 的差。
//
// 8. 用法總結 (決策樹)
//      要重入嗎?              是 → recursive_*  否 → 一般版
//        要 timeout 嗎?         是 → timed_*     否 → 一般版
//      要多 reader?           是 → shared_*    否 → 一般版
//    四個維度組合 → std::mutex / recursive_mutex / timed_mutex /
//    recursive_timed_mutex / shared_mutex / (沒 shared_timed) ...。
//    一般專案 90% 場景 std::mutex 就夠;特殊需求才往變體走。
// =====================================================================

// =====================================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =====================================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── LC 不考 timed_mutex /
//     recursive_mutex。實戰 callback 重入或 deadline-aware lock 才會用到。
//   → 應用聯想:
//     - lesson 30 Q5 BoundedBlockingQueue 的 push,若你想改成「等 100ms
//       拿不到鎖就放棄」,把 mutex 換成 timed_mutex + try_lock_for 即可。
//     - lesson 30 Q9 Traffic Light 若擴充為「車有最大等待時間,過了就走
//       fallback」就要 timed_mutex。
//
// 主要 API 對照 (cppreference):
//   - std::recursive_mutex              https://en.cppreference.com/w/cpp/thread/recursive_mutex
//   - std::timed_mutex                  https://en.cppreference.com/w/cpp/thread/timed_mutex
//   - std::recursive_timed_mutex        https://en.cppreference.com/w/cpp/thread/recursive_timed_mutex
//   - timed_mutex::try_lock_for         https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_for
//   - timed_mutex::try_lock_until       https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_until
//
// 練習建議:
//   - 把 lesson 30 Q5 BoundedBlockingQueue::push 加上 try_push_for(d, x)
//     版本 (mutex 換 timed_mutex)。
// =====================================================================

/*
補充筆記：mutex_variants
  - mutex_variants 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - mutex_variants 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】mutex 變體：recursive_mutex / timed_mutex
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. recursive_mutex 什麼時候用？為什麼被視為設計氣味？
//     答：std::mutex 不可重入——同一 thread 第二次 lock() 是 UB，實務上表現為自我死鎖。
//         recursive_mutex 允許同一 thread 重複上鎖 N 次（要 unlock N 次才真的釋放），
//         常被用來救「public 函式鎖了，內部又呼叫另一個也會鎖的 public 函式」。但這
//         通常代表鎖粒度設計不良：正解是拆出「不上鎖的 private 實作函式」，由 public
//         薄殼負責上鎖一次。此外它較慢，也讓「我現在持有哪些不變式」難以推理。
//     追問：為什麼「難以推理」是實質問題？（遞迴上鎖代表你可能在不變式尚未恢復的中間
//         狀態下，又進入了另一段假設不變式成立的臨界區）
//
// 🔥 Q2. timed_mutex 的實際用途？
//     答：try_lock_for / try_lock_until 讓你設定「最多等這麼久」，避免無限等待。用途
//         有二：一是把潛在死鎖降級成可觀測、可恢復的錯誤（逾時就退讓重試、記錄告警），
//         而不是整個服務靜默卡死；二是在有延遲預算的路徑上，寧可放棄這次操作也不要
//         拖垮上游。
//     追問：try_lock 迴圈重試要注意什麼？（必須先釋放已持有的鎖再退讓，否則仍然是
//           「佔有並等待」；另外要加隨機退讓時間避免活鎖 livelock）
//
// ⚠️ 陷阱. std::mutex 對同一 thread 連續 lock() 兩次，會死鎖還是 UB？
//     答：標準規定是 UB。常見實作的表現是自我死鎖（程式卡住），但這只是「碰巧的症狀」
//         ——不能寫程式去依賴它，也不能靠「它會卡住所以測得出來」來驗收。
//     為什麼會錯：許多人記成「mutex 不可重入 = 會死鎖」，把某個實作的觀察結果當成標準
//         保證。要重入請明確使用 recursive_mutex，但更好的是重新設計介面。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------
// Demo A:recursive_mutex
//
// 模擬一個樹狀資料結構:走訪節點時,函式會遞迴呼叫自己。
// 每次進入函式都要拿鎖 (例如為了在拿鎖期間印出整顆樹)。
// 用 std::mutex → 第二次 lock() 是 UB (deadlock 或直接當掉)。
// 用 std::recursive_mutex → OK,計數器會記得「同一執行緒已經拿了 N 次」。
// ---------------------------------------------------------------------
struct TreeNode {
    int value;
    std::vector<TreeNode> children;
};

class RecursivePrinter {
public:
    void print(const TreeNode& node, int depth = 0) {
        // 同一個執行緒在 print() 內又會呼叫自己 (子節點)。
        // 第二次進來時,recursive_mutex 不會 deadlock,而是把
        // 「同一執行緒持有計數」加 1。離開時 unlock 也要對等次數。
        std::lock_guard<std::recursive_mutex> lk(mtx_);

        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << node.value << '\n';

        for (const auto& c : node.children) {
            print(c, depth + 1);   // ← 同一執行緒,再 lock 一次
        }
    }

private:
    std::recursive_mutex mtx_;
};

void demo_recursive_mutex() {
    std::cout << "=== Demo A: recursive_mutex ===\n";

    TreeNode tree{1, {
        TreeNode{2, {
            TreeNode{4, {}},
            TreeNode{5, {}},
        }},
        TreeNode{3, {
            TreeNode{6, {}},
        }},
    }};

    RecursivePrinter p;

    // 兩個 thread 同時 print → 兩棵樹的輸出不會交錯,
    // 但同一個 thread 內部的遞迴 lock 不會 deadlock。
    std::thread t1([&]{ p.print(tree); });
    std::thread t2([&]{ p.print(tree); });
    t1.join(); t2.join();
    std::cout << '\n';
}

// ---------------------------------------------------------------------
// Demo B:timed_mutex + try_lock_for
//
// 情境:一個 worker 持有 mutex 在做慢工作 (3 秒);
//      另一個 thread 想 access,但 *最多只等 500 ms*,
//      等不到就放棄、走 fallback 路徑 (不會卡到天荒地老)。
//
// 這個 pattern 在「latency-critical 但 best-effort」很常見:
//   - 量度系統:metric snapshot 抓不到鎖就用上一次的值
//   - GUI 重繪:資料鎖拿不到就跳過這次 frame
//   - Watchdog:某個 worker 卡住不要讓自己也卡住
// ---------------------------------------------------------------------
std::timed_mutex tmtx;
int shared_value = 0;

void slow_worker() {
    std::lock_guard<std::timed_mutex> lk(tmtx);
    std::cout << "  [slow_worker] 取得鎖,模擬 3 秒慢動作...\n";
    std::this_thread::sleep_for(3s);
    shared_value = 42;
    std::cout << "  [slow_worker] 完成,釋放鎖\n";
}

void impatient_reader(int id) {
    // try_lock_for 回傳 bool:拿到 = true,timeout = false。
    if (tmtx.try_lock_for(500ms)) {
        std::lock_guard<std::timed_mutex> lk(tmtx, std::adopt_lock);
        std::cout << "  [reader " << id << "] 拿到鎖,值 = "
                  << shared_value << '\n';
    } else {
        // ← fallback 路徑:不卡住,做點 best-effort 的事。
        std::cout << "  [reader " << id
                  << "] 等了 500ms 沒拿到,跳過 (fallback)\n";
    }
}

void demo_timed_mutex() {
    std::cout << "=== Demo B: timed_mutex + try_lock_for ===\n";

    std::thread w(slow_worker);
    std::this_thread::sleep_for(50ms);   // 確保 worker 先拿到

    // 三個 reader 各自最多等 500ms。它們應該全部 timeout。
    std::thread r1(impatient_reader, 1);
    std::thread r2(impatient_reader, 2);
    std::thread r3(impatient_reader, 3);

    r1.join(); r2.join(); r3.join();
    w.join();

    // 這次 worker 早就放掉了,直接拿到。
    impatient_reader(99);
    std::cout << '\n';
}

// ---------------------------------------------------------------------
// Demo C:try_lock_until 用絕對時間
//
// try_lock_for 是「相對時間」(從現在開始最多等 N)。
// try_lock_until 是「絕對時間」(等到指定時間點為止)。
//
// 用 *until* 的好處是:你可以在 callstack 裡傳遞同一個 deadline,
// 不會因為層層的 sleep / 計算而把 "最多等到 X" 重複放大成 "等更久"。
//
// 這是 RPC framework / database client 處理 deadline 的標準做法。
// ---------------------------------------------------------------------
void deadline_aware(std::timed_mutex& m,
                    std::chrono::steady_clock::time_point deadline,
                    int id)
{
    // 不管前面已經花了多久,這裡是「等到 deadline 為止」。
    if (m.try_lock_until(deadline)) {
        std::lock_guard<std::timed_mutex> lk(m, std::adopt_lock);
        std::cout << "  [deadline_aware " << id << "] 拿到鎖\n";
        std::this_thread::sleep_for(80ms);
    } else {
        std::cout << "  [deadline_aware " << id
                  << "] 已超過 deadline,放棄\n";
    }
}

void demo_try_lock_until() {
    std::cout << "=== Demo C: try_lock_until (absolute deadline) ===\n";

    std::timed_mutex m;
    auto deadline = std::chrono::steady_clock::now() + 300ms;

    // 三個 thread 共用「同一個 deadline」。前面拿到的會佔住一段
    // 時間,後面的會剩下越來越短的等待視窗,deadline 一到就放棄。
    std::thread a(deadline_aware, std::ref(m), deadline, 1);
    std::thread b(deadline_aware, std::ref(m), deadline, 2);
    std::thread c(deadline_aware, std::ref(m), deadline, 3);
    std::thread d(deadline_aware, std::ref(m), deadline, 4);
    a.join(); b.join(); c.join(); d.join();
    std::cout << '\n';
}

// ---------------------------------------------------------------------
// Demo D:recursive_timed_mutex = 兩個合一
//
// 用得最少的版本,但偶爾在 callback heavy 又有 deadline 的設計
// (例如 GUI / event loop) 會用到。範例就只示範它能做兩件事:
//   - 同一個 thread 連 lock N 次
//   - 提供 try_lock_for
// ---------------------------------------------------------------------
void demo_recursive_timed_mutex() {
    std::cout << "=== Demo D: recursive_timed_mutex (兩個都有) ===\n";

    std::recursive_timed_mutex rtm;

    // 1. 同一 thread 連 lock 兩次,證明可重入。
    rtm.lock();
    bool nested = rtm.try_lock_for(10ms);
    std::cout << "  same-thread re-entry try_lock_for: "
              << (nested ? "OK" : "FAIL") << '\n';
    if (nested) rtm.unlock();
    rtm.unlock();

    // 2. 換另一個 thread 來 try_lock_for,正常該拿到 (鎖已釋放)。
    std::thread t([&]{
        bool got = rtm.try_lock_for(10ms);
        std::cout << "  other-thread try_lock_for after release: "
                  << (got ? "OK" : "FAIL") << '\n';
        if (got) rtm.unlock();
    });
    t.join();
    std::cout << '\n';
}

// ---------------------------------------------------------------------
// 實戰範例: RPC 的 deadline 傳遞 (try_lock_until)
// ---------------------------------------------------------------------
// 應用場景: RPC server 收到 request, header 帶 deadline (例如
// "60 秒後過期")。整段處理路徑共用同一個 deadline:
//   handler → db_layer → 鎖一個共享 connection pool slot
// 每層拿鎖都用 try_lock_until(deadline), 而不是 try_lock_for(N)。
// 好處: 不論前面花了多久, 這層永遠用 *剩下的時間* 來嘗試, 不會
// 把 client 的 60 秒誤拉成 60+60+60 秒。
//
// 這就是 gRPC C++、AWS SDK、database client 內部的 timing 慣例。
// ---------------------------------------------------------------------
void demo_rpc_deadline() {
    std::cout << "=== Demo E (extra): RPC deadline 跨函式傳遞 ===\n";
    std::timed_mutex conn_pool;
    auto deadline = std::chrono::steady_clock::now() + 200ms;

    auto rpc_handler = [&](int id){
        // step 1: parse 假裝花 50 ms
        std::this_thread::sleep_for(50ms);
        // step 2: 拿 connection ── 用 *同一個* deadline, 自動扣掉前面 50 ms
        if (conn_pool.try_lock_until(deadline)) {
            std::lock_guard<std::timed_mutex> lk(conn_pool, std::adopt_lock);
            std::cout << "  [rpc " << id << "] got conn, query DB...\n";
            std::this_thread::sleep_for(80ms);
        } else {
            std::cout << "  [rpc " << id << "] deadline 已過, 503 給 client\n";
        }
    };
    // 一條 thread 先卡住 conn pool 100 ms
    std::thread hog([&]{
        std::lock_guard<std::timed_mutex> lk(conn_pool);
        std::this_thread::sleep_for(100ms);
    });
    // 兩個 rpc handler 進來搶
    std::thread h1(rpc_handler, 1);
    std::thread h2(rpc_handler, 2);
    hog.join(); h1.join(); h2.join();
    std::cout << '\n';
}

// ---------------------------------------------------------------------
// main
// ---------------------------------------------------------------------
int main() {
    demo_recursive_mutex();
    demo_timed_mutex();
    demo_try_lock_until();
    demo_recursive_timed_mutex();
    demo_rpc_deadline();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：spinlock 何時會比 std::mutex 快?何時反而更慢?
    //    A：critical section 極短 (< 100 ns) 且爭用低 → spinlock 快,因
    //       為省了 futex syscall (約 1-2 µs)。一旦 critical section 變長
    //       (> 1 µs) 或爭用高 → spinlock 燒 CPU 不睡覺,所有等待者都在
    //       100% CPU 上空轉,total throughput 比 mutex 慢得多 (mutex 會
    //       讓等待者 sleep 到 futex wakeup)。原則:user-space 99% 場景用
    //       std::mutex,kernel/中斷 handler 不能 sleep 才用 spinlock。
    //
    //  Q2：try_lock_for(100ms) 真的會精確等 100 ms 嗎?
    //    A：不會。它的語意是「至少等 100 ms 才放棄」── 實際 wakeup 受
    //       OS 排程粒度限制 (Linux 預設 HZ ~250-1000,粒度 1-4 ms)。實測
    //       try_lock_for(100ms) 可能在 100-104 ms 之間返回。要更精確 (例
    //       如 RPC deadline) 用 try_lock_until 配絕對時間點,跨函式呼叫
    //       傳同一個 deadline,每層各自檢查還剩多少 budget,不會疊加誤差。
    //
    //  Q3：為什麼標準沒有 std::shared_recursive_mutex?
    //    A：reader 持鎖時若想升級成 writer,必然 deadlock (兩個 reader
    //       都升級就互相等);加上 recursive 同一 thread 又能 reader-then-
    //       writer 重入,語意更複雜、實作更 buggy、效能更差。委員會評估
    //       後決定不提供,逼使用者重新設計 (用 std::shared_mutex + 釋放
    //       後重抓 unique_lock)。Boost.Thread 有 boost::shared_recursive_
    //       mutex 但用的人少且踩坑多 ── 出現「想要這個」通常代表設計味道。
    //
    return 0;
}

// =====================================================================
// 帶走的重點:
//
// 1. std::mutex 不可重入,同一 thread 二次 lock = UB。如果你
//    *真的* 需要重入,用 std::recursive_mutex。但這通常是設計味道。
//
// 2. std::timed_mutex 解鎖了「等不到就放棄」的 latency 預算。
//    try_lock_for(duration) 是相對時間,try_lock_until(tp) 是
//    絕對時間。後者在傳遞 deadline 跨函式呼叫時更乾淨。
//
// 3. try_lock_for / try_lock_until 都會被 OS 排程粒度影響,
//    不要當成精確 timer。它的語意是「至少等這麼久才放棄」。
//
// 4. std::recursive_timed_mutex = recursive + timed,但很少用到。
//    如果你發現自己在用,先想能不能拆成 try_lock + 不重入。
//
// 5. 在 lock 之外,std::lock_guard / std::unique_lock / std::scoped_lock
//    對所有這些 mutex 變體都成立 —— RAII 才是主要使用方式,
//    手動 lock()/unlock() 通常表示出錯機會大。
// =====================================================================
