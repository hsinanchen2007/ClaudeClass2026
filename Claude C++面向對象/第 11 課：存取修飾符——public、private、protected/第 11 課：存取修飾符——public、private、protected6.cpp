// =============================================================================
//  第 11 課 -6  —  綜合實戰：用 public/private 把「銀行帳戶」的不變量真正鎖住
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { public: /* 介面 */ private: /* 狀態與輔助函式 */ };
//   標準：  C++98 起
//   標頭檔：本例需 <iostream>、<string>、<vector>
//   關鍵詞：encapsulation、class invariant、interface vs implementation、const correctness
//
//   這是本課的整合範例：把前面五個檔案的概念（public 介面、private 狀態、
//   private 輔助函式、const 正確性）合起來，做出一個「不變量真的守得住」的類別。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個類別要守住哪些不變量】
// 設計一個類別之前，先把不變量寫下來 —— 這決定了哪些成員必須 private：
//     (I1) balance >= 0               任何時刻餘額不可為負
//     (I2) transactionCount 等於實際發生過的異動次數（不可被外部竄改）
//     (I3) accountId 一旦設定就不再改變（帳號是身分，不是狀態）
//     (I4) 每一筆成功的異動都必須留下紀錄（稽核需求）
// 只要 balance 或 transactionCount 是 public，(I1)(I2)(I4) 立刻失守 ——
// 因為外界一行 acc.balance = -1; 就繞過了全部檢查。
//
// 【2. 介面（public）與實作（private）的分界線怎麼畫】
// 分界的判準是：「這件事是使用者需要知道的『能力』，還是我內部怎麼做到的『步驟』？」
//   * deposit / withdraw / display / getBalance → 能力，放 public
//   * addTransaction（記錄稽核軌跡）           → 步驟，放 private
// addTransaction 之所以必須 private，是因為它會遞增 transactionCount。
// 若它是 public，外界就能憑空製造假的交易紀錄，(I2)(I4) 立刻被破壞。
// 換句話說：**任何會改變 private 狀態的函式，本身就必須被納入管制**。
//
// 【3. 為什麼查詢函式要加 const】
// 原始版本的 getBalance()/getOwner()/display() 都沒有 const，這在實務上會出事：
//     void printReceipt(const BankAccount& acc) { acc.display(); }   // ❌ 編譯失敗
// 只要有人想用 const reference 傳遞帳戶（這是傳遞大型物件的標準做法，避免複製），
// 所有非 const 的成員函式都不能呼叫。本檔已把所有唯讀函式補上 const。
// 這就是 const correctness：唯讀就標 const，讓編譯器幫你把關，也讓介面能被 const 使用。
//
// 【4. 回傳型別的選擇：值 vs const reference】
// getOwner() 回傳 string（複製一份）還是 const string&（不複製）？
//   * 回傳 const string&：零複製，效能好；但呼叫端若保存這個 reference，
//     而帳戶物件先被銷毀，就成了 dangling reference。
//   * 回傳 string：安全，但每次呼叫都複製。
// 本檔選 const string&（標準做法），並在註解提醒生命週期。
// 關鍵是：絕不要回傳「非 const」的 reference 到 private 成員 ——
//     double& balanceRef() { return balance; }   // ❌ 等於把 private 開成 public
// 那會讓外界拿到可寫的通道，封裝瞬間歸零。
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼用 init() 而不是建構函式（本課的暫時性妥協）
//     本課還沒教到建構函式（第 13 課），所以用 init() 代替。
//     但要清楚知道這是有缺陷的：物件在 init() 被呼叫「之前」處於未初始化的
//     半成品狀態，而編譯器不會阻止你先呼叫 deposit()。
//     建構函式的價值正是消滅這個空窗期 —— 物件一誕生就滿足不變量。
//     本檔用 m_initialized 旗標把這個空窗期擋起來，示範「沒有建構函式時
//     要多寫多少防禦程式碼」，也順帶說明為什麼建構函式是必要的。
//
// (B) private 成員函式不參與「介面」，可以自由重構
//     addTransaction 的簽名、實作、甚至存在與否，都可以隨時改，
//     因為沒有任何外部程式碼能呼叫它。這正是「把步驟藏起來」的收益：
//     未來要改成寫進資料庫、送到 message queue，都不會影響呼叫端。
//
// (C) 存取權不影響物件大小
//     把成員從 public 移到 private 不會改變 sizeof。access specifier 是
//     編譯期的名稱存取規則，不產生執行期程式碼。本檔會印出 sizeof 佐證，
//     該數值為實作定義（取決於編譯器、平台、std::string 的實作與對齊）。
//
// 【注意事項 Pay Attention】
// 1. 任何會改動 private 狀態的函式（如 addTransaction）本身也必須 private。
// 2. 唯讀成員函式一律加 const，否則 const reference 無法呼叫它們。
// 3. 絕不回傳非 const 的 reference 或指標指向 private 成員 —— 那等於解除封裝。
// 4. 回傳 const reference 效能好，但呼叫端不可讓它活得比物件久（dangling）。
// 5. 用 init() 取代建構函式會留下「未初始化空窗期」，需要額外旗標防禦；
//    這是第 13 課要用建構函式解決的問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝實戰
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. addTransaction() 為什麼必須是 private？把它設成 public 會怎樣？
//     答：它會遞增 transactionCount 並輸出稽核紀錄。設成 public，外界就能
//         憑空製造假交易、竄改交易次數，「交易次數等於實際異動次數」這條
//         不變量立刻失守。原則：任何會改動 private 狀態的函式，本身就必須受管制。
//     追問：那要怎麼判斷一個函式該 public 還是 private？
//         → 問它是「使用者需要的能力」還是「我內部的實作步驟」。
//           能力放 public，步驟放 private；步驟之後可以自由重構。
//
// 🔥 Q2. 為什麼 getBalance() 要加 const？
//     答：因為只有 const 成員函式能被 const 物件／const reference 呼叫。
//         而傳遞大型物件的標準做法就是 const reference（避免複製）。
//         `void audit(const BankAccount& a) { a.getBalance(); }`
//         若 getBalance() 沒有 const，這個函式根本編譯不過。
//     追問：const 成員函式能不能修改成員？
//         → 一般不行；但宣告為 mutable 的成員可以（常用於 cache、mutex 這種
//           「邏輯上不算物件狀態」的欄位）。
//
// ⚠️ 陷阱. 「我把 balance 設成 private，再提供 double& getBalanceRef() 讓呼叫端方便修改」——問題在哪？
//     答：回傳非 const 的 reference 等於把 private 成員完全公開，而且更隱蔽。
//         呼叫端拿到 double& 之後可以寫 ref = -999999;，
//         繞過所有檢查與稽核 —— 封裝被完全解除，但程式碼表面看起來還是「有 getter」。
//     為什麼會錯：多數人把封裝理解成「成員有沒有標 private」這個語法特徵，
//         而不是「有沒有可寫的通道流出去」這個實質問題。
//         真正該檢查的是：所有 public 成員函式的回傳型別裡，
//         有沒有非 const 的 reference／指標指向內部狀態。有的話，private 就白設了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class BankAccount {
public:
    // ========== 對外介面（public）：使用者需要的「能力」 ==========

    // 建立帳戶（目前用普通函數，第 13 課會改用建構函數）
    void init(const string& ownerName, const string& accId, double initialBalance) {
        if (m_initialized) {                       // (I3) 帳號是身分，不可重設
            cout << "警告：帳戶已初始化，拒絕重複 init" << endl;
            return;
        }
        owner = ownerName;
        accountId = accId;
        if (initialBalance >= 0) {
            balance = initialBalance;
        } else {
            balance = 0;                            // (I1) 餘額不可為負
            cout << "警告：初始餘額不能為負，已設為 0" << endl;
        }
        m_initialized = true;
    }

    // 存款
    bool deposit(double amount) {
        if (!ready()) return false;
        if (amount <= 0) {
            cout << "錯誤：存款金額必須大於 0" << endl;
            return false;
        }
        balance += amount;
        addTransaction("存款", amount);             // (I4) 成功才記錄
        return true;
    }

    // 提款
    bool withdraw(double amount) {
        if (!ready()) return false;
        if (amount <= 0) {
            cout << "錯誤：提款金額必須大於 0" << endl;
            return false;
        }
        if (amount > balance) {                     // (I1) 守住「不可為負」
            cout << "錯誤：餘額不足（目前: $" << balance << "）" << endl;
            return false;
        }
        balance -= amount;
        addTransaction("提款", amount);
        return true;
    }

    // ---- 查詢（只讀操作）：全部加 const，才能被 const reference 呼叫 ----
    double getBalance() const { return balance; }

    // 回傳 const reference：零複製。呼叫端不可讓它活得比本物件久。
    const string& getOwner()     const { return owner; }
    const string& getAccountId() const { return accountId; }
    int getTransactionCount()    const { return transactionCount; }

    // 稽核軌跡也只給唯讀視圖 —— 回傳 const reference，外界改不了
    const vector<string>& history() const { return m_history; }

    // 顯示帳戶資訊
    void display() const {
        cout << "================================" << endl;
        cout << "帳戶持有人: " << owner << endl;
        cout << "帳號:       " << accountId << endl;
        cout << "餘額:       $" << balance << endl;
        cout << "交易次數:   " << transactionCount << endl;
        cout << "================================" << endl;
    }

private:
    // ========== 內部資料（外界不能直接碰）==========
    string owner = "";
    string accountId = "";
    double balance = 0.0;
    int transactionCount = 0;
    bool m_initialized = false;          // 補住「init 之前」的空窗期
    vector<string> m_history;            // (I4) 稽核軌跡

    // ========== 內部輔助函數 ==========
    // 必須 private：它會遞增 transactionCount，開放出去等於允許偽造交易紀錄
    void addTransaction(const string& type, double amount) {
        transactionCount++;
        cout << "[交易 #" << transactionCount << "] "
             << type << " $" << amount
             << " → 餘額: $" << balance << endl;
        m_history.push_back(type + " $" + to_string(static_cast<long>(amount)));
    }

    // private 守門函式：沒 init 就不准動帳
    bool ready() const {
        if (!m_initialized) {
            cout << "錯誤：帳戶尚未初始化，操作被拒絕" << endl;
            return false;
        }
        return true;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2043. Simple Bank System
//   題目：銀行有 n 個帳戶（編號 1..n），實作 transfer / deposit / withdraw。
//         交易有效的條件是：帳號在 1..n 範圍內，且提出／轉出金額不超過餘額。
//         無效交易回傳 false 且不得改變任何餘額。
//   為什麼用到本主題：這題本質就是本課在講的事 —— 把 balance 陣列設成 private，
//         把「帳號合法」「餘額足夠」兩條不變量鎖在成員函式裡。
//         若 balance 是 public，呼叫端可以直接寫成負數，這題的驗證邏輯就形同虛設。
//         注意 valid() 是 private：它是內部檢查步驟，不是對外能力。
//   複雜度：三個操作皆為 O(1)。餘額用 long long 以免大額累加溢位。
// -----------------------------------------------------------------------------
class Bank {
private:
    vector<long long> m_balance;         // private：唯一的真相來源

    // private 輔助函式：帳號合法性檢查（實作細節，不對外）
    bool valid(int account) const {
        return account >= 1 && account <= static_cast<int>(m_balance.size());
    }

public:
    explicit Bank(vector<long long> balance) : m_balance(std::move(balance)) {}

    bool transfer(int account1, int account2, long long money) {
        if (!valid(account1) || !valid(account2)) return false;
        if (m_balance[account1 - 1] < money) return false;
        m_balance[account1 - 1] -= money;
        m_balance[account2 - 1] += money;
        return true;
    }

    bool deposit(int account, long long money) {
        if (!valid(account)) return false;
        m_balance[account - 1] += money;
        return true;
    }

    bool withdraw(int account, long long money) {
        if (!valid(account)) return false;
        if (m_balance[account - 1] < money) return false;
        m_balance[account - 1] -= money;
        return true;
    }

    // 只給唯讀視圖，外界無法直接改餘額
    const vector<long long>& balances() const { return m_balance; }
};

// 只接受 const reference 的稽核函式：
// 這證明 display()/getBalance() 必須是 const，否則此函式編譯不過。
void auditReport(const BankAccount& acc) {
    cout << "  [稽核] " << acc.getOwner() << " / " << acc.getAccountId()
         << " 餘額 $" << acc.getBalance()
         << "，共 " << acc.getTransactionCount() << " 筆異動" << endl;
}

int main() {
    cout << "=== 未初始化就操作：被守門函式擋下 ===" << endl;
    BankAccount fresh;
    fresh.deposit(100);          // 尚未 init，拒絕

    cout << "\n=== 正常流程 ===" << endl;
    BankAccount acc;
    acc.init("陳信安", "ACC-001", 5000.0);
    acc.display();

    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(10000);   // 餘額不足
    acc.deposit(-500);     // 無效金額

    cout << endl;
    acc.display();

    cout << "\n=== 不變量守住了：帳號不可重設 ===" << endl;
    acc.init("駭客", "ACC-999", 0);   // 拒絕
    cout << "  帳號仍為： " << acc.getAccountId() << endl;

    cout << "\n=== const 正確性：const reference 只能呼叫 const 成員函式 ===" << endl;
    auditReport(acc);

    cout << "\n=== 稽核軌跡（唯讀視圖） ===" << endl;
    for (const string& h : acc.history()) {
        cout << "  - " << h << endl;
    }

    cout << "\n=== 安全地查詢 ===" << endl;
    cout << "  查詢餘額: $" << acc.getBalance() << endl;

    // 以下操作全部會被編譯器攔截：
    // acc.balance = 999999;         // ❌ private！
    // acc.owner = "";               // ❌ private！
    // acc.transactionCount = 0;     // ❌ private！
    // acc.addTransaction("偷", 1);  // ❌ private 函數！
    cout << "  （acc.balance = 999999; 等四種寫法都會編譯失敗）" << endl;

    cout << "\n=== LeetCode 2043. Simple Bank System ===" << endl;
    Bank bank({10, 100, 20, 50, 30});
    cout << "  初始餘額: [10, 100, 20, 50, 30]" << endl;
    cout << "  withdraw(3, 10)      = " << boolalpha << bank.withdraw(3, 10) << endl;
    cout << "  transfer(5, 1, 20)   = " << bank.transfer(5, 1, 20) << endl;
    cout << "  deposit(5, 20)       = " << bank.deposit(5, 20) << endl;
    cout << "  transfer(3, 4, 15)   = " << bank.transfer(3, 4, 15)
         << "  （帳戶3餘額只剩10，不足）" << endl;
    cout << "  withdraw(10, 50)     = " << bank.withdraw(10, 50)
         << "  （帳號10不存在）" << endl;
    cout << "  最終餘額: [";
    const vector<long long>& b = bank.balances();
    for (size_t i = 0; i < b.size(); ++i) cout << (i ? ", " : "") << b[i];
    cout << "]" << endl;

    cout << "\n=== 存取權不影響物件大小（數值為實作定義） ===" << endl;
    cout << "  sizeof(BankAccount) = " << sizeof(BankAccount) << " bytes" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected6.cpp" -o access6

// === 預期輸出 ===
// === 未初始化就操作：被守門函式擋下 ===
// 錯誤：帳戶尚未初始化，操作被拒絕
//
// === 正常流程 ===
// ================================
// 帳戶持有人: 陳信安
// 帳號:       ACC-001
// 餘額:       $5000
// 交易次數:   0
// ================================
// [交易 #1] 存款 $2000 → 餘額: $7000
// [交易 #2] 提款 $1500 → 餘額: $5500
// 錯誤：餘額不足（目前: $5500）
// 錯誤：存款金額必須大於 0
//
// ================================
// 帳戶持有人: 陳信安
// 帳號:       ACC-001
// 餘額:       $5500
// 交易次數:   2
// ================================
//
// === 不變量守住了：帳號不可重設 ===
// 警告：帳戶已初始化，拒絕重複 init
//   帳號仍為： ACC-001
//
// === const 正確性：const reference 只能呼叫 const 成員函式 ===
//   [稽核] 陳信安 / ACC-001 餘額 $5500，共 2 筆異動
//
// === 稽核軌跡（唯讀視圖） ===
//   - 存款 $2000
//   - 提款 $1500
//
// === 安全地查詢 ===
//   查詢餘額: $5500
//   （acc.balance = 999999; 等四種寫法都會編譯失敗）
//
// === LeetCode 2043. Simple Bank System ===
//   初始餘額: [10, 100, 20, 50, 30]
//   withdraw(3, 10)      = true
//   transfer(5, 1, 20)   = true
//   deposit(5, 20)       = true
//   transfer(3, 4, 15)   = false  （帳戶3餘額只剩10，不足）
//   withdraw(10, 50)     = false  （帳號10不存在）
//   最終餘額: [30, 100, 10, 50, 30]
//
// === 存取權不影響物件大小（數值為實作定義） ===
//   sizeof(BankAccount) = 104 bytes
