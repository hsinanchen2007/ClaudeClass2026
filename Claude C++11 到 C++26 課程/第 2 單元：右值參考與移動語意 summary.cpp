// ╔══════════════════════════════════════════════════════════════════╗
// ║  第 2 單元：右值參考與移動語意 — 完整教學                        ║
// ║  從零開始，一步一步帶你徹底搞懂 C++11 最重要的特性              ║
// ║  編譯：g++ -std=c++17 -O2 -o summary "第 2 單元：右值參考與移動語意 summary.cpp" ║
// ╚══════════════════════════════════════════════════════════════════╝
//
// 本檔案是一本「迷你書」，涵蓋以下章節：
//   第 1 章：為什麼需要移動語意？— 從一個效能問題說起
//   第 2 章：左值與右值 — 你必須先搞懂的基礎
//   第 3 章：引用家族 — T&、const T&、T&&
//   第 4 章：右值引用的重要陷阱 — 有名字就是左值
//   第 5 章：深拷貝的代價 — 理解「為什麼慢」
//   第 6 章：移動建構子 — 偷取資源的藝術
//   第 7 章：移動賦值運算子 — 偷取 + 清理
//   第 8 章：統一賦值（Copy-and-Swap）— 一招打天下
//   第 9 章：Rule of Three → Five → Zero
//   第 10 章：std::move 的真相 — 它什麼都沒移動
//   第 11 章：noexcept — 一個關鍵字值百倍效能
//   第 12 章：std::forward 與完美轉發
//   第 13 章：實戰模式與最佳實踐
//   第 14 章：效能實測
//   第 15 章：常見錯誤與陷阱大全

#include <iostream>
#include <cstring>      // strlen, strcpy
#include <utility>      // std::move, std::swap, std::forward
#include <string>
#include <vector>
#include <memory>       // unique_ptr
#include <type_traits>  // is_move_constructible 等
#include <algorithm>    // std::copy
#include <chrono>
#include <functional>

using namespace std;

// ================================================================
//  輔助工具
// ================================================================
void title(const char* ch) {
    cout << "\n";
    cout << "================================================================\n";
    cout << "  " << ch << "\n";
    cout << "================================================================\n\n";
}

void subtitle(const char* s) {
    cout << "--- " << s << " ---\n";
}


// ================================================================
//
//  第 1 章：為什麼需要移動語意？— 從一個效能問題說起
//
// ================================================================
// 想像你有一個類別管理一大塊動態記憶體（例如 100 萬個整數的陣列）。
// 每次拷貝這個物件，就要：
//   1. 配置一塊新的 100 萬個整數的記憶體
//   2. 把舊的 100 萬個整數一個一個複製過去
// 這非常慢！
//
// 但如果你知道某個物件「馬上就要被丟掉了」（例如一個臨時物件），
// 何必費力複製？直接「偷走」它的記憶體不就好了？
//
// 這就是移動語意的核心思想：
//   【如果來源馬上要消失，就偷它的資源，而不是複製】
//
// 問題是：C++ 怎麼知道一個物件「馬上要消失」？
// 答案就是：右值引用（Rvalue Reference）。

void chapter1() {
    title("第 1 章：為什麼需要移動語意？");

    // 先用 string 直觀感受「複製 vs 移動」的差異
    string original(100000, 'A');  // 一個 10 萬字元的字串

    // 複製：配置新記憶體 + 逐字元複製 → 慢
    auto t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        string copy = original;  // 每次都要 new + memcpy
        (void)copy;
    }
    auto t2 = chrono::high_resolution_clock::now();
    auto copy_ms = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

    // 移動：只偷指標 → 快
    auto t3 = chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        string temp = original;           // 先複製一份（公平比較）
        string moved = std::move(temp);   // 這步是 O(1) 的移動
        (void)moved;
    }
    auto t4 = chrono::high_resolution_clock::now();
    auto move_ms = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();

    cout << "  複製 10000 次 (10 萬字元): " << copy_ms << " ms\n";
    cout << "  複製+移動 10000 次:        " << move_ms << " ms\n";
    cout << "  （移動本身幾乎是零成本，差異主要來自避免第二次複製）\n\n";

    cout << "  【核心問題】如何讓 C++ 知道何時可以「偷」而非「複製」？\n";
    cout << "  【答案】右值引用 T&& — 用來標記「可以被偷走資源」的物件\n";
}


// ================================================================
//
//  第 2 章：左值與右值 — 你必須先搞懂的基礎
//
// ================================================================
// 在學右值引用之前，你必須先理解什麼是左值、什麼是右值。
//
// 【最簡單的判斷法】
//   能放在 = 左邊的 → 左值（有名字、有地址、可以取 &）
//   只能放在 = 右邊的 → 右值（臨時的、沒名字、不能取 &）
//
// 【左值範例】
//   int x = 42;        x 是左值（有名字 "x"、可以 &x）
//   int& ref = x;      ref 是左值
//   arr[0]             陣列元素是左值
//   *ptr               解引用是左值
//   "Hello"            字串字面值是左值（特殊！存在靜態區）
//   ++x                前置遞增回傳引用，是左值
//
// 【右值範例】
//   42                 字面值是右值
//   x + y              運算結果是右值（臨時的）
//   func()             回傳值（非引用）是右值
//   string("temp")     臨時物件是右值
//   x++                後置遞增回傳舊值副本，是右值

void chapter2() {
    title("第 2 章：左值與右值");

    int x = 42;

    // 左值：有名字、可以取地址
    subtitle("左值範例");
    cout << "  x 是左值，&x = " << &x << "\n";
    int& ref = x;
    cout << "  ref 是左值，&ref = " << &ref << " (和 &x 相同)\n";

    int arr[3] = {10, 20, 30};
    cout << "  arr[0] 是左值，&arr[0] = " << &arr[0] << "\n";

    // 前置 vs 後置遞增
    int y = 10;
    int& pre = ++y;   // ++y 回傳引用 → 左值 → 可以取地址
    // int& post = y++;  // ❌ y++ 回傳舊值副本 → 右值 → 不能綁到 T&
    cout << "  ++y 是左值（回傳引用），y++ 是右值（回傳副本）\n\n";

    // 右值：臨時的、不能取地址
    subtitle("右值範例");
    cout << "  42 是右值（字面值）\n";
    cout << "  x + 1 = " << (x + 1) << " 是右值（運算結果）\n";
    cout << "  string(\"temp\") 是右值（臨時物件）\n";
    // &42;              // ❌ 不能取右值的地址
    // &(x + 1);         // ❌ 不能取右值的地址
    // &string("temp");  // ❌ 不能取右值的地址

    cout << "\n  【記住】有名字 → 左值    沒名字 → 右值\n";
    cout << "  【記住】能 & 取址 → 左值    不能取址 → 右值\n";
}


// ================================================================
//
//  第 3 章：引用家族 — T&、const T&、T&&
//
// ================================================================
// C++ 有三種引用，各自有不同的綁定規則：
//
//   T&        左值引用       只能綁定左值
//   const T&  const 左值引用 可以綁定左值和右值（萬能！）
//   T&&       右值引用       只能綁定右值
//
// 為什麼 const T& 可以綁定右值？
//   因為 C++ 有一條特殊規則：const 引用可以延長臨時物件的生命週期。
//   這在 C++11 之前是唯一能「捕獲」臨時物件的方式。
//
// T&& 是 C++11 新增的。它的目的是：
//   讓你能區分「這個值是臨時的（可以偷）」和「這個值還要用（不能偷）」

void chapter3() {
    title("第 3 章：引用家族");

    int x = 42;

    // ---- T& 左值引用 ----
    subtitle("T& 左值引用 — 只能綁定左值");
    int& lref = x;          // ✅ 左值 → 左值引用
    cout << "  int& lref = x;        ✅ x 是左值\n";
    // int& bad = 42;        // ❌ 右值 → 不能綁到 T&
    cout << "  int& bad = 42;        ❌ 42 是右值（編譯錯誤）\n\n";

    // ---- const T& const 左值引用 ----
    subtitle("const T& — 萬能引用（左值右值都能綁）");
    const int& clref1 = x;  // ✅ 綁定左值
    const int& clref2 = 42; // ✅ 綁定右值（特殊規則！延長臨時物件生命週期）
    cout << "  const int& c1 = x;    ✅ 綁定左值\n";
    cout << "  const int& c2 = 42;   ✅ 綁定右值（延長生命週期）\n\n";

    // ---- T&& 右值引用 ----
    subtitle("T&& 右值引用 — 只能綁定右值");
    int&& rref = 42;         // ✅ 右值 → 右值引用
    // int&& bad = x;         // ❌ 左值 → 不能綁到 T&&
    cout << "  int&& rref = 42;      ✅ 42 是右值\n";
    cout << "  int&& bad = x;        ❌ x 是左值（編譯錯誤）\n";
    cout << "  int&& ok = move(x);   ✅ move(x) 把 x 轉成右值\n\n";

    // T&& 綁定後可以修改（和 const T& 不同！）
    rref = 100;
    cout << "  rref 綁定後可以修改：rref = " << rref << "\n";
    cout << "  而 const T& 綁定後不能修改\n\n";

    // 綁定規則總結
    subtitle("綁定規則總結表");
    cout << "  ┌─────────────┬───────────┬───────────┐\n";
    cout << "  │   引用類型   │  左值(x)  │ 右值(42) │\n";
    cout << "  ├─────────────┼───────────┼───────────┤\n";
    cout << "  │ T&          │    ✅     │    ❌     │\n";
    cout << "  │ const T&    │    ✅     │    ✅     │\n";
    cout << "  │ T&&         │    ❌     │    ✅     │\n";
    cout << "  └─────────────┴───────────┴───────────┘\n";

    (void)lref; (void)clref1; (void)clref2;
}


// ================================================================
//
//  第 4 章：右值引用的重要陷阱 — 有名字就是左值
//
// ================================================================
// 這是整個移動語意中最容易搞混的地方！
//
// int&& rref = 42;  // rref 綁定了右值 42
// 問題：rref 本身是左值還是右值？
// 答案：rref 是左值！因為它有名字 "rref"、可以取地址 &rref。
//
// 【關鍵規則】
//   右值引用「變數」本身是左值。
//   它的「型別」是 T&&，但它的「值類別」是左值。
//   型別和值類別是兩回事！

// 用來測試傳入的是左值還是右值
void detect(const string& s) { cout << "    收到左值: \"" << s << "\"\n"; }
void detect(string&& s)      { cout << "    收到右值: \"" << s << "\"\n"; }

// 函數參數 T&& 在函數內也是左值
void take_rvalue_ref(string&& s) {
    cout << "  在函數內，s 的型別是 string&&\n";
    cout << "  但 s 有名字 → s 是左值！\n";
    cout << "  直接傳 s:       ";
    detect(s);                   // ← 呼叫左值版本！
    cout << "  傳 move(s):     ";
    detect(std::move(s));        // ← 呼叫右值版本
}

void chapter4() {
    title("第 4 章：右值引用的重要陷阱 — 有名字就是左值");

    // 陷阱展示
    subtitle("右值引用變數本身是左值");
    int&& rref = 42;
    cout << "  rref 的型別是 int&&（右值引用）\n";
    cout << "  但 rref 有名字 → rref 是左值\n";
    cout << "  可以取地址：&rref = " << &rref << "\n\n";

    // 不能把左值綁到另一個右值引用
    // int&& rref2 = rref;         // ❌ rref 是左值！
    int&& rref2 = std::move(rref); // ✅ move 把它轉回右值
    cout << "  int&& rref2 = rref;         ❌ rref 是左值\n";
    cout << "  int&& rref2 = move(rref);   ✅ move 轉成右值\n\n";

    // 函數參數也一樣
    subtitle("函數參數 T&& 在函數內是左值");
    take_rvalue_ref(string("Hello"));
    cout << "\n";

    // 函數重載展示
    subtitle("函數重載：左值 vs 右值");
    string name = "Dragon";
    cout << "  傳左值:       ";
    detect(name);                     // 左值版本
    cout << "  傳臨時物件:   ";
    detect(string("Phoenix"));        // 右值版本
    cout << "  傳 move(name):";
    detect(std::move(name));          // 右值版本
    cout << "\n";

    cout << "  【鐵律】有名字 → 左值，不管它的型別是什麼\n";
    cout << "  【鐵律】要把左值「變回」右值，用 std::move\n";

    (void)rref2;
}


// ================================================================
//
//  第 5 章：深拷貝的代價 — 理解「為什麼慢」
//
// ================================================================
// 在進入移動建構子之前，我們先用一個簡單的類別來理解深拷貝有多貴。
//
// SimpleString 管理一個動態分配的 char 陣列：
//   建構：new char[n] + strcpy   → 配置 + 複製
//   拷貝：new char[n] + strcpy   → 又配置 + 又複製（深拷貝）
//   解構：delete[] data          → 釋放

// 這個類別我們會在後面的章節一步一步加上移動語意
class SimpleString {
    char* m_data;
    size_t m_len;
    static int s_copyCount;
    static int s_moveCount;

public:
    // ──── 一般建構 ────
    SimpleString(const char* str = "")
        : m_len(strlen(str)), m_data(new char[m_len + 1]) {
        strcpy(m_data, str);
    }

    // ──── 解構 ────
    ~SimpleString() {
        delete[] m_data;
    }

    // ──── 拷貝建構（深拷貝）────
    // 要做的事：1. 配置新記憶體  2. 複製所有資料
    // 時間複雜度：O(n)，n = 字串長度
    SimpleString(const SimpleString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        strcpy(m_data, other.m_data);
        ++s_copyCount;
    }

    // ──── 移動建構（偷資源）────
    // 要做的事：1. 偷指標  2. 歸零來源  3. 複製基本型別
    // 時間複雜度：O(1)，與資料大小無關！
    SimpleString(SimpleString&& other) noexcept
        : m_data(other.m_data)      // 步驟 1：偷走指標
        , m_len(other.m_len)        // 步驟 3：複製基本型別
    {
        other.m_data = nullptr;     // 步驟 2：歸零來源（讓它安全解構）
        other.m_len = 0;
        ++s_moveCount;
    }

    // ──── swap ────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──── 統一賦值（Copy-and-Swap）────
    SimpleString& operator=(SimpleString other) noexcept {
        swap(other);
        return *this;
    }

    // ──── 存取 ────
    const char* c_str() const { return m_data ? m_data : "(null)"; }
    size_t length() const { return m_len; }

    static void resetCounters() { s_copyCount = s_moveCount = 0; }
    static void printCounters() {
        cout << "    拷貝: " << s_copyCount << " 次, 移動: " << s_moveCount << " 次\n";
    }
};
int SimpleString::s_copyCount = 0;
int SimpleString::s_moveCount = 0;

void chapter5() {
    title("第 5 章：深拷貝的代價");

    subtitle("拷貝 = 配置新記憶體 + 複製全部資料 = O(n)");
    {
        SimpleString a("Hello, this is a string with some content!");
        cout << "  a = \"" << a.c_str() << "\"\n";
        cout << "  a 佔用 " << (a.length() + 1) << " bytes 的堆積記憶體\n\n";

        SimpleString::resetCounters();
        SimpleString b = a;  // 深拷貝：new + strcpy
        cout << "  b = a（深拷貝）:\n";
        cout << "    b = \"" << b.c_str() << "\"\n";
        cout << "    a 和 b 各自擁有獨立的記憶體 ✅\n";
        SimpleString::printCounters();
    }
    cout << "\n";

    subtitle("如果拷貝 10000 次呢？");
    {
        SimpleString source(string(10000, 'X').c_str());
        SimpleString::resetCounters();

        auto t1 = chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; ++i) {
            SimpleString copy = source;  // 每次 new 10001 bytes + strcpy
            (void)copy;
        }
        auto t2 = chrono::high_resolution_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        cout << "  拷貝 10000 個 10000 字元的字串: " << ms << " ms\n";
        SimpleString::printCounters();
        cout << "  每次拷貝都要：new char[10001] + memcpy(10001 bytes)\n";
        cout << "  這就是我們需要移動語意的原因！\n";
    }
}


// ================================================================
//
//  第 6 章：移動建構子 — 偷取資源的藝術
//
// ================================================================
// 移動建構子的簽名：
//   ClassName(ClassName&& other) noexcept
//
// 移動三步驟（以管理 char* 的類別為例）：
//   步驟 1 — 偷取：m_data = other.m_data;       （偷走指標）
//   步驟 2 — 歸零：other.m_data = nullptr;       （讓來源安全解構）
//   步驟 3 — 複製基本型別：m_len = other.m_len;  （int/size_t 直接複製）
//
// 為什麼步驟 2 很重要？
//   如果不歸零，other 解構時會 delete 同一塊記憶體 → double free！
//   delete nullptr 是安全的（什麼都不做），所以設成 nullptr 就安全了。
//
// 移動後的 other 處於「有效但未指定」(valid but unspecified) 狀態：
//   ✅ 可以安全解構
//   ✅ 可以重新賦值
//   ❌ 不應該讀取它的值（因為已被偷走）

void chapter6() {
    title("第 6 章：移動建構子");

    subtitle("拷貝 vs 移動的對比");
    {
        SimpleString a("Dragon Sword");
        cout << "  a = \"" << a.c_str() << "\"\n\n";

        // 拷貝建構
        SimpleString::resetCounters();
        cout << "  【拷貝建構】SimpleString b = a;\n";
        SimpleString b = a;
        cout << "    b = \"" << b.c_str() << "\"\n";
        cout << "    a = \"" << a.c_str() << "\" (a 不受影響)\n";
        SimpleString::printCounters();
        cout << "\n";

        // 移動建構
        SimpleString::resetCounters();
        cout << "  【移動建構】SimpleString c = std::move(a);\n";
        SimpleString c = std::move(a);
        cout << "    c = \"" << c.c_str() << "\"\n";
        cout << "    a = \"" << a.c_str() << "\" (a 被掏空了！)\n";
        SimpleString::printCounters();
    }
    cout << "\n";

    subtitle("從臨時物件移動（最常見的場景）");
    {
        SimpleString::resetCounters();
        // 臨時物件是右值 → 自動觸發移動建構
        SimpleString d = SimpleString("Phoenix Staff");
        cout << "  d = \"" << d.c_str() << "\"\n";
        SimpleString::printCounters();
        cout << "  （臨時物件是右值，自動走移動路徑，可能被 RVO 優化掉）\n";
    }
    cout << "\n";

    subtitle("移動後可以重新賦值");
    {
        SimpleString a("First");
        SimpleString b = std::move(a);
        cout << "  移動後 a = \"" << a.c_str() << "\"\n";
        a = SimpleString("Reborn");  // 重新賦值是安全的
        cout << "  重新賦值後 a = \"" << a.c_str() << "\"\n";
    }
    cout << "\n";

    // 移動三步驟圖解
    subtitle("移動三步驟圖解");
    cout << "  移動前：\n";
    cout << "    this:   m_data → [空]        m_len = 0\n";
    cout << "    other:  m_data → [D|r|a|g|o|n|\\0]  m_len = 6\n\n";
    cout << "  步驟 1 偷取：this.m_data = other.m_data\n";
    cout << "    this:   m_data → [D|r|a|g|o|n|\\0]  ← 偷來的！\n";
    cout << "    other:  m_data → [D|r|a|g|o|n|\\0]  ← 還指著同一塊\n\n";
    cout << "  步驟 2 歸零：other.m_data = nullptr\n";
    cout << "    this:   m_data → [D|r|a|g|o|n|\\0]\n";
    cout << "    other:  m_data → nullptr  ← 安全了！\n\n";
    cout << "  步驟 3 複製基本型別：this.m_len = other.m_len; other.m_len = 0\n";
}


// ================================================================
//
//  第 7 章：移動賦值運算子 — 偷取 + 清理
//
// ================================================================
// 移動賦值和移動建構的差異：
//   移動建構：this 是全新的（不需要清理舊資料）
//   移動賦值：this 已經存在（需要先釋放舊資源！）
//
// 簽名：ClassName& operator=(ClassName&& other) noexcept
//
// 步驟：
//   1. 自我賦值檢查：if (this != &other)
//   2. 釋放自己的舊資源：delete[] m_data;
//   3. 偷取：m_data = other.m_data;
//   4. 歸零：other.m_data = nullptr;
//   5. 回傳 *this

// 為了教學，用一個追蹤所有操作的類別
class TrackedBuffer {
    char* m_data;
    size_t m_len;
public:
    TrackedBuffer(const char* str = "")
        : m_len(strlen(str)), m_data(new char[m_len + 1]) {
        strcpy(m_data, str);
        cout << "    [建構] \"" << m_data << "\"\n";
    }

    ~TrackedBuffer() {
        cout << "    [解構] \"" << (m_data ? m_data : "null") << "\"\n";
        delete[] m_data;
    }

    // 拷貝建構
    TrackedBuffer(const TrackedBuffer& o)
        : m_len(o.m_len), m_data(new char[m_len + 1]) {
        strcpy(m_data, o.m_data);
        cout << "    [拷貝建構💰] \"" << m_data << "\"\n";
    }

    // 移動建構
    TrackedBuffer(TrackedBuffer&& o) noexcept
        : m_data(o.m_data), m_len(o.m_len) {
        o.m_data = nullptr; o.m_len = 0;
        cout << "    [移動建構⚡] \"" << m_data << "\"\n";
    }

    // ★ 拷貝賦值
    TrackedBuffer& operator=(const TrackedBuffer& o) {
        cout << "    [拷貝賦值💰]\n";
        if (this != &o) {
            delete[] m_data;                     // 先釋放舊的
            m_len = o.m_len;
            m_data = new char[m_len + 1];        // 配置新的
            strcpy(m_data, o.m_data);            // 複製
        }
        return *this;
    }

    // ★ 移動賦值
    TrackedBuffer& operator=(TrackedBuffer&& o) noexcept {
        cout << "    [移動賦值⚡]\n";
        if (this != &o) {
            delete[] m_data;                     // 先釋放舊的
            m_data = o.m_data;  m_len = o.m_len; // 偷取
            o.m_data = nullptr; o.m_len = 0;     // 歸零
        }
        return *this;
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

void chapter7() {
    title("第 7 章：移動賦值運算子");

    subtitle("拷貝賦值 vs 移動賦值");
    {
        TrackedBuffer a("Alpha");
        TrackedBuffer b("Beta");
        cout << "\n  拷貝賦值 b = a:\n";
        b = a;
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        cout << "\n  移動賦值 b = move(a):\n";
        b = std::move(a);
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        cout << "\n  臨時物件賦值 b = TrackedBuffer(\"Gamma\"):\n";
        b = TrackedBuffer("Gamma");
        cout << "    b=\"" << b.c_str() << "\"\n";
    }
    cout << "\n";

    subtitle("觸發時機");
    cout << "  b = a;              → a 是左值 → 拷貝賦值\n";
    cout << "  b = move(a);        → move(a) 是右值 → 移動賦值\n";
    cout << "  b = func();         → 回傳值是右值 → 移動賦值\n";
    cout << "  b = Type(\"temp\");   → 臨時物件是右值 → 移動賦值\n";
}


// ================================================================
//
//  第 8 章：統一賦值（Copy-and-Swap）— 一招打天下
//
// ================================================================
// 前一章我們分別寫了拷貝賦值和移動賦值，要寫兩個函數。
// 有一個更優雅的方法：只寫一個 operator=，同時處理拷貝和移動。
//
// 技巧：把參數改成傳值（不是 const T& 也不是 T&&）
//
//   T& operator=(T other) {   // ← 傳值！
//       swap(*this, other);
//       return *this;
//   }
//
// 為什麼這樣就行了？
//   傳入左值 → other 由拷貝建構子建構 → swap → 走拷貝路徑
//   傳入右值 → other 由移動建構子建構 → swap → 走移動路徑
//
// 優點：
//   1. 一個函數搞定兩種賦值
//   2. 異常安全：如果 new 在拷貝建構階段失敗，原物件不受影響
//   3. 自動處理自我賦值（a = a 時 other 是 a 的拷貝，交換後仍正確）

void chapter8() {
    title("第 8 章：統一賦值（Copy-and-Swap）");

    // SimpleString 已經用了這個技巧
    subtitle("一個 operator= 同時處理拷貝和移動");
    {
        SimpleString a("Dragon");
        SimpleString b("Knight");

        cout << "  拷貝路徑（傳左值 → 拷貝建構 other → swap）:\n";
        SimpleString::resetCounters();
        b = a;
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
        SimpleString::printCounters();
        cout << "\n";

        cout << "  移動路徑（傳右值 → 移動建構 other → swap）:\n";
        SimpleString::resetCounters();
        b = std::move(a);
        cout << "    a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
        SimpleString::printCounters();
        cout << "\n";

        cout << "  臨時物件路徑:\n";
        SimpleString::resetCounters();
        b = SimpleString("Phoenix");
        cout << "    b=\"" << b.c_str() << "\"\n";
        SimpleString::printCounters();
    }
    cout << "\n";

    subtitle("Copy-and-Swap 程式碼（回顧 SimpleString）");
    cout << "  void swap(SimpleString& other) noexcept {\n";
    cout << "      std::swap(m_data, other.m_data);\n";
    cout << "      std::swap(m_len, other.m_len);\n";
    cout << "  }\n\n";
    cout << "  SimpleString& operator=(SimpleString other) { // ← 傳值！\n";
    cout << "      swap(other);      // 交換內容\n";
    cout << "      return *this;     // other 解構時釋放舊資料\n";
    cout << "  }\n";
}


// ================================================================
//
//  第 9 章：Rule of Three → Five → Zero
//
// ================================================================

void chapter9() {
    title("第 9 章：Rule of Three → Five → Zero");

    subtitle("Rule of Three（C++98）");
    cout << "  如果類別需要自訂以下任何一個，三個都要寫：\n";
    cout << "    1. ~ClassName()                 解構子\n";
    cout << "    2. ClassName(const ClassName&)   拷貝建構\n";
    cout << "    3. operator=(const ClassName&)   拷貝賦值\n\n";
    cout << "  為什麼？需要解構子 → 表示管理了資源\n";
    cout << "  → 預設的逐成員複製（淺拷貝）不安全 → 要自訂拷貝\n\n";

    subtitle("Rule of Five（C++11）");
    cout << "  = Rule of Three + 移動建構 + 移動賦值：\n";
    cout << "    1. ~ClassName()                       解構子\n";
    cout << "    2. ClassName(const ClassName&)         拷貝建構\n";
    cout << "    3. operator=(const ClassName&)         拷貝賦值\n";
    cout << "    4. ClassName(ClassName&&) noexcept     移動建構  ← 新增\n";
    cout << "    5. operator=(ClassName&&) noexcept     移動賦值  ← 新增\n\n";
    cout << "  用統一賦值可以簡化成 4 個：\n";
    cout << "    解構 + 拷貝建構 + 移動建構 + operator=(T other)\n\n";

    subtitle("Rule of Zero（最佳實踐！）");
    cout << "  只用 RAII 成員（string, vector, unique_ptr...）\n";
    cout << "  → 什麼特殊函式都不用寫！編譯器全部自動生成正確版本\n\n";

    // Rule of Zero 範例
    struct SafeClass {
        string name;          // 自動管理
        vector<int> data;     // 自動管理
        // 不需要寫任何解構/拷貝/移動！
    };

    SafeClass s1;
    s1.name = "Hero";
    s1.data = {1, 2, 3};
    SafeClass s2 = s1;              // 安全拷貝
    SafeClass s3 = std::move(s1);   // 安全移動
    cout << "  SafeClass：只用 string + vector 成員\n";
    cout << "    s2.name = " << s2.name << "\n";
    cout << "    s3.name = " << s3.name << "\n";
    cout << "    s1.name = \"" << s1.name << "\" (已被移動)\n\n";

    // 自動生成規則
    subtitle("重要：自訂任何一個 → 移動不自動生成");
    cout << "  自訂了解構子 → 編譯器不自動生成移動建構/移動賦值\n";
    cout << "  自訂了拷貝建構 → 同上\n";
    cout << "  自訂了拷貝賦值 → 同上\n";
    cout << "  → 這就是為什麼 Rule of Five 說「要嘛全寫，要嘛全不寫」\n";

    // type_traits 驗證
    cout << "\n  用 type_traits 檢查：\n";
    cout << boolalpha;

    struct OnlyDestructor {
        ~OnlyDestructor() {}  // 自訂解構 → 移動不自動生成
    };
    cout << "    OnlyDestructor 可移動建構？ "
         << is_move_constructible_v<OnlyDestructor> << "\n";
    cout << "    OnlyDestructor nothrow？    "
         << is_nothrow_move_constructible_v<OnlyDestructor> << "\n";
    cout << "    （看似可以移動，但實際上退回拷貝，不是真正的移動）\n";
}


// ================================================================
//
//  第 10 章：std::move 的真相 — 它什麼都沒移動
//
// ================================================================

void chapter10() {
    title("第 10 章：std::move 的真相");

    subtitle("std::move 只是一個 cast，不做任何移動");
    cout << "  std::move(x)  ≡  static_cast<T&&>(x)\n\n";
    cout << "  它只是把左值「標記為」右值引用\n";
    cout << "  真正的移動發生在接收端的移動建構子或移動賦值中\n\n";

    subtitle("std::move 的實作（超級簡單）");
    cout << "  template<typename T>\n";
    cout << "  remove_reference_t<T>&& move(T&& arg) noexcept {\n";
    cout << "      return static_cast<remove_reference_t<T>&&>(arg);\n";
    cout << "  }\n\n";

    subtitle("三種 move 不會觸發移動的情況");

    // 情況 1：const 物件
    cout << "  1. const 物件 → move 後型別是 const T&& → 匹配 const T&（拷貝！）\n";
    {
        const string c = "World";
        string d = std::move(c);  // 拷貝！不是移動！
        cout << "     const string c = \"World\";\n";
        cout << "     string d = move(c);  // c 仍然是 \"" << c << "\" → 拷貝！\n\n";
    }

    // 情況 2：基本型別
    cout << "  2. 基本型別（int, double...）→ 沒有資源可搬，移動 = 複製\n";
    {
        int x = 42;
        int y = std::move(x);
        cout << "     int x = 42; int y = move(x);\n";
        cout << "     x 仍然 = " << x << " → 無差異\n\n";
    }

    // 情況 3：SSO 短字串
    cout << "  3. SSO 短字串 → string 的 Small String Optimization\n";
    cout << "     短字串存在物件內部（棧上），不在堆積上 → 移動 ≈ 複製\n\n";

    subtitle("unique_ptr — 只能 move 不能 copy");
    {
        auto p = make_unique<int>(42);
        cout << "  auto p = make_unique<int>(42);\n";
        cout << "  *p = " << *p << "\n";
        // auto q = p;  // ❌ unique_ptr 禁止複製
        auto q = std::move(p);  // ✅ 轉移所有權
        cout << "  auto q = move(p);  // p → nullptr, q 接管\n";
        cout << "  p = " << (p ? "非空" : "nullptr") << "\n";
        cout << "  *q = " << *q << "\n";
    }
}


// ================================================================
//
//  第 11 章：noexcept — 一個關鍵字值百倍效能
//
// ================================================================

void chapter11() {
    title("第 11 章：noexcept 的重要性");

    cout << "  移動建構子一定要加 noexcept！\n\n";
    cout << "  為什麼？因為 vector 擴容時需要搬移元素：\n";
    cout << "    有 noexcept → vector 使用移動（快！）\n";
    cout << "    沒 noexcept → vector 退回使用拷貝（慢！）\n\n";
    cout << "  原因：vector 需要「強異常安全保證」\n";
    cout << "  如果移動到一半拋出例外 → 原始資料已被破壞 → 無法恢復\n";
    cout << "  所以 vector 只在確定不會拋例外時才敢用移動\n\n";

    // 效能測試
    subtitle("效能測試：有 vs 沒有 noexcept");
    struct WithNoexcept {
        vector<int> data;
        WithNoexcept() : data(1000, 42) {}
        WithNoexcept(const WithNoexcept& o) : data(o.data) {}
        WithNoexcept(WithNoexcept&& o) noexcept : data(std::move(o.data)) {}
    };
    struct WithoutNoexcept {
        vector<int> data;
        WithoutNoexcept() : data(1000, 42) {}
        WithoutNoexcept(const WithoutNoexcept& o) : data(o.data) {}
        WithoutNoexcept(WithoutNoexcept&& o) : data(std::move(o.data)) {} // 沒 noexcept！
    };

    {
        auto t1 = chrono::high_resolution_clock::now();
        vector<WithNoexcept> v1;
        for (int i = 0; i < 50000; ++i) v1.emplace_back();
        auto t2 = chrono::high_resolution_clock::now();
        auto ms1 = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();

        auto t3 = chrono::high_resolution_clock::now();
        vector<WithoutNoexcept> v2;
        for (int i = 0; i < 50000; ++i) v2.emplace_back();
        auto t4 = chrono::high_resolution_clock::now();
        auto ms2 = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();

        cout << "  有 noexcept（擴容用移動）: " << ms1 << " ms\n";
        cout << "  沒 noexcept（擴容用拷貝）: " << ms2 << " ms\n";
        if (ms1 > 0)
            cout << "  差距約: " << (double)ms2 / ms1 << " 倍\n";
    }
}


// ================================================================
//
//  第 12 章：std::forward 與完美轉發
//
// ================================================================

void fwd_target(const string& s) { cout << "    → const T& 左值版本\n"; }
void fwd_target(string&& s)      { cout << "    → T&& 右值版本\n"; }

// 錯誤：直接傳 arg → 永遠左值
template<typename T>
void wrapper_bad(T&& arg) {
    fwd_target(arg);  // arg 有名字 → 左值 → 永遠呼叫左值版本
}

// 正確：用 std::forward
template<typename T>
void wrapper_good(T&& arg) {
    fwd_target(std::forward<T>(arg));
}

// 泛型工廠
class Gadget {
    string name_;
public:
    Gadget(const string& n) : name_(n) { cout << "    Gadget(const T&) 拷貝\n"; }
    Gadget(string&& n) : name_(std::move(n)) { cout << "    Gadget(T&&) 移動\n"; }
};

template<typename T, typename... Args>
unique_ptr<T> my_make(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

void chapter12() {
    title("第 12 章：std::forward 與完美轉發");

    subtitle("問題：T&& 參數在函數內是左值");
    string s = "Hello";
    cout << "  不用 forward（錯誤）:\n";
    cout << "    傳左值:  "; wrapper_bad(s);
    cout << "    傳右值:  "; wrapper_bad(string("tmp"));
    cout << "    兩個都呼叫左值版本！❌\n\n";

    cout << "  用 forward（正確）:\n";
    cout << "    傳左值:  "; wrapper_good(s);
    cout << "    傳右值:  "; wrapper_good(string("tmp"));
    cout << "    正確區分！✅\n\n";

    subtitle("三種轉發方式對比");
    cout << "  ┌─────────────────────────┬───────────┬───────────┐\n";
    cout << "  │       轉發方式           │ 傳左值時  │ 傳右值時  │\n";
    cout << "  ├─────────────────────────┼───────────┼───────────┤\n";
    cout << "  │ target(arg)             │   左值    │   左值 ❌ │\n";
    cout << "  │ target(move(arg))       │   右值 ❌ │   右值    │\n";
    cout << "  │ target(forward<T>(arg)) │   左值 ✅ │   右值 ✅ │\n";
    cout << "  └─────────────────────────┴───────────┴───────────┘\n\n";

    subtitle("forward 的原理：引用折疊");
    cout << "  傳入左值 s → T 推導為 string& → T&& = string& (& + && = &)\n";
    cout << "    → forward<string&>(arg) = static_cast<string&>(arg) → 左值\n\n";
    cout << "  傳入右值 string(\"x\") → T = string → T&& = string&&\n";
    cout << "    → forward<string>(arg) = static_cast<string&&>(arg) → 右值\n\n";

    subtitle("實戰：泛型工廠函式");
    string name = "Sword";
    cout << "  傳左值:\n";
    auto g1 = my_make<Gadget>(name);
    cout << "  傳右值:\n";
    auto g2 = my_make<Gadget>(string("Shield"));
    cout << "\n";

    subtitle("emplace_back — 標準庫中最常見的完美轉發");
    cout << "  push_back(obj)          → 拷貝到容器\n";
    cout << "  push_back(move(obj))    → 移動到容器\n";
    cout << "  emplace_back(args...)   → 在容器內直接建構（零拷貝零移動！）\n";
    {
        vector<string> v;
        v.reserve(3);
        string x = "Alpha";
        cout << "\n";
        // emplace_back 用完美轉發把引數傳給 string 的建構子
        v.emplace_back("directly constructed");  // 直接在 vector 內部建構
        cout << "  emplace_back(\"directly constructed\") → 零拷貝零移動\n";
    }
}


// ================================================================
//
//  第 13 章：實戰模式與最佳實踐
//
// ================================================================

void chapter13() {
    title("第 13 章：實戰模式與最佳實踐");

    // 模式 1：建構子 pass-by-value + move
    subtitle("模式 1：建構子 pass-by-value + move（推薦）");
    cout << "  class Hero {\n";
    cout << "      string name_;\n";
    cout << "  public:\n";
    cout << "      Hero(string name) : name_(move(name)) {}\n";
    cout << "  };\n\n";
    cout << "  傳左值：拷貝到參數 → move 到成員（1 拷貝 + 1 移動）\n";
    cout << "  傳右值：移動到參數 → move 到成員（2 移動，零拷貝）\n\n";

    // 實際展示
    {
        struct Hero {
            string name_;
            Hero(string name) : name_(std::move(name)) {}
        };

        SimpleString::resetCounters();
        string n = "Arthur";
        Hero h1(n);                  // 左值：1 拷貝 + 1 移動
        Hero h2(string("Merlin"));   // 右值：1 移動（或 RVO）+ 1 移動
        cout << "  h1.name_ = " << h1.name_ << "\n";
        cout << "  h2.name_ = " << h2.name_ << "\n\n";
    }

    // 模式 2：swap 實現（3 次移動，0 次拷貝）
    subtitle("模式 2：用 std::swap 交換兩個物件");
    {
        string a = "Left", b = "Right";
        cout << "  swap 前：a=\"" << a << "\" b=\"" << b << "\"\n";
        // swap 內部：
        //   T tmp = move(a);   // 移動建構
        //   a = move(b);       // 移動賦值
        //   b = move(tmp);     // 移動賦值
        // 3 次移動，0 次拷貝！
        std::swap(a, b);
        cout << "  swap 後：a=\"" << a << "\" b=\"" << b << "\"\n";
        cout << "  內部是 3 次移動，0 次拷貝\n\n";
    }

    // 模式 3：容器操作
    subtitle("模式 3：容器操作");
    {
        vector<string> vec;
        vec.reserve(3);

        string s = "reusable";
        vec.push_back(s);              // 拷貝（s 還要用）
        vec.push_back(std::move(s));   // 移動（s 不再需要）
        vec.emplace_back("in-place");  // 原地建構（最快）
        cout << "  push_back(s)       → 拷貝\n";
        cout << "  push_back(move(s)) → 移動\n";
        cout << "  emplace_back(...)  → 原地建構（零拷貝零移動）\n\n";
    }

    // 模式 4：從容器提取元素
    subtitle("模式 4：從容器提取元素");
    {
        vector<string> vec = {"Alpha", "Beta", "Gamma"};
        string last = std::move(vec.back());  // 移動出來
        vec.pop_back();                       // 移除（解構空殻）
        cout << "  string last = move(vec.back()); vec.pop_back();\n";
        cout << "  last = \"" << last << "\"\n\n";
    }

    // 模式 5：unique_ptr 所有權轉移
    subtitle("模式 5：unique_ptr 所有權轉移");
    {
        auto p1 = make_unique<int>(42);
        auto p2 = std::move(p1);  // p1 → nullptr，p2 接管
        cout << "  unique_ptr 只能 move 不能 copy\n";
        cout << "  p1 = " << (p1 ? "非空" : "nullptr") << "\n";
        cout << "  *p2 = " << *p2 << "\n";
    }
}


// ================================================================
//
//  第 14 章：效能實測
//
// ================================================================

void chapter14() {
    title("第 14 章：效能實測");

    // 測試 1：string
    subtitle("測試 1：string 拷貝 vs 移動 (10000 字元)");
    {
        const int N = 2000000;
        string source(10000, 'x');

        auto t1 = chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            string copy = source; (void)copy;
        }
        auto t2 = chrono::high_resolution_clock::now();

        auto t3 = chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            string temp = source;
            string moved = std::move(temp); (void)moved;
        }
        auto t4 = chrono::high_resolution_clock::now();

        cout << "  拷貝: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << " ms\n";
        cout << "  移動: " << chrono::duration_cast<chrono::milliseconds>(t4 - t3).count() << " ms\n\n";
    }

    // 測試 2：vector 不同大小
    subtitle("測試 2：vector<int> 拷貝+移動 vs 純拷貝");
    {
        auto bench = [](size_t size) {
            const int N = 5000;
            vector<int> source(size, 42);
            long long copy_us, move_us;
            {
                auto t = chrono::high_resolution_clock::now();
                for (int i = 0; i < N; ++i) { vector<int> c = source; (void)c; }
                copy_us = chrono::duration_cast<chrono::microseconds>(
                    chrono::high_resolution_clock::now() - t).count();
            }
            {
                auto t = chrono::high_resolution_clock::now();
                for (int i = 0; i < N; ++i) {
                    vector<int> tmp = source;
                    vector<int> m = std::move(tmp); (void)m;
                }
                move_us = chrono::duration_cast<chrono::microseconds>(
                    chrono::high_resolution_clock::now() - t).count();
            }
            cout << "  size=" << size
                 << "\t拷貝: " << copy_us << " us"
                 << "\t移動: " << move_us << " us";
            if (move_us > 0)
                cout << "\t加速: " << (double)copy_us / move_us << "x";
            cout << "\n";
        };
        bench(100);
        bench(1000);
        bench(100000);
    }
    cout << "\n";

    // 測試 3：push_back vs emplace_back
    subtitle("測試 3：push_back 拷貝 vs move vs emplace_back");
    {
        const int N = 1000000;
        {
            auto t = chrono::high_resolution_clock::now();
            vector<string> v; v.reserve(N);
            for (int i = 0; i < N; ++i) {
                string s = "Hello World!!!!!";
                v.push_back(s);
            }
            cout << "  push_back(copy): "
                 << chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - t).count() << " ms\n";
        }
        {
            auto t = chrono::high_resolution_clock::now();
            vector<string> v; v.reserve(N);
            for (int i = 0; i < N; ++i) {
                string s = "Hello World!!!!!";
                v.push_back(std::move(s));
            }
            cout << "  push_back(move): "
                 << chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - t).count() << " ms\n";
        }
        {
            auto t = chrono::high_resolution_clock::now();
            vector<string> v; v.reserve(N);
            for (int i = 0; i < N; ++i) {
                v.emplace_back("Hello World!!!!!");
            }
            cout << "  emplace_back:    "
                 << chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - t).count() << " ms\n";
        }
    }
}


// ================================================================
//
//  第 15 章：常見錯誤與陷阱大全
//
// ================================================================

void chapter15() {
    title("第 15 章：常見錯誤與陷阱大全");

    // 陷阱 1
    subtitle("陷阱 1：move 後繼續使用物件的值");
    {
        string s = "Important";
        string t = std::move(s);
        cout << "  ❌ string s = \"Important\"; string t = move(s);\n";
        cout << "     s = \"" << s << "\"  ← 已被掏空，不應該讀取\n";
        cout << "  ✅ 可以重新賦值：s = \"New\";\n";
        s = "New";
        cout << "     s = \"" << s << "\"\n\n";
    }

    // 陷阱 2
    subtitle("陷阱 2：move const 物件 → 退化為拷貝");
    {
        const string c = "Immovable";
        string d = std::move(c);  // const string&& → 匹配 const string&（拷貝！）
        cout << "  const string c = \"Immovable\";\n";
        cout << "  string d = move(c);  // c 仍然是 \"" << c << "\"\n";
        cout << "  → const T&& 只能匹配 const T&（拷貝建構），不是移動！\n\n";
    }

    // 陷阱 3
    subtitle("陷阱 3：return std::move(local) 阻止 RVO");
    cout << "  ❌ string func() { string s = \"x\"; return move(s); }\n";
    cout << "     move 阻止了 NRVO 優化，反而更慢！\n";
    cout << "  ✅ string func() { string s = \"x\"; return s; }\n";
    cout << "     編譯器會自動做 NRVO（直接在呼叫者空間建構）\n\n";

    // 陷阱 4
    subtitle("陷阱 4：忘記 noexcept → vector 不用移動");
    cout << "  ❌ MyClass(MyClass&& o) { ... }           // 沒有 noexcept\n";
    cout << "  ✅ MyClass(MyClass&& o) noexcept { ... }   // 有 noexcept\n";
    cout << "  → 沒有 noexcept，vector 擴容時退回使用拷貝（慢很多）\n\n";

    // 陷阱 5
    subtitle("陷阱 5：右值引用變數本身是左值");
    cout << "  string&& r = string(\"temp\");\n";
    cout << "  func(r);        → 呼叫左值版本！（r 有名字 → 左值）\n";
    cout << "  func(move(r));  → 呼叫右值版本   （move 轉回右值）\n\n";

    // 陷阱 6
    subtitle("陷阱 6：move 基本型別沒有效果");
    {
        int x = 42;
        int y = std::move(x);
        cout << "  int x = 42; int y = move(x);\n";
        cout << "  x = " << x << " → int 沒有資源可搬，move 無效\n\n";
    }

    // 陷阱 7
    subtitle("陷阱 7：只寫解構不寫拷貝 → 淺拷貝 → 崩潰");
    cout << "  class Bad {\n";
    cout << "      int* data;\n";
    cout << "  public:\n";
    cout << "      Bad(int v) : data(new int(v)) {}\n";
    cout << "      ~Bad() { delete data; }   // 只寫了這個\n";
    cout << "  };\n";
    cout << "  Bad a(42); Bad b = a;  // 淺拷貝 → double free 💥\n";

    cout << "\n";
    subtitle("總結：安全使用移動語意的檢查清單");
    cout << "  ✅ 移動建構/賦值加 noexcept\n";
    cout << "  ✅ 移動後不再使用物件的值\n";
    cout << "  ✅ 不要 move const 物件\n";
    cout << "  ✅ 不要 return move(local)\n";
    cout << "  ✅ 不要 move 基本型別\n";
    cout << "  ✅ 用 emplace_back > push_back(move) > push_back(copy)\n";
    cout << "  ✅ 優先 Rule of Zero（只用 RAII 成員）\n";
    cout << "  ✅ 如果要 Rule of Five，用 Copy-and-Swap 簡化\n";
}


// ================================================================
//
//  main：按順序執行所有章節
//
// ================================================================
int main() {
    cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    cout << "║  右值參考與移動語意 — 完整教學（15 章）                         ║\n";
    cout << "╚══════════════════════════════════════════════════════════════════╝\n";

    chapter1();
    chapter2();
    chapter3();
    chapter4();
    chapter5();
    chapter6();
    chapter7();
    chapter8();
    chapter9();
    chapter10();
    chapter11();
    chapter12();
    chapter13();
    chapter14();
    chapter15();

    cout << "\n";
    cout << "================================================================\n";
    cout << "  恭喜！你已經完整學完右值參考與移動語意的所有核心知識。\n";
    cout << "  建議順序複習：\n";
    cout << "    1. 左值/右值 → 2. 三種引用 → 3. 有名字=左值\n";
    cout << "    4. 移動建構/賦值 → 5. Copy-and-Swap → 6. Rule of 3/5/0\n";
    cout << "    7. std::move → 8. noexcept → 9. std::forward\n";
    cout << "    10. 實戰模式 → 11. 陷阱大全\n";
    cout << "================================================================\n";

    return 0;
}
