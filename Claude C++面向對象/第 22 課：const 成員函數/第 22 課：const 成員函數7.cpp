/*
# 第 22 課：const 成員函數

---

## 22.1 回顧：const 在 C 語言中的角色

你在 C 語言中已經用過 `const`：

```
C 語言中的 const：
  const int MAX = 100;           // 常量變數
  void print(const char* str);   // 不修改指標指向的內容
  const int* p;                  // 指向常量的指標
  int* const p;                  // 常量指標
```

在 C++ 的類別中，`const` 有了一個全新的角色——**修飾成員函數**，告訴編譯器「這個函數不會修改對象的狀態」。

---

## 22.2 什麼是 const 成員函數？

```
普通成員函數：
  void setHp(int hp) { hp_ = hp; }
  → 可以修改成員變數

const 成員函數：
  int getHp() const { return hp_; }
                ↑↑↑↑↑
  → 承諾「不修改任何成員變數」
  → 編譯器會強制執行這個承諾
```

`const` 放在函數參數列表的右括號**之後**、函數體的**之前**：

```cpp
返回類型 函數名(參數) const {
    // 函數體：不能修改任何成員變數
}
```

---

## 22.3 基礎範例：const 的強制力

```cpp
#include <iostream>
#include <string>
using namespace std;

class Potion {
private:
    string name_;
    int healAmount_;
    int quantity_;

public:
    Potion(const string& name, int heal, int qty)
        : name_(name), healAmount_(heal), quantity_(qty)
    {
    }

    // ====== const 成員函數：承諾不修改對象 ======

    const string& getName() const { return name_; }
    int getHealAmount() const { return healAmount_; }
    int getQuantity() const { return quantity_; }

    void printInfo() const {
        cout << "  " << name_ << " (回復:" << healAmount_
             << " 數量:" << quantity_ << ")" << endl;

        // 以下會編譯錯誤！const 函數不能修改成員
        // quantity_ = 0;       // 錯誤！
        // name_ = "被篡改";    // 錯誤！
    }

    // ====== 非 const 成員函數：可以修改對象 ======

    bool use() {
        if (quantity_ <= 0) {
            cout << "  " << name_ << " 已用完！" << endl;
            return false;
        }
        quantity_--;
        cout << "  使用 " << name_ << "，回復 " << healAmount_
             << " HP (剩餘:" << quantity_ << ")" << endl;
        return true;
    }

    void restock(int amount) {
        if (amount > 0) {
            quantity_ += amount;
            cout << "  補貨 " << name_ << " +" << amount
                 << " (總計:" << quantity_ << ")" << endl;
        }
    }
};

int main() {
    cout << "=== const 成員函數基礎 ===" << endl;

    Potion potion("治療藥水", 50, 3);

    // const 函數：只讀操作
    cout << "\n--- const 函數（只讀）---" << endl;
    potion.printInfo();
    cout << "  名稱：" << potion.getName() << endl;
    cout << "  數量：" << potion.getQuantity() << endl;

    // 非 const 函數：修改操作
    cout << "\n--- 非 const 函數（修改）---" << endl;
    potion.use();
    potion.use();
    potion.printInfo();

    return 0;
}
```

### 預期輸出

```
=== const 成員函數基礎 ===

--- const 函數（只讀）---
  治療藥水 (回復:50 數量:3)
  名稱：治療藥水
  數量：3

--- 非 const 函數（修改）---
  使用 治療藥水，回復 50 HP (剩餘:2)
  使用 治療藥水，回復 50 HP (剩餘:1)
  治療藥水 (回復:50 數量:1)
```

---

## 22.4 const 對象的限制

這是 `const` 成員函數最重要的應用場景——**const 對象只能調用 const 成員函數**：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Shield {
private:
    string name_;
    int defense_;
    int durability_;

public:
    Shield(const string& name, int def, int dur)
        : name_(name), defense_(def), durability_(dur)
    {
    }

    // const 成員函數
    const string& getName() const { return name_; }
    int getDefense() const { return defense_; }
    int getDurability() const { return durability_; }

    void printInfo() const {
        cout << "  " << name_ << " [防禦:" << defense_
             << " 耐久:" << durability_ << "]" << endl;
    }

    // 非 const 成員函數
    void takeDamage(int dmg) {
        durability_ -= dmg;
        if (durability_ < 0) durability_ = 0;
        cout << "  " << name_ << " 耐久 -" << dmg
             << " (剩餘:" << durability_ << ")" << endl;
    }

    void repair() {
        durability_ = 100;
        cout << "  " << name_ << " 修復完成" << endl;
    }
};

// 接收 const 引用的函數——模擬「只看不碰」
void inspectShield(const Shield& s) {
    cout << "\n--- 檢查盾牌（const 引用）---" << endl;

    // ✅ 可以調用 const 成員函數
    s.printInfo();
    cout << "  防禦力：" << s.getDefense() << endl;
    cout << "  耐久度：" << s.getDurability() << endl;

    // ❌ 不能調用非 const 成員函數
    // s.takeDamage(10);   // 編譯錯誤！
    // s.repair();          // 編譯錯誤！
}

int main() {
    cout << "=== const 對象的限制 ===" << endl;

    // 非 const 對象：所有函數都能調用
    cout << "\n--- 非 const 對象 ---" << endl;
    Shield shield("鐵盾", 40, 100);
    shield.printInfo();        // ✅ const 函數
    shield.takeDamage(20);     // ✅ 非 const 函數
    shield.repair();           // ✅ 非 const 函數

    // const 對象：只能調用 const 函數
    cout << "\n--- const 對象 ---" << endl;
    const Shield legendaryShield("傳說之盾", 100, 999);
    legendaryShield.printInfo();       // ✅ const 函數
    legendaryShield.getDefense();      // ✅ const 函數
    // legendaryShield.takeDamage(10); // ❌ 編譯錯誤！
    // legendaryShield.repair();       // ❌ 編譯錯誤！

    // const 引用參數
    inspectShield(shield);

    return 0;
}
```

### 預期輸出

```
=== const 對象的限制 ===

--- 非 const 對象 ---
  鐵盾 [防禦:40 耐久:100]
  鐵盾 耐久 -20 (剩餘:80)
  鐵盾 修復完成

--- const 對象 ---
  傳說之盾 [防禦:100 耐久:999]

--- 檢查盾牌（const 引用）---
  鐵盾 [防禦:40 耐久:100]
  防禦力：40
  耐久度：100
```

這裡的規則非常嚴格：

```
非 const 對象 → 可以調用 const 和非 const 函數
const 對象    → 只能調用 const 函數

這就像：
  普通人 → 可以「看」也可以「碰」展品
  參觀者 → 只能「看」，不能「碰」（const 身份）
```

---

## 22.5 const 正確性（const correctness）

`const` 正確性是 C++ 中一個非常重要的概念——**所有不修改對象的函數都應該標記為 const**。

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 反面教材：忘記加 const =====
class BadDesign {
private:
    string name_;
    int value_;

public:
    BadDesign(const string& n, int v) : name_(n), value_(v) {}

    // 忘記加 const！這些函數明明不修改對象
    string getName() { return name_; }       // 缺少 const
    int getValue() { return value_; }        // 缺少 const
    void print() { cout << name_ << ":" << value_ << endl; } // 缺少 const
};

// ===== 正確設計：const 正確 =====
class GoodDesign {
private:
    string name_;
    int value_;

public:
    GoodDesign(const string& n, int v) : name_(n), value_(v) {}

    // 所有不修改對象的函數都加 const
    const string& getName() const { return name_; }
    int getValue() const { return value_; }
    void print() const { cout << name_ << ":" << value_ << endl; }

    // 只有修改對象的函數才不加 const
    void setValue(int v) { value_ = v; }
};

// 需要 const 引用的函數
void processBad(const BadDesign& b) {
    // b.getName();   // ❌ 編譯錯誤！getName 不是 const
    // b.print();     // ❌ 編譯錯誤！print 不是 const
    cout << "  BadDesign：什麼都不能做！" << endl;
}

void processGood(const GoodDesign& g) {
    g.print();         // ✅ 完美
    cout << "  name = " << g.getName() << endl;  // ✅
    cout << "  value = " << g.getValue() << endl; // ✅
}

int main() {
    cout << "=== const 正確性 ===" << endl;

    BadDesign bad("壞設計", 42);
    GoodDesign good("好設計", 42);

    cout << "\n--- 用 const 引用傳遞 ---" << endl;
    processBad(bad);      // 幾乎什麼都做不了
    processGood(good);    // 正常工作

    return 0;
}
```

### 預期輸出

```
=== const 正確性 ===

--- 用 const 引用傳遞 ---
  BadDesign：什麼都不能做！
  好設計:42
  name = 好設計
  value = 42
```

**忘記加 `const` 的後果**：當別人用 `const` 引用接收你的對象時，連 getter 都不能調用。這是一個非常常見的 C++ 新手錯誤。

---

## 22.6 const 成員函數的底層原理

每個成員函數都有一個隱含的 `this` 指標。`const` 改變的就是 `this` 的類型：

```
普通成員函數：
  void setHp(int hp)
  → 隱含參數：Player* const this
  → this 指標本身是 const（不能指向別的對象）
  → 但 this 指向的內容可以修改

const 成員函數：
  int getHp() const
  → 隱含參數：const Player* const this
  → this 指標本身是 const
  → this 指向的內容也是 const（不能修改！）
```

圖解：

```
普通成員函數的 this：
  this ──→ [ name_ | hp_ | maxHp_ ]
            可讀寫   可讀寫  可讀寫

const 成員函數的 this：
  this ──→ [ name_ | hp_ | maxHp_ ]
            只讀     只讀    只讀
```

```cpp
#include <iostream>
using namespace std;

class Demo {
private:
    int value_;

public:
    Demo(int v) : value_(v) {}

    // 普通成員函數
    void modify() {
        // this 的類型是 Demo* const
        this->value_ = 999;        // ✅ 可以修改
        cout << "  modify(): value_ = " << value_ << endl;
    }

    // const 成員函數
    void inspect() const {
        // this 的類型是 const Demo* const
        // this->value_ = 999;     // ❌ 編譯錯誤！
        cout << "  inspect(): value_ = " << value_ << endl;
    }
};

int main() {
    cout << "=== this 指標與 const ===" << endl;

    Demo d(42);
    d.inspect();    // const 函數
    d.modify();     // 非 const 函數
    d.inspect();    // 再次查看

    return 0;
}
```

### 預期輸出

```
=== this 指標與 const ===
  inspect(): value_ = 42
  modify(): value_ = 999
  inspect(): value_ = 999
```

---

## 22.7 const 重載（const overloading）

同一個函數可以同時有 `const` 和非 `const` 兩個版本——編譯器會根據對象是否為 `const` 來選擇調用哪個：

```cpp
#include <iostream>
#include <string>
using namespace std;

class TextBuffer {
private:
    string content_;

public:
    TextBuffer(const string& text) : content_(text) {}

    // const 版本：返回 const 引用（只讀）
    const string& getText() const {
        cout << "  [調用 const 版本]" << endl;
        return content_;
    }

    // 非 const 版本：返回非 const 引用（可讀寫）
    string& getText() {
        cout << "  [調用非 const 版本]" << endl;
        return content_;
    }

    void print() const {
        cout << "  內容：「" << content_ << "」" << endl;
    }
};

int main() {
    cout << "=== const 重載 ===" << endl;

    // 非 const 對象
    cout << "\n--- 非 const 對象 ---" << endl;
    TextBuffer buf("Hello");
    buf.getText();                   // 調用非 const 版本
    buf.getText() = "Modified!";     // 可以通過引用修改
    buf.print();

    // const 對象
    cout << "\n--- const 對象 ---" << endl;
    const TextBuffer constBuf("ReadOnly");
    constBuf.getText();              // 調用 const 版本
    // constBuf.getText() = "Hack!"; // ❌ 編譯錯誤！返回的是 const 引用
    constBuf.print();

    // const 引用
    cout << "\n--- const 引用 ---" << endl;
    const TextBuffer& ref = buf;
    ref.getText();                   // 調用 const 版本
    ref.print();

    return 0;
}
```

### 預期輸出

```
=== const 重載 ===

--- 非 const 對象 ---
  [調用非 const 版本]
  [調用非 const 版本]
  內容：「Modified!」

--- const 對象 ---
  [調用 const 版本]
  內容：「ReadOnly」

--- const 引用 ---
  [調用 const 版本]
  內容：「Modified!」
```

選擇規則：

```
對象/引用是 const → 調用 const 版本
對象/引用非 const → 調用非 const 版本（如果有的話）
                  → 如果沒有非 const 版本，也會調用 const 版本
```

**const 重載常見用途**：像 `std::vector` 的 `operator[]` 就有兩個版本——`const` 版本返回 `const` 引用，非 `const` 版本返回普通引用。

---

## 22.8 const 成員函數與函數調用鏈

一個 `const` 成員函數內部只能調用其他 `const` 成員函數：

```cpp
#include <iostream>
#include <string>
using namespace std;

class GameCharacter {
private:
    string name_;
    int hp_;
    int maxHp_;
    int level_;

public:
    GameCharacter(const string& name, int maxHp, int level)
        : name_(name), hp_(maxHp), maxHp_(maxHp), level_(level)
    {
    }

    // ====== const 函數群 ======
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getMaxHp() const { return maxHp_; }
    int getLevel() const { return level_; }

    double getHpPercent() const {
        // const 函數可以調用其他 const 函數 ✅
        return (static_cast<double>(getHp()) / getMaxHp()) * 100.0;
    }

    string getStatusText() const {
        // 可以調用 getName(), getHpPercent() — 都是 const ✅
        string status = getName() + " Lv." + to_string(getLevel());
        double pct = getHpPercent();

        if (pct > 50.0)
            status += " [健康]";
        else if (pct > 20.0)
            status += " [受傷]";
        else if (pct > 0.0)
            status += " [瀕死]";
        else
            status += " [死亡]";

        return status;
    }

    void printFullStatus() const {
        // const 函數可以調用其他 const 函數
        cout << "  " << getStatusText() << endl;
        cout << "  HP: " << getHp() << "/" << getMaxHp()
             << " (" << getHpPercent() << "%)" << endl;

        // 不能調用非 const 函數：
        // takeDamage(10);  // ❌ 編譯錯誤！
    }

    // ====== 非 const 函數 ======
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
        // 非 const 函數可以調用 const 函數 ✅
        cout << "  " << getName() << " 受傷！" << getStatusText() << endl;
    }
};

int main() {
    cout << "=== const 函數調用鏈 ===" << endl;

    GameCharacter hero("戰士", 200, 5);

    cout << "\n--- 初始狀態 ---" << endl;
    hero.printFullStatus();

    cout << "\n--- 受傷後 ---" << endl;
    hero.takeDamage(120);
    hero.printFullStatus();

    hero.takeDamage(60);
    hero.printFullStatus();

    return 0;
}
```

### 預期輸出

```
=== const 函數調用鏈 ===

--- 初始狀態 ---
  戰士 Lv.5 [健康]
  HP: 200/200 (100%)

--- 受傷後 ---
  戰士 受傷！戰士 Lv.5 [受傷]
  戰士 Lv.5 [受傷]
  HP: 80/200 (40%)
  戰士 受傷！戰士 Lv.5 [瀕死]
  戰士 Lv.5 [瀕死]
  HP: 20/200 (10%)
```

調用規則圖解：

```
const 函數 ──→ const 函數     ✅ 可以
const 函數 ──→ 非 const 函數  ❌ 不行
非 const 函數 → const 函數    ✅ 可以
非 const 函數 → 非 const 函數 ✅ 可以
```

---

## 22.9 實際應用：const 引用參數的威力

`const` 成員函數最大的實際價值在於——**讓你的類別可以安全地通過 const 引用傳遞**：

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    string element_;

public:
    Monster(const string& name, int hp, int atk, const string& elem)
        : name_(name), hp_(hp), attack_(atk), element_(elem)
    {
    }

    // 全部是 const — 讓 Monster 可以安全地到處傳遞
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getAttack() const { return attack_; }
    const string& getElement() const { return element_; }
    bool isAlive() const { return hp_ > 0; }

    void printInfo() const {
        cout << "  " << name_ << " [" << element_ << "] HP:"
             << hp_ << " ATK:" << attack_ << endl;
    }

    // 非 const：會修改狀態
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// ===== 接收 const 引用的各種函數 =====

// 1. 分析函數：只需要讀取
void analyzeMonster(const Monster& m) {
    cout << "  分析 " << m.getName() << "：" << endl;
    cout << "    屬性：" << m.getElement() << endl;
    cout << "    威脅等級：";
    if (m.getAttack() >= 50) cout << "高";
    else if (m.getAttack() >= 30) cout << "中";
    else cout << "低";
    cout << endl;
}

// 2. 比較函數：比較兩個怪物
void compareMonsters(const Monster& a, const Monster& b) {
    cout << "  比較 " << a.getName() << " vs " << b.getName() << "：";
    if (a.getAttack() > b.getAttack())
        cout << a.getName() << " 更強！" << endl;
    else if (a.getAttack() < b.getAttack())
        cout << b.getName() << " 更強！" << endl;
    else
        cout << "不分上下！" << endl;
}

// 3. 列表顯示：展示一整組怪物
void showMonsterList(const vector<Monster>& monsters) {
    cout << "  怪物圖鑑（共 " << monsters.size() << " 隻）：" << endl;
    for (const auto& m : monsters) {
        cout << "    ";
        m.printInfo();
    }
}

// 4. 搜尋函數：找出特定屬性的怪物
const Monster* findByElement(const vector<Monster>& monsters,
                              const string& element)
{
    for (const auto& m : monsters) {
        if (m.getElement() == element) {
            return &m;
        }
    }
    return nullptr;
}

int main() {
    cout << "============================================" << endl;
    cout << "   第 22 課：const 引用參數的實際應用" << endl;
    cout << "============================================" << endl;

    vector<Monster> monsters = {
        Monster("火龍", 500, 60, "火"),
        Monster("冰狼", 300, 40, "冰"),
        Monster("雷鷹", 200, 55, "雷"),
        Monster("土蟲", 800, 20, "土")
    };

    // 展示列表——通過 const 引用安全傳遞
    cout << "\n=== 怪物列表 ===" << endl;
    showMonsterList(monsters);

    // 分析怪物
    cout << "\n=== 分析怪物 ===" << endl;
    analyzeMonster(monsters[0]);
    analyzeMonster(monsters[3]);

    // 比較怪物
    cout << "\n=== 比較怪物 ===" << endl;
    compareMonsters(monsters[0], monsters[2]);
    compareMonsters(monsters[1], monsters[3]);

    // 搜尋怪物
    cout << "\n=== 搜尋怪物 ===" << endl;
    const Monster* found = findByElement(monsters, "雷");
    if (found) {
        cout << "  找到：";
        found->printInfo();
    }

    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson22 lesson22.cpp
./lesson22
```

### 預期輸出

```
============================================
   第 22 課：const 引用參數的實際應用
============================================

=== 怪物列表 ===
  怪物圖鑑（共 4 隻）：
      火龍 [火] HP:500 ATK:60
      冰狼 [冰] HP:300 ATK:40
      雷鷹 [雷] HP:200 ATK:55
      土蟲 [土] HP:800 ATK:20

=== 分析怪物 ===
  分析 火龍：
    屬性：火
    威脅等級：高
  分析 土蟲：
    屬性：土
    威脅等級：低

=== 比較怪物 ===
  比較 火龍 vs 雷鷹：火龍 更強！
  比較 冰狼 vs 土蟲：冰狼 更強！

=== 搜尋怪物 ===
  找到：  雷鷹 [雷] HP:200 ATK:55
```

如果 `Monster` 的 getter 和 `printInfo` 沒有 `const`，上面這些函數**全部都會編譯失敗**。

---

## 22.10 常見錯誤與陷阱

### 陷阱 1：在 const 函數中意外修改

```cpp
class Trap1 {
private:
    int count_;
    int cache_[10];
public:
    // 看起來無害，但其實修改了 cache_
    int getCount() const {
        // cache_[0] = count_;  // ❌ 編譯錯誤！不能修改
        return count_;
    }
};
```

### 陷阱 2：通過指標間接修改

```cpp
class Trap2 {
private:
    int* data_;      // 指向堆上的數據
public:
    Trap2() : data_(new int(42)) {}
    ~Trap2() { delete data_; }

    // 危險！const 只保護指標本身，不保護指向的內容
    void sneakyModify() const {
        // data_ = nullptr;  // ❌ 不能修改指標本身
        *data_ = 999;        // ⚠ 居然可以！const 不保護指向的數據
    }

    int getValue() const { return *data_; }
};
```

```
const 保護的範圍：
  ┌─────────────────────────────────┐
  │ 對象本身                         │
  │ ┌──────────┐                    │
  │ │ data_ ───┼──→ [堆上的 int]   │ ← const 不保護這裡！
  │ │ (指標)   │    (指向的內容)     │
  │ └──────────┘                    │
  │  ↑ const 保護這裡               │
  └─────────────────────────────────┘
```

這是 C++ `const` 的一個重要限制——它是**淺層的**（shallow const），只保護對象的直接成員，不保護指標指向的間接數據。

### 陷阱 3：忘記給 getter 加 const

```cpp
class Trap3 {
private:
    int value_;
public:
    Trap3(int v) : value_(v) {}

    // 忘記加 const！
    int getValue() { return value_; }
};

void process(const Trap3& t) {
    // t.getValue();  // ❌ 編譯錯誤！
    // 因為 getValue 不是 const 函數
}
```

**養成習慣**：寫完一個成員函數，如果它不修改任何成員，立刻加上 `const`。

---

## 22.11 const 正確性檢查清單

```
寫完一個成員函數後，問自己：

  1. 這個函數會修改任何成員變數嗎？
     └→ 否 → 加 const
     └→ 是 → 不加 const

  2. 這個函數調用的其他成員函數都是 const 嗎？
     └→ 如果你的函數是 const，它只能調用 const 函數

  3. 返回值類型正確嗎？
     └→ const 函數返回成員的引用 → 必須是 const 引用
     └→ 返回值（拷貝）→ 不需要 const

  4. 有沒有通過指標間接修改數據？
     └→ const 不保護指標指向的內容，需要自律
```

---

## 22.12 本課重點回顧

| 概念 | 說明 |
|------|------|
| const 成員函數 | 承諾不修改任何成員變數，編譯器強制執行 |
| 語法位置 | `const` 放在參數列表 `)` 之後、`{` 之前 |
| const 對象的限制 | 只能調用 const 成員函數 |
| const 正確性 | 所有不修改對象的函數都應標記 const |
| this 指標 | const 函數中 this 是 `const T* const` |
| const 重載 | 同一函數的 const 和非 const 版本可共存 |
| 調用鏈 | const 函數只能調用 const 函數 |
| 實際價值 | 讓類別可以安全地通過 const 引用傳遞 |
| 淺層 const | 只保護直接成員，不保護指標指向的內容 |
| 養成習慣 | 不修改成員的函數，立刻加 const |

---

## 22.13 下一課預告

下一課是 **第 23 課：mutable 關鍵字**。我們將學習一個特殊的例外——當你的 `const` 函數確實需要修改某個成員變數時（例如快取、計數器），`mutable` 允許你在不破壞 const 語義的前提下修改特定成員。

準備好進入 **第 23 課：mutable 關鍵字** 了嗎？
*/



#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Monster {
private:
    string name_;
    int hp_;
    int attack_;
    string element_;

public:
    Monster(const string& name, int hp, int atk, const string& elem)
        : name_(name), hp_(hp), attack_(atk), element_(elem)
    {
    }

    // 全部是 const — 讓 Monster 可以安全地到處傳遞
    // const 成員函數：承諾不修改對象狀態
    // 在 const 成員函數中，this 的類型是 const Monster* const，表示 this 是一個指向 Monster 對象的常量指針，並且指向的對象也是常量（不可修改）
    // 這裡的 getName(), getHp(), getAttack(), getElement(), isAlive() 函數都是 const 成員函數，承諾不修改對象的狀態，因此 this 的類型是 const Monster* const
    // 這裡的 printInfo() 函數也是 const 成員函數，承諾不修改對象的狀態，因此 this 的類型是 const Monster* const
    const string& getName() const { return name_; }
    int getHp() const { return hp_; }
    int getAttack() const { return attack_; }
    const string& getElement() const { return element_; }
    bool isAlive() const { return hp_ > 0; }

    void printInfo() const {
        cout << "  " << name_ << " [" << element_ << "] HP:"
             << hp_ << " ATK:" << attack_ << endl;
    }

    // 非 const：會修改狀態
    // 在非 const 成員函數中，this 的類型是 Monster* const，表示 this 是一個指向 Monster 對象的常量指針（指針本身不可修改，但指向的對象可以修改）
    // 這裡的 takeDamage() 函數是非 const 成員函數，會修改對象的狀態（hp_），因此 this 的類型是 Monster* const
    void takeDamage(int dmg) {
        hp_ = max(0, hp_ - dmg);
    }
};

// ===== 接收 const 引用的各種函數 =====

// 1. 分析函數：只需要讀取
// 在 analyzeMonster() 函數中，參數 m 的類型是 const Monster&，表示 m 是一個指向 Monster 對象的常量引用，承諾不修改 m 所指向的對象
// 這裡的 analyzeMonster() 函數接收 const Monster&，表示它只會讀取 Monster 的資料，不會修改它，因此可以安全地接受 const 引用
void analyzeMonster(const Monster& m) {
    cout << "  分析 " << m.getName() << "：" << endl;
    cout << "    屬性：" << m.getElement() << endl;
    cout << "    威脅等級：";
    if (m.getAttack() >= 50) cout << "高";
    else if (m.getAttack() >= 30) cout << "中";
    else cout << "低";
    cout << endl;
}

// 2. 比較函數：比較兩個怪物
// 在 compareMonsters() 函數中，參數 a 和 b 的類型都是 const Monster&，表示它們都是指向 Monster 對象的常量引用，承諾不修改它們所指向的對象
// 這裡的 compareMonsters() 函數接收兩個 const Monster&，表示它只會讀取這兩個 Monster 的資料，不會修改它們，因此可以安全地接受 const 引用
void compareMonsters(const Monster& a, const Monster& b) {
    cout << "  比較 " << a.getName() << " vs " << b.getName() << "：";
    if (a.getAttack() > b.getAttack())
        cout << a.getName() << " 更強！" << endl;
    else if (a.getAttack() < b.getAttack())
        cout << b.getName() << " 更強！" << endl;
    else
        cout << "不分上下！" << endl;
}

// 3. 列表顯示：展示一整組怪物
// 在 showMonsterList() 函數中，參數 monsters 的類型是 const vector<Monster>&，表示 monsters 是一個指向 vector<Monster> 對象的常量引用，承諾不修改它所指向的對象
// 這裡的 showMonsterList() 函數接收 const vector<Monster>&，表示它只會讀取 monsters 的資料，不會修改它，因此可以安全地接受 const 引用
void showMonsterList(const vector<Monster>& monsters) {
    cout << "  怪物圖鑑（共 " << monsters.size() << " 隻）：" << endl;
    for (const auto& m : monsters) {
        cout << "    ";
        m.printInfo();
    }
}

// 4. 搜尋函數：找出特定屬性的怪物
// 在 findByElement() 函數中，參數 monsters 的類型是 const vector<Monster>&，表示 monsters 是一個指向 vector<Monster> 對象的常量引用，承諾不修改它所指向的對象
// 這裡的 findByElement() 函數接收 const vector<Monster>&，表示它只會讀取 monsters 的資料，不會修改它，因此可以安全地接受 const 引用
const Monster* findByElement(const vector<Monster>& monsters,
                              const string& element)
{
    for (const auto& m : monsters) {
        if (m.getElement() == element) {
            return &m;
        }
    }
    return nullptr;
}

int main() {
    cout << "============================================" << endl;
    cout << "   第 22 課：const 引用參數的實際應用" << endl;
    cout << "============================================" << endl;

    vector<Monster> monsters = {
        Monster("火龍", 500, 60, "火"),
        Monster("冰狼", 300, 40, "冰"),
        Monster("雷鷹", 200, 55, "雷"),
        Monster("土蟲", 800, 20, "土")
    };

    // 展示列表——通過 const 引用安全傳遞
    cout << "\n=== 怪物列表 ===" << endl;
    showMonsterList(monsters);

    // 分析怪物
    cout << "\n=== 分析怪物 ===" << endl;
    analyzeMonster(monsters[0]);
    analyzeMonster(monsters[3]);

    // 比較怪物
    cout << "\n=== 比較怪物 ===" << endl;
    compareMonsters(monsters[0], monsters[2]);
    compareMonsters(monsters[1], monsters[3]);

    // 搜尋怪物
    cout << "\n=== 搜尋怪物 ===" << endl;
    const Monster* found = findByElement(monsters, "雷");
    if (found) {
        cout << "  找到：";
        found->printInfo();
    }

    return 0;
}
