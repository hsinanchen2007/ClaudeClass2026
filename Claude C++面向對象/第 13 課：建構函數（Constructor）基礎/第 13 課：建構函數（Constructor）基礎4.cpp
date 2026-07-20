// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 4  —  四種物件的建構時機
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : storage duration（儲存期）決定 constructor 何時執行
//   四種情況  : 全域／靜態、區域（自動）、區塊內、動態（new）
//   標準版本  : C++98 起；本檔用到的 magic static（函式內 static 執行緒安全）是 C++11
//   標頭檔    : <iostream>、<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 建構時機由「儲存期」決定，不是由「寫在哪一行」決定】
//   ┌────────────┬──────────────┬───────────────────────────────┐
//   │ 物件種類    │ 儲存期        │ constructor 執行時機           │
//   ├────────────┼──────────────┼───────────────────────────────┤
//   │ 全域／namespace│ static     │ main() 之前（動態初始化階段）   │
//   │ 區域變數     │ automatic    │ 執行到宣告那一行時              │
//   │ 區塊內變數   │ automatic    │ 進入區塊並執行到宣告時          │
//   │ new 出來的   │ dynamic      │ new 運算子執行時                │
//   │ 函式內 static│ static       │ **第一次**執行到那一行時         │
//   └────────────┴──────────────┴───────────────────────────────┘
//   注意最後一列：函式內的 static 物件不是在 main() 前建構，
//   而是「第一次執行到」才建構，而且只建構一次。這個差別是 Singleton 的關鍵。
//
// 【2. destructor 的順序是建構順序的完全相反】
//   自動物件離開 scope 時依「後建構先解構」的堆疊順序銷毀；
//   全域物件則在 main() 結束後、依「與建構相反的順序」銷毀。
//   本檔的輸出會清楚呈現這個 LIFO 特性——這正是 RAII 能正確運作的基礎：
//   內層取得的資源一定先於外層釋放。
//
// 【3. static initialization order fiasco（靜態初始化順序問題）】
//   這是全域物件真正的危險所在。標準保證：
//     * **同一個** translation unit（同一個 .cpp）內的全域物件，
//       依定義出現的順序建構；
//     * **不同** translation unit 之間的順序，**完全未定義**。
//   所以下面這種寫法是定時炸彈：
//       // a.cpp
//       Logger  gLogger("app");
//       // b.cpp
//       Service gService;          // 它的 constructor 用到了 gLogger
//   若連結器決定先建構 gService，它就會用到一個還沒建構的 gLogger → UB。
//   而且這種 bug 會隨著「改變檔案連結順序」而出現或消失，極難追查。
//
//   標準解法是 Construct On First Use（本檔示範）：
//       Logger& logger() { static Logger inst("app"); return inst; }
//   把全域物件換成「函式內的 static」，它保證在第一次被使用時才建構，
//   順序問題自然消失。C++11 起還保證這個初始化是執行緒安全的（magic static）。
//
// 【4. new / delete 與自動物件的差別】
//   new Box() 做兩件事：配置記憶體 + 呼叫 constructor。
//   delete p 也做兩件事：呼叫 destructor + 釋放記憶體。
//   兩者必須配對；忘了 delete 就是記憶體洩漏，destructor 永遠不會執行。
//   現代 C++ 的答案是不要自己寫 new/delete，改用 std::unique_ptr / make_unique，
//   讓「自動物件的 scope 規則」去管理堆積上的物件——這就是 RAII。
//
// 【概念補充 Concept Deep Dive】
//   ▍全域物件的兩階段初始化
//     static storage duration 的物件先經歷 static initialization
//     （zero-initialization 或 constant-initialization，發生在程式載入時，
//      不需要執行任何程式碼），再經歷 dynamic initialization
//     （執行 constructor）。所以即使 dynamic initialization 還沒輪到，
//     那塊記憶體也已經是全 0，不是垃圾——這是全域物件與區域物件的重大差異。
//
//   ▍為什麼全域物件的 constructor 輸出會出現在「main 開始」之前
//     因為它真的比 main() 早執行。C runtime 在呼叫 main 之前，
//     會先跑完一份「需要動態初始化的全域物件」清單。
//
//   ▍magic static 的成本
//     C++11 保證函式內 static 的初始化是執行緒安全的，實作上通常是
//     一個 guard variable + atomic 檢查。已初始化之後的檢查成本極低
//     （通常只是一次 load + 分支預測命中），不必為此避開這個慣用法。
//
// 【注意事項 Pay Attention】
//   1. 跨 translation unit 的全域物件初始化順序未定義——不要讓它們互相依賴。
//   2. 全域物件的 destructor 在 main() 之後執行，此時 cout 仍可用，
//      但若它依賴其他已被銷毀的全域物件，同樣是 UB（destruction order fiasco）。
//   3. new 之後忘記 delete：記憶體洩漏 + destructor 不執行（副作用也不會發生）。
//   4. 本檔為教學而在 constructor / destructor 內做 I/O，正式程式碼應避免。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】物件生命週期與建構時機
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 全域物件、區域物件、函式內 static 物件，分別在什麼時候建構？
//     答：全域物件在 main() 之前的動態初始化階段；區域物件在執行到宣告那一行；
//         函式內 static 物件在「第一次」執行到那一行時建構，且只建構一次。
//         解構順序則是：自動物件離開 scope 時（LIFO），
//         static 物件在 main() 結束後依建構的相反順序銷毀。
//     追問：函式內 static 的初始化是執行緒安全的嗎？→ C++11 起保證是
//         （magic static）；C++98 沒有這個保證，舊程式常見用 double-checked
//         locking 手動處理。
//
// 🔥 Q2. 什麼是 static initialization order fiasco？怎麼解決？
//     答：不同 translation unit 之間的全域物件初始化順序未定義。若 A.cpp 的
//         全域物件在 constructor 中用到 B.cpp 的全域物件，可能用到尚未建構的東西。
//         解法是 Construct On First Use：把全域物件改成函式內的 static，
//         由「第一次呼叫」決定建構時機，順序依賴自然消失。
//     追問：那解構順序有沒有對應的問題？→ 有，稱為 destruction order fiasco。
//         若必須避免，可用「刻意不解構」的 heap 物件（Meyers 提過的做法），
//         代價是報告成洩漏，需權衡。
//
// ⚠️ 陷阱. 「new 出來的物件離開 scope 就會被銷毀」
//     答：不會。離開 scope 的只是那個「指標變數」，它指向的堆積物件仍然存在，
//         destructor 不會執行。這就是記憶體洩漏——而且若 destructor 裡有
//         關檔案、解鎖、寫入 flush 等副作用，那些副作用也一併消失了。
//     為什麼會錯：把「指標的生命週期」和「被指向物件的生命週期」混為一談。
//         正解是用 std::unique_ptr 讓兩者重新綁在一起。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
using namespace std;

class Box {
private:
    string label;

public:
    Box(const string& l = "空盒子") : label(l) {
        cout << "  [建構] " << label << endl;
    }
    ~Box() {
        cout << "  [解構] " << label << endl;
    }
    string getLabel() const { return label; }
};

// ====== 全域物件：main() 之前就建構 ======
Box globalBox("全域盒子");

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔講的是 storage duration 與 constructor／destructor 的執行時機，
//         這是 C++ 特有的語言機制。LeetCode 在單一函式呼叫內評測，
//         既觀察不到全域初始化順序，也不會考解構順序，沒有題目對應得上。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】用 Construct On First Use 避開靜態初始化順序問題
//   情境：設定管理員（ConfigManager）幾乎會被所有模組用到，很自然會寫成全域物件。
//         但只要有另一個 .cpp 的全域物件在自己的 constructor 裡讀設定，
//         就踩進「跨 translation unit 初始化順序未定義」的地雷。
//   作法：改成函式內 static，第一次呼叫才建構；C++11 起還保證執行緒安全。
//         這也是 Singleton 最推薦的現代寫法（Meyers Singleton）。
// -----------------------------------------------------------------------------
class ConfigManager {
private:
    string env_;
    int    poolSize_;

    ConfigManager() : env_("production"), poolSize_(16) {
        cout << "  [ConfigManager 建構] 只會發生一次" << endl;
    }

public:
    // 禁止複製，避免出現「第二份設定」
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& instance() {
        static ConfigManager inst;   // magic static：第一次呼叫才建構，且執行緒安全
        return inst;
    }

    const string& env()      const { return env_; }
    int           poolSize() const { return poolSize_; }
};

int main() {
    cout << "=== main() 開始（全域物件已經建構完了）===" << endl;

    cout << "\n--- 區域物件 ---" << endl;
    Box localBox("區域盒子");        // 執行到這一行時建構

    cout << "\n--- 進入區塊 ---" << endl;
    {
        Box blockBox("區塊盒子");     // 進入區塊、執行到宣告時建構
        cout << "  區塊內..." << endl;
    }   // blockBox 在這裡離開 scope → destructor 立刻執行
    cout << "--- 離開區塊（上面那行解構就是證據）---" << endl;

    cout << "\n--- 動態物件（手動 new/delete）---" << endl;
    Box* heapBox = new Box("堆積盒子");   // new 時建構
    delete heapBox;                       // delete 時解構；忘了寫就洩漏

    cout << "\n--- 動態物件（unique_ptr，推薦）---" << endl;
    {
        auto smart = std::make_unique<Box>("智慧指標盒子");
        cout << "  持有中: " << smart->getLabel() << endl;
    }   // 離開 scope 自動 delete，不可能忘記
    cout << "--- unique_ptr 已自動釋放 ---" << endl;

    cout << "\n--- 函式內 static：第一次呼叫才建構 ---" << endl;
    cout << "  （注意下一行才會看到建構訊息）" << endl;
    ConfigManager& cfg = ConfigManager::instance();   // 此刻才真正建構
    cout << "  env = " << cfg.env() << endl;
    cout << "  第二次呼叫不會再建構：" << endl;
    cout << "  poolSize = " << ConfigManager::instance().poolSize() << endl;

    cout << "\n--- LIFO 示範：後建構的先解構 ---" << endl;
    {
        Box first("先建構");
        Box second("後建構");
        cout << "  即將離開 scope，觀察解構順序" << endl;
    }

    cout << "\n=== main() 即將結束（全域物件會在這之後解構）===" << endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎4.cpp" -o ctor4


// === 預期輸出 ===
//   [建構] 全域盒子
// === main() 開始（全域物件已經建構完了）===
//
// --- 區域物件 ---
//   [建構] 區域盒子
//
// --- 進入區塊 ---
//   [建構] 區塊盒子
//   區塊內...
//   [解構] 區塊盒子
// --- 離開區塊（上面那行解構就是證據）---
//
// --- 動態物件（手動 new/delete）---
//   [建構] 堆積盒子
//   [解構] 堆積盒子
//
// --- 動態物件（unique_ptr，推薦）---
//   [建構] 智慧指標盒子
//   持有中: 智慧指標盒子
//   [解構] 智慧指標盒子
// --- unique_ptr 已自動釋放 ---
//
// --- 函式內 static：第一次呼叫才建構 ---
//   （注意下一行才會看到建構訊息）
//   [ConfigManager 建構] 只會發生一次
//   env = production
//   第二次呼叫不會再建構：
//   poolSize = 16
//
// --- LIFO 示範：後建構的先解構 ---
//   [建構] 先建構
//   [建構] 後建構
//   即將離開 scope，觀察解構順序
//   [解構] 後建構
//   [解構] 先建構
//
// === main() 即將結束（全域物件會在這之後解構）===
//   [解構] 區域盒子
//   [解構] 全域盒子
