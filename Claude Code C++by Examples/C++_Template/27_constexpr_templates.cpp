// ============================================================================
//  27_constexpr_templates.cpp  ──  編譯期計算 (constexpr / consteval)
// ============================================================================
//
//  【本篇目標】
//    把「能在編譯期算的事」搬到編譯期，省下 runtime 成本，並把錯誤儘早抓。
//    本篇重點：
//      ① constexpr 函式的演進 (C++11 → C++14 → C++17 → C++20)
//      ② constexpr 變數 vs constexpr 函式
//      ③ consteval (C++20)：強制必須編譯期執行
//      ④ 編譯期容器 (constexpr vector / array)
//      ⑤ 編譯期遞迴 (Fibonacci) 與編譯期 hash
//
// ----------------------------------------------------------------------------
//  【constexpr 演進】
//
//    C++11 constexpr 函式幾乎只能寫 「return expr;」 一行。
//        constexpr int square(int x) { return x * x; }
//
//    C++14：函式內可以有 if / for / 區域變數，幾乎像普通函式。
//        constexpr int factorial(int n) {
//            int r = 1;
//            for (int i = 2; i <= n; ++i) r *= i;
//            return r;
//        }
//
//    C++17：lambda 可以是 constexpr。
//
//    C++20：
//      - constexpr 函式中可以使用 std::vector、std::string (動態配置在
//        constant evaluation 內合法，但結果不能逃出 constant evaluation)
//      - virtual / try-catch 也可以 constexpr
//      - 加入 consteval (immediate function)：必須編譯期執行，否則錯
//
//    注意：「constexpr」是「最多可以」編譯期執行的標記，並不強制。如果
//    參數是 runtime 值，constexpr 函式照樣 runtime 跑。consteval 才是強制。
//
// ----------------------------------------------------------------------------
//  【constexpr 變數】
//        constexpr int N = 10;        // 編譯期常數
//        constexpr int arr[N] = {};   // N 可以當陣列大小
//
//    constexpr 變數本身就是「const + 編譯期已知」。可以做 NTTP 引數。
//
// ----------------------------------------------------------------------------
//  【consteval (C++20) 範例】
//
//        consteval int magic() { return 42; }
//        constexpr int a = magic();    // OK
//        int b = magic();              // ❌ 編譯錯誤：不能 runtime 呼叫
//
//    用途：強制某段程式碼必須在編譯期執行 (例如 reflection / DSL)。
//
// ----------------------------------------------------------------------------
//  【實務應用】
//    - 編譯期查表：把 sin / log 等預先算好成 constexpr 陣列
//    - 編譯期 hash：字串 → ID 表，避免 runtime 字串比較
//    - 編譯期常數推導：固定大小 buffer 的 N 由其他 constexpr 計算得到
//
//  參考：https://en.cppreference.com/cpp/language/constexpr
// ============================================================================

/*
補充筆記：constexpr_templates
  - constexpr_templates 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - constexpr_templates 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

// ─── 1. C++14 多行 constexpr：階乘 ──────────────────────────────────────
constexpr long long factorial(int n) {
    long long r = 1;
    for (int i = 2; i <= n; ++i) r *= i;
    return r;
}

// ─── 2. Leetcode 70 ── Climbing Stairs (Fibonacci) ──────────────────────
//   題目：每次走 1 或 2 階，n 階梯有幾種走法？
//        f(n) = f(n-1) + f(n-2)，f(1)=1, f(2)=2
//   範例：n=2 → 2；n=3 → 3；n=4 → 5
//
//   解法：DP O(n) 時間、O(1) 空間。寫成 constexpr → n 為編譯期常數時編譯期算。
//
//   時間：O(n)；空間：O(1)。
constexpr long long climb_stairs(int n) {
    if (n <= 2) return n;
    long long a = 1, b = 2;
    for (int i = 3; i <= n; ++i) { long long c = a + b; a = b; b = c; }
    return b;
}

// ─── 3. Leetcode 509 ── Fibonacci Number ────────────────────────────────
//   題目：F(0)=0, F(1)=1, F(n)=F(n-1)+F(n-2)
//   範例：F(2)=1, F(3)=2, F(4)=3
//
//   時間：O(n)；空間：O(1)。
constexpr long long fib(int n) {
    if (n < 2) return n;
    long long a = 0, b = 1;
    for (int i = 2; i <= n; ++i) { long long c = a + b; a = b; b = c; }
    return b;
}

// ─── 4. 編譯期查表：sin 的近似值表 (示範用，精度低) ─────────────────────
//   想像嵌入式系統，runtime 算 sin 太貴，事先算好 360 個值。
//   用 constexpr 函式生成 std::array。
constexpr double poly_sin(double x) {
    // Taylor 5 階近似：x - x^3/6 + x^5/120
    double x2 = x * x;
    return x * (1 - x2 / 6.0 + x2 * x2 / 120.0);
}

constexpr std::array<double, 360> make_sin_table() {
    std::array<double, 360> a{};
    for (int i = 0; i < 360; ++i) {
        double rad = i * 3.14159265358979 / 180.0;
        a[i] = poly_sin(rad);
    }
    return a;
}

constexpr auto sin_table = make_sin_table();    // 整個表編譯期算好

// ─── 5. 工作實用：編譯期字串 hash (FNV-1a) ───────────────────────────────
//   把「字串」轉成「整數」做 switch / map key，比 runtime strcmp 快。
//   字串 literal 在 C++17 起可以是 std::string_view，constexpr 可用。
constexpr std::uint64_t fnv1a(std::string_view s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (char c : s) {
        h ^= static_cast<std::uint8_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

// 用例：把 command 字串編譯期 hash，做 switch
void dispatch(std::string_view cmd) {
    switch (fnv1a(cmd)) {
    case fnv1a("start"):  std::cout << "[start]\n";  break;
    case fnv1a("stop"):   std::cout << "[stop]\n";   break;
    case fnv1a("pause"):  std::cout << "[pause]\n";  break;
    default:              std::cout << "[unknown: " << cmd << "]\n"; break;
    }
}

// ─── 6. consteval (C++20)：強制編譯期 ───────────────────────────────────
#if __cplusplus >= 202002L
consteval int compile_time_only() { return 42; }
#endif

// ─── 7. Leetcode 326 ── Power of Three (constexpr 編譯期檢查) ───────────
//   難度: easy
//   題目：判斷 n 是否為 3 的次方。例如 27 → true、45 → false。
//
//   解法：除以 3 直到不能整除，看是否剩 1。O(log₃ n)。
//
//   為什麼放在這裡？
//     寫成 constexpr 函式 → n 為編譯期常數時可在 static_assert 驗證。
constexpr bool is_power_of_three(int n) {
    if (n < 1) return false;
    while (n % 3 == 0) n /= 3;
    return n == 1;
}

static_assert(is_power_of_three(27));
static_assert(!is_power_of_three(45));

// ─── 8. 工作實用：編譯期 CRC-like 校驗碼 (XOR 校驗) ─────────────────────
//   給每段「協議封包」算簡易 XOR checksum，常用於嵌入式 / 通訊。
//   constexpr 版本可在編譯期算好查表，runtime 零成本。
constexpr std::uint8_t xor_checksum(std::string_view s) {
    std::uint8_t c = 0;
    for (char ch : s) c ^= static_cast<std::uint8_t>(ch);
    return c;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    constexpr auto f10 = factorial(10);
    static_assert(f10 == 3628800);
    std::cout << "factorial(10)        = " << f10 << "\n";

    constexpr auto cs5 = climb_stairs(5);
    static_assert(cs5 == 8);
    std::cout << "climb_stairs(5)      = " << cs5 << "\n";

    constexpr auto f15 = fib(15);
    static_assert(f15 == 610);
    std::cout << "fib(15)              = " << f15 << "\n";

    std::cout << "sin_table[30] (~0.5) = " << sin_table[30]  << "\n";
    std::cout << "sin_table[90] (~1.0) = " << sin_table[90]  << "\n";

    dispatch("start");
    dispatch("pause");
    dispatch("eject");

    // (7) Leetcode 326 Power of Three
    std::cout << std::boolalpha;
    std::cout << "is_power_of_three(27)  = " << is_power_of_three(27)  << "\n";
    std::cout << "is_power_of_three(45)  = " << is_power_of_three(45)  << "\n";
    std::cout << "is_power_of_three(243) = " << is_power_of_three(243) << "\n";

    // (8) xor_checksum (編譯期常數)
    constexpr auto cs = xor_checksum("hello");
    std::cout << "xor_checksum(\"hello\") = " << static_cast<int>(cs) << "\n";

#if __cplusplus >= 202002L
    constexpr int v = compile_time_only();
    static_assert(v == 42);
    std::cout << "consteval compile_time_only() = " << v << "\n";
#endif

    return 0;
}

// ============================================================================
//  【小結】
//    1. constexpr 從 C++11 → C++20 越來越強大，現在幾乎任何純函式都可寫
//       constexpr。
//    2. 編譯期查表是最常見的工程應用：以空間換時間。
//    3. 編譯期字串 hash 讓你能用 switch + 字面字串，可讀性與效能兼具。
//    4. consteval 強制編譯期，用於必須避免 runtime 路徑的場景。
//
//  【下一篇】
//    28_capstone_mini_stl.cpp ── 綜合應用：自製迷你容器。
// ============================================================================
