// =============================================================================
//  第 12 課：struct 與 class 的差異 5  —  什麼時候該用 class（有不變條件要守）
// =============================================================================
//
// 【主題資訊 Information】
//   主題：class 的正確適用場景 —— 存在不變條件（invariant）需要強制維護
//   標準版本：C++98 起即可；本檔的 NSDMI（balance = 0.0）需 C++11
//   標頭檔：<string>
//   核心概念：封裝（encapsulation）的目的不是「藏起來」，而是「讓非法狀態無法表達」
//
// 【詳細解釋 Explanation】
//
// 【1. 不變條件是什麼？為什麼它決定了要用 class】
//   BankAccount 有一條規則必須永遠成立：balance >= 0。
//   如果 balance 是 public 的，任何人都能寫 `acc.balance = -999;`，
//   而你的所有檢查邏輯完全被繞過。這不是「別人會不會亂寫」的問題 ——
//   而是「編譯器有沒有辦法幫你保證」的問題。
//   把 balance 設為 private 之後，唯一能改動它的入口就是 deposit()/withdraw()，
//   而這兩個函式都會先驗證再修改。於是不變條件從「靠紀律維持」升級成
//   「靠型別系統保證」。這就是選擇 class 的真正理由。
//
// 【2. 為什麼 deposit / withdraw 要回傳 bool？】
//   金額非法時有三種處理方式，各有適用場景：
//     (a) 回傳 bool（本檔採用）：呼叫端自行決定怎麼處理，無例外開銷。
//         缺點是呼叫端可能忽略回傳值（可用 C++17 的 [[nodiscard]] 強制檢查）。
//     (b) 丟出例外：適合「這絕不該發生」的情況，錯誤無法被靜默忽略。
//     (c) 靜默修正（例如負數就當 0）：最危險，會把 bug 藏起來，通常應避免。
//   關鍵是：無論選哪一種，物件都不可以進入非法狀態。本檔用 [[nodiscard]]
//   標註，讓「忘記檢查回傳值」變成編譯警告。
//
// 【3. getter 為什麼要標 const？】
//   getBalance() 不修改任何成員，標上 const 之後：
//     * 編譯器會擋住你在函式內不小心寫入成員（多一層保護）
//     * const BankAccount& 也能呼叫它 —— 這點極其重要，否則把物件以
//       const 參考傳進函式後，連讀值都做不到
//   原始版本沒有標 const，本檔補上了。這是 const-correctness 的基本功。
//
// 【4. init() 是個過渡設計，建構函式才是正解】
//   本檔保留 init() 是為了配合課程進度（建構函式在第 13 課才教），但要誠實指出
//   它的缺陷：物件在 init() 被呼叫之前處於「已建構但未初始化」的中間狀態，
//   而使用者完全可能忘記呼叫。真正的解法是把初始化寫進建構函式 ——
//   讓「物件存在」與「物件合法」在同一個瞬間發生，中間沒有空窗期。
//
// 【概念補充 Concept Deep Dive】
//   * 用 double 存金額在真實金融系統中是有問題的：二進位浮點數無法精確表示
//     0.1、0.2 這類十進位小數，長期累加會產生誤差。本檔的輸出就示範了這一點。
//     業界正解是用整數存「最小貨幣單位」（例如以分為單位的 long long），
//     或使用十進位定點數型別。這是教材常見的簡化，但值得知道真相。
//   * private 只擋編譯期的名稱查找，不做執行期保護（見本課第 2 檔）。
//     它防的是「意外」與「維護時的疏忽」，不是惡意攻擊。
//   * 封裝真正的長期價值在於「改內部表示不會波及呼叫端」：哪天你把 double
//     換成 long long cents，只要 getBalance() 的介面不變，外部程式碼一行都不用改。
//
// 【注意事項 Pay Attention】
//   1. getter 一律標 const，否則 const 物件無法讀取。
//   2. 回傳 bool 的驗證函式，呼叫端很容易忘記檢查。C++17 的 [[nodiscard]]
//      能讓編譯器發出警告，成本極低，建議一律加上。
//   3. 別用「靜默修正非法輸入」當作驗證 —— 那只是把錯誤延後爆炸，
//      而且爆炸點會離真正的 bug 很遠。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】封裝與不變條件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 封裝的目的是什麼？只是為了「藏起來」嗎？
//     答：不是。目的是讓「非法狀態無法被表達」——把資料設為 private 之後，
//         所有修改都必須經過你寫的驗證邏輯，不變條件從靠紀律維持升級成
//         由型別系統保證。附帶好處是內部表示可以自由重構而不影響呼叫端。
//     追問：那全部欄位都寫 getter/setter 算封裝嗎？→ 不算。若 setter 只是
//           無腦賦值、沒有任何驗證，那等同 public 成員，只是多打了很多字。
//
// 🔥 Q2. 成員函式什麼時候該標 const？
//     答：只要不修改物件的可觀察狀態就該標。最實際的理由是：沒標 const 的函式
//         無法透過 const 物件或 const 參考呼叫，會讓你的類別無法被安全地傳遞。
//     追問：mutable 是做什麼的？→ 讓成員在 const 函式中仍可修改，
//           典型用途是快取或 mutex 這類「不影響邏輯狀態」的成員。
//
// ⚠️ 陷阱. 用 double 存金額有什麼問題？
//     答：二進位浮點數無法精確表示 0.1 這類十進位小數，反覆累加會累積誤差，
//         可能出現「餘額顯示 0 但實際是 1e-13」而導致比較判斷失準。
//     為什麼會錯：多數人把 double 當成「更精確的 float」，忽略了它的誤差來源
//         是進位制而非位元數 —— 再多位元也表示不出精確的 0.1。
//         正解是用整數存最小貨幣單位（分），或用十進位定點數。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
using namespace std;

// ✅ 適合用 class —— 有行為、有保護
class BankAccount {
public:
    void init(const string& name, double initial) {
        ownerName = name;
        if (initial >= 0) balance = initial;   // 非法初始值不套用，維持 0
    }

    // [[nodiscard]]（C++17）：呼叫端忽略回傳值時編譯器會警告
    [[nodiscard]] bool deposit(double amount) {
        if (amount <= 0) return false;         // 拒絕非法輸入，而不是靜默修正
        balance += amount;
        return true;
    }

    [[nodiscard]] bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;   // 守住 balance >= 0
        balance -= amount;
        return true;
    }

    // getter 一律標 const，否則 const BankAccount& 連讀值都做不到
    double getBalance() const { return balance; }
    string getOwner()   const { return ownerName; }

private:
    string ownerName;
    double balance = 0.0;   // 不變條件：balance >= 0，由 deposit/withdraw 共同保證
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用整數存「最小貨幣單位」避開浮點誤差
//   情境：電商訂單金額累計。真實系統絕不用 double 存錢 —— 二進位浮點數表示
//   不出精確的 0.1，上千筆訂單累加之後會出現對不上帳的分毫誤差。
//   業界做法是用整數存「分」（cents），顯示時才除以 100 格式化。
//   這個 class 同時示範了封裝的另一個好處：內部用 long long，
//   對外仍以「元」為單位溝通，呼叫端完全不需要知道內部表示。
// -----------------------------------------------------------------------------
class Money {
public:
    // 工廠函式：從「元」建立，內部立刻轉成「分」
    static Money fromYuan(double yuan) {
        // +0.5 是四捨五入，避免 double 轉整數時的截斷誤差
        return Money(static_cast<long long>(yuan * 100 + (yuan >= 0 ? 0.5 : -0.5)));
    }

    void add(const Money& other) { m_cents += other.m_cents; }   // 整數加法，零誤差

    long long cents() const { return m_cents; }

    string toString() const {
        long long whole = m_cents / 100;
        long long frac  = m_cents % 100;
        if (frac < 0) frac = -frac;
        string s = to_string(whole) + ".";
        if (frac < 10) s += "0";
        s += to_string(frac);
        return s;
    }

private:
    explicit Money(long long cents) : m_cents(cents) {}
    long long m_cents = 0;    // 不變條件：一律以「分」為單位，外部無從誤用
};

int main() {
    cout << "=== 基本操作 ===" << endl;
    BankAccount acc;
    acc.init("陳信安", 1000);

    // 回傳值一定要檢查 —— [[nodiscard]] 會讓忽略它變成編譯警告
    if (!acc.deposit(500))  cout << "存款失敗" << endl;
    if (!acc.withdraw(200)) cout << "提款失敗" << endl;
    cout << acc.getOwner() << ": $" << acc.getBalance() << endl;

    cout << "\n=== 不變條件被守住 ===" << endl;
    // acc.balance = -999;   // ❌ 編譯錯誤：balance 是 private，非法狀態無法表達
    struct Attempt { const char* desc; double amount; bool isWithdraw; };
    vector<Attempt> attempts = {
        {"提款超過餘額", 99999.0, true},
        {"存入負數",       -50.0, false},
        {"提款 0 元",        0.0, true},
        {"正常提款 300",   300.0, true},
    };
    for (const Attempt& a : attempts) {
        bool ok = a.isWithdraw ? acc.withdraw(a.amount) : acc.deposit(a.amount);
        cout << "  " << a.desc << " -> " << (ok ? "接受" : "拒絕")
             << "，餘額仍為 " << acc.getBalance() << endl;
    }
    cout << "  餘額全程未曾為負（不變條件成立）" << endl;

    cout << "\n=== const 正確性 ===" << endl;
    const BankAccount& ref = acc;          // 以 const 參考傳遞
    cout << "  透過 const 參考讀取: " << ref.getOwner()
         << " $" << ref.getBalance() << endl;
    // ref.deposit(1);   // ❌ 編譯錯誤：deposit 非 const，const 物件不可呼叫

    cout << "\n=== 浮點誤差實測：為什麼不能用 double 存錢 ===" << endl;
    double sum = 0.0;
    for (int i = 0; i < 10; ++i) sum += 0.1;   // 十次 0.1 相加
    cout << fixed << setprecision(20);
    cout << "  double 累加 10 次 0.1 = " << sum << endl;
    cout << "  等於 1.0 嗎? " << (sum == 1.0 ? "是" : "否（有誤差！）") << endl;
    cout.unsetf(ios::fixed);
    cout << setprecision(6);

    cout << "\n=== 實務：用整數分存錢，零誤差 ===" << endl;
    Money total = Money::fromYuan(0.0);
    for (int i = 0; i < 10; ++i) total.add(Money::fromYuan(0.1));
    cout << "  Money 累加 10 次 0.10 元 = " << total.toString() << " 元"
         << "（內部 " << total.cents() << " 分）" << endl;
    cout << "  整數運算不會累積誤差，這是金融系統的標準做法" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異5.cpp" -o demo5
// 註：使用 [[nodiscard]] 屬性，需 C++17 以上。

// === 預期輸出 ===
// === 基本操作 ===
// 陳信安: $1300
//
// === 不變條件被守住 ===
//   提款超過餘額 -> 拒絕，餘額仍為 1300
//   存入負數 -> 拒絕，餘額仍為 1300
//   提款 0 元 -> 拒絕，餘額仍為 1300
//   正常提款 300 -> 接受，餘額仍為 1000
//   餘額全程未曾為負（不變條件成立）
//
// === const 正確性 ===
//   透過 const 參考讀取: 陳信安 $1000
//
// === 浮點誤差實測：為什麼不能用 double 存錢 ===
//   double 累加 10 次 0.1 = 0.99999999999999988898
//   等於 1.0 嗎? 否（有誤差！）
//
// === 實務：用整數分存錢，零誤差 ===
//   Money 累加 10 次 0.10 元 = 1.00 元（內部 100 分）
//   整數運算不會累積誤差，這是金融系統的標準做法
