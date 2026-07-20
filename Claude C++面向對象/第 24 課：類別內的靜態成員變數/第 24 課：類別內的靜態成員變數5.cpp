// =============================================================================
//  第 24 課：類別內的靜態成員變數 5  —  私有靜態成員與存取控制
// =============================================================================
//
// 【主題資訊 Information】
//   語法:
//     class C {
//         inline static T data_ = ...;      // private：外部不可直接碰
//     public:
//         static T  get()      { return data_; }        // 唯讀出口
//         static void set(T v) { /* 驗證後才寫入 */ }   // 受控寫入口
//     };
//   標準版本: private static 為 C++98；inline static 就地初始化為 C++17
//   複雜度: 存取 O(1)，不經 this
//   標頭檔: <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 存取控制與 static 是兩個正交的概念】
//   很多人以為「static 就是公開的全域變數」，這是混淆。
//     * static  決定「有幾份」——整個程式一份。
//     * public/private 決定「誰能碰」——與份數無關。
//   所以 private static 完全合法，而且是實務上的預設選擇：
//   資料只有一份，但仍然只有類別自己能改。
//   本檔的 totalDeposits_ / accountCount_ / interestRate_ 就是這種設計。
//
// 【2. 為什麼一定要包一層 getter/setter】
//   如果 interestRate_ 是 public，任何一行程式碼都能寫
//       BankAccount::interestRate_ = 3.0;   // 300% 利率
//   而且出事時無從追查是誰改的。包成 setInterestRate 之後：
//     * 可以驗證（本檔擋掉 0%~20% 之外的值）
//     * 可以記錄（要稽核就在函式裡加 log）
//     * 可以改實作（日後改成查資料庫，呼叫端完全不用動）
//   這就是封裝的實際價值：不是為了「藏起來」，是為了「保留改動的自由」。
//
// 【3. 靜態資料 = 所有物件共用的政策】
//   interestRate_ 是「銀行的政策」，不是「某個帳戶的屬性」，
//   所以它天然屬於類別而非物件。判斷準則很簡單：
//   如果這個值「對每個物件都應該一樣」，它就該是 static。
//   本檔調整一次利率，三個帳戶的利息全部改變 —— 這正是共用一份的證據。
//
// 【4. static 成員函式為什麼能存取 private static 成員】
//   存取控制是「類別層級」而非「物件層級」：
//   同一個類別的成員函式（包含 static 成員函式）就是可以碰自己的 private。
//   static 成員函式沒有 this，因此只能存取 static 成員 ——
//   這也是為什麼 getAccountCount() 能讀 accountCount_，
//   卻不能讀 balance_（那需要知道是「哪一個」帳戶）。
//
// 【概念補充 Concept Deep Dive】
//   * private static 成員仍然佔用靜態儲存區，只是名字在類別外不可見。
//     它並沒有比 public static「更安全」於多執行緒 —— 存取控制是編譯期概念，
//     跟資料競爭完全無關。要執行緒安全仍需 std::atomic 或 mutex。
//   * 本檔的 totalDeposits_ 用 double 累加金額，是教學簡化。
//     實務上金額不該用浮點數：0.1 在二進位無法精確表示，
//     大量加減會累積誤差。正式系統用整數的「最小幣值單位」（分）
//     或 decimal 型別。
//   * setInterestRate 的驗證失敗只印訊息、不回報狀態，
//     呼叫端無從得知成功與否。實務上應回傳 bool 或丟例外。
//     本檔保留原樣是為了聚焦在存取控制主題。
//
// 【注意事項 Pay Attention】
//   1. private 只擋「編譯期的名字存取」，擋不住多執行緒的資料競爭。
//   2. 靜態成員一改，所有物件同時受影響 —— 這是特性也是風險。
//   3. 金額不要用 double；本檔僅為教學。
//   4. 驗證失敗要讓呼叫端知道（回傳值或例外），只印 log 會被忽略。
//   5. 靜態狀態會跨單元測試殘留，測試前需重置或改用可注入的設定物件。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】私有靜態成員與存取控制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. static 成員可以是 private 嗎?這樣做的意義是什麼?
//     答：可以。static 決定「有幾份」，private 決定「誰能存取」，
//         兩者正交。private static 是實務預設：資料整個程式一份，
//         但只能透過類別自己提供的介面修改，
//         因此可以加驗證、加稽核 log，日後也能換掉底層實作。
//     追問：那要怎麼從外面讀它?
//         → 提供 public static getter（本檔的 getAccountCount()）。
//         static 成員函式沒有 this，所以只能碰 static 成員。
//
// 🔥 Q2. 為什麼 static 成員函式能存取 private static 資料，
//        但不能存取一般成員變數?
//     答：存取控制是類別層級的，同類別的成員函式本來就能碰自己的 private。
//         但 static 成員函式沒有 this 指標，
//         而一般成員變數必須先知道「是哪一個物件」才找得到，
//         所以編譯期就會直接報錯。
//     追問：那 static 成員函式收一個物件參數就能存取了嗎?
//         → 可以。static bool cmp(const A& x, const A& y) 這種寫法很常見，
//         此時物件是明確傳進來的，不需要 this。
//
// ⚠️ 陷阱. 「把靜態成員設成 private 就變執行緒安全了。」
//     答：完全無關。private 是編譯期的名字可見性檢查，
//         編譯完成後不存在任何執行期檢查。
//         兩條執行緒同時呼叫 deposit()，totalDeposits_ += amount
//         仍然是非原子的讀-改-寫，一樣是資料競爭（未定義行為）。
//     為什麼會錯：把「別人不能亂碰」直覺推論成「不會同時碰」。
//         但 private 限制的是「哪段程式碼能寫這個名字」，
//         不是「同一時間有幾條執行緒在執行那段程式碼」。
//         要執行緒安全必須用 std::atomic<double> 或在 setter 內加鎖。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner_;
    double balance_;

    // 私有靜態成員：外部不能直接訪問
    // 這些靜態成員用於追蹤銀行的總存款、帳戶數量和利率
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    inline static double totalDeposits_ = 0.0;
    inline static int accountCount_ = 0;
    inline static double interestRate_ = 0.03;   // 3% 利率

public:
    BankAccount(const string& owner, double initial)
        : owner_(owner), balance_(initial > 0 ? initial : 0)
    {
        accountCount_++;
        totalDeposits_ += balance_;
        cout << "  [開戶] " << owner_ << " 存入 " << balance_ << endl;
    }

    void deposit(double amount) {
        if (amount <= 0) return;
        balance_ += amount;
        totalDeposits_ += amount;
    }

    double getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }

    // 計算利息——使用靜態的利率
    double calculateInterest() const {
        return balance_ * interestRate_;
    }

    // 公開靜態資訊的 getter（通過 public 函數控制訪問）
    static int getAccountCount() { return accountCount_; }
    static double getTotalDeposits() { return totalDeposits_; }
    static double getInterestRate() { return interestRate_; }

    // 設定利率（帶驗證）
    static void setInterestRate(double rate) {
        if (rate < 0 || rate > 0.20) {
            cout << "  利率必須在 0%~20% 之間！" << endl;
            return;
        }
        interestRate_ = rate;
        cout << "  利率已調整為 " << (rate * 100) << "%" << endl;
    }

    void printStatement() const {
        cout << "  " << owner_ << "：餘額 " << balance_
             << "，利息 " << calculateInterest() << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目：停車場有大/中/小三種車位，各有固定數量；
//         addCar(carType) 有位就停進去回傳 true，滿了回傳 false。
//   為什麼用到本主題：這題的核心與本檔完全同構 ——
//         「把計數封裝在類別內，只能透過驗證過的介面修改」。
//         直接暴露 public 計數器就能被外部改成負數，正是本檔要避免的事。
//   誠實說明：1603 的配額屬於「單一停車場物件」，所以用一般成員變數才正確；
//         只有當配額是「所有物件共用的政策」（像本檔的利率）時才該用 static。
//         這個區分本身就是本課的重點，故一併示範。
// -----------------------------------------------------------------------------
class ParkingSystem {
private:
    // 每個停車場物件各一份 —— 這裡刻意不是 static
    int slots_[3];

public:
    ParkingSystem(int big, int medium, int small) : slots_{big, medium, small} {}

    bool addCar(int carType) {
        int idx = carType - 1;            // carType 為 1/2/3
        if (idx < 0 || idx > 2) return false;
        if (slots_[idx] <= 0) return false;
        --slots_[idx];
        return true;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP client 的全域預設逾時設定
//   情境：整個服務共用一組連線預設值（連線逾時、重試次數）。
//         這是「政策」不是「某條連線的屬性」，所以用 private static 存，
//         再用經過驗證的 static setter 修改，避免有人設出 0 秒或負數重試。
//   關鍵：setter 回傳 bool 讓呼叫端知道有沒有生效 ——
//         這正是上面【概念補充】指出本檔 setInterestRate 的不足之處。
// -----------------------------------------------------------------------------
class HttpClientConfig {
private:
    inline static int connectTimeoutMs_ = 3000;
    inline static int maxRetries_       = 2;

public:
    static int connectTimeoutMs() { return connectTimeoutMs_; }
    static int maxRetries()       { return maxRetries_; }

    // 逾時必須落在 100ms ~ 60s；超出範圍視為設定錯誤，明確回報失敗
    static bool setConnectTimeoutMs(int ms) {
        if (ms < 100 || ms > 60000) return false;
        connectTimeoutMs_ = ms;
        return true;
    }

    // 重試次數 0~5；0 代表不重試，是合法設定
    static bool setMaxRetries(int n) {
        if (n < 0 || n > 5) return false;
        maxRetries_ = n;
        return true;
    }
};

int main() {
    cout << "=== 靜態成員與訪問控制 ===" << endl;

    cout << "\n--- 開戶 ---" << endl;
    BankAccount a1("陳信安", 10000);
    BankAccount a2("王小明", 5000);
    BankAccount a3("李大華", 20000);

    // 通過公開的靜態函數訪問私有靜態數據
    cout << "\n--- 銀行統計 ---" << endl;
    cout << "  帳戶數：" << BankAccount::getAccountCount() << endl;
    cout << "  總存款：" << BankAccount::getTotalDeposits() << endl;
    cout << "  當前利率：" << (BankAccount::getInterestRate() * 100) << "%" << endl;

    // 計算各帳戶利息
    cout << "\n--- 利息計算（利率 3%）---" << endl;
    a1.printStatement();
    a2.printStatement();
    a3.printStatement();

    // 調整利率——影響所有帳戶
    cout << "\n--- 調整利率 ---" << endl;
    BankAccount::setInterestRate(0.05);

    cout << "\n--- 利息計算（利率 5%）---" << endl;
    a1.printStatement();
    a2.printStatement();
    a3.printStatement();

    // 非法利率
    BankAccount::setInterestRate(0.50);   // 被攔截

    // 不能直接訪問私有靜態成員：
    // BankAccount::totalDeposits_ = 0;   // ❌ 編譯錯誤！private

    cout << "\n=== LeetCode 1603. Design Parking System ===" << endl;
    {
        ParkingSystem ps(1, 1, 0);        // 大 1、中 1、小 0
        cout << "  addCar(1) 大車 = " << boolalpha << ps.addCar(1) << endl;  // true
        cout << "  addCar(2) 中車 = " << ps.addCar(2) << endl;               // true
        cout << "  addCar(3) 小車 = " << ps.addCar(3) << endl;               // false（沒小車位）
        cout << "  addCar(1) 大車 = " << ps.addCar(1) << endl;               // false（已滿）
    }

    cout << "\n=== 日常實務：全域 HTTP 預設設定 ===" << endl;
    cout << "  預設連線逾時 = " << HttpClientConfig::connectTimeoutMs() << " ms" << endl;
    cout << "  預設重試次數 = " << HttpClientConfig::maxRetries() << endl;

    cout << "  設 5000ms  → " << boolalpha
         << HttpClientConfig::setConnectTimeoutMs(5000) << endl;
    cout << "  設 10ms    → " << HttpClientConfig::setConnectTimeoutMs(10)
         << "（低於下限，被擋下）" << endl;
    cout << "  設重試 0 次 → " << HttpClientConfig::setMaxRetries(0)
         << "（0 是合法值，代表不重試）" << endl;
    cout << "  設重試 9 次 → " << HttpClientConfig::setMaxRetries(9)
         << "（超過上限，被擋下）" << endl;

    cout << "  最終逾時 = " << HttpClientConfig::connectTimeoutMs()
         << " ms，重試 = " << HttpClientConfig::maxRetries() << endl;
    cout << "  ↑ 被擋下的設定沒有生效，且呼叫端從回傳值就知道失敗了。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第 24 課：類別內的靜態成員變數5.cpp -o static_member5

// === 預期輸出 ===
// === 靜態成員與訪問控制 ===
//
// --- 開戶 ---
//   [開戶] 陳信安 存入 10000
//   [開戶] 王小明 存入 5000
//   [開戶] 李大華 存入 20000
//
// --- 銀行統計 ---
//   帳戶數：3
//   總存款：35000
//   當前利率：3%
//
// --- 利息計算（利率 3%）---
//   陳信安：餘額 10000，利息 300
//   王小明：餘額 5000，利息 150
//   李大華：餘額 20000，利息 600
//
// --- 調整利率 ---
//   利率已調整為 5%
//
// --- 利息計算（利率 5%）---
//   陳信安：餘額 10000，利息 500
//   王小明：餘額 5000，利息 250
//   李大華：餘額 20000，利息 1000
//   利率必須在 0%~20% 之間！
//
// === LeetCode 1603. Design Parking System ===
//   addCar(1) 大車 = true
//   addCar(2) 中車 = true
//   addCar(3) 小車 = false
//   addCar(1) 大車 = false
//
// === 日常實務：全域 HTTP 預設設定 ===
//   預設連線逾時 = 3000 ms
//   預設重試次數 = 2
//   設 5000ms  → true
//   設 10ms    → false（低於下限，被擋下）
//   設重試 0 次 → true（0 是合法值，代表不重試）
//   設重試 9 次 → false（超過上限，被擋下）
//   最終逾時 = 5000 ms，重試 = 0
//   ↑ 被擋下的設定沒有生效，且呼叫端從回傳值就知道失敗了。
