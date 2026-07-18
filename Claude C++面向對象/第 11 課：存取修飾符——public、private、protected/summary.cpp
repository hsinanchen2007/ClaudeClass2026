/*
 * ================================================================
 * 【第 11 課：存取修飾符——public、private、protected】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 為什麼需要存取控制：防止外界隨意修改對象內部資料
 * 2. public：任何人都可以存取（對外介面）
 * 3. private：只有類別自己的成員函數可以存取（內部實現）
 * 4. protected：自己和子類可以存取（繼承時使用，第八階段詳述）
 * 5. class 預設 private，struct 預設 public（唯一語法差異）
 * 6. 存取權限表：類別自身 / 子類 / 外部的可見性
 * 7. 同一類別的不同對象可以互相存取彼此的 private 成員
 * 8. 多個存取修飾符區塊（語法合法，但不建議分散）
 * 9. 最小權限原則：成員變數幾乎總是 private
 * 10. 設計決策流程：何時用 public / protected / private
 * ================================================================
 */

#include <iostream>
#include <string>
using namespace std;


// ===== 重點一：沒有存取控制的危險 =====
// 說明：如果所有成員都是 public，外界可以隨意修改，導致資料不一致。
// 例：帳戶餘額可以被設成負數，帳戶主人名稱可以被清空——沒有任何防護！
// 這在小程式中可能不明顯，但在大型專案中是重大安全隱患。

class DangerousAccount {
public:
    string owner;
    double balance = 0.0;
    // 問題：任何地方都能做 acc.balance = -999999.0; 毫無防護
};


// ===== 重點二：public —— 完全公開（對外介面）=====
// 說明：public 成員可以被任何程式碼存取。
// 用途：對外公開的函數介面、常數、需要公開的資料。
// 比喻：商店的服務台，任何顧客都可以來使用。

class Light {
public:
    bool isOn = false;   // public 變數：可以被直接讀取和修改

    void toggle() { isOn = !isOn; }

    void show() {
        cout << "燈: " << (isOn ? "開" : "關") << endl;
    }
};


// ===== 重點三：private —— 完全私有（只有自己）=====
// 說明：private 成員只能被該類別自己的成員函數存取。
// 外部程式碼（包括 main）無法直接存取 private 成員。
// 比喻：保險箱裡的東西，只有保險箱本身（類別內部函數）能碰。
// 用途：需要保護的資料、內部輔助函數。

class Safe {
private:
    string password = "secret123";   // 外界無法存取
    double money = 50000.0;          // 外界無法存取

public:
    bool unlock(const string& input) {
        if (input == password) {     // 類別內部可以存取 private
            cout << "保險箱已開啟，裡面有 $" << money << endl;
            return true;
        } else {
            cout << "密碼錯誤！" << endl;
            return false;
        }
    }
    // 注意：mySafe.password 或 mySafe.money = 0 在外部是編譯錯誤！
};


// ===== 重點四：protected —— 對子類開放（繼承用）=====
// 說明：protected 介於 public 和 private 之間。
// 外界（main 等）不能存取，但子類的成員函數可以存取。
// 目前先理解概念，第八階段（繼承）時會大量使用。
// 比喻：家族的內部資訊，外人不知道，但家族成員（子類）可以知道。

class Animal {
protected:
    string species = "未知";   // 外界不可存取，但子類可以
    int legs = 0;

public:
    void showInfo() {
        cout << species << ", " << legs << " 條腿" << endl;
    }
};

class DogAnimal : public Animal {
public:
    void setup() {
        species = "犬科";   // 子類可以存取父類的 protected 成員
        legs = 4;
    }
};

class Spider : public Animal {
public:
    void setup() {
        species = "蛛形綱";
        legs = 8;
    }
};


// ===== 重點五：class vs struct 預設存取 =====
// 說明：這是 class 和 struct 在 C++ 中唯一的語法差異。
// class 裡未指定修飾符的成員預設是 private。
// struct 裡未指定修飾符的成員預設是 public。

class MyClass {
    int x = 10;   // 預設 private！外部不能直接存取
public:
    int getX() { return x; }
};

struct MyStruct {
    int x = 10;   // 預設 public！外部可以直接存取
};


// ===== 重點六：存取權限總結 =====
// 成員的可見性規則：
//
//  存取者                   | public | protected | private
//  類別自己的成員函數        |   V    |     V     |    V
//  子類的成員函數            |   V    |     V     |    X
//  外部程式碼（main 等）     |   V    |     X     |    X
//  friend 函數/類別          |   V    |     V     |    V
//
// V = 可存取，X = 不可存取


// ===== 重點七：同一類別的不同對象可以互相存取 private =====
// 說明：private 是「類別層級」的存取控制，不是「對象層級」的。
// 也就是說：同一個類別的成員函數，可以存取「任何同類型對象」的 private 成員。
// 理由：同一個類別的作者寫了所有成員函數，他知道如何正確使用自己的私有資料。
// 這個特性在實作「比較」、「複製」等功能時非常有用。

class Box {
private:
    double width = 0;
    double height = 0;

public:
    void setSize(double w, double h) { width = w; height = h; }
    double area() { return width * height; }

    // isLargerThan 是 Box 的成員函數
    // 所以它可以存取「任何 Box 對象」的 private 成員，包括 other.width
    bool isLargerThan(const Box& other) {
        return (width * height) > (other.width * other.height);
    }

    void show() {
        cout << width << " x " << height << " = " << area() << endl;
    }
};


// ===== 重點八：多個存取修飾符區塊（語法合法，不建議分散）=====
// 說明：一個類別中可以出現多個 public / private 區塊，語法上合法。
// 但不建議這樣做！應把所有 public 集中在一起，private 集中在一起，
// 這樣閱讀程式碼時一目瞭然。

class Demo {
public:
    void func1() { cout << "func1" << endl; }
private:
    int data1 = 0;
public:
    // 第二個 public 區塊——合法但不推薦
    void func2() { cout << "func2, data1=" << data1 << endl; }
private:
    int data2 = 0;
public:
    void func3() {
        cout << "func3, data1=" << data1 << ", data2=" << data2 << endl;
    }
};


// ===== 重點九：最小權限原則（設計原則）=====
// 說明：成員變數幾乎總是應該設為 private，只透過 public 函數存取。
// 這叫做「最小權限原則（Principle of Least Privilege）」。
// 好處：
//   1. 資料驗證：修改前可以檢查值是否合法
//   2. 可維護性：未來改內部儲存方式，外部介面不用改
//   3. 除錯方便：異常值只可能從 setter 進入，容易追蹤

class BadStudent {
public:
    string name;
    int age;       // 問題：可以設成 -5 或 999，沒有任何防護
    double gpa;    // 問題：可以設成 100.0，沒有任何防護
};

class GoodStudent {
public:
    // 受控的存取介面
    void setName(const string& n) {
        if (!n.empty()) name = n;
    }

    void setAge(int a) {
        if (a > 0 && a < 150) age = a;
        else cout << "無效年齡: " << a << endl;
    }

    void setGpa(double g) {
        if (g >= 0.0 && g <= 4.0) gpa = g;
        else cout << "無效 GPA: " << g << endl;
    }

    string getName() { return name; }
    int    getAge()  { return age; }
    double getGpa()  { return gpa; }

    void show() {
        cout << name << ", " << age << " 歲, GPA: " << gpa << endl;
    }

private:
    // 資料藏在 private，外界無法直接修改
    string name = "未命名";
    int age = 0;
    double gpa = 0.0;
};


// ===== 重點十：銀行帳戶完整設計（封裝示範）=====
// 說明：正確的封裝設計——資料 private，操作 public，輔助函數 private。
// 存款/提款必須經過驗證，外界無法直接修改 balance，保證資料完整性。

class BankAccount {
public:
    void init(const string& ownerName, const string& accId, double initialBalance) {
        owner = ownerName;
        accountId = accId;
        balance = (initialBalance >= 0) ? initialBalance : 0;
    }

    bool deposit(double amount) {
        if (amount <= 0) { cout << "錯誤：金額需大於0" << endl; return false; }
        balance += amount;
        addTransaction("存款", amount);
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0) { cout << "錯誤：金額需大於0" << endl; return false; }
        if (amount > balance) {
            cout << "錯誤：餘額不足（目前: $" << balance << "）" << endl;
            return false;
        }
        balance -= amount;
        addTransaction("提款", amount);
        return true;
    }

    double getBalance()       { return balance; }
    string getOwner()         { return owner; }
    string getAccountId()     { return accountId; }
    int    getTransactionCount() { return transactionCount; }

    void display() {
        cout << "================================" << endl;
        cout << "持有人: " << owner << " | 帳號: " << accountId << endl;
        cout << "餘額: $" << balance << " | 交易次數: " << transactionCount << endl;
        cout << "================================" << endl;
    }

private:
    // 資料完全保護——外界只能透過 public 函數存取
    string owner = "";
    string accountId = "";
    double balance = 0.0;
    int transactionCount = 0;

    // 私有輔助函數——內部邏輯，外界不需要知道
    void addTransaction(const string& type, double amount) {
        transactionCount++;
        cout << "[交易 #" << transactionCount << "] "
             << type << " $" << amount
             << " → 餘額: $" << balance << endl;
    }
};


int main() {
    cout << "===== 重點一：沒有存取控制的危險 =====" << endl;
    DangerousAccount da;
    da.owner = "陳信安";
    da.balance = 10000.0;
    da.balance = -999999.0;   // 任何人都能這樣做！
    da.owner = "";             // 帳戶名被清空？
    cout << "持有人: '" << da.owner << "', 餘額: " << da.balance << endl;


    cout << "\n===== 重點二：public（完全公開）=====" << endl;
    Light lamp;
    lamp.show();
    lamp.toggle();
    lamp.show();
    lamp.isOn = false;   // 可以直接存取 public 變數
    lamp.show();


    cout << "\n===== 重點三：private（完全私有）=====" << endl;
    Safe mySafe;
    mySafe.unlock("wrong");       // public 函數可以呼叫
    mySafe.unlock("secret123");
    // mySafe.password;           // 編譯錯誤！private 不能從外部存取
    // mySafe.money = 0;          // 編譯錯誤！


    cout << "\n===== 重點四：protected（子類可存取）=====" << endl;
    DogAnimal dog;
    dog.setup();
    dog.showInfo();

    Spider spider;
    spider.setup();
    spider.showInfo();
    // dog.species = "test";   // 編譯錯誤！外界不能存取 protected


    cout << "\n===== 重點五：class vs struct 預設存取 =====" << endl;
    MyClass mc;
    cout << "class: " << mc.getX() << endl;   // 必須透過 public 函數

    MyStruct ms;
    cout << "struct: " << ms.x << endl;        // 可以直接存取


    cout << "\n===== 重點七：同類別對象互訪 private =====" << endl;
    Box boxA, boxB;
    boxA.setSize(5, 4);   // 面積 20
    boxB.setSize(3, 8);   // 面積 24
    cout << "Box a: "; boxA.show();
    cout << "Box b: "; boxB.show();
    // boxA.isLargerThan(boxB) 內部可以存取 boxB.width（private）
    cout << (boxA.isLargerThan(boxB) ? "a 比 b 大" : "b 比 a 大（或一樣大）") << endl;


    cout << "\n===== 重點八：多個存取區塊（不推薦分散）=====" << endl;
    Demo demo;
    demo.func1();
    demo.func2();
    demo.func3();


    cout << "\n===== 重點九：最小權限原則（GoodStudent）=====" << endl;
    GoodStudent gs;
    gs.setName("陳信安");
    gs.setAge(25);
    gs.setGpa(3.8);
    gs.show();

    gs.setAge(-5);    // 被攔截
    gs.setGpa(5.0);   // 被攔截
    gs.show();         // 值沒變


    cout << "\n===== 重點十：銀行帳戶完整封裝 =====" << endl;
    BankAccount acc;
    acc.init("陳信安", "ACC-001", 5000.0);
    acc.display();
    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(10000);   // 餘額不足
    acc.deposit(-500);     // 無效金額
    cout << endl;
    acc.display();
    // acc.balance = 999999; // 編譯錯誤！private


    cout << "\n===== 存取修飾符使用原則速查 =====" << endl;
    cout << "成員變數  -> 幾乎總是 private（用 getter/setter 保護）" << endl;
    cout << "對外函數  -> public（這是類別的使用介面）" << endl;
    cout << "輔助函數  -> private（內部邏輯，外界不需要知道）" << endl;
    cout << "給子類的  -> protected（繼承時使用，第八階段詳述）" << endl;

    return 0;
}
