/*
================================================================================
【C++_Cpp11/summary.cpp】

本目錄主題：C++11 核心語法（現代 C++ 的起點）

從檔名可見本目錄涵蓋：
  - auto / decltype / trailing return type
  - nullptr
  - range-based for
  - brace-init（統一初始化）與 std::initializer_list
  - =default / =delete
  - override / final
  - constexpr（C++11 版本）
  - static_assert
  - type alias（using）
  - enum class（強型別列舉）
  - raw string literal
  - user-defined literals（自訂字面值）

本 summary 的原則：
  - 不額外加入 題庫 類範例
  - 用「短、可執行」的片段，把 C++11 最常用/最容易踩坑的點講清楚

編譯：
  g++ -std=c++11 -Wall -Wextra summary.cpp -o summary && ./summary
（用 -std=c++17 也可，C++17 只會讓你多一些語法糖。）
================================================================================
*/

/*
補充筆記：C++_Cpp11/C++_Cpp11 summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Cpp11/C++_Cpp11 summary 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Cpp11/C++_Cpp11 summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// -----------------------------------------------------------------------------
// 【重點 1】auto：讓編譯器幫你寫出「一長串」型別（規則類似 template 推導）
// -----------------------------------------------------------------------------
static void demo_auto() {
    std::cout << "\n[demo_auto]\n";

    std::vector<int> v{1, 2, 3};

    // (1) 迭代器：最常見的 auto 用法
    for (auto it = v.begin(); it != v.end(); ++it) {
        *it += 10;
    }

    // (2) 值 / 參考 / const 推導差異（這是 auto 最常踩坑的地方）
    int x = 7;
    const int& cr = x;

    auto a = cr;   // a：int（去掉 top-level const & reference）
    auto& b = cr;  // b：const int&（保留 const + reference）

    a = 99;        // 不影響 x
    (void)b;
    std::cout << "  x=" << x << ", a=" << a << "\n";

    // (3) range-based for：auto 也常搭配 & 避免拷貝
    std::cout << "  v=";
    for (const auto& e : v) std::cout << e << ' ';
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】decltype：查「表達式」的型別（常用於泛型程式、推導回傳型別）
// -----------------------------------------------------------------------------
static void demo_decltype() {
    std::cout << "\n[demo_decltype]\n";

    int x = 1;
    const int cx = 2;
    const int& rx = x;

    // 重要差異：
    //  - decltype(x)   -> int
    //  - decltype((x)) -> int&   （注意多一層括號後，它變成「lvalue 表達式」）
    static_assert(std::is_same<decltype(x), int>::value, "decltype(x) 應為 int");
    static_assert(std::is_same<decltype((x)), int&>::value, "decltype((x)) 應為 int&");
    static_assert(std::is_same<decltype(cx), const int>::value, "decltype(cx) 應為 const int");
    static_assert(std::is_same<decltype(rx), const int&>::value, "decltype(rx) 應為 const int&");

    std::cout << "  decltype tests passed\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】nullptr：指標的「正確空值」，解決 0/NULL 的多載歧義
// -----------------------------------------------------------------------------
static void takes_int(int) { std::cout << "  takes_int\n"; }
static void takes_ptr(const int*) { std::cout << "  takes_ptr\n"; }

static void demo_nullptr() {
    std::cout << "\n[demo_nullptr]\n";
    takes_ptr(nullptr);  // 明確叫指標版本
    // takes_ptr(NULL);  // 可能是 0，會導致多載歧義（不同平台/實作不一致）
}

// -----------------------------------------------------------------------------
// 【重點 4】range-based for：讀寫容器的標準寫法
// -----------------------------------------------------------------------------
static void demo_range_for() {
    std::cout << "\n[demo_range_for]\n";
    std::vector<std::string> names{"a", "b", "c"};
    for (auto& s : names) s += "!";
    for (const auto& s : names) std::cout << ' ' << s;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】統一初始化（brace-init）+ initializer_list：能防 narrowing，但也有優先序陷阱
// -----------------------------------------------------------------------------
static void demo_brace_init() {
    std::cout << "\n[demo_brace_init]\n";

    // (1) 防 narrowing：下面這行會「編譯錯」而不是默默截斷（這是 brace-init 的價值）
    // int a{3.14}; // ❌

    // (2) vector<int>(3, 9) 與 vector<int>{3, 9} 的差異
    std::vector<int> v1(3, 9);  // 3 個 9
    std::vector<int> v2{3, 9};  // 兩個元素：3 與 9

    std::cout << "  v1(size=" << v1.size() << "):";
    for (int x : v1) std::cout << ' ' << x;
    std::cout << "\n";

    std::cout << "  v2(size=" << v2.size() << "):";
    for (int x : v2) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 6】=default / =delete：更精準地控制「編譯器自動產生的成員函式」
// -----------------------------------------------------------------------------
struct NoCopy {
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;            // 禁止拷貝
    NoCopy& operator=(const NoCopy&) = delete; // 禁止拷貝賦值
};

static void demo_default_delete() {
    std::cout << "\n[demo_default_delete]\n";
    NoCopy a;
    (void)a;
    std::cout << "  NoCopy constructed; copy is deleted\n";
}

// -----------------------------------------------------------------------------
// 【重點 7】override / final：把「虛擬覆寫」變成編譯期可檢查
// -----------------------------------------------------------------------------
struct Base {
    virtual ~Base() = default;
    virtual void f() { std::cout << "  Base::f\n"; }
};
struct Derived final : Base {
    void f() override { std::cout << "  Derived::f\n"; }
};

static void demo_override_final() {
    std::cout << "\n[demo_override_final]\n";
    Base* p = new Derived();
    p->f();
    delete p;
}

// -----------------------------------------------------------------------------
// 【重點 8】constexpr（C++11）：讓某些運算在編譯期完成
// -----------------------------------------------------------------------------
// C++11 的 constexpr 函式限制較多（只能有單一 return 表達式等）。
constexpr int square(int x) { return x * x; }

static void demo_constexpr_static_assert() {
    std::cout << "\n[demo_constexpr_static_assert]\n";
    static_assert(square(5) == 25, "constexpr 計算失敗");
    std::cout << "  square(6)=" << square(6) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 9】type alias（using）：可讀性比 typedef 好，模板也更友善
// -----------------------------------------------------------------------------
template <class T>
using Vec = std::vector<T>;

static void demo_type_alias() {
    std::cout << "\n[demo_type_alias]\n";
    Vec<int> v{1, 2, 3};
    std::cout << "  Vec<int>.size=" << v.size() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 10】enum class：強型別列舉（不會自動轉 int、不污染命名空間）
// -----------------------------------------------------------------------------
enum class Color : std::uint8_t { Red = 1, Green = 2, Blue = 3 };

static void demo_enum_class() {
    std::cout << "\n[demo_enum_class]\n";
    Color c = Color::Red;
    std::cout << "  Color::Red underlying=" << static_cast<int>(c) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 11】raw string literal：寫 regex / JSON / Windows path 很好用
// -----------------------------------------------------------------------------
static void demo_raw_string() {
    std::cout << "\n[demo_raw_string]\n";
    std::string path = R"(D:\git\ClaudeClass\Claude Code C++by Examples)";
    std::cout << "  path=" << path << "\n";

    // 自訂分隔符 R"delim( ... )delim"：內容若含 )" 也能安全寫入
    std::string tricky = R"tag(He said: ") and then left.)tag";
    std::cout << "  tricky=" << tricky << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 12】user-defined literals：把「字面值」轉成你要的型別/單位
// -----------------------------------------------------------------------------
// 這裡示範一個簡單「秒」字面值：3_s 代表 3 秒（用 long long 儲存）
constexpr long long operator"" _s(unsigned long long x) {
    return static_cast<long long>(x);
}

static void demo_user_defined_literals() {
    std::cout << "\n[demo_user_defined_literals]\n";
    auto t = 3_s;
    std::cout << "  3_s = " << t << " seconds\n";
}

// -----------------------------------------------------------------------------
// 【重點 13】trailing return type：當回傳型別依賴參數型別/表達式時很實用
// -----------------------------------------------------------------------------
// C++11 沒有 decltype(auto)，常用寫法是：
//   auto f(T a, U b) -> decltype(a+b)
template <class T, class U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

static void demo_trailing_return() {
    std::cout << "\n[demo_trailing_return]\n";
    std::cout << "  add(1, 2.5)=" << add(1, 2.5) << "\n";
}

int main() {
    demo_auto();
    demo_decltype();
    demo_nullptr();
    demo_range_for();
    demo_brace_init();
    demo_default_delete();
    demo_override_final();
    demo_constexpr_static_assert();
    demo_type_alias();
    demo_enum_class();
    demo_raw_string();
    demo_user_defined_literals();
    demo_trailing_return();

    std::cout << "\n[done]\n";
    return 0;
}

