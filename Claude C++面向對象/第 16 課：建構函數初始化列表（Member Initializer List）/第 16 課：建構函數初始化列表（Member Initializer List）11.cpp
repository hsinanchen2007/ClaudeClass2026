/*
好的，信安！讓我們進入第三階段最關鍵的一課。

---

# 第 16 課：建構函數初始化列表（Member Initializer List）

---

## 16.1 什麼是初始化列表？

在前面的課程中，我們都是在建構函數的**函數體內**用賦值的方式來設定成員變數：

```cpp
// 函數體內賦值（之前的寫法）
Student(const string& n, int a) {
    name = n;    // 這是「賦值」，不是「初始化」
    age = a;
}
```

初始化列表是另一種語法，它在建構函數體**之前**就完成成員的初始化：

```cpp
// 初始化列表（本課要學的寫法）
Student(const string& n, int a) 
    : name(n), age(a)    // ← 這就是初始化列表
{
    // 函數體（可以為空）
}
```

語法結構是：在建構函數參數列表的右括號 `)` 之後，加一個冒號 `:`，然後列出 `成員(初始值)` 的列表，用逗號分隔。

---

## 16.2 賦值 vs 初始化：本質區別

這個區別非常重要。用你熟悉的 C 語言概念來理解：

```c
// C 語言中
int x = 10;   // 這是初始化：宣告的同時給值
int y;
y = 10;       // 這是賦值：先宣告（此時 y 是垃圾值），再賦值
```

在 C++ 的類別中，同樣的區別存在：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Demo {
private:
    string name;
    int value;

public:
    // 方式 A：函數體內賦值
    // 過程：name 先被預設建構（空字串），然後被賦值為 n
    // 相當於：string name;  然後  name = n;  （兩步）
    Demo(const string& n, int v) {
        cout << "  [賦值方式]" << endl;
        cout << "  賦值前 name = [" << name << "]（已經被預設建構過了）" << endl;
        name = n;     // 賦值
        value = v;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

class DemoInit {
private:
    string name;
    int value;

public:
    // 方式 B：初始化列表
    // 過程：name 直接用 n 來建構，一步到位
    // 相當於：string name(n);  或  string name = n;  （一步）
    DemoInit(const string& n, int v) 
        : name(n), value(v) 
    {
        cout << "  [初始化列表方式]" << endl;
        cout << "  name 直接就是 [" << name << "]（一步完成）" << endl;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

int main() {
    cout << "=== 函數體內賦值 ===" << endl;
    Demo d1("Hello", 42);
    d1.print();
    
    cout << "\n=== 初始化列表 ===" << endl;
    DemoInit d2("Hello", 42);
    d2.print();
    
    return 0;
}
```

### 預期輸出

```
=== 函數體內賦值 ===
  [賦值方式]
  賦值前 name = [](已經被預設建構過了)
  name = Hello, value = 42

=== 初始化列表 ===
  [初始化列表方式]
  name 直接就是 [Hello]（一步完成）
  name = Hello, value = 42
```

### 流程對比圖

```
函數體內賦值（兩步）：
  ┌─────────────┐     ┌─────────────┐
  │ 成員預設建構  │ ──→ │  賦值新的值   │
  │ name = ""   │     │ name = "Hello" │
  └─────────────┘     └─────────────┘
       第一步                第二步

初始化列表（一步）：
  ┌──────────────────┐
  │ 直接用參數建構     │
  │ name("Hello")    │
  └──────────────────┘
         一步到位
```

對於 `int` 這樣的基本型別，差異不大。但對於 `string`、`vector` 等類別型別，初始化列表省去了一次預設建構 + 一次賦值，**直接用拷貝建構完成**，效率更高。

---

## 16.3 必須使用初始化列表的四種情況

有些成員**只能在初始化列表中設定**，在函數體內賦值會**編譯錯誤**：

### 情況 1：const 成員變數

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    const int studentId;  // const 成員：一旦初始化就不能修改
    string name;

public:
    // 錯誤寫法：
    // Student(int id, const string& n) {
    //     studentId = id;  // 編譯錯誤！不能給 const 賦值！
    //     name = n;
    // }
    
    // 正確寫法：使用初始化列表
    Student(int id, const string& n) 
        : studentId(id), name(n) 
    { }

    void print() const {
        cout << "  學號: " << studentId << ", 姓名: " << name << endl;
    }
};

int main() {
    Student s(20250001, "張三");
    s.print();
    
    // s.studentId = 999;  // 編譯錯誤！const 不能修改
    
    return 0;
}
```

### 預期輸出

```
  學號: 20250001, 姓名: 張三
```

**原因**：`const` 變數必須在宣告的時候就初始化，之後不能再修改。函數體執行時，成員變數已經「宣告」過了（通過預設建構），再賦值就是在修改 const，所以編譯錯誤。初始化列表在「宣告」的同時就給值，所以 OK。

---

### 情況 2：引用成員變數

```cpp
#include <iostream>
#include <string>
using namespace std;

class Logger {
private:
    ostream& output;  // 引用成員：必須在初始化時綁定
    string prefix;

public:
    // 引用必須在宣告時綁定，所以必須用初始化列表
    Logger(ostream& os, const string& p) 
        : output(os), prefix(p) 
    { }
    
    void log(const string& message) const {
        output << "[" << prefix << "] " << message << endl;
    }
};

int main() {
    Logger consoleLogger(cout, "INFO");
    consoleLogger.log("程式啟動");
    consoleLogger.log("初始化完成");
    
    return 0;
}
```

### 預期輸出

```
[INFO] 程式啟動
[INFO] 初始化完成
```

**原因**：引用（和 C 語言中的指標不同）一旦綁定就不能改變指向，而且必須在宣告時就綁定。所以和 `const` 一樣，必須用初始化列表。

---

### 情況 3：沒有預設建構函數的成員物件

```cpp
#include <iostream>
#include <string>
using namespace std;

class Engine {
private:
    int horsepower;
    string fuelType;

public:
    // Engine 只有帶參建構函數，沒有預設建構函數
    Engine(int hp, const string& fuel) 
        : horsepower(hp), fuelType(fuel) 
    {
        cout << "  引擎建構: " << hp << " 馬力, " << fuel << endl;
    }
    
    void print() const {
        cout << "  引擎: " << horsepower << " HP (" << fuelType << ")" << endl;
    }
};

class Car {
private:
    string brand;
    Engine engine;   // Engine 沒有預設建構函數！

public:
    // 錯誤寫法：
    // Car(const string& b, int hp, const string& fuel) {
    //     brand = b;
    //     engine = Engine(hp, fuel);  // 編譯錯誤！engine 無法預設建構！
    // }
    
    // 正確寫法：在初始化列表中建構 engine
    Car(const string& b, int hp, const string& fuel) 
        : brand(b), engine(hp, fuel)  // 直接把參數傳給 Engine 的建構函數
    {
        cout << "  汽車建構: " << brand << endl;
    }
    
    void print() const {
        cout << "  品牌: " << brand << endl;
        engine.print();
    }
};

int main() {
    Car car("BMW", 300, "汽油");
    cout << endl;
    car.print();
    
    return 0;
}
```

### 預期輸出

```
  引擎建構: 300 馬力, 汽油
  汽車建構: BMW

  品牌: BMW
  引擎: 300 HP (汽油)
```

**注意建構順序**：初始化列表中 `engine(hp, fuel)` 先執行（成員初始化），然後才進入 `Car` 建構函數的函數體。

---

### 情況 4：基類的建構函數（繼承時使用，後面的課會詳講）

```cpp
#include <iostream>
#include <string>
using namespace std;

class Animal {
private:
    string species;

public:
    Animal(const string& s) : species(s) {
        cout << "  Animal 建構: " << species << endl;
    }
    
    string getSpecies() const { return species; }
};

class Dog : public Animal {
private:
    string name;

public:
    // 用初始化列表調用基類建構函數
    Dog(const string& n) 
        : Animal("犬科"),   // 調用基類建構函數
          name(n)            // 初始化自己的成員
    {
        cout << "  Dog 建構: " << name << endl;
    }
    
    void print() const {
        cout << "  " << name << " (" << getSpecies() << ")" << endl;
    }
};

int main() {
    Dog d("旺財");
    d.print();
    return 0;
}
```

### 預期輸出

```
  Animal 建構: 犬科
  Dog 建構: 旺財
  旺財 (犬科)
```

---

## 16.4 四種必須情況的總結

| 情況 | 原因 | 範例 |
|------|------|------|
| `const` 成員 | const 只能初始化，不能賦值 | `const int id;` |
| 引用成員 | 引用必須在宣告時綁定 | `ostream& out;` |
| 無預設建構函數的成員物件 | 無法先預設建構再賦值 | `Engine engine;` |
| 基類建構函數 | 基類必須在派生類之前建構 | `: Animal("犬科")` |

---

## 16.5 初始化順序：一個重要的陷阱

初始化列表中成員的初始化順序**不是由列表中的書寫順序決定的**，而是由**成員在類別中宣告的順序**決定的：

```cpp
#include <iostream>
using namespace std;

class OrderDemo {
private:
    int a;    // 第 1 個宣告
    int b;    // 第 2 個宣告
    int c;    // 第 3 個宣告

public:
    // 注意：初始化列表故意寫成 c, a, b 的順序
    // 但實際初始化順序是 a, b, c（按宣告順序）
    OrderDemo(int val) 
        : c(val + 2),     // 寫在第 1 個，但第 3 個執行
          a(val),          // 寫在第 2 個，但第 1 個執行
          b(val + 1)       // 寫在第 3 個，但第 2 個執行
    {
        cout << "a = " << a << ", b = " << b << ", c = " << c << endl;
    }
};

int main() {
    OrderDemo d(10);
    return 0;
}
```

### 預期輸出

```
a = 10, b = 11, c = 12
```

在這個例子中沒有問題。但如果成員之間有依賴關係，就會出大問題：

```cpp
#include <iostream>
using namespace std;

class DangerousOrder {
private:
    int length;    // 第 1 個宣告
    int* data;     // 第 2 個宣告

public:
    // 危險！看起來 data 先被初始化，但實際上 length 先執行
    DangerousOrder(int len) 
        : data(new int[length]),  // 用 length 來分配，但 length 還沒初始化！
          length(len)              // length 在 data 之前宣告，所以先初始化
    {
        // 此時 data 分配了垃圾大小的記憶體！
    }
    
    ~DangerousOrder() { delete[] data; }
};

class SafeOrder {
private:
    int length;    // 第 1 個宣告
    int* data;     // 第 2 個宣告

public:
    // 安全：初始化列表的順序和宣告順序一致
    SafeOrder(int len) 
        : length(len),            // 先初始化 length
          data(new int[length])   // 再用 length 分配
    {
        cout << "安全分配了 " << length << " 個元素" << endl;
    }
    
    ~SafeOrder() { delete[] data; }
};

int main() {
    // DangerousOrder d(10);  // 未定義行為！不要這樣做
    SafeOrder s(10);          // OK
    return 0;
}
```

### 預期輸出

```
安全分配了 10 個元素
```

### 最佳實踐

> **初始化列表中的書寫順序，永遠保持和成員宣告順序一致。**

大多數編譯器在啟用警告時會幫你檢測這個問題：

```bash
g++ -Wall -Wextra -std=c++17 -o test test.cpp
# 會看到警告：'DangerousOrder::data' will be initialized after 'int DangerousOrder::length'
```

---

## 16.6 初始化列表中的表達式

初始化列表不僅可以直接傳參數，還可以使用任意表達式：

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

class Circle {
private:
    double radius;
    double area;
    double circumference;

public:
    // 在初始化列表中使用表達式計算
    Circle(double r) 
        : radius(r),
          area(M_PI * r * r),              // 用公式計算
          circumference(2.0 * M_PI * r)     // 用公式計算
    { }

    void print() const {
        cout << "  半徑: " << radius << endl;
        cout << "  面積: " << area << endl;
        cout << "  周長: " << circumference << endl;
    }
};

class FullName {
private:
    string firstName;
    string lastName;
    string fullName;

public:
    // 初始化列表中做字串串接
    FullName(const string& first, const string& last) 
        : firstName(first),
          lastName(last),
          fullName(last + " " + first)    // 串接
    { }

    void print() const {
        cout << "  姓: " << lastName << endl;
        cout << "  名: " << firstName << endl;
        cout << "  全名: " << fullName << endl;
    }
};

int main() {
    cout << "=== Circle ===" << endl;
    Circle c(5.0);
    c.print();
    
    cout << "\n=== FullName ===" << endl;
    FullName fn("信安", "陳");
    fn.print();
    
    return 0;
}
```

### 預期輸出

```
=== Circle ===
  半徑: 5
  面積: 78.5398
  周長: 31.4159

=== FullName ===
  姓: 陳
  名: 信安
  全名: 陳 信安
```

---

## 16.7 初始化列表配合預設值（C++11 類別內初始化）

C++11 引入了**類別內成員初始化（In-class Member Initializer）**，可以在宣告成員時直接給預設值：

```cpp
#include <iostream>
#include <string>
using namespace std;

class GameConfig {
private:
    // C++11：直接在宣告時給預設值
    int screenWidth = 1280;
    int screenHeight = 720;
    bool fullscreen = false;
    string title = "My Game";
    int fps = 60;

public:
    // 預設建構函數：所有成員使用類別內的預設值
    GameConfig() {
        cout << "[預設建構] 使用所有預設值" << endl;
    }
    
    // 部分自定義：初始化列表會覆蓋類別內預設值
    GameConfig(int w, int h) 
        : screenWidth(w), screenHeight(h)  // 只覆蓋寬高
        // fullscreen、title、fps 使用類別內的預設值
    {
        cout << "[部分自定義] 自定義解析度" << endl;
    }
    
    // 完全自定義
    GameConfig(int w, int h, bool fs, const string& t, int f) 
        : screenWidth(w), screenHeight(h), fullscreen(fs), 
          title(t), fps(f) 
    {
        cout << "[完全自定義] 所有參數指定" << endl;
    }

    void print() const {
        cout << "  遊戲: " << title << endl;
        cout << "  解析度: " << screenWidth << "x" << screenHeight << endl;
        cout << "  全螢幕: " << (fullscreen ? "是" : "否") << endl;
        cout << "  FPS: " << fps << endl;
    }
};

int main() {
    cout << "=== 配置 1：全部預設 ===" << endl;
    GameConfig cfg1;
    cfg1.print();
    
    cout << "\n=== 配置 2：只改解析度 ===" << endl;
    GameConfig cfg2(1920, 1080);
    cfg2.print();
    
    cout << "\n=== 配置 3：全部自定義 ===" << endl;
    GameConfig cfg3(2560, 1440, true, "RPG World", 144);
    cfg3.print();
    
    return 0;
}
```

### 預期輸出

```
=== 配置 1：全部預設 ===
[預設建構] 使用所有預設值
  遊戲: My Game
  解析度: 1280x720
  全螢幕: 否
  FPS: 60

=== 配置 2：只改解析度 ===
[部分自定義] 自定義解析度
  遊戲: My Game
  解析度: 1920x1080
  全螢幕: 否
  FPS: 60

=== 配置 3：全部自定義 ===
[完全自定義] 所有參數指定
  遊戲: RPG World
  解析度: 2560x1440
  全螢幕: 是
  FPS: 144
```

### 優先順序

```
初始化列表中有指定  →  使用初始化列表的值
初始化列表中沒有    →  使用類別內預設值
類別內也沒有預設值  →  基本型別不初始化（垃圾值），類別型別調用預設建構函數
```

這個特性非常實用——你可以在類別內設定「合理的預設值」，然後在各個建構函數中只覆蓋需要改變的部分，大幅減少重複程式碼。

---

## 16.8 效能對比：初始化列表 vs 函數體賦值

```cpp
#include <iostream>
#include <string>
using namespace std;

class HeavyObject {
private:
    string data;

public:
    HeavyObject() {
        data = "";
        cout << "  HeavyObject 預設建構" << endl;
    }
    
    HeavyObject(const string& s) {
        data = s;
        cout << "  HeavyObject 帶參建構: " << s << endl;
    }
    
    HeavyObject& operator=(const string& s) {
        data = s;
        cout << "  HeavyObject 賦值: " << s << endl;
        return *this;
    }
};

// 使用函數體賦值
class ContainerA {
private:
    HeavyObject obj;

public:
    ContainerA(const string& s) {
        // 步驟：先預設建構 obj，再賦值
        cout << "  --- 開始賦值 ---" << endl;
        obj = s;
        cout << "  --- 賦值完成 ---" << endl;
    }
};

// 使用初始化列表
class ContainerB {
private:
    HeavyObject obj;

public:
    ContainerB(const string& s) 
        : obj(s)  // 直接帶參建構，不經過預設建構
    {
        cout << "  --- 初始化列表完成 ---" << endl;
    }
};

int main() {
    cout << "=== 方式 A：函數體賦值（兩步）===" << endl;
    ContainerA a("Hello");
    
    cout << "\n=== 方式 B：初始化列表（一步）===" << endl;
    ContainerB b("Hello");
    
    return 0;
}
```

### 預期輸出

```
=== 方式 A：函數體賦值（兩步）===
  HeavyObject 預設建構
  --- 開始賦值 ---
  HeavyObject 賦值: Hello
  --- 賦值完成 ---

=== 方式 B：初始化列表（一步）===
  HeavyObject 帶參建構: Hello
  --- 初始化列表完成 ---
```

差異一目了然：

| 方式 | 操作次數 | 調用的函數 |
|------|----------|-----------|
| 函數體賦值 | **2 次** | 預設建構 + 賦值運算子 |
| 初始化列表 | **1 次** | 帶參建構 |

當成員是大型物件（如包含大量資料的容器），這個效能差異會非常明顯。

---

## 16.9 完整綜合範例

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// ============================================================
// 範例：RPG 武器系統
// 展示初始化列表的所有技巧
// ============================================================

// 武器稀有度（沒有預設建構函數 → 必須用初始化列表）
class Rarity {
private:
    string name;
    int stars;

public:
    Rarity(const string& n, int s) : name(n), stars(s) { }
    
    string getName() const { return name; }
    int getStars() const { return stars; }
    
    void print() const {
        cout << name << " (";
        for (int i = 0; i < stars; i++) cout << "*";
        cout << ")";
    }
};

// 武器類別
class Weapon {
private:
    const int weaponId;           // const 成員 → 必須用初始化列表
    string name;
    Rarity rarity;                // 無預設建構函數 → 必須用初始化列表
    double baseDamage;
    double critRate;
    
    // C++11 類別內預設值
    int enhanceLevel = 0;         // 強化等級，預設 0
    bool isEquipped = false;      // 是否裝備，預設否

public:
    // 建構函數：展示各種初始化方式的結合
    Weapon(int id, const string& n, const string& rarityName, 
           int rarityStars, double dmg, double crit = 0.05)
        : weaponId(id),                              // const 成員
          name(n),                                    // 一般成員
          rarity(rarityName, rarityStars),            // 無預設建構的成員物件
          baseDamage(dmg),                            // 一般成員
          critRate(crit)                              // 帶預設參數
          // enhanceLevel 和 isEquipped 使用類別內預設值
    {
        cout << "  鍛造武器: " << name << " [ID:" << weaponId << "]" << endl;
    }
    
    // 強化武器
    void enhance() {
        if (enhanceLevel < 15) {
            enhanceLevel++;
            baseDamage *= 1.1;  // 每次強化增加 10% 傷害
            cout << "  " << name << " 強化至 +" << enhanceLevel 
                 << "（傷害: " << baseDamage << "）" << endl;
        } else {
            cout << "  " << name << " 已達最高強化等級！" << endl;
        }
    }
    
    void equip() { isEquipped = true; }
    void unequip() { isEquipped = false; }

    void print() const {
        cout << "  ┌────────────────────────────────┐" << endl;
        cout << "  │ [" << weaponId << "] " << name << endl;
        cout << "  │ 稀有度: ";
        rarity.print();
        cout << endl;
        cout << "  │ 傷害: " << baseDamage 
             << "  暴擊率: " << (critRate * 100) << "%" << endl;
        cout << "  │ 強化: +" << enhanceLevel 
             << "  " << (isEquipped ? "[已裝備]" : "[未裝備]") << endl;
        cout << "  └────────────────────────────────┘" << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 16 課：初始化列表 綜合範例" << endl;
    cout << "============================================" << endl;
    
    cout << "\n--- 鍛造武器 ---" << endl;
    Weapon sword(1001, "屠龍劍", "傳說", 5, 150.0, 0.15);
    Weapon bow(1002, "風之弓", "史詩", 4, 95.0, 0.25);
    Weapon staff(1003, "智慧之杖", "精良", 3, 120.0);  // 暴擊率用預設值
    
    cout << "\n--- 武器資訊 ---" << endl;
    sword.print();
    bow.print();
    staff.print();
    
    cout << "\n--- 強化武器 ---" << endl;
    sword.enhance();
    sword.enhance();
    sword.enhance();
    
    cout << "\n--- 裝備武器 ---" << endl;
    sword.equip();
    
    cout << "\n--- 最終狀態 ---" << endl;
    sword.print();
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson16 lesson16.cpp
./lesson16
```

### 預期輸出

```
============================================
   第 16 課：初始化列表 綜合範例
============================================

--- 鍛造武器 ---
  鍛造武器: 屠龍劍 [ID:1001]
  鍛造武器: 風之弓 [ID:1002]
  鍛造武器: 智慧之杖 [ID:1003]

--- 武器資訊 ---
  ┌────────────────────────────────┐
  │ [1001] 屠龍劍
  │ 稀有度: 傳說 (*****)
  │ 傷害: 150  暴擊率: 15%
  │ 強化: +0  [未裝備]
  └────────────────────────────────┘
  ┌────────────────────────────────┐
  │ [1002] 風之弓
  │ 稀有度: 史詩 (****)
  │ 傷害: 95  暴擊率: 25%
  │ 強化: +0  [未裝備]
  └────────────────────────────────┘
  ┌────────────────────────────────┐
  │ [1003] 智慧之杖
  │ 稀有度: 精良 (***)
  │ 傷害: 120  暴擊率: 5%
  │ 強化: +0  [未裝備]
  └────────────────────────────────┘

--- 強化武器 ---
  屠龍劍 強化至 +1（傷害: 165）
  屠龍劍 強化至 +2（傷害: 181.5）
  屠龍劍 強化至 +3（傷害: 199.65）

--- 裝備武器 ---

--- 最終狀態 ---
  ┌────────────────────────────────┐
  │ [1001] 屠龍劍
  │ 稀有度: 傳說 (*****)
  │ 傷害: 199.65  暴擊率: 15%
  │ 強化: +3  [已裝備]
  └────────────────────────────────┘
```

---

## 16.10 本課重點回顧

| 概念 | 說明 |
|------|------|
| 初始化列表語法 | `: member1(value1), member2(value2)` 寫在參數列表後、函數體前 |
| 初始化 vs 賦值 | 初始化列表是「初始化」（一步），函數體是「賦值」（兩步） |
| **必須使用**的情況 | const 成員、引用成員、無預設建構的成員物件、基類建構 |
| 初始化順序 | 按**成員宣告順序**，不是列表書寫順序 |
| 類別內預設值 | C++11 可在宣告時給預設值，初始化列表可覆蓋 |
| 效能優勢 | 省去一次預設建構 + 賦值，直接一步建構 |
| 最佳實踐 | **永遠使用初始化列表**，保持書寫順序和宣告順序一致 |

---

## 16.11 下一課預告

下一課我們將學習 **解構函數（Destructor）**——它是建構函數的「鏡像」，在對象被銷毀時自動調用，負責清理資源（釋放記憶體、關閉檔案、斷開連接等）。對於寫過 C 語言、手動管理記憶體的你來說，這個概念會非常親切。

準備好進入 **第 17 課：解構函數（Destructor）** 了嗎？
*/



#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// ============================================================
// 範例：RPG 武器系統
// 展示初始化列表的所有技巧
// ============================================================

// 武器稀有度（沒有預設建構函數 → 必須用初始化列表）
class Rarity {
private:
    string name;
    int stars;

public:
    Rarity(const string& n, int s) : name(n), stars(s) { }
    
    string getName() const { return name; }
    int getStars() const { return stars; }
    
    void print() const {
        cout << name << " (";
        for (int i = 0; i < stars; i++) cout << "*";
        cout << ")";
    }
};

// 武器類別
class Weapon {
private:
    const int weaponId;           // const 成員 → 必須用初始化列表
    string name;
    Rarity rarity;                // 無預設建構函數 → 必須用初始化列表
    double baseDamage;
    double critRate;
    
    // C++11 類別內預設值
    int enhanceLevel = 0;         // 強化等級，預設 0
    bool isEquipped = false;      // 是否裝備，預設否

public:
    // 建構函數：展示各種初始化方式的結合
    Weapon(int id, const string& n, const string& rarityName, 
           int rarityStars, double dmg, double crit = 0.05)
        : weaponId(id),                              // const 成員
          name(n),                                    // 一般成員
          rarity(rarityName, rarityStars),            // 無預設建構的成員物件
          baseDamage(dmg),                            // 一般成員
          critRate(crit)                              // 帶預設參數
          // enhanceLevel 和 isEquipped 使用類別內預設值
    {
        cout << "  鍛造武器: " << name << " [ID:" << weaponId << "]" << endl;
    }
    
    // 強化武器
    void enhance() {
        if (enhanceLevel < 15) {
            enhanceLevel++;
            baseDamage *= 1.1;  // 每次強化增加 10% 傷害
            cout << "  " << name << " 強化至 +" << enhanceLevel 
                 << "（傷害: " << baseDamage << "）" << endl;
        } else {
            cout << "  " << name << " 已達最高強化等級！" << endl;
        }
    }
    
    void equip() { isEquipped = true; }
    void unequip() { isEquipped = false; }

    void print() const {
        cout << "  ┌────────────────────────────────┐" << endl;
        cout << "  │ [" << weaponId << "] " << name << endl;
        cout << "  │ 稀有度: ";
        rarity.print();
        cout << endl;
        cout << "  │ 傷害: " << baseDamage 
             << "  暴擊率: " << (critRate * 100) << "%" << endl;
        cout << "  │ 強化: +" << enhanceLevel 
             << "  " << (isEquipped ? "[已裝備]" : "[未裝備]") << endl;
        cout << "  └────────────────────────────────┘" << endl;
    }
};

int main() {
    cout << "============================================" << endl;
    cout << "   第 16 課：初始化列表 綜合範例" << endl;
    cout << "============================================" << endl;
    
    cout << "\n--- 鍛造武器 ---" << endl;
    Weapon sword(1001, "屠龍劍", "傳說", 5, 150.0, 0.15);
    Weapon bow(1002, "風之弓", "史詩", 4, 95.0, 0.25);
    Weapon staff(1003, "智慧之杖", "精良", 3, 120.0);  // 暴擊率用預設值
    
    cout << "\n--- 武器資訊 ---" << endl;
    sword.print();
    bow.print();
    staff.print();
    
    cout << "\n--- 強化武器 ---" << endl;
    sword.enhance();
    sword.enhance();
    sword.enhance();
    
    cout << "\n--- 裝備武器 ---" << endl;
    sword.equip();
    
    cout << "\n--- 最終狀態 ---" << endl;
    sword.print();
    
    return 0;
}
