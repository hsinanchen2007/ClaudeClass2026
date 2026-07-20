// =============================================================================
//  課程 2.7：執行緒的移動語意 6  —  移動賦值的殺手陷阱：std::terminate()
// =============================================================================
//
// 【主題資訊 Information】
//   thread& operator=(thread&& x) noexcept;      標頭檔：<thread>
//   標準：C++11 起                                標準依據：[thread.thread.assign]
//
//   標準原文語意（逐字）：
//     Effects: If joinable(), calls terminate().
//              Otherwise, assigns the state of x to *this and
//              sets x to a default constructed state.
//     Returns: *this.
//
//   ★ 請把第一句讀三遍：**If joinable(), calls terminate().**
//     這是**標準保證的行為（standard-guaranteed）**，不是未定義行為（UB）。
//     任何符合標準的實作，在這個情況下都必須 abort，不會有平台差異。
//
//   注意 noexcept 與 terminate 並不矛盾：noexcept 的意思是「不會丟例外」，
//   而呼叫 terminate() 正是「不丟例外」的一種極端履行方式。
//   本機實測 is_nothrow_move_assignable_v<std::thread> == true。
//
// 【詳細解釋 Explanation】
//
// 【1. 陷阱長什麼樣子】
//     std::thread t1(f);      // t1 現在 joinable
//     std::thread t2(g);      // t2 現在 joinable
//     t1 = std::move(t2);     // ☠️ t1 仍 joinable → std::terminate() → 程式 abort
//
// 為什麼危險？因為這一行看起來太無辜了。它跟
//     std::thread t3 = std::move(t2);   // 移動「建構」—— 永遠安全
// 只差一個字元（有沒有型別宣告），但一個是安全的移動建構、
// 另一個是可能直接讓程式 abort 的移動賦值。
//
//   移動建構：目標是**全新物件**，不可能 joinable → 永遠安全
//   移動賦值：目標是**既存物件**，可能還握著執行緒 → 可能 terminate
//
// 【2. 為什麼標準選擇 terminate，而不是「自動 join」或「自動 detach」】
// 這是本課最值得理解的設計決策。標準委員會有三個選項：
//
//   選項 A：自動 join 舊執行緒
//     t1 = std::move(t2); 這一行會**阻塞**，等 t1 原本那條執行緒跑完。
//     問題：這是一個看起來像「賦值」的無害運算式，卻可能一等就是好幾秒
//     甚至永遠（若舊執行緒是無窮迴圈）。在 C++ 的傳統裡，賦值不該有
//     隱藏的阻塞行為 —— 這會讓效能問題與死結變得極難追查
//     （你看著一行賦值，怎麼也想不到程式卡在那）。
//
//   選項 B：自動 detach 舊執行緒
//     舊執行緒變成無主的孤兒，繼續在背景跑。
//     問題：它可能還在存取即將被銷毀的物件（區域變數、成員），
//     變成典型的 use-after-free；而且**沒有任何人能再等它、觀察它**。
//     這種錯誤會在幾天後以隨機當機的形式出現，是最難除錯的一類 bug。
//
//   選項 C：terminate（標準的選擇）
//     程式立刻死在**出錯的那一行**，core dump 裡的堆疊直指現場。
//
// 標準選 C 的理由是：**A 和 B 都是「靜默地做一件使用者沒要求的事」，
// 而且都會把問題推遲到很遠的地方才爆發。C 雖然粗暴，但它把錯誤留在
// 案發現場**。這與 std::thread 解構子的設計是同一個哲學
// （[thread.thread.destr]：若 joinable() 則 terminate），
// 也是 C++「不為你做你沒要求的事」原則的體現。
//
// 換句話說：標準不是不知道怎麼「救」你，而是刻意認定
// 「你沒表態要 join 還是 detach」本身就是一個必須當場修正的程式錯誤。
//
// 【3. 正確做法：賦值之前必須讓目標變成 non-joinable】
// 三種合法途徑：
//     t1.join();                  // ① 等它跑完（最常見）
//     t1.detach();                // ② 放它自生自滅（要確定它不碰已失效的資源）
//     // ③ t1 本來就是空的（預設建構、或已被移走）
// 之後才可以安全地：
//     t1 = std::move(t2);
//
// 防禦式寫法（在泛型或不確定狀態時很有用）：
//     if (t1.joinable()) t1.join();
//     t1 = std::move(t2);
//
// 【4. 這個陷阱最容易出現的三個真實場景】
//
//   (a) 迴圈中重複指派同一個變數
//         std::thread w;
//         for (auto& job : jobs) {
//             w = std::thread(run, job);   // ☠️ 第二圈開始，w 仍 joinable
//         }
//       正解：迴圈內先 if (w.joinable()) w.join();，
//             或改用 vector<thread> 各存各的（見檔 5）。
//
//   (b) 類別成員的「重新啟動」介面
//         void Service::restart() {
//             worker_ = std::thread(&Service::loop, this);  // ☠️ 舊的還在跑
//         }
//       正解：先停止並 join 舊的（本檔實務範例示範完整寫法）。
//
//   (c) 誤以為「自己賦值給自己」是無害的
//         t1 = std::move(t1);   // ☠️ t1 joinable → terminate
//       thread 的移動賦值**沒有自我賦值保護**（不像某些容器會檢查 this != &x）。
//       標準的規則就是純粹的「if joinable() then terminate()」，先檢查，
//       不管來源是誰。這行在任何 t1 joinable 的情況下都會 abort。
//
// 【5. 與 C++20 std::jthread 的對比 —— 這裡有個重要的差異】
// 常見的說法是「jthread 只是解構時會自動 join」，但在**移動賦值**這件事上，
// 兩者的行為是**根本不同**的：
//
//   std::thread  的 operator=：若 joinable() → **terminate()**
//   std::jthread 的 operator=：若 joinable() → **request_stop() 然後 join()**
//                                              （[thread.jthread.assign]）
//
// 也就是說 jthread 選了前面說的「選項 A」，但它**有本錢這樣選**：
// jthread 內建 stop_token 機制，request_stop() 能真正要求舊執行緒收工，
// 所以那個 join 不會無限期等下去（前提是 worker 有檢查 stop_token）。
// std::thread 沒有任何取消機制，自動 join 只能盲等，所以走不了這條路。
//
// 本機實測（C++20，GCC 15.2）：對一個 joinable 的 jthread 做移動賦值，
// 程式**正常存活**，舊執行緒收到 stop_requested() == true 後自行結束。
// 這是把設計理由講清楚後很自然的結論：**能安全自動收尾的型別才敢自動收尾。**
//
// 【概念補充 Concept Deep Dive】
//
// (A) terminate() 到底做了什麼
// std::terminate() 呼叫目前的 terminate handler，預設是 std::abort()，
// 在 Linux 上送出 SIGABRT（exit code 134 = 128 + 6）。
// 預設 handler 不做堆疊回溯、**不呼叫任何區域物件的解構子**，
// 所以不要指望 RAII 在這時候還會運作。
// 本機實測 libstdc++ 會先印出：terminate called without an active exception
// （因為此時並沒有例外在飛，只是單純呼叫 terminate）。
//
// (B) 可以用 std::set_terminate 攔截嗎？
// 可以裝一個自訂 handler 印出診斷訊息，但**無法讓程式繼續執行** ——
// terminate handler 依標準不得返回（若返回，行為是呼叫 abort）。
// 它只適合用來記錄 log、輸出更好的錯誤訊息，不是救援機制。
//
// (C) 為什麼編譯器不能在編譯期擋下來
// 「t1 此刻是否 joinable」是**執行期狀態**，取決於程式流程（有沒有走到
// join、有沒有走進某個 if）。編譯器無法在一般情況下靜態判定。
// 這也正是要用 RAII 包裝（scoped_thread / jthread）的理由：
// 把「一定要收尾」變成型別的責任，而不是靠人記得。
// 靜態分析工具（clang-tidy 的 bugprone 系列、Coverity）能抓到部分明顯案例。
//
// (D) 這與解構子的規則是同一條
// [thread.thread.destr]：若 joinable() 則 terminate()。
// 所以「忘記 join」的懲罰有兩個入口：物件解構時、以及移動賦值時。
// 記憶方式：**任何會讓 thread 物件失去現有握把的操作，
// 都不允許它此刻還握著一條沒交代的執行緒。**
//
// 【注意事項 Pay Attention】
// 1. 這是**標準保證的 terminate**，不是 UB。不要寫成「可能會 crash」——
//    它是必然 abort，而且所有符合標準的實作行為一致。
// 2. 移動「建構」永遠安全；只有移動「賦值」有這個陷阱。分清楚兩者。
// 3. 自我賦值 t = std::move(t) 同樣會 terminate，沒有特殊保護。
// 4. 用 t1.swap(t2) 或 std::swap(t1, t2) **不會**觸發 terminate ——
//    交換只是對調兩個握把，沒有任何一方失去所有權，兩條執行緒都還有主人。
//    這是需要「換手但不想先 join」時的安全選擇。
// 5. 本檔預設**不會** abort：危險那行是刻意註解掉的教學示範。
//    想親眼看到 terminate，請用 -DDEMONSTRATE_TERMINATE 編譯（見檔尾）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動賦值與 std::terminate
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. t1 和 t2 都是 joinable 的 thread，執行 t1 = std::move(t2); 會怎樣？
//     答：程式呼叫 std::terminate() 直接 abort。標準 [thread.thread.assign]
//         明文規定「If joinable(), calls terminate()」。
//         這是**標準保證的行為，不是 UB**，所有實作一致。
//         正解是先 t1.join()（或 t1.detach()）讓它變成 non-joinable 再賦值。
//     追問：那 std::thread t3 = std::move(t2); 呢？
//           → 完全安全。那是移動**建構**，t3 是全新物件不可能 joinable。
//             陷阱只存在於移動**賦值**。
//
// 🔥 Q2. 標準為什麼選 terminate，而不是自動 join 或自動 detach？
//     答：自動 join 會讓一行看似無害的賦值變成不可預期的阻塞（甚至永久卡住），
//         而 C++ 的傳統是賦值不該隱含阻塞；自動 detach 會產生無主執行緒，
//         可能存取已銷毀的物件，變成幾天後才隨機爆發的 use-after-free。
//         兩者都是「靜默做使用者沒要求的事」，且把問題推遲到遠處。
//         terminate 雖然粗暴，但讓程式死在出錯的那一行，core dump 直指現場。
//     追問：那為什麼 C++20 的 jthread 就敢自動 join？
//           → 因為 jthread 有 stop_token。它的移動賦值是
//             request_stop() + join()，能主動要求舊執行緒收工，
//             不是盲等。**有取消機制才敢自動收尾。**
//
// 🔥 Q3. 有沒有辦法「換掉」一個 joinable 的 thread 而不 terminate、也不先 join？
//     答：有 —— 用 swap。t1.swap(t2) 或 std::swap(t1, t2) 只是對調兩個握把，
//         沒有任何一方失去所有權（原本 t1 的執行緒現在由 t2 負責），
//         所以不觸發 terminate。當然你最後仍然要把兩個都收掉。
//     追問：那 swap 之後責任怎麼算？→ 完全對調。原本欠 t1 的那次 join，
//           現在變成欠 t2；責任總數不變，只是換人負責。
//
// ⚠️ 陷阱. 「移動賦值到 joinable 的 thread 是未定義行為（UB），
//            所以結果可能是 crash、也可能碰巧跑過去」
//     答：**錯，這不是 UB。** 標準明文規定要呼叫 terminate()，
//         是完全確定的行為 —— 不存在「碰巧跑過去」的可能，也沒有
//         「換個編譯器就好了」這回事。本機實測輸出
//         "terminate called without an active exception" 後 SIGABRT（rc=134）。
//     為什麼會錯：把所有「不該做的事」一律歸類成 UB。C++ 標準其實有
//         多個層級：UB（如 data race）、實作定義（如 thread::id 印法）、
//         未指定（如執行緒排程順序）、以及**標準明文規定的 terminate**。
//         面試時能精確講出「這是 standard-guaranteed terminate，不是 UB」，
//         是很明顯的加分點。
//
// ⚠️ 陷阱 2. 「t = std::move(t); 自我賦值應該是安全的無操作（no-op）」
//     答：錯。std::thread 的移動賦值**沒有自我賦值檢查**。規則就是先看
//         joinable()，是就 terminate()。所以只要 t 當時 joinable，
//         這行就會 abort，跟來源是不是它自己完全無關。
//     為什麼會錯：套用了「良好實作的 operator= 應該處理自我賦值」這個
//         通則。但那個通則針對的是**複製**賦值（避免先釋放再讀取自己）；
//         thread 的移動賦值語意由標準直接規定，優先於慣例。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

// -----------------------------------------------------------------------------
// 【日常實務範例】可重啟的背景健康檢查服務（restart 的正確寫法）
// 情境：監控服務有一條背景執行緒定期 ping 後端。設定變更時要「重啟」它。
//       最常見的災難寫法是：
//           void restart() { worker_ = std::thread(&Service::loop, this); }
//       —— 舊的 worker_ 還在跑（joinable），這行直接 std::terminate()，
//       整個監控服務當場 abort。
// 正確做法（本範例）：
//   ① 用 atomic<bool> 通知舊執行緒該收工（std::thread 沒有取消機制，
//      必須自己做一個 —— 這正是 C++20 jthread 內建 stop_token 的動機）；
//   ② join 舊執行緒，確定它真的結束了（此時 worker_ 變 non-joinable）；
//   ③ 這時候才做移動賦值，安全。
// -----------------------------------------------------------------------------
class HealthChecker {
    std::thread       worker_;
    std::atomic<bool> stop_{false};   // 自製的「取消旗標」

public:
    void start(int round) {
        stopWorker();                 // ★ 關鍵：先確保 worker_ 是 non-joinable
        stop_.store(false, std::memory_order_relaxed);
        // 此刻 worker_ 保證 non-joinable，這個移動賦值才安全
        worker_ = std::thread([this, round]() {
            int ticks = 0;
            while (!stop_.load(std::memory_order_relaxed) && ticks < 100) {
                ++ticks;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            std::cout << "  [checker#" << round << "] 收到停止訊號，結束（tick="
                      << ticks << "）\n";
        });
        std::cout << "  [service] 第 " << round << " 版健康檢查已啟動\n";
    }

    void stopWorker() {
        if (worker_.joinable()) {                       // ① 有東西要收
            stop_.store(true, std::memory_order_relaxed); // ② 通知收工
            worker_.join();                              // ③ 等它真的結束
        }
    }

    ~HealthChecker() { stopWorker(); }                   // RAII：絕不留下 joinable

    HealthChecker(const HealthChecker&)            = delete;
    HealthChecker& operator=(const HealthChecker&) = delete;
    HealthChecker()                                = default;
};

int main() {
    std::cout << "=== 原始示範：移動賦值的陷阱與正解 ===\n";
    std::thread t1([]() {});
    std::thread t2([]() {});

    // 危險！t1 還是 joinable，程式會呼叫 std::terminate()
    // t1 = std::move(t2);
    //
    // ↑ 這一行是**刻意保持註解**的教學示範。
    //   若解除註解，程式會在此處 abort（標準保證，非 UB）：
    //     terminate called without an active exception
    //     Aborted (core dumped)          ← SIGABRT，exit code 134
    //   想親眼看到，請用 -DDEMONSTRATE_TERMINATE 編譯（見檔尾說明），
    //   不要直接解除註解 —— 那會讓這個檔案的其他示範全部跑不到。

    std::cout << "賦值前：t1.joinable() = " << std::boolalpha << t1.joinable()
              << "（此時做移動賦值就會 terminate）\n";

    // 正確做法：先 join 或 detach
    t1.join();
    std::cout << "join 後：t1.joinable() = " << t1.joinable()
              << "（現在才可以安全賦值）\n";
    t1 = std::move(t2);  // 現在安全了
    t1.join();
    std::cout << "移動賦值成功，程式存活\n";

    std::cout << "\n=== swap 是安全的替代方案（不觸發 terminate）===\n";
    std::thread a([]() {}), b([]() {});
    std::cout << "swap 前 a.joinable()=" << a.joinable()
              << " b.joinable()=" << b.joinable() << "\n";
    a.swap(b);   // 只是對調握把，沒有人失去所有權 → 安全
    std::cout << "swap 後 a.joinable()=" << a.joinable()
              << " b.joinable()=" << b.joinable()
              << "（兩條執行緒都還有主人，責任只是換人負責）\n";
    a.join();
    b.join();

    std::cout << "\n=== 實務：可重啟的健康檢查服務 ===\n";
    {
        HealthChecker svc;
        svc.start(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        svc.start(2);   // 重啟：內部先 stop + join，再做移動賦值 → 不會 terminate
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }   // 解構 → stopWorker() → 乾淨結束
    std::cout << "  [service] 已安全關閉\n";

#ifdef DEMONSTRATE_TERMINATE
    // -------------------------------------------------------------------------
    // 刻意示範：standard-guaranteed std::terminate（abort by design）
    // 預設**不編譯**這段。加 -DDEMONSTRATE_TERMINATE 才會啟用。
    // 啟用後程式必定 abort（SIGABRT / rc=134），這是標準規定的行為，
    // 不是 bug、也不是 UB。實測輸出見檔尾。
    // -------------------------------------------------------------------------
    std::cout << "\n=== [DEMONSTRATE_TERMINATE] 即將觸發標準保證的 terminate ===\n"
              << std::flush;
    std::thread danger1([]() {});
    std::thread danger2([]() {});
    std::cout << "danger1.joinable() = " << danger1.joinable()
              << " → 下一行移動賦值必定 abort\n" << std::flush;
    danger1 = std::move(danger2);   // ☠️ 標準保證：呼叫 std::terminate()
    std::cout << "這一行永遠不會被印出來\n";   // 不可達
#endif

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意6.cpp" -o move6
//
// 想親眼看到標準保證的 terminate（程式會 abort，這是設計如此）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_TERMINATE "課程 2.7：執行緒的移動語意6.cpp" -o move6_boom
//   ./move6_boom            # 必定 SIGABRT，echo $? 得到 134

//     以下為某次實際執行結果；健康檢查那段的 tick 數每次執行都不同

// === 預期輸出 ===
// 註：預設編譯（不加 -DDEMONSTRATE_TERMINATE）程式**正常結束，rc=0**。
//     （取決於排程與計時精度），其餘輸出順序是確定的。
//
// === 原始示範：移動賦值的陷阱與正解 ===
// 賦值前：t1.joinable() = true（此時做移動賦值就會 terminate）
// join 後：t1.joinable() = false（現在才可以安全賦值）
// 移動賦值成功，程式存活
//
// === swap 是安全的替代方案（不觸發 terminate）===
// swap 前 a.joinable()=true b.joinable()=true
// swap 後 a.joinable()=true b.joinable()=true（兩條執行緒都還有主人，責任只是換人負責）
//
// === 實務：可重啟的健康檢查服務 ===
//   [service] 第 1 版健康檢查已啟動
//   [checker#1] 收到停止訊號，結束（tick=5）
//   [service] 第 2 版健康檢查已啟動
//   [checker#2] 收到停止訊號，結束（tick=5）
//   [service] 已安全關閉
//
// === 加上 -DDEMONSTRATE_TERMINATE 的實際輸出（程式必定 abort）===
// 註：以下同樣是實跑貼上。這是**標準保證的 abort**，不是 UB、不是 bug。
//     收集輸出必須用 stdbuf -o0，否則 abort 會讓緩衝區內容全部遺失。
//
// （前面各段輸出相同，接著：）
// === [DEMONSTRATE_TERMINATE] 即將觸發標準保證的 terminate ===
// danger1.joinable() = true → 下一行移動賦值必定 abort
// terminate called without an active exception
// Aborted (core dumped)
// $ echo $?
// 134                      ← 128 + 6 (SIGABRT)
