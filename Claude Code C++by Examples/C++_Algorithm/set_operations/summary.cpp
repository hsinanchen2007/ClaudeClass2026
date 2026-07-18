/*
================================================================================
【C++_Algorithm/set_operations/summary.cpp】
本章末總整理 — 集合運算 (Set Operations on Sorted Ranges)
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Set_operations_.28on_sorted_ranges.29
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、重要觀念:這裡的「集合」是已排序 range,不是 std::set 容器
--------------------------------------------------------------------------------
  ★ STL 的 set_xxx 演算法操作的是「已排序的 range」(可以是 vector / array /
    deque,只要排好序就好),不是 std::set 容器。

  ★ 數學上一般「集合」沒有重複元素,但這些演算法把「多重集 (multiset)」
    當作集合處理 — 元素出現次數會影響結果。例如:
       a = {1, 2, 2, 3},b = {2, 3}
       set_intersection(a, b) → {2, 3}   (取兩邊「次數的最小值」)
       set_union(a, b)        → {1, 2, 2, 3}  (取兩邊「次數的最大值」)
       set_difference(a-b)    → {1, 2}   (a 中超出 b 的次數)

  此章節同時收錄 std::merge 與 std::inplace_merge (它們不是嚴格的集合運算,
  但都是「合併兩個已排序 range」的演算法)。

--------------------------------------------------------------------------------
二、本章涵蓋的 API
--------------------------------------------------------------------------------
  std::set_union(a1,a2,b1,b2, out [, comp])               → out 結束 iterator
  std::set_intersection(a1,a2,b1,b2, out [, comp])         → 同上
  std::set_difference(a1,a2,b1,b2, out [, comp])           → 同上
  std::set_symmetric_difference(a1,a2,b1,b2, out [, comp]) → 同上
  std::includes(a1,a2,b1,b2 [, comp])                       → bool (b 是否為 a 子集?)
  std::merge(a1,a2,b1,b2, out [, comp])                    → 合併兩排序 range
  std::inplace_merge(first, mid, last [, comp])            → 原地合併 [first,mid) 與 [mid,last)

--------------------------------------------------------------------------------
三、共通前置條件 (Preconditions)
--------------------------------------------------------------------------------
  1. 兩條輸入 range 都必須已用同一個 comp 排序 (預設 less<T>)。
  2. 輸出 range 必須有足夠空間;不知道大小時用 std::back_inserter。
  3. 輸入與輸出範圍不可重疊 (set_xxx 與 merge);違反 → UB。
  4. comp 必須是 strict weak ordering。

--------------------------------------------------------------------------------
四、各運算的「多重集」語意 (重要!)
--------------------------------------------------------------------------------
  a = {1, 2, 2, 3}
  b = {2, 2, 2, 4}

  union              → {1, 2, 2, 2, 3, 4}    (各元素次數取兩邊 max)
  intersection       → {2, 2}                (各元素次數取兩邊 min)
  difference (a-b)   → {1, 3}                (a 中超出 b 的次數)
  symmetric diff     → {1, 2, 3, 4}          (= (a-b) ∪ (b-a))
  includes(a ⊇ b)?   → false                 (b 有 3 個 2,a 只有 2 個)

  ★ 若你要的是「忽略重複」的純集合運算,先做 unique 或用 std::set/std::unordered_set。

--------------------------------------------------------------------------------
五、複雜度
--------------------------------------------------------------------------------
  API                            時間              空間
  ──────────────────────────────  ──────────────    ──────────
  set_union                       O(N1 + N2)        O(1) (輸出端不算)
  set_intersection                O(N1 + N2)        O(1)
  set_difference                  O(N1 + N2)        O(1)
  set_symmetric_difference        O(N1 + N2)        O(1)
  includes                        O(N1 + N2)        O(1)
  merge                           O(N1 + N2)        O(1)
  inplace_merge                   O(N1 + N2)        O(N) (記憶體足夠時)
                                  O(N log N) (退化)  O(1)

  ★ inplace_merge 的退化版本是 O(N log N);若 OS 給不到暫存,變慢但不會錯。

--------------------------------------------------------------------------------
六、迭代器需求
--------------------------------------------------------------------------------
  - 輸入:InputIterator (對所有 set_xxx 與 merge 都夠)
  - 輸出:OutputIterator
  - inplace_merge:BidirectionalIterator (兩段都在同一 range 內)

  常用搭配:
      std::vector<int> out;
      std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                            std::back_inserter(out));

--------------------------------------------------------------------------------
七、與 std::set 容器搭配
--------------------------------------------------------------------------------
  ★ std::set 已天然有序,可以直接作為兩個輸入 range:
      std::set<int> A{1, 2, 3};
      std::set<int> B{2, 3, 4};
      std::vector<int> out;
      std::set_intersection(A.begin(), A.end(), B.begin(), B.end(),
                            std::back_inserter(out));   // {2, 3}

  unordered_set / hash 集合不能直接用,因為 set_xxx 要求「有序」。

--------------------------------------------------------------------------------
八、merge vs inplace_merge
--------------------------------------------------------------------------------
  std::merge:
    - 輸入兩條 range,寫到第三條目的端;不修改原資料。

  std::inplace_merge:
    - 同一條 range 內,以 mid 為界分成兩個已排序段,原地合併。
    - 用於 mergesort 的 merge 步驟;或 partial_sort 之後的延伸。

--------------------------------------------------------------------------------
九、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. 沒排序就用 → UB。
     - 「我以為已排序」是最常見的 bug 來源。

  2. 兩條 range 用不同 comp 排序 → UB。
     - 若 a 是 less,b 是 greater → 結果完全錯亂。

  3. 輸出端沒空間 → UB。
     - 用 back_inserter 最安全;否則必須先 resize 到上限。

  4. 多重集行為被誤以為是「純集合去重」:
     - 若不想要重複,先 std::unique 或改用 std::set。

  5. 輸入 / 輸出 range 重疊 → UB。
     - 除非用 inplace_merge 這類專門設計的演算法。

  6. includes 不是雙向關係:
     - includes(a, b) 問「a 是否包含 b」(b ⊆ a),反向要交換參數。

  7. set_difference 不是 b - a:
     - 是 a - b;順序很重要。

--------------------------------------------------------------------------------
十、選擇準則 (Decision Table)
--------------------------------------------------------------------------------
  需求                                          選擇
  ─────────────────────────────────────────  ─────────────────────────
  兩名單共通項                                   set_intersection
  合併兩名單去重                                  set_union
  A 有但 B 沒有                                  set_difference (A - B)
  只出現在一邊的元素                              set_symmetric_difference
  B ⊆ A?                                       includes
  合併兩排序資料 (不去重)                         merge
  排序好的「左半 + 右半」合併                     inplace_merge (mergesort 用)

--------------------------------------------------------------------------------
十一、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 兩名單共同好友 → set_intersection。
  - 合併兩份排序 log → merge。
  - 權限差異 (新增/移除) → set_difference / set_symmetric_difference。
  - 子集判斷 (RBAC) → includes。
  - 合併設定鍵清單去重 → set_union。
================================================================================
*/
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

static void print(const std::vector<int>& v, const char* label) {
    std::cout << label << ":";
    for (int x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_set_ops() {
    std::cout << "\n[demo_set_ops]\n";

    // 注意:兩條都已排序。若不確定,先呼叫 sort。
    std::vector<int> a{1, 2, 2, 4, 5};
    std::vector<int> b{2, 3, 4, 4};

    std::vector<int> uni, inter, diff, sym;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(uni));
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(inter));
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(diff));
    std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(sym));

    print(uni,   "  union");                  // {1,2,2,3,4,4,5}
    print(inter, "  intersection");           // {2,4}
    print(diff,  "  difference (a-b)");       // {1,2,5}
    print(sym,   "  symmetric_difference");   // {1,2,3,4,5}

    std::vector<int> subset{2, 4};
    std::cout << "  includes(a, {2,4})? "
              << std::includes(a.begin(), a.end(), subset.begin(), subset.end()) << "\n";
}

static void demo_merge() {
    std::cout << "\n[demo_merge]\n";
    std::vector<int> a{1, 3, 5};
    std::vector<int> b{2, 4, 6};
    std::vector<int> out;
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
    print(out, "  merge");   // {1,2,3,4,5,6}
}

int main() {
    demo_set_ops();
    demo_merge();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
