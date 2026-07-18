/*
# 第 12 課：struct 與 class 的差異

## 一、C++ 中 struct 和 class 幾乎相同

這可能會讓你意外：**在 C++ 中，struct 和 class 只有一個語法層面的差異**——預設存取修飾符不同。除此之外，struct 能做的所有事情 class 都能做，反之亦然。

```cpp
#include <iostream>
#include <string>
using namespace std;

// 用 struct 寫的「完整 OOP」
struct DogStruct {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（struct）汪汪！" << endl;
    }
};

// 用 class 寫的完全等價版本
class DogClass {
private:
    string name;
    int age = 0;

public:
    void setName(const string& n) { name = n; }
    void setAge(int a) { age = a; }

    void bark() {
        cout << name << "（class）汪汪！" << endl;
    }
};

int main() {
    DogStruct ds;
    ds.setName("旺財");
    ds.bark();

    DogClass dc;
    dc.setName("小黑");
    dc.bark();

    return 0;
}
```

**預期輸出：**
```
旺財（struct）汪汪！
小黑（class）汪汪！
```

功能上**完全一樣**。struct 在 C++ 中也可以有 private 成員、成員函數、繼承、虛函數——一切 class 能做的。

---

## 二、唯一的語法差異

### 差異一：預設存取修飾符

```cpp
#include <iostream>
using namespace std;

struct StructExample {
    int x = 10;    // 預設 public
    void show() { cout << "struct x = " << x << endl; }
};

class ClassExample {
    int x = 10;    // 預設 private
    void show() { cout << "class x = " << x << endl; }
};

int main() {
    StructExample s;
    cout << s.x << endl;    // ✅ public，可以存取
    s.show();               // ✅ public

    ClassExample c;
    // cout << c.x << endl; // ❌ 編譯錯誤！private
    // c.show();            // ❌ 編譯錯誤！private

    return 0;
}
```

### 差異二：預設繼承方式

這個差異要到第八階段（繼承）才會用到，這裡先做個預告：

```cpp
struct Base {
    int value = 42;
};

struct DerivedStruct : Base { };     // 預設 public 繼承
class DerivedClass : Base { };       // 預設 private 繼承
```

**完整對比表**：

| 特性 | struct | class |
|------|--------|-------|
| 預設成員存取 | `public` | `private` |
| 預設繼承方式 | `public` | `private` |
| 能否有 private 成員 | ✅ 可以 | ✅ 可以 |
| 能否有成員函數 | ✅ 可以 | ✅ 可以 |
| 能否繼承 | ✅ 可以 | ✅ 可以 |
| 能否有虛函數 | ✅ 可以 | ✅ 可以 |
| 能否用模板 | ✅ 可以 | ✅ 可以 |

**除了預設存取和預設繼承方式，沒有任何差異。**

---

## 三、C 的 struct vs C++ 的 struct

這個差異反而更大。你在 C 語言課程中用的 struct，和 C++ 的 struct 是不一樣的：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== C 語言的 struct（在 C++ 中也能編譯）=====
// 只能有資料，不能有函數
struct C_Style {
    char name[50];
    int age;
    float score;
};
// C 中必須寫 struct C_Style s; （C++ 中可省略 struct 關鍵字）

// ===== C++ 的 struct =====
// 可以有函數、建構函數、存取控制、繼承……
struct CPP_Style {
    string name;
    int age = 0;
    float score = 0.0f;

    void show() {
        cout << name << ", " << age << " 歲, " << score << " 分" << endl;
    }

    bool isPassing() {
        return score >= 60.0f;
    }
};

int main() {
    // C 風格
    C_Style cs;
    // strcpy(cs.name, "小明");  // 需要 cstring
    cs.age = 20;
    cs.score = 85.5f;

    // C++ 風格
    CPP_Style cpps;
    cpps.name = "小明";
    cpps.age = 20;
    cpps.score = 85.5f;
    cpps.show();
    cout << "及格: " << (cpps.isPassing() ? "是" : "否") << endl;

    return 0;
}
```

**預期輸出：**
```
小明, 20 歲, 85.5 分
及格: 是
```

| 特性 | C 的 struct | C++ 的 struct |
|------|------------|--------------|
| 成員函數 | ❌ 不支援 | ✅ 支援 |
| 存取控制 | ❌ 全部公開 | ✅ public/private/protected |
| 建構/解構函數 | ❌ 不支援 | ✅ 支援 |
| 繼承 | ❌ 不支援 | ✅ 支援 |
| 宣告變數時 | 必須寫 `struct Dog d;` | 可以直接寫 `Dog d;` |
| 類內初始化 | ❌ 不支援 | ✅ 支援（C++11） |

---

## 四、什麼時候用 struct，什麼時候用 class？

這是 C++ 社群中最常見的慣例（不是強制規定，而是約定俗成）：

### 用 struct：簡單資料的集合

當一個型別主要是**一組資料的集合**，沒有複雜的行為邏輯，所有成員都是 public 時，用 `struct`：

```cpp
#include <iostream>
using namespace std;

// ✅ 適合用 struct —— 純資料集合
struct Point {
    double x = 0;
    double y = 0;
};

struct Color {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
};

struct Config {
    int width = 800;
    int height = 600;
    bool fullscreen = false;
    int fps = 60;
};

struct DateInfo {
    int year = 2025;
    int month = 1;
    int day = 1;
};

int main() {
    Point p;
    p.x = 3.5;
    p.y = 7.2;
    cout << "Point(" << p.x << ", " << p.y << ")" << endl;

    Color red;
    red.r = 255;
    cout << "Color(" << red.r << ", " << red.g << ", " << red.b << ")" << endl;

    Config cfg;
    cout << cfg.width << "x" << cfg.height
         << (cfg.fullscreen ? " 全螢幕" : " 視窗") << endl;

    return 0;
}
```

**預期輸出：**
```
Point(3.5, 7.2)
Color(255, 0, 0)
800x600 視窗
```

**這些型別的共同特點**：
- 所有成員都是 public
- 沒有資料驗證需求（任何值都合理）
- 只是把幾個相關的值綁在一起
- 很少或沒有成員函數

---

### 用 class：有行為、需要保護的對象

當一個型別有**複雜行為**、需要**資料保護**、有**不變條件（invariant）**時，用 `class`：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ✅ 適合用 class —— 有行為、有保護
class BankAccount {
public:
    void init(const string& name, double initial) {
        ownerName = name;
        if (initial >= 0) balance = initial;
    }

    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        return true;
    }

    double getBalance() { return balance; }
    string getOwner() { return ownerName; }

private:
    string ownerName;
    double balance = 0.0;   // 不變條件：balance >= 0
};

int main() {
    BankAccount acc;
    acc.init("陳信安", 1000);
    acc.deposit(500);
    acc.withdraw(200);
    cout << acc.getOwner() << ": $" << acc.getBalance() << endl;
    return 0;
}
```

**預期輸出：**
```
陳信安: $1300
```

**這個型別需要 class 因為**：
- `balance` 有不變條件：不能為負數
- 修改 `balance` 必須經過驗證
- 外界不應該直接接觸內部資料

---

### struct 也能加少量函數

struct 加上一些便利函數是完全正常的，不需要因為加了函數就改成 class：

```cpp
#include <iostream>
#include <cmath>
using namespace std;

struct Point {
    double x = 0;
    double y = 0;

    // 便利函數 —— 不改變「純資料集合」的本質
    double distanceTo(const Point& other) {
        double dx = x - other.x;
        double dy = y - other.y;
        return sqrt(dx * dx + dy * dy);
    }

    void print() {
        cout << "(" << x << ", " << y << ")" << endl;
    }
};

struct Rectangle {
    double width = 0;
    double height = 0;

    double area() { return width * height; }
    double perimeter() { return 2 * (width + height); }
};

int main() {
    Point a, b;
    a.x = 0; a.y = 0;
    b.x = 3; b.y = 4;

    a.print();
    b.print();
    cout << "距離: " << a.distanceTo(b) << endl;

    Rectangle r;
    r.width = 5;
    r.height = 3;
    cout << "面積: " << r.area() << ", 周長: " << r.perimeter() << endl;

    return 0;
}
```

**預期輸出：**
```
(0, 0)
(3, 4)
距離: 5
面積: 15, 周長: 16
```

**關鍵判斷標準**：成員是不是全部 public？有沒有需要保護的不變條件？如果都是 public 且沒有不變條件，就算有函數也可以用 struct。

---

## 五、決策流程圖

```
設計一個新型別時：

所有成員都應該公開嗎？
├── 是 → 有需要保護的不變條件嗎？
│         ├── 否 → 用 struct ✅
│         │         （純資料集合、配置、座標、顏色等）
│         │
│         └── 是 → 用 class ✅
│                   （即使目前都是 public，未來可能需要保護）
│
└── 否 → 用 class ✅
          （有 private 成員、複雜行為、資料驗證）
```

---

## 六、真實專案中的 struct vs class

讓我們看一個同時使用兩者的完整範例：

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// ===== struct：純資料 =====
struct Vector2D {
    double x = 0;
    double y = 0;

    double length() {
        return sqrt(x * x + y * y);
    }

    Vector2D add(const Vector2D& other) {
        Vector2D result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
    }
};

struct CharacterStats {
    int maxHp = 100;
    int maxMp = 50;
    int attack = 10;
    int defense = 5;
    double speed = 1.0;
};

// ===== class：有行為和保護 =====
class GameCharacter {
public:
    void init(const string& charName, const CharacterStats& charStats) {
        name = charName;
        stats = charStats;
        currentHp = stats.maxHp;
        currentMp = stats.maxMp;
    }

    void moveTo(const Vector2D& target) {
        Vector2D direction;
        direction.x = target.x - position.x;
        direction.y = target.y - position.y;

        double dist = direction.length();
        if (dist > 0) {
            // 根據速度移動（簡化版）
            position.x += (direction.x / dist) * stats.speed;
            position.y += (direction.y / dist) * stats.speed;
        }
        cout << name << " 移動到 (" << position.x << ", " << position.y << ")" << endl;
    }

    bool takeDamage(int damage) {
        int actualDamage = damage - stats.defense;
        if (actualDamage < 1) actualDamage = 1;  // 最少受 1 點傷害

        currentHp -= actualDamage;
        if (currentHp < 0) currentHp = 0;

        cout << name << " 受到 " << actualDamage << " 點傷害"
             << "（HP: " << currentHp << "/" << stats.maxHp << "）" << endl;

        return currentHp > 0;  // 返回是否存活
    }

    bool useSkill(int mpCost) {
        if (currentMp < mpCost) {
            cout << name << ": MP 不足！" << endl;
            return false;
        }
        currentMp -= mpCost;
        cout << name << " 使用技能"
             << "（MP: " << currentMp << "/" << stats.maxMp << "）" << endl;
        return true;
    }

    void showStatus() {
        cout << "--- " << name << " ---" << endl;
        cout << "HP: " << currentHp << "/" << stats.maxHp << endl;
        cout << "MP: " << currentMp << "/" << stats.maxMp << endl;
        cout << "位置: (" << position.x << ", " << position.y << ")" << endl;
        cout << "攻擊: " << stats.attack << " / 防禦: " << stats.defense << endl;
    }

private:
    string name;
    CharacterStats stats;   // struct 作為 class 的成員
    Vector2D position;      // struct 作為 class 的成員
    int currentHp = 0;
    int currentMp = 0;
};

int main() {
    // 用 struct 定義角色屬性（純資料）
    CharacterStats warriorStats;
    warriorStats.maxHp = 150;
    warriorStats.maxMp = 30;
    warriorStats.attack = 20;
    warriorStats.defense = 12;
    warriorStats.speed = 0.8;

    CharacterStats mageStats;
    mageStats.maxHp = 80;
    mageStats.maxMp = 120;
    mageStats.attack = 8;
    mageStats.defense = 3;
    mageStats.speed = 1.2;

    // 用 class 建立角色（有行為和保護）
    GameCharacter warrior;
    warrior.init("戰士", warriorStats);

    GameCharacter mage;
    mage.init("法師", mageStats);

    // 遊戲模擬
    cout << "===== 初始狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    cout << "\n===== 戰鬥 =====" << endl;
    Vector2D target;
    target.x = 5.0;
    target.y = 3.0;
    warrior.moveTo(target);

    mage.useSkill(40);           // 法師放技能
    warrior.takeDamage(35);      // 戰士受到攻擊
    warrior.takeDamage(35);      // 再次受到攻擊

    mage.useSkill(40);           // 法師再放技能
    mage.useSkill(40);           // 第三次
    mage.useSkill(40);           // MP 不足？

    cout << "\n===== 最終狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    return 0;
}
```

**預期輸出：**
```
===== 初始狀態 =====
--- 戰士 ---
HP: 150/150
MP: 30/30
位置: (0, 0)
攻擊: 20 / 防禦: 12

--- 法師 ---
HP: 80/80
MP: 120/120
位置: (0, 0)
攻擊: 8 / 防禦: 3

===== 戰鬥 =====
戰士 移動到 (0.685994, 0.411597)
法師 使用技能（MP: 80/120）
戰士 受到 23 點傷害（HP: 127/150）
戰士 受到 23 點傷害（HP: 104/150）
法師 使用技能（MP: 40/120）
法師 使用技能（MP: 0/120）
法師: MP 不足！

===== 最終狀態 =====
--- 戰士 ---
HP: 104/150
MP: 30/30
位置: (0.685994, 0.411597)
攻擊: 20 / 防禦: 12

--- 法師 ---
HP: 80/80
MP: 0/120
位置: (0, 0)
攻擊: 8 / 防禦: 3
```

**設計分析**：

| 型別 | 用什麼 | 原因 |
|------|--------|------|
| `Vector2D` | struct | 純資料（x, y），所有成員公開，無不變條件 |
| `CharacterStats` | struct | 純資料配置，所有成員公開 |
| `GameCharacter` | class | 有行為邏輯、資料保護（HP 不能直接被設為負數）、不變條件（currentHp ≤ maxHp） |

---

## 七、常見誤解澄清

### 誤解 1：「struct 是 C 的東西，C++ 應該用 class」

**錯誤**。C++ 的 struct 是一等公民，在標準庫中大量使用。例如 `std::pair` 就是 struct。

### 誤解 2：「struct 不能有成員函數」

**錯誤**。C++ 的 struct 可以有成員函數、建構函數、虛函數——一切 class 能做的。

### 誤解 3：「用了 private 就必須改成 class」

**不一定**，但這是一個強烈的信號。如果你的 struct 開始需要 private，通常意味著它已經不是「簡單資料集合」了，應該改用 class。

### 誤解 4：「struct 效能比 class 好」

**完全一樣**。編譯器對 struct 和 class 的處理方式完全相同，不存在效能差異。

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| 語法差異 | 只有預設存取（struct=public, class=private）和預設繼承方式 |
| 功能差異 | 沒有。struct 能做的 class 都能做 |
| struct 使用時機 | 純資料集合、全 public、無不變條件 |
| class 使用時機 | 有行為、有保護、有不變條件 |
| C struct vs C++ struct | C++ struct 功能遠超 C struct |
| 效能差異 | 完全沒有 |
| 實務慣例 | 看需求選擇，保持團隊一致 |

---

第二階段「類別與對象基礎」到此完成！🎉

接下來進入 **第三階段：建構與解構**，從 **第 13 課：建構函數（constructor）基礎** 開始。建構函數是建立對象時自動呼叫的特殊函數，它會取代我們目前用的 `init()` 函數，讓對象的初始化更安全、更優雅。準備好就告訴我！
*/



#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// ===== struct：純資料 =====
struct Vector2D {
    double x = 0;
    double y = 0;

    double length() {
        return sqrt(x * x + y * y);
    }

    Vector2D add(const Vector2D& other) {
        Vector2D result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
    }
};

struct CharacterStats {
    int maxHp = 100;
    int maxMp = 50;
    int attack = 10;
    int defense = 5;
    double speed = 1.0;
};

// ===== class：有行為和保護 =====
class GameCharacter {
public:
    void init(const string& charName, const CharacterStats& charStats) {
        name = charName;
        stats = charStats;
        currentHp = stats.maxHp;
        currentMp = stats.maxMp;
    }

    void moveTo(const Vector2D& target) {
        Vector2D direction;
        direction.x = target.x - position.x;
        direction.y = target.y - position.y;

        double dist = direction.length();
        if (dist > 0) {
            // 根據速度移動（簡化版）
            position.x += (direction.x / dist) * stats.speed;
            position.y += (direction.y / dist) * stats.speed;
        }
        cout << name << " 移動到 (" << position.x << ", " << position.y << ")" << endl;
    }

    bool takeDamage(int damage) {
        int actualDamage = damage - stats.defense;
        if (actualDamage < 1) actualDamage = 1;  // 最少受 1 點傷害

        currentHp -= actualDamage;
        if (currentHp < 0) currentHp = 0;

        cout << name << " 受到 " << actualDamage << " 點傷害"
             << "（HP: " << currentHp << "/" << stats.maxHp << "）" << endl;

        return currentHp > 0;  // 返回是否存活
    }

    bool useSkill(int mpCost) {
        if (currentMp < mpCost) {
            cout << name << ": MP 不足！" << endl;
            return false;
        }
        currentMp -= mpCost;
        cout << name << " 使用技能"
             << "（MP: " << currentMp << "/" << stats.maxMp << "）" << endl;
        return true;
    }

    void showStatus() {
        cout << "--- " << name << " ---" << endl;
        cout << "HP: " << currentHp << "/" << stats.maxHp << endl;
        cout << "MP: " << currentMp << "/" << stats.maxMp << endl;
        cout << "位置: (" << position.x << ", " << position.y << ")" << endl;
        cout << "攻擊: " << stats.attack << " / 防禦: " << stats.defense << endl;
    }

private:
    string name;
    CharacterStats stats;   // struct 作為 class 的成員
    Vector2D position;      // struct 作為 class 的成員
    int currentHp = 0;
    int currentMp = 0;
};

int main() {
    // 用 struct 定義角色屬性（純資料）
    CharacterStats warriorStats;
    warriorStats.maxHp = 150;
    warriorStats.maxMp = 30;
    warriorStats.attack = 20;
    warriorStats.defense = 12;
    warriorStats.speed = 0.8;

    CharacterStats mageStats;
    mageStats.maxHp = 80;
    mageStats.maxMp = 120;
    mageStats.attack = 8;
    mageStats.defense = 3;
    mageStats.speed = 1.2;

    // 用 class 建立角色（有行為和保護）
    GameCharacter warrior;
    warrior.init("戰士", warriorStats);

    GameCharacter mage;
    mage.init("法師", mageStats);

    // 遊戲模擬
    cout << "===== 初始狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    cout << "\n===== 戰鬥 =====" << endl;
    Vector2D target;
    target.x = 5.0;
    target.y = 3.0;
    warrior.moveTo(target);

    mage.useSkill(40);           // 法師放技能
    warrior.takeDamage(35);      // 戰士受到攻擊
    warrior.takeDamage(35);      // 再次受到攻擊

    mage.useSkill(40);           // 法師再放技能
    mage.useSkill(40);           // 第三次
    mage.useSkill(40);           // MP 不足？

    cout << "\n===== 最終狀態 =====" << endl;
    warrior.showStatus();
    cout << endl;
    mage.showStatus();

    return 0;
}
