// ============================================================================
//  12_variable_template.cpp  ──  Variable Template (C++14)
// ============================================================================
//
//  【本篇目標】
//    Variable template = 把「變數」(其實多半是常數) 也做成 template。C++14
//    引入。重點：
//      - 語法 template<typename T> constexpr T pi = T(3.14159265358979);
//      - 與 alias template、function template 各自的角色
//      - 標準庫 <type_traits> 從 C++17 起大量加上 _v 後綴，本質就是
//        variable template
//      - 實務：型別專屬常數、參數化常數
//
// ----------------------------------------------------------------------------
//  【動機 ── 沒有 variable template 怎麼寫常數？】
//    你想要一個 pi 常數，且希望可以是 float / double / long double。
//
//    C++03 寫法：
//        const double pi = 3.14159265358979;        // 只能一個型別
//        // 或寫成 function：
//        template<typename T> T pi() { return T(3.14159265358979); }
//        // 呼叫時 pi<float>()，多一個 () 看起來怪
//
//    C++14 起：
//        template<typename T>
//        constexpr T pi = T(3.14159265358979);
//
//        // 用法簡潔自然：
//        float  a = pi<float>;
//        double b = pi<double>;
//
// ----------------------------------------------------------------------------
//  【標準庫的 _v 後綴 (C++17)】
//    這些其實就是 variable template：
//        std::is_integral<T>::value     // 老寫法
//        std::is_integral_v<T>          // C++17 起
//        std::is_same<A,B>::value
//        std::is_same_v<A,B>
//
//    定義就一行：
//        template<typename T>
//        inline constexpr bool is_integral_v = is_integral<T>::value;
//
//    實務上 _v 比 ::value 短很多，metaprogramming 程式碼會清爽很多。
//
// ----------------------------------------------------------------------------
//  【可以是 const 也可以是 inline constexpr】
//    通常寫 constexpr：表示編譯期已決定值，可以放在 NTTP / 陣列大小等位置。
//    從 C++17 起 constexpr 變數隱含 inline，不會違反 ODR；C++14 則建議
//    顯式加 inline 在 header 用。
//
//  參考：https://en.cppreference.com/cpp/language/variable_template
// ============================================================================

/*
補充筆記：variable_template
  - variable_template 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - variable_template 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <iostream>
#include <type_traits>
#include <vector>

// ─── 1. 經典：型別專屬的 pi ──────────────────────────────────────────────
template <typename T>
constexpr T pi = T(3.14159265358979323846L);

// ─── 2. 自定 _v 風格：simplify is_integral / is_same ─────────────────────
template <typename T>
constexpr bool is_integral_v = std::is_integral<T>::value;

template <typename A, typename B>
constexpr bool is_same_v = std::is_same<A, B>::value;

// ─── 3. 參數化常數：bit-mask 表 ─────────────────────────────────────────
//   情境：與硬體 / 通訊協定打交道時常見「第 N 個 bit 為 1 的 mask」。
//   傳統寫法：const uint32_t bit0 = 1u, bit1 = 2u, bit2 = 4u, ...，又長又醜。
//   variable template 一行解決：
template <unsigned N>
constexpr unsigned long long bit = 1ULL << N;

// ─── 4. 數學常數系列：tau (= 2π) / 黃金比例 ──────────────────────────────
template <typename T>
constexpr T tau = T(2) * pi<T>;

template <typename T>
constexpr T golden = T(1.6180339887498948482L);

// ─── 5. 工作實用：型別大小常數表 ─────────────────────────────────────────
//   在某些 protocol parser / serializer 裡，常需要查 「T 佔幾個 byte」。
//   sizeof(T) 已經做到了，但用 variable template 包一層可加註解或限制：
template <typename T>
constexpr std::size_t bytes_of = sizeof(T);

// ─── 6. SFINAE 用例：用 variable template 結合 enable_if (預告 file 16) ───
//   把通用 sum 函式限制只接受 integral 型別。先看用法不必懂語法細節。
template <typename T,
          typename = typename std::enable_if<is_integral_v<T>>::type>
T sum_int(const std::vector<T>& v) {
    T s{};
    for (const T& x : v) s += x;
    return s;
}

// ─── 7. Leetcode 50 ── Pow(x, n) (用 variable template 表達單位元) ──────
//   難度: medium
//   題目：實作 pow(x, n) — 即 x 的 n 次方。
//   範例：pow(2.0, 10) = 1024；pow(2.0, -2) = 0.25
//
//   經典「快速冪」O(log |n|)：把 n 用二進位拆解，每次平方 x 並依 bit 累乘。
//
//   為什麼放這裡？
//     用我們前面定義的 variable template (T 專屬常數) 表達「乘法單位元 1」，
//     讓 pow_t 對 int、double、long 都通用。
template <typename T>
constexpr T one_v = T(1);            // 乘法單位元

template <typename T>
T pow_t(T x, long long n) {
    if (n < 0) { x = one_v<T> / x; n = -n; }
    T result = one_v<T>;
    T base   = x;
    while (n > 0) {
        if (n & 1) result *= base;
        base *= base;
        n >>= 1;
    }
    return result;
}

// ─── 8. 工作實用：型別專屬「epsilon」(浮點誤差容忍值) ───────────────────
//   數值比較常需要 abs(a - b) < epsilon 而非直接 ==。
//   不同精度的 epsilon 不一樣，用 variable template 一次性定義。
template <typename T>
constexpr T epsilon_v = T(1e-9);

template <>
constexpr float epsilon_v<float> = 1e-5f;            // float 精度較低

template <typename T>
bool nearly_equal(T a, T b) {
    T diff = a > b ? a - b : b - a;
    return diff < epsilon_v<T>;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    std::cout.precision(20);
    std::cout << "pi<float>       = " << pi<float>       << "\n";
    std::cout << "pi<double>      = " << pi<double>      << "\n";
    std::cout << "pi<long double> = " << pi<long double> << "\n";

    std::cout << "tau<double>     = " << tau<double>     << "\n";
    std::cout << "golden<double>  = " << golden<double>  << "\n";

    std::cout << "bit<0> = " << bit<0> << ", bit<3> = " << bit<3>
              << ", bit<10> = " << bit<10> << "\n";

    std::cout << std::boolalpha;
    std::cout << "is_integral_v<int>          = " << is_integral_v<int>          << "\n";
    std::cout << "is_integral_v<double>       = " << is_integral_v<double>       << "\n";
    std::cout << "is_same_v<int,int>          = " << is_same_v<int,int>          << "\n";
    std::cout << "is_same_v<int,long>         = " << is_same_v<int,long>         << "\n";

    std::cout << "bytes_of<char>   = " << bytes_of<char>   << "\n";
    std::cout << "bytes_of<int>    = " << bytes_of<int>    << "\n";
    std::cout << "bytes_of<double> = " << bytes_of<double> << "\n";

    std::cout << "sum_int = " << sum_int(std::vector<int>{1, 2, 3, 4, 5}) << "\n";

    // (7) Leetcode 50 Pow(x, n)
    std::cout << "pow_t(2.0, 10)  = " << pow_t(2.0, 10) << "\n";
    std::cout << "pow_t(2.0, -2)  = " << pow_t(2.0, -2) << "\n";
    std::cout << "pow_t(3LL, 5)   = " << pow_t<long long>(3, 5) << "\n";

    // (8) epsilon_v + nearly_equal
    std::cout << "nearly_equal<double>(0.1+0.2, 0.3) = "
              << nearly_equal<double>(0.1 + 0.2, 0.3) << "\n";
    std::cout << "epsilon_v<float>  = " << epsilon_v<float>  << "\n";
    std::cout << "epsilon_v<double> = " << epsilon_v<double> << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：variable template 跟「constexpr 函式」、「static const 成員」
    //      的差別在哪？怎麼選？
    //    A：(a) `template<typename T> constexpr T pi = ...` 用法最直接，
    //       寫 `pi<float>` 就能拿值，沒 `()`；(b) constexpr function
    //       `pi<T>()` 多一層括號、但允許更複雜的計算邏輯；(c) class 的
    //       static const 成員需要先包進 struct，呼叫端寫 `Pi<float>::value`
    //       囉嗦。簡單常數選 variable template，複雜邏輯選 constexpr 函
    //       式，需要綁定到型別 trait 結構時選 static const 成員。
    //
    //  Q2：為什麼標準庫 C++17 開始大量加 `_v` 後綴？
    //    A：因為 `std::is_integral<T>::value` 既要打 `::value` 又是
    //       dependent name，在 template 內呼叫時還可能被當成歧義。
    //       `std::is_integral_v<T>` 用 variable template 包成
    //       `inline constexpr bool is_integral_v = is_integral<T>::value;`
    //       讓 metaprogramming 程式碼從 `if constexpr (std::is_integral<T>::value)`
    //       縮短成 `if constexpr (std::is_integral_v<T>)`，可讀性與打字
    //       速度都大幅提升。
    //
    //  Q3：在 header 寫 variable template 要不要加 inline？會不會違反 ODR？
    //    A：C++17 起 constexpr 變數 (含 variable template 的特化) 隱含
    //       inline，多個 .cpp 各自實例化同一個 `pi<double>` 不會撞符號。
    //       C++14 必須顯式寫 `template<typename T> inline constexpr T pi
    //       = ...;` 才安全。建議無腦加 constexpr (能編就加)，C++17 以後
    //       不必顯式 inline；C++14 跨翻譯單元共用時自己加 inline 保險。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. Variable template 把「值」也參數化，用法跟普通變數一樣，但每個
//       型別參數產出獨立的存儲。
//    2. 常見於：(a) 型別專屬常數 (b) trait 的 _v 後綴 (c) bit / index 表。
//    3. 通常加上 constexpr (C++17 起隱含 inline)。
//
//  【下一篇】
//    13_variadic_templates.cpp ── 進階篇開始：可變參數模板。
// ============================================================================
