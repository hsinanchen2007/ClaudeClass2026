/*
 * ================================================================
 * 【第 16 課：建構函數初始化列表（Member Initializer List）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 初始化列表 vs 函數體賦值的本質區別（初始化=一步 vs 賦值=兩步）
 * 2. 必須使用初始化列表的四種情況：
 *    - const 成員變數（不能賦值只能初始化）
 *    - 引用成員變數（必須在宣告時綁定）
 *    - 沒有預設建構函數的成員物件
 *    - 基類的建構函數（繼承時）
 * 3. 初始化順序陷阱：按成員宣告順序，不是列表書寫順序
 * 4. 危險的順序依賴（DangerousOrder vs SafeOrder）
 * 5. 初始化列表中可以使用表達式（Circle 面積計算、FullName 字串串接）
 * 6. C++11 類別內預設值配合初始化列表（GameConfig 覆蓋規則）
 * 7. 效能對比：函數體賦值=預設建構+賦值（兩步），初始化列表=直接建構（一步）
 * 8. 綜合範例：RPG 武器系統（Rarity + Weapon）
 * ================================================================
 */

#include <iostream>
#include <string>
#include <cmath>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================================================================
// 重點一：初始化列表 vs 函數體賦值的本質區別
// ================================================================
// 函數體內賦值 = 先預設建構（第一步），再賦值（第二步）→ 兩步
// 初始化列表   = 直接用參數建構（一步到位）→ 一步
//
// 用 C 語言的概念來理解：
//   int x = 10;    ← 初始化（宣告的同時給值）→ 初始化列表
//   int y;
//   y = 10;        ← 賦值（先宣告再給值）    → 函數體賦值
//
// ┌───────────────────────────────────────────────────────────┐
// │ 函數體內賦值（兩步）：                                      │
// │   成員預設建構 → name = ""    → name = "Hello" （賦值）    │
// │                                                           │
// │ 初始化列表（一步）：                                        │
// │   直接用參數建構 → name("Hello")  （一步到位）              │
// └───────────────────────────────────────────────────────────┘

// --- 方式 A：函數體內賦值（兩步）---
class DemoAssign {
private:
    string name;
    int value;

public:
    DemoAssign(const string& n, int v) {
        // 進入函數體之前，name 已經被預設建構為空字串（第一步）
        cout << "  [賦值方式] 賦值前 name = [" << name << "]（已被預設建構）" << endl;
        name = n;     // 賦值（第二步）
        value = v;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

// --- 方式 B：初始化列表（一步）---
class DemoInitList {
private:
    string name;
    int value;

public:
    DemoInitList(const string& n, int v)
        : name(n), value(v)    // 直接用 n 來建構 name，一步到位
    {
        cout << "  [初始化列表] name 直接就是 [" << name << "]（一步完成）" << endl;
    }

    void print() const {
        cout << "  name = " << name << ", value = " << value << endl;
    }
};

// ================================================================
// 重點二：必須使用初始化列表的四種情況
// ================================================================
// ┌──────────────────────────────────┬──────────────────────────────┐
// │ 情況                              │ 原因                          │
// ├──────────────────────────────────┼──────────────────────────────┤
// │ const 成員變數                    │ const 只能初始化，不能賦值    │
// │ 引用成員變數                      │ 引用必須在宣告時綁定          │
// │ 沒有預設建構函數的成員物件         │ 無法先預設建構再賦值          │
// │ 基類的建構函數（繼承時）           │ 基類必須在派生類之前建構      │
// └──────────────────────────────────┴──────────────────────────────┘

// --- 情況 1：const 成員變數 ---
// const 變數必須在宣告時就初始化，之後不能修改。
// 函數體執行時，成員已經「宣告」過了，再賦值就是修改 const → 編譯錯誤。
class Student {
private:
    const int studentId;  // const 成員：一旦初始化就不能修改
    string name;

public:
    // 錯誤寫法：
    // Student(int id, const string& n) {
    //     studentId = id;  // 編譯錯誤！不能給 const 賦值！
    //     name = n;
    // }

    // 正確寫法：使用初始化列表
    Student(int id, const string& n)
        : studentId(id), name(n)
    { }

    void print() const {
        cout << "  學號: " << studentId << ", 姓名: " << name << endl;
    }
};

// --- 情況 2：引用成員變數 ---
// 引用一旦綁定就不能改變指向，且必須在宣告時綁定。
class Logger {
private:
    ostream& output;  // 引用成員：必須在初始化時綁定
    string prefix;

public:
    // 錯誤寫法：
    // Logger(ostream& os, const string& p) {
    //     output = os;  // 編譯錯誤！引用必須在初始化列表中綁定！
    //     prefix = p;
    // }

    // 正確寫法
    Logger(ostream& os, const string& p)
        : output(os), prefix(p)
    { }

    void log(const string& message) const {
        output << "  [" << prefix << "] " << message << endl;
    }
};

// --- 情況 3：沒有預設建構函數的成員物件 ---
// 如果成員物件的類別只有帶參建構函數，沒有預設建構函數，
// 那就無法在函數體之前自動預設建構它，必須在初始化列表中提供參數。
class Engine {
private:
    int horsepower;
    string fuelType;

public:
    // Engine 只有帶參建構函數，沒有預設建構函數
    Engine(int hp, const string& fuel)
        : horsepower(hp), fuelType(fuel)
    {
        cout << "  引擎建構: " << hp << " 馬力, " << fuel << endl;
    }

    void print() const {
        cout << "  引擎: " << horsepower << " HP (" << fuelType << ")" << endl;
    }
};

class Car {
private:
    string brand;
    Engine engine;   // Engine 沒有預設建構函數！

public:
    // 錯誤寫法：
    // Car(const string& b, int hp, const string& fuel) {
    //     brand = b;
    //     engine = Engine(hp, fuel);  // 編譯錯誤！engine 無法預設建構！
    // }

    // 正確寫法：在初始化列表中建構 engine
    Car(const string& b, int hp, const string& fuel)
        : brand(b), engine(hp, fuel)  // 直接把參數傳給 Engine 的建構函數
    {
        cout << "  汽車建構: " << brand << endl;
    }

    void print() const {
        cout << "  品牌: " << brand << endl;
        engine.print();
    }
};

// --- 情況 4：基類的建構函數（繼承時）---
// 派生類必須在初始化列表中調用基類的建構函數。
class Animal {
private:
    string species;

public:
    Animal(const string& s) : species(s) {
        cout << "  Animal 建構: " << species << endl;
    }

    string getSpecies() const { return species; }
};

class Dog : public Animal {
private:
    string name;

public:
    // 用初始化列表調用基類建構函數
    Dog(const string& n)
        : Animal("犬科"),   // 調用基類建構函數（必須用初始化列表）
          name(n)            // 初始化自己的成員
    {
        cout << "  Dog 建構: " << name << endl;
    }

    void print() const {
        cout << "  " << name << " (" << getSpecies() << ")" << endl;
    }
};

// ================================================================
// 重點三：初始化順序陷阱
// ================================================================
// 初始化列表中成員的初始化順序不是由列表書寫順序決定的，
// 而是由成員在類別中宣告的順序決定的！
//
// 最佳實踐：初始化列表的書寫順序，永遠保持和成員宣告順序一致。
// 編譯時加 -Wall -Wextra 可以偵測順序不一致的問題。

class OrderDemo {
private:
    int a;    // 第 1 個宣告 → 第 1 個初始化
    int b;    // 第 2 個宣告 → 第 2 個初始化
    int c;    // 第 3 個宣告 → 第 3 個初始化

public:
    // 注意：初始化列表故意寫成 c, a, b 的順序
    // 但實際初始化順序仍然是 a, b, c（按宣告順序）
    OrderDemo(int val)
        : c(val + 2),     // 寫在第 1 個，但第 3 個執行
          a(val),          // 寫在第 2 個，但第 1 個執行
          b(val + 1)       // 寫在第 3 個，但第 2 個執行
    {
        cout << "  a = " << a << ", b = " << b << ", c = " << c << endl;
    }
};

// ================================================================
// 重點四：危險的順序依賴（DangerousOrder vs SafeOrder）
// ================================================================
// 當一個成員的初始化值依賴另一個成員時，宣告順序就至關重要。
// 如果順序不對，就會用到未初始化的垃圾值，造成未定義行為！

class DangerousOrder {
private:
    int length;    // 第 1 個宣告 → 先初始化
    int* data;     // 第 2 個宣告 → 後初始化

public:
    // 危險！看起來 data 用 length 分配，但 length 確實先初始化，
    // 然而如果寫反了（data 寫在 length 前面宣告），就會用垃圾值！
    // 這裡的問題是：書寫順序讓人誤以為 data 先於 length 初始化
    DangerousOrder(int len)
        : data(new int[length]),  // 用 length 來分配，但如果 length 還沒初始化就是垃圾值！
          length(len)              // 實際上 length 先宣告所以先初始化（此例碰巧安全）
    {
        // 注意：因為 length 宣告在 data 之前，所以 length 先被初始化為 len，
        // 然後 data 才用 length（此時已經是 len）分配。
        // 但這段程式碼很容易誤導人，因為書寫順序和宣告順序相反！
    }

    ~DangerousOrder() { delete[] data; }
};

class SafeOrder {
private:
    int length;    // 第 1 個宣告
    int* data;     // 第 2 個宣告

public:
    // 安全：初始化列表的書寫順序和宣告順序一致
    SafeOrder(int len)
        : length(len),            // 先初始化 length（和宣告順序一致）
          data(new int[length])   // 再用已初始化的 length 分配記憶體
    {
        cout << "  安全分配了 " << length << " 個元素" << endl;
    }

    ~SafeOrder() { delete[] data; }
};

// ================================================================
// 重點五：初始化列表中可以使用表達式
// ================================================================
// 初始化列表不僅可以直接傳參數，還可以使用任意表達式：
// 算術運算、函數調用、字串串接等都可以。

class Circle {
private:
    double radius;
    double area;
    double circumference;

public:
    // 在初始化列表中使用公式計算
    Circle(double r)
        : radius(r),
          area(M_PI * r * r),              // 面積公式
          circumference(2.0 * M_PI * r)    // 周長公式
    { }

    void print() const {
        cout << "  半徑: " << radius << endl;
        cout << "  面積: " << area << endl;
        cout << "  周長: " << circumference << endl;
    }
};

class FullName {
private:
    string firstName;
    string lastName;
    string fullName;

public:
    // 初始化列表中做字串串接
    FullName(const string& first, const string& last)
        : firstName(first),
          lastName(last),
          fullName(last + " " + first)    // 表達式：字串串接
    { }

    void print() const {
        cout << "  姓: " << lastName << ", 名: " << firstName
             << ", 全名: " << fullName << endl;
    }
};

// ================================================================
// 重點六：C++11 類別內預設值配合初始化列表
// ================================================================
// C++11 引入了類別內成員初始化（In-class Member Initializer），
// 可以在宣告成員時直接給預設值。
//
// 優先順序規則：
// ┌────────────────────────┬──────────────────────────────────┐
// │ 初始化列表中有指定      │ → 使用初始化列表的值（覆蓋預設值）│
// │ 初始化列表中沒有        │ → 使用類別內預設值                │
// │ 類別內也沒有預設值      │ → 基本型別不初始化（垃圾值），    │
// │                        │   類別型別調用預設建構函數         │
// └────────────────────────┴──────────────────────────────────┘

class GameConfig {
private:
    // C++11：直接在宣告時給預設值
    int screenWidth = 1280;
    int screenHeight = 720;
    bool fullscreen = false;
    string title = "My Game";
    int fps = 60;

public:
    // 預設建構函數：所有成員使用類別內的預設值
    GameConfig() {
        cout << "  [預設建構] 使用所有預設值" << endl;
    }

    // 部分自定義：初始化列表會覆蓋類別內預設值
    GameConfig(int w, int h)
        : screenWidth(w), screenHeight(h)  // 只覆蓋寬高
        // fullscreen、title、fps 使用類別內的預設值
    {
        cout << "  [部分自定義] 只改解析度" << endl;
    }

    // 完全自定義：初始化列表覆蓋所有預設值
    GameConfig(int w, int h, bool fs, const string& t, int f)
        : screenWidth(w), screenHeight(h), fullscreen(fs),
          title(t), fps(f)
    {
        cout << "  [完全自定義] 所有參數指定" << endl;
    }

    void print() const {
        cout << "  遊戲: " << title << endl;
        cout << "  解析度: " << screenWidth << "x" << screenHeight << endl;
        cout << "  全螢幕: " << (fullscreen ? "是" : "否") << endl;
        cout << "  FPS: " << fps << endl;
    }
};

// ================================================================
// 重點七：效能對比（函數體賦值 vs 初始化列表）
// ================================================================
// ┌──────────────┬──────────────┬─────────────────────────────┐
// │ 方式          │ 操作次數      │ 調用的函數                   │
// ├──────────────┼──────────────┼─────────────────────────────┤
// │ 函數體賦值    │ 2 次         │ 預設建構函數 + 賦值運算子     │
// │ 初始化列表    │ 1 次         │ 帶參建構函數                  │
// └──────────────┴──────────────┴─────────────────────────────┘
// 當成員是大型物件（如包含大量資料的容器），效能差異非常明顯。

class HeavyObject {
private:
    string data;

public:
    HeavyObject() {
        data = "";
        cout << "    HeavyObject 預設建構" << endl;
    }

    HeavyObject(const string& s) {
        data = s;
        cout << "    HeavyObject 帶參建構: " << s << endl;
    }

    HeavyObject& operator=(const string& s) {
        data = s;
        cout << "    HeavyObject 賦值運算子: " << s << endl;
        return *this;
    }
};

// 使用函數體賦值（兩步：預設建構 + 賦值）
class ContainerA {
private:
    HeavyObject obj;

public:
    ContainerA(const string& s) {
        // 進入函數體之前，obj 已經被預設建構了（第一步）
        cout << "    --- 開始賦值 ---" << endl;
        obj = s;    // 賦值（第二步）
        cout << "    --- 賦值完成 ---" << endl;
    }
};

// 使用初始化列表（一步：直接帶參建構）
class ContainerB {
private:
    HeavyObject obj;

public:
    ContainerB(const string& s)
        : obj(s)  // 直接帶參建構，不經過預設建構
    {
        cout << "    --- 初始化列表完成 ---" << endl;
    }
};

// ================================================================
// 重點八：綜合範例 —— RPG 武器系統
// ================================================================
// 展示初始化列表的所有技巧：
//   - const 成員（weaponId）
//   - 沒有預設建構函數的成員物件（Rarity）
//   - C++11 類別內預設值（enhanceLevel、isEquipped）
//   - 帶預設參數的建構函數
//   - 一般成員的初始化

// 武器稀有度（沒有預設建構函數 → 必須用初始化列表）
class Rarity {
private:
    string name;
    int stars;

public:
    Rarity(const string& n, int s) : name(n), stars(s) { }

    string getName() const { return name; }
    int getStars() const { return stars; }

    void print() const {
        cout << name << " (";
        for (int i = 0; i < stars; i++) cout << "*";
        cout << ")";
    }
};

// 武器類別
class Weapon {
private:
    const int weaponId;           // const 成員 → 必須用初始化列表
    string name;
    Rarity rarity;                // 無預設建構函數 → 必須用初始化列表
    double baseDamage;
    double critRate;

    // C++11 類別內預設值
    int enhanceLevel = 0;         // 強化等級，預設 0
    bool isEquipped = false;      // 是否裝備，預設否

public:
    // 建構函數：展示各種初始化方式的結合
    Weapon(int id, const string& n, const string& rarityName,
           int rarityStars, double dmg, double crit = 0.05)
        : weaponId(id),                              // const 成員
          name(n),                                    // 一般成員
          rarity(rarityName, rarityStars),            // 無預設建構的成員物件
          baseDamage(dmg),                            // 一般成員
          critRate(crit)                              // 帶預設參數
          // enhanceLevel 和 isEquipped 使用類別內預設值（不寫在列表中）
    {
        cout << "  鍛造武器: " << name << " [ID:" << weaponId << "]" << endl;
    }

    void enhance() {
        if (enhanceLevel < 15) {
            enhanceLevel++;
            baseDamage *= 1.1;  // 每次強化增加 10% 傷害
            cout << "  " << name << " 強化至 +" << enhanceLevel
                 << "（傷害: " << baseDamage << "）" << endl;
        } else {
            cout << "  " << name << " 已達最高強化等級！" << endl;
        }
    }

    void equip() { isEquipped = true; }
    void unequip() { isEquipped = false; }

    void print() const {
        cout << "  +---------------------------------+" << endl;
        cout << "  | [" << weaponId << "] " << name << endl;
        cout << "  | 稀有度: ";
        rarity.print();
        cout << endl;
        cout << "  | 傷害: " << baseDamage
             << "  暴擊率: " << (critRate * 100) << "%" << endl;
        cout << "  | 強化: +" << enhanceLevel
             << "  " << (isEquipped ? "[已裝備]" : "[未裝備]") << endl;
        cout << "  +---------------------------------+" << endl;
    }
};

// ================================================================
// main()：示範所有重點
// ================================================================
int main() {
    cout << "=====================================================" << endl;
    cout << "  第 16 課：建構函數初始化列表（Member Initializer List）" << endl;
    cout << "=====================================================" << endl;

    // --- 重點一：初始化列表 vs 函數體賦值 ---
    cout << "\n【1】初始化列表 vs 函數體賦值的本質區別" << endl;
    cout << "--- 函數體賦值（兩步：先預設建構，再賦值）---" << endl;
    DemoAssign d1("Hello", 42);
    d1.print();

    cout << "--- 初始化列表（一步：直接建構）---" << endl;
    DemoInitList d2("Hello", 42);
    d2.print();

    // --- 重點二：必須使用初始化列表的四種情況 ---
    cout << "\n【2】必須使用初始化列表的四種情況" << endl;

    cout << "\n情況 1：const 成員變數" << endl;
    Student s(20250001, "張三");
    s.print();
    cout << "  // const int studentId; → 只能初始化，不能賦值" << endl;

    cout << "\n情況 2：引用成員變數" << endl;
    Logger consoleLogger(cout, "INFO");
    consoleLogger.log("程式啟動");
    consoleLogger.log("初始化完成");
    cout << "  // ostream& output; → 引用必須在宣告時綁定" << endl;

    cout << "\n情況 3：沒有預設建構函數的成員物件" << endl;
    Car car("BMW", 300, "汽油");
    car.print();
    cout << "  // Engine engine; → Engine 只有帶參建構，無法預設建構" << endl;

    cout << "\n情況 4：基類的建構函數（繼承時）" << endl;
    Dog dog("旺財");
    dog.print();
    cout << "  // : Animal(\"犬科\") → 必須在初始化列表中調用基類建構" << endl;

    // --- 重點三：初始化順序陷阱 ---
    cout << "\n【3】初始化順序陷阱：按宣告順序，不是書寫順序" << endl;
    cout << "  列表書寫順序: c, a, b" << endl;
    cout << "  實際初始化順序: a, b, c（按宣告順序）" << endl;
    OrderDemo od(10);
    cout << "  結果正確（此例沒有依賴問題），但書寫順序容易誤導！" << endl;

    // --- 重點四：危險的順序依賴 ---
    cout << "\n【4】危險的順序依賴（DangerousOrder vs SafeOrder）" << endl;
    cout << "  DangerousOrder: data(new int[length]), length(len)" << endl;
    cout << "    → 書寫順序暗示 data 先用 length 分配，但 length 宣告在前，" << endl;
    cout << "      實際上 length 先初始化。書寫順序反過來，容易誤導讀者！" << endl;
    cout << "  SafeOrder: length(len), data(new int[length])" << endl;
    cout << "    → 書寫順序和宣告順序一致，清晰明瞭。" << endl;
    SafeOrder so(10);
    cout << "  最佳實踐：初始化列表的書寫順序永遠保持和宣告順序一致！" << endl;
    cout << "  編譯加 -Wall -Wextra 可偵測順序不一致的警告。" << endl;

    // --- 重點五：初始化列表中使用表達式 ---
    cout << "\n【5】初始化列表中可以使用表達式" << endl;

    cout << "--- Circle: 面積和周長用公式計算 ---" << endl;
    Circle circle(5.0);
    circle.print();

    cout << "--- FullName: 字串串接 ---" << endl;
    FullName fn("信安", "陳");
    fn.print();

    // --- 重點六：C++11 類別內預設值配合初始化列表 ---
    cout << "\n【6】C++11 類別內預設值配合初始化列表（GameConfig）" << endl;

    cout << "配置 1：全部使用預設值" << endl;
    GameConfig cfg1;
    cfg1.print();

    cout << "\n配置 2：只覆蓋解析度，其餘用預設值" << endl;
    GameConfig cfg2(1920, 1080);
    cfg2.print();

    cout << "\n配置 3：全部自定義（覆蓋所有預設值）" << endl;
    GameConfig cfg3(2560, 1440, true, "RPG World", 144);
    cfg3.print();

    cout << "\n  覆蓋規則：" << endl;
    cout << "    初始化列表有指定 → 使用列表的值" << endl;
    cout << "    初始化列表沒有   → 使用類別內預設值" << endl;
    cout << "    類別內也沒有     → 基本型別垃圾值 / 類別型別預設建構" << endl;

    // --- 重點七：效能對比 ---
    cout << "\n【7】效能對比：函數體賦值 vs 初始化列表" << endl;

    cout << "方式 A：函數體賦值（預設建構 + 賦值 = 兩步）" << endl;
    ContainerA ca("Hello");

    cout << "\n方式 B：初始化列表（直接帶參建構 = 一步）" << endl;
    ContainerB cb("Hello");

    cout << "\n  結論：" << endl;
    cout << "    函數體賦值：預設建構函數 + 賦值運算子 = 2 次操作" << endl;
    cout << "    初始化列表：帶參建構函數 = 1 次操作" << endl;
    cout << "    對於大型物件，效能差異非常明顯！" << endl;

    // --- 重點八：綜合範例 RPG 武器系統 ---
    cout << "\n【8】綜合範例：RPG 武器系統" << endl;
    cout << "  展示：const成員 + 無預設建構成員 + 類別內預設值 + 預設參數" << endl;

    cout << "\n--- 鍛造武器 ---" << endl;
    Weapon sword(1001, "屠龍劍", "傳說", 5, 150.0, 0.15);
    Weapon bow(1002, "風之弓", "史詩", 4, 95.0, 0.25);
    Weapon staff(1003, "智慧之杖", "精良", 3, 120.0);  // 暴擊率用預設值 0.05

    cout << "\n--- 武器資訊 ---" << endl;
    sword.print();
    bow.print();
    staff.print();

    cout << "\n--- 強化武器 ---" << endl;
    sword.enhance();
    sword.enhance();
    sword.enhance();

    cout << "\n--- 裝備武器 ---" << endl;
    sword.equip();

    cout << "\n--- 最終狀態 ---" << endl;
    sword.print();

    // --- 重點回顧 ---
    cout << "\n=====================================================" << endl;
    cout << "本課重點回顧：" << endl;
    cout << "  1. 初始化列表語法: : member(value) 寫在參數列表後、函數體前" << endl;
    cout << "  2. 初始化(一步) vs 賦值(兩步)：初始化列表省去預設建構的開銷" << endl;
    cout << "  3. 四種必須使用的情況：const成員、引用成員、無預設建構成員、基類" << endl;
    cout << "  4. 初始化順序按成員宣告順序，不是列表書寫順序" << endl;
    cout << "  5. 書寫順序和宣告順序不一致 → 容易造成依賴性 bug" << endl;
    cout << "  6. 初始化列表中可以使用任意表達式（公式計算、字串串接等）" << endl;
    cout << "  7. C++11 類別內預設值：初始化列表會覆蓋預設值，未覆蓋的用預設值" << endl;
    cout << "  8. 效能：對類別型別成員，初始化列表省去預設建構+賦值的雙重開銷" << endl;
    cout << "  9. 最佳實踐：永遠使用初始化列表，書寫順序保持和宣告順序一致" << endl;
    cout << "=====================================================" << endl;

    return 0;
}
