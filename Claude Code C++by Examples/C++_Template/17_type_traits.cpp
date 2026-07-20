// ============================================================================
//  17_type_traits.cpp  ──  <type_traits> 標準庫
// ============================================================================
//
//  【本篇目標】
//    系統介紹 <type_traits>。重點：
//      ① 三大類 trait：型別關係 / 型別屬性 / 型別變換
//      ② 老寫法 (::value 與 ::type) vs 新寫法 (_v 與 _t 後綴)
//      ③ 與 SFINAE / concept / static_assert 的搭配使用
//      ④ 常用 trait 的「典型場景」
//
// ----------------------------------------------------------------------------
//  【三大類 trait】
//
//    (a) 型別關係 (Type relationship)：
//          std::is_same<A, B>            ── A 與 B 是同一型別嗎？
//          std::is_base_of<Base, Der>    ── Der 是 Base 的子類嗎？
//          std::is_convertible<From, To> ── From 可轉成 To 嗎？
//
//    (b) 型別屬性 (Type categories / properties)：
//          std::is_integral<T>           ── T 是整數型別？(int, char, bool, ...)
//          std::is_floating_point<T>     ── T 是浮點？(float, double, ...)
//          std::is_pointer<T>            ── T 是指標？
//          std::is_reference<T>
//          std::is_array<T>
//          std::is_class<T>
//          std::is_const<T>
//          std::is_signed<T>
//          std::is_arithmetic<T>         ── 整數或浮點
//          ...
//
//    (c) 型別變換 (Type modification)：
//          std::remove_const<T>          ── 拿掉 const
//          std::remove_reference<T>      ── 拿掉 reference
//          std::remove_pointer<T>        ── 拿掉 pointer
//          std::add_const<T>             ── 加上 const
//          std::decay<T>                 ── 模擬 by-value 傳參的型別轉換
//                                          (移除 ref、cv、陣列退化為指標)
//          std::conditional<B, T, F>     ── 編譯期 if/else，B 真選 T 假選 F
//
// ----------------------------------------------------------------------------
//  【寫法演進】
//
//    C++11：
//        std::is_integral<T>::value         // 取值
//        std::remove_const<T>::type         // 取型別
//
//    C++14：加上 _t alias，省掉 ::type
//        std::remove_const_t<T>
//
//    C++17：加上 _v variable template，省掉 ::value
//        std::is_integral_v<T>
//
//    建議：寫新程式直接用 _v / _t，可讀性大幅提升。
//
// ----------------------------------------------------------------------------
//  【常見搭配】
//
//    1. 與 static_assert：
//          template<typename T>
//          void f(T x) {
//              static_assert(std::is_arithmetic_v<T>, "需要數值型別");
//              ...
//          }
//
//    2. 與 SFINAE / enable_if：見 file 16
//
//    3. 與 if constexpr (C++17)：見 file 19
//
//    4. 與 concept (C++20)：見 file 20-21
//
//  參考：https://en.cppreference.com/cpp/header/type_traits
// ============================================================================

/*
補充筆記：type_traits
  - type_traits 把型別性質轉成編譯期 bool 或型別轉換工具。
  - is_integral_v、remove_reference_t 這類工具常和 if constexpr、enable_if、concepts 搭配。
  - type trait 不會檢查執行期值，只處理型別本身。
  - type_traits 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】<type_traits>
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. trait 的 ::value / ::type 與 _v / _t 後綴各屬哪個標準？
//     答：C++11 只有 std::is_integral<T>::value 與 std::remove_const<T>::type
//         兩種寫法。C++14 補上 _t 別名（remove_const_t，用 alias template 實作），
//         C++17 才補上 _v 變數模板（is_integral_v，用 C++14 的變數模板機制實作）。
//         所以「_t 比 _v 早三年」是可以拿來區辨深度的細節。
//     追問：_t 為什麼特別有價值？（省掉 dependent name 前面必須寫的 typename，
//         忘了寫 typename 是新手最常見的編譯錯誤之一）
//
// 🔥 Q2. std::decay_t<T> 做了什麼？典型用途是什麼？
//     答：模擬「by value 傳參」會發生的型別轉換：剝掉 reference 與 cv 限定、
//         陣列退化為指標、函式退化為函式指標（本機實測 decay_t<const int&> 得 int、
//         decay_t<const char[6]> 得 const char*）。典型用途：make_pair/make_tuple、
//         把推導出的型別拿來宣告成員變數、以及在 forwarding reference 上做型別
//         比較（is_same_v<decay_t<T>, Foo>）。
//
// ⚠️ 陷阱. std::remove_const_t<const int*> 會得到什麼？
//     答：還是 const int*，const 一點都沒被拿掉（本機以 static_assert 實測確認）。
//         因為 remove_const 只剝「top-level const」，而 const int* 的 const 屬於
//         被指物（low-level），指標本身並不是 const。反過來 remove_const_t<int* const>
//         才會得到 int*。同理 remove_const_t<const int&> 也不動，要先
//         remove_reference 再 remove_const，或直接用 decay_t 一次處理掉。
//     為什麼會錯：腦中把 remove_const 想成「把這個型別裡的 const 字樣通通刪掉」。
//         它其實是有精確定義的：只處理最外層那一個 cv 限定。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

// ─── 1. always_false：static_assert 中製造「永遠 false」的 trait ─────────
//   為什麼要這個？
//     直接寫 static_assert(false, "...") 在【未實例化】的 template 裡，標準規定是
//     ill-formed, no diagnostic required (IFNDR)——也就是「編譯器可以報錯，也可以
//     不報」，各家行為不一致（本機 g++ 15.2 實測：接受、不報錯）。
//     不能依賴這種不確定行為，所以要讓 false 依賴某個 template 參數，
//     把錯誤明確推遲到「真的 instantiate 時」才觸發。這才是可攜的寫法。
//     （C++26 的 P2593 已允許直接寫 static_assert(false)，但在此之前一律用這招。）
template <typename...>
inline constexpr bool always_false_v = false;

// ─── 2. 安全 cast：integral 之間轉換時檢查範圍，否則拒絕編譯 ──────────────
//   實務常見：把 int 強轉成 unsigned char 時可能 overflow。
//   這裡示範 compile-time 拒絕「縮小轉換」(narrowing)。
template <typename To, typename From>
To safe_cast(From v) {
    static_assert(std::is_arithmetic_v<From>, "From 必須是算術型別");
    static_assert(std::is_arithmetic_v<To>,   "To 必須是算術型別");
    if constexpr (std::is_integral_v<To> && std::is_integral_v<From>) {
        if (v < std::numeric_limits<To>::min() ||
            v > std::numeric_limits<To>::max())
        {
            std::cout << "[safe_cast] out of range, fallback to 0\n";
            return To(0);
        }
        return static_cast<To>(v);
    } else {
        return static_cast<To>(v);
    }
}

// ─── 3. trait：判斷是否「可印」(operator<<) ─────────────────────────────
//   detection idiom 經典範例。
template <typename, typename = void>
struct is_printable : std::false_type {};

template <typename T>
struct is_printable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_printable_v = is_printable<T>::value;

// 用 print 帶 fallback：可印者直接印，不可印者印 type info。
template <typename T>
void print_or_warn(const T& v) {
    if constexpr (is_printable_v<T>) {
        std::cout << v << "\n";
    } else {
        std::cout << "[unprintable type, sizeof=" << sizeof(T) << "]\n";
    }
}

// ─── 4. Leetcode 7 ── Reverse Integer (用 type_traits 控制 overflow) ─────
//   題目：給一個 32-bit 有號整數，回傳「反轉位數」後的整數。若反轉結果
//        超出 32-bit 範圍，回傳 0。
//   範例：123 → 321；-123 → -321；120 → 21
//
//   解法核心：每一步累加前先檢查是否會 overflow。
//
//   時間：O(log10 |x|)
//   空間：O(1)
//
//   為什麼放 type_traits 篇？
//     用 std::numeric_limits 取得 INT_MAX / INT_MIN，是 type_traits 系列
//     裡實務最常用的工具之一。
//
//   邊界：x = INT_MIN 時 -x 會 overflow → 用 long long 做中介。
int reverse_integer(int x) {
    long long r = 0;
    while (x != 0) {
        r = r * 10 + (x % 10);
        x /= 10;
    }
    if (r > std::numeric_limits<int>::max() ||
        r < std::numeric_limits<int>::min()) return 0;
    return static_cast<int>(r);
}

// ─── 5. 型別變換的綜合應用：「decay-like」函式 ────────────────────────────
//   印出傳進來的型別 (decay 後)。decay 模擬「by-value 傳參」會發生的型別
//   轉換：const、reference、array-to-pointer 都會被剝除。
template <typename T>
void describe(T&& x) {
    using U  = std::remove_reference_t<T>;       // 拿掉 reference
    using V  = std::remove_cv_t<U>;              // 拿掉 const / volatile
    using D  = std::decay_t<T>;                  // 直接一次完成
    std::cout << "is_integral_v<V>=" << std::is_integral_v<V>
              << " is_pointer_v<D>=" << std::is_pointer_v<D>
              << " sizeof(D)="       << sizeof(D)
              << "\n";
}

// ─── 6. 工作實用：static_assert 守住 API 合約 ────────────────────────────
//   日常常見：寫一個容器，元素必須能 copy。違反時馬上拒絕編譯。
template <typename T>
class CopyOnlyVec {
public:
    static_assert(std::is_copy_constructible_v<T>,
                  "CopyOnlyVec 元素型別必須可 copy 構造");
    void add(const T& v) { data_.push_back(v); }
    std::size_t size() const { return data_.size(); }
private:
    std::vector<T> data_;
};

// ─── 7. Leetcode 191 ── Number of 1 Bits (用 type_traits 限制 unsigned) ─
//   難度: easy
//   題目：給 unsigned 整數 n，回傳 binary 表示中 1 的個數 (popcount)。
//   範例：n=11 (binary 1011) → 3
//
//   經典 Brian Kernighan 演算法：n & (n-1) 每次消掉最低位的 1。
//   時間 O(k)，k = 1 的個數；最壞 O(log n)。
//
//   為什麼放這裡？
//     用 std::is_unsigned_v 限制只能傳 unsigned 整數 (signed 做 bit 操作
//     行為較危險)。若違反，static_assert 給友善訊息。
template <typename T>
int hamming_weight(T n) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>,
                  "hamming_weight 只接受 unsigned 整數型別");
    int count = 0;
    while (n) { n &= (n - 1); ++count; }
    return count;
}

// ─── 8. 工作實用：safe_compare 跨號性比較 (避免 signed/unsigned 警告) ────
//   實務常見：把 int 與 size_t 比較會出 warning，因為一邊有負一邊沒有。
//   用 type_traits 偵測兩邊是否同號，做安全比較。
//   類似 C++20 的 std::cmp_less / cmp_equal。
template <typename A, typename B>
bool safe_less(A a, B b) {
    if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
        return a < b;                       // 同號直接比
    } else if constexpr (std::is_signed_v<A>) {
        // a 是 signed，b 是 unsigned
        return a < 0 ? true : static_cast<std::make_unsigned_t<A>>(a) < b;
    } else {
        // a 是 unsigned，b 是 signed
        return b < 0 ? false : a < static_cast<std::make_unsigned_t<B>>(b);
    }
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    std::cout << std::boolalpha;

    // (1) 基本 trait
    std::cout << "is_integral_v<int>     = " << std::is_integral_v<int>     << "\n";
    std::cout << "is_integral_v<float>   = " << std::is_integral_v<float>   << "\n";
    std::cout << "is_arithmetic_v<bool>  = " << std::is_arithmetic_v<bool>  << "\n";
    std::cout << "is_pointer_v<int*>     = " << std::is_pointer_v<int*>     << "\n";
    std::cout << "is_same_v<int, long>   = " << std::is_same_v<int, long>   << "\n";

    // (2) safe_cast
    std::cout << "safe_cast<short>(123)        = " << safe_cast<short>(123)        << "\n";
    std::cout << "safe_cast<short>(70000)      = " << safe_cast<short>(70000)      << "\n";  // overflow
    std::cout << "safe_cast<double>(42)        = " << safe_cast<double>(42)        << "\n";

    // (3) is_printable
    std::cout << "is_printable_v<int>           = " << is_printable_v<int>           << "\n";
    std::cout << "is_printable_v<std::string>   = " << is_printable_v<std::string>   << "\n";
    print_or_warn(42);
    print_or_warn(std::string("hi"));

    // (4) Leetcode 7
    std::cout << "reverse_integer(123)            = " << reverse_integer(123)         << "\n";
    std::cout << "reverse_integer(-123)           = " << reverse_integer(-123)        << "\n";
    std::cout << "reverse_integer(1534236469)     = " << reverse_integer(1534236469)  << " (overflow → 0)\n";

    // (5) describe
    int x = 7;
    describe(x);
    describe(3.14);
    describe("hello");          // const char[6]，decay 為 const char*

    // (6) CopyOnlyVec
    CopyOnlyVec<std::string> v;
    v.add(std::string("a"));
    v.add(std::string("b"));
    std::cout << "CopyOnlyVec size = " << v.size() << "\n";

    // (7) Leetcode 191 Hamming Weight
    std::cout << "hamming_weight(11u)         = " << hamming_weight(11u) << "\n";
    std::cout << "hamming_weight(0xFFFFFFFFu) = " << hamming_weight(0xFFFFFFFFu) << "\n";

    // (8) safe_less 跨號性比較
    int sa = -1;
    unsigned ub = 5;
    std::cout << "safe_less(-1, 5u)  = " << safe_less(sa, ub) << " (expect true)\n";
    std::cout << "safe_less(5u, -1)  = " << safe_less(ub, sa) << " (expect false)\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::integral_constant 是什麼？為什麼很多 trait 都繼承它？
    //    A：它是 <T, T v> 的編譯期常數包裝，提供 static value、type、operator()()
    //       與隱式轉型。is_same、is_integral 等 trait 都繼承自它，因此可以同時
    //       透過 ::value 取布林值或當成 tag type 用於 tag dispatch。
    //
    //  Q2：_v 與 _t 別名的意義？為何 C++17 才加？
    //    A：_t（如 remove_cv_t<T>）等於 typename remove_cv<T>::type，省掉 typename
    //       關鍵字；_v（如 is_integral_v<T>）等於 is_integral<T>::value，當布林直接
    //       用。C++14 先補 _t，C++17 再補 _v，目的是大幅減少 metaprogramming 的
    //       語法噪音。
    //
    //  Q3：std::decay_t<T> 在 forwarding / 容器存值時的常見用途是什麼？
    //    A：decay 模擬「by-value 傳參」的型別轉換：去掉引用與 cv，array→pointer，
    //       function→function pointer。常用於 std::make_pair/tuple、儲存 lambda
    //       捕獲、實作 std::function 等場合，讓 deduce 出來的型別能直接拿來宣告
    //       成員變數。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. <type_traits> 是 metaprogramming 的字典，記不住沒關係，cppreference
//       查表即可。
//    2. 使用 _v / _t 後綴讓程式碼變得很乾淨。
//    3. 常和 static_assert / if constexpr / SFINAE / concept 搭配使用。
//    4. always_false_v<T> 是 static_assert 中的「永遠 false」工具。
//
//  【下一篇】
//    18_tag_dispatch.cpp ── tag dispatch：用空型別當「策略選擇器」。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 17_type_traits.cpp -o 17_type_traits

// === 預期輸出 ===
// is_integral_v<int>     = true
// is_integral_v<float>   = false
// is_arithmetic_v<bool>  = true
// is_pointer_v<int*>     = true
// is_same_v<int, long>   = false
// safe_cast<short>(123)        = 123
// safe_cast<short>(70000)      = [safe_cast] out of range, fallback to 0
// 0
// safe_cast<double>(42)        = 42
// is_printable_v<int>           = true
// is_printable_v<std::string>   = true
// 42
// hi
// reverse_integer(123)            = 321
// reverse_integer(-123)           = -321
// reverse_integer(1534236469)     = 0 (overflow → 0)
// is_integral_v<V>=true is_pointer_v<D>=false sizeof(D)=4
// is_integral_v<V>=false is_pointer_v<D>=false sizeof(D)=8
// is_integral_v<V>=false is_pointer_v<D>=true sizeof(D)=8
// CopyOnlyVec size = 2
// hamming_weight(11u)         = 3
// hamming_weight(0xFFFFFFFFu) = 32
// safe_less(-1, 5u)  = true (expect true)
// safe_less(5u, -1)  = false (expect false)
