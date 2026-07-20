// =============================================================================
//  第 18 課：對象的生命週期 6  —  Construct On First Use：初始化順序的標準解法
// =============================================================================
//
// 【主題資訊 Information】
//   模式名稱：Construct On First Use（首次使用時建構），
//             搭配靜態區域變數時亦稱 Meyers Singleton。
//   語法：   T& getT() { static T instance(...); return instance; }
//   解決什麼：5.cpp 的 static initialization order fiasco
//             （跨翻譯單元的全域物件初始化順序未指定）。
//   標準版本：靜態區域變數 C++98 起；其初始化的執行緒安全性由 C++11 保證。
//   標頭檔：<iostream>、<string>
//   複雜度：第一次呼叫 O(建構成本)，之後每次只多一次旗標檢查。
//
// 【詳細解釋 Explanation】
//
// 【1. 核心想法：把「順序」從連結期搬到執行期】
//   5.cpp 的問題根源是：初始化時機由連結器決定，而你控制不了連結器。
//   這個模式的解法非常直接——不要讓它在啟動時初始化，
//   改成「第一次有人要用它的時候」才初始化。
//   於是初始化順序 = 函式被呼叫的順序 = 你自己寫的程式邏輯。
//   從「不可控的建置細節」變成「可控的程式碼」，問題就消失了。
//
// 【2. 相依關係如何被自動滿足】
//   看本檔的關鍵兩行：
//       Config& getConfig() { static Config config; return config; }
//       ConnectionPool& getPool() {
//           static ConnectionPool pool(getConfig().getMax());   // ← 重點
//           return pool;
//       }
//   建構 pool 之前必須先求值 getConfig()，而那個呼叫本身就會確保
//   config 已完成建構。相依關係直接寫在程式碼裡，由語言強制執行，
//   不需要任何額外的註冊機制或初始化順序表。
//   ★ 這就是為什麼這個模式優於「手動 initAll() 函式」——
//     後者要靠人記得把順序寫對，前者由編譯器保證。
//
// 【3. 為什麼要回傳「參考」而不是值或指標？】
//   ● 回傳值：每次呼叫都複製一份，就不是同一個物件了，失去單例語意。
//   ● 回傳指標：呼叫端要檢查 nullptr，而這個指標永遠不可能是 null，
//               白白增加雜訊；也讓呼叫端誤以為可以 delete 它。
//   ● 回傳參考：語意最準確——「這裡有一個一定存在的物件，你只能用它」。
//   若想禁止呼叫端修改，就回傳 const T&（如 4.cpp 的 config()）。
//
// 【4. 這個模式的代價（要誠實面對）】
//   它解決了「初始化順序」，但沒有解決「這是全域狀態」：
//     ● 相依關係隱形：函式簽名上看不出它用了 getConfig()。
//     ● 難以測試：無法在單元測試中替換成假設定。
//     ● 解構順序問題仍在：靜態物件依建構反序解構，
//       若 A 解構時要用 B 而 B 先解構，一樣是使用已銷毀物件。
//       這是本模式「沒有」解決的部分，常見對策是刻意讓單例永不解構
//       （用 new 且不 delete，交給 OS 回收）。
//   所以實務準則是：優先用依賴注入（把 Config& 當參數傳），
//   單例只保留給真正全程式唯一的資源。
//
// 【概念補充 Concept Deep Dive】
//   ● 兩種變體的差別：
//       static T instance;          // 物件在靜態儲存區，程式結束時會解構
//       static T* p = new T();      // 物件在堆積，永不解構（刻意洩漏）
//     後者稱為 "leaky singleton"，專門用來迴避解構順序問題：
//     既然永遠不解構，就不可能有「解構太早」的問題。
//     對整個行程存活的資源（log、記憶體池）這是可接受的取捨，
//     且行程結束時 OS 會回收全部記憶體，不是真正的洩漏。
//   ● C++11 之後這個模式免費得到執行緒安全的初始化（magic statics），
//     不需要 double-checked locking——那個老技巧在 C++11 之前
//     其實是有 bug 的（缺少記憶體屏障）。
//   ● 本檔的輸出順序值得注意：因為採用延遲初始化，
//     兩個 [初始化] 訊息會出現在 main() 開始之後，
//     而不是像 5.cpp 那樣出現在 main() 之前。
//
// 【注意事項 Pay Attention】
//   1. 本模式解決「初始化順序」，不解決「解構順序」。
//      跨單例的解構相依仍需靠 leaky singleton 或明確的關閉流程處理。
//   2. 回傳參考，不要回傳指標或值。
//   3. 初始化執行緒安全由 C++11 保證，但初始化「之後」的並行存取
//      仍需自行同步。
//   4. 不要因為方便就把所有全域狀態都改成單例——它仍然是全域狀態，
//      會讓相依關係隱形、單元測試困難。
//   5. 若建構函式在初始化過程中又遞迴呼叫同一個 getter，是未定義行為
//      （gcc 實作為死鎖）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Construct On First Use
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 這個模式為什麼能解決初始化順序問題？
//     答：因為它把初始化時機從「程式啟動時、順序由連結器決定」
//         改成「第一次呼叫該函式時」。而呼叫順序是程式邏輯，完全可控。
//         更進一步，把相依寫成 static Pool pool(getConfig().getMax());
//         之後，建構 pool 前必然先完成 config 的建構——
//         相依關係由語言強制執行，不必靠人維護順序表。
//     追問：為什麼要回傳參考而不是指標？
//         → 這個物件一定存在，回傳指標會逼呼叫端做無意義的 null 檢查，
//           還可能讓人誤以為該 delete 它。參考精準表達「必定存在、不可轉移擁有權」。
//
// 🔥 Q2. 有些程式碼寫 static T* p = new T(); 且從不 delete，這是記憶體洩漏嗎？
//     答：技術上是刻意不回收，但目的明確——迴避「解構順序問題」。
//         既然永不解構，就不可能發生「A 解構時 B 已經死了」。
//         對存活到行程結束的資源（log 系統、記憶體池）這是常見且可接受的
//         取捨，行程結束時 OS 會回收全部記憶體。
//     追問：那什麼情況下不能這樣做？
//         → 當解構函式有「外部可見的副作用」時，例如必須把緩衝區
//           flush 到磁碟、關閉網路連線、釋放 OS 具名資源（鎖、共享記憶體）。
//           這些不能交給 OS 隨手回收，必須明確關閉。
//
// ⚠️ 陷阱. 用了 Meyers Singleton，全域狀態的問題就解決了嗎？
//     答：沒有。它只解決了「初始化順序」這一個技術問題。
//         全域狀態本身的三個壞處都還在：相依關係隱形（看簽名看不出來）、
//         單元測試無法替換假物件、解構順序問題依然存在。
//     為什麼會錯：把「消除了一個 bug」誤讀成「這個設計現在是好的」。
//         這個模式是「當你已經決定要用全域物件時，正確的實作方式」，
//         而不是「應該多用全域物件的理由」。
//         真正的改善方向是依賴注入——把 Config& 當參數傳進需要的地方，
//         相依關係就寫在型別簽名上，測試時也能輕鬆替換。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Config {
private:
    int maxConnections;
public:
    Config() : maxConnections(100) {
        cout << "  [Config] 初始化" << endl;
    }
    int getMax() const { return maxConnections; }
};

class ConnectionPool {
private:
    int poolSize;
public:
    ConnectionPool(int size) : poolSize(size) {
        cout << "  [ConnectionPool] 初始化: " << poolSize << endl;
    }
    int getMax() const { return poolSize; }
};

// 用函數包裝，保證初始化順序
Config& getConfig() {
    static Config config;    // 第一次調用時建構
    return config;
}

ConnectionPool& getPool() {
    // getConfig() 保證在使用前就已初始化
    static ConnectionPool pool(getConfig().getMax());
    return pool;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依賴注入 vs 單例：同一份邏輯的兩種寫法
//   情境：一個「建立工作階段」的函式需要讀取設定。
//   ● 單例版：函式簽名看不出它依賴 Config，測試時無法替換設定。
//   ● 注入版：Config 是參數，相依關係寫在型別上，測試可傳入任意假設定。
//   下面用一個「模擬測試」證明差異：注入版可以用不同設定跑出不同結果，
//   單例版則永遠綁在同一份全域設定上。
// -----------------------------------------------------------------------------
struct AppSettings {
    string env;
    int    maxRetry;
};

// 單例版：相依隱形
AppSettings& globalSettings() {
    static AppSettings s{"production", 3};
    return s;
}

string createSessionSingleton() {
    // 從簽名完全看不出這個函式讀了全域設定
    const AppSettings& s = globalSettings();
    return "session[env=" + s.env + ", retry=" + to_string(s.maxRetry) + "]";
}

// 注入版：相依寫在簽名上
string createSessionInjected(const AppSettings& s) {
    return "session[env=" + s.env + ", retry=" + to_string(s.maxRetry) + "]";
}

// 註：本檔不加 LeetCode 範例。
//     本主題是「跨翻譯單元的初始化順序」與全域狀態的設計取捨，
//     LeetCode 的單檔評測環境不存在這個情境；硬掛一題會讓讀者
//     誤以為單例是解題常規手段，故從缺。

int main() {
    cout << "=== 安全的初始化順序 ===" << endl;
    cout << "  Pool size = " << getPool().getMax() << endl;  
    // 假設 ConnectionPool 也有 getMax() 的話
    
    getPool();   // 第二次調用，不會重複初始化

    cout << "\n=== 觀察：初始化發生在 main() 之後（延遲）===" << endl;
    cout << "  對照 5.cpp：那裡的初始化訊息出現在 main() 之前" << endl;
    cout << "  這裡則是「第一次呼叫 getPool() 時」才建構" << endl;

    cout << "\n=== 日常實務：單例 vs 依賴注入 ===" << endl;
    {
        cout << "  單例版（相依隱形，測試時無法替換）：" << endl;
        cout << "    " << createSessionSingleton() << endl;
        cout << "    再呼叫一次仍是同一份全域設定：" << endl;
        cout << "    " << createSessionSingleton() << endl;

        cout << "  注入版（相依寫在簽名上，可自由替換）：" << endl;
        AppSettings prod{"production", 3};
        AppSettings test{"unit-test", 0};
        vector<AppSettings> cases = {prod, test};
        for (const AppSettings& s : cases) {
            cout << "    " << createSessionInjected(s) << endl;
        }
        cout << "  → 注入版能用不同設定驗證行為，單例版做不到" << endl;
    }

    return 0;
}
// 這段程式碼展示了如何使用函數包裝全域對象來確保初始化順序：
// - Config 和 ConnectionPool 都是全域對象，但它們被包裝在函數中，使用 static 局部變數來確保它們在第一次調用時才被初始化。
// - getConfig() 會在第一次調用時建構 Config 對象，然後 getPool() 會在第一次調用時建構 ConnectionPool 對象，
// 並且依賴於已經初始化的 Config 對象，這樣就避免了初始化順序的問題。

// 編譯: g++ -std=c++17 -Wall -Wextra "第 18 課：對象的生命週期（Object Lifetime）6.cpp" -o life6

// 【輸出說明】第一行 "Pool size = " 之後會先插入兩行初始化訊息，才印出 100。
//   這不是錯誤，而是 C++17 明定的求值順序：對 a << b，
//   左運算元先於右運算元求值。所以 "  Pool size = " 先被輸出，
//   接著求值 getPool()（觸發兩個物件的建構並印出訊息），最後才印 100。
//   在 C++14 以前這個順序是未指定的，同樣的程式碼可能有不同的輸出排列。

// === 預期輸出 ===
// === 安全的初始化順序 ===
//   Pool size =   [Config] 初始化
//   [ConnectionPool] 初始化: 100
// 100
//
// === 觀察：初始化發生在 main() 之後（延遲）===
//   對照 5.cpp：那裡的初始化訊息出現在 main() 之前
//   這裡則是「第一次呼叫 getPool() 時」才建構
//
// === 日常實務：單例 vs 依賴注入 ===
//   單例版（相依隱形，測試時無法替換）：
//     session[env=production, retry=3]
//     再呼叫一次仍是同一份全域設定：
//     session[env=production, retry=3]
//   注入版（相依寫在簽名上，可自由替換）：
//     session[env=production, retry=3]
//     session[env=unit-test, retry=0]
//   → 注入版能用不同設定驗證行為，單例版做不到
