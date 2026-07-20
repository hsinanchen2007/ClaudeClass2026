// ============================================================================
//  18_tag_dispatch.cpp  ──  Tag Dispatch
// ============================================================================
//
//  【本篇目標】
//    Tag dispatch 是 template metaprogramming 的「優雅替代品」── 用「空
//    結構作為 tag」來指引 overload resolution，比 SFINAE 直觀許多。STL
//    iterator 演算法就是經典例子 (random access vs bidirectional)。
//
//  【動機】
//    SFINAE / enable_if 寫多了，整個函式宣告會被「條件判斷」淹沒，看不出
//    主邏輯。Tag dispatch 的想法：
//        ① 定義空型別 (struct integer_tag {}; struct float_tag {};)
//        ② 寫一個 trait 把「型別」對應到 tag
//        ③ 多載函式：每個 overload 多吃一個 tag 參數
//        ④ 呼叫時依 trait 取 tag，自動選對版本
//
//  【最小範例】
//
//        struct int_tag {};
//        struct float_tag {};
//
//        template<typename T>
//        using number_tag_t = std::conditional_t<
//            std::is_integral_v<T>, int_tag, float_tag>;
//
//        template<typename T> void impl(T x, int_tag)   { std::cout << "int " << x << "\n"; }
//        template<typename T> void impl(T x, float_tag) { std::cout << "fp  " << x << "\n"; }
//
//        template<typename T>
//        void process(T x) { impl(x, number_tag_t<T>{}); }
//
//  【STL 經典應用：iterator_category】
//    std::iterator_traits<It>::iterator_category 是 STL 預先定義的 5 種 tag：
//        input_iterator_tag        : 只能讀，只能單方向走訪一次
//        output_iterator_tag       : 只能寫
//        forward_iterator_tag      : 可重複讀寫，單方向
//        bidirectional_iterator_tag: 可雙向 (++/--)
//        random_access_iterator_tag: 可隨機跳 (it+5)
//        contiguous_iterator_tag   : C++20 起新增 (記憶體連續)
//
//    std::advance、std::distance 等「演算法」內部用 tag dispatch，依 iterator
//    類別選最快版本：
//        - random access：直接 it += n，O(1)
//        - bidirectional：迴圈走 n 次，O(n)
//
//  【Tag Dispatch vs SFINAE vs if constexpr】
//    - SFINAE：可以讀但寫起來囉嗦，C++11/14 主流
//    - Tag dispatch：邏輯直觀，多 overload 一字排開，少了 enable_if 的雜訊
//    - if constexpr (C++17)：最簡潔，把 tag 改寫成編譯期 if，但有時不能用
//      (例如不同 tag 對應「函式不存在的特化」)
//    三者都要會，依場景選用。
//
//  參考：https://en.cppreference.com/cpp/iterator/iterator_tags
// ============================================================================

/*
補充筆記：tag_dispatch
  - tag_dispatch 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - tag_dispatch 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】tag dispatch
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 tag dispatch？相對 SFINAE 有什麼優點？
//     答：用一個「空標籤型別」當額外參數，靠正常的重載決議選實作，而不是靠
//         SFINAE 把候選剔除。優點：語法直觀、多個 overload 一字排開、錯誤訊息
//         乾淨，而且可以用「繼承層次」天然表達優先序。缺點是需要有能產生 tag
//         的 trait 體系。
//     追問：runtime 成本多少？（零。tag 是空型別、選哪個 overload 在編譯期就定了，
//         inline 之後整層 dispatch 會完全消失）
//
// 🔥 Q2. STL 的 std::advance 為什麼要用 tag dispatch？
//     答：依 iterator_traits<It>::iterator_category 選最快的實作——random access
//         直接 it += n 是 O(1)，bidirectional 與 input 只能跑迴圈是 O(n)。
//         呼叫端永遠只寫 advance(it, n)，效能隨容器自動最佳化。
//     追問：為什麼「繼承層次」是關鍵？（本機以 is_base_of 實測確認
//         input ← forward ← bidirectional ← random_access 是一條繼承鏈，
//         所以某個類別沒有專屬 overload 時，會自動退回 base 的版本 —— 這就是
//         不必寫任何條件式就得到的 fallback 優先序）
//
// Q3. C++17 有了 if constexpr，tag dispatch 是不是過時了？
//     答：沒有。if constexpr 只能在「同一個函式本體」內部分支；tag dispatch 是
//         overload 層級的，可以被第三方擴充新的 tag、不同版本可以有不同簽章與
//         不同型別需求。自家可控的型別用 if constexpr 較簡潔，要開放擴充的
//         框架（如 STL 的 iterator 體系）仍然要用 tag。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <type_traits>
#include <vector>

// ─── 1. 自製 tag dispatch：integer 走整數版、float 走浮點版 ──────────────
struct int_tag   {};
struct float_tag {};

template <typename T>
using number_tag_t = std::conditional_t<
    std::is_integral_v<T>, int_tag, float_tag>;

template <typename T>
void describe_impl(T x, int_tag)   { std::cout << "[int_tag]   value = " << x << "\n"; }

template <typename T>
void describe_impl(T x, float_tag) { std::cout << "[float_tag] value = " << x << "\n"; }

template <typename T>
void describe(T x) {
    static_assert(std::is_arithmetic_v<T>, "需要數值型別");
    describe_impl(x, number_tag_t<T>{});      // 在這裡產生 tag 物件
}

// ─── 2. 自製 advance：對不同 iterator 類別做不同事 ──────────────────────
//   這就是 STL std::advance 的精神 (簡化版)。
//   - random_access：直接 it += n，O(1)
//   - bidirectional：迴圈，O(|n|)，可走負方向
//   - input/forward：迴圈 ++，O(n)，不能走負

template <typename It, typename Distance>
void advance_impl(It& it, Distance n, std::random_access_iterator_tag) {
    it += n;
}

template <typename It, typename Distance>
void advance_impl(It& it, Distance n, std::bidirectional_iterator_tag) {
    if (n >= 0) while (n--) ++it;
    else        while (n++) --it;
}

template <typename It, typename Distance>
void advance_impl(It& it, Distance n, std::input_iterator_tag) {
    while (n-- > 0) ++it;
}

template <typename It, typename Distance>
void my_advance(It& it, Distance n) {
    using cat = typename std::iterator_traits<It>::iterator_category;
    advance_impl(it, n, cat{});
}

// ─── 3. Leetcode 26 ── Remove Duplicates from Sorted Array (示範用 it 走法) ──
//   題目：給排序好的陣列，移除重複元素 (in-place)，回傳新長度。前 k 個元素
//        必須是去重後的有效資料。
//   範例：[1,1,2] → 長度 2，nums 變 [1,2,...]
//
//   經典雙指針：i 指目前獨特區尾端，j 走訪。
//
//   時間：O(n)
//   空間：O(1)
//
//   為什麼放這裡？
//     原 LC 用 vector<int>。我們做成 template 對任意「random access iterator」
//     的容器都能跑；用 my_advance 演示 tag dispatch。
template <typename It>
std::size_t remove_duplicates(It first, It last) {
    if (first == last) return 0;
    It k = first;
    for (It j = first; j != last; ++j) {
        if (*j != *k) {
            ++k;
            *k = *j;
        }
    }
    return std::distance(first, k) + 1;
}

// ─── 4. 工作實用：用 tag dispatch 做「算法策略」 ────────────────────────
//   想像我們在做 hash table，桶位有「small」(<32 個) 用線性掃，「big」(≥32)
//   用 hash。可以把策略做成 tag，從 size 決定。
//   下面只是示意：N 是 NTTP，依 N 大小選擇分支。
struct linear_tag {};
struct hash_tag   {};

template <std::size_t N>
using bucket_strategy_t = std::conditional_t<(N < 32), linear_tag, hash_tag>;

template <std::size_t N>
void describe_strategy_impl(linear_tag) { std::cout << "N=" << N << " → linear scan\n"; }

template <std::size_t N>
void describe_strategy_impl(hash_tag)   { std::cout << "N=" << N << " → hash table\n"; }

template <std::size_t N>
void describe_strategy() { describe_strategy_impl<N>(bucket_strategy_t<N>{}); }

// ─── 5. Leetcode 283 ── Move Zeroes (用 tag dispatch 選整數版 / 通用版) ──
//   難度: easy
//   題目：給陣列 nums，把所有 0 移到尾端，並保持非 0 元素相對順序。原地。
//   範例：[0,1,0,3,12] → [1,3,12,0,0]
//
//   解法：雙指針 — slow 指目前已填好的尾部，fast 找下一個非 0。
//
//   為什麼放在這裡？
//     依「T 是否為整數」分流：整數版假設「0」是 T(0)；其他型別用「!= T{}」
//     當預設「空」。用 tag dispatch 選擇實作。
struct integer_zero_tag {};
struct generic_zero_tag {};

template <typename T>
using zero_tag_t = std::conditional_t<std::is_integral_v<T>,
                                       integer_zero_tag, generic_zero_tag>;

template <typename T>
void move_zeroes_impl(std::vector<T>& v, integer_zero_tag) {
    std::size_t slow = 0;
    for (std::size_t fast = 0; fast < v.size(); ++fast) {
        if (v[fast] != T(0)) std::swap(v[slow++], v[fast]);
    }
}

template <typename T>
void move_zeroes_impl(std::vector<T>& v, generic_zero_tag) {
    std::size_t slow = 0;
    for (std::size_t fast = 0; fast < v.size(); ++fast) {
        if (!(v[fast] == T{})) std::swap(v[slow++], v[fast]);
    }
}

template <typename T>
void move_zeroes(std::vector<T>& v) {
    move_zeroes_impl(v, zero_tag_t<T>{});
}

// ─── 6. 工作實用：依「型別大小」選擇 by-value 或 by-reference 傳參 ───────
//   小型 trivial 物件 by value (寄存器傳遞)；大型 by const reference。
//   現代編譯器通常自動最佳化，但 tag dispatch 讓 API 設計意圖更清楚。
struct by_value_tag {};
struct by_ref_tag {};

template <typename T>
using pass_strategy_t = std::conditional_t<
    (sizeof(T) <= 16) && std::is_trivially_copyable_v<T>,
    by_value_tag, by_ref_tag>;

template <typename T>
void describe_pass_impl(by_value_tag) { std::cout << "[by value] sizeof=" << sizeof(T) << "\n"; }

template <typename T>
void describe_pass_impl(by_ref_tag)   { std::cout << "[by ref]   sizeof=" << sizeof(T) << "\n"; }

template <typename T>
void describe_pass() { describe_pass_impl<T>(pass_strategy_t<T>{}); }

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) describe
    describe(42);
    describe(3.14);

    // (2) my_advance
    std::vector<int> v{1, 2, 3, 4, 5};         // random access
    auto it = v.begin();
    my_advance(it, 3);                          // → 跳到 v[3] = 4
    std::cout << "vector advance 3: " << *it << "\n";

    std::list<int> l{10, 20, 30, 40};           // bidirectional
    auto lit = l.begin();
    my_advance(lit, 2);                         // 走兩步 → 30
    std::cout << "list advance 2: " << *lit << "\n";

    // (3) Leetcode 26
    std::vector<int> nums{1, 1, 2, 2, 2, 3};
    auto k = remove_duplicates(nums.begin(), nums.end());
    std::cout << "removed_dup len = " << k << ", first " << k << " elems: ";
    for (std::size_t i = 0; i < k; ++i) std::cout << nums[i] << ' ';
    std::cout << "\n";

    // (4) bucket strategy
    describe_strategy<8>();
    describe_strategy<100>();

    // (5) Leetcode 283 Move Zeroes
    std::vector<int> nums2{0, 1, 0, 3, 12};
    move_zeroes(nums2);
    std::cout << "move_zeroes: ";
    for (int x : nums2) std::cout << x << ' ';
    std::cout << "\n";

    // (6) describe_pass
    describe_pass<int>();              // sizeof 4，by value
    describe_pass<std::string>();      // sizeof 大，by ref

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：tag dispatch 跟 SFINAE 比起來差在哪？
    //    A：tag dispatch 用「不同 tag type 的 overload」決定走哪條路徑，編譯器
    //       直接用 overload resolution 解，錯誤訊息乾淨；SFINAE 用 enable_if 把
    //       不該選的候選移除，但失敗訊息常常一大片 trait 名稱，較難讀。
    //
    //  Q2：tag dispatch 在 runtime 有額外成本嗎？
    //    A：沒有。tag 物件（例如 random_access_iterator_tag{}）是空 type，編譯期
    //       決定走哪個 overload，inliner 通常會把整層 dispatch 函式都消掉，最終
    //       產生的程式碼跟手寫直接呼叫的版本一致。
    //
    //  Q3：C++17 之後 if constexpr 是不是讓 tag dispatch 完全過時了？
    //    A：不全然。if constexpr 只能在「同一個函式 body」裡分支，且分支條件要能
    //       寫成 constexpr 布林；tag dispatch 透過 overload 還能跨多個 TU、可被
    //       第三方擴充新的 tag。對自家、可控制的型別 if constexpr 較簡潔。
    //
    return 0;
}

// ============================================================================
//  【小結 & 經驗法則】
//    1. Tag dispatch 是讀起來最直觀的「條件 overload」工具。
//    2. STL 演算法 (advance、distance、algorithm 內部) 廣泛使用。
//    3. C++17 之後 if constexpr 可以取代許多 tag dispatch，但 tag dispatch
//       仍適合需要「不同 overload 簽名 / 不同型別需求」的場景。
//    4. tag 通常是空 struct，無 runtime 成本。
//
//  【下一篇】
//    19_if_constexpr.cpp ── 編譯期 if，現代 metaprogramming 的清涼劑。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 18_tag_dispatch.cpp -o 18_tag_dispatch

// === 預期輸出 ===
// [int_tag]   value = 42
// [float_tag] value = 3.14
// vector advance 3: 4
// list advance 2: 30
// removed_dup len = 3, first 3 elems: 1 2 3
// N=8 → linear scan
// N=100 → hash table
// move_zeroes: 1 3 12 0 0
// [by value] sizeof=4
// [by ref]   sizeof=32
