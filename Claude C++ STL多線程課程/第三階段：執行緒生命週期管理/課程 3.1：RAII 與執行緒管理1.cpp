// 檔案：lesson_3_1_exception_leaks_thread.cpp
// 說明：例外跳過 join()，導致 std::thread 解構時中止程式
//
// 【本檔是「刻意示範錯誤」的範例，執行時一定會 abort，這就是本課重點】
//
// ⚠️ 這【不是】未定義行為，而是標準規定的行為：
//    throw 之後 t.join() 被跳過，堆疊展開時 t 解構，
//    此時 t 仍是 joinable → 解構函式呼叫 std::terminate()。
//    例外因此【來不及】傳到 main 的 catch，"捕獲例外" 永遠不會印出。
//    → 一定會發生，不是隨機。
//
//    實測結果：exit code 134 (SIGABRT)，stderr 顯示
//    "terminate called without an active exception"。
//    （"工作中" 是否來得及印出取決於排程，兩種都可能。）
//
// ✅ 正確作法：用 RAII 包裝執行緒，讓解構函式保證 join()
//    —— 這正是本課接下來 ThreadGuard / ScopedThread 要解決的問題，
//    C++20 的 std::jthread 則已內建此行為。

// =============================================================================
//  課程 3.1：RAII 與執行緒管理1.cpp  —  例外跳過 join()：標準保證的 std::terminate
// =============================================================================
//
// 【主題資訊 Information】
//   ~thread();                       // C++11 起，標頭 <thread>
//   語意（[thread.thread.destr]）：
//       if (joinable()) std::terminate();
//       // 否則什麼都不做
//   bool joinable() const noexcept;  // 等價於 get_id() != std::thread::id{}
//
//   複雜度：O(1)。terminate() 預設呼叫 std::abort() → SIGABRT。
//   相關：std::set_terminate / std::get_terminate（<exception>）。
//
// 【詳細解釋 Explanation】
//
// 【1. 這裡到底發生了什麼——四個步驟】
//   ① riskyFunction() 建立 t，執行緒開始跑（可能還沒被排到 CPU）。
//   ② throw 觸發「堆疊展開（stack unwinding）」。
//   ③ 展開時，作用域內所有「已完整建構」的自動物件依建構的反序解構
//      → t 的解構函式被呼叫。
//   ④ 此刻 t 仍是 joinable（既沒 join 也沒 detach）
//      → ~thread() 依標準呼叫 std::terminate() → abort。
//
//   關鍵在於：例外的傳播「還沒走完」就被 terminate 打斷了。
//   main() 的 catch 區塊位於展開的終點，而程式在展開的中途就死了，
//   所以「捕獲例外」這行【永遠】不會印出——這不是機率問題。
//
// 【2. 為什麼這不是 UB，而是「標準保證」】
//   未定義行為（UB）的意思是「標準不規範，任何結果都合法」，
//   因此描述 UB 時不能說「一定會 crash」。
//   但本檔【不是】UB：標準明文規定 ~thread() 在 joinable 時呼叫 terminate。
//   這是「標準規定的、可預測的、每次都發生」的行為。
//   → 所以檔頭寫「一定會 abort」是精確的，不是誇飾。
//
//   請把這兩者分清楚，面試最愛考：
//     joinable 的 thread 被解構      → 標準保證 terminate（確定）
//     detach 後存取已消滅的區域變數  → UB（不確定，可能看似正常）
//
// 【3. 為什麼 try/catch 救不了它】
//   直覺會想「那我在 riskyFunction 裡包一層 catch 不就好了？」
//   包 catch 確實可以在展開【之前】接住例外，但只要 t 還是 joinable，
//   離開作用域時仍然會 terminate。catch 改變的是「例外往哪走」，
//   不是「t 有沒有被 join」。
//   真正的修法只有一個方向：讓「解構時一定會 join」變成型別的責任
//   → 這就是 RAII（見本課 2~5 號檔案）。
//
// 【4. 委員會為什麼選擇「炸掉」而不是自動處理】
//   ~thread() 有三種可能設計，標準選了第三種：
//     (a) 自動 detach：執行緒繼續跑，但它引用的區域變數已經消滅
//                      → 懸空存取，是「安靜的資料損毀」，最難除錯。
//     (b) 自動 join  ：解構函式默默阻塞，可能等到天荒地老，
//                      甚至死結；而且是「看不見的」長等待。
//     (c) terminate  ：立刻、在錯誤現場、每次都一樣地炸掉（fail-fast）。
//   (a)(b) 都會把 bug 藏起來，(c) 則保證你當場看到它。
//   歷史上 C++0x 草案一度採用 (a)，後來被 N2802（Howard Hinnant,
//   "A plea to reconsider detach-on-destruction for thread objects"）
//   說服改掉。C++20 的 std::jthread（P0660）最終選了 (b) 自動 join，
//   但配套 stop_token 讓「等待」可以被中斷——這是 (b) 當年最大的反對理由
//   被解掉之後，才敢做的取捨。
//
// 【概念補充 Concept Deep Dive】
//
// (A) std::thread 其實非常「薄」
//     本機實測（g++ 15.2 / x86-64 Linux）：sizeof(std::thread) = 8，
//     sizeof(std::thread::id) = 8。它裡面就只有一個 pthread_t。
//     joinable() 不過是拿它和「空 id」比較。
//     所以 terminate 不是因為「物件很複雜」，而是因為「這 8 bytes 代表
//     一條還在跑、還沒被回收的 OS 執行緒」。
//
// (B) 沒被 join 也沒被 detach 的執行緒，作業系統層面會怎樣
//     glibc 的 joinable pthread 在結束後仍保留 thread descriptor 與堆疊，
//     等著被 join 取回傳值——沒人 join 就一直留著（本機 ulimit -s = 8192 KiB，
//     即每條執行緒預設保留 8 MiB 虛擬位址空間；此值為實作定義）。
//     這正是標準要你「明確表態」的實際理由：join 或 detach，二選一。
//
// (C) 「terminate called without an active exception」為什麼這樣寫
//     本機實測 libstdc++ 的三種情況（訊息屬實作定義，非標準規定）：
//       ① joinable thread 解構，當下無例外        → without an active exception
//       ② joinable thread 在堆疊展開途中解構（本檔）→ without an active exception
//       ③ 在 catch 區塊內呼叫 std::terminate()     → after throwing an instance of 'X'
//     ②很反直覺：例外明明正在傳播，訊息卻說「沒有作用中的例外」。
//     原因是這句話判斷的是「例外是否已被某個 catch 接手」
//     （__cxa_begin_catch 是否執行過），而不是「是否有例外在飛」。
//     本檔的例外正在展開途中、還沒被任何 catch 接住，所以歸類為 ②。
//
// (D) exit code 134 的算法
//     134 = 128 + 6，6 是 SIGABRT。shell 對「被訊號終止」的行程
//     一律回報 128 + 訊號編號。看到 134 就該想到 abort()。
//
// 【注意事項 Pay Attention】
//   1. 本檔【故意】不能正常結束，這是教材設計，不是待修的 bug。
//   2. 「工作中」是否印得出來【每次執行都不同】：主執行緒可能在子執行緒
//      被排到 CPU 之前就 throw + terminate。本機實測 8 次有 3 次印出。
//      這一行是排程競賽，而 terminate 本身不是。
//   3. 收集這種會 abort 的程式輸出時要用 stdbuf -o0，否則 stdout 緩衝區
//      內容會隨 abort 一起消失，看到的輸出會比實際少。
//   4. 別把 t.join() 寫在 throw 後面就以為「有寫就好」——它是無法到達的
//      程式碼（-Wall -Wextra 在本機並不會對此發出警告，不能依賴編譯器提醒）。
//   5. 解構函式在 C++11 起隱式 noexcept：任何從解構函式逃出的例外
//      同樣會導致 terminate。這是下一個檔案要處理的陷阱。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】joinable 的 std::thread 被解構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 解構時如果還是 joinable，會發生什麼事？
//     答：標準規定直接呼叫 std::terminate()（[thread.thread.destr]），
//         預設 terminate handler 呼叫 abort()，程式收到 SIGABRT，
//         shell 看到 exit code 134（128+6）。這是標準保證的行為，
//         每次都發生，不是未定義行為、也不是機率事件。
//     追問：為什麼不設計成自動 join 或自動 detach？
//           → 自動 detach 會讓執行緒存取已消滅的區域變數（安靜的損毀）；
//             自動 join 會讓解構函式隱式阻塞、甚至死結。兩者都把 bug 藏起來，
//             所以標準選擇當場 fail-fast。C++20 的 jthread 因為有了
//             stop_token 讓等待可被中斷，才改採自動 join。
//
// 🔥 Q2. 本檔的例外為什麼傳不到 main 的 catch？
//     答：throw 觸發堆疊展開，展開途中 t 被解構並呼叫 terminate，
//         程式在例外抵達 catch 之前就結束了。terminate 發生在展開的中途，
//         而 catch 在展開的終點。所以「捕獲例外」永遠不會印出。
//     追問：那在 riskyFunction 內部加一層 try/catch 能救嗎？
//           → 不能。catch 只改變例外的流向，不會讓 t 變成非 joinable；
//             只要離開作用域時 t 仍 joinable，照樣 terminate。
//
// ⚠️ 陷阱. 這支程式「一定會 abort」，那是不是代表它的輸出也是固定的？
//     答：不是。「abort」是標準保證的，但「工作中」這行印不印得出來
//         取決於排程——主執行緒可能在子執行緒還沒被排到 CPU 前就
//         throw 並 terminate。本機實測 8 次只有 3 次印出。
//     為什麼會錯：多數人把「行為確定」和「輸出確定」畫上等號。
//         實際上這支程式同時包含兩種性質：解構→terminate 是【標準保證】，
//         子執行緒有沒有搶到時間片是【排程競賽】。描述教材時必須分開講，
//         否則就會寫出「一定會印出 工作中」這種經不起重跑的句子。
//
// ⚠️ 陷阱2. 用 catch (...) 包住整個 main，是不是就能把程式救回來？
//     答：不行。std::terminate() 不是丟例外，它是「直接結束程式」的呼叫，
//         沒有任何 catch 攔得住它。唯一能介入的是 std::set_terminate()
//         安裝自訂 handler，但那只能換一種死法（例如印診斷再 abort），
//         無法讓程式繼續正常執行。
//     為什麼會錯：把 terminate 想成「一種特別嚴重的例外」。它不是例外，
//         它是例外機制【放棄】之後的收尾動作，本質上和 abort() 同級。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stdexcept>
#include <thread>

void riskyFunction() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });

    // 💀 如果這裡拋出例外...
    throw std::runtime_error("發生錯誤！");

    t.join();  // 💀 永遠不會執行！
}  // 💀 t 解構時仍是 joinable → std::terminate()

int main() {
    try {
        riskyFunction();
    } catch (...) {
        std::cout << "捕獲例外" << std::endl;
    }
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.1：RAII 與執行緒管理1.cpp" -o raii1
// 執行: stdbuf -o0 ./raii1; echo "exit=$?"
//       （必須用 stdbuf -o0 關掉輸出緩衝：本檔以 abort 結束，緩衝區內容不會被
//         flush，直接執行會看到比實際更少的輸出。）

// 註 1:本檔【刻意】不能正常結束。退出碼固定為 134（128+6，SIGABRT），
//      stderr 固定印出 "terminate called without an active exception"
//      —— 這是標準規定 ~thread() 在 joinable 時呼叫 terminate 的結果,
//      不是未定義行為,每次執行都一樣。
//
// 註 2:但 stdout 的內容【每次執行都不同】。子執行緒的 "工作中" 能不能印出來,
//      取決於它有沒有在主執行緒 throw + terminate 之前被排到 CPU。
//      本機實測連跑 20 次:3 次印出 "工作中"、17 次 stdout 全空。
//      下方預期輸出取【多數情形】(stdout 全空);請勿把任一種當成保證。
//
// 註 3:"捕獲例外" 在任何一次執行都【不會】出現 —— 程式在堆疊展開的中途就被
//      terminate 打斷,例外根本走不到 main 的 catch。這一行是確定的。

// === 預期輸出 ===
// （stdout：多數情形為空，約 15% 的執行會多出一行 "工作中"）
//
// --- stderr ---
// terminate called without an active exception
//
// --- 退出碼 ---
// exit=134
