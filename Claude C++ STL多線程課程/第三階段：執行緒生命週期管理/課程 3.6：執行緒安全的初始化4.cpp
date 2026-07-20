// =============================================================================
//  課程 3.6：執行緒安全的初始化4.cpp  —  用 call_once 實作執行緒安全單例
// =============================================================================
//
// 【主題資訊 Information】
//   模式      : Singleton(單例)+ std::call_once 惰性初始化
//   標準版本  : C++11(once_flag / call_once / = delete)
//   標頭檔    : <mutex>
//   成員配置  : static Singleton* instance;  static std::once_flag initFlag;
//               —— 兩者都必須在類別外定義,否則連結階段會找不到符號
//   複雜度    : 首次呼叫執行建構;之後每次 getInstance() 是一次原子檢查
//
// 【詳細解釋 Explanation】
//
// 【1. 單例的三個必要零件】
// 一個能用的單例要同時做到三件事,少一件都會出問題:
//   (a) 建構子私有 —— 否則別人可以自己 new 一個,「單」就不成立。
//   (b) 複製與賦值刪除 —— 這是最常被忘記的一件。若沒有 = delete,
//       別人可以寫 Singleton s = Singleton::getInstance(); 複製出第二份,
//       前功盡棄。本例用 C++11 的 = delete 明確禁止,錯誤訊息也清楚。
//   (c) 全域唯一的存取點 —— 也就是 getInstance()。
//
// 【2. 為什麼存取點需要 call_once】
// getInstance() 會被多條執行緒同時呼叫。若寫成
//     if (!instance) instance = new Singleton();
// 就是課程 3.6/2 講過的經典競態:兩條執行緒可能同時通過檢查,
// 建出兩個物件、洩漏一個,而且建構子的副作用會發生兩次。
// call_once 一次解決「只執行一次」與「其他人要等它做完」兩個需求。
//
// 【3. static 成員為什麼一定要在類別外定義】
//     static Singleton* instance;        // 類別內:只是「宣告」
//     Singleton* Singleton::instance = nullptr;   // 類別外:才是「定義」
// 類別內的 static 資料成員(C++17 以前,非 inline、非 constexpr)只是宣告,
// 沒有分配儲存空間。少了類別外那行,編譯會過但連結會失敗
// (undefined reference)。once_flag 同理。
// C++17 起可以寫 inline static Singleton* instance = nullptr; 就不必分兩處。
//
// 【4. 這個實作的真實代價:物件永遠不會被解構】
// instance 是用 new 配置的,而程式裡沒有任何地方 delete 它 ——
// 這個單例會活到行程結束,解構子永遠不執行。
// 這是「刻意的取捨」而不是單純的疏忽,理由是:
//   * 避免「static 解構順序問題」:若在程式結束時銷毀單例,而其他
//     static 物件的解構子還想使用它,就會存取到已銷毀的物件。
//     這個問題有個名字叫 static destruction order fiasco。
//   * 行程結束時作業系統會回收全部記憶體,單純的記憶體並不會真的「漏」。
// 但要注意:如果解構子有「必須執行」的副作用(flush 檔案、送出關閉封包、
// 釋放 OS 層資源),這個做法就會靜默地跳過它們 —— 那才是真正的 bug。
// 需要確定的解構,請改用本課第 5 個範例檔的 magic static 寫法。
//
// 【概念補充 Concept Deep Dive】
//
// (A) once_flag 為什麼必須是 static 成員而不是區域變數
//   旗標代表「這件事做過了沒」這個全域唯一的事實。若把它寫成
//   getInstance() 內的區域變數,每次呼叫都會得到一個全新的旗標,
//   等於每次都認為「還沒初始化」,保護完全失效。
//   (寫成函式內的 static 區域變數則是可以的 —— 那就只有一份。)
//
// (B) 為什麼刪除的是複製建構子而不是只把它設成 private
//   C++11 之前的慣用法是「宣告成 private 但不實作」,讓誤用在連結期報錯。
//   = delete 更好:錯誤在編譯期就出現,訊息明確指出「已被刪除」,
//   而且連類別自己的成員函式與 friend 也一併被禁止,語意更完整。
//
// (C) 單例本身並不會讓它的方法變成執行緒安全
//   call_once 保護的只有「建立」。doSomething() 若會修改成員狀態,
//   多條執行緒同時呼叫仍然是資料競爭,必須另外用 mutex/atomic 保護。
//   本例的 doSomething() 只印字串、不碰狀態,所以是安全的。
//
// 【注意事項 Pay Attention】
// 1. static 成員必須在類別外定義,否則連結失敗(C++17 起可用 inline static)。
// 2. 記得同時刪除複製建構子與複製賦值運算子,只刪一個仍可能被繞過。
// 3. 這個寫法的單例永遠不會解構;解構子有必要副作用時不可使用。
// 4. call_once 只保證「建立」安全,不保證「使用」安全。
// 5. 單例是全域狀態,會讓單元測試難以替換相依。能用依賴注入時優先用注入。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全的單例
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 實作單例時,除了把建構子設成 private,還有什麼一定要做?
//     答：必須刪除複製建構子與複製賦值運算子。否則
//         Singleton s = Singleton::getInstance();
//         就能複製出第二個物件,單例的保證直接被破壞。
//         另外存取點要用 call_once 或 magic static 保證多執行緒下只建一次。
//     追問：為什麼用 = delete 而不是宣告成 private 不實作?
//         → = delete 在編譯期就報錯且訊息明確;舊寫法要到連結期才發現,
//           而且類別自己的成員函式仍可呼叫得到。
//
// 🔥 Q2. 這個版本的單例,物件什麼時候被解構?
//     答：永遠不會。instance 是 new 出來的,程式裡沒有任何 delete,
//         它會活到行程結束。這通常是刻意的 —— 為了避開 static 解構順序問題,
//         而且行程結束時記憶體本來就會被作業系統回收。
//     追問：那什麼情況下這樣做是有害的?
//         → 當解構子有「必須執行」的副作用時,例如 flush 檔案緩衝、
//           送出關閉訊息、釋放 OS 層資源。那些工作會被靜默跳過。
//           這時應改用函式內 static(magic static),它保證會被解構。
//
// ⚠️ 陷阱. 「getInstance() 用了 call_once,所以這個單例是執行緒安全的,
//         多條執行緒可以放心同時呼叫它的任何方法。」哪裡錯了?
//     答：call_once 保護的範圍只有「建立那一次」。它確保物件只被建構一次、
//         而且大家看到的都是完整建構好的物件。但物件建好之後,
//         若多條執行緒同時呼叫會修改成員的方法,那是另一個資料競爭,
//         call_once 完全不管。
//     為什麼會錯：把「取得實例的過程是執行緒安全的」誤讀成
//         「這個類別是執行緒安全的」。前者只涵蓋一個瞬間,
//         後者需要類別自己為每個會改狀態的操作加上同步。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

void say(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << msg << std::endl;
}

// -----------------------------------------------------------------------------
// 原始示範:call_once 版單例
// -----------------------------------------------------------------------------
class Singleton {
    static Singleton* instance;
    static std::once_flag initFlag;

    Singleton() { say("  Singleton 建立"); }

public:
    Singleton(const Singleton&) = delete;             // (b) 禁止複製
    Singleton& operator=(const Singleton&) = delete;  // (b) 禁止賦值

    static Singleton& getInstance() {
        std::call_once(initFlag, []() {
            instance = new Singleton();
        });
        return *instance;
    }

    void doSomething() { say("  工作中"); }
};

// (3) static 成員必須在類別外定義,否則連結失敗
Singleton* Singleton::instance = nullptr;
std::once_flag Singleton::initFlag;

// -----------------------------------------------------------------------------
// 【日常實務範例】程式全域唯一的計量器(metrics counter)
//   情境: 服務要統計「每個 API 被呼叫幾次」,這份統計必須是全程式唯一的一份,
//         而且會被大量 worker thread 同時更新。
//         這裡刻意示範【概念補充 C】的重點:單例的「取得」是安全的,
//         但「更新內部狀態」必須自己加鎖 —— 兩者是分開的兩件事。
//   為什麼用本主題: 這是單例最常見的真實用途,也最容易踩到
//                   「以為 call_once 保護了一切」的誤解。
// -----------------------------------------------------------------------------
class Metrics {
    static Metrics* instance;
    static std::once_flag flag;

    std::mutex mtx_;                    // 保護底下的計數,call_once 不管這塊
    long apiCalls_ = 0;

    Metrics() { say("    [metrics] 計量器建立(只會出現一次)"); }

public:
    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;

    static Metrics& get() {
        std::call_once(flag, []() { instance = new Metrics(); });
        return *instance;
    }

    // 會修改狀態 → 必須自己同步
    void recordCall() {
        std::lock_guard<std::mutex> lock(mtx_);
        ++apiCalls_;
    }

    long total() {
        std::lock_guard<std::mutex> lock(mtx_);
        return apiCalls_;
    }
};

Metrics* Metrics::instance = nullptr;
std::once_flag Metrics::flag;

void apiWorker(int callsPerThread) {
    for (int i = 0; i < callsPerThread; ++i) {
        Metrics::get().recordCall();
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   考的是執行緒之間的交替與順序控制,與「單例只建立一次」是不同的問題;
//   設計題(146 LRU Cache、155 Min Stack、705 Design HashSet、
//   707 Design Linked List)則都是單執行緒的資料結構題,本身不涉及初始化競態。
//   單例模式在 LeetCode 上沒有對應題型,因此誠實從缺;
//   本課第 7 個範例檔會示範一個真正用得上 146 的組合
//   (執行緒安全的延遲初始化 + LRU 快取)。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:兩條執行緒同時取得單例 ===" << std::endl;
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    t1.join();
    t2.join();
    std::cout << "  ↑「Singleton 建立」只出現一次" << std::endl;

    std::cout << "\n=== 驗證真的是同一個物件 ===" << std::endl;
    std::cout << "  兩次 getInstance() 位址相同? " << std::boolalpha
              << (&Singleton::getInstance() == &Singleton::getInstance())
              << std::endl;

    std::cout << "\n=== 實務:8 條執行緒各記錄 1000 次呼叫 ===" << std::endl;
    std::vector<std::thread> pool;
    for (int i = 0; i < 8; ++i) pool.emplace_back(apiWorker, 1000);
    for (std::thread& t : pool) t.join();
    std::cout << "  總呼叫次數 = " << Metrics::get().total()
              << " (期望 8000;計數正確是因為 recordCall 自己有鎖,"
                 "不是因為 call_once)" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.6：執行緒安全的初始化4.cpp" -o once_singleton

// 注意:以下為某一次實際執行的結果。
//   * 兩條執行緒印出「工作中」的先後順序每次執行都可能不同。
//   * 但關鍵不變量每次都成立:「Singleton 建立」恰好一行、
//     「[metrics] 計量器建立」恰好一行、總呼叫次數恰好 8000。
//   * 本例的單例刻意不解構(見【詳細解釋 4】),因此不會有任何銷毀訊息。

// === 預期輸出 ===
// === 原始示範:兩條執行緒同時取得單例 ===
//   Singleton 建立
//   工作中
//   工作中
//   ↑「Singleton 建立」只出現一次
//
// === 驗證真的是同一個物件 ===
//   兩次 getInstance() 位址相同? true
//
// === 實務:8 條執行緒各記錄 1000 次呼叫 ===
//     [metrics] 計量器建立(只會出現一次)
//   總呼叫次數 = 8000 (期望 8000;計數正確是因為 recordCall 自己有鎖,不是因為 call_once)
