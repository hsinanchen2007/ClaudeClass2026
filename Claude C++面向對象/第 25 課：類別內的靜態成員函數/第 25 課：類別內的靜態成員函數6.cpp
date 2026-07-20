// =============================================================================
//  第 25 課：類別內的靜態成員函數 6  —  靜態與非靜態的互相呼叫
// =============================================================================
//
// 【主題資訊 Information】
//   非靜態 → 靜態: 永遠可以（靜態函式不需要額外資訊）
//   靜態 → 非靜態: 不行，除非自己提供一個物件
//   靜態 → 靜態:   可以（本檔 printSystemInfo 呼叫 isEnabled）
//   標準版本: C++98；inline static 資料成員初始化為 C++17
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 方向性：一邊通、一邊不通】
//   非靜態成員函式擁有 this，所以它「什麼都有」——
//   要呼叫靜態函式時，只是不使用 this 而已，毫無障礙。
//   本檔 log() 呼叫 isEnabled()、遞增 logCount_，都是這個方向。
//   反過來，靜態函式沒有 this，要呼叫非靜態函式就缺了「對誰呼叫」，
//   所以 printSystemInfo() 碰不到 module_，也不能呼叫 log()。
//   一句話：資訊多的能呼叫資訊少的，反之不行。
//
// 【2. 這個設計在實務上的意義：全域開關 + 個體身分】
//   本檔的 Logger 把兩種狀態刻意分開：
//     * module_（非靜態）：這個 logger 是哪個模組的 —— 屬於個體。
//     * enabled_ / logCount_（靜態）：整個日誌系統的開關與總計數 —— 屬於全體。
//   於是 Logger::disable() 一呼叫，所有 logger 同時停止輸出，
//   不必逐一去關。輸出中「關閉日誌」之後兩條訊息都消失，就是證據。
//
// 【3. const 成員函式竟然改得動 logCount_】
//   log() 宣告成 const，卻在函式體裡寫 logCount_++ —— 這完全合法。
//   因為 const 修飾的是 *this，保護的是「這個物件的非靜態成員」。
//   靜態成員不屬於任何物件，自然不在保護範圍內。
//   這一點很反直覺，是本檔最值得記住的細節（見下方陷阱題）。
//
// 【概念補充 Concept Deep Dive】
//   * 靜態函式呼叫非靜態函式的正解是「把物件當參數傳進去」：
//       static void flushAll(Logger& l) { l.log("flush"); }   // 合法
//     缺的只是物件，不是權限。
//   * enabled_ 與 logCount_ 是共享的可變狀態，多執行緒下需自行同步。
//     logCount_++ 是非原子的讀-改-寫，同時記錄就是資料競爭（未定義行為）。
//     實務上的日誌系統一定會用 atomic 或 mutex。
//   * 本檔的開關是布林值。實務上通常做成「等級」（TRACE < DEBUG < INFO
//     < WARN < ERROR），只輸出高於門檻的訊息 —— 下方實務範例示範這個作法。
//   * 「先檢查是否啟用再組字串」很重要：
//     若寫成 log("x=" + expensive())，即使日誌關閉，
//     字串組裝與 expensive() 仍會先被求值。
//     這是日誌系統要用巨集或惰性求值的真正原因。
//
// 【注意事項 Pay Attention】
//   1. const 成員函式仍可修改靜態成員 —— const 保護的只有 *this。
//   2. 靜態函式要操作物件，必須把物件當參數明確傳入。
//   3. 共享的靜態狀態在多執行緒下需要 atomic 或鎖。
//   4. 呼叫端組好的字串會先被求值，關閉日誌並不能省下這筆成本。
//   5. 全域開關會影響所有實例，這是特性也是風險（測試時尤其要留意重置）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態與非靜態的互相呼叫
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 非靜態函式可以呼叫靜態函式嗎?反過來呢?
//     答：非靜態呼叫靜態永遠可以 —— 非靜態函式有 this，
//         而靜態函式根本不需要它，只是不用而已。
//         靜態呼叫非靜態則不行，因為缺少「要對哪個物件呼叫」的資訊；
//         除非把物件當參數傳進去（static void f(Logger& l) { l.log(...); }）。
//     追問：那靜態函式呼叫另一個靜態函式呢?
//         → 完全沒問題。本檔 printSystemInfo() 就呼叫了
//         isEnabled() 與 getLogCount()，兩者都不需要 this。
//
// 🔥 Q2. 為什麼把 enabled_ 設計成靜態成員，而 module_ 是非靜態?
//     答：判準是「這個值對每個物件都應該一樣嗎」。
//         日誌的總開關是整個系統的政策，所有 logger 都該遵守，
//         所以是靜態；module_ 是「這個 logger 屬於哪個模組」，
//         每個實例本來就不同，所以是非靜態。
//         好處是 Logger::disable() 一呼叫，全部 logger 同時停用。
//     追問：這樣設計有什麼風險?
//         → 它是全域可變狀態：任何程式碼都能關掉全域日誌，
//         而且在單元測試之間會殘留，可能造成「某個測試把日誌關了，
//         後面的測試就收不到輸出」這種難以追查的問題。
//
// ⚠️ 陷阱. 「log() 宣告成 const，所以它不可能修改任何東西。」
//     答：它確實修改了 —— logCount_++ 就寫在 const 函式裡，而且完全合法。
//         const 成員函式修飾的是 *this，
//         保證的只有「不修改這個物件的非靜態成員」。
//         靜態成員不屬於任何物件，因此不在 const 的保護範圍內。
//     為什麼會錯：把 const 讀成「這個函式沒有副作用」。
//         實際上 const 只是一個很窄的承諾：不動 *this 的非靜態成員。
//         它管不到靜態成員、管不到全域變數、管不到透過指標成員
//         間接改到的資料（那叫邏輯常數性 vs 位元常數性），
//         也管不到 mutable 成員。看到 const 不等於看到「純函式」。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   「哪種函式能呼叫哪種函式」是語言的存取規則，LeetCode 判題不涉及。
//   本檔改以「分級日誌」的實務範例延伸同一個設計：
//   等級門檻是全域政策（靜態），模組名稱是個體屬性（非靜態）。
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    string module_;
    inline static int logCount_ = 0;
    inline static bool enabled_ = true;

public:
    Logger(const string& module) : module_(module) {}

    // ====== 靜態函數 ======
    // 靜態函數只能訪問靜態成員，不能訪問非靜態成員（因為它們不屬於任何實例）❌
    // 靜態函數可以修改靜態變數，這些變數對所有實例共享 ✅
    // 這些函數提供了對日誌系統的全局控制，例如啟用/停用日誌、獲取已記錄的日誌數量等
    // 靜態函數可以直接通過類名調用，不需要實例化對象 ✅
    // 靜態函數可以調用其他靜態函數 ✅
    static void enable() { enabled_ = true; }
    static void disable() { enabled_ = false; }
    static bool isEnabled() { return enabled_; }
    static int getLogCount() { return logCount_; }

    // 靜態函數可以調用其他靜態函數 ✅
    // 這個函數用於顯示當前日誌系統的狀態和已記錄的日誌數量
    // 注意：這裡不能訪問 module_，因為它是非靜態成員，與任何特定實例相關聯 ❌
    // 這個函數展示了靜態函數如何提供全局信息，而不依賴於任何特定的 Logger 實例
    // 這裡的輸出會顯示日誌系統是否啟用，以及已經記錄了多少條日誌，這些信息對所有 Logger 實例都是共享的
    static void printSystemInfo() {
        cout << "  [系統] 日誌 " << (isEnabled() ? "啟用" : "停用")
             << "，已記錄 " << getLogCount() << " 條" << endl;
    }

    // ====== 非靜態函數 ======
    // 非靜態函數可以訪問非靜態成員，因為它們屬於特定的實例 ✅
    // 非靜態函數也可以訪問靜態成員，因為靜態成員對所有實例共享 ✅
    // 非靜態函數提供了對特定 Logger 實例的操作，例如記錄消息、顯示警告等
    // 非靜態函數可以直接訪問同一類中的靜態函數和靜態變數，這些成員對所有實例都是共享的 ✅
    // 非靜態函數需要通過實例化對象來調用，因為它們依賴於特定的實例狀態 ✅
    // 非靜態函數可以調用靜態函數 ✅
    void log(const string& message) const {
        if (!isEnabled()) return;    // 調用靜態函數 ✅
        logCount_++;                 // 訪問靜態變數 ✅
        cout << "  [" << module_ << "] " << message   // 訪問非靜態成員 ✅
             << " (#" << logCount_ << ")" << endl;
    }

    void warn(const string& message) const {
        if (!isEnabled()) return;
        logCount_++;
        cout << "  ⚠ [" << module_ << "] " << message
             << " (#" << logCount_ << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】分級日誌（level-based logging）
//   情境：正式環境只想看 WARN 以上，開發時想看全部。
//         做法是把「門檻等級」放成靜態成員（全域政策），
//         模組名稱維持非靜態（個體屬性）—— 與上面的 Logger 同一個設計，
//         只是把布林開關升級成等級門檻，這才是實務上的常見形式。
//   額外重點：被過濾掉的訊息仍然「被計數」了嗎?
//         本範例刻意分開統計「送進來的」與「真正輸出的」，
//         用來說明過濾發生在哪一層。
// -----------------------------------------------------------------------------
class LeveledLogger {
public:
    enum Level { kTrace = 0, kDebug = 1, kInfo = 2, kWarn = 3, kError = 4 };

private:
    string module_;                              // 個體屬性

    inline static Level threshold_ = kInfo;      // 全域政策
    inline static long  submitted_ = 0;          // 送進來的訊息數
    inline static long  emitted_   = 0;          // 真正輸出的訊息數

    static const char* levelName(Level l) {
        switch (l) {
            case kTrace: return "TRACE";
            case kDebug: return "DEBUG";
            case kInfo:  return "INFO ";
            case kWarn:  return "WARN ";
            case kError: return "ERROR";
        }
        return "?????";
    }

public:
    explicit LeveledLogger(const string& module) : module_(module) {}

    // 全域政策的存取點：靜態函式，不需要任何 logger 物件
    static void setThreshold(Level l) { threshold_ = l; }
    static Level threshold()          { return threshold_; }
    static long  submitted()          { return submitted_; }
    static long  emitted()            { return emitted_; }
    static void  resetCounters()      { submitted_ = 0; emitted_ = 0; }

    // 非靜態：需要知道是哪個模組，所以要有 this
    // 注意這裡同樣是 const，卻仍然修改了靜態計數器 —— 完全合法
    void write(Level l, const string& message) const {
        ++submitted_;                      // 不論是否輸出都算「送進來」
        if (l < threshold_) return;        // 低於門檻就丟棄
        ++emitted_;
        cout << "  " << levelName(l) << " [" << module_ << "] " << message << endl;
    }
};

int main() {
    cout << "=== 靜態與非靜態的互動 ===" << endl;

    Logger gameLog("遊戲");
    Logger netLog("網路");

    Logger::printSystemInfo();

    cout << "\n--- 正常記錄 ---" << endl;
    gameLog.log("遊戲啟動");
    netLog.log("連接伺服器");
    gameLog.log("載入地圖");
    netLog.warn("延遲過高");

    Logger::printSystemInfo();

    // 關閉日誌
    cout << "\n--- 關閉日誌 ---" << endl;
    Logger::disable();
    gameLog.log("這條不會顯示");
    netLog.log("這條也不會顯示");
    Logger::printSystemInfo();

    // 重新開啟
    cout << "\n--- 重新開啟 ---" << endl;
    Logger::enable();
    gameLog.log("日誌恢復");
    Logger::printSystemInfo();

    cout << "\n  註：log() 宣告成 const，卻能執行 logCount_++ —— 這完全合法。" << endl;
    cout << "      const 修飾的是 *this，只保護「這個物件的非靜態成員」，" << endl;
    cout << "      靜態成員不屬於任何物件，因此不在保護範圍內。" << endl;

    cout << "\n=== 日常實務：分級日誌 ===" << endl;
    {
        LeveledLogger dbLog("db");
        LeveledLogger apiLog("api");

        auto emitAll = [&] {
            dbLog.write(LeveledLogger::kTrace, "連線池狀態輪詢");
            dbLog.write(LeveledLogger::kDebug, "SQL: SELECT 1");
            apiLog.write(LeveledLogger::kInfo,  "收到 GET /health");
            apiLog.write(LeveledLogger::kWarn,  "回應時間 1200ms");
            dbLog.write(LeveledLogger::kError, "連線逾時");
        };

        cout << "  --- 門檻 = INFO（正式環境的預設）---" << endl;
        LeveledLogger::resetCounters();
        emitAll();
        cout << "    送進來 " << LeveledLogger::submitted()
             << " 條，實際輸出 " << LeveledLogger::emitted() << " 條" << endl;

        cout << "  --- 門檻 = TRACE（開發時想看全部）---" << endl;
        LeveledLogger::setThreshold(LeveledLogger::kTrace);
        LeveledLogger::resetCounters();
        emitAll();
        cout << "    送進來 " << LeveledLogger::submitted()
             << " 條，實際輸出 " << LeveledLogger::emitted() << " 條" << endl;

        cout << "  --- 門檻 = ERROR（只看嚴重問題）---" << endl;
        LeveledLogger::setThreshold(LeveledLogger::kError);
        LeveledLogger::resetCounters();
        emitAll();
        cout << "    送進來 " << LeveledLogger::submitted()
             << " 條，實際輸出 " << LeveledLogger::emitted() << " 條" << endl;

        cout << "  ↑ 一行 setThreshold() 就改變了所有 logger 的行為 ——" << endl;
        cout << "    門檻是全域政策（靜態），模組名稱是個體屬性（非靜態）。" << endl;
        cout << "    另注意「送進來」永遠是 5：過濾發生在函式內部，" << endl;
        cout << "    呼叫端組好的字串仍然已經被求值了。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 25 課：類別內的靜態成員函數6.cpp -o static_func6

// === 預期輸出 ===
// === 靜態與非靜態的互動 ===
//   [系統] 日誌 啟用，已記錄 0 條
//
// --- 正常記錄 ---
//   [遊戲] 遊戲啟動 (#1)
//   [網路] 連接伺服器 (#2)
//   [遊戲] 載入地圖 (#3)
//   ⚠ [網路] 延遲過高 (#4)
//   [系統] 日誌 啟用，已記錄 4 條
//
// --- 關閉日誌 ---
//   [系統] 日誌 停用，已記錄 4 條
//
// --- 重新開啟 ---
//   [遊戲] 日誌恢復 (#5)
//   [系統] 日誌 啟用，已記錄 5 條
//
//   註：log() 宣告成 const，卻能執行 logCount_++ —— 這完全合法。
//       const 修飾的是 *this，只保護「這個物件的非靜態成員」，
//       靜態成員不屬於任何物件，因此不在保護範圍內。
//
// === 日常實務：分級日誌 ===
//   --- 門檻 = INFO（正式環境的預設）---
//   INFO  [api] 收到 GET /health
//   WARN  [api] 回應時間 1200ms
//   ERROR [db] 連線逾時
//     送進來 5 條，實際輸出 3 條
//   --- 門檻 = TRACE（開發時想看全部）---
//   TRACE [db] 連線池狀態輪詢
//   DEBUG [db] SQL: SELECT 1
//   INFO  [api] 收到 GET /health
//   WARN  [api] 回應時間 1200ms
//   ERROR [db] 連線逾時
//     送進來 5 條，實際輸出 5 條
//   --- 門檻 = ERROR（只看嚴重問題）---
//   ERROR [db] 連線逾時
//     送進來 5 條，實際輸出 1 條
//   ↑ 一行 setThreshold() 就改變了所有 logger 的行為 ——
//     門檻是全域政策（靜態），模組名稱是個體屬性（非靜態）。
//     另注意「送進來」永遠是 5：過濾發生在函式內部，
//     呼叫端組好的字串仍然已經被求值了。
