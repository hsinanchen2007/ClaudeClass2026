// =============================================================================
//  03_constexpr_relaxed.cpp  —  C++14 放寬 constexpr 函式限制
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/constexpr
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ C++11 vs C++14 的 constexpr 差異                           │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 規定 constexpr 函式 body 必須是「single return statement」(再加
//  static_assert 等少數例外)。要算 factorial 只能寫遞迴 + 三元：
//
//      constexpr int fact(int n) {
//          return n <= 1 ? 1 : n * fact(n - 1);   // C++11 式
//      }
//
//  C++14 大幅放寬：constexpr 函式可以有
//   * 區域變數
//   * if、switch、for、while 等控制流
//   * 多條 statement、for 迴圈、變數修改
//
//  所以可以寫成「正常迴圈」：
//
//      constexpr int fact(int n) {
//          int r = 1;
//          for (int i = 2; i <= n; ++i) r *= i;
//          return r;
//      }
//
//  唯一限制：函式體內不能有 try-block / new-delete / goto / 未初始化變
//  數 / 非常規的東西（具體 list 隨標準演進變化）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼重要？                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 的限制讓「複雜編譯期計算」極不直觀；C++14 起「正常程式碼」就能
//  在編譯期跑 — constexpr 真正可用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：迴圈版 factorial
//   * Demo 2：constexpr 內判斷 + 修改區域變數
//   * Demo 3：編譯期迴圈 — Fibonacci 迭代版
//
//   附註：原本想做「sieve of Eratosthenes」，但 std::array::operator[]
//   要 C++17 才是 constexpr — 所以本檔在 C++14 用 fibonacci 迭代代替。
// =============================================================================

/*
補充筆記：constexpr_relaxed
  - constexpr_relaxed 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - C++14 放寬 constexpr 函式限制，允許區域變數、迴圈和多個 return。
  - 即使 constexpr 函式可寫得像一般函式，仍要避免依賴執行期才有的狀態。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++14 放寬的 constexpr（最能區分「背過」與「真懂」的題）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++14 的 constexpr 相對 C++11 放寬了什麼？
//     答：C++11 的 constexpr 函式本體實質只能是「單一 return 陳述式」，複雜計算
//         只能靠三元運算子與遞迴；C++14 放寬為可宣告區域變數、可用 if/switch、
//         可用 for/while 迴圈、可修改（生命週期起於該次常量求值內的）物件。
//         本檔的迴圈版 factorial 正是 C++14 才合法，C++11 下 GCC 會直接報
//         「body of constexpr function not a return-statement」。
//     追問：後續版本再放寬什麼？（C++17 加 if constexpr 與 constexpr lambda；
//           C++20 加動態配置、try/catch、virtual 呼叫，並引入 consteval/constinit）
//
// 🔥 Q2. 還有一項「不在函式本體」的改動最常被漏答，是什麼？
//     答：非靜態成員函式標 constexpr 時，C++11 會「隱式加上 const」，C++14 起不再。
//         同一段 struct S { int v; constexpr int get() { return v; } };
//         在 C++11 下 get() 是 const 成員（可對 const S 物件呼叫），
//         在 C++14 下不是——對 const 物件呼叫會因為 discards qualifiers 而編譯錯。
//     追問：對舊碼移植的影響？（C++11 寫的 constexpr getter 升到 C++14 後可能突然
//           不能對 const 物件呼叫，必須自己補上 const）
//
// Q3. C++14 的 constexpr 函式「仍然」不能做什麼？
//     答：區域變數不可以不初始化；不可以是 static 或 thread_local（constexpr 函式
//         內的 static 區域變數要到 C++23 才開放）；型別必須是 literal type。
//         另外一個常被問的分界：C++11 不允許 constexpr 函式回傳 void，C++14 允許。
//
// Q4. constexpr 函式與 constexpr 變數的差別？
//     答：constexpr 變數「保證」在編譯期求值；constexpr 函式只是「有資格」。
//         同一個 factorial，寫成 constexpr long long f = factorial(10); 是編譯期算完；
//         若把執行期才知道的變數當引數，它就退化成普通函式在執行期跑。
//         也就是說 constexpr 函式必須「編譯期與執行期兩種模式都成立」。
//
// ⚠️ 陷阱. 函式標了 constexpr，是不是就代表它一定在編譯期算完？
//     答：不是。constexpr 是「允許」而非「保證」。要強制編譯期求值，必須把結果放進
//         真正需要 constant expression 的位置——constexpr 變數、static_assert、
//         陣列長度、模板引數。本檔用 static_assert 驗證，就是在強制它編譯期成立。
//     為什麼會錯：多數人把 constexpr 直接讀成「這個函式會在編譯期執行」，於是以為
//         加上關鍵字就自動得到最佳化。真正「必須在編譯期求值」的關鍵字是 C++20 的
//         consteval，C++14 的 constexpr 給的只是資格。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

// 迴圈版 factorial — C++11 的 constexpr 寫不出
constexpr long long factorial(int n) {
    long long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

// 條件 + 區域變數
constexpr int sumDigits(int n) {
    int s = 0;
    while (n > 0) {
        s += n % 10;
        n /= 10;
    }
    return s;
}

// 迭代版 Fibonacci — 比 C++11 版的 fib(n-1) + fib(n-2) 快得多 (O(n) vs O(2^n))
constexpr long long fibIter(int n) {
    long long a = 0, b = 1;
    for (int i = 0; i < n; ++i) {
        long long t = a + b;
        a = b;
        b = t;
    }
    return a;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：編譯期跑迴圈
    // ─────────────────────────────────────────────────────────
    constexpr long long f10 = factorial(10);
    constexpr long long f20 = factorial(20);
    static_assert(f10 == 3628800, "");
    static_assert(f20 == 2432902008176640000LL, "");
    std::cout << "[Demo1] factorial(10)=" << f10
              << " factorial(20)=" << f20 << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：sumDigits 編譯期
    // ─────────────────────────────────────────────────────────
    constexpr int s = sumDigits(12345);
    static_assert(s == 15, "");
    std::cout << "[Demo2] sumDigits(12345) = " << s << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：編譯期 Fibonacci（迭代版 — 用 C++14 放寬版才寫得出）
    // ─────────────────────────────────────────────────────────
    constexpr long long f50 = fibIter(50);
    static_assert(f50 == 12586269025LL, "");
    std::cout << "[Demo3] fibIter(50) = " << f50 << "  (compile-time)\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：編譯期算這麼多東西不會拖慢編譯嗎？
    //    A：會。constexpr 編譯期計算「會吃編譯時間」。一般「常數查表」夠
    //       小不影響；幾千次以上的迴圈會明顯慢化編譯。要平衡。
    //
    //  Q2：constexpr 函式可以呼叫非 constexpr 函式嗎？
    //    A：在「編譯期 evaluate」時不行（會編譯錯）；在「runtime 呼叫」時
    //       可以。換言之 constexpr 是「兩用」的承諾 — 兩種模式都該編得過。
    //
    //  Q3：C++17 / C++20 的 constexpr 又再放寬什麼？
    //    A：C++17：lambda 可以是 constexpr。
    //       C++20：constexpr 內可 throw、可動態配置（new/delete）、可 vector
    //       和 string 在編譯期；此外引入 consteval / constinit。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 70. Climbing Stairs（C++14 relaxed 版 — 迴圈寫法）
    //   題意：每次走 1 或 2 階，求走 n 階共有幾種走法。
    //   為什麼放這？C++11 只能用遞迴 (O(2^n))，C++14 放寬可寫迴圈 (O(n))。
    // ─────────────────────────────────────────────────────────
    // helper 用「迭代版 fibonacci」 — climbStairs(n) = fib(n+1)
    // 注意：fibIter 已宣告在檔案上方
    constexpr long long s10 = fibIter(11);    // climbStairs(10)
    constexpr long long s30 = fibIter(31);    // climbStairs(30)
    static_assert(s10 == 89, "");
    static_assert(s30 == 1346269LL, "");
    std::cout << "[LC70] climbStairs(10) = " << s10 << '\n';
    std::cout << "[LC70] climbStairs(30) = " << s30 << " (compile-time)\n";

    // ─────────────────────────────────────────────────────────
    // 實用範例：編譯期 GCD — 工作上常見的數學工具函式
    //   C++11 只能寫 single return；C++14 可以 while 直接表達 Euclid 演算法
    // ─────────────────────────────────────────────────────────
    auto gcd_runtime = [](int a, int b) {
        while (b != 0) { int t = a % b; a = b; b = t; }
        return a;
    };
    std::cout << "[Demo4] gcd(48, 18) = " << gcd_runtime(48, 18) << '\n';
    // 也可以宣告為 constexpr 函式（檔案 scope）— 因 C++14 放寬，迴圈合法

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 03_constexpr_relaxed.cpp -o 03_constexpr_relaxed

// === 預期輸出 ===
// [Demo1] factorial(10)=3628800 factorial(20)=2432902008176640000
// [Demo2] sumDigits(12345) = 15
// [Demo3] fibIter(50) = 12586269025  (compile-time)
// [LC70] climbStairs(10) = 89
// [LC70] climbStairs(30) = 1346269 (compile-time)
// [Demo4] gcd(48, 18) = 6
