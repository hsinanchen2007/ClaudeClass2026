// =============================================================================
//  第 2.6 章 範例 6  —  通用計時包裝器：連「函式本身」也要完美轉發
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（forward）、<chrono>（計時）
//   本檔樣式：
//     template<class Func, class... Args>
//     auto timed_call(Func&& func, Args&&... args)
//         -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...));
//   這個尾端回傳型別寫法是 C++11 的做法；C++14 起可直接寫 decltype(auto)。
//
// 【詳細解釋 Explanation】
//
// 【1. 包裝器的鐵律：不可以改變被包裝者的任何語意】
//   計時器唯一該做的事是「量時間」。呼叫端原本呼叫 process(input, 10000) 會發生什麼，
//   包了 timed_call 之後就該一模一樣：同樣的多載、同樣的複製/移動次數、
//   同樣的回傳值類別。任何一點偏差都會讓「加上計時後效能就變了」，
//   量到的數字反而失去意義。完美轉發正是達成這件事的唯一手段。
//
// 【2. 為什麼連 Func 都要寫成 Func&& 並 forward】
//   函式物件不一定是無狀態的。lambda 可以捕捉、可以是 mutable，
//   也可能有「只在右值上才能呼叫」的 operator()（用 && 修飾）：
//       struct OnlyRvalue { void operator()() && { } };
//   如果包裝器把它當左值呼叫，這種型別根本編譯不過。
//   寫 std::forward<Func>(func)(...) 才能讓右值可呼叫物件維持右值身分。
//
// 【3. 回傳型別為什麼要用 decltype(...) 而不是 auto】
//   單寫 auto 會做 decay：回傳參考時會被降級成「值」，多一次複製；
//   回傳 int& 會變成 int，呼叫端就改不到原物件了。
//   -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
//   保留了完整的參考性與 const 性。C++14 的 decltype(auto) 是同一件事的簡寫。
//
// 【4. 本檔仍有一處不完美：auto result = ...】
//   result 被宣告成 auto（值），所以被包裝的函式回傳值會先被搬進 result，
//   離開時再回傳一次。要做到滿分的轉發，需要 C++17 的寫法：
//       decltype(auto) result = std::forward<Func>(func)(...);
//       return static_cast<decltype(result)>(result);
//   而且回傳 void 的函式在本檔的寫法下會編譯失敗（不能宣告 void 變數）。
//   保留現狀是為了讓範例聚焦在「引數轉發」，但這個限制要知道。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼計時結果要印到 stderr？因為耗時每次執行都不同。
//       把不可重現的內容混進 stdout，會讓自動化測試無法比對輸出。
//       本檔已改成 std::cerr，因此下方「預期輸出」是穩定可重現的。
//   (B) high_resolution_clock 在 libstdc++ 上是 system_clock 的別名，
//       會受系統時間調整影響。要量「一段程式跑多久」，
//       正確的選擇是 std::chrono::steady_clock（保證單調遞增）。
//   (C) 計時包裝器量到的是「牆鐘時間」，包含被排程搶走的時間。
//       要量 CPU 時間得用 clock() 或 getrusage()。
//
// 【注意事項 Pay Attention】
//   1. 引數只轉發一次。若包裝器要在呼叫前先印出引數，就不能再 forward 它們。
//   2. 本檔寫法不支援回傳 void 的被包裝函式（auto result 無法宣告成 void）。
//   3. 計時本身有開銷（兩次 clock 讀取，數十奈秒等級）；量微秒級的函式時
//      這個開銷不可忽略，應改成「跑 N 次取平均」。
//   4. 開啟最佳化後，編譯器可能把「結果沒被使用」的呼叫整段刪掉，量到 0。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】完美轉發包裝器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 包裝器為什麼連可呼叫物件 Func 也要寫成 Func&& 並轉發？
//     答：因為可呼叫物件可能有狀態，也可能把 operator() 用 && 修飾成
//         「只有右值能呼叫」。若包裝器一律以左值呼叫它，這類型別會編譯失敗；
//         有 mutable 捕捉的 lambda 也會因 const 化而無法修改自身狀態。
//     追問：那 std::invoke 又解決了什麼？
//         → 它額外支援成員函式指標與成員資料指標，寫法統一成 invoke(f, args...)。
//
// 🔥 Q2. 為什麼回傳型別要寫 -> decltype(...)，而不是直接 auto？
//     答：auto 會套用「值推導」而 decay 掉參考與 const。被包裝的函式若回傳
//         int&，包裝後會變成 int，呼叫端拿到的是副本，改了也沒用。
//         decltype(...)（或 C++14 的 decltype(auto)）才能原樣保留。
//
// ⚠️ 陷阱. 在包裝器裡先 std::cout << arg 印出引數、再 forward 給目標函式，
//          為什麼可能出事？
//     答：印出來那一步沒問題，問題在「印完之後才 forward」其實是安全的，
//         但反過來——先 forward 再使用——就會拿到被搬空的物件。
//         真正的陷阱是在迴圈或重試邏輯裡重複 forward 同一個引數：
//         第一次成功搬走後，第二次拿到的是空殼。
//     為什麼會錯：大家把 forward 當成「不做事的型別標註」，
//         忽略它是一張「授權下游把資源搬走」的一次性通行證。
//         重試包裝器因此不能轉發引數，只能以左值重複傳遞。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <chrono>

// 通用的計時包裝器
template<typename Func, typename... Args>
auto timed_call(Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    auto start = std::chrono::high_resolution_clock::now();

    // 完美轉發函式物件和所有引數
    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    // ★ 耗時每次執行都不同 → 寫到 stderr，讓 stdout 保持可重現、可自動比對
    std::cerr << "  [計時] 耗時: " << us << " us\n";

    return result;
}

std::string process(const std::string& input, int repeat) {
    std::string result;
    for (int i = 0; i < repeat; ++i) result += input;
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】用「計數」取代「計時」的可重現版本包裝器
//   情境：效能回歸測試不能靠牆鐘時間——同一台機器上跑兩次就不一樣，
//         CI 上更是天差地遠。實務做法是量「確定性的指標」：
//         配置次數、比較次數、複製次數。這種數字每次跑都完全相同，
//         可以直接寫進單元測試斷言。
//   本例包裝器統計「被包裝函式被呼叫幾次、字串總共成長多少位元組」。
// -----------------------------------------------------------------------------
struct CallStats {
    int calls = 0;
    std::size_t bytes_produced = 0;
};

template<typename Func, typename... Args>
auto counted_call(CallStats& stats, Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    ++stats.calls;
    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);
    stats.bytes_produced += result.size();
    return result;
}

int main() {
    std::string input = "Hello";
    auto result = timed_call(process, input, 10000);
    std::cout << "結果長度: " << result.size() << "\n";
    std::cout << "輸入字串未被破壞: \"" << input << "\"\n";

    std::cout << "\n=== 日常實務：可重現的計數式包裝器 ===\n";
    {
        CallStats stats;
        auto a = counted_call(stats, process, std::string("ab"), 3);
        auto b = counted_call(stats, process, input, 2);
        std::cout << "  process(\"ab\", 3) = " << a << "\n";
        std::cout << "  process(\"Hello\", 2) = " << b << "\n";
        std::cout << "  呼叫次數 = " << stats.calls << "\n";
        std::cout << "  產生位元組總數 = " << stats.bytes_produced << "\n";
        std::cout << "  （這些數字每次執行完全相同，可直接寫成測試斷言）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.6 章：stdforward — 完美轉發的核心6.cpp" -o timed_call

// 註：timed_call 印出的耗時每次執行都不同，已改寫到 stderr，
//     故不出現在下方（stdout）的預期輸出中。要看它請執行時不要丟棄 stderr。

// === 預期輸出 ===
// 結果長度: 50000
// 輸入字串未被破壞: "Hello"
//
// === 日常實務：可重現的計數式包裝器 ===
//   process("ab", 3) = ababab
//   process("Hello", 2) = HelloHello
//   呼叫次數 = 2
//   產生位元組總數 = 16
//   （這些數字每次執行完全相同，可直接寫成測試斷言）
