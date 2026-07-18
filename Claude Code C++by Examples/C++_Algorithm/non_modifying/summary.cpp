/*
================================================================================
【C++_Algorithm/non_modifying/summary.cpp】
本章末總整理 — 非修改型序列演算法 (Non-modifying Sequence Operations)
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Non-modifying_sequence_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、本章涵蓋的 API 分組
--------------------------------------------------------------------------------
  Predicate 檢查:
    all_of  / any_of / none_of                   → bool
  條件走訪:
    for_each / for_each_n                        → 回傳走訪後的 functor 或迭代器
  搜尋:
    find / find_if / find_if_not                 → iterator (找到一個元素)
    adjacent_find                                → 找第一對相鄰相等
    find_first_of                                → 找出現在另一組 set 的第一個元素
    find_end                                     → 找最後一次「子序列」出現位置
    search / search_n                            → 子序列搜尋 / 連續 N 個相同
    mismatch                                     → 找出第一個不同位置
  計數與比較:
    count / count_if                             → 計數
    equal                                        → 兩 range 是否逐元素相等
    lexicographical_compare                      → 字典序 (放在 permutation 章節)

--------------------------------------------------------------------------------
二、共通特性
--------------------------------------------------------------------------------
  - 「非修改型」= 演算法不會改動範圍內元素。
  - 大多只要求 InputIterator;少數 (例如 adjacent_find) 要 ForwardIterator。
  - 通常單次走訪 O(N);特殊版本 (search、find_end) 可能是 O(N*M)。
  - 「沒找到」回 last,不是 nullptr,不是 npos;判斷一定要與 last 比。

--------------------------------------------------------------------------------
三、API 對照表 (Cheat Sheet)
--------------------------------------------------------------------------------
  API             輸入                 回傳                 找不到/空
  ──────────────  ───────────────────  ───────────────────  ──────────────
  all_of          range + predicate    bool                 空 range → true (vacuously)
  any_of          range + predicate    bool                 空 range → false
  none_of         range + predicate    bool                 空 range → true
  for_each        range + functor      functor (C++11+)     —
  for_each_n      first + n + functor  iterator (first+n)   —
  find            range + value        iterator             找不到 → last
  find_if         range + predicate    iterator             同上
  find_if_not     range + predicate    iterator             同上
  adjacent_find   range                iterator             沒相鄰相等 → last
  count           range + value        difference_type      0
  count_if        range + predicate    difference_type      0
  equal           兩 range             bool                 兩端皆空 → true
  mismatch        兩 range             pair<it1,it2>        全部相等 → (last1, ...)
  search          被搜 + 子序列         iterator             找不到 → last1
  search_n        range + n + value    iterator             找不到 → last
  find_first_of   range + 字符集 range  iterator             找不到 → last1
  find_end        被搜 + 子序列         iterator             找不到 → last1

--------------------------------------------------------------------------------
四、複雜度與迭代器需求
--------------------------------------------------------------------------------
  API               時間複雜度        迭代器
  ────────────────  ─────────────────  ─────────────────────
  find / find_if    O(N)               InputIterator
  count(_if)        O(N)               InputIterator
  equal             O(min(N1, N2))     InputIterator (4-iter 版要看)
  mismatch          同 equal           InputIterator
  for_each          O(N) (N 次呼叫)    InputIterator
  adjacent_find     O(N)               ForwardIterator
  search            O(N*M) 最壞        ForwardIterator
                    (C++17 後有 BMH 等 Searcher 物件可加速)
  search_n          O(N)               ForwardIterator
  find_end          O(N*M)             ForwardIterator
  find_first_of     O(N*M)             InputIterator + ForwardIterator

--------------------------------------------------------------------------------
五、空範圍語意 (Empty Range Semantics)
--------------------------------------------------------------------------------
  ★ 重要邏輯陷阱:
    std::all_of  on empty → true   (vacuous truth — 命題對空集合自動為真)
    std::any_of  on empty → false  (找不到任何符合)
    std::none_of on empty → true   (確實沒有符合)

  口訣:「沒有資料」對 all 是「沒有反例」,所以 all 為真。

--------------------------------------------------------------------------------
六、equal 與 mismatch 的兩種版本 (重要區別)
--------------------------------------------------------------------------------
  C++03 風格 (3-iterator):
      std::equal(a.begin(), a.end(), b.begin())
    → 只用一條 (first1, last1) 與第二條的 first2,不檢查 b 長度。
    → 若 b 比 a 短 → UB (越界讀取)。

  C++14 起 (4-iterator):
      std::equal(a.begin(), a.end(), b.begin(), b.end())
    → 同時帶兩端,長度不同直接回 false。
    → 安全版,務必優先使用。

  mismatch 同理也有 3/4-iterator 兩版。

--------------------------------------------------------------------------------
七、find 系列的細微差異
--------------------------------------------------------------------------------
  find       : 用 == 比較
  find_if    : 用 predicate 比較
  find_if_not: 取反 predicate

  ★ 不要混淆 find_first_of vs search:
    - find_first_of: 在 [first1, last1) 找「第一個出現在 [first2, last2) 中」的元素
                     (字符集模式)
    - search:       在 [first1, last1) 找「子序列 [first2, last2) 的位置」
                     (子串模式)

--------------------------------------------------------------------------------
八、search 與 C++17 Searcher 物件
--------------------------------------------------------------------------------
  傳統:
      std::search(text.begin(), text.end(), pat.begin(), pat.end());

  C++17 起,可帶第三個參數選擇 Searcher 演算法:
      std::default_searcher
      std::boyer_moore_searcher           (適合長 pattern)
      std::boyer_moore_horspool_searcher
  範例:
      auto s = std::boyer_moore_searcher(pat.begin(), pat.end());
      std::search(text.begin(), text.end(), s);

--------------------------------------------------------------------------------
九、for_each 與 range-based for 的取捨
--------------------------------------------------------------------------------
  - 簡單走訪 + 不需傳 functor 物件 → 用 range-based for 更直觀。
  - 需要把 functor 物件帶有狀態 (例如統計次數) → for_each 回傳 functor,
    可以拿回累積結果:
        auto cnt = std::for_each(v.begin(), v.end(), Counter{});
        std::cout << cnt.value();
  - 支援執行策略 (par/seq) → 只能用 for_each (C++17),range-for 不能。

--------------------------------------------------------------------------------
十、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. equal/mismatch 用 3-iterator 版本沒檢查第二條長度 → 越界讀取。
  2. 把「找不到」誤判:必須與 last 比較,不要與 nullptr 或 npos 比。
  3. find 對「未排序資料」是 O(N);若要 O(log N),先排序並用 binary_search 章節。
  4. all_of 對空 range 回 true 容易誤判;業務邏輯先檢查 .empty()。
  5. adjacent_find 比較「位置相鄰」的元素,不是「值相鄰」。
  6. search 預設用 == 比較,若要不分大小寫等,要傳自訂 BinaryPredicate。
  7. 字串搜尋若資料大且查詢頻繁,改用 std::string::find 或 Boyer-Moore searcher。

--------------------------------------------------------------------------------
十一、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 驗證請求參數全部非空 → all_of。
  - 偵測 log 中是否有任一錯誤訊息 → any_of。
  - 找特定使用者 ID → find / find_if。
  - 統計符合條件的記錄數 → count_if。
  - 比較兩份設定檔差異點 → mismatch。
  - 在 log 中尋找錯誤序列 (子串) → search。
  - 找出檔案內容是否含敏感字 (字符集) → find_first_of。
================================================================================
*/
#include <algorithm>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_predicates() {
    std::cout << "\n[demo_predicates]\n";
    std::vector<int> v{1, 2, 3, 4, 5};

    std::cout << "  all_of >0 ? "
              << std::all_of(v.begin(), v.end(), [](int x) { return x > 0; }) << "\n";
    std::cout << "  any_of even ? "
              << std::any_of(v.begin(), v.end(), [](int x) { return (x % 2) == 0; }) << "\n";
    std::cout << "  none_of <0 ? "
              << std::none_of(v.begin(), v.end(), [](int x) { return x < 0; }) << "\n";

    // 空 range 的特殊邏輯:vacuous truth
    std::vector<int> empty;
    std::cout << "  empty all_of >0 ? "
              << std::all_of(empty.begin(), empty.end(), [](int x) { return x > 0; })
              << "  (vacuous true)\n";
}

static void demo_find_and_adjacent() {
    std::cout << "\n[demo_find_and_adjacent]\n";
    std::vector<int> v{1, 2, 3, 3, 4};

    auto it = std::find(v.begin(), v.end(), 3);
    if (it != v.end()) std::cout << "  find(3) index=" << (it - v.begin()) << "\n";

    auto it2 = std::adjacent_find(v.begin(), v.end());
    if (it2 != v.end()) std::cout << "  adjacent_find first dup=" << *it2 << "\n";
}

static void demo_equal_mismatch() {
    std::cout << "\n[demo_equal_mismatch]\n";
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{1, 2, 4};

    // 4-iterator 版本 (安全)
    std::cout << "  equal? " << std::equal(a.begin(), a.end(), b.begin(), b.end()) << "\n";

    auto mm = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    if (mm.first != a.end()) {
        std::cout << "  mismatch: a=" << *mm.first << ", b=" << *mm.second << "\n";
    }
}

int main() {
    demo_predicates();
    demo_find_and_adjacent();
    demo_equal_mismatch();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
