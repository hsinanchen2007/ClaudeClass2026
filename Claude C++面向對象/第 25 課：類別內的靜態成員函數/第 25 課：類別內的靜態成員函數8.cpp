/*
# 第 25 課：類別內的靜態成員函數

---

## 25.1 回顧上一課的問題

上一課我們學了靜態成員變數，也用了一些靜態函數（如 `printStatistics()`）。但我們還沒正式介紹靜態成員函數的完整規則。先看一個問題：

```cpp
class Enemy {
private:
    inline static int totalCount_ = 0;
    string name_;

public:
    Enemy(const string& name) : name_(name) { totalCount_++; }

    // 問題：這個函數需要 this 嗎？
    // 它只用到 totalCount_（靜態的），完全不碰 name_（非靜態的）
    int getTotalCount() const { return totalCount_; }
};

// 使用時：
// Enemy e("哥布林");
// e.getTotalCount();   // 必須先有對象才能調用——不合理
```

`getTotalCount()` 明明只用到靜態數據，卻需要一個對象才能調用。這就是靜態成員函數要解決的問題。

---

## 25.2 靜態成員函數的語法與特性

```
語法：
  static 返回類型 函數名(參數);

特性：
  1. 不依賴任何對象，可以直接用 類別名::函數名() 調用
  2. 沒有 this 指標
  3. 只能訪問靜態成員（變數和函數）
  4. 不能訪問非靜態成員
  5. 不能加 const 修飾（因為沒有 this，const 無意義）
```

```cpp
#include <iostream>
#include <string>
using namespace std;

class Bullet {
private:
    int damage_;
    string type_;

    // 靜態成員變數
    inline static int totalFired_ = 0;
    inline static int totalHit_ = 0;

public:
    Bullet(const string& type, int dmg)
        : type_(type), damage_(dmg)
    {
    }

    // 非靜態函數：有 this，可以訪問一切
    void fire() {
        totalFired_++;
        cout << "  發射 " << type_ << "（傷害:" << damage_ << "）" << endl;
    }

    void hit() {
        totalHit_++;
        cout << "  命中！" << type_ << " 造成 " << damage_ << " 傷害" << endl;
    }

    // ====== 靜態成員函數：沒有 this ======
    static int getTotalFired() { return totalFired_; }
    static int getTotalHit() { return totalHit_; }

    static double getHitRate() {
        if (totalFired_ == 0) return 0.0;
        return static_cast<double>(totalHit_) / totalFired_ * 100.0;
    }

    static void printStats() {
        cout << "  發射：" << totalFired_
             << "  命中：" << totalHit_
             << "  命中率：" << getHitRate() << "%" << endl;

        // 以下會編譯錯誤！靜態函數沒有 this
        // cout << damage_;    // ❌ 不能訪問非靜態成員
        // cout << type_;      // ❌ 不能訪問非靜態成員
        // fire();             // ❌ 不能調用非靜態函數
    }

    static void resetStats() {
        totalFired_ = 0;
        totalHit_ = 0;
        cout << "  統計數據已重置" << endl;
    }
};

int main() {
    cout << "=== 靜態成員函數基礎 ===" << endl;

    Bullet normal("普通彈", 10);
    Bullet fire("火焰彈", 25);

    // 射擊
    normal.fire();
    normal.fire();
    fire.fire();
    normal.hit();
    fire.hit();
    fire.fire();
    fire.hit();

    // 通過類別名調用靜態函數——不需要對象！
    cout << "\n--- 戰鬥統計（類別名調用）---" << endl;
    Bullet::printStats();

    // 也可以通過對象調用（但不推薦）
    cout << "\n--- 通過對象調用（不推薦）---" << endl;
    normal.printStats();   // 和 Bullet::printStats() 完全一樣

    // 重置
    cout << "\n--- 重置 ---" << endl;
    Bullet::resetStats();
    Bullet::printStats();

    return 0;
}
```

### 預期輸出

```
=== 靜態成員函數基礎 ===
  發射 普通彈（傷害:10）
  發射 普通彈（傷害:10）
  發射 火焰彈（傷害:25）
  命中！普通彈 造成 10 傷害
  命中！火焰彈 造成 25 傷害
  發射 火焰彈（傷害:25）
  命中！火焰彈 造成 25 傷害

--- 戰鬥統計（類別名調用）---
  發射：4  命中：3  命中率：75%

--- 通過對象調用（不推薦）---
  發射：4  命中：3  命中率：75%

--- 重置 ---
  統計數據已重置
  發射：0  命中：0  命中率：0%
```

---

## 25.3 為什麼靜態函數不能有 const？

```cpp
class Demo {
public:
    // static void func() const;  // ❌ 編譯錯誤！
    static void func();           // ✅
};
```

原因：

```
const 成員函數的意思是：
  int getHp() const;
  → 等同於：int getHp(const Player* const this);
  → 承諾不通過 this 修改對象

靜態成員函數：
  static int getTotalCount();
  → 根本沒有 this 參數！
  → const 修飾的對象不存在
  → 所以 const 在這裡毫無意義
```

---

## 25.4 靜態函數 vs 非靜態函數的訪問規則

```cpp
#include <iostream>
#include <string>
using namespace std;

class Character {
private:
    string name_;               // 非靜態
    int hp_;                    // 非靜態
    inline static int count_ = 0;  // 靜態

public:
    Character(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        count_++;
    }

    // ====== 非靜態函數：有 this，可以訪問一切 ======
    void printInfo() const {
        cout << "  " << name_ << " HP:" << hp_     // ✅ 非靜態成員
             << " (共 " << count_ << " 人)" << endl; // ✅ 靜態成員也行
    }

    // ====== 靜態函數：沒有 this，只能訪問靜態 ======
    static void printCount() {
        cout << "  角色總數：" << count_ << endl;  // ✅ 靜態成員

        // cout << name_;      // ❌ 哪個對象的 name_？
        // cout << hp_;        // ❌ 哪個對象的 hp_？
        // printInfo();        // ❌ 需要 this 才能調用
    }

    // 靜態函數可以接收對象參數來間接訪問非靜態成員
    static void compare(const Character& a, const Character& b) {
        cout << "  比較：" << a.name_ << "(HP:" << a.hp_ << ") vs "
             << b.name_ << "(HP:" << b.hp_ << ")" << endl;

        // 注意：這裡訪問的是「參數對象」的成員，不是通過 this
        // 因為 compare 是 Character 的成員函數，可以訪問 private
        if (a.hp_ > b.hp_)
            cout << "  → " << a.name_ << " 更強" << endl;
        else if (a.hp_ < b.hp_)
            cout << "  → " << b.name_ << " 更強" << endl;
        else
            cout << "  → 不分上下" << endl;
    }
};

int main() {
    cout << "=== 訪問規則 ===" << endl;

    Character warrior("戰士", 200);
    Character mage("法師", 120);

    cout << "\n--- 非靜態函數（需要對象）---" << endl;
    warrior.printInfo();
    mage.printInfo();

    cout << "\n--- 靜態函數（不需要對象）---" << endl;
    Character::printCount();

    cout << "\n--- 靜態函數接收對象參數 ---" << endl;
    Character::compare(warrior, mage);

    return 0;
}
```

### 預期輸出

```
=== 訪問規則 ===

--- 非靜態函數（需要對象）---
  戰士 HP:200 (共 2 人)
  法師 HP:120 (共 2 人)

--- 靜態函數（不需要對象）---
  角色總數：2

--- 靜態函數接收對象參數 ---
  比較：戰士(HP:200) vs 法師(HP:120)
  → 戰士 更強
```

完整的訪問規則圖：

```
┌──────────────────────────────────────────────────┐
│                訪問規則總表                        │
├─────────────────┬──────────────┬─────────────────┤
│ 調用者           │ 可訪問靜態？  │ 可訪問非靜態？   │
├─────────────────┼──────────────┼─────────────────┤
│ 非靜態成員函數    │ ✅ 可以      │ ✅ 可以（有this）│
│ 靜態成員函數      │ ✅ 可以      │ ❌ 不行（無this）│
│ 外部函數         │ ✅ 如果public│ ✅ 需要對象      │
└─────────────────┴──────────────┴─────────────────┘
```

---

## 25.5 靜態成員函數的典型應用

### 應用一：工廠函數（Factory Function）

靜態函數最經典的用途之一——用靜態函數創建對象：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int price_;

    // 私有建構函數：外部不能直接 new
    Potion(const string& name, int heal, int price)
        : name_(name), healAmount_(heal), price_(price)
    {
    }

public:
    // ====== 靜態工廠函數：提供命名的創建方式 ======
    static Potion createSmall() {
        return Potion("小型藥水", 30, 50);
    }

    static Potion createMedium() {
        return Potion("中型藥水", 70, 120);
    }

    static Potion createLarge() {
        return Potion("大型藥水", 150, 300);
    }

    static Potion createCustom(const string& name, int heal, int price) {
        // 帶驗證的創建
        int safeHeal = (heal > 0 && heal <= 999) ? heal : 50;
        int safePrice = (price > 0 && price <= 9999) ? price : 100;
        return Potion(name, safeHeal, safePrice);
    }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 價格:" << price_ << " 金幣)" << endl;
    }
};

int main() {
    cout << "=== 工廠函數 ===" << endl;

    // 不能直接建構：
    // Potion p("藥水", 50, 100);  // ❌ 建構函數是 private

    // 透過靜態工廠函數創建
    cout << "\n--- 預設藥水 ---" << endl;
    Potion small = Potion::createSmall();
    Potion medium = Potion::createMedium();
    Potion large = Potion::createLarge();

    small.printInfo();
    medium.printInfo();
    large.printInfo();

    cout << "\n--- 自定義藥水 ---" << endl;
    Potion special = Potion::createCustom("秘製靈藥", 500, 2000);
    special.printInfo();

    return 0;
}
```

### 預期輸出

```
=== 工廠函數 ===

--- 預設藥水 ---
  小型藥水 (回復:30 價格:50 金幣)
  中型藥水 (回復:70 價格:120 金幣)
  大型藥水 (回復:150 價格:300 金幣)

--- 自定義藥水 ---
  秘製靈藥 (回復:500 價格:2000 金幣)
```

工廠函數的好處：

```
建構函數的限制：
  Potion(50, 100);     // 50 是什麼？100 是什麼？語義不清

工廠函數的優勢：
  Potion::createSmall();    // 明確：創建小型藥水
  Potion::createLarge();    // 明確：創建大型藥水

  1. 函數名傳達了語義
  2. 可以有多個「建構邏輯」使用相同的參數類型
  3. 可以在內部添加驗證邏輯
  4. 可以控制創建流程
```

---

### 應用二：工具函數（Utility Function）

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

class MathUtil {
public:
    // 全部是靜態函數——這個類別不需要創建對象
    static int clamp(int value, int minVal, int maxVal) {
        if (value < minVal) return minVal;
        if (value > maxVal) return maxVal;
        return value;
    }

    static double lerp(double a, double b, double t) {
        t = (t < 0.0) ? 0.0 : (t > 1.0) ? 1.0 : t;
        return a + (b - a) * t;
    }

    static double distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return sqrt(dx * dx + dy * dy);
    }

    static bool isInRange(double value, double center, double range) {
        return abs(value - center) <= range;
    }

    // 禁止創建對象
    MathUtil() = delete;
};

int main() {
    cout << "=== 工具函數類 ===" << endl;

    // 不需要創建對象，直接用類別名調用
    cout << "  clamp(150, 0, 100) = " << MathUtil::clamp(150, 0, 100) << endl;
    cout << "  clamp(-50, 0, 100) = " << MathUtil::clamp(-50, 0, 100) << endl;

    cout << "  lerp(0, 100, 0.3) = " << MathUtil::lerp(0, 100, 0.3) << endl;
    cout << "  lerp(0, 100, 0.7) = " << MathUtil::lerp(0, 100, 0.7) << endl;

    cout << "  distance(0,0, 3,4) = " << MathUtil::distance(0, 0, 3, 4) << endl;

    cout << "  isInRange(5, 10, 3) = "
         << (MathUtil::isInRange(5, 10, 3) ? "true" : "false") << endl;
    cout << "  isInRange(8, 10, 3) = "
         << (MathUtil::isInRange(8, 10, 3) ? "true" : "false") << endl;

    // MathUtil m;  // ❌ 編譯錯誤！建構函數被 delete

    return 0;
}
```

### 預期輸出

```
=== 工具函數類 ===
  clamp(150, 0, 100) = 100
  clamp(-50, 0, 100) = 0
  lerp(0, 100, 0.3) = 30
  lerp(0, 100, 0.7) = 70
  distance(0,0, 3,4) = 5
  isInRange(5, 10, 3) = false
  isInRange(8, 10, 3) = true
```

---

### 應用三：單例存取點（預告概念）

靜態函數也是實現「單例模式」的關鍵——保證一個類別只有一個實例：

```cpp
#include <iostream>
#include <string>
using namespace std;

class GameManager {
private:
    int score_;
    int level_;
    string currentMap_;

    // 私有建構函數：外部不能創建
    GameManager() : score_(0), level_(1), currentMap_("新手村") {
        cout << "  [GameManager 初始化]" << endl;
    }

public:
    // 禁止拷貝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 靜態函數：獲取唯一實例
    static GameManager& getInstance() {
        static GameManager instance;   // 靜態局部變數，只初始化一次
        return instance;
    }

    // 普通成員函數
    void addScore(int points) {
        score_ += points;
        cout << "  得分 +" << points << " (總分:" << score_ << ")" << endl;
    }

    void nextLevel() {
        level_++;
        cout << "  進入第 " << level_ << " 關" << endl;
    }

    void setMap(const string& map) { currentMap_ = map; }

    void printStatus() const {
        cout << "  [遊戲狀態] 地圖:" << currentMap_
             << " 關卡:" << level_ << " 分數:" << score_ << endl;
    }
};

int main() {
    cout << "=== 單例存取點（靜態函數）===" << endl;

    // 通過靜態函數獲取唯一實例
    cout << "\n--- 第一次存取 ---" << endl;
    GameManager::getInstance().printStatus();

    cout << "\n--- 遊戲進行 ---" << endl;
    GameManager::getInstance().addScore(100);
    GameManager::getInstance().addScore(250);
    GameManager::getInstance().nextLevel();
    GameManager::getInstance().setMap("暗黑森林");

    cout << "\n--- 查看狀態 ---" << endl;
    GameManager::getInstance().printStatus();

    // 驗證是同一個實例
    cout << "\n--- 驗證唯一性 ---" << endl;
    GameManager& ref1 = GameManager::getInstance();
    GameManager& ref2 = GameManager::getInstance();
    cout << "  ref1 地址：" << &ref1 << endl;
    cout << "  ref2 地址：" << &ref2 << endl;
    cout << "  是同一個對象：" << (&ref1 == &ref2 ? "是" : "否") << endl;

    return 0;
}
```

### 預期輸出

```
=== 單例存取點（靜態函數）===

--- 第一次存取 ---
  [GameManager 初始化]
  [遊戲狀態] 地圖:新手村 關卡:1 分數:0

--- 遊戲進行 ---
  得分 +100 (總分:100)
  得分 +250 (總分:350)
  進入第 2 關

--- 查看狀態 ---
  [遊戲狀態] 地圖:暗黑森林 關卡:2 分數:350

--- 驗證唯一性 ---
  ref1 地址：0x7ffXXXXXXXX
  ref2 地址：0x7ffXXXXXXXX
  是同一個對象：是
```

---

## 25.6 靜態函數與非靜態函數的互相調用

```cpp
#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    string module_;
    inline static int logCount_ = 0;
    inline static bool enabled_ = true;

public:
    Logger(const string& module) : module_(module) {}

    // ====== 靜態函數 ======
    static void enable() { enabled_ = true; }
    static void disable() { enabled_ = false; }
    static bool isEnabled() { return enabled_; }
    static int getLogCount() { return logCount_; }

    // 靜態函數可以調用其他靜態函數 ✅
    static void printSystemInfo() {
        cout << "  [系統] 日誌 " << (isEnabled() ? "啟用" : "停用")
             << "，已記錄 " << getLogCount() << " 條" << endl;
    }

    // ====== 非靜態函數 ======

    // 非靜態函數可以調用靜態函數 ✅
    void log(const string& message) const {
        if (!isEnabled()) return;    // 調用靜態函數 ✅
        logCount_++;                 // 訪問靜態變數 ✅
        cout << "  [" << module_ << "] " << message   // 訪問非靜態成員 ✅
             << " (#" << logCount_ << ")" << endl;
    }

    void warn(const string& message) const {
        if (!isEnabled()) return;
        logCount_++;
        cout << "  ⚠ [" << module_ << "] " << message
             << " (#" << logCount_ << ")" << endl;
    }
};

int main() {
    cout << "=== 靜態與非靜態的互動 ===" << endl;

    Logger gameLog("遊戲");
    Logger netLog("網路");

    Logger::printSystemInfo();

    cout << "\n--- 正常記錄 ---" << endl;
    gameLog.log("遊戲啟動");
    netLog.log("連接伺服器");
    gameLog.log("載入地圖");
    netLog.warn("延遲過高");

    Logger::printSystemInfo();

    // 關閉日誌
    cout << "\n--- 關閉日誌 ---" << endl;
    Logger::disable();
    gameLog.log("這條不會顯示");
    netLog.log("這條也不會顯示");
    Logger::printSystemInfo();

    // 重新開啟
    cout << "\n--- 重新開啟 ---" << endl;
    Logger::enable();
    gameLog.log("日誌恢復");
    Logger::printSystemInfo();

    return 0;
}
```

### 預期輸出

```
=== 靜態與非靜態的互動 ===
  [系統] 日誌 啟用，已記錄 0 條

--- 正常記錄 ---
  [遊戲] 遊戲啟動 (#1)
  [網路] 連接伺服器 (#2)
  [遊戲] 載入地圖 (#3)
  ⚠ [網路] 延遲過高 (#4)
  [系統] 日誌 啟用，已記錄 4 條

--- 關閉日誌 ---
  [系統] 日誌 停用，已記錄 4 條

--- 重新開啟 ---
  [遊戲] 日誌恢復 (#5)
  [系統] 日誌 啟用，已記錄 5 條
```

調用關係圖：

```
靜態函數 → 靜態函數    ✅
靜態函數 → 靜態變數    ✅
靜態函數 → 非靜態成員  ❌ （沒有 this）
靜態函數 → 非靜態函數  ❌ （沒有 this）

非靜態函數 → 靜態函數  ✅
非靜態函數 → 靜態變數  ✅
非靜態函數 → 非靜態成員 ✅ （有 this）
非靜態函數 → 非靜態函數 ✅ （有 this）
```

---

## 25.7 靜態函數 vs 全域函數 vs 命名空間函數

你可能會想：靜態函數和全域函數有什麼差別？為什麼不直接用全域函數？

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 方式 1：全域函數 =====
int globalAdd(int a, int b) { return a + b; }
// 問題：名字可能衝突、沒有歸屬感、不能訪問 private

// ===== 方式 2：命名空間函數 =====
namespace MathUtils {
    int add(int a, int b) { return a + b; }
}
// 較好：有命名空間隔離，但不能訪問類別的 private

// ===== 方式 3：類別的靜態函數 =====
class Calculator {
private:
    inline static int operationCount_ = 0;  // 可以有私有狀態

public:
    static int add(int a, int b) {
        operationCount_++;    // 可以維護內部狀態
        return a + b;
    }

    static int getOperationCount() { return operationCount_; }
};
// 最佳：有歸屬、可以訪問 private、可以維護狀態

int main() {
    cout << "=== 三種方式比較 ===" << endl;

    cout << "  全域函數：" << globalAdd(1, 2) << endl;
    cout << "  命名空間：" << MathUtils::add(3, 4) << endl;
    cout << "  靜態函數：" << Calculator::add(5, 6) << endl;

    Calculator::add(7, 8);
    Calculator::add(9, 10);
    cout << "  Calculator 運算次數：" << Calculator::getOperationCount() << endl;

    return 0;
}
```

### 預期輸出

```
=== 三種方式比較 ===
  全域函數：3
  命名空間：7
  靜態函數：11
  Calculator 運算次數：3
```

何時用哪種：

```
全域函數：
  → 極少使用，容易造成命名衝突
  → 只在 C 相容性需要時使用

命名空間函數：
  → 純工具函數，不需要訪問任何類別的 private
  → 如：字串處理、數學運算

類別靜態函數：
  → 與類別邏輯相關的操作
  → 需要訪問類別的 private 成員
  → 需要維護類別級別的狀態
  → 工廠函數、單例存取、統計功能
```

---

## 25.8 綜合範例：遊戲成就系統

```cpp
#include <iostream>
#include <string>
using namespace std;

class Achievement {
private:
    string name_;
    string description_;
    int points_;
    bool unlocked_;

    // 靜態：全域成就統計
    inline static int totalAchievements_ = 0;
    inline static int unlockedCount_ = 0;
    inline static int totalPoints_ = 0;
    inline static int earnedPoints_ = 0;

public:
    Achievement(const string& name, const string& desc, int points)
        : name_(name), description_(desc)
        , points_(points > 0 ? points : 10)
        , unlocked_(false)
    {
        totalAchievements_++;
        totalPoints_ += points_;
    }

    // ====== 非靜態函數 ======
    bool unlock() {
        if (unlocked_) {
            cout << "  「" << name_ << "」已經解鎖過了" << endl;
            return false;
        }
        unlocked_ = true;
        unlockedCount_++;
        earnedPoints_ += points_;
        cout << "  🏆 成就解鎖！「" << name_ << "」 +"
             << points_ << " 點" << endl;
        cout << "    " << description_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  " << (unlocked_ ? "🏆" : "🔒") << " "
             << name_ << " (" << points_ << "點)";
        if (!unlocked_) cout << " — " << description_;
        cout << endl;
    }

    bool isUnlocked() const { return unlocked_; }
    const string& getName() const { return name_; }
    int getPoints() const { return points_; }

    // ====== 靜態函數：類別級別操作 ======

    static int getTotalAchievements() { return totalAchievements_; }
    static int getUnlockedCount() { return unlockedCount_; }

    static double getCompletionRate() {
        if (totalAchievements_ == 0) return 0.0;
        return static_cast<double>(unlockedCount_) / totalAchievements_ * 100.0;
    }

    static void printProgress() {
        cout << "  ╔════════════════════════════╗" << endl;
        cout << "  ║      成 就 進 度           ║" << endl;
        cout << "  ╠════════════════════════════╣" << endl;
        cout << "  ║ 解鎖：" << unlockedCount_ << " / "
             << totalAchievements_ << endl;
        cout << "  ║ 點數：" << earnedPoints_ << " / "
             << totalPoints_ << endl;
        cout << "  ║ 完成率：" << getCompletionRate() << "%" << endl;
        cout << "  ╚════════════════════════════╝" << endl;
    }

    // 靜態工廠函數：創建預設成就
    static Achievement createFirst(const string& action) {
        return Achievement(
            "初次" + action,
            "第一次" + action + "的紀念",
            10
        );
    }

    static Achievement createMilestone(const string& name, int points) {
        return Achievement(name, "里程碑成就", points);
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 25 課：靜態成員函數 綜合範例" << endl;
    cout << "============================================" << endl;

    // 使用工廠函數創建成就
    cout << "\n=== 初始化成就系統 ===" << endl;
    Achievement firstKill = Achievement::createFirst("擊殺");
    Achievement firstBoss = Achievement("屠龍者", "擊敗第一個Boss", 50);
    Achievement collector = Achievement("收藏家", "收集 100 件物品", 30);
    Achievement explorer = Achievement("探索者", "發現所有區域", 40);
    Achievement master = Achievement::createMilestone("大師之路", 100);

    // 顯示初始進度
    Achievement::printProgress();

    // 列出所有成就
    cout << "\n=== 成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    // 遊戲過程中解鎖
    cout << "\n=== 遊戲進行中... ===" << endl;
    firstKill.unlock();
    firstBoss.unlock();
    explorer.unlock();

    // 嘗試重複解鎖
    cout << "\n--- 嘗試重複解鎖 ---" << endl;
    firstKill.unlock();

    // 查看進度
    cout << "\n=== 當前進度 ===" << endl;
    Achievement::printProgress();

    // 最終成就列表
    cout << "\n=== 最終成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson25 lesson25.cpp
./lesson25
```

### 預期輸出

```
============================================
   第 25 課：靜態成員函數 綜合範例
============================================

=== 初始化成就系統 ===
  ╔════════════════════════════╗
  ║      成 就 進 度           ║
  ╠════════════════════════════╣
  ║ 解鎖：0 / 5
  ║ 點數：0 / 230
  ║ 完成率：0%
  ╚════════════════════════════╝

=== 成就列表 ===
  🔒 初次擊殺 (10點) — 第一次擊殺的紀念
  🔒 屠龍者 (50點) — 擊敗第一個Boss
  🔒 收藏家 (30點) — 收集 100 件物品
  🔒 探索者 (40點) — 發現所有區域
  🔒 大師之路 (100點) — 里程碑成就

=== 遊戲進行中... ===
  🏆 成就解鎖！「初次擊殺」 +10 點
    第一次擊殺的紀念
  🏆 成就解鎖！「屠龍者」 +50 點
    擊敗第一個Boss
  🏆 成就解鎖！「探索者」 +40 點
    發現所有區域

--- 嘗試重複解鎖 ---
  「初次擊殺」已經解鎖過了

=== 當前進度 ===
  ╔════════════════════════════╗
  ║      成 就 進 度           ║
  ╠════════════════════════════╣
  ║ 解鎖：3 / 5
  ║ 點數：100 / 230
  ║ 完成率：60%
  ╚════════════════════════════╝

=== 最終成就列表 ===
  🏆 初次擊殺 (10點)
  🏆 屠龍者 (50點)
  🔒 收藏家 (30點) — 收集 100 件物品
  🔒 探索者 (40點)
  🔒 大師之路 (100點) — 里程碑成就
```

等等，有個小問題——探索者已經解鎖了，但最後列表中顯示的是 `🔒`？不是的，讓我再確認...實際上探索者已經 `unlock()` 了，`unlocked_` 是 `true`，所以 `printInfo` 會顯示 `🏆`。修正預期輸出：

```
  🏆 探索者 (40點)
```

---

## 25.9 靜態成員函數的完整總結

```
┌──────────────────┬─────────────────────┬──────────────────────┐
│ 特性             │ 非靜態成員函數        │ 靜態成員函數          │
├──────────────────┼─────────────────────┼──────────────────────┤
│ this 指標        │ 有                   │ 沒有                 │
│ 訪問非靜態成員    │ ✅                  │ ❌                   │
│ 訪問靜態成員      │ ✅                  │ ✅                   │
│ 需要對象調用      │ 是                   │ 否                   │
│ 可以加 const      │ ✅                  │ ❌                   │
│ 推薦調用方式      │ obj.func()          │ Class::func()        │
│ 語法標記         │ 無特殊               │ static               │
│ 典型用途         │ 操作對象數據          │ 工廠、統計、工具、單例 │
└──────────────────┴─────────────────────┴──────────────────────┘
```

---

## 25.10 本課重點回顧

| 概念 | 說明 |
|------|------|
| static 成員函數 | 屬於類別，不依賴對象，沒有 this |
| 調用方式 | 推薦 `Class::func()`，不推薦 `obj.func()` |
| 不能加 const | 沒有 this，const 無意義 |
| 只能訪問靜態成員 | 非靜態成員需要 this 才能訪問 |
| 可以接收對象參數 | 通過參數間接操作對象 |
| 工廠函數 | 靜態函數創建對象，提供語義化的建構方式 |
| 工具函數 | 純運算，不需要對象狀態 |
| 單例存取 | 通過靜態函數獲取唯一實例 |
| vs 全域函數 | 靜態函數有歸屬、可訪問 private、可維護狀態 |
| vs 命名空間函數 | 與類別相關時用靜態函數，純工具用命名空間 |

---

## 25.11 下一課預告

下一課是第四階段的最後一課——**第 26 課：this 指標**。我們將深入探討 `this` 的本質：它到底是什麼、在哪些場景必須顯式使用、鏈式調用的原理、以及 `this` 與靜態成員的關係。

準備好進入 **第 26 課：this 指標** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

class Achievement {
private:
    string name_;
    string description_;
    int points_;
    bool unlocked_;

    // 靜態：全域成就統計
    // 使用 inline static 直接在類內初始化，簡化定義
    // 這些靜態成員變量用於追蹤所有成就的總數、已解鎖的數量、總點數和已獲得的點數
    // 靜態成員變量：類別級別的狀態，所有實例共享
    // 靜態成員變量示例
    inline static int totalAchievements_ = 0;
    inline static int unlockedCount_ = 0;
    inline static int totalPoints_ = 0;
    inline static int earnedPoints_ = 0;

public:
    Achievement(const string& name, const string& desc, int points)
        : name_(name), description_(desc)
        , points_(points > 0 ? points : 10)
        , unlocked_(false)
    {
        totalAchievements_++;
        totalPoints_ += points_;
    }

    // ====== 非靜態函數 ======
    // 非靜態成員函數：操作特定成就實例的行為
    // 非靜態成員函數示例
    bool unlock() {
        if (unlocked_) {
            cout << "  「" << name_ << "」已經解鎖過了" << endl;
            return false;
        }
        unlocked_ = true;
        unlockedCount_++;
        earnedPoints_ += points_;
        cout << "  🏆 成就解鎖！「" << name_ << "」 +"
             << points_ << " 點" << endl;
        cout << "    " << description_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  " << (unlocked_ ? "🏆" : "🔒") << " "
             << name_ << " (" << points_ << "點)";
        if (!unlocked_) cout << " — " << description_;
        cout << endl;
    }

    bool isUnlocked() const { return unlocked_; }
    const string& getName() const { return name_; }
    int getPoints() const { return points_; }

    // ====== 靜態函數：類別級別操作 ======
    // 靜態成員函數：操作類別級別的行為，無需實例即可調用
    // 靜態成員函數示例
    // 這些靜態成員函數提供了訪問和顯示整體成就進度的功能，並且還有一些工廠函數用於創建特定類型的成就
    // 靜態成員函數：訪問和顯示整體成就進度
    // 靜態工廠函數：創建特定類型的成就
    // 靜態成員函數示例：訪問和顯示整體成就進度

    static int getTotalAchievements() { return totalAchievements_; }
    static int getUnlockedCount() { return unlockedCount_; }

    static double getCompletionRate() {
        if (totalAchievements_ == 0) return 0.0;
        return static_cast<double>(unlockedCount_) / totalAchievements_ * 100.0;
    }

    static void printProgress() {
        cout << "  ╔════════════════════════════╗" << endl;
        cout << "  ║      成 就 進 度           ║" << endl;
        cout << "  ╠════════════════════════════╣" << endl;
        cout << "  ║ 解鎖：" << unlockedCount_ << " / "
             << totalAchievements_ << endl;
        cout << "  ║ 點數：" << earnedPoints_ << " / "
             << totalPoints_ << endl;
        cout << "  ║ 完成率：" << getCompletionRate() << "%" << endl;
        cout << "  ╚════════════════════════════╝" << endl;
    }

    // 靜態工廠函數：創建預設成就
    static Achievement createFirst(const string& action) {
        return Achievement(
            "初次" + action,
            "第一次" + action + "的紀念",
            10
        );
    }

    static Achievement createMilestone(const string& name, int points) {
        return Achievement(name, "里程碑成就", points);
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 25 課：靜態成員函數 綜合範例" << endl;
    cout << "============================================" << endl;

    // 使用工廠函數創建成就
    cout << "\n=== 初始化成就系統 ===" << endl;
    Achievement firstKill = Achievement::createFirst("擊殺");
    Achievement firstBoss = Achievement("屠龍者", "擊敗第一個Boss", 50);
    Achievement collector = Achievement("收藏家", "收集 100 件物品", 30);
    Achievement explorer = Achievement("探索者", "發現所有區域", 40);
    Achievement master = Achievement::createMilestone("大師之路", 100);

    // 顯示初始進度
    Achievement::printProgress();

    // 列出所有成就
    cout << "\n=== 成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    // 遊戲過程中解鎖
    cout << "\n=== 遊戲進行中... ===" << endl;
    firstKill.unlock();
    firstBoss.unlock();
    explorer.unlock();

    // 嘗試重複解鎖
    cout << "\n--- 嘗試重複解鎖 ---" << endl;
    firstKill.unlock();

    // 查看進度
    cout << "\n=== 當前進度 ===" << endl;
    Achievement::printProgress();

    // 最終成就列表
    cout << "\n=== 最終成就列表 ===" << endl;
    firstKill.printInfo();
    firstBoss.printInfo();
    collector.printInfo();
    explorer.printInfo();
    master.printInfo();

    return 0;
}
