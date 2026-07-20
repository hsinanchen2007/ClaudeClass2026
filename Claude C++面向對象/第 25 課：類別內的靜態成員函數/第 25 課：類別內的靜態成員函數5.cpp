// =============================================================================
//  第 25 課：類別內的靜態成員函數 5  —  單例存取點（Meyers' Singleton）
// =============================================================================
//
// 【主題資訊 Information】
//   模式:
//     class C {
//         C();                                   // private
//     public:
//         C(const C&) = delete;                  // 禁止拷貝
//         C& operator=(const C&) = delete;
//         static C& getInstance() {              // 靜態存取點
//             static C instance;                 // function-local static
//             return instance;
//         }
//     };
//   標準版本: function-local static 的「初始化 thread-safe」保證是 C++11 起
//   標頭檔: <string>；本檔的執行緒驗證另需 <atomic> <thread> <vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼存取點一定是靜態函式】
//   呼叫 getInstance() 的當下還沒有物件 —— 它的任務就是把唯一那個交出來。
//   非靜態成員函式需要 this，等於要求「先有物件才能拿到物件」，邏輯矛盾。
//
// 【2. function-local static 的三個關鍵性質】
//   (a) 惰性初始化：第一次執行到那一行才建構。
//       本檔輸出中「[GameManager 初始化]」出現在第一次 getInstance() 時，
//       而不是程式一啟動就出現 —— 這就是證據。
//   (b) 只初始化一次：之後每次呼叫都直接回傳同一個物件。
//   (c) C++11 起初始化保證 thread-safe：
//       編譯器插入 guard 變數，多執行緒同時第一次呼叫時，
//       只有一條會執行建構，其餘會等待。
//       這是標準明文保證（[stmt.dcl]），不是實作巧合。
//       下方實務範例用「建構次數必須恰好為 1」實測這件事。
//
// 【3. 為什麼要 delete 拷貝建構子與拷貝賦值】
//   getInstance() 回傳的是參考。若忘了禁止拷貝，使用者可以寫：
//       GameManager copy = GameManager::getInstance();   // 複製出第二份！
//   那就完全破壞了「唯一」這個前提，而且錯誤非常隱蔽 ——
//   程式照跑，只是狀態默默分裂成兩份。
//   delete 之後這行會直接編譯失敗，問題在編譯期就被擋住。
//
// 【4. Meyers' Singleton 解決了什麼、沒解決什麼】
//   解決：static initialization order fiasco。
//         跨 translation unit 的靜態物件初始化順序是 unspecified，
//         但 function-local static 是「第一次用到才建構」，
//         順序自然等於使用順序，不再有未初始化的問題。
//   沒解決：解構順序。所有靜態物件仍在 main 之後依反向順序解構，
//         若某個單例的解構子去碰另一個已被解構的單例，一樣會出事。
//         真的需要時的作法是刻意不解構（leaky singleton），
//         或用 shared_ptr 明確表達相依關係。
//
// 【概念補充 Concept Deep Dive】
//   * 單例是被廣泛批評的模式：它是包裝過的全域狀態，
//     會讓單元測試難寫（測試之間狀態殘留、無法替換成 mock）、
//     隱藏相依關係（函式簽名看不出它其實用了 GameManager）。
//     現代作法多改用「依賴注入」：把物件當參數傳進去。
//     真正適合單例的情境是「本質上就只有一個」的資源，例如行程層級的 log sink。
//   * getInstance() 回傳參考而非指標，是刻意的：
//     參考不可能是 null，呼叫端不必檢查，也無法 delete 它。
//   * 這個單例「永遠不會是空的」，但它的狀態會跨測試殘留 ——
//     寫測試時通常要提供 reset() 或改用可注入的設計。
//   * 本檔沒有印出物件位址。位址每次執行都不同（ASLR），
//     印出來無法當成穩定的預期輸出；要驗證唯一性，比較指標得到布林值即可。
//
// 【注意事項 Pay Attention】
//   1. 一定要 delete 拷貝建構子與拷貝賦值，否則唯一性可被輕易破壞。
//   2. C++11 之前 function-local static 的初始化「不」保證 thread-safe。
//   3. Meyers' Singleton 只解決初始化順序，不解決解構順序。
//   4. 單例是全域狀態，會傷害可測試性；能用依賴注入就別用單例。
//   5. 驗證唯一性請比較指標印布林值，不要印位址（每次執行都不同）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】單例模式與靜態存取點
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Meyers' Singleton 為什麼是執行緒安全的?
//     答：因為它用的是 function-local static。
//         C++11 起標準規定：若多條執行緒同時第一次執行到該宣告，
//         只有一條會執行初始化，其餘必須等待初始化完成 ——
//         編譯器會插入一個 guard 變數來實現。
//         所以不需要自己加鎖，也不需要 double-checked locking。
//     追問：C++11 之前呢?
//         → 沒有這個保證。當年必須自己加鎖，
//         而知名的 double-checked locking 寫法在沒有記憶體模型的 C++98 下
//         其實是壞的（沒有 happens-before 保證，可能拿到未建構完成的物件）。
//
// 🔥 Q2. 為什麼 getInstance() 回傳參考而不是指標?
//     答：參考不可能是 null，呼叫端不必做無謂的空指標檢查；
//         而且參考無法被 delete，避免使用者誤刪唯一實例。
//         回傳指標則兩個風險都存在。
//     追問：那為什麼還要 delete 拷貝建構子?
//         → 因為 GameManager copy = GameManager::getInstance();
//         會從那個參考複製出第二個物件，唯一性就沒了。
//         這個錯誤不會有任何執行期徵兆，狀態只是默默分裂，
//         所以必須在編譯期擋掉。
//
// ⚠️ 陷阱. 「單例保證只有一個實例，所以它天生執行緒安全。」
//     答：兩件事完全不同。C++11 保證的只有「初始化」是執行緒安全的 ——
//         也就是建構子只會被執行一次。
//         但建立之後的成員函式呼叫完全沒有任何保護：
//         兩條執行緒同時呼叫 addScore()，score_ += points
//         仍然是非原子的讀-改-寫，就是資料競爭（未定義行為）。
//     為什麼會錯：把「建立過程安全」誤推成「使用過程安全」。
//         單例反而讓這件事更危險 —— 因為它天生就是被所有執行緒共享的，
//         比一般物件更容易踩到競爭。要安全就得自己在成員函式裡加鎖，
//         或把狀態換成 std::atomic。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 從缺，理由如下
//   單例解決的是「全域唯一資源的存取與初始化時機」，屬於程式架構問題，
//   LeetCode 判題只驗演算法輸入輸出。
//   事實上在 LeetCode 用單例反而有害：判題程式在同一行程內連續跑多筆測資，
//   單例的狀態不會重置，會造成跨測資污染（本課 24 的綜合檔有可執行示範）。
//   本檔改以「設定中心」的實務範例，實測 C++11 的一次性初始化保證。
//
// =============================================================================

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

class GameManager {
private:
    int score_;
    int level_;
    string currentMap_;

    // 私有建構函數：外部不能創建
    // 這個類別只能有一個實例，通過靜態函數 getInstance() 獲取
    // 這是一種常見的設計模式，稱為 "singleton"（單例模式）
    // 在第一次調用 getInstance() 時，靜態局部變數 instance 會被創建並初始化
    // 之後每次調用 getInstance() 都會返回同一個 instance，確保全局只有一個 GameManager 對象
    // 單例模式適合用於管理遊戲狀態、配置、資源等全局共享的對象
    // 在遊戲開發中，GameManager 通常負責管理遊戲的整體狀態，例如分數、關卡、當前地圖等
    // 這個類別的設計確保了遊戲中只有一個 GameManager 實例，避免了多個實例之間的狀態不一致問題
    // 單例模式的實現方式有很多種，這裡使用了 C++11 以後的 "Meyers' Singleton" 實現方式，利用了靜態局部變數的特性來確保線程安全和唯一性
    // 在 C++11 以後，靜態局部變數的初始化是線程安全的，因此這種實現方式不需要額外的鎖來保護 instance 的創建
    // 這個建構函數是私有的，外部無法直接創建 GameManager 對象
    GameManager() : score_(0), level_(1), currentMap_("新手村") {
        cout << "  [GameManager 初始化]" << endl;
    }

public:
    // 禁止拷貝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 靜態函數：獲取唯一實例
    // 這個函數會返回 GameManager 的唯一實例，確保全局只有一個 GameManager 對象
    // 這個函數是靜態的，因為它不依賴於任何實例，可以直接通過類別名調用
    // 第一次調用 getInstance() 時，靜態局部變數 instance 會被創建並初始化
    // 之後每次調用 getInstance() 都會返回同一個 instance，確保全局只有一個 GameManager 對象
    // 單例模式適合用於管理遊戲狀態、配置、資源等全局共享的對象
    static GameManager& getInstance() {
        static GameManager instance;   // 靜態局部變數，只初始化一次
        return instance;
    }

    // 普通成員函數
    // 這些函數操作 GameManager 的狀態，例如增加分數、進入下一關、設置當前地圖等
    // 這些函數不是靜態的，因為它們需要操作 GameManager 的實例變數
    // 這些函數可以通過 getInstance() 獲取的實例來調用，例如 GameManager::getInstance().addScore(100);
    // 這些函數提供了對 GameManager 狀態的操作接口，讓遊戲邏輯可以通過這些函數來修改遊戲狀態，例如增加分數、進入下一關、設置當前地圖等
    // 這些函數的實現非常簡單，主要是修改 GameManager 的成員變數並輸出當前狀態的訊息
    void addScore(int points) {
        score_ += points;
        cout << "  得分 +" << points << " (總分:" << score_ << ")" << endl;
    }

    void nextLevel() {
        level_++;
        cout << "  進入第 " << level_ << " 關" << endl;
    }

    void setMap(const string& map) { currentMap_ = map; }

    void printStatus() const {
        cout << "  [遊戲狀態] 地圖:" << currentMap_
             << " 關卡:" << level_ << " 分數:" << score_ << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定中心：實測「初始化只發生一次」
//   情境：服務啟動後所有模組都要讀同一份設定。設定的載入很昂貴
//         （讀檔、解析），而且必須保證只做一次。
//   驗證方式：用「計數」而非「計時」——
//         讓 8 條執行緒同時第一次呼叫 instance()，
//         用 atomic 記錄建構子被執行幾次。
//         C++11 保證 function-local static 的初始化是 thread-safe 的，
//         所以這個數字必須「恰好是 1」。這個結果是確定的，每次執行都相同。
//   注意：保證的是「初始化」，不是「之後的使用」。
//         下面 readCount_ 刻意用 atomic，正是因為讀取計數不在保證範圍內。
// -----------------------------------------------------------------------------
class AppConfig {
private:
    string logLevel_;
    int    maxConnections_;

    // 記錄建構子實際被執行幾次（正確的話永遠是 1）
    inline static atomic<int> constructCount_{0};
    // 單例被使用的次數 —— 這部分「不」受 C++11 的保證涵蓋，故用 atomic
    inline static atomic<long> readCount_{0};

    AppConfig() : logLevel_("info"), maxConnections_(100) {
        constructCount_.fetch_add(1, memory_order_relaxed);
    }

public:
    AppConfig(const AppConfig&)            = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    static AppConfig& instance() {
        static AppConfig cfg;      // C++11：初始化保證 thread-safe
        return cfg;
    }

    const string& logLevel() const { return logLevel_; }
    int maxConnections()     const { return maxConnections_; }

    void markRead() { readCount_.fetch_add(1, memory_order_relaxed); }

    static int  constructions() { return constructCount_.load(memory_order_relaxed); }
    static long reads()         { return readCount_.load(memory_order_relaxed); }
};

int main() {
    cout << "=== 單例存取點（靜態函數）===" << endl;

    // 通過靜態函數獲取唯一實例
    cout << "\n--- 第一次存取 ---" << endl;
    GameManager::getInstance().printStatus();

    cout << "\n--- 遊戲進行 ---" << endl;
    GameManager::getInstance().addScore(100);
    GameManager::getInstance().addScore(250);
    GameManager::getInstance().nextLevel();
    GameManager::getInstance().setMap("暗黑森林");

    cout << "\n--- 查看狀態 ---" << endl;
    GameManager::getInstance().printStatus();

    // 驗證是同一個實例
    cout << "\n--- 驗證唯一性 ---" << endl;
    GameManager& ref1 = GameManager::getInstance();
    GameManager& ref2 = GameManager::getInstance();
    // 刻意不印位址：位址每次執行都不同（ASLR），無法當成穩定的預期輸出。
    // 要驗證的是「是不是同一個物件」，比較指標得到布林值即可。
    cout << "  兩次 getInstance() 是同一個對象："
         << (&ref1 == &ref2 ? "是" : "否") << endl;
    cout << "  （位址本身每次執行都不同，故不列印）" << endl;

    cout << "\n=== 日常實務：設定中心的一次性初始化（8 執行緒實測）===" << endl;
    {
        constexpr int kThreads = 8;

        cout << "  啟動前建構次數 = " << AppConfig::constructions()
             << "（尚未有人呼叫 instance()，惰性初始化還沒發生）" << endl;

        vector<thread> workers;
        workers.reserve(kThreads);
        for (int i = 0; i < kThreads; ++i) {
            workers.emplace_back([] {
                // 8 條執行緒「同時」第一次取得單例
                AppConfig& c = AppConfig::instance();
                c.markRead();
            });
        }
        for (auto& w : workers) w.join();

        cout << "  " << kThreads << " 條執行緒各取用一次後：" << endl;
        cout << "    建構子執行次數 = " << AppConfig::constructions()
             << "（必須恰好為 1）" << endl;
        cout << "    取用次數       = " << AppConfig::reads()
             << "（必須等於 " << kThreads << "）" << endl;
        cout << "    設定內容       = logLevel:" << AppConfig::instance().logLevel()
             << " maxConnections:" << AppConfig::instance().maxConnections() << endl;
        cout << "  結果："
             << ((AppConfig::constructions() == 1 && AppConfig::reads() == kThreads)
                 ? "✔ 正確（C++11 保證 function-local static 只初始化一次）"
                 : "✘ 不符預期")
             << endl;
        cout << "  註：這裡驗的是「初始化只發生一次」。" << endl;
        cout << "      建立之後的成員函式呼叫並不在這個保證範圍內 ——" << endl;
        cout << "      readCount_ 用 atomic 正是因為它需要自己負責同步。" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread 第 25 課：類別內的靜態成員函數5.cpp -o static_func5

// === 預期輸出 ===
// === 單例存取點（靜態函數）===
//
// --- 第一次存取 ---
//   [GameManager 初始化]
//   [遊戲狀態] 地圖:新手村 關卡:1 分數:0
//
// --- 遊戲進行 ---
//   得分 +100 (總分:100)
//   得分 +250 (總分:350)
//   進入第 2 關
//
// --- 查看狀態 ---
//   [遊戲狀態] 地圖:暗黑森林 關卡:2 分數:350
//
// --- 驗證唯一性 ---
//   兩次 getInstance() 是同一個對象：是
//   （位址本身每次執行都不同，故不列印）
//
// === 日常實務：設定中心的一次性初始化（8 執行緒實測）===
//   啟動前建構次數 = 0（尚未有人呼叫 instance()，惰性初始化還沒發生）
//   8 條執行緒各取用一次後：
//     建構子執行次數 = 1（必須恰好為 1）
//     取用次數       = 8（必須等於 8）
//     設定內容       = logLevel:info maxConnections:100
//   結果：✔ 正確（C++11 保證 function-local static 只初始化一次）
//   註：這裡驗的是「初始化只發生一次」。
//       建立之後的成員函式呼叫並不在這個保證範圍內 ——
//       readCount_ 用 atomic 正是因為它需要自己負責同步。
