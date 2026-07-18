/*
 * ================================================================
 * 【第 13～19 課總複習】summary13-19.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary13-19 summary13-19.cpp
 *
 * 涵蓋課程：
 *   第 13 課：建構函數（Constructor）基礎
 *   第 14 課：預設建構函數（Default Constructor）
 *   第 15 課：帶參數的建構函數（Parameterized Constructor）
 *   第 16 課：建構函數初始化列表（Member Initializer List）
 *   第 17 課：解構函數（Destructor）
 *   第 18 課：對象的生命週期（Object Lifetime）
 *   第 19 課：動態對象的創建與銷毀（new / delete）
 *
 * 每課重點速查：
 *   13: 建構函數語法、四種調用時機、重載、資料驗證、Most Vexing Parse
 *   14: 預設建構函數、= default vs 手動空建構、= delete、陣列需求、歧義陷阱
 *   15: const 引用傳遞、同名解決(this/m_/suffix_/init list)、五種調用語法、
 *       窄化防護、explicit、預設參數
 *   16: 初始化列表 vs 函數體賦值(一步 vs 兩步)、四種必須使用情況
 *       (const/引用/無預設建構成員/基類)、順序陷阱、效能對比
 *   17: ~ClassName 語法、LIFO 解構順序、RAII(動態記憶體/檔案/計時器)、
 *       記憶體洩漏、編譯器自動生成的解構函數、不要拋異常
 *   18: 四種存儲期(自動/靜態全域/靜態局部/動態)、for 每次迭代建構解構、
 *       延遲初始化(static local)、Static Initialization Order Fiasco、
 *       臨時對象生命週期(const& 延長)、懸空引用/野指標陷阱
 *   19: new/delete 語法、new vs malloc、new[]/delete[] 配對、
 *       bad_alloc/nothrow、四種洩漏場景、RAII、unique_ptr/make_unique
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cmath>
#include <memory>    // unique_ptr, make_unique
#include <chrono>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================================================================
// 第 13 課：建構函數（Constructor）基礎
// ================================================================
// ┌─────────────────┬──────────────────────────────────┐
// │ 特徵             │ 說明                              │
// ├─────────────────┼──────────────────────────────────┤
// │ 函數名           │ 必須與類別名完全相同              │
// │ 返回值           │ 沒有返回值，連 void 都不寫        │
// │ 調用時機         │ 對象創建時自動調用                │
// │ 可以重載         │ 一個類別可以有多個建構函數        │
// │ 存取權限         │ 通常是 public                     │
// └─────────────────┴──────────────────────────────────┘
//
// 四種調用時機：
// ┌──────────────┬───────────────────────────────┐
// │ 對象類型      │ 建構時機                       │
// ├──────────────┼───────────────────────────────┤
// │ 全域對象      │ main() 之前                    │
// │ 局部對象      │ 執行到宣告語句時                │
// │ 區塊內對象    │ 進入區塊，執行到宣告時          │
// │ 動態對象      │ new 運算子執行時                │
// └──────────────┴───────────────────────────────┘
//
// 三大陷阱：
//   1. void ClassName() { } → 不是建構函數，是普通函數！
//   2. ClassName obj(); → 函數宣告（Most Vexing Parse），不是對象創建！
//      正確寫法：ClassName obj; 或 ClassName obj{};
//   3. 定義了任何建構函數後，編譯器不再自動生成預設建構函數

class L13_Car {
private:
    string brand;
    int year;
    double mileage;

public:
    // 建構函數 1：預設建構
    L13_Car() : brand("未知"), year(2024), mileage(0) {}

    // 建構函數 2：重載——部分指定
    L13_Car(const string& b) : brand(b), year(2024), mileage(0) {}

    // 建構函數 3：重載——完整指定，帶驗證
    L13_Car(const string& b, int y, double mi) : brand(b) {
        year = (y >= 1886 && y <= 2026) ? y : 2024;      // 年份驗證
        mileage = (mi >= 0) ? mi : 0;                     // 里程驗證
        if (y != year || mi != mileage)
            cout << "    (警告：數據被修正)" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << ", " << mileage << " km" << endl;
    }
};

void demo_lesson13() {
    cout << "\n===== 第 13 課：建構函數基礎 =====" << endl;

    // 重載示範
    L13_Car c1;                             // 預設建構
    L13_Car c2("Toyota");                   // 部分指定
    L13_Car c3("BMW", 2020, 35000);         // 完整指定
    L13_Car c4("時光機", 1800, -100);       // 驗證修正
    c1.print(); c2.print(); c3.print(); c4.print();

    // 區塊內建構/解構時機
    {
        L13_Car block("區塊內");
        block.print();
    } // block 在此離開作用域

    // 動態對象
    L13_Car* heap = new L13_Car("動態");
    heap->print();
    delete heap;

    cout << "  陷阱提醒：" << endl;
    cout << "    Student s(); ← 是函數宣告（Most Vexing Parse）！" << endl;
    cout << "    正確：Student s; 或 Student s{};" << endl;
}


// ================================================================
// 第 14 課：預設建構函數（Default Constructor）
// ================================================================
// 定義：不需要傳入任何參數就能調用的建構函數
//   形式 1：MyClass() { }              → 無參數
//   形式 2：MyClass(int x = 0) { }    → 所有參數都有預設值
//
// 編譯器自動生成的預設建構函數：
// ┌────────────────────────────────────┬────────────────────────┐
// │ 成員類型                           │ 自動生成的行為          │
// ├────────────────────────────────────┼────────────────────────┤
// │ 基本型別（int, double 等）         │ 不初始化（垃圾值）      │
// │ 類別型別（string, vector 等）      │ 調用該類別的預設建構函數│
// └────────────────────────────────────┴────────────────────────┘
//
// = default vs 手動空建構 { } 的差異：
// ┌────────────────────┬───────────────────┬────────────────────┐
// │ 場景                │ = default          │ 手動寫 { }          │
// ├────────────────────┼───────────────────┼────────────────────┤
// │ T obj;              │ 基本型別不初始化   │ 基本型別不初始化     │
// │ T obj{};            │ 觸發值初始化，歸零 │ 只調用空建構，不歸零 │
// │ 語義                │ 平凡(trivial)建構  │ 使用者定義的建構     │
// └────────────────────┴───────────────────┴────────────────────┘
//
// = delete：明確禁止預設建構（表達設計意圖）
// 全預設參數的建構函數也算預設建構函數（注意歧義！）
// 對象陣列 T arr[N]; 需要預設建構函數

class L14_A { public: int value; L14_A() = default; };   // 平凡建構
class L14_B { public: int value; L14_B() { } };          // 使用者定義

// 全預設參數 → 也是預設建構函數
class L14_Color {
    int r, g, b;
public:
    L14_Color(int red = 0, int green = 0, int blue = 0)
        : r(red), g(green), b(blue) {}
    void print() const { cout << "  RGB(" << r << "," << g << "," << b << ")" << endl; }
};

// = delete：沒有連接資訊就不能創建
class L14_DbConn {
    string host; int port;
public:
    L14_DbConn() = delete;
    L14_DbConn(const string& h, int p) : host(h), port(p) {}
    void print() const { cout << "  " << host << ":" << port << endl; }
};

void demo_lesson14() {
    cout << "\n===== 第 14 課：預設建構函數 =====" << endl;

    // = default vs 手動空建構差異
    L14_A a1;       // value 是垃圾值
    L14_B b1;       // value 是垃圾值
    L14_A a2{};     // 值初始化 → value = 0（= default 保留平凡性）
    L14_B b2{};     // 只調用空建構 → value 可能不是 0
    cout << "  = default + {}: a2.value = " << a2.value << " (應為 0)" << endl;
    cout << "  手動 {} + {}: b2.value = " << b2.value << " (可能不為 0)" << endl;

    // 全預設參數
    L14_Color black;                   // (0,0,0)
    L14_Color red(255);                // (255,0,0)
    L14_Color custom(100, 200, 50);    // (100,200,50)
    black.print(); red.print(); custom.print();

    // = delete
    // L14_DbConn db;  // 編譯錯誤！
    L14_DbConn db("localhost", 5432);
    db.print();

    // 陣列需要預設建構函數
    L14_Color colors[3];  // 3 次調用預設建構
    cout << "  陣列 colors[0]: "; colors[0].print();

    cout << "  重點：= default 保留平凡性(triviality)，手動 { } 不保留" << endl;
    cout << "  注意：不能同時有無參建構 + 全預設參數建構（歧義！）" << endl;
}


// ================================================================
// 第 15 課：帶參數的建構函數
// ================================================================
// 傳遞方式推薦：
//   基本型別（int, double）→ 值傳遞
//   類別型別（string, vector）→ const 引用（避免無謂複製）
//   const 引用可以綁定字面值和臨時對象（普通引用不行）
//
// 參數同名問題的四種解決方案：
//   1. this->name = name;         // this 指針
//   2. m_name = name;             // m_ 前綴
//   3. name_ = name;              // 底線後綴（Google 風格）
//   4. : name(name) { }           // 初始化列表（最推薦！）
//
// 五種調用語法：
//   T obj(args);                  // 直接初始化
//   T obj{args};                  // 統一初始化（禁止窄化轉換！）
//   T obj = T(args);              // 拷貝初始化
//   T obj = {args};               // 等號 + 大括號
//   T* p = new T(args);           // 動態分配
//
// explicit：禁止單參數建構的隱式轉換
//   Distance d = 50.0;            → 允許（沒有 explicit）
//   explicit 後必須 SafeDistance d(50.0); 明確調用

class L15_Player {
    string name_;     // Google 風格底線後綴
    int level_;
public:
    // explicit + const 引用 + 預設參數
    explicit L15_Player(const string& name, int level = 1)
        : name_(name), level_(level) {}  // 初始化列表中同名不衝突
    void print() const { cout << "  " << name_ << " Lv." << level_ << endl; }
};

// explicit 對比
class L15_Dist {
    double m;
public:
    L15_Dist(double meters) : m(meters) {}
    void print() const { cout << "  " << m << " m" << endl; }
};

class L15_SafeDist {
    double m;
public:
    explicit L15_SafeDist(double meters) : m(meters) {}
    void print() const { cout << "  " << m << " m" << endl; }
};

void showDist(L15_Dist d) { d.print(); }

void demo_lesson15() {
    cout << "\n===== 第 15 課：帶參數的建構函數 =====" << endl;

    // 五種調用語法
    L15_Player p1("勇者", 10);         // 直接初始化
    L15_Player p2{"法師", 5};          // 統一初始化
    L15_Player p3 = L15_Player("盜賊", 7); // 拷貝初始化
    // L15_Player p4 = {"弓手", 3};    // 錯誤！explicit 禁止
    p1.print(); p2.print(); p3.print();

    // 隱式轉換 vs explicit
    L15_Dist d1 = 100.0;              // OK：隱式轉換
    showDist(200.0);                   // OK：隱式轉換
    L15_SafeDist sd1(100.0);           // OK：直接初始化
    // L15_SafeDist sd2 = 100.0;       // 錯誤！explicit 禁止

    // 窄化轉換防護
    // L15_Player bad{string("test"), 3.14};  // 錯誤！double→int 是窄化
    cout << "  {} 初始化禁止窄化轉換，比 () 更安全" << endl;
    cout << "  explicit 防止單參數隱式轉換，建議單參數建構都加 explicit" << endl;
}


// ================================================================
// 第 16 課：建構函數初始化列表（Member Initializer List）
// ================================================================
// 語法：ClassName(params) : member1(val1), member2(val2) { }
//
// 初始化(一步) vs 賦值(兩步)：
//   初始化列表 → 直接用參數建構成員（一步到位）
//   函數體賦值 → 先預設建構，再賦值（兩步，多一次操作）
//
// 四種必須使用初始化列表的情況：
// ┌──────────────────────────────────┬──────────────────────────────┐
// │ 情況                              │ 原因                          │
// ├──────────────────────────────────┼──────────────────────────────┤
// │ const 成員變數                    │ const 只能初始化，不能賦值    │
// │ 引用成員變數                      │ 引用必須在宣告時綁定          │
// │ 沒有預設建構函數的成員物件         │ 無法先預設建構再賦值          │
// │ 基類的建構函數（繼承時）           │ 基類必須在派生類之前建構      │
// └──────────────────────────────────┴──────────────────────────────┘
//
// 順序陷阱：初始化順序按成員「宣告順序」，不是列表「書寫順序」！
//   最佳實踐：書寫順序永遠和宣告順序一致，加 -Wall -Wextra 偵測
//
// C++11 類別內預設值 + 初始化列表覆蓋規則：
//   初始化列表有指定 → 用列表的值
//   初始化列表沒有   → 用類別內預設值
//   都沒有           → 基本型別垃圾值 / 類別型別調用預設建構

// 展示四種必須使用的情況
class L16_Engine {
    int hp; string fuel;
public:
    L16_Engine(int h, const string& f) : hp(h), fuel(f) {}
    void print() const { cout << hp << "HP(" << fuel << ")"; }
};

class L16_Vehicle {
    const int vin;              // ① const 成員
    ostream& log;               // ② 引用成員
    L16_Engine engine;          // ③ 無預設建構的成員
    string brand;

    // C++11 類別內預設值
    int speed = 0;
    bool running = false;

public:
    L16_Vehicle(int id, ostream& os, int hp, const string& fuel, const string& b)
        : vin(id),              // ① const → 必須用初始化列表
          log(os),              // ② 引用 → 必須用初始化列表
          engine(hp, fuel),     // ③ 無預設建構 → 必須用初始化列表
          brand(b)              // speed, running 使用類別內預設值
    {}

    void print() const {
        cout << "  [" << vin << "] " << brand << " ";
        engine.print();
        cout << " 速度:" << speed << " " << (running ? "運行中" : "停止") << endl;
    }
};

// 順序陷阱示範
class L16_OrderDemo {
    int length;   // 第 1 個宣告 → 先初始化
    int doubled;  // 第 2 個宣告 → 後初始化
public:
    // 正確：書寫順序和宣告順序一致
    L16_OrderDemo(int val) : length(val), doubled(length * 2) {}
    void print() const { cout << "  length=" << length << " doubled=" << doubled << endl; }
};

// ④ 基類建構（繼承時必須用初始化列表）
class L16_Animal {
    string species;
public:
    L16_Animal(const string& s) : species(s) {}
    string getSpecies() const { return species; }
};

class L16_Dog : public L16_Animal {
    string name;
public:
    L16_Dog(const string& n)
        : L16_Animal("犬科"),   // ④ 基類建構 → 必須用初始化列表
          name(n) {}
    void print() const { cout << "  " << name << " (" << getSpecies() << ")" << endl; }
};

void demo_lesson16() {
    cout << "\n===== 第 16 課：建構函數初始化列表 =====" << endl;

    // 四種必須使用的情況
    L16_Vehicle v(1001, cout, 300, "汽油", "BMW");
    v.print();

    // 基類建構
    L16_Dog dog("旺財");
    dog.print();

    // 順序陷阱
    L16_OrderDemo od(5);
    od.print();

    cout << "  效能：初始化列表(1步) > 函數體賦值(2步)" << endl;
    cout << "  規則：初始化順序按宣告順序，不是書寫順序！" << endl;
    cout << "  最佳實踐：永遠使用初始化列表，書寫順序=宣告順序" << endl;
}


// ================================================================
// 第 17 課：解構函數（Destructor）
// ================================================================
// 語法：~ClassName()，無返回值，無參數，不能重載，每個類別只能一個
//
// 建構 vs 解構對照：
// ┌──────────────┬─────────────────────┬─────────────────────┐
// │ 特徵          │ 建構函數             │ 解構函數             │
// ├──────────────┼─────────────────────┼─────────────────────┤
// │ 名稱          │ ClassName()          │ ~ClassName()         │
// │ 參數          │ 可以有（可重載）     │ 不能有（不可重載）   │
// │ 數量          │ 可以有多個           │ 只能有一個           │
// │ 調用時機      │ 對象創建時           │ 對象銷毀時           │
// │ 職責          │ 獲取資源/初始化      │ 釋放資源/清理        │
// └──────────────┴─────────────────────┴─────────────────────┘
//
// 解構順序與建構順序相反（LIFO，後建構先解構）
//
// RAII（Resource Acquisition Is Initialization）：
//   建構函數獲取資源 → 解構函數釋放資源 → 離開作用域自動清理
//   應用：動態記憶體、檔案、鎖、計時器
//
// 編譯器自動生成的解構函數：
//   - 基本型別(int) → 不做任何事
//   - 類別型別(string) → 調用成員的解構函數
//   - 裸指標(int*) → 不做任何事！不會 delete！→ 必須手動寫
//
// 解構函數中不要拋出異常！（try-catch 包住）

class L17_Tracker {
    string name;
    static int count;
public:
    L17_Tracker(const string& n) : name(n) {
        count++;
        cout << "  [+] " << name << " (存活:" << count << ")" << endl;
    }
    ~L17_Tracker() {
        count--;
        cout << "  [-] " << name << " (存活:" << count << ")" << endl;
    }
    static int getCount() { return count; }
};
int L17_Tracker::count = 0;

// RAII 示範：動態陣列
class L17_DynArray {
    int* data;
    int size;
public:
    L17_DynArray(int sz) : data(new int[sz]()), size(sz) {
        cout << "  [建構] 分配 " << size << " 個 int" << endl;
    }
    ~L17_DynArray() {
        delete[] data;
        cout << "  [解構] 釋放 " << size << " 個 int" << endl;
    }
    void set(int i, int v) { if (i >= 0 && i < size) data[i] = v; }
    int get(int i) const { return (i >= 0 && i < size) ? data[i] : -1; }
};

// RAII 示範：自動計時器
class L17_ScopedTimer {
    string task;
    chrono::high_resolution_clock::time_point start;
public:
    L17_ScopedTimer(const string& t) : task(t),
        start(chrono::high_resolution_clock::now()) {}
    ~L17_ScopedTimer() {
        auto ms = chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start).count();
        cout << "  [計時] " << task << ": " << ms << " ms" << endl;
    }
};

void demo_lesson17() {
    cout << "\n===== 第 17 課：解構函數 =====" << endl;

    // LIFO 解構順序 + static 追蹤
    {
        L17_Tracker a("Alpha");
        L17_Tracker b("Beta");
        cout << "  --- 即將離開區塊 ---" << endl;
    } // Beta 先解構，Alpha 後解構（LIFO）

    // RAII：動態陣列自動釋放
    {
        L17_DynArray arr(5);
        arr.set(0, 42);
        cout << "  arr[0] = " << arr.get(0) << endl;
    } // 自動 delete[]

    // RAII：自動計時器
    {
        L17_ScopedTimer timer("模擬計算");
        volatile int sum = 0;
        for (int i = 0; i < 1000000; i++) sum += i;
    } // 自動印出耗時

    cout << "  重點：裸指標成員不會被自動 delete → 必須自己寫解構函數" << endl;
    cout << "  解構函數中不要拋異常（用 try-catch 包住）" << endl;
}


// ================================================================
// 第 18 課：對象的生命週期（Object Lifetime）
// ================================================================
// 四種存儲期：
// ┌──────────┬────────────┬────────────────┬──────────────────┐
// │ 存儲期    │ 宣告位置    │ 誕生            │ 死亡              │
// ├──────────┼────────────┼────────────────┼──────────────────┤
// │ 自動(棧)  │ 函數/區塊內 │ 執行到宣告時    │ 離開作用域時      │
// │ 靜態(全域)│ 函數外      │ main() 之前     │ main() 之後       │
// │ 靜態(局部)│ 函數內static│ 第一次執行到時  │ main() 之後       │
// │ 動態(堆)  │ new 創建    │ new 時          │ delete 時         │
// └──────────┴────────────┴────────────────┴──────────────────┘
//
// for 迴圈：每次迭代都是完整的 建構 → 使用 → 解構 週期
//
// 靜態局部對象（Lazy Initialization）：
//   只在第一次執行到時初始化，之後不再重複
//   C++11 保證線程安全（Magic Statics）
//   → 單例模式基礎
//
// Static Initialization Order Fiasco：
//   不同 .cpp 的全域對象初始化順序未定義！
//   解決：用函數包裝靜態局部對象
//
// 臨時對象生命週期：
//   Temp("x").show();          → 語句結束時死亡
//   Temp t = Temp("x");        → 和 t 的生命週期相同
//   const Temp& r = Temp("x"); → 延長到 r 離開作用域
//
// 懸空引用/野指標陷阱：
//   不要返回局部對象的引用 → Dangling Reference
//   不要返回局部對象的地址 → Dangling Pointer
//   安全做法：返回值（副本）或用 new 分配

class L18_Probe {
    string name;
public:
    L18_Probe(const string& n) : name(n) { cout << "  [+] " << name << endl; }
    ~L18_Probe() { cout << "  [-] " << name << endl; }
    void work() const { cout << "  [=] " << name << endl; }
};

// 延遲初始化示範
class L18_Singleton {
    string data;
public:
    L18_Singleton(const string& d) : data(d) {
        cout << "  [Singleton建構] " << data << endl;
    }
    void use() const { cout << "  [Singleton使用] " << data << endl; }
};

L18_Singleton& getSingleton() {
    static L18_Singleton instance("唯一實例");  // 只初始化一次
    return instance;
}

// Static Init Order Fiasco 解決方案
class L18_Config {
    int maxConn;
public:
    L18_Config() : maxConn(100) {}
    int getMax() const { return maxConn; }
};

L18_Config& getConfig() {
    static L18_Config cfg;
    return cfg;
}

void demo_lesson18() {
    cout << "\n===== 第 18 課：對象的生命週期 =====" << endl;

    // 嵌套作用域 LIFO
    cout << "--- 嵌套作用域 LIFO ---" << endl;
    {
        L18_Probe a("外層");
        {
            L18_Probe b("內層");
        } // b 先死
    } // a 後死

    // for 迴圈：每次迭代完整建構→解構
    cout << "\n--- for 迴圈：每次迭代 建構→解構 ---" << endl;
    for (int i = 0; i < 2; i++) {
        L18_Probe p("迴圈#" + to_string(i));
        p.work();
    }

    // 延遲初始化（只初始化一次）
    cout << "\n--- 延遲初始化（static local）---" << endl;
    getSingleton().use();  // 第一次：建構 + 使用
    getSingleton().use();  // 第二次：只使用（不再建構）

    // 臨時對象生命週期
    cout << "\n--- 臨時對象生命週期 ---" << endl;
    L18_Probe("臨時物件").work();   // 語句結束即死亡
    cout << "  (臨時物件已死亡)" << endl;

    const L18_Probe& ref = L18_Probe("const引用續命");
    ref.work();
    cout << "  (ref 綁定的臨時物件還活著)" << endl;
    // 臨時物件在 ref 離開作用域時才死亡

    cout << "\n  陷阱：不要返回局部對象的引用(懸空引用)或地址(野指標)" << endl;
    cout << "  解決：返回值(副本) 或 new + delete" << endl;
    cout << "  全域初始化順序陷阱 → 用函數包裝 static 局部對象解決" << endl;
}


// ================================================================
// 第 19 課：動態對象的創建與銷毀（new / delete）
// ================================================================
// new  做兩件事：① 分配記憶體  ② 調用建構函數
// delete 做兩件事：① 調用解構函數  ② 釋放記憶體
//
// new vs malloc：
// ┌───────────┬──────────────────────────┬─────────────────────┐
// │           │ new / delete             │ malloc / free        │
// ├───────────┼──────────────────────────┼─────────────────────┤
// │ 調用建構   │ 是                       │ 否！                 │
// │ 調用解構   │ 是（delete 時）          │ 否！（free 時）      │
// │ 返回型別   │ 正確的指標型別            │ void*（需強轉）      │
// │ 失敗行為   │ 拋出 bad_alloc           │ 返回 NULL            │
// └───────────┴──────────────────────────┴─────────────────────┘
//
// new[] / delete[] 配對：
//   new T[n]  → delete[] p;    // 正確
//   new T     → delete p;      // 正確
//   混用 → 未定義行為！
//
// delete nullptr 安全，double delete 是未定義行為
// 好習慣：delete 後設為 nullptr
//
// 記憶體洩漏四場景：
//   1. 忘記 delete
//   2. 覆蓋指標（丟失原地址）
//   3. 提前 return（跳過 delete）
//   4. 異常中斷（跳過 delete）
//
// RAII 解決一切：建構獲取 → 解構釋放 → 不管怎麼離開都安全
// 現代 C++：unique_ptr / make_unique 取代裸 new/delete

class L19_Weapon {
    string name;
    int damage;
    int* durability;
public:
    L19_Weapon(const string& n, int dmg, int dur)
        : name(n), damage(dmg), durability(new int(dur)) {
        cout << "  [鍛造] " << name << " (攻擊:" << damage
             << " 耐久:" << *durability << ")" << endl;
    }
    ~L19_Weapon() {
        cout << "  [銷毀] " << name << endl;
        delete durability;
    }
    void use() {
        if (*durability > 0) {
            *durability -= 10;
            cout << "  使用 " << name << " (耐久:" << *durability << ")" << endl;
        } else {
            cout << "  " << name << " 已損壞！" << endl;
        }
    }
};

void demo_lesson19() {
    cout << "\n===== 第 19 課：new / delete =====" << endl;

    // 基本語法
    cout << "--- 基本型別 ---" << endl;
    int* p1 = new int(42);
    int* p2 = new int{100};
    cout << "  *p1 = " << *p1 << ", *p2 = " << *p2 << endl;
    delete p1; delete p2;

    // 動態陣列
    cout << "\n--- 動態陣列 ---" << endl;
    int* arr = new int[5]{10, 20, 30, 40, 50};
    cout << "  arr: ";
    for (int i = 0; i < 5; i++) cout << arr[i] << " ";
    cout << endl;
    delete[] arr;  // 必須用 delete[] 對應 new[]

    // 裸指標方式（手動管理）
    cout << "\n--- 裸指標（手動管理）---" << endl;
    {
        L19_Weapon* sword = new L19_Weapon("鐵劍", 25, 50);
        sword->use();
        sword->use();
        delete sword;  // 必須手動 delete
    }

    // unique_ptr 方式（自動管理）
    cout << "\n--- unique_ptr（自動管理）---" << endl;
    {
        auto bow = make_unique<L19_Weapon>("長弓", 20, 40);
        bow->use();
        // 不需要 delete！離開作用域自動清理
    }

    // 異常安全示範
    cout << "\n--- 異常安全（unique_ptr）---" << endl;
    try {
        auto staff = make_unique<L19_Weapon>("法杖", 35, 30);
        staff->use();
        throw runtime_error("戰鬥中斷！");
    } catch (const exception& e) {
        cout << "  異常: " << e.what() << " (法杖已自動清理)" << endl;
    }

    // delete nullptr 安全
    int* safe = nullptr;
    delete safe;  // 完全安全

    cout << "\n  記憶體洩漏四場景：忘記/覆蓋指標/提前return/異常" << endl;
    cout << "  RAII 解決一切：建構獲取→解構釋放→自動管理" << endl;
    cout << "  現代 C++：用 unique_ptr/make_unique 取代裸 new/delete" << endl;
}


// ================================================================
// 主程式
// ================================================================
int main() {
    cout << "============================================================" << endl;
    cout << "  第 13～19 課總複習：建構函數 → 解構函數 → 生命週期 → new/delete" << endl;
    cout << "============================================================" << endl;

    demo_lesson13();   // 建構函數基礎
    demo_lesson14();   // 預設建構函數
    demo_lesson15();   // 帶參數的建構函數
    demo_lesson16();   // 初始化列表
    demo_lesson17();   // 解構函數
    demo_lesson18();   // 對象的生命週期
    demo_lesson19();   // new / delete

    // ================================================================
    // 七課速查表
    // ================================================================
    cout << "\n============================================================" << endl;
    cout << "七課速查表" << endl;
    cout << "============================================================" << endl;
    cout << "第13課 建構函數基礎：" << endl;
    cout << "  - 函數名=類別名，無返回值，自動調用" << endl;
    cout << "  - 可重載，帶驗證是第一道防線" << endl;
    cout << "  - 陷阱：Most Vexing Parse, 定義建構後預設建構消失" << endl;
    cout << "第14課 預設建構函數：" << endl;
    cout << "  - = default 保留平凡性，T obj{} 會歸零" << endl;
    cout << "  - = delete 明確禁止預設建構" << endl;
    cout << "  - 陣列 T arr[N] 需要預設建構函數" << endl;
    cout << "第15課 帶參建構函數：" << endl;
    cout << "  - 類別型別參數推薦 const&" << endl;
    cout << "  - 同名解決最推薦初始化列表 : name(name)" << endl;
    cout << "  - explicit 禁止隱式轉換，{} 禁止窄化" << endl;
    cout << "第16課 初始化列表：" << endl;
    cout << "  - 一步到位 vs 函數體賦值兩步" << endl;
    cout << "  - 四必須：const/引用/無預設建構成員/基類" << endl;
    cout << "  - 順序按宣告順序，書寫順序要一致" << endl;
    cout << "第17課 解構函數：" << endl;
    cout << "  - ~ClassName()，無參數，不能重載" << endl;
    cout << "  - LIFO 解構順序，RAII 自動管理資源" << endl;
    cout << "  - 裸指標必須手動 delete，不要拋異常" << endl;
    cout << "第18課 對象生命週期：" << endl;
    cout << "  - 四種存儲期：自動/靜態全域/靜態局部/動態" << endl;
    cout << "  - static local 延遲初始化，C++11 線程安全" << endl;
    cout << "  - 不要返回局部對象的引用或地址" << endl;
    cout << "第19課 new/delete：" << endl;
    cout << "  - new=分配+建構，delete=解構+釋放" << endl;
    cout << "  - new[]/delete[] 必須配對，不能混用" << endl;
    cout << "  - unique_ptr/make_unique 取代裸 new/delete" << endl;
    cout << "============================================================" << endl;

    return 0;
}
