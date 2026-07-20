// =============================================================================
//  第 2.7 章 範例 5  —  日誌與重試包裝器：完美轉發「該用」與「不該用」的分界
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<utility>（forward）、<stdexcept>（runtime_error）、<string>
//   兩個包裝器的關鍵差異：
//     logged_call(tag, func, args...)   → 只呼叫一次 → 可以、也應該轉發
//     retry(n, func, args...)           → 可能呼叫多次 → 絕對不可以轉發
//   本檔是整個完美轉發章節裡最重要的一課：轉發不是越多越好。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「只呼叫一次」才可以轉發】
//   std::forward 的語意是「我把這個物件的資源交給你，之後我不再用它」。
//   這是一張一次性的通行證。logged_call 呼叫目標函式恰好一次，
//   之後不再碰引數，所以交出去完全安全。
//
// 【2. 為什麼重試包裝器轉發會壞掉（本章最容易被寫錯的地方）】
//   假設 retry 內部寫成 return func(std::forward<Args>(args)...);
//     * 第 1 次嘗試：若引數是右值，目標函式可能把它的內容搬走
//     * 第 1 次失敗，進入第 2 次嘗試
//     * 第 2 次拿到的是「已經被掏空」的物件——空字串、空容器
//   結果是重試看起來有在跑，但送出去的是空資料。
//   最惡毒的地方在於：第一次就成功時完全正常，只有在「真的需要重試」時才出錯，
//   而那正是線上服務出狀況、最不希望再出第二個問題的時候。
//
// 【3. 正確做法與它的代價】
//   本檔的 retry 用 func(args...)（不轉發），代價是：
//     * 引數若是右值，本來可以移動，現在每次都複製
//   這個代價是刻意付出的——正確性優先於效能。
//   若真的很在意，進階做法是「前 n-1 次用複製，最後一次才轉發」，
//   但這需要在迴圈中判斷是否為最後一輪，複雜度上升且容易寫錯，
//   實務上除非量測證明是瓶頸，否則不值得。
//
// 【4. retry 的尾端回傳型別為什麼寫 decltype(func(args...))】
//   注意它寫的是 func(args...) 而不是 forward 版本——這是刻意保持一致：
//   宣告的回傳型別要反映「實際會怎麼呼叫」。若這裡寫 forward 版而本體不轉發，
//   在某些「左值與右值多載回傳不同型別」的極端情況下兩者會不一致。
//
// 【概念補充 Concept Deep Dive】
//   (A) 為什麼 retry 迴圈後面還要 throw？因為編譯器無法證明迴圈一定會回傳
//       （max_attempts 可能 <= 0）。少了這行會有「control reaches end of
//       non-void function」的警告。這是防禦性程式碼，正常路徑不會走到。
//   (B) 本檔用全域計數器 unstable_counter 模擬「前兩次失敗、第三次成功」，
//       讓輸出完全可重現——不使用亂數，正是為了讓預期輸出穩定。
//   (C) 例外安全：retry 在最後一次仍失敗時原樣 rethrow，
//       保留了原始的例外型別與訊息，這是包裝器該有的禮貌。
//
// 【注意事項 Pay Attention】
//   1. 會重複呼叫目標的包裝器（retry / 迴圈 / 快取失效重算）一律不可轉發引數。
//   2. 同理，把引數存進成員或容器之後又轉發，也會拿到空殼。
//   3. logged_call 的 auto result 會讓回傳值多一次搬移；
//      要完全零成本需 C++14 的 decltype(auto)，且無法支援回傳 void 的函式。
//   4. fetch_data 的 timeout 參數在本示範中未使用，故刻意不命名以避免
//      -Wunused-parameter；真實實作當然會用到它。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】包裝器中的轉發時機
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼重試包裝器裡「不可以」使用 std::forward？
//     答：forward 等於授權目標函式把引數的資源搬走。第一次嘗試若失敗，
//         引數可能已被掏空，第二次重試就送出空字串／空容器。
//         正確做法是以左值重複傳遞 func(args...)，用一點複製成本換正確性。
//     追問：這個 bug 為什麼特別難發現？
//         → 第一次就成功時行為完全正常，只有真的需要重試時才出錯——
//           也就是線上剛好出狀況的那一刻。
//
// 🔥 Q2. 那什麼樣的包裝器才可以轉發？
//     答：只呼叫目標「恰好一次」、而且轉發之後不再使用該引數的包裝器。
//         logged_call、計時器、單次委派的工廠都屬於這一類。
//         判準很簡單：問自己「這個引數之後還會被讀到嗎？」
//
// ⚠️ 陷阱. 「那我在 retry 的最後一次才 forward 不就兩全其美了？」
//          這個想法哪裡有問題？
//     答：想法本身方向正確，但實作上必須在迴圈中判斷是否為最後一輪，
//         而且一旦「最後一次」的判斷與例外處理路徑不同步（例如提前 break、
//         或某次拋出的不是預期的例外型別），就會在某條路徑上轉發兩次。
//         這類程式碼的錯誤成本很高，收益卻只是省下幾次複製。
//     為什麼會錯：大家傾向把「能優化」當成「該優化」。
//         正確的順序是先確保語意正確，再用量測決定是否值得引入複雜度——
//         而這裡的複雜度是「正確性風險」，不只是可讀性。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <chrono>
#include <functional>
#include <stdexcept>

// ===== 日誌包裝器 =====
template<typename Func, typename... Args>
auto logged_call(const std::string& tag, Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    std::cout << "[LOG] " << tag << " 開始\n";

    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);

    std::cout << "[LOG] " << tag << " 結束\n";
    return result;
}

// ===== 重試包裝器 =====
template<typename Func, typename... Args>
auto retry(int max_attempts, Func&& func, Args&&... args)
    -> decltype(func(args...))
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            std::cout << "  嘗試第 " << attempt << " 次...\n";
            // 注意：這裡不能 forward args，因為要重複使用
            // 只有最後一次才 forward（但這裡無法預知哪次成功）
            // 所以用普通傳遞，犧牲一點效率換取正確性
            return func(args...);
        } catch (const std::exception& e) {
            std::cout << "  失敗: " << e.what() << "\n";
            if (attempt == max_attempts) throw;
        }
    }
    throw std::runtime_error("unreachable");
}

// 模擬不穩定的操作（用計數器而非亂數，讓輸出完全可重現）
// 第二個參數 timeout 在此示範中未使用，故不命名以避免 -Wunused-parameter
int unstable_counter = 0;
std::string fetch_data(const std::string& url, int /*timeout*/) {
    ++unstable_counter;
    if (unstable_counter < 3) {
        throw std::runtime_error("connection timeout");
    }
    return "data from " + url;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】示範「轉發版重試」為什麼會送出空資料
//   情境：上傳服務把 payload 交給 retry 包裝器。
//     broken_retry 內部誤用了 std::forward，第一次嘗試失敗後，
//     payload 已被目標函式搬空，第二次送出去的是空字串。
//     這正是「重試機制看起來有在跑、實際上送出空資料」的真實故障樣態。
// -----------------------------------------------------------------------------
int upload_attempts = 0;

// 目標函式：收到右值就把內容搬走（真實的實作幾乎都會這麼做，這才是重點）
std::string upload(std::string&& payload) {
    ++upload_attempts;
    std::string taken = std::move(payload);   // ★ 資源被搬走了
    if (upload_attempts < 2) {
        throw std::runtime_error("503 service unavailable");
    }
    return "uploaded " + std::to_string(taken.size()) + " bytes";
}

// ✗ 錯誤示範：會重複呼叫卻使用了 forward
template<typename Func, typename... Args>
auto broken_retry(int max_attempts, Func&& func, Args&&... args)
    -> decltype(func(std::forward<Args>(args)...))
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            std::cout << "    嘗試第 " << attempt << " 次...\n";
            return func(std::forward<Args>(args)...);   // ✗ 第二次拿到空殼
        } catch (const std::exception& e) {
            std::cout << "    失敗: " << e.what() << "\n";
            if (attempt == max_attempts) throw;
        }
    }
    throw std::runtime_error("unreachable");
}

int main() {
    // 日誌包裝器
    auto result = logged_call("process",
        [](int a, int b) { return a + b; },
        10, 20);
    std::cout << "結果: " << result << "\n\n";

    // 重試包裝器（正確版：不轉發）
    try {
        auto data = retry(5, fetch_data, std::string("https://api.example.com"), 30);
        std::cout << "成功: " << data << "\n";
    } catch (...) {
        std::cout << "最終失敗\n";
    }

    // ============================================================
    // 日常實務：轉發版重試為什麼會送出空資料
    // ============================================================
    std::cout << "\n=== 日常實務：誤用 forward 的重試包裝器 ===\n";
    {
        std::string payload(120, 'x');   // 120 bytes 的內容
        std::cout << "  原始 payload 長度 = " << payload.size() << " bytes\n";
        try {
            // 注意：傳的是 std::move(payload)，第一次呼叫就會被 upload 搬走
            auto r = broken_retry(3, upload, std::move(payload));
            std::cout << "  結果: " << r << "\n";
            std::cout << "  ↑ 預期應上傳 120 bytes，實際卻是 0 bytes\n";
            std::cout << "     因為第一次嘗試已把 payload 搬空，第二次送出空字串\n";
        } catch (const std::exception& e) {
            std::cout << "  最終失敗: " << e.what() << "\n";
        }
        std::cout << "  結論：會重複呼叫的包裝器，引數必須以左值重複傳遞\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術5.cpp" -o retry_wrapper

// 註：本檔未附 LeetCode 範例。重試／日誌包裝器屬於服務端工程議題，
//     LeetCode 題目沒有對應情境；硬套一題只會失真。
//
// 註：被 std::move 之後的 std::string 處於 valid but unspecified 狀態。
//     下方輸出顯示第二次上傳為 0 bytes，是 libstdc++ 把來源置空的實作行為；
//     標準並未保證被移動後的字串一定是空的——但「不可再依賴其內容」
//     這件事是標準保證的，這正是本示範要傳達的重點。

// === 預期輸出 ===
// [LOG] process 開始
// [LOG] process 結束
// 結果: 30
//
//   嘗試第 1 次...
//   失敗: connection timeout
//   嘗試第 2 次...
//   失敗: connection timeout
//   嘗試第 3 次...
// 成功: data from https://api.example.com
//
// === 日常實務：誤用 forward 的重試包裝器 ===
//   原始 payload 長度 = 120 bytes
//     嘗試第 1 次...
//     失敗: 503 service unavailable
//     嘗試第 2 次...
//   結果: uploaded 0 bytes
//   ↑ 預期應上傳 120 bytes，實際卻是 0 bytes
//      因為第一次嘗試已把 payload 搬空，第二次送出空字串
//   結論：會重複呼叫的包裝器，引數必須以左值重複傳遞
