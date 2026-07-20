// =============================================================================
//  課程 3.1：RAII 與執行緒管理 4  —  FlexibleThread：把 join/detach 做成「策略」
// =============================================================================
//
// 【主題資訊 Information】
//   class FlexibleThread {
//   public:
//       enum class Action { join, detach };            // 解構時的處置策略
//   private:
//       std::thread t;
//       Action      action;
//   public:
//       FlexibleThread(std::thread thread, Action a);
//       ~FlexibleThread();                             // 依 action 決定 join/detach
//       FlexibleThread(const FlexibleThread&)            = delete;
//       FlexibleThread& operator=(const FlexibleThread&) = delete;
//   };
//
//   標準版本：C++11(enum class 亦為 C++11)
//   標頭檔  ：<thread>、<chrono>(sleep_for)
//   複雜度  ：join 策略阻塞至執行緒結束;detach 策略為 O(1) 立即返回
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要把「處置方式」參數化?】
//   前兩課的守衛都寫死了 join。但真實系統裡確實存在兩種背景工作:
//
//     (a) 結果被需要 → 必須 join
//         例:平行計算的分片、必須落盤的交易紀錄、要回傳給呼叫端的資料。
//         這種工作若沒等它做完就繼續,結果就是錯的。
//
//     (b) 結果不被需要 → 可以 detach(fire-and-forget)
//         例:非關鍵的統計上報、快取預熱、清理暫存檔。
//         主流程不該為了它們而變慢。
//
//   FlexibleThread 用一個 enum class 讓呼叫端在【建立當下】就宣告意圖,
//   而不是散落在各處的 if/else。這是 policy(策略)的最簡單形式。
//
// 【2. 為什麼用 enum class 而不是 bool?】
//       FlexibleThread ft(std::thread(f), true);         // ← true 是什麼?
//       FlexibleThread ft(std::thread(f), Action::join); // ← 一看就懂
//   bool 參數是可讀性殺手(所謂的 boolean trap)。enum class 還額外提供:
//     * 強型別:不會和 int 隱式互換,不會被誤傳
//     * 有作用域:Action::join 不會污染外層命名空間
//   這兩點正是 C++11 引入 enum class(scoped enumeration)的原因,
//   對照舊的 unscoped enum,後者會把列舉值洩漏到外層作用域且能隱式轉 int。
//
// 【3. detach() 到底做了什麼?為什麼危險?】
//   detach() 把 std::thread 物件與底層執行緒【切斷關聯】:
//     * 物件變成非 joinable,可以安全解構
//     * 底層執行緒繼續執行,結束時由 runtime 自動回收資源
//     * 【沒有任何人能再等待它、查詢它、或取消它】
//
//   最後一點是所有危險的根源。被 detach 的執行緒常見的三種災難:
//
//     (a) dangling reference:它捕捉了 main 或某個函式的區域變數參考,
//         而那個作用域已經結束。此時存取就是未定義行為 —— 沒有固定症狀,
//         可能看似正常、可能讀到垃圾、可能毀損無關的記憶體。
//
//     (b) 存取已銷毀的全域物件:main() 結束後,靜態儲存期物件(包含
//         std::cout)會被解構。若 detached thread 此時還在 std::cout <<,
//         同樣是未定義行為。
//
//     (c) 程序直接結束:main() 回傳會呼叫 exit(),而 exit() 【不會】等待
//         任何 detached thread。工作可能做到一半就被連根拔起。
//
//   本檔的 main() 用 sleep_for(100ms) 硬等 —— 請注意這是【教學用的權宜
//   之計,不是正確做法】。用時間去猜「應該夠久了吧」是典型的競態條件
//   (race condition):機器忙碌、被排程延遲、工作變重,任何一個因素都
//   會讓這個假設失效。真正的解法是用 join、std::future,或 C++20 的
//   std::latch/jthread 來建立確定的同步關係。
//
// 【4. 解構函式為何仍要檢查 joinable()?】
//   和 ScopedThread 不同,FlexibleThread 的建構函式【沒有】驗證 joinable,
//   所以呼叫端可以傳進一個預設建構的空 thread。此時:
//     * 對非 joinable 的 thread 呼叫 join()   → 丟 std::system_error
//     * 對非 joinable 的 thread 呼叫 detach() → 同樣丟 std::system_error
//   兩條路都會從 noexcept 的解構函式逸出 → std::terminate()。
//   所以 if (t.joinable()) 這個檢查在這個設計裡是【必要】的,
//   不能照抄 ScopedThread 的簡潔寫法。這是「不變式強度決定實作繁簡」
//   的具體例子:建構函式驗得越嚴,解構函式就能寫得越簡單。
//
// 【概念補充 Concept Deep Dive】
//
// (A) detach 之後,執行緒的資源何時回收?
//   在 pthread 模型裡,執行緒有 joinable 與 detached 兩種狀態。
//   detached 的執行緒結束時,其 stack 與 thread descriptor 由 runtime
//   立即回收,不需要別人來 pthread_join。這正是「不 join 也不 detach
//   就是資源洩漏」的另一面:joinable 的執行緒結束後,資源會一直留著
//   等人來收屍(zombie thread)—— 雖然 std::thread 的解構函式會先
//   terminate(),所以在 C++ 裡你不太有機會看到這種洩漏。
//
// (B) exit() 與 detached thread 的關係
//   main() 回傳等同呼叫 std::exit()。std::exit() 會:
//     1. 解構所有靜態儲存期物件、執行 atexit 註冊的函式
//     2. flush 並關閉 stdio 串流
//     3. 把控制權交回作業系統
//   全程【不等待】任何 detached thread。所以「detach 的執行緒會不會做完」
//   完全取決於它和 main 的賽跑結果,沒有任何標準保證。
//   若真的需要等,C++11 之後也沒有標準做法可以「等待所有 detached
//   thread」—— 這正是為什麼建議一律用 join 或 jthread。
//
// (C) 這個設計與 std::jthread 的關係
//   C++20 的 std::jthread 等同於「策略永遠是 join,而且附帶取消管道」:
//       ~jthread() { if (joinable()) { request_stop(); join(); } }
//   它解決了「自動 join 可能永遠不返回」的疑慮 —— 因為 request_stop()
//   會讓合作良好的工作函式主動結束。FlexibleThread 的 detach 策略在
//   jthread 的世界裡幾乎沒有存在必要:需要 fire-and-forget 時,更好的
//   工具是把工作丟進 thread pool,而不是開一個沒人管的執行緒。
//
// (D) 為什麼沒有提供移動操作?
//   同 ScopedThread:使用者自訂了解構函式,隱式移動操作被抑制;
//   複製又被 = delete,所以 FlexibleThread 不可搬移、不能進容器。
//   若要補,移動賦值必須先處理「自己原本持有的 thread」——
//   而處理方式又得依 action 而定,邏輯會比 JoiningThread 更繁瑣。
//
// 【注意事項 Pay Attention】
//   1. detach 的執行緒不可捕捉區域變數的參考或指標;要捕捉就用 by-value,
//      或改用 shared_ptr 延長生命週期。
//   2. 用 sleep_for 等待 detached thread 是競態條件,不是同步。
//      本檔為了讓輸出可觀察而使用,實務請改用 join / future / latch。
//   3. 建構函式未驗證 joinable,故解構函式的 if (t.joinable()) 不可省略。
//   4. 傳入非 joinable 的 thread 不會有任何錯誤提示,它會被靜默忽略 ——
//      這是本設計比 ScopedThread 寬鬆(也比較容易藏 bug)的地方。
//   5. 原始版本使用了 std::chrono 卻只 include <thread>,能編譯是因為
//      libstdc++ 的 <thread> 會傳遞含入 <chrono>;此為實作細節,不可依賴,
//      本檔已顯式補上 <chrono>。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】join / detach 策略與 detached thread 的風險
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. join() 和 detach() 的差別是什麼?各自在什麼時候用?
//     答：join() 阻塞呼叫端直到目標執行緒結束,並建立 happens-before
//         關係(子執行緒的寫入之後保證可見);detach() 則切斷關聯,
//         執行緒變成沒人管的背景工作,結束時由 runtime 自行回收。
//         需要結果、需要保證完成、或執行緒會存取呼叫端的資料 → join;
//         真正的 fire-and-forget 且完全不碰呼叫端狀態 → 才考慮 detach。
//     追問：兩個都不呼叫會怎樣?
//         → std::thread 解構時 joinable() 為真 → std::terminate()。
//           這是標準明文規定的確定行為,不是未定義行為。
//
// 🔥 Q2. main() 結束時,還在跑的 detached thread 會發生什麼事?
//     答：main() 回傳等同 std::exit(),它不等待任何 detached thread,
//         而是直接解構靜態物件(含 std::cout)並結束行程。detached
//         thread 可能被中途截斷;若它此時還在存取 std::cout 或其他
//         已解構的靜態物件,就是未定義行為 —— 沒有固定症狀。
//     追問：那要怎麼安全地等它?
//         → 標準沒有「等待所有 detached thread」的機制。正確做法是
//           一開始就別 detach:用 join、std::future、C++20 std::latch,
//           或交給有明確關閉流程的 thread pool。
//
// ⚠️ 陷阱 1. 「我 detach 之後 sleep 了 100ms,實測每次都印得出來,
//              這樣應該就安全了吧?」
//     答：不安全。這只是把競態視窗放大到「多數情況下會贏」,並沒有
//         建立任何同步關係。機器負載變高、執行緒被排程延遲、工作量
//         增加,任何一項都會讓假設失效,而失敗時的症狀是未定義行為,
//         不會有明確的錯誤訊息。
//     為什麼會錯：把「測了很多次都對」當成「保證正確」。並行程式的
//         錯誤本質上是機率性的,測試通過只代表這次沒踩到,
//         不代表競態不存在。
//
// ⚠️ 陷阱 2. 「detach 之後那個 std::thread 物件還能用嗎?」
//     答：物件本身仍然存在且可以安全解構,但已變成非 joinable ——
//         get_id() 回傳預設建構的 id,再呼叫 join() 或 detach() 都會丟
//         std::system_error。它和底層執行緒之間已經沒有任何關聯。
//     為什麼會錯：把 detach 想成「背景執行但我還握著它」。實際上
//         detach 的語意是【放棄控制權】,std::thread 物件變成一個空殼。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 主角:解構策略可選的執行緒守衛
// -----------------------------------------------------------------------------
class FlexibleThread {
public:
    enum class Action { join, detach };

private:
    std::thread t;
    Action      action;

public:
    FlexibleThread(std::thread thread, Action a)
        : t(std::move(thread)), action(a) {}

    ~FlexibleThread() {
        // 建構函式未驗證 joinable,故此檢查不可省略
        if (t.joinable()) {
            if (action == Action::join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }

    FlexibleThread(const FlexibleThread&) = delete;
    FlexibleThread& operator=(const FlexibleThread&) = delete;
};

// -----------------------------------------------------------------------------
// 示範 1:兩種策略並陳
//   ft1 用 join   → 解構時阻塞等待,輸出順序確定
//   ft2 用 detach → 解構時立即返回,之後靠 sleep 硬等(教學用,非正確做法)
// -----------------------------------------------------------------------------
void demoTwoPolicies() {
    {
        FlexibleThread ft1(
            std::thread([]() { std::cout << "  [join 策略] 我一定會被等到\n"; }),
            FlexibleThread::Action::join);
    }  // ft1 解構 → join(),上面那行保證已經印出

    std::cout << "  [main] join 策略的執行緒已確定結束\n";

    {
        FlexibleThread ft2(
            std::thread([]() { std::cout << "  [detach 策略] 我沒人等,能不能印完看運氣\n"; }),
            FlexibleThread::Action::detach);
    }  // ft2 解構 → detach(),立即返回,執行緒可能還在跑

    // ⚠️ 用時間「猜」它做完了 —— 這是競態條件,不是同步
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "  [main] 硬等 100ms 之後才敢繼續(這是權宜之計)\n";
}

// -----------------------------------------------------------------------------
// 示範 2:非 joinable 的 thread 會被靜默忽略
//   FlexibleThread 不像 ScopedThread 會丟例外,這是設計寬鬆的代價。
// -----------------------------------------------------------------------------
void demoEmptyThread() {
    {
        FlexibleThread ft(std::thread{}, FlexibleThread::Action::join);
        std::cout << "  傳入預設建構的空 thread:建構成功,無任何警告\n";
    }  // 解構時 joinable() 為 false → 什麼都不做
    std::cout << "  解構完成,未丟例外也未 terminate()\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 / 1115 / 1116 / 1117 / 1195)由評測
//   框架負責建立與回收執行緒,作答者只實作被呼叫的成員函式,拿不到
//   std::thread 物件,因此 join/detach 策略在那些題目裡完全不存在。
//   本檔主題與之無交集,硬套只會誤導,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單結帳:哪些背景工作能 detach、哪些不能
//
//   情境:電商結帳流程,除了主要的扣款,還會觸發兩類背景工作:
//     * 寫入交易稽核紀錄(audit log)—— 法遵要求,【必須】完成才能回應,
//       否則會發生「使用者看到成功、稽核卻查無此筆」的資料不一致。
//       → Action::join
//     * 上報行為分析事件(analytics)—— 掉了頂多少一筆統計數字,
//       不該讓使用者多等。
//       → Action::detach
//
//   關鍵設計:detach 的那條執行緒【只捕捉 by-value 的副本】,
//   絕不碰 checkout() 的區域變數參考 —— 因為 checkout() 可能早就返回了。
// -----------------------------------------------------------------------------
struct CheckoutResult {
    std::string orderId;
    int         amount;
    bool        auditWritten;
};

// 模擬的稽核儲存(實務上是資料庫);用 atomic 讓 detached thread 也能安全累加
std::atomic<int> g_analyticsSent{0};

CheckoutResult checkout(const std::string& orderId, int amount) {
    CheckoutResult result{orderId, amount, false};

    {
        // (1) 稽核紀錄:join —— 必須在函式返回前完成
        //     捕捉 &result 是安全的,因為 join 保證執行緒在此作用域內結束
        FlexibleThread auditWriter(
            std::thread([&result]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                result.auditWritten = true;  // join() 建立 happens-before,無 data race
            }),
            FlexibleThread::Action::join);

        // (2) 分析上報:detach —— 只捕捉副本,不碰任何區域變數的參考
        FlexibleThread analyticsSender(
            std::thread([orderId, amount]() {  // by-value 捕捉,刻意複製
                (void)orderId;
                (void)amount;
                g_analyticsSent.fetch_add(1, std::memory_order_relaxed);
            }),
            FlexibleThread::Action::detach);
    }
    // auditWriter 先解構(反向順序)…實際上 analyticsSender 後宣告故先解構(detach,
    // 立即返回),接著 auditWriter 解構並 join → result.auditWritten 已確定寫入

    return result;
}

int main() {
    std::cout << "=== 示範 1:join 與 detach 兩種策略 ===\n";
    demoTwoPolicies();

    std::cout << "\n=== 示範 2:非 joinable 的 thread ===\n";
    demoEmptyThread();

    std::cout << "\n=== 日常實務:訂單結帳的兩類背景工作 ===\n";
    std::vector<std::string> orders{"ORD-1001", "ORD-1002", "ORD-1003"};
    int auditedCount = 0;
    for (const auto& id : orders) {
        CheckoutResult r = checkout(id, 1200);
        if (r.auditWritten) ++auditedCount;
    }
    std::cout << "  稽核紀錄完成筆數 = " << auditedCount << " / " << orders.size()
              << "(join 策略 → 確定值)\n";

    // detached 的分析上報沒有同步關係,只能「盡量等」再讀 —— 故意展示不確定性
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "  分析事件上報數(detach 策略 → 無保證,此處硬等 100ms 後觀察)= "
              << g_analyticsSent.load(std::memory_order_relaxed) << "\n";

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.1：RAII 與執行緒管理4.cpp" -o raii4

// 註:示範 1 的 detach 策略那一行、以及最後的分析事件上報數,都【沒有】同步保證。
//     本機此次執行時 detached thread 都在 sleep 期間跑完了,但這不是標準保證的結果,
//     在負載較重的機器上有可能觀察到不同輸出。

// === 預期輸出 ===
// === 示範 1:join 與 detach 兩種策略 ===
//   [join 策略] 我一定會被等到
//   [main] join 策略的執行緒已確定結束
//   [detach 策略] 我沒人等,能不能印完看運氣
//   [main] 硬等 100ms 之後才敢繼續(這是權宜之計)
//
// === 示範 2:非 joinable 的 thread ===
//   傳入預設建構的空 thread:建構成功,無任何警告
//   解構完成,未丟例外也未 terminate()
//
// === 日常實務:訂單結帳的兩類背景工作 ===
//   稽核紀錄完成筆數 = 3 / 3(join 策略 → 確定值)
//   分析事件上報數(detach 策略 → 無保證,此處硬等 100ms 後觀察)= 3
//
// === 全部示範結束 ===
