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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++11 總覽：值類別與移動語義（本目錄無專門檔案，集中於此）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++11 的五種值類別是什麼？
//     答：三個基本類別 + 兩個複合類別。
//         lvalue ：有身分、不可移動（變數名、*p、回傳左值參考的函式呼叫）
//         prvalue：無身分、可移動（字面值〔字串字面值除外，它是 lvalue〕、
//                  回傳非參考的函式呼叫、a+b）
//         xvalue ：有身分「且」可移動（std::move(x)、回傳右值參考的函式呼叫）
//         glvalue = lvalue ∪ xvalue（有身分）；rvalue = prvalue ∪ xvalue（可移動）
//     追問：std::move(x) 是 prvalue 嗎？（❌ 是 xvalue，這是最常見的錯答。可用
//           decltype 驗證：xvalue 運算式的 decltype 得 T&&，prvalue 得 T。本機
//           g++ -std=c++11 實測 decltype(std::move(x)) 為 int&&、decltype(f())
//           為 int，兩個 static_assert 都通過）
//
// 🔥 Q2. std::move 與 std::forward 差在哪？
//     答：std::move 是「無條件」轉型成右值；std::forward<T> 是「條件式」轉型，
//         依 T 保留原本的值類別。move 用在明確的右值參考參數上（語意：我不要它
//         了，拿去）；forward 用在 forwarding reference T&& 上（語意：原樣傳下
//         去）。forward 必須顯式寫出 <T>，因為它的判斷依據就是 T，無法從實參推導。
//     追問：std::move 真的搬了東西嗎？（沒有。它只是 static_cast 成 T&&，真正搬
//           資源的是隨後被重載決議選中的移動建構子／移動賦值；若型別根本沒有移動
//           操作，會靜默退回拷貝，一句警告都不會有）
//
// 🔥 Q3. 為什麼「具名的右值參考本身是左值」？
//     答：值類別是「運算式」的性質，不是「型別」的性質。void f(Widget&& w) 中，
//         w 的型別是右值參考，但運算式 w 有名字、可取址 → 它是左值。口訣就是
//         「有名字就是左值」。所以在 f 內部要繼續轉傳時必須寫 g(std::move(w))，
//         否則會呼叫到 g 的拷貝版本。
//         （本機實測：f(int&& w) 中 &w 合法，且 decltype((w)) 為 int&）
//
// ⚠️ 陷阱. 對 const 物件呼叫 std::move 會發生什麼事？
//     答：靜默拷貝，沒有任何警告。std::move(constObj) 的結果型別是 const T&&；
//         移動建構子的參數是 T&&，綁不上 const T&&，但拷貝建構子的 const T& 可以
//         綁右值 → 重載決議選中拷貝：
//           const std::string s = "...";  std::string t = std::move(s);  // 實際是拷貝
//         （本機以會印字的 copy/move ctor 實測：const 版印 COPY、非 const 版印 MOVE）
//     為什麼會錯：把 std::move 當成「動詞」，以為寫了就一定會搬。它只是轉型，搬
//         不搬完全由重載決議決定，而 const 讓它無聲地退回拷貝。連帶後果：把成員
//         宣告成 const std::string，會讓整個類別的移動建構退化成拷貝。
//
// Q5. RVO、NRVO 與移動語義的優先順序？return std::move(x); 是好習慣嗎？
//     答：優先序是 copy elision > move > copy。
//         RVO（回傳 prvalue 臨時物件）：⚠️ C++17 起是「強制」的 guaranteed copy
//         elision，連拷貝／移動建構子都不需要存在——本機實測把 copy 與 move 都
//         = delete，-std=c++17 仍能編譯，-std=c++11 則報 use of deleted function。
//         NRVO（回傳具名的區域變數）：至今仍是「可選」優化，標準只允許不強制，
//         且即使實際不呼叫，仍要求拷貝／移動建構子可存取（本機 -std=c++17 與
//         -std=c++20 對上述 deleted 版本皆編譯失敗）。
//     追問：那 return std::move(x); 呢？（是反最佳化。它讓回傳運算式變成 xvalue、
//           不再是「具名物件」，直接取消 NRVO 的資格，強迫多做一次移動。本機實測：
//           return x; 一個建構訊息都不印〔零次〕，return std::move(x); 印出 MOVE。
//           正確寫法就是 return x;——C++11 起回傳區域變數本來就會先當右值做重載
//           決議，該選到移動的自然會選到）
// ═══════════════════════════════════════════════════════════════════════════

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <cstdint>
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

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 (節錄) ===
//
// [demo_auto]
//   x=7, a=99
//   v=11 12 13
//
// [demo_decltype]
//   decltype tests passed
//
// [demo_nullptr]
//   takes_ptr
//
// [demo_range_for]
//  a! b! c!
//
// [demo_brace_init]
//   v1(size=3): 9 9 9
//   v2(size=2): 3 9
//
// [demo_default_delete]
//   NoCopy constructed; copy is deleted
// …（後略，完整輸出共 44 行）
