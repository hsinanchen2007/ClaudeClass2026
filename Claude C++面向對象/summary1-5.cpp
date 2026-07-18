/*
 * ================================================================
 *  【第 1～5 課總複習】summary1-5.cpp
 * ================================================================
 *  涵蓋課程：
 *    第 1 課：C++ 的歷史與設計哲學
 *    第 2 課：C 與 C++ 的關鍵差異
 *    第 3 課：C++ 編譯環境設置（g++、clang++、MSVC）
 *    第 4 課：命名空間（namespace）基礎
 *    第 5 課：輸入輸出流（iostream）入門
 *
 *  編譯：g++ -std=c++17 -Wall -Wextra -o summary1-5 summary1-5.cpp
 *  MSVC：cl /std:c++17 /utf-8 /EHsc /W4 summary1-5.cpp
 * ================================================================
 */

#include <iostream>
#include <iomanip>      // setw, setprecision, fixed, hex 等格式控制
#include <string>
#include <vector>
#include <algorithm>    // std::sort
#include <limits>       // numeric_limits
#include <cmath>        // std::sqrt

// ================================================================
// 第 1 課：C++ 的歷史與設計哲學
// ================================================================
//
// 【歷史時間線】
//   1972  Dennis Ritchie 創造 C 語言
//   1979  Bjarne Stroustrup 開發「C with Classes」
//   1983  更名為 C++（Rick Mascitti 建議，++ 來自遞增運算子）
//   1998  C++98 — 第一個 ISO 標準
//   2011  C++11 — 現代 C++ 的起點
//   2014/17/20/23 持續演進
//
// 【設計動機】
//   Stroustrup 想結合 Simula 的組織能力（class/物件）與 C 的執行效率
//
// 【四大設計原則】
//   1. 零開銷原則 — 不使用的功能不產生開銷
//   2. 直接映射硬體 — 保留指標、位元操作等底層能力
//   3. C 的相容性 — 大部分 C 程式碼也是合法 C++
//   4. 高階抽象可選 — class、template 等功能不強制使用
//
// 【四種範式】
//   程序式 / 物件導向 / 泛型 / 函數式
// ================================================================

// --- 範式一：程序式（純函數，零 OOP 開銷）---
int proceduralAdd(int a, int b) { return a + b; }

// --- 範式二：物件導向（class + 封裝）---
class Calculator {
    int lastResult_ = 0;          // 私有成員
public:
    int multiply(int a, int b) { lastResult_ = a * b; return lastResult_; }
    int getLastResult() const { return lastResult_; }   // const 保證不修改
};

// --- 範式三：泛型（template）---
template<typename T>
T getMax(T a, T b) { return (a > b) ? a : b; }

// --- 零開銷 & 硬體映射示範 ---
void demo_lesson1() {
    std::cout << "=== 第 1 課：C++ 歷史與設計哲學 ===\n\n";

    // 程序式
    std::cout << "[程序式] proceduralAdd(3,5) = " << proceduralAdd(3, 5) << "\n";

    // 物件導向
    Calculator calc;
    std::cout << "[OOP] calc.multiply(4,6) = " << calc.multiply(4, 6) << "\n";

    // 泛型 — 同一函數自動推導 int / double / char
    std::cout << "[泛型] getMax(10,20)     = " << getMax(10, 20) << "\n";
    std::cout << "[泛型] getMax(3.14,2.71) = " << getMax(3.14, 2.71) << "\n";
    std::cout << "[泛型] getMax('a','z')   = " << getMax('a', 'z') << "\n";

    // 函數式 — lambda + std::sort
    std::vector<int> nums = {5, 2, 8, 1, 9, 3};
    std::sort(nums.begin(), nums.end(), [](int a, int b) { return a < b; });
    std::cout << "[函數式] 排序: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << "\n";

    // 硬體映射 — 指標 & 位元操作
    int val = 42;
    int* ptr = &val;            // 取址
    *ptr = 100;                 // 透過指標修改
    unsigned char flags = 0b00001111;
    flags <<= 2;                // 位元左移 → 0b00111100 = 60
    std::cout << "[硬體] *ptr=" << val << ", flags<<2=" << (int)flags << "\n\n";
}

// ================================================================
// 第 2 課：C 與 C++ 的關鍵差異
// ================================================================
//
// 【九大差異速查表】
//   項目           C                    C++
//   ───────────── ──────────────────── ────────────────────
//   變數宣告       區塊開頭(C89)         任意位置
//   I/O            printf/scanf          cout/cin（型別安全）
//   記憶體         malloc/free           new/delete（調建構/解構）
//   bool           需 stdbool.h          內建 bool/true/false
//   函數重載       不支援                支援（同名不同參數）
//   預設參數       不支援                支援（從右側開始）
//   引用           只有指標              支援 int& ref（更安全）
//   struct         只有資料              可含成員函數
//   字串           char[] + strcpy       std::string（+ 連接）
// ================================================================

// --- 差異五：函數重載 ---
int square(int x)    { return x * x; }       // int 版本
double square(double x) { return x * x; }    // double 版本
int add(int a, int b) { return a + b; }
int add(int a, int b, int c) { return a + b + c; }

// --- 差異六：預設參數（從右側開始）---
void greet(const std::string& name,
           const std::string& greeting = "Hello",
           const std::string& punct = "!") {
    std::cout << "  " << greeting << ", " << name << punct << "\n";
}

// --- 差異七：引用（Reference）---
void swapByRef(int& a, int& b) { int t = a; a = b; b = t; }  // 不需 * &

// --- 差異八：struct 可含成員函數 ---
struct Point {
    int x, y;
    void print() const { std::cout << "Point(" << x << "," << y << ")"; }
};

void demo_lesson2() {
    std::cout << "=== 第 2 課：C 與 C++ 關鍵差異 ===\n\n";

    // 變數宣告 — 在使用點附近宣告並初始化
    int sum = 0;
    for (int i = 0; i < 5; i++) sum += i;  // i 作用域僅在 for 內
    std::cout << "[變數宣告] sum(0..4) = " << sum << "\n";

    // I/O — 自動型別識別，不需 %d %f
    std::cout << "[cout] int=" << 42 << " double=" << 3.14
              << " bool=" << std::boolalpha << true << std::noboolalpha << "\n";

    // new/delete（vs malloc/free）
    int* p = new int(100);   // 配置+初始化，不需 sizeof/轉型
    std::cout << "[new] *p = " << *p << "\n";
    delete p; p = nullptr;   // 釋放後設 nullptr

    int* arr = new int[3]{10, 20, 30};
    std::cout << "[new[]] arr: " << arr[0] << " " << arr[1] << " " << arr[2] << "\n";
    delete[] arr;             // 陣列必須 delete[]！

    // 函數重載 — 編譯器依參數選版本
    std::cout << "[重載] square(5)=" << square(5)
              << " square(3.14)=" << square(3.14) << "\n";
    std::cout << "[重載] add(1,2)=" << add(1, 2)
              << " add(1,2,3)=" << add(1, 2, 3) << "\n";

    // 預設參數
    greet("Alice");                  // Hello, Alice!
    greet("Bob", "Hi");              // Hi, Bob!
    greet("Charlie", "Hey", "~");    // Hey, Charlie~

    // 引用 — 語法簡潔，不需 & 取址
    int x = 10, y = 20;
    swapByRef(x, y);
    std::cout << "[引用] swap後 x=" << x << " y=" << y << "\n";

    int orig = 42;
    int& alias = orig;  // 別名，修改 alias 即修改 orig
    alias = 99;
    std::cout << "[引用] alias=99 → orig=" << orig << "\n";

    // struct 有成員函數
    Point pt{3, 7};     // 不需 struct 關鍵字（C 需要）
    std::cout << "[struct] "; pt.print(); std::cout << "\n";

    // std::string
    std::string s = "Hello" + std::string(", ") + "World!";
    std::cout << "[string] " << s << " len=" << s.length()
              << " sub=" << s.substr(0, 5) << "\n\n";
}

// ================================================================
// 第 3 課：C++ 編譯環境設置
// ================================================================
//
// 【三大編譯器】g++（GCC）、clang++（LLVM）、cl（MSVC）
//
// 【開發編譯】g++ -std=c++17 -Wall -Wextra -g -O0 file.cpp -o out
// 【發布編譯】g++ -std=c++17 -Wall -Wextra -O2 file.cpp -o out
// 【MSVC】    cl /std:c++17 /utf-8 /EHsc /W4 /Zi file.cpp
//
// 【編譯器偵測巨集】
//   __clang__  → Clang（先判斷，因 Clang 也定義 __GNUC__）
//   __GNUC__   → GCC
//   _MSC_VER   → MSVC（值為版本號，如 1940）
//
// 【C++ 標準偵測】__cplusplus
//   199711L=C++98  201103L=C++11  201402L=C++14
//   201703L=C++17  202002L=C++20  202302L=C++23
//   MSVC 需加 /Zc:__cplusplus 才正確
//
// 【OS 偵測】_WIN32/_WIN64  __linux__  __APPLE__
//
// 【多檔案編譯】
//   g++ main.cpp util.cpp -o program
//   分步：g++ -c main.cpp → g++ -c util.cpp → g++ main.o util.o -o program
//   標頭檔保護：#pragma once 或 #ifndef HEADER_H
//
// 【常見錯誤】
//   'g++' not recognized → PATH 未設定
//   undefined reference  → 沒有連結對應 .cpp
//   'auto' error         → 需加 -std=c++11 或更高
// ================================================================

void demo_lesson3() {
    std::cout << "=== 第 3 課：編譯環境設置 ===\n\n";

    // 編譯器偵測
    std::cout << "[編譯器] ";
#if defined(__clang__)
    std::cout << "Clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(__GNUC__)
    std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__;
#elif defined(_MSC_VER)
    std::cout << "MSVC " << _MSC_VER;
#else
    std::cout << "Unknown";
#endif
    std::cout << "\n";

    // C++ 標準偵測
    std::cout << "[標準] __cplusplus = " << __cplusplus << " → ";
#if __cplusplus >= 202002L
    std::cout << "C++20+";
#elif __cplusplus >= 201703L
    std::cout << "C++17";
#elif __cplusplus >= 201402L
    std::cout << "C++14";
#elif __cplusplus >= 201103L
    std::cout << "C++11";
#else
    std::cout << "C++98/03";
#endif
    std::cout << "\n";

    // OS 偵測
    std::cout << "[OS] ";
#if defined(_WIN64)
    std::cout << "Windows 64-bit";
#elif defined(_WIN32)
    std::cout << "Windows 32-bit";
#elif defined(__linux__)
    std::cout << "Linux";
#elif defined(__APPLE__)
    std::cout << "macOS";
#else
    std::cout << "Unknown";
#endif
    std::cout << "\n";

    // 現代 C++ 功能驗證
    auto x = 42;                                    // auto（C++11）
    std::vector<std::string> v = {"A", "B", "C"};   // 初始化列表（C++11）
    auto mul = [](int a, int b) { return a * b; };   // lambda（C++11）
    std::cout << "[C++11] auto=" << x << " lambda=" << mul(6, 7) << " vec=";
    for (const auto& s : v) std::cout << s << " ";   // 範圍 for
    std::cout << "\n\n";
}

// ================================================================
// 第 4 課：命名空間（namespace）基礎
// ================================================================
//
// 【目的】解決名稱衝突：不同函式庫可能定義相同名稱的函數
//
// 【語法】namespace Name { 變數/函式/類別... }
//
// 【三種存取方式】
//   1. 完全限定名稱：math::PI          ← 最安全，推薦
//   2. using 宣告：  using math::PI;   ← 引入特定成員
//   3. using 指令：  using namespace math; ← 引入全部（慎用！）
//
// 【重要規則】
//   - 標頭檔(.h)中 絕對禁止 using namespace！
//   - 大型專案優先用完全限定名稱
//   - 在函式/區塊內用 using 宣告可局部簡化
//
// 【命名空間別名】namespace mt = math_tools;
// 【匿名命名空間】namespace { } — 只在本檔案內可見（取代 static）
// 【巢狀命名空間】C++17: namespace a::b::c { }
// 【擴展】同名 namespace 可分散在多處，編譯器自動合併
// ================================================================

namespace math_ns {
    const double PI = 3.14159265;
    double circleArea(double r) { return PI * r * r; }
}

namespace str_ns {
    std::string toUpper(const std::string& s) {
        std::string r = s;
        for (char& c : r) if (c >= 'a' && c <= 'z') c -= 32;
        return r;
    }
}

// 巢狀命名空間（C++17 寫法）
namespace company::graphics {
    void draw() { std::cout << "  [Graphics] draw\n"; }
}

// 匿名命名空間 — 只在本檔案可見
namespace {
    int fileLocalCounter = 0;
    void countCall() { ++fileLocalCounter; }
}

void demo_lesson4() {
    std::cout << "=== 第 4 課：命名空間基礎 ===\n\n";

    // 方式一：完全限定名稱（最安全）
    std::cout << "[完全限定] math_ns::PI = " << math_ns::PI << "\n";
    std::cout << "[完全限定] circleArea(5) = " << math_ns::circleArea(5) << "\n";

    // 方式二：using 宣告（引入特定成員）
    {
        using str_ns::toUpper;
        std::cout << "[using宣告] toUpper(\"hello\") = " << toUpper("hello") << "\n";
    }

    // 方式三：using 指令（僅在區塊內）
    {
        using namespace math_ns;
        std::cout << "[using指令] PI = " << PI << "\n";
    }

    // 別名
    {
        namespace mt = math_ns;
        std::cout << "[別名] mt::PI = " << mt::PI << "\n";
    }

    // 巢狀命名空間
    company::graphics::draw();

    // 匿名命名空間
    countCall(); countCall(); countCall();
    std::cout << "[匿名ns] fileLocalCounter = " << fileLocalCounter << "\n\n";
}

// ================================================================
// 第 5 課：輸入輸出流（iostream）入門
// ================================================================
//
// 【四個標準流】
//   cout — 標準輸出（有緩衝）    對應 C 的 stdout
//   cin  — 標準輸入              對應 C 的 stdin
//   cerr — 標準錯誤（無緩衝）    對應 C 的 stderr
//   clog — 標準日誌（有緩衝）    對應 C 的 stderr
//
// 【<< 插入運算子】
//   - 自動識別型別，不需 %d %f
//   - 可鏈式串接：cout << a << b << c
//
// 【換行比較】
//   std::endl → '\n' + flush（慢）
//   '\n'      → 僅換行（快，推薦大量輸出時使用）
//   std::flush→ 只 flush 不換行
//
// 【>> 提取運算子】
//   - 自動跳過前導空白
//   - 遇到空白停止 → "Hello World" 只讀到 "Hello"
//   - 可連續串接：cin >> a >> b >> c
//
// 【getline】
//   std::getline(cin, str) → 讀整行含空格
//
// 【cin >> 與 getline 混用陷阱】
//   cin >> x 後緩衝區殘留 '\n'，getline 會讀到空字串
//   解法：cin.ignore(numeric_limits<streamsize>::max(), '\n');
//
// 【流狀態】
//   good() — 正常     eof()  — 到結尾
//   fail() — 失敗(可恢復：clear() + ignore())
//   bad()  — 嚴重錯誤(不可恢復)
//
// 【iomanip 格式控制】
//   setprecision(n) — 精度（sticky）
//   fixed / scientific / defaultfloat — 浮點格式
//   setw(n) — 欄寬（non-sticky，只影響下一個輸出）
//   setfill(c) — 填充字元    left / right — 對齊
//   hex / oct / dec — 進位    showbase — 顯示前綴
//   boolalpha — bool 輸出 true/false
// ================================================================

void demo_lesson5() {
    std::cout << "=== 第 5 課：iostream 入門 ===\n\n";

    // << 自動型別識別 + 鏈式輸出
    std::cout << "[<<] int=" << 42 << " double=" << 3.14
              << " char=" << 'A' << " str=" << "Hi" << "\n";

    // 換行方式
    std::cout << "[endl] " << std::endl;      // flush（慢）
    std::cout << "[\\n] \n";                   // 僅換行（快）

    // cin >> 與 getline 說明（不做互動，僅展示規則）
    std::cout << "[cin >>] 遇空白停止，'Hello World' 只讀 'Hello'\n";
    std::cout << "[getline] 讀整行含空格\n";
    std::cout << "[陷阱] cin>>x 後需 cin.ignore() 再 getline\n\n";

    // --- iomanip 格式化 ---
    double pi = 3.14159265358979;

    // 精度
    std::cout << "[精度] 預設: " << pi << "\n";
    std::cout << "  setprecision(3): " << std::setprecision(3) << pi << "\n";
    std::cout << "  fixed+prec(2):   " << std::fixed << std::setprecision(2) << pi << "\n";
    std::cout << "  scientific:      " << std::scientific << std::setprecision(3) << pi << "\n";
    std::cout << std::defaultfloat << std::setprecision(6); // 恢復

    // 欄寬 & 對齊（setw 只影響下一個輸出！）
    std::cout << "  right setw(10): [" << std::right << std::setw(10) << 42 << "]\n";
    std::cout << "  left  setw(10): [" << std::left  << std::setw(10) << 42 << "]\n";
    std::cout << "  補零  setw(6):  [" << std::right << std::setfill('0')
              << std::setw(6) << 42 << "]\n";
    std::cout << std::setfill(' '); // 恢復

    // 進位
    int num = 255;
    std::cout << "  dec=" << std::dec << num
              << " hex=" << std::hex << num
              << " oct=" << std::oct << num << "\n";
    std::cout << std::dec; // 恢復

    // boolalpha
    std::cout << "  bool: " << std::boolalpha << true << "/" << false
              << std::noboolalpha << "\n";

    // 表格示範
    std::cout << "\n  " << std::left
              << std::setw(10) << "商品" << std::setw(8) << "單價"
              << std::setw(6) << "數量" << std::setw(10) << "小計" << "\n";
    std::cout << "  " << std::string(34, '-') << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  " << std::setw(10) << "蘋果" << std::setw(8) << 35.0
              << std::setw(6) << 3 << std::setw(10) << 105.0 << "\n";
    std::cout << "  " << std::setw(10) << "香蕉" << std::setw(8) << 25.5
              << std::setw(6) << 5 << std::setw(10) << 127.5 << "\n";
    std::cout << std::defaultfloat << std::right; // 恢復

    // cerr & clog
    std::cout << "\n[cerr] 無緩衝，立即輸出（用於錯誤訊息）\n";
    std::cerr << "  → 這是 cerr 輸出\n";
    std::cout << "[clog] 有緩衝（用於日誌）\n";
    std::clog << "  → 這是 clog 輸出\n\n";
}

// ================================================================
// 主程式
// ================================================================
int main() {
    std::cout << "================================================================\n";
    std::cout << "  C++ 面向對象 第 1～5 課 總複習\n";
    std::cout << "================================================================\n\n";

    demo_lesson1();   // 歷史與設計哲學
    demo_lesson2();   // C vs C++ 差異
    demo_lesson3();   // 編譯環境設置
    demo_lesson4();   // 命名空間
    demo_lesson5();   // iostream

    std::cout << "================================================================\n";
    std::cout << "  第 1～5 課總複習完成！\n";
    std::cout << "================================================================\n";

    return 0;
}

/*
 * ================================================================
 * 五課重點速查表
 * ================================================================
 *
 * 課次   主題                    核心關鍵字
 * ─────  ──────────────────────  ──────────────────────────────
 * 第1課  歷史與設計哲學          零開銷、硬體映射、C相容、多範式
 * 第2課  C vs C++ 差異           new/delete、引用、重載、預設參數、string
 * 第3課  編譯環境設置            g++ -std=c++17 -Wall、__cplusplus、_MSC_VER
 * 第4課  命名空間                namespace、using、別名、匿名ns、巢狀ns
 * 第5課  iostream                cout/cin、<</>>/getline、iomanip、cerr/clog
 *
 * ================================================================
 */
