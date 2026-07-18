// ============================================================
// std::is_heap / std::is_heap_until    (C++11 起)
// 分類 (Category): Heap operations (堆積操作)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/is_heap
//   * https://en.cppreference.com/w/cpp/algorithm/is_heap_until
//   * https://cplusplus.com/reference/algorithm/is_heap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// 兩個兄弟函式 (與 is_sorted / is_sorted_until 同樣的兄弟模式):
//
//   ┌───────────────┬─────────────────────────────────────┐
//   │ is_heap       │ 回 bool — 「整段是不是合法 heap」    │
//   │ is_heap_until │ 回迭代器 — 「第一個破壞 heap 的位置」│
//   └───────────────┴─────────────────────────────────────┘
//
// 兩者本質是同一個演算法,差別在於回傳的資訊:
//
//   is_heap == (is_heap_until(...) == last)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、用途                                                   │
// └────────────────────────────────────────────────────────────┘
//
//   * 「assert / 不變式檢查」 — 確認 heap 操作後容器仍是合法 heap。
//   * 「對外接收 heap」時的 sanity check — 例如把外部資料當成 heap 使用前。
//   * 「找出 heap 的有效前綴」 — 例如部分元素已 heap 化、後段是 raw 尾巴,
//     用 is_heap_until 知道從哪裡開始要重建。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、預設是 max-heap                                        │
// └────────────────────────────────────────────────────────────┘
//
// 預設用 operator< 判定 max-heap (父 ≥ 子)。
// 想檢查 min-heap → 傳 std::greater<>:
//
//   std::is_heap(v.begin(), v.end(), std::greater<int>{});
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   bool is_heap(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   bool is_heap(RandomIt first, RandomIt last, Compare comp);
//
//   template <class RandomIt>
//   RandomIt is_heap_until(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   RandomIt is_heap_until(RandomIt first, RandomIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與邊界                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) 比較
//   空間: O(1)
//
//   邊界:空範圍與單元素 → is_heap 回 true (vacuous truth)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 預設檢查 max-heap;min-heap 請傳 std::greater。
//   2. comp 必須與 make_heap / push_heap / pop_heap 用的相同。
//   3. is_heap_until 回傳「第一個破壞」位置 — 對 debug 很有用。
//   4. 線上系統可在 debug build 加 assert,正式發行版可條件編譯掉。
//
// ============================================================

/*
補充筆記：std::is_heap
  - is_heap 只檢查 range 是否滿足 heap property，不會改動元素。
  - 預設檢查最大堆；若建立 heap 時用了自訂比較器，檢查時也要用同一個比較器。
  - 它適合在教學或 debug 中驗證 make_heap/push_heap/pop_heap 後的不變式。
  - std::is_heap 操作的是 heap 性質，不是 std::priority_queue 物件；資料通常仍放在 vector 這類 random access range 中。
  - heap 預設是 max-heap，front() 會是最大元素；若要 min-heap，要提供反向比較器並在所有 heap 操作中保持一致。
  - make_heap 把既有範圍整理成 heap；push_heap 假設新元素已 push_back 到尾端；pop_heap 會把最大元素換到尾端但不移除它。
  - pop_heap 後通常要再呼叫 container.pop_back() 才真的刪掉元素；漏掉這步會讓已彈出的值仍留在容器裡。
  - heap 只保證父節點和子節點的局部順序，不保證整個陣列排序；不要拿 heap 內部順序當 sorted list 使用。
  - sort_heap 會破壞 heap 結構並產生排序結果；排序後若還要繼續 push_heap，必須重新 make_heap。
*/
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::cout << std::boolalpha;

    // --- 範例 1: 已是 max-heap ---
    std::vector<int> a{9, 6, 4, 3, 5, 1, 2, 1};
    std::cout << "a is max-heap: "
              << std::is_heap(a.begin(), a.end()) << '\n';

    // --- 範例 2: 不是 heap (升序排列,根 1 比子 2,3 小) ---
    std::vector<int> b{1, 2, 3, 4, 5};
    std::cout << "b is max-heap: "
              << std::is_heap(b.begin(), b.end()) << '\n';
    auto it = std::is_heap_until(b.begin(), b.end());
    std::cout << "b heap-until index = " << (it - b.begin())
              << " (val=" << *it << ")\n";

    // --- 範例 3: min-heap 檢查 ---
    std::vector<int> c{1, 2, 3, 5, 4};
    std::cout << "c is min-heap: "
              << std::is_heap(c.begin(), c.end(), std::greater<int>{}) << '\n';

    // --- 範例 4: 空範圍 → true ---
    std::vector<int> e;
    std::cout << "empty is heap: "
              << std::is_heap(e.begin(), e.end()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_validate_max_heap();
    void practical_assert_invariant();
    void practical_is_heap_until_locate_tail();
    void leetcode_958_complete_binary_tree_heap_property();
    void practical_unit_test_heap_invariant();
    leetcode_validate_max_heap();
    practical_assert_invariant();
    practical_is_heap_until_locate_tail();
    leetcode_958_complete_binary_tree_heap_property();
    practical_unit_test_heap_invariant();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 風格驗證題:判斷一棵以陣列表示的完全二元樹是否為 max-heap
// ----------------------------------------------------------------
// 題目:給陣列 nums (索引 i 子節點為 2i+1 / 2i+2),判斷是否為 max-heap。
//
// 為什麼用 std::is_heap + std::is_heap_until:
//   * is_heap 直接回答 yes/no — O(N)。
//   * is_heap_until 找出第一個「違反 heap property」的位置 — 偵錯有用。
//
// 複雜度:時間 O(N);空間 O(1)。
void leetcode_validate_max_heap() {
    std::vector<int> good{9, 5, 8, 3, 4, 7, 6};
    std::vector<int> bad {9, 5, 8, 3, 4, 7, 10};

    std::cout << std::boolalpha;
    std::cout << "LCval good: " << std::is_heap(good.begin(), good.end()) << '\n';
    std::cout << "LCval bad : " << std::is_heap(bad.begin(),  bad.end())  << '\n';
    auto it = std::is_heap_until(bad.begin(), bad.end());
    std::cout << "LCval bad first violation at index "
              << (it - bad.begin()) << " (val=" << *it << ")\n";
}

// ----------------------------------------------------------------
// 實務範例 1:debug 模式 assert 確保容器是 heap
// ----------------------------------------------------------------
// 場景:使用 push_heap / pop_heap 維護 vector,擔心其他程式碼意外破壞。
//      在 debug build 加 assert(std::is_heap(...)) 第一時間捕獲錯誤。
void practical_assert_invariant() {
    std::vector<int> heap{9, 6, 4, 3, 5, 1, 2, 1};
    assert(std::is_heap(heap.begin(), heap.end()));   // 通過

    heap.push_back(10);
    std::push_heap(heap.begin(), heap.end());
    assert(std::is_heap(heap.begin(), heap.end()));   // 仍然通過
    std::cout << "practical assert: invariant maintained, top=" << heap.front() << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:用 is_heap_until 量化「已 heap 的前綴長度」
// ----------------------------------------------------------------
// 場景:部分元素已 heap 化,後段是新近 push_back 但未 heapify 的尾巴。
//      用 is_heap_until 知道從哪裡開始要重建 heap。
void practical_is_heap_until_locate_tail() {
    std::vector<int> v{9, 6, 4, 3, 5, 1, 2,  10, 11};
    auto it = std::is_heap_until(v.begin(), v.end());
    std::cout << "practical: heap prefix length = " << (it - v.begin())
              << ", need re-heapify from index " << (it - v.begin()) << '\n';
    std::make_heap(v.begin(), v.end());
    std::cout << "practical: after re-make_heap, is_heap = "
              << std::is_heap(v.begin(), v.end()) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 風格題:在陣列序列化的完全二元樹上檢查 min-heap property
// ----------------------------------------------------------------
// 題目:給一個陣列代表完全二元樹 (層序),判斷是否為 min-heap。
//
// 為什麼用 std::is_heap (with std::greater<int>{}):
//   標準函式庫預設檢查 max-heap;傳入 std::greater 即等價檢查 min-heap。
//   不需自己寫迴圈比較父子關係。
//
// 複雜度:時間 O(N);空間 O(1)。
void leetcode_958_complete_binary_tree_heap_property() {
    std::vector<int> tree1{1, 3, 5, 7, 9, 6, 8};   // min-heap
    std::vector<int> tree2{1, 3, 5, 7, 9, 4, 8};   // 4 < parent 5 → 不是 min-heap

    std::cout << std::boolalpha;
    std::cout << "LC958 tree1 is min-heap: "
              << std::is_heap(tree1.begin(), tree1.end(), std::greater<int>{}) << '\n';
    std::cout << "LC958 tree2 is min-heap: "
              << std::is_heap(tree2.begin(), tree2.end(), std::greater<int>{}) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:單元測試 — 驗證自製 priority_queue 的 invariant
// ----------------------------------------------------------------
// 場景:你寫了一個自製 priority_queue (內部用 vector),
//      在單元測試裡每次操作後都用 is_heap 驗證內部仍是 heap,
//      第一時間捕獲實作 bug。
void practical_unit_test_heap_invariant() {
    std::vector<int> pq;
    auto push = [&](int v) {
        pq.push_back(v);
        std::push_heap(pq.begin(), pq.end());
    };
    auto pop = [&]() {
        std::pop_heap(pq.begin(), pq.end());
        int top = pq.back();
        pq.pop_back();
        return top;
    };
    push(5); push(1); push(8); push(3);
    bool ok1 = std::is_heap(pq.begin(), pq.end());
    int t = pop();
    bool ok2 = std::is_heap(pq.begin(), pq.end());
    std::cout << std::boolalpha
              << "unit test: after pushes is_heap=" << ok1
              << ", popped=" << t
              << ", after pop is_heap=" << ok2 << '\n';
}

// === 預期輸出 (Expected output) ===
// a is max-heap: true
// b is max-heap: false
// b heap-until index = 1 (val=2)
// c is min-heap: true
// empty is heap: true
// LCval good: true
// LCval bad : false
// LCval bad first violation at index 6 (val=10)
// practical assert: invariant maintained, top=10
// practical: heap prefix length = 7, need re-heapify from index 7
// practical: after re-make_heap, is_heap = true
// LC958 tree1 is min-heap: true
// LC958 tree2 is min-heap: false
// unit test: after pushes is_heap=true, popped=8, after pop is_heap=true
