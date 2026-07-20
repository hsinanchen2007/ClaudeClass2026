// =============================================================================
//  04_init_capture.cpp  —  Init Capture (C++14)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/lambda
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、init capture 是什麼？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 的捕獲只能捕獲「外部已存在的變數」：
//
//      int x = 5;
//      auto f = [x] { return x; };
//
//  C++14 起允許在捕獲列表「直接初始化新名字」：
//
//      auto f = [n = 42] { return n; };          // 內部 n，外部沒有
//      auto g = [v = std::vector{1,2,3}] { ... };
//
//  好處：
//   1. 可以「在 closure 內部」搬入一個 move-only 物件（如 unique_ptr）
//   2. 可以做「捕獲時的計算 / 轉換」，不用先建一個臨時變數
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最重要的應用：搬移 unique_ptr 進 lambda                │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 的捕獲做不到這件事 — by value 需要拷貝建構，但 unique_ptr 不可
//  拷貝；by ref 又怕外部那條 unique_ptr 被釋放。C++14 init capture 解決：
//
//      auto p = std::make_unique<int>(42);
//      auto f = [up = std::move(p)] { return *up; };
//      // 此時 p 已經是 nullptr，up 在 closure 內持有資源
//
//  搭配 std::function 包裝後，可以把整個帶 unique_ptr 的 lambda 塞進
//  容器、傳給 thread 之類，所有權跟著 closure 走。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、init capture 也可以接 ref                              │
//  └────────────────────────────────────────────────────────────┘
//
//      int x = 10;
//      auto f = [&r = x] { ... };       // r 是 x 的 ref（自定名字）
//      auto g = [&r = std::as_const(x)] { ... };
//                                       // 用 as_const 避免意外修改原物件
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔範例                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：基本 init capture
//  * Demo 2：搬移 unique_ptr 進 lambda
//  * Demo 3：捕獲時做轉換（避免污染外層 scope）
// =============================================================================

/*
補充筆記：init_capture
  - init_capture 會產生一個 closure 物件；capture list 決定它保存資料還是借用資料。
  - 值捕獲安全但可能拷貝，參考捕獲便宜但有生命週期風險。
  - lambda 若被存起來、丟到 thread 或包進 std::function，捕獲物件必須活得夠久。
  - init_capture 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】init-capture（廣義捕獲，C++14）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. init-capture 解決了什麼問題？是哪個標準加入的？
//     答：C++14（不是 C++11）。[name = expr] 允許在閉包內新建一個成員，並用任意運算式
//     初始化它，不必對應同名的外部變數。兩大用途：① 移動捕獲
//     [p = std::move(ptr)]，把 unique_ptr 這種 move-only 物件搬進閉包（C++11 做不到，
//     因為當時捕獲只能複製或取參考）；② 計算捕獲 [n = v.size()]，只算一次。
//     追問：想要具名的參考捕獲怎麼寫？（[&r = x]）
//
// 🔥 Q2. init-capture 成員的型別怎麼決定？
//     答：如同 auto name = expr; 的推導規則——會 decay（去掉參考與頂層 const）。
//     所以 [v = someVector] 得到的是一份副本；要參考語意必須明寫 [&v = someVector]。
//
// ⚠️ 陷阱. [p = std::move(ptr)] 之後，外部的 ptr 還能用嗎？
//     答：ptr 處於「有效但未指定」狀態。對 std::unique_ptr 而言移動後保證為 nullptr，
//     但那是 unique_ptr 自己的規格，不是所有型別的通則——一般型別只保證可安全解構、
//     可賦新值，不可假設其內容。
//     為什麼會錯：大家把「unique_ptr 被移動後變 nullptr」推廣成所有型別的普遍保證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本 init capture — 直接在捕獲列表裡命名一個新變數
    // ─────────────────────────────────────────────────────────
    auto greet = [name = std::string{"Bob"}](const std::string& hello) {
        return hello + ", " + name + "!";
    };
    std::cout << "[Demo1] " << greet("Hi") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：搬移 unique_ptr 進 closure
    //   為什麼重要？unique_ptr 不可拷貝，C++11 老式 [p] 會編譯錯。
    // ─────────────────────────────────────────────────────────
    auto p = std::make_unique<std::string>("hello unique_ptr");
    auto holder = [up = std::move(p)] {
        // 這個 closure 現在「擁有」這個字串
        return *up;
    };
    std::cout << "[Demo2] outer p is "
              << (p ? "still valid" : "now nullptr") << '\n';
    std::cout << "[Demo2] holder() = " << holder() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：捕獲時做轉換 — 避免污染外層 scope
    //   傳統寫法需要先建臨時變數：
    //
    //     auto upper = toUpper(s);
    //     auto f = [upper] { ... };  // 多一個一次性的局部變數
    //
    //   init capture 一行解決：
    // ─────────────────────────────────────────────────────────
    std::string raw = "hello";
    auto upper = [s = std::string{raw}]() mutable {
        // ⚠️ toupper 的參數要先轉 unsigned char（char 可能是負值 → UB）
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    };
    std::cout << "[Demo3] " << upper() << '\n';
    // 注意：因為要 ++c 修改字串，所以要 mutable

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：把大型 vector 用 move 搬進 lambda，避免拷貝
    //   工作上常見：取得一個大資料集後，要包成 callback 排隊處理
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> bigData;
        bigData.reserve(5);
        for (int i = 1; i <= 5; ++i) bigData.push_back(i * 100);

        // 用 init capture 把 bigData「搬」進去，外面那條變空殼
        auto consume = [v = std::move(bigData)]() {
            long long sum = 0;
            for (int x : v) sum += x;
            return sum;
        };
        std::cout << "[move-into-lambda] outer size after move = "
                  << bigData.size() << '\n';                // 0
        std::cout << "[move-into-lambda] consume() = "
                  << consume() << '\n';                     // 1500
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：lazy / cached value — 第一次呼叫才算，之後直接拿快取
    //   實務應用：昂貴運算的結果可以延遲到「真的用到」時才做
    // ─────────────────────────────────────────────────────────
    {
        // computed 用 init capture 預先放快取槽；done 標記是否算過
        auto lazy = [computed = 0, done = false]() mutable {
            if (!done) {
                std::cout << "  (heavy work runs once)\n";
                computed = 42;        // 假裝這是昂貴運算的結果
                done = true;
            }
            return computed;
        };
        std::cout << "[lazy] first  = " << lazy() << '\n';   // 印 heavy work
        std::cout << "[lazy] second = " << lazy() << '\n';   // 不再印
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：init capture 的型別怎麼推導？
    //    A：用初始化的右側做 auto 推導，等同於 `auto name = expr;`。
    //       想要 reference 就寫 `[&name = expr]`。
    //
    //  Q2：能不能在 init capture 內部用 if / for？
    //    A：不行。init capture 是「初始化器」，不是函式體；只能放
    //       expression。要做複雜邏輯，就在外部寫好再 move 進來，或包成
    //       一個 IILE：[v = []{ /* ... */ return val; }()]
    //
    //  Q3：unique_ptr lambda 怎麼塞進 std::function？
    //    A：std::function 要求被包裝的物件「可拷貝」，所以含 unique_ptr 的
    //       lambda 不能直接放 std::function（會編譯錯）。
    //       改用 std::move_only_function（C++23）或 std::function 改包成
    //       shared_ptr 再丟入。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 04_init_capture.cpp -o 04_init_capture

// === 預期輸出 ===
// [Demo1] Hi, Bob!
// [Demo2] outer p is now nullptr
// [Demo2] holder() = hello unique_ptr
// [Demo3] HELLO
// [move-into-lambda] outer size after move = 0
// [move-into-lambda] consume() = 1500
// [lazy] first  =   (heavy work runs once)
// 42
// [lazy] second = 42
