// =============================================================================
//  第 18 課：對象的生命週期 4  —  靜態局部對象與延遲初始化（Meyers Singleton）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：函式內的  static T obj(args);
//   語意：第一次「執行到」這一行時才建構；之後每次呼叫都跳過建構，
//         直接沿用同一個物件。程式結束時（main 返回後）才解構。
//   標準版本：靜態區域變數本身 C++98 就有；
//             ★「初始化具備執行緒安全」是 C++11 才明文保證的
//               （俗稱 magic statics / thread-safe statics）。
//             gcc 從 4.3 起實作，可用 -fno-threadsafe-statics 關閉。
//   標頭檔：<iostream>、<string>；本檔的執行緒驗證另用 <thread>、<atomic>
//   複雜度：第一次呼叫 O(建構成本)，之後每次呼叫只多一個旗標檢查。
//
// 【詳細解釋 Explanation】
//
// 【1. 靜態區域變數：作用域小，生命週期長】
//   這是理解本檔的關鍵——作用域與生命週期是兩件事：
//       作用域（誰看得到）  ：只有該函式內部
//       生命週期（活多久）  ：從第一次執行到宣告處，直到程式結束
//   這個組合非常有用：它給你一個「全域唯一、但外界無法直接存取」的物件，
//   存取路徑被強制收斂到那個函式。
//
// 【2. 為什麼叫「延遲初始化」（lazy initialization）？】
//   全域物件在 main() 之前就全部建構完畢，不管你用不用得到。
//   靜態區域變數則是「第一次執行到那一行」才建構。好處是：
//     ● 程式啟動更快（不必初始化用不到的東西）
//     ● 若這輩子都沒呼叫到該函式，就完全不會付出建構成本
//     ● 建構時機在「執行期」，可以依賴那時才知道的資訊
//   本檔輸出可以清楚看到：「程式啟動」訊息印完之後，
//   直到第一次呼叫 getResource() 才出現 [建構]。
//
// 【3. C++11 的執行緒安全保證（最重要的一段）】
//   標準 [stmt.dcl] 規定：若多個執行緒同時第一次執行到該宣告，
//   只有一個執行緒會執行初始化，其餘執行緒會「阻塞等待」直到完成。
//   編譯器產生的碼大致是：
//       if (!(guard & 1)) {                  // 快速路徑：只讀一個旗標
//           if (__cxa_guard_acquire(&guard)) {   // 慢路徑：搶鎖
//               construct();
//               __cxa_guard_release(&guard);
//           }
//       }
//   ★ 這代表兩件事：
//     (a) 建構「保證只發生一次」，不需要自己加鎖或用 double-checked locking。
//     (b) 進入穩定狀態後，每次呼叫只多一個旗標檢查（幾乎免費）。
//   本檔用 8 條執行緒同時搶著初始化，實際驗證建構次數確實是 1。
//   ★ 注意保證的範圍：只保證「初始化」是安全的。
//     初始化完成之後，多執行緒對該物件的並行讀寫仍然需要你自己同步。
//
// 【4. Meyers Singleton 及其優點】
//   把靜態區域變數包在函式裡回傳參考，就是知名的 Meyers Singleton：
//       Config& getConfig() { static Config c; return c; }
//   相對於「全域變數」，它一次解決了三個問題：
//     (a) 初始化順序未定義（static initialization order fiasco）——
//         見 5.cpp 與 6.cpp，這是本課的重點對照。
//     (b) 啟動成本——用不到就不建構。
//     (c) 執行緒安全——C++11 免費提供。
//
// 【概念補充 Concept Deep Dive】
//   ● 解構時機與順序：靜態區域物件在 main 返回後、由 atexit 機制
//     依「建構完成的反序」解構。所以先被初始化的後解構。
//     ★ 由此衍生一個真實陷阱——「解構順序問題（destruction order fiasco）」：
//       若 A 的解構函式用到 B，而 B 比 A 早解構，就是使用已銷毀物件。
//       常見解法是刻意「洩漏」單例（用 new 且永不 delete），
//       讓它活到行程結束，由 OS 回收記憶體。
//   ● 若建構函式拋出例外：該次初始化視為未完成，guard 會被重置，
//     下一次呼叫該函式時會「重新嘗試」建構。這是標準規定的行為。
//   ● 靜態區域變數存放在 .data 或 .bss 區段，不在堆疊上，
//     所以遞迴呼叫該函式也只有一份。
//   ● 遞迴陷阱：若初始化過程中又遞迴呼叫同一個函式，
//     會遇到「正在初始化中」的狀態，這是未定義行為（gcc 實作為死鎖）。
//
// 【注意事項 Pay Attention】
//   1. 執行緒安全「只涵蓋初始化」，不涵蓋之後對該物件的並行存取。
//   2. 解構順序是建構的反序；跨單例相依時可能踩到解構順序問題。
//   3. 單例會讓相依關係隱形、使單元測試難以替換假物件（mock），
//      不要因為方便就到處使用——它本質上是一個受管理的全域變數。
//   4. 建構函式若拋例外，下次呼叫會重新嘗試初始化（不是永久失敗）。
//   5. -fno-threadsafe-statics 會關掉這個保證，嵌入式專案偶爾為了
//      省下 guard 成本而使用，屆時必須自行確保單執行緒初始化。
//   6. 初始化中遞迴呼叫自己是未定義行為（gcc 實測為死鎖）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】靜態局部對象
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 函式內的 static 物件，何時建構？何時解構？
//     答：第一次「執行到」該宣告時才建構（延遲初始化），
//         之後每次呼叫都跳過；直到 main 返回後，才由 atexit 機制
//         依「建構完成的反序」解構。
//         它的作用域只有該函式，生命週期卻是整個程式——
//         作用域與生命週期是兩件不同的事。
//     追問：如果這個函式從來沒被呼叫過呢？
//         → 那該物件永遠不會被建構，也就不會被解構。
//           這正是延遲初始化最大的好處：用不到就不付成本。
//
// 🔥 Q2. 多執行緒同時第一次呼叫 getResource()，會建構幾次？
//     答：恰好一次。C++11 起標準保證靜態區域變數的初始化是執行緒安全的
//         （magic statics）：一個執行緒負責建構，其餘阻塞等待完成。
//         不需要自己加鎖，也不需要 double-checked locking。
//     追問：那初始化完成之後，多執行緒讀寫這個物件安全嗎？
//         → 不安全。保證只涵蓋「初始化」這個動作本身。
//           之後的並行存取仍要靠 mutex 或 atomic 自行同步。
//
// ⚠️ 陷阱. Meyers Singleton 既然執行緒安全又能解決初始化順序問題，
//          那是不是可以放心大量使用？
//     答：不建議。它解決的是「初始化」的問題，卻沒解決「它是全域狀態」
//         這個更根本的問題：相依關係變得隱形（看函式簽名看不出來用了它）、
//         單元測試無法替換成假物件、而且會踩到「解構順序問題」——
//         若 A 的解構函式用到 B，而 B 比 A 先解構，就是使用已銷毀物件。
//     為什麼會錯：把「執行緒安全」誤讀成「這個設計沒有問題了」。
//         執行緒安全只是把一個特定 bug 消掉，並沒有讓全域狀態變成好設計。
//         實務上常見的折衷是依賴注入（把 Config& 當參數傳進去），
//         單例只留在真正全程式唯一的資源（如 log 系統）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <chrono>
using namespace std;

class ExpensiveResource {
private:
    string name;
    
public:
    ExpensiveResource(const string& n) : name(n) {
        cout << "  [建構] " << name << "（模擬耗時初始化...）" << endl;
    }
    
    ~ExpensiveResource() {
        cout << "  [解構] " << name << endl;
    }
    
    void use() const {
        cout << "  [使用] " << name << endl;
    }
};

ExpensiveResource& getResource() {
    // 靜態局部對象：只在第一次調用時建構
    // C++11 保證這個初始化是線程安全的
    // 注意：這裡的資源名稱可以是固定的，因為它只建構一次
    static ExpensiveResource resource("共享資源");
    return resource;
}

// -----------------------------------------------------------------------------
// 【示範】實際驗證 C++11 的 magic statics：8 條執行緒同時搶初始化
//   用 atomic 計數器記錄建構函式被執行了幾次。
//   若標準保證成立，無論跑幾次，答案都必須是 1。
//   註：這裡只印「建構次數」與「所有執行緒是否拿到同一個物件」這兩個
//       確定性的結果，不印執行緒 id 或位址（那些每次執行都不同）。
// -----------------------------------------------------------------------------
atomic<int> g_ctorCount{0};

struct RaceTarget {
    int id;
    RaceTarget() : id(42) {
        g_ctorCount.fetch_add(1, memory_order_relaxed);
        // 稍微拖長初始化時間，讓多執行緒更容易真的撞在一起
        this_thread::sleep_for(chrono::milliseconds(20));
    }
};

RaceTarget& getRaceTarget() {
    static RaceTarget t;      // C++11 保證：只有一個執行緒會執行這個初始化
    return t;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】應用程式設定的延遲載入（Meyers Singleton）
//   情境：設定要從檔案/環境變數讀出來，成本不低，而且程式的許多角落都要用。
//   用 static 區域變數，可以做到：
//     ● 程式啟動時不付出載入成本，第一次真的要用才載入
//     ● 全程式共用同一份，不會重複解析
//     ● 不必擔心與其他全域物件的初始化順序（見 5.cpp 的反例）
//   這裡用一個計數器證明「無論被要求幾次，實際載入只發生一次」。
// -----------------------------------------------------------------------------
class AppConfig {
    string logLevel;
    int    maxConnections;
public:
    static int loadCount;          // 統計真正執行載入的次數

    AppConfig() : logLevel("INFO"), maxConnections(100) {
        ++loadCount;
        cout << "    [載入] 解析設定檔中...（只會出現一次）" << endl;
    }
    const string& getLogLevel() const { return logLevel; }
    int getMaxConnections() const { return maxConnections; }
};
int AppConfig::loadCount = 0;

const AppConfig& config() {
    static AppConfig instance;     // 第一次呼叫才載入
    return instance;
}

// 註：本檔不加 LeetCode 範例。
//     靜態區域變數與延遲初始化屬於「程式生命週期管理」的機制，
//     LeetCode 的題目不會因為使用它而改變解法或複雜度；
//     硬掛一題（例如拿它當快取）反而會誤導讀者以為單例是解題常規手段，
//     故從缺。

int main() {
    cout << "=== 延遲初始化展示 ===" << endl;
    
    cout << "\n--- 程式啟動，但還沒使用資源 ---" << endl;
    cout << "  (注意：資源還沒被建構)\n" << endl;
    
    cout << "--- 第一次調用 getResource() ---" << endl;
    getResource().use();    // 第一次：觸發建構, 然後使用
    
    cout << "\n--- 第二次調用 getResource() ---" << endl;
    getResource().use();    // 第二次：不再建構，直接使用, 因為已經存在了
    
    cout << "\n--- 第三次調用 getResource() ---" << endl;
    getResource().use();    // 第三次：同上, 仍然使用同一個已建構的資源

    // ====== 驗證 C++11 magic statics ======
    cout << "\n=== 驗證：8 條執行緒同時初始化，只會建構一次 ===" << endl;
    {
        const int N = 8;
        vector<thread> workers;
        atomic<int> sameObject{0};

        RaceTarget* firstSeen = nullptr;
        mutex mtx;

        for (int i = 0; i < N; ++i) {
            workers.emplace_back([&]() {
                RaceTarget& r = getRaceTarget();
                lock_guard<mutex> lk(mtx);
                if (firstSeen == nullptr) firstSeen = &r;
                // 只記錄「是不是同一個物件」的布林結果，不印位址
                if (firstSeen == &r) sameObject.fetch_add(1, memory_order_relaxed);
            });
        }
        for (thread& t : workers) t.join();

        cout << "  參與執行緒數      : " << N << endl;
        cout << "  建構函式執行次數  : " << g_ctorCount.load() << "（標準保證必為 1）" << endl;
        cout << "  拿到同一個物件的數: " << sameObject.load() << " / " << N << endl;
    }

    cout << "\n=== 日常實務：設定的延遲載入 ===" << endl;
    {
        cout << "  尚未存取設定，loadCount = " << AppConfig::loadCount << endl;
        cout << "  第一次存取 config():" << endl;
        // 刻意先在獨立敘述取得參考：否則依 C++17「左運算元先於右運算元」
        // 的求值順序，載入訊息會插進下一行輸出的中間
        const AppConfig& c1 = config();
        cout << "    logLevel = " << c1.getLogLevel() << endl;
        cout << "  第二次存取 config():" << endl;
        cout << "    maxConnections = " << config().getMaxConnections() << endl;
        cout << "  實際載入次數 = " << AppConfig::loadCount << "（無論被要求幾次都只載入一次）" << endl;
    }
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
// 程式結束時，靜態局部對象才被解構

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "第 18 課：對象的生命週期（Object Lifetime）4.cpp" -o life4
//   註：本檔用到 std::thread 驗證 magic statics，必須加 -pthread。

// === 預期輸出 ===
// === 延遲初始化展示 ===
//
// --- 程式啟動，但還沒使用資源 ---
//   (注意：資源還沒被建構)
//
// --- 第一次調用 getResource() ---
//   [建構] 共享資源（模擬耗時初始化...）
//   [使用] 共享資源
//
// --- 第二次調用 getResource() ---
//   [使用] 共享資源
//
// --- 第三次調用 getResource() ---
//   [使用] 共享資源
//
// === 驗證：8 條執行緒同時初始化，只會建構一次 ===
//   參與執行緒數      : 8
//   建構函式執行次數  : 1（標準保證必為 1）
//   拿到同一個物件的數: 8 / 8
//
// === 日常實務：設定的延遲載入 ===
//   尚未存取設定，loadCount = 0
//   第一次存取 config():
//     [載入] 解析設定檔中...（只會出現一次）
//     logLevel = INFO
//   第二次存取 config():
//     maxConnections = 100
//   實際載入次數 = 1（無論被要求幾次都只載入一次）
//
// === main() 結束 ===
//   [解構] 共享資源
