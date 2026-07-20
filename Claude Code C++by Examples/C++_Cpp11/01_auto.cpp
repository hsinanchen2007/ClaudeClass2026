// =============================================================================
//  01_auto.cpp  —  auto 型別推導 (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/auto
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、auto 是什麼？                                          │
//  └────────────────────────────────────────────────────────────┘
//
//  C++98 寫法：
//      std::vector<std::pair<std::string, int>>::const_iterator it = v.begin();
//
//  C++11 起 auto 讓編譯器推導型別：
//      auto it = v.begin();      // 跟上面等價、但好讀很多
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、推導規則 — 跟 template type deduction 一致             │
//  └────────────────────────────────────────────────────────────┘
//
//  auto x = expr;
//
//  把 auto 想成「無名 template parameter T」：
//      template <class T> void f(T x) { ... }
//      f(expr);                   // T 怎麼推 → auto 就是什麼
//
//  關鍵點：
//   * 「值」推導：拷貝去掉 reference / top-level const。
//        const int& r = x;
//        auto a = r;              // a 是 int（去掉 const & ref）
//
//   * 「ref / 指標」推導：保留 const-volatile。
//        const int& r = x;
//        auto& b = r;             // b 是 const int&
//        const auto* p = &x;      // p 是 const int*
//
//   * 陣列退化成指標（除非用 auto&）：
//        int arr[3];
//        auto a = arr;            // a 是 int*
//        auto& b = arr;           // b 是 int (&)[3]
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、實務最有用的場景                                       │
//  └────────────────────────────────────────────────────────────┘
//
//   1) iterator
//      auto it = container.begin();
//
//   2) lambda 物件（型別是「無名 closure type」，根本沒辦法寫）
//      auto f = [](int x) { return x + 1; };
//
//   3) range-based for
//      for (auto& x : v) { ... }
//
//   4) 結合 STL function 回傳型別
//      auto result = std::find_if(v.begin(), v.end(), pred);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、要避免的用法                                           │
//  └────────────────────────────────────────────────────────────┘
//
//   * auto 對 「proxy 物件」會抓到代理（不是你以為的型別）：
//        std::vector<bool> v{true, false};
//        auto x = v[0];           // x 是「proxy reference」，不是 bool
//        bool y = v[0];           // y 才是真的 bool
//
//   * 純粹想表達「這就是一個 int」時還是寫 int — auto 不是萬靈丹
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：基本推導
//   * Demo 2：值 vs 參考 vs const
//   * Demo 3：陣列退化
//   * LeetCode 1929. Concatenation of Array — auto 在 range-for 中的用法
// =============================================================================

/*
補充筆記：auto
  - auto 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - auto 會依初始化式推導型別；沒有初始化式就不能推導。
  - auto x = expr 會複製或移動值，若要避免複製大型物件應寫 const auto&。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto 型別推導
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto 的推導規則是什麼？
//     答：沿用 template type deduction，分三種情形。`auto x = e` 按值推導，丟掉
//         top-level const 與 reference，陣列／函數退化成指標；`auto& x`、
//         `const auto& x` 保留 const 且不退化；`auto&& x` 是 forwarding
//         reference，接左值得左值參考、接右值得右值參考。
//     追問：那 `const auto* p = &x;` 的 const 為什麼沒被丟掉？
//         （被丟的只有 top-level const，這裡的 const 修飾的是「被指的物件」）
//
// 🔥 Q2. auto 推導與 template 推導唯一的差異在哪？
//     答：braced initializer。`auto x = {1, 2, 3};` 推導成
//         `std::initializer_list<int>`；但同一組 `{1, 2, 3}` 傳給
//         `template <class T> void f(T)` 卻完全推不出 T——它是 non-deduced
//         context，編譯器不會替你猜成 initializer_list。
//     追問：所以完美轉發為什麼「轉發不了」`f({1, 2, 3})`？（正是同一個原因）
//
// ⚠️ 陷阱. `std::vector<bool> v{true}; auto b = v[0];` 的 b 是什麼型別？
//     答：不是 bool，而是 proxy 型別 `std::vector<bool>::reference`。
//         vector<bool> 是位元壓縮的特化版，`operator[]` 回傳的是代理物件而不是
//         `bool&`。之後若 v 被 clear() 或擴容，b 內部指向的位元就失效，再讀是 UB。
//     為什麼會錯：多數人腦中的模型是「auto 會推成我看到的那個值的型別」，但
//         auto 忠實推導的是「運算式真正的型別」，而代理物件本來就設計成隱形的。
//         解法是顯式型別初始化慣用法：`auto b = static_cast<bool>(v[0]);`
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <typeinfo>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本推導
    // ─────────────────────────────────────────────────────────
    auto i  = 42;            // int
    auto d  = 3.14;          // double
    auto s  = "hello";       // const char*（注意不是 std::string）
    auto v  = std::vector<int>{1, 2, 3};

    std::cout << "[Demo1] i=" << i << " d=" << d
              << " s=" << s << " size=" << v.size() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：值 vs 參考 vs const
    // ─────────────────────────────────────────────────────────
    int x = 5;
    const int& cr = x;

    auto  a = cr;            // a 是 int（去 const、去 &）
    auto& b = cr;            // b 是 const int&（保留 const + ref）
    const auto& c = x;       // 顯式 const ref

    a = 99;                   // OK，a 跟 x 是兩條
    // b = 99;                // ❌ 編譯錯：b 是 const int&
    (void)b; (void)c;
    std::cout << "[Demo2] x=" << x << " (a copied: " << a << ")\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：陣列退化
    // ─────────────────────────────────────────────────────────
    int arr[3] = {10, 20, 30};
    auto  p = arr;            // int*
    auto& r = arr;            // int (&)[3]

    std::cout << "[Demo3] sizeof(arr)=" << sizeof(arr)
              << " sizeof(p)="   << sizeof(p)        // 通常 8
              << " sizeof(r)="   << sizeof(r) << '\n'; // 同 sizeof(arr)

    // ─────────────────────────────────────────────────────────
    // LeetCode 1929. Concatenation of Array
    //   題意：給 nums，回傳 [nums..., nums...] (拼成兩倍長度)
    //   為什麼放這？示範 auto 在 range-for 與 reserve 後的 push_back 標準姿勢。
    // ─────────────────────────────────────────────────────────
    std::vector<int> nums{1, 2, 1};
    std::vector<int> ans;
    ans.reserve(nums.size() * 2);
    for (auto x : nums) ans.push_back(x);   // auto = int (按值)
    for (auto x : nums) ans.push_back(x);
    std::cout << "[LC1929] ans =";
    for (auto v2 : ans) std::cout << ' ' << v2;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：auto x = 0; auto& y = 0; 哪個對？
    //    A：第一個對；第二個錯（不能對 rvalue 取非 const ref）。
    //       const auto& y = 0; 才行。
    //
    //  Q2：「auto a = some_lambda;」的 a 是什麼型別？
    //    A：每個 lambda 都是編譯器自動產生的「unique unnamed class」，所以
    //       a 的型別是某個只此一份的型別。沒辦法寫名字 — 必須用 auto。
    //
    //  Q3：可不可以宣告 auto 沒初始化？
    //    A：不行。編譯器需要 initializer 才能推導型別：
    //         auto x;            // ❌
    //         auto x = 5;        // ✅
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 1672. Richest Customer Wealth
    //   題意：給 accounts[i][j] 是第 i 位客戶在第 j 家銀行的存款，
    //         回傳「最有錢客戶」的總財富。
    //   為什麼放這？示範 auto 在「巢狀容器走訪」中的便利寫法。
    // ─────────────────────────────────────────────────────────
    std::vector<std::vector<int>> accounts{{1, 2, 3}, {3, 2, 1}, {7, 7, 7}};
    int mx = 0;
    for (const auto& row : accounts) {   // auto = vector<int>，用 const ref 避免拷貝
        int sum2 = 0;
        for (auto v3 : row) sum2 += v3;   // auto = int
        if (sum2 > mx) mx = sum2;
    }
    std::cout << "[LC1672] max wealth = " << mx << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：解析設定鍵值對
    //   工作上常見：把字串解析成 (key, value) — 用 auto 接 STL 回傳
    // ─────────────────────────────────────────────────────────
    std::string config = "host=localhost;port=8080;user=admin";
    std::size_t start = 0;
    while (start < config.size()) {
        auto end = config.find(';', start);          // auto = size_t
        if (end == std::string::npos) end = config.size();
        auto kv = config.substr(start, end - start);  // auto = std::string
        auto eq = kv.find('=');                       // auto = size_t
        if (eq != std::string::npos) {
            std::cout << "[Config] " << kv.substr(0, eq)
                      << " => " << kv.substr(eq + 1) << '\n';
        }
        start = end + 1;
    }

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 01_auto.cpp -o 01_auto

// === 預期輸出 ===
// [Demo1] i=42 d=3.14 s=hello size=3
// [Demo2] x=5 (a copied: 99)
// [Demo3] sizeof(arr)=12 sizeof(p)=8 sizeof(r)=12
// [LC1929] ans = 1 2 1 1 2 1
// [LC1672] max wealth = 21
// [Config] host => localhost
// [Config] port => 8080
// [Config] user => admin
