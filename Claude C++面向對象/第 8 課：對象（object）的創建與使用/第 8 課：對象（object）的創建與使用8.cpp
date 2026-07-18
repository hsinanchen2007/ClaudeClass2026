/*
# 第 8 課：對象（object）的創建與使用

## 一、對象是什麼？

上一課我們說 class 是**藍圖**，那對象就是根據藍圖**實際造出來的東西**。

```
class Dog = 「狗」這個概念的定義
Dog myDog = 一隻具體的、活生生的狗
```

更精確地說：**對象是類別的一個實例（instance）**。每個對象在記憶體中佔有自己的空間，擁有自己獨立的成員變數副本。

---

## 二、對象的創建方式

### 2.1 棧上創建（Stack Allocation）—— 最常用

```cpp
#include <iostream>
#include <string>
using namespace std;

class Dog {
public:
    string name;
    int age = 0;

    void bark() {
        cout << name << " 汪汪！" << endl;
    }
};

int main() {
    Dog d1;           // 建立對象，就像宣告一個普通變數
    d1.name = "旺財";
    d1.age = 3;
    d1.bark();

    Dog d2;           // 再建一個，完全獨立
    d2.name = "小黑";
    d2.age = 5;
    d2.bark();

    return 0;
}   // d1 和 d2 在這裡自動銷毀
```

**預期輸出：**
```
旺財 汪汪！
小黑 汪汪！
```

**特點**：
- 對象儲存在**棧（stack）** 上
- 離開作用域時**自動銷毀**，不需要手動管理
- 這是 C++ 中最常用、最安全的方式

---

### 2.2 堆上創建（Heap Allocation）—— 用 new / delete

```cpp
#include <iostream>
#include <string>
using namespace std;

class Dog {
public:
    string name;
    int age = 0;

    void bark() {
        cout << name << " 汪汪！" << endl;
    }
};

int main() {
    Dog* ptr = new Dog();    // 在堆上建立，返回指標
    ptr->name = "阿花";      // 用 -> 存取成員
    ptr->age = 2;
    ptr->bark();

    delete ptr;              // 必須手動釋放！
    ptr = nullptr;           // 好習慣：釋放後設為 nullptr

    return 0;
}
```

**預期輸出：**
```
阿花 汪汪！
```

**特點**：
- 對象儲存在**堆（heap）** 上
- 必須用 `delete` 手動釋放，否則**記憶體洩漏**
- 透過**指標**操作，用 `->` 而不是 `.`
- 後續課程會學到智能指標（`std::unique_ptr`）來自動管理

---

### 2.3 棧 vs 堆：完整對比

```cpp
#include <iostream>
#include <string>
using namespace std;

class Cat {
public:
    string name;

    void meow() {
        cout << name << " 喵～" << endl;
    }
};

int main() {
    // ===== 棧上 =====
    Cat c1;                // 自動分配
    c1.name = "咪咪";
    c1.meow();             // 用 . 存取

    // ===== 堆上 =====
    Cat* c2 = new Cat();   // 手動分配
    c2->name = "花花";
    c2->meow();            // 用 -> 存取

    // c2 也可以用 (*c2).meow()，但 -> 更方便
    (*c2).meow();          // 和上一行等價

    delete c2;             // 手動釋放
    c2 = nullptr;

    return 0;
}   // c1 在這裡自動銷毀
```

**預期輸出：**
```
咪咪 喵～
花花 喵～
花花 喵～
```

| 比較項目 | 棧上（Stack） | 堆上（Heap） |
|---------|--------------|-------------|
| 語法 | `Dog d;` | `Dog* p = new Dog();` |
| 存取方式 | `d.name` | `p->name` |
| 釋放 | 自動（離開作用域） | 手動 `delete p;` |
| 速度 | 快 | 較慢（需要記憶體管理） |
| 記憶體洩漏風險 | 無 | 有（忘記 delete） |
| 使用時機 | 大多數情況 | 需要動態大小或跨作用域存活 |

---

## 三、每個對象是獨立的

這是非常重要的概念：**同一個類別建立的不同對象，各自擁有獨立的成員變數副本**。

```cpp
#include <iostream>
#include <string>
using namespace std;

class Counter {
public:
    int count = 0;

    void increment() {
        count++;
    }

    void show() {
        cout << "count = " << count << endl;
    }
};

int main() {
    Counter a;
    Counter b;

    a.increment();   // a 的 count 變成 1
    a.increment();   // a 的 count 變成 2
    a.increment();   // a 的 count 變成 3

    b.increment();   // b 的 count 變成 1（和 a 無關！）

    cout << "a: ";
    a.show();

    cout << "b: ";
    b.show();

    return 0;
}
```

**預期輸出：**
```
a: count = 3
b: count = 1
```

**記憶體中的樣子**：

```
棧記憶體:
┌──────────────┐
│  對象 a       │
│  count = 3   │  ← a 有自己的 count
├──────────────┤
│  對象 b       │
│  count = 1   │  ← b 有自己的 count，和 a 完全無關
└──────────────┘
```

修改 `a.count` 完全不會影響 `b.count`，它們是**獨立的記憶體空間**。

---

## 四、對象的大小（sizeof）

一個對象在記憶體中佔多少空間？答案是：**所有成員變數大小的總和（可能有對齊填充）**。成員函數**不佔**對象的空間。

```cpp
#include <iostream>
using namespace std;

class Empty {
    // 什麼都沒有
};

class OnlyFunction {
public:
    void doSomething() {
        cout << "doing" << endl;
    }
};

class WithData {
public:
    int x;      // 4 bytes
    int y;      // 4 bytes
    double z;   // 8 bytes

    void show() {
        cout << x << ", " << y << ", " << z << endl;
    }
};

class Mixed {
public:
    char a;     // 1 byte
    int b;      // 4 bytes
    char c;     // 1 byte
};

int main() {
    cout << "sizeof(Empty)        = " << sizeof(Empty) << " bytes" << endl;
    cout << "sizeof(OnlyFunction) = " << sizeof(OnlyFunction) << " bytes" << endl;
    cout << "sizeof(WithData)     = " << sizeof(WithData) << " bytes" << endl;
    cout << "sizeof(Mixed)        = " << sizeof(Mixed) << " bytes" << endl;

    return 0;
}
```

**預期輸出（典型 64 位系統）：**
```
sizeof(Empty)        = 1 bytes
sizeof(OnlyFunction) = 1 bytes
sizeof(WithData)     = 16 bytes
sizeof(Mixed)        = 12 bytes
```

**解析：**

| 類別 | 分析 |
|------|------|
| `Empty` | 空類別大小為 **1**，不是 0。因為 C++ 規定每個對象必須有唯一的地址 |
| `OnlyFunction` | 成員函數不佔對象空間，所以和空類別一樣是 **1** |
| `WithData` | `int(4) + int(4) + double(8) = 16`，沒有填充 |
| `Mixed` | `char(1) + 填充(3) + int(4) + char(1) + 填充(3) = 12`，因為記憶體對齊 |

---

### 記憶體對齊（Memory Alignment）簡述

```
Mixed 的記憶體佈局：

地址:  0    1    2    3    4    5    6    7    8    9   10   11
     ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
     │ a  │pad │pad │pad │    b（4 bytes）    │ c  │pad │pad │pad │
     └────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘

- char a 佔 1 byte，但 int b 需要 4 byte 對齊，所以中間填充 3 bytes
- char c 佔 1 byte，但整個 struct 需要按最大成員（int = 4）對齊，所以尾部填充 3 bytes
- 總計：1 + 3 + 4 + 1 + 3 = 12 bytes
```

你在 C 語言中學過的 struct 對齊規則，在 C++ class 中**完全一樣**。

---

## 五、多個對象的交互

對象之間可以互相傳遞、比較、交互：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Player {
public:
    string name;
    int hp = 100;       // 生命值
    int attack = 10;    // 攻擊力

    void attackTarget(Player& target) {
        cout << name << " 攻擊了 " << target.name << "！" << endl;
        target.hp -= attack;
        cout << target.name << " 剩餘 HP: " << target.hp << endl;
    }

    void showStatus() {
        cout << "[" << name << "] HP: " << hp
             << " / 攻擊力: " << attack << endl;
    }

    bool isAlive() {
        return hp > 0;
    }
};

int main() {
    Player warrior;
    warrior.name = "戰士";
    warrior.hp = 120;
    warrior.attack = 15;

    Player mage;
    mage.name = "法師";
    mage.hp = 80;
    mage.attack = 25;

    cout << "===== 戰鬥開始 =====" << endl;
    warrior.showStatus();
    mage.showStatus();

    cout << "\n--- 第 1 回合 ---" << endl;
    warrior.attackTarget(mage);     // 戰士攻擊法師

    cout << "\n--- 第 2 回合 ---" << endl;
    mage.attackTarget(warrior);     // 法師攻擊戰士

    cout << "\n--- 第 3 回合 ---" << endl;
    warrior.attackTarget(mage);     // 戰士再攻擊法師

    cout << "\n===== 戰鬥結束 =====" << endl;
    warrior.showStatus();
    mage.showStatus();

    // 檢查存活狀態
    cout << "\n" << warrior.name << (warrior.isAlive() ? " 存活" : " 陣亡") << endl;
    cout << mage.name << (mage.isAlive() ? " 存活" : " 陣亡") << endl;

    return 0;
}
```

**預期輸出：**
```
===== 戰鬥開始 =====
[戰士] HP: 120 / 攻擊力: 15
[法師] HP: 80 / 攻擊力: 25

--- 第 1 回合 ---
戰士 攻擊了 法師！
法師 剩餘 HP: 65

--- 第 2 回合 ---
法師 攻擊了 戰士！
戰士 剩餘 HP: 95

--- 第 3 回合 ---
戰士 攻擊了 法師！
法師 剩餘 HP: 50

===== 戰鬥結束 =====
[戰士] HP: 95 / 攻擊力: 15
[法師] HP: 50 / 攻擊力: 25

戰士 存活
法師 存活
```

**注意這行**：`void attackTarget(Player& target)` — 參數是**引用**。這表示函數直接操作傳入的對象本身，而不是複製一份。所以 `target.hp -= attack` 真的會修改原始對象的 HP。

---

## 六、對象陣列

可以建立同一類別的多個對象組成的陣列：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    string name = "未命名";
    int score = 0;

    void show() {
        cout << name << ": " << score << " 分" << endl;
    }
};

int main() {
    // 建立包含 3 個 Student 對象的陣列
    Student students[3];

    // 分別設定
    students[0].name = "小明";
    students[0].score = 85;

    students[1].name = "小華";
    students[1].score = 92;

    students[2].name = "小美";
    students[2].score = 78;

    // 用迴圈遍歷
    cout << "===== 成績單 =====" << endl;
    for (int i = 0; i < 3; i++) {
        students[i].show();
    }

    // 找最高分
    int maxIdx = 0;
    for (int i = 1; i < 3; i++) {
        if (students[i].score > students[maxIdx].score) {
            maxIdx = i;
        }
    }
    cout << "\n最高分: " << students[maxIdx].name
         << " (" << students[maxIdx].score << " 分)" << endl;

    return 0;
}
```

**預期輸出：**
```
===== 成績單 =====
小明: 85 分
小華: 92 分
小美: 78 分

最高分: 小華 (92 分)
```

---

## 七、對象作為函數參數

對象可以用三種方式傳遞給函數：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Box {
public:
    double width = 0;
    double height = 0;

    double area() {
        return width * height;
    }
};

// 方式 1：傳值（複製一份，修改不影響原對象）
void printByValue(Box b) {
    cout << "面積 = " << b.area() << endl;
    b.width = 999;  // 修改的是複製品，原對象不受影響
}

// 方式 2：傳引用（直接操作原對象）
void doubleSize(Box& b) {
    b.width *= 2;
    b.height *= 2;
}

// 方式 3：傳 const 引用（不複製，但也不能修改）
void printByConstRef(const Box& b) {
    cout << "寬: " << b.width << ", 高: " << b.height << endl;
    // b.width = 10;  // ❌ 編譯錯誤！const 不允許修改
}

int main() {
    Box box;
    box.width = 5.0;
    box.height = 3.0;

    cout << "--- 傳值 ---" << endl;
    printByValue(box);
    cout << "原始 width = " << box.width << endl;  // 仍然是 5

    cout << "\n--- 傳引用 ---" << endl;
    doubleSize(box);
    cout << "放大後 width = " << box.width << endl;  // 變成 10

    cout << "\n--- 傳 const 引用 ---" << endl;
    printByConstRef(box);

    return 0;
}
```

**預期輸出：**
```
--- 傳值 ---
面積 = 15
原始 width = 5

--- 傳引用 ---
放大後 width = 10

--- 傳 const 引用 ---
寬: 10, 高: 6
```

**最佳實踐**：

| 方式 | 語法 | 何時使用 |
|------|------|---------|
| 傳值 | `void f(Box b)` | 需要一份獨立副本時（少用，有複製成本） |
| 傳引用 | `void f(Box& b)` | 需要修改原對象時 |
| 傳 const 引用 | `void f(const Box& b)` | **最常用！** 只讀取、不複製、不修改 |

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| 對象 | 類別的實例，記憶體中的實體 |
| 棧上創建 | `Dog d;` — 自動管理生命週期 |
| 堆上創建 | `Dog* p = new Dog(); ... delete p;` — 手動管理 |
| `.` vs `->` | 棧對象用 `.`，指標用 `->` |
| 對象獨立性 | 每個對象有自己獨立的成員變數副本 |
| sizeof | 只計算成員變數大小（含對齊填充），不計算成員函數 |
| 空類別大小 | 1 byte（保證唯一地址） |
| 記憶體對齊 | 和 C 語言 struct 規則一致 |
| 對象交互 | 對象可以透過引用互相操作 |
| 參數傳遞 | 優先使用 `const 引用` 傳遞對象 |

---

下一課是 **第 9 課：成員變數（member variables）**，我們會深入探討成員變數的各種類型、初始化方式，以及未初始化時的危險行為。準備好就告訴我！
*/



#include <iostream>
#include <string>
using namespace std;

class Box {
public:
    double width = 0;
    double height = 0;

    double area() {
        return width * height;
    }
};

// 方式 1：傳值（複製一份，修改不影響原對象）
void printByValue(Box b) {
    cout << "面積 = " << b.area() << endl;
    b.width = 999;  // 修改的是複製品，原對象不受影響
}

// 方式 2：傳引用（直接操作原對象）
void doubleSize(Box& b) {
    b.width *= 2;
    b.height *= 2;
}

// 方式 3：傳 const 引用（不複製，但也不能修改）
void printByConstRef(const Box& b) {
    cout << "寬: " << b.width << ", 高: " << b.height << endl;
    // b.width = 10;  // ❌ 編譯錯誤！const 不允許修改
}

int main() {
    Box box;
    box.width = 5.0;
    box.height = 3.0;

    cout << "--- 傳值 ---" << endl;
    printByValue(box);
    cout << "原始 width = " << box.width << endl;  // 仍然是 5

    cout << "\n--- 傳引用 ---" << endl;
    doubleSize(box);
    cout << "放大後 width = " << box.width << endl;  // 變成 10

    cout << "\n--- 傳 const 引用 ---" << endl;
    printByConstRef(box);

    return 0;
}
