// =============================================================================
//  第 7 課：類別（class）的定義與語法3.cpp  —  類內／類外定義與成員函式的完整語法
// =============================================================================
//
//  ⚠️ 檔案結構說明：本檔【前段是一個 /* */ 註解區塊】（本課完整講義），
//     裡面有多個示範用的 class 與 int main()，那些程式碼【不會被編譯】。
//     真正會編譯的程式碼在檔案後段的 BankAccount 定義之後。
//     搜尋 BankAccount / main 時請以【最後一次出現】為準。
//
// 【主題資訊 Information】
//   類內定義：class C { void f() { ... } };          // 隱式 inline
//   類外定義：class C { void f(); };
//             void C::f() { ... }                     // 需要 C:: 限定
//   類內初始化（NSDMI）：double balance = 0.0;        // C++11 起
//   標準版本：類內／類外定義為 C++98；類內初始值為 C++11
//   標頭檔  ：語言核心特性
//
// 【詳細解釋 Explanation】
//
// 【1. 類內定義 vs 類外定義，差別在哪】
//   兩者的【語意完全相同】，差別只在：
//     • 類內定義：自動具有 inline 性質，
//       可以直接寫在標頭檔中被多個 .cpp 包含而不會產生重複定義錯誤。
//       缺點是類別宣告會被實作細節塞滿，難以一眼看出介面。
//     • 類外定義：類別宣告保持乾淨（只剩函式簽名，像一份目錄），
//       實作可以放到 .cpp 檔，修改實作時不必重新編譯所有包含者。
//   實務慣例：
//       短小（一兩行）、需要 inline 的 → 類內
//       稍長、或需要隱藏實作的       → 類外（宣告放 .hpp、定義放 .cpp）
//   本檔刻意兩種都示範：display() 在類內，deposit/withdraw 在類外。
//
// 【2. 類外定義為什麼一定要寫 BankAccount::】
//   因為 void deposit(double) 和 void BankAccount::deposit(double)
//   是【兩個完全不同】的函式 ——
//   前者是一個自由函式，後者才是類別的成員。
//   少寫 BankAccount:: 不會有語法錯誤，
//   而是會產生一個獨立的全域函式，同時類別的 deposit 變成
//   「有宣告但沒有定義」，直到連結階段才會爆出 undefined reference。
//   這是初學者最常見的連結錯誤來源之一。
//
// 【3. 類內初始值（C++11）解決了什麼】
//   balance = 0.0 這個寫法讓「帳戶預設餘額為 0」變成類別定義的一部分。
//   在 C++11 之前，只能靠建構子的初始化列表；
//   若有多個建構子，每一個都要記得寫，漏掉一個就是未初始化的 UB。
//   類內初始值只寫一次，所有建構子都會套用
//   （建構子的初始化列表若有指定，則以初始化列表為準）。
//   本檔的 ownerName 與 accountId 是 std::string，
//   它們的預設建構子本來就會產生空字串，所以不需要明寫。
//   但 balance 是 double —— 沒有類內初始值就是不確定的值。
//
// 【4. 這個類別的設計缺陷（本課刻意保留）】
//   BankAccount 把 balance 設為 public，
//   代表任何人都能寫 acc1.balance = 999999; 直接繞過 deposit/withdraw
//   的所有檢查。deposit 裡的「金額必須大於 0」形同虛設。
//   真正的封裝要等第 20 課：把 balance 設為 private，
//   讓「餘額不可為負」成為型別本身的保證。
//   本檔的重點是【語法】——類內／類外定義、類內初始值 ——
//   請不要把它當成金融系統的設計範本。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 用 double 存金額是真實世界的嚴重錯誤
//   本檔的輸出剛好都是整數（1000、500、200）所以看不出問題，
//   但 double 是二進位浮點數，無法精確表示 0.1 這類十進位小數。
//       0.1 + 0.2 == 0.3        // 在 double 下為 false
//   金融系統一律用【整數的最小單位】（例如以「分」為單位存 long long）
//   或十進位定點數型別。
//   本機實測（以 setprecision(17) 印出）：把 0.1 累加十次得到
//       0.99999999999999989      // 而不是 1.0
//   `== 1.0` 的結果是 false，`0.1 + 0.2 == 0.3` 同樣是 false。
//   注意這個誤差用預設的 6 位有效數字印出來會顯示成「1」——
//   看起來完全正常，但比較時就是不相等。
//   這種「印出來對、比較起來錯」的誤差，在對帳時就是無法平的帳。
//
// (B) 為什麼 display() 應該是 const 成員函式
//   display() 只讀不寫，宣告成 void display() const 之後：
//   const BankAccount& 也能呼叫它，而且編譯器會保證它不修改成員。
//   本檔未加 const 是簡化；
//   一旦某處出現 const BankAccount& acc 然後呼叫 acc.display()，
//   就會編譯失敗 —— 這是「const 正確性」要一開始就做對的理由，
//   事後補會牽連整份程式碼。
//
// (C) 餘額為什麼印成 $1300 而不是 $1300.00
//   std::cout 對 double 使用預設格式（6 位有效數字），
//   會自動去掉尾隨的零。要顯示成金額格式必須明確指定：
//       std::cout << std::fixed << std::setprecision(2) << balance;
//   （見第 5 課 15 號檔）
//
// 【注意事項 Pay Attention】
//   1. 本檔前段是註解區塊，其中的 class 與 main 都不會被編譯。
//   2. 類外定義【必須】寫 ClassName::，否則會變成一個獨立的自由函式，
//      並在連結階段報 undefined reference。
//   3. 類內定義隱式 inline；類外定義若寫在標頭檔中要自己加 inline。
//   4. 類內初始值是 C++11 起的功能。
//   5. 本檔的 balance 是 public，所有檢查都能被繞過 ——
//      這是教學順序上的簡化，封裝見第 20 課。
//   6. 【絕對不要】用 double 存真實金額，浮點誤差會導致對不上帳。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類內／類外定義與成員初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員函式寫在類別內和寫在類別外，有什麼實質差別？
//     答：語意完全相同，差別在兩點：
//         ① 類內定義【隱式 inline】，因此可以放在標頭檔被多個 .cpp
//            包含而不違反單一定義規則（ODR）；
//            類外定義若要放標頭檔，必須自己加 inline。
//         ② 工程上，類外定義能讓類別宣告保持乾淨（像一份介面目錄），
//            實作改動時不必重新編譯所有包含該標頭的檔案。
//     追問：inline 是在要求編譯器展開嗎？
//           → 現代編譯器早就不把 inline 當成展開的指示了。
//             inline 現在的實質意義是【連結性質】：
//             允許多個翻譯單元出現相同定義。
//             要不要真的展開完全由最佳化器決定。
//
// 🔥 Q2. 類外定義時忘記寫 BankAccount:: 會發生什麼？
//     答：不會有語法錯誤 —— 編譯器會把它當成一個全域的自由函式定義。
//         同時類別裡的 deposit 就變成「只有宣告、沒有定義」，
//         直到【連結階段】才會報 undefined reference to
//         'BankAccount::deposit(double)'。
//         這種錯誤訊息離真正的問題點很遠，是初學者最常卡住的地方之一。
//     追問：那類內初始值和建構子初始化列表衝突時，以哪個為準？
//           → 以【建構子的初始化列表】為準。
//             類內初始值的角色是「所有建構子的預設值」，
//             任何建構子若明確指定了該成員，就會覆蓋掉它。
//
// ⚠️ 陷阱. 這個 BankAccount 的 deposit 檢查了「金額必須大於 0」，
//          withdraw 也檢查了餘額是否足夠 ——
//          所以餘額不可能變成負數，對嗎？
//     答：不對。balance 是 public 成員，
//         任何人都可以寫 acc1.balance = -99999;
//         完全繞過那兩個函式。
//         檢查邏輯只在「大家願意透過 deposit/withdraw 存取」時才有效，
//         而型別本身沒有提供任何保證。
//     為什麼會錯：把「函式裡寫了檢查」等同於「不變量成立」。
//         不變量能不能成立，取決於【有沒有其他路徑可以繞過檢查】。
//         只要資料是 public，就永遠存在繞過的路徑，
//         而且出問題時無法追查是誰改的（沒有單一入口可以下中斷點）。
//         這正是封裝的核心論點：private 不是為了「防駭客」，
//         而是為了把可能修改狀態的程式碼【收斂到可以檢查的少數幾個地方】。
//         （順帶一提，本檔用 double 存金額也是真實世界的嚴重錯誤，
//           見上方【概念補充 (A)】。）
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第 7 課：類別（class）的定義與語法

## 一、什麼是類別（class）？

在上一課我們談到，OOP 的核心是「把資料和操作資料的函數綁在一起」。**類別（class）** 就是實現這個想法的語法工具。

你可以把 class 想成一張**藍圖（blueprint）**：

- 它**定義**了一個東西有什麼屬性（資料）和能做什麼事（函數）
- 但它本身**不是**實際的東西——你需要根據藍圖去「建造」出來，那才是**對象（object）**

打個比方：

```
class Car = 汽車的設計圖紙
    → 圖紙上寫了：有引擎、有輪子、能加速、能煞車

Car myCar = 根據圖紙造出來的一輛實際的車
    → 這輛車有具體的引擎馬力、具體的輪子尺寸
```

---

## 二、class 的基本語法

```cpp
class 類別名稱 {
存取修飾符:
    // 成員變數（屬性）
    // 成員函數（行為）
};  // ← 注意這個分號！很多初學者會忘記
```

讓我們看一個最簡單的完整範例：

```cpp
#include <iostream>
#include <string>
using namespace std;

// ========== 類別定義 ==========
class Dog {
public:
    // 成員變數（Member Variables）—— 描述「狗有什麼」
    string name;
    int age;
    string breed;  // 品種

    // 成員函數（Member Functions）—— 描述「狗能做什麼」
    void bark() {
        cout << name << " 說：汪汪！" << endl;
    }

    void sit() {
        cout << name << " 乖乖坐下了" << endl;
    }

    void showInfo() {
        cout << "名字: " << name << endl;
        cout << "年齡: " << age << " 歲" << endl;
        cout << "品種: " << breed << endl;
    }
};  // ← 別忘了分號！

// ========== 主程式 ==========
int main() {
    Dog myDog;              // 根據 Dog 類別建造一個對象
    myDog.name = "旺財";    // 設定屬性
    myDog.age = 3;
    myDog.breed = "柴犬";

    myDog.showInfo();       // 調用成員函數
    myDog.bark();
    myDog.sit();

    return 0;
}
```

**預期輸出：**
```
名字: 旺財
年齡: 3 歲
品種: 柴犬
旺財 說：汪汪！
旺財 乖乖坐下了
```

---

## 三、語法細節逐一解析

### 3.1 類別名稱的命名慣例

```cpp
class Dog { };          // ✅ 大駝峰（PascalCase）—— C++ 最常用
class BankAccount { };  // ✅ 多個單字也用大駝峰
class dog { };          // ⚠️ 能編譯，但不符合慣例
class DOG { };          // ⚠️ 全大寫通常留給巨集（macro）
```

**C++ 慣例**：類別名用 **PascalCase**（每個單字首字母大寫），和變數名的 camelCase 或 snake_case 區分開。

---

### 3.2 存取修飾符（Access Specifiers）

class 有三種存取等級：

```cpp
class Example {
public:      // 任何人都能存取
    int a;

protected:   // 只有自己和子類能存取
    int b;

private:     // 只有自己能存取
    int c;
};
```

**關鍵規則**：在 class 中，如果你不寫任何存取修飾符，**預設是 `private`**。

```cpp
class Foo {
    int x;      // 這是 private！（class 預設）
public:
    int y;      // 這是 public
};
```

這和 `struct` 不同——`struct` 預設是 `public`（第 12 課會詳細比較）。

**目前階段**：我們先集中使用 `public`，到第 11 課會深入講解三種修飾符的差異。

---

### 3.3 成員變數（Member Variables）

成員變數就是定義在類別內部的變數，描述對象的**屬性/狀態**：

```cpp
class Student {
public:
    string name;        // 姓名
    int age;            // 年齡
    float gpa;          // 成績
    bool isEnrolled;    // 是否在學
};
```

**注意**：成員變數不能在宣告時使用其他成員來初始化（C++11 之前），但可以給**預設值**：

```cpp
class Student {
public:
    string name = "未命名";   // ✅ C++11 起支援類內初始化
    int age = 0;              // ✅ 給預設值
    float gpa = 0.0f;         // ✅
    bool isEnrolled = false;  // ✅
};
```

---

### 3.4 成員函數（Member Functions）

成員函數定義在類別內部，描述對象的**行為/能力**。

成員函數有兩種定義方式：

#### 方式一：類內定義（inline，直接寫在 class 裡面）

```cpp
class Calculator {
public:
    int add(int a, int b) {
        return a + b;   // 直接在 class 內寫完整函數體
    }

    int subtract(int a, int b) {
        return a - b;
    }
};
```

**適合**：函數體很短（1~3 行）的情況。

#### 方式二：類外定義（宣告和實現分離）

```cpp
class Calculator {
public:
    int add(int a, int b);          // 類內只宣告
    int subtract(int a, int b);     // 類內只宣告
    void showResult(int result);    // 類內只宣告
};

// 類外定義：用 類別名::函數名 的語法
int Calculator::add(int a, int b) {
    return a + b;
}

int Calculator::subtract(int a, int b) {
    return a - b;
}

void Calculator::showResult(int result) {
    cout << "結果 = " << result << endl;
}
```

**`::` 是什麼？** 這是**範圍解析運算子（scope resolution operator）**，告訴編譯器：「這個 `add` 函數是屬於 `Calculator` 類別的，不是一個獨立的全域函數。」

**適合**：函數體較長的情況，讓類別定義保持簡潔。在實際專案中，通常把宣告放在 `.h` 標頭檔，實現放在 `.cpp` 原始碼檔。

---

### 3.5 成員函數可以直接存取成員變數

這是 class 最核心的特性之一——成員函數天生就能存取同一個類別的成員變數，不需要額外傳參數：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Person {
public:
    string name;
    int age;

    void introduce() {
        // 直接使用 name 和 age，不需要傳參數！
        cout << "你好，我叫 " << name << "，今年 " << age << " 歲。" << endl;
    }
};

int main() {
    Person p1;
    p1.name = "小明";
    p1.age = 20;
    p1.introduce();  // 輸出：你好，我叫 小明，今年 20 歲。

    Person p2;
    p2.name = "小華";
    p2.age = 22;
    p2.introduce();  // 輸出：你好，我叫 小華，今年 22 歲。

    return 0;
}
```

**預期輸出：**
```
你好，我叫 小明，今年 20 歲。
你好，我叫 小華，今年 22 歲。
```

**對比 C 語言**：在 C 中，你必須把 struct 指標手動傳入函數 `introduce(struct Person* p)`。在 C++ 中，成員函數自動知道自己屬於哪個對象（背後是 `this` 指標，第 26 課會講）。

---

## 四、完整實戰範例：銀行帳戶

讓我們用一個稍微複雜的例子，把以上所有概念串起來：

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
public:
    // ===== 成員變數 =====
    string ownerName;
    string accountId;
    double balance = 0.0;  // 類內預設值

    // ===== 成員函數（類內定義）=====
    void display() {
        cout << "========================" << endl;
        cout << "帳戶持有人: " << ownerName << endl;
        cout << "帳號:       " << accountId << endl;
        cout << "餘額:       $" << balance << endl;
        cout << "========================" << endl;
    }

    // ===== 成員函數（類內宣告，類外定義）=====
    void deposit(double amount);
    void withdraw(double amount);
};

// 類外定義
void BankAccount::deposit(double amount) {
    if (amount > 0) {
        balance += amount;
        cout << "存入 $" << amount << "，目前餘額: $" << balance << endl;
    } else {
        cout << "錯誤：存款金額必須大於 0" << endl;
    }
}

void BankAccount::withdraw(double amount) {
    if (amount <= 0) {
        cout << "錯誤：提款金額必須大於 0" << endl;
    } else if (amount > balance) {
        cout << "錯誤：餘額不足！目前餘額: $" << balance << endl;
    } else {
        balance -= amount;
        cout << "提取 $" << amount << "，目前餘額: $" << balance << endl;
    }
}

int main() {
    // 建立第一個帳戶
    BankAccount acc1;
    acc1.ownerName = "陳信安";
    acc1.accountId = "ACC-001";

    acc1.display();
    acc1.deposit(1000.0);
    acc1.deposit(500.0);
    acc1.withdraw(200.0);
    acc1.withdraw(2000.0);  // 故意超額
    acc1.deposit(-100.0);   // 故意輸入負數

    cout << endl;
    acc1.display();

    cout << "\n--- 第二個帳戶 ---\n" << endl;

    // 建立第二個帳戶——獨立的對象，有自己的資料
    BankAccount acc2;
    acc2.ownerName = "小明";
    acc2.accountId = "ACC-002";
    acc2.balance = 3000.0;

    acc2.display();
    acc2.withdraw(500.0);
    acc2.display();

    return 0;
}
```

**預期輸出：**
```
========================
帳戶持有人: 陳信安
帳號:       ACC-001
餘額:       $0
========================
存入 $1000，目前餘額: $1000
存入 $500，目前餘額: $1500
提取 $200，目前餘額: $1300
錯誤：餘額不足！目前餘額: $1300
錯誤：存款金額必須大於 0

========================
帳戶持有人: 陳信安
帳號:       ACC-001
餘額:       $1300
========================

--- 第二個帳戶 ---

========================
帳戶持有人: 小明
帳號:       ACC-002
餘額:       $3000
========================
提取 $500，目前餘額: $2500
========================
帳戶持有人: 小明
帳號:       ACC-002
餘額:       $2500
========================
```

**這個範例展示了：**
- `acc1` 和 `acc2` 是同一個類別建造出來的兩個**獨立對象**，各自擁有自己的 `balance`
- 成員函數內可以直接存取成員變數（`balance += amount`）
- 混合使用類內定義（`display`）和類外定義（`deposit`、`withdraw`）

---

## 五、class 定義的常見錯誤

### 錯誤 1：忘記結尾分號

```cpp
class Dog {
public:
    string name;
}   // ❌ 缺少分號，編譯錯誤！

class Dog {
public:
    string name;
};  // ✅ 正確
```

編譯器報錯通常是莫名其妙的訊息，所以養成習慣：**class 結尾一定加 `;`**。

### 錯誤 2：在成員函數外直接使用成員變數

```cpp
class Dog {
public:
    string name;
    cout << name << endl;  // ❌ 不能在這裡寫執行語句！
};
```

類別定義裡只能放**宣告**（變數宣告、函數宣告/定義），不能放獨立的執行語句。

### 錯誤 3：類外定義忘記加類別名

```cpp
class Dog {
public:
    void bark();
};

void bark() {              // ❌ 這是一個全域函數，不是 Dog 的成員！
    cout << "汪！" << endl;
}

void Dog::bark() {         // ✅ 正確：屬於 Dog 類別的 bark
    cout << "汪！" << endl;
}
```

---

## 六、class 與 C 語言 struct + 函數的完整對比

```cpp
// ===== C 語言風格 =====
struct Dog_C {
    char name[50];
    int age;
};

void dog_bark(struct Dog_C* d) {
    printf("%s 汪汪!\n", d->name);
}

// 使用
struct Dog_C d;
strcpy(d.name, "旺財");
d.age = 3;
dog_bark(&d);  // 必須手動傳入指標


// ===== C++ 類別風格 =====
class Dog_CPP {
public:
    string name;
    int age;

    void bark() {
        cout << name << " 汪汪!" << endl;
    }
};

// 使用
Dog_CPP d;
d.name = "旺財";
d.age = 3;
d.bark();      // 自然的「對象.行為」語法
```

C++ 的 class 讓「資料」和「行為」的關係從**語法層面**就綁在一起，而不是靠程式設計師的自律。

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| class | 定義對象的藍圖，包含成員變數和成員函數 |
| 成員變數 | 描述對象的屬性/狀態 |
| 成員函數 | 描述對象的行為/能力 |
| 存取修飾符 | `public`、`private`、`protected` 控制存取權限 |
| class 預設存取 | `private`（struct 預設是 `public`） |
| 類內定義 | 函數直接寫在 class 裡面 |
| 類外定義 | 用 `類別名::函數名` 在 class 外面實現 |
| `::` | 範圍解析運算子 |
| 類內初始化 | C++11 起可在宣告時給成員變數預設值 |
| 結尾分號 | class 定義結束後必須加 `;` |

---

下一課是 **第 8 課：對象（object）的創建與使用**，我們會深入探討如何建立對象、對象在記憶體中的佈局，以及多個對象之間的獨立性。準備好了就告訴我！
*/



#include <iostream>
#include <string>
using namespace std;

class BankAccount {
public:
    // ===== 成員變數 =====
    string ownerName;
    string accountId;
    double balance = 0.0;  // 類內預設值

    // ===== 成員函數（類內定義）=====
    void display() {
        cout << "========================" << endl;
        cout << "帳戶持有人: " << ownerName << endl;
        cout << "帳號:       " << accountId << endl;
        cout << "餘額:       $" << balance << endl;
        cout << "========================" << endl;
    }

    // ===== 成員函數（類內宣告，類外定義）=====
    void deposit(double amount);
    void withdraw(double amount);
};

// 類外定義
void BankAccount::deposit(double amount) {
    if (amount > 0) {
        balance += amount;
        cout << "存入 $" << amount << "，目前餘額: $" << balance << endl;
    } else {
        cout << "錯誤：存款金額必須大於 0" << endl;
    }
}

void BankAccount::withdraw(double amount) {
    if (amount <= 0) {
        cout << "錯誤：提款金額必須大於 0" << endl;
    } else if (amount > balance) {
        cout << "錯誤：餘額不足！目前餘額: $" << balance << endl;
    } else {
        balance -= amount;
        cout << "提取 $" << amount << "，目前餘額: $" << balance << endl;
    }
}

int main() {
    // 建立第一個帳戶
    BankAccount acc1;
    acc1.ownerName = "陳信安";
    acc1.accountId = "ACC-001";

    acc1.display();
    acc1.deposit(1000.0);
    acc1.deposit(500.0);
    acc1.withdraw(200.0);
    acc1.withdraw(2000.0);  // 故意超額
    acc1.deposit(-100.0);   // 故意輸入負數

    cout << endl;
    acc1.display();

    cout << "\n--- 第二個帳戶 ---\n" << endl;

    // 建立第二個帳戶——獨立的對象，有自己的資料
    BankAccount acc2;
    acc2.ownerName = "小明";
    acc2.accountId = "ACC-002";
    acc2.balance = 3000.0;

    acc2.display();
    acc2.withdraw(500.0);
    acc2.display();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 7 課：類別（class）的定義與語法3.cpp" -o class3
//   類內初始值（double balance = 0.0;）是 C++11 起的功能。

// 註 1:本檔前段是 /* */ 註解區塊（本課完整講義），
//      其中的 class 與 int main() 都【不會被編譯】。

// 註 2:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。

// 註 3:兩次「錯誤」訊息是【刻意觸發】的：
//        withdraw(2000) → 超過餘額 1300，被 withdraw 的檢查擋下
//        deposit(-100)  → 金額為負，被 deposit 的檢查擋下
//      但請注意：balance 是 public 成員，
//      直接寫 acc1.balance = -99999; 就能完全繞過這兩個檢查。

// 註 4:餘額印成 $1300 而不是 $1300.00，
//      是因為 iostream 對 double 使用預設的 6 位有效數字格式
//      並去掉尾隨的零。要顯示金額格式需 std::fixed << std::setprecision(2)。

// 註 5:⚠️ 本檔用 double 存金額，這在真實系統中是嚴重錯誤 ——
//      二進位浮點無法精確表示 0.1 這類十進位小數。
//      本檔的數字剛好都是整數所以看不出問題。
//      實務請用整數的最小單位（例如以「分」為單位的 long long）。

// === 預期輸出 ===
// ========================
// 帳戶持有人: 陳信安
// 帳號:       ACC-001
// 餘額:       $0
// ========================
// 存入 $1000，目前餘額: $1000
// 存入 $500，目前餘額: $1500
// 提取 $200，目前餘額: $1300
// 錯誤：餘額不足！目前餘額: $1300
// 錯誤：存款金額必須大於 0
//
// ========================
// 帳戶持有人: 陳信安
// 帳號:       ACC-001
// 餘額:       $1300
// ========================
//
// --- 第二個帳戶 ---
//
// ========================
// 帳戶持有人: 小明
// 帳號:       ACC-002
// 餘額:       $3000
// ========================
// 提取 $500，目前餘額: $2500
// ========================
// 帳戶持有人: 小明
// 帳號:       ACC-002
// 餘額:       $2500
// ========================
