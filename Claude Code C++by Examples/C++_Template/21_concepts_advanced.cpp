// ============================================================================
//  21_concepts_advanced.cpp  ──  Concepts 進階：requires-expression 與組合
// ============================================================================
//
//  【本篇目標】
//    在 file 20 的基礎上深入：
//      ① requires-expression 四種子句 (simple、type、compound、nested)
//      ② concept 之間的「組合」(&& || !) 與「subsumption」
//      ③ 多參數 concept
//      ④ 善用標準庫 concept 寫實用工具
//      ⑤ 與 SFINAE 寫法的對照
//
// ----------------------------------------------------------------------------
//  【requires-expression 的 4 種子句】
//
//        template<typename T>
//        concept Foo = requires(T a, T b) {
//
//            // (1) Simple requirement：只要這個表達式合法即可，不要求型別
//            a + b;
//            a.size();
//
//            // (2) Type requirement：要求某個型別存在
//            typename T::value_type;
//
//            // (3) Compound requirement：表達式合法 + 結果型別需滿足某 concept
//            { a == b }      -> std::convertible_to<bool>;
//            { a.size() }    -> std::same_as<std::size_t>;
//            { a.clone() }   -> std::same_as<T>;
//
//            // (4) Nested requirement：用 requires + bool 表達式做進一步條件
//            requires sizeof(T) <= 16;
//            requires std::integral<typename T::value_type>;
//        };
//
// ----------------------------------------------------------------------------
//  【組合 concept】
//
//        template<typename T> concept A = ...;
//        template<typename T> concept B = ...;
//
//        template<typename T> concept AB  = A<T> && B<T>;       // 同時滿足
//        template<typename T> concept AorB = A<T> || B<T>;      // 任一滿足
//        template<typename T> concept NotA = !A<T>;             // 否定
//
//    Subsumption (包含關係)：若 AB 比 A 嚴格 (multi-AND)，編譯器會優先選 AB
//    版本的 overload。這是 concept 比 SFINAE 強大的地方 ── overload 自然
//    成為「最具體者勝」。
//
// ----------------------------------------------------------------------------
//  【多參數 concept】
//
//        template<typename From, typename To>
//        concept Convertible = requires(From f) {
//            { static_cast<To>(f) };
//        };
//
//    或直接用標準庫 std::convertible_to<From, To>。
//
// ----------------------------------------------------------------------------
//  【實務優先順序】
//
//    1. 先看 std::concepts / std::ranges 有沒有現成 concept。
//    2. 沒有就用 std 概念的「組合」(&&、||)。
//    3. 再不行才寫 requires-expression。
//
//  參考：
//    https://en.cppreference.com/cpp/concepts
//    https://en.cppreference.com/cpp/language/requires
// ============================================================================

#if __cplusplus < 202002L
#error "本檔需要 C++20 或更高，請用 g++ -std=c++20 編譯"
#endif

/*
補充筆記：concepts_advanced
  - concepts_advanced 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - concepts_advanced 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <algorithm>
#include <concepts>
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

// ─── 1. Hashable + EqualityComparable 組合 ───────────────────────────────
template <typename T>
concept Hashable = requires(T x) {
    { std::hash<T>{}(x) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept HashableEq = Hashable<T> && std::equality_comparable<T>;

// ─── 2. Leetcode 217 ── Contains Duplicate ───────────────────────────────
//   題目：給整數陣列，判斷是否有任何元素出現超過一次。
//   範例：[1,2,3,1] → true；[1,2,3,4] → false
//
//   經典 hash 解法 O(n)：邊掃邊塞 set，遇到已存在就 true。
//
//   為什麼用 concept？
//     原題 vector<int>。我們做成 template，但不限定 int ── 只要 T 是
//     Hashable + EqualityComparable 即可。如此 std::string、自訂 class
//     都能直接用。
//
//   時間：O(n) 平均
//   空間：O(n)
template <HashableEq T>
bool contains_duplicate(const std::vector<T>& v) {
    std::unordered_set<T> seen;
    for (const T& x : v) {
        if (!seen.insert(x).second) return true;
    }
    return false;
}

// ─── 3. requires-expression：「有 push_back」的容器 ──────────────────────
//   detection：T 必須有 push_back(T::value_type) 與 size() 成員。
template <typename T>
concept BackPushable = requires(T c, typename T::value_type v) {
    typename T::value_type;            // (2) type requirement
    c.push_back(v);                    // (1) simple requirement
    { c.size() } -> std::convertible_to<std::size_t>;   // (3) compound
};

template <BackPushable C>
void fill_zero(C& c, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) c.push_back({});
}

// ─── 4. nested requires：限定 sizeof ────────────────────────────────────
//   小型物件版本：sizeof(T) <= 16，可以「by value」傳；大型則改 const T&。
template <typename T>
concept SmallTrivial = std::is_trivially_copyable_v<T> && requires {
    requires sizeof(T) <= 16;          // (4) nested requirement
};

template <SmallTrivial T>
T identity_small(T v) { return v; }    // 小物件 by value

// ─── 5. concept overloading：subsumption demo ────────────────────────────
//   兩個 overload 同名，concept 不同。傳入 T 時編譯器會選「最具體」的版本。
template <std::integral T>
void print_kind(T) { std::cout << "[integral] "; }

template <std::floating_point T>
void print_kind(T) { std::cout << "[floating] "; }

template <typename T>
void print_kind(T) { std::cout << "[other] "; }    // 無 concept = 最寬鬆

// ─── 6. 多參數 concept：Convertible-style helper ────────────────────────
template <typename From, typename To>
concept ConvertibleTo = std::convertible_to<From, To>;

template <typename To, typename From>
    requires ConvertibleTo<From, To>
To convert(From f) { return static_cast<To>(f); }

// ─── 7. 工作實用：限定範圍 + 元素型別都要對 ─────────────────────────────
//   想對「字串容器」一律 to_upper：要求 (a) 是 range (b) 元素是 std::string。
template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, std::string>
void to_upper_all(R& r) {
    for (auto& s : r) {
        std::ranges::transform(s, s.begin(),
            [](unsigned char c) { return std::toupper(c); });
    }
}

// ─── 8. Leetcode 1346 ── Check If N and Its Double Exist ─────────────────
//   難度: easy
//   題目：給整數陣列 arr，判斷是否存在 i != j 使 arr[i] == 2 * arr[j]。
//   範例：[10,2,5,3] → true (10 == 2*5)
//
//   解法：邊掃邊用 set 紀錄看過的數；遇到 x 就檢查 2x 或 x/2 (若 x 偶) 是否已存在。
//
//   為什麼放在這裡？
//     用 std::integral concept 限制只允許整數型別。subsume 讓 signed_integral
//     有更精準的版本可以選 (但本範例只一個版本，保持簡潔)。
//
//   時間：O(n)；空間：O(n)。
template <std::integral T>
bool check_double_exist(const std::vector<T>& arr) {
    std::unordered_set<T> seen;
    for (T x : arr) {
        if (seen.count(static_cast<T>(2 * x))) return true;
        if (x % 2 == 0 && seen.count(static_cast<T>(x / 2))) return true;
        seen.insert(x);
    }
    return false;
}

// ─── 9. 工作實用：BackPushable 容器 + 元素必須 streamable (組合 concept) ─
//   要把容器內容印出時保證雙重需求都滿足。
template <typename T>
concept Streamable = requires(std::ostream& os, T x) {
    { os << x } -> std::same_as<std::ostream&>;
};

template <BackPushable C>
    requires Streamable<typename C::value_type>
void dump_container(const C& c) {
    std::cout << "[";
    bool first = true;
    for (const auto& x : c) {
        if (!first) std::cout << ", ";
        std::cout << x;
        first = false;
    }
    std::cout << "] (size=" << c.size() << ")\n";
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    std::cout << std::boolalpha;

    // (1)(2) contains_duplicate
    std::cout << "contains_duplicate({1,2,3,1}) = "
              << contains_duplicate(std::vector<int>{1,2,3,1}) << "\n";
    std::cout << "contains_duplicate({a,b,c})   = "
              << contains_duplicate(std::vector<std::string>{"a","b","c"}) << "\n";

    // (3) BackPushable
    std::vector<int> v;
    fill_zero(v, 5);
    std::cout << "fill_zero produced size = " << v.size() << "\n";

    // (4) SmallTrivial
    std::cout << "identity_small(7) = " << identity_small(7) << "\n";

    // (5) print_kind subsumption
    print_kind(42);    std::cout << "\n";
    print_kind(3.14);  std::cout << "\n";
    print_kind("hi");  std::cout << "\n";

    // (6) convert
    std::cout << "convert<long>(42) = " << convert<long>(42) << "\n";

    // (7) to_upper_all
    std::vector<std::string> words{"hello", "world"};
    to_upper_all(words);
    for (const auto& s : words) std::cout << s << ' ';
    std::cout << "\n";

    // (8) Leetcode 1346
    std::cout << "check_double_exist({10,2,5,3}) = "
              << check_double_exist(std::vector<int>{10, 2, 5, 3}) << "\n";
    std::cout << "check_double_exist({3,1,7,11}) = "
              << check_double_exist(std::vector<int>{3, 1, 7, 11}) << "\n";

    // (9) dump_container
    dump_container(std::vector<int>{1, 2, 3, 4});

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：requires expression 與 requires clause 兩個 requires 怎麼區分？
    //    A：requires clause 是「條件」，寫在 template 後或函式宣告，回傳 bool；
    //       requires expression 是 requires (T x) { x + x; } 這類「需求列表」，
    //       本身才回傳 bool，常被當作 requires clause 的條件。兩個 requires 連
    //       寫的 requires requires 就是「clause 加上一個 inline expression」。
    //
    //  Q2：concept subsume（包含關係）有什麼用？
    //    A：當 concept A 的需求是 concept B 的超集，編譯器知道 A 比 B 嚴格。多個
    //       overload 同時匹配時，最嚴格的會被選中，不會 ambiguous。這讓「整數
    //       版」與「signed 整數版」可以共存且自動分流。注意：subsume 看的是
    //       「原子限制（atomic constraint）」是否相同，&& 拆開後比較。
    //
    //  Q3：requires expression 裡的「compound requirement」可以寫什麼？
    //    A：{ expr } noexcept -> Convertible 三件事：(1) expr 合法；(2) 不會丟
    //       exception；(3) 結果型別滿足某個 concept（用 -> 接）。常用來確認
    //       某個函式回傳值能否轉成特定型別，比 SFINAE 寫起來簡潔很多。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. requires-expression 4 種子句：simple / type / compound / nested。
//    2. Concept 用 && / || / ! 組合。Subsumption 讓最具體者勝出。
//    3. 多參數 concept 寫關係 (Convertible / Same / DerivedFrom)。
//    4. 寫新程式碼時，concept 已經夠強大，可大幅取代 SFINAE。
//
//  【下一篇】
//    22_crtp.cpp ── CRTP，靜態多型 / mixin 設計模式。
// ============================================================================
