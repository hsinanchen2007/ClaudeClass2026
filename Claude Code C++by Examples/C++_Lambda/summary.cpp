/*
================================================================================
【C++_Lambda/summary.cpp】

本目錄主題：Lambda（匿名函式物件）與 functional 工具

你需要掌握的重點：
  - 基本語法：[](){}，回傳型別多半可省略
  - 捕獲（capture）：
      [=] 值捕獲、[&] 參考捕獲、[x] 捕獲特定變數、[this] 捕獲 this
      C++14 init-capture： [p = std::move(ptr)] {...}
  - mutable：允許修改「值捕獲」的副本
  - generic lambda（C++14）：[](auto x){...}
  - 與演算法搭配：std::sort/std::find_if/std::for_each...
  - std::function：型別擦除的可呼叫物件（有額外成本）
  - std::bind：可用但常被 lambda 取代（可讀性/陷阱）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯（並示範 C++14 語法仍可在 C++17 下編譯）

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Lambda/C++_Lambda summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Lambda/C++_Lambda summary 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Lambda/C++_Lambda summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lambda 總覽（標準版本與閉包性質）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lambda 相關特性各是哪個標準加入的？
//     答：lambda 基本語法 C++11；generic lambda（auto 參數）與 init-capture
//     [x = expr] 是 C++14；constexpr lambda 與 [*this] 捕獲是 C++17；模板化 lambda
//     []<class T>(...) 是 C++20，同一版起 [=] 隱式捕獲 this 被棄用、無捕獲 lambda 可
//     預設建構與賦值。答錯版本在面試中很扣分，尤其 generic lambda 常被誤答成 C++11。
//     追問：C++23 又加了什麼？（deducing this：[](this auto&& self, ...){}，讓 lambda
//     可以直接遞迴呼叫自己）
//
// 🔥 Q2. lambda 可以遞迴嗎？
//     答：不能直接遞迴——在本體內 auto f = [](int n){ return f(n-1); } 的 f 尚未完成型別
//     推導。三種解法：① std::function<int(int)> f = [&f](int n){...};（有型別擦除成本，
//     且 [&f] 參考自身，f 一離開作用域即懸垂）；② Y-combinator 風格
//     auto f = [](auto&& self, int n) -> int { ... }; f(f, 5);；③ C++23 的 deducing this。
//
// Q3. 閉包型別有哪些特殊性質？
//     答：有捕獲者不可賦值；C++20 前無預設建構子；有隱式複製／移動建構子；參考捕獲的
//     成員在複製後由兩個閉包共享同一被參考物；無捕獲者可隱式轉成函式指標；每個 lambda
//     型別唯一，所以 C++20 前要把 lambda 當 std::map 的比較器必須傳入實體：
//     auto cmp = ...; std::map<K, V, decltype(cmp)> m(cmp);
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

static void demo_basic_and_capture() {
    std::cout << "\n[demo_basic_and_capture]\n";

    int a = 10;
    int b = 20;

    auto by_value = [=]() { return a + b; }; // 捕獲副本
    auto by_ref = [&]() { a += 1; b += 1; }; // 捕獲參考，可修改外部

    std::cout << "  by_value()=" << by_value() << "\n";
    by_ref();
    std::cout << "  after by_ref: a=" << a << ", b=" << b << "\n";
}

static void demo_mutable_and_init_capture() {
    std::cout << "\n[demo_mutable_and_init_capture]\n";

    int x = 5;
    auto f = [x]() mutable { // mutable 讓你能修改「捕獲副本 x」
        x += 10;
        return x;
    };
    std::cout << "  f()=" << f() << ", f()=" << f() << "\n";

    // C++14 init-capture：捕獲一個「表達式結果」
    std::string s = "hello";
    auto g = [t = std::move(s)]() { return t.size(); };
    std::cout << "  moved capture size=" << g() << "\n";
}

static void demo_generic_lambda_and_algorithms() {
    std::cout << "\n[demo_generic_lambda_and_algorithms]\n";

    std::vector<int> v{5, 1, 4, 2, 3};
    std::sort(v.begin(), v.end(), [](int lhs, int rhs) { return lhs < rhs; });

    auto is_even = [](auto n) { return (n % 2) == 0; }; // generic lambda
    auto it = std::find_if(v.begin(), v.end(), is_even);
    if (it != v.end()) std::cout << "  first even=" << *it << "\n";

    int sum = std::accumulate(v.begin(), v.end(), 0, [](int acc, int x) { return acc + x; });
    std::cout << "  sum=" << sum << "\n";
}

static void demo_std_function_and_bind() {
    std::cout << "\n[demo_std_function_and_bind]\n";

    // std::function：把可呼叫物件「裝箱」成固定型別（有 type-erasure 成本）
    std::function<int(int)> inc = [](int x) { return x + 1; };
    std::cout << "  std::function inc(7)=" << inc(7) << "\n";

    // std::bind：可用，但常被 lambda 取代（閱讀性通常更差）
    auto plus10 = std::bind(std::plus<int>{}, std::placeholders::_1, 10);
    std::cout << "  bind plus10(5)=" << plus10(5) << "\n";
}

int main() {
    demo_basic_and_capture();
    demo_mutable_and_init_capture();
    demo_generic_lambda_and_algorithms();
    demo_std_function_and_bind();

    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
//
// [demo_basic_and_capture]
//   by_value()=30
//   after by_ref: a=11, b=21
//
// [demo_mutable_and_init_capture]
//   f()=15, f()=25
//   moved capture size=5
//
// [demo_generic_lambda_and_algorithms]
//   first even=2
//   sum=15
//
// [demo_std_function_and_bind]
//   std::function inc(7)=8
//   bind plus10(5)=15
//
// [done]
