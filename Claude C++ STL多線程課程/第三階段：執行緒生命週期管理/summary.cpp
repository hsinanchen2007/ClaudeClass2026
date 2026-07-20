// =============================================================================
//  第三階段：執行緒生命週期管理 — 總複習 summary.cpp
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是第三階段(3.1–3.6)的教科書式總整理,涵蓋六個主題:
//
//     3.1 RAII 與執行緒管理      ThreadGuard / ScopedThread / FlexibleThread
//     3.2 執行緒守衛類別設計      JoiningThread / ManagedThread<Policy>
//     3.3 std::jthread (C++20)   自動 join + stop_token 協作取消
//     3.4 執行緒例外處理          exception_ptr / future / promise
//     3.5 執行緒本地儲存          thread_local
//     3.6 執行緒安全的初始化      call_once / magic static
//
//   標準版本：本檔的【可執行部分】只用 C++11/14/17,以 -std=c++17 編譯零警告;
//             jthread / stop_token 的示範函式刻意保留為註解(需 C++20),
//             完整可執行版本請見「課程 3.3：std jthread (C++20)2–7」各檔。
//   標頭檔  ：<thread>、<mutex>、<future>、<exception>、<atomic>
//
// 【詳細解釋 Explanation】
//
// 【1. 貫穿整個第三階段的那一條規則】
//   一切都源自 std::thread 解構函式的這一行([thread.thread.destr]):
//
//       ~thread() { if (joinable()) std::terminate(); }
//
//   請務必精確理解它的性質:這是【標準明文規定的確定行為】,
//   不是未定義行為、不是「可能崩潰」,而且 catch 攔不到 ——
//   std::terminate() 根本不走例外機制。
//
//   為什麼標準要這麼激烈?因為另外兩個選項都更糟:
//     自動 join()   → 解構函式無預警阻塞,可能永遠不返回
//     自動 detach() → 執行緒存取已銷毀的區域變數,懸空參考
//   標準的判斷是:「立刻大聲失敗」勝過「安靜地做錯事」。
//   於是「必須明確選擇 join 或 detach」成了程式設計者的責任 ——
//   而第三階段的全部內容,都是在回答「怎麼讓這個責任不被遺忘」。
//
// 【2. 守衛類別的演進脈絡:每一步都在解決前一步的問題】
//   ┌────────────────┬──────────┬────────────┬──────────┬────────────────┐
//   │ 設計           │ 所有權   │ 可入容器   │ 解構策略 │ 它解決了什麼   │
//   ├────────────────┼──────────┼────────────┼──────────┼────────────────┤
//   │ ThreadGuard    │ 否(參考)│ 否         │ join     │ 例外跳過 join  │
//   │ ScopedThread   │ 是       │ 否(不可移)│ join     │ 宣告順序依賴   │
//   │ FlexibleThread │ 是       │ 否(不可移)│ join/det │ 策略需可選     │
//   │ JoiningThread  │ 是       │ 是(可移動)│ join     │ 動態數量管理   │
//   │ ManagedThread  │ 是       │ 是         │ 編譯期   │ 執行期分支成本 │
//   │ std::jthread   │ 是       │ 是         │ stop+join│ 全部,且標準化 │
//   └────────────────┴──────────┴────────────┴──────────┴────────────────┘
//   理解這條線比背下任何一個類別都重要:每個設計都是在回答
//   「前一個設計留下了什麼問題」。
//
// 【3. ⚠️ 三種「移動賦值到 joinable 目標」的規則,彼此完全不同】
//   這是本階段最容易被含混帶過、面試卻最愛問的一點:
//
//       std::thread   a = std::move(b);   → 【std::terminate()】程式死亡
//       JoiningThread a = std::move(b);   → 先 join 自己的(可能阻塞)再接手
//       std::jthread  a = std::move(b);   → request_stop() + join() 再接手,
//                                            程式【存活】
//
//   三者是【不同的規則】,不是「類似的行為」。特別注意 jthread 之所以
//   敢自動 join,是因為它同時提供了 stop_token 這條協作取消管道 ——
//   沒有那條管道,自動 join 就等於「可能永遠不返回」。
//
//   ⚠️ 由此延伸出一個實務陷阱:自己寫的類別若持有 std::thread 成員,
//   把移動賦值寫成 = default 會【逐成員移動】,直接踩到第一條規則
//   → std::terminate()。必須顯式實作,先按自己的策略處置舊資源。
//   (本階段「執行緒守衛類別設計3/4」有可實際觀察的重現路徑。)
//
// 【4. 例外處理的四種方案,以及各自的適用時機】
//   前提:例外機制以【執行緒】為單位。例外從執行緒進入函式逸出即
//   std::terminate(),包在 join() 外面的 try/catch 永遠攔不到。
//   所以必須有人【顯式地把例外物件搬過去】:
//
//     (a) 執行緒內就地捕獲     最簡單,但呼叫端完全感知不到失敗
//     (b) exception_ptr 成員   可跨執行緒傳遞任意型別,但同步要自己管
//     (c) std::future/async    推薦:自動裝箱、get() 自動 rethrow
//     (d) std::promise         最靈活:可精確控制何時、以什麼回覆
//
//   核心機制都是 std::current_exception() 把例外【型別抹除】成
//   exception_ptr,再由 std::rethrow_exception() 在另一條執行緒
//   以【原始型別】重新拋出 —— 自訂例外類別的額外欄位完整保留。
//
// 【5. thread_local:最好的同步是不需要同步】
//   面對共享可變狀態,你只有三個選擇:加鎖、改用 atomic、或【不共享】。
//   thread_local 是第三條路的語言層支援:每條執行緒一份獨立實體,
//   彼此是不同的物件(連位址都不同),因此不可能有 data race。
//   ⚠️ 但它解決不了 race condition —— 那是邏輯層的時序問題。
//   ⚠️ 也不可用它管理必須釋放的資源:detached thread 與 std::exit()
//      的情況下,thread_local 的解構【不保證】發生。
//
// 【6. 執行緒安全初始化:call_once vs magic static】
//   兩者都保證「只初始化一次,且其他執行緒會等到完成」:
//     * static 區域變數(magic static,C++11 起保證執行緒安全)
//       —— 單例模式首選,最簡潔,編譯器用 guard variable 實作。
//     * std::call_once + std::once_flag
//       —— 適合初始化邏輯不在建構函式裡、或需要傳參數的情況。
//       重要性質:若初始化函式【拋出例外】,once_flag【不會】被設定,
//       下一個執行緒會重新嘗試 —— 這讓「可重試的初始化」得以實作,
//       是它勝過 magic static 的關鍵場景。
//
// 【概念補充 Concept Deep Dive】
//
// (A) join() 本身就是同步點
//   join() 返回之後,子執行緒的所有寫入對呼叫端都保證可見 ——
//   兩者之間有 happens-before 關係([thread.thread.member])。
//   所以「join 完再讀子執行緒寫的變數」不需要任何 mutex 或 atomic。
//   這是初學者最常畫蛇添足的地方。
//
// (B) 為什麼守衛類別的解構函式不該讓例外逸出
//   C++11 起解構函式預設 noexcept(true)。若例外從中逸出,直接
//   std::terminate();若又發生在 stack unwinding 期間更是必死。
//   而 join() 確實可能丟 std::system_error。所以生產等級的守衛
//   應該寫成 try { ... } catch (...) { /* log */ } —— 吞例外不漂亮,
//   但比 terminate 好,這是解構函式的普遍兩難。
//
// (C) 為什麼 std::thread 不像 unique_ptr 那樣「解構就釋放」
//   因為「釋放執行緒」有兩種語意完全不同的做法(join 阻塞 / detach 懸空),
//   標準無法替你選;而 unique_ptr 的「釋放記憶體」只有一種意思。
//   標準的態度一貫是:模稜兩可時,要求你明講。
//
// (D) 執行緒不是越多越好
//   每條執行緒佔用核心資源與預設 8 MB 的 stack 虛擬位址空間。
//   CPU-bound 工作開超過核心數(本機 hardware_concurrency() 實測 16)
//   只會增加 context switch;I/O-bound 才可能受益於超額訂閱。
//   生產做法是固定大小的 thread pool 重複利用執行緒。
//
// 【注意事項 Pay Attention】
//   1. 解構 joinable 的 std::thread → std::terminate(),標準保證的確定
//      行為,不是未定義行為,catch 攔不到。
//   2. 持有 std::thread 成員的類別,移動賦值【不可】寫成 = default
//      (會逐成員移動 → 撞上 terminate 規則)。
//   3. std::thread / JoiningThread / std::jthread 的移動賦值規則
//      【三者各不相同】,不可混為一談。
//   4. 例外不會自動跨執行緒;包在 join() 外的 try/catch 攔不到。
//   5. thread_local 的解構在 detached thread 與 std::exit() 時不保證發生。
//   6. call_once 的初始化函式若拋例外,flag 不會被設定,可以重試;
//      magic static 亦然(下一個執行緒會重新嘗試初始化)。
//   7. 本檔的 jthread 示範函式刻意保留為註解(需 C++20);
//      可執行版本見「課程 3.3：std jthread (C++20)2–7」。
//   8. 本檔多處由多條執行緒直接輸出,行的先後順序【每次執行都不同】。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒生命週期管理(第三階段總覽)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個 joinable 的 std::thread 被解構會發生什麼?為什麼標準要這樣設計?
//     答：呼叫 std::terminate(),程式終止。這是標準明文規定的【確定行為】,
//         不是未定義行為,而且 catch 攔不到(terminate 不走例外機制)。
//         設計理由:自動 join 會讓解構函式無預警阻塞、可能永遠不返回;
//         自動 detach 會讓執行緒存取已銷毀的區域變數。兩者都比「立刻
//         大聲失敗」更難除錯,所以標準要求程式設計者明確做出選擇。
//     追問：那 C++20 的 jthread 為什麼就敢自動 join?
//         → 因為它同時內建 stop_token 協作取消管道。解構時先
//           request_stop() 再 join(),寫得好的工作函式會看到停止請求
//           並主動返回,那個 join 才不會無限期阻塞。是【先補上前提條件,
//           才敢做那件事】,不是單純放寬規則。
//
// 🔥 Q2. std::thread、JoiningThread、std::jthread 三者的移動賦值,
//        在「目標本身仍 joinable」時各自會怎樣?
//     答：三條規則完全不同。std::thread → std::terminate(),程式死亡;
//         JoiningThread(自訂)→ 先對自己持有的執行緒 join(可能阻塞)
//         再接手;std::jthread → 先 request_stop() 再 join(),然後接手,
//         程式【存活】。它們是不同的規則,不是「類似的行為」。
//     追問：所以自己寫的類別持有 std::thread 成員時,移動賦值能不能
//         寫成 = default?
//         → 絕對不行。= default 會逐成員移動,對 std::thread 成員就是
//           呼叫它的移動賦值 → 目標若仍 joinable 就 std::terminate()。
//           必須顯式實作:先按自己的策略處置舊資源,再接手來源。
//           順帶一提,移動【建構】= default 是安全的(目標還沒有舊資源),
//           這個不對稱正是 Rule of Five 的核心。
//
// 🔥 Q3. 子執行緒裡拋出的例外,怎麼讓主執行緒處理?
//     答：例外機制以執行緒為單位,例外從執行緒進入函式逸出即
//         std::terminate() —— 包在 join() 外面的 try/catch 永遠攔不到。
//         必須在子執行緒內 catch,用 std::current_exception() 裝箱成
//         std::exception_ptr,再透過 promise/future(或自訂成員 + join)
//         搬到主執行緒,由 std::rethrow_exception() 重新拋出。
//         實務上最推薦 std::async/future —— 它把整套流程包好了,
//         get() 會自動重新拋出,呼叫端的寫法與同步程式完全一樣。
//     追問：exception_ptr 會不會把例外降級成基底類別?
//         → 不會。它是型別抹除的持有者,保存的是例外物件本身(含動態
//           型別),rethrow_exception() 拋出的是原始型別,自訂例外類別的
//           額外欄位完整可取。
//
// ⚠️ 陷阱 1. 「join() 之後要讀子執行緒寫的變數,是不是該加個 mutex?」
//     答：不用,加了是多餘的。join() 的返回與子執行緒的結束之間有
//         happens-before 關係,子執行緒所有寫入在 join() 返回後都保證可見。
//     為什麼會錯：把「多執行緒」直接等同於「一律要同步原語」,
//         忽略了 join()(以及 thread 的建構)本身就是標準規定的同步點。
//         真正需要同步的是「兩條執行緒【同時存活期間】」的共享存取。
//
// ⚠️ 陷阱 2. 「thread_local 的物件一定會在執行緒結束時解構,
//              所以可以拿它做 RAII 管理資源。」
//     答：不可靠。解構只在執行緒【正常結束】時保證發生。被 detach 的
//         執行緒若在 main() 之後才被系統終止,解構不保證執行;呼叫
//         std::exit() 或 std::abort() 時,其他執行緒的 thread_local
//         解構完全不會執行。拿它管必須釋放的資源(檔案、連線)會漏。
//     為什麼會錯：把 thread_local 的解構保證等同於區域物件的 RAII 保證。
//         區域物件由作用域嚴格保證;thread_local 則依賴「執行緒正常結束」
//         這個並不總是成立的前提。
//
// ⚠️ 陷阱 3. 「call_once 的初始化函式丟了例外,那個 once_flag 就廢了吧?」
//     答：正好相反。標準規定:初始化函式若以例外結束,該次呼叫【不算】
//         完成,once_flag 不會被設定,下一個(或同一個)執行緒呼叫
//         call_once 時會【重新嘗試】執行。這正是它支援「可重試初始化」
//         的關鍵性質 —— 例如連線資料庫失敗後可以再試。
//     為什麼會錯：把 once_flag 想成「呼叫過就標記完成」的布林旗標。
//         它記錄的其實是「是否有一次【成功完成】的初始化」。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ============================================================
 * 【第三階段：執行緒生命週期管理】總複習 summary.cpp
 * ============================================================
 *
 * 本階段涵蓋課程：
 *   - 課程 3.1：RAII 與執行緒管理
 *   - 課程 3.2：執行緒守衛類別設計
 *   - 課程 3.3：std::jthread (C++20)
 *   - 課程 3.4：執行緒例外處理
 *   - 課程 3.5：執行緒本地儲存
 *   - 課程 3.6：執行緒安全的初始化
 *
 * ============================================================
 * 重點摘要：
 *
 *  3.1  RAII 原則：建構取得資源、解構釋放資源。
 *       std::thread 若在 joinable 狀態下被解構，會呼叫
 *       std::terminate() 使程式崩潰。RAII 包裝類別讓
 *       執行緒無論正常返回或拋出例外都能被安全回收。
 *
 *  3.2  完整的執行緒守衛類別需支援移動語意（move）、
 *       完美轉發（perfect forwarding），並禁止複製。
 *       移動賦值前須先對自身的 joinable 執行緒呼叫 join()。
 *       可使用編譯期模板參數（if constexpr）選擇 join/detach。
 *
 *  3.3  std::jthread（C++20）是標準化的 RAII 執行緒：
 *         - 解構時自動呼叫 request_stop() + join()
 *         - 內建協作式取消機制：stop_token / stop_source
 *         - stop_callback 可在停止時觸發回調
 *
 *  3.4  執行緒中拋出的例外不會自動跨執行緒傳遞。
 *       解決方案：
 *         a) 在執行緒內部捕獲（最簡單，但無法傳回呼叫者）
 *         b) std::exception_ptr + current_exception()
 *            + rethrow_exception()（可跨執行緒傳遞）
 *         c) std::future / std::async（推薦，自動處理）
 *         d) std::promise::set_exception()（彈性控制）
 *
 *  3.5  thread_local 關鍵字讓每個執行緒擁有獨立的變數副本，
 *       無需同步。初始化發生在執行緒首次存取時，銷毀在
 *       執行緒結束時。常見用途：錯誤碼、快取、隨機數產生器。
 *
 *  3.6  std::call_once + std::once_flag 確保初始化函式只
 *       被執行一次，即使多個執行緒同時競爭。若初始化函式
 *       拋出例外，flag 不會被設定，下一個執行緒會繼續嘗試。
 *       C++11 起，函式內的 static 區域變數初始化本身也是
 *       執行緒安全的，可作為單例模式的最簡實作。
 *
 * ============================================================
 * 編譯指令（C++17，含 C++20 的 jthread 部分需 C++20）：
 *
 *   g++ -std=c++17 -pthread -o summary summary.cpp
 *
 * 若編譯器支援 C++20：
 *   g++ -std=c++20 -pthread -o summary summary.cpp
 *
 * ============================================================
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <algorithm>
#include <vector>
#include <vector>


// ============================================================
// ===== 課程 3.1：RAII 與執行緒管理 =====
// ============================================================
//
// 【核心問題】
//   std::thread 在 joinable 狀態（即執行緒已啟動但尚未 join/detach）
//   下被解構時，C++ 標準規定直接呼叫 std::terminate()，
//   造成程式崩潰（abort）。
//
//   以下是典型的危險情境：
//     void riskyFunction() {
//         std::thread t([]{ std::cout << "工作中\n"; });
//         throw std::runtime_error("錯誤！");
//         t.join();  // 永遠不會執行！
//     }  // t 仍是 joinable → std::terminate() 被呼叫！
//
// 【RAII 原則】
//   Resource Acquisition Is Initialization（資源獲取即初始化）
//   - 建構函式：獲取資源（例如：接管執行緒）
//   - 解構函式：釋放資源（例如：join 或 detach 執行緒）
//   - 優點：無論正常返回或拋出例外，解構函式都一定被呼叫，
//           確保資源被正確釋放，不會洩漏。
//
// ------------------------------------------------------------
// 【方案一】ThreadGuard：持有 std::thread 的「引用」
//
//   優點：設計簡單，適合在現有 std::thread 上加上 RAII 保護。
//   缺點：必須確保被引用的 std::thread 物件的生命週期比
//         ThreadGuard 長，不適合放入容器。
// ------------------------------------------------------------

class ThreadGuard {
    std::thread& t;  // 持有引用（非擁有）

public:
    // explicit 防止隱式轉換
    explicit ThreadGuard(std::thread& thread) : t(thread) {}

    ~ThreadGuard() {
        // 若執行緒仍是 joinable，就 join 它
        // joinable() 為 false 的情況：已被 join、已被 detach、
        // 或是預設建構的空執行緒
        if (t.joinable()) {
            t.join();
        }
    }

    // 禁止複製——複製後兩個 guard 都會嘗試 join 同一個執行緒，
    // 第二次 join 會因為不再 joinable 而拋出例外
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// 使用示範
void demo_ThreadGuard() {
    std::thread t([]() {
        std::cout << "[ThreadGuard] 執行緒工作完成" << std::endl;
    });

    ThreadGuard guard(t);  // 保證 t 會被 join

    // 即使以下這行拋出例外，guard 的解構函式仍會執行並 join
    // throw std::runtime_error("測試例外");

    // 正常返回：guard 解構 → 自動 join t
}


// ------------------------------------------------------------
// 【方案二】ScopedThread：擁有（owns）std::thread
//
//   移動語意：透過 std::move 接管 std::thread 的所有權。
//   優點：執行緒與 ScopedThread 綁定，不會有懸空引用問題。
//         建構時立即驗證執行緒是否有效（joinable），
//         若無效就早失敗（fail fast），避免後續問題。
//   缺點：禁止複製，但可以加上移動支援使其放入容器。
// ------------------------------------------------------------

class ScopedThread {
    std::thread t;  // 擁有執行緒（不是引用）

public:
    // 接受一個右值 std::thread，移動進來
    explicit ScopedThread(std::thread thread)
        : t(std::move(thread))
    {
        // 立即檢查：如果傳入的是空執行緒（未關聯任何執行緒），
        // 則提前拋出例外，讓問題在建構時暴露
        if (!t.joinable()) {
            throw std::logic_error("ScopedThread：傳入的執行緒無效（not joinable）");
        }
    }

    ~ScopedThread() {
        // 因建構時已驗證，此處一定是 joinable，直接 join
        t.join();
    }

    // 禁止複製（std::thread 本身就不可複製）
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

void demo_ScopedThread() {
    // 直接在建構時傳入匿名執行緒，省去手動管理
    ScopedThread st(std::thread([]() {
        std::cout << "[ScopedThread] 安全的執行緒，自動 join" << std::endl;
    }));
    // 離開此函式時，st 解構 → 自動 join
}


// ------------------------------------------------------------
// 【方案三】FlexibleThread：可選擇 join 或 detach
//
//   某些情況下，我們希望在作用域結束時自動 detach（讓執行緒
//   在背景繼續執行），而不是 join（等待完成）。
//   FlexibleThread 讓使用者在建構時指定行為。
// ------------------------------------------------------------

class FlexibleThread {
public:
    // 使用強型別列舉（enum class）避免整數隱式轉換的混淆
    enum class Action { join, detach };

private:
    std::thread t;
    Action action;

public:
    FlexibleThread(std::thread thread, Action a)
        : t(std::move(thread)), action(a) {}

    ~FlexibleThread() {
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

void demo_FlexibleThread() {
    // 這個執行緒在作用域結束時會被 join（等待完成）
    FlexibleThread ft1(
        std::thread([]() { std::cout << "[FlexibleThread] join 我" << std::endl; }),
        FlexibleThread::Action::join
    );

    // 這個執行緒在作用域結束時會被 detach（背景繼續）
    FlexibleThread ft2(
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // 注意：detach 後若主程式已退出，此輸出可能看不到
        }),
        FlexibleThread::Action::detach
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}


// ============================================================
// ===== 課程 3.2：執行緒守衛類別設計 =====
// ============================================================
//
// 【設計目標】
//   一個完善的執行緒守衛類別應具備：
//     1. 自動在解構時 join（或 detach）
//     2. 支援移動語意——可以轉移所有權，可放入容器
//     3. 禁止複製——避免雙重 join
//     4. 完美轉發——直接在建構時傳入可呼叫物件與參數
//     5. 提供 join()、detach()、joinable()、get_id() 等工具方法
//
// 【完美轉發（Perfect Forwarding）】
//   template<typename Func, typename... Args>
//   JoiningThread(Func&& f, Args&&... args)
//       : t(std::forward<Func>(f), std::forward<Args>(args)...)
//   {}
//
//   std::forward 保留參數的值類別（左值/右值），
//   避免不必要的複製，並將參數「完美地」傳遞給 std::thread 的建構子。
//
// 【移動賦值的注意事項】
//   JoiningThread& operator=(JoiningThread&& other) noexcept {
//       if (this != &other) {
//           if (t.joinable()) {
//               t.join();  // ← 先處理自身管理的執行緒！
//           }              //   否則被覆蓋時就會 terminate
//           t = std::move(other.t);
//       }
//       return *this;
//   }
// ------------------------------------------------------------

class JoiningThread {
    std::thread t;

public:
    // 預設建構：不管理任何執行緒（t 是空的）
    JoiningThread() noexcept = default;

    // 從一個已存在的 std::thread 移動建構（接管所有權）
    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}

    // 直接建構執行緒（完美轉發可呼叫物件與所有參數）
    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    // 移動建構：從另一個 JoiningThread 搬走執行緒
    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}

    // 移動賦值：先 join 自己的執行緒，再接管別人的
    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {
            if (t.joinable()) {
                t.join();  // 重要：先結束自己管理的執行緒
            }
            t = std::move(other.t);
        }
        return *this;
    }

    // 禁止複製
    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;

    // 解構：自動 join
    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }

    // --- 工具方法（委派給底層 std::thread）---
    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }
    void join() { t.join(); }
    void detach() { t.detach(); }
    std::thread& get() noexcept { return t; }
};

void demo_JoiningThread() {
    // 方式一：直接在建構時傳入 lambda（完美轉發）
    JoiningThread jt1([]() {
        std::cout << "[JoiningThread] 方式一：直接建構" << std::endl;
    });

    // 方式二：從已存在的 std::thread 移動
    std::thread raw([]() {
        std::cout << "[JoiningThread] 方式二：從 std::thread 移動" << std::endl;
    });
    JoiningThread jt2(std::move(raw));
    // 注意：raw 移動後成為空執行緒，不可再使用

    // 方式三：帶參數
    JoiningThread jt3([](int x, const std::string& s) {
        std::cout << "[JoiningThread] 方式三：帶參數 x=" << x
                  << " s=" << s << std::endl;
    }, 42, std::string("hello"));

    // 所有 JoiningThread 在此函式結束時自動 join
}

void demo_JoiningThread_in_vector() {
    // 因為 JoiningThread 支援移動語意，可以放入 vector
    std::vector<JoiningThread> threads;

    for (int i = 0; i < 4; ++i) {
        // emplace_back 使用完美轉發，直接在 vector 內部建構
        threads.emplace_back([i]() {
            std::cout << "[Vector] Worker " << i << " 執行中" << std::endl;
        });
    }

    std::cout << "[Vector] 所有執行緒已建立，等待結束..." << std::endl;
    // vector 解構時，每個 JoiningThread 的解構函式自動 join
}


// ------------------------------------------------------------
// 【DetachingThread】：解構時自動 detach 的變體
//
//   適合「發射後不管」（fire-and-forget）的背景任務。
//   注意：detach 後若主程式退出，未完成的執行緒會被強制結束，
//   可能導致資源未釋放，使用時需謹慎。
// ------------------------------------------------------------

class DetachingThread {
    std::thread t;

public:
    template<typename Func, typename... Args>
    explicit DetachingThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    // 允許移動
    DetachingThread(DetachingThread&&) = default;
    DetachingThread& operator=(DetachingThread&&) = default;

    DetachingThread(const DetachingThread&) = delete;
    DetachingThread& operator=(const DetachingThread&) = delete;

    ~DetachingThread() {
        if (t.joinable()) {
            t.detach();  // 解構時自動 detach，不等待
        }
    }
};


// ------------------------------------------------------------
// 【ManagedThread】：以模板參數在編譯期決定行為
//
//   使用 if constexpr（C++17）在編譯期選擇分支，
//   沒有任何執行期開銷。使用型別別名增加可讀性。
// ------------------------------------------------------------

enum class ThreadAction { Join, Detach };

template<ThreadAction action>
class ManagedThread {
    std::thread t;

public:
    template<typename Func, typename... Args>
    explicit ManagedThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    ManagedThread(ManagedThread&&) = default;
    ManagedThread& operator=(ManagedThread&&) = default;

    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;

    ~ManagedThread() {
        if (t.joinable()) {
            if constexpr (action == ThreadAction::Join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
};

// 型別別名：讓使用者不用每次寫完整的模板語法
using AutoJoinThread   = ManagedThread<ThreadAction::Join>;
using AutoDetachThread = ManagedThread<ThreadAction::Detach>;

void demo_ManagedThread() {
    AutoJoinThread jt([]() {
        std::cout << "[ManagedThread] AutoJoinThread：離開作用域時自動 join" << std::endl;
    });
    // jt 解構時自動 join
}


// ============================================================
// ===== 課程 3.3：std::jthread (C++20) =====
// ============================================================
//
// 【std::jthread 是什麼？】
//   C++20 引入的 std::jthread（joining thread）是 std::thread
//   的改良版，內建兩大功能：
//     1. RAII 自動 join：解構時自動呼叫 request_stop() + join()
//     2. 協作式取消機制：內建 stop_token / stop_source 系統
//
// 【std::thread vs std::jthread 對比】
//
//   std::thread                    std::jthread
//   ─────────────────────          ─────────────────────────────
//   必須手動呼叫 join/detach        解構時自動 join（永不忘記）
//   忘記 join → std::terminate()   不會有這個問題
//   無內建取消機制                 內建 stop_token 協作式取消
//   C++11 起可用                   C++20 起可用
//
// 【stop_token 機制三元件】
//   std::stop_source  → 持有者，可以發出停止請求
//   std::stop_token   → 觀察者，檢查是否已收到停止請求
//   std::stop_callback → 當停止被請求時，自動執行指定的回調函式
//
// 【jthread 的完整介面（概念性）】
//   class jthread {
//   public:
//       jthread() noexcept;
//       template<typename F, typename... Args>
//       explicit jthread(F&& f, Args&&... args);
//       ~jthread();  // 自動 request_stop() + join()
//       jthread(jthread&&) noexcept;
//       jthread& operator=(jthread&&) noexcept;
//       stop_source get_stop_source() noexcept;
//       stop_token get_stop_token() const noexcept;
//       bool request_stop() noexcept;
//       bool joinable() const noexcept;
//       void join();
//       void detach();
//       id get_id() const noexcept;
//       static unsigned int hardware_concurrency() noexcept;
//   };
// ------------------------------------------------------------

// 【jthread 基本用法示範】
// （此函式使用 std::jthread，需 C++20 編譯）
//
// void demo_jthread_basic() {
//     std::jthread jt([]() {
//         std::cout << "[jthread] Hello from jthread！" << std::endl;
//     });
//     // 不需要呼叫 join()！
//     // 離開作用域時，jt 解構 → 自動 join
// }

// 【jthread 例外安全示範】
// 即使函式拋出例外，jthread 也能正確 join
//
// void demo_jthread_exception_safe() {
//     std::jthread jt([]() {
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         std::cout << "[jthread] 執行緒完成（即使發生例外）" << std::endl;
//     });
//     throw std::runtime_error("例外！");
//     // jt 解構時會先等待執行緒結束，再傳播例外
// }

// 【stop_token 協作式取消示範】
// 執行緒函式的第一個參數若為 std::stop_token，
// jthread 會自動注入它的 stop_token
//
// void demo_jthread_stop_token() {
//     std::jthread jt([](std::stop_token stoken) {
//         int count = 0;
//         while (!stoken.stop_requested()) {
//             std::cout << "[jthread] 工作中... " << ++count << std::endl;
//             std::this_thread::sleep_for(std::chrono::milliseconds(200));
//         }
//         std::cout << "[jthread] 收到停止請求，結束" << std::endl;
//     });
//
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     jt.request_stop();  // 請求停止
//     // jt 解構時先確認執行緒已結束再返回
// }

// 【stop_callback 示範】
// 停止被請求時，自動執行回調（不需要執行緒主動輪詢）
//
// void demo_jthread_stop_callback() {
//     std::jthread jt([](std::stop_token stoken) {
//         // 當停止被請求時，callback 會被立即觸發
//         std::stop_callback cb(stoken, []() {
//             std::cout << "[jthread] 停止回調：正在清理資源..." << std::endl;
//         });
//
//         while (!stoken.stop_requested()) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(300));
//         }
//         std::cout << "[jthread] 執行緒結束" << std::endl;
//     });
//
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     jt.request_stop();
// }

// 【jthread 存放於 vector 的示範】
// void demo_jthread_vector() {
//     std::vector<std::jthread> workers;
//
//     for (int i = 0; i < 3; ++i) {
//         workers.emplace_back([](std::stop_token stoken, int id) {
//             std::cout << "Worker " << id << " 啟動" << std::endl;
//             int count = 0;
//             while (!stoken.stop_requested()) {
//                 ++count;
//                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
//             }
//             std::cout << "Worker " << id << " 結束，執行了 " << count << " 次" << std::endl;
//         }, i);
//     }
//
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     // 逐一請求停止
//     for (auto& w : workers) {
//         w.request_stop();
//     }
//     // vector 解構時，每個 jthread 自動 join
// }

// 【何時用 jthread vs thread】
// C++20 可用時    → 優先使用 jthread（更安全、功能更豐富）
// 需要取消機制    → 使用 jthread（內建 stop_token）
// 需要 detach     → 使用 thread（jthread 不支援後台 detach）
// 舊版本相容性    → 使用 thread + 自訂守衛類別（如 JoiningThread）


// ============================================================
// ===== 課程 3.4：執行緒例外處理 =====
// ============================================================
//
// 【核心問題】
//   執行緒是獨立的執行環境，每個執行緒有自己的呼叫堆疊。
//   在執行緒 A 中拋出的例外，不會自動傳遞到執行緒 B。
//   若例外在執行緒函式內未被捕獲，C++ 會呼叫 std::terminate()，
//   導致整個程式崩潰（不只是那個執行緒）。
//
//   【錯誤示範】
//     void worker() { throw std::runtime_error("錯誤！"); }
//     int main() {
//         std::thread t(worker);
//         try {
//             t.join();
//         } catch (...) { /* 捕獲不到！*/ }
//     }
//     → 程式因未捕獲例外而 abort
//
// 【解決方案 A】執行緒內部捕獲（最簡單）
//   適合：錯誤在執行緒內部可以完全處理的情況
//   缺點：呼叫端無法得知執行緒是否發生錯誤
// ------------------------------------------------------------

void demo_exception_internal_catch() {
    auto worker = []() {
        try {
            throw std::runtime_error("執行緒內的錯誤！");
        } catch (const std::exception& e) {
            // 在執行緒內部處理，外部無法感知
            std::cout << "[方案A] 執行緒內捕獲：" << e.what() << std::endl;
        }
    };

    std::thread t(worker);
    t.join();
    std::cout << "[方案A] 主執行緒：程式正常繼續" << std::endl;
}


// ------------------------------------------------------------
// 【解決方案 B】std::exception_ptr 跨執行緒傳遞例外
//
//   std::exception_ptr 是一個可以儲存任何例外的智慧指標。
//   關鍵函式：
//     std::current_exception()  → 在 catch 區塊中，捕獲當前例外
//                                  並回傳 exception_ptr
//     std::rethrow_exception()  → 重新拋出 exception_ptr 所儲存的例外
//     std::make_exception_ptr() → 直接從例外物件建立 exception_ptr
//
//   流程：
//     1. 執行緒內部：catch → current_exception() → 存入共享變數
//     2. 主執行緒：join 後 → 檢查共享變數 → rethrow_exception()
// ------------------------------------------------------------

void demo_exception_ptr() {
    std::exception_ptr exPtr = nullptr;  // 共享的例外儲存位置

    auto worker = [&exPtr]() {
        try {
            throw std::runtime_error("來自工作執行緒的錯誤！");
        } catch (...) {
            // 捕獲所有例外，儲存到 exPtr
            exPtr = std::current_exception();
        }
    };

    std::thread t(worker);
    t.join();  // 等待執行緒完成

    // 現在可以安全地在主執行緒中處理例外
    if (exPtr) {
        try {
            std::rethrow_exception(exPtr);  // 重新拋出例外
        } catch (const std::exception& e) {
            std::cout << "[方案B] 主執行緒捕獲例外：" << e.what() << std::endl;
        }
    }
}


// ------------------------------------------------------------
// 【SafeThread】：封裝例外傳遞的 RAII 類別
//
//   將「例外儲存」的邏輯封裝在類別中，讓使用者透過 join()
//   自動感知執行緒中的例外，使用方式接近同步函式呼叫。
// ------------------------------------------------------------

class SafeThread {
    std::thread t;
    std::exception_ptr exPtr = nullptr;

public:
    template<typename Func, typename... Args>
    explicit SafeThread(Func&& f, Args&&... args) {
        // 包裝原本的函式，在外層加上例外捕獲
        t = std::thread([this,
                         func = std::forward<Func>(f)]() mutable {
            try {
                func();
            } catch (...) {
                exPtr = std::current_exception();
            }
        });
    }

    // join 後若有例外，重新拋出（讓呼叫端感知）
    void join() {
        t.join();
        if (exPtr) {
            std::rethrow_exception(exPtr);
        }
    }

    ~SafeThread() {
        if (t.joinable()) {
            t.join();  // RAII：確保執行緒被 join
        }
    }
};

void demo_SafeThread() {
    try {
        SafeThread st([]() {
            throw std::runtime_error("SafeThread 內的錯誤！");
        });
        st.join();  // 例外在此被重新拋出
    } catch (const std::exception& e) {
        std::cout << "[SafeThread] 主執行緒捕獲：" << e.what() << std::endl;
    }
}


// ------------------------------------------------------------
// 【解決方案 C】std::future / std::async（推薦方式）
//
//   std::async 自動在另一個執行緒執行函式，並將結果（或例外）
//   儲存在 std::future 中。呼叫 future.get() 時：
//     - 若成功：返回結果值
//     - 若例外：重新拋出例外
//   整個過程自動完成，無需手動管理 exception_ptr。
//
//   std::launch::async → 強制在新執行緒執行（不延遲）
//   std::launch::deferred → 延遲執行（直到 get() 被呼叫）
// ------------------------------------------------------------

int task_that_may_fail(int id) {
    if (id == 2) {
        throw std::runtime_error("Task 2 失敗（模擬錯誤）");
    }
    return id * 10;
}

void demo_future_exception() {
    std::vector<std::future<int>> futures;

    // 啟動多個非同步任務
    for (int i = 0; i < 5; ++i) {
        futures.push_back(
            std::async(std::launch::async, task_that_may_fail, i)
        );
    }

    // 收集結果（例外在 get() 時被重新拋出）
    for (int i = 0; i < 5; ++i) {
        try {
            int result = futures[i].get();
            std::cout << "[future] Task " << i << " 結果：" << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[future] Task " << i << " 例外：" << e.what() << std::endl;
        }
    }
}


// ------------------------------------------------------------
// 【解決方案 D】std::promise 精確控制例外傳遞
//
//   std::promise 允許在任意時間點設定值或例外，
//   搭配 std::future 可以靈活地在執行緒間傳遞結果。
//
//   prom.set_value(42)              → 設定成功結果
//   prom.set_exception(exPtr)       → 設定例外（當 future.get()
//                                      時會重新拋出）
// ------------------------------------------------------------

void promise_worker(std::promise<int> prom) {
    try {
        throw std::runtime_error("Promise worker 發生錯誤！");
        prom.set_value(42);  // 若無例外，設定結果
    } catch (...) {
        // 將例外傳遞給 future
        prom.set_exception(std::current_exception());
    }
}

void demo_promise_exception() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    std::thread t(promise_worker, std::move(prom));

    try {
        int value = fut.get();  // 等待並取得結果（或重新拋出例外）
        std::cout << "[promise] 結果：" << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "[promise] 主執行緒捕獲：" << e.what() << std::endl;
    }

    t.join();
}


// ============================================================
// ===== 課程 3.5：執行緒本地儲存 =====
// ============================================================
//
// 【什麼是 thread_local？】
//   thread_local 關鍵字（C++11 起）使每個執行緒擁有獨立的
//   變數副本。各執行緒對自己的副本進行讀寫，互不干擾，
//   因此不需要任何同步（mutex/atomic）。
//
// 【變數儲存類型比較】
//
//   全域/static 變數         thread_local 變數
//   ────────────────────    ────────────────────────────
//   所有執行緒共享一份        每個執行緒各有一份
//   需要 mutex 等同步保護     不需要同步（各自獨立）
//   程式啟動時初始化          首次存取時（執行緒開始後）初始化
//   程式結束時銷毀            執行緒結束時銷毀（呼叫解構函式）
//
// 【thread_local 可用的三個位置】
//   1. 全域變數：thread_local int g_tl = 0;
//   2. 函式內的 static 變數：thread_local static int f_tl = 0;
//   3. 類別的 static 成員：thread_local static int cls_tl;
//
// 【生命週期】
//   執行緒啟動
//     → 首次存取時初始化（若為複雜物件，呼叫建構函式）
//     → 執行緒執行期間持續存在
//     → 執行緒結束時銷毀（呼叫解構函式，注意析構順序）
// ------------------------------------------------------------

// 全域 thread_local 變數：每個執行緒有獨立的 counter
thread_local int tl_counter = 0;

void demo_thread_local_basic() {
    auto increment = [](const std::string& name) {
        for (int i = 0; i < 3; ++i) {
            ++tl_counter;
            std::cout << "[thread_local] " << name
                      << ": counter = " << tl_counter << std::endl;
        }
    };

    std::thread t1(increment, "Thread A");
    std::thread t2(increment, "Thread B");

    t1.join();
    t2.join();

    // 兩個執行緒的 counter 各自從 0 開始，互不影響
    // 預期輸出：A 和 B 各自計數到 3（但輸出順序不定）
}


// 對比：普通全域變數（共享）vs thread_local（獨立）
int global_counter = 0;              // 所有執行緒共享，有競爭條件
thread_local int local_tl_counter = 0;  // 每個執行緒獨立

void demo_global_vs_thread_local() {
    auto work = [](const std::string& name) {
        ++global_counter;   // 非原子操作，有競爭條件（僅示範用）
        ++local_tl_counter;

        std::cout << "[對比] " << name
                  << " global=" << global_counter
                  << " local=" << local_tl_counter << std::endl;
    };

    std::thread t1(work, "A");
    std::thread t2(work, "B");
    std::thread t3(work, "C");

    t1.join();
    t2.join();
    t3.join();

    // global_counter 會是 3（三個執行緒各加一次，但有競爭）
    // local_tl_counter 每個執行緒都是 1（各自獨立）
}


// 【用途一】執行緒專屬錯誤碼（類似 POSIX errno）
//   errno 就是用 thread_local 實作的，確保每個執行緒
//   設定和讀取自己的錯誤碼，不互相干擾。
thread_local int tl_last_error = 0;

void setThreadError(int code) { tl_last_error = code; }
int  getThreadError() { return tl_last_error; }

void demo_thread_local_error_code() {
    auto worker = [](int id) {
        setThreadError(id * 100);
        std::cout << "[errno] Thread " << id
                  << " error: " << getThreadError() << std::endl;
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();
    // Thread 1 → error: 100，Thread 2 → error: 200，互不影響
}


// 【用途二】執行緒專屬快取
//   每個執行緒有自己的快取，避免快取競爭（cache contention）
//   和同步開銷，可以顯著提高效能。
thread_local std::string tl_cache;

std::string getComputedResult(int thread_id) {
    if (tl_cache.empty()) {
        // 模擬耗時計算（每個執行緒只計算一次）
        tl_cache = "Thread-" + std::to_string(thread_id) + "-result";
        std::cout << "[cache] Thread " << thread_id << " 首次計算..." << std::endl;
    }
    return tl_cache;  // 後續呼叫直接從快取返回
}

void demo_thread_local_cache() {
    auto worker = [](int id) {
        // 呼叫兩次：第一次計算，第二次從快取取
        std::cout << "[cache] " << getComputedResult(id) << std::endl;
        std::cout << "[cache] " << getComputedResult(id) << " (快取)" << std::endl;
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();
}


// 【用途三】執行緒專屬隨機數產生器
//   std::mt19937 不是執行緒安全的（且使用 mutex 共享一個產生器
//   會成為效能瓶頸）。使用 thread_local 讓每個執行緒有自己的
//   隨機數引擎，既安全又高效。
thread_local std::mt19937 tl_rng{std::random_device{}()};

int threadSafeRandInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(tl_rng);  // 每個執行緒使用自己的 rng，無需同步
}

void demo_thread_local_rng() {
    auto worker = [](int id) {
        std::cout << "[rng] Thread " << id << ": "
                  << threadSafeRandInt(1, 100) << ", "
                  << threadSafeRandInt(1, 100) << ", "
                  << threadSafeRandInt(1, 100) << std::endl;
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();
}


// 【thread_local 物件的生命週期展示】
class TlResource {
    int id;
public:
    TlResource(int i) : id(i) {
        std::cout << "[TlResource] Thread " << id << " Resource 建構" << std::endl;
    }
    ~TlResource() {
        std::cout << "[TlResource] Thread " << id << " Resource 銷毀" << std::endl;
    }
};

// 注意：thread_local 物件在執行緒首次存取時建構，執行緒結束時銷毀
// thread_local TlResource tl_res{0};  // 每個執行緒有自己的 TlResource
// （此處注釋掉以避免在主執行緒啟動時就建構）


// ============================================================
// ===== 課程 3.6：執行緒安全的初始化 =====
// ============================================================
//
// 【核心問題：競爭條件（Race Condition）下的初始化】
//   當多個執行緒同時執行 "if (ptr == nullptr) { ptr = new X(); }"
//   時，可能有多個執行緒都通過了 nullptr 檢查，導致物件被
//   初始化多次，造成記憶體洩漏或未定義行為。
//
//   這是「雙重檢查鎖定（Double-Checked Locking）」的問題根源。
//   在 C++11 記憶體模型出現前，即使加了鎖，DCL 也可能因
//   編譯器/CPU 重排而失效。
//
// 【解決方案一】std::call_once + std::once_flag
//
//   std::once_flag flag;
//   void initDB() { db = new Database(); }
//   void use() {
//       std::call_once(flag, initDB);  // 保證只執行一次
//       db->query();
//   }
//
//   工作原理：
//     - 第一個到達的執行緒執行初始化函式
//     - 其他執行緒等待直到初始化函式完成
//     - 之後所有呼叫直接略過（flag 已設定）
//     - 若初始化函式拋出例外：
//         * flag 不會被設定
//         * 例外傳播給當前執行緒
//         * 下一個執行緒會再次嘗試初始化
//
// 【解決方案二】函式內 static 區域變數（C++11 保證安全）
//   C++11 標準明確規定：static 區域變數的初始化是執行緒安全的。
//   編譯器/runtime 會自動確保只有一個執行緒執行初始化。
// ------------------------------------------------------------

// --- 不安全的初始化（示範問題，請勿在生產環境使用）---
// Database* db = nullptr;
// void unsafe_init() {
//     if (db == nullptr) {      // 多個執行緒可能同時通過此檢查！
//         db = new Database();  // 可能被初始化多次！
//     }
// }


// --- call_once 示範 ---

class Database {
    int id;
public:
    Database(int i = 0) : id(i) {
        std::cout << "[Database] 初始化（id=" << id << "）" << std::endl;
    }
    void query(int thread_id) const {
        std::cout << "[Database] Thread " << thread_id
                  << " 查詢中（db id=" << id << "）" << std::endl;
    }
};

Database* g_db = nullptr;
std::once_flag g_db_init_flag;

void demo_call_once_basic() {
    auto initDatabase = []() {
        g_db = new Database(42);
    };

    auto initAndUse = [&](int thread_id) {
        // 無論多少執行緒同時呼叫，initDatabase 只會被執行一次
        std::call_once(g_db_init_flag, initDatabase);
        g_db->query(thread_id);
    };

    std::thread t1(initAndUse, 1);
    std::thread t2(initAndUse, 2);
    std::thread t3(initAndUse, 3);

    t1.join();
    t2.join();
    t3.join();

    delete g_db;
    g_db = nullptr;
}


// --- call_once 與例外：失敗後重試 ---

std::once_flag retry_flag;
std::atomic<int> attempt_count{0};

void mayFailInit() {
    int current = ++attempt_count;
    std::cout << "[call_once] 嘗試 #" << current << std::endl;

    if (current < 3) {
        throw std::runtime_error("初始化失敗（模擬）");
    }

    std::cout << "[call_once] 初始化成功！" << std::endl;
}

void demo_call_once_with_exception() {
    auto worker = []() {
        try {
            // 若 mayFailInit 拋出例外，flag 不會被設定，
            // 下一個執行緒會再次嘗試
            std::call_once(retry_flag, mayFailInit);
            std::cout << "[call_once] 繼續執行..." << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[call_once] 捕獲例外：" << e.what() << std::endl;
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}


// --- 執行緒安全的單例模式（Singleton）---
// 【方法一】使用 call_once（明確控制初始化）

class SingletonV1 {
    static SingletonV1* instance;
    static std::once_flag init_flag;

    SingletonV1() {
        std::cout << "[Singleton v1] 建立（call_once 版本）" << std::endl;
    }

public:
    SingletonV1(const SingletonV1&) = delete;
    SingletonV1& operator=(const SingletonV1&) = delete;

    static SingletonV1& getInstance() {
        std::call_once(init_flag, []() {
            instance = new SingletonV1();
        });
        return *instance;
    }

    void doWork() {
        std::cout << "[Singleton v1] 執行工作" << std::endl;
    }
};

SingletonV1* SingletonV1::instance = nullptr;
std::once_flag SingletonV1::init_flag;


// 【方法二】使用 static 區域變數（推薦，C++11 保證執行緒安全）
//   C++11 標準 §6.7 規定：若多個執行緒同時進入 static 區域變數
//   的宣告，初始化只會由一個執行緒執行，其他執行緒等待完成。

class SingletonV2 {
    SingletonV2() {
        std::cout << "[Singleton v2] 建立（static 局部變數版本）" << std::endl;
    }

public:
    SingletonV2(const SingletonV2&) = delete;
    SingletonV2& operator=(const SingletonV2&) = delete;

    static SingletonV2& getInstance() {
        static SingletonV2 instance;  // C++11 保證執行緒安全！
        return instance;
    }

    void doWork() {
        std::cout << "[Singleton v2] 執行工作" << std::endl;
    }
};


// 【延遲初始化（Lazy Initialization）封裝示範】
//   使用 call_once 實作成員的延遲初始化——只在第一次需要時才建構，
//   並且保證多執行緒環境下只建構一次。

class ServiceWithLazyCache {
    mutable std::once_flag cache_flag;
    mutable std::unique_ptr<std::string> cache;

    void initCache() const {
        std::cout << "[LazyCache] 正在初始化快取..." << std::endl;
        cache = std::make_unique<std::string>("快取資料（只初始化一次）");
    }

public:
    // const 方法：邏輯上不修改物件，但允許延遲初始化
    // 使用 mutable 讓 const 方法能修改 cache_flag 和 cache
    const std::string& getCache() const {
        std::call_once(cache_flag, &ServiceWithLazyCache::initCache, this);
        return *cache;
    }
};

void demo_lazy_init() {
    ServiceWithLazyCache service;

    std::thread t1([&]() {
        std::cout << "[LazyCache] Thread 1: " << service.getCache() << std::endl;
    });
    std::thread t2([&]() {
        std::cout << "[LazyCache] Thread 2: " << service.getCache() << std::endl;
    });

    t1.join();
    t2.join();
    // initCache() 只被呼叫一次，兩個執行緒共用同一份快取
}

void demo_singleton() {
    // 方法一：call_once 版本
    std::thread ta([]() { SingletonV1::getInstance().doWork(); });
    std::thread tb([]() { SingletonV1::getInstance().doWork(); });
    ta.join();
    tb.join();

    // 方法二：static 區域變數版本（更簡潔）
    std::thread tc([]() { SingletonV2::getInstance().doWork(); });
    std::thread td([]() { SingletonV2::getInstance().doWork(); });
    tc.join();
    td.join();
}


// ============================================================
// ===== main()：示範本階段所有關鍵概念 =====
// ============================================================

// =============================================================================
// 【LeetCode 實戰範例】—— 本階段【刻意不提供】
//
//   說明:LeetCode 現有的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)考的全是【執行緒之間的順序協調】,
//   解法核心是 condition_variable / semaphore / atomic ——
//   那是第四、第五階段的主題。
//
//   本階段(執行緒生命週期管理)的所有主題 —— RAII 守衛、join/detach
//   的選擇、terminate 規則、跨執行緒例外傳遞、thread_local、call_once ——
//   在那些題目裡【完全不會出現】:執行緒由評測框架建立與回收,作答者
//   拿不到 std::thread 物件,題目也沒有錯誤路徑與初始化競爭。
//   硬套一題只會建立錯誤的關聯,故從缺。
//   (LeetCode 1114 與 1116 的真實解法已在【第一階段 summary.cpp】完整示範,
//    另可參考「課程 3.2：執行緒守衛類別設計2」——該處以
//    std::vector<JoiningThread> 驅動 1114 的三條執行緒,與該檔主題相符。)
// =============================================================================

// =============================================================================
// 【日常實務範例】影像縮圖服務:把第三階段六個主題全部串起來
//
//   情境:一個上傳影像後產生縮圖的背景服務。它必須:
//     1. 延遲載入設定(第一次真的要用時才讀,且只讀一次)      → 3.6 call_once
//     2. 開固定數量的 worker 處理佇列                        → 3.2 守衛 + 容器
//     3. 每個 worker 統計自己處理了幾張、失敗幾張(免鎖)     → 3.5 thread_local
//     4. 任何一張圖失敗都要把【原因】回報給呼叫端            → 3.4 例外傳遞
//     5. 關閉時必須等所有 worker 收工,不能有半張圖寫到一半  → 3.1 RAII
//     6. 即使中途丟例外,也絕不能讓 joinable 的 thread 被解構 → 3.1 terminate 規則
//
//   這六件事在真實服務裡是同時發生的,本範例把它們組裝成一個可執行的整體。
//
//   ⚠️ 設計重點(也是最容易寫錯的地方):
//     * 成員宣告順序 —— workers 必須宣告在【最後】。成員以宣告順序建構、
//       【反向】解構,所以 workers 會最先被解構(先 join),此時佇列、
//       mutex、統計都還活著,worker 才能安全收尾。順序寫反就是懸空存取。
//     * 關閉流程三步缺一不可:設旗標 → notify_all → 等待退出。
//       少了 notify_all,阻塞在 cv.wait() 上的 worker 永遠不會醒來,
//       解構函式會永久卡死。
//     * thread_local 統計無法被主執行緒直接讀(那是各執行緒自己的物件),
//       所以 worker 在【結束前】主動把自己的統計寫回共享結果 ——
//       這正是 thread_local「避免共享」與「最終仍需匯總」的正確配合方式。
// =============================================================================
namespace thumbnail_service {

// ---- 3.6:延遲且只做一次的設定載入(可重試)----------------------------------
struct Config {
    int maxWidth;
    int maxHeight;
};

std::once_flag        g_configFlag;
std::unique_ptr<Config> g_config;
std::atomic<int>      g_configLoadAttempts{0};

const Config& getConfig() {
    // call_once 保證:即使多條 worker 同時第一次呼叫,也只有一條真的載入,
    // 其餘會【等待】它完成後才返回 —— 不是「跳過」,是「等待」。
    std::call_once(g_configFlag, []() {
        g_configLoadAttempts.fetch_add(1, std::memory_order_relaxed);
        g_config = std::make_unique<Config>(Config{256, 256});
    });
    return *g_config;
}

// ---- 3.5:每個 worker 自己的統計,完全免鎖 ------------------------------------
thread_local int tls_processed = 0;
thread_local int tls_failed    = 0;

struct WorkerStat {
    int processed = 0;
    int failed    = 0;
};

// ---- 3.4:自訂例外型別,帶得回失敗的檔名 -------------------------------------
class ThumbnailError : public std::runtime_error {
    std::string file_;
public:
    ThumbnailError(const std::string& file, const std::string& why)
        : std::runtime_error(why), file_(file) {}
    const std::string& file() const noexcept { return file_; }
};

struct Job {
    std::string name;
    int         width;
    int         height;
    bool        corrupted;
};

// ---- 3.1 / 3.2:RAII 守衛 —— 解構時一定 join,例外路徑也一樣 -----------------
class JoiningWorker {
    std::thread t_;
public:
    JoiningWorker() noexcept = default;
    template <typename Fn>
    explicit JoiningWorker(Fn&& fn) : t_(std::forward<Fn>(fn)) {}

    JoiningWorker(JoiningWorker&& o) noexcept : t_(std::move(o.t_)) {}
    JoiningWorker& operator=(JoiningWorker&& o) noexcept {
        if (this != &o) {
            // ⚠️ 絕不能寫成 = default:那會逐成員移動,
            //    目標若仍 joinable 就是 std::terminate()
            if (t_.joinable()) t_.join();
            t_ = std::move(o.t_);
        }
        return *this;
    }
    JoiningWorker(const JoiningWorker&) = delete;
    JoiningWorker& operator=(const JoiningWorker&) = delete;

    ~JoiningWorker() {
        // 解構函式不可讓例外逸出(隱含 noexcept → 逸出即 terminate)
        try {
            if (t_.joinable()) t_.join();
        } catch (...) {
            // 生產環境應記 log;這裡吞掉以保證解構不拋
        }
    }
};

class ThumbnailService {
    std::queue<Job>             jobs_;
    std::mutex                  m_;
    std::condition_variable     cv_;
    bool                        stopping_ = false;

    std::mutex                  resultMutex_;
    std::vector<WorkerStat>     stats_;
    std::vector<std::string>    errors_;

    // ⚠️ 必須宣告在最後:成員反向解構 → workers 先 join,
    //    此時 jobs_/m_/cv_/stats_ 都還活著
    std::vector<JoiningWorker>  workers_;

public:
    explicit ThumbnailService(unsigned n) : stats_(n) {
        workers_.reserve(n);
        for (unsigned i = 0; i < n; ++i) {
            workers_.emplace_back([this, i]() { workerLoop(i); });
        }
    }

    void submit(const Job& j) {
        {
            std::lock_guard<std::mutex> lk(m_);
            jobs_.push(j);
        }
        cv_.notify_one();
    }

    // 關閉三步:設旗標 → 喚醒全部 → 等待退出(由 workers_ 解構完成)
    void shutdown() {
        {
            std::lock_guard<std::mutex> lk(m_);
            stopping_ = true;
        }
        cv_.notify_all();
        workers_.clear();   // 逐一解構 → 各自 join
    }

    ~ThumbnailService() {
        if (!workers_.empty()) shutdown();
    }

    WorkerStat total() const {
        WorkerStat t;
        for (const auto& s : stats_) { t.processed += s.processed; t.failed += s.failed; }
        return t;
    }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    void workerLoop(unsigned id) {
        const Config& cfg = getConfig();   // 3.6:只有第一個到的會真的載入

        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lk(m_);
                cv_.wait(lk, [this] { return stopping_ || !jobs_.empty(); });
                if (stopping_ && jobs_.empty()) break;   // 唯一的退出點
                job = std::move(jobs_.front());
                jobs_.pop();
            }

            // 3.4:失敗以例外表達,但【不能讓它逸出執行緒進入函式】
            try {
                processOne(job, cfg);
                ++tls_processed;                    // 3.5:免鎖,這是本執行緒獨有的
            } catch (const ThumbnailError& e) {
                ++tls_failed;
                std::lock_guard<std::mutex> lk(resultMutex_);
                errors_.push_back(std::string("[") + e.file() + "] " + e.what());
            }
        }

        // thread_local 是本執行緒私有的,主執行緒讀不到 →
        // 結束前主動把統計寫回共享結果
        std::lock_guard<std::mutex> lk(resultMutex_);
        stats_[id].processed = tls_processed;
        stats_[id].failed    = tls_failed;
    }

    static void processOne(const Job& j, const Config& cfg) {
        if (j.corrupted) {
            throw ThumbnailError(j.name, "decode failed: corrupted header");
        }
        // 模擬等比例縮圖運算
        double scale = std::min(static_cast<double>(cfg.maxWidth)  / j.width,
                                static_cast<double>(cfg.maxHeight) / j.height);
        volatile double w = j.width * scale;   // volatile 防止整段被最佳化掉
        volatile double h = j.height * scale;
        (void)w; (void)h;
    }
};

void demo() {
    const unsigned WORKERS = 4;
    const int      TOTAL   = 40;
    const int      BAD     = 5;   // 每第 8 張損毀

    ThumbnailService svc(WORKERS);

    for (int i = 0; i < TOTAL; ++i) {
        svc.submit(Job{"img_" + std::to_string(i) + ".jpg",
                       1920, 1080, (i % 8 == 7)});
    }
    svc.shutdown();   // 設旗標 → 喚醒 → join 全部

    WorkerStat t = svc.total();
    std::cout << "  提交 " << TOTAL << " 張,成功 " << t.processed
              << " 張,失敗 " << t.failed << " 張" << std::endl;
    std::cout << "  成功+失敗 = " << (t.processed + t.failed)
              << " (應等於提交數:" << std::boolalpha
              << (t.processed + t.failed == TOTAL) << ")" << std::endl;
    std::cout << "  預期失敗數 " << BAD << ",實際 " << t.failed
              << " → " << (t.failed == BAD ? "相符" : "不符") << std::endl;
    std::cout << "  設定實際載入次數 = " << g_configLoadAttempts.load()
              << " (call_once 保證只載入一次,即使 " << WORKERS
              << " 條 worker 同時第一次呼叫)" << std::endl;
    std::cout << "  前兩筆錯誤訊息(型別完整保留,取得到檔名):" << std::endl;
    for (std::size_t i = 0; i < svc.errors().size() && i < 2; ++i) {
        std::cout << "    " << svc.errors()[i] << std::endl;
    }
    std::cout << "  ← 每個 worker 的統計用 thread_local 累計(全程免鎖),"
              << std::endl;
    std::cout << "    結束前才寫回共享結果;workers 宣告在最後,"
              << "解構時先 join 再拆佇列。" << std::endl;
}

}  // namespace thumbnail_service

int main() {
    std::cout << "\n"
              << "============================================================\n"
              << " 第三階段：執行緒生命週期管理 - 總複習示範\n"
              << "============================================================\n"
              << std::endl;

    // --- 3.1 RAII 與執行緒管理 ---
    std::cout << "--- 課程 3.1：RAII 與執行緒管理 ---\n" << std::endl;

    std::cout << "[3.1.1] ThreadGuard（引用版 RAII）：\n";
    demo_ThreadGuard();

    std::cout << "\n[3.1.2] ScopedThread（擁有版 RAII）：\n";
    demo_ScopedThread();

    std::cout << "\n[3.1.3] FlexibleThread（可選 join/detach）：\n";
    demo_FlexibleThread();

    // --- 3.2 執行緒守衛類別設計 ---
    std::cout << "\n--- 課程 3.2：執行緒守衛類別設計 ---\n" << std::endl;

    std::cout << "[3.2.1] JoiningThread（完整守衛類別）：\n";
    demo_JoiningThread();

    std::cout << "\n[3.2.2] JoiningThread 放入 vector：\n";
    demo_JoiningThread_in_vector();

    std::cout << "\n[3.2.3] ManagedThread（編譯期決定行為）：\n";
    demo_ManagedThread();

    // --- 3.3 std::jthread (C++20) ---
    std::cout << "\n--- 課程 3.3：std::jthread (C++20) ---\n" << std::endl;
    std::cout << "[3.3] std::jthread 需要 C++20，請參閱上方已注釋的示範函式：\n"
              << "      demo_jthread_basic()          → 基本使用\n"
              << "      demo_jthread_exception_safe() → 例外安全\n"
              << "      demo_jthread_stop_token()     → stop_token 取消\n"
              << "      demo_jthread_stop_callback()  → stop_callback 回調\n"
              << "      demo_jthread_vector()         → vector 中的 jthread\n"
              << std::endl;

    // --- 3.4 執行緒例外處理 ---
    std::cout << "--- 課程 3.4：執行緒例外處理 ---\n" << std::endl;

    std::cout << "[3.4.A] 執行緒內部捕獲：\n";
    demo_exception_internal_catch();

    std::cout << "\n[3.4.B] std::exception_ptr 跨執行緒傳遞：\n";
    demo_exception_ptr();

    std::cout << "\n[3.4.C] SafeThread RAII 包裝：\n";
    demo_SafeThread();

    std::cout << "\n[3.4.D] std::future / std::async（推薦）：\n";
    demo_future_exception();

    std::cout << "\n[3.4.E] std::promise 精確控制：\n";
    demo_promise_exception();

    // --- 3.5 執行緒本地儲存 ---
    std::cout << "\n--- 課程 3.5：執行緒本地儲存 ---\n" << std::endl;

    std::cout << "[3.5.1] thread_local 基本示範（各自獨立計數）：\n";
    demo_thread_local_basic();

    std::cout << "\n[3.5.2] 全域變數 vs thread_local 對比：\n";
    demo_global_vs_thread_local();

    std::cout << "\n[3.5.3] 用途一：執行緒專屬錯誤碼：\n";
    demo_thread_local_error_code();

    std::cout << "\n[3.5.4] 用途二：執行緒專屬快取：\n";
    demo_thread_local_cache();

    std::cout << "\n[3.5.5] 用途三：執行緒專屬隨機數產生器：\n";
    demo_thread_local_rng();

    // --- 3.6 執行緒安全的初始化 ---
    std::cout << "\n--- 課程 3.6：執行緒安全的初始化 ---\n" << std::endl;

    std::cout << "[3.6.1] std::call_once 基本示範：\n";
    demo_call_once_basic();

    std::cout << "\n[3.6.2] call_once 與例外（失敗後重試）：\n";
    demo_call_once_with_exception();

    std::cout << "\n[3.6.3] 執行緒安全的單例模式：\n";
    demo_singleton();

    std::cout << "\n[3.6.4] 延遲初始化（Lazy Init）：\n";
    demo_lazy_init();

    std::cout << "\n--- 日常實務：影像縮圖服務（六個主題整合）---\n\n";
    thumbnail_service::demo();

    std::cout << "\n"
              << "============================================================\n"
              << " 第三階段所有示範執行完成！\n"
              << "============================================================\n"
              << std::endl;

    return 0;
}

/*
 * ============================================================
 * 本階段核心觀念速查表
 * ============================================================
 *
 * 【RAII 執行緒管理】
 *   ┌─────────────────────────────────────────────────────┐
 *   │ 問題                    解決方案                     │
 *   │ std::thread 未 join →   使用 RAII 包裝類別          │
 *   │ 例外導致 join 被跳過 →   RAII 確保解構時 join        │
 *   │ 程式崩潰（terminate） →  永遠不讓 joinable 的 thread │
 *   │                          在未 join/detach 下解構     │
 *   └─────────────────────────────────────────────────────┘
 *
 * 【執行緒守衛比較】
 *   ThreadGuard     → 持有引用，簡單，不可移動
 *   ScopedThread    → 持有所有權，建構時驗證
 *   JoiningThread   → 完整版，支援移動，可入容器
 *   ManagedThread   → 模板版，編譯期決定行為
 *   std::jthread    → C++20 標準版，內建取消機制
 *
 * 【例外處理方案比較】
 *   執行緒內部捕獲     → 最簡單，但呼叫端感知不到
 *   exception_ptr     → 可跨執行緒傳遞任意例外
 *   std::future       → 推薦！自動傳遞，語法最簡潔
 *   std::promise      → 最靈活，可精確控制時間點
 *
 * 【thread_local 使用場景】
 *   - errno 風格的執行緒專屬錯誤碼
 *   - 避免鎖競爭的執行緒專屬快取
 *   - 每個執行緒獨立的隨機數產生器
 *   - 執行緒 ID、名稱等執行緒元資料
 *
 * 【執行緒安全初始化比較】
 *   call_once + once_flag → 適合複雜初始化邏輯，可重試
 *   static 區域變數       → 單例模式首選，最簡潔
 *   簡單 mutex 保護       → 效能較差，不推薦用於初始化
 *
 * ============================================================
 */

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread summary.cpp -o summary3
//   本檔【可執行的部分】只用到 C++11/14/17,以 -std=c++17 編譯零警告通過。
//   檔中 std::jthread / stop_token / stop_callback 的示範函式刻意保留為註解
//   (它們需要 -std=c++20);完整可執行版本請見
//   「課程 3.3：std jthread (C++20)2 ~ 7」各檔。

// 註:本檔多處由多條執行緒直接輸出,以下內容含【非決定性】成分,每次執行都不同:
//   * [Vector] Worker 0~3、[thread_local]、[LazyCache] 等段落的【行序】——
//     std::cout 保證不會有 data race,但【不保證整行的原子性】,
//     缺少輸出鎖時行序會變、甚至同一行被切開(輸出中的
//     「[LazyCache] Thread 1: [LazyCache] 正在初始化快取...」正是這種交錯,
//     不是 bug,也不是未定義行為);
//   * 執行緒 id 與任何毫秒數(實作定義 / 受排程影響)。
//   ⚠️ 相對地,以下數值是【確定的】,可以拿來驗證正確性:
//   * 影像縮圖服務:提交 40 張 → 成功 35 + 失敗 5,兩者相加必等於 40;
//   * 設定載入次數必為 1(call_once 的保證,即使 4 條 worker 同時第一次呼叫);
//   * call_once 失敗重試示範中,最終必定成功初始化一次。
//   以下為本機(16 邏輯核心)某一次的實際執行結果。

// === 預期輸出 ===
//
// ============================================================
//  第三階段：執行緒生命週期管理 - 總複習示範
// ============================================================
//
// --- 課程 3.1：RAII 與執行緒管理 ---
//
// [3.1.1] ThreadGuard（引用版 RAII）：
// [ThreadGuard] 執行緒工作完成
//
// [3.1.2] ScopedThread（擁有版 RAII）：
// [ScopedThread] 安全的執行緒，自動 join
//
// [3.1.3] FlexibleThread（可選 join/detach）：
// [FlexibleThread] join 我
//
// --- 課程 3.2：執行緒守衛類別設計 ---
//
// [3.2.1] JoiningThread（完整守衛類別）：
// [JoiningThread] 方式一：直接建構
// [JoiningThread] 方式二：從 std::thread 移動
// [JoiningThread] 方式三：帶參數 x=42 s=hello
//
// [3.2.2] JoiningThread 放入 vector：
// [Vector] Worker 0 執行中
// [Vector] Worker 1 執行中
// [Vector] Worker 2 執行中
// [Vector] Worker [Vector] 所有執行緒已建立，等待結束...3 執行中
//
//
// [3.2.3] ManagedThread（編譯期決定行為）：
// [ManagedThread] AutoJoinThread：離開作用域時自動 join
//
// --- 課程 3.3：std::jthread (C++20) ---
//
// [3.3] std::jthread 需要 C++20，請參閱上方已注釋的示範函式：
//       demo_jthread_basic()          → 基本使用
//       demo_jthread_exception_safe() → 例外安全
//       demo_jthread_stop_token()     → stop_token 取消
//       demo_jthread_stop_callback()  → stop_callback 回調
//       demo_jthread_vector()         → vector 中的 jthread
//
// --- 課程 3.4：執行緒例外處理 ---
//
// [3.4.A] 執行緒內部捕獲：
// [方案A] 執行緒內捕獲：執行緒內的錯誤！
// [方案A] 主執行緒：程式正常繼續
//
// [3.4.B] std::exception_ptr 跨執行緒傳遞：
// [方案B] 主執行緒捕獲例外：來自工作執行緒的錯誤！
//
// [3.4.C] SafeThread RAII 包裝：
// [SafeThread] 主執行緒捕獲：SafeThread 內的錯誤！
//
// [3.4.D] std::future / std::async（推薦）：
// [future] Task 0 結果：0
// [future] Task 1 結果：10
// [future] Task 2 例外：Task 2 失敗（模擬錯誤）
// [future] Task 3 結果：30
// [future] Task 4 結果：40
//
// [3.4.E] std::promise 精確控制：
// [promise] 主執行緒捕獲：Promise worker 發生錯誤！
//
// --- 課程 3.5：執行緒本地儲存 ---
//
// [3.5.1] thread_local 基本示範（各自獨立計數）：
// [thread_local] Thread A: counter = 1
// [thread_local] Thread A: counter = 2
// [thread_local] Thread A: counter = 3
// [thread_local] Thread B: counter = 1
// [thread_local] Thread B: counter = 2
// [thread_local] Thread B: counter = 3
//
// [3.5.2] 全域變數 vs thread_local 對比：
// [對比] A global=1 local=1
// [對比] B global=2 local=1
// [對比] C global=3 local=1
//
// [3.5.3] 用途一：執行緒專屬錯誤碼：
// [errno] Thread 1 error: 100
// [errno] Thread 2 error: 200
//
// [3.5.4] 用途二：執行緒專屬快取：
// [cache] [cache] [cache] Thread 1 首次計算...
// Thread-1-result
// [cache] Thread-1-result (快取)
// [cache] Thread 2 首次計算...
// Thread-2-result
// [cache] Thread-2-result (快取)
//
// [3.5.5] 用途三：執行緒專屬隨機數產生器：
// [rng] Thread 1: [rng] Thread 2: 55, 57, 26
// 38, 41, 76
//
// --- 課程 3.6：執行緒安全的初始化 ---
//
// [3.6.1] std::call_once 基本示範：
// [Database] 初始化（id=42）
// [Database] Thread 1 查詢中（db id=42）
// [Database] Thread 3 查詢中（db id=42）
// [Database] Thread 2 查詢中（db id=42）
//
// [3.6.2] call_once 與例外（失敗後重試）：
// [call_once] 嘗試 #1
// [call_once] 嘗試 #2
// [call_once] 捕獲例外：初始化失敗（模擬）
// [call_once] 捕獲例外：初始化失敗（模擬）
// [call_once] 嘗試 #3
// [call_once] 初始化成功！
// [call_once] 繼續執行...
// [call_once] 繼續執行...
//
// [3.6.3] 執行緒安全的單例模式：
// [Singleton v1] 建立（call_once 版本）
// [Singleton v1] 執行工作
// [Singleton v1] 執行工作
// [Singleton v2] 建立（static 局部變數版本）
// [Singleton v2] 執行工作
// [Singleton v2] 執行工作
//
// [3.6.4] 延遲初始化（Lazy Init）：
// [LazyCache] Thread 2: [LazyCache] 正在初始化快取...
// [LazyCache] Thread 1: 快取資料（只初始化一次）
// 快取資料（只初始化一次）
//
// --- 日常實務：影像縮圖服務（六個主題整合）---
//
//   提交 40 張,成功 35 張,失敗 5 張
//   成功+失敗 = 40 (應等於提交數:true)
//   預期失敗數 5,實際 5 → 相符
//   設定實際載入次數 = 1 (call_once 保證只載入一次,即使 4 條 worker 同時第一次呼叫)
//   前兩筆錯誤訊息(型別完整保留,取得到檔名):
//     [img_7.jpg] decode failed: corrupted header
//     [img_23.jpg] decode failed: corrupted header
//   ← 每個 worker 的統計用 thread_local 累計(全程免鎖),
//     結束前才寫回共享結果;workers 宣告在最後,解構時先 join 再拆佇列。
//
// ============================================================
//  第三階段所有示範執行完成！
// ============================================================
