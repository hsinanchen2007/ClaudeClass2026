// =============================================================================
//  課程 3.1：RAII 與執行緒管理 2  —  ThreadGuard：持有「參考」的執行緒守衛
// =============================================================================
//
// 【主題資訊 Information】
//   class ThreadGuard {
//       std::thread& t;                              // 持有參考,不擁有所有權
//   public:
//       explicit ThreadGuard(std::thread& thread);
//       ~ThreadGuard();                              // joinable() → join()
//       ThreadGuard(const ThreadGuard&)            = delete;
//       ThreadGuard& operator=(const ThreadGuard&) = delete;
//   };
//
//   標準版本：C++11(std::thread 起)
//   標頭檔  ：<thread>
//   複雜度  ：解構時的 join() 會阻塞到目標執行緒結束,時間取決於工作本身
//   別名    ：《C++ Concurrency in Action》書中稱此模式為 thread_guard
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::thread 需要一個守衛?】
//   std::thread 的解構函式規則非常嚴格,而且對初學者非常不友善:
//
//       ~thread() { if (joinable()) std::terminate(); }
//
//   請注意這句話的份量 —— 這不是「未定義行為」,不是「可能崩潰」,而是
//   標準明文規定([thread.thread.destr])的 std::terminate()。一個 joinable
//   的 std::thread 被解構,程式必定終止。這是標準保證的確定結果,
//   跟 data race 那種「什麼都可能發生」的 UB 完全是兩回事。
//
//   為什麼標準要設計得這麼激烈?因為另外兩個選項都更糟:
//     (a) 解構時自動 join()  → 解構函式會無預警阻塞,可能永遠不返回。
//                              解構函式阻塞是災難:它可能發生在 stack
//                              unwinding 期間,無法取消,也無法回報錯誤。
//     (b) 解構時自動 detach() → 執行緒繼續存取已被銷毀的區域變數,
//                              變成 dangling reference,靜默的記憶體毀損。
//   標準委員會的判斷是:「立刻、大聲地失敗」勝過「安靜地做錯事」。
//   於是把選擇權(join 還是 detach)強制交還給程式設計師 —— 而 RAII 守衛
//   就是幫我們「不會忘記做選擇」的工具。
//
// 【2. 為什麼手動 join() 不夠?】
//   看起來很合理的程式碼:
//
//       std::thread t(work);
//       do_something_that_may_throw();   // ← 這裡丟出例外
//       t.join();                        // ← 永遠不會執行到
//
//   例外一丟,控制流直接跳過 t.join(),接著 stack unwinding 銷毀 t,
//   而 t 仍是 joinable → std::terminate()。原本只是一個可以被 catch 的
//   例外,升級成整個行程死亡。
//
//   加 try/catch 可以解決,但很快就會失控:
//       try { do_something(); } catch (...) { t.join(); throw; }
//       t.join();
//   每多一個執行緒就要多一層,而且 join() 寫了兩次。RAII 把這件事
//   收斂成「宣告一個守衛物件」一行。
//
// 【3. 本檔的關鍵設計:持有「參考」而不是「值」】
//   ThreadGuard 的成員是 std::thread&,這是刻意的:
//     * 呼叫端仍然擁有那個 std::thread,還能對它做別的事(get_id()、
//       在 guard 解構前先手動 join()、甚至改成 detach())。
//     * ThreadGuard 只負責「保證離開作用域時 t 已被處理」這一件事。
//   代價是:守衛不管理生命週期,所以【宣告順序變成正確性的一部分】。
//   下一課的 ScopedThread 用「持有值(move 進來)」消除這個坑,
//   兩者是刻意的對照組,不是誰取代誰。
//
// 【4. 宣告順序為何是正確性問題】
//   C++ 保證同一作用域內的自動變數以【宣告的相反順序】解構。
//
//       std::thread t(work);      // 先宣告 → 後解構
//       ThreadGuard g(t);         // 後宣告 → 先解構  ✅ 正確
//
//   解構順序:g 先解構 → g 呼叫 t.join() → t 變成非 joinable
//             → t 解構時 joinable() 為 false → 安全。
//
//   若寫反了(先 guard 後 thread)根本無法編譯,因為 guard 要綁定的
//   t 還不存在。真正的危險是【成員變數】的情境:類別裡如果把
//   ThreadGuard 成員宣告在 std::thread 成員【之前】,成員解構順序同樣是
//   反向,thread 會先被解構 → joinable → std::terminate()。
//
// 【5. 為什麼要 = delete 複製?】
//   若允許複製,兩個守衛會指向同一個 std::thread,第一個解構時 join(),
//   第二個解構時再對已經 join 過的 thread 呼叫 join() —— 對非 joinable
//   的 thread 呼叫 join() 會丟出 std::system_error(errc::invalid_argument)。
//   刪除複製是「讓錯誤在編譯期就發生」的典型手法。
//   註:此處也沒有提供移動操作。因為成員是參考,參考無法重新綁定,
//   移動語意在這個設計裡沒有意義。
//
// 【概念補充 Concept Deep Dive】
//
// (A) stack unwinding 期間解構函式一定會被呼叫嗎?
//   會 —— 但有前提:例外必須「被某處 catch 住」。若整個程式沒有任何
//   handler,標準允許實作【不做 unwinding】就直接 std::terminate()
//   ([except.terminate])。也就是說 uncaught exception 時,你的守衛
//   可能根本沒機會執行。這是實作定義的行為,GCC 在多數情況下仍會
//   unwinding,但不可依賴。真正的教訓:別讓例外逸出 main()。
//
// (B) 解構函式裡呼叫會拋例外的函式,風險在哪?
//   join() 可能丟出 std::system_error(例如對自己 join、或 deadlock 被偵測)。
//   自 C++11 起,解構函式預設是 noexcept(true);若例外從 noexcept 函式逸出,
//   直接呼叫 std::terminate()。本檔的 join() 在正常路徑不會拋,但嚴謹的
//   生產程式碼會把它包成:
//       ~ThreadGuard() noexcept { try { if (t.joinable()) t.join(); } catch (...) {} }
//   吞掉例外不漂亮,但比 terminate() 好 —— 這是解構函式的普遍兩難。
//
// (C) join() 底層做了什麼?
//   在 Linux/glibc 上 std::thread::join() 最終是 pthread_join(),它會:
//     1. 阻塞呼叫端,等待目標執行緒的 thread descriptor 標記為已結束;
//     2. 回收目標執行緒的核心資源與 stack(不 join 也不 detach 就是資源洩漏);
//     3. 把 std::thread 內部的 native handle 重設 → joinable() 變 false。
//   步驟 3 正是「join 之後再 join 會丟例外」的原因:handle 已經沒了。
//
// (D) join() 提供什麼記憶體序保證?
//   join() 回傳之後,子執行緒中所有的寫入對呼叫端都是可見的 ——
//   子執行緒的結束與 join() 的返回之間有 happens-before 關係
//   ([thread.thread.member])。因此下面這段【沒有】data race:
//       int result = 0;
//       std::thread t([&]{ result = 42; });
//       t.join();
//       std::cout << result;   // 保證讀到 42,不需要任何 atomic 或 mutex
//   這是初學者最常誤以為「要加鎖」的地方 —— join() 本身就是同步點。
//
// 【注意事項 Pay Attention】
//   1. 守衛必須比 std::thread 先解構 → 宣告順序:thread 在前,guard 在後。
//      類別成員亦同(成員以宣告順序建構、反向解構)。
//   2. 成員是參考 → 若 std::thread 先被銷毀,守衛持有 dangling reference,
//      解構時存取它是未定義行為(不會有固定症狀,可能看似正常)。
//   3. 解構 joinable 的 std::thread 呼叫 std::terminate() —— 這是標準
//      明文保證的確定結果,不是 UB。
//   4. 對非 joinable 的 thread 呼叫 join() 會丟 std::system_error,
//      所以解構函式裡的 if (t.joinable()) 檢查不可省略。
//   5. 此守衛只做 join,不支援 detach;需要二選一請見「RAII 與執行緒管理4」
//      的 FlexibleThread,或直接改用 C++20 的 std::jthread。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】RAII 執行緒守衛(ThreadGuard)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個 joinable 的 std::thread 被解構時會發生什麼事?
//     答：呼叫 std::terminate(),程式終止。這是標準 [thread.thread.destr]
//         明文規定的行為,是【確定的結果】,不是未定義行為。設計理由是
//         自動 join 會讓解構函式無預警阻塞、自動 detach 會產生 dangling
//         reference,兩者都比「立刻大聲失敗」更難除錯。
//     追問：那為什麼 C++20 的 std::jthread 就敢在解構時自動 join?
//         → 因為 jthread 同時內建了 stop_token 協作取消機制:解構時先
//           request_stop() 再 join(),被良好撰寫的工作函式會看到停止請求
//           而主動返回,所以那個 join 不會無限期阻塞。std::thread 沒有
//           取消管道,自動 join 才會變成「可能永遠不返回」。
//
// 🔥 Q2. 為什麼 ThreadGuard 要在 main 裡宣告在 std::thread 之【後】?
//     答：自動變數以宣告的相反順序解構。thread 先宣告、guard 後宣告,
//         解構時 guard 先跑並 join 掉 thread,thread 才解構且已非 joinable。
//         若順序相反,thread 會在還 joinable 的狀態下先被解構 → terminate()。
//     追問：如果它們是同一個 class 的成員變數呢?
//         → 規則完全一樣:成員按宣告順序建構、反向解構。所以 std::thread
//           成員要宣告在守衛成員之前。這也是為什麼多數人偏好「守衛直接
//           持有 thread(by value)」的設計 —— 順序問題從此消失。
//
// ⚠️ 陷阱 1. 「join() 完之後要不要加 mutex 才能安全讀取子執行緒寫的變數?」
//     答：不用。join() 的返回與子執行緒的結束之間有 happens-before 關係,
//         子執行緒所有寫入在 join() 返回後都保證可見。加 mutex 是多餘的。
//     為什麼會錯：多數人把「多執行緒」直接等同於「一律要同步原語」,
//         忽略了 join()/thread 建構本身就是標準規定的同步點。真正需要
//         同步的是「兩個執行緒同時存活期間」的共享存取,不是 join 之後。
//
// ⚠️ 陷阱 2. 「把 ThreadGuard 的成員從 std::thread& 改成 std::thread
//              (by value),其他都不動,應該更安全吧?」
//     答：不能只改型別。建構函式若還是 ThreadGuard(std::thread& t) : t(t) {}
//         會變成【複製】std::thread —— 而 std::thread 的複製建構是 deleted,
//         直接編譯失敗。必須改成傳值 + std::move 進來,而且呼叫端從此
//         失去對該 thread 的控制權。這正是下一課 ScopedThread 的設計。
//     為什麼會錯：以為「值語意比參考安全」是可以無腦套用的原則,
//         忽略 std::thread 是 move-only 型別,值語意會連帶改變所有權模型。
// ═══════════════════════════════════════════════════════════════════════════

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 主角:持有參考的執行緒守衛
// -----------------------------------------------------------------------------
class ThreadGuard {
    std::thread& t;

public:
    explicit ThreadGuard(std::thread& thread) : t(thread) {}

    ~ThreadGuard() {
        if (t.joinable()) {
            t.join();
        }
    }

    // 禁止複製:兩個守衛管同一個 thread,第二次 join() 會丟 system_error
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// -----------------------------------------------------------------------------
// 示範 1:基本用法 —— 正常路徑
// -----------------------------------------------------------------------------
void demoBasic() {
    std::cout << "  [main] 準備啟動 worker\n";

    {
        std::thread t([]() {
            std::cout << "  [worker] 工作中\n";
        });

        ThreadGuard guard(t);  // 保證離開作用域時 t 已被 join
    }  // guard 先解構 → join;t 後解構,此時已非 joinable → 安全

    // 走到這裡代表 join() 已完成,worker 的輸出必定在上一行之前
    std::cout << "  [main] worker 已回收\n";
}

// -----------------------------------------------------------------------------
// 示範 2:例外路徑 —— RAII 的真正價值
//   即使中途丟出例外,guard 的解構函式仍會在 stack unwinding 時執行,
//   所以 t 一定會被 join,不會走到「解構 joinable thread → terminate()」。
// -----------------------------------------------------------------------------
void demoException() {
    try {
        std::thread t([]() {
            std::cout << "  [worker] 開始工作\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "  [worker] 工作完成\n";
        });

        ThreadGuard guard(t);

        throw std::runtime_error("處理途中發生錯誤");
        // 沒有 guard 的話,這裡之後的 t.join() 永遠不會執行,
        // t 會在 joinable 狀態下被解構 → std::terminate()
    } catch (const std::exception& e) {
        std::cout << "  [main] 攔截到例外: " << e.what() << "\n";
        std::cout << "  [main] 執行緒已被守衛安全 join,程式繼續執行\n";
    }
}

// -----------------------------------------------------------------------------
// 示範 3:join() 的 happens-before 保證
//   子執行緒的寫入在 join() 返回後保證可見,不需要 atomic 或 mutex。
// -----------------------------------------------------------------------------
void demoHappensBefore() {
    int result = 0;  // 普通 int,沒有 atomic

    {
        std::thread t([&result]() { result = 42; });
        ThreadGuard guard(t);
    }  // guard 解構 → join() → 建立 happens-before

    // 這裡讀 result 沒有 data race:join() 是標準規定的同步點
    std::cout << "  子執行緒寫入的值 = " << result << "(不需 mutex 即保證可見)\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題組(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)考的都是【執行緒之間的順序協調】,
//   解法核心是 condition_variable / semaphore / atomic,執行緒的建立與
//   回收由 LeetCode 的評測框架處理,題目根本碰不到 join()。
//   本檔主題是「執行緒生命週期的 RAII 管理」,與那些題目沒有真實交集,
//   硬湊一題只會誤導。故此處從缺,改以貼近真實工程的實務範例呈現。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】批次影像處理:任何失敗路徑都不能洩漏 worker
//
//   情境:服務收到一批影像要縮圖,分給多個 worker 平行處理。
//         若其中一張圖的中繼資料損毀,要立刻中止整批並回報錯誤。
//   風險:中止時如果直接 throw,那些還在跑的 worker 執行緒會在
//         joinable 狀態下被解構 → std::terminate() → 整個服務死掉,
//         而不是回傳一個 HTTP 500。
//   解法:每個 worker 配一個 ThreadGuard,無論正常結束或例外逸出,
//         離開作用域時一律 join,錯誤才能以例外的形式正常往上傳遞。
// -----------------------------------------------------------------------------
struct ImageTask {
    std::string name;
    int         width;
    int         height;
    bool        metadataOk;
};

// 回傳成功處理的張數;遇到損毀中繼資料則丟例外(但保證所有 worker 已回收)
int processImageBatch(const std::vector<ImageTask>& tasks) {
    std::vector<int>         pixelCounts(tasks.size(), 0);
    std::vector<std::thread> workers;
    std::vector<std::unique_ptr<ThreadGuard>> guards;

    workers.reserve(tasks.size());
    guards.reserve(tasks.size());

    // 注意:workers 必須先 reserve 且不可再成長,否則 reallocation 會
    //       讓 guards 裡的參考失效(dangling)—— 這正是「持有參考」設計
    //       的代價,也是下一課改用「持有值」的動機。
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        workers.emplace_back([&tasks, &pixelCounts, i]() {
            pixelCounts[i] = tasks[i].width * tasks[i].height;
        });
        guards.emplace_back(std::make_unique<ThreadGuard>(workers.back()));
    }

    // 驗證階段:任何一張圖的中繼資料損毀就中止整批
    for (const auto& task : tasks) {
        if (!task.metadataOk) {
            throw std::runtime_error("中繼資料損毀: " + task.name);
            // ← 例外逸出時,guards 與 workers 依序解構,
            //   每個 ThreadGuard 都會 join 它的 worker,不會 terminate()
        }
    }

    // guards 解構(反向順序)→ 全部 join
    guards.clear();

    int total = 0;
    for (int c : pixelCounts) total += c;
    return total;
}

int main() {
    std::cout << "=== 示範 1:基本用法 ===\n";
    demoBasic();

    std::cout << "\n=== 示範 2:例外路徑仍安全 join ===\n";
    demoException();

    std::cout << "\n=== 示範 3:join() 的 happens-before 保證 ===\n";
    demoHappensBefore();

    std::cout << "\n=== 日常實務:批次影像處理(全部正常)===\n";
    std::vector<ImageTask> good{
        {"cover.jpg",     1920, 1080, true},
        {"thumb.png",      320,  240, true},
        {"banner.webp",   1280,  400, true},
    };
    std::cout << "  總像素數 = " << processImageBatch(good) << "\n";

    std::cout << "\n=== 日常實務:批次影像處理(中途失敗)===\n";
    std::vector<ImageTask> bad{
        {"ok.jpg",         800,  600, true},
        {"corrupted.raw",  640,  480, false},
        {"also_ok.png",    100,  100, true},
    };
    try {
        int total = processImageBatch(bad);  // 先求值,避免半行輸出殘留
        std::cout << "  總像素數 = " << total << "\n";
    } catch (const std::exception& e) {
        std::cout << "  批次中止: " << e.what() << "\n";
        std::cout << "  所有 worker 已被守衛回收,服務未崩潰\n";
    }

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.1：RAII 與執行緒管理2.cpp" -o raii2

// === 預期輸出 ===
// === 示範 1:基本用法 ===
//   [main] 準備啟動 worker
//   [worker] 工作中
//   [main] worker 已回收
//
// === 示範 2:例外路徑仍安全 join ===
//   [worker] 開始工作
//   [worker] 工作完成
//   [main] 攔截到例外: 處理途中發生錯誤
//   [main] 執行緒已被守衛安全 join,程式繼續執行
//
// === 示範 3:join() 的 happens-before 保證 ===
//   子執行緒寫入的值 = 42(不需 mutex 即保證可見)
//
// === 日常實務:批次影像處理(全部正常)===
//   總像素數 = 2662400
//
// === 日常實務:批次影像處理(中途失敗)===
//   批次中止: 中繼資料損毀: corrupted.raw
//   所有 worker 已被守衛回收,服務未崩潰
//
// === 全部示範結束 ===
