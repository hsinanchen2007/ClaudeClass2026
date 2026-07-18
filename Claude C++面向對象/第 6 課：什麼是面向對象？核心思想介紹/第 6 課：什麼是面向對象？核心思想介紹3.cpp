/*
好的，信安！讓我們繼續 OOP 課程，從第 6 課開始。

---

# 第 6 課：什麼是面向對象？核心思想介紹

## 一、為什麼需要面向對象？

在你學過的 C 語言中，程式是以**函數**為中心組織的——資料和操作資料的函數是分開的。這種方式叫做**程序式程式設計（Procedural Programming）**。

```c
// C 語言的程序式風格
struct Student {
    char name[50];
    int age;
    float gpa;
};

void print_student(struct Student* s) {
    printf("Name: %s, Age: %d, GPA: %.1f\n", s->name, s->age, s->gpa);
}

void set_gpa(struct Student* s, float new_gpa) {
    s->gpa = new_gpa;  // 任何人都可以隨便改資料！
}
```

這有幾個問題：

1. **資料暴露**：`struct` 的所有成員對外完全公開，任何人都能直接修改 `s->gpa = -999.0`，沒有任何保護。
2. **資料和函數分離**：`print_student` 和 `set_gpa` 與 `Student` 之間沒有語言層面的綁定關係，只是「碰巧」操作同一個 struct。
3. **難以擴展**：如果你要區分「大學生」和「研究生」，C 語言沒有原生的繼承機制，你必須手動管理。

**面向對象程式設計（Object-Oriented Programming, OOP）** 就是為了解決這些問題而誕生的。

---

## 二、OOP 的四大核心思想

### 1. 封裝（Encapsulation）

**核心概念**：把資料和操作資料的函數綁在一起，並控制外界的存取權限。

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:    // 外界不能直接存取
    string name;
    int age;
    float gpa;

public:     // 外界可以使用的介面
    void setGpa(float new_gpa) {
        if (new_gpa >= 0.0f && new_gpa <= 4.0f) {  // 加入驗證！
            gpa = new_gpa;
        } else {
            cout << "錯誤：GPA 必須在 0.0 到 4.0 之間" << endl;
        }
    }

    float getGpa() const {
        return gpa;
    }
};
```

**對比 C 語言**：在 C 中，`s->gpa = -999.0` 完全合法且不會有任何警告。但在 C++ 的封裝下，所有修改必須經過 `setGpa()`，非法值會被攔截。

封裝的本質是：**隱藏實現細節，只暴露必要的介面。**

---

### 2. 繼承（Inheritance）

**核心概念**：新的類別可以從已有的類別「繼承」屬性和行為，實現程式碼重用。

```cpp
#include <iostream>
#include <string>
using namespace std;

// 基類（父類）
class Animal {
public:
    string name;

    void eat() {
        cout << name << " 正在吃東西" << endl;
    }
};

// 派生類（子類）—— 繼承了 Animal 的一切
class Dog : public Animal {
public:
    void bark() {
        cout << name << " 汪汪叫！" << endl;
    }
};

class Cat : public Animal {
public:
    void meow() {
        cout << name << " 喵喵叫！" << endl;
    }
};

int main() {
    Dog d;
    d.name = "旺財";
    d.eat();   // 繼承自 Animal
    d.bark();  // Dog 自己的行為

    Cat c;
    c.name = "咪咪";
    c.eat();   // 繼承自 Animal
    c.meow();  // Cat 自己的行為

    return 0;
}
```

**預期輸出：**
```
旺財 正在吃東西
旺財 汪汪叫！
咪咪 正在吃東西
咪咪 喵喵叫！
```

**在 C 語言中怎麼做？** 你必須手動在 `Dog` 結構體中包含一個 `Animal` 成員，然後手動轉發函數調用——繁瑣且容易出錯。C++ 的繼承讓這一切變得自然。

---

### 3. 多型（Polymorphism）

**核心概念**：同一個介面，不同的實現。透過基類指標或引用，可以調用不同派生類的實現。

```cpp
#include <iostream>
#include <string>
using namespace std;

class Shape {
public:
    virtual double area() {  // virtual = 虛函數，允許派生類覆寫
        return 0;
    }
    virtual ~Shape() {}  // 虛解構函數（後續課程會詳解）
};

class Circle : public Shape {
private:
    double radius;
public:
    Circle(double r) : radius(r) {}

    double area() override {  // 覆寫基類的 area()
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}

    double area() override {
        return width * height;
    }
};

// 這個函數接受任何 Shape！
void printArea(Shape* s) {
    cout << "面積 = " << s->area() << endl;
}

int main() {
    Circle c(5.0);
    Rectangle r(4.0, 6.0);

    printArea(&c);  // 呼叫 Circle::area()
    printArea(&r);  // 呼叫 Rectangle::area()

    return 0;
}
```

**預期輸出：**
```
面積 = 78.5398
面積 = 24
```

**多型的威力**：`printArea` 函數根本不知道傳進來的是圓形還是矩形，但它能「自動」調用正確的 `area()` 實現。這就是多型。

---

### 4. 抽象（Abstraction）

**核心概念**：只展示「是什麼」，隱藏「怎麼做」。讓使用者關注功能而非實現。

```cpp
#include <iostream>
using namespace std;

// 抽象概念：資料庫
class Database {
public:
    virtual void connect() = 0;     // = 0 表示純虛函數，必須由子類實現
    virtual void query(const char* sql) = 0;
    virtual void disconnect() = 0;
    virtual ~Database() {}
};

// 具體實現：MySQL
class MySQL : public Database {
public:
    void connect() override {
        cout << "連接到 MySQL 資料庫..." << endl;
    }
    void query(const char* sql) override {
        cout << "MySQL 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "斷開 MySQL 連接" << endl;
    }
};

// 具體實現：SQLite
class SQLite : public Database {
public:
    void connect() override {
        cout << "打開 SQLite 檔案..." << endl;
    }
    void query(const char* sql) override {
        cout << "SQLite 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "關閉 SQLite 檔案" << endl;
    }
};

// 使用者只需要知道 Database 介面
void doWork(Database* db) {
    db->connect();
    db->query("SELECT * FROM users");
    db->disconnect();
}

int main() {
    MySQL mysql;
    SQLite sqlite;

    cout << "=== 使用 MySQL ===" << endl;
    doWork(&mysql);

    cout << "\n=== 使用 SQLite ===" << endl;
    doWork(&sqlite);

    return 0;
}
```

**預期輸出：**
```
=== 使用 MySQL ===
連接到 MySQL 資料庫...
MySQL 執行: SELECT * FROM users
斷開 MySQL 連接

=== 使用 SQLite ===
打開 SQLite 檔案...
SQLite 執行: SELECT * FROM users
關閉 SQLite 檔案
```

**抽象的意義**：`doWork()` 函數完全不關心底層是 MySQL 還是 SQLite。如果未來要支援 PostgreSQL，只需新增一個繼承 `Database` 的類別，`doWork()` 完全不用修改。

---

## 三、四大思想的關係圖

```
         面向對象程式設計 (OOP)
         ┌──────────────────────┐
         │                      │
    ┌────┴────┐           ┌─────┴─────┐
    │  封裝    │           │   繼承     │
    │Encapsulation│       │Inheritance │
    └────┬────┘           └─────┬─────┘
         │                      │
         │    ┌─────────────────┤
         │    │                 │
    ┌────┴────┴──┐        ┌────┴─────┐
    │   抽象      │        │   多型    │
    │ Abstraction │        │Polymorphism│
    └────────────┘        └──────────┘

封裝 → 保護資料，控制存取
繼承 → 重用程式碼，建立層次
多型 → 同一介面，不同行為（需要繼承作為基礎）
抽象 → 隱藏細節，只暴露介面（結合封裝和繼承）
```

---

## 四、OOP 思維 vs 程序式思維對比

| 面向 | 程序式（C 風格） | 面向對象（C++ 風格） |
|------|------------------|---------------------|
| 組織方式 | 以**函數**為中心 | 以**對象**為中心 |
| 資料保護 | 無，struct 全部公開 | 有，private/protected |
| 程式碼重用 | 函數調用 | 繼承 + 組合 |
| 擴展性 | 修改原有程式碼 | 新增派生類，不動原有程式碼 |
| 現實映射 | 較弱 | 較強（對象模擬真實事物） |

---

## 五、一句話記住四大思想

> **封裝**是把東西裝進盒子，只留一個操作窗口。
> **繼承**是兒子自動擁有父親的能力。
> **多型**是同一個命令，不同的人執行出不同的結果。
> **抽象**是只告訴你「按這個按鈕就行」，不告訴你裡面怎麼運作。

---

## 六、編譯與執行

以上每個範例都可以獨立編譯執行：

```bash
g++ -std=c++17 -o example example.cpp
./example
```

Windows (VS Code) 環境中：
```bash
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -o example.exe example.cpp
./example.exe
```

---

## 本課重點回顧

1. **OOP 誕生原因**：解決程序式程式設計中資料暴露、程式碼重用困難、擴展性差的問題
2. **四大核心**：封裝、繼承、多型、抽象
3. **封裝**：資料 + 方法綁定，存取控制
4. **繼承**：程式碼重用，建立類別層次
5. **多型**：相同介面、不同實現，運行時決定行為
6. **抽象**：隱藏實現細節，只暴露介面

---

這就是 OOP 的全貌鳥瞰。從下一課開始，我們會一步步深入每一個概念。

準備好進入 **第 7 課：類別（class）的定義與語法** 了嗎？
*/



#include <iostream>
using namespace std;

// 抽象概念：資料庫
class Database {
public:
    virtual void connect() = 0;     // = 0 表示純虛函數，必須由子類實現
    virtual void query(const char* sql) = 0;
    virtual void disconnect() = 0;
    virtual ~Database() {}
};

// 具體實現：MySQL
class MySQL : public Database {
public:
    void connect() override {
        cout << "連接到 MySQL 資料庫..." << endl;
    }
    void query(const char* sql) override {
        cout << "MySQL 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "斷開 MySQL 連接" << endl;
    }
};

// 具體實現：SQLite
class SQLite : public Database {
public:
    void connect() override {
        cout << "打開 SQLite 檔案..." << endl;
    }
    void query(const char* sql) override {
        cout << "SQLite 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "關閉 SQLite 檔案" << endl;
    }
};

// 使用者只需要知道 Database 介面
void doWork(Database* db) {
    db->connect();
    db->query("SELECT * FROM users");
    db->disconnect();
}

int main() {
    MySQL mysql;
    SQLite sqlite;

    cout << "=== 使用 MySQL ===" << endl;
    doWork(&mysql);

    cout << "\n=== 使用 SQLite ===" << endl;
    doWork(&sqlite);

    return 0;
}
