// =============================================================================
//  10_lambda_pitfalls.cpp  —  Lambda 常見陷阱
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/lambda
//
//  這個檔案不教新功能，集中說明 lambda 在實務上最常踩的雷。每個案例都要
//  能說出「會發生什麼問題、怎麼避免」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 1：dangling reference（懸掛參考）                     │
//  └────────────────────────────────────────────────────────────┘
//
//  by-ref 捕獲指向某個區域變數，但 lambda 比那個變數活得久 → UB。
//
//      std::function<int()> makeCounter() {
//          int n = 0;
//          return [&n] { return ++n; };  // ❌ n 會在函式 return 後死亡
//      }
//
//  正確做法：by-value（含 init capture move）。
//
//      std::function<int()> makeCounter() {
//          return [n = 0]() mutable { return ++n; }; // ✅ 內部自有 n
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 2：[this] 在物件已死後被呼叫                          │
//  └────────────────────────────────────────────────────────────┘
//
//  把成員函式裡的 lambda 存到容器 / 註冊成 callback，但物件先死了：
//
//      class Widget {
//      public:
//          void registerCb(EventBus& bus) {
//              bus.add([this] { onTick(); }); // 之後 Widget 死了 → UB
//          }
//      };
//
//  解法：
//   * C++17 [*this]：把 *this 整個拷貝進 closure（成本較大但安全）
//   * 用 std::weak_ptr + shared_from_this：呼叫前 lock()，物件死了就略過
//   * 在物件 destructor 主動把 callback unregister
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 3：[=] 仍隱式捕獲 this 指標（C++20 棄用未移除）       │
//  └────────────────────────────────────────────────────────────┘
//
//  [=] 會「順便」隱式捕獲 this，造成「我以為整個物件都複製了，其實只複製了
//  一個 this 指標」的誤會 —— 物件死掉後 closure 就懸空。C++20 把這種隱式捕獲
//  標記為 deprecated，但【沒有移除】：本機 g++/clang 以 -std=c++20 與 c++23
//  實測，都只出一個 -Wdeprecated 警告，程式照常編譯並輸出正確值。
//  所以那個經典的懸空 this bug 原封不動，C++20 並沒有幫你修掉。
//  實務一律明寫 [=, this]（捕指標）或 [*this]（C++17，複製整個物件）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 4：兩條路 return 不同型別 → 推導失敗                  │
//  └────────────────────────────────────────────────────────────┘
//
//      auto f = [](bool b) {
//          if (b) return 1;       // int
//          else   return 1.0;     // double
//      };  // ❌ 型別衝突
//
//  解法：明寫 return type → `(bool b) -> double { ... }`
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 5：std::function 容器中 lambda 拷貝成本               │
//  └────────────────────────────────────────────────────────────┘
//
//  含大型捕獲 (e.g. vector<int> 一萬筆) 的 lambda 每次塞進 std::function
//  都會拷貝一次。要不要 move 進去？要不要改 by-ref？要不要包成
//  shared_ptr？視場景決定。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔範例與 LeetCode                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：dangling 陷阱對照（安全版 vs 危險版註解）
//  * Demo 2：return 型別衝突的修法
//  * LeetCode 202. Happy Number — 用 lambda + set 一起時要注意捕獲
// =============================================================================

/*
補充筆記：lambda_pitfalls
  - lambda_pitfalls 會產生一個 closure 物件；capture list 決定它保存資料還是借用資料。
  - 值捕獲安全但可能拷貝，參考捕獲便宜但有生命週期風險。
  - lambda 若被存起來、丟到 thread 或包進 std::function，捕獲物件必須活得夠久。
  - lambda_pitfalls 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】lambda 的生命週期陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼「回傳一個參考捕獲了區域變數的 lambda」是災難？
//     答：auto makeAdder(int n){ return [&n](int x){ return x + n; }; } 中 n 是函式參數，
//     makeAdder 一返回就銷毀，閉包持有的參考立即懸垂，之後呼叫是 UB。而且它常常「看起來
//     能跑」，因為那塊堆疊還沒被覆寫——這是最惡劣的隱性 bug。正解是值捕獲 [n]。
//     規則：只要 lambda 會活得比目前作用域久，就不可以參考捕獲區域物件。
//     追問：為什麼編譯器不報錯？（生命週期分析不是強制的；-Wdangling、clang-tidy、
//     AddressSanitizer 可以幫忙抓）
//
// 🔥 Q2. C++17 的 [*this] 捕獲解決什麼問題？
//     答：在成員函式中 [this]（以及 [=]／[&] 的隱含行為）捕獲的是指標，閉包裡的 member
//     其實是 this->member。若閉包活得比物件久（存進成員、非同步回呼、丟給 std::thread），
//     就是懸垂指標 UB。[*this] 把整個物件複製進閉包，物件銷毀後閉包仍持有自己的副本。
//     追問：還有什麼替代方案？（[self = shared_from_this()] 延長物件壽命）
//
// 🔥 Q3. 在迴圈裡建立 lambda 並存起來，捕獲迴圈變數有什麼坑？
//     答：for (int i = 0; i < 3; ++i) v.push_back([&i]{ ... }); 三個閉包都參考同一個 i，
//     且迴圈結束後 i 已離開作用域，全部懸垂；即使 i 還活著，三個閉包也會讀到同一個值。
//     正解是 [i] 值捕獲，各自持有 0/1/2 的副本。
//
// ⚠️ 陷阱. 「我用了 [=]，所以一定安全」——對嗎？
//     答：不對。值捕獲只複製「那一層」：捕獲一個 T* 只複製指標值，所指物件照樣可能先死；
//     捕獲 std::string_view 也一樣；捕獲一個「內含參考的物件」同樣不會深拷貝。
//     為什麼會錯：安全性取決於「被捕獲的型別本身的語意」，不是捕獲的方式。真正安全的做法
//     是值捕獲擁有型的物件（std::string、std::vector），或用 shared_ptr 延壽。
// ═══════════════════════════════════════════════════════════════════════════

#include <functional>
#include <iostream>
#include <unordered_set>

// 安全版：by-value / init capture
std::function<int()> safeCounter() {
    return [n = 0]() mutable { return ++n; };
}

// 危險版（保留作對照，不會被呼叫）
// std::function<int()> dangerCounter() {
//     int n = 0;
//     return [&n] { return ++n; };  // ❌ 回傳後 n 死亡 → UB
// }

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：safeCounter 可以正確累積
    // ─────────────────────────────────────────────────────────
    auto c = safeCounter();
    std::cout << "[Demo1] " << c() << ' ' << c() << ' ' << c() << '\n'; // 1 2 3

    // ─────────────────────────────────────────────────────────
    // Demo 2：return 型別衝突的修法
    //   原本兩條路 return int / double 會編譯錯，要明寫 -> double
    // ─────────────────────────────────────────────────────────
    auto half = [](int n) -> double {
        if (n % 2 == 0) return n / 2;     // int 會被 promote 成 double
        else            return n / 2.0;
    };
    std::cout << "[Demo2] half(7) = " << half(7) << '\n';

    // ─────────────────────────────────────────────────────────
    // LeetCode 202. Happy Number
    //   題意：對 n 不斷做「各位數平方和」，最終會變成 1 (happy) 或進入循環。
    //   此題用 unordered_set 紀錄走過的數，看到重複就代表進入循環 → false。
    //
    //   為什麼放這？示範「兩個 lambda 分工」 — 一個算下一步、一個跑主迴圈，
    //   並注意「主迴圈 lambda 用 [&] 捕獲 set，因為要持續修改」。
    // ─────────────────────────────────────────────────────────
    auto next = [](int n) {
        int s = 0;
        while (n > 0) { int d = n % 10; s += d * d; n /= 10; }
        return s;
    };
    auto isHappy = [&next](int n) {
        std::unordered_set<int> seen;
        while (n != 1 && !seen.count(n)) {
            seen.insert(n);
            n = next(n);
        }
        return n == 1;
    };
    std::cout << "[LC202] isHappy(19) = " << std::boolalpha << isHappy(19) << '\n';
    std::cout << "[LC202] isHappy(2)  = " << std::boolalpha << isHappy(2)  << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：把 lambda 「存進 vector」時，務必把外部依賴 by value 帶走
    //   工作場景：建立一堆「延後執行」的工作；任務丟進 queue，到 worker 才跑。
    //   危險寫法：迴圈裡用 [&] 捕獲 i — 所有 lambda 共用同一條 reference，
    //   迴圈結束後 i 早就死了，執行時拿到的是 UB（或最後一輪的殘值）。
    //   正確做法：[i] 值捕獲，把當下的 i 「拍進去」。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<std::function<int()>> tasks;
        // ❌ 假如這樣寫：
        // for (int i = 0; i < 3; ++i)
        //     tasks.emplace_back([&i] { return i; });   // dangling ref to i
        //
        // ✅ 正確寫法：用值捕獲 [i]，每個 lambda 拍下自己當下的 i
        for (int i = 0; i < 3; ++i)
            tasks.emplace_back([i] { return i * i; });

        std::cout << "[deferred-tasks] results:";
        for (auto& t : tasks) std::cout << ' ' << t();
        std::cout << '\n'; // 預期：0 1 4
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：return type 衝突 + 用 `static_cast` 在 return 處統一型別
    //   除了 `-> T` 之外，也可在 return 處 cast；兩種寫法工作場景都會看到
    // ─────────────────────────────────────────────────────────
    {
        // 不寫 -> double，但 return 端 cast — 同樣解決推導衝突
        auto safeDiv = [](int a, int b) {
            if (b == 0) return static_cast<double>(0);  // 強制 double
            return static_cast<double>(a) / b;
        };
        std::cout << "[safeDiv] 7/2 = " << safeDiv(7, 2) << '\n';   // 3.5
        std::cout << "[safeDiv] 1/0 = " << safeDiv(1, 0) << '\n';   // 0
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：應該預設用值還是參考捕獲？
    //    A：lambda「會被存起來、跨 thread、跨 scope」 → 一律 by value；
    //       lambda「立刻在當前 scope 用完」 → 用 by ref 較省。
    //
    //  Q2：怎麼快速分辨 lambda 是否安全？
    //    A：3 個檢查
    //         (a) 它會不會逃出當前 scope？（return 出去、塞容器、傳 thread）
    //         (b) 它捕獲了什麼 ref / this？
    //         (c) 那個被 ref 的東西能不能保證活得比 lambda 久？
    //       任何一條不確定，就把 ref 改成 by value 或包 shared_ptr。
    //
    //  Q3：lambda 內 throw 會發生什麼？
    //    A：跟一般函式一樣，例外會傳到呼叫點。Lambda 預設 noexcept(false)，
    //       要寫 noexcept 才會變成 noexcept；放到 std::vector 會影響
    //       move 策略。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 10_lambda_pitfalls.cpp -o 10_lambda_pitfalls

// === 預期輸出 ===
// [Demo1] 1 2 3
// [Demo2] half(7) = 3.5
// [LC202] isHappy(19) = true
// [LC202] isHappy(2)  = false
// [deferred-tasks] results: 0 1 4
// [safeDiv] 7/2 = 3.5
// [safeDiv] 1/0 = 0
