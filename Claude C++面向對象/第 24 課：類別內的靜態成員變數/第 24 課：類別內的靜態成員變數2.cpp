// =============================================================================
//  第 24 課：類別內的靜態成員變數 2  —  inline static 與類別內初始化
// =============================================================================
//
// 【主題資訊 Information】
//   四種寫法與所需標準:
//     static int x;                        // C++98:必須在類別外定義一次
//     static const int N = 100;            // C++98:整數/列舉型別可類別內初始化
//     inline static int x = 4;             // C++17:類別內定義 + 初始化 ✅
//     inline static const string S = "..." ;// C++17:非整數型別也可以了
//     static constexpr double PI = 3.14;   // C++17 起隱含 inline
//   標準版本：本檔核心特性 inline static 是 **C++17**;以 -std=c++17 編譯。
//   複雜度：存取靜態成員是直接定址,O(1),不需經過任何物件。
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. C++17 之前的痛點:宣告與定義必須分開】
//   在 C++17 之前,類別內寫的 static 成員只是「宣告」,你還必須在某個
//   .cpp 檔中補一行「定義」:
//       class GameConfig { static int maxPlayers; };   // 宣告
//       int GameConfig::maxPlayers = 4;                // 定義(要放在 .cpp)
//   忘了寫定義 → 連結錯誤 undefined reference;
//   寫在標頭檔又被多個 .cpp 引入 → 連結錯誤 multiple definition。
//   這是 C++ 初學者最常撞、也最難自行看懂錯誤訊息的坑之一。
//
// 【2. inline static 怎麼解決它(C++17)】
//   inline 的意義不是「內聯展開」,而是「允許在多個編譯單元中重複定義,
//   且連結器保證只留一份」。把它套用到靜態資料成員之後,
//   就能直接在類別內完成定義與初始化,標頭檔可以被任意多個 .cpp 引入,
//   不需要也不可以再寫類別外定義。純標頭檔函式庫因此變得容易許多。
//
// 【3. 為什麼 static const int 早就能類別內初始化,string 卻不行】
//   C++98 就允許 static const 的「整數或列舉型別」在類別內給初值,
//   因為那個值可以直接當成編譯期常數內嵌到使用處(例如當陣列大小),
//   通常不需要真的配置一塊記憶體。
//   而 std::string 需要執行期建構、需要實體儲存,所以在 C++17
//   有了 inline 之前,只能寫在類別外。本檔的 GAME_NAME 正是這個差異的例子。
//
// 【4. 靜態成員屬於「類別」,不屬於任何物件】
//   maxPlayers 只有一份,全程式共用;它不佔用任何 GameConfig 物件的空間
//   (這點會在第 6 號檔案用 sizeof 實測)。
//   因此存取方式是 GameConfig::maxPlayers,而不需要先建立物件。
//
// 【概念補充 Concept Deep Dive】
//   * 靜態成員具有靜態儲存期(static storage duration):在 main 之前完成
//     初始化(常數初始化或動態初始化),在 main 結束後才解構。
//   * static const int 若只被「當成值使用」(如當陣列大小),多數實作
//     不會為它配置儲存空間;但只要對它取位址或綁到 const 參考,
//     就需要一個真實位址 —— 在 C++17 之前這正是必須補類別外定義的時機,
//     C++17 起因隱含 inline 而不再需要。
//   * static constexpr 成員自 C++17 起隱含就是 inline,所以
//     constexpr static double PI = 3.14; 也不需要類別外定義。
//   * 跨編譯單元的靜態物件「初始化順序」是未指定的(static initialization
//     order fiasco);要避免相依,用函式內的 static 區域變數(Meyers singleton)。
//
// 【注意事項 Pay Attention】
//   1. inline static 需要 C++17。用 -std=c++11/14 編譯本檔會失敗 ——
//      這點務必用 -pedantic-errors 實際編譯驗證,不要憑印象。
//   2. 有了 inline static 就「不可以」再寫類別外定義,否則是重複定義。
//   3. 非 const 的靜態成員是全域可變狀態:任何人都能改,且改的是同一份。
//      多執行緒下需要自行同步,否則是資料競爭(未定義行為)。
//   4. const 靜態成員不可被賦值(本檔 MAX_LEVEL = 200 會編譯失敗)。
//   5. 跨編譯單元的初始化順序未指定,不要讓一個靜態物件的初始化
//      依賴另一個編譯單元的靜態物件。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】inline static 與靜態成員初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++17 的 inline static 解決了什麼問題?這裡的 inline 是什麼意思?
//     答：解決「靜態成員必須在類別外再定義一次」的麻煩 ——
//         忘了寫會 undefined reference,寫在標頭檔被多次引入又會
//         multiple definition。這裡的 inline 不是「內聯展開」的意思,
//         而是「允許多個編譯單元各有一份定義,連結器保證合併成一份」。
//         有了它,純標頭檔函式庫就能直接在類別內完成定義與初始化。
//     追問：那還需要寫類別外定義嗎?→ 不需要,而且不可以再寫,否則重複定義。
//
// 🔥 Q2. 為什麼 static const int 從 C++98 就能在類別內給初值,std::string 不行?
//     答：因為整數/列舉型別的常數值可以直接內嵌到使用處(例如當陣列大小),
//         多數情況不需要實際配置儲存空間;而 std::string 需要執行期建構
//         與實體儲存,所以在 C++17 有 inline 之前只能寫在類別外。
//     追問：那 static const int 什麼時候還是需要類別外定義?→ 當你對它
//         取位址或把它綁到 const 參考時,它就需要一個真實位址。
//         C++17 起因為隱含 inline,這個需求也消失了。
//
// ⚠️ 陷阱. 「inline static 是 C++11 的新功能吧?反正 auto、lambda 都是 C++11。」
//     答：不是,它是 **C++17**。C++11 帶來的是 auto、lambda、nullptr、
//         範圍 for 等等;類別內的非 const 靜態資料成員定義,一直要到
//         C++17 的 inline 變數(inline variable)才成立。
//         用 -std=c++11 編譯本檔會直接失敗。
//     為什麼會錯：把「現代 C++ 的新特性」全部歸到印象最深的 C++11。
//         驗證方法很簡單:用 -std=c++11 -pedantic-errors 實際編一次。
//         注意一定要加 -pedantic-errors —— 只用 -fsyntax-only 或不加它,
//         GCC 可能把某些特性當擴充放行,讓你誤以為舊標準也支援。
//
//     本機實測(GCC 15.2.0,對本檔實際編譯):
//         g++ -std=c++11 -pedantic-errors ... → 失敗
//         g++ -std=c++14 -pedantic-errors ... → 失敗
//         g++ -std=c++17 -pedantic-errors ... → 成功
//       c++11/14 的錯誤訊息正是:
//         error: inline variables are only available with '-std=c++17'
//                or '-std=gnu++17' [-Wc++17-extensions]
//       編譯器自己把答案講得很清楚:inline variable 是 C++17 的特性。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺,理由如下
//   靜態成員初始化屬於「編譯單元與連結」層面的語言機制,LeetCode 判題
//   只執行單一測試流程,不涉及多個 .cpp 的連結問題,無題目能檢驗此知識點。
//   (順帶一提:LeetCode 的判題程序常在同一個行程內連續跑多筆測資,
//    因此類別的 static 狀態會跨測資殘留,是實務上很常見的 WA 來源 ——
//    這點在本課 summary.cpp 有進一步說明。)
//
// =============================================================================

#include <iostream>
#include <string>
using namespace std;

class GameConfig {
public:
    // C++17 inline static：直接在類別內定義並初始化
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    inline static int maxPlayers = 4;
    inline static double gravity = 9.8;
    inline static string version = "1.0.3";

    // const static 整數型別：即使 C++11 也可以類別內初始化
    // 這些常量用於遊戲配置，不會改變
    static const int MAX_LEVEL = 100;
    static const int MAX_ITEMS = 999;

    // C++17 inline static const 也可以用於非整數型別
    // 這裡我們定義了一個遊戲名稱常量，直接在類別內初始化
    inline static const string GAME_NAME = "冒險世界";

    static void printConfig() {
        cout << "  遊戲：" << GAME_NAME << " v" << version << endl;
        cout << "  最大玩家數：" << maxPlayers << endl;
        cout << "  重力：" << gravity << endl;
        cout << "  最大等級：" << MAX_LEVEL << endl;
        cout << "  最大物品：" << MAX_ITEMS << endl;
    }
};

// 不需要類別外定義了！（C++17）

// -----------------------------------------------------------------------------
// 【日常實務範例】功能開關（feature flag）與建置資訊
//   情境：服務需要一份「全程式共用、隨時可查」的設定與建置資訊。
//         這正是靜態成員的典型用途 —— 它屬於「這個系統」，不屬於任何一個物件，
//         也不該為了讀一個開關而先 new 一個物件出來。
//   重點 1：BUILD_* 是編譯期就定死的常量 → static constexpr / inline static const。
//   重點 2：功能開關是執行期可調的全域狀態 → inline static（非 const），
//           但一律走帶驗證的靜態函式修改，不直接公開變數。
//   注意：非 const 的靜態成員是全域可變狀態，多執行緒下需自行同步。
// -----------------------------------------------------------------------------
class FeatureFlags {
private:
    // 私有靜態狀態：不讓外界直接寫
    inline static bool  enableNewCheckout_ = false;
    inline static bool  enableDarkMode_    = true;
    inline static int   rolloutPercent_    = 0;      // 灰度發布比例 0~100
    inline static int   changeCount_       = 0;      // 被調整過幾次（稽核用）

public:
    // 編譯期常量：整數型別 C++98 起即可類別內初始化
    static const int    CONFIG_VERSION = 3;
    // 非整數型別要類別內初始化，需要 C++17 的 inline
    inline static const string BUILD_ID   = "2026.07.19-1a2b3c";
    inline static const string ENVIRONMENT = "production";
    // constexpr 靜態成員自 C++17 起隱含 inline
    static constexpr double ROLLOUT_STEP = 12.5;

    static bool newCheckoutEnabled() { return enableNewCheckout_; }
    static bool darkModeEnabled()    { return enableDarkMode_; }
    static int  rolloutPercent()     { return rolloutPercent_; }
    static int  changeCount()        { return changeCount_; }

    // 帶驗證的修改入口（同第 21 課：不直接公開可寫變數）
    static bool setRollout(int percent) {
        if (percent < 0 || percent > 100) return false;
        rolloutPercent_ = percent;
        enableNewCheckout_ = (percent > 0);
        ++changeCount_;
        return true;
    }

    static void toggleDarkMode() {
        enableDarkMode_ = !enableDarkMode_;
        ++changeCount_;
    }

    static void dump() {
        cout << "    build=" << BUILD_ID
             << "  env=" << ENVIRONMENT
             << "  configVer=" << CONFIG_VERSION << endl;
        cout << "    newCheckout=" << (newCheckoutEnabled() ? "on" : "off")
             << "  darkMode=" << (darkModeEnabled() ? "on" : "off")
             << "  rollout=" << rolloutPercent() << "%" << endl;
    }
};

int main() {
    cout << "=== C++17 inline static ===" << endl;

    GameConfig::printConfig();

    // 可以修改非 const 的靜態成員
    cout << "\n--- 修改配置 ---" << endl;
    GameConfig::maxPlayers = 8;
    GameConfig::gravity = 15.0;
    GameConfig::version = "2.0.0";
    GameConfig::printConfig();

    // GameConfig::MAX_LEVEL = 200;  // ❌ 編譯錯誤！const

    // ─────────────────────────────────────────────────────────
    cout << "\n=== 日常實務：功能開關與建置資訊 ===" << endl;

    // 完全不需要建立任何物件，就能查詢整個系統的設定
    cout << "\n--- 啟動時的預設狀態 ---" << endl;
    FeatureFlags::dump();

    cout << "\n--- 灰度發布：先開 25% ---" << endl;
    cout << "    setRollout(25)  → " << (FeatureFlags::setRollout(25) ? "成功" : "失敗") << endl;
    FeatureFlags::dump();

    cout << "\n--- 非法值被擋下 ---" << endl;
    cout << "    setRollout(150) → " << (FeatureFlags::setRollout(150) ? "成功" : "失敗")
         << "（超出 0~100）" << endl;
    cout << "    setRollout(-5)  → " << (FeatureFlags::setRollout(-5) ? "成功" : "失敗")
         << "（超出 0~100）" << endl;

    cout << "\n--- 全量發布 + 切換主題 ---" << endl;
    FeatureFlags::setRollout(100);
    FeatureFlags::toggleDarkMode();
    FeatureFlags::dump();
    cout << "    累計調整次數：" << FeatureFlags::changeCount()
         << "（被擋下的兩次不計入）" << endl;
    cout << "    ROLLOUT_STEP = " << FeatureFlags::ROLLOUT_STEP
         << "（constexpr 靜態成員，C++17 起隱含 inline）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 24 課：類別內的靜態成員變數2.cpp" -o l24_2
// 執行: ./l24_2        (rc=0)

// === 預期輸出 ===
// === C++17 inline static ===
//   遊戲：冒險世界 v1.0.3
//   最大玩家數：4
//   重力：9.8
//   最大等級：100
//   最大物品：999
//
// --- 修改配置 ---
//   遊戲：冒險世界 v2.0.0
//   最大玩家數：8
//   重力：15
//   最大等級：100
//   最大物品：999
//
// === 日常實務：功能開關與建置資訊 ===
//
// --- 啟動時的預設狀態 ---
//     build=2026.07.19-1a2b3c  env=production  configVer=3
//     newCheckout=off  darkMode=on  rollout=0%
//
// --- 灰度發布：先開 25% ---
//     setRollout(25)  → 成功
//     build=2026.07.19-1a2b3c  env=production  configVer=3
//     newCheckout=on  darkMode=on  rollout=25%
//
// --- 非法值被擋下 ---
//     setRollout(150) → 失敗（超出 0~100）
//     setRollout(-5)  → 失敗（超出 0~100）
//
// --- 全量發布 + 切換主題 ---
//     build=2026.07.19-1a2b3c  env=production  configVer=3
//     newCheckout=on  darkMode=off  rollout=100%
//     累計調整次數：3（被擋下的兩次不計入）
//     ROLLOUT_STEP = 12.5（constexpr 靜態成員，C++17 起隱含 inline）
