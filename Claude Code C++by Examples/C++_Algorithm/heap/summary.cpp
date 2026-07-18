/*
================================================================================
【C++_Algorithm/heap/summary.cpp】
本章末總整理 — Heap 演算法家族
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Heap_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、Heap 是什麼?
--------------------------------------------------------------------------------
  Heap 是一個「滿足堆性質 (heap property) 的陣列表示法」:
    對 max-heap,任意 i: arr[i] >= arr[2i+1] 且 arr[i] >= arr[2i+2]
  也就是「父節點不小於子節點」,因此 arr[0] (front) 是最大值。

  ⚠️ 重要觀念:heap 不是「容器」也不是「排好序的陣列」。
     - heap 只保證 front 是極值;其餘位置「不是」按值大小排序。
     - 若需要「完整有序」,要呼叫 sort_heap 把它變回有序序列。

--------------------------------------------------------------------------------
二、本章涵蓋的 API
--------------------------------------------------------------------------------
  std::make_heap(first, last [, comp])
    → 把任意 range 轉成 heap。

  std::push_heap(first, last [, comp])
    → 把「已 push_back 到 last - 1 位置的新元素」上浮到正確位置。
    → 前置條件:[first, last - 1) 已是 heap。

  std::pop_heap(first, last [, comp])
    → 把 *first 與 *(last - 1) 交換,然後讓 [first, last - 1) 重新成為 heap。
    → 注意:這只是「邏輯彈出」,容器大小沒變。要真正移除請再呼叫
      container.pop_back()。

  std::sort_heap(first, last [, comp])
    → 把 heap 轉成有序序列;呼叫後 [first, last) 是排序好的,heap 性質被破壞。

  std::is_heap(first, last [, comp])             → bool
  std::is_heap_until(first, last [, comp])       → iterator
    → 找出 heap 性質第一個被破壞的位置。

--------------------------------------------------------------------------------
三、複雜度 (Complexity)
--------------------------------------------------------------------------------
  API           時間複雜度    說明
  ────────────  ────────────  ──────────────────────────────
  make_heap     O(N)          線性時間 (Floyd 建堆法)
  push_heap     O(log N)      只需從底層往上 sift up
  pop_heap      O(log N)      換到尾後,從頂端 sift down
  sort_heap     O(N log N)    其實就是 heapsort
  is_heap       O(N)          掃一遍

  ★ 一個常見誤解:make_heap 是 O(N log N)? 不是,是 O(N)。
    證明用「每層節點數 × 該層最大下沉距離」的求和會得到 O(N)。

--------------------------------------------------------------------------------
四、迭代器需求
--------------------------------------------------------------------------------
  全部需要 RandomAccessIterator:
    - 可用於:std::vector / std::deque / std::array / 原生陣列
    - 不可用於:std::list / std::forward_list

--------------------------------------------------------------------------------
五、預設 max-heap、如何切成 min-heap?
--------------------------------------------------------------------------------
  預設 comp = std::less<T> → max-heap (front 為最大)。
  傳入 std::greater<>{} → min-heap (front 為最小)。

  示範:
      std::make_heap(v.begin(), v.end(), std::greater<>{});  // min-heap
      std::pop_heap(v.begin(), v.end(), std::greater<>{});   // 同樣比較器!

  ★ 強制:同一組 heap 操作必須用同一個 comp,否則 UB。

--------------------------------------------------------------------------------
六、heap 與 std::priority_queue 的關係
--------------------------------------------------------------------------------
  std::priority_queue 是建立在 heap 演算法之上的「容器轉接器」:
    - 內部預設用 std::vector<T> 與 std::less<T> → max-heap
    - 提供 push/pop/top/empty/size,封裝了 make_heap/push_heap/pop_heap

  何時用 heap 演算法 vs priority_queue?
    - 想用「現成 container 介面」→ priority_queue。
    - 想直接操作 vector,或要混用其他 vector API (例如批次 reserve、
      iterator 遍歷、make_heap 後再轉 sort_heap) → 用 heap 演算法。

--------------------------------------------------------------------------------
七、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. pop_heap 不會真的刪元素:
       std::pop_heap(v.begin(), v.end());   // 只把極值移到尾端
       int top = v.back();
       v.pop_back();                         // 真正移除,這兩步要配對

  2. push_heap 的前置條件:
       新元素要先 push_back 到 vector,再呼叫 push_heap。
       不是先 push_heap 再放元素。

       v.push_back(10);
       std::push_heap(v.begin(), v.end());

  3. 改了 heap 中間元素就破壞了 heap 性質:
       v[5] = 999;  // 不能保證仍是 heap → 必須 make_heap 重建。

  4. 兩次操作 comp 不一致:
       make_heap(...) 用預設 less,pop_heap(...) 用 greater → UB。

  5. sort_heap 之後 heap 性質消失:
       排序後若還要 heap 操作,必須再 make_heap。

  6. 與 std::sort 比效率:
       一次性整段排序,std::sort 通常比 make_heap + sort_heap 快。
       Heap 的優勢在「動態維護 top」,而不是「一次排完」。

  7. 自訂物件用 heap:
       comp 要符合 strict weak ordering;若用成員指標當 key,記得處理 nullptr。

--------------------------------------------------------------------------------
八、選擇準則速查表
--------------------------------------------------------------------------------
  需求                                          選擇
  ──────────────────────────────────────────  ──────────────────────
  動態維護「目前最大/最小」                      priority_queue 或 vector + heap
  Top-K (動態流) 取前 K 大                       min-heap of size K
  一次性排序                                     std::sort (比 heapsort 快)
  Heapsort 教學用                                make_heap + sort_heap
  確認某 vector 是否合法 heap                    is_heap / is_heap_until
  以 heap 為 base 做 Dijkstra / Prim             min-heap (priority_queue 即可)

--------------------------------------------------------------------------------
九、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 任務排程:依優先度動態加入/取出任務 → priority_queue。
  - 即時統計 Top-K:資料流中只保留前 K 名 → min-heap size K。
  - Median maintenance:同時維護左半 (max-heap) 與右半 (min-heap)。
  - 多路合併 (k-way merge):用 min-heap 從 k 個來源輪流取最小元素。
================================================================================
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

static void print(const std::vector<int>& v, const char* label) {
    std::cout << label << ":";
    for (int x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_heap_ops() {
    std::cout << "\n[demo_heap_ops]\n";

    std::vector<int> v{3, 1, 4, 2, 8, 5};
    std::make_heap(v.begin(), v.end());  // 預設 max-heap
    print(v, "  after make_heap");
    std::cout << "  heap top(front)=" << v.front() << "\n";
    std::cout << "  is_heap? " << std::is_heap(v.begin(), v.end()) << "\n";

    // push 流程:先 push_back,再 push_heap
    v.push_back(10);
    std::push_heap(v.begin(), v.end());
    print(v, "  after push_heap(10)");

    // pop 流程:pop_heap 移到尾端,再 pop_back 真正刪除
    std::pop_heap(v.begin(), v.end());
    int popped = v.back();
    v.pop_back();
    print(v, "  after pop_heap + pop_back");
    std::cout << "  popped=" << popped << "\n";

    // sort_heap:把 heap 轉成已排序序列 (heap 性質會消失)
    std::sort_heap(v.begin(), v.end());
    print(v, "  after sort_heap (sorted)");
    std::cout << "  is_heap? (應為 0 或 1,看是否還滿足) "
              << std::is_heap(v.begin(), v.end()) << "\n";
}

static void demo_min_heap() {
    std::cout << "\n[demo_min_heap]\n";

    // 用 greater 比較器建立 min-heap
    std::vector<int> v{3, 1, 4, 2, 8, 5};
    std::make_heap(v.begin(), v.end(), std::greater<>{});
    std::cout << "  min-heap top = " << v.front() << "\n";  // 1
}

int main() {
    demo_heap_ops();
    demo_min_heap();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
