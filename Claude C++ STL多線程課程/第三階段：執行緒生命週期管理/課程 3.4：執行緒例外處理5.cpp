// =============================================================================
//  課程 3.4：執行緒例外處理 — 第 5 部分：std::async / std::future（推薦做法）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<future>（async / future / promise / packaged_task）、<stdexcept>
//   標準版本：C++11
//
//     template<class F, class... Args>
//     std::future<R> std::async(std::launch policy, F&& f, Args&&... args);
//
//     enum class launch { async = 1, deferred = 2 };   // 可用 | 組合
//
//     T future<T>::get();       // 取得結果；若存的是例外則在此重新拋出
//     void future<T>::wait() const;
//     std::future_status wait_for(const chrono::duration&) const;
//     bool future<T>::valid() const noexcept;
//
//   複雜度：get() 會阻塞至結果就緒；例外重拋為 O(1)（引用計數）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 future 是三種做法中的首選】
//   前面示範過三種跨執行緒回報錯誤的方式：
//     (a) 就地捕獲          —— 最簡單，但呼叫端完全感知不到失敗（第 2 部分）
//     (b) exception_ptr     —— 可傳遞，但樣板碼多、要自己管生命週期（第 3 部分）
//     (c) 自寫 SafeThread   —— 封裝了 (b)，但不支援回傳值（第 4 部分）
//   std::future 一次解決全部問題：
//     * 同一個 get() 既取【回傳值】也取【例外】，不必分兩套機制；
//     * 例外自動裝箱、自動傳遞、在 get() 處自動重拋，零樣板碼；
//     * 生命週期由共享狀態的引用計數管理，不會懸空。
//   所以實務準則是：需要回報成敗或取得結果時，優先用 std::async / std::future。
//
// 【2. get() 到底做了什麼】
//   共享狀態（shared state）裡存的是「值」或「例外」二選一。get() 的邏輯是：
//       等待就緒 → 若存的是例外 → rethrow_exception(那個例外)
//                → 否則         → 回傳（或搬移出）那個值
//   注意本檔的關鍵現象：worker() 裡的 return 42 【永遠不會執行】，
//   因為它前面就拋出例外了。所以 get() 走的是重拋路徑，
//   int value 這個變數從頭到尾沒有被賦值過 —— 這正是為什麼
//   「結果: ...」那行不會被印出來。
//
// 【3. launch policy：async vs deferred，一個常被忽略的坑】
//   * std::launch::async    —— 保證【立刻在新執行緒上】執行。
//   * std::launch::deferred —— 【延後】到第一次 get()/wait() 時才在
//                              呼叫端執行，根本不開新執行緒（惰性求值）。
//   * 【不指定 policy】（單參數版 std::async(f)）—— 等同 async|deferred，
//     由實作自行選擇。這意味著它【可能根本不平行執行】！
//   若你要的是真正的並行，就必須明確寫出 std::launch::async，
//   像本檔這樣。不寫的話，程式在某些實作/負載下會悄悄退化成序列執行，
//   而且不會有任何警告。這是 std::async 最惡名昭彰的陷阱。
//
// 【4. std::async 回傳的 future 有個特殊解構行為】
//   一般的 future 解構時不阻塞。但【由 std::async 產生】的 future 例外：
//   它的解構子會【阻塞等待任務完成】（等同隱含 join）。
//   所以下面這行是常見的意外序列化來源：
//       std::async(std::launch::async, task1);   // 臨時 future 立刻解構 → 阻塞！
//       std::async(std::launch::async, task2);   // task1 做完才會開始
//   兩個任務看似平行，實際完全序列。
//   正確做法是把 future 存進具名變數，讓它們的生命週期延續到需要的時候。
//   本檔的 demoParallelWithFutures() 會實測展示這個差異。
//
// 【5. get() 只能呼叫一次】
//   get() 會把結果【搬移】出共享狀態，之後 future 變成 invalid（valid() 為 false）。
//   再次呼叫 get() 會丟出 std::future_error（錯誤碼 no_state）。
//   需要多次讀取或多方共享請改用 std::shared_future。
//
// 【概念補充 Concept Deep Dive】
//   * 共享狀態是什麼：一塊引用計數的堆積物件，內含
//     「值或 exception_ptr」、就緒旗標、以及供等待用的同步原語
//     （libstdc++ 用 mutex + condition_variable）。
//     future 與 promise 各持有一個把手。所以第 3 部分的 exception_ptr
//     並不是另一套機制 —— future 內部用的正是它。
//
//   * async 與 get 之間的同步保證：標準規定任務的完成
//     synchronizes-with get() 的返回，建立 happens-before 關係。
//     所以任務中寫入的其他資料，在 get() 之後從呼叫端讀取都是安全的，
//     不需要額外加鎖。
//
//   * std::promise 的定位：async 是「我給你一個函式，你去跑」；
//     promise 是「我自己決定何時、在哪裡設定結果」。
//     promise 有 set_value() 與 set_exception()，適合結果不是由單一函式
//     算出來的場景（例如非同步 I/O 回呼、事件驅動架構）。
//     兩者底層是同一個共享狀態機制。
//
//   * packaged_task：把 callable 包成「可呼叫且會把結果寫進 future」的物件，
//     常用於自己實作執行緒池 —— 池子存 packaged_task，
//     呼叫端拿它的 future。這是 async 與 promise 之間的中間層。
//
// 【注意事項 Pay Attention】
//   1. 【務必明確指定 std::launch::async】。不指定時實作可以選擇 deferred，
//      程式會悄悄退化成序列執行且無任何警告。
//   2. std::async 產生的 future，其【解構子會阻塞】等待任務完成。
//      不要寫成不接回傳值的裸呼叫，否則兩個任務會被意外序列化。
//   3. get() 只能呼叫一次，之後 future 失效。要多次讀取請用 std::shared_future。
//      對 invalid 的 future 呼叫 get() 會丟 std::future_error。
//   4. 若 future 從未呼叫 get()，任務中拋出的例外會被【安靜丟棄】——
//      跟第 4 部分「忘記 join」的後果一樣。
//   5. 例外在 get() 處重拋時，是在【呼叫 get() 的那條執行緒】上拋出的，
//      所以 try/catch 要包住 get()，不是包住 async。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::async 與 std::future 的例外處理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 非同步任務中拋出的例外，什麼時候、在哪一條執行緒上被重新拋出？
//     答：在呼叫 future::get() 的【那一刻】、於【呼叫 get() 的那條執行緒】上
//         重新拋出。任務拋出的例外會先被裝進共享狀態（內部用 exception_ptr），
//         get() 發現存的是例外就 rethrow_exception。
//         所以 try/catch 必須包住 get()，包住 async 是沒用的。
//     追問：如果從來沒呼叫 get() 呢？
//         → 例外會被安靜丟棄，錯誤完全消失。這跟「忘記 join」的後果一樣，
//           是實務上真實會發生的漏報來源。
//
// 🔥 Q2. std::async 不指定 launch policy 會有什麼問題？
//     答：不指定時等同 async|deferred，實作【可以選擇 deferred】——
//         也就是不開新執行緒，延後到第一次 get()/wait() 時才在呼叫端執行。
//         結果是你以為在平行，實際上完全序列，而且沒有任何警告。
//         要保證真正並行，必須明確寫 std::async(std::launch::async, f)。
//     追問：deferred 有什麼實際用途嗎？
//         → 有。它是惰性求值：結果可能根本用不到時，就不必浪費資源去算。
//           但這應該是【刻意選擇】，不該是預設行為下的意外。
//
// ⚠️ 陷阱. 這兩行為什麼不是平行執行的？
//         std::async(std::launch::async, task1);
//         std::async(std::launch::async, task2);
//     答：因為 std::async 回傳的 future 沒有被接住，是個【臨時物件】，
//         在該敘述結尾就立刻解構。而 async 產生的 future 其解構子
//         【會阻塞等待任務完成】（這是 async 專有的特殊規定）。
//         所以第一行等 task1 跑完才會執行到第二行，兩者完全序列。
//     為什麼會錯：以為 future 解構跟其他 RAII 型別一樣不阻塞，
//         或根本沒意識到臨時物件會立刻解構。
//         修法是把 future 接進具名變數：
//           auto f1 = std::async(std::launch::async, task1);
//           auto f2 = std::async(std::launch::async, task2);
//         這樣兩者才真正同時跑，直到 f1/f2 離開作用域才各自等待。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   LeetCode 並行題（1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded）
//   要求的是「多條執行緒之間的細粒度順序協調」，需要 mutex /
//   condition_variable / semaphore 這類可反覆等待與喚醒的原語。
//   future 是【一次性】的結果傳遞通道（get 只能呼叫一次），
//   無法表達「你印完換我印、我印完再換你」的往復同步，
//   用它去解那些題目會非常彆扭。故本檔從缺。
//   （future 真正的戰場是「把工作分出去、等結果回來」，見下方實務範例。）

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【基本示範】非同步任務拋出例外，在 get() 處重新拋出
// -----------------------------------------------------------------------------
int worker() {
    throw std::runtime_error("非同步任務的錯誤！");
    return 42;   // ← 永遠不會執行到；get() 走的是重拋路徑
}

// -----------------------------------------------------------------------------
// 【對照示範】launch policy：async 真的開執行緒，deferred 不開
// -----------------------------------------------------------------------------
void demoLaunchPolicy() {
    auto reportThread = [](const char* label) {
        std::cout << "    [" << label << "] 任務執行於執行緒 "
                  << std::this_thread::get_id() << std::endl;
    };

    std::cout << "  main 執行緒 id = " << std::this_thread::get_id() << std::endl;

    auto fa = std::async(std::launch::async, reportThread, "async");
    fa.get();

    auto fd = std::async(std::launch::deferred, reportThread, "deferred");
    std::cout << "    （deferred 建立後尚未執行任何東西）" << std::endl;
    fd.get();   // ← 到這一刻才在 main 上執行

    std::cout << "  結論：async 的 id 與 main 不同、deferred 的 id 與 main 相同"
              << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】臨時 future 會被意外序列化（async 專有的阻塞解構）
// -----------------------------------------------------------------------------
void demoParallelWithFutures() {
    auto task = [](int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); };

    auto t0 = std::chrono::steady_clock::now();
    // 刻意示範錯誤寫法：不接回傳值 → 臨時 future 立刻解構 → 阻塞等待
    // 註：libstdc++ 把 std::async 標成 [[nodiscard]]，正是因為這個陷阱太常見。
    //     這裡用 static_cast<void> 明確表達「我知道我在丟棄它」以壓下警告；
    //     在真實程式中看到這個警告，應該視為 bug 而不是雜訊。
    static_cast<void>(std::async(std::launch::async, task, 100));
    static_cast<void>(std::async(std::launch::async, task, 100));
    auto serialMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t0).count();

    t0 = std::chrono::steady_clock::now();
    {
        auto f1 = std::async(std::launch::async, task, 100);   // 接進具名變數
        auto f2 = std::async(std::launch::async, task, 100);
    }   // 離開作用域才各自等待
    auto parallelMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - t0).count();

    std::cout << "  不接回傳值（臨時 future）：" << serialMs
              << " ms ← 兩個 100ms 任務被序列化了" << std::endl;
    std::cout << "  接進具名變數：            " << parallelMs
              << " ms ← 真正平行" << std::endl;
    std::cout << "  （實際毫秒數每次執行都不同，但兩者的倍數關係穩定）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】get() 只能呼叫一次
// -----------------------------------------------------------------------------
void demoGetOnce() {
    auto f = std::async(std::launch::async, []() { return 7; });
    std::cout << "  第一次 get() = " << f.get() << std::endl;
    std::cout << "  get() 之後 valid() = " << std::boolalpha << f.valid() << std::endl;
    try {
        f.get();   // 對 invalid 的 future 呼叫 get()
    } catch (const std::future_error& e) {
        std::cout << "  第二次 get() 丟出 std::future_error: " << e.what()
                  << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】平行呼叫多個下游服務並匯總（scatter-gather）
//   情境：電商的商品頁需要同時向三個後端取資料 —— 庫存服務、定價服務、
//         評論服務。三者互相獨立，序列呼叫會讓延遲累加（300ms），
//         平行呼叫則約等於最慢的那一個（120ms）。
//         但任一服務可能失敗（逾時、5xx），必須讓呼叫端明確知道是哪一個掛了。
//   為何用 future：這是 future 最典型的戰場 ——
//     * 每個下游呼叫【有回傳值】，用 exception_ptr 還要另外傳值，很繁瑣；
//     * 失敗要帶著型別完整的例外回來，future 的 get() 自動處理；
//     * 值與例外用【同一個 get()】取回，錯誤處理集中在一處。
//   關鍵寫法：先把三個 future 全部建立（三者同時開跑），
//         再逐一 get()。若寫成「建一個 get 一個」就退化成序列了。
// -----------------------------------------------------------------------------
struct ServiceError : std::runtime_error {
    std::string service;
    ServiceError(std::string s, const std::string& msg)
        : std::runtime_error(msg), service(std::move(s)) {}
};

int fetchStock(int itemId) {
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    return 42 + itemId;
}

double fetchPrice(int itemId) {
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    return 199.5 + itemId;
}

int fetchReviewCount(int itemId, bool healthy) {
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    if (!healthy) {
        throw ServiceError("review", "review-service 回應 503 Service Unavailable");
    }
    return 1024 + itemId;
}

void loadProductPage(int itemId, bool reviewHealthy) {
    auto t0 = std::chrono::steady_clock::now();

    // 三個 future 全部先建立 → 三個下游服務【同時】開跑
    auto fStock = std::async(std::launch::async, fetchStock, itemId);
    auto fPrice = std::async(std::launch::async, fetchPrice, itemId);
    auto fReview = std::async(std::launch::async, fetchReviewCount, itemId,
                              reviewHealthy);

    try {
        // 再逐一 get()：值與例外都從這裡出來
        int stock = fStock.get();
        double price = fPrice.get();
        int reviews = fReview.get();

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        std::cout << "  ✓ 商品 " << itemId << " 載入成功：庫存=" << stock
                  << " 價格=" << price << " 評論數=" << reviews << std::endl;
        std::cout << "    耗時約 " << ms
                  << " ms（≈ 最慢的下游 120ms，不是三者相加的 260ms）" << std::endl;
    } catch (const ServiceError& e) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();
        std::cout << "  ✗ 商品 " << itemId << " 載入失敗" << std::endl;
        std::cout << "    故障服務: " << e.service << std::endl;
        std::cout << "    原因: " << e.what() << std::endl;
        std::cout << "    （耗時約 " << ms << " ms；其餘 future 解構時會等它們結束）"
                  << std::endl;
    }
}

int main() {
    std::cout << "=== 基本示範：例外在 get() 處重新拋出 ===" << std::endl;
    {
        std::future<int> result = std::async(std::launch::async, worker);

        try {
            int value = result.get();  // 例外在這裡被重新拋出
            std::cout << "結果: " << value << std::endl;   // 不會執行到
        } catch (const std::exception& e) {
            std::cout << "捕獲: " << e.what() << std::endl;
        }
    }

    std::cout << "\n=== launch policy：async vs deferred ===" << std::endl;
    demoLaunchPolicy();

    std::cout << "\n=== 陷阱：臨時 future 造成的意外序列化 ===" << std::endl;
    demoParallelWithFutures();

    std::cout << "\n=== get() 只能呼叫一次 ===" << std::endl;
    demoGetOnce();

    std::cout << "\n=== 日常實務：平行呼叫下游服務（成功情境） ===" << std::endl;
    loadProductPage(7, true);

    std::cout << "\n=== 日常實務：平行呼叫下游服務（評論服務故障） ===" << std::endl;
    loadProductPage(8, false);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理5.cpp" -o except5
//   本檔只用到 C++11 起就有的 <future> 設施，以 -std=c++17 -pedantic-errors 驗證通過。

// 註:
//   ⚠️ 執行緒 id 為實作定義的數值、耗時毫秒數受排程影響，【每次執行都不同】。
//   結構性的結論（async 的 id 與 main 不同、deferred 相同；
//   臨時 future 約 2 倍慢；平行版約等於最慢的下游）則是穩定的。

//   （實際毫秒數每次執行都不同，但兩者的倍數關係穩定）

// === 預期輸出 ===
// === 基本示範：例外在 get() 處重新拋出 ===
// 捕獲: 非同步任務的錯誤！
//
// === launch policy：async vs deferred ===
//   main 執行緒 id = 139709243017152
//     [async] 任務執行於執行緒 139709235852992
//     （deferred 建立後尚未執行任何東西）
//     [deferred] 任務執行於執行緒 139709243017152
//   結論：async 的 id 與 main 不同、deferred 的 id 與 main 相同
//
// === 陷阱：臨時 future 造成的意外序列化 ===
//   不接回傳值（臨時 future）：200 ms ← 兩個 100ms 任務被序列化了
//   接進具名變數：            100 ms ← 真正平行
//
// === get() 只能呼叫一次 ===
//   第一次 get() = 7
//   get() 之後 valid() = false
//   第二次 get() 丟出 std::future_error: std::future_error: No associated state
//
// === 日常實務：平行呼叫下游服務（成功情境） ===
//   ✓ 商品 7 載入成功：庫存=49 價格=206.5 評論數=1031
//     耗時約 120 ms（≈ 最慢的下游 120ms，不是三者相加的 260ms）
//
// === 日常實務：平行呼叫下游服務（評論服務故障） ===
//   ✗ 商品 8 載入失敗
//     故障服務: review
//     原因: review-service 回應 503 Service Unavailable
//     （耗時約 120 ms；其餘 future 解構時會等它們結束）
