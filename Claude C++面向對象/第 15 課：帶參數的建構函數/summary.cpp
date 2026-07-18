/*
 * ================================================================
 * 【第 15 課：帶參數的建構函數】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 值傳遞 vs const 引用傳遞（推薦 const 引用）
 * 2. const 引用可以綁定字面值和臨時對象（普通引用不行）
 * 3. 參數名與成員變數同名的問題及四種解決方案
 * 4. 初始化列表中同名也不衝突
 * 5. 帶參建構函數的五種調用語法
 * 6. 大括號初始化禁止窄化轉換（Narrowing Conversion）
 * 7. 單參數建構函數的隱式轉換 & explicit 關鍵字
 * 8. 預設參數的進階用法
 * ================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// ================================================================
// 重點一：值傳遞 vs const 引用傳遞
// ================================================================
// 對於基本型別（int、double、bool 等）：直接值傳遞即可，開銷很小。
// 對於類別型別（string、vector 等）：推薦 const 引用傳遞，避免不必要的複製。
//
// ┌──────────────────┬─────────────────────────────────────┐
// │ 傳遞方式          │ 說明                                 │
// ├──────────────────┼─────────────────────────────────────┤
// │ 值傳遞 string n   │ 呼叫時複製一次 + 賦值時又複製一次     │
// │ const string& n   │ 不複製，只在賦值給成員時複製一次（推薦）│
// │ string& n         │ 不能接受字面值和臨時對象（不推薦）     │
// └──────────────────┴─────────────────────────────────────┘

class StudentValue {
private:
    string name;
    int age;

public:
    // 值傳遞：每次調用都複製 string
    StudentValue(string n, int a) {
        name = n;    // 這裡又複製了一次
        age = a;
    }

    void print() const {
        cout << "  [值傳遞] " << name << ", " << age << " 歲" << endl;
    }
};

class StudentRef {
private:
    string name;
    int age;

public:
    // const 引用傳遞（推薦）：不複製 string，只傳遞引用
    StudentRef(const string& n, int a) {
        name = n;    // 只在這裡複製一次
        age = a;
    }

    void print() const {
        cout << "  [const引用] " << name << ", " << age << " 歲" << endl;
    }
};

// ================================================================
// 重點二：const 引用可以綁定字面值和臨時對象
// ================================================================
// 普通引用（string&）不能綁定到字面值 "hello" 或臨時對象 string("temp")
// const 引用（const string&）可以！
//
// Demo(string& s)       → Demo d("hello");     // 編譯錯誤！
// Demo(const string& s) → Demo d("hello");     // OK！

class Demo {
public:
    string data;
    Demo(const string& s) { data = s; }
};

// ================================================================
// 重點三：參數名與成員變數同名的問題
// ================================================================
// 錯誤示範：
//   BadExample(string name, int age) {
//       name = name;   // 把參數 name 賦給參數 name 自己！成員沒被修改！
//       age = age;     // 同理！
//   }

// --- 解決方案 1：使用 this 指針 ---
class Solution_This {
private:
    string name;
    int age;

public:
    Solution_This(string name, int age) {
        this->name = name;   // this->name 是成員，name 是參數
        this->age = age;
    }

    void print() const {
        cout << "  [this] " << name << ", " << age << " 歲" << endl;
    }
};

// --- 解決方案 2A：參數加底線前綴 ---
class Solution_UnderscorePrefix {
private:
    string name;
    int age;

public:
    Solution_UnderscorePrefix(const string& _name, int _age) {
        name = _name;
        age = _age;
    }
};

// --- 解決方案 2B：成員變數加 m_ 前綴（很多公司採用）---
class Solution_MPrefix {
private:
    string m_name;
    int m_age;

public:
    Solution_MPrefix(const string& name, int age) {
        m_name = name;
        m_age = age;
    }
};

// --- 解決方案 2C：成員變數加底線後綴（Google C++ 風格指南）---
class Solution_Suffix {
private:
    string name_;
    int age_;

public:
    Solution_Suffix(const string& name, int age) {
        name_ = name;
        age_ = age;
    }
};

// --- 解決方案 3：使用初始化列表（最推薦！）---
// 初始化列表中，括號外是成員變數，括號內是參數，即使同名也不衝突！
class Solution_InitList {
private:
    string name;
    int age;

public:
    Solution_InitList(const string& name, int age)
        : name(name), age(age) { }   // 簡潔且不衝突！

    void print() const {
        cout << "  [初始化列表] " << name << ", " << age << " 歲" << endl;
    }
};

// ================================================================
// 重點四：帶參建構函數的五種調用語法
// ================================================================
//   語法 1：直接初始化（括號）     Point p1(3.0, 4.0);
//   語法 2：統一初始化（大括號）   Point p2{5.0, 6.0};
//   語法 3：拷貝初始化（等號）     Point p3 = Point(7.0, 8.0);
//   語法 4：等號 + 大括號          Point p4 = {9.0, 10.0};
//   語法 5：動態分配               Point* p5 = new Point(1.0, 2.0);

class Point {
private:
    double x, y;

public:
    Point(double x, double y) : x(x), y(y) {
        cout << "  建構 Point(" << x << ", " << y << ")" << endl;
    }

    void print() const {
        cout << "  (" << x << ", " << y << ")" << endl;
    }
};

// ================================================================
// 重點五：大括號初始化禁止窄化轉換
// ================================================================
// 小括號允許窄化轉換（如 double → int，截斷為整數部分）
// 大括號禁止窄化轉換（會產生編譯錯誤）
//
// Holder h1(3.14);     // OK：double → int，截斷為 3
// Holder h2{3.14};     // 編譯錯誤！禁止窄化
// Holder h3{int(3.14)};// OK：明確轉換

class Holder {
public:
    int value;
    Holder(int v) : value(v) { }
};

// ================================================================
// 重點六：單參數建構函數的隱式轉換 & explicit
// ================================================================
// 單參數建構函數默認允許隱式轉換：
//   Distance d = 50.0;       // double 隱式轉換為 Distance
//   showDistance(200.0);      // 函數參數中也會隱式轉換
//
// 用 explicit 禁止隱式轉換：
//   explicit SafeDistance(double m) : meters(m) { }
//   SafeDistance d = 50.0;    // 編譯錯誤！
//   SafeDistance d(50.0);     // OK：直接初始化

class Distance {
private:
    double meters;

public:
    // 沒有 explicit → 允許隱式轉換
    Distance(double m) : meters(m) {
        cout << "  建構 Distance: " << meters << " m" << endl;
    }

    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showDistance(Distance d) {
    d.print();
}

class SafeDistance {
private:
    double meters;

public:
    // explicit → 禁止隱式轉換
    explicit SafeDistance(double m) : meters(m) {
        cout << "  建構 SafeDistance: " << meters << " m" << endl;
    }

    void print() const {
        cout << "  距離: " << meters << " 公尺" << endl;
    }
};

void showSafeDistance(SafeDistance d) {
    d.print();
}

// ================================================================
// 重點七：預設參數的進階用法
// ================================================================
// 預設參數必須從右到左依次提供，不能跳過中間的參數。
// 一個典型用法：只有第一個參數是必需的，其餘都有合理預設值。

class HttpRequest {
private:
    string url;
    string method;
    int timeout;
    bool followRedirect;

public:
    // 只有 url 是必需的
    HttpRequest(const string& url,
                const string& method = "GET",
                int timeout = 30,
                bool followRedirect = true)
        : url(url), method(method), timeout(timeout),
          followRedirect(followRedirect) { }

    void print() const {
        cout << "  [" << method << "] " << url
             << " (timeout=" << timeout << "s"
             << ", redirect=" << (followRedirect ? "是" : "否")
             << ")" << endl;
    }
};

// ================================================================
// 綜合範例：遊戲角色
// ================================================================

class GameCharacter {
private:
    string name_;         // Google 風格：底線後綴
    string classType_;
    int level_;
    int hp_;
    int mp_;
    double attackPower_;

public:
    // explicit + const 引用 + 預設參數
    explicit GameCharacter(const string& name,
                           const string& classType = "戰士",
                           int level = 1)
        : name_(name), classType_(classType), level_(level)
    {
        // 根據職業設定基礎屬性
        if (classType_ == "戰士") {
            hp_ = 150 + level_ * 20;
            mp_ = 30 + level_ * 5;
            attackPower_ = 15.0 + level_ * 3.0;
        } else if (classType_ == "法師") {
            hp_ = 80 + level_ * 10;
            mp_ = 100 + level_ * 15;
            attackPower_ = 25.0 + level_ * 5.0;
        } else if (classType_ == "弓箭手") {
            hp_ = 100 + level_ * 12;
            mp_ = 50 + level_ * 8;
            attackPower_ = 20.0 + level_ * 4.0;
        } else {
            hp_ = 100 + level_ * 15;
            mp_ = 50 + level_ * 10;
            attackPower_ = 15.0 + level_ * 3.5;
        }
    }

    void printStatus() const {
        cout << "  ┌──────────────────────────┐" << endl;
        cout << "  │ " << name_ << " [" << classType_ << "]" << endl;
        cout << "  │ 等級: " << level_ << endl;
        cout << "  │ HP: " << hp_ << "  MP: " << mp_ << endl;
        cout << "  │ 攻擊力: " << attackPower_ << endl;
        cout << "  └──────────────────────────┘" << endl;
    }
};

// ================================================================
// 座標類別：展示 explicit 和窄化防護
// ================================================================
class Coordinate {
private:
    int x_, y_;

public:
    explicit Coordinate(int x, int y) : x_(x), y_(y) { }

    void print() const {
        cout << "  (" << x_ << ", " << y_ << ")" << endl;
    }
};

int main() {
    cout << "=============================================" << endl;
    cout << "   第 15 課：帶參數的建構函數" << endl;
    cout << "=============================================" << endl;

    // --- 重點一：值傳遞 vs const 引用 ---
    cout << "\n【1】值傳遞 vs const 引用傳遞" << endl;
    string myName = "張三";
    StudentValue sv(myName, 20);     // 值傳遞：複製兩次
    sv.print();
    StudentRef sr(myName, 20);       // const 引用：只複製一次
    sr.print();

    // --- 重點二：const 引用可綁定字面值 ---
    cout << "\n【2】const 引用可綁定字面值和臨時對象" << endl;
    Demo d1("直接傳字串");            // const 引用 OK！
    Demo d2(string("臨時字串"));      // const 引用 OK！
    cout << "  d1: " << d1.data << endl;
    cout << "  d2: " << d2.data << endl;

    // --- 重點三：參數名與成員同名的解決方案 ---
    cout << "\n【3】參數名與成員同名的解決方案" << endl;

    Solution_This st("王五", 25);
    st.print();

    Solution_InitList si("趙六", 30);
    si.print();

    cout << "  四種命名風格：" << endl;
    cout << "    A: 參數加底線前綴  → _name, _age" << endl;
    cout << "    B: 成員加 m_ 前綴  → m_name, m_age" << endl;
    cout << "    C: 成員加底線後綴  → name_, age_ (Google 風格)" << endl;
    cout << "    D: 使用 this 指針  → this->name = name" << endl;
    cout << "    最推薦：初始化列表  → : name(name), age(age)" << endl;

    // --- 重點四：五種調用語法 ---
    cout << "\n【4】帶參建構函數的五種調用語法" << endl;

    cout << "語法 1：直接初始化 Point p1(3.0, 4.0);" << endl;
    Point p1(3.0, 4.0);
    p1.print();

    cout << "語法 2：統一初始化 Point p2{5.0, 6.0};" << endl;
    Point p2{5.0, 6.0};
    p2.print();

    cout << "語法 3：拷貝初始化 Point p3 = Point(7.0, 8.0);" << endl;
    Point p3 = Point(7.0, 8.0);
    p3.print();

    cout << "語法 4：等號+大括號 Point p4 = {9.0, 10.0};" << endl;
    Point p4 = {9.0, 10.0};
    p4.print();

    cout << "語法 5：動態分配 Point* p5 = new Point(1.0, 2.0);" << endl;
    Point* p5 = new Point(1.0, 2.0);
    p5->print();
    delete p5;

    // --- 重點五：大括號禁止窄化轉換 ---
    cout << "\n【5】大括號初始化禁止窄化轉換" << endl;
    double pi = 3.14;
    Holder h1(pi);        // OK：double → int，截斷為 3
    // Holder h2{pi};     // 編譯錯誤！大括號禁止窄化
    Holder h3{3};          // OK：int → int，沒有窄化
    cout << "  h1.value = " << h1.value << " (double 3.14 截斷為 3)" << endl;
    cout << "  h3.value = " << h3.value << " (int 3，無窄化)" << endl;

    // --- 重點六：隱式轉換 & explicit ---
    cout << "\n【6】隱式轉換 & explicit" << endl;

    cout << "Distance（允許隱式轉換）：" << endl;
    Distance dist1(100.0);
    Distance dist2 = 50.0;     // 隱式轉換！
    showDistance(200.0);        // 函數參數隱式轉換！

    cout << "SafeDistance（explicit 禁止隱式轉換）：" << endl;
    SafeDistance sd1(100.0);              // OK：直接初始化
    // SafeDistance sd2 = 50.0;           // 編譯錯誤！explicit 禁止
    // showSafeDistance(200.0);           // 編譯錯誤！explicit 禁止
    showSafeDistance(SafeDistance(200.0)); // OK：明確轉換

    // --- 重點七：預設參數 ---
    cout << "\n【7】預設參數的進階用法" << endl;
    HttpRequest r1("https://example.com");
    r1.print();

    HttpRequest r2("https://api.example.com/data", "POST");
    r2.print();

    HttpRequest r3("https://slow-server.com", "GET", 120);
    r3.print();

    HttpRequest r4("https://redirect.com", "GET", 10, false);
    r4.print();

    // --- 綜合範例：遊戲角色 ---
    cout << "\n【8】綜合範例：遊戲角色" << endl;
    GameCharacter hero1("勇者小明");                   // 預設職業和等級
    GameCharacter hero2("暗影法師", "法師");           // 預設等級
    GameCharacter hero3("神射手", "弓箭手", 10);       // 全部指定

    hero1.printStatus();
    hero2.printStatus();
    hero3.printStatus();

    // --- explicit + 窄化防護 ---
    cout << "\n【9】Coordinate (explicit + 窄化防護)" << endl;
    Coordinate c1(10, 20);         // OK：直接初始化
    Coordinate c2{30, 40};         // OK：大括號初始化
    // Coordinate c3 = {50, 60};   // 錯誤！explicit 禁止拷貝列表初始化
    // Coordinate c4{3.7, 4.2};    // 錯誤！double→int 是窄化
    Coordinate c5{int(3.7), int(4.2)};  // OK：明確轉換
    c1.print();
    c2.print();
    c5.print();

    // --- 陣列中使用帶參建構函數 ---
    cout << "\n【10】陣列中使用帶參建構函數" << endl;
    GameCharacter party[3] = {
        GameCharacter("坦克", "戰士", 5),
        GameCharacter("治癒者", "法師", 3),
        GameCharacter("輸出手", "弓箭手", 7)
    };
    for (int i = 0; i < 3; i++) {
        party[i].printStatus();
    }

    // --- 重點回顧 ---
    cout << "\n=============================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 類別型別參數推薦用 const 引用傳遞，避免不必要的複製" << endl;
    cout << "  2. const 引用可以綁定字面值和臨時對象" << endl;
    cout << "  3. 參數同名問題：推薦用初始化列表 : name(name) 解決" << endl;
    cout << "  4. 五種調用語法：()、{}、= T()、= {}、new T()" << endl;
    cout << "  5. 大括號 {} 禁止窄化轉換，比小括號更安全" << endl;
    cout << "  6. explicit 禁止單參數建構的隱式轉換" << endl;
    cout << "  7. 預設參數從右到左提供，讓 API 更靈活" << endl;
    cout << "  8. Google 風格：成員加底線後綴 name_、age_" << endl;
    cout << "=============================================" << endl;

    return 0;
}
