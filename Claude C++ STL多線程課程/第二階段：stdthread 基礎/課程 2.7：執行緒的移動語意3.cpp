// =============================================================================
//  課程 2.7：執行緒的移動語意 3  —  從函式回傳 std::thread（工廠函式）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：std::thread makeWorker(args...);        標頭檔：<thread>
//   標準：C++11 起可回傳（move-only 型別可依值回傳）
//         C++17 起 return std::thread(...) 這種 prvalue 回傳為
//         **保證省略複製/移動**（guaranteed copy elision，P0135R1）
//
//   為什麼 move-only 型別可以依值回傳：
//     return 陳述式會把回傳值「移動建構」到呼叫端的物件，
//     不需要複製建構子。thread 有移動建構子，所以完全合法。
//
// 【詳細解釋 Explanation】
//
// 【1. 工廠函式模式：把「怎麼建執行緒」封裝起來】
// 直接寫 std::thread t(worker, arg1, arg2, ...) 有幾個問題：
//   * 建立執行緒的細節（要餵哪些參數、要不要 std::ref、要不要先做初始化）
//     散落在每個呼叫點；
//   * 想加一層「建立前先檢查資源」「建立後登記到監控表」時，
//     必須改所有呼叫點。
// 工廠函式把這些收斂到一個地方，呼叫端只寫 auto t = makeWorker(42);
// 這與 std::make_unique / std::make_shared 是同一個設計動機。
//
// 【2. return 的時候到底發生了什麼：三種情境要分清楚】
//
//   (a) return std::thread([]{...});          ← 回傳的是 prvalue（純右值）
//       C++17 起這是 **保證省略**：那個 thread 物件根本不會先在函式內部
//       建立再搬走，而是**直接在呼叫端的目標記憶體上建構**。
//       連移動建構子都不會被呼叫（即使你把它 delete 掉也照樣編得過）。
//       這不是最佳化，是標準規定的語意。
//
//   (b) std::thread t(...); return t;         ← 回傳的是具名區域變數
//       這叫 NRVO（Named Return Value Optimization）。編譯器**可以**（但
//       C++17 也**不保證**）省略。若沒省略，標準規定回傳具名區域變數時
//       會先嘗試把它當 rvalue 處理（[class.copy.elision]/3），
//       所以會選中移動建構子 —— 依然合法、依然不需要複製。
//
//   (c) return std::move(t);                  ← 多此一舉，而且有害
//       顯式 std::move 會**阻止 NRVO**（因為回傳的不再是具名物件本身，
//       而是一個 xvalue 運算式），逼編譯器一定要做一次移動。
//       對 thread 而言那次移動很便宜（8 bytes），但這是壞習慣：
//       對 vector/string 這類型別會實際造成效能損失。GCC 有
//       -Wpessimizing-move / -Wredundant-move 會警告這種寫法。
//       **規則：回傳區域變數時，永遠不要加 std::move。**
//
// 【3. 為什麼工廠函式回傳 thread 是安全的（不會 terminate）】
// 呼叫端寫 std::thread t = createThread(42);
// 這是**移動建構**（或被完全省略），不是移動賦值 —— t 是全新物件，
// 不可能處於 joinable 狀態，因此不會踩到「移動賦值到 joinable 物件」
// 那個 terminate 陷阱（見檔 6）。
//
// 但下面這樣寫就要小心：
//     std::thread t = createThread(1);
//     t = createThread(2);        // ← 移動賦值！t 此時仍 joinable → terminate
// 正解是先 t.join(); 再賦值。
//
// 【4. 工廠函式的回傳值不可以被忽略】
// 呼叫 createThread(42); 卻不接住回傳值，會發生什麼？
// 那個暫時的 thread 物件在整條陳述式結束時解構，此時它仍 joinable
// → 解構子呼叫 std::terminate() → 程式 abort。
// 這是很容易犯的錯，所以工廠函式應該標上 [[nodiscard]]（C++17），
// 讓「忘記接住」變成編譯期警告。本檔的 makeReportWorker 就這樣做。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 保證省略（C++17）的真正機制
// C++17 重新定義了 prvalue：prvalue 不再是「一個暫時物件」，而是
// 「一份用來初始化物件的配方」。物件要到被實際使用（materialize）時才誕生。
// 所以 return std::thread(f); 的那個 prvalue 直接拿去初始化呼叫端的變數，
// 中間根本沒有第二個物件存在，自然沒有東西需要搬 —— 這就是為什麼
// 「即使移動建構子被 delete 也能編過」。C++11/14 時期則只是「允許省略」，
// 但仍要求移動建構子必須可用（accessible）。
//
// (B) 兩層省略：函式內 → 回傳值 → 呼叫端變數
// 完整的 return std::thread(f); 傳到 auto t = createThread(); 理論上有
// 兩次可省略點（區域 → 回傳暫存 → 目標變數）。C++17 之後這條路徑上的
// prvalue 全部合併，最終只有一個 thread 物件從頭到尾存在。
//
// (C) 為什麼不用回傳 std::unique_ptr<std::thread>
// 有人會想「thread 不能複製，那我包一層指標比較好傳」。這是多餘的：
// thread 本身已經是 move-only 的握把，再包一層 unique_ptr 只是多一次
// 堆積配置與一層間接存取，語意上沒有任何增益。
// 真正需要包一層的理由只有兩個：需要多型（thread 沒有虛擬函式，用不到），
// 或需要延遲建構（用 std::optional<std::thread> 更輕量）。
//
// (D) C++20 的 std::jthread 也可以這樣回傳
// jthread 同樣是 move-only，工廠函式模式完全適用，而且更安全 ——
// 呼叫端就算忘了 join，jthread 的解構子會自動 request_stop() + join()，
// 不會 terminate。
//
// 【注意事項 Pay Attention】
// 1. 回傳區域 thread 變數時**不要**寫 return std::move(t);，
//    會抑制 NRVO 且觸發 -Wpessimizing-move。
// 2. 工廠函式建議加 [[nodiscard]]：忽略回傳值 → 暫存物件解構時 joinable
//    → std::terminate()。這是標準保證的 abort，不是 UB。
// 3. 工廠函式內若在啟動執行緒**之後**才丟例外，那條執行緒已經在跑了；
//    要嘛先做完所有可能失敗的事再啟動，要嘛用 RAII 包裝（見檔 7）。
// 4. lambda 用 [id] 以**值**捕捉是必要的：工廠函式的參數 id 在函式回傳後
//    就消滅了，若寫成 [&id] 捕捉參考，執行緒讀到的是懸空參考 → UB。
// 5. 輸出順序非決定性：工廠回傳後執行緒可能已經跑完、可能還沒開始。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】回傳 std::thread
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::thread 不能複製，為什麼函式可以「依值回傳」它？
//     答：因為依值回傳只需要移動建構子，不需要複製建構子。
//         C++17 起 return std::thread(...) 這種 prvalue 更是「保證省略」，
//         物件直接在呼叫端建構，連移動都不會發生。
//     追問：那 C++11 呢？→ C++11/14 允許省略但不保證，而且要求移動建構子
//           必須可用；thread 有移動建構子，所以一樣編得過。
//
// 🔥 Q2. return t; 與 return std::move(t); 有什麼差別？該用哪個？
//     答：用 return t;。回傳具名區域變數時，標準規定編譯器會先把它當
//         rvalue 處理，所以自動選中移動建構子，而且還有機會套用 NRVO
//         把移動完全省掉。加上 std::move 反而**抑制 NRVO**，強迫做一次
//         移動，GCC 會發出 -Wpessimizing-move 警告。
//     追問：有沒有例外？→ 有。當回傳型別與區域變數型別**不同**時
//           （例如區域是 unique_ptr<Derived>、回傳 unique_ptr<Base>），
//           隱式的 rvalue 處理在 C++20 前不適用，需要顯式 std::move。
//
// ⚠️ 陷阱. 呼叫 createThread(42); 但不接住回傳值，會發生什麼？
//     答：暫存的 thread 物件在陳述式結尾解構，此時 joinable() == true，
//         解構子依標準呼叫 std::terminate() → 程式 abort（SIGABRT）。
//         **這是標準保證的行為，不是 UB。**
//     為什麼會錯：多數人以為「沒接住就是被丟掉，頂多執行緒變成孤兒繼續跑」，
//         那是 detach 的語意。thread 的解構子刻意選擇 abort 而不是自動
//         detach，就是要逼你明確表態。防禦方式：工廠函式加 [[nodiscard]]。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <thread>
#include <vector>

// 原始示範：最單純的工廠函式
std::thread createThread(int id) {
    return std::thread([id]() {          // [id] 必須以值捕捉：參數在回傳後即消滅
        std::cout << "執行緒 " << id << std::endl;
    });
    // 這裡是 prvalue → C++17 保證省略，連移動建構子都不會被呼叫
}

// -----------------------------------------------------------------------------
// 【日常實務範例】報表匯出服務：工廠函式回傳「已啟動」的 worker
// 情境：批次系統要為每個部門啟動一條匯出執行緒。建立執行緒之前要先驗參數、
//       之後要登記到監控表 —— 這些細節統一收在工廠函式裡，呼叫端只管拿走握把。
// 重點 1：[[nodiscard]] 讓「忘了接住回傳值」變成編譯期警告，
//         而不是執行期的 std::terminate()。
// 重點 2：所有可能失敗的檢查都放在「啟動執行緒之前」，
//         避免執行緒已經跑起來才丟例外、變成沒人負責 join 的孤兒。
// -----------------------------------------------------------------------------
[[nodiscard]] std::thread makeReportWorker(std::string dept, int rows) {
    // ── 先做完所有可能失敗的事 ──────────────────────────
    if (rows <= 0) {
        throw std::invalid_argument("rows 必須為正數");
    }
    // ── 檢查都過了才啟動執行緒 ──────────────────────────
    return std::thread([dept = std::move(dept), rows]() {   // 以值捕捉，安全
        std::cout << "  [worker] 匯出 " << dept
                  << " 部門，共 " << rows << " 筆\n";
    });
}

int main() {
    std::cout << "=== 原始示範：工廠函式回傳 thread ===\n";
    std::thread t = createThread(42);   // 移動建構（實際被省略）；t 是新物件，安全
    t.join();

    std::cout << "\n=== 重新指派要小心：這是移動賦值，不是移動建構 ===\n";
    // t 現在已 join 過 → non-joinable → 下面這行才安全
    std::cout << "t.joinable() = " << std::boolalpha << t.joinable()
              << "（false 才可以移動賦值）\n";
    t = createThread(7);               // 安全：t 目前是 non-joinable
    t.join();
    // 若上面沒有先 join 就再 t = createThread(...)，會呼叫 std::terminate()（見檔 6）

    std::cout << "\n=== 實務：報表匯出工廠 ===\n";
    std::vector<std::thread> workers;
    workers.push_back(makeReportWorker("財務", 1200));
    workers.push_back(makeReportWorker("人資", 350));
    for (auto& w : workers) w.join();

    std::cout << "\n=== 實務：啟動前就失敗，不會留下孤兒執行緒 ===\n";
    try {
        std::thread bad = makeReportWorker("業務", -1);
        bad.join();
    } catch (const std::invalid_argument& e) {
        std::cout << "捕捉到 std::invalid_argument: " << e.what() << "\n";
        std::cout << "（因為檢查在啟動執行緒之前，所以沒有任何執行緒被建立）\n";
    }

    // 注意：下面這行若解除註解，GCC 會發出 -Wunused-result 警告，
    //       而且執行期會因暫存 thread 解構時仍 joinable 而 std::terminate()。
    // makeReportWorker("測試", 1);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 2.7：執行緒的移動語意3.cpp" -o move3

// === 預期輸出 ===
// 註：以下為某次實際執行結果。子執行緒的輸出（"執行緒 42"、"[worker] ..."）
//     何時出現取決於 OS 排程，**每次執行都可能不同**；
//     但因為每段之後都有 join()，段與段之間的順序是有保證的。
//
// === 原始示範：工廠函式回傳 thread ===
// 執行緒 42
//
// === 重新指派要小心：這是移動賦值，不是移動建構 ===
// t.joinable() = false（false 才可以移動賦值）
// 執行緒 7
//
// === 實務：報表匯出工廠 ===
//   [worker] 匯出 財務 部門，共 1200 筆
//   [worker] 匯出 人資 部門，共 350 筆
//
// === 實務：啟動前就失敗，不會留下孤兒執行緒 ===
// 捕捉到 std::invalid_argument: rows 必須為正數
// （因為檢查在啟動執行緒之前，所以沒有任何執行緒被建立）
