// =============================================================================
//  課程 3.4：執行緒例外處理 — 第 2 部分：在執行緒內部就地捕獲
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>、<stdexcept>、<exception>
//   標準版本：C++11（std::thread 與本檔用到的例外機制）
//
//   核心規則（[thread.thread.constr]）：
//     若執行緒的進入點函式以【未捕獲的例外】結束，
//     實作會呼叫 std::terminate()。
//     這是標準【保證】的行為，不是未定義行為。
//
//   複雜度：try/catch 區塊在「沒有拋出例外」時的執行期成本為零
//           （zero-cost exception model，見下方概念補充）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼例外不能跨執行緒自動傳播】
//   例外傳播的機制是 stack unwinding —— 沿著【呼叫堆疊】往回找 handler。
//   但每條執行緒有自己【獨立的】堆疊，兩者之間沒有呼叫關係：
//     * main 呼叫 t.join() 只是「等待」，不是「呼叫」worker；
//     * worker 的堆疊底部就是執行緒進入點，往下沒有 main 的 frame。
//   所以 worker 拋出的例外往外找 handler 時，找到堆疊底部就沒路了。
//   標準規定這種情況呼叫 std::terminate()，程式立刻中止。
//
//   這不是「設計缺陷」，而是物理限制：例外要傳給另一條執行緒，
//   必須有人把例外物件【複製或搬運】過去，這是 std::exception_ptr
//   （見第 3 部分）與 std::future（見第 5 部分）在做的事。
//
// 【2. 就地捕獲：最簡單、也最常被低估的做法】
//   本檔示範的模式是：在執行緒進入點函式裡把整段工作包在 try/catch 中，
//   例外就地處理掉，絕不讓它逃出執行緒邊界。
//     優點：最簡單、零額外機制、不需要任何跨執行緒同步。
//     缺點：呼叫端【完全感知不到】出過錯。join() 正常返回，
//           程式繼續跑，錯誤只留在 log 裡。
//   所以它適用於「這個錯誤本來就只需要記錄、不需要讓上層改變行為」的場景，
//   例如背景的統計上報、快取預熱、非關鍵的清理工作。
//   若上層需要知道成敗，就必須改用 exception_ptr 或 future。
//
// 【3. catch 該寫在哪一層】
//   一個常見錯誤是把 try/catch 寫在 std::thread 的【建構處】：
//       try { std::thread t(worker); t.join(); }
//       catch (...) { }   // ✗ 攔不到 worker 內部拋的例外
//   這只能攔到「建立執行緒失敗」（std::system_error）與 join 本身的錯誤，
//   攔不到 worker 內部的例外 —— 那個例外在另一條堆疊上，早就 terminate 了。
//   catch 必須寫在【執行緒進入點函式的內部】，也就是本檔 worker() 的寫法。
//
// 【4. 一定要用 catch (...) 兜底】
//   就地捕獲的目的是「保證例外不逃出執行緒」。
//   只寫 catch (const std::exception&) 是不夠的 ——
//   C++ 允許拋出任何型別（int、const char*、自訂非衍生類別），
//   第三方函式庫也可能拋出不繼承 std::exception 的東西。
//   漏掉任何一種就會 terminate。所以完整的防護一定要有 catch (...) 收尾。
//
// 【概念補充 Concept Deep Dive】
//   * zero-cost exception model：現代 GCC/Clang 在 x86-64 上採用
//     table-driven 的例外實作。try 區塊【不會】產生任何執行期指令，
//     成本完全轉移到「真的拋出時」——那時才去查 .eh_frame 表決定怎麼 unwind。
//     所以「為了效能不敢用 try/catch」在正常路徑上是沒有根據的；
//     真正昂貴的是 throw 本身（要配置例外物件、查表、逐層 unwind），
//     大約是數微秒等級，不該用在正常控制流上。
//
//   * std::terminate() 的實際行為：呼叫目前的 terminate_handler，
//     預設是 std::abort() → 送出 SIGABRT → shell 看到 exit code 134
//     （128 + 6）。這條路徑【不經過】例外機制，所以任何 catch 都攔不到，
//     而且解構子也不會執行 —— 這是它比「乾淨結束」危險得多的地方。
//
//   * 為什麼 std::cout 在多執行緒下不會壞掉：C++11 起
//     [iostream.objects] 要求標準串流物件的並行使用不造成 data race。
//     但它【不保證原子性】，多條執行緒同時輸出仍會交錯。
//     本檔只有一條 worker，所以輸出是穩定的。
//
// 【注意事項 Pay Attention】
//   1. 例外【不會】自動跨執行緒傳播。讓它逃出執行緒進入點 = std::terminate()。
//      這是標準保證的中止，不是未定義行為，也不是「可能會當機」。
//   2. try/catch 必須寫在執行緒進入點【函式內部】，寫在建立 thread 的地方無效。
//   3. 一定要有 catch (...) 兜底，只 catch std::exception 會漏掉非標準例外型別。
//   4. 就地捕獲後呼叫端無從得知失敗。需要回報成敗請用第 3 部分的
//      exception_ptr 或第 5 部分的 std::future。
//   5. 解構子中拋出例外同樣會 terminate（解構子預設 noexcept），
//      這在執行緒清理路徑上特別容易踩到。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒內的例外處理
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 執行緒函式拋出例外但沒有捕獲，會發生什麼事？主執行緒能 catch 到嗎？
//     答：呼叫 std::terminate()，程式立刻中止。主執行緒【絕對攔不到】，
//         因為每條執行緒有獨立的堆疊，兩者之間沒有呼叫關係，
//         stack unwinding 找不到跨執行緒的 handler。
//         這是標準保證的行為，不是未定義行為。
//     追問：那要怎麼把例外傳回主執行緒？
//         → 三種標準做法：(a) std::exception_ptr + current_exception()
//           + rethrow_exception()；(b) std::promise::set_exception()；
//           (c) 最簡潔的 std::async/std::future —— 例外會自動存進共享狀態，
//           在呼叫 future::get() 時於呼叫端重新拋出。
//
// 🔥 Q2. 為什麼就地捕獲一定要寫 catch (...)，只寫 catch (const std::exception&)
//        不夠嗎？
//     答：不夠。C++ 允許拋出任意型別 —— int、const char*、或第三方函式庫
//         自訂的、不繼承 std::exception 的類別。任何一個漏掉都會逃出執行緒
//         進入點並觸發 terminate()。就地捕獲的目的是「保證不逃逸」，
//         所以必須有 catch (...) 兜底。
//     追問：catch (...) 裡拿不到例外資訊怎麼辦？
//         → 用 std::current_exception() 取得 exception_ptr 存起來，
//           之後再 rethrow 並分型別處理；或在 catch (...) 內部再用
//           try { throw; } catch (const std::exception& e) { ... } 的
//           重新拋出技巧來分流。
//
// ⚠️ 陷阱. 這樣寫為什麼攔不到 worker 內的例外？
//         try { std::thread t(worker); t.join(); } catch (...) { /* 處理 */ }
//     答：因為 worker 是在【另一條堆疊】上執行的，它的例外往外傳時
//         根本走不到 main 的這個 try 區塊。這個 catch 只能攔到
//         「建立執行緒失敗」（std::system_error）或 join() 自身的錯誤。
//         worker 內部的例外在它自己的執行緒上就已經 terminate 了 ——
//         程式在 join() 返回前就死了，這個 catch 永遠不會被執行。
//     為什麼會錯：把 t.join() 誤當成「呼叫 worker」。join 只是【等待】，
//         不是呼叫；main 的堆疊上從來沒有 worker 的 frame。
//         把並行執行緒想成一般函式呼叫，是這個錯誤的根源。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔主題是「例外邊界與執行緒生命週期」，屬於 C++ 錯誤處理機制。
//   LeetCode 並行題（1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded）
//   全部聚焦於同步原語的順序協調，題目框架也不會讓解法拋出例外，
//   兩者沒有真實交集。硬套一題只會模糊焦點，故從缺。

#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【基本示範】在執行緒內部就地捕獲例外
// -----------------------------------------------------------------------------
void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (const std::exception& e) {
        std::cout << "執行緒內捕獲: " << e.what() << std::endl;
    }
}

// -----------------------------------------------------------------------------
// 【對照示範】catch (...) 為什麼不可少：非 std::exception 型別
//   C++ 允許拋出任意型別。只 catch std::exception 會漏掉這些，
//   漏掉就會逃出執行緒進入點 → std::terminate()。
// -----------------------------------------------------------------------------
void demoCatchAll() {
    auto safeRun = [](const char* label, auto&& body) {
        std::thread t([label, body]() {
            try {
                body();
            } catch (const std::exception& e) {
                std::cout << "  [" << label << "] 被 catch(std::exception) 接住: "
                          << e.what() << std::endl;
            } catch (...) {
                // ← 兜底：沒有這一層，下面兩種例外都會讓程式 terminate
                std::cout << "  [" << label
                          << "] 被 catch(...) 接住（非 std::exception 型別）"
                          << std::endl;
            }
        });
        t.join();
    };

    safeRun("標準例外", [] { throw std::runtime_error("我繼承 std::exception"); });
    safeRun("拋出 int", [] { throw 42; });
    safeRun("拋出字串", [] { throw "我是 const char*"; });
}

// -----------------------------------------------------------------------------
// 【對照示範】catch (...) 內仍可分辨型別：用「重新拋出」技巧
// -----------------------------------------------------------------------------
void demoRethrowDispatch() {
    std::thread t([]() {
        try {
            throw std::out_of_range("索引 7 超出範圍");
        } catch (...) {
            // 在 catch(...) 內部用裸 throw; 把目前例外再拋一次，就能分型別接
            try {
                throw;
            } catch (const std::out_of_range& e) {
                std::cout << "  分流為 out_of_range: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "  分流為一般 exception: " << e.what() << std::endl;
            } catch (...) {
                std::cout << "  分流為未知型別" << std::endl;
            }
        }
    });
    t.join();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】背景指標上報（metrics reporter）—— 失敗只記錄、不影響主流程
//   情境：服務每隔一段時間把 QPS、延遲等指標推送到監控後端（Prometheus /
//         Datadog）。上報失敗是【可容忍】的：網路抖動、後端暫時不可用都很常見，
//         絕不能因為指標推不上去就讓整個服務掛掉。
//   為何用「就地捕獲」：這正是就地捕獲最合適的場景 ——
//         呼叫端不需要、也不應該對上報失敗做出反應，只要記錄下來即可。
//         若這裡改用 future 把例外傳回主執行緒，主流程反而被迫處理
//         一個它根本不在乎的錯誤，是過度設計。
//   反例對照：如果是「寫入使用者訂單」這種【不可丟失】的操作，
//         就必須用 exception_ptr 或 future 把失敗回報給呼叫端。
// -----------------------------------------------------------------------------
class MetricsReporter {
public:
    void reportLoop(int rounds) {
        for (int i = 1; i <= rounds; ++i) {
            try {
                pushOnce(i);
                ++succeeded_;
                std::cout << "  [metrics] 第 " << i << " 次上報成功" << std::endl;
            } catch (const std::exception& e) {
                // 就地吞掉：記錄後繼續，絕不讓例外逃出執行緒
                ++failed_;
                std::cout << "  [metrics] 第 " << i << " 次上報失敗（已記錄，繼續）: "
                          << e.what() << std::endl;
            } catch (...) {
                ++failed_;
                std::cout << "  [metrics] 第 " << i
                          << " 次上報失敗（未知例外型別，已記錄，繼續）" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    int succeeded() const { return succeeded_; }
    int failed() const { return failed_; }

private:
    // 模擬推送：第 2 次遇到網路錯誤、第 4 次遇到非標準例外型別
    void pushOnce(int round) {
        if (round == 2) throw std::runtime_error("connection refused (後端暫時不可用)");
        if (round == 4) throw std::string("第三方函式庫拋出的非標準型別");
    }

    int succeeded_ = 0;
    int failed_ = 0;
};

int main() {
    std::cout << "=== 基本示範：執行緒內就地捕獲 ===" << std::endl;
    {
        std::thread t(worker);
        t.join();
    }
    std::cout << "程式正常結束" << std::endl;

    std::cout << "\n=== catch (...) 為什麼不可少 ===" << std::endl;
    demoCatchAll();

    std::cout << "\n=== catch (...) 內用重新拋出分流型別 ===" << std::endl;
    demoRethrowDispatch();

    std::cout << "\n=== 日常實務：背景指標上報（失敗只記錄） ===" << std::endl;
    {
        MetricsReporter reporter;
        std::thread t(&MetricsReporter::reportLoop, &reporter, 5);
        t.join();
        std::cout << "  [main] 上報結束：成功 " << reporter.succeeded()
                  << " 次、失敗 " << reporter.failed() << " 次" << std::endl;
        std::cout << "  [main] 主流程完全不受影響，服務繼續運作" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.4：執行緒例外處理2.cpp" -o except2
//   本檔只用到 C++11 起就有的功能，以 -std=c++17 -pedantic-errors 驗證通過。

// 註:
//   （本檔輸出是【確定的】：每一段都只有單一執行緒在輸出，
//   且 join() 保證段與段之間有明確順序，因此沒有交錯問題。）

// === 預期輸出 ===
// === 基本示範：執行緒內就地捕獲 ===
// 執行緒內捕獲: 執行緒內的錯誤！
// 程式正常結束
//
// === catch (...) 為什麼不可少 ===
//   [標準例外] 被 catch(std::exception) 接住: 我繼承 std::exception
//   [拋出 int] 被 catch(...) 接住（非 std::exception 型別）
//   [拋出字串] 被 catch(...) 接住（非 std::exception 型別）
//
// === catch (...) 內用重新拋出分流型別 ===
//   分流為 out_of_range: 索引 7 超出範圍
//
// === 日常實務：背景指標上報（失敗只記錄） ===
//   [metrics] 第 1 次上報成功
//   [metrics] 第 2 次上報失敗（已記錄，繼續）: connection refused (後端暫時不可用)
//   [metrics] 第 3 次上報成功
//   [metrics] 第 4 次上報失敗（未知例外型別，已記錄，繼續）
//   [metrics] 第 5 次上報成功
//   [main] 上報結束：成功 3 次、失敗 2 次
//   [main] 主流程完全不受影響，服務繼續運作
