// =============================================================================
//  05_generic_lambda.cpp  —  Generic Lambda (C++14) / Template Lambda (C++20)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/lambda
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、generic lambda（C++14）— 用 auto 當參數型別            │
//  └────────────────────────────────────────────────────────────┘
//
//  C++14 起允許參數寫 `auto`，這等同於宣告了一個「函式 template 的
//  operator()」：
//
//      auto add = [](auto a, auto b) { return a + b; };
//
//  編譯器產生的 closure 大致是：
//
//      class __lambda {
//      public:
//          template <class T1, class T2>
//          auto operator()(T1 a, T2 b) const { return a + b; }
//      };
//
//  好處：
//   * 一份 lambda 同時支援 int, double, string, vector ...
//   * 用 `auto&`、`const auto&` 可以處理「不希望拷貝」的情況
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、template lambda（C++20）— 顯式 template 參數           │
//  └────────────────────────────────────────────────────────────┘
//
//  generic lambda 用 auto 很方便，但「拿不到型別名字」很煩，例如：
//
//      auto f = [](auto v) {
//          using T = decltype(v);            // 要這樣才能取到型別
//          T x{};
//      };
//
//  C++20 起可以直接寫：
//
//      auto f = []<class T>(T v) {           // 顯式 template 參數
//          T x{};                            // 直接用 T
//      };
//
//  這在「想限制兩個參數同型別」、或「需要型別本身做 traits 判斷」時很有用：
//
//      auto sameTypeAdd = []<class T>(T a, T b) { return a + b; };
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔範例與 LeetCode                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：generic lambda — auto 參數
//  * Demo 2：const auto& — 不拷貝的通用印出器
//  * Demo 3 (C++20)：template lambda — 限制同型別
//  * LeetCode 1108. Defanging an IP Address — 把 "1.1.1.1" → "1[.]1[.]1[.]1"
// =============================================================================

/*
補充筆記：generic_lambda
  - generic_lambda 會產生一個 closure 物件；capture list 決定它保存資料還是借用資料。
  - 值捕獲安全但可能拷貝，參考捕獲便宜但有生命週期風險。
  - lambda 若被存起來、丟到 thread 或包進 std::function，捕獲物件必須活得夠久。
  - generic_lambda 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/
#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：generic lambda — 同一個 lambda 處理多種型別
    // ─────────────────────────────────────────────────────────
    auto add = [](auto a, auto b) { return a + b; };
    std::cout << "[Demo1] add(1, 2)         = " << add(1, 2) << '\n';
    std::cout << "[Demo1] add(1.5, 2.5)     = " << add(1.5, 2.5) << '\n';
    std::cout << "[Demo1] add(string, char*)= "
              << add(std::string{"hello, "}, "world") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：const auto& 印出器 — 對任意可印型別都能用
    // ─────────────────────────────────────────────────────────
    auto print = [](const auto& x) { std::cout << x; };
    print("[Demo2] "); print(42); print(' ');
    print(3.14); print(' ');
    print(std::string{"strs"}); print('\n');

    // ─────────────────────────────────────────────────────────
    // Demo 3：（C++20）template lambda — 限制兩參數同型別
    //   讓使用者明顯看到 "兩邊一定要同型別" 這個約束
    // ─────────────────────────────────────────────────────────
#if __cplusplus >= 202002L
    auto sameTypeAdd = []<class T>(T a, T b) { return a + b; };
    std::cout << "[Demo3] sameTypeAdd(2,3) = " << sameTypeAdd(2, 3) << '\n';
    // sameTypeAdd(1, 2.0); // ❌ 編譯錯：T 推導衝突 (int vs double)
#else
    std::cout << "[Demo3] (skipped: not C++20)\n";
#endif

    // ─────────────────────────────────────────────────────────
    // LeetCode 1108. Defanging an IP Address
    //   題意：把 IP 字串中所有 '.' 換成 '[.]'。
    //
    //   為什麼放這？示範 generic lambda 的「const auto&」用在標準演算法
    //   for_each 中，不必先寫死 char 型別。
    // ─────────────────────────────────────────────────────────
    {
        std::string ip = "192.168.0.1";
        std::string out;
        out.reserve(ip.size() + 6);

        std::for_each(ip.begin(), ip.end(), [&out](const auto& c) {
            if (c == '.') out += "[.]";
            else          out += c;
        });
        std::cout << "[LC1108] defang = " << out << '\n';
        // 預期：192[.]168[.]0[.]1
    }

    // ─────────────────────────────────────────────────────────
    // 額外實用例：用 generic lambda 處理 std::map<K,V>
    //   不必硬寫 std::pair<const std::string, int> 之類冗長型別
    // ─────────────────────────────────────────────────────────
    std::map<std::string, int> scores{{"alice", 90}, {"bob", 85}};
    std::for_each(scores.begin(), scores.end(),
                  [](const auto& kv) {
                      std::cout << "[map] " << kv.first << "=" << kv.second << '\n';
                  });

    // ─────────────────────────────────────────────────────────
    // LeetCode 1512. Number of Good Pairs
    //   題意：算「i<j 且 nums[i]==nums[j]」的 pair 數。
    //   方法：用 generic lambda 寫一個「對任何相等可比較容器」適用的計算器，
    //         展示 auto 參數讓同一邏輯適用 vector<int> 或 vector<string>。
    // ─────────────────────────────────────────────────────────
    {
        auto goodPairs = [](const auto& v) {
            int count = 0;
            for (size_t i = 0; i < v.size(); ++i)
                for (size_t j = i + 1; j < v.size(); ++j)
                    if (v[i] == v[j]) ++count;
            return count;
        };
        std::vector<int> a{1, 2, 3, 1, 1, 3};
        std::vector<std::string> b{"a", "b", "a", "a"};
        std::cout << "[LC1512] int  goodPairs = " << goodPairs(a) << '\n'; // 4
        std::cout << "[LC1512] str  goodPairs = " << goodPairs(b) << '\n'; // 3
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：通用 max — 寫一次 generic lambda、各型別通吃
    //   工作上要在 int / double / 自訂可比較型別中找最大值，generic lambda
    //   一行寫完就不用為每種型別各寫一個 helper
    // ─────────────────────────────────────────────────────────
    {
        auto maxOf = [](const auto& a, const auto& b, const auto& c) {
            auto r = a;
            if (b > r) r = b;
            if (c > r) r = c;
            return r;
        };
        std::cout << "[maxOf] int    = " << maxOf(3, 7, 2) << '\n';     // 7
        std::cout << "[maxOf] double = " << maxOf(1.1, 2.2, 0.5) << '\n'; // 2.2
        std::cout << "[maxOf] string = "
                  << maxOf(std::string{"apple"},
                           std::string{"banana"},
                           std::string{"cherry"}) << '\n';              // cherry
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：generic lambda 跟 function template 差在哪？
    //    A：本質一樣（都是 template）；lambda 寫起來短、就地用，function
    //       template 才能「跨檔案重用」、可以特化、可以放 namespace。
    //
    //  Q2：什麼時候應該用 const auto& 而不是 auto？
    //    A：參數可能是 vector、string、map 之類大物件、且不需要修改時。
    //       對 int、double 這類小物件用 auto（按值傳）反而更快。
    //
    //  Q3：C++20 之前要拿到 auto 參數的型別，怎麼辦？
    //    A：用 decltype：
    //         auto f = [](auto x) {
    //             using T = std::decay_t<decltype(x)>;
    //             ...
    //         };
    //
    return 0;
}
