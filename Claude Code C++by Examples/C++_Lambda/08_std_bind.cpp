// =============================================================================
//  08_std_bind.cpp  —  std::bind / placeholders（為何現代 C++ 偏好 lambda）
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/utility/functional/bind
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、std::bind 是什麼？                                     │
//  └────────────────────────────────────────────────────────────┘
//
//  std::bind 是 C++11 引入的工具：對任意可呼叫物件做「部分套用」(partial
//  application)，把某些參數先綁定，產生一個新的可呼叫物件。
//
//      using namespace std::placeholders;
//      int add(int a, int b) { return a + b; }
//
//      auto add5 = std::bind(add, 5, _1);
//      add5(10);                  // 等於 add(5, 10) = 15
//
//      auto rev = std::bind(add, _2, _1);
//      rev(1, 2);                 // 等於 add(2, 1) = 3 — 換參數順序
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、為什麼現代 C++ 不太用 std::bind？                     │
//  └────────────────────────────────────────────────────────────┘
//
//  (1) Lambda 大部分時候更清楚：
//        std::bind(add, 5, _1)   ⟶   [](int b){ return add(5, b); }
//      右邊讀者不需要記得 _1 是什麼、也不需要查 placeholders 是什麼。
//
//  (2) std::bind 的「值/參考」很容易踩坑：bind 預設「按值」儲存被綁定的
//      參數；要按參考要用 std::ref / std::cref：
//
//        int x = 1;
//        auto f = std::bind(printer, x);          // 拷貝 x（之後改 x 沒用）
//        auto g = std::bind(printer, std::ref(x)); // 真的綁 x 的 ref
//
//  (3) 巢狀 placeholders 跟其他 bind 結合會非常難讀。
//
//  Scott Meyers《Effective Modern C++》Item 34 結論：「絕大多數情況優先選
//  lambda，留下 std::bind 給少數歷史相容場景」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、bind 真正還有用的場合                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  * 老 codebase / 老介面已用 bind，維護時要看得懂
//  * 跟舊 std::function 簽名搭配時，bind 的語法極短
//  * 少數要保持 callable signature 與 perfect forwarding 的場景
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔範例                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：bind 部分套用、placeholders、std::ref
//  * Demo 2：等價 lambda 對照
// =============================================================================

/*
補充筆記：std_bind
  - std_bind 會產生一個 closure 物件；capture list 決定它保存資料還是借用資料。
  - 值捕獲安全但可能拷貝，參考捕獲便宜但有生命週期風險。
  - lambda 若被存起來、丟到 thread 或包進 std::function，捕獲物件必須活得夠久。
  - std_bind 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/
#include <functional>
#include <iostream>
#include <string>

static int add(int a, int b) { return a + b; }
static void greet(const std::string& greeting, const std::string& name) {
    std::cout << greeting << ", " << name << "!\n";
}

int main() {
    using namespace std::placeholders;   // _1, _2, ...

    // ─────────────────────────────────────────────────────────
    // Demo 1：bind 三個典型用法
    // ─────────────────────────────────────────────────────────
    auto add5  = std::bind(add, 5, _1);          // 固定 a = 5
    auto rev   = std::bind(add, _2, _1);         // 把參數順序反過來
    auto hi    = std::bind(greet, "Hi", _1);     // 固定 greeting

    std::cout << "[Demo1-bind] add5(10)  = " << add5(10) << '\n';   // 15
    std::cout << "[Demo1-bind] rev(1, 2) = " << rev(1, 2) << '\n';  // 3
    hi("Bob");

    // bind 的值 vs ref 陷阱
    int counter = 100;
    auto byVal = std::bind([](int n) { std::cout << "byVal=" << n << '\n'; },
                           counter);              // 拷貝 100
    auto byRef = std::bind([](int& n) { std::cout << "byRef=" << n << '\n'; },
                           std::ref(counter));    // 綁 counter
    counter = 999;
    byVal();      // 仍印 100（bind 時就拷貝了）
    byRef();      // 印 999（ref 看到當下的 counter）

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 lambda 寫一遍 — 對照可讀性
    // ─────────────────────────────────────────────────────────
    auto add5_l = [](int b) { return add(5, b); };
    auto rev_l  = [](int a, int b) { return add(b, a); };
    auto hi_l   = [](const std::string& name) { greet("Hi", name); };

    std::cout << "[Demo2-lambda] add5(10)  = " << add5_l(10) << '\n';
    std::cout << "[Demo2-lambda] rev(1, 2) = " << rev_l(1, 2) << '\n';
    hi_l("Alice");

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 std::bind 對成員函式做「固定 this」 dispatcher
    //   工作上常見：把某物件的成員函式包成「無 this 參數的」可呼叫物件，
    //   讓 callback 持有者不必認識物件型別。
    // ─────────────────────────────────────────────────────────
    {
        struct Counter {
            int value = 0;
            void addBy(int n) { value += n; }
        };
        Counter c;

        // bind 把 c 固定成第一個參數 (this)，addBy 變成只吃 int 的可呼叫物件
        auto adder = std::bind(&Counter::addBy, &c, _1);
        adder(3);
        adder(7);
        std::cout << "[bind-member] counter.value = " << c.value << '\n';   // 10

        // 等價 lambda 版本（現代寫法）— 對讀者更友善
        auto adderLambda = [&c](int n) { c.addBy(n); };
        adderLambda(5);
        std::cout << "[lambda-member] counter.value = " << c.value << '\n'; // 15
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：std::bind 建立「固定 log level」的記錄函式
    //   一份通用 log 函式 + bind 出每個等級各自的版本
    // ─────────────────────────────────────────────────────────
    {
        auto logImpl = [](const std::string& level, const std::string& msg) {
            std::cout << "  [" << level << "] " << msg << '\n';
        };
        auto info  = std::bind(logImpl, "INFO",  _1);
        auto warn  = std::bind(logImpl, "WARN",  _1);
        auto error = std::bind(logImpl, "ERROR", _1);

        info("starting up");
        warn("disk almost full");
        error("connection lost");
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::bind_front (C++20) 是什麼？
    //    A：bind 的簡化版 — 只能「從前面開始」依序綁參數，不能換順序。
    //       但語法沒有 placeholders，乾淨許多：
    //
    //         auto f = std::bind_front(greet, "Hi");
    //         f("Bob");   // greet("Hi", "Bob")
    //
    //       C++23 還有 std::bind_back，從後面綁起。
    //
    //  Q2：bind 跟 std::function 一起用會發生什麼？
    //    A：bind 回傳的物件可以隱式包進 std::function，這是 bind 在 C++11
    //       時代被廣泛使用的原因（lambda 包成 std::function 也可以，但當時
    //       lambda 寫起來比 bind 還短的場景不多）。
    //
    //  Q3：為什麼要用 std::ref？
    //    A：bind / make_tuple / thread 預設用「值」儲存參數。傳 ref 時必須
    //       用 std::ref / std::cref 包成 reference_wrapper，告訴 bind 你
    //       要的是 reference 而不是拷貝。
    //
    return 0;
}
