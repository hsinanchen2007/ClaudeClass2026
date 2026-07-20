// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 7  —  在 constructor 裡驗證資料
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : constructor 作為「類別不變式（class invariant）」的守門員
//   標準版本  : C++98 起；本檔用到的 <stdexcept> 例外型別自 C++98 即有
//   標頭檔    : <iostream>、<string>、<stdexcept>
//   核心概念  : 若物件成功建構，它的狀態就必須是合法的——這是 constructor 的契約
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼是類別不變式（class invariant）】
//   不變式是「這個類別的物件在任何可被外界觀察的時刻都必須成立的條件」，例如：
//       * balance >= 0
//       * accountId 一定是 10 碼
//       * owner 不是空字串
//   不變式的價值在於：一旦建立，**類別的其他成員函式就不必再檢查一次**。
//   deposit()、withdraw() 可以放心假設 balance 是合法的，程式碼因此變乾淨。
//   而守住不變式的第一道關卡，就是 constructor。
//
// 【2. 驗證失敗時的三種策略，以及它們的代價】
//   ┌──────────────┬────────────────────────┬──────────────────────────┐
//   │ 策略          │ 做法                    │ 代價                      │
//   ├──────────────┼────────────────────────┼──────────────────────────┤
//   │ A. 悄悄修正   │ 換成預設值並印警告      │ 呼叫端不知道自己錯了      │
//   │ B. 拋出例外   │ throw std::invalid_argument│ 呼叫端必須處理，但錯不了 │
//   │ C. 靜態工廠   │ 回傳 optional/expected  │ 不用例外，但呼叫端要解包  │
//   └──────────────┴────────────────────────┴──────────────────────────┘
//   本檔原本的寫法是 A。它作為教學很直觀，但在真實系統中是有問題的：
//   把 id="123" 悄悄改成 "0000000000"，等於**製造出一個看起來合法、
//   實際上是錯的帳戶**。呼叫端拿到物件、以為成功了，錯誤要到很久以後才浮現。
//   本檔同時示範 A 與 B、C，讓你看見差別。
//
// 【3. 為什麼「從 constructor 拋出例外」是安全且正確的】
//   很多人以為 constructor 拋例外會留下半成品物件——不會。標準規定：
//     * constructor 沒有正常結束 → 物件的生命週期從未開始
//       → 這個物件的 destructor **不會**被呼叫
//     * 但**已經建構完成的成員與 base class**，會依相反順序自動解構
//   所以只要每個成員自己是 RAII（string、vector、unique_ptr…），
//   從 constructor 拋例外完全不會洩漏資源。這正是「用 RAII 型別當成員」的價值。
//   唯一的真正陷阱是：constructor 裡自己 new 了裸指標又在後面拋例外，
//   那塊記憶體就洩漏了——因為裸指標成員沒有 destructor 幫你收拾。
//
// 【4. 為什麼不要在 constructor 裡呼叫 virtual function 做驗證】
//   執行 Base 的 constructor 時，物件的動態型別就是 Base，
//   即使實際上在建 Derived，也只會呼叫到 Base 的版本，derived 的 override 不生效。
//   若呼叫的是 pure virtual function，行為是 UB。
//   CERT 規則 OOP50-CPP 明文禁止此寫法。要做「依子類別而異的驗證」，
//   請用兩階段初始化 + factory function 把兩步包起來。
//
// 【概念補充 Concept Deep Dive】
//   ▍驗證應該放在哪一層
//     constructor 適合驗證「這個型別本身的不變式」（餘額非負、ID 長度）。
//     至於「這個帳號在資料庫中是否存在」這種需要外部資源的檢查，
//     不該塞進 constructor——它會讓建立物件變成一個可能失敗、可能很慢、
//     難以測試的操作。那類檢查屬於上層的 service。
//
//   ▍為什麼用整數存金額
//     本檔的 balance 用 double 是為了貼近原始教材，但真實金融系統
//     幾乎一律用整數（分）或十進位定點型別。double 無法精確表示 0.1，
//     累加後會出現 0.30000000000000004 這類誤差，對帳會對不起來。
//
//   ▍noexcept 與驗證
//     會拋例外的 constructor 不能標 noexcept。若你希望某個型別的建構
//     絕不失敗（例如要放進 noexcept 的 move 操作中），就得把驗證移到
//     工廠函式，讓 constructor 只做「已知合法的資料」的搬運。
//
// 【注意事項 Pay Attention】
//   1. 「悄悄修正」會把錯誤藏起來，讓 bug 延後爆發——教學可以，正式系統要慎用。
//   2. constructor 拋例外不會洩漏 RAII 成員，但會洩漏自己 new 的裸指標。
//   3. 不要在 constructor 裡呼叫 virtual function（呼叫 pure virtual 是 UB）。
//   4. 金額請勿用浮點數；本檔沿用 double 僅為對照原始教材。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】constructor 中的驗證與例外
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. constructor 裡拋出例外，物件會怎樣？會不會洩漏資源？
//     答：物件的生命週期從未開始，所以它自己的 destructor **不會**被呼叫。
//         但已經建構完成的成員與 base class 會依反序自動解構。
//         因此只要成員都是 RAII 型別就不會洩漏；若你在 constructor 裡
//         new 了裸指標又在之後拋例外，那塊記憶體就洩漏了。
//     追問：怎麼避免？→ 讓每個資源都由自己的 RAII 成員持有
//         （unique_ptr、vector、string），不要在 constructor 裡管理裸資源。
//
// 🔥 Q2. 建構失敗時，該拋例外還是回傳錯誤碼？
//     答：constructor 沒有回傳值，所以它「只能」拋例外。
//         如果專案禁用例外（嵌入式、遊戲引擎常見），標準做法是把
//         constructor 設為 private，改用靜態工廠函式回傳
//         std::optional<T>（C++17）或 std::expected<T,E>（C++23），
//         由呼叫端解包。關鍵是不要留下「建構成功但狀態非法」的物件。
//     追問：兩階段初始化（先建構、再 init()）不行嗎？→ 可行但很脆弱：
//         物件在 init() 之前處於半成品狀態，任何人都可能在那時候用它。
//
// ⚠️ 陷阱. 在 base class 的 constructor 裡呼叫 virtual function 做驗證，
//         想讓每個 derived class 有自己的驗證邏輯。
//     答：不會如你預期。執行 Base 的 constructor 時，物件的動態型別就是 Base，
//         呼叫到的永遠是 Base 的版本，derived 的 override 不會生效。
//         若那是 pure virtual function，行為是 UB。
//     為什麼會錯：以為 virtual 分派在「物件記憶體配置完成」後就能運作。
//         實際上 vptr 是在建構過程中逐層設定的——Base 的 constructor 執行時，
//         vptr 還指著 Base 的 vtable，derived 部分根本尚未初始化。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <stdexcept>
#include <optional>
using namespace std;

// -----------------------------------------------------------------------------
// 策略 A：悄悄修正（原始教材的作法）—— 直觀，但會把錯誤藏起來
// -----------------------------------------------------------------------------
class BankAccount {
private:
    string owner;
    double balance;
    string accountId;

public:
    BankAccount(string ownerName, double initialBalance, string id) {
        // 驗證帳戶名
        if (ownerName.empty()) {
            cout << "警告：帳戶名不能為空，使用預設值" << endl;
            owner = "未知";
        } else {
            owner = ownerName;
        }

        // 驗證初始餘額
        if (initialBalance < 0) {
            cout << "警告：初始餘額不能為負數，設為 0" << endl;
            balance = 0.0;
        } else {
            balance = initialBalance;
        }

        // 驗證帳戶 ID
        if (id.length() != 10) {
            cout << "警告：帳戶 ID 必須為 10 位，使用預設 ID" << endl;
            accountId = "0000000000";
        } else {
            accountId = id;
        }

        cout << "帳戶創建成功: " << owner << endl;
    }

    void print() const {
        cout << "  帳戶: " << accountId
             << ", 戶主: " << owner
             << ", 餘額: $" << balance << endl;
    }
};

// -----------------------------------------------------------------------------
// 策略 B：拋出例外 —— 建構成功就保證合法，不存在「假的」帳戶
// -----------------------------------------------------------------------------
class StrictAccount {
private:
    string    owner_;
    long long balanceCents_;   // 用整數分，避免浮點誤差
    string    accountId_;

public:
    StrictAccount(const string& owner, long long balanceCents, const string& id)
        : owner_(owner), balanceCents_(balanceCents), accountId_(id) {
        if (owner_.empty())
            throw invalid_argument("帳戶名不能為空");
        if (balanceCents_ < 0)
            throw invalid_argument("初始餘額不能為負數");
        if (accountId_.length() != 10)
            throw invalid_argument("帳戶 ID 必須為 10 碼，收到 "
                                   + to_string(accountId_.length()) + " 碼");
        // 走到這裡 → 不變式成立，其他成員函式不必再檢查一次
    }

    // 因為不變式已在建構時確立，這裡可以放心假設 balanceCents_ >= 0
    void withdraw(long long cents) {
        if (cents > balanceCents_) throw runtime_error("餘額不足");
        balanceCents_ -= cents;
    }

    void print() const {
        cout << "  帳戶: " << accountId_ << ", 戶主: " << owner_
             << ", 餘額: $" << (balanceCents_ / 100) << "."
             << (balanceCents_ % 100 < 10 ? "0" : "")
             << (balanceCents_ % 100) << endl;
    }
};

// -----------------------------------------------------------------------------
// 策略 C：靜態工廠 + optional —— 不用例外也能避免「非法但存在」的物件
// -----------------------------------------------------------------------------
class SafeAccount {
private:
    string    owner_;
    long long balanceCents_;
    string    accountId_;

    // private constructor：只有 create() 能呼叫，且只在資料已驗證後
    SafeAccount(const string& o, long long b, const string& id)
        : owner_(o), balanceCents_(b), accountId_(id) {}

public:
    static optional<SafeAccount> create(const string& owner,
                                        long long balanceCents,
                                        const string& id) {
        if (owner.empty())          return nullopt;
        if (balanceCents < 0)       return nullopt;
        if (id.length() != 10)      return nullopt;
        return SafeAccount(owner, balanceCents, id);
    }

    void print() const {
        cout << "  帳戶: " << accountId_ << ", 戶主: " << owner_
             << ", 餘額(分): " << balanceCents_ << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：LeetCode 的設計題保證輸入落在題目給定的 constraints 內，
//         因此完全不需要——也不會考——建構時的參數驗證。
//         把驗證硬塞進 LeetCode 解法反而是扣分的過度設計。
//         本檔談的是真實系統的防禦性設計，屬於工程議題而非演算法議題。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔載入：三種策略在同一個場景下的實際差別
//   情境：服務啟動時讀取 config，需要 port 與 workerCount。
//         設定檔可能被人手改壞（port=99999、workerCount=-1）。
//   * 悄悄修正 → 服務用著「不是你設定的」參數啟動，事後很難察覺
//   * 拋出例外 → 啟動立刻失敗並說明原因，這通常正是你要的（fail fast）
//   對啟動期設定而言，fail fast 幾乎永遠優於悄悄修正：
//   一個用錯 port 啟動的服務，比一個拒絕啟動的服務難除錯得多。
// -----------------------------------------------------------------------------
class ServerConfig {
private:
    int port_;
    int workerCount_;

public:
    ServerConfig(int port, int workerCount)
        : port_(port), workerCount_(workerCount) {
        if (port_ < 1 || port_ > 65535)
            throw invalid_argument("port 必須介於 1..65535，收到 " + to_string(port_));
        if (workerCount_ < 1 || workerCount_ > 1024)
            throw invalid_argument("workerCount 必須介於 1..1024，收到 "
                                   + to_string(workerCount_));
    }

    void print() const {
        cout << "  設定生效: port=" << port_
             << ", workers=" << workerCount_ << endl;
    }
};

int main() {
    cout << "=== 策略 A：悄悄修正（原始作法）===" << endl;
    cout << "正常創建：" << endl;
    BankAccount a1("王五", 10000.0, "1234567890");
    a1.print();

    cout << "\n非法數據：" << endl;
    BankAccount a2("", -500.0, "123");   // 全部都是非法值
    a2.print();
    cout << "  ⚠️ 注意：a2 看起來「創建成功」了，但它其實不是呼叫端要的帳戶。" << endl;

    cout << "\n=== 策略 B：拋出例外（建構成功即保證合法）===" << endl;
    try {
        StrictAccount ok("王五", 1000000, "1234567890");
        ok.print();
        ok.withdraw(250050);
        cout << "  提款 $2500.50 後 → ";
        ok.print();
    } catch (const exception& e) {
        cout << "  不該走到這裡: " << e.what() << endl;
    }

    try {
        StrictAccount bad("", -500, "123");
        bad.print();
    } catch (const invalid_argument& e) {
        cout << "  建構被拒絕: " << e.what() << endl;
        cout << "  → 沒有產生任何「半合法」的物件，呼叫端一定知道失敗了" << endl;
    }

    cout << "\n=== 策略 C：靜態工廠 + optional（不使用例外）===" << endl;
    if (auto acc = SafeAccount::create("趙六", 50000, "9876543210")) {
        cout << "  建立成功 → ";
        acc->print();
    } else {
        cout << "  建立失敗" << endl;
    }

    if (auto acc = SafeAccount::create("錢七", 50000, "abc")) {
        cout << "  建立成功 → ";
        acc->print();
    } else {
        cout << "  建立失敗：ID 長度不符，回傳 nullopt（沒有例外，也沒有壞物件）" << endl;
    }

    cout << "\n=== 日常實務：啟動期設定要 fail fast ===" << endl;
    try {
        ServerConfig good(8080, 16);
        good.print();
    } catch (const exception& e) {
        cout << "  不該走到這裡: " << e.what() << endl;
    }

    try {
        ServerConfig bad(99999, 16);      // 有人把 port 打錯了
        bad.print();
    } catch (const invalid_argument& e) {
        cout << "  啟動中止: " << e.what() << endl;
        cout << "  → 比「悄悄改成 8080 然後在別台機器上聽」好太多了" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎7.cpp" -o ctor7

// === 預期輸出 ===
// === 策略 A：悄悄修正（原始作法）===
// 正常創建：
// 帳戶創建成功: 王五
//   帳戶: 1234567890, 戶主: 王五, 餘額: $10000
//
// 非法數據：
// 警告：帳戶名不能為空，使用預設值
// 警告：初始餘額不能為負數，設為 0
// 警告：帳戶 ID 必須為 10 位，使用預設 ID
// 帳戶創建成功: 未知
//   帳戶: 0000000000, 戶主: 未知, 餘額: $0
//   ⚠️ 注意：a2 看起來「創建成功」了，但它其實不是呼叫端要的帳戶。
//
// === 策略 B：拋出例外（建構成功即保證合法）===
//   帳戶: 1234567890, 戶主: 王五, 餘額: $10000.00
//   提款 $2500.50 後 →   帳戶: 1234567890, 戶主: 王五, 餘額: $7499.50
//   建構被拒絕: 帳戶名不能為空
//   → 沒有產生任何「半合法」的物件，呼叫端一定知道失敗了
//
// === 策略 C：靜態工廠 + optional（不使用例外）===
//   建立成功 →   帳戶: 9876543210, 戶主: 趙六, 餘額(分): 50000
//   建立失敗：ID 長度不符，回傳 nullopt（沒有例外，也沒有壞物件）
//
// === 日常實務：啟動期設定要 fail fast ===
//   設定生效: port=8080, workers=16
//   啟動中止: port 必須介於 1..65535，收到 99999
//   → 比「悄悄改成 8080 然後在別台機器上聽」好太多了
