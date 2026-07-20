// ============================================================================
//  16_sfinae.cpp  ──  SFINAE (Substitution Failure Is Not An Error)
// ============================================================================
//
//  【本篇目標】
//    SFINAE 是 C++ template metaprogramming 的核心工具之一。本篇徹底拆解：
//      ① 「Substitution Failure Is Not An Error」是什麼意思？
//      ② std::enable_if 怎麼運作？
//      ③ SFINAE 的常見三種寫法 (return type / 預設模板參數 / 函式參數)
//      ④ 為什麼 C++20 起有 concept 後，SFINAE 還是要懂？
//      ⑤ 實戰：依型別屬性走不同實作。
//
// ----------------------------------------------------------------------------
//  【什麼是 SFINAE】
//    當編譯器在 template argument deduction / substitution 過程中，把推導
//    出來的型別代入「函式宣告」時若發生失敗，C++ 規則是：
//        ▶「靜悄悄地把這個 candidate 從 overload set 中拿掉」
//        ▶「不算編譯錯誤」
//    →「Substitution Failure Is Not An Error」 ── SFINAE。
//
//    舉例：
//        template<typename T>
//        typename T::iterator foo(T t);     // 候選 1
//        void                  foo(int t);  // 候選 2
//
//        foo(42);    // 對候選 1 嘗試代入 T=int → int::iterator 不存在 → 失敗
//                    // 但這個失敗「不報錯」，編譯器靜默地把候選 1 拿掉
//                    // 然後選候選 2 → 編譯成功
//
//    這就是 SFINAE 的魔力：用「型別不匹配」做為 overload 過濾條件。
//
// ----------------------------------------------------------------------------
//  【std::enable_if】
//
//    定義 (簡化版)：
//        template<bool B, typename T = void>
//        struct enable_if {};                    // 沒有 ::type
//
//        template<typename T>
//        struct enable_if<true, T> { using type = T; };  // 有 ::type
//
//    用法：當 B 為 false → enable_if::type 不存在 → 觸發 SFINAE → candidate 被剔除。
//    C++14 起有 alias enable_if_t；C++17 起有 _v 變體。
//
// ----------------------------------------------------------------------------
//  【三種寫法】
//
//    ① 在「回傳型別」加 enable_if：
//        template<typename T>
//        std::enable_if_t<std::is_integral_v<T>, T>
//        only_int(T x) { return x * 2; }
//
//    ② 在「額外的預設模板參數」位置：
//        template<typename T,
//                 typename = std::enable_if_t<std::is_integral_v<T>>>
//        T only_int(T x) { return x * 2; }
//
//    ③ 在「函式參數預設值」位置 (較不常見)：
//        template<typename T>
//        T only_int(T x, std::enable_if_t<std::is_integral_v<T>>* = nullptr) { ... }
//
//    三種等價，個人偏好。我推薦 ②，視覺上最不打擾本體。
//
// ----------------------------------------------------------------------------
//  【SFINAE 常見模式】
//    - 兩條 overload，一條「整數版」、一條「浮點版」、一條「預設版」。
//    - 「擁有特定 member」者 vs 「沒有」者，各自走不同路徑。
//    - 與 C++20 concept 的關係：concept 是 SFINAE 的優雅替代，但仍以
//      SFINAE 為底層機制，且現存程式碼用 SFINAE 的太多，必須會讀。
//
//  參考：
//    https://en.cppreference.com/cpp/types/enable_if
//    https://en.cppreference.com/cpp/language/sfinae
// ============================================================================

/*
補充筆記：sfinae
  - SFINAE 的意思是 substitution failure is not an error：替換失敗會讓該 overload 從候選中消失。
  - enable_if 常用來依型別特性開關函式，但錯誤訊息通常比 concepts 難讀。
  - detection idiom 可測試某個表達式是否存在，例如 T 是否有 size()。
  - sfinae 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】SFINAE / enable_if
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. SFINAE 是什麼？
//     答：Substitution Failure Is Not An Error。重載決議時把推導出的模板實參
//         代入函式「宣告」，若產生無效的型別或表達式，編譯器不報錯，只是靜靜
//         把該候選從 overload set 剔除，繼續試其他候選；全部候選都失敗才報錯。
//     追問：現代的替代方案？（C++17 if constexpr 處理函式內部分支、C++20
//         concepts/requires 處理「該不該進入這個重載」，錯誤訊息好非常多）
//
// 🔥 Q2. std::enable_if 怎麼運作？有哪幾種放法？各屬哪個標準？
//     答：enable_if<B,T> 只有在 B 為 true 時才有 ::type，否則沒有 → 觸發 SFINAE。
//         三個位置：① 回傳型別（本檔用法）② 額外的預設模板參數（最推薦，
//         不污染函式簽章）③ 函式參數。標準版本要分清楚：std::enable_if 是 C++11、
//         enable_if_t 別名是 C++14、is_integral_v 這種 _v 是 C++17。
//     追問：建構子為什麼不能用「回傳型別」那一版？（建構子沒有回傳型別，
//         只能用第 ② 種）
//
// Q3. std::void_t 是哪個標準？detection idiom 怎麼運作？
//     答：C++17（但在 C++11 只要一行就能自製：template<class...> using void_t = void;）。
//         原理是把任意型別序列映射到 void，配合「偏特化 + SFINAE」：若
//         T::size() 存在，偏特化的 void_t<decltype(...)> 替換成功 → 選中
//         true_type；不存在則替換失敗 → 退回主模板 false_type。本檔的
//         has_size 就是這個模式。
//
// ⚠️ 陷阱. SFINAE 為什麼救不了「函式本體裡的錯誤」？
//     答：替換只發生在 immediate context——模板參數列、回傳型別、函式參數型別。
//         本機實測：auto f(T t) -> decltype(t.foo()) 在沒有 foo() 時會被靜默剔除、
//         正確 fallback 到別的候選；但 void g(T t){ t.foo(); } 在沒有 foo() 時是
//         hard error（error: has no member named 'foo'），直接編譯失敗，
//         fallback 完全救不到。
//     為什麼會錯：把 SFINAE 想成「編譯器會試著編譯整個函式，編不過就換一個」。
//         實際上它只試「宣告」；宣告一旦通過就定案，本體裡的錯誤已經是真錯誤。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <sstream>
#include <type_traits>
#include <vector>
#include <string>

// ─── 1. enable_if：分流「整數 vs 浮點」 ──────────────────────────────────
template <typename T>
std::enable_if_t<std::is_integral_v<T>, T>
double_it(T x) {
    std::cout << "[integral] ";
    return x * 2;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T>
double_it(T x) {
    std::cout << "[floating] ";
    return x * T(2);
}

// ─── 2. 「有沒有 size() 成員？」──────────────────────────────────────────
//   這是經典 detection idiom：用 SFINAE 試探某個表達式是否合法。
//   下面 has_size_v 就是「探針」。
//
//   用法：has_size_v<T> 為 true → T 有 size() 可呼叫。
template <typename, typename = void>
struct has_size : std::false_type {};

template <typename T>
struct has_size<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T>
constexpr bool has_size_v = has_size<T>::value;

// ─── 3. 用 has_size 寫一個通用 print：容器版印出 size 與內容 ─────────────
template <typename T>
std::enable_if_t<has_size_v<T>>
print_anything(const T& v) {
    std::cout << "[container size=" << v.size() << "] ";
    for (const auto& x : v) std::cout << x << ' ';
    std::cout << "\n";
}

template <typename T>
std::enable_if_t<!has_size_v<T>>
print_anything(const T& v) {
    std::cout << "[scalar] " << v << "\n";
}

// ─── 4. Leetcode 88 ── Merge Sorted Array (示範 SFINAE 分流) ─────────────
//   題目：兩個排序好的陣列 nums1、nums2 (其中 nums1 末段有空間放 nums2)，
//        把 nums2 合併進 nums1，使結果排序。
//   範例：nums1=[1,2,3,0,0,0], m=3, nums2=[2,5,6], n=3 → [1,2,2,3,5,6]
//
//   經典解法：從尾巴往前填，避免覆寫到還沒處理的元素。O(m+n)。
//
//   這題本來跟 SFINAE 沒關係，但我們以它為素材：包成 template 後，依元素
//   是否為「整數」分兩個版本：
//     - integral：可以用 += 累加、可以做 hash
//     - 其他    ：只走最基本比較邏輯
//
//   實際工作上：不會這樣硬要分。但這個 demo 讓你理解 SFINAE 在「依型別屬性
//   選擇實作」這件事上的威力。
template <typename T>
std::enable_if_t<std::is_integral_v<T>>
merge_sorted(std::vector<T>& a, int m, const std::vector<T>& b, int n) {
    std::cout << "[integral merge] ";
    int i = m - 1, j = n - 1, k = m + n - 1;
    while (j >= 0) {
        if (i >= 0 && a[i] > b[j]) a[k--] = a[i--];
        else                       a[k--] = b[j--];
    }
}

template <typename T>
std::enable_if_t<!std::is_integral_v<T>>
merge_sorted(std::vector<T>& a, int m, const std::vector<T>& b, int n) {
    std::cout << "[generic merge] ";
    int i = m - 1, j = n - 1, k = m + n - 1;
    while (j >= 0) {
        if (i >= 0 && a[i] > b[j]) a[k--] = a[i--];
        else                       a[k--] = b[j--];
    }
}

// ─── 5. 工作實用：safe_divide：integral 用 整數除、floating 用浮點除 ─────
//   實務常見：避免 0 除錯誤、避免兩個 int 相除被誤截斷。
template <typename T>
std::enable_if_t<std::is_integral_v<T>, T>
safe_divide(T a, T b) {
    if (b == 0) { std::cout << "[safe_divide:int] 0!\n"; return 0; }
    return a / b;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T>
safe_divide(T a, T b) {
    if (b == T(0)) { std::cout << "[safe_divide:fp] NaN!\n"; return T(0); }
    return a / b;
}

// ─── 6. Leetcode 268 ── Missing Number (限定 integral 的 XOR 解法) ──────
//   難度: easy
//   題目：給 [0..n] 中缺少一個的陣列，找出缺少哪個。
//   範例：[3,0,1] → 2；[9,6,4,2,3,5,7,0,1] → 8
//
//   解法：XOR 全部數字與 [0..n] → 缺少的會剩下來。O(n) 時間、O(1) 空間。
//
//   為什麼放在這裡？
//     用 SFINAE 限制 T 必須是 integral (XOR 對浮點無意義)。
template <typename T>
std::enable_if_t<std::is_integral_v<T>, T>
missing_number(const std::vector<T>& nums) {
    T result = static_cast<T>(nums.size());
    for (std::size_t i = 0; i < nums.size(); ++i) {
        result ^= static_cast<T>(i) ^ nums[i];
    }
    return result;
}

// ─── 7. 工作實用：SFINAE 風格 to_string ──────────────────────────────────
//   依「型別有沒有 to_string 可呼叫」分流：
//     - integral / floating → std::to_string
//     - 其他 → 直接用 ostringstream
template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, std::string>
to_str_safe(const T& v) { return std::to_string(v); }

template <typename T>
std::enable_if_t<!std::is_arithmetic_v<T>, std::string>
to_str_safe(const T& v) {
    std::ostringstream os; os << v; return os.str();
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) double_it 分流
    std::cout << "double_it(7)        = " << double_it(7)    << "\n";
    std::cout << "double_it(3.14)     = " << double_it(3.14) << "\n";

    // (2) has_size + print_anything
    std::cout << std::boolalpha;
    std::cout << "has_size<vector<int>> = " << has_size_v<std::vector<int>> << "\n";
    std::cout << "has_size<int>         = " << has_size_v<int>              << "\n";

    print_anything(std::vector<int>{1, 2, 3});
    print_anything(42);

    // (3) Leetcode 88 整數版
    std::vector<int> a{1, 2, 3, 0, 0, 0};
    std::vector<int> b{2, 5, 6};
    merge_sorted(a, 3, b, 3);
    for (int x : a) std::cout << x << ' ';
    std::cout << "\n";

    // (3') 字串版 (走 generic merge)
    std::vector<std::string> sa{"ant","cat","dog","","",""};
    std::vector<std::string> sb{"bee","frog","yak"};
    merge_sorted(sa, 3, sb, 3);
    for (const auto& s : sa) std::cout << s << ' ';
    std::cout << "\n";

    // (4) safe_divide
    std::cout << "safe_divide(7, 2)   = " << safe_divide(7, 2)     << "\n";
    std::cout << "safe_divide(7.0,2.0)= " << safe_divide(7.0, 2.0) << "\n";
    std::cout << "safe_divide(7, 0)   = " << safe_divide(7, 0)     << "\n";

    // (6) Leetcode 268 Missing Number
    std::cout << "missing_number({3,0,1})         = "
              << missing_number(std::vector<int>{3, 0, 1}) << "\n";
    std::cout << "missing_number({9,6,4,2,3,5,7,0,1}) = "
              << missing_number(std::vector<int>{9, 6, 4, 2, 3, 5, 7, 0, 1}) << "\n";

    // (7) to_str_safe
    std::cout << "to_str_safe(42)         = " << to_str_safe(42) << "\n";
    std::cout << "to_str_safe(3.14)       = " << to_str_safe(3.14) << "\n";
    std::cout << "to_str_safe(string)     = " << to_str_safe(std::string("hi")) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：SFINAE 跟 if constexpr / concepts 比起來該選哪個？
    //    A：C++20 之後優先用 concepts，語法乾淨且錯誤訊息可讀；單一函式內部分支
    //       選擇用 if constexpr 取代 tag dispatch；舊 codebase（C++14/17）才需要
    //       enable_if SFINAE。SFINAE 錯誤訊息冗長、編譯慢，是其最大缺點。
    //
    //  Q2：std::enable_if 與 std::void_t 的使用情境差在哪？
    //    A：enable_if 用於「我已知條件 (trait::value)」時把候選函式從 overload
    //       set 移除；void_t 則用於「我想偵測某個 expression / typedef 是否合法」
    //       的 detection idiom（typename = void_t<decltype(expr)>），更通用。
    //
    //  Q3：SFINAE 為什麼是「Substitution Failure Is Not An Error」而不是直接報錯？
    //    A：它只在「immediate context」中替換失敗才靜默移除候選；若失敗發生在
    //       函式 body 內（non-immediate context）仍然是硬錯。所以 SFINAE 的條件
    //       一定要寫在 template 參數列、回傳型別或 default arg 等 immediate 位置。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. SFINAE = 「代入失敗就靜默剔除候選」，是 C++ template 重要機制。
//    2. enable_if 是把 SFINAE 實作為「型別存不存在」的工具。
//    3. 用 std::void_t + decltype 做的 detection idiom 是最常見、最強大
//       的 trait 寫法。
//    4. 雖然 C++20 concept 可以取代大部分 SFINAE，但理解 SFINAE 仍是讀
//       懂 STL / Boost 程式碼的必備技能。
//
//  【下一篇】
//    17_type_traits.cpp ── 系統介紹 <type_traits>。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 16_sfinae.cpp -o 16_sfinae

// === 預期輸出 ===
// double_it(7)        = [integral] 14
// double_it(3.14)     = [floating] 6.28
// has_size<vector<int>> = true
// has_size<int>         = false
// [container size=3] 1 2 3
// [scalar] 42
// [integral merge] 1 2 2 3 5 6
// [generic merge] ant bee cat dog frog yak
// safe_divide(7, 2)   = 3
// safe_divide(7.0,2.0)= 3.5
// safe_divide(7, 0)   = [safe_divide:int] 0!
// 0
// missing_number({3,0,1})         = 2
// missing_number({9,6,4,2,3,5,7,0,1}) = 8
// to_str_safe(42)         = 42
// to_str_safe(3.14)       = 3.140000
// to_str_safe(string)     = hi
