/*
# 第 26 課：this 指標

---

## 26.1 this 到底是什麼？

在之前的課程中，我們多次提到 `this`，但還沒有系統地講解它。`this` 是一個**隱含的指標**，指向調用成員函數的那個對象本身：

```
當你寫：
  hero.takeDamage(30);

編譯器實際上做的是：
  takeDamage(&hero, 30);
             ↑↑↑↑↑↑
             this 就是這個

this 的類型：
  在普通成員函數中：Player* const this
  在 const 成員函數中：const Player* const this
```

圖解：

```
記憶體中：
  ┌──────────────────┐
  │ hero 對象         │ ← 地址：0x7fff1234
  │ ┌──────────────┐ │
  │ │ name_: "勇者" │ │
  │ │ hp_: 100     │ │
  │ │ atk_: 25     │ │
  │ └──────────────┘ │
  └──────────────────┘

  hero.takeDamage(30);
  → this = &hero = 0x7fff1234
  → this->hp_ 就是 hero.hp_
```

---

## 26.2 基礎驗證：this 指向誰？

```cpp
#include <iostream>
#include <string>
using namespace std;

class Knight {
private:
    string name_;
    int hp_;

public:
    Knight(const string& name, int hp)
        : name_(name), hp_(hp)
    {
    }

    void showThis() const {
        cout << "  " << name_ << " 的地址：" << this << endl;
    }

    void takeDamage(int dmg) {
        cout << "  this = " << this << " → " << name_
             << " 受到 " << dmg << " 傷害" << endl;
        hp_ -= dmg;
    }

    void printInfo() const {
        // 以下兩種寫法完全等價：
        cout << "  名字：" << name_ << endl;        // 隱式使用 this
        cout << "  名字：" << this->name_ << endl;   // 顯式使用 this
        // 編譯器會把 name_ 自動轉成 this->name_
    }
};

int main() {
    cout << "=== this 指向誰？ ===" << endl;

    Knight k1("亞瑟", 200);
    Knight k2("蘭斯洛特", 180);

    // 驗證 this 就是對象的地址
    cout << "\n--- 地址比較 ---" << endl;
    cout << "  &k1 = " << &k1 << endl;
    k1.showThis();
    cout << "  &k2 = " << &k2 << endl;
    k2.showThis();

    // 不同對象調用同一個函數，this 不同
    cout << "\n--- 不同對象，不同 this ---" << endl;
    k1.takeDamage(30);
    k2.takeDamage(50);

    // 驗證隱式 vs 顯式
    cout << "\n--- 隱式 vs 顯式 this ---" << endl;
    k1.printInfo();

    return 0;
}
```

### 預期輸出

```
=== this 指向誰？ ===

--- 地址比較 ---
  &k1 = 0x7fffXXXXXXX0
  亞瑟 的地址：0x7fffXXXXXXX0
  &k2 = 0x7fffXXXXXXX8
  蘭斯洛特 的地址：0x7fffXXXXXXX8

--- 不同對象，不同 this ---
  this = 0x7fffXXXXXXX0 → 亞瑟 受到 30 傷害
  this = 0x7fffXXXXXXX8 → 蘭斯洛特 受到 50 傷害

--- 隱式 vs 顯式 this ---
  名字：亞瑟
  名字：亞瑟
```

---

## 26.3 必須使用 this 的場景

大多數時候 `this` 是隱式的，不需要寫出來。但有幾個場景**必須**顯式使用 `this`：

### 場景一：參數名與成員變數同名

```cpp
#include <iostream>
#include <string>
using namespace std;

class Weapon {
private:
    string name;
    int damage;
    int durability;

public:
    // 參數名和成員名完全相同！
    Weapon(const string& name, int damage, int durability) {
        // 不用 this 的話，name = name 是自我賦值（參數給參數）
        // 必須用 this-> 區分：
        this->name = name;
        this->damage = damage;
        this->durability = durability;
    }

    // setter 也常遇到同名問題
    void setDamage(int damage) {
        if (damage < 0) damage = 0;   // 這裡的 damage 是參數
        this->damage = damage;         // this->damage 是成員
    }

    void print() const {
        cout << "  " << name << " (傷害:" << damage
             << " 耐久:" << durability << ")" << endl;
    }
};

int main() {
    cout << "=== 場景一：同名消歧 ===" << endl;

    Weapon sword("鐵劍", 25, 100);
    sword.print();

    sword.setDamage(40);
    sword.print();

    return 0;
}
```

### 預期輸出

```
=== 場景一：同名消歧 ===
  鐵劍 (傷害:25 耐久:100)
  鐵劍 (傷害:40 耐久:100)
```

不過在我們的課程中，我們用**成員加下劃線後綴**（如 `name_`）來避免同名問題，這是更好的做法：

```cpp
// 推薦風格：不需要 this 來消歧
Weapon(const string& name, int damage, int durability)
    : name_(name), damage_(damage), durability_(durability)
{
}
```

---

### 場景二：返回自身引用（鏈式調用）

這是我們在第 21 課已經見過的，現在來深入理解原理：

```cpp
#include <iostream>
#include <string>
using namespace std;

class QueryBuilder {
private:
    string table_;
    string conditions_;
    string orderBy_;
    int limit_;

public:
    QueryBuilder() : limit_(0) {}

    // 每個函數返回 *this（自身的引用）
    QueryBuilder& from(const string& table) {
        table_ = table;
        return *this;    // *this 解引用 this 指標，得到對象本身
    }

    QueryBuilder& where(const string& condition) {
        if (!conditions_.empty()) conditions_ += " AND ";
        conditions_ += condition;
        return *this;
    }

    QueryBuilder& orderBy(const string& field) {
        orderBy_ = field;
        return *this;
    }

    QueryBuilder& limit(int n) {
        limit_ = (n > 0) ? n : 0;
        return *this;
    }

    string build() const {
        string sql = "SELECT * FROM " + table_;
        if (!conditions_.empty()) sql += " WHERE " + conditions_;
        if (!orderBy_.empty()) sql += " ORDER BY " + orderBy_;
        if (limit_ > 0) sql += " LIMIT " + to_string(limit_);
        return sql;
    }
};

int main() {
    cout << "=== 場景二：鏈式調用 ===" << endl;

    // 鏈式調用的原理：
    // query.from("players")  → 返回 query 自身的引用
    //       .where("lv > 10")→ 在返回的引用上繼續調用 → 返回 query 引用
    //       .orderBy("lv")   → 繼續...
    //       .limit(20)       → 繼續...
    //       .build()         → 最後產出 SQL

    string sql = QueryBuilder()
        .from("players")
        .where("level > 10")
        .where("hp > 0")
        .orderBy("level DESC")
        .limit(20)
        .build();

    cout << "  " << sql << endl;

    cout << "\n--- 另一個查詢 ---" << endl;
    string sql2 = QueryBuilder()
        .from("items")
        .where("rarity = 'legendary'")
        .where("price < 10000")
        .limit(5)
        .build();

    cout << "  " << sql2 << endl;

    return 0;
}
```

### 預期輸出

```
=== 場景二：鏈式調用 ===
  SELECT * FROM players WHERE level > 10 AND hp > 0 ORDER BY level DESC LIMIT 20

--- 另一個查詢 ---
  SELECT * FROM items WHERE rarity = 'legendary' AND price < 10000 LIMIT 5
```

鏈式調用的內部原理：

```
QueryBuilder q;

q.from("players").where("lv > 10").limit(20).build();

展開後等價於：

QueryBuilder& ref1 = q.from("players");   // ref1 就是 q
QueryBuilder& ref2 = ref1.where("lv>10"); // ref2 也是 q
QueryBuilder& ref3 = ref2.limit(20);      // ref3 也是 q
string sql = ref3.build();                 // 最終在 q 上調用

全程都是同一個對象 q！
```

---

### 場景三：將自身傳遞給其他函數

```cpp
#include <iostream>
#include <string>
using namespace std;

class Player;  // 前向聲明

// 外部系統：接收 Player 的引用或指標
void registerPlayer(Player* p);
void logAction(const Player& p, const string& action);

class Player {
private:
    string name_;
    int hp_;

public:
    Player(const string& name, int hp)
        : name_(name), hp_(hp)
    {
        // 在建構時把自身註冊到系統
        registerPlayer(this);     // 傳遞 this 指標
    }

    void attack(Player& target) {
        // 把自身的引用傳給日誌系統
        logAction(*this, "攻擊了 " + target.getName());
        target.takeDamage(20);
    }

    void takeDamage(int dmg) {
        hp_ -= dmg;
        logAction(*this, "受到 " + to_string(dmg) + " 傷害");
    }

    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
};

// 外部函數的實現
void registerPlayer(Player* p) {
    cout << "  [系統] 註冊玩家：" << p->getName()
         << " (地址:" << p << ")" << endl;
}

void logAction(const Player& p, const string& action) {
    cout << "  [日誌] " << p.getName() << " " << action
         << " (HP:" << p.getHp() << ")" << endl;
}

int main() {
    cout << "=== 場景三：傳遞 this ===" << endl;

    cout << "\n--- 創建玩家 ---" << endl;
    Player warrior("戰士", 200);
    Player mage("法師", 120);

    cout << "\n--- 戰鬥 ---" << endl;
    warrior.attack(mage);
    mage.attack(warrior);

    return 0;
}
```

### 預期輸出

```
=== 場景三：傳遞 this ===

--- 創建玩家 ---
  [系統] 註冊玩家：戰士 (地址:0x7fffXXXXXXXX)
  [系統] 註冊玩家：法師 (地址:0x7fffXXXXXXXX)

--- 戰鬥 ---
  [日誌] 戰士 攻擊了 法師 (HP:200)
  [日誌] 法師 受到 20 傷害 (HP:100)
  [日誌] 法師 攻擊了 戰士 (HP:100)
  [日誌] 戰士 受到 20 傷害 (HP:180)
```

```
this 的使用方式：
  this       → Player* 指標（傳遞地址）
  *this      → Player& 引用（傳遞對象本身）
  this->name_→ 訪問成員
```

---

### 場景四：自我賦值檢查

在之後的拷貝賦值運算子中會詳細學到，這裡先預覽：

```cpp
#include <iostream>
using namespace std;

class Buffer {
private:
    int* data_;
    int size_;

public:
    Buffer(int size) : size_(size), data_(new int[size]) {
        for (int i = 0; i < size; i++) data_[i] = i;
        cout << "  [建構] 大小:" << size_ << endl;
    }

    ~Buffer() {
        delete[] data_;
        cout << "  [解構]" << endl;
    }

    // 拷貝賦值運算子——必須檢查自我賦值
    Buffer& operator=(const Buffer& other) {
        cout << "  [賦值] ";

        // 自我賦值檢查：this == &other 嗎？
        if (this == &other) {
            cout << "偵測到自我賦值，跳過" << endl;
            return *this;
        }

        // 正常賦值邏輯
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        for (int i = 0; i < size_; i++) data_[i] = other.data_[i];
        cout << "完成" << endl;

        return *this;
    }

    void print() const {
        cout << "  [";
        for (int i = 0; i < size_; i++) {
            if (i > 0) cout << ", ";
            cout << data_[i];
        }
        cout << "]" << endl;
    }
};

int main() {
    cout << "=== 場景四：自我賦值檢查 ===" << endl;

    Buffer buf(5);
    buf.print();

    // 自我賦值：buf = buf
    buf = buf;   // this == &other，被安全攔截
    buf.print();

    return 0;
}
```

### 預期輸出

```
=== 場景四：自我賦值檢查 ===
  [建構] 大小:5
  [0, 1, 2, 3, 4]
  [賦值] 偵測到自我賦值，跳過
  [0, 1, 2, 3, 4]
  [解構]
```

如果不檢查自我賦值，`delete[] data_` 會先銷毀數據，然後 `data_ = new int[size_]; ... data_[i] = other.data_[i]` 就在讀已經被釋放的記憶體——**未定義行為**！

---

## 26.4 this 指標的完整類型分析

```cpp
#include <iostream>
using namespace std;

class TypeDemo {
private:
    int value_;

public:
    TypeDemo(int v) : value_(v) {}

    // 普通成員函數
    void normalFunc() {
        // this 的類型：TypeDemo* const
        // → 指向非 const 的 TypeDemo
        // → 指標本身是 const（不能讓 this 指向別的對象）

        this->value_ = 42;     // ✅ 可以修改
        // this = nullptr;     // ❌ this 本身是 const
    }

    // const 成員函數
    void constFunc() const {
        // this 的類型：const TypeDemo* const
        // → 指向 const 的 TypeDemo
        // → 指標本身也是 const

        // this->value_ = 42;  // ❌ 指向 const 對象，不能修改
        // this = nullptr;     // ❌ this 本身也是 const
        int v = this->value_;  // ✅ 可以讀取
    }
};

int main() {
    cout << "=== this 的類型 ===" << endl;

    cout << "  普通成員函數中：TypeDemo* const this" << endl;
    cout << "  const 成員函數中：const TypeDemo* const this" << endl;
    cout << "  靜態成員函數中：沒有 this" << endl;

    return 0;
}
```

整理成表：

```
┌──────────────────────┬───────────────────────────┐
│ 函數類型             │ this 的類型                │
├──────────────────────┼───────────────────────────┤
│ void func()          │ MyClass* const this        │
│ void func() const    │ const MyClass* const this  │
│ static void func()   │ 沒有 this                 │
└──────────────────────┴───────────────────────────┘

解讀：
  MyClass* const this
  ├── MyClass*  → 指向 MyClass 類型
  ├── * const   → 指標本身不能改（不能讓 this 指向別處）
  └── 指向的內容可以改（可以修改成員）

  const MyClass* const this
  ├── const MyClass* → 指向 const MyClass
  ├── * const        → 指標本身不能改
  └── 指向的內容也不能改（不能修改成員）
```

---

## 26.5 this 與靜態成員的關係

```cpp
#include <iostream>
using namespace std;

class Demo {
private:
    int instanceVar_ = 10;
    inline static int staticVar_ = 20;

public:
    void normalFunc() {
        // 有 this
        cout << "  this = " << this << endl;
        cout << "  instanceVar_ = " << this->instanceVar_ << endl;  // ✅
        cout << "  staticVar_ = " << staticVar_ << endl;             // ✅ 不用 this
    }

    static void staticFunc() {
        // 沒有 this
        // cout << this;               // ❌ 編譯錯誤
        // cout << instanceVar_;        // ❌ 需要 this
        cout << "  staticVar_ = " << staticVar_ << endl;  // ✅
    }
};

int main() {
    cout << "=== this 與靜態成員 ===" << endl;

    Demo d;
    cout << "\n--- 普通函數（有 this）---" << endl;
    d.normalFunc();

    cout << "\n--- 靜態函數（沒有 this）---" << endl;
    Demo::staticFunc();

    return 0;
}
```

### 預期輸出

```
=== this 與靜態成員 ===

--- 普通函數（有 this）---
  this = 0x7fffXXXXXXXX
  instanceVar_ = 10
  staticVar_ = 20

--- 靜態函數（沒有 this）---
  staticVar_ = 20
```

---

## 26.6 this 的常見誤區

### 誤區一：在建構函數中洩漏 this

```cpp
#include <iostream>
#include <string>
using namespace std;

class Dangerous {
private:
    int value_;

public:
    Dangerous(int v) : value_(v) {
        // ⚠ 在建構函數中，對象還沒完全初始化
        // 如果把 this 傳給外部，外部可能會使用未初始化的部分

        cout << "  建構中... value_ = " << value_ << endl;
        cout << "  this = " << this << " (對象可能還沒完全就緒)" << endl;

        // 這裡把 this 傳出去是危險的：
        // someGlobalFunction(this);  // ⚠ 對象可能還沒完全建構
    }
};

int main() {
    cout << "=== 誤區一：建構中洩漏 this ===" << endl;
    Dangerous d(42);
    cout << "  建構完成，現在使用 this 才安全" << endl;
    return 0;
}
```

### 誤區二：返回局部對象的 this

```cpp
#include <iostream>
using namespace std;

class Trap {
private:
    int value_;

public:
    Trap(int v) : value_(v) {}

    // ❌ 危險！返回指向局部對象的指標
    // static Trap* createBad() {
    //     Trap local(99);
    //     return &local;   // local 離開作用域就死了！
    // }

    // ✅ 安全：返回動態分配的對象
    static Trap* createGood() {
        return new Trap(99);   // 堆上的對象不會自動銷毀
    }

    // ✅ 安全：返回值（拷貝）
    static Trap createBest() {
        return Trap(99);       // 返回值，拷貝/移動到調用方
    }

    int getValue() const { return value_; }
};

int main() {
    cout << "=== 誤區二：安全的創建方式 ===" << endl;

    Trap* p = Trap::createGood();
    cout << "  動態創建：" << p->getValue() << endl;
    delete p;

    Trap t = Trap::createBest();
    cout << "  值返回：" << t.getValue() << endl;

    return 0;
}
```

### 預期輸出

```
=== 誤區二：安全的創建方式 ===
  動態創建：99
  值返回：99
```

---

## 26.7 解引用 this：*this 的各種用法

```cpp
#include <iostream>
#include <string>
using namespace std;

class Score {
private:
    string playerName_;
    int points_;

public:
    Score(const string& name, int pts)
        : playerName_(name), points_(pts)
    {
    }

    // 用法 1：返回自身引用（鏈式調用）
    Score& addPoints(int pts) {
        points_ += pts;
        return *this;    // *this 是 Score&
    }

    // 用法 2：返回自身的拷貝（不修改原物件）
    Score doubled() const {
        Score copy = *this;     // 拷貝自身
        copy.points_ *= 2;
        return copy;
    }

    // 用法 3：比較自身與另一個對象
    bool isHigherThan(const Score& other) const {
        // 先檢查是不是和自己比
        if (this == &other) return false;
        return points_ > other.points_;
    }

    // 用法 4：把自身傳給外部函數
    void printVia(void (*printFunc)(const Score&)) const {
        printFunc(*this);    // 把自身傳出去
    }

    const string& getName() const { return playerName_; }
    int getPoints() const { return points_; }
};

// 外部的列印函數
void fancyPrint(const Score& s) {
    cout << "  ★ " << s.getName() << "：" << s.getPoints() << " 分 ★" << endl;
}

int main() {
    cout << "=== *this 的各種用法 ===" << endl;

    Score s("陳信安", 100);

    // 用法 1：鏈式調用
    cout << "\n--- 鏈式加分 ---" << endl;
    s.addPoints(50).addPoints(30).addPoints(20);
    cout << "  總分：" << s.getPoints() << endl;

    // 用法 2：拷貝自身
    cout << "\n--- 翻倍（不影響原物件）---" << endl;
    Score doubled = s.doubled();
    cout << "  原始：" << s.getPoints() << endl;
    cout << "  翻倍：" << doubled.getPoints() << endl;

    // 用法 3：比較
    cout << "\n--- 比較 ---" << endl;
    Score other("對手", 150);
    cout << "  " << s.getName() << " 高於 " << other.getName() << "？ "
         << (s.isHigherThan(other) ? "是" : "否") << endl;
    cout << "  和自己比？ "
         << (s.isHigherThan(s) ? "是" : "否") << endl;

    // 用法 4：傳給外部函數
    cout << "\n--- 傳遞 *this ---" << endl;
    s.printVia(fancyPrint);

    return 0;
}
```

### 預期輸出

```
=== *this 的各種用法 ===

--- 鏈式加分 ---
  總分：200

--- 翻倍（不影響原物件）---
  原始：200
  翻倍：400

--- 比較 ---
  陳信安 高於 對手？ 否
  和自己比？ 否

--- 傳遞 *this ---
  ★ 陳信安：200 分 ★
```

---

## 26.8 綜合範例：RPG 隊伍系統

```cpp
#include <iostream>
#include <string>
using namespace std;

class Hero {
private:
    string name_;
    string role_;      // 職業
    int hp_;
    int maxHp_;
    int attack_;
    Hero* leader_;     // 隊長指標

    inline static int heroCount_ = 0;

public:
    Hero(const string& name, const string& role, int maxHp, int atk)
        : name_(name), role_(role)
        , hp_(maxHp), maxHp_(maxHp), attack_(atk)
        , leader_(nullptr)
    {
        heroCount_++;
        cout << "  [加入] " << role_ << " " << name_ << endl;
    }

    // ====== this 的各種應用 ======

    // 應用 1：設定隊長——把自己設為隊長
    Hero& becomeLeader() {
        leader_ = this;     // 自己就是自己的隊長
        cout << "  ★ " << name_ << " 成為隊長！" << endl;
        return *this;
    }

    // 應用 2：跟隨另一個英雄——需要比較 this
    Hero& follow(Hero& other) {
        if (this == &other) {
            cout << "  " << name_ << "：不能跟隨自己" << endl;
            return *this;
        }
        leader_ = &other;
        cout << "  " << name_ << " 跟隨 " << other.name_ << endl;
        return *this;
    }

    // 應用 3：鏈式強化
    Hero& boostHp(int amount) {
        maxHp_ += amount;
        hp_ += amount;
        cout << "  " << name_ << " HP+" << amount << endl;
        return *this;
    }

    Hero& boostAttack(int amount) {
        attack_ += amount;
        cout << "  " << name_ << " ATK+" << amount << endl;
        return *this;
    }

    // 應用 4：對比——誰更適合當隊長
    const Hero& betterLeader(const Hero& other) const {
        if (this == &other) return *this;
        // 比較綜合能力
        int myScore = maxHp_ + attack_ * 3;
        int otherScore = other.maxHp_ + other.attack_ * 3;
        if (myScore >= otherScore) return *this;
        return other;
    }

    // 應用 5：治療另一個英雄
    void heal(Hero& target, int amount) {
        if (this == &target) {
            cout << "  " << name_ << " 自我治療 ";
        } else {
            cout << "  " << name_ << " 治療 " << target.name_ << " ";
        }
        int actual = amount;
        if (target.hp_ + actual > target.maxHp_) {
            actual = target.maxHp_ - target.hp_;
        }
        target.hp_ += actual;
        cout << "+" << actual << " (HP:" << target.hp_
             << "/" << target.maxHp_ << ")" << endl;
    }

    // Getter
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    void takeDamage(int dmg) {
        hp_ = (hp_ - dmg > 0) ? hp_ - dmg : 0;
    }

    void printStatus() const {
        cout << "  [" << role_ << "] " << name_
             << " HP:" << hp_ << "/" << maxHp_
             << " ATK:" << attack_;
        if (leader_) {
            if (leader_ == this)
                cout << " (隊長)";
            else
                cout << " (跟隨:" << leader_->name_ << ")";
        }
        cout << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 26 課：this 指標 綜合範例" << endl;
    cout << "============================================" << endl;

    // 創建隊伍
    cout << "\n=== 創建英雄 ===" << endl;
    Hero warrior("亞瑟", "戰士", 300, 40);
    Hero mage("梅林", "法師", 150, 70);
    Hero healer("伊蓮", "治療師", 200, 20);

    // 鏈式強化
    cout << "\n=== 鏈式強化 ===" << endl;
    warrior.boostHp(50).boostAttack(10).boostHp(30);
    mage.boostAttack(20);

    // 決定隊長
    cout << "\n=== 選擇隊長 ===" << endl;
    const Hero& better = warrior.betterLeader(mage);
    cout << "  更適合當隊長：" << better.getName() << endl;

    // 組隊
    cout << "\n=== 組隊 ===" << endl;
    warrior.becomeLeader();
    mage.follow(warrior);
    healer.follow(warrior);

    // 嘗試跟隨自己
    warrior.follow(warrior);

    // 隊伍狀態
    cout << "\n=== 隊伍狀態 ===" << endl;
    warrior.printStatus();
    mage.printStatus();
    healer.printStatus();

    // 戰鬥
    cout << "\n=== 戰鬥 ===" << endl;
    warrior.takeDamage(120);
    mage.takeDamage(80);
    cout << "  亞瑟 HP:" << warrior.getHp() << endl;
    cout << "  梅林 HP:" << mage.getHp() << endl;

    // 治療
    cout << "\n=== 治療 ===" << endl;
    healer.heal(warrior, 60);
    healer.heal(mage, 100);
    healer.heal(healer, 30);    // 自我治療

    // 最終狀態
    cout << "\n=== 最終狀態 ===" << endl;
    warrior.printStatus();
    mage.printStatus();
    healer.printStatus();

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson26 lesson26.cpp
./lesson26
```

### 預期輸出

```
============================================
   第 26 課：this 指標 綜合範例
============================================

=== 創建英雄 ===
  [加入] 戰士 亞瑟
  [加入] 法師 梅林
  [加入] 治療師 伊蓮

=== 鏈式強化 ===
  亞瑟 HP+50
  亞瑟 ATK+10
  亞瑟 HP+30
  梅林 ATK+20

=== 選擇隊長 ===
  更適合當隊長：梅林

=== 組隊 ===
  ★ 亞瑟 成為隊長！
  梅林 跟隨 亞瑟
  伊蓮 跟隨 亞瑟
  亞瑟：不能跟隨自己

=== 隊伍狀態 ===
  [戰士] 亞瑟 HP:380/380 ATK:50 (隊長)
  [法師] 梅林 HP:150/150 ATK:90 (跟隨:亞瑟)
  [治療師] 伊蓮 HP:200/200 ATK:20 (跟隨:亞瑟)

=== 戰鬥 ===
  亞瑟 HP:260
  梅林 HP:70

=== 治療 ===
  伊蓮 治療 亞瑟 +60 (HP:320/380)
  伊蓮 治療 梅林 +80 (HP:150/150)
  伊蓮 自我治療 +0 (HP:200/200)

=== 最終狀態 ===
  [戰士] 亞瑟 HP:320/380 ATK:50 (隊長)
  [法師] 梅林 HP:150/150 ATK:90 (跟隨:亞瑟)
  [治療師] 伊蓮 HP:200/200 ATK:20 (跟隨:亞瑟)
```

---

## 26.9 this 指標使用場景總結

```
┌─────────────────────────────┬────────────────────────────┐
│ 場景                         │ 用法                       │
├─────────────────────────────┼────────────────────────────┤
│ 參數與成員同名               │ this->name = name;         │
│ 鏈式調用                     │ return *this;              │
│ 傳遞自身給外部函數（指標）    │ func(this);               │
│ 傳遞自身給外部函數（引用）    │ func(*this);              │
│ 自我賦值檢查                 │ if (this == &other)        │
│ 自我操作檢查                 │ if (this == &target)       │
│ 比較後返回自身               │ return *this;              │
│ 拷貝自身                     │ MyClass copy = *this;      │
└─────────────────────────────┴────────────────────────────┘
```

---

## 26.10 本課重點回顧

| 概念 | 說明 |
|------|------|
| this 本質 | 指向當前對象的隱含指標 |
| this 的類型 | 普通函數：`T* const`；const 函數：`const T* const` |
| 靜態函數 | 沒有 this |
| `this->member` | 顯式訪問成員（通常可省略） |
| `*this` | 解引用得到對象本身，用於返回引用或傳遞 |
| `return *this` | 鏈式調用的核心 |
| `this == &other` | 自我賦值/自我操作檢查 |
| 同名消歧 | 用 `this->` 區分參數和成員（建議用下劃線後綴避免） |
| 建構中的 this | 對象未完全建構，傳出 this 有風險 |

---

## 26.11 第四階段總結

恭喜你完成了**第四階段：封裝深入**！回顧整個階段：

| 課次 | 主題 | 核心概念 |
|------|------|----------|
| 第 20 課 | 封裝的意義 | 不變量、三層防護、介面穩定性 |
| 第 21 課 | getter/setter | 返回方式、行為取代 setter、鏈式調用 |
| 第 22 課 | const 成員函數 | const 正確性、const 重載、調用鏈 |
| 第 23 課 | mutable | 快取、延遲初始化、邏輯狀態 vs 實現細節 |
| 第 24 課 | 靜態成員變數 | 類別共享數據、inline static、constexpr static |
| 第 25 課 | 靜態成員函數 | 無 this、工廠函數、工具類、單例 |
| 第 26 課 | this 指標 | 隱含指標、鏈式調用、自我檢查 |

這七課構成了封裝的完整體系：

```
封裝的完整圖景：
  ┌─────────────────────────────────────────┐
  │ private（數據隱藏）                      │
  │   ├── 普通成員變數（每個對象一份）        │
  │   ├── 靜態成員變數（類別共享一份）        │
  │   └── mutable 成員（const 中可修改）     │
  │                                         │
  │ public（受控介面）                       │
  │   ├── getter（const，只讀訪問）          │
  │   ├── setter / 行為函數（帶驗證的修改）   │
  │   ├── 靜態函數（無 this，類別級操作）     │
  │   └── this 指標（自身引用、鏈式調用）     │
  └─────────────────────────────────────────┘
```

---

## 26.12 下一課預告

下一課進入 **第五階段：拷貝與移動語義**，從 **第 27 課：淺拷貝與深拷貝** 開始。這是 C++ 中非常重要的概念——當你的類別管理動態記憶體時，預設的拷貝行為會導致災難性的 bug。我們會深入理解為什麼，以及如何正確處理。

準備好進入 **第 27 課：淺拷貝與深拷貝** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

class Hero {
private:
    string name_;
    string role_;      // 職業
    int hp_;
    int maxHp_;
    int attack_;
    Hero* leader_;     // 隊長指標

    inline static int heroCount_ = 0;

public:
    Hero(const string& name, const string& role, int maxHp, int atk)
        : name_(name), role_(role)
        , hp_(maxHp), maxHp_(maxHp), attack_(atk)
        , leader_(nullptr)
    {
        heroCount_++;
        cout << "  [加入] " << role_ << " " << name_ << endl;
    }

    // ====== this 的各種應用 ======

    // 應用 1：設定隊長——把自己設為隊長
    Hero& becomeLeader() {
        leader_ = this;     // 自己就是自己的隊長
        cout << "  ★ " << name_ << " 成為隊長！" << endl;
        return *this;
    }

    // 應用 2：跟隨另一個英雄——需要比較 this
    Hero& follow(Hero& other) {
        if (this == &other) {
            cout << "  " << name_ << "：不能跟隨自己" << endl;
            return *this;
        }
        leader_ = &other;
        cout << "  " << name_ << " 跟隨 " << other.name_ << endl;
        return *this;
    }

    // 應用 3：鏈式強化
    Hero& boostHp(int amount) {
        maxHp_ += amount;
        hp_ += amount;
        cout << "  " << name_ << " HP+" << amount << endl;
        return *this;
    }

    Hero& boostAttack(int amount) {
        attack_ += amount;
        cout << "  " << name_ << " ATK+" << amount << endl;
        return *this;
    }

    // 應用 4：對比——誰更適合當隊長
    const Hero& betterLeader(const Hero& other) const {
        if (this == &other) return *this;
        // 比較綜合能力
        int myScore = maxHp_ + attack_ * 3;
        int otherScore = other.maxHp_ + other.attack_ * 3;
        if (myScore >= otherScore) return *this;
        return other;
    }

    // 應用 5：治療另一個英雄
    void heal(Hero& target, int amount) {
        if (this == &target) {
            cout << "  " << name_ << " 自我治療 ";
        } else {
            cout << "  " << name_ << " 治療 " << target.name_ << " ";
        }
        int actual = amount;
        if (target.hp_ + actual > target.maxHp_) {
            actual = target.maxHp_ - target.hp_;
        }
        target.hp_ += actual;
        cout << "+" << actual << " (HP:" << target.hp_
             << "/" << target.maxHp_ << ")" << endl;
    }

    // Getter
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }

    void takeDamage(int dmg) {
        hp_ = (hp_ - dmg > 0) ? hp_ - dmg : 0;
    }

    void printStatus() const {
        cout << "  [" << role_ << "] " << name_
             << " HP:" << hp_ << "/" << maxHp_
             << " ATK:" << attack_;
        if (leader_) {
            if (leader_ == this)
                cout << " (隊長)";
            else
                cout << " (跟隨:" << leader_->name_ << ")";
        }
        cout << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 26 課：this 指標 綜合範例" << endl;
    cout << "============================================" << endl;

    // 創建隊伍
    cout << "\n=== 創建英雄 ===" << endl;
    Hero warrior("亞瑟", "戰士", 300, 40);
    Hero mage("梅林", "法師", 150, 70);
    Hero healer("伊蓮", "治療師", 200, 20);

    // 鏈式強化
    cout << "\n=== 鏈式強化 ===" << endl;
    warrior.boostHp(50).boostAttack(10).boostHp(30);
    mage.boostAttack(20);

    // 決定隊長
    cout << "\n=== 選擇隊長 ===" << endl;
    const Hero& better = warrior.betterLeader(mage);
    cout << "  更適合當隊長：" << better.getName() << endl;

    // 組隊
    cout << "\n=== 組隊 ===" << endl;
    warrior.becomeLeader();
    mage.follow(warrior);
    healer.follow(warrior);

    // 嘗試跟隨自己
    warrior.follow(warrior);

    // 隊伍狀態
    cout << "\n=== 隊伍狀態 ===" << endl;
    warrior.printStatus();
    mage.printStatus();
    healer.printStatus();

    // 戰鬥
    cout << "\n=== 戰鬥 ===" << endl;
    warrior.takeDamage(120);
    mage.takeDamage(80);
    cout << "  亞瑟 HP:" << warrior.getHp() << endl;
    cout << "  梅林 HP:" << mage.getHp() << endl;

    // 治療
    cout << "\n=== 治療 ===" << endl;
    healer.heal(warrior, 60);
    healer.heal(mage, 100);
    healer.heal(healer, 30);    // 自我治療

    // 最終狀態
    cout << "\n=== 最終狀態 ===" << endl;
    warrior.printStatus();
    mage.printStatus();
    healer.printStatus();

    return 0;
}
