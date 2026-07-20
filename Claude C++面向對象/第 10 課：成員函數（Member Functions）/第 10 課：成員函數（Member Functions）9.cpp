/*
# 第 10 課：成員函數（Member Functions）

## 一、回顧與深入

前面幾課我們已經用過成員函數了，但都是基本用法。這一課要深入探討成員函數的各種特性和技巧。

成員函數 = 定義在類別內部的函數 = 描述對象「能做什麼」的行為。

它和普通函數最大的區別：**成員函數天生能存取同一個對象的所有成員變數和其他成員函數。**

---

## 二、成員函數的完整定義方式

### 2.1 回顧：類內定義 vs 類外定義

```cpp
#include <iostream>
using namespace std;

class Rectangle {
public:
    double width = 0;
    double height = 0;

    // 類內定義（隱式 inline）
    double area() {
        return width * height;
    }

    // 類內宣告，類外定義
    double perimeter();
    void scale(double factor);
    void print();
};

// 類外定義
double Rectangle::perimeter() {
    return 2 * (width + height);
}

void Rectangle::scale(double factor) {
    width *= factor;
    height *= factor;
}

void Rectangle::print() {
    cout << "Rectangle(" << width << " x " << height << ")" << endl;
    cout << "  面積: " << area() << endl;            // 成員函數調用另一個成員函數
    cout << "  周長: " << perimeter() << endl;       // 同上
}

int main() {
    Rectangle r;
    r.width = 5.0;
    r.height = 3.0;

    r.print();

    r.scale(2.0);
    cout << "\n放大 2 倍後:" << endl;
    r.print();

    return 0;
}
```

**預期輸出：**
```
Rectangle(5 x 3)
  面積: 15
  周長: 16
  
放大 2 倍後:
Rectangle(10 x 6)
  面積: 60
  周長: 32
```

**注意**：在 `print()` 中直接調用了 `area()` 和 `perimeter()`，不需要寫 `this->area()` 或任何前綴。成員函數之間可以直接互相調用。

---

### 2.2 類內定義的隱式 inline

當你把函數體直接寫在 class 定義裡面時，編譯器會將它視為**隱式 inline**：

```cpp
class Foo {
public:
    // 隱式 inline —— 編譯器「可能」在呼叫處展開，避免函數調用開銷
    int getValue() {
        return value;
    }

private:
    int value = 42;
};
```

這不是說它一定會被 inline，只是告訴編譯器「這個函數很短，你可以考慮 inline」。這是 C++ 的效能優化機制之一。

類外定義的函數預設不是 inline，但你可以手動加上 `inline` 關鍵字：

```cpp
class Foo {
public:
    int getValue();
private:
    int value = 42;
};

inline int Foo::getValue() {   // 手動 inline
    return value;
}
```

**實際開發中**：短函數（1~3 行）寫在類內，長函數寫在類外。不需要特別擔心 inline，現代編譯器會自己判斷。

---

## 三、成員函數的參數與返回值

成員函數和普通函數一樣，可以有各種參數和返回值：

```cpp
#include <iostream>
#include <string>
using namespace std;

class StringHelper {
public:
    string text = "";

    // 無參數、無返回值
    void clear() {
        text = "";
    }

    // 有參數、無返回值
    void append(const string& suffix) {
        text += suffix;
    }

    // 無參數、有返回值
    int length() {
        return text.length();
    }

    // 有參數、有返回值
    char charAt(int index) {
        if (index < 0 || index >= (int)text.length()) {
            cout << "錯誤：索引超出範圍" << endl;
            return '\0';
        }
        return text[index];
    }

    // 多個參數
    string substring(int start, int len) {
        if (start < 0 || start >= (int)text.length()) {
            return "";
        }
        return text.substr(start, len);
    }

    // 返回 bool
    bool contains(const string& target) {
        return text.find(target) != string::npos;
    }

    // 返回自身的引用（鏈式調用的基礎）
    StringHelper& appendChain(const string& suffix) {
        text += suffix;
        return *this;  // 返回自己（this 指標會在第 26 課詳講）
    }
};

int main() {
    StringHelper sh;

    sh.append("Hello");
    sh.append(", ");
    sh.append("World!");

    cout << "文字: " << sh.text << endl;
    cout << "長度: " << sh.length() << endl;
    cout << "第 0 個字元: " << sh.charAt(0) << endl;
    cout << "子字串(7, 5): " << sh.substring(7, 5) << endl;
    cout << "包含 \"World\": " << (sh.contains("World") ? "是" : "否") << endl;

    cout << "\n--- 鏈式調用 ---" << endl;
    StringHelper sh2;
    sh2.appendChain("C++").appendChain(" is").appendChain(" awesome!");
    cout << sh2.text << endl;

    return 0;
}
```

**預期輸出：**
```
文字: Hello, World!
長度: 13
第 0 個字元: H
子字串(7, 5): World
包含 "World": 是

--- 鏈式調用 ---
C++ is awesome!
```

**鏈式調用**的原理：`appendChain` 返回 `*this`（自己的引用），所以 `sh2.appendChain("C++")` 返回的還是 `sh2`，接著就能繼續 `.appendChain(" is")`。這個模式在很多 C++ 程式庫中很常見。`*this` 的細節我們在第 26 課會完整解說。

---

## 四、成員函數重載（Overloading）

和普通函數一樣，成員函數也可以**重載**——同一個函數名，不同的參數列表：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Printer {
public:
    // 同名函數，不同參數 —— 這就是重載
    void print(int value) {
        cout << "整數: " << value << endl;
    }

    void print(double value) {
        cout << "浮點數: " << value << endl;
    }

    void print(const string& value) {
        cout << "字串: " << value << endl;
    }

    void print(int value, int width) {
        cout << "整數(寬度 " << width << "): ";
        // 簡單的右對齊
        string s = to_string(value);
        for (int i = 0; i < width - (int)s.length(); i++) {
            cout << " ";
        }
        cout << s << endl;
    }
};

int main() {
    Printer p;

    p.print(42);              // 呼叫 print(int)
    p.print(3.14);            // 呼叫 print(double)
    p.print("Hello");         // 呼叫 print(const string&)
    p.print(42, 10);          // 呼叫 print(int, int)

    return 0;
}
```

**預期輸出：**
```
整數: 42
浮點數: 3.14
字串: Hello
整數(寬度 10):         42
```

**重載的規則**：

```cpp
class Example {
public:
    void foo(int x);           // ✅ OK
    void foo(double x);        // ✅ OK —— 參數型別不同
    void foo(int x, int y);    // ✅ OK —— 參數數量不同
    // int foo(int x);         // ❌ 錯誤！只有返回值不同，不算重載
};
```

編譯器根據你呼叫時傳入的**參數型別和數量**來決定呼叫哪個版本，這叫做**靜態多型（compile-time polymorphism）**。

---

## 五、帶預設參數的成員函數

```cpp
#include <iostream>
#include <string>
using namespace std;

class Logger {
public:
    string name = "APP";

    // level 有預設值 "INFO"
    void log(const string& message, const string& level = "INFO") {
        cout << "[" << level << "] " << name << ": " << message << endl;
    }

    // repeat 有預設值 1
    void divider(char ch = '-', int repeat = 40) {
        for (int i = 0; i < repeat; i++) {
            cout << ch;
        }
        cout << endl;
    }
};

int main() {
    Logger logger;
    logger.name = "MyApp";

    logger.divider();                  // 使用全部預設值
    logger.log("程式啟動");            // level 使用預設值 "INFO"
    logger.log("連線失敗", "ERROR");   // 指定 level
    logger.log("嘗試重連", "WARN");
    logger.divider('=', 40);           // 指定 ch，使用預設 repeat
    logger.log("重連成功");
    logger.divider('*', 20);           // 全部指定

    return 0;
}
```

**預期輸出：**
```
----------------------------------------
[INFO] MyApp: 程式啟動
[ERROR] MyApp: 連線失敗
[WARN] MyApp: 嘗試重連
========================================
[INFO] MyApp: 重連成功
********************
```

**預設參數的規則**：
- 預設值必須從**右邊**開始，不能跳過

```cpp
void foo(int a, int b = 10, int c = 20);   // ✅ 正確
// void foo(int a = 5, int b, int c = 20); // ❌ 錯誤！b 沒有預設值但在有預設值的 a 後面
```

- 呼叫時，省略的參數也必須從**右邊**開始

```cpp
foo(1);         // a=1, b=10, c=20
foo(1, 2);      // a=1, b=2,  c=20
foo(1, 2, 3);   // a=1, b=2,  c=3
```

---

## 六、成員函數調用成員函數

成員函數之間可以自由互相調用：

```cpp
#include <iostream>
#include <string>
using namespace std;

class TemperatureConverter {
public:
    double celsius = 0.0;

    // 基礎轉換函數
    double toFahrenheit() {
        return celsius * 9.0 / 5.0 + 32.0;
    }

    double toKelvin() {
        return celsius + 273.15;
    }

    // 判斷函數 —— 調用其他成員函數
    bool isBoiling() {
        return celsius >= 100.0;
    }

    bool isFreezing() {
        return celsius <= 0.0;
    }

    string getState() {
        if (isFreezing()) return "固態（冰）";    // 調用自己的成員函數
        if (isBoiling()) return "氣態（水蒸氣）";
        return "液態（水）";
    }

    // 綜合報告 —— 調用多個成員函數
    void report() {
        cout << "=========================" << endl;
        cout << "攝氏:   " << celsius << " °C" << endl;
        cout << "華氏:   " << toFahrenheit() << " °F" << endl;
        cout << "克式:   " << toKelvin() << " K" << endl;
        cout << "水的狀態: " << getState() << endl;
        cout << "=========================" << endl;
    }
};

int main() {
    TemperatureConverter t;

    t.celsius = -10;
    t.report();

    cout << endl;
    t.celsius = 25;
    t.report();

    cout << endl;
    t.celsius = 100;
    t.report();

    return 0;
}
```

**預期輸出：**
```
=========================
攝氏:   -10 °C
華氏:   14 °F
克式:   263.15 K
水的狀態: 固態（冰）
=========================

=========================
攝氏:   25 °C
華氏:   77 °F
克式:   298.15 K
水的狀態: 液態（水）
=========================

=========================
攝氏:   100 °C
華氏:   212 °F
克式:   373.15 K
水的狀態: 氣態（水蒸氣）
=========================
```

**呼叫鏈**：`report()` → `toFahrenheit()`, `toKelvin()`, `getState()` → `isFreezing()`, `isBoiling()`。這展示了成員函數之間的層次化組織。

---

## 七、成員函數背後的秘密：隱藏的 this

你有沒有想過，當 `d1.bark()` 和 `d2.bark()` 都調用同一個 `bark()` 函數時，函數怎麼知道要用 `d1` 的 `name` 還是 `d2` 的 `name`？

答案是：**編譯器在背後偷偷傳入了一個 `this` 指標。**

```cpp
#include <iostream>
#include <string>
using namespace std;

class Dog {
public:
    string name;

    void bark() {
        // 你寫的：
        cout << name << " 汪汪！" << endl;

        // 編譯器實際看到的（等價的寫法）：
        // cout << this->name << " 汪汪！" << endl;
    }
};

int main() {
    Dog d1;
    d1.name = "旺財";

    Dog d2;
    d2.name = "小黑";

    d1.bark();   // 編譯器轉換為：Dog::bark(&d1)  → this = &d1
    d2.bark();   // 編譯器轉換為：Dog::bark(&d2)  → this = &d2

    return 0;
}
```

**預期輸出：**
```
旺財 汪汪！
小黑 汪汪！
```

**概念圖解**：

```
你寫的程式碼:             編譯器在背後做的事:
                         
d1.bark();        →      Dog::bark(&d1)
                              ↓
                         void bark(Dog* this) {
                             cout << this->name << " 汪汪！";
                         }
                         this 指向 d1 → name 是 "旺財"

d2.bark();        →      Dog::bark(&d2)
                              ↓
                         void bark(Dog* this) {
                             cout << this->name << " 汪汪！";
                         }
                         this 指向 d2 → name 是 "小黑"
```

**重點**：
- 每個成員函數都有一個隱藏的 `this` 參數
- `this` 是指向「調用這個函數的對象」的指標
- 你寫 `name` 等同於 `this->name`
- 第 26 課會完整講解 `this` 的所有用法

---

## 八、驗證 this 的存在

我們可以直接印出 `this` 來驗證：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Widget {
public:
    int id = 0;

    void showAddress() {
        cout << "對象 id=" << id
             << " 的地址: " << this << endl;
    }
};

int main() {
    Widget a, b, c;
    a.id = 1;
    b.id = 2;
    c.id = 3;

    a.showAddress();   // this == &a
    b.showAddress();   // this == &b
    c.showAddress();   // this == &c

    // 驗證：直接取地址比較
    cout << "\n直接取地址驗證:" << endl;
    cout << "&a = " << &a << endl;
    cout << "&b = " << &b << endl;
    cout << "&c = " << &c << endl;

    return 0;
}
```

**可能的輸出（地址每次不同，但配對會一致）：**
```
對象 id=1 的地址: 0x7ffd5a3b4c90
對象 id=2 的地址: 0x7ffd5a3b4c94
對象 id=3 的地址: 0x7ffd5a3b4c98

直接取地址驗證:
&a = 0x7ffd5a3b4c90
&b = 0x7ffd5a3b4c94
&c = 0x7ffd5a3b4c98
```

成員函數中的 `this` 和外部的 `&a`、`&b`、`&c` 完全一致，證實了 `this` 就是指向調用者的指標。

---

## 九、成員函數 vs 普通函數的完整對比

```cpp
#include <iostream>
using namespace std;

class Circle {
public:
    double radius = 0;

    // 成員函數：自動知道 radius
    double area() {
        return 3.14159 * radius * radius;
    }
};

// 普通函數：必須手動傳入 radius
double circleArea(double radius) {
    return 3.14159 * radius * radius;
}

int main() {
    Circle c;
    c.radius = 5.0;

    // 成員函數風格
    cout << "成員函數: " << c.area() << endl;

    // 普通函數風格
    cout << "普通函數: " << circleArea(c.radius) << endl;

    return 0;
}
```

**預期輸出：**
```
成員函數: 78.5398
普通函數: 78.5398
```

| 比較 | 成員函數 | 普通函數 |
|------|---------|---------|
| 定義位置 | class 內部或用 `類別::` 在外部 | 任何地方 |
| 存取成員變數 | 直接存取（透過隱藏的 this） | 必須透過參數傳入 |
| 呼叫方式 | `對象.函數()` | `函數(參數)` |
| 歸屬 | 屬於某個類別 | 不屬於任何類別 |
| 存取 private | 可以 | 不可以（除非是 friend） |

---

## 十、綜合實戰：簡易計算器類別

```cpp
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

class Calculator {
public:
    double result = 0.0;
    string history = "";
    int operationCount = 0;

    // --- 基本運算 ---
    void add(double value) {
        record("+", value);
        result += value;
    }

    void subtract(double value) {
        record("-", value);
        result -= value;
    }

    void multiply(double value) {
        record("*", value);
        result *= value;
    }

    // 除法需要檢查除以零
    bool divide(double value) {
        if (value == 0.0) {
            cout << "錯誤：不能除以零！" << endl;
            return false;
        }
        record("/", value);
        result /= value;
        return true;
    }

    // --- 進階運算（調用成員函數）---
    void square() {
        multiply(result);  // 調用自己的 multiply
    }

    void negate() {
        multiply(-1);      // 調用自己的 multiply
    }

    // --- 工具函數 ---
    void reset() {
        result = 0.0;
        history = "";
        operationCount = 0;
        cout << "計算器已重置" << endl;
    }

    void showResult() {
        cout << "目前結果: " << result << endl;
    }

    void showHistory() {
        cout << "操作歷史 (" << operationCount << " 次操作):" << endl;
        cout << history << endl;
    }

    // --- 重載：不同型別的設定方式 ---
    void setValue(double val) {
        result = val;
        history = "初始值: " + to_string(val) + "\n";
        operationCount = 0;
    }

    void setValue(int val) {
        setValue((double)val);  // 呼叫 double 版本，避免重複程式碼
    }

private:
    // 私有的輔助函數 —— 外界不能直接呼叫
    void record(const string& op, double value) {
        history += "  " + op + " " + to_string(value) + " = ";
        operationCount++;
    }
};

int main() {
    Calculator calc;

    calc.setValue(10);
    calc.add(5);
    calc.showResult();

    calc.multiply(3);
    calc.showResult();

    calc.subtract(8);
    calc.showResult();

    calc.divide(7);
    calc.showResult();

    calc.divide(0);  // 測試除以零

    cout << "\n";
    calc.showHistory();

    cout << "\n--- 重置 ---" << endl;
    calc.reset();
    calc.setValue(100);
    calc.subtract(30);
    calc.showResult();

    return 0;
}
```

**預期輸出：**
```
目前結果: 15
目前結果: 45
目前結果: 37
目前結果: 5.28571
錯誤：不能除以零！

操作歷史 (4 次操作):
初始值: 10.000000
  + 5.000000 =   * 3.000000 =   - 8.000000 =   / 7.000000 = 

--- 重置 ---
計算器已重置
目前結果: 70
```

**這個範例展示了本課所有概念**：
- 類內定義和類外定義混用
- 函數重載（兩個 `setValue`）
- 成員函數互相調用（`square()` 調用 `multiply()`，`setValue(int)` 調用 `setValue(double)`）
- 帶預設參數的概念延伸
- 私有輔助函數（`record`）—— 外界不需要知道的內部操作
- 返回 bool 表示操作是否成功

---

## 本課重點回顧

| 概念 | 說明 |
|------|------|
| 類內定義 | 短函數直接寫在 class 裡，隱式 inline |
| 類外定義 | 長函數用 `類別名::函數名` 在外部實現 |
| 函數重載 | 同名函數，不同參數列表（型別或數量） |
| 預設參數 | 從右往左給預設值，呼叫時從右往左省略 |
| 成員函數互調 | 成員函數之間可以直接互相呼叫 |
| 隱藏的 this | 編譯器自動傳入指向調用者的指標 |
| 鏈式調用 | 返回 `*this` 實現 `a.foo().bar().baz()` |
| 私有輔助函數 | 放在 `private` 中，只供內部使用 |

---

下一課是 **第 11 課：存取修飾符——public、private、protected**，我們會完整講解三種存取等級的差異、使用時機，以及為什麼「盡量 private」是好習慣。準備好就告訴我！
*/

// =============================================================================
//  第 10 課：成員函數 9  —  綜合實戰：Calculator（含私有輔助函數）
// =============================================================================
//
// 【主題資訊 Information】
//   本檔上半部是第 10 課的完整講義（以區塊註解保存），
//   下半部是可執行的綜合實戰：一個帶運算歷史與私有輔助函數的計算器。
//   標準：  C++11 起（用到預設成員初始化 double result = 0.0;）；以 C++17 編譯。
//   標頭檔：<iostream>、<string>、<cmath>
//
// 【詳細解釋 Explanation】
//
// 【1. 這個 Calculator 用上了本課的哪些概念】
//     - 成員函數互相呼叫：square() 直接呼叫 multiply(result)、
//       negate() 呼叫 multiply(-1) —— 不必重寫乘法邏輯。
//     - 私有輔助函數：record() 放在 private，只供內部記帳用，
//       外界呼叫 calc.record(...) 會編譯錯誤。這是封裝的最小示範。
//     - 函數重載：setValue(double) 與 setValue(int)，
//       後者轉呼前者，避免兩份重複邏輯。
//     - 回傳值表達成敗：divide() 回傳 bool 讓呼叫端能判斷是否成功。
//
// 【2. 私有輔助函數為什麼重要】
//   record() 是「實作細節」而非「對外能力」。把它設為 private 有兩個好處：
//     (a) 外界無法繞過運算直接偽造歷史紀錄，類別的不變量得以維持；
//     (b) 日後想改歷史的儲存格式（例如改存 vector<Op> 而非字串），
//         只要不動公開介面，所有呼叫端都不受影響。
//   這正是「能 private 就 private」這條準則的實際價值。
//
// 【3. ⚠️ 請注意本檔 history 的輸出長相 —— 這是刻意保留的教學素材】
//   執行後你會看到歷史印成：
//       + 5.000000 =   * 3.000000 =   - 8.000000 =   / 7.000000 =
//   每個 "=" 後面都是空的、而且全部擠在同一行。原因有二：
//     (a) record() 是在**運算執行之前**被呼叫的（見 add()：先 record 再 +=），
//         所以它寫入 "= " 時，結果根本還沒算出來；
//     (b) record() 只寫了 "= " 就結束，沒有補上結果，也沒有換行。
//   這是一個很典型的「先記錄後計算」順序錯誤。
//   要修正的話有兩種常見做法：
//     方案一：把 record() 移到運算之後，並把 result 一起傳入；
//     方案二：讓 record() 只負責記錄運算元，另設 finishRecord() 補上結果。
//   ★ 這裡刻意不改動原程式碼，因為「預期輸出」必須忠實反映實際執行結果；
//     請把它當成待重構的練習題。下方【日常實務範例】示範了修好的版本。
//
// 【概念補充 Concept Deep Dive】
//   divide() 用 value == 0.0 判斷除以零 —— 這是浮點比較的**少數合理例外**：
//   我們要擋的正是「恰好是零」這個特定位元樣式，而 -0.0 == 0.0 為 true，
//   所以正負零都會被擋下。但它擋不住**極接近零**的值：
//   例如 1e-320（非正規數 subnormal）不等於 0，除下去會得到 inf。
//   若要防範這種情況，應改判 std::abs(value) < epsilon 並自行決定 epsilon。
//
//   另外 to_string(double) 固定輸出 6 位小數，所以 5 會印成 "5.000000"。
//   這是 to_string 對浮點的規格（等同 sprintf 的 "%f"），
//   不是精度問題。要控制格式應改用 ostringstream + setprecision。
//
// 【注意事項 Pay Attention】
//   1. 私有成員函數只是**存取控制**，不是安全機制；
//      它擋的是誤用，不是惡意存取。
//   2. history 用 string += 逐次累加，在大量操作下會有反覆重配置的成本；
//      高頻場景應改用 vector 或先 reserve()。
//   3. 浮點的 == 只在「比對特定位元樣式」（如剛好是 0）時才適用，
//      一般數值比較請用門檻或 epsilon。
//   4. square() 呼叫 multiply(result) 會同時讀寫 result，
//      因為引數是**傳值**、在函式進入前就先求值，所以安全；
//      若改成傳引用就會出現讀寫交錯的問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】綜合實戰：類別的內部組織
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼要把 record() 設成 private？它不是也很有用嗎？
//     答：因為它是**實作細節**，不是對外能力。設為 private 讓外界
//         無法繞過實際運算偽造歷史，維持了類別的不變量；
//         同時日後改變歷史的儲存格式時，公開介面不變，呼叫端零修改。
//     追問：那衍生類別想用怎麼辦？
//         → 改成 protected。但要注意 protected 的封裝性其實接近 public ——
//           任何人都能藉由繼承取得存取權（第 11 課的重點）。
//
// 🔥 Q2. square() 直接呼叫 multiply(result)，同時讀又寫 result，會有問題嗎？
//     答：不會。multiply 的參數是**傳值**，引數 result 在進入函式前就已求值
//         並複製一份，函式內對 this->result 的修改不影響那份副本。
//         但若參數改成 double&（傳引用），就會變成邊改邊讀，結果錯誤。
//     追問：那 negate() 呼叫 multiply(-1) 為什麼要小心？
//         → -1 是 int，會隱式轉成 double。這裡安全，
//           但若同時存在 multiply(int) 重載，選中的版本就會改變。
//
// ⚠️ 陷阱. divide() 用 value == 0.0 擋除以零，這樣就安全了嗎？
//     答：只擋得住「恰好是零」。極接近零的非正規數（例如 1e-320）
//         不等於 0，仍會通過檢查，除完得到 inf 而非錯誤 ——
//         程式不會崩潰，但結果從此被 inf 污染，更難追查。
//     為什麼會錯：把「除以零」想成單一情況，
//         但浮點世界裡真正的風險是「除以一個小到讓結果溢位的數」。
//         穩健做法是判 std::abs(value) < epsilon，epsilon 依領域決定。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <sstream>
using namespace std;

class Calculator {
public:
    double result = 0.0;
    string history = "";
    int operationCount = 0;

    // --- 基本運算 ---
    void add(double value) {
        record("+", value);
        result += value;
    }

    void subtract(double value) {
        record("-", value);
        result -= value;
    }

    void multiply(double value) {
        record("*", value);
        result *= value;
    }

    // 除法需要檢查除以零
    bool divide(double value) {
        if (value == 0.0) {
            cout << "錯誤：不能除以零！" << endl;
            return false;
        }
        record("/", value);
        result /= value;
        return true;
    }

    // --- 進階運算（調用成員函數）---
    void square() {
        multiply(result);  // 調用自己的 multiply
    }

    void negate() {
        multiply(-1);      // 調用自己的 multiply
    }

    // --- 工具函數 ---
    void reset() {
        result = 0.0;
        history = "";
        operationCount = 0;
        cout << "計算器已重置" << endl;
    }

    void showResult() {
        cout << "目前結果: " << result << endl;
    }

    void showHistory() {
        cout << "操作歷史 (" << operationCount << " 次操作):" << endl;
        cout << history << endl;
    }

    // --- 重載：不同型別的設定方式 ---
    void setValue(double val) {
        result = val;
        history = "初始值: " + to_string(val) + "\n";
        operationCount = 0;
    }

    void setValue(int val) {
        setValue((double)val);  // 呼叫 double 版本，避免重複程式碼
    }

private:
    // 私有的輔助函數 —— 外界不能直接呼叫
    void record(const string& op, double value) {
        history += "  " + op + " " + to_string(value) + " = ";
        operationCount++;
    }
};


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 682. Baseball Game
//   題目：依序處理一串操作紀錄，維護一個得分清單：
//         整數 x → 記錄新分數 x
//         "+"   → 記錄「前兩筆之和」
//         "D"   → 記錄「前一筆的兩倍」
//         "C"   → 撤銷（移除）前一筆
//         最後回傳所有紀錄的總和。
//   為什麼用到本主題：這題與本檔 Calculator 是同一個模式 ——
//         **一個維護內部歷史狀態的物件，用成員函數逐步改變它**，
//         並用私有輔助函數（push）集中「寫入歷史」這件事。
//         而且 "C"（撤銷）正是 Calculator 的 history 想做卻做不到的事：
//         要能撤銷，歷史就必須是結構化的（vector），而不是一坨字串。
//   複雜度：時間 O(n)，空間 O(n)。
// -----------------------------------------------------------------------------
class BaseballScore {
    vector<int> m_records;      // 結構化的歷史 —— 這才撤銷得了

    // 私有輔助函數：與 Calculator::record() 同樣的角色
    void push(int score) { m_records.push_back(score); }

public:
    void apply(const string& op) {
        if (op == "C") {
            if (!m_records.empty()) m_records.pop_back();
        } else if (op == "D") {
            if (!m_records.empty()) push(m_records.back() * 2);
        } else if (op == "+") {
            size_t n = m_records.size();
            if (n >= 2) push(m_records[n - 1] + m_records[n - 2]);
        } else {
            push(stoi(op));
        }
    }

    int total() const {
        int sum = 0;
        for (int s : m_records) sum += s;
        return sum;
    }

    size_t size() const { return m_records.size(); }
};

int calPoints(const vector<string>& operations) {
    BaseballScore game;
    for (const string& op : operations) game.apply(op);
    return game.total();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】把 Calculator 的歷史修好：可稽核的操作日誌
//   情境：金融／醫療系統的計算器不只要算對，還要能**事後稽核**——
//         每一步做了什麼、當時的值是多少，都得留下可讀的紀錄。
//   為什麼用到本主題：這是上面【詳細解釋 3】指出的問題的正解示範。
//         關鍵差異有三：
//           (a) record() 移到**運算之後**呼叫，才拿得到結果；
//           (b) 歷史存成結構化的 vector<Step>，而非一坨字串 ——
//               這樣才能撤銷、篩選、匯出成 CSV／JSON；
//           (c) 格式化與儲存分離：儲存原始數值，要輸出時才格式化。
// -----------------------------------------------------------------------------
class AuditedCalculator {
    struct Step {
        string op;          // "+", "-", "*", "/"
        double operand;
        double resultAfter; // 這一步做完之後的值
    };

    double       m_value = 0.0;
    vector<Step> m_steps;

    // private 輔助函數：只有在運算真的完成後才會被呼叫
    void record(const string& op, double operand) {
        m_steps.push_back(Step{op, operand, m_value});   // m_value 此時已是新值
    }

public:
    void setValue(double v) {
        m_value = v;
        m_steps.clear();
    }

    AuditedCalculator& add(double v)      { m_value += v; record("+", v); return *this; }
    AuditedCalculator& subtract(double v) { m_value -= v; record("-", v); return *this; }
    AuditedCalculator& multiply(double v) { m_value *= v; record("*", v); return *this; }

    bool divide(double v) {
        if (std::abs(v) < 1e-12) {         // 不只擋 0，也擋「小到會讓結果爆掉」的數
            cout << "  拒絕除以近似零的值: " << v << endl;
            return false;
        }
        m_value /= v;
        record("/", v);
        return true;
    }

    double value() const { return m_value; }

    // 撤銷最後一步：因為歷史是結構化的，這才做得到
    bool undo() {
        if (m_steps.empty()) return false;
        const Step& last = m_steps.back();
        // 反向運算還原
        if      (last.op == "+") m_value -= last.operand;
        else if (last.op == "-") m_value += last.operand;
        else if (last.op == "*") m_value /= last.operand;
        else if (last.op == "/") m_value *= last.operand;
        m_steps.pop_back();
        return true;
    }

    void showAuditLog() const {
        cout << "  稽核日誌 (" << m_steps.size() << " 步):" << endl;
        for (size_t i = 0; i < m_steps.size(); ++i) {
            const Step& s = m_steps[i];
            ostringstream oss;
            oss.setf(ios::fixed);
            oss.precision(2);
            oss << "    [" << (i + 1) << "] " << s.op << " " << s.operand
                << "  ->  " << s.resultAfter;
            cout << oss.str() << endl;
        }
    }
};

int main() {
    Calculator calc;

    calc.setValue(10);
    calc.add(5);
    calc.showResult();

    calc.multiply(3);
    calc.showResult();

    calc.subtract(8);
    calc.showResult();

    calc.divide(7);
    calc.showResult();

    calc.divide(0);  // 測試除以零

    cout << "\n";
    calc.showHistory();

    cout << "\n--- 重置 ---" << endl;
    calc.reset();
    calc.setValue(100);
    calc.subtract(30);
    calc.showResult();


    cout << "\n=== LeetCode 682. Baseball Game ===" << endl;
    vector<string> ops1{"5", "2", "C", "D", "+"};
    cout << "  {5,2,C,D,+} -> " << calPoints(ops1) << endl;
    vector<string> ops2{"5", "-2", "4", "C", "D", "9", "+", "+"};
    cout << "  {5,-2,4,C,D,9,+,+} -> " << calPoints(ops2) << endl;
    vector<string> ops3{"1", "C"};
    cout << "  {1,C} -> " << calPoints(ops3) << "  (全被撤銷)" << endl;

    cout << "\n=== 日常實務：修好的可稽核計算器 ===" << endl;
    AuditedCalculator ac;
    ac.setValue(10);
    ac.add(5).multiply(3).subtract(8);      // 鏈式串接
    ac.divide(7);
    cout << "  目前值: " << ac.value() << endl;
    ac.showAuditLog();                       // 這次每一步都有結果，不再是空的 "="

    cout << "  --- 測試除以近似零 ---" << endl;
    ac.divide(0.0);                          // 被擋下
    ac.divide(1e-320);                       // 非正規數也被擋下（原版擋不住）

    cout << "  --- 測試撤銷 ---" << endl;
    cout << "  撤銷前: " << ac.value() << endl;
    ac.undo();                               // 撤銷 /7
    cout << "  撤銷 /7 之後: " << ac.value() << endl;
    ac.showAuditLog();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）9.cpp" -o member9

// === 預期輸出 ===
// 目前結果: 15
// 目前結果: 45
// 目前結果: 37
// 目前結果: 5.28571
// 錯誤：不能除以零！
// 
// 操作歷史 (4 次操作):
// 初始值: 10.000000
//   + 5.000000 =   * 3.000000 =   - 8.000000 =   / 7.000000 =
// 
// --- 重置 ---
// 計算器已重置
// 目前結果: 70
// 
// === LeetCode 682. Baseball Game ===
//   {5,2,C,D,+} -> 30
//   {5,-2,4,C,D,9,+,+} -> 27
//   {1,C} -> 0  (全被撤銷)
// 
// === 日常實務：修好的可稽核計算器 ===
//   目前值: 5.28571
//   稽核日誌 (4 步):
//     [1] + 5.00  ->  15.00
//     [2] * 3.00  ->  45.00
//     [3] - 8.00  ->  37.00
//     [4] / 7.00  ->  5.29
//   --- 測試除以近似零 ---
//   拒絕除以近似零的值: 0
//   拒絕除以近似零的值: 9.99989e-321
//   --- 測試撤銷 ---
//   撤銷前: 5.28571
//   撤銷 /7 之後: 37
//   稽核日誌 (3 步):
//     [1] + 5.00  ->  15.00
//     [2] * 3.00  ->  45.00
//     [3] - 8.00  ->  37.00
