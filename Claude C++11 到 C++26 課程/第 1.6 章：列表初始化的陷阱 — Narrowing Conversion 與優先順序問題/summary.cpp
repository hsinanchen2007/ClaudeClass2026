// ============================================================================
// 第 1.6 章：列表初始化的陷阱 — Narrowing Conversion 與優先順序問題（總複習）
// ============================================================================
// 編譯指令: g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
//
// 本檔案是第 1.6 章的完整總複習，涵蓋列表初始化（brace initialization）的
// 十大常見陷阱，包含完整可編譯的程式碼範例與詳盡的中文註解。
// 閱讀本檔案即可掌握本章所有核心概念。
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <initializer_list>
#include <type_traits>
#include <map>
#include <cstdint>

// ============================================================================
// 【陷阱 1】initializer_list 建構子的優先順序
// ============================================================================
//
// 核心觀念：
//   當類別同時擁有「一般建構子」和「std::initializer_list 建構子」時，
//   使用 {} 初始化會「優先」匹配 initializer_list 建構子。
//   這是 C++11 列表初始化中最常被忽略的規則。
//
// 規則整理：
//   - 使用 () 初始化 → 遵循傳統的多載解析規則
//   - 使用 {} 初始化 → 編譯器會先嘗試匹配 initializer_list 建構子
//     只有在完全無法匹配時，才會退回到一般建構子
//
// 實務影響：
//   Container c(5);    → 呼叫 Container(size_t) — 建立大小為 5 的容器
//   Container c{5};    → 呼叫 Container(initializer_list<int>) — 建立含有元素 5 的容器
//   兩者語意完全不同！
// ============================================================================

class Container
{
public:
    // 建構子 1：指定容器大小
    explicit Container(std::size_t size)
    {
        std::cout << "  Container(size_t size=" << size << ")\n";
    }

    // 建構子 2：指定大小與初始值
    Container(std::size_t size, int value)
    {
        std::cout << "  Container(size_t size=" << size
                  << ", int value=" << value << ")\n";
    }

    // 建構子 3：接受 initializer_list
    // 一旦存在此建構子，{} 初始化就會優先匹配它
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

void demo_trap1_initializer_list_priority()
{
    std::cout << "===== 【陷阱 1】initializer_list 建構子的優先順序 =====\n\n";

    // 使用 () → 傳統多載解析，匹配 Container(size_t)
    std::cout << "Container c1(5);        → ";
    Container c1(5);

    // 使用 {} → 優先匹配 initializer_list<int>，將 5 當作列表的一個元素
    std::cout << "Container c2{5};        → ";
    Container c2{5};

    // 使用 () → 匹配 Container(size_t, int)
    std::cout << "Container c3(5, 10);    → ";
    Container c3(5, 10);

    // 使用 {} → 優先匹配 initializer_list<int>，列表包含 5 和 10
    std::cout << "Container c4{5, 10};    → ";
    Container c4{5, 10};

    // 明確傳入 initializer_list 物件
    std::cout << "Container c5({5, 10});  → ";
    Container c5({5, 10});

    std::cout << "\n";
}

// ============================================================================
// 【陷阱 2】窄化轉換（Narrowing Conversion）
// ============================================================================
//
// 核心觀念：
//   C++11 的列表初始化（{} 初始化）禁止「窄化轉換」。
//   窄化轉換是指可能導致資料損失的隱式型別轉換。
//   傳統的 () 和 = 初始化則不會禁止窄化轉換。
//
// 窄化轉換的四大類型：
//   (a) 浮點數 → 整數          例: double → int
//   (b) 大整數 → 小整數        例: long long → int
//   (c) 有號 ↔ 無號 整數       例: int → unsigned（負數會出問題）
//   (d) 指標 → bool            例: int* → bool
//
// 常量表達式的例外：
//   如果 {} 內是編譯期常量，且值可以精確表示為目標型別，則允許。
//   例: int x{3};     → 合法（3 可以精確表示為 int）
//       int x{3.14};  → 錯誤（3.14 有小數部分，無法精確表示為 int）
//       int x{100LL}; → 合法（100 在 int 範圍內）
//
// 設計目的：
//   防止程式設計師在不知不覺中進行可能丟失資料的轉換。
//   這是 {} 初始化相較於 () 初始化的一大安全優勢。
// ============================================================================

void demo_trap2_narrowing_conversion()
{
    std::cout << "===== 【陷阱 2】窄化轉換 =====\n";

    // --- 2.1 浮點數 → 整數 ---
    std::cout << "\n--- 2.1 浮點數 → 整數 ---\n";

    double d = 3.14;

    int a(d);       // 合法：使用 () 不會禁止窄化，a = 3（截斷小數）
    int b = d;      // 合法：使用 = 不會禁止窄化，b = 3
    // int c{d};    // 編譯錯誤！d 是 double 變數，{} 禁止窄化轉換
    // int e = {d}; // 編譯錯誤！同上

    // 常量表達式例外：值在範圍內且無小數部分時允許
    int f{3};       // 合法：3 是整數常量，可精確表示為 int
    // int g{3.14}; // 編譯錯誤！3.14 有小數部分

    std::cout << "int a(3.14) = " << a << "  ← () 允許，但截斷小數\n";
    std::cout << "int f{3}    = " << f << "  ← {} 允許，整數常量無損失\n";
    std::cout << "int c{d};        ← 編譯錯誤！{} 禁止 double→int 窄化\n";

    // --- 2.2 大整數 → 小整數 ---
    std::cout << "\n--- 2.2 大整數 → 小整數 ---\n";

    long long big = 1000;

    int i1(big);     // 合法：使用 () 不會報錯
    int i2 = big;    // 合法：使用 = 不會報錯
    // int i3{big};  // 編譯錯誤！即使 big 的值 1000 放得進 int，
                     // 但 big 是變數（非常量表達式），編譯器無法保證安全

    int i4{100LL};   // 合法：100LL 是常量表達式，100 在 int 範圍內
    // int i5{10'000'000'000LL}; // 編譯錯誤！超出 int 範圍

    std::cout << "int i1(big)  = " << i1 << "  ← () 允許\n";
    std::cout << "int i4{100LL} = " << i4 << " ← {} 允許，常量在 int 範圍內\n";
    std::cout << "int i3{big};       ← 編譯錯誤！big 是變數，即使值放得下\n";

    // --- 2.3 有號 ↔ 無號 轉換 ---
    std::cout << "\n--- 2.3 有號 ↔ 無號 轉換 ---\n";

    int negative = -1;

    unsigned u1(negative);    // 合法但危險：-1 變成一個非常大的無號數
    unsigned u2 = negative;   // 合法但危險：同上
    // unsigned u3{negative}; // 編譯錯誤！{} 禁止有號→無號的窄化

    unsigned u4{0};           // 合法：0 是常量且在無號範圍內
    // unsigned u5{-1};       // 編譯錯誤！-1 是負數常量，不在 unsigned 範圍內

    std::cout << "unsigned u1(-1)   = " << u1 << "  ← () 允許，但值是垃圾\n";
    std::cout << "unsigned u3{negative}; ← 編譯錯誤！{} 禁止有號→無號窄化\n";

    // --- 2.4 指標 → bool ---
    std::cout << "\n--- 2.4 指標 → bool ---\n";

    int* ptr = nullptr;

    bool b1(ptr);    // 合法：指標隱式轉換為 bool
    bool b2 = ptr;   // 合法：同上
    // bool b3{ptr}; // 編譯錯誤！C++11 禁止指標→bool 的窄化轉換

    std::cout << "bool b1(nullptr) = " << std::boolalpha << b1 << "  ← () 允許\n";
    std::cout << "bool b3{ptr};          ← 編譯錯誤！{} 禁止指標→bool\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 3】auto 與大括號的推導規則
// ============================================================================
//
// 核心觀念：
//   auto 搭配 {} 的型別推導在 C++11/14 和 C++17 之間有重大變化。
//
// 規則對照表：
//   語法               C++11/14                    C++17
//   -----------------------------------------------------------------
//   auto x = {1,2,3}   initializer_list<int>       initializer_list<int>
//   auto x = {42}      initializer_list<int>       initializer_list<int>
//   auto x{42}         initializer_list<int>       int（重大改變！）
//   auto x{1,2,3}      initializer_list<int>       編譯錯誤
//
// 重點變化（C++17）：
//   auto x{42}; 不再推導為 initializer_list<int>，而是推導為 int。
//   auto x{1,2,3}; 變成編譯錯誤（直接列表初始化只允許一個元素）。
//
// 實務建議：
//   - 如果要明確建立 initializer_list，使用 auto x = {1, 2, 3}; 語法
//   - 如果只要單一值，使用 auto x{42}; 在 C++17 會推導為 int
// ============================================================================

void demo_trap3_auto_brace()
{
    std::cout << "===== 【陷阱 3】auto 與大括號的推導規則 =====\n";

    // --- auto = {...} 語法（C++11/14/17 行為一致） ---
    std::cout << "\n--- auto = {...} ---\n";

    auto a = {1, 2, 3};  // 型別: std::initializer_list<int>，所有版本皆同
    auto b = {42};        // 型別: std::initializer_list<int>（即使只有一個元素！）

    std::cout << "auto a = {1, 2, 3}; → initializer_list<int>, size=" << a.size() << "\n";
    std::cout << "auto b = {42};      → initializer_list<int>, size=" << b.size() << "\n";

    // --- auto x{value} 語法（C++17 行為改變） ---
    std::cout << "\n--- auto x{value} (C++17 改變) ---\n";

    auto c{42};           // C++17: 推導為 int（不是 initializer_list）
    // auto d{1, 2};      // C++17: 編譯錯誤！直接列表初始化不允許多元素

    std::cout << "auto c{42}; → type is "
              << (std::is_same<decltype(c), int>::value ? "int" : "other")
              << ", value=" << c << "\n";

    // --- 完整對比表 ---
    std::cout << "\n--- 對比表 ---\n";
    std::cout << "語法              C++11/14               C++17\n";
    std::cout << "auto x = {1,2,3}  initializer_list<int>  initializer_list<int>\n";
    std::cout << "auto x = {42}     initializer_list<int>  initializer_list<int>\n";
    std::cout << "auto x{42}        initializer_list<int>  int\n";
    std::cout << "auto x{1,2,3}     initializer_list<int>  編譯錯誤\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 4】空大括號 {} 的歧義
// ============================================================================
//
// 核心觀念：
//   當類別同時有「預設建構子」和「initializer_list 建構子」時，
//   空大括號 {} 的行為是：呼叫「預設建構子」，而不是 initializer_list。
//
// 規則：
//   Widget w{};    → 呼叫預設建構子（特殊規則：空 {} 優先匹配預設建構子）
//   Widget w({});  → 呼叫 initializer_list 建構子，傳入空列表
//   Widget w{{}}; → 呼叫 initializer_list 建構子，列表含一個預設初始化的元素
//
// 陷阱點：
//   std::vector<int> v{{}};  → 包含 1 個元素（值為 0），不是空 vector！
//   因為 {} 被解析為 initializer_list 中的一個「預設初始化的 int（= 0）」
// ============================================================================

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

void demo_trap4_empty_braces()
{
    std::cout << "===== 【陷阱 4】空大括號 {} 的歧義 =====\n";

    // --- 有 initializer_list 建構子的類別 ---
    std::cout << "\n--- Widget（有 initializer_list 建構子）---\n";

    std::cout << "Widget w1;      → ";
    Widget w1;          // 預設建構子

    std::cout << "Widget w2{};    → ";
    Widget w2{};        // 預設建構子（空 {} 優先匹配預設建構子）

    std::cout << "Widget w3({});  → ";
    Widget w3({});      // initializer_list 建構子，傳入空列表

    std::cout << "Widget w4{{}}; → ";
    Widget w4{{}};      // initializer_list 建構子，列表含一個 int{} = 0

    // --- 無 initializer_list 建構子的類別 ---
    std::cout << "\n--- Gadget（無 initializer_list 建構子）---\n";

    std::cout << "Gadget g1;      → ";
    Gadget g1;          // 預設建構子

    std::cout << "Gadget g2{};    → ";
    Gadget g2{};        // 預設建構子

    // --- std::vector 的案例（最容易出錯的地方）---
    std::cout << "\n--- std::vector 的空大括號陷阱 ---\n";

    std::vector<int> v1;      // 空 vector
    std::vector<int> v2{};    // 空 vector（空 {} → 預設建構子）
    std::vector<int> v3({});  // 空 vector（透過空 initializer_list 建構）
    std::vector<int> v4{{}};  // 注意：包含 1 個元素（值為 0）！

    std::cout << "vector<int> v1;     size = " << v1.size() << "\n";
    std::cout << "vector<int> v2{};   size = " << v2.size() << "\n";
    std::cout << "vector<int> v3({}); size = " << v3.size() << "\n";
    std::cout << "vector<int> v4{{}}; size = " << v4.size()
              << " (元素值: " << v4[0] << ") ← 不是空的！\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 5】std::vector 的著名陷阱 — () vs {}
// ============================================================================
//
// 核心觀念：
//   std::vector<int> 有多個建構子，其中：
//     vector(size_t count)           → 建立 count 個元素（值為 0）
//     vector(size_t count, int val)  → 建立 count 個元素（值為 val）
//     vector(initializer_list<int>)  → 用列表中的值初始化元素
//
//   使用 () 和 {} 會呼叫完全不同的建構子：
//     vector<int> v(5);      → 5 個元素，值都是 0
//     vector<int> v{5};      → 1 個元素，值是 5
//     vector<int> v(5, 10);  → 5 個元素，值都是 10
//     vector<int> v{5, 10};  → 2 個元素，值是 5 和 10
//
// 實務建議：
//   - 想指定「大小」時使用 ()
//   - 想指定「元素內容」時使用 {}
// ============================================================================

void demo_trap5_vector_pitfall()
{
    std::cout << "===== 【陷阱 5】std::vector 的著名陷阱 =====\n";

    // --- int 版本 ---
    std::cout << "\n--- vector<int> ---\n";

    std::vector<int> v1(5);       // 5 個元素，全部為 0
    std::vector<int> v2{5};       // 1 個元素，值為 5
    std::vector<int> v3(5, 10);   // 5 個元素，全部為 10
    std::vector<int> v4{5, 10};   // 2 個元素：5 和 10

    // 輔助 lambda：印出 vector 內容
    auto printVec = [](const std::string& name, const std::vector<int>& v) {
        std::cout << name << " (size=" << v.size() << "): {";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << v[i];
        }
        std::cout << "}\n";
    };

    printVec("v1(5)    ", v1);    // {0, 0, 0, 0, 0}
    printVec("v2{5}    ", v2);    // {5}
    printVec("v3(5, 10)", v3);    // {10, 10, 10, 10, 10}
    printVec("v4{5, 10}", v4);    // {5, 10}

    // --- string 版本（展示 {} 不匹配時的退回行為）---
    std::cout << "\n--- vector<string> ---\n";

    std::vector<std::string> s1(3);            // 3 個空字串
    // std::vector<std::string> s2{3};         // 編譯錯誤！int 無法轉為 string
    std::vector<std::string> s3(3, "hello");   // 3 個 "hello"
    std::vector<std::string> s4{"a", "b"};     // 2 個元素："a" 和 "b"

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
    printStrVec("s3(3, \"hello\")", s3);
    printStrVec("s4{\"a\", \"b\"} ", s4);
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 6】建構子選擇的微妙差異 — 即使轉換不完美也優先匹配
// ============================================================================
//
// 核心觀念：
//   當 {} 內的值「可以轉換」為 initializer_list 的元素型別時，
//   即使存在更精確匹配的一般建構子，編譯器仍然會優先選擇 initializer_list 版本。
//
// 範例：
//   class Converter 有三個建構子：
//     Converter(int)
//     Converter(double)
//     Converter(initializer_list<double>)
//
//   Converter c(42);   → 呼叫 Converter(int)      （精確匹配）
//   Converter c{42};   → 呼叫 Converter(initializer_list<double>)
//                        因為 42（int）可以隱式轉換為 double
//
// 結論：
//   只要 {} 內的值可以轉換為 initializer_list 的元素型別，
//   initializer_list 建構子就會「劫持」所有 {} 初始化。
// ============================================================================

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

void demo_trap6_constructor_selection()
{
    std::cout << "===== 【陷阱 6】建構子選擇的微妙差異 =====\n";

    std::cout << "\nConverter c1(42);   → ";
    Converter c1(42);       // 呼叫 Converter(int)

    std::cout << "Converter c2{42};   → ";
    Converter c2{42};       // 呼叫 initializer_list<double>！
                            // 因為 42 (int) 可以隱式轉換為 double

    std::cout << "Converter c3(3.14); → ";
    Converter c3(3.14);     // 呼叫 Converter(double)

    std::cout << "Converter c4{3.14}; → ";
    Converter c4{3.14};     // 呼叫 initializer_list<double>

    std::cout << "\n重點：即使存在更精確的 Converter(int) 建構子，\n";
    std::cout << "      {} 初始化仍然優先匹配 initializer_list<double>，\n";
    std::cout << "      因為 int 可以隱式轉換為 double。\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 7】如何強制呼叫非 initializer_list 建構子
// ============================================================================
//
// 核心觀念：
//   當類別有 initializer_list 建構子時，使用 {} 初始化會被「劫持」。
//   要繞過這個行為，必須使用 () 初始化。
//
// 解決方案：
//   方案 1：使用小括號 () 初始化（最簡單、最直接）
//     ForceConstruct f(5, 10);   → 呼叫 (int, int) 建構子
//     ForceConstruct f{5, 10};   → 呼叫 initializer_list 建構子
//
//   方案 2：讓元素型別無法轉換為 initializer_list 的元素型別
//     （通常不實用，僅理論上可行）
//
// 實務建議：
//   記住一個原則 —
//     {} → 優先 initializer_list
//     () → 匹配最佳的非 initializer_list 建構子
// ============================================================================

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

void demo_trap7_force_non_initlist()
{
    std::cout << "===== 【陷阱 7】如何強制呼叫非 initializer_list 建構子 =====\n";

    // 問題：想呼叫 (int, int) 版本
    std::cout << "\n--- 問題：{} 被 initializer_list 劫持 ---\n";
    std::cout << "ForceConstruct f1{5, 10};  → ";
    ForceConstruct f1{5, 10};   // 匹配 initializer_list 版本

    // 解決方案：使用 () 初始化
    std::cout << "\n--- 解決：使用 () 呼叫 ---\n";
    std::cout << "ForceConstruct f2(5, 10);  → ";
    ForceConstruct f2(5, 10);   // 匹配 (int, int) 版本

    std::cout << "\n結論：\n";
    std::cout << "  使用 {} → 優先匹配 initializer_list 建構子\n";
    std::cout << "  使用 () → 匹配最佳的非 initializer_list 建構子\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 8】聚合類別（Aggregate）的特殊初始化規則
// ============================================================================
//
// 核心觀念：
//   聚合類別可以使用 {} 進行「聚合初始化」，不需要建構子。
//   未提供的成員會被值初始化（對基本型別來說就是 0）。
//
// 聚合類別的條件（C++11/14）：
//   1. 沒有使用者宣告的建構子
//   2. 沒有私有（private）或保護（protected）的非靜態成員
//   3. 沒有虛擬函式（virtual function）
//   4. 沒有虛擬、私有、保護的基礎類別
//
// 聚合初始化 vs 列表初始化：
//   聚合初始化使用 {} 時，按照成員宣告順序依次賦值。
//   非聚合類別無法進行聚合初始化，必須透過建構子。
//
// C++17 擴展：
//   C++17 允許聚合類別有基礎類別（public、非虛擬），
//   並且可以在 {} 中包含基礎類別的初始值。
// ============================================================================

// 聚合類別：沒有使用者定義的建構子
struct Aggregate
{
    int x;
    int y;
    int z;
};

// 非聚合類別：有使用者定義的建構子
struct NonAggregate
{
    int x;
    int y;
    int z;

    NonAggregate() : x{0}, y{0}, z{0} {}  // 使用者定義的建構子 → 不再是聚合類別
};

void demo_trap8_aggregate()
{
    std::cout << "===== 【陷阱 8】聚合類別的特殊初始化規則 =====\n";

    // --- 聚合類別：可以直接用 {} 按成員順序初始化 ---
    std::cout << "\n--- 聚合類別 Aggregate ---\n";

    Aggregate a1{1, 2, 3};   // x=1, y=2, z=3
    Aggregate a2{1, 2};      // x=1, y=2, z=0（未提供的成員值初始化為 0）
    Aggregate a3{1};          // x=1, y=0, z=0
    Aggregate a4{};           // x=0, y=0, z=0（全部值初始化為 0）

    std::cout << "Aggregate a1{1, 2, 3}: x=" << a1.x
              << ", y=" << a1.y << ", z=" << a1.z << "\n";
    std::cout << "Aggregate a2{1, 2}:    x=" << a2.x
              << ", y=" << a2.y << ", z=" << a2.z << "\n";
    std::cout << "Aggregate a3{1}:       x=" << a3.x
              << ", y=" << a3.y << ", z=" << a3.z << "\n";
    std::cout << "Aggregate a4{}:        x=" << a4.x
              << ", y=" << a4.y << ", z=" << a4.z << "\n";

    // --- 非聚合類別：必須透過建構子 ---
    std::cout << "\n--- 非聚合類別 NonAggregate ---\n";

    // NonAggregate n1{1, 2, 3};  // 編譯錯誤！不是聚合類別，無法聚合初始化
    NonAggregate n2{};            // 呼叫預設建構子

    std::cout << "NonAggregate n2{}: 呼叫預設建構子\n";
    std::cout << "NonAggregate n1{1,2,3}; → 編譯錯誤！不是聚合類別\n";

    std::cout << "\n--- 聚合類別的條件 (C++11/14) ---\n";
    std::cout << "1. 沒有使用者宣告的建構子\n";
    std::cout << "2. 沒有私有或保護的非靜態成員\n";
    std::cout << "3. 沒有虛擬函式\n";
    std::cout << "4. 沒有虛擬、私有、保護的基礎類別\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 9】initializer_list 的生命週期問題
// ============================================================================
//
// 核心觀念：
//   std::initializer_list 只是一個輕量級的「參考」物件，
//   它不擁有底層的陣列資料。底層陣列是一個臨時物件。
//
// 安全的使用方式：
//   (a) 立即用於初始化容器：
//       std::vector<int> v{1, 2, 3};  → 安全，元素被複製到 vector
//   (b) 在 range-for 中使用：
//       for (int x : {1, 2, 3}) { ... }  → 安全，底層陣列在迴圈期間存活
//
// 危險的使用方式：
//   (a) 從函式回傳 initializer_list：
//       std::initializer_list<int> f() { return {1,2,3}; }
//       → 危險！底層陣列在函式返回後已銷毀，使用回傳值是未定義行為
//   (b) 將 initializer_list 作為類別成員儲存：
//       class Holder { std::initializer_list<int> data_; };
//       → 危險！建構子結束後底層陣列可能已銷毀
//
// 正確做法：
//   將 initializer_list 的元素複製到 std::vector 等容器中儲存。
// ============================================================================

// 危險範例：從函式回傳 initializer_list
std::initializer_list<int> dangerousReturn()
{
    return {1, 2, 3};  // 危險！底層陣列是函式內的臨時物件
}

void demo_trap9_lifetime()
{
    std::cout << "===== 【陷阱 9】initializer_list 的生命週期問題 =====\n";

    // --- 安全用法 1：立即用於初始化容器 ---
    std::cout << "\n--- 安全用法 ---\n";

    std::vector<int> v{1, 2, 3};  // 安全：vector 的建構子會複製所有元素
    std::cout << "std::vector<int> v{1, 2, 3}; → 安全，元素被複製到 vector\n";

    // --- 安全用法 2：在 range-for 中使用 ---
    for (int x : {1, 2, 3})
    {
        // 底層陣列在整個 for 迴圈期間存活
        (void)x;  // 避免未使用變數的警告
    }
    std::cout << "for (int x : {1, 2, 3}) { ... } → 安全，陣列在迴圈期間存活\n";

    // --- 危險用法 1：從函式回傳 initializer_list ---
    std::cout << "\n--- 危險用法 ---\n";

    std::cout << "std::initializer_list<int> dangerousReturn() {\n";
    std::cout << "    return {1, 2, 3};  // 危險！底層陣列在返回後已銷毀\n";
    std::cout << "}\n";
    std::cout << "→ 使用回傳的 initializer_list 是未定義行為！\n";

    // auto list = dangerousReturn();  // 未定義行為！請勿使用

    // --- 危險用法 2：作為類別成員儲存 ---
    std::cout << "\n--- 危險：作為成員儲存 ---\n";
    std::cout << "class Holder {\n";
    std::cout << "    std::initializer_list<int> data_;  // 危險！\n";
    std::cout << "};\n";
    std::cout << "→ 建構子結束後，底層陣列可能已銷毀\n";

    // --- 正確做法：複製到容器 ---
    std::cout << "\n--- 正確做法 ---\n";
    std::cout << "class SafeHolder {\n";
    std::cout << "    std::vector<int> data_;  // 安全！\n";
    std::cout << "    SafeHolder(std::initializer_list<int> init)\n";
    std::cout << "        : data_(init) {}  // 元素被複製到 vector\n";
    std::cout << "};\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 10】嵌套大括號的解析
// ============================================================================
//
// 核心觀念：
//   C++11 的列表初始化允許嵌套使用 {}，
//   這在初始化 pair、map、vector<vector<int>> 等複合型別時非常方便。
//
// 常見用法：
//   std::pair<int, int> p = {1, 2};           → 直覺的 pair 初始化
//   std::map<string, int> m = {{"a", 1}};     → 嵌套 {} 初始化 map
//   std::vector<vector<int>> mat = {{1,2}, {3,4}};  → 二維矩陣
//
// 注意事項：
//   嵌套層級越多，可讀性越低。
//   適度使用可以讓程式碼更簡潔，過度使用則會造成混淆。
// ============================================================================

void demo_trap10_nested_braces()
{
    std::cout << "===== 【陷阱 10】嵌套大括號的解析 =====\n";

    // --- std::pair 的初始化 ---
    std::cout << "\n--- std::pair 的初始化 ---\n";

    std::pair<int, int> p1(1, 2);      // 傳統的 () 方式
    std::pair<int, int> p2{1, 2};      // C++11 的 {} 方式
    std::pair<int, int> p3 = {1, 2};   // C++11 的 = {} 方式

    std::cout << "pair p1(1, 2):   " << p1.first << ", " << p1.second << "\n";
    std::cout << "pair p2{1, 2}:   " << p2.first << ", " << p2.second << "\n";
    std::cout << "pair p3 = {1,2}: " << p3.first << ", " << p3.second << "\n";

    // --- std::map 的嵌套初始化 ---
    std::cout << "\n--- std::map 的嵌套初始化 ---\n";

    std::map<std::string, int> m1{
        {"one", 1},      // 每個 {} 是一個 pair<string, int>
        {"two", 2},
        {"three", 3}
    };

    std::cout << "map 使用嵌套 {} 初始化:\n";
    for (const auto& [key, value] : m1)  // C++17 結構化繫結
    {
        std::cout << "  " << key << " -> " << value << "\n";
    }

    // --- vector<vector<int>> 二維矩陣 ---
    std::cout << "\n--- vector<vector<int>> 二維矩陣 ---\n";

    std::vector<std::vector<int>> matrix{
        {1, 2, 3},      // 第 0 列
        {4, 5, 6},      // 第 1 列
        {7, 8, 9}       // 第 2 列
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
    std::cout << "\n";
}

// ============================================================================
// 主程式：依序展示所有十大陷阱
// ============================================================================

int main()
{
    std::cout << std::boolalpha;

    std::cout << "================================================================\n";
    std::cout << " 第 1.6 章 總複習：列表初始化的陷阱 — 十大陷阱完整展示\n";
    std::cout << "================================================================\n\n";

    demo_trap1_initializer_list_priority();     // 陷阱 1：initializer_list 優先順序
    demo_trap2_narrowing_conversion();          // 陷阱 2：窄化轉換
    demo_trap3_auto_brace();                    // 陷阱 3：auto 與大括號
    demo_trap4_empty_braces();                  // 陷阱 4：空大括號的歧義
    demo_trap5_vector_pitfall();                // 陷阱 5：vector 的 () vs {}
    demo_trap6_constructor_selection();          // 陷阱 6：建構子選擇差異
    demo_trap7_force_non_initlist();            // 陷阱 7：強制呼叫非 initializer_list
    demo_trap8_aggregate();                     // 陷阱 8：聚合類別特殊規則
    demo_trap9_lifetime();                      // 陷阱 9：initializer_list 生命週期
    demo_trap10_nested_braces();                // 陷阱 10：嵌套大括號解析

    std::cout << "================================================================\n";
    std::cout << " 【總結】實務建議\n";
    std::cout << "================================================================\n";
    std::cout << "\n";
    std::cout << "1. 當類別有 initializer_list 建構子時，{} 會優先匹配它\n";
    std::cout << "2. {} 初始化禁止窄化轉換，這是安全優勢\n";
    std::cout << "3. auto 與 {} 的推導規則在 C++17 有重大變化\n";
    std::cout << "4. 空 {} 優先匹配預設建構子，而非 initializer_list\n";
    std::cout << "5. vector<int> v(5) 和 v{5} 語意完全不同\n";
    std::cout << "6. 即使轉換不完美，{} 仍優先匹配 initializer_list\n";
    std::cout << "7. 想繞過 initializer_list 優先權，使用 () 初始化\n";
    std::cout << "8. 聚合類別可以直接用 {} 按成員順序初始化\n";
    std::cout << "9. 不要回傳或儲存 initializer_list，改用 vector\n";
    std::cout << "10. 嵌套 {} 適合初始化複合型別，但不要過度嵌套\n";
    std::cout << "\n";

    return 0;
}
