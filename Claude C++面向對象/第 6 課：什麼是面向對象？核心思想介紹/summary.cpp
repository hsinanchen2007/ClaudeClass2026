/*
 * ================================================================
 * 【第 6 課：什麼是面向對象？核心思想介紹】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 面向對象程式設計（OOP）的誕生動機
 * 2. 封裝（Encapsulation）—— 隱藏資料、控制存取
 * 3. 繼承（Inheritance）—— 程式碼重用與擴展
 * 4. 多型（Polymorphism）—— 同一介面、不同行為
 * 5. 抽象（Abstraction）—— 只暴露必要細節
 * 6. OOP vs 程序式程式設計的比較
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
using namespace std;

// ================================================================
// 重點一：OOP 的誕生動機
// ================================================================
// 程序式（C 風格）：資料與函數分離，無法保護資料
// C++ OOP 目標：把資料與操作綁在一起，並控制存取
//
// C 語言風格的問題：
//   struct Student { char name[50]; float gpa; };
//   void setGpa(Student* s, float g) { s->gpa = g; }  // 任何人都可亂改
//
// OOP 解決方案：將資料「封裝」在類別中，只透過受控的介面修改

// ================================================================
// 重點二：封裝（Encapsulation）
// ================================================================
// 核心：把資料（member variables）和操作（member functions）封裝在 class 中
// private 成員外界不能直接存取；public 成員是對外介面
// 好處：可在 setter 中加入驗證邏輯，防止非法資料

class Student {
private:
    string name;   // 私有：外界無法直接存取
    float gpa;     // 私有：必須透過 setGpa 修改

public:
    // 建構函數
    Student(const string& n, float g) : name(n), gpa(0.0f) {
        setGpa(g);  // 使用自己的 setter，進行驗證
    }

    // Setter 帶有驗證邏輯 —— 封裝的精髓
    void setGpa(float newGpa) {
        if (newGpa >= 0.0f && newGpa <= 4.0f) {
            gpa = newGpa;
        } else {
            cout << "[錯誤] GPA 必須在 0.0~4.0 之間，拒絕設定：" << newGpa << endl;
        }
    }

    // Getter
    float getGpa() const { return gpa; }
    const string& getName() const { return name; }

    void print() const {
        cout << "學生：" << name << "，GPA：" << gpa << endl;
    }
};

// ================================================================
// 重點三：繼承（Inheritance）
// ================================================================
// 語法：class 子類 : public 父類 { ... };
// 子類「繼承」父類所有 public/protected 成員
// 目的：程式碼重用，避免重複撰寫相同邏輯
// 子類可以新增自己的屬性和方法

// 父類（基類）
class Animal {
public:
    string name;

    // 建構函數
    Animal(const string& n) : name(n) {}

    // 父類的通用行為
    void eat() {
        cout << name << " 正在吃東西" << endl;
    }

    void sleep() {
        cout << name << " 正在睡覺" << endl;
    }
};

// 子類：繼承 Animal 的一切，並新增自己的行為
class Dog : public Animal {
public:
    Dog(const string& n) : Animal(n) {}  // 呼叫父類建構函數

    // Dog 特有的行為
    void bark() {
        cout << name << " 汪汪叫！" << endl;
    }
};

class Cat : public Animal {
public:
    Cat(const string& n) : Animal(n) {}

    // Cat 特有的行為
    void meow() {
        cout << name << " 喵喵叫！" << endl;
    }
};

// ================================================================
// 重點四：多型（Polymorphism）
// ================================================================
// 「多型」= 同一個介面，根據實際物件類型呼叫不同實作
// 實現方式：virtual（虛函數）+ override（覆寫）
// 必須透過「指標或參考」才能觸發多型行為
//
// virtual 關鍵字的作用：
//   - 告訴編譯器「這個函數可能被子類覆寫」
//   - 呼叫時根據「實際物件類型」動態決定呼叫哪個版本（動態分派）
//
// override 關鍵字（C++11）：
//   - 明確標示「這個函數是覆寫父類的 virtual 函數」
//   - 若簽名不符合父類，編譯器會報錯（防止拼錯名稱）

class Shape {
public:
    string color;

    Shape(const string& c = "白色") : color(c) {}

    // virtual 讓子類可以覆寫此函數
    virtual double area() const {
        return 0.0;
    }

    // virtual 函數：顯示資訊（可被覆寫）
    virtual void describe() const {
        cout << color << "的形狀，面積 = " << area() << endl;
    }

    // 虛解構函數：透過基類指標刪除子類物件時，確保子類解構函數被呼叫
    virtual ~Shape() {}
};

class Circle : public Shape {
private:
    double radius;

public:
    Circle(double r, const string& c = "紅色")
        : Shape(c), radius(r) {}

    // override 明確標示覆寫
    double area() const override {
        return 3.14159 * radius * radius;
    }

    void describe() const override {
        cout << color << "的圓形，半徑=" << radius
             << "，面積=" << area() << endl;
    }
};

class Rectangle : public Shape {
private:
    double width, height;

public:
    Rectangle(double w, double h, const string& c = "藍色")
        : Shape(c), width(w), height(h) {}

    double area() const override {
        return width * height;
    }

    void describe() const override {
        cout << color << "的矩形，" << width << "x" << height
             << "，面積=" << area() << endl;
    }
};

// 多型的核心展示：接受 Shape* 的函數，可以處理任何子類
void printShapeInfo(const Shape* s) {
    // 根據實際物件類型，動態呼叫正確的 describe() 和 area()
    s->describe();
}

// ================================================================
// 重點五：抽象（Abstraction）
// ================================================================
// 抽象 = 隱藏複雜的實現細節，只暴露「需要知道的」介面
// 純虛函數（pure virtual）：= 0，使類別成為「抽象類別」
// 抽象類別不能被實例化，只能作為基類使用

class Vehicle {  // 抽象類別（包含純虛函數）
public:
    string brand;
    Vehicle(const string& b) : brand(b) {}

    // 純虛函數 = 0：子類「必須」實作這個函數
    virtual void start() = 0;
    virtual void stop() = 0;

    // 非純虛函數：有預設實作，子類可以選擇覆寫
    virtual void honk() {
        cout << brand << "：嗶嗶！" << endl;
    }

    virtual ~Vehicle() {}
};

class Car : public Vehicle {
public:
    Car(const string& b) : Vehicle(b) {}

    void start() override {
        cout << brand << " 汽車：引擎發動，轟轟轟！" << endl;
    }

    void stop() override {
        cout << brand << " 汽車：踩煞車，停止。" << endl;
    }
};

class Bicycle : public Vehicle {
public:
    Bicycle(const string& b) : Vehicle(b) {}

    void start() override {
        cout << brand << " 自行車：開始踩踏板！" << endl;
    }

    void stop() override {
        cout << brand << " 自行車：手捏煞車，停止。" << endl;
    }

    void honk() override {
        cout << brand << "：鈴鈴！" << endl;
    }
};

// ================================================================
// 重點六：OOP 四大特性總結對照
// ================================================================
//
// ┌──────────┬─────────────────────────────────────────────────────┐
// │ 特性     │ 說明                                                 │
// ├──────────┼─────────────────────────────────────────────────────┤
// │ 封裝     │ 資料 + 函數綁在一起；private/public 控制存取         │
// │ 繼承     │ 子類繼承父類成員；實現程式碼重用                     │
// │ 多型     │ virtual + override；同一介面，不同行為               │
// │ 抽象     │ 純虛函數；隱藏細節，只暴露必要介面                   │
// └──────────┴─────────────────────────────────────────────────────┘

int main() {
    cout << "========================================" << endl;
    cout << "   第 6 課：面向對象核心思想展示" << endl;
    cout << "========================================" << endl;

    // --- 封裝示範 ---
    cout << "\n【封裝 Encapsulation】" << endl;
    Student s1("張三", 3.8f);
    s1.print();
    s1.setGpa(-1.0f);  // 非法值，會被攔截
    s1.setGpa(3.9f);   // 合法值
    s1.print();

    // --- 繼承示範 ---
    cout << "\n【繼承 Inheritance】" << endl;
    Dog dog("旺財");
    Cat cat("咪咪");

    dog.eat();   // 繼承自 Animal
    dog.bark();  // Dog 自己的行為

    cat.eat();   // 繼承自 Animal
    cat.meow();  // Cat 自己的行為

    // --- 多型示範 ---
    cout << "\n【多型 Polymorphism】" << endl;
    // 用父類指標儲存子類物件 —— 多型的關鍵
    vector<Shape*> shapes;
    shapes.push_back(new Circle(5.0, "紅色"));
    shapes.push_back(new Rectangle(4.0, 6.0, "藍色"));
    shapes.push_back(new Circle(3.0, "綠色"));

    for (const Shape* s : shapes) {
        printShapeInfo(s);  // 動態分派：根據實際類型呼叫正確的函數
    }

    // 釋放記憶體（虛解構函數確保正確刪除）
    for (Shape* s : shapes) {
        delete s;
    }

    // --- 抽象示範 ---
    cout << "\n【抽象 Abstraction】" << endl;
    // Vehicle v("X");  // 編譯錯誤！抽象類別不能實例化
    Car car("Toyota");
    Bicycle bike("Giant");

    // 透過基類指標使用（多型 + 抽象）
    vector<Vehicle*> vehicles = { &car, &bike };
    for (Vehicle* v : vehicles) {
        v->start();
        v->honk();
        v->stop();
        cout << endl;
    }

    cout << "========================================" << endl;
    cout << " OOP 四大支柱：封裝、繼承、多型、抽象 " << endl;
    cout << "========================================" << endl;

    return 0;
}
