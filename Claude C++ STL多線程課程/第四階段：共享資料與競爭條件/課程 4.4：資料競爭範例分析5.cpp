// =============================================================================
//  課程 4.4：資料競爭範例分析5.cpp  —  Singleton：初始化競爭與雙重檢查鎖定
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   getInstance() 對 static 指標 instance 一邊讀一邊寫，完全沒有同步
//   → genuine data race → 【未定義行為 (UB)】。
//   本檔【不會】宣稱「一定會建立兩個實例」或「一定只建立一個」。
//
// 【主題資訊 Information】
//   主題：    lazy initialization 的競爭；五大競爭模式之三（複合操作）
//   語法：    if (instance == nullptr) instance = new Singleton();   // check-then-act
//   標準版本：magic static（function-local static 初始化執行緒安全）為 C++11
//             std::call_once / std::once_flag 為 C++11
//   標頭檔：  <mutex>（call_once）
//   偵測工具：g++ -fsanitize=thread -g -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. 這又是 check-then-act，只是套上了 Singleton 的外衣】
//       if (instance == nullptr) {   // ← check
//           // ★ 縫隙：兩條執行緒可以同時通過這個檢查
//           instance = new Singleton();   // ← act：兩個都 new，一個被覆蓋掉
//       }
//   後果有三個，嚴重程度遞增：
//     ① 記憶體洩漏：被覆蓋掉的那個物件永遠沒人 delete。
//     ② 違反 Singleton 的語意：曾經有兩個實例存在過，
//        若建構函式有副作用（開檔、綁 port、註冊回呼）就會執行兩次。
//     ③ 不同執行緒可能拿到【不同】的實例指標，
//        「全域唯一狀態」的假設整個破產 —— 這是最難查的一種。
//
// 【2. 經典的「雙重檢查鎖定 (DCLP)」以及它為什麼曾經是錯的】
//   1990 年代流行的寫法：
//       if (instance == nullptr) {              // 第一次檢查（不加鎖，為了效能）
//           lock_guard lk(mtx);
//           if (instance == nullptr)            // 第二次檢查（加鎖後再確認）
//               instance = new Singleton();
//       }
//   直覺上很完美，但在 C++11 之前它是【壞的】，原因是這一行：
//       instance = new Singleton();
//   實際上是三個步驟：
//       ① 配置記憶體  ② 在該記憶體上建構物件  ③ 把位址寫入 instance
//   編譯器與 CPU 都可以把 ③ 排到 ② 前面（因為單執行緒下看不出差別）。
//   於是另一條執行緒可能在第一次檢查時看到 instance 非空、直接回傳，
//   拿到一個【尚未建構完成】的物件 → 使用未初始化的成員 → UB。
//   → 這就是著名的 "Double-Checked Locking is Broken" 宣言（2004）。
//
// 【3. C++11 之後的三種正確寫法】
//   (a) Meyers Singleton（首選，最簡單）
//         static Singleton& instance() { static Singleton s; return s; }
//       C++11 保證 function-local static 的【初始化】是執行緒安全的
//       （所謂 magic static）：編譯器插入 guard variable，
//       確保只建構一次，其他執行緒會等待建構完成。
//       零手寫同步、零洩漏（程式結束時自動解構）、效能極佳
//       （初始化後只剩一次 guard 檢查，通常被最佳化成一次載入 + 分支）。
//   (b) std::call_once + std::once_flag
//         static std::once_flag flag;
//         std::call_once(flag, []{ instance = new Singleton(); });
//       語意最明確，適合「初始化動作不只是建構一個物件」的情形。
//   (c) 修好的 DCLP：把指標改成 std::atomic<Singleton*>，
//       寫入用 memory_order_release、讀取用 memory_order_acquire。
//       這是唯一還需要手寫 DCLP 的場合，但既然有 (a)(b)，
//       實務上幾乎沒有理由這樣寫。
//
// 【4. 「magic static 執行緒安全」的精確範圍（極常被誤解）】
//   C++11 保證的是【初始化】只發生一次、且並行呼叫者會等待完成。
//   它【不保證】物件建構完成之後的任何操作是執行緒安全的。
//       Logger::instance().write("x");   // instance() 安全
//                          ^^^^^^^^^^    // write() 要不要鎖是另一回事
//   把兩者混為一談，是「我用了 Meyers Singleton，所以我的單例執行緒安全」
//   這句常見錯誤的來源。
//
// 【5. 為什麼本檔的錯誤版本多數時候測不出來】
//   兩條執行緒各只呼叫一次 getInstance()，而執行緒建立成本
//   （本機約數十 μs）遠大於一次指標比較。第二條執行緒啟動時，
//   第一條通常早就完成初始化了。要撞到必須讓兩者【同時】首次呼叫 ——
//   本檔第二段用「柵欄」讓多條執行緒同時起跑，把競爭視窗放大到可以觀察。
//
// 【概念補充 Concept Deep Dive】
//
// (A) magic static 在本機的實作（實測）
//   g++ 對 `static Singleton s;` 產生一個 guard variable，並呼叫
//   `__cxa_guard_acquire` / `__cxa_guard_release`（Itanium C++ ABI）。
//   快路徑只是檢查 guard 的第一個 byte：已初始化就直接跳過，
//   成本約等於一次載入加一次分支。
//   只有第一次會真的進入 __cxa_guard_acquire 走同步邏輯。
//   → 所以「Meyers Singleton 每次呼叫都要同步、很慢」是過時的說法。
//
// (B) 為什麼 Meyers Singleton 也解決了解構順序問題
//   用 `new` 出來的 Singleton 永遠不會被解構（除非自己管理），
//   這在需要 flush 檔案、關閉連線時會出問題。
//   function-local static 的解構會在程式結束時以「建構的相反順序」自動執行。
//   （但要注意 static destruction order fiasco：
//     若解構函式又去用另一個已被解構的 singleton，仍會出事。）
//
// (C) 為什麼不是「所有 lazy 初始化都該用 call_once」
//   call_once 每次呼叫都要檢查 once_flag 的狀態（雖然很便宜），
//   而 magic static 的快路徑通常更短，且程式碼更少、不可能寫錯。
//   只有當初始化邏輯無法塞進一個物件的建構函式時，才選 call_once。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定會建立兩個實例」，也不可說「一定只有一個」。
// 2. C++11 的 magic static 只保證【初始化】安全，不保證後續成員操作安全。
// 3. C++11 之前的手寫 DCLP 是壞的，原因是「配置 / 建構 / 賦值」可被重排。
// 4. 用 new 的 Singleton 永遠不會解構；需要收尾動作時要特別注意。
// 5. Singleton 本身是被廣泛質疑的設計（隱藏相依、難測試）；
//    能用依賴注入就不要用 Singleton。本檔的重點是「初始化競爭」，
//    不是在推薦這個模式。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   Singleton 的 lazy initialization 競爭沒有對應的 LeetCode 題目：
//   允許使用的設計題（146/155/705/707/1603）都是資料結構設計，
//   並行題（1114～1117/1195）講的是執行緒間的順序協調，
//   兩者都無法誠實對應「初始化只能發生一次」這個主題。
//   故從缺，改以下方兩個真實情境（連線池的 lazy 建立、設定檔的一次性載入）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Singleton 與執行緒安全的初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++11 之前的雙重檢查鎖定為什麼是錯的?
//     答：因為 `instance = new Singleton();` 是三個步驟
//         （配置記憶體 → 建構物件 → 寫入指標），
//         編譯器與 CPU 都可以把「寫入指標」排到「建構物件」之前。
//         另一條執行緒在第一次（無鎖的）檢查時看到指標非空就直接回傳，
//         拿到一個尚未建構完成的物件 → 使用未初始化成員 → UB。
//     追問：C++11 之後要怎麼寫才對?
//         → 首選 Meyers Singleton（function-local static，magic static 保證）；
//           或 std::call_once；真要手寫 DCLP 就必須用 std::atomic 搭配
//           release / acquire 順序。
//
// 🔥 Q2. Meyers Singleton 是執行緒安全的嗎?
//     答：它的【初始化】是執行緒安全的 —— C++11 起標準保證
//         function-local static 只會被建構一次，其他執行緒會等待完成。
//         但這個保證【到初始化為止】，物件建構完成後對它做的任何操作
//         都沒有保護，要不要加鎖是完全獨立的另一個問題。
//     追問：那它每次呼叫都要付同步成本嗎?
//         → 幾乎不用。實作（Itanium ABI）的快路徑只檢查 guard variable 的
//           一個 byte，已初始化就直接跳過，成本約等於一次載入加一次分支；
//           只有第一次才真的走 __cxa_guard_acquire 的同步邏輯。
//
// ⚠️ 陷阱. 「我改用 Meyers Singleton 了，所以我的 Logger 現在執行緒安全」——錯在哪?
//     答：只有「取得那個物件」這件事變安全了。
//         Logger::instance() 保證回傳同一個、已建構完成的物件，
//         但接下來的 .write(...) 若會修改內部緩衝區，
//         多執行緒同時呼叫仍然是 data race。
//     為什麼會錯：把「取得單例」與「使用單例」混為一談。
//         C++11 給的保證精確地只涵蓋初始化這一步；
//         物件本身的成員函式安不安全，取決於你有沒有自己保護它的狀態。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <map>
#include <chrono>

// -----------------------------------------------------------------------------
// 【錯誤示範】無同步的 lazy 初始化 —— 本課主角
// -----------------------------------------------------------------------------
class Singleton {
    static Singleton* instance;

public:
    // 危險！雙重檢查鎖定的錯誤實作
    static Singleton* getInstance() {
        if (instance == nullptr) {          // 第一次檢查
            // ← 多執行緒可能同時通過這裡！
            instance = new Singleton();     // 可能建立多個實例
        }
        return instance;
    }
};

Singleton* Singleton::instance = nullptr;

// -----------------------------------------------------------------------------
// 【放大競爭視窗的版本】用柵欄讓多條執行緒同時首次呼叫
//   這裡刻意計數「建構函式被執行了幾次」，讓「建立多個實例」變成可觀察的事實。
//   注意：這段仍然是 data race / UB，只是我們把窗口撐開以便教學觀察。
//   建構函式裡的 sleep 是為了把「配置 → 建構」這段拉長，
//   模擬真實 Singleton 建構時要開檔、連線、讀設定的耗時。
// -----------------------------------------------------------------------------
std::atomic<int> unsafeCtorCount{0};

class UnsafeLazy {
    static UnsafeLazy* inst;

public:
    UnsafeLazy() {
        unsafeCtorCount.fetch_add(1);
        // 模擬耗時的初始化（開檔 / 連線 / 讀設定）
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    static UnsafeLazy* get() {
        if (inst == nullptr) {          // ← 多條執行緒可同時通過
            inst = new UnsafeLazy();    // ← 於是同時 new，互相覆蓋（洩漏 + 多實例）
        }
        return inst;
    }
};
UnsafeLazy* UnsafeLazy::inst = nullptr;

// -----------------------------------------------------------------------------
// 【正確版 a】Meyers Singleton —— C++11 magic static，首選寫法
// -----------------------------------------------------------------------------
std::atomic<int> meyersCtorCount{0};

class MeyersSingleton {
private:
    MeyersSingleton() {
        meyersCtorCount.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

public:
    MeyersSingleton(const MeyersSingleton&) = delete;
    MeyersSingleton& operator=(const MeyersSingleton&) = delete;

    static MeyersSingleton& instance() {
        static MeyersSingleton s;   // C++11 保證：只建構一次，並行呼叫者會等待
        return s;
    }

    int id() const { return 42; }
};

// -----------------------------------------------------------------------------
// 【正確版 b】std::call_once —— 初始化動作較複雜時使用
// -----------------------------------------------------------------------------
std::atomic<int> callOnceCtorCount{0};

class CallOnceSingleton {
    static CallOnceSingleton* inst;
    static std::once_flag flag;

    CallOnceSingleton() {
        callOnceCtorCount.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

public:
    static CallOnceSingleton* get() {
        std::call_once(flag, [] { inst = new CallOnceSingleton(); });
        return inst;
    }
};
CallOnceSingleton* CallOnceSingleton::inst = nullptr;
std::once_flag CallOnceSingleton::flag;

// -----------------------------------------------------------------------------
// 【日常實務範例 1】資料庫連線池的 lazy 建立
//   情境：連線池很貴（要建 TCP 連線、認證、預熱），所以延後到第一次使用才建。
//         若用錯誤的 lazy 寫法，服務剛啟動、流量瞬間湧入時，
//         幾十條執行緒會【同時】建立各自的連線池 ——
//         資料庫瞬間收到數百條連線，直接打爆 max_connections。
//         這是真實世界中「服務一上線就掛掉」的經典成因。
//   正解：Meyers Singleton（或 call_once），保證只建立一次，
//         其他執行緒會等待第一個建好。
// -----------------------------------------------------------------------------
class ConnectionPool {
private:
    std::vector<std::string> conns;

    ConnectionPool() {
        // 模擬昂貴的初始化：建立 5 條連線
        for (int i = 0; i < 5; ++i) {
            conns.push_back("conn-" + std::to_string(i));
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

public:
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    static ConnectionPool& instance() {
        static ConnectionPool pool;   // 只會被建立一次
        return pool;
    }

    size_t size() const { return conns.size(); }

    // ⚠️ 注意：instance() 安全，不代表這個函式安全。
    //    它會修改 conns，所以【仍然需要】自己的鎖 ——
    //    這正是「magic static 只保證初始化」的實例。
    std::string acquire() {
        static std::mutex mtx;                      // 保護 conns 的鎖
        std::lock_guard<std::mutex> lock(mtx);
        if (conns.empty()) return "";
        std::string c = conns.back();
        conns.pop_back();
        return c;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】設定檔的一次性載入（call_once 的典型用途）
//   情境：設定要從磁碟讀取並解析，只該做一次。
//         初始化動作不只是「建構一個物件」，還包括讀檔、解析、驗證、
//         套用預設值 —— 這種「一段流程」用 call_once 表達得最清楚。
//   額外優點：call_once 若初始化過程拋出例外，flag 不會被設為已完成，
//         下一個呼叫者會【重新嘗試】—— 這個重試語意是 magic static 沒有的。
// -----------------------------------------------------------------------------
class AppConfig {
private:
    static std::once_flag flag;
    static std::map<std::string, std::string> values;
    static std::atomic<int> loadCount;

    static void load() {
        loadCount.fetch_add(1);
        // 模擬讀檔與解析
        std::this_thread::sleep_for(std::chrono::microseconds(300));
        values["db.host"] = "db.internal";
        values["db.port"] = "5432";
        values["log.level"] = "info";
    }

public:
    static std::string get(const std::string& key) {
        std::call_once(flag, load);     // 不論多少執行緒同時進來，load() 只跑一次
        auto it = values.find(key);
        return it == values.end() ? "" : it->second;
    }

    static int loadTimes() { return loadCount.load(); }
};
std::once_flag AppConfig::flag;
std::map<std::string, std::string> AppConfig::values;
std::atomic<int> AppConfig::loadCount{0};

// 讓多條執行緒「同時」起跑的簡易柵欄，用來放大競爭視窗
void waitForGo(std::atomic<bool>& go) {
    while (!go.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
}

int main() {
    std::cout << "=== 錯誤示範：原始的無同步 Singleton（data race / UB）===\n";
    {
        std::thread t1([]{ Singleton::getInstance(); });
        std::thread t2([]{ Singleton::getInstance(); });
        t1.join();
        t2.join();
        std::cout << "兩條執行緒各呼叫一次 getInstance()（沒有輸出可看）\n";
        std::cout << "→ 只跑兩條、又幾乎不可能同時首次呼叫，所以測不出問題；\n";
        std::cout << "  下一段用柵欄放大競爭視窗，讓錯誤變成可觀察的事實。\n";
    }

    std::cout << "\n=== 放大競爭視窗：16 條執行緒同時首次呼叫 ===\n";
    {
        std::atomic<bool> go{false};
        std::vector<std::thread> ths;
        for (int i = 0; i < 16; ++i) {
            ths.emplace_back([&go] { waitForGo(go); UnsafeLazy::get(); });
        }
        go.store(true, std::memory_order_release);   // 一聲令下同時起跑
        for (auto& t : ths) t.join();

        std::cout << "UnsafeLazy 的建構函式被執行了 " << unsafeCtorCount.load()
                  << " 次（Singleton 應該只有 1 次）\n";
        std::cout << "→ 這個次數每次執行都不同，但通常【大於 1】；\n";
        std::cout << "  多出來的物件全部洩漏，且部分執行緒拿到了不同的實例。\n";
    }

    std::cout << "\n=== 正確版 a：Meyers Singleton（C++11 magic static）===\n";
    {
        std::atomic<bool> go{false};
        std::vector<std::thread> ths;
        for (int i = 0; i < 16; ++i) {
            ths.emplace_back([&go] { waitForGo(go); MeyersSingleton::instance(); });
        }
        go.store(true, std::memory_order_release);
        for (auto& t : ths) t.join();

        std::cout << "建構函式執行次數: " << meyersCtorCount.load() << " (必定為 1)\n";
    }

    std::cout << "\n=== 正確版 b：std::call_once ===\n";
    {
        std::atomic<bool> go{false};
        std::vector<std::thread> ths;
        for (int i = 0; i < 16; ++i) {
            ths.emplace_back([&go] { waitForGo(go); CallOnceSingleton::get(); });
        }
        go.store(true, std::memory_order_release);
        for (auto& t : ths) t.join();

        std::cout << "建構函式執行次數: " << callOnceCtorCount.load() << " (必定為 1)\n";
    }

    std::cout << "\n=== 日常實務 1：連線池只建立一次 ===\n";
    {
        std::atomic<bool> go{false};
        std::vector<std::thread> ths;
        std::atomic<int> gotConn{0};
        for (int i = 0; i < 16; ++i) {
            ths.emplace_back([&go, &gotConn] {
                waitForGo(go);
                ConnectionPool& pool = ConnectionPool::instance();
                if (!pool.acquire().empty()) gotConn.fetch_add(1);
            });
        }
        go.store(true, std::memory_order_release);
        for (auto& t : ths) t.join();

        std::cout << "16 條執行緒同時取用連線池\n";
        std::cout << "成功取得連線數: " << gotConn.load() << " (必定 = 5，池內就 5 條)\n";
        std::cout << "→ 連線池只被建立一次；但 acquire() 仍需自己的鎖，\n";
        std::cout << "  因為 magic static 只保證初始化，不保證成員操作。\n";
    }

    std::cout << "\n=== 日常實務 2：設定檔一次性載入（call_once）===\n";
    {
        std::atomic<bool> go{false};
        std::vector<std::thread> ths;
        for (int i = 0; i < 16; ++i) {
            ths.emplace_back([&go] { waitForGo(go); AppConfig::get("db.host"); });
        }
        go.store(true, std::memory_order_release);
        for (auto& t : ths) t.join();

        std::cout << "16 條執行緒同時讀設定\n";
        std::cout << "load() 實際執行次數: " << AppConfig::loadTimes() << " (必定為 1)\n";
        std::cout << "db.host   = " << AppConfig::get("db.host") << "\n";
        std::cout << "log.level = " << AppConfig::get("log.level") << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析5.cpp' -o race5
//
// 偵測資料競爭（前兩段是 UB，唯一可靠的判定方式）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.4：資料競爭範例分析5.cpp' -o race5_tsan
//   ./race5_tsan  → 會報出 getInstance() / UnsafeLazy::get() 對 instance 指標的競爭

// ⚠️「UnsafeLazy 的建構函式被執行了 N 次」這一行【每次執行都可能不同】——
// 該段是 genuine data race → UB，不受標準保證。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）連續五次實測都是 16 次
// （因為建構函式裡的 200 μs sleep 把競爭視窗撐得夠寬，16 條全部通過了檢查），
// 但這是本平台的觀察結果，不是保證值；換機器可能是 2、5 或任何數字。
// 其餘各段（Meyers、call_once、連線池、設定載入）皆有標準保證，必定為 1 / 5 / 1。

// === 預期輸出 ===
// === 錯誤示範：原始的無同步 Singleton（data race / UB）===
// 兩條執行緒各呼叫一次 getInstance()（沒有輸出可看）
// → 只跑兩條、又幾乎不可能同時首次呼叫，所以測不出問題；
//   下一段用柵欄放大競爭視窗，讓錯誤變成可觀察的事實。
//
// === 放大競爭視窗：16 條執行緒同時首次呼叫 ===
// UnsafeLazy 的建構函式被執行了 16 次（Singleton 應該只有 1 次）
// → 這個次數每次執行都不同，但通常【大於 1】；
//   多出來的物件全部洩漏，且部分執行緒拿到了不同的實例。
//
// === 正確版 a：Meyers Singleton（C++11 magic static）===
// 建構函式執行次數: 1 (必定為 1)
//
// === 正確版 b：std::call_once ===
// 建構函式執行次數: 1 (必定為 1)
//
// === 日常實務 1：連線池只建立一次 ===
// 16 條執行緒同時取用連線池
// 成功取得連線數: 5 (必定 = 5，池內就 5 條)
// → 連線池只被建立一次；但 acquire() 仍需自己的鎖，
//   因為 magic static 只保證初始化，不保證成員操作。
//
// === 日常實務 2：設定檔一次性載入（call_once）===
// 16 條執行緒同時讀設定
// load() 實際執行次數: 1 (必定為 1)
// db.host   = db.internal
// log.level = info
