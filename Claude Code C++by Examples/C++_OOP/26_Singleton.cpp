/*=============================================================================
 * 檔名：26_Singleton.cpp
 * 主題：Singleton 單例模式 - 整個程式只有「一個」物件
 * 適合：學完 static 與 RAII，想了解第一個正式設計模式的人
 *
 * 【課題介紹】
 *   有些東西，整個程式從頭到尾「只該存在一份」：
 *
 *     - Logger 紀錄器：所有模組寫到同一個檔案
 *     - 應用程式的 Config：全部模組讀同一份設定
 *     - 連線池、執行緒池、ID 產生器
 *
 *   如果讓每次想用就 new 一個出來，會：
 *     - 設定不同步、互相覆寫紀錄
 *     - 重複連線、浪費資源
 *
 *   解法：Singleton 設計模式 (單例模式)。
 *
 *       「Singleton 是一個類別，保證『程式中只能有一個實例 (instance)』，
 *        並提供一個全域可存取的取得介面，例如 Logger::instance()。」
 *
 * 【關鍵實作要點】
 *   為了強迫「只能有一個」，我們：
 *     1. 建構子設成 private  → 外人不能 new
 *     2. 複製建構/賦值 = delete → 不能複製
 *     3. 提供一個 static 的 getInstance() / instance() 函式對外
 *
 * 【現代 C++11 之後最簡單的寫法：Meyer's Singleton】
 *
 *       class Logger {
 *       public:
 *           static Logger& instance() {
 *               static Logger inst;     // 函式內 static 區域變數
 *               return inst;
 *           }
 *       private:
 *           Logger() {}                  // 建構子 private
 *           Logger(const Logger&) = delete;
 *           Logger& operator=(const Logger&) = delete;
 *       };
 *
 *   重點：函式內的 static 區域變數 — C++11 起標準保證它的初始化是
 *   「執行緒安全 (thread-safe)」、「lazy (第一次被呼叫到才建構)」、「只建一次」。
 *
 *   這就是俗稱的 Meyer's Singleton (Scott Meyers 推廣的寫法)，三行解決所有問題。
 *
 * 【為什麼 instance() 回傳「Logger&」而不是 Logger*？】
 *   - 用 reference 表示「絕不會是 null」，呼叫者用起來更安全。
 *   - 避免使用者寫 delete logger; 之類的災難。
 *
 * 【何時不該用 Singleton？】
 *   - 它本質上是「美化過的全域變數」，過度使用會讓單元測試 (unit test)
 *     非常難寫 (因為狀態跨測試保留)。
 *   - 若該物件的需求不嚴格「全程式唯一」，傳參數 / DI (依賴注入) 通常更乾淨。
 *
 * 【日常實用範例】
 *   做一個 Logger 寫到 stdout，並支援等級 (INFO/WARN/ERROR)。
 *
 * 【對應 Leetcode】1603. Design Parking System
 *   題目簡述：設計一個停車場系統，停三種車 (big/medium/small) 各有上限車位數。
 *           addCar(carType) → 若還有空位回 true 並停車，否則回 false。
 *   為什麼選這題：整個程式同一個停車場，是非常自然的 Singleton 場景；
 *   不同模組 (例如不同入口柵欄、計費模組) 都呼叫同一個 ParkingSystem。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/storage_duration   (static)
 *   https://cplusplus.com/doc/tutorial/classes/
 *=============================================================================*/

/*
補充筆記：Singleton
  - Singleton 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - Singleton 保證某型別只有一個全域可取得的實例；常見寫法是 function local static instance。
  - C++11 起 function local static 的初始化是 thread-safe，因此 static Singleton& instance(){ static Singleton s; return s; } 是常見安全基礎。
  - Singleton 建構子通常設 private，並刪除 copy/move，避免外部建立或複製第二個實例。
  - Singleton 的缺點是隱藏依賴；函式看起來沒有參數，實際上卻依賴全域狀態，測試和重用會變困難。
  - 若物件需要可替換、可 mock、可控制生命週期，dependency injection 往往比 Singleton 更好。
  - Singleton 的解構時機在程式結束階段，跨 translation unit 的 static 物件解構順序可能造成問題。
  - 多執行緒下 Singleton 內部狀態仍需要同步；thread-safe 初始化不代表之後所有操作都 thread-safe。
  - 只有在確定概念上真的只有一個，例如 process-wide logger 或 registry，且接受全域狀態代價時，才考慮 Singleton。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Singleton 單例模式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 如何實作 thread-safe 的 Singleton？
//     答：C++11 起最簡潔的是 **Meyers Singleton**：函式內 `static Singleton inst;`
//         —— 標準保證 function-local static 的初始化是「執行緒安全且只執行一次」
//         （俗稱 magic static），而且是 lazy 的，第一次呼叫才建構。再把建構子設
//         private、copy / move 全部 `= delete`（本檔 Logger 正是這個範式）。
//     追問：那還需要 double-checked locking 嗎？（不需要。DCLP 在 C++11 之前是
//           壞的、需要 memory barrier；有了 magic static 就沒有理由再手寫它）
//
// 🔥 Q2. `instance()` 為什麼回傳 reference 而不是 pointer？
//     答：reference 表達「絕不會是 null」，呼叫端不必檢查；也避免使用者寫出
//         `delete logger;` 這種災難（物件的生命週期由 static storage duration 管，
//         不該由呼叫端釋放）。
//
// Q3. Singleton 的缺點是什麼？
//     答：① 本質是「美化過的全域變數」，隱藏依賴 —— 函式看起來沒有參數，其實依賴
//         全域狀態，單元測試很難注入替身、狀態還會跨測試殘留。② destruction order
//         fiasco —— 跨 translation unit 的 static 物件解構順序無法保證，若某個
//         Singleton 在解構時去用另一個已被解構的 Singleton 就是 UB。
//     追問：替代方案？（依賴注入 —— 把物件當參數傳進去，可替換、可 mock、生命週期明確）
//
// ⚠️ 陷阱. Meyers Singleton 是 thread-safe 的，所以 `Logger::instance().log(...)`
//          也是 thread-safe 的吧？
//     答：不是。標準保證的只有「初始化」執行緒安全且只做一次；至於之後對這個實例做的
//         任何操作，仍然要自己同步。本檔 Logger::log() 內部特地放了
//         `std::lock_guard<std::mutex>` 正是為了這件事。
//     為什麼會錯：多數人把「thread-safe initialization」直接理解成「這個物件
//         thread-safe」。兩者是完全不同層次的保證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <mutex>      // 演示「執行緒安全」用，雖然 Meyer's 已經自帶執行緒安全
#include <vector>
#include <utility>

class Logger {
public:
    enum class Level { Info, Warn, Error };

    // 取得唯一實例
    static Logger& instance() {
        static Logger inst;     // 第一次呼叫才初始化、之後重複呼叫都回同一個
        return inst;
    }

    void log(Level lvl, const std::string& msg) {
        std::lock_guard<std::mutex> lk(mu_);    // 多執行緒同時 log 也安全
        std::cout << "[" << levelStr(lvl) << "] " << msg << std::endl;
        ++count_;
    }

    int count() const { return count_; }

private:
    std::mutex mu_;
    int        count_ = 0;

    // 建構子 / 解構子設為 private，讓外人不能直接建立或銷毀
    Logger() { std::cout << "(Logger 建立 — 一輩子只會看到這行一次)\n"; }
    ~Logger() { std::cout << "(Logger 銷毀)\n"; }

    // 禁止複製與移動
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&)      = delete;

    static const char* levelStr(Level l) {
        switch (l) {
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
        }
        return "?";
    }
};

// 不同模組各自呼叫 Logger，看是否都拿到同一個實例
void moduleA() {
    Logger::instance().log(Logger::Level::Info, "moduleA 啟動");
}
void moduleB() {
    Logger::instance().log(Logger::Level::Warn, "moduleB 警告");
}

// -----------------------------------------------------------------------------
// 範例：對應 Leetcode 359 - Logger Rate Limiter (Singleton 版)
// -----------------------------------------------------------------------------
// 整個程式同一個 rate limiter，所有模組都呼叫同一個實例 → 典型 Singleton 場景。
class RateLimiter {
public:
    static RateLimiter& instance() {
        static RateLimiter inst;
        return inst;
    }
    bool shouldPrintMessage(int timestamp, const std::string& message) {
        // 簡化：用 vector<pair> 紀錄；正式應用會用 unordered_map
        for (auto& p : recent_) {
            if (p.first == message) {
                if (timestamp - p.second < 10) return false;
                p.second = timestamp;
                return true;
            }
        }
        recent_.push_back({message, timestamp});
        return true;
    }
private:
    RateLimiter() = default;
    RateLimiter(const RateLimiter&)            = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;
    std::vector<std::pair<std::string, int>> recent_;
};

// -----------------------------------------------------------------------------
// 範例：日常實用 - SimpleConfig Singleton 設定中心
// -----------------------------------------------------------------------------
// 整個程式只有「一份」設定，所有模組讀同一份。
class SimpleConfig {
public:
    static SimpleConfig& instance() {
        static SimpleConfig inst;
        return inst;
    }
    void setEnv(const std::string& env) { env_ = env; }
    const std::string& env() const { return env_; }
    int dbPort() const { return env_ == "prod" ? 5432 : 15432; }
private:
    SimpleConfig() : env_("dev") {}
    SimpleConfig(const SimpleConfig&)            = delete;
    SimpleConfig& operator=(const SimpleConfig&) = delete;
    std::string env_;
};

int main() {
    std::cout << "===== 啟動程式 =====" << std::endl;
    moduleA();
    moduleB();

    Logger& lg = Logger::instance();
    lg.log(Logger::Level::Error, "main 直接拿 logger 用");

    // 確認是同一個實例：位址比對
    Logger& lg2 = Logger::instance();
    std::cout << "lg & lg2 同一個? " << (&lg == &lg2 ? "yes" : "no") << "\n";
    std::cout << "目前已記錄 " << lg.count() << " 條訊息\n";

    // Logger l;             // ← 編譯錯誤：建構子 private
    // Logger l = lg;        // ← 編譯錯誤：複製被 delete

    std::cout << "===== Leetcode 1603 - Design Parking System (Singleton) =====" << std::endl;
    // 用同樣的 Meyer's Singleton 範式做停車場系統。
    // 「configure」一次設定容量，之後不同入口模組共用同一個 instance。
    class ParkingSystem {
    public:
        static ParkingSystem& instance() {
            static ParkingSystem inst;
            return inst;
        }
        // 依照 LC 1603：carType ∈ {1=big, 2=medium, 3=small}
        void configure(int big, int medium, int small) {
            slots_[0] = big; slots_[1] = medium; slots_[2] = small;
        }
        bool addCar(int carType) {
            int idx = carType - 1;     // 1/2/3 → 0/1/2
            if (slots_[idx] <= 0) return false;
            --slots_[idx];
            return true;
        }
    private:
        ParkingSystem() = default;
        int slots_[3] = {0, 0, 0};
    };
    ParkingSystem::instance().configure(1, 1, 0);     // big=1, medium=1, small=0
    std::cout << "addCar(big)    = " << ParkingSystem::instance().addCar(1) << "  (預期 1)\n";
    std::cout << "addCar(medium) = " << ParkingSystem::instance().addCar(2) << "  (預期 1)\n";
    std::cout << "addCar(big)    = " << ParkingSystem::instance().addCar(1) << "  (預期 0)\n";
    std::cout << "addCar(small)  = " << ParkingSystem::instance().addCar(3) << "  (預期 0)\n";

    std::cout << "===== Leetcode 359 - RateLimiter Singleton =====" << std::endl;
    auto& rl = RateLimiter::instance();
    std::cout << rl.shouldPrintMessage(1,  "foo") << std::endl;   // 1
    std::cout << rl.shouldPrintMessage(2,  "foo") << std::endl;   // 0 (太頻繁)
    std::cout << rl.shouldPrintMessage(11, "foo") << std::endl;   // 1

    std::cout << "===== SimpleConfig Singleton =====" << std::endl;
    SimpleConfig::instance().setEnv("prod");
    std::cout << "env = " << SimpleConfig::instance().env()
              << ", dbPort = " << SimpleConfig::instance().dbPort() << std::endl;
    return 0;
}

/* 預期輸出（順序固定，最後一行 ~Logger 在程式結束時才出現）：
 * ===== 啟動程式 =====
 * (Logger 建立 — 一輩子只會看到這行一次)
 * [INFO] moduleA 啟動
 * [WARN] moduleB 警告
 * [ERROR] main 直接拿 logger 用
 * lg & lg2 同一個? yes
 * 目前已記錄 3 條訊息
 * ===== Leetcode 1603 - Design Parking System (Singleton) =====
 * addCar(big)    = 1  (預期 1)
 * addCar(medium) = 1  (預期 1)
 * addCar(big)    = 0  (預期 0)
 * addCar(small)  = 0  (預期 0)
 * ===== Leetcode 359 - RateLimiter Singleton =====
 * 1
 * 0
 * 1
 * ===== SimpleConfig Singleton =====
 * env = prod, dbPort = 5432
 * (Logger 銷毀)
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. Singleton 強制全域唯一：建構子 private + 複製/移動 = delete + static instance()。
 *   2. Meyer's Singleton (函式內 static) 是 C++11 起最簡潔且執行緒安全的寫法。
 *   3. instance() 回傳 reference 較佳，避免 null 與誤 delete。
 *   4. Singleton 是「美化的全域變數」，會讓測試變難，能不用就不用。
 *
 * 【下一篇預告】
 *   27_Factory.cpp
 *   工廠模式 (Factory) — 把「決定建立哪個子類別」的邏輯集中一處，
 *   延伸自第 17 篇的 Shape 範例。
 *=============================================================================*/
