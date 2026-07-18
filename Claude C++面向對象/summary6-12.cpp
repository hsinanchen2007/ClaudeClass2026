/*
 * ================================================================
 *  【第 6～12 課總複習】summary6-12.cpp
 * ================================================================
 *  涵蓋課程：
 *    第  6 課：什麼是面向對象？核心思想介紹（封裝/繼承/多型/抽象）
 *    第  7 課：類別（class）的定義與語法
 *    第  8 課：對象（object）的創建與使用
 *    第  9 課：成員變數（Member Variables）
 *    第 10 課：成員函數（Member Functions）
 *    第 11 課：存取修飾符——public、private、protected
 *    第 12 課：struct 與 class 的差異
 *
 *  編譯：g++ -std=c++17 -Wall -Wextra -o summary6-12 summary6-12.cpp
 * ================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
using namespace std;

// ================================================================
// 第 6 課：OOP 四大支柱
// ================================================================
//
// 【封裝 Encapsulation】資料+函數綁在 class，private/public 控制存取
// 【繼承 Inheritance】  子類繼承父類成員，class 子 : public 父 {}
// 【多型 Polymorphism】 virtual + override，同一介面不同行為（需指標/引用）
// 【抽象 Abstraction】  純虛函數 = 0，使類別不可實例化，子類必須實作
// ================================================================

// --- 封裝：private 資料 + public 介面 + 驗證 ---
class Student_L6 {
    string name_;
    float gpa_ = 0.0f;
public:
    Student_L6(const string& n, float g) : name_(n) { setGpa(g); }
    void setGpa(float g) {
        if (g >= 0.0f && g <= 4.0f) gpa_ = g;
        else cout << "  [拒絕] GPA 需 0~4，收到: " << g << "\n";
    }
    void print() const { cout << "  " << name_ << " GPA=" << gpa_ << "\n"; }
};

// --- 繼承：子類繼承父類，並新增自己的行為 ---
class Animal_L6 {
public:
    string name;
    Animal_L6(const string& n) : name(n) {}
    void eat() { cout << "  " << name << " 正在吃東西\n"; }
};
class Dog_L6 : public Animal_L6 {
public:
    Dog_L6(const string& n) : Animal_L6(n) {}  // 呼叫父類建構函數
    void bark() { cout << "  " << name << " 汪汪！\n"; }
};

// --- 多型：virtual + override，透過基類指標呼叫正確的子類函數 ---
class Shape_L6 {
public:
    virtual double area() const { return 0; }
    virtual void describe() const { cout << "  形狀，面積=" << area() << "\n"; }
    virtual ~Shape_L6() {}   // 虛解構函數：確保透過基類指標刪除時正確析構
};
class Circle_L6 : public Shape_L6 {
    double r_;
public:
    Circle_L6(double r) : r_(r) {}
    double area() const override { return 3.14159 * r_ * r_; }
    void describe() const override { cout << "  圓形 r=" << r_ << " 面積=" << area() << "\n"; }
};
class Rect_L6 : public Shape_L6 {
    double w_, h_;
public:
    Rect_L6(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    void describe() const override { cout << "  矩形 " << w_ << "x" << h_ << " 面積=" << area() << "\n"; }
};

// --- 抽象：純虛函數 = 0，不可實例化 ---
class Vehicle_L6 {
public:
    string brand;
    Vehicle_L6(const string& b) : brand(b) {}
    virtual void start() = 0;   // 純虛函數：子類必須實作
    virtual ~Vehicle_L6() {}
};
class Car_L6 : public Vehicle_L6 {
public:
    Car_L6(const string& b) : Vehicle_L6(b) {}
    void start() override { cout << "  " << brand << " 引擎發動！\n"; }
};

// ================================================================
// 第 7 課：class 的定義與語法
// ================================================================
//
// 【語法】class Name { public: / private: / protected: ... };  ← 別忘分號！
// 【藍圖】class = 設計圖，object = 依圖建出的實體
// 【命名】PascalCase（如 BankAccount）
// 【預設】class 內未標存取修飾符 → private
// 【類內定義】短函數直接寫在 class {}（隱式 inline）
// 【類外定義】長函數只在 class 宣告，外面用 ClassName::funcName 實作
// 【類內初始化】C++11 起可在宣告時給預設值：int hp = 100;
// ================================================================

class Calc_L7 {
public:
    int add(int a, int b) { return a + b; }   // 類內定義
    int subtract(int a, int b);               // 類內只宣告
};
// 類外定義：必須加 Calc_L7:: 前綴
int Calc_L7::subtract(int a, int b) { return a - b; }

// ================================================================
// 第 8 課：對象的創建與使用
// ================================================================
//
// 【棧上創建】Dog d;         → 用 . 存取，離開作用域自動銷毀（最常用）
// 【堆上創建】Dog* p = new Dog(); → 用 -> 存取，必須 delete，否則記憶體洩漏
// 【每個對象獨立】a.count 和 b.count 互不影響
// 【sizeof】只計成員變數（含對齊填充），空類別 = 1 byte，函數不佔空間
// 【參數傳遞】
//   傳值       void f(Box b)        — 複製，少用
//   傳引用     void f(Box& b)       — 可修改原對象
//   傳const引用 void f(const Box& b) — 最推薦！不複製、不可改
// ================================================================

class Counter_L8 {
public:
    int count = 0;
    void increment() { count++; }
};

class Empty_L8 {};   // sizeof = 1

class WithData_L8 {
public:
    char a;    // 1 byte
    int  b;    // 4 bytes（前面 3 bytes 填充對齊）
    char c;    // 1 byte（後面 3 bytes 填充對齊）
    // 典型 sizeof = 12
};

// ================================================================
// 第 9 課：成員變數（Member Variables）
// ================================================================
//
// 【基本型別】int, double, bool, char → 不自動初始化，是垃圾值！必須手動給值
// 【class 型別】string, vector 等 → 有預設建構函數，自動初始化（安全）
// 【組合 Composition】類別包含另一個類別對象 → has-a 關係（如 Car 有 Engine）
// 【固定陣列】double data[2][2]; 可作成員，C++11 可類內初始化 = {}
// 【最佳實踐】一律使用類內初始化（C++11）給所有基本型別預設值
// ================================================================

class Engine_L9 {
public:
    int hp = 0;
    string fuel = "汽油";
    void start() { cout << "  " << hp << "hp " << fuel << " 引擎啟動\n"; }
};
class Car_L9 {
public:
    string brand;
    Engine_L9 engine;   // 組合：Car「擁有」Engine（has-a）
    void drive() { cout << "  " << brand << " → "; engine.start(); }
};

// ================================================================
// 第 10 課：成員函數（Member Functions）
// ================================================================
//
// 【類內 vs 類外】短函數放類內（inline），長函數類外定義（ClassName::func）
// 【鏈式調用】返回 *this 引用 → obj.a().b().c()
// 【函數重載】同名不同參數（型別/數量），只有返回值不同不算！
// 【預設參數】從右往左設定，呼叫時從右往左省略
// 【成員函數互調】直接呼叫，不需前綴（編譯器用 this-> 處理）
// 【隱藏 this】每個成員函數都有隱藏的 this 指標指向呼叫者
//   d1.bark() → 編譯器: Dog::bark(&d1)，this == &d1
// ================================================================

class Builder_L10 {
public:
    string text;
    // 鏈式調用：返回 *this
    Builder_L10& add(const string& s) { text += s; return *this; }
};

class Printer_L10 {
public:
    // 函數重載：同名不同參數
    void print(int v)           { cout << "  int: " << v << "\n"; }
    void print(double v)        { cout << "  double: " << v << "\n"; }
    void print(const string& v) { cout << "  string: " << v << "\n"; }
};

class Logger_L10 {
public:
    // 預設參數：從右側開始
    void log(const string& msg, const string& level = "INFO") {
        cout << "  [" << level << "] " << msg << "\n";
    }
};

// ================================================================
// 第 11 課：存取修飾符 public / private / protected
// ================================================================
//
// 【存取權限表】
//   存取者          public  protected  private
//   類別自己           V        V         V
//   子類               V        V         X
//   外部(main等)       V        X         X
//
// 【class 預設 private，struct 預設 public】（唯一語法差異）
// 【同類別不同對象】可互訪 private（private 是類別層級，非對象層級）
// 【最小權限原則】成員變數幾乎總是 private，透過 public getter/setter 存取
// ================================================================

class BankAcct_L11 {
    string owner_;
    double balance_ = 0;
    int txCount_ = 0;
    void record(const string& type, double amt) {   // private 輔助函數
        txCount_++;
        cout << "  [#" << txCount_ << "] " << type << " $" << amt << " → $" << balance_ << "\n";
    }
public:
    BankAcct_L11(const string& o, double init) : owner_(o), balance_(init > 0 ? init : 0) {}
    bool deposit(double amt) {
        if (amt <= 0) { cout << "  錯誤：金額需>0\n"; return false; }
        balance_ += amt; record("存款", amt); return true;
    }
    bool withdraw(double amt) {
        if (amt <= 0 || amt > balance_) { cout << "  錯誤：金額無效或餘額不足\n"; return false; }
        balance_ -= amt; record("提款", amt); return true;
    }
    double getBalance() const { return balance_; }
    void display() const {
        cout << "  " << owner_ << " 餘額=$" << balance_ << " 交易" << txCount_ << "筆\n";
    }
    // 同類別對象互訪 private（private 是類別層級）
    bool hasMore(const BankAcct_L11& other) const { return balance_ > other.balance_; }
};

// ================================================================
// 第 12 課：struct 與 class 的差異
// ================================================================
//
// 【唯一語法差異】預設存取：struct=public，class=private
// 【繼承預設】     struct 繼承預設 public，class 繼承預設 private
// 【功能完全相同】 struct 也能有 private、virtual、建構/解構函數
// 【慣例】
//   struct → 純資料（POD）、所有成員 public、無或少行為
//   class  → 有封裝（private資料+public介面）、完整 OOP
//
// 【對照表】
//   特性             struct      class
//   預設存取         public      private
//   預設繼承         public      private
//   可有 private     V           V
//   可有 virtual     V           V
//   慣例用途         POD 資料    OOP 設計
// ================================================================

struct RGB_L12 { uint8_t r, g, b; };             // struct 典型：純資料
struct Config_L12 { string host; int port; };     // struct 典型：設定資料

// struct 也能做完整 OOP（但慣例上用 class）
struct Point_L12 {
private:
    double x_, y_;
public:
    Point_L12(double x = 0, double y = 0) : x_(x), y_(y) {}
    double x() const { return x_; }
    double y() const { return y_; }
    void print() const { cout << "  Point(" << x_ << "," << y_ << ")\n"; }
};

// ================================================================
// 主程式
// ================================================================
int main() {
    cout << "================================================================\n";
    cout << "  C++ 面向對象 第 6～12 課 總複習\n";
    cout << "================================================================\n";

    // ============================================================
    // 第 6 課：OOP 四大支柱
    // ============================================================
    cout << "\n=== 第 6 課：OOP 四大支柱 ===\n";

    // 封裝
    cout << "\n[封裝] private 資料 + setter 驗證\n";
    Student_L6 s("張三", 3.8f);
    s.print();
    s.setGpa(-1.0f);   // 被攔截
    s.setGpa(3.9f);
    s.print();

    // 繼承
    cout << "\n[繼承] 子類繼承父類成員\n";
    Dog_L6 dog("旺財");
    dog.eat();    // 繼承自 Animal
    dog.bark();   // Dog 自己的

    // 多型
    cout << "\n[多型] virtual + override，透過基類指標動態分派\n";
    vector<Shape_L6*> shapes = { new Circle_L6(5), new Rect_L6(4, 6) };
    for (auto* sp : shapes) sp->describe();    // 動態呼叫正確版本
    for (auto* sp : shapes) delete sp;         // 虛解構函數確保正確釋放

    // 抽象
    cout << "\n[抽象] 純虛函數 = 0，子類必須實作\n";
    // Vehicle_L6 v("X");  // 編譯錯誤！抽象類別不可實例化
    Car_L6 car("Toyota");
    car.start();

    // ============================================================
    // 第 7 課：class 定義與語法
    // ============================================================
    cout << "\n=== 第 7 課：class 定義與語法 ===\n";

    // 類內 vs 類外定義
    Calc_L7 calc;
    cout << "  add(5,3)=" << calc.add(5, 3)             // 類內定義
         << " subtract(10,4)=" << calc.subtract(10, 4)  // 類外定義
         << "\n";

    // class 預設 private
    // class Foo { int x; };  → x 是 private，外部不能直接存取

    // 類內初始化（C++11）
    // class Player { int hp = 100; };  → 建立即有預設值，避免垃圾值

    // ============================================================
    // 第 8 課：對象的創建與使用
    // ============================================================
    cout << "\n=== 第 8 課：對象創建與使用 ===\n";

    // 棧 vs 堆
    Counter_L8 ca, cb;                 // 棧上：自動管理
    ca.increment(); ca.increment();
    cb.increment();
    cout << "  [棧] ca.count=" << ca.count << " cb.count=" << cb.count << " (獨立)\n";

    Counter_L8* cp = new Counter_L8(); // 堆上：手動管理
    cp->increment();                    // -> 存取
    cout << "  [堆] cp->count=" << cp->count << "\n";
    delete cp; cp = nullptr;            // 必須 delete！

    // sizeof
    cout << "  sizeof(Empty)=" << sizeof(Empty_L8) << " (空類=1byte)\n";
    cout << "  sizeof(WithData char+int+char)=" << sizeof(WithData_L8) << " (含對齊填充)\n";

    // 參數傳遞（概念說明）
    cout << "  [參數] 傳值=複製，傳引用=可改，const引用=最推薦\n";

    // ============================================================
    // 第 9 課：成員變數
    // ============================================================
    cout << "\n=== 第 9 課：成員變數 ===\n";

    // 基本型別不自動初始化（危險！）
    cout << "  int/double/bool/char → 不自動初始化 → 垃圾值！\n";
    cout << "  string/vector       → 自動初始化   → 安全\n";

    // 組合（Composition）
    Car_L9 myCar;
    myCar.brand = "Honda";
    myCar.engine.hp = 120;
    myCar.engine.fuel = "汽油";
    myCar.drive();   // car.engine.start() → 鏈式 . 存取內層對象

    // 類內初始化
    cout << "  [最佳] C++11 類內初始化：int hp = 100; 直接給預設值\n";

    // ============================================================
    // 第 10 課：成員函數
    // ============================================================
    cout << "\n=== 第 10 課：成員函數 ===\n";

    // 鏈式調用（return *this）
    Builder_L10 b;
    b.add("C++").add(" is").add(" awesome!");
    cout << "  [鏈式] " << b.text << "\n";

    // 函數重載
    cout << "  [重載] 同名不同參數：\n";
    Printer_L10 pr;
    pr.print(42);       // int 版
    pr.print(3.14);     // double 版
    pr.print("Hello");  // string 版

    // 預設參數
    cout << "  [預設參數]\n";
    Logger_L10 lg;
    lg.log("程式啟動");              // level = "INFO"（預設）
    lg.log("連線失敗", "ERROR");    // 指定 level

    // this 指標
    cout << "  [this] 成員函數有隱藏 this 指標 → d.bark() 等同 Dog::bark(&d)\n";

    // ============================================================
    // 第 11 課：存取修飾符
    // ============================================================
    cout << "\n=== 第 11 課：存取修飾符 ===\n";

    // 完整封裝的銀行帳戶
    BankAcct_L11 acc("陳信安", 5000);
    acc.display();
    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(99999);   // 餘額不足 → 被攔截
    // acc.balance_ = 0;   // 編譯錯誤！private
    acc.display();

    // 同類別對象互訪 private
    BankAcct_L11 acc2("小明", 8000);
    cout << "  陳信安 vs 小明: " << (acc.hasMore(acc2) ? "陳信安多" : "小明多") << "\n";

    // 權限表速查
    cout << "\n  [權限表]\n";
    cout << "  public:    類別/子類/外部 都可存取\n";
    cout << "  protected: 類別/子類 可存取，外部不行\n";
    cout << "  private:   只有類別自己可存取\n";
    cout << "  → 成員變數幾乎總是 private（最小權限原則）\n";

    // ============================================================
    // 第 12 課：struct vs class
    // ============================================================
    cout << "\n=== 第 12 課：struct vs class ===\n";

    // struct 預設 public
    RGB_L12 red = {255, 0, 0};
    cout << "  [struct] RGB=(" << (int)red.r << "," << (int)red.g << "," << (int)red.b << ")\n";

    Config_L12 cfg = {"localhost", 8080};
    cout << "  [struct] Config: " << cfg.host << ":" << cfg.port << "\n";

    // struct 也能有 private + 建構函數
    Point_L12 pt(3.5, 7.2);
    pt.print();
    // pt.x_ = 0;   // 編譯錯誤！struct 也能有 private

    cout << "\n  [差異速查]\n";
    cout << "  struct 預設 public  繼承預設 public  → 慣用於純資料(POD)\n";
    cout << "  class  預設 private 繼承預設 private → 慣用於完整OOP\n";
    cout << "  功能完全相同，差異僅在預設存取！\n";

    // ============================================================
    cout << "\n================================================================\n";
    cout << "  第 6～12 課總複習完成！\n";
    cout << "================================================================\n";

    return 0;
}

/*
 * ================================================================
 * 七課重點速查表
 * ================================================================
 *
 * 課次    主題                核心關鍵字
 * ─────  ──────────────────  ──────────────────────────────────────
 * 第 6課  OOP 核心思想        封裝/繼承/多型(virtual+override)/抽象(=0)
 * 第 7課  class 定義語法      class{}; 類內/類外定義 :: 類內初始化
 * 第 8課  對象創建使用        棧(.) 堆(new/->delete) sizeof 對齊 const引用
 * 第 9課  成員變數            基本型別不初始化=垃圾 string/vector安全 組合
 * 第10課  成員函數            重載 預設參數 鏈式調用(*this) 隱藏this
 * 第11課  存取修飾符          public/private/protected 最小權限原則
 * 第12課  struct vs class     預設存取不同 功能相同 struct=POD class=OOP
 *
 * ================================================================
 */
