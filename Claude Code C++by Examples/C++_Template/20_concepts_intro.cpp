// ============================================================================
//  20_concepts_intro.cpp  ──  Concepts 入門 (C++20)
// ============================================================================
//
//  【本篇目標】
//    Concepts 是 C++20 的旗艦功能：把「對 template 參數的限制」變成「具名
//    的型別需求」，比 SFINAE / enable_if 易讀百倍，錯誤訊息也清楚很多。
//
//  【動機 ── SFINAE 錯誤訊息的痛】
//    傳錯型別給 SFINAE 函式，編譯器會吐出十幾行模板實例化堆疊，人類根本
//    讀不下。Concept 直接告訴你「T 不滿足 Numeric 要求」。
//
//  【語法】
//
//        // 定義 concept (像在定義「型別需求集合」)：
//        template <typename T>
//        concept Numeric = std::is_arithmetic_v<T>;
//
//        // 用法 1：在 template 參數位置取代 typename
//        template <Numeric T>
//        T add(T a, T b) { return a + b; }
//
//        // 用法 2：requires 子句 (放在 template 參數列後)
//        template <typename T>
//            requires Numeric<T>
//        T add(T a, T b) { return a + b; }
//
//        // 用法 3：trailing requires (函式簽名後)
//        template <typename T>
//        T add(T a, T b) requires Numeric<T> { return a + b; }
//
//        // 用法 4：縮寫語法 (auto 隱藏 template)
//        Numeric auto add(Numeric auto a, Numeric auto b) { return a + b; }
//
//  【requires expression】
//    可以驗證「某段語法在 T 上是合法的」：
//
//        template<typename T>
//        concept Addable = requires(T a, T b) {
//            a + b;                  // 必須能 a+b
//            { a + b } -> std::same_as<T>;  // 而且結果型別必須是 T
//        };
//
//    更詳細的 requires expression 在下一篇 (file 21) 講。
//
//  【標準庫 concept】
//    <concepts> 標頭檔含大量預定義 concept：
//        std::integral, std::floating_point, std::signed_integral
//        std::same_as<T,U>, std::convertible_to<F,T>, std::derived_from<D,B>
//        std::equality_comparable, std::totally_ordered
//        std::regular, std::movable, std::copyable
//
//    <ranges> / <iterator> 內還有：
//        std::ranges::range, std::input_iterator, std::random_access_iterator
//
//    建議：寫新程式優先用標準庫 concept，意義清楚、文件齊全。
//
//  【編譯與支援】
//    - GCC 10+、Clang 12+、MSVC 2019 16.8+ 起支援。
//    - 必須開 -std=c++20 或更高。
//
//  參考：
//    https://en.cppreference.com/cpp/language/constraints
//    https://en.cppreference.com/cpp/concepts
// ============================================================================

#if __cplusplus < 202002L
#error "本檔需要 C++20 或更高，請用 g++ -std=c++20 編譯"
#endif

/*
補充筆記：concepts_intro
  - concept 把模板需求寫成可讀的約束，錯誤訊息通常比 SFINAE 清楚。
  - requires clause 不是執行期 if，而是限制 overload 是否參與編譯期選擇。
  - 好的 concept 應描述語意需求，不只是檢查某個語法剛好存在。
  - concepts_intro 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// ─── 1. 自定 concept：Numeric ───────────────────────────────────────────
template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

// 用 concept 限制 add：
template <Numeric T>
T add(T a, T b) { return a + b; }

// ─── 2. requires expression：Hashable ───────────────────────────────────
//   要求 T 能被 std::hash 取 hash 值。
template <typename T>
concept Hashable = requires(T x) {
    { std::hash<T>{}(x) } -> std::convertible_to<std::size_t>;
};

template <Hashable T>
std::size_t hash_of(const T& v) { return std::hash<T>{}(v); }

// ─── 3. Printable：能 std::cout << t ─────────────────────────────────────
template <typename T>
concept Printable = requires(std::ostream& os, T x) {
    { os << x } -> std::convertible_to<std::ostream&>;
};

template <Printable T>
void println(const T& v) { std::cout << v << "\n"; }

// ─── 4. Leetcode 1480 ── Running Sum (用 Numeric concept 限制) ───────────
//   題目見 file 01。這裡用 Numeric 限制 T，確保元素型別「真的可加」。
//
//   差別：
//     - file 01 沒有限制 → 傳了非數值型別 (例如 vector<vector<int>>) 會
//       在 += 那行報錯，錯誤訊息很長。
//     - 這裡有 concept → 編譯器在實例化時直接告訴你 「T 不滿足 Numeric」，
//       簡潔直白。
//
//   時間：O(n)；空間：O(n)。
template <Numeric T>
std::vector<T> running_sum(const std::vector<T>& nums) {
    std::vector<T> r;
    r.reserve(nums.size());
    T acc{};
    for (const T& x : nums) { acc += x; r.push_back(acc); }
    return r;
}

// ─── 5. 用標準庫 concept 寫 max ────────────────────────────────────────
//   std::totally_ordered 表示「型別完全可比較」(<, <=, >, >=, ==, != 都行)。
template <std::totally_ordered T>
T max_concept(T a, T b) { return a > b ? a : b; }

// ─── 6. 縮寫語法 (Abbreviated Function Templates) ───────────────────────
//   寫起來像普通函式，但每個 auto 都暗中是個 template 參數，被 concept 限制。
auto sum_two(Numeric auto a, Numeric auto b) {
    return a + b;
}

// ─── 7. 工作實用：限定容器元素型別 ───────────────────────────────────────
//   要求 T 是「整數型容器元素」，否則拒絕 instantiate。
//   std::ranges::range 表示「可以對它做 begin()/end()」。
template <std::ranges::range R>
    requires std::integral<std::ranges::range_value_t<R>>
auto sum_range(const R& r) {
    using T = std::ranges::range_value_t<R>;
    T s{};
    for (const auto& x : r) s += x;
    return s;
}

// ─── 8. Leetcode 1929 ── Concatenation of Array (用 concept 限制 range) ──
//   難度: easy
//   題目：給陣列 nums，回傳 nums + nums (長度 2n)。
//   範例：[1,2,1] → [1,2,1,1,2,1]
//
//   為什麼放在這裡？
//     用 std::ranges::range 限制可走訪、std::copyable 限制元素可複製，
//     錯誤訊息比 SFINAE 清楚。
template <std::ranges::range R>
    requires std::copyable<std::ranges::range_value_t<R>>
auto get_concatenation(const R& nums) {
    using T = std::ranges::range_value_t<R>;
    std::vector<T> result;
    result.reserve(2 * std::ranges::size(nums));
    for (const auto& x : nums) result.push_back(x);
    for (const auto& x : nums) result.push_back(x);
    return result;
}

// ─── 9. 工作實用：限定可 streamable 的「印出帶 prefix」 ─────────────────
template <Printable T>
void log_prefix(const std::string& prefix, const T& v) {
    std::cout << "[" << prefix << "] " << v << "\n";
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    std::cout << "add(3, 4)              = " << add(3, 4)             << "\n";
    std::cout << "add(1.5, 2.0)          = " << add(1.5, 2.0)         << "\n";
    // add(std::string("a"), std::string("b"));   // 編譯錯誤：Numeric 不滿足

    println(42);
    println(std::string("hello"));
    // println(std::vector<int>{1,2,3});           // 編譯錯誤：Printable 不滿足

    std::cout << "hash_of(\"foo\") = "
              << hash_of(std::string("foo")) << "\n";

    // running_sum
    auto rs = running_sum(std::vector<int>{1, 2, 3, 4});
    for (int x : rs) std::cout << x << ' ';
    std::cout << "\n";

    // max
    std::cout << "max_concept(7, 3) = " << max_concept(7, 3) << "\n";

    // sum_two 縮寫語法
    std::cout << "sum_two(1, 2.5)   = " << sum_two(1, 2.5) << "\n";

    // sum_range (要 integral 容器)
    std::cout << "sum_range vector<int> = " << sum_range(std::vector<int>{1,2,3,4,5}) << "\n";
    // sum_range(std::vector<double>{1.0});     // 編譯錯誤：T 不是 integral

    // Leetcode 1929 Concatenation of Array
    auto cc = get_concatenation(std::vector<int>{1, 2, 1});
    std::cout << "concat: ";
    for (int x : cc) std::cout << x << ' ';
    std::cout << "\n";

    // log_prefix
    log_prefix("INFO", 42);
    log_prefix("WARN", std::string("disk almost full"));

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：寫 concept 約束有幾種語法？
    //    A：四種等價寫法：
    //       (1) template<typename T> requires C<T> 在 template 後面接 requires；
    //       (2) template<C T>          直接以 concept 名稱當 type 參數約束；
    //       (3) auto f(C auto x)       縮寫函式語法；
    //       (4) template<typename T> void f(T) requires C<T> 放在函式列尾。
    //
    //  Q2：concepts 跟 SFINAE 的錯誤訊息差在哪？
    //    A：concept 失敗時編譯器會直接印「不滿足 C<T>」並列出哪一條子需求未達；
    //       SFINAE 失敗則往往輸出整段 substitution failure 的展開，要從訊息中
    //       自行推敲哪個 trait 不成立。對使用者體驗差很多。
    //
    //  Q3：concept 是否會影響函式的 overload resolution？
    //    A：會。受約束程度更高（subsume）的候選會勝出；無約束 < 有約束 < 約束
    //       更強。這讓我們可以寫多個同名函式，依概念強弱自然分流，不必再用
    //       enable_if 互斥條件強行排他。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. Concept 把「對 T 的要求」變成具名 + 可組合 + 易讀。
//    2. 寫法多種：template<Concept T>、requires 子句、縮寫 auto、trailing。
//    3. 標準庫已有大量 concept，先用現成的，再考慮自製。
//    4. C++20 起的程式碼建議用 concept，可讀性與錯誤訊息都遠勝 SFINAE。
//
//  【下一篇】
//    21_concepts_advanced.cpp ── requires expression 進階 + 組合 concept。
// ============================================================================
