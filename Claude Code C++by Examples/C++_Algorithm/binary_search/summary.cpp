/*
================================================================================
【C++_Algorithm/binary_search/summary.cpp】
本章末總整理 — 二分搜尋家族 (Binary Search Family)
================================================================================

本檔僅為章末教科書式總整理:整理觀念、API 對照、選擇準則、複雜度與陷阱。
依規則 8,本檔不包含任何 LeetCode 題目實作;LeetCode 題目放在各主題檔。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Binary_search_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、本章涵蓋的四個 API
--------------------------------------------------------------------------------
  std::binary_search(first, last, value [, comp])
    → bool。資料中是否存在 value。
    → 只回答「有沒有」,不告訴你「在哪」。

  std::lower_bound(first, last, value [, comp])
    → iterator。指向「第一個 >= value」的位置。
    → 若所有元素都 < value,回傳 last。
    → 等價於「value 可以插入而仍保持有序」的最左插入點。

  std::upper_bound(first, last, value [, comp])
    → iterator。指向「第一個 > value」的位置。
    → 若所有元素都 <= value,回傳 last。
    → 等價於「value 可以插入而仍保持有序」的最右插入點。

  std::equal_range(first, last, value [, comp])
    → std::pair<iterator, iterator>。
    → 等價於 { lower_bound, upper_bound },一次取得「等於 value」的半開區間。

--------------------------------------------------------------------------------
二、共通前置條件 (Preconditions)
--------------------------------------------------------------------------------
  1. 資料必須已就 comp 排序。預設 comp = std::less<T>。
     - 未排序就呼叫 → 行為未定義 (UB),不會丟例外、不會回傳錯誤值。
  2. 二分搜尋需要的迭代器類別:
     - 邏輯上需 ForwardIterator 即可 (cppreference 明定 LegacyForwardIterator)。
     - 但在非 RandomAccessIterator 上,雖然「比較次數」仍是 O(log N),
       走訪到中點仍是 O(N) (要逐步 ++),所以總時間退化為 O(N)。
     - 結論:對 std::list (BidirectionalIterator) 用 lower_bound 不會出錯,
       但效能跟 std::find 同等級 (O(N))。
  3. 比較器 comp 必須符合 strict weak ordering:
     - 不可寫 `<=`,要寫 `<` (或符合嚴格弱序的等價形式)。
     - 排序時用什麼 comp,二分搜尋就要用同一個 comp。

--------------------------------------------------------------------------------
三、複雜度 (Complexity)
--------------------------------------------------------------------------------
  - 比較次數:O(log N)
  - 迭代器位移:RandomAccessIterator → O(log N) 時間
                ForwardIterator       → O(N) 時間 (步進不能跳)
  - 空間:O(1)
  - 標記:C++20 起全部為 constexpr。

--------------------------------------------------------------------------------
四、回傳語意速查 (Return-value semantics)
--------------------------------------------------------------------------------
  資料:1 2 2 2 3 5 8  (index 0..6)

         查找值 →   2          4          0          9
  lower_bound      index 1    index 4    index 0    end (7)
  upper_bound      index 4    index 4    index 0    end (7)
  binary_search    true       false      false      false
  equal_range      [1,4)      [4,4)      [0,0)      [7,7)
                   重複 3 個   不存在     不存在     不存在

  口訣:
    - lower_bound 找「插入點 (最左)」也就是「第一個不小於 value」。
    - upper_bound 找「插入點 (最右)」也就是「第一個嚴格大於 value」。
    - upper - lower = value 出現次數。

--------------------------------------------------------------------------------
五、選擇準則 (Which API to choose?)
--------------------------------------------------------------------------------
  - 只要知道存不存在        → binary_search
  - 要拿到位置 / 要做插入   → lower_bound (最常用)
  - 要算「等於 value」幾個 → upper_bound - lower_bound 或 equal_range
  - 要找「最後一個 <= x」  → upper_bound(x) - 1   (若不等於 begin)
  - 要找「最後一個 <  x」  → lower_bound(x) - 1   (若不等於 begin)
  - 要找「第一個  >  x」  → upper_bound(x)
  - 要找「第一個 >= x」  → lower_bound(x)

  注意:做「- 1」之前必須先確認 iterator != begin,否則退到非法位置。

--------------------------------------------------------------------------------
六、容易踩的坑 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. 沒排序就用 → UB。
     - 若是「以鍵排序但帶 payload」,要確認排序與查找用同一個 comp。

  2. 把 binary_search 當「找位置」用 → 它只回 bool。
     - 想拿位置請用 lower_bound 並比較 *it == value (注意 == 比較是另一回事,
       lower_bound 內部只用 comp,沒有用 ==)。

  3. 用 list / set 的 begin/end 餵 std::lower_bound:
     - 不會錯但慢 (O(N))。
     - std::set 內建 .lower_bound() 成員函式,時間 O(log N),要用容器版本。

  4. 比較器與排序器不一致:
     - 例如 std::sort 用 lambda A,但二分搜尋用 lambda B → UB。

  5. 用 binary_search 找浮點數:
     - 浮點等值比較危險,通常要寫範圍判斷 (用 lower_bound + 容差) 或排序後線性掃。

  6. 異質比較 (Heterogeneous lookup):
     - C++14 起,lower_bound/upper_bound/equal_range 接受比較器只能拿
       「range value 與 value」比較;但 std::set/std::map 的成員函式版本支援
       透明比較器 (transparent comparator,如 std::less<>) 才能異質鍵查找。

  7. partition_point 與 lower_bound 是兄弟:
     - 兩者都是 O(log N) 二分;差別只在「拿值比較」vs「拿一元述詞 (predicate)」。
     - 找「第一個讓 pred 為 false 的元素」用 partition_point。

--------------------------------------------------------------------------------
七、與容器二分搜尋成員的差異
--------------------------------------------------------------------------------
  std::set / std::map / std::multiset / std::multimap 都有同名成員:
      .lower_bound() / .upper_bound() / .equal_range() / .find() / .count()
      .contains()    (C++20)

  優先用成員函式:
    - 對紅黑樹是 O(log N);用 std 自由函式是退化的 O(N) (Bidirectional Iter)。
    - 成員函式支援異質鍵 (透明比較器);自由函式不支援。

--------------------------------------------------------------------------------
八、實務工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 已排序設定檔關鍵字 → 確認某 key 是否被支援 (binary_search)。
  - 排序事件 log,依時間戳找特定區間 → equal_range / lower_bound + upper_bound。
  - 維持「有序 vector 取代 set」結構 → 插入用 lower_bound 找插入點。
  - 二分答案 (Binary search on answer) → 對單調可行性函式做二分。
  - 浮點區間統計 → 取代 multiset.count,用排序 vector + equal_range。
================================================================================
*/
#include <algorithm>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_binary_search_family() {
    std::cout << "\n[demo_binary_search_family]\n";

    // 已排序、含重複元素;這是觀察四個 API 差異的最好資料
    std::vector<int> v{1, 2, 2, 2, 3, 5, 8};

    // (1) binary_search:只回 true / false
    std::cout << "  binary_search(2)=" << std::binary_search(v.begin(), v.end(), 2) << "\n";
    std::cout << "  binary_search(4)=" << std::binary_search(v.begin(), v.end(), 4) << "\n";

    // (2) lower_bound / upper_bound:取得邊界 iterator
    auto lb = std::lower_bound(v.begin(), v.end(), 2);
    auto ub = std::upper_bound(v.begin(), v.end(), 2);
    std::cout << "  lower_bound(2) index=" << (lb - v.begin()) << "\n";  // 1
    std::cout << "  upper_bound(2) index=" << (ub - v.begin()) << "\n";  // 4
    std::cout << "  count of 2 = " << (ub - lb) << "\n";                 // 3

    // (3) equal_range:一次取兩個邊界
    auto range = std::equal_range(v.begin(), v.end(), 2);
    std::cout << "  equal_range(2) = [" << (range.first - v.begin())
              << "," << (range.second - v.begin()) << ")\n";

    // (4) 衍生用法:找「最後一個 <= 4」
    //     技巧:upper_bound(4) 指向「第一個 > 4」,前一格就是「最後一個 <= 4」
    auto it = std::upper_bound(v.begin(), v.end(), 4);
    if (it != v.begin()) {
        --it;
        std::cout << "  last <=4 is " << *it << " at index " << (it - v.begin()) << "\n";
    }

    // (5) 找不到的情況:lower_bound 回 end → 必須先檢查
    auto miss = std::lower_bound(v.begin(), v.end(), 100);
    if (miss == v.end()) std::cout << "  100 not found (lower_bound = end)\n";
}

int main() {
    demo_binary_search_family();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
