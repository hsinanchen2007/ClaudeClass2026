// =============================================================================
//  09_constexpr.cpp  —  constexpr (C++11) — 編譯期常數運算
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/constexpr
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、constexpr 是什麼？                                     │
//  └────────────────────────────────────────────────────────────┘
//
//  「在編譯期就能算出值」 — 適用於變數與函式：
//
//      constexpr int square(int x) { return x * x; }
//      constexpr int n = square(5);   // n 是編譯期常數 25
//
//  常見場景：
//   * 取代 #define 巨集（型別安全、有 scope）
//   * 取代 enum-as-int（更清楚）
//   * template 的非型別參數需要編譯期常數
//   * 編譯期決定 array 大小、static_assert 條件
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、C++11 的 constexpr 函式限制                            │
//  └────────────────────────────────────────────────────────────┘
//
//  C++11 對 constexpr 函式有嚴格限制：「函式體必須是 single return」。
//
//      constexpr int factorial(int n) {
//          return n <= 1 ? 1 : n * factorial(n - 1);   // 三元運算子
//      }
//
//  禁止：迴圈、區域變數、if 陳述句、多條 statement。要做複雜運算只能用
//  「遞迴 + 三元運算子」。
//
//  C++14 大幅放寬 — 見 C++_Cpp14/03_constexpr_relaxed.cpp。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、constexpr vs const                                     │
//  └────────────────────────────────────────────────────────────┘
//
//   * const：「執行期不變」 — 可以是 runtime 計算後不變
//   * constexpr：「編譯期就算出來」 — 比 const 強的承諾
//
//      int x = 5;
//      const int a = x;           // OK，runtime 取值
//      constexpr int b = x;       // ❌ x 不是編譯期常數
//      constexpr int c = 5;       // ✅
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、constexpr ctor — 製作編譯期常數物件                    │
//  └────────────────────────────────────────────────────────────┘
//
//      class Vec3 {
//          double x, y, z;
//      public:
//          constexpr Vec3(double x_, double y_, double z_)
//              : x(x_), y(y_), z(z_) {}
//      };
//      constexpr Vec3 origin{0, 0, 0};   // 編譯期物件
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：constexpr 函式 + static_assert 編譯期驗證
//   * Demo 2：constexpr 物件 + array 大小
//   * LeetCode 509. Fibonacci Number — 編譯期算 Fibonacci
// =============================================================================

/*
補充筆記：constexpr
  - constexpr 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - constexpr 函式可在編譯期求值，但也能在執行期被呼叫；它不是只能用於常數。
  - constexpr 變數必須能在編譯期初始化，適合取代許多魔術常數。
*/
#include <iostream>

constexpr int square(int x) { return x * x; }

constexpr long long factorial(int n) {
    return n <= 1 ? 1LL : n * factorial(n - 1);
}

// LeetCode 509 風格：fibonacci
constexpr long long fib(int n) {
    return n <= 1 ? n : fib(n - 1) + fib(n - 2);
}

class Vec3 {
public:
    double x, y, z;
    constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    constexpr double normSq() const { return x*x + y*y + z*z; }
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：編譯期算
    // ─────────────────────────────────────────────────────────
    constexpr int s = square(7);
    constexpr long long f10 = factorial(10);
    static_assert(s == 49, "square wrong");
    static_assert(f10 == 3628800, "fact wrong");
    std::cout << "[Demo1] square(7)=" << s << " factorial(10)=" << f10 << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：constexpr 物件 + array 大小
    // ─────────────────────────────────────────────────────────
    constexpr Vec3 v{3.0, 4.0, 0.0};
    constexpr double n2 = v.normSq();
    static_assert(n2 == 25.0, "");
    std::cout << "[Demo2] v.normSq() = " << n2 << '\n';

    constexpr int N = square(4);
    int arr[N]{};                          // OK：N 是 constexpr int = 16
    std::cout << "[Demo2] arr size = " << sizeof(arr)/sizeof(arr[0]) << '\n';

    // ─────────────────────────────────────────────────────────
    // LeetCode 509. Fibonacci Number — 編譯期算
    //   注意：這個版本是「教學示意」，遞迴複雜度 O(2^n)；n 大會編譯時間爆炸。
    //   實務上要 constexpr fib 大 n 用 C++14 放寬版（見 Cpp14/03）寫迴圈。
    // ─────────────────────────────────────────────────────────
    constexpr long long fib10 = fib(10);
    static_assert(fib10 == 55, "");
    std::cout << "[LC509] fib(10) at compile time = " << fib10 << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：constexpr 函式可以在 runtime 呼叫嗎？
    //    A：可以！constexpr 函式同時可以「編譯期 + 執行期」呼叫；只有
    //       「呼叫處需要編譯期常數」時才強制編譯期算。例如：
    //         int r = square(rand());     // runtime 呼叫，仍合法
    //
    //  Q2：constexpr int x = 5; 跟 inline constexpr int x = 5; 差在哪？
    //    A：C++17 起變數可以加 inline → 多個 TU 可以共用同一份；C++11/14
    //       只能透過 static 或放在 anonymous namespace 避免重複定義。
    //
    //  Q3：throw 在 constexpr 函式中行不行？
    //    A：C++11 不行（constexpr 函式必須能編譯期算）；C++14 起有條件允
    //       許「runtime 路徑」throw，但「編譯期 evaluate」時 throw 仍會
    //       編譯失敗。
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 70. Climbing Stairs
    //   題意：每次走 1 或 2 階，求走 n 階共有幾種走法（= Fibonacci(n+1)）。
    //   為什麼放這？C++11 constexpr 限制 single return，用遞迴三元表達。
    //   實際 helper 已宣告在檔案上方（fib）— climbStairs(n) = fib(n + 1)。
    // ─────────────────────────────────────────────────────────
    constexpr long long stairs5 = fib(5 + 1);   // 8
    constexpr long long stairs10 = fib(10 + 1); // 89
    static_assert(stairs5  == 8,  "");
    static_assert(stairs10 == 89, "");
    std::cout << "[LC70] climbStairs(5)  = " << stairs5  << '\n';
    std::cout << "[LC70] climbStairs(10) = " << stairs10 << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：編譯期單位換算 — 把魔術常數用 constexpr 函式取代
    //   工作上常見：「1 KB = ？bytes」等常數，用 constexpr 函式表達意圖
    // ─────────────────────────────────────────────────────────
    // 注意 C++11 的 constexpr 函式只能 single return，所以用三元 / 直接表達式
    constexpr long long KB = 1024;
    constexpr long long MB = 1024 * KB;
    constexpr long long GB = 1024 * MB;
    static_assert(GB == 1073741824LL, "1 GB == 2^30");
    std::cout << "[Demo4] 1 GB = " << GB << " bytes (compile-time)\n";

    return 0;
}
