// =============================================================================
//  課程 3.2：執行緒守衛類別設計 2  —  把守衛放進容器：std::vector<JoiningThread>
// =============================================================================
//
// 【主題資訊 Information】
//   本檔沿用「執行緒守衛類別設計 1」的 JoiningThread,重點放在
//   【容器整合】—— 這是移動語意真正發揮價值的場景。
//
//       std::vector<JoiningThread> threads;
//       threads.emplace_back(workerFunc, arg);   // 走完美轉發建構函式
//       ...
//   離開作用域 → vector 解構 → 逐一解構元素 → 每個都自動 join
//
//   標準版本：C++11
//   標頭檔  ：<thread>、<utility>、<vector>
//   複雜度  ：emplace_back 攤提 O(1);vector 解構為 O(N) 次 join,
//             總時間取決於最慢的那條執行緒
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「能放進 vector」是一道分水嶺】
//   ScopedThread / FlexibleThread 都不可搬移,所以你只能寫成:
//       ScopedThread t1{std::thread(w)};
//       ScopedThread t2{std::thread(w)};
//       ScopedThread t3{std::thread(w)};   // 數量寫死在原始碼裡
//   但真實需求幾乎都是「開 hardware_concurrency() 條」或「每個分片一條」,
//   數量在執行期才知道。要動態管理 N 個守衛,容器是唯一實際的選擇,
//   而容器要求元素可移動 —— 這就是 JoiningThread 補上移動語意的理由。
//
// 【2. vector 解構時到底發生什麼】
//   std::vector 的解構函式會【由後往前】逐一解構元素,然後釋放緩衝區。
//   每個 JoiningThread 解構時 join 自己那條執行緒,所以:
//     * 全部執行緒都會被等到,一條都不會漏
//     * 等待是【串行】的:先等最後一個、再等倒數第二個…
//   注意第二點的效能意涵:總等待時間是所有執行緒中【最慢那條】的時間
//   (因為它們是並行執行的),不是加總 —— 但如果 join 的順序不巧,
//   你會在某一個 join 上卡很久,而不是「均勻地等」。這對總時間沒有影響,
//   但在除錯時看堆疊會覺得「卡在某個 join」,不要誤以為是那條執行緒有問題。
//
// 【3. emplace_back 與完美轉發建構函式的配合】
//       threads.emplace_back([i]() { ... });
//   emplace_back 會在 vector 的記憶體上【就地建構】元素,把參數原樣
//   轉發給 JoiningThread 的建構函式 —— 這裡命中的是可變參數樣板那個,
//   於是 lambda 被直接轉發給 std::thread 的建構函式,執行緒立刻啟動。
//
//   對比 push_back:
//       threads.push_back(JoiningThread([i](){...}));   // 先建臨時物件再移動
//   多了一次移動(雖然 std::thread 的移動很便宜)。更重要的是 emplace_back
//   在語意上更貼近「在容器裡生一個出來」。
//
// 【4. reallocation:一個容易被忽略但很關鍵的細節】
//   vector 空間不足時會配置新緩衝區、把舊元素搬過去。搬的時候用的是
//   【移動建構】(因為 JoiningThread 的移動建構標了 noexcept)。
//   對 JoiningThread 而言這完全安全:移動只是把 std::thread 的 handle
//   換個位置,底層執行緒不受任何影響,也不會被 join 或 detach。
//
//   ⚠️ 但這件事在【持有參考的守衛】上就是災難。回想「RAII 與執行緒管理2」
//   的 ThreadGuard 持有 std::thread&:若那些 std::thread 放在會成長的
//   vector 裡,reallocation 會讓所有既有的參考懸空。
//   這正是本課從「持有參考」演進到「持有值」的實際理由之一。
//   實務建議:知道數量就先 reserve(),既避免重配置也少一次搬移。
//
// 【5. 為什麼不要在 worker 裡直接 std::cout】
//   五條執行緒同時對 std::cout 輸出,你會看到兩件事:
//     (a) 行與行之間的【順序】完全不確定 —— 沒有任何同步關係決定誰先誰後。
//     (b) 單一行的內容可能被切開交錯 —— 因為 "Worker " << i << "\n"
//         是三次獨立的 operator<< 呼叫,中間可能被切換走。
//   關於 (b) 要精確一點:std::cout 本身在 C++11 之後是執行緒安全的
//   (同一個 stream 的並行使用不會造成 data race,[iostream.objects]),
//   但那只保證「不會毀損 stream 物件」,【不保證輸出不交錯】。
//   本檔的示範因此改成:worker 只寫自己專屬的槽位,全部 join 之後
//   由主執行緒統一輸出 —— 這樣結果才是確定的、可驗證的。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 lambda 要用 [i] 值捕捉,不能用 [&i]?
//   迴圈變數 i 的生命週期只到迴圈結束。若用 [&i] 捕捉參考,
//   執行緒可能在迴圈早已結束後才讀 i → 懸空參考(未定義行為),
//   而且所有執行緒會看到同一個 i,結果毫無意義。
//   值捕捉 [i] 會在建立 lambda 的當下把 i 複製一份進 closure,
//   每條執行緒各有一份自己的副本,這才是正確的。
//
// (B) std::vector<JoiningThread> 為什麼不需要自訂解構?
//   因為 vector 的解構函式本來就會解構所有元素,而 JoiningThread 的
//   解構函式會 join。RAII 是可組合的:把會自己清乾淨的東西放進
//   會清乾淨元素的容器,整體就自動正確。這是 RAII 最漂亮的性質。
//
// (C) hardware_concurrency() 該怎麼用?
//   std::thread::hardware_concurrency() 回傳「實作對硬體並行度的估計」。
//   本機實測為 16。但請注意三件事:
//     1. 標準允許它回傳 0(代表「無法判定」),必須自己準備 fallback。
//     2. 它【不感知 cgroup / container 限制】—— 在被限制成 2 核的容器裡,
//        它可能仍回報宿主機的核心數,導致你開太多執行緒。
//     3. 它是【實作定義】的值,不同機器、不同函式庫都可能不同。
//   實務寫法:
//       unsigned n = std::thread::hardware_concurrency();
//       if (n == 0) n = 4;   // 明確的 fallback
//
// (D) 執行緒不是越多越好
//   建立執行緒有成本(核心資源、預設 8MB 的 stack 虛擬位址空間、
//   排程器負擔)。CPU-bound 工作開超過核心數只會增加 context switch;
//   I/O-bound 工作才可能受益於超額訂閱。真正的生產做法是用固定大小的
//   thread pool 重複利用執行緒,而不是每個任務開一條。
//
// 【注意事項 Pay Attention】
//   1. worker 直接 std::cout 的輸出【順序不確定、且可能交錯】。
//      std::cout 執行緒安全只保證不毀損 stream,不保證輸出不交錯。
//   2. 迴圈變數必須值捕捉 [i],用 [&i] 是懸空參考(未定義行為)。
//   3. 知道數量就 reserve(),避免 reallocation 的額外搬移。
//   4. 元素移動建構應標 noexcept,vector 才能提供強異常保證。
//   5. hardware_concurrency() 可能回傳 0,且不感知 cgroup 限制 ——
//      本機實測 16,但這是實作定義的值,不可寫死在程式邏輯裡。
//   6. vector 解構是【串行】join 每個元素,總耗時取決於最慢那條執行緒。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器管理執行緒
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::vector<std::thread> 可以,std::vector<ScopedThread> 不行?
//     答：vector 要求元素可移動(reallocation 與解構都需要)。std::thread
//         本身有移動建構與移動賦值,所以可以進 vector;而 ScopedThread
//         自訂了解構函式,隱式移動操作被抑制,複製又被 = delete,
//         兩條路都斷 → 不可搬移 → 進不了 vector。
//     追問：那 vector<std::thread> 有什麼風險?
//         → 它的解構函式【不會】幫你 join。vector 解構時逐一解構
//           std::thread 元素,任何一個還 joinable 就 std::terminate()。
//           所以用 vector<std::thread> 一定要記得迴圈 join;
//           改用 vector<JoiningThread> 或 vector<std::jthread> 才自動安全。
//
// 🔥 Q2. 五條執行緒同時 std::cout << "Worker " << i << "\n" 會怎樣?
//     答：兩件事。(1) 行的先後順序完全不確定,沒有任何同步關係;
//         (2) 同一行的內容可能被其他執行緒的輸出切開交錯,因為那是
//         三次獨立的 operator<< 呼叫。std::cout 在 C++11 之後是執行緒
//         安全的,但那只保證 stream 物件不會被毀損,不保證輸出不交錯。
//     追問：加 mutex 就能讓輸出順序變成 0,1,2,3,4 嗎?
//         → 不能。mutex 只能保證「一次一個執行緒寫」,消除交錯;
//           但誰先搶到鎖仍然不確定,順序依舊是隨機的。要固定順序,
//           就得讓每條執行緒寫進自己的槽位,全部 join 之後再依序輸出
//           (本檔的做法),或用 condition_variable 明確編排順序
//           (LeetCode 1114 的做法)。
//
// ⚠️ 陷阱 1. 「for (int i…) threads.emplace_back([&i]{ use(i); });
//              為什麼每條執行緒印出來的 i 都一樣、甚至是垃圾值?」
//     答：[&i] 捕捉的是迴圈變數的【參考】。所有 lambda 共用同一個 i,
//         而且 i 在迴圈結束後就離開作用域了 —— 執行緒之後再讀它就是
//         懸空參考,屬於未定義行為,沒有固定症狀。
//     為什麼會錯：把 lambda 捕捉想成「拍一張快照」。[&] 捕捉的是
//         參考,不是值;真正拍快照的是 [i] 或 [=]。
//
// ⚠️ 陷阱 2. 「我用 vector<std::thread> 然後在最後迴圈 join,
//              但中間某個地方 return 了 —— 反正 vector 會幫我清理吧?」
//     答：不會。vector 的解構函式只負責解構元素,而 std::thread 的
//         解構函式對 joinable 的物件是呼叫 std::terminate()。
//         提前 return 會讓整個程式當場終止。
//     為什麼會錯：把「容器會清理元素」等同於「容器會正確釋放資源」。
//         容器只做解構;元素的解構語意是否正確,是元素型別的責任。
//         這正是要用 JoiningThread / std::jthread 的原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 沿用「執行緒守衛類別設計 1」的 JoiningThread(完整說明見該檔)
// -----------------------------------------------------------------------------
class JoiningThread {
    std::thread t;

public:
    JoiningThread() noexcept = default;

    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}

    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}

    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {
            if (t.joinable()) {
                t.join();
            }
            t = std::move(other.t);
        }
        return *this;
    }

    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;

    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }

    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }
    void join() { t.join(); }
    void detach() { t.detach(); }
    std::thread& get() noexcept { return t; }
};

// -----------------------------------------------------------------------------
// 示範 1:vector 裝載守衛,離開作用域自動全部 join
//
//   每個 worker 只寫自己專屬的槽位 results[i] —— 不同執行緒寫不同元素,
//   沒有任何兩者存取同一個記憶體位置,因此【沒有 data race】,不需要鎖。
//   (注意:std::vector<int> 的不同元素是不同物件,並行寫入是安全的;
//    唯一的例外是 std::vector<bool>,它是位元壓縮的特化,不可如此使用。)
// -----------------------------------------------------------------------------
void demoVectorOfGuards() {
    const int N = 5;
    std::vector<int>           results(N, 0);
    std::vector<JoiningThread> threads;
    threads.reserve(N);  // 先配置,避免 reallocation

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([i, &results]() {
            results[i] = i * i;  // 各寫各的槽位,無 data race
        });
    }

    std::cout << "  已建立 " << threads.size() << " 個 worker\n";
    threads.clear();  // 逐一解構 → 逐一 join(也可以等作用域結束)

    std::cout << "  全部 join 完成,結果:";
    for (std::size_t i = 0; i < results.size(); ++i) {
        std::cout << (i ? " " : "") << results[i];
    }
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範 2:hardware_concurrency() —— 實作定義的值
// -----------------------------------------------------------------------------
void demoHardwareConcurrency() {
    unsigned n = std::thread::hardware_concurrency();
    std::cout << "  hardware_concurrency() = " << n
              << "(實作定義;標準允許回傳 0,且不感知 cgroup 限制)\n";

    unsigned effective = (n == 0) ? 4u : n;  // 明確的 fallback
    std::cout << "  實際採用的執行緒數 = " << effective << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1114. Print in Order
//
//   題目：三條執行緒分別呼叫 first()/second()/third(),呼叫順序不定,
//         但必須保證輸出永遠是 "firstsecondthird"。
//
//   為什麼用到本主題：這題的解法本體(Foo 類別)只負責【順序協調】,
//         但要真正跑起來需要有人建立並回收那三條執行緒 ——
//         在 LeetCode 上是評測框架做的,在本機就得自己來。
//         本檔的 std::vector<JoiningThread> 正好是最自然的驅動方式:
//         三條執行緒放進容器、以【打亂的順序】啟動,離開作用域自動全部
//         join。輸出仍然固定是 firstsecondthird,證明順序是由 Foo 內部
//         的 condition_variable 決定,與執行緒的啟動順序無關。
//
//   解法：用一個 step 計數器 + condition_variable。
//         每個函式等到「輪到自己」才執行,做完推進 step 並喚醒其他人。
//         用 notify_all 而非 notify_one:被喚醒的執行緒可能不是該輪到的
//         那個,notify_one 有喚醒錯對象而全體卡死的風險。
//         while 迴圈(而非 if)是必要的 —— 條件變數允許 spurious wakeup。
// -----------------------------------------------------------------------------
class Foo {
    std::mutex              m;
    std::condition_variable cv;
    int                     step = 1;  // 下一個該執行的步驟

public:
    void first(const std::function<void()>& printFirst) {
        std::unique_lock<std::mutex> lk(m);
        printFirst();
        step = 2;
        cv.notify_all();
    }

    void second(const std::function<void()>& printSecond) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this] { return step == 2; });  // while 語意,免疫 spurious wakeup
        printSecond();
        step = 3;
        cv.notify_all();
    }

    void third(const std::function<void()>& printThird) {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this] { return step == 3; });
        printThird();
    }
};

void demoLeetCode1114() {
    Foo         foo;
    std::string out;
    std::mutex  outMutex;

    auto emit = [&out, &outMutex](const char* s) {
        std::lock_guard<std::mutex> lk(outMutex);
        out += s;
    };

    {
        std::vector<JoiningThread> threads;
        threads.reserve(3);

        // 刻意用「third → first → second」的順序啟動,證明結果與啟動順序無關
        threads.emplace_back([&] { foo.third ([&] { emit("third");  }); });
        threads.emplace_back([&] { foo.first ([&] { emit("first");  }); });
        threads.emplace_back([&] { foo.second([&] { emit("second"); }); });
    }  // vector 解構 → 三條執行緒全部 join

    std::cout << "  啟動順序 third→first→second,實際輸出 = \"" << out << "\"\n";
    std::cout << "  是否符合要求: " << std::boolalpha
              << (out == "firstsecondthird") << "\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】平行掃描 access log 統計 HTTP 狀態碼
//
//   情境:一台前端伺服器每天產生數百萬行 access log,維運要快速得到
//         各類狀態碼的分布(2xx/3xx/4xx/5xx)以判斷服務是否異常。
//
//   為何適合本主題:把 log 切成 N 個分片,每個分片一條執行緒。
//         N 在執行期才決定(取決於 hardware_concurrency 與檔案大小),
//         所以必須用容器管理守衛。
//
//   關鍵設計:每條執行緒統計到【自己專屬的 Counters 槽位】,
//         彼此不共享任何可變狀態 → 完全不需要鎖,也沒有 false sharing
//         以外的效能問題。全部 join 之後,主執行緒才做歸併(reduce)。
//         這是 map-reduce 在單機上最樸素也最有效的形式。
// -----------------------------------------------------------------------------
struct Counters {
    int ok       = 0;  // 2xx
    int redirect = 0;  // 3xx
    int clientErr= 0;  // 4xx
    int serverErr= 0;  // 5xx
};

void tallyChunk(const std::vector<int>& codes, std::size_t begin,
                std::size_t end, Counters& out) {
    for (std::size_t i = begin; i < end; ++i) {
        int c = codes[i];
        if      (c >= 500) ++out.serverErr;
        else if (c >= 400) ++out.clientErr;
        else if (c >= 300) ++out.redirect;
        else if (c >= 200) ++out.ok;
    }
}

Counters scanAccessLog(const std::vector<int>& codes, unsigned workers) {
    if (workers == 0) workers = 1;
    const std::size_t n     = codes.size();
    const std::size_t chunk = (n + workers - 1) / workers;  // 向上取整

    std::vector<Counters>      partial(workers);
    std::vector<JoiningThread> threads;
    threads.reserve(workers);

    for (unsigned w = 0; w < workers; ++w) {
        std::size_t begin = std::min(static_cast<std::size_t>(w) * chunk, n);
        std::size_t end   = std::min(begin + chunk, n);
        threads.emplace_back([&codes, begin, end, &partial, w]() {
            tallyChunk(codes, begin, end, partial[w]);  // 各寫各的槽位
        });
    }

    threads.clear();  // 全部 join

    Counters total;
    for (const auto& p : partial) {
        total.ok        += p.ok;
        total.redirect  += p.redirect;
        total.clientErr += p.clientErr;
        total.serverErr += p.serverErr;
    }
    return total;
}

int main() {
    std::cout << "=== 示範 1:vector<JoiningThread> 自動全部 join ===\n";
    demoVectorOfGuards();

    std::cout << "\n=== 示範 2:hardware_concurrency() ===\n";
    demoHardwareConcurrency();

    std::cout << "\n=== LeetCode 1114. Print in Order ===\n";
    demoLeetCode1114();

    std::cout << "\n=== 日常實務:平行掃描 access log ===\n";
    // 造一份可重現的假 log:10000 筆,依固定規則分布
    std::vector<int> codes;
    codes.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        if      (i % 100 == 0) codes.push_back(500);  // 1%   5xx
        else if (i % 20  == 0) codes.push_back(404);  // 4%   4xx(扣掉重疊)
        else if (i % 10  == 0) codes.push_back(301);  // 5%   3xx
        else                   codes.push_back(200);
    }

    Counters c = scanAccessLog(codes, 4);
    std::cout << "  總筆數 = " << codes.size() << "\n";
    std::cout << "  2xx = " << c.ok
              << " / 3xx = " << c.redirect
              << " / 4xx = " << c.clientErr
              << " / 5xx = " << c.serverErr << "\n";
    std::cout << "  合計檢查 = " << (c.ok + c.redirect + c.clientErr + c.serverErr)
              << "(應等於總筆數)\n";

    // 換不同的分片數,結果必須完全相同 —— 這是平行歸併正確性的基本驗證
    Counters c1 = scanAccessLog(codes, 1);
    Counters c7 = scanAccessLog(codes, 7);
    bool same = (c1.ok == c.ok && c1.redirect == c.redirect &&
                 c1.clientErr == c.clientErr && c1.serverErr == c.serverErr &&
                 c7.ok == c.ok && c7.redirect == c.redirect &&
                 c7.clientErr == c.clientErr && c7.serverErr == c.serverErr);
    std::cout << "  分片數 1 / 4 / 7 結果一致: " << std::boolalpha << same << "\n";

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.2：執行緒守衛類別設計2.cpp" -o guard2

// === 預期輸出 ===
// === 示範 1:vector<JoiningThread> 自動全部 join ===
//   已建立 5 個 worker
//   全部 join 完成,結果:0 1 4 9 16
//
// === 示範 2:hardware_concurrency() ===
//   hardware_concurrency() = 16(實作定義;標準允許回傳 0,且不感知 cgroup 限制)
//   實際採用的執行緒數 = 16
//
// === LeetCode 1114. Print in Order ===
//   啟動順序 third→first→second,實際輸出 = "firstsecondthird"
//   是否符合要求: true
//
// === 日常實務:平行掃描 access log ===
//   總筆數 = 10000
//   2xx = 9000 / 3xx = 500 / 4xx = 400 / 5xx = 100
//   合計檢查 = 10000(應等於總筆數)
//   分片數 1 / 4 / 7 結果一致: true
//
// === 全部示範結束 ===
