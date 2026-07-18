/*
 * ============================================================================
 *  第 24 課：類別內的靜態成員變數 —— 完整複習總結
 * ============================================================================
 *
 *  本檔案整合了第 24 課所有 8 個範例檔案的核心概念，
 *  涵蓋靜態成員變數的定義、初始化、訪問方式、生命週期、
 *  訪問控制、sizeof 特性、constexpr static，以及綜合實戰範例。
 *  閱讀本檔案即可完整複習本課所有知識點。
 *
 *  目錄：
 *    第一節：靜態成員變數的基本概念與傳統語法（對應檔案 1）
 *    第二節：C++17 inline static 簡化語法（對應檔案 2）
 *    第三節：靜態成員的兩種訪問方式（對應檔案 3）
 *    第四節：靜態成員的生命週期（對應檔案 4）
 *    第五節：靜態成員與訪問控制（private/public）（對應檔案 5）
 *    第六節：sizeof 與靜態成員——不佔對象空間（對應檔案 6）
 *    第七節：constexpr static——編譯期常量（對應檔案 7）
 *    第八節：綜合範例——遊戲實體管理系統（對應檔案 8）
 *    附錄：普通成員 vs 靜態成員 對比總結表
 *
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

/* ============================================================================
 *  第一節：靜態成員變數的基本概念與傳統語法
 * ============================================================================
 *
 *  核心觀念：
 *    - 普通成員變數：屬於「單一對象」，每個對象各有一份
 *    - 靜態成員變數：屬於「整個類別」，所有對象共享同一份
 *
 *  傳統語法（C++11 之前）：
 *    1. 在類別內「聲明」（declaration）：   static int totalCount_;
 *    2. 在類別外「定義並初始化」（definition）：int Soldier::totalCount_ = 0;
 *
 *  為什麼需要類別外定義？
 *    - 類別內的 static int totalCount_; 只是告訴編譯器「這個變數存在」
 *    - 類別外的 int Soldier::totalCount_ = 0; 才是在記憶體中真正分配空間
 *    - 靜態成員不住在棧上的對象裡面，它住在「靜態/全域存儲區」
 *    - 如果只有聲明沒有定義，會導致連結錯誤（linker error）：
 *      undefined reference to `Soldier::totalCount_'
 *
 *  記憶體佈局示意：
 *    棧（Stack）：存放 s1, s2 等對象（各有自己的 name_, id_）
 *    靜態/全域區：存放 totalCount_, nextId_（不屬於任何對象，所有對象共享）
 *
 *  常見用途：
 *    - 對象計數器：追蹤目前有多少個對象存活
 *    - 唯一 ID 產生器：每個新對象自動獲得遞增 ID
 */

// --- 範例：士兵管理（傳統語法：類別內聲明 + 類別外定義）---
class Soldier {
private:
    string name_;       // 普通成員：每個士兵各有一份
    int id_;            // 普通成員：每個士兵各有一份

    // 靜態成員變數的「聲明」——只告訴編譯器它存在
    static int totalCount_;     // 追蹤所有士兵的總數
    static int nextId_;         // 下一個可用的 ID（自動遞增）

public:
    // 建構函數：每創建一個士兵，totalCount_ 加 1，id_ 自動分配
    Soldier(const string& name)
        : name_(name), id_(nextId_++)   // nextId_++ 先用再加，確保每人 ID 不同
    {
        totalCount_++;                  // 全體共享的計數器加 1
        cout << "  [入伍] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    // 解構函數：士兵退役時，totalCount_ 減 1
    ~Soldier() {
        totalCount_--;
        cout << "  [退役] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

    void report() const {
        cout << "  士兵 " << name_ << " (ID:" << id_ << ") 報到！" << endl;
    }

    // 靜態成員可以通過普通成員函數訪問
    void showTotal() const {
        cout << "  目前總人數：" << totalCount_ << endl;
    }
};

// 靜態成員變數的「定義與初始化」——在類別外，真正分配記憶體空間
// 注意語法：類型 類別名::變數名 = 初始值;
int Soldier::totalCount_ = 0;      // 初始時沒有士兵
int Soldier::nextId_ = 1001;       // ID 從 1001 開始

// --- 測試函數 ---
void demo_section1() {
    cout << "\n==============================" << endl;
    cout << "  第一節：靜態成員變數基礎" << endl;
    cout << "==============================" << endl;

    cout << "\n--- 創建士兵 ---" << endl;
    Soldier s1("阿強");     // ID:1001, 總人數:1
    Soldier s2("阿明");     // ID:1002, 總人數:2
    Soldier s3("阿華");     // ID:1003, 總人數:3

    cout << "\n--- 報到 ---" << endl;
    s1.report();
    s2.report();
    s3.report();
    s1.showTotal();         // 顯示：目前總人數：3

    cout << "\n--- 作用域結束，逆序解構 ---" << endl;
    // s3、s2、s1 依次解構（LIFO 順序），totalCount_ 逐步遞減
}


/* ============================================================================
 *  第二節：C++17 inline static 簡化語法
 * ============================================================================
 *
 *  C++17 引入了 inline 關鍵字用於靜態成員，可以直接在類別內定義並初始化，
 *  不再需要在類別外額外定義。這是目前推薦的寫法。
 *
 *  三種初始化方式比較：
 *
 *    方式 1（C++11 之前）：
 *      類別內：  static int count_;
 *      類別外：  int MyClass::count_ = 0;
 *
 *    方式 2（C++11）：
 *      類別內：  static const int MAX = 100;    // 只限 const 整數型別
 *      不需要類別外定義
 *
 *    方式 3（C++17，推薦）：
 *      類別內：  inline static int count_ = 0;  // 任何類型都行
 *      不需要類別外定義
 *
 *  const static 整數型別的特例：
 *    - 即使是 C++11，static const int 也可以在類別內初始化
 *    - 但非整數型別（如 string、double）需要 C++17 的 inline static const
 */

// --- 範例：遊戲配置（C++17 inline static）---
class GameConfig {
public:
    // C++17 inline static：直接在類別內定義並初始化，無需類別外定義
    inline static int maxPlayers = 4;           // 可修改的靜態變數
    inline static double gravity = 9.8;         // 可修改的靜態變數
    inline static string version = "1.0.3";     // 可修改的靜態變數

    // const static 整數型別：即使 C++11 也可以在類別內初始化
    static const int MAX_LEVEL = 100;           // 不可修改的整數常量
    static const int MAX_ITEMS = 999;           // 不可修改的整數常量

    // C++17 inline static const：可用於非整數型別（如 string）
    inline static const string GAME_NAME = "冒險世界";  // 不可修改的字串常量

    // 靜態函數可以直接訪問靜態成員（不需要對象）
    static void printConfig() {
        cout << "  遊戲：" << GAME_NAME << " v" << version << endl;
        cout << "  最大玩家數：" << maxPlayers << endl;
        cout << "  重力：" << gravity << endl;
        cout << "  最大等級：" << MAX_LEVEL << endl;
        cout << "  最大物品：" << MAX_ITEMS << endl;
    }
};

// 不需要類別外定義了！（C++17 的好處）

void demo_section2() {
    cout << "\n==============================" << endl;
    cout << "  第二節：C++17 inline static" << endl;
    cout << "==============================" << endl;

    // 直接通過類別名調用靜態函數
    GameConfig::printConfig();

    // 可以修改非 const 的靜態成員
    cout << "\n--- 修改配置 ---" << endl;
    GameConfig::maxPlayers = 8;        // 合法：非 const
    GameConfig::gravity = 15.0;        // 合法：非 const
    GameConfig::version = "2.0.0";     // 合法：非 const
    GameConfig::printConfig();

    // GameConfig::MAX_LEVEL = 200;    // 編譯錯誤！const 不可修改
    // GameConfig::GAME_NAME = "新名";  // 編譯錯誤！const 不可修改
}


/* ============================================================================
 *  第三節：靜態成員的兩種訪問方式
 * ============================================================================
 *
 *  方式 1（推薦）：通過類別名訪問  ->  Enemy::totalKilled
 *    - 清楚表明這是屬於類別的共享數據
 *
 *  方式 2（不推薦）：通過對象訪問   ->  e1.totalKilled
 *    - 語法合法但容易誤導，看起來像是對象自己的數據
 *    - 實際上 e1.totalKilled 和 e2.totalKilled 是完全同一個變數
 *
 *  重要提醒：無論用哪種方式訪問，都是同一個變數，修改一處全部改變。
 */

// --- 範例：敵人擊殺統計（兩種訪問方式對比）---
class Enemy {
public:
    // inline static 公開成員：可從外部直接訪問
    inline static int totalKilled = 0;      // 被擊殺的總數
    inline static int totalSpawned = 0;     // 生成的總數

private:
    string name_;
    bool alive_;

public:
    Enemy(const string& name) : name_(name), alive_(true) {
        totalSpawned++;     // 每生成一個敵人，計數加 1
    }

    void kill() {
        if (alive_) {           // 防止重複擊殺
            alive_ = false;
            totalKilled++;      // 擊殺計數加 1
            cout << "  " << name_ << " 被擊殺！" << endl;
        }
    }

    const string& getName() const { return name_; }
};

void demo_section3() {
    cout << "\n==============================" << endl;
    cout << "  第三節：靜態成員的訪問方式" << endl;
    cout << "==============================" << endl;

    // 重置靜態成員（因為前面的範例可能已經改變了值）
    Enemy::totalKilled = 0;
    Enemy::totalSpawned = 0;

    Enemy e1("哥布林");
    Enemy e2("骷髏兵");
    Enemy e3("狼人");

    e1.kill();
    e3.kill();

    // 方式 1：通過類別名訪問（推薦）——清楚表明是類別共享的數據
    cout << "\n--- 方式 1：通過類別名訪問（推薦）---" << endl;
    cout << "  生成數：" << Enemy::totalSpawned << endl;    // 3
    cout << "  擊殺數：" << Enemy::totalKilled << endl;     // 2

    // 方式 2：通過對象訪問（不推薦）——容易誤導
    cout << "\n--- 方式 2：通過對象訪問（不推薦）---" << endl;
    cout << "  生成數：" << e1.totalSpawned << endl;        // 3（同一個變數）
    cout << "  擊殺數：" << e2.totalKilled << endl;         // 2（同一個變數）

    // 驗證：無論通過哪個對象訪問，都是同一個值
    cout << "\n--- 驗證：同一個變數 ---" << endl;
    cout << "  e1.totalKilled = " << e1.totalKilled << endl;    // 2
    cout << "  e2.totalKilled = " << e2.totalKilled << endl;    // 2
    cout << "  e3.totalKilled = " << e3.totalKilled << endl;    // 2
    cout << "  都是同一個值！" << endl;
}


/* ============================================================================
 *  第四節：靜態成員的生命週期
 * ============================================================================
 *
 *  靜態成員的生命週期與全域變數相同：
 *    - 程式啟動時初始化（在 main() 之前）
 *    - 程式結束時銷毀（在 main() 之後）
 *    - 即使所有對象都被銷毀了，靜態成員仍然存活
 *
 *  時間線：
 *    程式啟動 -> 靜態成員初始化
 *                  |
 *            main() 開始
 *                  |
 *            創建 obj -> 普通成員初始化
 *                  |
 *            obj 銷毀 -> 普通成員解構
 *                  |
 *            main() 結束
 *                  |
 *            靜態成員解構
 *    程式結束
 *
 *  關鍵區別：
 *    - 普通成員：隨對象創建而生，隨對象銷毀而亡
 *    - 靜態成員：隨程式啟動而生，隨程式結束而亡，壽命超過任何單一對象
 */

// --- 輔助類別：追蹤建構/解構時機 ---
class Tracker {
private:
    string label_;

public:
    Tracker(const string& label) : label_(label) {
        cout << "  [建構] " << label_ << endl;
    }
    ~Tracker() {
        cout << "  [解構] " << label_ << endl;
    }

    void ping() const {
        cout << "  " << label_ << " 存活中" << endl;
    }
};

// --- 範例：靜態成員 vs 普通成員的生命週期對比 ---
class LifecycleDemo {
public:
    // 靜態成員：程式開始時初始化，程式結束時才銷毀
    inline static Tracker staticTracker{"靜態成員 Tracker"};

    // 普通成員：隨對象創建/銷毀
    Tracker memberTracker;

    LifecycleDemo(const string& name) : memberTracker("普通成員 " + name) {
        cout << "  [建構] LifecycleDemo " << name << endl;
    }

    ~LifecycleDemo() {
        cout << "  [解構] LifecycleDemo" << endl;
    }
};

void demo_section4() {
    cout << "\n==============================" << endl;
    cout << "  第四節：靜態成員的生命週期" << endl;
    cout << "==============================" << endl;
    cout << "(靜態成員已在 main 之前初始化)" << endl;

    cout << "\n--- 創建對象 ---" << endl;
    {
        LifecycleDemo obj("測試");

        cout << "\n--- 使用中 ---" << endl;
        LifecycleDemo::staticTracker.ping();   // 靜態成員存活
        obj.memberTracker.ping();               // 普通成員存活

        cout << "\n--- 作用域結束 ---" << endl;
    }
    // obj 已銷毀（普通成員也被解構），但靜態成員還活著

    cout << "\n--- obj 已銷毀，靜態成員仍在 ---" << endl;
    LifecycleDemo::staticTracker.ping();       // 靜態成員依然存活！
}


/* ============================================================================
 *  第五節：靜態成員與訪問控制（private / public）
 * ============================================================================
 *
 *  靜態成員同樣受 public / private / protected 訪問控制：
 *    - private 靜態成員：外部不能直接訪問，必須通過公開的靜態函數（getter/setter）
 *    - public 靜態成員：外部可以直接訪問
 *
 *  最佳實踐：
 *    - 將靜態成員設為 private（封裝保護）
 *    - 提供 public 的靜態 getter/setter 函數來控制訪問
 *    - 在 setter 中加入驗證邏輯，防止非法修改
 *
 *  靜態成員函數的特點（詳見第 25 課）：
 *    - 不需要對象就能調用：BankAccount::getAccountCount()
 *    - 沒有 this 指標
 *    - 只能直接訪問靜態成員，不能訪問普通成員
 */

// --- 範例：銀行帳戶（私有靜態成員 + 公開靜態 getter/setter）---
class BankAccount {
private:
    string owner_;       // 普通成員：帳戶持有人
    double balance_;     // 普通成員：帳戶餘額

    // 私有靜態成員：外部不能直接訪問
    inline static double totalDeposits_ = 0.0;  // 銀行總存款
    inline static int accountCount_ = 0;         // 帳戶總數
    inline static double interestRate_ = 0.03;   // 共享利率（3%）

public:
    BankAccount(const string& owner, double initial)
        : owner_(owner), balance_(initial > 0 ? initial : 0)
    {
        accountCount_++;                // 帳戶數加 1
        totalDeposits_ += balance_;     // 總存款增加
        cout << "  [開戶] " << owner_ << " 存入 " << balance_ << endl;
    }

    void deposit(double amount) {
        if (amount <= 0) return;
        balance_ += amount;
        totalDeposits_ += amount;
    }

    double getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }

    // 計算利息——使用共享的靜態利率
    // 這展示了普通成員函數如何同時訪問普通成員和靜態成員
    double calculateInterest() const {
        return balance_ * interestRate_;    // 個人餘額 * 共享利率
    }

    // 公開靜態 getter：安全地讀取私有靜態數據
    static int getAccountCount() { return accountCount_; }
    static double getTotalDeposits() { return totalDeposits_; }
    static double getInterestRate() { return interestRate_; }

    // 公開靜態 setter（帶驗證）：安全地修改私有靜態數據
    static void setInterestRate(double rate) {
        if (rate < 0 || rate > 0.20) {      // 驗證利率範圍
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

void demo_section5() {
    cout << "\n==============================" << endl;
    cout << "  第五節：靜態成員與訪問控制" << endl;
    cout << "==============================" << endl;

    cout << "\n--- 開戶 ---" << endl;
    BankAccount a1("陳信安", 10000);
    BankAccount a2("王小明", 5000);
    BankAccount a3("李大華", 20000);

    // 通過公開的靜態函數訪問私有靜態數據
    cout << "\n--- 銀行統計 ---" << endl;
    cout << "  帳戶數：" << BankAccount::getAccountCount() << endl;       // 3
    cout << "  總存款：" << BankAccount::getTotalDeposits() << endl;      // 35000
    cout << "  當前利率：" << (BankAccount::getInterestRate() * 100) << "%" << endl; // 3%

    // 計算各帳戶利息（利率 3%）
    cout << "\n--- 利息計算（利率 3%）---" << endl;
    a1.printStatement();    // 10000 * 0.03 = 300
    a2.printStatement();    // 5000 * 0.03 = 150
    a3.printStatement();    // 20000 * 0.03 = 600

    // 調整利率——影響所有帳戶（因為利率是共享的靜態成員）
    cout << "\n--- 調整利率 ---" << endl;
    BankAccount::setInterestRate(0.05);     // 合法：5%

    cout << "\n--- 利息計算（利率 5%）---" << endl;
    a1.printStatement();    // 10000 * 0.05 = 500
    a2.printStatement();    // 5000 * 0.05 = 250
    a3.printStatement();    // 20000 * 0.05 = 1000

    // 非法利率——被 setter 中的驗證攔截
    BankAccount::setInterestRate(0.50);     // 被攔截！超過 20%

    // BankAccount::totalDeposits_ = 0;     // 編譯錯誤！private 不可直接訪問
}


/* ============================================================================
 *  第六節：sizeof 與靜態成員——不佔對象空間
 * ============================================================================
 *
 *  靜態成員不計入 sizeof(對象) 的大小：
 *    - 靜態成員存放在「靜態/全域存儲區」，不在對象內部
 *    - 無論有多少個靜態成員，對象大小都不會增加
 *    - 這是因為靜態成員屬於類別，不屬於對象
 *
 *  這意味著：
 *    - 使用靜態成員不會增加每個對象的記憶體開銷
 *    - 適合用於需要所有對象共享的數據（如配置、計數器等）
 */

// --- 範例：對比有無靜態成員的類別大小 ---
class WithStatic {
    int a_;                             // 4 bytes
    int b_;                             // 4 bytes
    inline static int shared_ = 0;      // 不計入 sizeof（存在靜態區）
    inline static double config_ = 0;   // 不計入 sizeof（存在靜態區）
};

class WithoutStatic {
    int a_;     // 4 bytes
    int b_;     // 4 bytes
};

void demo_section6() {
    cout << "\n==============================" << endl;
    cout << "  第六節：sizeof 與靜態成員" << endl;
    cout << "==============================" << endl;

    cout << "  WithStatic 大小：" << sizeof(WithStatic) << " bytes" << endl;
    cout << "  WithoutStatic 大小：" << sizeof(WithoutStatic) << " bytes" << endl;
    cout << "  兩者相同！靜態成員不佔對象空間。" << endl;
    // 兩者都是 8 bytes（兩個 int），靜態成員的空間不算在對象裡
}


/* ============================================================================
 *  第七節：constexpr static——編譯期常量
 * ============================================================================
 *
 *  constexpr static 在編譯期就確定了值，是最高效的常量定義方式。
 *
 *  C++17 以後，constexpr static 成員變數隱含 inline，不需要在類別外定義。
 *
 *  優點：
 *    - 編譯期計算，零執行期開銷
 *    - 可以用在需要編譯期常量的地方（如陣列大小、模板參數）
 *    - 其他 constexpr 常量可以基於已有的 constexpr 常量計算
 *
 *  三種靜態常量的比較：
 *
 *    static const int X = 10;
 *      -> 可以類別內初始化（僅限整數型別）
 *      -> 執行期常量
 *
 *    inline static const string S = "hello";
 *      -> C++17，任何型別
 *      -> 執行期常量
 *
 *    static constexpr double PI = 3.14;
 *      -> 編譯期常量（最高效）
 *      -> 可以用在陣列大小、模板參數等需要編譯期值的地方
 */

// --- 範例：數學常數類別（constexpr static）---
class MathConstants {
public:
    // constexpr static：編譯期常量
    static constexpr double PI = 3.14159265358979;
    static constexpr double E  = 2.71828182845905;
    static constexpr int    MAX_DIMENSION = 3;

    // 編譯期計算：基於已有的 constexpr 常量推導新常量
    static constexpr double TWO_PI = PI * 2.0;         // 編譯期計算完成
    static constexpr double PI_SQUARED = PI * PI;       // 編譯期計算完成

    // 靜態函數使用 constexpr 常量
    static double circleArea(double r) {
        return PI * r * r;
    }

    static double sphereVolume(double r) {
        return (4.0 / 3.0) * PI * r * r * r;
    }
};

// constexpr static 不需要類別外定義（C++17）
// 下面這行如果寫了反而會導致 multiple definition 錯誤：
// constexpr double MathConstants::PI;  // 不需要！

void demo_section7() {
    cout << "\n==============================" << endl;
    cout << "  第七節：constexpr static" << endl;
    cout << "==============================" << endl;

    cout << "  PI = " << MathConstants::PI << endl;
    cout << "  E  = " << MathConstants::E << endl;
    cout << "  2*PI = " << MathConstants::TWO_PI << endl;
    cout << "  PI^2 = " << MathConstants::PI_SQUARED << endl;

    cout << "\n  圓面積(r=5)：" << MathConstants::circleArea(5) << endl;
    cout << "  球體積(r=3)：" << MathConstants::sphereVolume(3) << endl;

    // constexpr 可以用在編譯期需要常量的地方（如陣列大小）
    int arr[MathConstants::MAX_DIMENSION] = {1, 2, 3};
    cout << "\n  陣列大小：" << sizeof(arr) / sizeof(arr[0]) << endl;
}


/* ============================================================================
 *  第八節：綜合範例——遊戲實體管理系統
 * ============================================================================
 *
 *  本節整合了前面所有概念，展示一個完整的實體管理系統：
 *    - inline static 成員變數：nextId_, activeCount_, totalCreated_, totalDestroyed_
 *    - constexpr static 常量：MAX_ENTITIES
 *    - 建構/解構函數中更新靜態計數器
 *    - 靜態查詢函數：printStatistics()
 *    - 對象的啟用/停用影響活躍計數
 *    - 棧對象離開作用域自動解構 vs 堆對象手動 delete
 */

// --- 範例：遊戲實體管理系統 ---
class Entity {
private:
    string name_;           // 實體名稱
    string type_;           // 實體類型（玩家、NPC、敵人、陷阱...）
    int id_;                // 唯一 ID（由靜態 nextId_ 自動分配）
    bool active_;           // 是否處於活躍狀態

    // ====== 靜態成員：類別級別的管理數據 ======
    inline static int nextId_ = 1;              // 下一個可用 ID
    inline static int activeCount_ = 0;          // 目前活躍的實體數量
    inline static int totalCreated_ = 0;         // 總共創建過的實體數量
    inline static int totalDestroyed_ = 0;       // 總共銷毀過的實體數量

    // 靜態常量：最大實體數量上限
    static constexpr int MAX_ENTITIES = 100;

public:
    Entity(const string& name, const string& type)
        : name_(name), type_(type), id_(nextId_++), active_(true)
    {
        totalCreated_++;
        activeCount_++;

        // 超過上限時發出警告（利用 constexpr 常量進行比較）
        if (activeCount_ > MAX_ENTITIES) {
            cout << "  警告：實體數量超過上限 " << MAX_ENTITIES << "！" << endl;
        }

        cout << "  [+] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已創建 [活躍:" << activeCount_ << "]" << endl;
    }

    ~Entity() {
        if (active_) {          // 只有活躍的實體銷毀時才減少活躍計數
            activeCount_--;
        }
        totalDestroyed_++;
        cout << "  [-] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已銷毀 [活躍:" << activeCount_ << "]" << endl;
    }

    // 停用實體（不銷毀對象，但不計入活躍數）
    void deactivate() {
        if (active_) {
            active_ = false;
            activeCount_--;
            cout << "  [x] " << name_ << " 已停用 [活躍:"
                 << activeCount_ << "]" << endl;
        }
    }

    // 重新啟用實體
    void activate() {
        if (!active_) {
            active_ = true;
            activeCount_++;
            cout << "  [o] " << name_ << " 已重新啟用 [活躍:"
                 << activeCount_ << "]" << endl;
        }
    }

    // Getter
    const string& getName() const { return name_; }
    int getId() const { return id_; }
    bool isActive() const { return active_; }

    // ====== 靜態查詢函數 ======
    static int getActiveCount() { return activeCount_; }
    static int getTotalCreated() { return totalCreated_; }
    static int getTotalDestroyed() { return totalDestroyed_; }
    static constexpr int getMaxEntities() { return MAX_ENTITIES; }

    // 打印全局統計信息
    static void printStatistics() {
        cout << "  +--------------------------+" << endl;
        cout << "  | 實體管理統計              |" << endl;
        cout << "  | 已創建：" << totalCreated_ << endl;
        cout << "  | 已銷毀：" << totalDestroyed_ << endl;
        cout << "  | 活躍中：" << activeCount_ << endl;
        cout << "  | 上限：  " << MAX_ENTITIES << endl;
        cout << "  +--------------------------+" << endl;
    }
};

void demo_section8() {
    cout << "\n==============================" << endl;
    cout << "  第八節：綜合範例" << endl;
    cout << "==============================" << endl;

    // 初始統計
    cout << "\n=== 初始狀態 ===" << endl;
    Entity::printStatistics();

    // 創建堆上的實體（需要手動 delete）
    cout << "\n=== 創建實體 ===" << endl;
    Entity* player = new Entity("勇者", "玩家");
    Entity* npc = new Entity("村長", "NPC");

    {
        // 創建棧上的實體（離開作用域時自動銷毀）
        cout << "\n=== 進入戰鬥區域 ===" << endl;
        Entity enemy1("哥布林", "敵人");
        Entity enemy2("骷髏兵", "敵人");
        Entity trap("地刺陷阱", "陷阱");

        Entity::printStatistics();      // 5 個活躍實體

        // 停用陷阱（觸發後不再活躍，但對象仍然存在）
        cout << "\n--- 陷阱觸發 ---" << endl;
        trap.deactivate();              // activeCount_ 減 1
        Entity::printStatistics();      // 4 個活躍實體

        cout << "\n--- 離開戰鬥區域 ---" << endl;
    }
    // enemy1, enemy2, trap 的解構函數被自動調用
    // 注意：trap 已停用，解構時不會再減 activeCount_

    cout << "\n=== 回到安全區 ===" << endl;
    Entity::printStatistics();          // 2 個活躍實體（player, npc）

    // 手動清理堆上的對象
    cout << "\n=== 清理 ===" << endl;
    delete npc;
    delete player;

    cout << "\n=== 最終統計 ===" << endl;
    Entity::printStatistics();          // 0 個活躍實體，5 個已創建，5 個已銷毀
}


/* ============================================================================
 *  附錄：普通成員 vs 靜態成員 對比總結表
 * ============================================================================
 *
 *  +----------------+------------------+------------------+
 *  | 特性           | 普通成員變數      | 靜態成員變數      |
 *  +----------------+------------------+------------------+
 *  | 歸屬           | 屬於對象          | 屬於類別          |
 *  | 份數           | 每個對象一份      | 整個類別一份      |
 *  | 存儲位置       | 對象內部（棧/堆） | 靜態存儲區        |
 *  | 計入 sizeof    | 是               | 否               |
 *  | 生命週期       | 與對象相同        | 與程式相同        |
 *  | 訪問方式       | obj.member       | Class::member    |
 *  | 需要對象才能用 | 是               | 否               |
 *  | this 指標      | 通過 this 訪問   | 沒有 this        |
 *  | 類別外定義     | 不需要           | 需要（或 inline） |
 *  +----------------+------------------+------------------+
 *
 *  靜態成員變數的常見用途：
 *    1. 對象計數器：static int count_;  追蹤目前有多少個對象存活
 *    2. 唯一 ID 產生器：static int nextId_;  每個新對象自動獲得遞增 ID
 *    3. 全域配置：static double gravity_;  所有物理對象共用的重力值
 *    4. 共享常量：static const int MAX_HP = 9999;
 *    5. 快取/共享資源：static map<string, Texture> textureCache_;
 *
 * ============================================================================
 */


// ============================================================================
//  主函數：依序執行所有範例
// ============================================================================
int main() {
    cout << "============================================" << endl;
    cout << "  第 24 課：類別內的靜態成員變數" << endl;
    cout << "  完整複習總結" << endl;
    cout << "============================================" << endl;

    demo_section1();    // 靜態成員變數基礎（傳統語法）
    demo_section2();    // C++17 inline static
    demo_section3();    // 兩種訪問方式
    demo_section4();    // 生命週期
    demo_section5();    // 訪問控制
    demo_section6();    // sizeof 與靜態成員
    demo_section7();    // constexpr static
    demo_section8();    // 綜合範例

    cout << "\n============================================" << endl;
    cout << "  全部範例執行完畢！" << endl;
    cout << "============================================" << endl;

    return 0;
}
