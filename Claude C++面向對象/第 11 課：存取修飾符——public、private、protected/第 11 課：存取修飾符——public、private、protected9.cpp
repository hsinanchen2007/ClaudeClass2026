/*
# 第 11 課：存取修飾符——public、private、protected

## 一、為什麼需要存取控制？

先看一個沒有存取控制的危險例子：

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
public:
    string owner;
    double balance = 0.0;
};

int main() {
    BankAccount acc;
    acc.owner = "陳信安";
    acc.balance = 10000.0;

    // 任何人都能這樣做！
    acc.balance = -999999.0;   // 餘額變成負數？
    acc.balance = 0.0;         // 直接清空別人的錢？
    acc.owner = "";            // 帳戶名被清空？

    cout << acc.owner << ": $" << acc.balance << endl;
    return 0;
}
```

**預期輸出：**
```
: $0
```

所有資料完全暴露，任何程式碼都能隨意修改，沒有任何保護。這在小程式中可能沒問題，但在大型專案中是災難——任何一個模組的 bug 都可能悄悄破壞其他模組的資料。

**存取修飾符**就是 C++ 提供的保護機制。

---

## 二、三種存取修飾符

```
┌─────────────────────────────────────────────────────┐
│                    class MyClass                     │
│                                                     │
│  public:     任何人都能存取                           │
│  ─────────────────────────────────────               │
│  protected:  只有自己和子類能存取                      │
│  ─────────────────────────────────────               │
│  private:    只有自己能存取                            │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

### 2.1 public：完全公開

```cpp
#include <iostream>
using namespace std;

class Light {
public:
    bool isOn = false;

    void toggle() {
        isOn = !isOn;
    }

    void show() {
        cout << "燈: " << (isOn ? "開" : "關") << endl;
    }
};

int main() {
    Light lamp;
    lamp.show();        // ✅ 可以呼叫 public 函數
    lamp.toggle();      // ✅ 可以呼叫 public 函數
    lamp.show();
    lamp.isOn = false;  // ✅ 可以直接存取 public 變數
    lamp.show();
    return 0;
}
```

**預期輸出：**
```
燈: 關
燈: 開
燈: 關
```

`public` 成員就像商店的櫃台——任何人都可以來使用。

---

### 2.2 private：完全私有

```cpp
#include <iostream>
#include <string>
using namespace std;

class Safe {
private:
    string password = "secret123";
    double money = 50000.0;

public:
    bool unlock(const string& input) {
        if (input == password) {    // ✅ 類別內部可以存取 private
            cout << "保險箱已開啟，裡面有 $" << money << endl;
            return true;
        } else {
            cout << "密碼錯誤！" << endl;
            return false;
        }
    }
};

int main() {
    Safe mySafe;

    mySafe.unlock("wrong");      // ✅ 呼叫 public 函數
    mySafe.unlock("secret123");  // ✅ 呼叫 public 函數

    // mySafe.password;          // ❌ 編譯錯誤！private 不能從外部存取
    // mySafe.money = 0;         // ❌ 編譯錯誤！

    return 0;
}
```

**預期輸出：**
```
密碼錯誤！
保險箱已開啟，裡面有 $50000
```

如果你取消註解那兩行，編譯器會報錯：

```
error: 'std::string Safe::password' is private within this context
error: 'double Safe::money' is private within this context
```

`private` 成員就像保險箱裡的東西——只有保險箱自己（類別內部的函數）能碰。

---

### 2.3 protected：對子類開放

`protected` 介於 `public` 和 `private` 之間。外界不能存取，但**子類可以**：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Animal {
protected:
    string species = "未知";   // 外界不能存取，但子類可以
    int legs = 0;

public:
    void showInfo() {
        cout << species << ", " << legs << " 條腿" << endl;
    }
};

class Dog : public Animal {
public:
    void setup() {
        species = "犬科";   // ✅ 子類可以存取 protected
        legs = 4;           // ✅
    }
};

class Spider : public Animal {
public:
    void setup() {
        species = "蛛形綱"; // ✅ 子類可以存取 protected
        legs = 8;           // ✅
    }
};

int main() {
    Dog d;
    d.setup();
    d.showInfo();

    Spider s;
    s.setup();
    s.showInfo();

    // d.species = "test";  // ❌ 編譯錯誤！外界不能存取 protected
    // d.legs = 100;        // ❌ 編譯錯誤！

    return 0;
}
```

**預期輸出：**
```
犬科, 4 條腿
蛛形綱, 8 條腿
```

**目前先了解 protected 的概念即可**，到第八階段（繼承）時會大量使用。

---

## 三、存取權限總結表

```cpp
class MyClass {
public:
    int a;        // 所有人都能存取
protected:
    int b;        // 自己 + 子類
private:
    int c;        // 只有自己
};
```

| 存取者 | public | protected | private |
|--------|--------|-----------|---------|
| 類別自己的成員函數 | ✅ | ✅ | ✅ |
| 子類的成員函數 | ✅ | ✅ | ❌ |
| 外部程式碼（main 等） | ✅ | ❌ | ❌ |
| friend 函數/類別 | ✅ | ✅ | ✅ |

---

## 四、class vs struct 的預設存取

```cpp
#include <iostream>
using namespace std;

class MyClass {
    int x = 10;   // 預設 private
public:
    int getX() { return x; }
};

struct MyStruct {
    int x = 10;   // 預設 public
};

int main() {
    MyClass c;
    // cout << c.x;    // ❌ 編譯錯誤！x 是 private
    cout << "class: " << c.getX() << endl;

    MyStruct s;
    cout << "struct: " << s.x << endl;  // ✅ x 是 public

    return 0;
}
```

**預期輸出：**
```
class: 10
struct: 10
```

| 特性 | class | struct |
|------|-------|--------|
| 預設存取修飾符 | `private` | `public` |
| 其他功能 | 完全相同 | 完全相同 |

**實務慣例**：
- `struct` — 用於簡單的資料集合，成員通常全部 public（像 C 的 struct）
- `class` — 用於有行為、需要保護資料的對象

---

## 五、存取修飾符的排列風格

一個類別中可以有多個存取修飾符區塊，以下是常見的排列風格：

### 風格一：public 在前（最常見）

```cpp
class Player {
public:
    // 對外介面放最前面 —— 使用者最關心的
    void attack();
    void defend();
    int getHp() const;

protected:
    // 給子類用的
    int baseAttack = 10;

private:
    // 內部實現細節
    string name;
    int hp = 100;
    void calculateDamage();
};
```

**理由**：閱讀類別定義時，最先看到的是「這個類別能做什麼」（public 介面），而不是內部細節。

### 風格二：private 在前

```cpp
class Player {
private:
    string name;
    int hp = 100;

protected:
    int baseAttack = 10;

public:
    void attack();
    void defend();
};
```

有些團隊偏好這種風格，因為先定義資料再定義介面。

**建議**：選擇一種風格後保持一致。本課程採用**風格一（public 在前）**。

---

## 六、實際設計範例：正確的封裝

現在讓我們重新設計開頭的 `BankAccount`，加上正確的存取控制：

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
public:
    // === 對外介面 ===

    // 建立帳戶（目前用普通函數，第 13 課會改用建構函數）
    void init(const string& ownerName, const string& accId, double initialBalance) {
        owner = ownerName;
        accountId = accId;
        if (initialBalance >= 0) {
            balance = initialBalance;
        } else {
            balance = 0;
            cout << "警告：初始餘額不能為負，已設為 0" << endl;
        }
    }

    // 存款
    bool deposit(double amount) {
        if (amount <= 0) {
            cout << "錯誤：存款金額必須大於 0" << endl;
            return false;
        }
        balance += amount;
        addTransaction("存款", amount);
        return true;
    }

    // 提款
    bool withdraw(double amount) {
        if (amount <= 0) {
            cout << "錯誤：提款金額必須大於 0" << endl;
            return false;
        }
        if (amount > balance) {
            cout << "錯誤：餘額不足（目前: $" << balance << "）" << endl;
            return false;
        }
        balance -= amount;
        addTransaction("提款", amount);
        return true;
    }

    // 查詢（只讀操作）
    double getBalance() {
        return balance;
    }

    string getOwner() {
        return owner;
    }

    string getAccountId() {
        return accountId;
    }

    int getTransactionCount() {
        return transactionCount;
    }

    // 顯示帳戶資訊
    void display() {
        cout << "================================" << endl;
        cout << "帳戶持有人: " << owner << endl;
        cout << "帳號:       " << accountId << endl;
        cout << "餘額:       $" << balance << endl;
        cout << "交易次數:   " << transactionCount << endl;
        cout << "================================" << endl;
    }

private:
    // === 內部資料（外界不能直接碰）===
    string owner = "";
    string accountId = "";
    double balance = 0.0;
    int transactionCount = 0;

    // === 內部輔助函數 ===
    void addTransaction(const string& type, double amount) {
        transactionCount++;
        cout << "[交易 #" << transactionCount << "] "
             << type << " $" << amount
             << " → 餘額: $" << balance << endl;
    }
};

int main() {
    BankAccount acc;
    acc.init("陳信安", "ACC-001", 5000.0);
    acc.display();

    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(10000);   // 餘額不足
    acc.deposit(-500);     // 無效金額

    cout << endl;
    acc.display();

    // 安全地查詢
    cout << "\n查詢餘額: $" << acc.getBalance() << endl;

    // 以下操作全部會被編譯器攔截：
    // acc.balance = 999999;         // ❌ private！
    // acc.owner = "";               // ❌ private！
    // acc.transactionCount = 0;     // ❌ private！
    // acc.addTransaction("偷", 1);  // ❌ private 函數！

    return 0;
}
```

**預期輸出：**
```
================================
帳戶持有人: 陳信安
帳號:       ACC-001
餘額:       $5000
交易次數:   0
================================
[交易 #1] 存款 $2000 → 餘額: $7000
[交易 #2] 提款 $1500 → 餘額: $5500
錯誤：餘額不足（目前: $5500）
錯誤：存款金額必須大於 0

================================
帳戶持有人: 陳信安
帳號:       ACC-001
餘額:       $5500
交易次數:   2
================================

查詢餘額: $5500
```

**設計分析**：

```
外界能做的（public）：          外界不能做的（private）：
✅ deposit(金額)               ❌ 直接修改 balance
✅ withdraw(金額)              ❌ 直接修改 owner
✅ getBalance()                ❌ 直接修改 transactionCount
✅ display()                   ❌ 呼叫 addTransaction()
```

這就是**封裝**的力量——透過存取控制，確保資料只能通過合法管道被修改。

---

## 七、同一類別的不同對象可以互相存取 private

這是一個常讓人意外的規則：**private 是針對類別的，不是針對對象的**。同一個類別的不同對象之間，可以互相存取彼此的 private 成員。

```cpp
#include <iostream>
using namespace std;

class Box {
private:
    double width = 0;
    double height = 0;

public:
    void setSize(double w, double h) {
        width = w;
        height = h;
    }

    double area() {
        return width * height;
    }

    // 比較兩個 Box —— 可以直接存取 other 的 private 成員！
    bool isLargerThan(const Box& other) {
        // other.width 和 other.height 是 private，但這裡能存取
        // 因為 isLargerThan 是 Box 類別的成員函數
        return (width * height) > (other.width * other.height);
    }

    void show() {
        cout << width << " x " << height << " = " << area() << endl;
    }
};

int main() {
    Box a, b;
    a.setSize(5, 4);
    b.setSize(3, 8);

    cout << "Box a: "; a.show();
    cout << "Box b: "; b.show();

    if (a.isLargerThan(b)) {
        cout << "a 比 b 大" << endl;
    } else {
        cout << "b 比 a 大（或一樣大）" << endl;
    }

    return 0;
}
```

**預期輸出：**
```
Box a: 5 x 4 = 20
Box b: 3 x 8 = 24
b 比 a 大（或一樣大）
```

**為什麼允許這樣？** 因為 C++ 的存取控制是**類別層級**的，不是**對象層級**的。`isLargerThan` 是 `Box` 類別的成員函數，所以它能存取**任何 `Box` 對象**的 private 成員。

這個設計是有道理的：同一個類別的作者寫了所有成員函數，他知道如何正確使用自己的 private 資料。

---

## 八、多個存取修飾符區塊

一個類別中可以有多個相同修飾符的區塊：

```cpp
#include <iostream>
using namespace std;

class Demo {
public:
    void func1() { cout << "func1" << endl; }

private:
    int data1 = 0;

public:     // 第二個 public 區塊 —— 完全合法
    void func2() { cout << "func2, data1=" << data1 << endl; }

private:    // 第二個 private 區塊
    int data2 = 0;

public:     // 第三個 public 區塊
    void func3() {
        cout << "func3, data1=" << data1
             << ", data2=" << data2 << endl;
    }
};

int main() {
    Demo d;
    d.func1();    // ✅
    d.func2();    // ✅
    d.func3();    // ✅
    // d.data1;   // ❌ private
    // d.data2;   // ❌ private
    return 0;
}
```

**預期輸出：**
```
func1
func2, data1=0
func3, data1=0, data2=0
```

雖然語法上合法，但**不建議**把同一個修飾符分散成多個區塊——這會讓程式碼難以閱讀。最好集中寫：

```cpp
class Good {
public:
    // 所有 public 成員集中在這裡
    void func1();
    void func2();
    void func3();

private:
    // 所有 private 成員集中在這裡
    int data1 = 0;
    int data2 = 0;
};
```

---

## 九、設計原則：盡量 private

一個重要的 OOP 設計原則：

> **成員變數應該盡量設為 private，只透過 public 的成員函數來存取。**

這叫做**最小權限原則（Principle of Least Privilege）**。

```cpp
#include <iostream>
#include <string>
using namespace std;

// ===== 不好的設計 =====
class BadStudent {
public:
    string name;
    int age;
    double gpa;
    // 問題：任何人都能設 age = -5 或 gpa = 999
};

// ===== 好的設計 =====
class GoodStudent {
public:
    // 只提供受控的存取介面
    void setName(const string& n) {
        if (!n.empty()) {
            name = n;
        }
    }

    void setAge(int a) {
        if (a > 0 && a < 150) {
            age = a;
        } else {
            cout << "無效年齡: " << a << endl;
        }
    }

    void setGpa(double g) {
        if (g >= 0.0 && g <= 4.0) {
            gpa = g;
        } else {
            cout << "無效 GPA: " << g << endl;
        }
    }

    string getName() { return name; }
    int getAge() { return age; }
    double getGpa() { return gpa; }

    void show() {
        cout << name << ", " << age << " 歲, GPA: " << gpa << endl;
    }

private:
    string name = "未命名";
    int age = 0;
    double gpa = 0.0;
};

int main() {
    GoodStudent s;
    s.setName("陳信安");
    s.setAge(25);
    s.setGpa(3.8);
    s.show();

    // 嘗試設定不合理的值
    s.setAge(-5);      // 被攔截
    s.setGpa(5.0);     // 被攔截
    s.show();           // 值沒變

    return 0;
}
```

**預期輸出：**
```
陳信安, 25 歲, GPA: 3.8
無效年齡: -5
無效 GPA: 5
陳信安, 25 歲, GPA: 3.8
```

**好處**：
1. **資料驗證**：所有修改都經過檢查
2. **可維護性**：未來想改內部儲存方式（比如 gpa 改用字母等級），外部程式碼不用改
3. **除錯方便**：如果 gpa 出現異常值，只需要在 `setGpa` 加斷點，就能找到誰改的

getter/setter 模式會在第 21 課更深入探討。

---

## 十、何時用哪種修飾符？

```
做決定的流程：

這個成員需要被外界直接使用嗎？
├── 是 → public
│     （函數介面、常數等）
│
└── 否 → 子類需要用嗎？
          ├── 是 → protected
          │     （繼承時的共享資料）
          │
          └── 否 → private
                    （內部實現細節）
```

**實務經驗法則**：
- **成員變數**：幾乎總是 `private`
- **對外函數**（介面）：`public`
- **內部輔助函數**：`private`
- **給子類的資源**：`protected`（謹慎使用，到繼承時再說）

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| `public` | 任何人都能存取 |
| `private` | 只有類別自己的成員函數能存取 |
| `protected` | 自己 + 子類能存取 |
| class 預設 | `private` |
| struct 預設 | `public` |
| 同類別互訪 | 同一類別的不同對象可以互相存取 private |
| 最小權限原則 | 成員變數盡量 private，用 public 函數存取 |
| 排列風格 | 建議 public 在前，private 在後 |

---

下一課是 **第 12 課：struct 與 class 的差異**，我們會做一個完整的比較，並講解在 C++ 中什麼時候該用 struct、什麼時候該用 class。準備好就告訴我！
*/



#include <iostream>
#include <string>
using namespace std;

// ===== 不好的設計 =====
class BadStudent {
public:
    string name;
    int age;
    double gpa;
    // 問題：任何人都能設 age = -5 或 gpa = 999
};

// ===== 好的設計 =====
class GoodStudent {
public:
    // 只提供受控的存取介面
    void setName(const string& n) {
        if (!n.empty()) {
            name = n;
        }
    }

    void setAge(int a) {
        if (a > 0 && a < 150) {
            age = a;
        } else {
            cout << "無效年齡: " << a << endl;
        }
    }

    void setGpa(double g) {
        if (g >= 0.0 && g <= 4.0) {
            gpa = g;
        } else {
            cout << "無效 GPA: " << g << endl;
        }
    }

    string getName() { return name; }
    int getAge() { return age; }
    double getGpa() { return gpa; }

    void show() {
        cout << name << ", " << age << " 歲, GPA: " << gpa << endl;
    }

private:
    string name = "未命名";
    int age = 0;
    double gpa = 0.0;
};

int main() {
    GoodStudent s;
    s.setName("陳信安");
    s.setAge(25);
    s.setGpa(3.8);
    s.show();

    // 嘗試設定不合理的值
    s.setAge(-5);      // 被攔截
    s.setGpa(5.0);     // 被攔截
    s.show();           // 值沒變

    return 0;
}
