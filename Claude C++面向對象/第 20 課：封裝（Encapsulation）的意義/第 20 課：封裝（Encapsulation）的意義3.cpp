/*
好的，信安！第三階段已經順利完成，現在進入**第四階段：封裝深入**。

---

# 第 20 課：封裝（Encapsulation）的意義

---

## 20.1 什麼是封裝？

在第三階段中，你已經用過 `private` 和 `public`，但我們還沒深入討論**為什麼**要這樣做。封裝是 OOP 的四大核心支柱之一：

```
OOP 四大支柱：
  ┌─────────────┐
  │   封裝       │ ← 本階段（第 20～26 課）
  │ Encapsulation│
  ├─────────────┤
  │   繼承       │ ← 第八階段
  │ Inheritance  │
  ├─────────────┤
  │   多型       │ ← 第九階段
  │ Polymorphism │
  ├─────────────┤
  │   抽象       │ ← 第十階段
  │ Abstraction  │
  └─────────────┘
```

封裝的核心思想是：**將數據和操作數據的函數綁定在一起，並控制外部對數據的訪問**。

用一個現實類比：

```
手機的封裝：
  ┌──────────────────────────────┐
  │  外部介面（public）           │
  │  ┌────────────────────────┐  │
  │  │ 觸控螢幕               │  │
  │  │ 音量按鈕               │  │
  │  │ 充電接口               │  │
  │  └────────────────────────┘  │
  │                              │
  │  內部實現（private）          │
  │  ┌────────────────────────┐  │
  │  │ CPU 電路               │  │
  │  │ 記憶體晶片             │  │
  │  │ 天線模組               │  │
  │  │ 電池管理系統           │  │
  │  └────────────────────────┘  │
  └──────────────────────────────┘

  你不需要知道天線怎麼運作，
  只需要透過「撥號」介面打電話。
```

---

## 20.2 沒有封裝的災難

先看看如果**不封裝**會出什麼問題：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 沒有封裝的角色（全部 public）=====
struct CharacterBad {
    string name;
    int hp;         // 生命值
    int maxHp;      // 最大生命值
    int attack;     // 攻擊力
    int level;      // 等級
    int exp;        // 經驗值
    int gold;       // 金幣
};

int main() {
    cout << "=== 沒有封裝的災難 ===" << endl;

    CharacterBad hero;
    hero.name = "勇者";
    hero.hp = 100;
    hero.maxHp = 100;
    hero.attack = 25;
    hero.level = 1;
    hero.exp = 0;
    hero.gold = 50;

    cout << hero.name << " HP:" << hero.hp << "/" << hero.maxHp << endl;

    // 災難 1：生命值可以超過最大值
    hero.hp = 99999;
    cout << "超量回血：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 2：生命值可以是負數
    hero.hp = -500;
    cout << "負數 HP：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 3：最大生命值可以被設成 0
    hero.maxHp = 0;
    cout << "最大HP歸零：HP = " << hero.hp << "/" << hero.maxHp << endl;

    // 災難 4：攻擊力可以隨便改
    hero.attack = 999999;
    cout << "作弊攻擊力：" << hero.attack << endl;

    // 災難 5：等級與經驗不一致
    hero.level = 99;
    hero.exp = 0;    // 99 級但經驗值是 0？
    cout << "等級 " << hero.level << "，經驗 " << hero.exp << endl;

    // 災難 6：金幣可以無限刷
    hero.gold = 2147483647;
    cout << "無限金幣：" << hero.gold << endl;

    cout << "\n所有數據都被任意篡改，毫無防護！" << endl;

    return 0;
}
```

### 預期輸出

```
=== 沒有封裝的災難 ===
勇者 HP:100/100
超量回血：HP = 99999/100
負數 HP：HP = -500/100
最大HP歸零：HP = -500/0
作弊攻擊力：999999
等級 99，經驗 0
無限金幣：2147483647

所有數據都被任意篡改，毫無防護！
```

問題一目了然——**任何人都可以把數據改成不合理的值**，沒有任何保護機制。

---

## 20.3 封裝的三層防護

封裝提供三層防護：

```
第一層：訪問控制
  → private / protected / public 限制誰能碰數據

第二層：數據驗證
  → setter 函數檢查數據是否合法

第三層：不變量維護（invariant）
  → 確保對象始終處於合法狀態
  → 例如：hp 永遠在 [0, maxHp] 範圍內
```

用同樣的角色範例，加上封裝：

```cpp
#include <iostream>
#include <string>
#include <algorithm>   // std::clamp (C++17)
using namespace std;

class Character {
private:
    // ====== 私有數據：外部無法直接碰 ======
    string name_;
    int hp_;
    int maxHp_;
    int attack_;
    int level_;
    int exp_;
    int gold_;

    // ====== 私有輔助函數 ======
    int expToNextLevel() const {
        return level_ * 100;   // 每級需要 level * 100 經驗
    }

    void checkLevelUp() {
        while (exp_ >= expToNextLevel()) {
            exp_ -= expToNextLevel();
            level_++;
            maxHp_ += 20;
            hp_ = maxHp_;       // 升級回滿血
            attack_ += 5;
            cout << "  ★ " << name_ << " 升級！Lv." << level_
                 << " (HP+" << 20 << " ATK+" << 5 << ")" << endl;
        }
    }

public:
    // ====== 建構函數：確保初始狀態合法 ======
    Character(const string& name, int maxHp, int attack)
        : name_(name)
        , hp_(maxHp > 0 ? maxHp : 100)         // 防止無效值
        , maxHp_(maxHp > 0 ? maxHp : 100)
        , attack_(attack > 0 ? attack : 10)
        , level_(1)
        , exp_(0)
        , gold_(0)
    {
        cout << "  [創建] " << name_ << " HP:" << hp_
             << " ATK:" << attack_ << endl;
    }

    // ====== 公開介面：受控的操作 ======

    // 受傷——有保護的 HP 減少
    void takeDamage(int damage) {
        if (damage <= 0) {
            cout << "  " << name_ << "：無效的傷害值" << endl;
            return;
        }
        hp_ = max(0, hp_ - damage);   // HP 不會低於 0
        cout << "  " << name_ << " 受到 " << damage
             << " 點傷害 (HP: " << hp_ << "/" << maxHp_ << ")" << endl;

        if (hp_ == 0) {
            cout << "  " << name_ << " 倒下了！" << endl;
        }
    }

    // 治療——有保護的 HP 增加
    void heal(int amount) {
        if (amount <= 0 || hp_ == 0) {
            cout << "  " << name_ << "：無法治療" << endl;
            return;
        }
        int oldHp = hp_;
        hp_ = min(hp_ + amount, maxHp_);   // 不超過上限
        cout << "  " << name_ << " 恢復 " << (hp_ - oldHp)
             << " 點生命 (HP: " << hp_ << "/" << maxHp_ << ")" << endl;
    }

    // 獲得經驗——自動檢查升級
    void gainExp(int amount) {
        if (amount <= 0) return;
        exp_ += amount;
        cout << "  " << name_ << " 獲得 " << amount << " 經驗值" << endl;
        checkLevelUp();   // 內部自動處理升級邏輯
    }

    // 獲得金幣——有上限保護
    void earnGold(int amount) {
        if (amount <= 0) return;
        gold_ = min(gold_ + amount, 999999);   // 金幣上限
        cout << "  " << name_ << " 獲得 " << amount
             << " 金幣 (總計: " << gold_ << ")" << endl;
    }

    // 花費金幣——檢查是否足夠
    bool spendGold(int amount) {
        if (amount <= 0 || amount > gold_) {
            cout << "  " << name_ << "：金幣不足！"
                 << "(需要 " << amount << "，擁有 " << gold_ << ")" << endl;
            return false;
        }
        gold_ -= amount;
        cout << "  " << name_ << " 花費 " << amount
             << " 金幣 (剩餘: " << gold_ << ")" << endl;
        return true;
    }

    // 顯示狀態——只讀，不修改
    void printStatus() const {
        cout << "  ┌─────────────────────┐" << endl;
        cout << "  │ " << name_ << " (Lv." << level_ << ")" << endl;
        cout << "  │ HP:  " << hp_ << " / " << maxHp_ << endl;
        cout << "  │ ATK: " << attack_ << endl;
        cout << "  │ EXP: " << exp_ << " / " << expToNextLevel() << endl;
        cout << "  │ Gold: " << gold_ << endl;
        cout << "  └─────────────────────┘" << endl;
    }

    // 查詢函數（只讀）
    bool isAlive() const { return hp_ > 0; }
    const string& getName() const { return name_; }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 20 課：封裝的意義" << endl;
    cout << "============================================" << endl;

    // 創建角色——建構函數保證初始狀態合法
    cout << "\n=== 創建角色 ===" << endl;
    Character hero("勇者", 100, 25);
    hero.printStatus();

    // 正常戰鬥流程
    cout << "\n=== 戰鬥 ===" << endl;
    hero.takeDamage(30);
    hero.takeDamage(50);
    hero.heal(40);

    // 嘗試非法操作——全部被攔截
    cout << "\n=== 嘗試非法操作 ===" << endl;
    hero.takeDamage(-100);     // 負傷害？不允許
    hero.heal(-50);            // 負治療？不允許

    // 經驗與升級——自動管理
    cout << "\n=== 經驗與升級 ===" << endl;
    hero.gainExp(80);
    hero.gainExp(50);     // 應該觸發升級（80+50=130 >= 100）
    hero.printStatus();

    // 金幣系統——有保護
    cout << "\n=== 金幣系統 ===" << endl;
    hero.earnGold(200);
    hero.spendGold(150);
    hero.spendGold(100);   // 應該失敗——金幣不足

    // 致命傷害
    cout << "\n=== 致命傷害 ===" << endl;
    hero.takeDamage(9999);
    hero.heal(50);          // 已死亡，無法治療

    hero.printStatus();

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson20 lesson20.cpp
./lesson20
```

### 預期輸出

```
============================================
   第 20 課：封裝的意義
============================================

=== 創建角色 ===
  [創建] 勇者 HP:100 ATK:25
  ┌─────────────────────┐
  │ 勇者 (Lv.1)
  │ HP:  100 / 100
  │ ATK: 25
  │ EXP: 0 / 100
  │ Gold: 0
  └─────────────────────┘

=== 戰鬥 ===
  勇者 受到 30 點傷害 (HP: 70/100)
  勇者 受到 50 點傷害 (HP: 20/100)
  勇者 恢復 40 點生命 (HP: 60/100)

=== 嘗試非法操作 ===
  勇者：無效的傷害值
  勇者：無法治療

=== 經驗與升級 ===
  勇者 獲得 80 經驗值
  勇者 獲得 50 經驗值
  ★ 勇者 升級！Lv.2 (HP+20 ATK+5)
  ┌─────────────────────┐
  │ 勇者 (Lv.2)
  │ HP:  120 / 120
  │ ATK: 30
  │ EXP: 30 / 200
  │ Gold: 0
  └─────────────────────┘

=== 金幣系統 ===
  勇者 獲得 200 金幣 (總計: 200)
  勇者 花費 150 金幣 (剩餘: 50)
  勇者：金幣不足！(需要 100，擁有 50)

=== 致命傷害 ===
  勇者 受到 9999 點傷害 (HP: 0/120)
  勇者 倒下了！
  勇者：無法治療
  ┌─────────────────────┐
  │ 勇者 (Lv.2)
  │ HP:  0 / 120
  │ ATK: 30
  │ EXP: 30 / 200
  │ Gold: 50
  └─────────────────────┘
```

---

## 20.4 封裝的核心概念：不變量（Invariant）

**不變量**是指「對象在任何時刻都必須滿足的條件」。這是封裝最重要的概念：

```
Character 類別的不變量：
  1. hp_ >= 0               （生命值不能是負數）
  2. hp_ <= maxHp_           （生命值不超過上限）
  3. maxHp_ > 0              （最大生命值必須為正）
  4. attack_ > 0             （攻擊力必須為正）
  5. level_ >= 1             （等級至少為 1）
  6. exp_ >= 0               （經驗不能是負數）
  7. exp_ < expToNextLevel() （經驗不超過升級門檻）
  8. 0 <= gold_ <= 999999    （金幣在合法範圍）
```

**封裝的目的就是維護不變量**。每個 public 成員函數都必須保證：

```
函數開始前：不變量成立
  ↓
函數執行中：可以暫時違反
  ↓
函數結束後：不變量必須恢復成立
```

如果數據是 public 的，任何外部代碼都可以直接修改數據，**不變量就無法維護**。

---

## 20.5 封裝 vs 不封裝的對比

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 不封裝版本 =====
struct RawStack {
    int data[100];
    int top;         // 應該在 [-1, 99] 範圍內
};

// 使用者必須自己小心
void rawPush(RawStack& s, int val) {
    // 如果忘記檢查呢？
    s.data[++s.top] = val;
}

// ===== 封裝版本 =====
class SafeStack {
private:
    static const int MAX_SIZE = 100;
    int data_[MAX_SIZE];
    int top_;

public:
    SafeStack() : top_(-1) {}

    bool push(int val) {
        if (top_ >= MAX_SIZE - 1) {
            cout << "  棧已滿，無法推入 " << val << endl;
            return false;
        }
        data_[++top_] = val;
        return true;
    }

    bool pop(int& out) {
        if (top_ < 0) {
            cout << "  棧為空，無法彈出" << endl;
            return false;
        }
        out = data_[top_--];
        return true;
    }

    bool isEmpty() const { return top_ < 0; }
    int size() const { return top_ + 1; }
};

int main() {
    cout << "=== 不封裝 vs 封裝 ===" << endl;

    // 不封裝——可以直接搞破壞
    cout << "\n--- 不封裝的 RawStack ---" << endl;
    RawStack raw;
    raw.top = -1;       // 使用者得記得初始化
    rawPush(raw, 10);
    rawPush(raw, 20);
    cout << "  Top = " << raw.top << endl;

    raw.top = 999;      // 直接越界！沒人阻止
    cout << "  直接修改 top = " << raw.top << " (越界！危險！)" << endl;

    // 封裝——安全可靠
    cout << "\n--- 封裝的 SafeStack ---" << endl;
    SafeStack safe;       // 建構函數自動初始化
    safe.push(10);
    safe.push(20);
    cout << "  Size = " << safe.size() << endl;

    // safe.top_ = 999;  // 編譯錯誤！private 不可訪問
    // safe.data_[0] = 0; // 編譯錯誤！

    // 嘗試從空棧彈出
    int val;
    safe.pop(val);
    safe.pop(val);
    safe.pop(val);     // 空棧，被安全攔截

    return 0;
}
```

### 預期輸出

```
=== 不封裝 vs 封裝 ===

--- 不封裝的 RawStack ---
  Top = 1
  直接修改 top = 999 (越界！危險！)

--- 封裝的 SafeStack ---
  Size = 2
  Size = 0
  棧為空，無法彈出
```

---

## 20.6 封裝的四大好處

```
好處一：數據保護
  → 防止外部代碼破壞對象的合法狀態
  → 維護不變量

好處二：介面穩定
  → 外部代碼只依賴 public 介面
  → 內部實現可以自由修改，不影響使用者

好處三：降低耦合
  → 使用者不需要知道內部結構
  → 減少代碼之間的依賴關係

好處四：簡化使用
  → 使用者只需要知道「做什麼」，不需要知道「怎麼做」
  → 就像開車不需要懂引擎原理
```

**介面穩定性**的例子：

```cpp
// ===== 版本 1：用陣列存儲 =====
class PlayerList_v1 {
private:
    string names[100];  // 內部用陣列
    int count;
public:
    void addPlayer(const string& name);
    void removePlayer(const string& name);
    int getCount() const;
};

// ===== 版本 2：改用 vector 存儲 =====
class PlayerList_v2 {
private:
    vector<string> names;  // 內部改用 vector
public:
    // public 介面完全不變！
    void addPlayer(const string& name);
    void removePlayer(const string& name);
    int getCount() const;
};

// 使用者的代碼不需要做任何修改：
// PlayerList list;
// list.addPlayer("Alice");
// list.removePlayer("Bob");
// cout << list.getCount();
```

內部從陣列改成 `vector`，**使用者的代碼完全不受影響**——這就是封裝帶來的介面穩定性。

---

## 20.7 封裝的層次

封裝不只是 `private`，它有多個層次：

```
層次 1：語法層封裝（access control）
  → private / protected / public
  → 編譯器強制執行

層次 2：邏輯層封裝（data validation）
  → setter 函數中的驗證邏輯
  → 維護不變量

層次 3：介面層封裝（interface design）
  → 只暴露必要的操作
  → 隱藏實現細節

層次 4：模組層封裝（module/namespace）
  → 用命名空間和頭文件組織代碼
  → 控制哪些符號對外可見
```

一個好的封裝設計原則：**暴露盡可能少的介面**。

```
不好的設計：          好的設計：
┌────────────┐      ┌────────────┐
│  public:   │      │  public:   │
│  func1()   │      │  doTask()  │   ← 只暴露高層操作
│  func2()   │      │            │
│  func3()   │      │  private:  │
│  func4()   │      │  step1()   │   ← 內部步驟隱藏
│  func5()   │      │  step2()   │
│  func6()   │      │  step3()   │
│            │      │  helper()  │
│  private:  │      │  data_     │
│  data_     │      └────────────┘
└────────────┘
```

---

## 20.8 本課重點回顧

| 概念 | 說明 |
|------|------|
| 封裝的定義 | 將數據和操作綁定，控制外部對數據的訪問 |
| 不變量（invariant） | 對象在任何時刻都必須滿足的條件 |
| 封裝的核心目的 | 維護不變量，防止對象進入非法狀態 |
| 數據保護 | private 阻止外部直接修改 |
| 介面穩定 | 內部實現可變，public 介面不變 |
| 降低耦合 | 使用者不依賴內部細節 |
| 簡化使用 | 使用者只需知道「做什麼」 |
| 暴露最少原則 | 只公開必要的介面 |
| 三層防護 | 訪問控制 → 數據驗證 → 不變量維護 |

---

## 20.9 下一課預告

下一課是 **第 21 課：getter 與 setter 設計模式**。我們將深入探討：如何正確設計 getter/setter、什麼時候不該寫 setter、返回值的引用 vs 拷貝問題，以及常見的設計陷阱。

準備好進入 **第 21 課：getter 與 setter 設計模式** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

// ===== 不封裝版本 =====
// 這個結構體沒有任何封裝，所有成員都是 public 的，外部程式可以隨意修改它們，導致各種災難。
struct RawStack {
    int data[100];
    int top;         // 應該在 [-1, 99] 範圍內
};

// 使用者必須自己小心
void rawPush(RawStack& s, int val) {
    // 如果忘記檢查呢？
    s.data[++s.top] = val;
}

// ===== 封裝版本 =====
// 這個類使用 private 封裝了內部數據，提供了受控的 public 函數來操作棧，確保安全性和一致性。
class SafeStack {
private:
    static const int MAX_SIZE = 100;
    int data_[MAX_SIZE];
    int top_;

public:
    SafeStack() : top_(-1) {}

    bool push(int val) {
        if (top_ >= MAX_SIZE - 1) {
            cout << "  棧已滿，無法推入 " << val << endl;
            return false;
        }
        data_[++top_] = val;
        return true;
    }

    bool pop(int& out) {
        if (top_ < 0) {
            cout << "  棧為空，無法彈出" << endl;
            return false;
        }
        out = data_[top_--];
        return true;
    }

    bool isEmpty() const { return top_ < 0; }
    int size() const { return top_ + 1; }
};

int main() {
    cout << "=== 不封裝 vs 封裝 ===" << endl;

    // 不封裝——可以直接搞破壞
    cout << "\n--- 不封裝的 RawStack ---" << endl;
    RawStack raw;
    raw.top = -1;       // 使用者得記得初始化
    rawPush(raw, 10);
    rawPush(raw, 20);
    cout << "  Top = " << raw.top << endl;

    raw.top = 999;      // 直接越界！沒人阻止
    cout << "  直接修改 top = " << raw.top << " (越界！危險！)" << endl;

    // 封裝——安全可靠
    cout << "\n--- 封裝的 SafeStack ---" << endl;
    SafeStack safe;       // 建構函數自動初始化
    safe.push(10);
    safe.push(20);
    cout << "  Size = " << safe.size() << endl;

    // safe.top_ = 999;  // 編譯錯誤！private 不可訪問
    // safe.data_[0] = 0; // 編譯錯誤！

    // 嘗試從空棧彈出
    int val;
    safe.pop(val);
    safe.pop(val);
    safe.pop(val);     // 空棧，被安全攔截

    return 0;
}
