// =============================================================================
//  課程 3.4：執行緒例外處理 6  —  std::promise::set_exception 跨執行緒傳遞例外
// =============================================================================
//
// 【主題資訊 Information】
//   void promise<T>::set_value(const T&);
//   void promise<T>::set_exception(std::exception_ptr p);
//   std::future<T> promise<T>::get_future();          // 一個 promise 只能取一次
//   T future<T>::get();                                // 取值,或【重新拋出】例外
//   std::exception_ptr std::current_exception() noexcept;
//
//   標準版本：C++11
//   標頭檔  ：<future>(promise / future)、<exception>(exception_ptr)
//   複雜度  ：set_value / set_exception 為 O(1);get() 阻塞至共享狀態就緒
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼例外不能「自己」跨執行緒傳播】
//   C++ 的例外機制是【以執行緒為單位】的。每條執行緒有自己的 stack,
//   例外沿著自己的 stack 往上 unwinding,到頂還沒被 catch 就結束了 ——
//   而執行緒函式的「頂」就是它的進入點,沒有更上層可以傳。
//
//   標準規定([thread.thread.constr]):若例外從執行緒的進入函式逸出,
//   會呼叫 std::terminate()。所以下面這段是【行不通】的:
//       try {
//           std::thread t([]{ throw std::runtime_error("x"); });
//           t.join();
//       } catch (...) { }        // ← 永遠攔不到,程式在子執行緒裡就終止了
//
//   要跨執行緒傳例外,必須有人【顯式地把例外物件搬過去】。
//   promise/future 就是標準提供的那條管道。
//
// 【2. exception_ptr:把例外「裝箱」的型別抹除容器】
//   std::current_exception() 在 catch 區塊中呼叫,會回傳一個
//   std::exception_ptr —— 它是【型別抹除】的智慧指標,不論當初丟的是
//   std::runtime_error、int、還是自訂型別,都能原封不動地存起來。
//
//   關鍵性質:
//     * 它會【延長例外物件的生命週期】(通常是引用計數的複本或參考),
//       所以 catch 區塊結束、例外物件本該銷毀之後,它依然有效。
//     * 可以自由複製、儲存進容器、跨執行緒傳遞。
//     * 用 std::rethrow_exception(p) 就能在【任何執行緒】重新拋出,
//       而且拋出的是【原本的型別】,不是被降級成基底類別。
//   這一點很重要:catch (const std::exception&) 接住後再 throw; 也能保型別,
//   但 exception_ptr 讓你能把它【存起來晚點再拋】,這是普通 rethrow 做不到的。
//
// 【3. 本檔的流程】
//       主執行緒            子執行緒
//       --------            --------
//       建立 promise
//       get_future()  ───►  (promise 被 move 進去)
//                           try { 工作 }
//                           catch → current_exception()
//                                 → prom.set_exception(p)
//       fut.get()     ◄───  共享狀態變成「就緒(含例外)」
//         └─ 偵測到存的是例外 → rethrow_exception → 在主執行緒重新拋出
//       catch 攔到原始型別的例外
//
//   注意 get() 的行為:若共享狀態存的是【值】就回傳值;存的是【例外】
//   就在呼叫端重新拋出。所以呼叫端只要照常寫 try/catch 即可,
//   完全不需要判斷「這次是成功還是失敗」。這是很優雅的設計。
//
// 【4. promise 必須被 move,不能被複製】
//       std::thread t(worker, std::move(prom));
//   std::promise 是 move-only 型別 —— 它持有共享狀態的「寫入端」所有權,
//   複製會產生「兩個人都能寫」的歧義。move 之後,主執行緒手上的 prom
//   處於 moved-from 狀態,不該再使用;寫入端的責任完全轉移給子執行緒。
//
//   ⚠️ 這裡有個 std::thread 的細節:std::thread 的建構函式會【複製或移動】
//   引數到內部儲存,再以右值傳給函式。所以 worker 的參數必須是
//   std::promise<int>(傳值),不能是 std::promise<int>&。
//
// 【5. ⚠️ 原始碼裡的無法到達程式碼(dead code)】
//       throw std::runtime_error("Worker 錯誤!");
//       prom.set_value(42);            // ← 永遠不會執行到
//   throw 之後的那行是無法到達的。這在教學範例裡是刻意的(模擬「工作
//   途中失敗」),但要意識到它是 dead code —— 有些編譯器在開啟
//   -Wunreachable-code 時會警告(GCC 目前預設不報)。
//   本檔保留這個結構以忠實呈現課文,但改寫成由參數決定是否失敗,
//   讓成功與失敗兩條路徑都能真的被執行到。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 什麼是 broken promise?
//   若 promise 被銷毀時【既沒 set_value 也沒 set_exception】,
//   共享狀態會被自動設為一個 std::future_error,錯誤碼是
//   std::future_errc::broken_promise。等在 future.get() 上的執行緒
//   會收到這個例外而不是永久卡死。
//   這是標準刻意設計的安全網:忘記回覆的 promise 不會造成死結。
//   本檔的示範 3 實際重現這個情況。
//
// (B) get() 只能呼叫一次
//   std::future::get() 會把共享狀態【移走】,呼叫後 future 變成無效
//   (valid() 回傳 false)。再呼叫一次會丟 std::future_error
//   (no_state)。若需要多次讀取或多方共享,要用 std::shared_future,
//   它的 get() 回傳 const 參考且可重複呼叫。
//
// (C) set_exception 之後再 set_value 會怎樣?
//   丟 std::future_error,錯誤碼 promise_already_satisfied。
//   共享狀態只能被設定一次 —— 這個不變式讓 future 端的語意保持單純。
//
// (D) 與「例外處理 3」的 exception_ptr 手動傳遞有何不同?
//   手動方式(自己宣告 exception_ptr 成員 + join 後檢查)需要自己管理
//   同步:誰寫、誰讀、什麼時候可以安全讀。promise/future 把這件事
//   包裝好了 —— 共享狀態內建同步,get() 會阻塞到就緒為止,
//   而且 set 與 get 之間有 happens-before 保證。
//   簡言之:自己做要處理同步,用 promise/future 則是現成的。
//
// 【注意事項 Pay Attention】
//   1. 例外從執行緒進入函式逸出 → std::terminate()。子執行緒內必須
//      自己 catch,再透過 promise 傳出來。
//   2. std::promise 是 move-only,傳給 std::thread 時必須 std::move。
//   3. get_future() 對同一個 promise 只能呼叫一次,第二次丟
//      std::future_error(future_already_retrieved)。
//   4. future::get() 只能呼叫一次;需要多方讀取請用 std::shared_future。
//   5. promise 未設定任何值就銷毀 → future 端收到 broken_promise,
//      不會永久阻塞。
//   6. current_exception() 必須在 catch 區塊(或其呼叫鏈)中呼叫;
//      不在例外處理中時回傳空的 exception_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】跨執行緒的例外傳遞
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 子執行緒裡拋出的例外,主執行緒能用 try/catch 包住 join() 接到嗎?
//     答：不能。例外機制以執行緒為單位,例外只會沿著自己的 stack unwinding。
//         標準規定:例外從執行緒進入函式逸出即呼叫 std::terminate()。
//         必須在子執行緒內 catch,用 std::current_exception() 裝箱成
//         exception_ptr,再透過 promise/future(或自己的成員 + join)
//         搬到主執行緒 rethrow。
//     追問：為什麼 exception_ptr 能保留原本的型別?
//         → 它是型別抹除的持有者,內部保存的是例外物件本身(含動態型別),
//           不是靜態型別的複本。rethrow_exception() 拋出的是原始型別,
//           所以主執行緒的 catch (const MyError&) 依然能精確匹配。
//
// 🔥 Q2. promise 建立後,如果 worker 忘記呼叫 set_value/set_exception 就結束了,
//        等在 future.get() 上的主執行緒會不會永遠卡住?
//     答：不會。promise 解構時若共享狀態尚未就緒,標準規定會自動設定成
//         std::future_error(future_errc::broken_promise)。
//         主執行緒的 get() 會拋出這個例外而不是永久阻塞。
//         這是標準刻意提供的安全網。
//     追問：那如果 promise 被移動走了呢?
//         → moved-from 的 promise 不再持有共享狀態,它解構時什麼也不做。
//           真正負責的是持有共享狀態的那一個(通常是子執行緒手上那份)。
//
// ⚠️ 陷阱 1. 「future.get() 拿到值之後,再 get() 一次應該還是同一個值吧?」
//     答：不是。std::future::get() 會把共享狀態【移走】,呼叫後 future
//         失效(valid() == false),第二次呼叫丟 std::future_error(no_state)。
//         要重複讀取或讓多個執行緒都拿到結果,必須用 std::shared_future
//         (它的 get() 回傳 const 參考,可重複呼叫)。
//     為什麼會錯：把 future 想成「一個存著結果的盒子」。它其實是
//         「一次性的取貨單」—— 設計上假設結果可能是 move-only 型別,
//         所以預設就是移動語意。
//
// ⚠️ 陷阱 2. 「我在 catch 外面呼叫 std::current_exception(),它應該會回傳
//              剛剛那個例外吧?」
//     答：不會。current_exception() 只在【例外處理進行中】才有意義,
//         不在 catch 區塊(或其呼叫鏈)裡呼叫時回傳空的 exception_ptr。
//         把空的 exception_ptr 傳給 set_exception 是未定義行為;
//         而對空的 exception_ptr 呼叫 rethrow_exception 同樣是未定義行為。
//     為什麼會錯：以為它是「取得最近一次例外」的全域查詢。實際上它讀的是
//         當前執行緒【正在處理中】的例外狀態,離開處理區就沒有了。
// ═══════════════════════════════════════════════════════════════════════════

#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 課文原型(改寫成成功/失敗兩條路徑都能真的執行到)
//
//   原始碼是:
//       throw std::runtime_error("Worker 錯誤!");
//       prom.set_value(42);          // ← dead code,永遠不會執行
//   保留其精神,但用參數控制,讓兩條路徑都能被示範。
// -----------------------------------------------------------------------------
void worker(std::promise<int> prom, bool shouldFail) {
    try {
        if (shouldFail) {
            throw std::runtime_error("Worker 錯誤!");
        }
        prom.set_value(42);
    } catch (...) {
        // 裝箱:把任意型別的例外原封不動存進共享狀態
        prom.set_exception(std::current_exception());
    }
}

// -----------------------------------------------------------------------------
// 示範 1:失敗路徑 —— 例外在 get() 處重新拋出
// -----------------------------------------------------------------------------
void demoFailure() {
    std::promise<int> prom;
    std::future<int>  fut = prom.get_future();

    std::thread t(worker, std::move(prom), true);  // promise 必須 move

    try {
        int value = fut.get();   // 共享狀態存的是例外 → 在此重新拋出
        std::cout << "  結果: " << value << "\n";
    } catch (const std::exception& e) {
        std::cout << "  捕獲: " << e.what() << "\n";
    }

    t.join();
}

// -----------------------------------------------------------------------------
// 示範 2:成功路徑 —— 同一段呼叫端程式碼,不需要判斷成功或失敗
// -----------------------------------------------------------------------------
void demoSuccess() {
    std::promise<int> prom;
    std::future<int>  fut = prom.get_future();

    std::thread t(worker, std::move(prom), false);

    try {
        int value = fut.get();
        std::cout << "  結果: " << value << "\n";
    } catch (const std::exception& e) {
        std::cout << "  捕獲: " << e.what() << "\n";
    }

    t.join();
}

// -----------------------------------------------------------------------------
// 示範 3:broken promise —— 忘記設定就銷毀,不會永久卡死
// -----------------------------------------------------------------------------
void demoBrokenPromise() {
    std::future<int> fut;

    {
        std::promise<int> prom;
        fut = prom.get_future();
        // 什麼都不做,直接讓 prom 解構
    }  // prom 解構 → 共享狀態自動設為 broken_promise

    try {
        int v = fut.get();
        std::cout << "  取得值: " << v << "\n";
    } catch (const std::future_error& e) {
        std::cout << "  捕獲 std::future_error: " << e.code().message() << "\n";
        std::cout << "  (未設定值就銷毀 promise → 不會永久阻塞,而是收到例外)\n";
    }
}

// -----------------------------------------------------------------------------
// 示範 4:get() 只能呼叫一次
// -----------------------------------------------------------------------------
void demoGetOnce() {
    std::promise<int> prom;
    std::future<int>  fut = prom.get_future();
    prom.set_value(7);

    std::cout << "  第一次 get() = " << fut.get() << "\n";
    std::cout << "  取值後 fut.valid() = " << std::boolalpha << fut.valid() << "\n";

    try {
        int again = fut.get();   // 共享狀態已被移走
        std::cout << "  第二次 get() = " << again << "\n";
    } catch (const std::future_error& e) {
        std::cout << "  第二次 get() 丟出 std::future_error: "
                  << e.code().message() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 示範 5:自訂例外型別 —— rethrow 後型別完整保留
// -----------------------------------------------------------------------------
class ConfigError : public std::runtime_error {
    int line_;
public:
    ConfigError(const std::string& msg, int line)
        : std::runtime_error(msg), line_(line) {}
    int line() const noexcept { return line_; }
};

void demoCustomExceptionType() {
    std::promise<void> prom;
    std::future<void>  fut = prom.get_future();

    std::thread t([p = std::move(prom)]() mutable {
        try {
            throw ConfigError("缺少必要欄位 listen_port", 42);
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });

    try {
        fut.get();
    } catch (const ConfigError& e) {   // ← 精確匹配到原始的衍生型別
        std::cout << "  捕獲 ConfigError: " << e.what()
                  << "(第 " << e.line() << " 行)\n";
        std::cout << "  型別完整保留,沒有被降級成 std::exception\n";
    }

    t.join();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)考的是【執行緒間的順序協調】,
//   題目保證每個函式都會正常返回,完全沒有錯誤路徑,也不需要把失敗
//   回報給呼叫端。本檔主題是【跨執行緒的錯誤傳遞】,與之無交集,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】平行拉取多個微服務設定,任一失敗都要帶原因回報
//
//   情境:服務啟動時要同時向 3 個設定中心拉取設定(逾時各不相同)。
//         若其中一個失敗(網路不通、格式錯誤),啟動流程必須中止,
//         而且要能告訴維運【是哪一個失敗、失敗原因是什麼】——
//         不能只回一個籠統的「啟動失敗」。
//
//   為何用 promise/future:
//     * 每個拉取任務一個 promise,主執行緒收集對應的 future。
//     * 成功就 set_value(設定內容),失敗就 set_exception(原始例外)。
//     * 主執行緒逐一 get():成功的拿到內容,失敗的會在 get() 處
//       重新拋出【原始型別】的例外,錯誤訊息與自訂欄位(這裡是行號)
//       完整保留,不會被壓成字串。
//   這比「回傳 bool + 全域 errno」乾淨得多,也比「子執行緒直接寫 log
//   然後 exit」好 —— 錯誤處理的決策權留在呼叫端。
// -----------------------------------------------------------------------------
struct FetchResult {
    std::string source;
    std::string payload;
    bool        ok;
    std::string error;
};

std::vector<FetchResult> fetchAllConfigs(const std::vector<std::string>& sources,
                                         const std::string& failingSource) {
    std::vector<std::promise<std::string>> promises(sources.size());
    std::vector<std::future<std::string>>  futures;
    std::vector<std::thread>               workers;

    futures.reserve(sources.size());
    workers.reserve(sources.size());

    for (auto& p : promises) futures.push_back(p.get_future());

    for (std::size_t i = 0; i < sources.size(); ++i) {
        workers.emplace_back(
            [p = std::move(promises[i]), src = sources[i], failingSource]() mutable {
                try {
                    if (src == failingSource) {
                        throw ConfigError("無法連線至 " + src, 0);
                    }
                    p.set_value("{\"service\":\"" + src + "\",\"ok\":true}");
                } catch (...) {
                    p.set_exception(std::current_exception());
                }
            });
    }

    std::vector<FetchResult> results;
    for (std::size_t i = 0; i < sources.size(); ++i) {
        try {
            results.push_back({sources[i], futures[i].get(), true, ""});
        } catch (const ConfigError& e) {
            // 原始型別完整保留,可取得自訂欄位
            results.push_back({sources[i], "", false, e.what()});
        }
    }

    for (auto& w : workers) w.join();
    return results;
}

int main() {
    std::cout << "=== 示範 1:失敗路徑(例外在 get() 重新拋出)===\n";
    demoFailure();

    std::cout << "\n=== 示範 2:成功路徑(呼叫端寫法完全相同)===\n";
    demoSuccess();

    std::cout << "\n=== 示範 3:broken promise ===\n";
    demoBrokenPromise();

    std::cout << "\n=== 示範 4:get() 只能呼叫一次 ===\n";
    demoGetOnce();

    std::cout << "\n=== 示範 5:自訂例外型別完整保留 ===\n";
    demoCustomExceptionType();

    std::cout << "\n=== 日常實務:平行拉取多個微服務設定 ===\n";
    std::vector<std::string> sources{"config-a", "config-b", "config-c"};
    auto results = fetchAllConfigs(sources, "config-b");

    int okCount = 0;
    for (const auto& r : results) {
        if (r.ok) {
            ++okCount;
            std::cout << "  [OK]   " << r.source << " → " << r.payload << "\n";
        } else {
            std::cout << "  [FAIL] " << r.source << " → " << r.error << "\n";
        }
    }
    std::cout << "  成功 " << okCount << " / " << results.size()
              << ",失敗原因已完整帶回呼叫端\n";

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理6.cpp" -o exc6

// 註:本檔輸出是【確定的】—— 每一段都在 get()/join() 之後才繼續,
//     而 get() 與 set_value/set_exception 之間有 happens-before 保證,
//     且所有輸出都由主執行緒產生,不會有交錯問題。

// === 預期輸出 ===
// === 示範 1:失敗路徑(例外在 get() 重新拋出)===
//   捕獲: Worker 錯誤!
//
// === 示範 2:成功路徑(呼叫端寫法完全相同)===
//   結果: 42
//
// === 示範 3:broken promise ===
//   捕獲 std::future_error: Broken promise
//   (未設定值就銷毀 promise → 不會永久阻塞,而是收到例外)
//
// === 示範 4:get() 只能呼叫一次 ===
//   第一次 get() = 7
//   取值後 fut.valid() = false
//   第二次 get() 丟出 std::future_error: No associated state
//
// === 示範 5:自訂例外型別完整保留 ===
//   捕獲 ConfigError: 缺少必要欄位 listen_port(第 42 行)
//   型別完整保留,沒有被降級成 std::exception
//
// === 日常實務:平行拉取多個微服務設定 ===
//   [OK]   config-a → {"service":"config-a","ok":true}
//   [FAIL] config-b → 無法連線至 config-b
//   [OK]   config-c → {"service":"config-c","ok":true}
//   成功 2 / 3,失敗原因已完整帶回呼叫端
//
// === 全部示範結束 ===
