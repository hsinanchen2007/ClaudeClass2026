// =============================================================================
//  第 11 課 -1  —  反面教材：全部 public 的類別，等於沒有類別
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { public: ... };
//   標準：  C++98 起即有 public / private / protected 三種 access specifier
//   標頭檔：本例僅需 <iostream>、<string>
//   關鍵詞：encapsulation（封裝）、invariant（不變量）、access specifier
//
//   本檔刻意示範「把所有成員開成 public」會發生什麼事。
//   ⚠️ 注意：本檔可以正常編譯、正常執行 —— 這正是問題所在。
//      編譯器不會阻止你寫出一個餘額為負的銀行帳戶，因為在語言層面它完全合法。
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼是 class invariant（類別不變量）】
// 「不變量」是指一個物件在「任何外部可觀察的時間點」都必須成立的條件。
// 以銀行帳戶為例，合理的不變量至少有：
//     (a) balance >= 0            —— 存款帳戶不該是負的
//     (b) owner 不可為空字串      —— 帳戶一定屬於某個人
//     (c) balance 的每次變動都要留下軌跡（稽核需求）
// 這三條都是「業務規則」，而不是 C++ 語法規則。編譯器不認識它們。
// 唯一能讓它們被強制執行的方法，是讓「修改狀態的唯一途徑」都經過你寫的函式。
//
// 而 public 資料成員，恰恰是把這條途徑徹底拆掉：
//     acc.balance = -999999.0;   // 沒有任何一行你寫的程式碼被執行
// 賦值運算子直接寫進物件的記憶體，沒有檢查點、沒有 log、沒有機會拒絕。
//
// 【2. 為什麼「大家都會自律」不是答案】
// 初學者常說：「我知道不能設成負數，我自己小心就好。」這在三個層面會失效：
//   * 規模：專案有 50 個檔案時，你無法保證每一處 acc.balance = ... 都對。
//   * 時間：半年後的你，和今天的你不是同一個人。
//   * 協作：別人不會讀你的心，只會讀你的介面。介面允許的事，就會有人做。
// 封裝的價值不在於「防小人」，而在於「把規則寫進型別，讓編譯器幫你記住」。
//
// 【3. public 成員讓「改實作」變成 breaking change】
// 這是比「資料被寫壞」更貴的代價。假設有一天你要把
//     double balance;
// 改成以「分」為單位的整數以避免浮點誤差：
//     long long balance_cents;
// 若 balance 是 private，只要改建構函式與 balance() 內部即可，外界零感知。
// 若 balance 是 public，所有讀寫它的程式碼（可能散落在數十個檔案、甚至別的
// 團隊的專案）全部要改。你的「內部實作細節」因為 public 而變成了「公開契約」。
//
// 【4. 這不是「多寫幾行 getter/setter」的儀式】
// 常見誤解是把封裝理解成「把成員設 private，然後每個都配一組 get/set」。
// 那只是把 public 成員換了個寫法，不變量一樣沒有被保護：
//     void setBalance(double b) { balance = b; }   // 和 public 完全等價
// 真正的封裝是「不暴露欄位，而暴露操作」：
//     void deposit(double amt);    // 內含 amt > 0 檢查
//     bool withdraw(double amt);   // 內含餘額足夠檢查，失敗回 false
// 介面描述的是「這個物件能做什麼」，而不是「這個物件裡面有什麼」。
//
// 【概念補充 Concept Deep Dive】
// (A) access specifier 是純編譯期概念，沒有執行期成本
//     public/private/protected 只影響「名稱查找階段是否允許存取」，
//     編譯後的機器碼完全相同 —— private 成員不會被加密、不會多一次檢查。
//     用 reinterpret_cast 硬轉指標仍可讀到 private 記憶體（那是 UB，
//     標準不保證結果，但足以說明它不是安全機制）。
//     結論：access control 防的是「意外」，不是「惡意」。它是設計工具，不是資安工具。
//
// (B) access specifier 不影響記憶體佈局的順序保證
//     C++11 起，同一個 access specifier 區段內的非靜態資料成員，位址遞增順序
//     與宣告順序一致。但「跨不同 access 區段」之間的相對順序，標準未規定，
//     由實作決定（實作定義）。所以把成員拆成好幾段 public/private 之後，
//     不要對整體佈局做任何假設。
//
// (C) 為什麼 struct 預設 public、class 預設 private
//     這是 C++ 對 C 相容性的直接後果：C 的 struct 所有成員本來就都能存取，
//     Stroustrup 為了讓既有 C 程式碼照樣編譯，保留了 struct 的預設 public；
//     而新引入的 class 關鍵字則選了「預設封閉」這個更安全的預設值。
//     除了預設存取權（以及預設繼承權）之外，兩者在語言規則上完全等價。
//
// 【注意事項 Pay Attention】
// 1. 「能編譯」不等於「設計正確」。本檔零錯誤零警告，但它是個壞設計。
// 2. public 成員 = 你對外承諾的契約。契約一旦公開，就很難收回。
// 3. 無腦 getter/setter 不是封裝；暴露「操作」而非「欄位」才是。
// 4. private 不是安全邊界，別拿它當資安機制（見概念補充 A）。
// 5. 真正需要「純資料聚合」（如函式回傳多個值、設定檔結構）時，
//    全 public 的 struct 是完全合理的選擇 —— 因為它根本沒有要維護的不變量。
//    判準是「有沒有不變量」，不是「public 一定壞」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝與 access specifier
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼要把資料成員設成 private？講一個 getter/setter 以外的理由。
//     答：為了保住 class invariant（不變量）。private 讓「修改狀態」的唯一入口
//         是你寫的成員函式，於是檢查、log、通知都有地方掛。
//         第二個理由更實際：private 成員屬於實作細節，改它不會波及呼叫端；
//         public 成員一旦被外界使用，就變成公開契約，改動即 breaking change。
//     追問：那把成員設 private 再配一組 get/set 算封裝嗎？
//         → 不算。setBalance(double) 和 public balance 在不變量保護上完全等價。
//           封裝要暴露的是 deposit()/withdraw() 這種「操作」。
//
// 🔥 Q2. private 能防止別人讀到那塊記憶體嗎？
//     答：不能。access control 是純編譯期的名稱存取檢查，不產生任何執行期程式碼。
//         繞過的方式（指標硬轉、記憶體掃描）都存在，只是那些行為屬於 UB，
//         標準不保證結果。private 防的是「意外誤用」，不是「惡意攻擊」。
//     追問：那 private 有執行期成本嗎？→ 完全沒有，編譯後的機器碼一模一樣。
//
// ⚠️ 陷阱. 「這個 class 全部 public，但我保證只有我自己用，所以沒差」——錯在哪？
//     答：錯在把「目前沒人誤用」當成「介面設計正確」。真正的成本不是今天有人
//         寫壞資料，而是你從此失去了「改實作」的自由：把 double balance 換成
//         long long balance_cents 這種純內部優化，會變成所有呼叫端都要改的
//         breaking change。
//     為什麼會錯：多數人腦中把封裝理解成「防止別人亂改」（防禦性思維），
//         所以推論出「沒有別人 → 不需要封裝」。但封裝的主要收益其實是
//         「解耦介面與實作」，這個收益跟有沒有別人完全無關 —— 單人專案照樣受益。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

// -----------------------------------------------------------------------------
// 【反面教材】所有成員都是 public —— 沒有任何不變量被保護
// -----------------------------------------------------------------------------
class BankAccount {
public:
    string owner;
    double balance = 0.0;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】同一個需求，用封裝改寫：可稽核的帳戶
//   情境：金融系統的帳戶必須滿足
//     (a) 餘額不可為負            (b) 存提金額必須為正
//     (c) 每一筆異動都要留稽核軌跡（合規要求，事後要能查）
//   這三條規則只有在「唯一入口是成員函式」時才能保證，這正是 private 的價值。
// -----------------------------------------------------------------------------
class SafeAccount {
private:
    string m_owner;
    double m_balance = 0.0;
    int    m_txCount = 0;     // 稽核用：異動筆數

    // 私有輔助函式：集中所有稽核輸出，未來要改成寫檔只要動這裡
    void audit(const string& action, double amt) {
        ++m_txCount;
        cout << "    [稽核#" << m_txCount << "] " << action
             << " $" << amt << " -> 餘額 $" << m_balance << endl;
    }

public:
    SafeAccount(const string& owner, double initial)
        : m_owner(owner), m_balance(initial > 0 ? initial : 0.0) {}

    // 暴露「操作」而非「欄位」：檢查點在此，外界繞不過去
    bool deposit(double amt) {
        if (amt <= 0) {
            cout << "    [拒絕] 存款金額必須為正，收到 " << amt << endl;
            return false;
        }
        m_balance += amt;
        audit("存入", amt);
        return true;
    }

    bool withdraw(double amt) {
        if (amt <= 0) {
            cout << "    [拒絕] 提款金額必須為正，收到 " << amt << endl;
            return false;
        }
        if (amt > m_balance) {
            cout << "    [拒絕] 餘額不足（餘額 $" << m_balance
                 << "，欲提 $" << amt << "）" << endl;
            return false;
        }
        m_balance -= amt;
        audit("提出", amt);
        return true;
    }

    // 只讀存取：回傳值而非 reference，外界拿不到可寫的通道
    double balance() const { return m_balance; }
    const string& owner() const { return m_owner; }
};

int main() {
    cout << "=== 反面教材：全 public，不變量完全失守 ===" << endl;
    BankAccount acc;
    acc.owner = "陳信安";
    acc.balance = 10000.0;
    cout << "  初始： " << acc.owner << " $" << acc.balance << endl;

    // 以下每一行都是合法 C++，編譯器一句話都不會說
    acc.balance = -999999.0;   // 餘額變成負數 —— 業務上荒謬，語法上合法
    cout << "  被設成負數： $" << acc.balance << endl;

    acc.balance = 0.0;         // 直接清空，沒有任何稽核紀錄
    cout << "  被清空：     $" << acc.balance << endl;

    acc.owner = "";            // 帳戶失去擁有者
    cout << "  擁有者被清空：[" << acc.owner << "]" << endl;
    cout << "  → 沒有任何一行檢查程式碼被執行過。" << endl;

    cout << "\n=== 封裝版：規則寫進型別，編譯器與函式共同把關 ===" << endl;
    SafeAccount safe("陳信安", 10000.0);
    cout << "  初始： " << safe.owner() << " $" << safe.balance() << endl;

    safe.deposit(5000.0);      // 正常存入
    safe.withdraw(2000.0);     // 正常提出
    safe.withdraw(999999.0);   // 被擋下：餘額不足
    safe.deposit(-100.0);      // 被擋下：金額非正

    cout << "  最終餘額： $" << safe.balance() << endl;

    // 下面這幾行若解除註解，會在「編譯期」就失敗 —— 這正是我們要的
    // safe.m_balance = -999999.0;   // ❌ error: 'm_balance' is private
    // safe.m_txCount = 0;           // ❌ error: 'm_txCount' is private
    cout << "  （safe.m_balance = -999999.0; 會編譯失敗，錯誤擋在上線前）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 11 課：存取修飾符——public、private、protected1.cpp" -o access1

// === 預期輸出 ===
// === 反面教材：全 public，不變量完全失守 ===
//   初始： 陳信安 $10000
//   被設成負數： $-999999
//   被清空：     $0
//   擁有者被清空：[]
//   → 沒有任何一行檢查程式碼被執行過。
//
// === 封裝版：規則寫進型別，編譯器與函式共同把關 ===
//   初始： 陳信安 $10000
//     [稽核#1] 存入 $5000 -> 餘額 $15000
//     [稽核#2] 提出 $2000 -> 餘額 $13000
//     [拒絕] 餘額不足（餘額 $13000，欲提 $999999）
//     [拒絕] 存款金額必須為正，收到 -100
//   最終餘額： $13000
//   （safe.m_balance = -999999.0; 會編譯失敗，錯誤擋在上線前）
