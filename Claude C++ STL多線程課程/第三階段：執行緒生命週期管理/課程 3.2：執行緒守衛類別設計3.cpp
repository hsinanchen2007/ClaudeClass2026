// =============================================================================
//  課程 3.2：執行緒守衛類別設計 3  —  DetachingThread：解構自動 detach 的反面教材
// =============================================================================
//
// 【主題資訊 Information】
//   class DetachingThread {
//       std::thread t;
//   public:
//       template<typename Func, typename... Args>
//       explicit DetachingThread(Func&& f, Args&&... args);
//       DetachingThread(DetachingThread&&)            = default;   // ⚠️ 見下文
//       DetachingThread& operator=(DetachingThread&&) = default;   // ⚠️ 有 terminate 風險
//       DetachingThread(const DetachingThread&)            = delete;
//       DetachingThread& operator=(const DetachingThread&) = delete;
//       ~DetachingThread();                                        // joinable → detach()
//   };
//
//   標準版本：C++11
//   標頭檔  ：<thread>、<chrono>、<utility>
//   複雜度  ：解構為 O(1)(detach 不等待);但【不提供任何完成保證】
//
//   ⚠️ 本檔是【對照組】,不是推薦寫法。它的價值在於把 detach 的每一個
//      風險攤開來看。生產環境請優先用 JoiningThread 或 C++20 std::jthread。
//
// 【詳細解釋 Explanation】
//
// 【1. 與 ScopedThread 的鏡像關係】
//   ScopedThread    解構 → join()   → 阻塞等待,有完成保證
//   DetachingThread 解構 → detach() → 立即返回,【沒有】任何保證
//
//   兩者在程式碼上只差一個字,語意上卻是天壤之別:
//   join 把「執行緒的生命週期」收攏進作用域內;
//   detach 把它放生到作用域外 —— 而作用域外的世界正在被拆除。
//
// 【2. detach 之後,那條執行緒面對的是什麼】
//   建立它的作用域結束時,下列東西會依序消失:
//     * 該作用域的所有區域變數(包含被 [&] 捕捉的一切)
//     * 呼叫端函式的整個 stack frame
//     * main() 結束後,所有靜態儲存期物件(含 std::cout、全域 logger)
//   detached thread 對這些一無所知,它會繼續執行並存取那些已經不存在的東西。
//   這就是未定義行為 —— 沒有固定症狀:可能看起來完全正常、可能印出亂碼、
//   可能毀損無關的記憶體讓程式在幾分鐘後才在別的地方崩潰。
//
//   ⚠️ 請注意這裡的用詞:是「未定義行為」,不是「一定會崩潰」。
//   detached thread 存取已銷毀物件【不保證】會有任何可見症狀,
//   這正是它比 std::terminate() 更難除錯的原因 —— terminate 至少會當場停下來。
//
// 【3. 唯一能讓 detach 變安全的做法:切斷所有外部依賴】
//   若真的必須 detach,那條執行緒必須【完全自給自足】:
//     (a) 不捕捉任何區域變數的參考或指標 —— 只能 by-value 捕捉
//     (b) 需要共享狀態時,用 std::shared_ptr 值捕捉,讓狀態的生命週期
//         由執行緒自己延長,而不是依賴建立者還活著
//     (c) 不觸碰 std::cout 等靜態物件(除非能保證 main 還沒結束)
//   本檔的【日常實務範例】示範的正是 (b) 這個模式 —— 它是 detach 在真實
//   系統裡唯一站得住腳的用法。
//
// 【4. ★ 本檔最重要的發現:= default 的移動賦值藏著 std::terminate()】
//   原始碼寫著:
//       DetachingThread& operator=(DetachingThread&&) = default;
//   看起來人畜無害,實際上它會被展開成「對成員 t 做移動賦值」,也就是
//   呼叫 std::thread::operator=(thread&&)。而標準規定:
//
//       若賦值目標本身仍是 joinable → 呼叫 std::terminate()
//
//   於是:
//       DetachingThread a(work1);
//       DetachingThread b(work2);
//       a = std::move(b);      // ← a.t 還 joinable → std::terminate()
//
//   這是【標準保證的確定結果】,不是未定義行為,也無法被 catch。
//   本機實測:程式以 SIGABRT 結束,exit code 134,
//   訊息為 "terminate called without an active exception"。
//
//   為什麼會出現這個 bug?因為作者只想著「解構時 detach」,卻忘了
//   移動賦值也是一個「舊資源必須先被處置」的時機。= default 不會
//   替你套用你的 detach 策略,它只是逐成員移動 —— 而 std::thread
//   的逐成員移動語意就是 terminate。
//
//   正確寫法應該與 JoiningThread 對稱,顯式處理舊資源:
//       DetachingThread& operator=(DetachingThread&& o) noexcept {
//           if (this != &o) {
//               if (t.joinable()) t.detach();   // 套用自己的策略
//               t = std::move(o.t);
//           }
//           return *this;
//       }
//   本檔保留原始的 = default 版本以呈現這個陷阱,並在示範 3 用
//   #ifdef DEMONSTRATE_UB 隔離出可實際觀察的重現路徑。
//
// 【5. 為什麼移動建構 = default 反而沒問題?】
//   移動【建構】的目標是一個尚未初始化的物件,它手上沒有舊資源要處置,
//   所以逐成員移動完全正確。真正需要「先處置舊資源」的只有移動【賦值】。
//   這個不對稱性是 Rule of Five 裡最常被忽略的一點。
//
// 【概念補充 Concept Deep Dive】
//
// (A) detach 之後執行緒的資源由誰回收?
//   底層是 pthread 的 detached state:執行緒結束時,其 stack 與
//   thread descriptor 由 runtime 直接回收,不需要別人 pthread_join。
//   所以 detach 不會造成資源洩漏 —— 它造成的是【正確性】問題,
//   不是資源問題。這個區別常被混淆。
//
// (B) 為什麼標準不提供「等待所有 detached thread」的機制?
//   因為 detach 的語意就是「我放棄對它的所有控制」。一旦切斷關聯,
//   執行期沒有保留任何可供查詢的 handle。若你需要等,你要的其實是
//   join,只是希望「晚一點再等」—— 那應該用 std::future、
//   C++20 的 std::latch,或把工作交給有明確關閉流程的 thread pool。
//
// (C) main() 結束的順序,為什麼對 detached thread 特別致命?
//   main() 回傳 → 呼叫 std::exit() → 解構靜態儲存期物件 → 行程結束。
//   全程不等待任何 detached thread。所以 detached thread 面對的是一個
//   「正在被拆除的世界」,而它自己完全不知情。
//   更糟的是:即使它只是呼叫 std::cout <<,而 std::cout 剛好已被解構,
//   症狀也可能完全看不出來(輸出消失、或者什麼事都沒發生),
//   讓人誤以為程式是正確的。
//
// (D) 那 std::jthread 怎麼解決這個問題?
//   它根本不提供「解構時 detach」的選項。jthread 的解構函式固定是
//   request_stop() + join():用協作取消讓工作【主動且快速地結束】,
//   然後確實等到它結束。這等於是說:與其放生,不如禮貌地請它收工再等它。
//   這是目前公認最好的預設。
//
// 【注意事項 Pay Attention】
//   1. detached thread 存取已銷毀的物件是【未定義行為】,沒有固定症狀,
//      不可寫成「一定會崩潰」。
//   2. 本檔的 operator=(DetachingThread&&) = default 在賦值目標仍
//      joinable 時會呼叫 std::terminate()(標準保證,本機實測 exit 134)。
//      示範 3 以 -DDEMONSTRATE_UB 隔離,預設不執行。
//   3. main() 用 sleep_for 等待 detached thread 是競態條件,不是同步;
//      本檔為了讓輸出可觀察而使用,實務不可依賴。
//   4. 要 detach 就必須讓執行緒自給自足:by-value 捕捉,共享狀態用
//      std::shared_ptr 延長生命週期。
//   5. 移動建構 = default 是安全的(目標沒有舊資源);只有移動賦值
//      需要顯式處理舊資源。這個不對稱性很容易被忽略。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】detach 的風險與 = default 的陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個類別持有 std::thread 成員,把移動賦值寫成 = default 有什麼問題?
//     答：= default 會逐成員移動,對 std::thread 成員就是呼叫
//         std::thread::operator=(thread&&)。標準規定:若賦值目標本身
//         仍是 joinable,就呼叫 std::terminate()。所以只要在「自己還
//         持有執行緒」時被移動賦值,程式就會終止 —— 這是標準保證的
//         確定結果,不是未定義行為,catch 也攔不到。
//     追問：那該怎麼寫?
//         → 顯式實作,先按自己的策略處置舊資源再接手:
//           if (this != &o) { if (t.joinable()) t.join()/t.detach();
//                             t = std::move(o.t); }
//           順帶一提,移動【建構】= default 是安全的,因為目標物件
//           還沒有舊資源要處置。這個不對稱性是 Rule of Five 的核心。
//
// 🔥 Q2. 什麼情況下 detach 是可以接受的?
//     答：只有當那條執行緒【完全自給自足】時 —— 不捕捉任何區域變數的
//         參考或指標,需要的共享狀態一律用 std::shared_ptr 值捕捉
//         (讓執行緒自己延長狀態的生命週期),而且不依賴 main() 還活著。
//         即使如此,你仍然沒有完成保證,所以工作必須是「掉了也沒關係」的。
//     追問：現代 C++ 有更好的選擇嗎?
//         → 幾乎總是有。需要結果用 std::future/std::async;需要背景常駐
//           用有明確關閉流程的 thread pool;需要可取消的背景工作用
//           C++20 的 std::jthread + stop_token。detach 在現代程式碼裡
//           已經很少是最佳解。
//
// ⚠️ 陷阱 1. 「detach 會不會造成資源洩漏?」
//     答：不會。pthread 的 detached 執行緒結束時,stack 與 descriptor
//         由 runtime 自動回收。detach 的問題是【正確性】(存取已銷毀
//         的物件、沒有完成保證),不是資源。反倒是「既不 join 也不
//         detach」才會在 std::thread 解構時 terminate。
//     為什麼會錯：把 detach 和 C 語言裡「忘記 free」的直覺混在一起。
//         兩者是完全不同類別的問題。
//
// ⚠️ 陷阱 2. 「detached thread 存取已銷毀的 std::cout,一定會 crash 吧?」
//     答：不一定。那是未定義行為,沒有固定症狀 —— 可能崩潰、可能輸出
//         消失、可能完全看不出異常。正因為它【可能看起來是對的】,
//         才比會當場 abort 的 std::terminate() 更難發現。
//     為什麼會錯：把「未定義行為」理解成「一定會出事的行為」。
//         標準的意思是「標準不對此做任何規定」,包含「看起來正常運作」
//         也是允許的結果之一。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>   // std::forward
#include <vector>

// -----------------------------------------------------------------------------
// 對照組:解構時自動 detach() 的執行緒守衛
//   ⚠️ 保留原始設計(含 = default 的移動賦值)以呈現陷阱,非推薦寫法。
// -----------------------------------------------------------------------------
class DetachingThread {
    std::thread t;

public:
    template<typename Func, typename... Args>
    explicit DetachingThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    // 移動建構 = default 是安全的:目標物件沒有舊資源要處置
    DetachingThread(DetachingThread&&) = default;

    // ⚠️ 移動賦值 = default 有 std::terminate() 風險:
    //    它會呼叫 std::thread::operator=,而目標若仍 joinable → terminate()
    DetachingThread& operator=(DetachingThread&&) = default;

    DetachingThread(const DetachingThread&) = delete;
    DetachingThread& operator=(const DetachingThread&) = delete;

    ~DetachingThread() {
        if (t.joinable()) {
            t.detach();
        }
    }
};

// -----------------------------------------------------------------------------
// 示範 1:基本行為 —— 解構立即返回,執行緒被放生
// -----------------------------------------------------------------------------
void demoDetachBasics() {
    std::cout << "  [main] 進入內層區塊\n";

    {
        DetachingThread dt([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "  [背景執行緒] 工作完成\n";
        });

        std::cout << "  [main] 離開內層區塊(dt 解構 → 自動 detach)\n";
    }  // dt 解構:執行緒仍在跑,被 detach 後繼續在背景執行

    std::cout << "  [main] 區塊已結束,但背景執行緒還在跑\n";

    // ⚠️ 用時間「猜」它做完了 —— 這是競態條件,不是同步。
    //    實務上正確的做法是 join;這裡只是為了讓輸出可觀察。
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << "  [main] 硬等 300ms 後才敢繼續\n";
}

// -----------------------------------------------------------------------------
// 示範 2:移動建構是安全的
// -----------------------------------------------------------------------------
void demoMoveConstruct() {
    auto make = []() {
        return DetachingThread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        });
    };

    DetachingThread moved = make();  // 移動建構(或 copy elision)
    std::cout << "  移動建構完成,程式正常存活\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
}

// -----------------------------------------------------------------------------
// 示範 3:移動賦值的 std::terminate() —— 預設不執行
//
//   a = std::move(b) 時,a.t 仍是 joinable,= default 的移動賦值會呼叫
//   std::thread::operator=,標準規定此時必須 std::terminate()。
//   這是【確定的結果】,不是未定義行為,catch 也攔不到。
//
//   觀察方式:
//     g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.2：執行緒守衛類別設計3.cpp" -o guard3_ub
//     ./guard3_ub
//   本機實測結果:terminate called without an active exception
//                 程式以 SIGABRT 結束,exit code 134
// -----------------------------------------------------------------------------
void demoMoveAssignTerminate() {
#ifdef DEMONSTRATE_UB
    DetachingThread a([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); });
    DetachingThread b([]() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); });

    std::cout << "  即將執行 a = std::move(b) …\n" << std::flush;
    a = std::move(b);   // ← 此處 std::terminate(),以下永遠不會執行
    std::cout << "  (這行永遠不會被印出)\n";
#else
    std::cout << "  已略過(預設不執行,以免整個程式 abort)。\n";
    std::cout << "  要親眼看到 terminate,請加上 -DDEMONSTRATE_UB 重新編譯。\n";
#endif
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)全部要求【確定的輸出順序】,
//   而本檔主題 detach 的核心性質正是「放棄完成保證」——
//   用 detach 解那些題目必然是錯的。兩者不但無交集,方向根本相反,
//   硬湊只會建立危險的錯誤聯想,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】背景快取預熱(cache warm-up)—— detach 唯一站得住腳的用法
//
//   情境:服務啟動後,想在背景把熱門商品資料預先載入快取,讓第一批
//         使用者不必等待。這個工作【掉了也沒關係】(頂多第一次查詢慢一點),
//         而且絕對不該拖慢啟動流程 —— 這正是 fire-and-forget 的定義。
//
//   關鍵:讓執行緒【自給自足】。
//     * 快取本體用 std::shared_ptr 持有,執行緒【值捕捉】那個 shared_ptr。
//       如此一來即使建立它的函式早已返回,快取物件仍因為引用計數而存活,
//       執行緒寫入的是合法記憶體,不是懸空指標。
//     * 不捕捉任何區域變數的參考,不依賴呼叫端的 stack frame。
//     * 進度用 std::atomic 回報,主執行緒可隨時查詢而不需要鎖。
//
//   對比:如果這裡改成 [&cache] 捕捉區域變數的參考,函式一返回就是
//         懸空參考 —— 未定義行為,而且很可能「測起來都正常」。
// -----------------------------------------------------------------------------
struct ProductCache {
    std::atomic<int>  loaded{0};
    std::atomic<bool> done{false};
    std::vector<std::string> entries;
    std::mutex        m;
};

// 啟動背景預熱後立即返回;回傳 shared_ptr 讓呼叫端可查詢進度
std::shared_ptr<ProductCache> startCacheWarmUp(int itemCount) {
    auto cache = std::make_shared<ProductCache>();

    {
        // 值捕捉 shared_ptr → 引用計數 +1 → 快取的生命週期由執行緒共同持有
        DetachingThread warmer([cache, itemCount]() {
            for (int i = 0; i < itemCount; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                {
                    std::lock_guard<std::mutex> lk(cache->m);
                    cache->entries.push_back("SKU-" + std::to_string(1000 + i));
                }
                cache->loaded.fetch_add(1, std::memory_order_relaxed);
            }
            cache->done.store(true, std::memory_order_release);
        });
    }  // warmer 解構 → detach → 函式立刻返回,預熱在背景繼續

    return cache;  // 呼叫端與背景執行緒共同持有,不會懸空
}

int main() {
    std::cout << "=== 示範 1:detach 的基本行為 ===\n";
    demoDetachBasics();

    std::cout << "\n=== 示範 2:移動建構是安全的 ===\n";
    demoMoveConstruct();

    std::cout << "\n=== 示範 3:移動賦值會 std::terminate() ===\n";
    demoMoveAssignTerminate();

    std::cout << "\n=== 日常實務:背景快取預熱(shared_ptr 自給自足)===\n";
    auto cache = startCacheWarmUp(20);
    std::cout << "  啟動函式已返回,主流程不被阻塞\n";

    // 主動輪詢直到完成(這是【真正的同步】,不是猜時間)
    while (!cache->done.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::cout << "  預熱完成,已載入筆數 = " << cache->loaded.load() << "\n";
    {
        std::lock_guard<std::mutex> lk(cache->m);
        std::cout << "  第一筆 = " << cache->entries.front()
                  << ",最後一筆 = " << cache->entries.back() << "\n";
    }
    std::cout << "  快取物件由 shared_ptr 共同持有,背景執行緒不會存取懸空記憶體\n";

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.2：執行緒守衛類別設計3.cpp" -o guard3
// 觀察移動賦值的 terminate(程式會 abort,exit code 134):
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.2：執行緒守衛類別設計3.cpp" -o guard3_ub

// 註:示範 1 依賴 sleep_for 讓 detached thread 有機會印出訊息,
//     這【沒有】任何同步保證,在負載較重的機器上可能觀察到不同結果。
//     相對地,最後的實務範例使用 atomic 旗標輪詢,那才是真正的同步。

// === 預期輸出 ===
// === 示範 1:detach 的基本行為 ===
//   [main] 進入內層區塊
//   [main] 離開內層區塊(dt 解構 → 自動 detach)
//   [main] 區塊已結束,但背景執行緒還在跑
//   [背景執行緒] 工作完成
//   [main] 硬等 300ms 後才敢繼續
//
// === 示範 2:移動建構是安全的 ===
//   移動建構完成,程式正常存活
//
// === 示範 3:移動賦值會 std::terminate() ===
//   已略過(預設不執行,以免整個程式 abort)。
//   要親眼看到 terminate,請加上 -DDEMONSTRATE_UB 重新編譯。
//
// === 日常實務:背景快取預熱(shared_ptr 自給自足)===
//   啟動函式已返回,主流程不被阻塞
//   預熱完成,已載入筆數 = 20
//   第一筆 = SKU-1000,最後一筆 = SKU-1019
//   快取物件由 shared_ptr 共同持有,背景執行緒不會存取懸空記憶體
//
// === 全部示範結束 ===
