// =============================================================================
//  第 8 課：對象（object）的創建與使用8.cpp  —  物件當參數：傳值 / 傳參考 / const 參考
// =============================================================================
//
//  ⚠️ 檔案結構說明：本檔【前 572 行是一個 /* */ 註解區塊】（本課完整講義），
//     裡面有多個示範用的 class 與 int main()，那些程式碼【不會被編譯】。
//     真正會編譯的程式從第 575 行的 #include 開始。
//     搜尋 Box / main 時請以【最後一次出現】為準。
//
// 【主題資訊 Information】
//   void f(Box b);          // 傳值      —— 複製一份，函式內的修改不影響原物件
//   void f(Box& b);         // 傳參考    —— 操作原物件本身，可修改
//   void f(const Box& b);   // const 參考 —— 不複製也不可修改（最常用）
//   標準版本：C++98 起（類內初始值為 C++11；移動語意為 C++11）
//   複雜度：傳值 O(複製成本)；傳參考 O(1)
//   標頭檔：語言核心特性
//
// 【詳細解釋 Explanation】
//
// 【1. 三種傳遞方式的選擇準則】
//   這是 C++ 日常寫程式最高頻的決策之一，準則其實很簡單：
//       需要修改呼叫者的物件        → T&
//       只讀，而且物件不小          → const T&      ← 預設就選這個
//       只讀，而且物件很小（int、指標、bool）→ 直接傳值 T
//       需要取得所有權（會存起來）  → T（傳值）並在函式內 std::move
//   本檔的 Box 只有兩個 double（本機 sizeof = 16），
//   其實傳值也不算貴；但一旦成員換成 std::string 或 std::vector，
//   傳值就會觸發堆積配置與整段資料複製 —— 差距是數量級的。
//
// 【2. 傳值為什麼「看起來有效果卻沒有效果」】
//   printByValue 內部把 b.width 改成 999，
//   但回到 main 之後 box.width 仍然是 5。
//   因為 b 是一份【獨立的複製品】，函式結束時就消失了。
//   這種錯誤特別難察覺，因為函式內若有輸出，
//   印出來的數值是「改過的」，畫面上一切正常 ——
//   直到後面才發現原物件從未改變。
//   本檔刻意在 printByValue 裡留下那行修改，就是要示範這一點。
//
// 【3. const 參考為什麼是預設選擇】
//   它同時給了三個保證：
//     ① 不複製 —— 成本是 O(1)，與物件大小無關
//     ② 不會修改 —— 編譯器強制檢查，讀者也能一眼看出意圖
//     ③ 能接受【暫存物件】—— 這點常被忽略：
//            printByConstRef(Box{1.0, 2.0});     // 合法
//            doubleSize(Box{1.0, 2.0});          // 編譯錯誤！
//        非 const 的左值參考不能綁定到暫存物件，
//        因為「修改一個即將消失的東西」幾乎必定是錯誤。
//
// 【4. const 參考的 const 是「不能透過這個參考修改」，不是「物件不會變」】
//   這是很細微但重要的區別：
//       Box box;
//       const Box& ref = box;
//       box.width = 99;         // 完全合法！ref.width 也跟著變成 99
//   const 限制的是【存取路徑】而不是物件本身。
//   真正的後果是在多執行緒或有別名（aliasing）的情況下，
//   不能因為參數是 const T& 就假設它在函式執行期間不會改變。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 傳值的成本到底有多大
//   對本檔的 Box（兩個 double，本機 sizeof = 16），
//   複製就是搬 16 bytes，幾乎免費。
//   但若 Box 有一個 std::vector<double> 成員且裝了一萬個元素：
//   傳值會觸發一次堆積配置 + 8 萬 bytes 的複製 + 之後的釋放。
//   這也是為什麼 C++ Core Guidelines 的建議是
//   【預設用 const T&，只有確定物件很小才傳值】。
//
// (B) 傳值在 C++11 之後不再總是壞事
//   若函式需要【取得一份自己的副本】（例如存進成員變數），
//   現代寫法反而是傳值再 move：
//       void setName(std::string n) { name_ = std::move(n); }
//   呼叫者傳左值時複製一次、傳右值（暫存物件）時是移動 ——
//   一個函式同時涵蓋兩種情況。
//   這比寫兩個多載（const& 與 &&）簡潔得多。
//
// (C) area() 應該是 const 成員函式
//   本檔的 area() 沒有加 const，這造成一個實際問題：
//   printByConstRef 收到的是 const Box&，
//   若它想呼叫 b.area() 會【編譯失敗】——
//   const 物件不能呼叫非 const 成員函式。
//   本檔剛好只在 printByValue（非 const）裡呼叫 area()，
//   所以沒有暴露這個問題。
//   這正是「const 正確性必須從一開始就做對」的實例：
//   一個成員函式漏了 const，會沿著呼叫鏈一路阻擋下去。
//
// 【注意事項 Pay Attention】
//   1. 本檔前 572 行是註解區塊，其中的 class 與 main 都不會被編譯。
//   2. 傳值只會修改到複製品，而且函式內的輸出看起來「正常」，
//      這種錯誤特別難察覺。
//   3. 非 const 的左值參考【不能】綁定暫存物件；const 參考可以。
//   4. const T& 的 const 限制的是【存取路徑】，
//      不保證物件在函式執行期間不被別的路徑修改。
//   5. 本檔的 area() 未加 const，因此 const Box& 無法呼叫它 ——
//      實務上所有不修改狀態的成員函式都應加 const。
//   6. 預設選 const T&；只有內建型別等小物件才傳值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】參數傳遞方式的選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼時候該用 const T&、什麼時候該傳值？
//     答：預設用 const T&，因為它不複製、不可修改、還能接受暫存物件。
//         只有兩種情況該傳值：
//         ① 物件很小（int、double、指標、bool 這類），
//            傳值反而少一次間接存取，通常更快；
//         ② 函式需要【取得一份自己的副本】存起來 ——
//            此時傳值再 std::move 進成員，
//            可以讓左值走複製、右值走移動，一個函式涵蓋兩種情況。
//     追問：那 T& 什麼時候用？
//           → 只在需要修改呼叫者的物件時。
//             若只是為了避免複製而用 T&，
//             會讓「這個函式會不會改我的東西」變得不明確 ——
//             const 在這裡是重要的溝通工具。
//
// 🔥 Q2. 為什麼 void f(Box&) 不能接受暫存物件，而 void f(const Box&) 可以？
//     答：因為非 const 的左值參考若能綁定暫存物件，
//         那些修改會寫進一個【即將消失】的物件裡，
//         幾乎必定是程式設計者的錯誤 —— 標準因此直接禁止它。
//         const 參考則沒有這個疑慮（反正不能改），
//         而且標準保證會把該暫存物件的生命週期延長到參考的作用域結束。
//     追問：那想接收暫存物件並修改它（例如取得資源）該怎麼寫？
//           → 用右值參考 T&&（C++11）。
//             這正是移動建構子與移動賦值的簽名，
//             語意是「這個物件反正要死了，我把它的資源搬走」。
//
// ⚠️ 陷阱. 函式參數宣告為 const Box& b，
//          那在函式執行期間 b.width 保證不會改變嗎？
//     答：不保證。const 限制的是【透過 b 這條路徑】不能修改，
//         而不是「這個物件不會變」。
//         如果同一個物件還有別的存取路徑（另一個非 const 的參考、
//         全域指標、或另一條執行緒），它隨時可能被改：
//             Box box;
//             const Box& ref = box;
//             box.width = 99;        // 合法，ref.width 也跟著變
//     為什麼會錯：把 const 讀成「這個東西是常數」。
//         它真正的意思是「【我】不會透過這個名字去改它」——
//         是對存取路徑的限制，不是對物件的宣告。
//         實際後果有兩個：
//         ① 編譯器【不能】因為參數是 const T& 就把讀取結果快取起來
//            （除非它能證明沒有別名）；
//         ② 多執行緒環境下，const 完全不等於執行緒安全 ——
//            要真正的不可變請用值語意或明確的同步機制。
// ═══════════════════════════════════════════════════════════════════════════

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

// 編譯: g++ -std=c++17 -Wall -Wextra "第 8 課：對象（object）的創建與使用8.cpp" -o obj8

// 註 1:本檔前 572 行是 /* */ 註解區塊（本課完整講義），
//      其中的 class 與 int main() 都【不會被編譯】。

// 註 2:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。

// 註 3:輸出中最值得注意的是「原始 width = 5」——
//      printByValue 內部明明把 b.width 改成了 999，
//      但那是複製品，原物件毫髮無傷。
//      這種錯誤在真實程式碼中特別難察覺，
//      因為函式內若有輸出，印出來的會是「改過的值」，畫面看起來完全正常。

// 註 4:接著 doubleSize(box) 用的是 Box&（參考），
//      所以 width 真的從 5 變成 10 —— 兩者對照即為本檔重點。

// 註 5:面積印成 15 而非 15.0，寬高印成 10 與 6 而非 10.0 / 6.0，
//      是 iostream 對 double 的預設格式（6 位有效數字、去掉尾隨零）所致。

// === 預期輸出 ===
// --- 傳值 ---
// 面積 = 15
// 原始 width = 5
//
// --- 傳引用 ---
// 放大後 width = 10
//
// --- 傳 const 引用 ---
// 寬: 10, 高: 6
