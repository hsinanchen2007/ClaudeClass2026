/*
# 第 21 課：getter 與 setter 設計模式

---

## 21.1 什麼是 getter 和 setter？

上一課我們學了封裝的意義——把數據藏在 `private` 裡保護起來。但有時候外部確實需要**讀取**或**修改**某些數據，這時就需要 getter 和 setter：

```
getter（取值器）：讓外部「讀取」私有數據
setter（設值器）：讓外部「受控地修改」私有數據

┌─────────────────────────────┐
│  private:                   │
│    int hp_;                 │
│    string name_;            │
│                             │
│  public:                    │
│    int getHp() const;       │  ← getter：只讀
│    void setHp(int newHp);   │  ← setter：帶驗證的寫入
└─────────────────────────────┘
```

重要的觀念：**getter/setter 不是給每個成員變數都機械地加上的**。它們是經過設計考量的介面。

---

## 21.2 基本 getter 和 setter 語法

```cpp
#include <iostream>
#include <string>
using namespace std;

class Player {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    Player(const string& name, int maxHp)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(1)
    {
    }

    // ====== Getter：返回數據的只讀訪問 ======

    // 基本型別的 getter：返回值的拷貝
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    // 字串的 getter：返回 const 引用（避免拷貝）
    const string& getName() const { return name_; }

    // ====== Setter：帶驗證的修改 ======

    void setHp(int newHp) {
        // 驗證：維護不變量
        if (newHp < 0) newHp = 0;
        if (newHp > maxHp_) newHp = maxHp_;
        hp_ = newHp;
    }

    void setName(const string& newName) {
        if (newName.empty()) {
            cout << "  名字不能為空！" << endl;
            return;
        }
        name_ = newName;
    }

    // 注意：level_ 沒有 setter！
    // 等級只能通過 gainExp() 等遊戲邏輯改變，不開放直接設定

    void printStatus() const {
        cout << "  " << name_ << " Lv." << level_
             << " HP:" << hp_ << "/" << maxHp_ << endl;
    }
};

int main() {
    cout << "=== Getter 與 Setter 基本用法 ===" << endl;

    Player hero("勇者", 100);
    hero.printStatus();

    // 使用 getter 讀取
    cout << "\n--- Getter ---" << endl;
    cout << "  名字：" << hero.getName() << endl;
    cout << "  HP：" << hero.getHp() << endl;
    cout << "  等級：" << hero.getLevel() << endl;

    // 使用 setter 修改（帶驗證）
    cout << "\n--- Setter（正常）---" << endl;
    hero.setHp(60);
    hero.printStatus();

    // setter 的保護作用
    cout << "\n--- Setter（異常值被攔截）---" << endl;
    hero.setHp(-500);       // 會被修正為 0
    hero.printStatus();

    hero.setHp(99999);      // 會被修正為 maxHp
    hero.printStatus();

    hero.setName("");        // 空名字被拒絕
    hero.setName("英雄");
    hero.printStatus();

    // hero.setLevel(99);   // 編譯錯誤！沒有 setLevel

    return 0;
}
```

### 預期輸出

```
=== Getter 與 Setter 基本用法 ===
  勇者 Lv.1 HP:100/100

--- Getter ---
  名字：勇者
  HP：100
  等級：1

--- Setter（正常）---
  勇者 Lv.1 HP:60/100

--- Setter（異常值被攔截）---
  勇者 Lv.1 HP:0/100
  勇者 Lv.1 HP:100/100
  名字不能為空！
  英雄 Lv.1 HP:100/100
```

---

## 21.3 getter 的返回方式選擇

getter 的返回方式很有講究，選錯會造成效能問題或安全問題：

```
基本型別（int, double, bool, char）：
  → 直接返回值（拷貝成本極低）
  → int getHp() const;

小型物件：
  → 直接返回值也可以
  → Point getPosition() const;

大型物件（string, vector, map）：
  → 返回 const 引用（避免不必要的拷貝）
  → const string& getName() const;

  ⚠ 注意：返回引用意味著對象不存在後引用就懸空
```

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Inventory {
private:
    string ownerName_;
    vector<string> items_;
    int gold_;

public:
    Inventory(const string& owner, int gold)
        : ownerName_(owner), gold_(gold) {}

    void addItem(const string& item) {
        items_.push_back(item);
    }

    // ====== 返回方式比較 ======

    // 方式 1：返回值（拷貝）— 適合基本型別
    int getGold() const { return gold_; }

    // 方式 2：返回 const 引用 — 適合大型物件，避免拷貝
    const string& getOwnerName() const { return ownerName_; }

    // 方式 3：返回 const 引用 — 讓外部可以「看」但不能「改」
    const vector<string>& getItems() const { return items_; }

    // 方式 4（錯誤示範）：返回非 const 引用 — 破壞封裝！
    // vector<string>& getItemsDangerous() { return items_; }
    // ↑ 外部可以直接修改 items_，繞過所有驗證！
};

int main() {
    cout << "=== Getter 返回方式 ===" << endl;

    Inventory inv("勇者", 500);
    inv.addItem("鐵劍");
    inv.addItem("治療藥水");
    inv.addItem("火炬");

    // 方式 1：返回值（拷貝）
    int gold = inv.getGold();
    gold = 0;  // 修改的是拷貝，不影響原物件
    cout << "  修改拷貝後，實際金幣：" << inv.getGold() << endl;

    // 方式 2：返回 const 引用
    const string& name = inv.getOwnerName();
    cout << "  擁有者：" << name << endl;
    // name = "壞人";  // 編譯錯誤！const 引用不能修改

    // 方式 3：返回 const 引用（容器）
    const vector<string>& items = inv.getItems();
    cout << "  物品數量：" << items.size() << endl;
    for (const auto& item : items) {
        cout << "    - " << item << endl;
    }
    // items.push_back("偷加的");  // 編譯錯誤！const 引用

    return 0;
}
```

### 預期輸出

```
=== Getter 返回方式 ===
  修改拷貝後，實際金幣：500
  擁有者：勇者
  物品數量：3
    - 鐵劍
    - 治療藥水
    - 火炬
```

---

## 21.4 危險的 getter：返回非 const 引用

這是一個**極其常見的封裝破壞**模式：

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class BankAccount {
private:
    string owner_;
    int balance_;
    vector<string> transactionLog_;

public:
    BankAccount(const string& owner, int initial)
        : owner_(owner), balance_(initial)
    {
        transactionLog_.push_back("開戶：" + to_string(initial));
    }

    // ===== 安全的 getter =====
    int getBalance() const { return balance_; }
    const string& getOwner() const { return owner_; }
    const vector<string>& getLog() const { return transactionLog_; }

    // ===== 危險的 getter（反面教材）=====
    int& getBalanceDangerous() { return balance_; }
    vector<string>& getLogDangerous() { return transactionLog_; }

    // ===== 正確的修改介面 =====
    bool deposit(int amount) {
        if (amount <= 0) return false;
        balance_ += amount;
        transactionLog_.push_back("存入：" + to_string(amount));
        return true;
    }

    bool withdraw(int amount) {
        if (amount <= 0 || amount > balance_) return false;
        balance_ -= amount;
        transactionLog_.push_back("取出：" + to_string(amount));
        return true;
    }

    void printStatement() const {
        cout << "  帳戶：" << owner_ << endl;
        cout << "  餘額：" << balance_ << endl;
        cout << "  交易記錄：" << endl;
        for (const auto& log : transactionLog_) {
            cout << "    " << log << endl;
        }
    }
};

int main() {
    cout << "=== 危險的 getter 示範 ===" << endl;

    BankAccount account("陳信安", 1000);

    // 正常操作
    cout << "\n--- 正常操作 ---" << endl;
    account.deposit(500);
    account.withdraw(200);
    account.printStatement();

    // 使用危險的 getter 繞過所有保護！
    cout << "\n--- 使用危險的 getter ---" << endl;

    // 直接修改餘額，沒有驗證，沒有記錄！
    account.getBalanceDangerous() = 999999;
    cout << "  餘額被直接竄改為：" << account.getBalance() << endl;

    // 直接竄改交易記錄！
    account.getLogDangerous().clear();
    account.getLogDangerous().push_back("一切正常，沒有異常");

    cout << "\n--- 竄改後的帳戶 ---" << endl;
    account.printStatement();

    return 0;
}
```

### 預期輸出

```
=== 危險的 getter 示範 ===

--- 正常操作 ---
  帳戶：陳信安
  餘額：1300
  交易記錄：
    開戶：1000
    存入：500
    取出：200

--- 使用危險的 getter ---
  餘額被直接竄改為：999999

--- 竄改後的帳戶 ---
  帳戶：陳信安
  餘額：999999
  交易記錄：
    一切正常，沒有異常
```

**教訓**：返回非 `const` 引用等於**把鑰匙交給外人**，封裝形同虛設。

---

## 21.5 什麼時候不該寫 setter？

很多初學者會機械地為每個成員變數都寫 getter/setter，這是**錯誤的做法**：

```
❌ 錯誤思維：
  「每個 private 成員都需要 getter 和 setter」

✅ 正確思維：
  「只在有合理需求時才提供 getter 或 setter」
```

判斷原則：

```
是否需要 getter？
  問自己：外部是否有「讀取」這個數據的合理需求？
  └→ 是 → 提供 getter
  └→ 否 → 不提供（完全隱藏）

是否需要 setter？
  問自己：外部是否有「直接修改」這個數據的合理需求？
  └→ 否 → 不提供 setter
  └→ 是 → 再問：能否用更有意義的「行為」取代？
        └→ 能 → 提供行為函數（如 takeDamage、heal）
        └→ 不能 → 提供帶驗證的 setter
```

```cpp
#include <iostream>
#include <string>
using namespace std;

class Enemy {
private:
    string name_;
    int hp_;
    int maxHp_;
    int attackPower_;
    int internalId_;        // 內部 ID，外部完全不需要知道
    int aiState_;           // AI 狀態，純粹的內部邏輯

public:
    Enemy(const string& name, int maxHp, int atk)
        : name_(name), hp_(maxHp), maxHp_(maxHp)
        , attackPower_(atk)
        , internalId_(rand())  // 內部使用
        , aiState_(0)          // 內部使用
    {
    }

    // ===== 有 getter，沒有 setter =====
    // 外部需要讀取，但不能直接修改
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }

    // ===== 沒有 getter，也沒有 setter =====
    // internalId_ — 外部完全不需要知道
    // aiState_    — 純粹的內部邏輯

    // ===== 用「行為」取代 setter =====
    // 不提供 setHp()，而是提供有意義的行為：
    void takeDamage(int damage) {
        if (damage <= 0) return;
        hp_ = max(0, hp_ - damage);
        cout << "  " << name_ << " 受到 " << damage
             << " 傷害 (HP:" << hp_ << "/" << maxHp_ << ")" << endl;

        if (hp_ == 0) {
            cout << "  " << name_ << " 被擊敗！" << endl;
        }
    }

    // 不提供 setAttackPower()，而是：
    void enrage() {  // 暴怒：攻擊力翻倍
        attackPower_ *= 2;
        cout << "  " << name_ << " 進入暴怒狀態！ATK="
             << attackPower_ << endl;
    }

    int attack() {
        aiState_++;   // 內部狀態更新，外部不知道
        cout << "  " << name_ << " 發動攻擊！(ATK:"
             << attackPower_ << ")" << endl;
        return attackPower_;
    }

    bool isAlive() const { return hp_ > 0; }
};

int main() {
    cout << "=== 行為取代 setter ===" << endl;

    Enemy goblin("哥布林", 50, 15);
    cout << "  " << goblin.getName() << " HP:" << goblin.getHp()
         << "/" << goblin.getMaxHp() << endl;

    // 不是 goblin.setHp(30)，而是：
    goblin.takeDamage(20);

    // 不是 goblin.setAttackPower(30)，而是：
    goblin.enrage();

    // 正常攻擊
    int dmg = goblin.attack();
    cout << "  造成了 " << dmg << " 點傷害" << endl;

    // 以下都不可能做到（封裝保護）：
    // goblin.hp_ = 9999;          // 編譯錯誤
    // goblin.internalId_;         // 編譯錯誤
    // goblin.aiState_ = -1;       // 編譯錯誤

    return 0;
}
```

### 預期輸出

```
=== 行為取代 setter ===
  哥布林 HP:50/50
  哥布林 受到 20 傷害 (HP:30/50)
  哥布林 進入暴怒狀態！ATK=30
  哥布林 發動攻擊！(ATK:30)
  造成了 30 點傷害
```

---

## 21.6 getter/setter 命名慣例

C++ 中有幾種常見的命名風格：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 風格 1：get/set 前綴（Java 風格）=====
class Style1 {
private:
    int hp_;
    string name_;
public:
    int getHp() const { return hp_; }
    void setHp(int hp) { hp_ = hp; }
    const string& getName() const { return name_; }
    void setName(const string& name) { name_ = name; }
};

// ===== 風格 2：無前綴，同名函數重載（C++ 風格）=====
class Style2 {
private:
    int hp_;
    string name_;
public:
    // getter：無參數
    int hp() const { return hp_; }
    const string& name() const { return name_; }

    // setter：有參數
    void hp(int newHp) { hp_ = newHp; }
    void name(const string& newName) { name_ = newName; }
};

// ===== 風格 3：STL 風格（getter 用名詞，沒有 setter）=====
// STL 容器的做法：vector::size(), string::length()
// 通常不提供 setter，只提供行為函數

int main() {
    cout << "=== 命名風格比較 ===" << endl;

    // 風格 1 的使用
    cout << "\n--- 風格 1：get/set 前綴 ---" << endl;
    Style1 s1;
    s1.setHp(100);
    s1.setName("風格一");
    cout << "  " << s1.getName() << " HP:" << s1.getHp() << endl;

    // 風格 2 的使用
    cout << "\n--- 風格 2：同名重載 ---" << endl;
    Style2 s2;
    s2.hp(200);             // setter
    s2.name("風格二");       // setter
    cout << "  " << s2.name() << " HP:" << s2.hp() << endl;  // getter

    cout << "\n兩種風格都可以，關鍵是在項目中保持一致。" << endl;

    return 0;
}
```

### 預期輸出

```
=== 命名風格比較 ===

--- 風格 1：get/set 前綴 ---
  風格一 HP:100

--- 風格 2：同名重載 ---
  風格二 HP:200

兩種風格都可以，關鍵是在項目中保持一致。
```

在我們的課程中，我會統一使用**風格 1（get/set 前綴）**，因為它最清晰易讀。

---

## 21.7 setter 的鏈式調用（Method Chaining）

setter 可以返回 `*this` 來支持鏈式調用：

```cpp
#include <iostream>
#include <string>
using namespace std;

class DialogBox {
private:
    string title_;
    string message_;
    int width_;
    int height_;
    bool visible_;

public:
    DialogBox()
        : title_("未命名"), message_(""), width_(200), height_(100)
        , visible_(false)
    {
    }

    // setter 返回自身引用，支持鏈式調用
    DialogBox& setTitle(const string& title) {
        title_ = title;
        return *this;       // 返回自身
    }

    DialogBox& setMessage(const string& msg) {
        message_ = msg;
        return *this;
    }

    DialogBox& setSize(int w, int h) {
        width_ = (w > 0) ? w : 200;
        height_ = (h > 0) ? h : 100;
        return *this;
    }

    DialogBox& show() {
        visible_ = true;
        return *this;
    }

    void print() const {
        cout << "  ┌";
        for (int i = 0; i < 30; i++) cout << "─";
        cout << "┐" << endl;
        cout << "  │ [" << title_ << "]" << endl;
        cout << "  │ " << message_ << endl;
        cout << "  │ Size: " << width_ << "x" << height_ << endl;
        cout << "  │ Visible: " << (visible_ ? "Yes" : "No") << endl;
        cout << "  └";
        for (int i = 0; i < 30; i++) cout << "─";
        cout << "┘" << endl;
    }
};

int main() {
    cout << "=== 鏈式調用 ===" << endl;

    // 傳統方式：一行一行設定
    cout << "\n--- 傳統方式 ---" << endl;
    DialogBox dlg1;
    dlg1.setTitle("警告");
    dlg1.setMessage("你確定要刪除嗎？");
    dlg1.setSize(400, 200);
    dlg1.show();
    dlg1.print();

    // 鏈式調用：一氣呵成
    cout << "\n--- 鏈式調用 ---" << endl;
    DialogBox dlg2;
    dlg2.setTitle("歡迎")
        .setMessage("歡迎回到遊戲世界！")
        .setSize(500, 250)
        .show();
    dlg2.print();

    return 0;
}
```

### 預期輸出

```
=== 鏈式調用 ===

--- 傳統方式 ---
  ┌──────────────────────────────┐
  │ [警告]
  │ 你確定要刪除嗎？
  │ Size: 400x200
  │ Visible: Yes
  └──────────────────────────────┘

--- 鏈式調用 ---
  ┌──────────────────────────────┐
  │ [歡迎]
  │ 歡迎回到遊戲世界！
  │ Size: 500x250
  │ Visible: Yes
  └──────────────────────────────┘
```

鏈式調用的關鍵就是 `return *this;`——返回自身的引用，讓下一個 `.` 運算子可以繼續調用。

---

## 21.8 綜合範例：RPG 裝備系統

```cpp
#include <iostream>
#include <string>
using namespace std;

// 裝備稀有度
enum class Rarity {
    Common,      // 普通
    Uncommon,    // 非凡
    Rare,        // 稀有
    Epic,        // 史詩
    Legendary    // 傳說
};

string rarityToString(Rarity r) {
    switch (r) {
        case Rarity::Common:    return "普通";
        case Rarity::Uncommon:  return "非凡";
        case Rarity::Rare:      return "稀有";
        case Rarity::Epic:      return "史詩";
        case Rarity::Legendary: return "傳說";
    }
    return "未知";
}

class Equipment {
private:
    string name_;
    Rarity rarity_;
    int basePower_;          // 基礎能力值
    int enhanceLevel_;       // 強化等級
    int maxEnhanceLevel_;    // 最大強化等級（依稀有度決定）
    bool isEquipped_;        // 是否已裝備

    // 私有輔助：根據稀有度決定最大強化等級
    static int maxLevelForRarity(Rarity r) {
        switch (r) {
            case Rarity::Common:    return 5;
            case Rarity::Uncommon:  return 10;
            case Rarity::Rare:      return 15;
            case Rarity::Epic:      return 20;
            case Rarity::Legendary: return 25;
        }
        return 5;
    }

    // 私有輔助：計算實際能力值
    int calculatePower() const {
        // 每級強化增加 basePower_ 的 10%
        return basePower_ + (basePower_ * enhanceLevel_ / 10);
    }

public:
    Equipment(const string& name, Rarity rarity, int basePower)
        : name_(name)
        , rarity_(rarity)
        , basePower_(basePower > 0 ? basePower : 10)
        , enhanceLevel_(0)
        , maxEnhanceLevel_(maxLevelForRarity(rarity))
        , isEquipped_(false)
    {
    }

    // ====== Getter：只讀訪問 ======
    const string& getName() const { return name_; }
    Rarity getRarity() const { return rarity_; }
    int getBasePower() const { return basePower_; }
    int getEnhanceLevel() const { return enhanceLevel_; }
    int getPower() const { return calculatePower(); }   // 計算屬性，不是直接返回
    bool isEquipped() const { return isEquipped_; }

    // ====== 沒有直接的 setter！用行為取代 ======

    // 「強化」行為取代 setEnhanceLevel
    bool enhance() {
        if (enhanceLevel_ >= maxEnhanceLevel_) {
            cout << "  " << name_ << " 已達最大強化等級！" << endl;
            return false;
        }
        enhanceLevel_++;
        cout << "  ✦ " << name_ << " 強化成功！+"
             << enhanceLevel_ << " (能力:" << calculatePower() << ")" << endl;
        return true;
    }

    // 「裝備/卸下」行為取代 setIsEquipped
    void equip() {
        if (isEquipped_) {
            cout << "  " << name_ << " 已經裝備了" << endl;
            return;
        }
        isEquipped_ = true;
        cout << "  裝備了 " << name_ << endl;
    }

    void unequip() {
        if (!isEquipped_) {
            cout << "  " << name_ << " 並未裝備" << endl;
            return;
        }
        isEquipped_ = false;
        cout << "  卸下了 " << name_ << endl;
    }

    // 「改名」— 少數合理的類 setter 行為
    bool rename(const string& newName) {
        if (newName.empty()) {
            cout << "  名字不能為空！" << endl;
            return false;
        }
        if (newName.length() > 20) {
            cout << "  名字太長！（最多 20 個字元）" << endl;
            return false;
        }
        string oldName = name_;
        name_ = newName;
        cout << "  " << oldName << " → " << name_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  ┌───────────────────────────┐" << endl;
        cout << "  │ " << name_;
        if (enhanceLevel_ > 0) cout << " +" << enhanceLevel_;
        cout << endl;
        cout << "  │ 稀有度：" << rarityToString(rarity_) << endl;
        cout << "  │ 能力值：" << calculatePower()
             << " (基礎:" << basePower_ << ")" << endl;
        cout << "  │ 強化：" << enhanceLevel_
             << "/" << maxEnhanceLevel_ << endl;
        cout << "  │ 狀態：" << (isEquipped_ ? "已裝備" : "背包中") << endl;
        cout << "  └───────────────────────────┘" << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 21 課：getter/setter 設計 綜合範例" << endl;
    cout << "============================================" << endl;

    // 創建裝備
    cout << "\n=== 創建裝備 ===" << endl;
    Equipment sword("炎龍劍", Rarity::Epic, 50);
    Equipment shield("橡木盾", Rarity::Common, 30);

    sword.printInfo();
    shield.printInfo();

    // 裝備操作（行為，不是 setter）
    cout << "\n=== 裝備操作 ===" << endl;
    sword.equip();
    sword.equip();       // 重複裝備——被攔截
    shield.equip();

    // 強化（行為，不是 setter）
    cout << "\n=== 強化炎龍劍 ===" << endl;
    for (int i = 0; i < 5; i++) {
        sword.enhance();
    }

    // 強化普通盾牌到頂
    cout << "\n=== 強化橡木盾（普通裝備，上限 5）===" << endl;
    for (int i = 0; i < 7; i++) {
        shield.enhance();  // 第 6、7 次會失敗
    }

    // 改名
    cout << "\n=== 改名 ===" << endl;
    sword.rename("烈焰魔劍");
    sword.rename("");     // 被攔截

    // 使用 getter 查看數據
    cout << "\n=== 使用 Getter 查看 ===" << endl;
    cout << "  武器：" << sword.getName() << endl;
    cout << "  稀有度：" << rarityToString(sword.getRarity()) << endl;
    cout << "  實際能力：" << sword.getPower() << endl;
    cout << "  強化等級：" << sword.getEnhanceLevel() << endl;
    cout << "  已裝備：" << (sword.isEquipped() ? "是" : "否") << endl;

    // 最終狀態
    cout << "\n=== 最終狀態 ===" << endl;
    sword.printInfo();
    shield.printInfo();

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson21 lesson21.cpp
./lesson21
```

### 預期輸出

```
============================================
   第 21 課：getter/setter 設計 綜合範例
============================================

=== 創建裝備 ===
  ┌───────────────────────────┐
  │ 炎龍劍
  │ 稀有度：史詩
  │ 能力值：50 (基礎:50)
  │ 強化：0/20
  │ 狀態：背包中
  └───────────────────────────┘
  ┌───────────────────────────┐
  │ 橡木盾
  │ 稀有度：普通
  │ 能力值：30 (基礎:30)
  │ 強化：0/5
  │ 狀態：背包中
  └───────────────────────────┘

=== 裝備操作 ===
  裝備了 炎龍劍
  炎龍劍 已經裝備了
  裝備了 橡木盾

=== 強化炎龍劍 ===
  ✦ 炎龍劍 強化成功！+1 (能力:55)
  ✦ 炎龍劍 強化成功！+2 (能力:60)
  ✦ 炎龍劍 強化成功！+3 (能力:65)
  ✦ 炎龍劍 強化成功！+4 (能力:70)
  ✦ 炎龍劍 強化成功！+5 (能力:75)

=== 強化橡木盾（普通裝備，上限 5）===
  ✦ 橡木盾 強化成功！+1 (能力:33)
  ✦ 橡木盾 強化成功！+2 (能力:36)
  ✦ 橡木盾 強化成功！+3 (能力:39)
  ✦ 橡木盾 強化成功！+4 (能力:42)
  ✦ 橡木盾 強化成功！+5 (能力:45)
  橡木盾 已達最大強化等級！
  橡木盾 已達最大強化等級！

=== 改名 ===
  炎龍劍 → 烈焰魔劍
  名字不能為空！

=== 使用 Getter 查看 ===
  武器：烈焰魔劍
  稀有度：史詩
  實際能力：75
  強化等級：5
  已裝備：是

=== 最終狀態 ===
  ┌───────────────────────────┐
  │ 烈焰魔劍 +5
  │ 稀有度：史詩
  │ 能力值：75 (基礎:50)
  │ 強化：5/20
  │ 狀態：已裝備
  └───────────────────────────┘
  ┌───────────────────────────┐
  │ 橡木盾 +5
  │ 稀有度：普通
  │ 能力值：45 (基礎:30)
  │ 強化：5/5
  │ 狀態：已裝備
  └───────────────────────────┘
```

---

## 21.9 設計決策總結表

```
┌────────────────────┬─────────────┬──────────────────────────┐
│ 成員變數           │ Getter?     │ Setter?                  │
├────────────────────┼─────────────┼──────────────────────────┤
│ name_              │ ✅ getName  │ ❌ 用 rename() 行為取代   │
│ rarity_            │ ✅ getRarity│ ❌ 稀有度不可改           │
│ basePower_         │ ✅ 有       │ ❌ 基礎值不可改           │
│ enhanceLevel_      │ ✅ 有       │ ❌ 用 enhance() 行為取代  │
│ maxEnhanceLevel_   │ ❌ 不需要   │ ❌ 內部邏輯               │
│ isEquipped_        │ ✅ 有       │ ❌ 用 equip/unequip 取代  │
│ calculatePower()   │ ✅ getPower │ N/A（計算屬性）           │
└────────────────────┴─────────────┴──────────────────────────┘
```

---

## 21.10 本課重點回顧

| 概念 | 說明 |
|------|------|
| getter 的作用 | 提供私有數據的只讀訪問 |
| setter 的作用 | 提供帶驗證的受控修改 |
| 基本型別 getter | 返回值（`int getHp() const`） |
| 大型物件 getter | 返回 const 引用（`const string& getName() const`） |
| 危險 getter | 返回非 const 引用會破壞封裝 |
| 行為取代 setter | 用 `takeDamage()` 取代 `setHp()`，更有意義 |
| 不是每個成員都需要 getter/setter | 只在有合理需求時才提供 |
| 鏈式調用 | setter 返回 `*this` 支持連續調用 |
| 命名慣例 | `getX/setX`（Java 風格）或同名重載（C++ 風格） |
| 設計原則 | 暴露最少的介面，優先用行為取代直接修改 |

---

## 21.11 下一課預告

下一課是 **第 22 課：const 成員函數**。我們將深入探討：為什麼 getter 後面要加 `const`、`const` 對象只能調用 `const` 成員函數、以及 `const` 正確性在封裝中的重要角色。

準備好進入 **第 22 課：const 成員函數** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

// 裝備稀有度
enum class Rarity {
    Common,      // 普通
    Uncommon,    // 非凡
    Rare,        // 稀有
    Epic,        // 史詩
    Legendary    // 傳說
};

string rarityToString(Rarity r) {
    switch (r) {
        case Rarity::Common:    return "普通";
        case Rarity::Uncommon:  return "非凡";
        case Rarity::Rare:      return "稀有";
        case Rarity::Epic:      return "史詩";
        case Rarity::Legendary: return "傳說";
    }
    return "未知";
}

class Equipment {
private:
    string name_;
    Rarity rarity_;
    int basePower_;          // 基礎能力值
    int enhanceLevel_;       // 強化等級
    int maxEnhanceLevel_;    // 最大強化等級（依稀有度決定）
    bool isEquipped_;        // 是否已裝備

    // 私有輔助：根據稀有度決定最大強化等級
    static int maxLevelForRarity(Rarity r) {
        switch (r) {
            case Rarity::Common:    return 5;
            case Rarity::Uncommon:  return 10;
            case Rarity::Rare:      return 15;
            case Rarity::Epic:      return 20;
            case Rarity::Legendary: return 25;
        }
        return 5;
    }

    // 私有輔助：計算實際能力值
    int calculatePower() const {
        // 每級強化增加 basePower_ 的 10%
        return basePower_ + (basePower_ * enhanceLevel_ / 10);
    }

public:
    Equipment(const string& name, Rarity rarity, int basePower)
        : name_(name)
        , rarity_(rarity)
        , basePower_(basePower > 0 ? basePower : 10)
        , enhanceLevel_(0)
        , maxEnhanceLevel_(maxLevelForRarity(rarity))
        , isEquipped_(false)
    {
    }

    // ====== Getter：只讀訪問 ======
    const string& getName() const { return name_; }
    Rarity getRarity() const { return rarity_; }
    int getBasePower() const { return basePower_; }
    int getEnhanceLevel() const { return enhanceLevel_; }
    int getPower() const { return calculatePower(); }   // 計算屬性，不是直接返回
    bool isEquipped() const { return isEquipped_; }

    // ====== 沒有直接的 setter！用行為取代 ======

    // 「強化」行為取代 setEnhanceLevel
    bool enhance() {
        if (enhanceLevel_ >= maxEnhanceLevel_) {
            cout << "  " << name_ << " 已達最大強化等級！" << endl;
            return false;
        }
        enhanceLevel_++;
        cout << "  ✦ " << name_ << " 強化成功！+"
             << enhanceLevel_ << " (能力:" << calculatePower() << ")" << endl;
        return true;
    }

    // 「裝備/卸下」行為取代 setIsEquipped
    void equip() {
        if (isEquipped_) {
            cout << "  " << name_ << " 已經裝備了" << endl;
            return;
        }
        isEquipped_ = true;
        cout << "  裝備了 " << name_ << endl;
    }

    void unequip() {
        if (!isEquipped_) {
            cout << "  " << name_ << " 並未裝備" << endl;
            return;
        }
        isEquipped_ = false;
        cout << "  卸下了 " << name_ << endl;
    }

    // 「改名」— 少數合理的類 setter 行為
    bool rename(const string& newName) {
        if (newName.empty()) {
            cout << "  名字不能為空！" << endl;
            return false;
        }
        if (newName.length() > 20) {
            cout << "  名字太長！（最多 20 個字元）" << endl;
            return false;
        }
        string oldName = name_;
        name_ = newName;
        cout << "  " << oldName << " → " << name_ << endl;
        return true;
    }

    void printInfo() const {
        cout << "  ┌───────────────────────────┐" << endl;
        cout << "  │ " << name_;
        if (enhanceLevel_ > 0) cout << " +" << enhanceLevel_;
        cout << endl;
        cout << "  │ 稀有度：" << rarityToString(rarity_) << endl;
        cout << "  │ 能力值：" << calculatePower()
             << " (基礎:" << basePower_ << ")" << endl;
        cout << "  │ 強化：" << enhanceLevel_
             << "/" << maxEnhanceLevel_ << endl;
        cout << "  │ 狀態：" << (isEquipped_ ? "已裝備" : "背包中") << endl;
        cout << "  └───────────────────────────┘" << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 21 課：getter/setter 設計 綜合範例" << endl;
    cout << "============================================" << endl;

    // 創建裝備
    cout << "\n=== 創建裝備 ===" << endl;
    Equipment sword("炎龍劍", Rarity::Epic, 50);
    Equipment shield("橡木盾", Rarity::Common, 30);

    sword.printInfo();
    shield.printInfo();

    // 裝備操作（行為，不是 setter）
    cout << "\n=== 裝備操作 ===" << endl;
    sword.equip();
    sword.equip();       // 重複裝備——被攔截
    shield.equip();

    // 強化（行為，不是 setter）
    cout << "\n=== 強化炎龍劍 ===" << endl;
    for (int i = 0; i < 5; i++) {
        sword.enhance();
    }

    // 強化普通盾牌到頂
    cout << "\n=== 強化橡木盾（普通裝備，上限 5）===" << endl;
    for (int i = 0; i < 7; i++) {
        shield.enhance();  // 第 6、7 次會失敗
    }

    // 改名
    cout << "\n=== 改名 ===" << endl;
    sword.rename("烈焰魔劍");
    sword.rename("");     // 被攔截

    // 使用 getter 查看數據
    cout << "\n=== 使用 Getter 查看 ===" << endl;
    cout << "  武器：" << sword.getName() << endl;
    cout << "  稀有度：" << rarityToString(sword.getRarity()) << endl;
    cout << "  實際能力：" << sword.getPower() << endl;
    cout << "  強化等級：" << sword.getEnhanceLevel() << endl;
    cout << "  已裝備：" << (sword.isEquipped() ? "是" : "否") << endl;

    // 最終狀態
    cout << "\n=== 最終狀態 ===" << endl;
    sword.printInfo();
    shield.printInfo();

    return 0;
}
