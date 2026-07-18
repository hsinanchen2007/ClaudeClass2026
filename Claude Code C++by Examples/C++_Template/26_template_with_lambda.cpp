// ============================================================================
//  26_template_with_lambda.cpp  ──  Template 與 Lambda
// ============================================================================
//
//  【本篇目標】
//    Lambda 從 C++11 開始一路升級，跟 template 結合得越來越緊密：
//      - C++11：lambda 引入；參數要寫具體型別。
//      - C++14：generic lambda：[](auto x){...}，每個 auto 是隱含 template
//               參數；本質就是 「unique 類別 + template operator()」。
//      - C++17：constexpr lambda；structured bindings 配合 lambda 更順手。
//      - C++20：template lambda：[]<typename T>(T x){...}，
//               可以用 typename 顯式宣告型別參數，搭配 concept、可取
//               typename T、做完整 metaprogramming。
//
//  【generic lambda 的本質】
//    [](auto x){ return x*2; } 等價於：
//        struct __anon {
//            template<typename T>
//            auto operator()(T x) const { return x * 2; }
//        };
//    每個 auto 對應一個 template 參數。可以拿來用在 STL 演算法、ranges
//    等任何接 callable 的地方。
//
//  【template lambda (C++20)】
//    語法：
//        []<typename T>(T x) { ... }
//        []<typename T>(std::vector<T>& v) { ... }       // 拿到 T
//        []<typename T> requires Numeric<T>(T x) { ... } // 加 concept
//
//    主要好處：能取出 vector 元素的 T 型別、能寫 concept、能做 perfect
//    forwarding：
//        []<typename T>(T&& x) { foo(std::forward<T>(x)); }
//
//  【capture 的 template 化】
//    capture by value: [v]、by reference: [&v]、init-capture: [v = expr]。
//    init-capture 配合 std::move 可避免複製：[v = std::move(x)]。
//
//  參考：
//    https://en.cppreference.com/cpp/language/lambda
// ============================================================================

/*
補充筆記：template_with_lambda
  - template_with_lambda 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - template_with_lambda 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ─── 1. generic lambda (C++14) ──────────────────────────────────────────
//   一個 lambda 即可同時對 int / double / string 排序。
auto less_than = [](auto a, auto b) { return a < b; };

// ─── 2. constexpr lambda (C++17) ────────────────────────────────────────
//   可以在編譯期執行。
constexpr auto cube = [](int x) { return x * x * x; };
static_assert(cube(3) == 27);

// ─── 3. template lambda (C++20) 範例 (本檔以 C++17 為主，這裡用 #if 包) ──
//   為了讓本檔在 -std=c++17 也能編譯，C++20 範例用 #if __cplusplus >= 202002L 包起來。
//   你若用 -std=c++20 就會看到這段執行。

// ─── 4. Leetcode 1370 ── Increasing Decreasing String ───────────────────
//   題目：給字串 s，依下列規則重排：
//     1. 取剩下中「最小」字元，放結果尾端 (升序)
//     2. 取剩下中「比上一個大」的最小字元，繼續放尾端 (升序)
//     3. 直到無法繼續，改成「最大」、再「比上一個小」(降序) 取
//     4. 反覆直到 s 用完
//   範例："aaaabbbbcccc" → "abccbaabccba"
//
//   經典作法：用桶 (count[26]) 計次。每一輪先升序掃一遍，再降序掃一遍。
//
//   為什麼放這裡？
//     用 generic lambda 處理「升序 / 降序」走法，把兩段邏輯抽成函式物件，
//     配合 std::for_each 寫得簡潔。
//
//   時間：O(n)；空間：O(1) (固定 26)。
std::string reorder_string(const std::string& s) {
    int cnt[26] = {0};
    for (char c : s) ++cnt[c - 'a'];

    std::string out;
    out.reserve(s.size());

    auto pick_asc = [&] {                          // 升序取一輪
        for (int i = 0; i < 26; ++i)
            if (cnt[i]) { out.push_back('a' + i); --cnt[i]; }
    };
    auto pick_desc = [&] {                         // 降序取一輪
        for (int i = 25; i >= 0; --i)
            if (cnt[i]) { out.push_back('a' + i); --cnt[i]; }
    };

    while (out.size() < s.size()) {
        pick_asc();
        pick_desc();
    }
    return out;
}

// ─── 5. STL 演算法搭配 lambda ─────────────────────────────────────────
//   日常工作 90% 的 lambda 用法：sort / find_if / for_each / accumulate。
template <typename T>
auto sum_squares(const std::vector<T>& v) {
    return std::accumulate(v.begin(), v.end(), T{},
        [](T acc, T x) { return acc + x * x; });
}

// ─── 6. 工作實用：通用 for_each_with_index ──────────────────────────────
//   常見需求：迭代時知道 index。沒有原生支援，自己包：
template <typename Container, typename F>
void for_each_with_index(const Container& c, F f) {
    std::size_t i = 0;
    for (const auto& x : c) { f(i++, x); }
}

// ─── 7. 工作實用：filter (lambda 配 generic 容器) ────────────────────────
template <typename Container, typename Pred>
auto filter(const Container& c, Pred p) {
    Container out;
    for (const auto& x : c) if (p(x)) out.push_back(x);
    return out;
}

// ─── 8. Leetcode 1365 ── How Many Numbers Are Smaller Than the Current ──
//   難度: easy
//   題目：對每個 nums[i]，回傳「比 nums[i] 小」的元素個數。
//   範例：[8,1,2,2,3] → [4,0,1,1,3]
//
//   解法：sort 後找位置；或 count[101] 桶計數 + 前綴和 O(n)。
//
//   為什麼放在這裡？
//     用 generic lambda 表達「count if <」這種「動態謂詞」，比寫 functor 簡潔。
//
//   時間：O(n²) 簡單版 (lambda + count_if)；O(n) 進階版。
std::vector<int> smaller_numbers_than_current(const std::vector<int>& nums) {
    std::vector<int> result;
    result.reserve(nums.size());
    for (int x : nums) {
        // generic lambda 捕獲 x，每次 count_if 算「比 x 小」的數量
        auto cnt = std::count_if(nums.begin(), nums.end(),
                                  [x](auto v) { return v < x; });
        result.push_back(static_cast<int>(cnt));
    }
    return result;
}

// ─── 9. 工作實用：通用 retry — generic lambda 接任意 callable ────────────
//   嘗試最多 N 次，回傳是否成功 (bool)。callable 必須回傳 bool。
template <typename F>
bool retry_n(int n, F&& fn) {
    for (int i = 0; i < n; ++i) {
        if (fn(i)) return true;
    }
    return false;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) generic lambda
    std::vector<int>         vi{3, 1, 4, 1, 5};
    std::vector<std::string> vs{"banana", "apple", "cherry"};
    std::sort(vi.begin(), vi.end(), less_than);
    std::sort(vs.begin(), vs.end(), less_than);
    std::cout << "sorted ints: ";
    for (int x : vi) std::cout << x << ' ';
    std::cout << "\n";
    std::cout << "sorted strs: ";
    for (const auto& s : vs) std::cout << s << ' ';
    std::cout << "\n";

    // (2) constexpr lambda
    std::cout << "cube(4) = " << cube(4) << "\n";

    // (3) template lambda (C++20)
#if __cplusplus >= 202002L
    auto print_vec = []<typename T>(const std::vector<T>& v) {
        for (const T& x : v) std::cout << x << ' ';
        std::cout << "\n";
    };
    print_vec(vi);
    print_vec(vs);
#else
    std::cout << "(template lambda 需要 C++20)\n";
#endif

    // (4) Leetcode 1370
    std::cout << "reorder \"aaaabbbbcccc\" = "
              << reorder_string("aaaabbbbcccc") << "\n";
    std::cout << "reorder \"rat\"          = "
              << reorder_string("rat") << "\n";

    // (5) sum_squares
    std::cout << "sum_squares({1,2,3,4}) = "
              << sum_squares(std::vector<int>{1, 2, 3, 4}) << "\n";

    // (6) for_each_with_index
    std::vector<std::string> names{"Alice", "Bob", "Carol"};
    for_each_with_index(names, [](std::size_t i, const std::string& s) {
        std::cout << i << ": " << s << "\n";
    });

    // (7) filter
    auto even = filter(std::vector<int>{1, 2, 3, 4, 5, 6}, [](int x) { return x % 2 == 0; });
    std::cout << "even: ";
    for (int x : even) std::cout << x << ' ';
    std::cout << "\n";

    // (8) Leetcode 1365 Smaller Numbers Than Current
    auto sm = smaller_numbers_than_current({8, 1, 2, 2, 3});
    std::cout << "smaller_than_current: ";
    for (int x : sm) std::cout << x << ' ';
    std::cout << "\n";

    // (9) retry_n
    int counter = 0;
    bool ok = retry_n(5, [&counter](int i) {
        ++counter;
        return i == 2;     // 第 3 次嘗試成功
    });
    std::cout << "retry_n ok=" << std::boolalpha << ok
              << " attempts=" << counter << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：C++14 的 generic lambda 跟 C++20 的 template lambda 差在哪？
    //    A：generic lambda（[](auto x){}）等於有個 template <typename T> 的
    //       operator()，但拿不到 T 名字，不能寫 sizeof...(Args) 之類；C++20
    //       template lambda（[]<typename T>(T x){}）讓你顯式宣告 template 參數，
    //       可拿到 T、做 SFINAE/concept、處理 forwarding reference 更乾淨。
    //
    //  Q2：lambda 裡寫 [&] 全捕獲跟具名捕獲哪個好？
    //    A：實務建議「具名捕獲」：清楚標示哪些變數會被使用、避免抓到不該活那麼
    //       久的 reference 造成 dangling。[&] / [=] 多用在 local 短命 lambda；
    //       回傳或存起來的 lambda 一律具名，並小心是 by ref 還是 by value。
    //
    //  Q3：把 lambda 當 template 參數傳給 STL（如 sort、find_if）有額外成本嗎？
    //    A：沒有。每個 lambda 是獨立 closure type，傳給 template 函式時編譯器
    //       inline 整個 operator()，效能等同手寫迴圈。改用 std::function 包起
    //       來才會有 type erasure 的 virtual call 與可能的 heap 配置成本。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. C++14 generic lambda = 隱含的 template operator()。
//    2. C++17 constexpr lambda 可以參與編譯期計算。
//    3. C++20 template lambda 提供完整 typename 控制與 concept。
//    4. 與 STL 演算法、ranges 搭配時，lambda 是最自然的 callable。
//
//  【下一篇】
//    27_constexpr_templates.cpp ── 編譯期計算。
// ============================================================================
