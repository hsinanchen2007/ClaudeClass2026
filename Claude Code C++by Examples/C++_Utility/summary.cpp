/*
================================================================================
【C++_Utility/summary.cpp】

本目錄主題：<utility> 與「標準庫雜項工具」的核心用法（課件版）

這些工具的共同點：
  - 它們不是「某個容器」或「某個演算法」，而是你在幾乎所有 C++ 專案都會用到的
    小型基礎積木：pair/tuple、optional/variant/any、move/forward、swap/exchange…
  - 用對它們，你的程式會更安全、更好讀、更不容易寫出隱性 bug

本檔目標（你可直接拿來當課件複習）：
  - 每一個工具都用「白話」先解釋它解決什麼問題
  - 再用最小、明確的示範逐一展示常用的 member functions / 相關 free functions
  - 最後附錄整理 cppreference 風格速查表（C++20+ 或冷門內容只提示）

編譯（建議）：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Utility/C++_Utility summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Utility/C++_Utility summary 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Utility/C++_Utility summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <bitset>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// C++17 才有：std::optional / std::variant / std::any
//（有些專案的語言標準設定若不是 C++17，clangd 會把它們視為不存在。）
#if __cplusplus >= 201703L
#include <any>
#include <optional>
#include <variant>
#endif

static void header(const char* title) { std::cout << "\n[" << title << "]\n"; }

// -----------------------------------------------------------------------------
// 【重點 1】std::pair：兩個值的打包（map 的元素也是 pair<const K, V>）
// -----------------------------------------------------------------------------
// 你可以把 pair 當成「最小單位的 tuple」：永遠兩格 (first/second)。
// 適合：
//   - 回傳 (成功/失敗, 結果)
//   - 回傳 (索引, 值)
//   - 回傳 (key, value) 這種天然就是二元的資料
//
// 不適合：
//   - 欄位需要語意化命名（那就寫 struct，程式更好讀）
static void demo_pair() {
    header("demo_pair");

    // (1) 基本建構
    std::pair<int, std::string> p1{1, "apple"};
    std::cout << "  p1=(" << p1.first << ", " << p1.second << ")\n";

    // (2) 明確寫出模板參數：可用於 C++11/14/17（也避免工具鏈誤判）
    std::pair<int, std::string> p2(2, std::string("banana"));
    std::cout << "  p2=(" << p2.first << ", " << p2.second << ")\n";

    // (3) C++17：結構化繫結（把 pair 拆成兩個名字）
#if defined(__cpp_structured_bindings) && (__cpp_structured_bindings >= 201606L)
    auto [id, name] = p1;
    std::cout << "  structured binding: id=" << id << ", name=" << name << "\n";
#else
    std::cout << "  (structured bindings 需要 C++17)\n";
#endif

    // (4) 比較：字典序（先比 first，再比 second）
    std::pair<int, std::string> a{1, "a"}, b{1, "b"}, c{2, "a"};
    std::cout << "  (1,\"a\") < (1,\"b\") ? " << (a < b) << "\n";
    std::cout << "  (1,\"a\") < (2,\"a\") ? " << (a < c) << "\n";

    // (5) swap：交換兩個 pair
    std::pair<int, std::string> x{1, "x"}, y{2, "y"};
    x.swap(y);
    std::cout << "  swap: x=(" << x.first << "," << x.second << ")\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】std::tuple：N 個值的打包 + std::tie / std::ignore
// -----------------------------------------------------------------------------
// tuple 代表「異質集合」：每格可以不同型別，格數也不限（由 template 參數決定）。
// 典型用途：
//   - 回傳多個值（>2）
//   - 把多個欄位放進關聯容器當 key（例如 map<tuple<...>, V>）
static void demo_tuple() {
    header("demo_tuple");

    std::tuple<int, std::string, double> t{7, "seven", 7.5};

    // (1) 取值：std::get<I>（I 必須是編譯期常數）
    std::cout << "  get<0>=" << std::get<0>(t)
              << ", get<1>=" << std::get<1>(t)
              << ", get<2>=" << std::get<2>(t) << "\n";

    // (2) 結構化繫結（C++17）
#if defined(__cpp_structured_bindings) && (__cpp_structured_bindings >= 201606L)
    auto [i, s, d] = t;
    std::cout << "  structured binding: " << i << ", " << s << ", " << d << "\n";
#else
    std::cout << "  (structured bindings 需要 C++17)\n";
#endif

    // (3) std::tie：把 tuple 指派給既有變數（常用於「只關心其中幾格」）
    int i2{};
    std::string s2;
    // double d2;   // 我們故意不要第三格
    std::tie(i2, s2, std::ignore) = t;
    std::cout << "  tie: i2=" << i2 << ", s2=" << s2 << "\n";

    // (4) tuple_size / tuple_element：編譯期反射
    std::cout << "  tuple_size=" << std::tuple_size<decltype(t)>::value << "\n";
    using FirstT = std::tuple_element<0, decltype(t)>::type;
    static_assert(std::is_same<FirstT, int>::value, "tuple_element<0> 應為 int");
}

// -----------------------------------------------------------------------------
// 【重點 3】std::optional<T>：有/沒有值（避免魔法值、避免裸指標）
// -----------------------------------------------------------------------------
// optional 很像「可能缺席的 T」：
//   - 有值：opt.has_value() == true，可用 *opt 或 opt.value()
//   - 沒值：代表「不存在 / 找不到 / 不適用」
//
// 注意：
//   - opt.value() 在沒值會丟例外；日常多用 if(opt) / *opt
//   - optional 不是用來表達「錯誤原因」；那通常需要 pair/variant 或 error_code
// -----------------------------------------------------------------------------
// 【重點 3】std::optional<T>：有/沒有值（避免魔法值、避免裸指標）
// -----------------------------------------------------------------------------
#if __cplusplus >= 201703L
static std::optional<int> parse_int(const std::string& s) {
    // 這裡只示範概念：用 stoi 解析，失敗回 nullopt
    try {
        size_t idx = 0;
        int v = std::stoi(s, &idx, 10);
        if (idx != s.size()) return std::nullopt; // 仍有殘留字元
        return v;
    } catch (...) {
        return std::nullopt;
    }
}

static void demo_optional() {
    header("demo_optional");

    for (const std::string& s : {"42", " 7", "abc", "123xyz"}) {
        auto r = parse_int(s);
        if (r) {
            std::cout << "  parse_int(\"" << s << "\") = " << *r << "\n";
        } else {
            std::cout << "  parse_int(\"" << s << "\") = (no value)\n";
        }
    }

    // 補充：value() / value_or()
    std::optional<int> a = 7;
    std::optional<int> b = std::nullopt;
    std::cout << "  a.value()=" << a.value() << "\n";
    std::cout << "  b.value_or(99)=" << b.value_or(99) << "\n";

    // 注意：b.value() 會丟 bad_optional_access（示範用 try/catch）
    try {
        (void)b.value();
    } catch (const std::bad_optional_access& e) {
        std::cout << "  b.value() throws bad_optional_access: " << e.what() << "\n";
    }
}
#else
static void demo_optional() {
    std::cout << "\n[demo_optional]\n";
    std::cout << "  (std::optional 需要 C++17)\n";
}
#endif

// -----------------------------------------------------------------------------
// 【重點 4】std::variant：型別安全 union（可選型別集合固定、編譯期已知）
// -----------------------------------------------------------------------------
// variant 表達：「在幾種型別中，當下持有其中一種」。
// 與 any 的差異：
//   - variant：型別集合固定（編譯期），拿錯型別會被保護（get_if / visit）
//   - any    ：可以裝任何型別（執行期），但你必須自己知道要 any_cast 成什麼
//
// 最關鍵用法：std::visit
#if __cplusplus >= 201703L
template <class... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};
template <class... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;
#endif

static void demo_variant() {
    header("demo_variant");

#if __cplusplus >= 201703L
    using Value = std::variant<int, double, std::string>;
    std::vector<Value> xs = {1, 2.5, std::string("hello"), 3};

    // (1) 用 visit 對每種型別做分派（比 get<T> 更安全、可擴充）
    for (const auto& v : xs) {
        std::visit(Overloaded{
            [](int x) { std::cout << "  int: " << x << "\n"; },
            [](double d) { std::cout << "  double: " << d << "\n"; },
            [](const std::string& s) { std::cout << "  string: " << s << "\n"; },
        }, v);
    }

    // (2) get_if：想「試著」拿某型別而不丟例外，用 get_if 最舒服
    Value v = std::string("world");
    if (auto p = std::get_if<std::string>(&v)) {
        std::cout << "  get_if<string>: " << *p << "\n";
    }

    // (3) index / holds_alternative：查目前裝什麼
    std::cout << "  index=" << v.index()
              << ", holds<string>=" << std::holds_alternative<std::string>(v)
              << ", holds<int>=" << std::holds_alternative<int>(v) << "\n";

    // (4) get<T>：拿錯型別會丟 bad_variant_access
    try {
        (void)std::get<int>(v);
    } catch (const std::bad_variant_access& e) {
        std::cout << "  get<int> throws bad_variant_access: " << e.what() << "\n";
    }
#else
    std::cout << "  (std::variant/std::visit 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 5】std::any：型別擦除（能裝任何型別，但需要 any_cast）
// -----------------------------------------------------------------------------
// any 適合「外掛式」或「框架型」場景：
//   - 你的容器/介面要能容納不固定、外部擴充的型別
//   - 但你也能接受：取出時必須 any_cast，型別不對就丟例外或回 nullptr
static void demo_any() {
    header("demo_any");

#if __cplusplus >= 201703L
    std::any a = 42;
    std::cout << "  a holds int? " << (a.type() == typeid(int)) << "\n";
    std::cout << "  any_cast<int>=" << std::any_cast<int>(a) << "\n";

    a = std::string("hi");
    if (auto p = std::any_cast<std::string>(&a)) {
        std::cout << "  any_cast<string> (ptr form) = " << *p << "\n";
    }

    // any_cast 拿錯型別：指標版回 nullptr，值版會丟 std::bad_any_cast
    if (std::any_cast<int>(&a) == nullptr) {
        std::cout << "  a is not int now\n";
    }

    // 值版本 any_cast 拿錯型別會丟 bad_any_cast
    try {
        (void)std::any_cast<int>(a);
    } catch (const std::bad_any_cast& e) {
        std::cout << "  any_cast<int>(wrong) throws bad_any_cast: " << e.what() << "\n";
    }
#else
    std::cout << "  (std::any 需要 C++17)\n";
#endif
}

// -----------------------------------------------------------------------------
// 【重點 6】std::move / std::forward：它們「只做轉型」，不會移動資料
// -----------------------------------------------------------------------------
// 這一點非常容易誤解：
//   - std::move(x)   ：把 x 轉成「右值」(rvalue)，讓「可移動」的型別得以觸發 move ctor/assign
//   - std::forward<T>(x)：在模板中保留傳入值的 value category（完美轉發）
//
// move/forward 不會自動搬資料，真正「搬」是由型別的 move ctor/assign 決定。
static void demo_move_forward_swap_exchange_as_const() {
    header("demo_move_forward_swap_exchange_as_const");

    // (1) move：把字串資源搬走（搬走後的物件仍需保持可用，但內容未規定）
    std::string s = "hello";
    std::string t = std::move(s);
    std::cout << "  after move: t=\"" << t << "\"; s.size()=" << s.size() << " (moved-from)\n";

    // (2) swap：交換兩個物件（對標準容器/字串通常是 O(1) 交換指標）
    std::string a = "AAA", b = "BBB";
    std::swap(a, b);
    std::cout << "  swap: a=\"" << a << "\", b=\"" << b << "\"\n";

    // (3) exchange：把 old value 取出，同時把新值放進去（常用於「狀態交換」）
    int x = 10;
    int old = std::exchange(x, 99);
    std::cout << "  exchange: old=" << old << ", x=" << x << "\n";

    // (4) as_const：取得 const 參考（避免不小心呼叫到 non-const overload）
    //     as_const 不會做拷貝，只是回傳 const T&
    std::vector<int> v{1, 2, 3};
    const auto& cv = std::as_const(v);
    std::cout << "  as_const: cv.size()=" << cv.size() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 7】std::bitset：固定大小 bit 操作 + 便於印出除錯
// -----------------------------------------------------------------------------
static void demo_bitset() {
    header("demo_bitset");

    std::bitset<8> bits;
    bits.set(0);            // 0000'0001
    bits.set(3);            // 0000'1001
    bits.flip(0);           // 0000'1000
    std::cout << "  bits=" << bits << " (as ulong=" << bits.to_ulong() << ")\n";

    // 常用：用 bitset 印出二進位（比自己寫迴圈可靠、可讀）
    std::uint32_t mask = 0b1010'1100u;
    std::cout << "  mask(32) = " << std::bitset<32>(mask) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 8】std::hash：把 key 轉成 size_t（給 unordered_* 用）
// -----------------------------------------------------------------------------
// 你通常不需要自己呼叫 hash，但你常常會需要：
//   - 自訂型別要能放進 unordered_map/unordered_set 時，提供 hash
// 這裡示範一個最常見的需求：hash(pair<int,int>)
struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const noexcept {
        // 簡單可用的組合方式：把兩個 int 的 hash 混在一起
        // 注意：hash 組合沒有標準唯一解；重點是「分散性」與「穩定性」。
        size_t h1 = std::hash<int>{}(p.first);
        size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};

static void demo_hash() {
    header("demo_hash");
    std::pair<int, int> p{3, 4};
    std::cout << "  hash(3,4)=" << PairHash{}(p) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 9】std::initializer_list：大括號初始化背後的「參數型別」
// -----------------------------------------------------------------------------
// initializer_list 常見於：
//   - 容器：vector<int> v{1,2,3};
//   - 讓 API 支援 { ... } 直覺寫法
//
// 注意：initializer_list 內部通常是指向「暫存陣列」的 view，
//       你不能把其中元素地址保存到 initializer_list 生命週期之外。
static int sum(std::initializer_list<int> xs) {
    int s = 0;
    for (int x : xs) s += x;
    return s;
}

static void demo_initializer_list() {
    header("demo_initializer_list");
    std::cout << "  sum{1,2,3,4}=" << sum({1, 2, 3, 4}) << "\n";
}

int main() {
    // 一次把整個目錄的工具串起來複習
    demo_pair();
    demo_tuple();
    demo_optional();
    demo_variant();
    demo_any();
    demo_move_forward_swap_exchange_as_const();
    demo_bitset();
    demo_hash();
    demo_initializer_list();

    std::cout << "\n[done]\n";
    return 0;
}

/*
================================================================================
【附錄：cppreference 風格速查（常用）】

pair<T1,T2>
  - members: first, second
  - modifiers: swap
  - comparisons: ==, !=, <, ...
  - tuple-like: get<0>/get<1>, tuple_size, tuple_element

tuple<Ts...>
  - get<I>, tuple_size, tuple_element
  - tie, ignore（常用搭配）

optional<T>（C++17）
  - observers: operator bool, has_value, value, value_or
  - modifiers: reset, emplace, swap

variant<Ts...>（C++17）
  - observers: index, holds_alternative, get/get_if
  - visitation: visit
  - modifiers: emplace, swap

any（C++17）
  - any_cast（值版/指標版）
  - type() 查詢目前型別

utility（常用 free functions）
  - move / forward
  - swap / exchange / as_const
================================================================================
*/

