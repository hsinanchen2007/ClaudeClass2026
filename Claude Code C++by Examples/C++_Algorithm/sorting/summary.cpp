/*
================================================================================
【C++_Algorithm/sorting/summary.cpp】
本章末總整理 — 排序家族 (Sorting Family)
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Sorting_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、本章涵蓋的 API
--------------------------------------------------------------------------------
  std::sort(first, last [, comp])
    → 整段排序;不保證穩定 (相等元素的相對順序可能被改變)。

  std::stable_sort(first, last [, comp])
    → 整段排序;保證穩定。

  std::partial_sort(first, middle, last [, comp])
    → 只把「前 k = middle - first 個最小元素」排到 [first, middle),
       且這 k 個內部有序;後段 [middle, last) 的元素順序不保證。

  std::partial_sort_copy(first, last, d_first, d_last [, comp])
    → 把原 range 排序後的前 k 個寫到目的 range (不修改原 range)。

  std::nth_element(first, nth, last [, comp])
    → 把「第 n 個位置」放上排序後該位置應在的元素;
       [first, nth) 都 <= *nth;[nth+1, last) 都 >= *nth;兩側內部不保序。

  std::is_sorted(first, last [, comp])              → bool
  std::is_sorted_until(first, last [, comp])        → iterator (首個破壞順序處)

--------------------------------------------------------------------------------
二、複雜度與穩定性 (Complexity & Stability)
--------------------------------------------------------------------------------
  API                  時間 (平均/最壞)            穩定?  額外空間
  ──────────────────  ──────────────────────────  ─────  ────────
  sort                 O(N log N) (最壞也保證)     不      O(log N) 堆疊
                       C++11 起最壞也是 O(N log N)
                       (typical: introsort)
  stable_sort          O(N log² N) (最壞)          穩定    可能 O(N) (記憶體足夠時 O(N log N))
                       O(N log N) (記憶體足夠)
  partial_sort         O(N log k) (k = middle-first) 不    O(1)
  partial_sort_copy    同上,但只寫到 d_first      不      O(1)
  nth_element          O(N) 平均;最壞 O(N²) 但    不      O(log N)
                       多數實作用 introselect
  is_sorted            O(N)                          —     O(1)

  記憶口訣:
    - 要全部排好 → sort (除非要穩定)。
    - 只要前 k 個 → partial_sort 或 partial_sort_copy。
    - 只要「第 k 個」(中位數、第 K 大) → nth_element,O(N) 最快。
    - 要保留等值順序 → stable_sort。

--------------------------------------------------------------------------------
三、迭代器需求 (Iterator Requirements)
--------------------------------------------------------------------------------
  sort / stable_sort / partial_sort / nth_element 需要 RandomAccessIterator (能 +n、-n、it1-it2)；
  但 is_sorted / is_sorted_until 只需要 ForwardIterator（forward_list 也能用）。
  - 適用容器:std::vector / std::deque / std::array / 原生陣列 / std::string
  - 不適用:std::list / std::forward_list (它們提供成員函式 .sort())
  - std::set/std::map 已天然有序,不需排序演算法

--------------------------------------------------------------------------------
四、穩定 vs 不穩定 — 何時非選 stable_sort 不可?
--------------------------------------------------------------------------------
  當有 secondary key 並希望「等值依原順序」保留時:
    - 例如先按 (字典序) 排序,再按 (長度) stable_sort,等長者保留字典序。
    - 多階段排序 (multi-key sort) 的通用做法是「最次要鍵先排,最重要鍵最後排
      (用 stable_sort)」。
  反之若資料中相等元素「不可區分」(例如整數),用 sort 即可,通常快 1.5~2x。

--------------------------------------------------------------------------------
五、比較器 (Comparator)
--------------------------------------------------------------------------------
  - 必須是 strict weak ordering:
      - 不可寫 `a <= b`;要寫 `a < b`。
      - irreflexive: !comp(a, a)
      - antisymmetric: comp(a,b) → !comp(b,a)
      - transitive
      - 不相等性也要 transitive: equiv(a,b) && equiv(b,c) → equiv(a,c)
  - 違反 → UB (常見現象:當機、無窮迴圈、結果錯亂)。
  - 預設:std::less<T> (即 operator<)。
  - 降序排序常見:std::sort(v.begin(), v.end(), std::greater<>{});

--------------------------------------------------------------------------------
六、執行策略 (Execution Policy, C++17)
--------------------------------------------------------------------------------
  std::sort(std::execution::par, v.begin(), v.end());
  - std::execution::seq / par / par_unseq
  - 需要 #include <execution>;某些實作 (GCC) 需要連結 TBB。
  - 平行版本:資料量足夠大才會比 seq 快,小資料反而更慢。

--------------------------------------------------------------------------------
七、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. partial_sort 不是 nth_element:
     - partial_sort 保證「前 k 個排好」;nth_element 只保證「第 k 個就位」。
     - 想要「中位數」用 nth_element (O(N));用 partial_sort 是浪費。

  2. nth_element 之後不要假設兩邊有序:
     - 標準只保證「左邊 <= *nth <= 右邊」,左右各自不保序。

  3. std::list 不能用 std::sort:
     - 編譯不會過 (不是 RandomAccessIterator)。
     - 改用 std::list 的成員函式:lst.sort();

  4. sort 修改範圍,iterator 失效規則:
     - 排序後原本指向某個元素的 iterator 仍指向「同一格」,但格內元素已換。
     - 換言之,iterator 沒被「失效」(沒重新配置記憶體),但語意上指向不同值。

  5. 浮點數 NaN 不可比:
     - NaN < x、x < NaN、NaN == NaN 全部為 false。
     - 若資料含 NaN,std::less 不滿足 strict weak ordering → UB。
     - 對策:先過濾掉 NaN,或用自訂比較器把 NaN 視為極大或極小。

  6. partial_sort_copy 的目的端大小限制了 k:
     - k = min(distance(first,last), distance(d_first,d_last))
     - 容易忘記給目的端 resize。

  7. is_sorted 不保證會「短路」到尾;它是 O(N) 但會在第一個逆序處立刻停。
     - 想知道「第一個逆序在哪」用 is_sorted_until。

--------------------------------------------------------------------------------
八、選擇準則速查表 (Quick Decision Table)
--------------------------------------------------------------------------------
  需求                                  選擇
  ─────────────────────────────────────  ────────────────────
  整段排序,不在乎穩定性                 std::sort
  整段排序,要保留等值原順序              std::stable_sort
  只要前 k 個最小(且要排好)              std::partial_sort
  只要前 k 個(且不想改原資料)            std::partial_sort_copy
  只要「第 k 個」的值 (中位數/Top-K 邊界) std::nth_element
  確認是否已排序                          std::is_sorted
  找到第一個逆序位置                       std::is_sorted_until
  std::list                                lst.sort() 成員函式
  std::set / std::map                      容器本身有序,不需排序

--------------------------------------------------------------------------------
九、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 事件 timeline 依時間排序 → sort。
  - 取得 top-10 熱門商品 → partial_sort 或 nth_element + sort 前 k。
  - 計算中位數 → nth_element 取 mid;偶數個時取兩個再平均。
  - 多條件排序 (姓 → 名 → 生日) → 多次 stable_sort 由次要到主要鍵。
  - 確認 log 已按時間遞增 → is_sorted (大致驗證資料完整性)。
================================================================================
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】排序家族總覽 (sorting family)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 排序家族怎麼選?請給一張決策表。
//     答:全部都要有序 → sort,O(n log n),不穩定、不配置額外記憶體。
//         全部有序又要保留等價元素原順序 → stable_sort,有額外記憶體時
//         O(n log n),配置不到時退化為 O(n log² n)。
//         要前 K 名且 K 個之間也有序 → partial_sort,約 O(n log k)。
//         同上但來源不可修改 / 只有 input iterator → partial_sort_copy,
//         k = min(來源大小, 目的區間大小)。
//         只要「是哪 K 個」、第 k 大或中位數 → nth_element,標準保證平均 O(n)。
//         只是要檢查有沒有排序 → is_sorted / is_sorted_until,O(n)。
//     追問:k 接近 n 時呢?(部分排序失去優勢,直接 sort)
//
// 🔥 Q2. 這一家人裡誰保證穩定?
//     答:只有 stable_sort。sort、partial_sort、partial_sort_copy、nth_element
//         都不保證 — 它們都靠 swap / partition 搬移元素。需要多鍵語意時,
//         要嘛用 stable_sort,要嘛把 tie-breaker 直接寫進 comparator
//         (std::tie(a.k1, a.k2) < std::tie(b.k1, b.k2))。
//     追問:「穩定」是否代表 iterator 會跟著元素走?
//           (不代表 — 值一樣整批搬移,穩定只描述等價元素之間的先後關係)
//
// 🔥 Q3. 這一家人共同的前置條件是什麼?
//     答:comparator 必須構成 strict weak ordering:irreflexive、asymmetric、
//         transitive,且 equivalence 本身也要有傳遞性。最常見的違反是寫
//         return a <= b;,後果是 undefined behavior (可能 crash,而不只是
//         順序錯)。另一個常被忽略的例子是含 NaN 的浮點資料 — NaN 與任何值的
//         比較都為 false,std::less 因此不構成 strict weak ordering。
//     追問:怎麼 debug?(-D_GLIBCXX_DEBUG 會檢查 comparator 的 irreflexivity)
//
// Q4. 哪些成員需要 random access iterator?
//     答:sort、stable_sort、partial_sort、nth_element 都要求 random access
//         iterator,所以 std::list / std::forward_list 不能用 (它們改用成員
//         函式 .sort())。兩個例外:is_sorted / is_sorted_until 只需要 forward
//         iterator;partial_sort_copy 的「來源端」只需要 input iterator,
//         但「目的端」仍需 random access iterator。
//     追問:std::map / std::set 為什麼也不行?
//           (除了 iterator 只是 bidirectional,元素型別是 pair<const Key, T> /
//            const key,不可賦值,而排序必須搬移元素)
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

template <class T>
static void print(const std::vector<T>& v, const char* label) {
    std::cout << label << ":";
    for (const auto& x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_sort_and_stable_sort() {
    std::cout << "\n[demo_sort_and_stable_sort]\n";

    std::vector<int> v{5, 1, 4, 2, 3};
    std::sort(v.begin(), v.end());
    print(v, "  sort");
    std::cout << "  is_sorted? " << std::is_sorted(v.begin(), v.end()) << "\n";

    // 示範 stable_sort:用 (key, original_index),只依 key 排,index 觀察穩定性
    std::vector<std::pair<int, int>> p{{2, 0}, {1, 1}, {2, 2}, {1, 3}};
    std::stable_sort(p.begin(), p.end(),
                     [](auto a, auto b) { return a.first < b.first; });
    std::cout << "  stable_sort by key =>";
    for (auto kv : p) std::cout << " (" << kv.first << "," << kv.second << ")";
    std::cout << "\n";  // 期望:(1,1) (1,3) (2,0) (2,2) — 同 key 內保留原順序
}

static void demo_partial_sort_and_nth_element() {
    std::cout << "\n[demo_partial_sort_and_nth_element]\n";

    std::vector<int> v{9, 1, 8, 2, 7, 3, 6, 4, 5};

    // partial_sort:只把最小的 3 個排到前 3 格
    std::vector<int> a = v;
    std::partial_sort(a.begin(), a.begin() + 3, a.end());
    print(a, "  partial_sort first3");  // 前 3 個是 1 2 3,後面順序不保證

    // nth_element:把第 5 小放到 index 4,左右不保序
    std::vector<int> b = v;
    auto nth = b.begin() + 4;
    std::nth_element(b.begin(), nth, b.end());
    std::cout << "  nth_element (5th smallest) = " << *nth << "\n";

    // is_sorted_until:示範資料中第一個逆序位置
    std::vector<int> c{1, 2, 3, 2, 5};
    auto bad = std::is_sorted_until(c.begin(), c.end());
    if (bad != c.end())
        std::cout << "  is_sorted_until: first break at index "
                  << (bad - c.begin()) << " value " << *bad << "\n";
}

int main() {
    demo_sort_and_stable_sort();
    demo_partial_sort_and_nth_element();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
