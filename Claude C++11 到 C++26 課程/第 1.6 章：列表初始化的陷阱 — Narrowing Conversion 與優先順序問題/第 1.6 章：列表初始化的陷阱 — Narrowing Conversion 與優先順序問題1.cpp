// 檔案名稱: brace_init_pitfalls.cpp
// 編譯指令: g++ -std=c++17 -Wall -Wextra -o brace_init_pitfalls brace_init_pitfalls.cpp
// 說明: 展示列表初始化的各種陷阱與解決方案

#include <iostream>
#include <vector>
#include <string>
#include <initializer_list>
#include <type_traits>
#include <map>
#include <cstdint>

// ===== 陷阱 1：initializer_list 優先順序 =====

class Container
{
public:
    // 建構子 1：指定大小
    explicit Container(std::size_t size)
    {
        std::cout << "  Container(size_t size=" << size << ")\n";
    }
    
    // 建構子 2：指定大小和初始值
    Container(std::size_t size, int value)
    {
        std::cout << "  Container(size_t size=" << size 
                  << ", int value=" << value << ")\n";
    }
    
    // 建構子 3：initializer_list
    Container(std::initializer_list<int> init)
    {
        std::cout << "  Container(initializer_list<int>, size=" 
                  << init.size() << ", values={";
        bool first = true;
        for (int v : init)
        {
            if (!first) std::cout << ", ";
            std::cout << v;
            first = false;
        }
        std::cout << "})\n";
    }
};

// ===== 陷阱 2：窄化轉換的各種情況 =====

void demonstrateNarrowing()
{
    std::cout << "===== 陷阱 2：窄化轉換 =====\n";
    
    // --- 2.1 浮點數到整數 ---
    std::cout << "\n--- 2.1 浮點數到整數 ---\n";
    
    double d = 3.14;
    
    int a(d);       // 合法，a = 3
    int b = d;      // 合法，b = 3
    // int c{d};    // 編譯錯誤！窄化轉換
    // int e = {d}; // 編譯錯誤！窄化轉換
    
    // 常量表達式可以（如果值在範圍內且無小數）
    int f{3};     // 合法，3.0 可以精確表示為 int
    // int g{3.14}; // 錯誤！有小數部分
    
    std::cout << "int a(3.14) = " << a << " (允許，但截斷)\n";
    std::cout << "int f{3.0} = " << f << " (允許，無小數)\n";
    std::cout << "int c{3.14}; // 編譯錯誤！\n";
    
    // --- 2.2 大整數到小整數 ---
    std::cout << "\n--- 2.2 大整數到小整數 ---\n";
    
    long long big = 1000;
    
    int i1(big);     // 合法
    int i2 = big;    // 合法
    // int i3{big};  // 編譯錯誤！即使值實際上放得下
    
    // 常量表達式可以（如果值在範圍內）
    int i4{100LL};   // 合法，100 在 int 範圍內
    // int i5{10'000'000'000LL};  // 錯誤！超出 int 範圍
    
    std::cout << "int i1(big) = " << i1 << " (允許)\n";
    std::cout << "int i4{100LL} = " << i4 << " (常量在範圍內，允許)\n";
    std::cout << "int i3{big}; // 編譯錯誤，即使 big=1000 放得下！\n";
    
    // --- 2.3 有號與無號之間 ---
    std::cout << "\n--- 2.3 有號與無號轉換 ---\n";
    
    int negative = -1;
    
    unsigned u1(negative);    // 合法，但結果很大
    unsigned u2 = negative;   // 合法，但結果很大
    // unsigned u3{negative}; // 編譯錯誤！
    
    unsigned u4{0};           // 合法
    // unsigned u5{-1};       // 錯誤！負數常量
    
    std::cout << "unsigned u1(-1) = " << u1 << " (允許，但危險)\n";
    std::cout << "unsigned u3{negative}; // 編譯錯誤！\n";
    
    // --- 2.4 指標相關 ---
    std::cout << "\n--- 2.4 指標到 bool ---\n";
    
    int* ptr = nullptr;
    
    bool b1(ptr);    // 合法
    bool b2 = ptr;   // 合法
    // bool b3{ptr}; // 編譯錯誤！C++11 禁止指標到 bool 的窄化
    
    std::cout << "bool b1(nullptr) = " << std::boolalpha << b1 << " (允許)\n";
    std::cout << "bool b3{ptr}; // 編譯錯誤！\n";
}

// ===== 陷阱 3：auto 與大括號的陷阱 =====

void demonstrateAutoBrace()
{
    std::cout << "\n===== 陷阱 3：auto 與大括號 =====\n";
    
    // --- C++11/14 的行為 ---
    std::cout << "\n--- 使用 auto = {...} ---\n";
    
    auto a = {1, 2, 3};   // std::initializer_list<int>
    auto b = {42};        // std::initializer_list<int>，即使只有一個元素！
    
    std::cout << "auto a = {1, 2, 3};\n";
    std::cout << "  type: std::initializer_list<int>\n";
    std::cout << "  size: " << a.size() << "\n";
    
    std::cout << "auto b = {42};\n";
    std::cout << "  type: std::initializer_list<int>\n";
    std::cout << "  size: " << b.size() << "\n";
    
    // --- C++17 的變化 ---
    std::cout << "\n--- C++17: auto x{value} ---\n";
    
    auto c{42};       // C++17: int（不再是 initializer_list）
    // auto d{1, 2};  // C++17: 編譯錯誤！
    
    std::cout << "auto c{42}; // C++17\n";
    std::cout << "  type: " << (std::is_same<decltype(c), int>::value ? "int" : "other") << "\n";
    std::cout << "  value: " << c << "\n";
    
    // --- 對比 ---
    std::cout << "\n--- 對比表 ---\n";
    std::cout << "語法              C++11/14               C++17\n";
    std::cout << "auto x = {1,2,3}  initializer_list<int>  initializer_list<int>\n";
    std::cout << "auto x = {42}     initializer_list<int>  initializer_list<int>\n";
    std::cout << "auto x{42}        initializer_list<int>  int\n";
    std::cout << "auto x{1,2,3}     initializer_list<int>  編譯錯誤\n";
}

// ===== 陷阱 4：空大括號的歧義 =====

class Widget
{
public:
    Widget()
    {
        std::cout << "  Widget() 預設建構子\n";
    }
    
    Widget(std::initializer_list<int> init)
    {
        std::cout << "  Widget(initializer_list<int>, size=" 
                  << init.size() << ")\n";
    }
};

class Gadget
{
public:
    Gadget()
    {
        std::cout << "  Gadget() 預設建構子\n";
    }
    
    // 沒有 initializer_list 建構子
};

void demonstrateEmptyBraces()
{
    std::cout << "\n===== 陷阱 4：空大括號的歧義 =====\n";
    
    std::cout << "\n--- Widget（有 initializer_list 建構子）---\n";
    
    std::cout << "Widget w1;\n";
    Widget w1;        // 預設建構子
    
    std::cout << "Widget w2{};\n";
    Widget w2{};      // 預設建構子（空大括號優先匹配預設建構子）
    
    std::cout << "Widget w3({});\n";
    Widget w3({});    // initializer_list 建構子（空列表）
    
    std::cout << "Widget w4{{}};\n";
    Widget w4{{}};    // initializer_list 建構子（包含一個預設初始化的 int）
    
    std::cout << "\n--- Gadget（無 initializer_list 建構子）---\n";
    
    std::cout << "Gadget g1;\n";
    Gadget g1;
    
    std::cout << "Gadget g2{};\n";
    Gadget g2{};
    
    std::cout << "\n--- std::vector 的情況 ---\n";
    
    std::vector<int> v1;      // 空 vector
    std::vector<int> v2{};    // 空 vector
    std::vector<int> v3({});  // 空 vector（透過空 initializer_list）
    std::vector<int> v4{{}};  // 包含一個元素 0！
    
    std::cout << "vector<int> v1;     size = " << v1.size() << "\n";
    std::cout << "vector<int> v2{};   size = " << v2.size() << "\n";
    std::cout << "vector<int> v3({}); size = " << v3.size() << "\n";
    std::cout << "vector<int> v4{{}}; size = " << v4.size() 
              << " (元素: " << v4[0] << ")\n";
}

// ===== 陷阱 5：std::vector 的著名陷阱 =====

void demonstrateVectorPitfall()
{
    std::cout << "\n===== 陷阱 5：std::vector 的著名陷阱 =====\n";
    
    std::cout << "\n--- int 版本 ---\n";
    
    std::vector<int> v1(5);       // 5 個元素，都是 0
    std::vector<int> v2{5};       // 1 個元素，值是 5
    std::vector<int> v3(5, 10);   // 5 個元素，都是 10
    std::vector<int> v4{5, 10};   // 2 個元素：5 和 10
    
    auto printVec = [](const std::string& name, const std::vector<int>& v) {
        std::cout << name << " (size=" << v.size() << "): {";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << v[i];
        }
        std::cout << "}\n";
    };
    
    printVec("v1(5)    ", v1);
    printVec("v2{5}    ", v2);
    printVec("v3(5, 10)", v3);
    printVec("v4{5, 10}", v4);
    
    std::cout << "\n--- string 版本 ---\n";
    
    std::vector<std::string> s1(3);           // 3 個空字串
    std::vector<std::string> s2{3};           // 編譯錯誤！int 不能轉成 string
    std::vector<std::string> s3(3, "hello");  // 3 個 "hello"
    std::vector<std::string> s4{"a", "b"};    // 2 個元素
    
    auto printStrVec = [](const std::string& name, 
                          const std::vector<std::string>& v) {
        std::cout << name << " (size=" << v.size() << "): {";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << "\"" << v[i] << "\"";
        }
        std::cout << "}\n";
    };
    
    printStrVec("s1(3)         ", s1);
    // printStrVec("s2{3}      ", s2);  // 無法編譯
    printStrVec("s3(3, \"hello\")", s3);
    printStrVec("s4{\"a\", \"b\"} ", s4);
}

// ===== 陷阱 6：建構子選擇的微妙差異 =====

class Converter
{
public:
    Converter(int value)
    {
        std::cout << "  Converter(int " << value << ")\n";
    }
    
    Converter(double value)
    {
        std::cout << "  Converter(double " << value << ")\n";
    }
    
    Converter(std::initializer_list<double> init)
    {
        std::cout << "  Converter(initializer_list<double>, size=" 
                  << init.size() << ")\n";
    }
};

void demonstrateConstructorSelection()
{
    std::cout << "\n===== 陷阱 6：建構子選擇的微妙差異 =====\n";
    
    std::cout << "\nConverter c1(42);\n";
    Converter c1(42);       // Converter(int)
    
    std::cout << "Converter c2{42};\n";
    Converter c2{42};       // initializer_list<double>！
                            // 42 可以轉換為 double
    
    std::cout << "Converter c3(3.14);\n";
    Converter c3(3.14);     // Converter(double)
    
    std::cout << "Converter c4{3.14};\n";
    Converter c4{3.14};     // initializer_list<double>
    
    std::cout << "\n--- 即使轉換不完美，也優先匹配 initializer_list ---\n";
    
    std::cout << "當 {} 內的型別可以轉換為 initializer_list 的元素型別時，\n";
    std::cout << "即使存在更精確匹配的建構子，也會優先選擇 initializer_list 版本。\n";
}

// ===== 陷阱 7：強制呼叫非 initializer_list 建構子 =====

class ForceConstruct
{
public:
    ForceConstruct(int a, int b)
    {
        std::cout << "  ForceConstruct(int " << a << ", int " << b << ")\n";
    }
    
    ForceConstruct(std::initializer_list<int> init)
    {
        std::cout << "  ForceConstruct(initializer_list, size=" 
                  << init.size() << ")\n";
    }
};

void demonstrateForceConstruct()
{
    std::cout << "\n===== 陷阱 7：如何強制呼叫非 initializer_list 建構子 =====\n";
    
    std::cout << "\n--- 問題：我想呼叫 (int, int) 版本 ---\n";
    
    std::cout << "ForceConstruct f1{5, 10};\n";
    ForceConstruct f1{5, 10};   // initializer_list 版本
    
    std::cout << "\n--- 解決方案 1：使用小括號 ---\n";
    
    std::cout << "ForceConstruct f2(5, 10);\n";
    ForceConstruct f2(5, 10);   // (int, int) 版本
    
    std::cout << "\n--- 解決方案 2：明確型別轉換 ---\n";
    
    // 如果必須使用 {}，可以讓元素無法轉換為 initializer_list 的元素型別
    // 但這通常不實用
    
    std::cout << "\n--- 結論 ---\n";
    std::cout << "當類別有 initializer_list 建構子時：\n";
    std::cout << "  使用 {} → 優先匹配 initializer_list\n";
    std::cout << "  使用 () → 匹配最佳的非 initializer_list 建構子\n";
}

// ===== 陷阱 8：聚合類別的特殊規則 =====

struct Aggregate
{
    int x;
    int y;
    int z;
};

struct NonAggregate
{
    int x;
    int y;
    int z;
    
    NonAggregate() : x{0}, y{0}, z{0} {}  // 有使用者定義的建構子
};

void demonstrateAggregate()
{
    std::cout << "\n===== 陷阱 8：聚合類別的特殊規則 =====\n";
    
    std::cout << "\n--- 聚合類別 Aggregate ---\n";
    
    Aggregate a1{1, 2, 3};      // 聚合初始化
    Aggregate a2{1, 2};         // z = 0
    Aggregate a3{1};            // y = 0, z = 0
    Aggregate a4{};             // 全部 = 0
    
    std::cout << "Aggregate a1{1, 2, 3}: x=" << a1.x 
              << ", y=" << a1.y << ", z=" << a1.z << "\n";
    std::cout << "Aggregate a2{1, 2}:    x=" << a2.x 
              << ", y=" << a2.y << ", z=" << a2.z << "\n";
    std::cout << "Aggregate a3{1}:       x=" << a3.x 
              << ", y=" << a3.y << ", z=" << a3.z << "\n";
    std::cout << "Aggregate a4{}:        x=" << a4.x 
              << ", y=" << a4.y << ", z=" << a4.z << "\n";
    
    std::cout << "\n--- 非聚合類別 NonAggregate ---\n";
    
    // NonAggregate n1{1, 2, 3};  // 編譯錯誤！不是聚合類別
    NonAggregate n2{};            // 呼叫預設建構子
    
    std::cout << "NonAggregate n2{}: 呼叫預設建構子\n";
    std::cout << "NonAggregate n1{1,2,3}; // 編譯錯誤！\n";
    
    std::cout << "\n--- 聚合類別的條件 (C++11/14) ---\n";
    std::cout << "1. 沒有使用者宣告的建構子\n";
    std::cout << "2. 沒有私有或保護的非靜態成員\n";
    std::cout << "3. 沒有虛擬函式\n";
    std::cout << "4. 沒有虛擬、私有、保護的基礎類別\n";
}

// ===== 陷阱 9：initializer_list 的生命週期 =====

std::initializer_list<int> dangerousReturn()
{
    return {1, 2, 3};  // 危險！底層陣列是臨時的
}

void demonstrateLifetime()
{
    std::cout << "\n===== 陷阱 9：initializer_list 的生命週期 =====\n";
    
    std::cout << "\n--- 安全的使用方式 ---\n";
    
    // 安全：立即使用
    std::vector<int> v{1, 2, 3};  // 建構子會複製元素
    std::cout << "std::vector<int> v{1, 2, 3}; // 安全：元素被複製\n";
    
    // 安全：在同一表達式中
    for (int x : {1, 2, 3})
    {
        // 底層陣列在整個 for 迴圈期間存活
    }
    std::cout << "for (int x : {1, 2, 3}) { ... } // 安全\n";
    
    std::cout << "\n--- 危險的使用方式 ---\n";
    
    std::cout << "std::initializer_list<int> dangerousReturn() {\n";
    std::cout << "    return {1, 2, 3};  // 危險！\n";
    std::cout << "}\n";
    std::cout << "底層陣列在函式返回後已銷毀，\n";
    std::cout << "使用回傳的 initializer_list 是未定義行為！\n";
    
    // auto list = dangerousReturn();  // 未定義行為！
    
    std::cout << "\n--- 危險：作為成員儲存 ---\n";
    
    std::cout << "class Holder {\n";
    std::cout << "    std::initializer_list<int> data_;  // 危險！\n";
    std::cout << "};\n";
    std::cout << "建構子結束後，底層陣列可能已銷毀\n";
    
    std::cout << "\n--- 正確做法：複製到容器 ---\n";
    
    std::cout << "class SafeHolder {\n";
    std::cout << "    std::vector<int> data_;\n";
    std::cout << "    SafeHolder(std::initializer_list<int> init)\n";
    std::cout << "        : data_(init) {}  // 複製元素，安全\n";
    std::cout << "};\n";
}

// ===== 陷阱 10：嵌套大括號的解析 =====

void demonstrateNestedBraces()
{
    std::cout << "\n===== 陷阱 10：嵌套大括號的解析 =====\n";
    
    std::cout << "\n--- std::pair 的初始化 ---\n";
    
    std::pair<int, int> p1(1, 2);      // 傳統方式
    std::pair<int, int> p2{1, 2};      // C++11
    std::pair<int, int> p3 = {1, 2};   // C++11
    
    std::cout << "pair<int,int> p1(1, 2):   " << p1.first << ", " << p1.second << "\n";
    std::cout << "pair<int,int> p2{1, 2}:   " << p2.first << ", " << p2.second << "\n";
    std::cout << "pair<int,int> p3 = {1,2}: " << p3.first << ", " << p3.second << "\n";
    
    std::cout << "\n--- std::map 的初始化 ---\n";
    
    std::map<std::string, int> m1{
        {"one", 1},
        {"two", 2},
        {"three", 3}
    };
    
    std::cout << "map initialized with nested braces:\n";
    for (const auto& [key, value] : m1)
    {
        std::cout << "  " << key << " -> " << value << "\n";
    }
    
    std::cout << "\n--- vector<vector<int>> ---\n";
    
    std::vector<std::vector<int>> matrix{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    
    std::cout << "matrix:\n";
    for (const auto& row : matrix)
    {
        std::cout << "  ";
        for (int n : row)
        {
            std::cout << n << " ";
        }
        std::cout << "\n";
    }
}

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 陷阱 1：initializer_list 優先順序 =====
    std::cout << "===== 陷阱 1：initializer_list 優先順序 =====\n\n";
    
    std::cout << "Container c1(5);\n";
    Container c1(5);
    
    std::cout << "\nContainer c2{5};\n";
    Container c2{5};        // initializer_list！不是 size
    
    std::cout << "\nContainer c3(5, 10);\n";
    Container c3(5, 10);    // size, value
    
    std::cout << "\nContainer c4{5, 10};\n";
    Container c4{5, 10};    // initializer_list！
    
    std::cout << "\nContainer c5({5, 10});\n";
    Container c5({5, 10});  // 明確使用 initializer_list
    
    // 陷阱 2
    demonstrateNarrowing();
    
    // 陷阱 3
    demonstrateAutoBrace();
    
    // 陷阱 4
    demonstrateEmptyBraces();
    
    // 陷阱 5
    demonstrateVectorPitfall();
    
    // 陷阱 6
    demonstrateConstructorSelection();
    
    // 陷阱 7
    demonstrateForceConstruct();
    
    // 陷阱 8
    demonstrateAggregate();
    
    // 陷阱 9
    demonstrateLifetime();
    
    // 陷阱 10
    demonstrateNestedBraces();
    
    return 0;
}
