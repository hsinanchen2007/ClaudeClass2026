// ============================================================================
//  array.cpp — std::array 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra array.cpp -o array && ./array
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/array
//  參考 (cplusplus.com): https://cplusplus.com/reference/array/array/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::array<T, N> 是 C++11 引入的「固定大小陣列」,是 C-style array 的物件包裝。
//  與 vector 的最大差別:大小 N 在編譯期就決定,無法成長/縮小,且元素直接放在
//  std::array 物件內部 (通常在 stack),完全沒有 heap 配置。
//
//  ▌ 底層資料結構
//  struct array { T elems[N]; };
//  幾乎就是 C-array 的 wrapper,但加上了 STL container 介面。
//
//  ▌ 所屬類別
//  Sequence container (序列容器),但和其他 container 不同 — 大小是 type 的一部分。
//
//  ▌ 時間複雜度
//      隨機存取 (operator[], at)        O(1)
//      front / back                     O(1)
//      fill                             O(N)
//      swap                             O(N)  ★ 不是 O(1),因為要逐元素交換
//      size / empty                     O(1)  且為 constexpr
//
//  ▌ 與其他 container 的比較
//      array  vs  C array  : array 提供 STL 介面、可被傳值、知道自己的 size
//      array  vs  vector   : array 大小固定,沒有 heap 配置;不能 push_back
//      array  vs  span     : span 是「視圖」不擁有資料,大小可在 runtime 決定
//
//  ▌ 適用情境
//      ✅ 元素數量在編譯期已知且固定 (如 RGB 色票、9x9 棋盤、固定查找表)
//      ✅ 想要 stack 配置以避免 heap allocation 成本
//      ✅ 需要 constexpr 容器 (array 大部分操作可在 constexpr context 使用)
//      ❌ 大小會變動 → 用 vector
//      ❌ 大型陣列放在 stack 上 (可能 stack overflow) → 用 vector
//
//  ▌ Iterator 失效規則
//      std::array 的 iterator 不會因為任何 member function 而失效
//      (因為從未發生 reallocation)。只有物件本身被銷毀時才失效。
//
// ============================================================================

/*
補充筆記：std::array
  - std::array 大小是型別的一部分，array<int,3> 和 array<int,4> 是不同型別。
  - 它的資料連續配置，可和 C API 透過 data() 互通，但大小不能在執行期改變。
  - 和 C-style array 相比，std::array 可拷貝、可回傳、可使用 begin/end。
  - std::array 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::array
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::array、C 陣列、std::vector 三者的差別？
//     答：std::array<T,N> 是固定大小的 aggregate，N 是 compile-time 常數，元素就在物件內部，
//         完全沒有動態配置；具備 size()/at()/iterator 等 STL 介面，可 copy/assign，傳參不會 decay 成指標。
//         C 陣列沒有這些介面、傳參會 decay、無法直接 copy。vector 大小可執行期改變、元素在 heap。
//     追問：std::array 的 size() 是 constexpr 嗎？（是）
//
// 🔥 Q2. std::array<int, 0> 合法嗎？
//     答：合法。empty() 回傳 true、size() 為 0、begin() == end()；
//         但對它呼叫 front() / back() 是 undefined behavior。
//
// ⚠️ 陷阱. std::array 的元素一定在 stack 上嗎？
//     答：不一定。元素是「嵌在 array 物件內部」，所以它跟著 array 物件本身的儲存期走：
//         局部變數就在 stack、global 就在靜態儲存區、new 出來的就在 heap。
//     為什麼會錯：把「不做動態配置」誤讀成「一定在 stack」，兩者不是同一件事。
// ═══════════════════════════════════════════════════════════════════════════

#include <array>
#include <iostream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <tuple>     // std::get, std::tuple_size

template <typename T, std::size_t N>
void print(const std::array<T, N>& a, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (const auto& e : a) std::cout << e << ' ';
    std::cout << "] (size=" << a.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構 / 初始化
    // ========================================================================
    // std::array 沒有自訂建構子,初始化方式只有一種 — aggregate initialization。
    // 注意需要兩層大括號 (C++14 起才允許省略內層) 是因為 array 內部包了一個
    // C-array 成員 elems[N]。

    std::array<int, 5> a1 = {1, 2, 3, 4, 5};       // 五個值
    std::array<int, 5> a2 = {1, 2};                // 後面自動補 0 → [1,2,0,0,0]
    std::array<int, 5> a3{};                       // 全部 value-init → [0,0,0,0,0]
    std::array<std::string, 3> a4 = {"hi", "yo", "bye"};

    print(a1, "a1                  ");
    print(a2, "a2 (部分初始化)    ");
    print(a3, "a3 (value-init)     ");
    print(a4, "a4 (字串)           ");

    // C++17 起的 class template argument deduction (CTAD)
    std::array a5 = {10, 20, 30};   // 推導為 std::array<int, 3>
    print(a5, "a5 (CTAD)           ");

    // ========================================================================
    //  2. 元素存取 (Element access)
    // ========================================================================
    //   operator[](i)  : 不檢查邊界,越界 UB
    //   at(i)          : 檢查邊界,越界丟 std::out_of_range
    //   front() / back(): 取第一個 / 最後一個 (空 array 即 N=0 時呼叫是 UB)
    //   data()          : 取得指向 elems[0] 的原始指標,等同於 &elems[0]

    std::cout << "\n[元素存取]\n";
    std::cout << "a1[2]      = " << a1[2] << '\n';        // 3
    std::cout << "a1.at(0)   = " << a1.at(0) << '\n';     // 1
    std::cout << "a1.front() = " << a1.front() << '\n';   // 1
    std::cout << "a1.back()  = " << a1.back() << '\n';    // 5

    try {
        std::cout << a1.at(99) << '\n';
    } catch (const std::out_of_range& ex) {
        std::cout << "at(99) 例外: " << ex.what() << '\n';
    }

    // data() 取得連續 buffer,可傳給 C API
    int* p = a1.data();
    std::cout << "data()[0] = " << p[0] << '\n';

    // std::get<I>(arr) 是編譯期版本的存取,越界會編譯失敗 (而非 runtime 例外)
    std::cout << "std::get<3>(a1) = " << std::get<3>(a1) << '\n';
    // std::get<10>(a1);  // 編譯錯誤,N 範圍檢查在 compile time

    // ========================================================================
    //  3. Iterators
    // ========================================================================
    // begin/end/cbegin/cend/rbegin/rend/crbegin/crend
    // 都是 random-access iterator,且皆為 constexpr (C++17 起)。

    std::cout << "\n[正向]   ";
    for (auto it = a1.begin(); it != a1.end(); ++it) std::cout << *it << ' ';
    std::cout << "\n[反向]   ";
    for (auto it = a1.rbegin(); it != a1.rend(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  4. 容量 (Capacity)
    // ========================================================================
    //   empty()    : N == 0 嗎?  (constexpr,編譯期就能算出)
    //   size()     : 永遠回傳 N    (constexpr)
    //   max_size() : 等於 size()   (constexpr)
    //
    // ★ array 沒有 reserve / capacity / shrink_to_fit,因為大小是固定的。

    std::cout << "\n[Capacity] size=" << a1.size()
              << ", max_size=" << a1.max_size()
              << ", empty=" << std::boolalpha << a1.empty() << '\n';

    // 特殊情況:N == 0 的 array 是合法的,但 front/back/data 行為要小心
    std::array<int, 0> empty_arr{};
    std::cout << "空 array.empty() = " << empty_arr.empty()
              << ", size=" << empty_arr.size() << '\n';

    // ========================================================================
    //  5. Operations (修改類)
    // ========================================================================

    // ──── fill(value) ────
    // 把所有元素填成同一個 value,O(N)
    std::array<int, 5> f{};
    f.fill(7);
    print(f, "after fill(7)       ");

    // ──── swap(other) ────
    // 注意:array 的 swap 是 O(N) 而非 O(1) (vector 的 swap 是 O(1))
    // 因為 array 沒有指標可以交換,必須逐元素 swap。
    std::array<int, 3> s1{1, 2, 3}, s2{9, 8, 7};
    s1.swap(s2);
    print(s1, "s1 after swap       ");
    print(s2, "s2 after swap       ");

    // 非成員 std::swap(a, b) — 透過 ADL 找到對應特化,行為等同成員 swap
    std::swap(s1, s2);
    print(s1, "after std::swap     ");

    // 非成員 std::get<I> — tuple 介面,與 std::tuple_element 等搭配
    // (前面已有 std::get<3>(a1) 的範例,此處再用一次強調與 tuple-like 體系的一致性)
    constexpr std::array<int, 4> ga{10, 20, 30, 40};
    static_assert(std::get<2>(ga) == 30);
    std::cout << "std::get<0>(ga) = " << std::get<0>(ga) << '\n';

    // ========================================================================
    //  6. Tuple-like 介面 (Structured bindings)
    // ========================================================================
    // std::array 支援 std::tuple_size / tuple_element / std::get,
    // 因此可以用 structured bindings 拆解。

    std::array<int, 3> t{100, 200, 300};
    auto [x, y, z] = t;
    std::cout << "\n[structured binding] x=" << x
              << ", y=" << y << ", z=" << z << '\n';

    // tuple_size 在 compile time 拿到 N
    constexpr auto N = std::tuple_size<decltype(t)>::value;
    std::cout << "tuple_size = " << N << '\n';

    // ========================================================================
    //  7. 比較運算子
    // ========================================================================
    // 字典序比較。兩個 array<T,N> 必須有「相同 N」才能比較。
    std::array<int, 3> c1{1,2,3}, c2{1,2,4};
    std::cout << "\n[比較] c1 < c2 ? " << (c1 < c2) << '\n';

    // ========================================================================
    //  8. constexpr 應用
    // ========================================================================
    // C++17 起大量函式為 constexpr,可在編譯期使用。
    constexpr std::array<int, 5> ce = {1, 2, 3, 4, 5};
    static_assert(ce.size() == 5);
    static_assert(ce[2] == 3);
    static_assert(ce.front() == 1);
    static_assert(ce.back() == 5);

    // ========================================================================
    //  9. std::to_array (C++20)
    // ========================================================================
    // 從 C-array 建立 std::array,自動推導大小與型別。
    auto a6 = std::to_array("hello");          // → array<char, 6> (含 '\0')
    std::cout << "to_array(\"hello\") size=" << a6.size() << '\n';

    auto a7 = std::to_array<long>({1, 2, 3});  // → array<long, 3>
    print(a7, "to_array<long>      ");

    // ========================================================================
    //  10. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) 部分初始化會自動補 0,而不是「未初始化」
    //      std::array<int, 5> a = {1, 2};   // [1,2,0,0,0]
    //      但 std::array<int, 5> a;          // 內容未定 (POD-like)
    //
    //  (2) 大型 array 放在 stack 上會 stack overflow
    //      std::array<int, 10000000> big;   // 40 MB 在 stack → 危險
    //      改用: auto big = std::make_unique<std::array<int, 10000000>>();
    //
    //  (3) array 的 swap 不是 O(1)
    //      因為大小固定且元素內嵌,只能逐元素交換 (O(N))。
    //
    //  (4) 兩個 array 大小不同就無法比較或互相 assign
    //      std::array<int,3> a; std::array<int,4> b;
    //      a == b;  // 編譯錯誤 (不同型別)
    //
    //  (5) 不要用 sizeof / 指標衰減的方式取得大小
    //      array 的 size 是 N,直接用 a.size() 或 std::tuple_size 取得。
    //
    //  (6) 傳入函式時要小心 template 參數
    //      void f(const std::array<int, N>& a)  // N 是 template parameter
    //      若想接受任意 N,寫成 template <std::size_t N>。
    //
    //  (7) 使用 fill() 後元素仍然是同一塊空間
    //      array 不會做任何 reallocation,所以 iterator 永遠不會失效。

    // ========================================================================
    //  11. 最佳實踐
    // ========================================================================
    //
    //  • 大小固定 + 不太大 → 優先 array,而非 vector,省 heap 配置
    //  • 元素數量 > 數萬 → 還是用 vector 或 unique_ptr<array> 避免 stack 爆掉
    //  • 用 std::to_array 從 brace-list 推導,省 N
    //  • 編譯期計算 → 善用 constexpr array (查找表、查表演算法)
    //  • 與 C API 互動 → a.data() 是合法且安全的指標
    //  • 想避免 size 不一致 bug → 直接用 std::array 而不要混用 raw array + size

    // ========================================================================
    //  12. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // std::array 在面試題中最常見的應用是「固定大小的計數陣列 (counting array)」。
    // 因為 ASCII / 26 個字母 / 10 個數字 都是「編譯期已知大小」,
    // 用 std::array<int, 26> 比 unordered_map<char,int> 快得多,且可 stack 配置。

    // ──── LC 242: Valid Anagram (有效的字母異位詞) ────
    // 兩字串若是字母重排,則每個字母的出現次數必須完全一致。
    // 用 array<int, 26> 計數,一次 O(n) 解決。
    {
        auto is_anagram = [](const std::string& s, const std::string& t) {
            if (s.size() != t.size()) return false;
            std::array<int, 26> cnt{};            // 全部 value-init 為 0
            for (char c : s) ++cnt[c - 'a'];
            for (char c : t) {
                if (--cnt[c - 'a'] < 0) return false;
            }
            return true;
        };
        std::cout << "\n[LC242 Anagram]\n";
        std::cout << R"(  "anagram" vs "nagaram" → )" << std::boolalpha
                  << is_anagram("anagram", "nagaram") << '\n';   // true
        std::cout << R"(  "rat"     vs "car"     → )"
                  << is_anagram("rat", "car") << '\n';            // false
    }

    // ──── LC 387: First Unique Character in a String (第一個不重複字元) ────
    // 找到第一個只出現一次的字元下標。
    // 兩遍掃描:第一遍計數,第二遍找第一個 cnt==1 的字元。
    {
        auto first_uniq = [](const std::string& s) -> int {
            std::array<int, 26> cnt{};
            for (char c : s) ++cnt[c - 'a'];
            for (int i = 0; i < (int)s.size(); ++i) {
                if (cnt[s[i] - 'a'] == 1) return i;
            }
            return -1;
        };
        std::cout << "[LC387 First Uniq] \"leetcode\" → " << first_uniq("leetcode") << '\n';
        // 預期輸出: 0  (字母 'l' 只出現一次)
        std::cout << "[LC387 First Uniq] \"loveleetcode\" → " << first_uniq("loveleetcode") << '\n';
        // 預期輸出: 2  (字母 'v')
    }

    // ──── LC 36 (簡化版): 9x9 棋盤檢查 ────
    // 使用 std::array<std::array<bool, 9>, 9> 表示固定大小棋盤的「可達性」。
    // 這展示了 array 的 stack 配置 + constexpr size 在棋盤類問題的優勢。
    {
        constexpr size_t N = 9;
        std::array<std::array<int, N>, N> board{};
        // 對角線填 1
        for (size_t i = 0; i < N; ++i) board[i][i] = 1;
        std::cout << "[9x9 Board diagonal sum] = ";
        int sum = 0;
        for (size_t i = 0; i < N; ++i) sum += board[i][i];
        std::cout << sum << '\n';
        // 預期輸出: 9
    }

    // ──── LC 169: Majority Element (多數元素) ────
    // 找出陣列中出現次數超過 n/2 的元素 (保證存在)。
    // Boyer-Moore 投票演算法:O(n) 時間 O(1) 空間,只需要兩個變數。
    // 用 std::array 是因為「大小固定」剛好契合面試題的小輸入,且編譯期已知能進 stack。
    {
        std::array<int, 7> nums{2, 2, 1, 1, 1, 2, 2};
        int candidate = 0, count = 0;
        for (int x : nums) {
            if (count == 0) candidate = x;
            count += (x == candidate) ? 1 : -1;
        }
        std::cout << "[LC169 Majority] = " << candidate << '\n';
        // 預期輸出: 2  (出現 4 次,超過 7/2)
    }

    // ========================================================================
    //  12. 實戰範例:固定維度查表 (Lookup Table) — 月份天數計算
    // ========================================================================
    // 真實場景:工作上常需要「固定大小的查表」 — 像月份天數、字元映射、bit count
    // 預計算等。這類資料在程式生命週期不變、大小編譯期已知,std::array 是首選:
    //   • 編譯期確定大小 → 可放 constexpr → 編譯器可能做常數摺疊
    //   • stack / static 區配置 → 完全沒有 heap 開銷
    //   • 連續記憶體 + 隨機存取 O(1) → cache 友善
    {
        // 平年每月天數
        constexpr std::array<int, 12> DAYS{31,28,31,30,31,30,31,31,30,31,30,31};
        auto days_in_month = [&](int month, bool leap) {
            int d = DAYS[month - 1];
            if (leap && month == 2) d = 29;
            return d;
        };
        std::cout << "[Lookup Table] 2024年2月 = " << days_in_month(2, true) << " 天\n";
        std::cout << "[Lookup Table] 2025年2月 = " << days_in_month(2, false) << " 天\n";
        // 預期輸出: 29 天 / 28 天
    }

    std::cout << "\n=== array demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::array 大小是 compile-time 常量,為何不直接用 C-style array?
    //    A：std::array 是聚合 (aggregate),保留 C array 的效能與佈局,但同時
    //       擁有完整的 STL 介面 (begin/end、size、swap、tuple-like) 並支援
    //       值傳遞 / 回傳。最重要的是它「不會 decay 成指標」,可避免大量陷阱。
    //
    //  Q2：std::array<int, 5> a{}; 與 std::array<int, 5> a; 有何差異?
    //    A：a{} 為 value-initialization,所有元素歸零;a; 為 default-initialization,
    //       對 trivial type (如 int) 不會初始化,讀取等同於 UB。寫程式時務必加上 {}。
    //
    //  Q3：std::array 為何沒有 push_back / insert / erase?
    //    A：因為大小在 compile-time 就固定 (template 參數 N),記憶體配置在 stack 上
    //       (或 static 區),不可動態改變。需要動態大小請改用 std::vector。
    //
    return 0;
}

/*
============================================================================
  附錄:std::array 完整 member function 一覽
============================================================================
  Implicit special members:
      (constructor)             aggregate initialization
      ~array()
      operator=                 implicit

  Element access:
      at, operator[], front, back, data

  Iterators:
      begin, end, cbegin, cend
      rbegin, rend, crbegin, crend

  Capacity:
      empty, size, max_size                 (皆 constexpr)

  Operations:
      fill                                  (C++11 起)
      swap                                  O(N)

  Non-member:
      operator==, !=, <, <=, >, >=, <=> (C++20)
      std::get<I>(array)
      std::swap(array, array)
      std::to_array (C++20)
      std::tuple_size, std::tuple_element

============================================================================
*/
