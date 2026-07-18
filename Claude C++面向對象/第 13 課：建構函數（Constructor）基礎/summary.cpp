/*
 * ================================================================
 * 【第 13 課：建構函數（Constructor）基礎】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 建構函數的定義：對象創建時自動調用的特殊成員函數
 * 2. 建構函數的語法規則：函數名=類別名、無返回值、自動調用
 * 3. 對比 C 語言的手動初始化 vs C++ 建構函數的自動初始化
 * 4. 建構函數的四種調用時機（全域、局部、區塊、動態）
 * 5. 帶參數的建構函數
 * 6. 建構函數重載（Overloading）
 * 7. 建構函數中的資料驗證
 * 8. 常見陷阱：返回值、Most Vexing Parse、預設建構消失
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cstring>
using namespace std;

// ================================================================
// 重點一：為什麼需要建構函數？
// ================================================================
// 沒有建構函數時，基本型別成員（int、float 等）不會自動初始化，
// 它們的值是未定義的「垃圾值」。
// string 等類別型別因為有自己的預設建構函數，會初始化為空字串。
//
// C 語言的問題：初始化和對象創建是分離的，容易忘記調用初始化函數。
// C++ 的建構函數把這兩步合為一步，保證對象一旦創建就已被正確初始化。

// --- C 語言風格（問題示範）---
// typedef struct {
//     char name[50];
//     int age;
// } Student_C;
// void student_init(Student_C* s, const char* name, int age) {
//     strcpy(s->name, name);
//     s->age = age;
// }
// // 問題：如果忘記調用 student_init()，成員就是垃圾值，編譯器不會警告！

// ================================================================
// 重點二：建構函數的語法規則
// ================================================================
// ┌─────────────────┬──────────────────────────────────┐
// │ 特徵             │ 說明                              │
// ├─────────────────┼──────────────────────────────────┤
// │ 函數名           │ 必須與類別名完全相同              │
// │ 返回值           │ 沒有返回值，連 void 都不寫        │
// │ 調用時機         │ 對象創建時自動調用                │
// │ 可以重載         │ 一個類別可以有多個建構函數        │
// │ 存取權限         │ 通常是 public（否則外界無法建構）  │
// └─────────────────┴──────────────────────────────────┘

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // 建構函數：函數名 = 類別名，沒有返回值（連 void 都不寫）
    Student() {
        cout << ">>> 預設建構函數被調用 <<<" << endl;
        name = "未命名";
        age = 0;
        gpa = 0.0f;
    }

    // 帶參數的建構函數（重載）
    Student(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        cout << ">>> 帶參建構函數被調用: " << name << " <<<" << endl;
    }

    void print() const {
        cout << "  姓名: " << name
             << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// ================================================================
// 重點三：建構函數的四種調用時機
// ================================================================
// ┌──────────────┬───────────────────────────────┐
// │ 對象類型      │ 建構時機                       │
// ├──────────────┼───────────────────────────────┤
// │ 全域對象      │ main() 之前                    │
// │ 局部對象      │ 執行到宣告語句時                │
// │ 區塊內對象    │ 進入區塊，執行到宣告時          │
// │ 動態對象      │ new 運算子執行時                │
// └──────────────┴───────────────────────────────┘

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

// ================================================================
// 重點四：建構函數重載（Overloading）
// ================================================================
// 和普通函數一樣，建構函數支援重載——可以有多個建構函數，只要參數列表不同。
// 編譯器根據傳入的參數數量和型別，自動匹配最合適的建構函數（重載解析）。

class Rectangle {
private:
    double width;
    double height;
    string color;

public:
    // 建構函數 1：無參數 → 預設矩形
    Rectangle() {
        width = 1.0;
        height = 1.0;
        color = "白色";
    }

    // 建構函數 2：正方形（只給一個邊長）
    Rectangle(double side) {
        width = side;
        height = side;
        color = "白色";
    }

    // 建構函數 3：指定寬高
    Rectangle(double w, double h) {
        width = w;
        height = h;
        color = "白色";
    }

    // 建構函數 4：指定寬高和顏色
    Rectangle(double w, double h, string c) {
        width = w;
        height = h;
        color = c;
    }

    void print() const {
        cout << "  " << color << " 矩形: "
             << width << " x " << height
             << ", 面積 = " << width * height << endl;
    }
};

// ================================================================
// 重點五：建構函數中的資料驗證
// ================================================================
// 建構函數不僅用來賦值，還可以加入驗證邏輯，
// 確保對象從一開始就處於合法狀態。
// 建構函數是防止非法對象存在的「第一道防線」。

class BankAccount {
private:
    string owner;
    double balance;
    string accountId;

public:
    BankAccount(string ownerName, double initialBalance, string id) {
        // 驗證帳戶名
        if (ownerName.empty()) {
            cout << "  警告：帳戶名不能為空，使用預設值" << endl;
            owner = "未知";
        } else {
            owner = ownerName;
        }

        // 驗證初始餘額
        if (initialBalance < 0) {
            cout << "  警告：初始餘額不能為負數，設為 0" << endl;
            balance = 0.0;
        } else {
            balance = initialBalance;
        }

        // 驗證帳戶 ID
        if (id.length() != 10) {
            cout << "  警告：帳戶 ID 必須為 10 位，使用預設 ID" << endl;
            accountId = "0000000000";
        } else {
            accountId = id;
        }
    }

    void print() const {
        cout << "  帳戶: " << accountId
             << ", 戶主: " << owner
             << ", 餘額: $" << balance << endl;
    }
};

// ================================================================
// 重點六：常見錯誤與陷阱
// ================================================================
//
// 陷阱 1：不要給建構函數寫返回值
//   void Bad() { }   // 錯誤！這會變成一個普通成員函數
//
// 陷阱 2：Most Vexing Parse（最令人困擾的解析）
//   Student s();     // 錯誤！這是「函數宣告」，不是對象創建！
//   Student s;       // 正確：調用預設建構函數
//   Student s{};     // 正確（C++11）：統一初始化語法
//
// 陷阱 3：定義了帶參建構函數後，預設建構函數就消失了
//   class Point {
//   public:
//       Point(int x, int y) { ... }  // 定義了帶參建構
//   };
//   Point p;         // 編譯錯誤！沒有預設建構函數！
//   // 原因：只要定義了「任何一個」建構函數，編譯器就不再自動生成預設建構函數

// ================================================================
// 重點七：綜合範例 —— Car 類別
// ================================================================

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
        cout << "  [建構] 創建預設汽車" << endl;
    }

    // 建構函數 2：只指定品牌和型號
    Car(string b, string m) {
        brand = b;
        model = m;
        year = 2024;
        mileage = 0.0;
        cout << "  [建構] 創建新車: " << brand << " " << model << endl;
    }

    // 建構函數 3：完整指定（帶驗證）
    Car(string b, string m, int y, double mi) {
        brand = b;
        model = m;

        // 年份驗證（1886 年是汽車發明的年份）
        if (y < 1886 || y > 2025) {
            cout << "    警告：年份不合法，使用 2024" << endl;
            year = 2024;
        } else {
            year = y;
        }

        // 里程驗證
        if (mi < 0) {
            cout << "    警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        } else {
            mileage = mi;
        }

        cout << "  [建構] 創建二手車: " << brand << " " << model
             << " (" << year << ")" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << " " << model
             << ", 里程: " << mileage << " km" << endl;
    }
};

int main() {
    cout << "=============================================" << endl;
    cout << "   第 13 課：建構函數（Constructor）基礎" << endl;
    cout << "=============================================" << endl;

    // --- 重點一 & 二：基本建構函數 ---
    cout << "\n【1】基本建構函數 & 帶參建構函數" << endl;

    Student s1;                        // 調用預設建構函數
    s1.print();

    Student s2("張三", 20, 3.8f);      // 調用帶參建構函數
    s2.print();

    Student s3("李四", 22, 3.5f);
    s3.print();

    // --- 重點三：建構函數的調用時機 ---
    cout << "\n【2】建構函數的調用時機" << endl;
    cout << "(全域對象在 main 之前建構，此處示範局部、區塊、動態)" << endl;

    Box localBox;                       // 局部對象：執行到這一行時建構

    {
        Box blockBox;                   // 區塊內對象：進入區塊時建構
        cout << "  區塊內..." << endl;
    }  // blockBox 在這裡離開作用域

    Box* heapBox = new Box();           // 動態對象：new 的時候建構
    delete heapBox;                     // 記得釋放

    // --- 重點四：建構函數重載 ---
    cout << "\n【3】建構函數重載" << endl;

    Rectangle r1;                       // 建構函數 1：無參數
    r1.print();

    Rectangle r2(5.0);                  // 建構函數 2：正方形
    r2.print();

    Rectangle r3(4.0, 6.0);            // 建構函數 3：指定寬高
    r3.print();

    Rectangle r4(3.0, 7.0, "紅色");     // 建構函數 4：指定寬高和顏色
    r4.print();

    // --- 重點五：建構函數中的資料驗證 ---
    cout << "\n【4】建構函數中的資料驗證" << endl;

    cout << "正常創建：" << endl;
    BankAccount a1("王五", 10000.0, "1234567890");
    a1.print();

    cout << "非法數據：" << endl;
    BankAccount a2("", -500.0, "123");
    a2.print();

    // --- 重點七：綜合範例 Car ---
    cout << "\n【5】綜合範例：Car 類別" << endl;

    Car c1;
    c1.print();

    Car c2("Toyota", "Camry");
    c2.print();

    Car c3("BMW", "M3", 2020, 35000.5);
    c3.print();

    Car c4("時光機", "DeLorean", 1800, -100.0);  // 測試非法數據
    c4.print();

    // --- 陷阱提醒 ---
    cout << "\n【6】常見陷阱提醒" << endl;
    cout << "  陷阱1: void Student() { } → 這不是建構函數，是普通函數！" << endl;
    cout << "  陷阱2: Student s(); → 這是函數宣告（Most Vexing Parse）！" << endl;
    cout << "         正確寫法: Student s; 或 Student s{};" << endl;
    cout << "  陷阱3: 定義了帶參建構函數後，預設建構函數消失！" << endl;
    cout << "         需手動加回，或使用 = default（下一課）" << endl;

    // --- 重點回顧 ---
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 建構函數 = 對象創建時自動調用的特殊成員函數" << endl;
    cout << "  2. 命名規則：函數名=類別名，沒有返回值" << endl;
    cout << "  3. 四種調用時機：全域/局部/區塊/動態(new)" << endl;
    cout << "  4. 可以重載：多個建構函數，參數不同" << endl;
    cout << "  5. 資料驗證：建構函數是確保對象合法性的第一道防線" << endl;
    cout << "  6. Most Vexing Parse: Student s() 是函數宣告！" << endl;
    cout << "  7. 定義任何建構函數後，預設建構函數不再自動生成" << endl;
    cout << "=============================================" << endl;

    return 0;
}
