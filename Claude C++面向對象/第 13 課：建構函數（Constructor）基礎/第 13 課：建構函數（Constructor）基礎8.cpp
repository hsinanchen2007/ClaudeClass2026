/*
好的，信安！我們進入 **第三階段：建構與解構**，從第 13 課開始。

---

# 第 13 課：建構函數（Constructor）基礎

---

## 13.1 什麼是建構函數？

在前面的課程中，我們學會了定義類別和創建對象。但有個問題——當我們創建一個對象時，它的成員變數處於什麼狀態？

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    string name;
    int age;
    float gpa;

    void print() {
        cout << "姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    Student s;
    s.print();  // name 是空字串, age 和 gpa 是未定義的垃圾值！
    return 0;
}
```

**問題**：`age` 和 `gpa` 沒有被初始化，它們的值是不確定的垃圾值（和你在 C 語言中遇到的一樣）。`string` 類型因為自身有預設建構函數，所以會被初始化為空字串，但基本型別（int、float 等）不會。

**建構函數（Constructor）** 就是用來解決這個問題的——它是一個**在對象創建時自動被調用的特殊成員函數**，負責初始化對象的狀態。

---

## 13.2 建構函數的語法規則

建構函數有幾個獨特的特徵，和普通成員函數不同：

| 特徵 | 說明 |
|------|------|
| 函數名 | **必須與類別名完全相同** |
| 返回值 | **沒有返回值，連 `void` 都不寫** |
| 調用時機 | **對象創建時自動調用**，不需要手動調用 |
| 可以重載 | 一個類別可以有多個建構函數（參數不同） |
| 存取權限 | 通常是 `public`（否則外界無法創建對象） |

```cpp
class 類別名 {
public:
    // 建構函數：函數名 = 類別名，無返回值
    類別名() {
        // 初始化程式碼
    }
    
    類別名(參數列表) {
        // 帶參數的初始化程式碼
    }
};
```

---

## 13.3 對比 C 語言的初始化方式

在 C 語言中，你需要手動調用初始化函數：

```c
// C 語言風格
typedef struct {
    char name[50];
    int age;
    float gpa;
} Student;

// 必須手動調用！而且有可能忘記調用！
void student_init(Student* s, const char* name, int age, float gpa) {
    strcpy(s->name, name);
    s->age = age;
    s->gpa = gpa;
}

int main() {
    Student s;
    // 如果忘記調用 student_init()，s 的內容就是垃圾值
    // 編譯器不會警告你！
    student_init(&s, "張三", 20, 3.8);
    return 0;
}
```

C 語言的問題是：**初始化和對象創建是分離的**，程式設計師可能忘記調用初始化函數，而編譯器不會提醒你。

C++ 的建構函數把這兩步合為一步，**保證對象一旦創建就已經被正確初始化**。

---

## 13.4 第一個建構函數範例

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // ====== 建構函數 ======
    // 函數名 = 類別名 "Student"
    // 沒有返回值（連 void 都不寫）
    Student() {
        cout << ">>> 建構函數被調用了！ <<<" << endl;
        name = "未命名";
        age = 0;
        gpa = 0.0f;
    }

    void print() const {
        cout << "姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== 準備創建對象 ===" << endl;
    
    Student s;  // 創建對象的瞬間，建構函數自動被調用
    
    cout << "=== 對象已創建 ===" << endl;
    s.print();  // 成員變數已被初始化，不再是垃圾值
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson13 lesson13.cpp
./lesson13
```

### 預期輸出

```
=== 準備創建對象 ===
>>> 建構函數被調用了！ <
=== 對象已創建 ===
姓名: 未命名, 年齡: 0, GPA: 0
```

**關鍵觀察**：我們從來沒有手動調用 `Student()`，它在 `Student s;` 這一行**自動**被調用了。

---

## 13.5 建構函數的調用時機

建構函數在不同場景下的調用時機：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Box {
private:
    string label;
    
public:
    Box() {
        label = "空盒子";
        cout << "建構函數: 創建了 [" << label << "]" << endl;
    }
    
    string getLabel() const { return label; }
};

// ====== 全域對象 ======
Box globalBox;  // 程式啟動時，在 main() 之前就建構

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    // ====== 局部對象 ======
    Box localBox;  // 執行到這一行時建構
    
    cout << "\n=== 進入區塊 ===" << endl;
    {
        // ====== 區塊內的對象 ======
        Box blockBox;  // 進入這個區塊時建構
        cout << "區塊內..." << endl;
    }  // blockBox 在這裡離開作用域
    
    cout << "\n=== 動態對象 ===" << endl;
    // ====== 動態分配的對象 ======
    Box* heapBox = new Box();  // new 的時候建構
    delete heapBox;  // 記得釋放
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
```

### 預期輸出

```
建構函數: 創建了 [空盒子]

=== main() 開始 ===
建構函數: 創建了 [空盒子]

=== 進入區塊 ===
建構函數: 創建了 [空盒子]
區塊內...

=== 動態對象 ===
建構函數: 創建了 [空盒子]

=== main() 結束 ===
```

**四種場景總結**：

| 對象類型 | 建構時機 | 範例 |
|----------|----------|------|
| 全域對象 | `main()` 之前 | `Box globalBox;` |
| 局部對象 | 執行到宣告語句時 | `Box localBox;` |
| 區塊內對象 | 進入區塊執行到宣告時 | `{ Box blockBox; }` |
| 動態對象 | `new` 運算子執行時 | `new Box()` |

---

## 13.6 帶參數的建構函數

只能初始化為固定值很不實用。實際上，建構函數可以接受參數：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // 無參數建構函數
    Student() {
        name = "未命名";
        age = 0;
        gpa = 0.0f;
        cout << "[預設建構] 創建學生: " << name << endl;
    }

    // 帶參數的建構函數
    Student(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        cout << "[帶參建構] 創建學生: " << name << endl;
    }

    void print() const {
        cout << "  姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== 使用預設建構函數 ===" << endl;
    Student s1;           // 調用無參數建構函數
    s1.print();
    
    cout << "\n=== 使用帶參數建構函數 ===" << endl;
    Student s2("張三", 20, 3.8f);   // 調用帶參數建構函數
    s2.print();
    
    Student s3("李四", 22, 3.5f);   // 另一個帶參數建構
    s3.print();
    
    return 0;
}
```

### 預期輸出

```
=== 使用預設建構函數 ===
[預設建構] 創建學生: 未命名
  姓名: 未命名, 年齡: 0, GPA: 0

=== 使用帶參數建構函數 ===
[帶參建構] 創建學生: 張三
  姓名: 張三, 年齡: 20, GPA: 3.8

[帶參建構] 創建學生: 李四
  姓名: 李四, 年齡: 22, GPA: 3.5
```

---

## 13.7 建構函數重載

和普通函數一樣，建構函數支援重載——可以有多個建構函數，只要參數列表不同：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Rectangle {
private:
    double width;
    double height;
    string color;

public:
    // 建構函數 1：無參數 → 預設的矩形
    Rectangle() {
        width = 1.0;
        height = 1.0;
        color = "白色";
        cout << "[建構1] 預設矩形" << endl;
    }

    // 建構函數 2：正方形（只給一個邊長）
    Rectangle(double side) {
        width = side;
        height = side;
        color = "白色";
        cout << "[建構2] 正方形, 邊長 = " << side << endl;
    }

    // 建構函數 3：指定寬高
    Rectangle(double w, double h) {
        width = w;
        height = h;
        color = "白色";
        cout << "[建構3] 矩形 " << w << " x " << h << endl;
    }

    // 建構函數 4：指定寬高和顏色
    Rectangle(double w, double h, string c) {
        width = w;
        height = h;
        color = c;
        cout << "[建構4] " << c << "矩形 " << w << " x " << h << endl;
    }

    void print() const {
        cout << "  " << color << " 矩形: " 
             << width << " x " << height 
             << ", 面積 = " << width * height << endl;
    }
};

int main() {
    Rectangle r1;                         // 調用建構函數 1
    r1.print();
    
    Rectangle r2(5.0);                    // 調用建構函數 2
    r2.print();
    
    Rectangle r3(4.0, 6.0);              // 調用建構函數 3
    r3.print();
    
    Rectangle r4(3.0, 7.0, "紅色");       // 調用建構函數 4
    r4.print();
    
    return 0;
}
```

### 預期輸出

```
[建構1] 預設矩形
  白色 矩形: 1 x 1, 面積 = 1
[建構2] 正方形, 邊長 = 5
  白色 矩形: 5 x 5, 面積 = 25
[建構3] 矩形 4 x 6
  白色 矩形: 4 x 6, 面積 = 24
[建構4] 紅色矩形 3 x 7
  紅色 矩形: 3 x 7, 面積 = 21
```

**編譯器如何選擇？** 根據你傳入的參數數量和型別，編譯器自動匹配最合適的建構函數，這就是**重載解析（Overload Resolution）**。

---

## 13.8 建構函數中的資料驗證

建構函數不僅用來賦值，還可以加入**驗證邏輯**，確保對象從一開始就處於合法狀態：

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner;
    double balance;
    string accountId;

public:
    BankAccount(string ownerName, double initialBalance, string id) {
        // 驗證帳戶名
        if (ownerName.empty()) {
            cout << "警告：帳戶名不能為空，使用預設值" << endl;
            owner = "未知";
        } else {
            owner = ownerName;
        }
        
        // 驗證初始餘額
        if (initialBalance < 0) {
            cout << "警告：初始餘額不能為負數，設為 0" << endl;
            balance = 0.0;
        } else {
            balance = initialBalance;
        }
        
        // 驗證帳戶 ID
        if (id.length() != 10) {
            cout << "警告：帳戶 ID 必須為 10 位，使用預設 ID" << endl;
            accountId = "0000000000";
        } else {
            accountId = id;
        }
        
        cout << "帳戶創建成功: " << owner << endl;
    }

    void print() const {
        cout << "  帳戶: " << accountId 
             << ", 戶主: " << owner 
             << ", 餘額: $" << balance << endl;
    }
};

int main() {
    cout << "=== 正常創建 ===" << endl;
    BankAccount a1("王五", 10000.0, "1234567890");
    a1.print();
    
    cout << "\n=== 非法數據 ===" << endl;
    BankAccount a2("", -500.0, "123");  // 全部都是非法值
    a2.print();
    
    return 0;
}
```

### 預期輸出

```
=== 正常創建 ===
帳戶創建成功: 王五
  帳戶: 1234567890, 戶主: 王五, 餘額: $10000

=== 非法數據 ===
警告：帳戶名不能為空，使用預設值
警告：初始餘額不能為負數，設為 0
警告：帳戶 ID 必須為 10 位，使用預設 ID
帳戶創建成功: 未知
  帳戶: 0000000000, 戶主: 未知, 餘額: $0
```

**重要觀念**：建構函數是你防止非法對象存在的**第一道防線**。一個好的建構函數應該保證——如果對象成功創建，它的狀態一定是合法的。

---

## 13.9 常見錯誤與陷阱

### 陷阱 1：不要給建構函數寫返回值

```cpp
class Bad {
public:
    void Bad() {  // 錯誤！建構函數不能有返回值！
        // ...     // 這會變成一個普通的成員函數，名字碰巧叫 Bad
    }
};
```

### 陷阱 2：宣告對象時不要加空括號

```cpp
Student s();   // 錯誤！這是函數宣告，不是對象創建！
               // 編譯器把它理解為：宣告一個函數 s，返回 Student

Student s;     // 正確：調用預設建構函數
Student s{};   // 正確（C++11）：統一初始化語法
```

這個陷阱被稱為 **最令人困擾的解析（Most Vexing Parse）**，是 C++ 中非常著名的坑。

### 陷阱 3：如果定義了帶參建構函數，預設建構函數就消失了

```cpp
class Point {
public:
    int x, y;
    
    // 只定義了帶參數的建構函數
    Point(int px, int py) {
        x = px;
        y = py;
    }
};

int main() {
    Point p1(3, 4);   // OK
    Point p2;          // 編譯錯誤！沒有預設建構函數！
    return 0;
}
```

**原因**：如果你沒有定義任何建構函數，編譯器會自動生成一個預設建構函數。但只要你定義了**任何一個**建構函數，編譯器就**不再自動生成**預設建構函數。這個問題我們在第 14 課會詳細講解。

---

## 13.10 完整綜合範例

```cpp
#include <iostream>
#include <string>
using namespace std;

class Car {
private:
    string brand;
    string model;
    int year;
    double mileage;

public:
    // 建構函數 1：完全預設
    Car() {
        brand = "未知";
        model = "未知";
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建預設汽車" << endl;
    }
    
    // 建構函數 2：只指定品牌和型號
    Car(string b, string m) {
        brand = b;
        model = m;
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建新車: " << brand << " " << model << endl;
    }
    
    // 建構函數 3：完整指定所有屬性
    Car(string b, string m, int y, double mi) {
        brand = b;
        model = m;
        
        // 年份驗證
        if (y < 1886 || y > 2025) {  // 1886 年是汽車發明的年份
            cout << "  警告：年份不合法，使用 2024" << endl;
            year = 2024;
        } else {
            year = y;
        }
        
        // 里程驗證
        if (mi < 0) {
            cout << "  警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        } else {
            mileage = mi;
        }
        
        cout << "[建構] 創建二手車: " << brand << " " << model 
             << " (" << year << ")" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << " " << model 
             << ", 里程: " << mileage << " km" << endl;
    }
};

int main() {
    cout << "===== 汽車展示廳 =====" << endl << endl;
    
    Car c1;
    c1.print();
    cout << endl;
    
    Car c2("Toyota", "Camry");
    c2.print();
    cout << endl;
    
    Car c3("BMW", "M3", 2020, 35000.5);
    c3.print();
    cout << endl;
    
    // 測試非法數據
    Car c4("時光機", "DeLorean", 1800, -100.0);
    c4.print();
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson13_full lesson13_full.cpp
./lesson13_full
```

### 預期輸出

```
===== 汽車展示廳 =====

[建構] 創建預設汽車
  2024 未知 未知, 里程: 0 km

[建構] 創建新車: Toyota Camry
  2024 Toyota Camry, 里程: 0 km

[建構] 創建二手車: BMW M3 (2020)
  2020 BMW M3, 里程: 35000.5 km

  警告：年份不合法，使用 2024
  警告：里程不能為負，設為 0
[建構] 創建二手車: 時光機 DeLorean (2024)
  2024 時光機 DeLorean, 里程: 0 km
```

---

## 13.11 本課重點回顧

| 概念 | 說明 |
|------|------|
| 建構函數 | 對象創建時自動調用的特殊成員函數 |
| 命名規則 | 必須與類別名相同，沒有返回值 |
| 調用時機 | 對象宣告時 / `new` 時自動調用 |
| 重載 | 一個類別可以有多個建構函數 |
| 資料驗證 | 建構函數是確保對象合法性的第一道防線 |
| Most Vexing Parse | `Student s();` 是函數宣告，不是對象創建 |
| 編譯器行為 | 定義了任何建構函數後，預設建構函數不再自動生成 |

---

## 13.12 下一課預告

下一課我們將深入探討**預設建構函數（Default Constructor）**——什麼時候編譯器會幫你生成？什麼時候不會？以及 C++11 的 `= default` 語法。

準備好進入 **第 14 課：預設建構函數** 了嗎？
*/



#include <iostream>
#include <string>
using namespace std;

class Car {
private:
    string brand;
    string model;
    int year;
    double mileage;

public:
    // 建構函數 1：完全預設
    Car() {
        brand = "未知";
        model = "未知";
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建預設汽車" << endl;
    }
    
    // 建構函數 2：只指定品牌和型號
    Car(string b, string m) {
        brand = b;
        model = m;
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建新車: " << brand << " " << model << endl;
    }
    
    // 建構函數 3：完整指定所有屬性
    Car(string b, string m, int y, double mi) {
        brand = b;
        model = m;
        
        // 年份驗證
        if (y < 1886 || y > 2025) {  // 1886 年是汽車發明的年份
            cout << "  警告：年份不合法，使用 2024" << endl;
            year = 2024;
        } else {
            year = y;
        }
        
        // 里程驗證
        if (mi < 0) {
            cout << "  警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        } else {
            mileage = mi;
        }
        
        cout << "[建構] 創建二手車: " << brand << " " << model 
             << " (" << year << ")" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << " " << model 
             << ", 里程: " << mileage << " km" << endl;
    }
};

int main() {
    cout << "===== 汽車展示廳 =====" << endl << endl;
    
    Car c1;
    c1.print();
    cout << endl;
    
    Car c2("Toyota", "Camry");
    c2.print();
    cout << endl;
    
    Car c3("BMW", "M3", 2020, 35000.5);
    c3.print();
    cout << endl;
    
    // 測試非法數據
    Car c4("時光機", "DeLorean", 1800, -100.0);
    c4.print();
    
    return 0;
}
