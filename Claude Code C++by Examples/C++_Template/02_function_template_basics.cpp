// ============================================================================
//  02_function_template_basics.cpp  ──  Function Template 基礎
// ============================================================================
//
//  【本篇目標】
//    把 function template 的「語法面」徹底講清楚。
//      - template<typename T> 與 template<class T> 的差別 (本質上沒差)
//      - 多個 template 參數
//      - Template argument deduction (引數型別推導)
//      - 顯式指定 vs 自動推導
//      - 不能推導的情況：回傳型別、未出現在引數的型別參數
//      - return type 用 auto / decltype(auto)
//
// ----------------------------------------------------------------------------
//  【語法骨架】
//
//        template <typename T>          // 宣告 1 個型別參數 T
//        T func(T a, T b) {             // 引數、回傳型別都可以用 T
//            ...
//        }
//
//    多個型別參數：
//
//        template <typename T, typename U>
//        auto add(T a, U b) { return a + b; }
//
// ----------------------------------------------------------------------------
//  【typename vs class】
//    寫 `template<typename T>` 或 `template<class T>` 在「宣告型別參數」
//    時完全等價，純粹是歷史原因。建議統一用 typename，比較不會誤導讀者
//    以為 T 一定要是 class。詳見 cppreference 條目：
//        https://en.cppreference.com/cpp/language/template_parameters
//
//    例外：在「template template parameter」(後面 file 10) 的位置，
//    早期 C++ (C++14 以前) 必須寫 class，C++17 起兩者都可以。
//
// ----------------------------------------------------------------------------
//  【Template Argument Deduction (引數推導)】
//
//        template<typename T> T id(T x) { return x; }
//
//        id(42);    // 推導 T = int
//        id(3.14);  // 推導 T = double
//        id<long>(42); // 顯式指定 T = long
//
//    推導大致規則 (簡化版)：
//      ◆ T 出現在引數型別 → 由實參型別推導
//      ◆ 引用 (T&、const T&、T&&) 會剝掉表面層次再推導
//      ◆ 陣列、函式類型會 decay 成 pointer (除非引數是 reference)
//      ◆ 只在「回傳型別」出現的 T 無法推導 → 必須顯式指定
//
//    遇到歧義時編譯器報錯，例如：
//
//        template<typename T> T mymax(T a, T b);
//        mymax(1, 2.0);    // ❌ 一個 int 一個 double，T 推不出
//
//    解法：
//      (a) 顯式指定：mymax<double>(1, 2.0);
//      (b) 用兩個型別參數 (見下面 add)。
//
// ----------------------------------------------------------------------------
//  【回傳型別寫法演進】
//
//      C++98：必須明寫
//          template<typename T, typename U>
//          ??? add(T a, U b) { return a + b; }   // 寫不出來
//
//      C++11：trailing return type
//          template<typename T, typename U>
//          auto add(T a, U b) -> decltype(a + b) { return a + b; }
//
//      C++14：直接 auto 推導
//          template<typename T, typename U>
//          auto add(T a, U b) { return a + b; }
//
//    建議：C++14 以後就直接用 auto 即可，最簡潔。
//
//  參考：https://en.cppreference.com/cpp/language/function_template
// ============================================================================

/*
補充筆記：function_template_basics
  - function_template_basics 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - function_template_basics 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Function Template 基礎與引數推導
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. mymax(1, 2.0) 為什麼編譯失敗？有哪些解法？
//     答：兩個引數對應同一個 T，int 與 double 推導結果衝突（deduction conflict）。
//         三種解法：顯式指定 mymax<double>(1, 2.0)；改用兩個型別參數
//         template<typename T, typename U>；或用 std::common_type_t<T,U> 統一。
//     追問：推導時 const、參考、陣列各會發生什麼事？
//
// 🔥 Q2. 回傳型別要依賴參數時怎麼寫？各是哪個標準？
//     答：C++11 用 trailing return type：auto add(T a, U b) -> decltype(a + b)，
//         因為寫在最前面時 a、b 還沒進入作用域。C++14 起可直接寫 auto add(T a, U b)
//         讓編譯器推導回傳型別 ── 這是 C++14 不是 C++11，是常被追打的分界。
//
// Q3. 為什麼 to<long>(42) 一定要顯式指定模板參數？
//     答：R 只出現在回傳型別、沒出現在任何函式引數，編譯器無從推導。慣例是把
//         「必須顯式指定的參數」排在最前面，後面的 T 仍由引數推導，呼叫端只要
//         寫 to<long>(42) 而不必把兩個型別都寫滿。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// ─── 1. 單型別參數：取絕對值 (示範用 std::abs 即可，這裡為練語法) ──────────
template <typename T>
T abs_t(T x) {
    return (x < 0) ? -x : x;
}

// ─── 2. 多型別參數：相加（回傳型別交給編譯器推導） ─────────────────────────
//   為什麼分兩個型別 T、U？
//     若硬寫 template<typename T> T add(T,T)，呼叫 add(1, 2.5) 會編譯錯誤
//     (ambiguous deduction)。分成兩個型別後，T = int、U = double，
//     回傳則靠 a + b 自動 promote 為 double。
template <typename T, typename U>
auto add(T a, U b) {
    return a + b;
}

// ─── 3. 回傳型別不能推導 → 顯式指定 ──────────────────────────────────────
//   題型：把任意型別的數值轉成 R 型別 (R 沒有出現在引數，必須顯式指定)。
//   呼叫範例：to<long>(42);
template <typename R, typename T>
R to(T v) {
    return static_cast<R>(v);
}

// ─── 4. Leetcode 1672 ── Richest Customer Wealth ─────────────────────────
//   題目：accounts[i][j] 表示第 i 個顧客在第 j 家銀行的存款。
//        回傳「最有錢顧客的總財產」。
//   範例：[[1,2,3],[3,2,1]] → 6
//
//   為什麼適合 template？
//     原題型別是 int，但這個邏輯適用任何可加的數值型別 (long long、double…)。
//
//   時間複雜度：O(m·n)
//   空間複雜度：O(1)
//   邊界條件：accounts 為空 → 回傳 T{} (即 0)。
//
//   實作要點：
//     - 用 std::vector<std::vector<T>> 接收
//     - max_wealth 初始化為 T{}（值初始化，int 為 0），避免讀到 garbage
template <typename T>
T maximum_wealth(const std::vector<std::vector<T>>& accounts) {
    T max_wealth = T{};
    for (const auto& row : accounts) {       // row 是 const std::vector<T>&
        T sum = T{};
        for (const T& v : row) sum += v;
        if (sum > max_wealth) max_wealth = sum;
    }
    return max_wealth;
}

// ─── 5. 工作實用範例：swap_if_greater ─────────────────────────────────────
//   常見場景：排序網路、保證一對值有固定大小關係 (a <= b)。
//   template 化後對任意可比較、可 swap 的型別都能用。
//
//   參考：std::swap 自 C++11 起為 noexcept 條件取決於 move 是否 noexcept。
template <typename T>
void swap_if_greater(T& a, T& b) {
    if (a > b) {
        T tmp = std::move(a);    // move 比 copy 快，對重物件更明顯
        a = std::move(b);
        b = std::move(tmp);
    }
}

// ─── 6. 工作實用範例：print_pair ──────────────────────────────────────────
//   日常 debug 印一組 (key, value) 用。
//   兩個型別參數讓 K 和 V 可以不同。
template <typename K, typename V>
void print_pair(const K& k, const V& v) {
    std::cout << "(" << k << ", " << v << ")\n";
}

// ─── 7. Leetcode 1295 ── Find Numbers with Even Number of Digits ─────────
//   難度: easy
//   題目：給 nums，回傳「位數為偶數」的元素個數。
//   範例：nums = [12,345,2,6,7896] → 2 (12 和 7896)
//
//   為什麼適合 template？
//     「算 x 有幾位數」這個邏輯對 int / long / long long 都通用。做成
//     template 後可避免為了不同型別寫多份。
//
//   時間：O(n · log10 max)；空間：O(1)。
template <typename T>
int find_numbers_even_digits(const std::vector<T>& nums) {
    int count = 0;
    for (T x : nums) {
        T v = x < 0 ? -x : x;          // 取絕對值
        int digits = 0;
        if (v == 0) digits = 1;
        while (v > 0) { ++digits; v /= 10; }
        if (digits % 2 == 0) ++count;
    }
    return count;
}

// ─── 8. 工作實用：percentage_of：算 part 占 whole 的百分比 ───────────────
//   每天工作可能會用到：進度條、容量百分比、命中率等。
//   T、U 不同型別 → 自動 promote (例如 int / size_t 的進度)。
template <typename T, typename U>
double percentage_of(T part, U whole) {
    if (whole == 0) return 0.0;
    return static_cast<double>(part) * 100.0 / static_cast<double>(whole);
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) abs_t
    std::cout << "abs_t(-7)        = " << abs_t(-7)      << "\n";
    std::cout << "abs_t(-3.14)     = " << abs_t(-3.14)   << "\n";

    // (2) add 跨型別
    std::cout << "add(1, 2.5)      = " << add(1, 2.5)    << "\n";   // double 4.5
    std::cout << "add(1, 2)        = " << add(1, 2)      << "\n";   // int 3
    std::cout << "add(std::string(\"foo\"), \"bar\") = "
              << add(std::string("foo"), "bar") << "\n"; // string "foobar"

    // (3) 顯式指定回傳型別
    auto x = to<long>(42);
    std::cout << "to<long>(42)     = " << x << " (sizeof=" << sizeof(x) << ")\n";

    // (4) Leetcode 1672
    std::vector<std::vector<int>> accounts{
        {1, 2, 3},
        {3, 2, 1},
    };
    std::cout << "maximum_wealth   = " << maximum_wealth(accounts) << "\n";

    // (5) swap_if_greater
    int p = 9, q = 4;
    swap_if_greater(p, q);
    std::cout << "after swap_if_greater: p=" << p << " q=" << q << "\n";

    // (6) print_pair
    print_pair("name", "Alice");
    print_pair(1, 100.5);

    // (7) Leetcode 1295
    std::vector<int> nums{12, 345, 2, 6, 7896};
    std::cout << "find_numbers_even_digits = " << find_numbers_even_digits(nums)
              << " (expect 2)\n";

    // (8) percentage_of
    std::cout << "percentage_of(35, 200) = " << percentage_of(35, 200) << " %\n";
    std::cout << "percentage_of(0, 0)    = " << percentage_of(0, 0) << " %\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：「implicit instantiation」與「explicit instantiation」差在哪？
    //    A：implicit = 你呼叫 max_t(3, 7)，編譯器自動推導 T 並產生對應版
    //       本，這是最常見的方式。explicit instantiation 寫法是
    //       `template int max_t<int>(int, int);`，強迫編譯器在這個翻譯單元
    //       立刻生成對應符號，常用於把 template 定義隱藏進 .cpp 並只匯
    //       出特定型別，減少 header 重編譯成本。
    //
    //  Q2：function template 跟普通 function overload 同名時，誰會被選中？
    //    A：overload resolution 規則：優先級為「非 template 的普通函式」
    //       > 「最特化的 template」 > 「一般 template」。例如同時存在
    //       `void f(int)` 與 `template<typename T> void f(T)`，呼叫 f(3)
    //       會選普通版本。這也是為什麼實務上「想為某型別客製行為」首選
    //       overload 而不是 template specialization。
    //
    //  Q3：為什麼 mymax(1, 2.0) 會編譯失敗？怎麼解？
    //    A：因為兩個引數對應同一個 T，但 int 與 double 推導不一致 (deduction
    //       conflict)。三種解法：(a) 顯式指定 mymax<double>(1, 2.0)；
    //       (b) 用兩個型別參數 template<typename T, typename U>；
    //       (c) 在引數加上 std::common_type_t<T,U> 或型別轉換包裝。
    //
    return 0;
}

// ============================================================================
//  【小結 & 容易踩的坑】
//    1. typename 與 class 在型別參數宣告時等價，建議統一 typename。
//    2. 推導失敗時最常見原因：兩個引數型別不同卻共用同一個 T。
//       解法：用兩個型別參數，或顯式指定。
//    3. 回傳型別不要怕用 auto；C++14 以後最乾淨。
//    4. Template 函式定義通常要放在 header (除非用 explicit instantiation)，
//       因為 instantiation 需要看到完整定義 ── 這點 file 25 會詳細解釋。
//
//  【下一篇】
//    03_class_template_basics.cpp ── 把同樣的概念延伸到 class。
// ============================================================================
