/*
 * ================================================================
 * 【第 10 課：成員函數（Member Functions）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 類內定義（隱式 inline）vs 類外定義（使用 類別:: 作用域運算子）
 * 2. 成員函數的參數與返回值：各種組合
 * 3. 返回 *this 實現鏈式調用（method chaining）
 * 4. 函數重載（Overloading）：同名函數，不同參數
 * 5. 預設參數：從右往左設定，從右往左省略
 * 6. 成員函數互相調用：不需要任何前綴直接呼叫
 * 7. 隱藏的 this 指標：成員函數背後的機制
 * 8. 驗證 this 存在：印出 this 等於對象地址
 * 9. 成員函數 vs 普通函數的本質差異
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cmath>
using namespace std;


// ===== 重點一：類內定義 vs 類外定義 =====
// 說明：
// 類內定義（inline）：函數體直接寫在 class {} 裡，編譯器視為「隱式 inline」。
//   適合短函數（1~3 行），編譯器可能在呼叫處展開，減少函數調用開銷。
// 類外定義：在 class 外用「類別名::函數名」定義，適合長函數，保持 class 定義整潔。
// 實務建議：短函數寫類內，長函數寫類外，不需要刻意追求 inline。

class Rectangle {
public:
    double width = 0;
    double height = 0;

    // 類內定義（隱式 inline）—— 短函數放這裡
    double area() {
        return width * height;
    }

    // 類內宣告，類外定義 —— 長函數的聲明放這裡
    double perimeter();
    void scale(double factor);
    void print();
};

// 類外定義：必須寫 Rectangle:: 前綴
double Rectangle::perimeter() {
    return 2 * (width + height);
}

void Rectangle::scale(double factor) {
    width *= factor;
    height *= factor;
}

void Rectangle::print() {
    cout << "Rectangle(" << width << " x " << height << ")" << endl;
    cout << "  面積: " << area() << endl;       // 成員函數可以直接呼叫另一個成員函數
    cout << "  周長: " << perimeter() << endl;
}


// ===== 重點二：成員函數的參數與返回值 =====
// 說明：成員函數和普通函數一樣，可以有各種參數和返回值。
// 特別介紹：返回 *this 引用，可實現「鏈式調用（method chaining）」。
// *this 是指向當前對象的指標解引用，即「當前對象本身」。
// 鏈式調用：sh.appendChain("A").appendChain("B") 因為每次返回的都是 sh 本身。

class StringHelper {
public:
    string text = "";

    void clear() { text = ""; }                             // 無參數、無返回值

    void append(const string& suffix) { text += suffix; }  // 有參數、無返回值

    int length() { return (int)text.length(); }             // 無參數、有返回值

    char charAt(int index) {                                 // 有參數、有返回值
        if (index < 0 || index >= (int)text.length()) {
            cout << "錯誤：索引超出範圍" << endl;
            return '\0';
        }
        return text[index];
    }

    string substring(int start, int len) {
        if (start < 0 || start >= (int)text.length()) return "";
        return text.substr(start, len);
    }

    bool contains(const string& target) {                   // 返回 bool
        return text.find(target) != string::npos;
    }

    // 返回自身引用（鏈式調用的基礎）
    // *this 解引用 this 指標得到當前對象本身，返回引用使呼叫方可以繼續鏈接
    StringHelper& appendChain(const string& suffix) {
        text += suffix;
        return *this;   // 返回自己
    }
};


// ===== 重點三：函數重載（Function Overloading）=====
// 說明：同一個作用域內可以定義多個同名但參數列表不同的函數。
// 編譯器根據呼叫時傳入的參數型別和數量決定呼叫哪個版本。
// 這稱為「靜態多型（compile-time polymorphism）」。
// 注意：只有返回值不同不算重載！必須是參數型別或數量不同。

class Printer {
public:
    void print(int value) {
        cout << "整數: " << value << endl;
    }

    void print(double value) {
        cout << "浮點數: " << value << endl;
    }

    void print(const string& value) {
        cout << "字串: " << value << endl;
    }

    void print(int value, int width) {       // 相同名，不同參數數量
        cout << "整數(寬度 " << width << "): ";
        string s = to_string(value);
        for (int i = 0; i < width - (int)s.length(); i++) cout << " ";
        cout << s << endl;
    }

    // int print(int value);   // 錯誤！只有返回值不同，不算重載，編譯器無法區分
};


// ===== 重點四：帶預設參數的成員函數 =====
// 說明：函數參數可以指定預設值，呼叫時省略的參數使用預設值。
// 規則：預設值必須從「右邊」開始設定，不能跳過。
//       呼叫時，省略的參數也必須從「右邊」開始省略。
// 例：void foo(int a, int b = 10, int c = 20)
//   foo(1)       → a=1, b=10, c=20
//   foo(1, 2)    → a=1, b=2,  c=20
//   foo(1, 2, 3) → a=1, b=2,  c=3

class Logger {
public:
    string name = "APP";

    // level 有預設值 "INFO"，不提供時自動使用
    void log(const string& message, const string& level = "INFO") {
        cout << "[" << level << "] " << name << ": " << message << endl;
    }

    // ch 和 repeat 都有預設值
    void divider(char ch = '-', int repeat = 40) {
        for (int i = 0; i < repeat; i++) cout << ch;
        cout << endl;
    }
};


// ===== 重點五：成員函數調用成員函數 =====
// 說明：成員函數之間可以自由互相呼叫，直接使用函數名即可，不需要任何前綴。
// 呼叫機制：編譯器自動轉換為 this->函數名()。
// 好處：可以構建層次化的邏輯——高層函數調用低層函數，職責分離。

class TemperatureConverter {
public:
    double celsius = 0.0;

    double toFahrenheit() { return celsius * 9.0 / 5.0 + 32.0; }
    double toKelvin()     { return celsius + 273.15; }

    bool isBoiling()  { return celsius >= 100.0; }
    bool isFreezing() { return celsius <= 0.0; }

    string getState() {
        if (isFreezing()) return "固態（冰）";       // 呼叫自己的成員函數
        if (isBoiling())  return "氣態（水蒸氣）";
        return "液態（水）";
    }

    void report() {
        cout << "攝氏:" << celsius << "°C | 華氏:" << toFahrenheit()
             << "°F | 克式:" << toKelvin() << "K | " << getState() << endl;
    }
};


// ===== 重點六：隱藏的 this 指標 =====
// 說明：每個成員函數都有一個隱藏的 this 參數，指向呼叫函數的對象。
// 當你寫 d1.bark() 時，編譯器實際執行 Dog::bark(&d1)，把 &d1 傳給 this。
// 因此在 bark() 內部，name 等同於 this->name。
// 這就是為什麼 d1.bark() 和 d2.bark() 雖然執行同一份程式碼，卻能正確識別各自的成員。

class Dog {
public:
    string name;

    void bark() {
        // 你寫：name
        // 編譯器看到：this->name
        cout << name << " 汪汪！" << endl;
    }
};

// ===== 重點七：驗證 this 的存在 =====
// 說明：可以直接印出 this 來驗證它就是指向當前對象的指標。
// 對象的地址（&對象）與函數內部的 this 值完全一致。

class Widget {
public:
    int id = 0;

    void showAddress() {
        cout << "對象 id=" << id << " 的 this 地址: " << this << endl;
    }
};


// ===== 重點八：成員函數 vs 普通函數 =====
// 說明：兩者的本質差異——成員函數天生能透過 this 存取同對象的所有成員。
// 普通函數必須透過參數明確傳入所需的資料。
// 何時用哪種？當函數操作的是對象本身的資料，就應該寫成成員函數。

class Circle {
public:
    double radius = 0;

    // 成員函數：自動知道 radius，不需要傳入
    double area() {
        return 3.14159 * radius * radius;
    }
};

// 普通函數：必須手動傳入 radius，否則不知道要算哪個 circle 的面積
double circleArea(double radius) {
    return 3.14159 * radius * radius;
}


// ===== 綜合實戰：計算器類別（展示本課所有概念）=====
class Calculator {
public:
    double result = 0.0;
    int operationCount = 0;

    // 重載：接受 double 或 int 都能設初值
    void setValue(double val) {
        result = val;
        operationCount = 0;
        cout << "初始值設為: " << val << endl;
    }
    void setValue(int val) { setValue((double)val); }   // 呼叫 double 版本

    // 基本運算（每個函數都呼叫私有的 record 輔助函數）
    void add(double v)      { result += v; record("+", v); }
    void subtract(double v) { result -= v; record("-", v); }
    void multiply(double v) { result *= v; record("*", v); }

    bool divide(double v) {
        if (v == 0.0) { cout << "錯誤：不能除以零！" << endl; return false; }
        result /= v; record("/", v);
        return true;
    }

    // 進階運算：調用自己的 multiply（成員函數互調）
    void square()  { multiply(result); }
    void negate()  { multiply(-1); }

    void showResult() { cout << "結果: " << result << endl; }

    void reset() {
        result = 0.0;
        operationCount = 0;
        cout << "計算器已重置" << endl;
    }

private:
    // 私有輔助函數：外界不需要知道的內部邏輯
    void record(const string& op, double value) {
        operationCount++;
        cout << "  [#" << operationCount << "] " << op << " " << value
             << " → " << result << endl;
    }
};


int main() {
    cout << "===== 重點一：類內 vs 類外定義 =====" << endl;
    Rectangle r;
    r.width = 5.0; r.height = 3.0;
    r.print();
    r.scale(2.0);
    cout << "放大2倍後："; r.print();


    cout << "\n===== 重點二：參數/返回值 & 鏈式調用 =====" << endl;
    StringHelper sh;
    sh.append("Hello"); sh.append(", "); sh.append("World!");
    cout << "文字: " << sh.text << endl;
    cout << "長度: " << sh.length() << endl;
    cout << "charAt(0): " << sh.charAt(0) << endl;
    cout << "substring(7,5): " << sh.substring(7, 5) << endl;
    cout << "含 'World': " << (sh.contains("World") ? "是" : "否") << endl;

    cout << "--- 鏈式調用 ---" << endl;
    StringHelper sh2;
    sh2.appendChain("C++").appendChain(" is").appendChain(" awesome!");
    cout << sh2.text << endl;


    cout << "\n===== 重點三：函數重載 =====" << endl;
    Printer p;
    p.print(42);          // 呼叫 print(int)
    p.print(3.14);        // 呼叫 print(double)
    p.print("Hello");     // 呼叫 print(const string&)
    p.print(42, 10);      // 呼叫 print(int, int)


    cout << "\n===== 重點四：預設參數 =====" << endl;
    Logger logger;
    logger.name = "MyApp";
    logger.divider();                      // 全部使用預設值
    logger.log("程式啟動");               // level 使用預設值 "INFO"
    logger.log("連線失敗", "ERROR");      // 指定 level
    logger.divider('=', 20);


    cout << "\n===== 重點五：成員函數互調 =====" << endl;
    TemperatureConverter t;
    t.celsius = -10; t.report();
    t.celsius = 25;  t.report();
    t.celsius = 100; t.report();


    cout << "\n===== 重點六 & 七：this 指標 =====" << endl;
    Dog d1, d2;
    d1.name = "旺財"; d2.name = "小黑";
    d1.bark();   // this == &d1，所以 name 是 "旺財"
    d2.bark();   // this == &d2，所以 name 是 "小黑"

    Widget a, b;
    a.id = 1; b.id = 2;
    a.showAddress();
    b.showAddress();
    cout << "驗證 &a = " << &a << "（應與上方 id=1 的地址相同）" << endl;


    cout << "\n===== 重點八：成員函數 vs 普通函數 =====" << endl;
    Circle c;
    c.radius = 5.0;
    cout << "成員函數: " << c.area() << endl;           // 自動知道 radius
    cout << "普通函數: " << circleArea(c.radius) << endl; // 必須手動傳入


    cout << "\n===== 綜合實戰：計算器 =====" << endl;
    Calculator calc;
    calc.setValue(10);   // 重載：傳入 int
    calc.add(5);
    calc.multiply(3);
    calc.subtract(8);
    calc.divide(7);
    calc.divide(0);      // 錯誤測試
    calc.showResult();

    return 0;
}
