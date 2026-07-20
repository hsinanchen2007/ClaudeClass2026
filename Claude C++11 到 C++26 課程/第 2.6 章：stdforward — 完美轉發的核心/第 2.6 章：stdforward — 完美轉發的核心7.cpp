// =============================================================================
//  第 2.6 章 範例 7  —  多層轉發：值類別如何穿越任意深度的呼叫鏈
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>
//   本檔結構：layer1 → layer2 → layer3 → final_target，每層都是
//     template<class T> void layerN(T&& arg) { layerN+1(std::forward<T>(arg)); }
//   複雜度：轉發本身零成本；三層轉發在開啟最佳化後通常被完全內聯掉。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「每一層都要寫 forward」是不可省略的】
//   值類別的資訊在每一層都會「重置」：不論上一層傳來的是左值還是右值，
//   進入 layer2 之後，arg 這個名字本身又變回左值。
//   所以只要有任何一層漏寫 forward，從那一層之後就永遠是左值——
//   而且下游的每一層都還在正確地呼叫 forward，看起來完全沒問題，
//   只是全部都在轉發一個「已經被降級成左值」的東西。
//   這種 bug 沒有編譯錯誤、沒有警告，只會表現成「移動優化莫名其妙失效」。
//
// 【2. 型別在三層之間是怎麼傳遞的】
//   傳入左值 s（std::string）：
//     layer1 的 T 推導為 std::string&  → forward<std::string&> → 左值
//     layer2 的 T 也推導為 std::string& → 左值
//     layer3 的 T 也推導為 std::string& → final_target(const std::string&)
//   傳入右值 std::string("tmp")：
//     layer1 的 T 推導為 std::string   → forward<std::string> → 右值
//     ...一路推導為 std::string        → final_target(std::string&&)
//   關鍵觀察：T 在每一層都被「重新推導」，但因為上一層 forward 出來的
//   運算式值類別正確，推導結果才會一致。這就是所謂的「轉發鏈」。
//
// 【3. 為什麼是 T&& 而不是把型別寫死】
//   若中間某層寫成 void layer2(const std::string& arg)，
//   整條鏈就在那裡斷掉——不只值類別遺失，連 std::string 以外的型別
//   都無法穿過。轉發參考同時保住了「型別」與「值類別」兩件事。
//
// 【概念補充 Concept Deep Dive】
//   (A) 執行期成本為零：每一層的 forward 都是 static_cast，
//       在 -O2 下這三層函式呼叫通常會被完全內聯，最終機器碼與
//       直接呼叫 final_target 相同。轉發是編譯期的抽象，不是執行期的間接層。
//   (B) 為什麼實務上會出現「多層」？框架幾乎都長這樣：
//       使用者呼叫 → 執行緒池排程 → 任務包裝 → 真正的工作函式。
//       std::thread、std::async、std::bind 內部都是多層轉發。
//   (C) 這條鏈上只有「最後一層」真正把資源搬走。中間層雖然都寫了 forward，
//       但它們只是把授權往下傳，沒有人真的動手。因此中間層可以安全地
//       在 forward 之前讀取 arg（例如印 log），只要不是在 forward 之後。
//
// 【注意事項 Pay Attention】
//   1. 漏寫任一層的 forward，不會有編譯錯誤，只會靜默失去移動優化。
//   2. 中間層在 forward 之後就不可以再使用 arg。
//   3. 中間層若把 arg 存進成員或容器，等於提前把資源用掉，
//      下游拿到的會是空殼——這是框架程式碼裡真實發生過的 bug。
//   4. 每層都要用 T&& + forward<T>；寫成 auto&& 參數（C++20 縮寫模板）
//      時，forward 要寫 std::forward<decltype(arg)>(arg)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多層完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 三層轉發中，若 layer2 漏寫 std::forward，會發生什麼？編譯會過嗎？
//     答：編譯完全會過，也不會有警告。但從 layer2 開始，傳下去的永遠是左值，
//         最終呼叫到 final_target(const std::string&)。
//         結果是「移動優化靜默失效」——效能變差但功能正確，極難察覺。
//     追問：怎麼在測試中抓到這種 bug？
//         → 用會計數複製/移動次數的探針型別，斷言「複製次數 == 0」。
//
// 🔥 Q2. 為什麼每一層都要重新 forward，值類別不能「自動」一路傳下去？
//     答：因為每一層的參數 arg 都是具名變數，具名即左值。
//         值類別資訊被保存在模板參數 T 裡，而不是在 arg 這個運算式上；
//         forward<T> 的工作就是每層重新把 T 裡的資訊「兌現」成正確的值類別。
//
// ⚠️ 陷阱. 中間層寫成 void layer2(const std::string& arg) 為什麼會毀掉整條鏈？
//     答：不只值類別遺失（之後永遠是左值），連泛型也一起沒了——
//         原本可以穿過任何型別的鏈，現在只能收 std::string。
//         下游即使全部正確使用 forward，也救不回來。
//     為什麼會錯：常見的想法是「反正中間層只是傳遞，用 const& 最省」。
//         但 const& 是「唯讀借用」的語意，它明確地丟棄了右值身分；
//         轉發鏈要的是「原樣代理」，這兩者根本不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>

// 參數刻意不命名：本示範只關心選到哪個多載，不使用參數值
// （命名但不使用會觸發 -Wextra 的 -Wunused-parameter）。
void final_target(const std::string&) { std::cout << "  最終: 左值\n"; }
void final_target(std::string&&)      { std::cout << "  最終: 右值\n"; }

// 第三層
template<typename T>
void layer3(T&& arg) {
    std::cout << "  layer3 → final_target\n";
    final_target(std::forward<T>(arg));
}

// 第二層
template<typename T>
void layer2(T&& arg) {
    std::cout << "  layer2 → layer3\n";
    layer3(std::forward<T>(arg));
}

// 第一層
template<typename T>
void layer1(T&& arg) {
    std::cout << "  layer1 → layer2\n";
    layer2(std::forward<T>(arg));
}

// -----------------------------------------------------------------------------
// 對照組：中間層「漏寫 forward」的斷鏈示範
//   broken2 沒有寫 std::forward，因此不論上游傳什麼，往下都變成左值。
//   注意：這段程式完全合法、沒有警告，只是移動優化靜默失效。
// -----------------------------------------------------------------------------
template<typename T>
void broken3(T&& arg) {
    final_target(std::forward<T>(arg));      // 這層寫對了，但已經來不及
}

template<typename T>
void broken2(T&& arg) {
    broken3(arg);                            // ✗ 漏寫 forward → 從這裡開始永遠是左值
}

template<typename T>
void broken1(T&& arg) {
    broken2(std::forward<T>(arg));           // 這層也寫對了
}

// -----------------------------------------------------------------------------
// 【日常實務範例】任務佇列：使用者 → 排程器 → 工作執行緒的三層轉發
//   情境：執行緒池的 submit(task, args...) 要把使用者的引數一路帶到
//         真正執行工作的地方。中間可能經過「排程器」「批次包裝」等多層。
//         實務上這條鏈只要有一層忘了 forward，使用者傳進來的大型
//         payload（影像緩衝、封包內容）就會被多複製一次——
//         在高吞吐服務上這是很典型的效能退化來源。
//   本例以「提交日誌訊息」示範，用 sink 印出它收到的是複製還是移動。
// -----------------------------------------------------------------------------
void logSink(const std::string& msg) {
    std::cout << "    [sink] 複製收下: " << msg << "\n";
}
void logSink(std::string&& msg) {
    std::cout << "    [sink] 移動收下: " << msg << "\n";
}

template<typename M>
void workerRun(M&& msg) { logSink(std::forward<M>(msg)); }

template<typename M>
void scheduler(M&& msg) {
    std::cout << "  [scheduler] 排入佇列\n";
    workerRun(std::forward<M>(msg));
}

template<typename M>
void submitLog(M&& msg) {
    std::cout << "  [submit] 接收使用者訊息\n";
    scheduler(std::forward<M>(msg));
}

int main() {
    std::string s = "Hello";

    std::cout << "傳入左值，穿越三層:\n";
    layer1(s);

    std::cout << "\n傳入右值，穿越三層:\n";
    layer1(std::string("tmp"));

    std::cout << "\n=== 對照組：中間層漏寫 forward（斷鏈） ===\n";
    std::cout << "斷鏈版，同樣傳右值:\n";
    broken1(std::string("tmp"));
    std::cout << "  ↑ 同樣傳右值，結果卻是左值：這就是漏寫 forward 的代價\n";

    std::cout << "\n=== 日常實務：任務佇列三層轉發 ===\n";
    std::string userMsg = "disk usage 91%";
    std::cout << "  提交左值（呼叫端還要用，應複製）:\n";
    submitLog(userMsg);
    std::cout << "  提交右值（用完即丟，應移動）:\n";
    submitLog(std::string("service restarted"));
    std::cout << "  呼叫端的 userMsg 完好: \"" << userMsg << "\"\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心7.cpp" -o multilayer

// === 預期輸出 ===
// 傳入左值，穿越三層:
//   layer1 → layer2
//   layer2 → layer3
//   layer3 → final_target
//   最終: 左值
//
// 傳入右值，穿越三層:
//   layer1 → layer2
//   layer2 → layer3
//   layer3 → final_target
//   最終: 右值
//
// === 對照組：中間層漏寫 forward（斷鏈） ===
// 斷鏈版，同樣傳右值:
//   最終: 左值
//   ↑ 同樣傳右值，結果卻是左值：這就是漏寫 forward 的代價
//
// === 日常實務：任務佇列三層轉發 ===
//   提交左值（呼叫端還要用，應複製）:
//   [submit] 接收使用者訊息
//   [scheduler] 排入佇列
//     [sink] 複製收下: disk usage 91%
//   提交右值（用完即丟，應移動）:
//   [submit] 接收使用者訊息
//   [scheduler] 排入佇列
//     [sink] 移動收下: service restarted
//   呼叫端的 userMsg 完好: "disk usage 91%"
