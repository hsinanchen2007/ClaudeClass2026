/*
 * ================================================================
 * 【第 12 課：struct 與 class 的差異】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. struct 與 class 的唯一語法差異：預設存取修飾符
 * 2. struct 可以使用完整的 OOP 功能（private、virtual、繼承）
 * 3. 慣例用法：struct 用於純資料、class 用於完整 OOP
 * 4. struct 的繼承預設為 public，class 預設為 private
 * 5. 何時用 struct，何時用 class
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cmath>
#include <cstdint>
using namespace std;

// ================================================================
// 重點一：唯一的語法差異 —— 預設存取修飾符
// ================================================================
// struct：成員預設為 public（C 語言相容）
// class ：成員預設為 private（OOP 安全性）
//
// 除此之外，struct 和 class 在 C++ 中「完全相同」！

struct StructExample {
    // 預設 public —— 不需要寫 public:
    int x;
    int y;

    void print() {  // 也是預設 public
        cout << "struct: x=" << x << ", y=" << y << endl;
    }
};

class ClassExample {
    // 預設 private —— 外界無法直接存取
    int x;
    int y;

public:  // 必須明確寫 public:
    ClassExample(int a, int b) : x(a), y(b) {}

    void print() {
        cout << "class: x=" << x << ", y=" << y << endl;
    }
};

// ================================================================
// 重點二：struct 可以使用完整的 OOP 功能
// ================================================================
// struct 不只是「資料容器」，它和 class 完全一樣強大
// 可以有：建構函數、解構函數、private/protected、virtual、繼承

struct Point {
private:              // struct 也可以有 private！
    double x_, y_;

public:
    // 建構函數
    Point(double x = 0, double y = 0) : x_(x), y_(y) {}

    // Getter
    double x() const { return x_; }
    double y() const { return y_; }

    // 成員函數
    double distanceTo(const Point& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        return sqrt(dx*dx + dy*dy);
    }

    // 虛函數（struct 也可以有！）
    virtual void describe() const {
        cout << "Point(" << x_ << ", " << y_ << ")" << endl;
    }

    virtual ~Point() {}
};

// struct 也可以繼承（預設 public 繼承）
struct ColorPoint : Point {       // struct 繼承預設是 public
    string color;

    ColorPoint(double x, double y, const string& c)
        : Point(x, y), color(c) {}

    void describe() const override {
        cout << color << "色的 Point(" << x() << ", " << y() << ")" << endl;
    }
};

// ================================================================
// 重點三：struct 與 class 的繼承預設差異
// ================================================================
// struct 繼承預設：public
// class  繼承預設：private
//
// struct Child : Base     等同於  struct Child : public Base
// class  Child : Base     等同於  class  Child : private Base

struct BaseStruct {
    int value = 42;
};

struct DerivedStruct : BaseStruct {  // 預設 public 繼承
    // value 可直接存取
};

class BaseClass {
public:
    int value = 42;
};

class DerivedClassPrivate : BaseClass {  // 預設 private 繼承
    // value 在外部無法存取（已變成 private）
};

// ================================================================
// 重點四：慣例用法（Convention）
// ================================================================
// 雖然語法上幾乎相同，業界有約定俗成的用法：
//
// 用 struct 的情況：
//   - 純資料集合（Plain Old Data, POD）
//   - 沒有或很少有函數
//   - 所有成員公開
//   - C 語言介面相容的資料結構
//
// 用 class 的情況：
//   - 有封裝（private 成員 + public 介面）
//   - 有繼承和多型
//   - 有複雜的建構/解構邏輯
//   - 完整的 OOP 設計

// 典型 struct 用法：純資料容器
struct RGB {
    uint8_t r, g, b;  // 全部 public，無需保護
};

struct Config {
    string hostname;
    int port;
    bool useSSL;
    int timeout;
};

// 典型 class 用法：有封裝的實體
class BankAccount {
private:
    string owner;
    double balance;
    string accountNumber;

public:
    BankAccount(const string& owner, const string& accNum)
        : owner(owner), balance(0.0), accountNumber(accNum) {}

    // 受保護的操作，帶有驗證
    bool deposit(double amount) {
        if (amount <= 0) return false;
        balance += amount;
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0 || amount > balance) return false;
        balance -= amount;
        return true;
    }

    double getBalance() const { return balance; }
    const string& getOwner() const { return owner; }
};

// ================================================================
// 重點五：完整對照表
// ================================================================
//
// ┌─────────────────┬──────────────┬──────────────┐
// │ 特性             │ struct        │ class         │
// ├─────────────────┼──────────────┼──────────────┤
// │ 預設存取修飾符   │ public        │ private       │
// │ 預設繼承方式     │ public        │ private       │
// │ 可有建構函數     │ ✓             │ ✓             │
// │ 可有解構函數     │ ✓             │ ✓             │
// │ 可有 private     │ ✓             │ ✓             │
// │ 可有 virtual     │ ✓             │ ✓             │
// │ 可以繼承         │ ✓             │ ✓             │
// │ 慣例用途         │ 純資料 POD   │ 完整 OOP      │
// └─────────────────┴──────────────┴──────────────┘

int main() {
    cout << "===========================================" << endl;
    cout << "   第 12 課：struct 與 class 的差異展示" << endl;
    cout << "===========================================" << endl;

    // --- 預設存取修飾符差異 ---
    cout << "\n【預設存取修飾符】" << endl;

    StructExample se;
    se.x = 10;   // OK：struct 預設 public
    se.y = 20;
    se.print();

    ClassExample ce(30, 40);
    // ce.x = 30;  // 錯誤！class 預設 private
    ce.print();

    // --- struct 完整 OOP 示範 ---
    cout << "\n【struct 也能用 OOP】" << endl;

    Point p1(3.0, 4.0);
    Point p2(0.0, 0.0);
    p1.describe();
    cout << "p1 到原點的距離：" << p1.distanceTo(p2) << endl;

    ColorPoint cp(1.0, 2.0, "紅");
    cp.describe();  // 多型：呼叫 ColorPoint::describe()

    // --- 繼承預設差異 ---
    cout << "\n【繼承預設存取差異】" << endl;

    DerivedStruct ds;
    ds.value = 99;   // OK：public 繼承，value 仍是 public
    cout << "struct 繼承，value = " << ds.value << endl;

    DerivedClassPrivate dcp;
    // dcp.value = 99;  // 錯誤！private 繼承，value 變成 private
    cout << "class 預設 private 繼承，外部無法存取 value" << endl;

    // --- 慣例用法 ---
    cout << "\n【慣例用法對比】" << endl;

    RGB red = {255, 0, 0};  // struct：直接初始化，簡潔
    cout << "RGB: (" << (int)red.r << ", " << (int)red.g << ", " << (int)red.b << ")" << endl;

    BankAccount account("王小明", "ACC-001");
    account.deposit(1000.0);
    account.withdraw(250.0);
    cout << account.getOwner() << " 的餘額：" << account.getBalance() << endl;
    bool ok = account.withdraw(2000.0);  // 超出餘額，失敗
    cout << "嘗試提取 2000：" << (ok ? "成功" : "失敗") << endl;

    cout << "\n=== 結論：struct 和 class 幾乎相同 ===" << endl;
    cout << "唯一差異：預設存取修飾符（struct=public，class=private）" << endl;
    cout << "慣例：struct 用於 POD 資料，class 用於 OOP 設計" << endl;

    return 0;
}
