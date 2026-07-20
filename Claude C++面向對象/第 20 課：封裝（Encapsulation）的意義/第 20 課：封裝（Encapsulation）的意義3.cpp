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



// =============================================================================
//  第 20 課：封裝的意義 3  —  索引不變式：越界寫入為什麼是最危險的一類 bug
// =============================================================================
//
// 【主題資訊 Information】
//   語法       ：class 內 private 資料 + public 操作；static const 成員常數
//   標準版本   ：C++98 起；class 內直接初始化的 static const int 亦為 C++98
//                （C++17 起可用 inline static / constexpr 更一般化）
//   標頭檔     ：<iostream>、<string>
//   核心概念   ：索引不變式 −1 <= top <= MAX_SIZE-1
//   複雜度     ：push / pop / size / isEmpty 皆為 O(1)
//
// 【詳細解釋 Explanation】
//   （第 1 檔講「不變式被破壞」、第 2 檔講「用動詞維護不變式」，
//     本檔聚焦一個特別危險的子類：索引不變式，破壞它會直接踩到記憶體。）
//
// 【1. 為什麼 RawStack 的問題比第 1 檔的角色更嚴重】
//   第 1 檔把 hp 設成 -500，結果是「遊戲邏輯錯了」——難看，但程式仍在
//   標準定義的行為之內。RawStack 不一樣：
//       raw.top = 999;
//       rawPush(raw, 42);        // 寫入 data[1000]，但陣列只有 100 格
//   這是「陣列越界寫入」，屬於未定義行為，而且是最惡劣的那種：
//     * 它踩到的是 RawStack 物件之外的記憶體，可能是別的變數、
//       可能是函式的回傳位址；
//     * 它「通常不會當場崩潰」。程式可能繼續跑好幾分鐘、幾小時，
//       直到某個完全無關的地方出現詭異結果才爆——此時現場早就沒了；
//     * 它是最經典的可利用漏洞（stack smashing）。
//   注意：這裡不能說「一定會 crash」或「一定會蓋到某個變數」——
//   未定義行為沒有「會發生什麼」的正確答案，實際結果取決於編譯器、
//   最佳化等級、記憶體佈局。正因為不可預測，才必須從源頭杜絕。
//   本檔的 main 刻意「只把 top 設成 999 並印出來，不再 push」，
//   就是為了示範危險而不真的觸發未定義行為。
//
// 【2. 兩個不變式，缺一不可】
//   一個安全的堆疊需要同時保證：
//       (a) −1 <= top_ <= MAX_SIZE − 1      // 索引永遠在合法範圍
//       (b) data_[0 .. top_] 都是已寫入的有效元素
//   RawStack 兩個都無法保證，因為 top 是 public：
//     * 使用者必須「記得」在使用前把 top 設成 −1（忘記就是讀未初始化的值）；
//     * rawPush 沒有上限檢查，第 101 次呼叫就越界；
//     * 任何人都能直接把 top 設成任意值。
//   SafeStack 則把兩者都收進 class：建構子保證 (a) 的起點，
//   push/pop 是唯一能改 top_ 的地方，因此 (a) 恆成立；
//   而 (b) 由「只有 push 會寫入、且寫入後才遞增 top_」保證。
//
// 【3. 為什麼建構子比「請記得初始化」可靠】
//   RawStack 的正確用法要求使用者寫 raw.top = -1;。
//   這在文件上寫得再清楚，實務上仍會被忘記——而且忘記時不會有任何警告，
//   top 只是一個未初始化的 int，讀它是未定義行為。
//   SafeStack 的建構子 SafeStack() : top_(-1) {} 讓「忘記初始化」
//   在語法上就不可能發生：只要物件存在，它就已經初始化過了。
//   這是 C++ 相對於 C 最實質的安全提升之一——把「紀律」變成「機制」。
//
// 【4. 錯誤回報：為什麼 pop 用 bool + 輸出參數】
//   pop 需要同時回報兩件事：「成功了嗎」與「彈出的值是什麼」。
//   本檔採用 bool pop(int& out) 的 C 風格；現代 C++ 有更好的選擇：
//       std::optional<int> pop();        // C++17，用型別表達「可能沒有值」
//   optional 的優勢是「無法忽略」——你必須先檢查才拿得到值，
//   而 bool + 輸出參數的版本，呼叫端完全可以忽略回傳值直接用 out，
//   而此時 out 從未被寫入。這正是本檔 main 裡 int val; 的隱患：
//   若忽略 pop 的回傳值就讀 val，就是讀未初始化的變數。
//   本檔已把 val 初始化為 0，並且只在 pop 回傳 true 時才印出它。
//
// 【概念補充 Concept Deep Dive】
//   * sizeof(RawStack) 在本機是 404 bytes（100 個 int × 4 + 1 個 int × 4，
//     且無需額外對齊填補）——這是「實作定義」的值，取決於 sizeof(int)
//     與對齊規則。SafeStack 相同（static const 成員不佔物件空間）。
//     注意 static const int MAX_SIZE 是 class 層級的常數，
//     所有物件共用，不會讓每個物件都多存一份。
//   * 固定大小陣列 vs std::vector 的取捨：本檔用 int data_[100] 是為了
//     示範索引不變式。實務上 std::vector 幾乎總是更好——它自己管理大小、
//     自動成長、at() 提供邊界檢查。但固定陣列在「絕不能動態配置」的
//     場合（嵌入式、即時系統、中斷處理常式）仍有其必要性。
//   * 即使改用 std::vector，operator[] 一樣不做邊界檢查（那是刻意的
//     零成本設計）；要檢查得用 at()，它越界時丟 std::out_of_range。
//     封裝的價值在此依然成立：把 at()/邊界判斷寫在 class 內一次，
//     而不是要求每個呼叫端自己記得。
//   * 開發期可以用 -fsanitize=address 抓越界存取，它會在越界當下就中止
//     並印出詳細位置，遠比事後除錯有效。本檔已用 ASan 驗證過無越界。
//
// 【注意事項 Pay Attention】
//   1. 陣列越界寫入是未定義行為，不能描述成「一定會 crash」或
//      「會蓋掉某個特定變數」——它不可預測，這正是它危險的原因。
//   2. RawStack 是反面教材，切勿照抄；正確做法是 SafeStack 或 std::vector。
//   3. bool + 輸出參數的錯誤回報容易被忽略；新程式碼建議用 std::optional。
//   4. static const int MAX_SIZE 不佔物件空間，不影響 sizeof。
//   5. std::vector 的 operator[] 同樣不做邊界檢查，需要檢查請用 at()。
//   6. 「請記得初始化」不是設計，建構子才是。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】索引不變式與越界存取
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 陣列越界寫入會發生什麼事？
//     答：那是未定義行為，沒有「會發生什麼」的標準答案。它可能立刻
//         segmentation fault，可能悄悄蓋掉相鄰的變數而讓程式在很久之後
//         出現無法解釋的結果，也可能因為最佳化而表現得像沒事一樣。
//         最危險的正是「不會當場崩潰」那種——現場消失，除錯極為困難。
//     追問：那要怎麼在開發期抓到？→ 用 -fsanitize=address（ASan）
//         或 valgrind，它們會在越界發生的當下就中止並指出確切位置。
//         正式版則靠封裝：把邊界檢查寫在唯一入口，讓越界不可能發生。
//
// 🔥 Q2. std::vector 的 operator[] 和 at() 差在哪？該用哪個？
//     答：operator[] 不做邊界檢查，越界是未定義行為；at() 會檢查，
//         越界時丟 std::out_of_range。差別是刻意的設計取捨——
//         C++ 的「不為你沒用到的功能付費」原則讓 operator[] 保持零成本。
//         索引本來就保證合法時（例如 for 迴圈跑 0..size()-1）用 operator[]；
//         索引來自外部輸入、無法信任時用 at()。
//     追問：at() 真的比較慢嗎？→ 要實測。邊界檢查本身只是一次比較，
//         在 -O2 下若編譯器能證明索引合法，檢查可能被完全消除；
//         但在無法證明時（例如索引來自執行期輸入）檢查會保留，
//         且可能阻礙向量化。結論是：別憑印象斷言，用實際的 benchmark 決定。
//
// ⚠️ 陷阱. 這段程式碼哪裡有問題？
//         SafeStack s;
//         int val;
//         s.pop(val);              // 沒有檢查回傳值
//         cout << val << endl;
//     答：s 是空的，pop 會回傳 false 而且「完全沒有寫入 val」。
//         此時 val 從未被初始化，讀取它是未定義行為——
//         印出來的可能是任何值，而且同一份程式碼在不同最佳化等級下
//         可能印出不同結果，甚至因為編譯器假設「不會發生 UB」而
//         把整段程式碼最佳化掉。
//         正確寫法是先檢查回傳值：
//             if (s.pop(val)) { cout << val << endl; }
//     為什麼會錯：習慣了「函式會把結果填給我」，於是把輸出參數當成
//         一定會被寫入。但 bool + 輸出參數的介面，失敗路徑通常
//         「不碰輸出參數」——這正是它最大的缺點，也是 C++17 改用
//         std::optional 的理由：optional 讓你不先檢查就根本拿不到值。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <optional>
#include <stack>
#include <string>
#include <vector>
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

// -----------------------------------------------------------------------------
// 現代寫法對照：用 std::optional 讓「失敗」無法被忽略（C++17）
//
// 與上面的 bool pop(int& out) 相比，呼叫端不先檢查就拿不到值，
// 從介面層次就杜絕了「讀到未初始化變數」的可能。
// -----------------------------------------------------------------------------
class ModernStack {
public:
    void push(int v) { data_.push_back(v); }

    std::optional<int> pop() {
        if (data_.empty()) return std::nullopt;   // 沒有值可回傳，型別上就講清楚
        const int v = data_.back();
        data_.pop_back();
        return v;
    }

    bool   empty() const { return data_.empty(); }
    size_t size()  const { return data_.size(); }

private:
    vector<int> data_;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//
// 題目：給一個只含 '(' ')' '{' '}' '[' ']' 的字串，判斷括號是否正確配對。
// 為什麼用到本主題：這是堆疊最經典的應用，而且它示範了本檔的核心論點——
//   演算法的正確性完全依賴「堆疊的不變式沒有被破壞」。
//   如果用 RawStack 那種 public top 的結構，任何一處誤改 top
//   都會讓判斷結果錯誤，而且錯得無聲無息（不會崩潰，只是答案錯）。
//   用封裝好的堆疊，我們只要相信 push/pop 正確，就能推理整個演算法。
//   注意「遇到右括號但堆疊為空」這個案例：正是需要先檢查再取值的地方。
// 複雜度：時間 O(n)，空間 O(n)。
// -----------------------------------------------------------------------------
bool isValidParentheses(const string& s) {
    std::stack<char> st;                    // 直接用標準庫封裝好的堆疊
    for (const char c : s) {
        switch (c) {
            case '(': case '[': case '{':
                st.push(c);
                break;
            case ')':
                if (st.empty() || st.top() != '(') return false;
                st.pop();
                break;
            case ']':
                if (st.empty() || st.top() != '[') return false;
                st.pop();
                break;
            case '}':
                if (st.empty() || st.top() != '{') return false;
                st.pop();
                break;
            default:
                return false;               // 題目保證只有括號字元
        }
    }
    return st.empty();                      // 還有沒配對完的左括號就是不合法
}

// -----------------------------------------------------------------------------
// 【日常實務範例】文字編輯器的「有上限的復原（undo）堆疊」
//
// 情境：編輯器要支援 Ctrl+Z，但不能無限累積歷史（會吃光記憶體），
//   所以要限制最多保留 N 步；超過就丟掉最舊的。
// 為何用到本主題：這是「固定容量」不變式的真實版本。
//   容量上限的處理邏輯（滿了要丟最舊的，而不是拒絕新資料）
//   如果散落在每個呼叫端，遲早會有人忘記；
//   收進 class 之後，「歷史永遠不超過 maxDepth_ 筆」才是可以被信賴的保證。
// -----------------------------------------------------------------------------
class UndoHistory {
public:
    explicit UndoHistory(size_t maxDepth)
        : maxDepth_(maxDepth == 0 ? 1 : maxDepth) {}   // 深度至少為 1

    void record(const string& action) {
        if (actions_.size() >= maxDepth_) {
            // 容量已滿：丟掉最舊的一筆，維持「不超過 maxDepth_」的不變式
            actions_.erase(actions_.begin());
            ++discarded_;
        }
        actions_.push_back(action);
    }

    std::optional<string> undo() {
        if (actions_.empty()) return std::nullopt;
        const string last = actions_.back();
        actions_.pop_back();
        return last;
    }

    size_t depth()          const { return actions_.size(); }
    size_t maxDepth()       const { return maxDepth_; }
    size_t discardedCount() const { return discarded_; }

    // 不變式自我檢查
    bool checkInvariant() const { return actions_.size() <= maxDepth_; }

private:
    vector<string> actions_;
    size_t         maxDepth_;
    size_t         discarded_ = 0;
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
    // 注意：val 一定要初始化。pop 失敗時「完全不會寫入 val」，
    //       若沒初始化又忽略回傳值就去讀它，就是讀取不定值（未定義行為）。
    int val = 0;
    if (safe.pop(val)) cout << "  彈出 " << val << endl;
    if (safe.pop(val)) cout << "  彈出 " << val << endl;
    if (safe.pop(val)) {
        cout << "  彈出 " << val << endl;
    } else {
        cout << "  第三次 pop 回傳 false，val 未被寫入（維持 " << val << "）" << endl;
    }

    // ────────────────────────────────────────────────────────────
    cout << "\n=== 現代寫法：std::optional 讓失敗無法被忽略 ===" << endl;
    {
        ModernStack ms;
        ms.push(7);
        ms.push(9);
        // 必須先解開 optional 才拿得到值，不檢查就取用會是錯誤
        while (const auto v = ms.pop()) {
            cout << "  彈出 " << *v << endl;
        }
        const auto none = ms.pop();
        cout << "  空堆疊再 pop → has_value() = "
             << (none.has_value() ? "true" : "false") << endl;
        cout << "  → 相較 bool + 輸出參數，這個介面不可能讓人讀到未初始化的值" << endl;
    }

    cout << "\n=== LeetCode 20. Valid Parentheses ===" << endl;
    {
        const string cases[] = {"()", "()[]{}", "(]", "([)]", "{[]}", "(", ")"};
        for (const string& c : cases) {
            cout << "  isValid(\"" << c << "\") = "
                 << (isValidParentheses(c) ? "true" : "false") << endl;
        }
        cout << "  → 演算法的正確性完全建立在「堆疊不變式沒被破壞」之上" << endl;
    }

    cout << "\n=== 日常實務：有上限的復原（undo）堆疊 ===" << endl;
    {
        UndoHistory hist(3);               // 最多保留 3 步
        const string edits[] = {"輸入 Hello", "輸入 World", "刪除一行",
                                "貼上段落", "調整縮排"};
        for (const string& e : edits) {
            hist.record(e);
            cout << "  記錄「" << e << "」→ 目前深度 " << hist.depth()
                 << "/" << hist.maxDepth() << endl;
        }
        cout << "  已丟棄最舊的 " << hist.discardedCount() << " 筆" << endl;
        cout << "  不變式（深度不超過上限）："
             << (hist.checkInvariant() ? "成立" : "不成立") << endl;

        cout << "  開始復原：" << endl;
        while (const auto action = hist.undo()) {
            cout << "    復原「" << *action << "」" << endl;
        }
        cout << "  歷史已空，再 undo 會回傳 nullopt" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：封裝（Encapsulation）的意義3.cpp" -o enc3
// 越界檢查: g++ -std=c++17 -fsanitize=address -g 同上 && ./enc3
//           （本檔已用 ASan 實測，無任何越界存取）
//
// 說明：
//   1. 本檔開頭 1..645 行是本課的講義（整段註解），實際程式碼從 #include 開始。
//   2. RawStack 的示範刻意「只把 top 設成 999 並印出來，不再 push」——
//      若真的在 top=999 時 push，就會發生陣列越界寫入（未定義行為）。
//      教材示範危險，但不會真的觸發它。
//   3. 「第三次 pop 回傳 false，val 未被寫入（維持 10）」中的 10，
//      是上一次成功 pop 留下的值。因為 val 有初始化，這個結果是確定的；
//      若 val 未初始化，此處就會是不定值，不可預測也不該印出。
//   4. UndoHistory 丟棄最舊 2 筆：記錄 5 筆、上限 3，故丟棄 5-3=2 筆。
//   5. 本檔所有輸出都是確定的：沒有用到亂數、時間或位址。

// === 預期輸出 ===
// === 不封裝 vs 封裝 ===
//
// --- 不封裝的 RawStack ---
//   Top = 1
//   直接修改 top = 999 (越界！危險！)
//
// --- 封裝的 SafeStack ---
//   Size = 2
//   彈出 20
//   彈出 10
//   棧為空，無法彈出
//   第三次 pop 回傳 false，val 未被寫入（維持 10）
//
// === 現代寫法：std::optional 讓失敗無法被忽略 ===
//   彈出 9
//   彈出 7
//   空堆疊再 pop → has_value() = false
//   → 相較 bool + 輸出參數，這個介面不可能讓人讀到未初始化的值
//
// === LeetCode 20. Valid Parentheses ===
//   isValid("()") = true
//   isValid("()[]{}") = true
//   isValid("(]") = false
//   isValid("([)]") = false
//   isValid("{[]}") = true
//   isValid("(") = false
//   isValid(")") = false
//   → 演算法的正確性完全建立在「堆疊不變式沒被破壞」之上
//
// === 日常實務：有上限的復原（undo）堆疊 ===
//   記錄「輸入 Hello」→ 目前深度 1/3
//   記錄「輸入 World」→ 目前深度 2/3
//   記錄「刪除一行」→ 目前深度 3/3
//   記錄「貼上段落」→ 目前深度 3/3
//   記錄「調整縮排」→ 目前深度 3/3
//   已丟棄最舊的 2 筆
//   不變式（深度不超過上限）：成立
//   開始復原：
//     復原「調整縮排」
//     復原「貼上段落」
//     復原「刪除一行」
//   歷史已空，再 undo 會回傳 nullopt
