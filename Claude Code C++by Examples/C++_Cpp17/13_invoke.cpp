// =============================================================================
//  13_invoke.cpp  —  std::invoke (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/utility/functional/invoke
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、是什麼？                                               │
//  └────────────────────────────────────────────────────────────┘
//
//      std::invoke(callable, args...);
//
//  「統一呼叫」任何「可呼叫物件」的 helper：
//   * 普通函式 / 函式指標 / lambda     → callable(args...)
//   * 成員函式指標                     → (obj.*pmf)(args...) 或 (ptr->*pmf)(args...)
//   * 成員資料指標                     → obj.*pmd 或 ptr->*pmd
//   * std::function / std::bind 結果    → callable(args...)
//
//  std::invoke 對「成員函式指標 vs 普通函式」用同一個語法呼叫 — 對寫泛
//  型程式碼極有用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼有用？                                           │
//  └────────────────────────────────────────────────────────────┘
//
//  寫一個「吃 callable + 參數」的泛型 helper：
//
//      template <class F, class... Args>
//      auto run(F&& f, Args&&... args) {
//          return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
//      }
//
//  上面這寫法對：
//   * run(::add, 1, 2)
//   * run(&Foo::method, foo, 1, 2)
//   * run([](int x){ return x*2; }, 5)
//   全部一視同仁。沒有 invoke 你要寫一堆 SFINAE / overload 處理「成員函
//  式指標」的特殊呼叫語法。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：對普通函式
//   * Demo 2：對成員函式指標
//   * Demo 3：對成員資料指標
//   * Demo 4：寫一個泛型 run helper
// =============================================================================

/*
補充筆記：invoke
  - invoke 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - std::invoke 統一呼叫函式、函式物件、成員函式指標和資料成員指標。
  - 泛型 wrapper 若要支援所有 callable 形式，std::invoke 比手寫 f(args...) 更完整。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::invoke
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼需要 std::invoke？它統一了什麼？
//     答：可呼叫物件的呼叫語法本來有好幾種：普通函式／函式物件是 f(a)，成員函式
//         指標是 (obj.*pmf)(a) 或 (ptr->*pmf)(a)，成員資料指標是 obj.*pmd，
//         還得處理 reference_wrapper 與智慧指標。
//         std::invoke(f, args...) 以標準的 INVOKE 語意把這些收斂成單一寫法，
//         泛型程式碼（std::function、std::thread、std::bind、演算法）就不必為每種
//         情況各寫一份特化。
//     追問：成員函式的第一個引數是什麼？（物件本身——invoke(pmf, obj, args...)，
//           obj 可以是物件、參考、指標或 reference_wrapper）
//
// Q2. std::apply 和它是什麼關係？
//     答：std::apply(f, tuple) 把 tuple 展開成引數後，內部同樣以 INVOKE 語意呼叫；
//         差別只在引數來源是一個 tuple 而不是逐個列出。
//         那個位置可放任何支援 get / tuple_size 的型別，例如 std::array、std::pair。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

static int add(int a, int b) { return a + b; }

struct Foo {
    int x = 10;
    int twice(int n) const { return x * n * 2; }
};

template <class F, class... Args>
auto run(F&& f, Args&&... args) {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：對普通函式
    // ─────────────────────────────────────────────────────────
    auto a = std::invoke(add, 3, 4);
    std::cout << "[Demo1] add(3, 4) = " << a << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：對成員函式指標 (object 形式)
    // ─────────────────────────────────────────────────────────
    Foo foo;
    auto b = std::invoke(&Foo::twice, foo, 5);     // (foo.*&Foo::twice)(5)
    std::cout << "[Demo2] foo.twice(5) = " << b << '\n';

    // 對 pointer 形式也可
    Foo* fooPtr = &foo;
    auto c = std::invoke(&Foo::twice, fooPtr, 7);   // (fooPtr->*...)
    std::cout << "[Demo2] fooPtr->twice(7) = " << c << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：對成員資料指標
    // ─────────────────────────────────────────────────────────
    auto d = std::invoke(&Foo::x, foo);             // foo.x
    std::cout << "[Demo3] foo.x = " << d << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 4：泛型 helper run
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo4] run add: "      << run(add, 10, 20) << '\n';
    std::cout << "[Demo4] run member: "   << run(&Foo::twice, foo, 3) << '\n';
    std::cout << "[Demo4] run lambda: "   << run([](int x){ return x*x; }, 9) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：直接呼叫 callable(args...) 跟 std::invoke 一樣嗎？
    //    A：對普通 function / lambda 是；對「成員函式指標」就不是 — 後者
    //       要 (obj.*pmf)(args...) 才合法，直接 pmf(obj, args...) 不行。
    //       std::invoke 統一處理。
    //
    //  Q2：std::invoke_result_t 是什麼？
    //    A：搭配 invoke 的 metafunction — 算「呼叫結果型別」：
    //         using R = std::invoke_result_t<F, Args...>;
    //       泛型程式碼裡常用來宣告變數 / 推導 trait。
    //
    //  Q3：std::apply 跟 invoke 差在哪？
    //    A：apply 把 tuple 當參數列展開呼叫 callable：
    //         std::apply(f, std::tuple{1, 2, 3});  // = f(1, 2, 3)
    //       internally 也會用到 invoke。「需要展開 tuple」時用 apply。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：用成員函式指標排序 — invoke 統一呼叫語法
    //   工作上常見：寫泛型 sort/find 對「成員 getter」做 key 提取
    // ─────────────────────────────────────────────────────────
    struct Employee {
        std::string name;
        int salary;
    };
    std::vector<Employee> emps{
        {"alice",   80000},
        {"bob",     50000},
        {"charlie", 90000},
    };
    // 用 invoke 取成員 → 不必寫一個專屬 comparator
    auto byKey = [](auto pmd, const auto& cont) {
        auto copy = cont;
        std::sort(copy.begin(), copy.end(), [&](const auto& a, const auto& b) {
            return std::invoke(pmd, a) < std::invoke(pmd, b);
        });
        return copy;
    };
    auto sortedByName   = byKey(&Employee::name,   emps);
    auto sortedBySalary = byKey(&Employee::salary, emps);
    std::cout << "[Demo5] by name:";
    for (const auto& e : sortedByName)   std::cout << ' ' << e.name;
    std::cout << '\n';
    std::cout << "[Demo5] by salary:";
    for (const auto& e : sortedBySalary) std::cout << ' ' << e.name;
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：retry helper — 對任意 callable 重試 N 次
    //   工作上常見：網路請求 / DB 操作的簡單重試
    // ─────────────────────────────────────────────────────────
    auto retry = [](int times, auto&& f, auto&&... args) {
        for (int i = 0; i < times; ++i) {
            if (std::invoke(f, args...)) {
                std::cout << "[Demo6] retry attempt #" << (i + 1) << " success\n";
                return true;
            }
        }
        return false;
    };
    int attempts = 0;
    auto flakyOp = [&attempts]() {
        ++attempts;
        return attempts >= 3;            // 第 3 次才成功
    };
    retry(5, flakyOp);

    return 0;
}
