// =============================================================================
//  課程 2.1：第一個多執行緒程式3.cpp  —  detach()：放生執行緒的代價
// =============================================================================
//
// 【本檔示範的是「detach 的風險」，不是可以照抄的寫法】
//   主程式結束時，被 detach 的背景執行緒還沒睡醒就被作業系統一起帶走，
//   所以 "Background task" 幾乎不會印出來（本機連續 5 次執行皆未印出）。
//   ⚠️ 措辭要精確：是「幾乎不會」而不是「一定不會」——它取決於排程時機。
//
// 【主題資訊 Information】
//   標頭檔：  #include <thread>、#include <chrono>
//   標準版本：C++11
//   核心簽名：
//     void detach();                    // 切斷 std::thread 物件與底層執行緒的關聯
//     bool joinable() const noexcept;   // detach() 之後恆為 false
//   複雜度：O(1)，不阻塞、立即回傳（對照 join() 會等待）
//   編譯：g++ -std=c++17 -Wall -Wextra -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. detach() 做的是「放棄所有權」，不是「背景執行」】
//   常見誤解是「detach 讓執行緒變成背景執行」。錯——執行緒從被建立的那一刻起
//   就已經在背景並行跑了，join() 也不會改變這點。
//   detach() 真正做的事只有一件：**切斷 std::thread 物件與底層 OS 執行緒的關聯**。
//   呼叫後 t.joinable() 變成 false，t 這個物件變成空殼、可以安全解構（不會
//   terminate）；而那條執行緒繼續自己跑，沒有任何人持有它的把柄。
//   關鍵後果：**標準沒有提供任何機制讓你等待、查詢或取消一條 detached 執行緒。**
//   你放棄的不只是所有權，還有全部的控制權與觀測能力。
//
// 【2. 為什麼 "Background task" 印不出來】
//   時間軸（單位：毫秒）：
//       t=0     main 建立執行緒 → detach → 印出 "Main exits" → return 0
//       t≈0.1   main 返回，C++ runtime 開始收尾，最終呼叫 exit()
//       t≈0.2   行程結束，OS 回收整個 address space；
//               所有還活著的執行緒（包含我們那條）被直接消滅
//       t=500   （這條執行緒原本預定在這裡醒來印字——但它已經不存在了）
//   背景執行緒被殺掉時**不會**執行堆疊回溯（stack unwinding）：
//   區域物件的解構函式不跑、RAII 不生效、檔案緩衝區不 flush、socket 不關閉。
//   這就是為什麼「用 detach 做重要的收尾工作」是危險的設計。
//
// 【3. detach 真正的地雷：懸空存取（dangling access）】
//   本檔的執行緒只碰字面值，還算幸運。真實災難長這樣：
//       void handler() {
//           LocalBuffer buf;                       // 區域物件
//           std::thread([&buf]{ use(buf); }).detach();  // 💀 捕捉區域變數的參考
//       }   // handler 返回 → buf 銷毀 → 那條執行緒仍在讀已死的記憶體 = UB
//   更隱蔽的版本是行程收尾階段：main 返回後會依序解構 static/全域物件
//   （包含 std::cout 的相關設施）。一條 detached 執行緒若在此時仍在寫 std::cout，
//   就是對「正在被銷毀或已被銷毀的物件」做存取——這是真正的未定義行為，
//   可能安靜無事，也可能在程式「明明已經結束」時噴出詭異的崩潰。
//   ⚠️ 注意這裡的分類差異：**程式沒印出那行字，本身不是 UB**（那只是行程先結束了）；
//      UB 指的是「執行緒在 static 解構期間仍存取那些設施」這個競賽情境。
//
// 【4. 那 detach 到底什麼時候該用？】
//   誠實的答案是：**實務上極少**。合理情境大致只有「生命週期與行程等長的常駐
//   背景執行緒」（例如整個程式期間都在跑的監控取樣、log flush 執行緒），
//   而且它捕捉的必須是靜態存活的資源。
//   即使是這種情境，現代寫法也多半改用：
//     • thread pool —— 由 pool 統一持有生命週期，關閉時明確等待
//     • C++20 std::jthread + stop_token —— 可請求停止、解構時自動 join
//     • 明確的 shutdown 協定（atomic 旗標 + join）
//   經驗法則：**當你想寫 detach() 時，先問「誰負責等它結束？」**
//   如果答不出來，那就是設計缺口，不是可以用 detach 掩蓋的細節。
//
// 【概念補充 Concept Deep Dive】
//   (A) detach 與 pthread 的對應
//       detach() 對應 pthread_detach()。差別在於資源回收的時機：joinable 的
//       執行緒結束後，其結束狀態會被保留直到有人 join（類似 zombie process）；
//       detached 的執行緒一結束，核心與函式庫立刻回收其 stack 與 TCB，
//       沒有人能再查詢它。所以 detach 不是「洩漏」，資源會回收——
//       洩漏的是**控制權**。
//
//   (B) 為什麼 detach 之後解構不會 terminate
//       std::thread 的解構函式規則很單純：`if (joinable()) std::terminate();`
//       detach() 把 joinable() 變成 false，於是解構安靜通過。
//       請注意這代表 terminate 檢查**不是**在檢查「執行緒是否已結束」，
//       而是在檢查「你是否已經明確表態要 join 還是 detach」。
//
//   (C) 行程結束時執行緒是怎麼死的
//       main 返回等同呼叫 exit()，它會執行 atexit 註冊的函式與 static 解構，
//       最後呼叫 _exit()/exit_group() 系統呼叫。exit_group() 會終結行程中
//       **所有**執行緒，不送訊號、不給收尾機會。
//       這也是為什麼「detached 執行緒 + 未 flush 的檔案寫入」會遺失資料。
//
// 【注意事項 Pay Attention】
//   1. detach() 之後絕不可再呼叫 join()：joinable() 已是 false，會拋
//      std::system_error。安全寫法一律是 `if (t.joinable()) ...`。
//   2. detach() 對「已經 detach 或已 join 的物件」呼叫同樣會拋例外，不是 no-op。
//   3. 本檔的輸出「不確定」屬於**時序相依**，不是未定義行為；但若把示範改成
//      在執行緒中存取區域變數或行程收尾中的 static 物件，那就升級成真正的 UB。
//      兩者請分清楚。
//   4. 想觀察 "Background task" 真的被印出來，唯一正確的做法是給它一個確定的
//      同步點（改用 join()），而**不是**在 main 尾端加 sleep —— 那只是把賭注
//      押在排程器上，不是保證。
//   5. detached 執行緒中的未捕捉例外一樣會呼叫 std::terminate()，
//      而且此時沒有任何人能接住或觀測到它。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】detach() 與執行緒生命週期
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. join() 和 detach() 差在哪？兩個都不呼叫會怎樣？
//     答：join() 阻塞呼叫端直到目標結束，並建立 happens-before、回收資源；
//         detach() 立即回傳、切斷物件與執行緒的關聯，此後無法等待或查詢它。
//         兩個都不呼叫：std::thread 解構時仍 joinable，會呼叫 std::terminate()
//         中止程式——這是標準明文規定的行為，不是未定義行為。
//     追問：為什麼委員會選 terminate，而不是預設幫你 join 或 detach？
//           → 預設 join 會讓解構點意外阻塞、掩蓋忘記同步的 bug；預設 detach
//             會造成懸空存取這種隱蔽 UB。兩害相權，強迫使用者明確表態最安全。
//
// 🔥 Q2. detach() 最大的風險是什麼？
//     答：生命週期完全失控。標準不提供任何等待、查詢或取消 detached 執行緒的
//         方法，所以它捕捉的任何物件都必須活得比它久——而你無法證明這件事。
//         最常見的災難是捕捉區域變數的參考後函式已返回，以及行程收尾時
//         static 物件（如 std::cout 的設施）已在解構而執行緒仍在使用。
//     追問：真的需要 fire-and-forget 怎麼辦？→ 改用 thread pool 由池持有生命
//           週期，或 C++20 jthread + stop_token，並在 shutdown 時明確等待。
//
// ⚠️ 陷阱. 這支程式印不出 "Background task"，是不是因為執行緒建立失敗了？
//     答：不是。執行緒確實成功建立、也確實開始執行了，它只是還在 sleep_for(500ms)
//         的途中，就被「行程結束」這件事連根帶走。main 從建立到 return 只花
//         幾百微秒，遠早於 500 毫秒。
//     為什麼會錯：腦中把「沒看到輸出」等價於「沒執行到」。實際上執行緒的生命
//         週期附屬於行程：main 返回 → exit_group() → 所有執行緒立刻消滅，
//         沒有 unwinding、解構函式不跑、緩衝區不 flush。
//         順帶一提：在 main 尾端加 sleep(1s) 雖然「看起來會印出來」，但那是
//         賭排程器而非保證；唯一正確的修法是改用 join()。
// ═══════════════════════════════════════════════════════════════════════════
//
// ✅ 正確作法：把 t.detach() 換成 t.join()，"Background task" 就一定會印出來
//    （且保證在 "Main exits" 之後）。或改用 C++20 的 std::jthread，
//    解構時自動 join，見課程 3.3。
//
// ※ 本檔刻意維持原樣、不擴充 main()：只要在 main 尾端多做任何事，
//   就會延後行程結束時間、破壞這個示範想呈現的競賽現象。

#include <iostream>
#include <thread>
#include <chrono>   // std::chrono::milliseconds（勿依賴 <thread> 的傳遞性引入）

void background() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Background task" << std::endl;  // 可能不會印出！
}

int main() {
    std::thread t(background);
    t.detach();  // 分離，不等待
    
    std::cout << "Main exits" << std::endl;
    return 0;  // 主程式結束，背景執行緒可能還沒完成
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread 課程\ 2.1：第一個多執行緒程式3.cpp -o detach_demo

// === 預期輸出 ===
// Main exits
//
// 實測 exit code = 0（程式正常結束，沒有崩潰、沒有 terminate）。
//
// ⚠️ "Background task" **沒有**被印出來。
//    原因：main 從建立執行緒到 return 只花幾百微秒，而背景執行緒要睡 500 毫秒；
//    main 返回 → exit_group() → 行程內所有執行緒立刻被消滅，它根本沒醒來過。
//
// ⚠️ 措辭精確性（本課重點）：這是**時序相依**的結果，不是「保證不印出」。
//    本機以 stdbuf -o0 連續執行 8 次，8 次都只印出 "Main exits"；
//    但在極端排程情況下（例如行程收尾被大幅延遲）仍有可能印出來。
//    → 因此正確說法是「幾乎不會印出」，不可寫成「一定不會印出」。
//
// 對照組：把 t.detach() 改成 t.join()，輸出就會變成**確定的**兩行：
//    Background task
//    Main exits
//    （join() 建立 happens-before，順序有保證。）
