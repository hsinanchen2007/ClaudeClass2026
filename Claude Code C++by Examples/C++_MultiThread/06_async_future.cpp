// =============================================================
// 06_async_future.cpp  --  std::async + std::future
// =============================================================
//
// 本課目標:
//   1. 在另一個執行緒上執行函式,並把它的 *回傳值* 拿回來
//      —— 這是 std::thread 沒辦法直接做到的。
//   2. 學會 std::future<T>:一個指向「之後才會出現的值」的
//      handle (代理物件)。
//   3. 看到例外會自動跨越執行緒邊界 (這是相對於原始
//      std::thread 的一大優點)。
//   4. 理解兩種啟動策略 (launch policy),以及那個惡名昭彰的
//      「std::async 回傳的 future 解構子會阻塞」陷阱。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 06_async_future.cpp \
//         -o 06_async_future
//
// 執行方式:
//     ./06_async_future
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     std::async + std::future ── 從 thread 拿回傳值
// 前置課程: lesson 01, 03
// 觀念詞彙:
//   - future         ── 對「以後才會到」的值的 handle
//   - promise        ── future 的另一頭,用來填值的物件
//   - launch policy  ── async / deferred,決定何時/在哪執行
//   - shared_future  ── 可被多個讀者 .get() 的 future
// 新介紹 API:
//   std::async(policy, fn, args...)  啟動非同步工作,得 future
//   future.get()                     阻塞,取得值或重新丟出例外
//   future.wait()                    阻塞,但不取出值
//   future.wait_for(d)               計時等待,回傳 status
//   std::launch::async               強制開新 thread
//   std::launch::deferred            在 .get() 才同步執行
//   std::promise<T> + .set_value     手動填一個 future
// 何時使用:
//   - 「丟一個工作背景跑,我要它的回傳值」
//   - 想讓例外自動跨 thread 傳遞
// 何時不要用:
//   - 大量短小工作 → thread pool (lesson 08)
//   - 只是要 fire-and-forget → 注意:future 解構會 *阻塞*
// 常見錯誤:
//   - std::async(work) 沒指定 policy → 可能在 .get() 才執行
//   - 不接 future 的回傳值 → 解構子阻塞,看似序列
//   - 同一個 future 呼叫兩次 .get() → UB
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── async / future 內部模型
// =============================================================
//
// 1. shared state 是什麼
//    promise 與 future 之間透過一個 *shared state* (heap 上的物件) 連通。
//    它包含:
//      - 結果欄位 (T 或 exception_ptr)
//      - 狀態旗標 (ready / not_ready)
//      - cv + mutex,讓 future.wait()/.get() 阻塞
//      - ref count (promise 端 + future 端)
//    每次 std::async 至少做一次 heap allocation + 一次 atomic ref-count
//    操作 → 不便宜。要每秒 fire 上萬個 task 應該用 thread pool (lesson 08)。
//
// 2. std::async 的 policy 陷阱
//    std::async(f) 不指定 policy 時,「實作可選」── 多數平台是
//    launch::async | launch::deferred,意思是 *可能* 真的開新 thread,
//    *也可能* 直到你 .get() 才執行 (deferred,單 thread)。
//    所以 `std::async(f); std::async(g);` 不指定可能變成「f 跑完才跑 g
//    都在主 thread」。
//    解法:永遠明寫 `std::async(std::launch::async, f)` 強制開 thread。
//
// 3. 為什麼 std::async 的 future 解構子會阻塞 (聞名的 ~future 死鎖)
//    當 future 從 std::async(launch::async) 取得時,標準規定其解構子
//    必須等 task 完成才能返回 (避免 dangling)。實務後果:
//        std::async(std::launch::async, slow_fn);   // 結果沒接住
//    這條 future 是個臨時值,在 *當下這行* 解構,於是 *阻塞當前 thread
//    直到 slow_fn 結束* ── 看起來是平行,實際上序列。把 future 接住:
//        auto fut = std::async(std::launch::async, slow_fn);
//        // 現在 fut 活到 scope 結束,解構才阻塞
//
// 4. promise / future / packaged_task / async ── 四件套對照
//      std::promise<T>           底層原料,你自己 set_value/set_exception
//      std::future<T>            promise 配對的「未來值」,只能 get 一次
//      std::shared_future<T>     future 的可複製版本,可被多消費者 get
//      std::packaged_task<F>     把可呼叫物件包成「執行後寫進它的 future」
//                                適合丟進 thread pool 執行
//      std::async(policy, F)     上面三個的高階組合,自動開 thread
//    思路:async = packaged_task + thread。如果你已經有自己的 pool
//    (lesson 08),選 packaged_task 自己塞;沒有 pool、就一次性平行,
//    選 async。
//
// 5. .get() 只能呼叫一次
//    .get() 把結果 *move 出來*,呼叫第二次拿到的是 valid()=false 的 future,
//    再 get 是 UB。需要多人讀同一個結果 → shared_future 或自己快取。
//
// 6. exception 怎麼傳遞
//    task 內 throw 任何例外 → promise 端 set_exception (透過
//    std::current_exception())。future.get() 端會 *重新 throw* 同樣的
//    例外。所以「平行執行 + 例外語意」是免費跨 thread 傳遞的,但要記得:
//    若 task 用 launch::deferred 而你從未 .get(),例外會被默默吞掉。
//
// 7. 為什麼 future 是「未來」而不是「立刻」
//    future.wait() 與 .get() 透過 cv 阻塞 caller。底層 OS 仍是 futex 等
//    promise 那側 set 完之後 notify。代價 ~1-3 µs 進出 kernel,跟一把
//    爭用 mutex 是同一個量級。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ async/future *不在* LC Concurrency 標籤的 9 題中。LC 偏好「3 條
//     thread 同步輸出」這類底層題,async/future 更像高階組合。
//   → 但 lesson 30 Q8 (Web Crawler) 若改用 std::async + std::future
//     收集每個 URL 的子工作回傳值,是經典變形。本課的 promise/future
//     模型也是 lesson 08 thread pool submit() 介面的核心。
//
// 主要 API 對照 (cppreference):
//   - std::async                        https://en.cppreference.com/w/cpp/thread/async
//   - std::launch (policy enum)         https://en.cppreference.com/w/cpp/thread/launch
//   - std::future<T>                    https://en.cppreference.com/w/cpp/thread/future
//   - std::shared_future<T>             https://en.cppreference.com/w/cpp/thread/shared_future
//   - std::promise<T>                   https://en.cppreference.com/w/cpp/thread/promise
//   - std::packaged_task<F>             https://en.cppreference.com/w/cpp/thread/packaged_task
//   - std::future_status                https://en.cppreference.com/w/cpp/thread/future_status
//
// 練習建議:
//   - 讀完本課,試著把 lesson 30 Q8 Web Crawler 從「raw thread + queue」
//     改寫成「std::async + 收 future」── 你會看到 future 的解構子阻塞
//     語意如何幫忙做 termination detection。
//   - 進階:讀 lesson 08 thread pool 的 submit<F>(...) → future<R> 介面
//     如何用 packaged_task 串接。
// =============================================================

/*
補充筆記：std::async_future
  - std::async 回傳 future，future::get 會等待結果並重新丟出工作中的例外。
  - launch::async 與 launch::deferred 代表立即開執行緒或延後到 get/wait 才執行。
  - future 只能 get 一次；需要共享結果才使用 shared_future。
  - std::async_future 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/
#include <iostream>
#include <future>     // std::async, std::future, std::launch
#include <thread>     // std::this_thread::sleep_for
#include <chrono>
#include <vector>
#include <stdexcept>
#include <numeric>    // std::accumulate

// -------------------------------------------------------------
// 一個假裝很耗時的函式。睡 `ms` 毫秒,然後回傳
// 1 + 2 + ... + n。我們有意 sleep,讓「並行做這件事」
// 真的能在牆上時間 (wall-clock) 看到改善。
// -------------------------------------------------------------
long long slow_sum(int n, int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    long long s = 0;
    for (int i = 1; i <= n; ++i) s += i;
    return s;
}

// 一個會失敗的版本,用來示範例外跨越執行緒。
int fails()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    throw std::runtime_error("something went wrong on the worker");
}

// -------------------------------------------------------------
// 輔助小工具:碼錶 (stopwatch)。每次呼叫 .ms() 都會回傳
// 自從建構以來經過了幾毫秒。純 C++,沒用到執行緒。
// -------------------------------------------------------------
struct Stopwatch {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    long long ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - t0).count();
    }
};

int main()
{
    // =========================================================
    // PART 1 —— 序列化的基準
    //
    // 三次 slow_sum,一個接一個。每次 ~200 ms,所以總牆上
    // 時間大約 ~600 ms。
    // =========================================================
    {
        Stopwatch sw;
        long long a = slow_sum(1000, 200);
        long long b = slow_sum(2000, 200);
        long long c = slow_sum(3000, 200);
        std::cout << "[sequential] sum = " << (a + b + c)
                  << "   wall = " << sw.ms() << " ms\n";
    }

    // =========================================================
    // PART 2 —— 用 std::async 平行化
    //
    // std::async( policy, fn, args... )  -->  std::future<R>
    //
    //   - 啟動 `fn(args...)` (依 policy 決定方式,見下)。
    //   - 回傳一個 std::future<R>,稍後它會持有回傳值
    //     (或執行函式時丟出來的例外)。
    //
    // 我們傳 std::launch::async 來 *強制* 真的開新執行緒。
    // 沒指定的話,實作可以拖到你呼叫 .get() 時才執行
    // (就是 std::launch::deferred 的情形),那樣就會默默地
    // 變成單執行緒。
    //
    // future.get():
    //   - 阻塞,直到值已經就緒,
    //   - 回傳那個值,
    //   - 若函式有丟例外,在這裡重新丟出,
    //   - 同一個 future 只能 .get() *一次*。
    // =========================================================
    {
        Stopwatch sw;

        std::future<long long> fa =
            std::async(std::launch::async, slow_sum, 1000, 200);
        std::future<long long> fb =
            std::async(std::launch::async, slow_sum, 2000, 200);
        std::future<long long> fc =
            std::async(std::launch::async, slow_sum, 3000, 200);

        // 此時三個工作 *已經* 同時在背景執行緒上跑。
        // .get() 會阻塞,直到該 future 的答案出來。
        long long a = fa.get();
        long long b = fb.get();
        long long c = fc.get();

        std::cout << "[parallel  ] sum = " << (a + b + c)
                  << "   wall = " << sw.ms() << " ms"
                  << "  (~3x faster: three 200ms tasks overlapped)\n";
    }

    // =========================================================
    // PART 3 —— 例外免費跨越執行緒邊界
    //
    // 用 std::thread 時,執行緒函式內未被處理的例外會呼叫
    // std::terminate —— 整個程式都死掉。改用 std::async,
    // 例外會被 future *捕獲*,等你呼叫 .get() 時才重新丟出。
    // 你可以照常 try/catch。
    // =========================================================
    {
        std::future<int> f = std::async(std::launch::async, fails);
        try {
            int x = f.get();           // 在這一行重新丟出
            std::cout << "got " << x << '\n';   // 永遠到不了
        } catch (const std::exception& e) {
            std::cout << "[exception ] caught from worker: "
                      << e.what() << '\n';
        }
    }

    // =========================================================
    // PART 4 —— 一個小型的平行歸約 (reduction)
    //
    // 把 vector 切成兩半,各用一個執行緒求和。這個模式可以
    // 推廣到 N 段、N 顆核心。
    // =========================================================
    {
        std::vector<int> v(10'000'000, 1);   // sum 應為 10,000,000

        Stopwatch sw;
        auto mid = v.begin() + v.size() / 2;

        std::future<long long> left = std::async(std::launch::async,
            [&] { return std::accumulate(v.begin(), mid, 0LL); });

        // 後半就在 *當前* 這個執行緒上做,沒有必要為了等它
        // 而再生一個 worker。這是個常見的慣用寫法。
        long long right = std::accumulate(mid, v.end(), 0LL);

        long long total = left.get() + right;
        std::cout << "[reduction ] total = " << total
                  << "   wall = " << sw.ms() << " ms\n";
    }

    // =========================================================
    // 實戰範例 1: future::wait_for 做 RPC timeout
    // =========================================================
    // 應用場景: 對外發 RPC 請求, 但不想無限等。給一個 timeout,
    // 過了就走 fallback (例如回 cached value 或記錄 error)。
    //
    // 重點: future::wait_for 不會取消正在跑的 task ── 即使 timeout
    // 走 fallback, 背景 task 仍在跑, future 解構時還是會等它。
    // 真正的「取消」需要 stop_token (lesson 07)。
    // =========================================================
    {
        std::cout << "\n[demo] RPC timeout with future::wait_for\n";
        auto slow_rpc = []{
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            return std::string("rpc-result");
        };
        std::future<std::string> f = std::async(std::launch::async, slow_rpc);
        // 只等 100 ms
        if (f.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready) {
            std::cout << "  got: " << f.get() << '\n';
        } else {
            std::cout << "  TIMEOUT! falling back to cached value\n";
            // (背景 task 仍在跑, f 解構時會等它完成)
        }
    }

    // =========================================================
    // 實戰範例 2: std::packaged_task 把 callable 變成 future
    // =========================================================
    // 應用場景: 自己手刻 thread pool 或 task queue 時, 想保有
    // 「呼叫者拿 future、worker 拿可呼叫物」的解耦。packaged_task
    // 就是把一個 callable 包成 (1) 一個可呼叫物 + (2) 一個 future。
    //
    //   packaged_task<int(int,int)> task(my_fn);
    //   future<int> f = task.get_future();
    //   // 把 task 丟給 worker
    //   worker_thread([t = std::move(task)]() mutable { t(3, 4); });
    //   // 主執行緒
    //   int r = f.get();
    //
    // 這是 lesson 08 thread pool submit() 的核心機制。
    // =========================================================
    {
        std::cout << "\n[demo] packaged_task + thread\n";
        std::packaged_task<int(int,int)> task([](int a, int b){
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return a + b;
        });
        std::future<int> fut = task.get_future();
        // 把 task 搬到另一條 thread 執行
        std::thread t(std::move(task), 7, 35);
        std::cout << "  result = " << fut.get() << " (預期 42)\n";
        t.join();
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::launch::async 跟 std::launch::deferred 有什麼差?
    //    A：async 強制建立新 thread 立即執行, future.get() 是同步等待。
    //       deferred 完全不啟動 thread, 直到呼叫 future.get() 才在「呼
    //       叫者的 thread」執行 ── 等於同步呼叫, 沒有並行。預設策略
    //       (省略第一參數) 是 async|deferred「擇一」, 由實作決定, 行
    //       為不可預期。實務上要平行就明確寫 std::launch::async。
    //
    //  Q2：std::async 回傳的 future 解構子為什麼會阻塞?
    //    A：這是標準的特例 ── 為了避免「async 啟動的 thread 沒人等就
    //       被遺棄」, std::async 的 future 解構子會 block 直到 task
    //       完成。後果: auto f = std::async(...); 在下一行就同步等了,
    //       完全沒並行。要平行多個 task, 必須把所有 future 全收進
    //       vector 後再 .get(), 別讓中間的 future 提前 destruct。
    //
    //  Q3：例外怎麼跨 thread 傳? std::thread 為什麼做不到?
    //    A：std::async / packaged_task 內部用 std::promise 包住 task,
    //       發生例外時呼叫 set_exception(current_exception()), future.
    //       get() 會 rethrow。std::thread 是純啟動 thread, 沒有 promise
    //       這層通道, 例外往外丟會直接 std::terminate, 所以 thread
    //       function 必須自己 try/catch 或不丟例外。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. std::async 是「我想從另一個執行緒拿到回傳值」的工具。
//    std::thread 只能乾淨地跑回傳 void 的工作。
//
// 2. *永遠* 傳 std::launch::async,除非你刻意想要 deferred
//    執行。預設策略是「async 或 deferred,由實作決定」,
//    這可能會默默把你「以為是平行」的程式碼變回序列。
//
//        // 不好:可能在 .get() 時才在 *呼叫者* 的執行緒上執行
//        auto f = std::async(work);
//
//        // 好:保證現在就開新執行緒執行
//        auto f = std::async(std::launch::async, work);
//
// 3. 解構子的陷阱。
//    std::async 回傳的 std::future 有一個特別的解構子:
//    它會 *阻塞* 到該工作完成。所以:
//
//        std::async(std::launch::async, slow_work);
//        // ^ 這裡產生的是個臨時的 future,馬上被銷毀,
//        //   所以這一行會 *阻塞* 到 slow_work 完成!
//
//    跟 std::async 用「fire-and-forget」幾乎永遠是錯的。
//    把 future 賦值給一個有名字的變數,讓你決定何時銷毀它。
//
// 4. 同一個 future 的 .get() 只能呼叫 *一次*。如果你需要
//    多個讀者,改用 std::shared_future。
//
// 5. 想從另一個你自己寫的執行緒手動把 future 填好,而不是
//    讓 std::async 幫你做?用 std::promise<T>:
//        std::promise<int> p;
//        std::future<int>  f = p.get_future();
//        std::thread t([&]{ p.set_value(42); });
//        std::cout << f.get();      // 42
//        t.join();
//    用於手刻的執行緒之間訊號傳遞很有用。
//
// 6. std::async 適合 *粒度較粗* 的工作 (數十毫秒以上)。
//    要跑成千上萬個小工作,改用執行緒池或平行演算法
//    (std::execution::par,後面有專門的一課)。
// =============================================================
