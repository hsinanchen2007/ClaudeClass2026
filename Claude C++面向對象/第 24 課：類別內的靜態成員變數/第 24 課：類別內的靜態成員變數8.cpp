/*
# 第 24 課：類別內的靜態成員變數

---

## 24.1 問題的起源：屬於「整個類別」的數據

到目前為止，我們學到的所有成員變數都是**屬於單一對象**的——每個對象都有自己獨立的一份。但有些數據在邏輯上不屬於任何一個對象，而是屬於**整個類別**：

```
普通成員變數（每個對象一份）：
  ┌──────────┐  ┌──────────┐  ┌──────────┐
  │ 勇者      │  │ 法師      │  │ 弓箭手    │
  │ hp_: 100  │  │ hp_: 80   │  │ hp_: 90   │
  │ atk_: 25  │  │ atk_: 40  │  │ atk_: 35  │
  └──────────┘  └──────────┘  └──────────┘
  每個角色都有自己的 hp_ 和 atk_

靜態成員變數（整個類別共享一份）：
  ┌──────────────────────────────────────┐
  │ Player 類別                          │
  │ static int totalCount_ = 3;          │  ← 所有對象共享
  │ static int maxLevel_ = 100;          │  ← 所有對象共享
  └──────────────────────────────────────┘
       ↑           ↑           ↑
  ┌──────────┐  ┌──────────┐  ┌──────────┐
  │ 勇者      │  │ 法師      │  │ 弓箭手    │
  └──────────┘  └──────────┘  └──────────┘
  三個對象都「看到」同一個 totalCount_ 和 maxLevel_
```

---

## 24.2 靜態成員變數的語法

```
聲明（在類別內）：
  static 數據類型 變數名;

定義與初始化（在類別外，通常在 .cpp 文件中）：
  數據類型 類別名::變數名 = 初始值;
```

```cpp
#include <iostream>
#include <string>
using namespace std;

class Soldier {
private:
    string name_;
    int id_;

    // ====== 靜態成員變數：聲明 ======
    static int totalCount_;     // 所有士兵的總數
    static int nextId_;         // 下一個可用的 ID

public:
    Soldier(const string& name)
        : name_(name), id_(nextId_++)
    {
        totalCount_++;
        cout << "  [入伍] " << name_ << " (ID:" << id_
             << " 總人數:" << totalCount_ << ")" << endl;
    }

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

// ====== 靜態成員變數：定義與初始化（類別外）======
int Soldier::totalCount_ = 0;
int Soldier::nextId_ = 1001;

int main() {
    cout << "=== 靜態成員變數基礎 ===" << endl;

    cout << "\n--- 創建士兵 ---" << endl;
    Soldier s1("阿強");
    Soldier s2("阿明");
    Soldier s3("阿華");

    cout << "\n--- 報到 ---" << endl;
    s1.report();
    s2.report();
    s3.report();
    s1.showTotal();

    cout << "\n--- 作用域結束，逆序解構 ---" << endl;
    // s3、s2、s1 依次解構

    return 0;
}
```

### 預期輸出

```
=== 靜態成員變數基礎 ===

--- 創建士兵 ---
  [入伍] 阿強 (ID:1001 總人數:1)
  [入伍] 阿明 (ID:1002 總人數:2)
  [入伍] 阿華 (ID:1003 總人數:3)

--- 報到 ---
  士兵 阿強 (ID:1001) 報到！
  士兵 阿明 (ID:1002) 報到！
  士兵 阿華 (ID:1003) 報到！
  目前總人數：3

--- 作用域結束，逆序解構 ---
  [退役] 阿華 (ID:1003 總人數:2)
  [退役] 阿明 (ID:1002 總人數:1)
  [退役] 阿強 (ID:1001 總人數:0)
```

---

## 24.3 為什麼需要類別外定義？

這是初學者最常犯的錯誤——忘記在類別外定義靜態成員：

```
類別內的 static int totalCount_;
  → 這只是「聲明」（declaration）
  → 告訴編譯器「這個變數存在」

類別外的 int Soldier::totalCount_ = 0;
  → 這是「定義」（definition）
  → 在記憶體中真正分配空間

如果只有聲明沒有定義 → 連結錯誤（linker error）！
```

```
錯誤訊息示例：
  undefined reference to `Soldier::totalCount_'
```

原因在於靜態成員的存儲位置：

```
記憶體佈局：

棧（Stack）：        堆（Heap）：         靜態/全域區：
┌────────────┐      ┌────────────┐      ┌──────────────────┐
│ s1 對象     │      │            │      │ totalCount_ = 0  │
│ ┌────────┐ │      │            │      │ nextId_ = 1001   │
│ │name_   │ │      │            │      └──────────────────┘
│ │id_     │ │      │            │       ↑ 靜態成員住在這裡
│ └────────┘ │      │            │       不屬於任何對象
│ s2 對象     │      │            │
│ ┌────────┐ │      │            │
│ │name_   │ │      │            │
│ │id_     │ │      │            │
│ └────────┘ │      │            │
└────────────┘      └────────────┘
```

靜態成員不住在棧上的對象裡面，它住在**靜態/全域存儲區**——所以需要在類別外單獨定義。

---

## 24.4 C++17 inline static：省去類別外定義

C++17 引入了 `inline` 關鍵字，可以在類別內直接初始化靜態成員，不需要類別外定義：

```cpp
#include <iostream>
#include <string>
using namespace std;

class GameConfig {
public:
    // C++17 inline static：直接在類別內定義並初始化
    inline static int maxPlayers = 4;
    inline static double gravity = 9.8;
    inline static string version = "1.0.3";

    // const static 整數型別：即使 C++11 也可以類別內初始化
    static const int MAX_LEVEL = 100;
    static const int MAX_ITEMS = 999;

    // C++17 inline static const 也可以用於非整數型別
    inline static const string GAME_NAME = "冒險世界";

    static void printConfig() {
        cout << "  遊戲：" << GAME_NAME << " v" << version << endl;
        cout << "  最大玩家數：" << maxPlayers << endl;
        cout << "  重力：" << gravity << endl;
        cout << "  最大等級：" << MAX_LEVEL << endl;
        cout << "  最大物品：" << MAX_ITEMS << endl;
    }
};

// 不需要類別外定義了！（C++17）

int main() {
    cout << "=== C++17 inline static ===" << endl;

    GameConfig::printConfig();

    // 可以修改非 const 的靜態成員
    cout << "\n--- 修改配置 ---" << endl;
    GameConfig::maxPlayers = 8;
    GameConfig::gravity = 15.0;
    GameConfig::version = "2.0.0";
    GameConfig::printConfig();

    // GameConfig::MAX_LEVEL = 200;  // ❌ 編譯錯誤！const

    return 0;
}
```

### 預期輸出

```
=== C++17 inline static ===
  遊戲：冒險世界 v1.0.3
  最大玩家數：4
  重力：9.8
  最大等級：100
  最大物品：999

--- 修改配置 ---
  遊戲：冒險世界 v2.0.0
  最大玩家數：8
  重力：15
  最大等級：100
  最大物品：999
```

三種初始化方式的比較：

```
方式 1（C++11 之前）：
  類別內：  static int count_;
  類別外：  int MyClass::count_ = 0;

方式 2（C++11）：
  類別內：  static const int MAX = 100;    // 只限 const 整數
  不需要類別外定義

方式 3（C++17，推薦）：
  類別內：  inline static int count_ = 0;  // 任何類型都行
  不需要類別外定義
```

---

## 24.5 靜態成員變數的訪問方式

靜態成員可以通過**兩種方式**訪問：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Enemy {
public:
    inline static int totalKilled = 0;
    inline static int totalSpawned = 0;

private:
    string name_;
    bool alive_;

public:
    Enemy(const string& name) : name_(name), alive_(true) {
        totalSpawned++;
    }

    void kill() {
        if (alive_) {
            alive_ = false;
            totalKilled++;
            cout << "  " << name_ << " 被擊殺！" << endl;
        }
    }

    const string& getName() const { return name_; }
};

int main() {
    cout << "=== 靜態成員的訪問方式 ===" << endl;

    Enemy e1("哥布林");
    Enemy e2("骷髏兵");
    Enemy e3("狼人");

    e1.kill();
    e3.kill();

    cout << "\n--- 方式 1：通過類別名訪問（推薦）---" << endl;
    cout << "  生成數：" << Enemy::totalSpawned << endl;
    cout << "  擊殺數：" << Enemy::totalKilled << endl;

    cout << "\n--- 方式 2：通過對象訪問（不推薦）---" << endl;
    cout << "  生成數：" << e1.totalSpawned << endl;
    cout << "  擊殺數：" << e2.totalKilled << endl;

    // 方式 2 的問題：看起來像是對象自己的數據
    // 但實際上 e1.totalSpawned 和 e2.totalSpawned 是同一個變數
    cout << "\n--- 驗證：同一個變數 ---" << endl;
    cout << "  e1.totalKilled = " << e1.totalKilled << endl;
    cout << "  e2.totalKilled = " << e2.totalKilled << endl;
    cout << "  e3.totalKilled = " << e3.totalKilled << endl;
    cout << "  都是同一個值！" << endl;

    return 0;
}
```

### 預期輸出

```
=== 靜態成員的訪問方式 ===
  哥布林 被擊殺！
  狼人 被擊殺！

--- 方式 1：通過類別名訪問（推薦）---
  生成數：3
  擊殺數：2

--- 方式 2：通過對象訪問（不推薦）---
  生成數：3
  擊殺數：2

--- 驗證：同一個變數 ---
  e1.totalKilled = 2
  e2.totalKilled = 2
  e3.totalKilled = 2
  都是同一個值！
```

**推薦用 `類別名::靜態成員` 的方式訪問**，因為這清楚地表明它是屬於類別的，不是屬於某個對象的。

---

## 24.6 靜態成員變數的生命週期

靜態成員的生命週期與全域變數相同——**程式開始時初始化，程式結束時銷毀**：

```cpp
#include <iostream>
#include <string>
using namespace std;

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

class MyClass {
public:
    // 靜態成員：程式開始時初始化
    inline static Tracker staticTracker{"靜態成員 Tracker"};

    // 普通成員
    Tracker memberTracker;

    MyClass(const string& name) : memberTracker("普通成員 " + name) {
        cout << "  [建構] MyClass " << name << endl;
    }

    ~MyClass() {
        cout << "  [解構] MyClass" << endl;
    }
};

int main() {
    cout << "=== 靜態成員的生命週期 ===" << endl;
    cout << "(靜態成員已在 main 之前初始化)" << endl;

    cout << "\n--- 創建對象 ---" << endl;
    {
        MyClass obj("測試");

        cout << "\n--- 使用中 ---" << endl;
        MyClass::staticTracker.ping();
        obj.memberTracker.ping();

        cout << "\n--- 作用域結束 ---" << endl;
    }
    // obj 已銷毀，但靜態成員還活著

    cout << "\n--- obj 已銷毀，靜態成員仍在 ---" << endl;
    MyClass::staticTracker.ping();

    cout << "\n--- main 結束 ---" << endl;
    return 0;
}
// 程式結束後，靜態成員才會被銷毀
```

### 預期輸出

```
=== 靜態成員的生命週期 ===
(靜態成員已在 main 之前初始化)

--- 創建對象 ---
  [建構] 靜態成員 Tracker
  [建構] 普通成員 測試
  [建構] MyClass 測試

--- 使用中 ---
  靜態成員 Tracker 存活中
  普通成員 測試 存活中

--- 作用域結束 ---
  [解構] MyClass
  [解構] 普通成員 測試

--- obj 已銷毀，靜態成員仍在 ---
  靜態成員 Tracker 存活中

--- main 結束 ---
  [解構] 靜態成員 Tracker
```

```
時間線：
  程式啟動 ──→ 靜態成員初始化
                   ↓
              main() 開始
                   ↓
              創建 obj → 普通成員初始化
                   ↓
              obj 銷毀 → 普通成員解構
                   ↓
              main() 結束
                   ↓
              靜態成員解構
  程式結束 ←──┘
```

---

## 24.7 靜態成員與訪問控制

靜態成員同樣受 `public` / `private` / `protected` 控制：

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner_;
    double balance_;

    // 私有靜態成員：外部不能直接訪問
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

    return 0;
}
```

### 預期輸出

```
=== 靜態成員與訪問控制 ===

--- 開戶 ---
  [開戶] 陳信安 存入 10000
  [開戶] 王小明 存入 5000
  [開戶] 李大華 存入 20000

--- 銀行統計 ---
  帳戶數：3
  總存款：35000
  當前利率：3%

--- 利息計算（利率 3%）---
  陳信安：餘額 10000，利息 300
  王小明：餘額 5000，利息 150
  李大華：餘額 20000，利息 600

--- 調整利率 ---
  利率已調整為 5%

--- 利息計算（利率 5%）---
  陳信安：餘額 10000，利息 500
  王小明：餘額 5000，利息 250
  李大華：餘額 20000，利息 1000
  利率必須在 0%~20% 之間！
```

注意利率調整後，**所有帳戶的利息計算都受到影響**——因為它們共享同一個 `interestRate_`。

---

## 24.8 靜態成員變數的常見用途

```
用途 1：對象計數器
  static int count_;  // 跟蹤目前有多少個對象存活

用途 2：唯一 ID 產生器
  static int nextId_; // 每個新對象自動獲得遞增 ID

用途 3：全域配置
  static double gravity_;     // 所有物理對象共用的重力值
  static int maxConnections_; // 最大連接數

用途 4：共享常量
  static const int MAX_HP = 9999;
  static const double PI = 3.14159;

用途 5：快取 / 共享資源
  static map<string, Texture> textureCache_;  // 紋理快取
```

---

## 24.9 sizeof 與靜態成員

靜態成員**不計入對象的大小**：

```cpp
#include <iostream>
using namespace std;

class WithStatic {
    int a_;                          // 4 bytes
    int b_;                          // 4 bytes
    inline static int shared_ = 0;   // 不計入 sizeof
    inline static double config_ = 0; // 不計入 sizeof
};

class WithoutStatic {
    int a_;   // 4 bytes
    int b_;   // 4 bytes
};

int main() {
    cout << "=== sizeof 與靜態成員 ===" << endl;
    cout << "  WithStatic 大小：" << sizeof(WithStatic) << " bytes" << endl;
    cout << "  WithoutStatic 大小：" << sizeof(WithoutStatic) << " bytes" << endl;
    cout << "  兩者相同！靜態成員不佔對象空間。" << endl;

    return 0;
}
```

### 預期輸出

```
=== sizeof 與靜態成員 ===
  WithStatic 大小：8 bytes
  WithoutStatic 大小：8 bytes
  兩者相同！靜態成員不佔對象空間。
```

---

## 24.10 constexpr static：編譯期常量

C++17 還允許 `constexpr static`，它在**編譯期**就確定了值：

```cpp
#include <iostream>
using namespace std;

class MathConstants {
public:
    // constexpr static：編譯期常量，最高效
    static constexpr double PI = 3.14159265358979;
    static constexpr double E  = 2.71828182845905;
    static constexpr int    MAX_DIMENSION = 3;

    // 編譯期計算
    static constexpr double TWO_PI = PI * 2.0;
    static constexpr double PI_SQUARED = PI * PI;

    static double circleArea(double r) {
        return PI * r * r;
    }

    static double sphereVolume(double r) {
        return (4.0 / 3.0) * PI * r * r * r;
    }
};

// constexpr static 不需要類別外定義（C++17）

int main() {
    cout << "=== constexpr static ===" << endl;
    cout << "  PI = " << MathConstants::PI << endl;
    cout << "  E  = " << MathConstants::E << endl;
    cout << "  2*PI = " << MathConstants::TWO_PI << endl;
    cout << "  PI^2 = " << MathConstants::PI_SQUARED << endl;

    cout << "\n  圓面積(r=5)：" << MathConstants::circleArea(5) << endl;
    cout << "  球體積(r=3)：" << MathConstants::sphereVolume(3) << endl;

    // 可以用在編譯期需要常量的地方
    int arr[MathConstants::MAX_DIMENSION] = {1, 2, 3};  // ✅
    cout << "\n  陣列大小：" << sizeof(arr) / sizeof(arr[0]) << endl;

    return 0;
}
```

### 預期輸出

```
=== constexpr static ===
  PI = 3.14159
  E  = 2.71828
  2*PI = 6.28319
  PI^2 = 9.8696
  圓面積(r=5)：78.5398
  球體積(r=3)：113.097
  陣列大小：3
```

各種靜態常量的比較：

```
static const int X = 10;
  → 可以類別內初始化（僅限整數型別）
  → 執行期常量

inline static const string S = "hello";
  → C++17，任何型別
  → 執行期常量

static constexpr double PI = 3.14;
  → 編譯期常量（最高效）
  → 可以用在陣列大小、模板參數等需要編譯期值的地方
```

---

## 24.11 綜合範例：遊戲實體管理系統

```cpp
#include <iostream>
#include <string>
using namespace std;

class Entity {
private:
    string name_;
    string type_;
    int id_;
    bool active_;

    // ====== 靜態成員：類別級別的管理數據 ======
    inline static int nextId_ = 1;
    inline static int activeCount_ = 0;
    inline static int totalCreated_ = 0;
    inline static int totalDestroyed_ = 0;

    // 靜態常量：配置
    static constexpr int MAX_ENTITIES = 100;

public:
    Entity(const string& name, const string& type)
        : name_(name), type_(type), id_(nextId_++), active_(true)
    {
        totalCreated_++;
        activeCount_++;

        if (activeCount_ > MAX_ENTITIES) {
            cout << "  ⚠ 警告：實體數量超過上限 " << MAX_ENTITIES << "！" << endl;
        }

        cout << "  [+] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已創建 [活躍:" << activeCount_ << "]" << endl;
    }

    ~Entity() {
        if (active_) {
            activeCount_--;
        }
        totalDestroyed_++;
        cout << "  [-] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已銷毀 [活躍:" << activeCount_ << "]" << endl;
    }

    // 停用（不銷毀，但不計入活躍數）
    void deactivate() {
        if (active_) {
            active_ = false;
            activeCount_--;
            cout << "  [x] " << name_ << " 已停用 [活躍:"
                 << activeCount_ << "]" << endl;
        }
    }

    // 重新啟用
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

    static void printStatistics() {
        cout << "  ┌─────────────────────────┐" << endl;
        cout << "  │ 實體管理統計             │" << endl;
        cout << "  │ 已創建：" << totalCreated_ << endl;
        cout << "  │ 已銷毀：" << totalDestroyed_ << endl;
        cout << "  │ 活躍中：" << activeCount_ << endl;
        cout << "  │ 上限：  " << MAX_ENTITIES << endl;
        cout << "  └─────────────────────────┘" << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 24 課：靜態成員變數 綜合範例" << endl;
    cout << "============================================" << endl;

    // 初始統計
    cout << "\n=== 初始狀態 ===" << endl;
    Entity::printStatistics();

    // 創建一些實體
    cout << "\n=== 創建實體 ===" << endl;
    Entity* player = new Entity("勇者", "玩家");
    Entity* npc = new Entity("村長", "NPC");

    {
        cout << "\n=== 進入戰鬥區域 ===" << endl;
        Entity enemy1("哥布林", "敵人");
        Entity enemy2("骷髏兵", "敵人");
        Entity trap("地刺陷阱", "陷阱");

        Entity::printStatistics();

        // 停用陷阱（觸發後不再活躍）
        cout << "\n--- 陷阱觸發 ---" << endl;
        trap.deactivate();
        Entity::printStatistics();

        cout << "\n--- 離開戰鬥區域 ---" << endl;
    }
    // enemy1, enemy2, trap 被解構

    cout << "\n=== 回到安全區 ===" << endl;
    Entity::printStatistics();

    // 清理動態對象
    cout << "\n=== 清理 ===" << endl;
    delete npc;
    delete player;

    cout << "\n=== 最終統計 ===" << endl;
    Entity::printStatistics();

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson24 lesson24.cpp
./lesson24
```

### 預期輸出

```
============================================
   第 24 課：靜態成員變數 綜合範例
============================================

=== 初始狀態 ===
  ┌─────────────────────────┐
  │ 實體管理統計             │
  │ 已創建：0
  │ 已銷毀：0
  │ 活躍中：0
  │ 上限：  100
  └─────────────────────────┘

=== 創建實體 ===
  [+] 玩家 "勇者" (ID:1) 已創建 [活躍:1]
  [+] NPC "村長" (ID:2) 已創建 [活躍:2]

=== 進入戰鬥區域 ===
  [+] 敵人 "哥布林" (ID:3) 已創建 [活躍:3]
  [+] 敵人 "骷髏兵" (ID:4) 已創建 [活躍:4]
  [+] 陷阱 "地刺陷阱" (ID:5) 已創建 [活躍:5]
  ┌─────────────────────────┐
  │ 實體管理統計             │
  │ 已創建：5
  │ 已銷毀：0
  │ 活躍中：5
  │ 上限：  100
  └─────────────────────────┘

--- 陷阱觸發 ---
  [x] 地刺陷阱 已停用 [活躍:4]
  ┌─────────────────────────┐
  │ 實體管理統計             │
  │ 已創建：5
  │ 已銷毀：0
  │ 活躍中：4
  │ 上限：  100
  └─────────────────────────┘

--- 離開戰鬥區域 ---
  [-] 陷阱 "地刺陷阱" (ID:5) 已銷毀 [活躍:4]
  [-] 敵人 "骷髏兵" (ID:4) 已銷毀 [活躍:3]
  [-] 敵人 "哥布林" (ID:3) 已銷毀 [活躍:2]

=== 回到安全區 ===
  ┌─────────────────────────┐
  │ 實體管理統計             │
  │ 已創建：5
  │ 已銷毀：3
  │ 活躍中：2
  │ 上限：  100
  └─────────────────────────┘

=== 清理 ===
  [-] NPC "村長" (ID:2) 已銷毀 [活躍:1]
  [-] 玩家 "勇者" (ID:1) 已銷毀 [活躍:0]

=== 最終統計 ===
  ┌─────────────────────────┐
  │ 實體管理統計             │
  │ 已創建：5
  │ 已銷毀：5
  │ 活躍中：0
  │ 上限：  100
  └─────────────────────────┘
```

---

## 24.12 普通成員 vs 靜態成員 對比總結

```
┌────────────────┬──────────────────┬──────────────────┐
│ 特性           │ 普通成員變數      │ 靜態成員變數      │
├────────────────┼──────────────────┼──────────────────┤
│ 歸屬           │ 屬於對象          │ 屬於類別          │
│ 份數           │ 每個對象一份      │ 整個類別一份      │
│ 存儲位置       │ 對象內部（棧/堆）  │ 靜態存儲區        │
│ 計入 sizeof    │ 是               │ 否               │
│ 生命週期       │ 與對象相同        │ 與程式相同        │
│ 訪問方式       │ obj.member       │ Class::member    │
│ 需要對象才能用  │ 是               │ 否               │
│ this 指標      │ 通過 this 訪問    │ 沒有 this        │
│ 類別外定義      │ 不需要           │ 需要（或 inline） │
└────────────────┴──────────────────┴──────────────────┘
```

---

## 24.13 本課重點回顧

| 概念 | 說明 |
|------|------|
| static 成員變數 | 屬於類別，所有對象共享一份 |
| 聲明 vs 定義 | 類別內聲明，類別外定義（或用 `inline static`） |
| inline static（C++17） | 可在類別內直接初始化，推薦使用 |
| constexpr static | 編譯期常量，可用於陣列大小等 |
| 訪問方式 | 推薦 `Class::member`，不推薦 `obj.member` |
| 存儲位置 | 靜態存儲區，不在對象內部 |
| 不計入 sizeof | 靜態成員不影響對象大小 |
| 生命週期 | 程式開始到結束，比任何對象都長 |
| 訪問控制 | 同樣受 public/private/protected 控制 |
| 常見用途 | 計數器、ID 產生器、全域配置、共享常量 |

---

## 24.14 下一課預告

下一課是 **第 25 課：類別內的靜態成員函數**。我們將學習不依賴任何對象就能調用的函數——它們沒有 `this` 指標，只能訪問靜態成員。這與本課的靜態成員變數是完美搭配。

準備好進入 **第 25 課：類別內的靜態成員函數** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

class Entity {
private:
    string name_;
    string type_;
    int id_;
    bool active_;

    // ====== 靜態成員：類別級別的管理數據 ======
    inline static int nextId_ = 1;
    inline static int activeCount_ = 0;
    inline static int totalCreated_ = 0;
    inline static int totalDestroyed_ = 0;

    // 靜態常量：配置
    static constexpr int MAX_ENTITIES = 100;

public:
    Entity(const string& name, const string& type)
        : name_(name), type_(type), id_(nextId_++), active_(true)
    {
        totalCreated_++;
        activeCount_++;

        if (activeCount_ > MAX_ENTITIES) {
            cout << "  ⚠ 警告：實體數量超過上限 " << MAX_ENTITIES << "！" << endl;
        }

        cout << "  [+] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已創建 [活躍:" << activeCount_ << "]" << endl;
    }

    ~Entity() {
        if (active_) {
            activeCount_--;
        }
        totalDestroyed_++;
        cout << "  [-] " << type_ << " \"" << name_ << "\" (ID:"
             << id_ << ") 已銷毀 [活躍:" << activeCount_ << "]" << endl;
    }

    // 停用（不銷毀，但不計入活躍數）
    void deactivate() {
        if (active_) {
            active_ = false;
            activeCount_--;
            cout << "  [x] " << name_ << " 已停用 [活躍:"
                 << activeCount_ << "]" << endl;
        }
    }

    // 重新啟用
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

    static void printStatistics() {
        cout << "  ┌─────────────────────────┐" << endl;
        cout << "  │ 實體管理統計             │" << endl;
        cout << "  │ 已創建：" << totalCreated_ << endl;
        cout << "  │ 已銷毀：" << totalDestroyed_ << endl;
        cout << "  │ 活躍中：" << activeCount_ << endl;
        cout << "  │ 上限：  " << MAX_ENTITIES << endl;
        cout << "  └─────────────────────────┘" << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 24 課：靜態成員變數 綜合範例" << endl;
    cout << "============================================" << endl;

    // 初始統計
    cout << "\n=== 初始狀態 ===" << endl;
    Entity::printStatistics();

    // 創建一些實體
    cout << "\n=== 創建實體 ===" << endl;
    Entity* player = new Entity("勇者", "玩家");
    Entity* npc = new Entity("村長", "NPC");

    {
        cout << "\n=== 進入戰鬥區域 ===" << endl;
        Entity enemy1("哥布林", "敵人");
        Entity enemy2("骷髏兵", "敵人");
        Entity trap("地刺陷阱", "陷阱");

        Entity::printStatistics();

        // 停用陷阱（觸發後不再活躍）
        cout << "\n--- 陷阱觸發 ---" << endl;
        trap.deactivate();
        Entity::printStatistics();

        cout << "\n--- 離開戰鬥區域 ---" << endl;
    }
    // enemy1, enemy2, trap 被解構

    cout << "\n=== 回到安全區 ===" << endl;
    Entity::printStatistics();

    // 清理動態對象
    cout << "\n=== 清理 ===" << endl;
    delete npc;
    delete player;

    cout << "\n=== 最終統計 ===" << endl;
    Entity::printStatistics();

    return 0;
}
