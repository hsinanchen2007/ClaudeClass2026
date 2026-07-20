// ============================================================
// std::pop_heap
// 分類 (Category): Heap operations (堆積操作)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/pop_heap
//   * https://cplusplus.com/reference/algorithm/pop_heap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::pop_heap 解的問題:
//
//   「我有一個合法 heap [first, last)。把『root』(最大或最小元素)
//    取出 — 把它放到尾端 (last - 1),然後對 [first, last - 1) 重新建立 heap。」
//
// 注意:它「不會縮小容器」!最大元素只是被換到尾端,還在容器裡。
//
// 標準的「移除 heap 頂」流程是兩步:
//
//   std::pop_heap(v.begin(), v.end());   // step 1: max 換到尾端,前段重新成 heap
//   v.pop_back();                          // step 2: 真的從容器移除尾端
//
// 內部做的事是「把 root 與尾端 swap,再對新 root 做 sift-down (下沉)」 —
// 從根往下走,若新 root 小於子節點,就和較大的子節點交換,直到不再小。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼是 O(log N)?                                     │
// └────────────────────────────────────────────────────────────┘
//
// sift-down 從根走到葉子,最多走樹高 log N 步;每步常數時間。
// 總共 O(log N)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼設計成「不縮小容器」?                            │
// └────────────────────────────────────────────────────────────┘
//
// 因為 STL 演算法是「容器無關」的 — 它不知道 begin/end 是什麼容器。
// 只能透過迭代器運作,真正的「縮小」要交給容器自己 (pop_back / erase)。
//
// 這個設計也讓 pop_heap 變成「heap sort」的核心步驟:
//
//   while (last > first + 1) {
//       std::pop_heap(first, last);   // 把最大 swap 到 last - 1
//       --last;                        // 縮小 heap 範圍 (但容器不變)
//   }
//   // 結束後 [first, end) 就是排好的升序!
//
// 這就是 std::sort_heap 內部做的事。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * priority queue 取出最高優先任務
//   * Heapsort
//   * LC 1046 Last Stone Weight、LC 215 Kth Largest 等
//   * 任何需要「拿出最大/最小,再加新值」的場景
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章與需求                                         │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void pop_heap(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void pop_heap(RandomIt first, RandomIt last, Compare comp);
//
//   * 需求:RandomAccessIterator;範圍必須已是合法 heap;at least 1 元素。
//   * comp 必須與 make_heap 用的相同。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. pop_heap 不會縮小容器 — 必須配合 pop_back 才能真的移除。
//   2. 對「非 heap」呼叫 → UB。
//   3. 連續 pop_heap (不縮小) 會破壞後段元素的 heap 結構。
//   4. comp 必須與 make_heap 用的相同。
//
// ============================================================

/*
補充筆記：std::pop_heap
  - pop_heap 會把 heap 頂端極值移到 range 最後，並修復前半段 heap。
  - 它不會刪除元素；通常下一行接容器的 pop_back 才是真的移除。
  - 讀範例時要分清「移到尾端」和「從容器消失」是兩個步驟。
  - std::pop_heap 操作的是 heap 性質，不是 std::priority_queue 物件；資料通常仍放在 vector 這類 random access range 中。
  - heap 預設是 max-heap，front() 會是最大元素；若要 min-heap，要提供反向比較器並在所有 heap 操作中保持一致。
  - make_heap 把既有範圍整理成 heap；push_heap 假設新元素已 push_back 到尾端；pop_heap 會把最大元素換到尾端但不移除它。
  - pop_heap 後通常要再呼叫 container.pop_back() 才真的刪掉元素；漏掉這步會讓已彈出的值仍留在容器裡。
  - heap 只保證父節點和子節點的局部順序，不保證整個陣列排序；不要拿 heap 內部順序當 sorted list 使用。
  - sort_heap 會破壞 heap 結構並產生排序結果；排序後若還要繼續 push_heap，必須重新 make_heap。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::pop_heap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. pop_heap 到底做了什麼?為什麼要配 pop_back?
//     答:它把 *first(極值)與 *(last - 1) 交換,再對 [first, last - 1) 做
//     sift-down 重建 heap。所以極值「被搬到尾端」但**仍在容器裡**,
//     容器的 size() 完全沒變。標準的移除流程是兩步:
//         std::pop_heap(v.begin(), v.end());   // 極值換到尾端
//         int top = v.back();                   // 取值
//         v.pop_back();                         // 真正移除
//     追問:為什麼設計成不縮容器?(答:演算法是容器無關的,只透過迭代器運作,
//     改變大小只有容器成員函式做得到 — 這和 std::remove 不真的刪除是同一個
//     道理。)
//
// 🔥 Q2. pop_heap 為什麼是 O(log N)?
//     答:換上來的新根多半是個小值,必須 sift-down — 從根往下,每層先比較左右
//     子節點取較大者,再與目前節點比較並視需要交換,最多走到葉子即樹高 log2(N)
//     層。標準保證比較次數最多 2 * log(last - first) 次(每層約 2 次比較)。
//
// ⚠️ 陷阱. 連續呼叫 pop_heap 但不縮小範圍,會發生什麼?
//     答:結果是錯的。第二次 pop_heap(v.begin(), v.end()) 的前置條件是
//     「[first, last) 是合法 heap」,但第一次 pop 之後尾端那個極值已經不滿足
//     heap property,整段不再是合法 heap → UB。
//     正確做法是每次都縮小 last(sort_heap 內部正是這樣做),或搭配 pop_back。
//     為什麼會錯:一般人把 pop_heap 想成「彈出」這種有狀態的操作,
//     但它其實是無狀態的區間變換 — 範圍要由呼叫端自己維護。
//     另注意:空範圍呼叫 pop_heap 也是 UB(前置條件要求至少一個元素)。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};
    std::make_heap(v.begin(), v.end());
    std::cout << "initial heap top = " << v.front() << '\n';

    // --- 範例 1: 取出 max,移除 ---
    std::pop_heap(v.begin(), v.end());          // 把 max 換到尾端
    int max_val = v.back();                      // 取得它
    v.pop_back();                                // 真的從容器移除
    std::cout << "popped " << max_val
              << ", new top = " << v.front() << '\n';

    // --- 範例 2: 連續 pop 形成降序輸出 (heap sort 雛形) ---
    std::vector<int> w{3, 1, 4, 1, 5, 9, 2, 6};
    std::make_heap(w.begin(), w.end());
    std::cout << "pop sequence: ";
    while (!w.empty()) {
        std::pop_heap(w.begin(), w.end());
        std::cout << w.back() << ' ';
        w.pop_back();
    }
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1046_last_stone_weight();
    void leetcode_1962_remove_stones();
    void practical_task_queue_pop();
    void leetcode_2208_half_array_sum();
    void practical_dijkstra_extract_min();
    leetcode_1046_last_stone_weight();
    leetcode_1962_remove_stones();
    practical_task_queue_pop();
    leetcode_2208_half_array_sum();
    practical_dijkstra_extract_min();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1046: 最後一塊石頭的重量 (Last Stone Weight)
// ----------------------------------------------------------------
// 題目:每次取最重兩塊 x ≤ y 相撞,若 y > x 把 y - x 放回。直到剩 0 或 1 塊。
//
// 為什麼用 std::pop_heap (反覆取 max):
//   題目就是「不停地取出 max 兩個」 — pop_heap + pop_back 的標準場景。
//
// 複雜度:時間 O(N log N);空間 O(1)。
void leetcode_1046_last_stone_weight() {
    std::vector<int> stones{2, 7, 4, 1, 8, 1};
    std::make_heap(stones.begin(), stones.end());
    while (stones.size() > 1) {
        std::pop_heap(stones.begin(), stones.end());
        int y = stones.back(); stones.pop_back();
        std::pop_heap(stones.begin(), stones.end());
        int x = stones.back(); stones.pop_back();
        if (y > x) {
            stones.push_back(y - x);
            std::push_heap(stones.begin(), stones.end());
        }
    }
    std::cout << "LC1046: " << (stones.empty() ? 0 : stones.front()) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1962: 移除石頭使總數最小
//                (Remove Stones to Minimize the Total)
// ----------------------------------------------------------------
// 題目:給陣列 piles 與 k,執行 k 次操作:每次把任一堆 p 變為 p - floor(p/2)。
//      求 k 次後總和最小值。
//
// 為什麼用 std::pop_heap (貪心取 max):
//   每次都對「目前最大堆」操作收益最多 — 用 max-heap 維護,每次取 max。
//
// 複雜度:時間 O(N + k log N);空間 O(1)。
void leetcode_1962_remove_stones() {
    std::vector<int> piles{5, 4, 9};
    int k = 2;
    long long sum = 0;
    for (int p : piles) sum += p;
    std::make_heap(piles.begin(), piles.end());
    for (int i = 0; i < k; ++i) {
        std::pop_heap(piles.begin(), piles.end());
        int top = piles.back(); piles.pop_back();
        int removed = top / 2;
        sum -= removed;
        piles.push_back(top - removed);
        std::push_heap(piles.begin(), piles.end());
    }
    std::cout << "LC1962: " << sum << '\n';
}

// ----------------------------------------------------------------
// 實務範例:從 priority queue 取出最高優先任務
// ----------------------------------------------------------------
// 場景:任務排程器持續處理「目前最高優先」的任務 —
//      pop_heap + pop_back 是 std::priority_queue::pop 的內部實作。
void practical_task_queue_pop() {
    struct Task { int prio; std::string name; };
    auto cmp = [](const Task& a, const Task& b){ return a.prio < b.prio; };
    std::vector<Task> q{
        {3, "build"}, {5, "alert"}, {1, "log"}, {4, "deploy"}
    };
    std::make_heap(q.begin(), q.end(), cmp);
    std::cout << "practical pop order:";
    while (!q.empty()) {
        std::pop_heap(q.begin(), q.end(), cmp);
        std::cout << " " << q.back().name << "(" << q.back().prio << ")";
        q.pop_back();
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2208: 將陣列總和減半的最少操作 (Min Ops to Halve Array Sum)
// 難度: medium
// ----------------------------------------------------------------
// 題目:每次操作把陣列中任一元素 a 改為 a/2 (double 除法)。
//      求最少幾次操作能讓總和 <= 原本的一半。
//
// 為什麼用 std::pop_heap (貪心):
//   每次對「目前最大值」操作收益最高 — 用 max-heap 不停 pop top 並 push (top/2)。
//
// 複雜度:時間 O(N + k log N);空間 O(1)。
void leetcode_2208_half_array_sum() {
    std::vector<double> nums{5, 19, 8, 1};
    double total = 0;
    for (double x : nums) total += x;
    double target = total / 2;
    double removed = 0;
    std::make_heap(nums.begin(), nums.end());
    int ops = 0;
    while (removed < target) {
        std::pop_heap(nums.begin(), nums.end());
        double top = nums.back(); nums.pop_back();
        double half = top / 2;
        removed += half;
        nums.push_back(half);
        std::push_heap(nums.begin(), nums.end());
        ++ops;
    }
    std::cout << "LC2208: " << ops << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Dijkstra 演算法的 extract-min 模式 (簡化版)
// ----------------------------------------------------------------
// 場景:最短路徑演算法中,每次從 priority queue 取出「目前距離最小的節點」 —
//      用 min-heap (std::greater) + pop_heap 是教科書級寫法。
void practical_dijkstra_extract_min() {
    struct NodeDist { int dist; std::string node; };
    auto cmp = [](const NodeDist& a, const NodeDist& b){ return a.dist > b.dist; };
    std::vector<NodeDist> pq{
        {5, "B"}, {2, "A"}, {8, "D"}, {3, "C"}
    };
    std::make_heap(pq.begin(), pq.end(), cmp);
    std::cout << "dijkstra extract order:";
    while (!pq.empty()) {
        std::pop_heap(pq.begin(), pq.end(), cmp);
        std::cout << " " << pq.back().node << "(" << pq.back().dist << ")";
        pq.pop_back();
    }
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra pop_heap.cpp -o pop_heap

// === 預期輸出 ===
// initial heap top = 9
// popped 9, new top = 6
// pop sequence: 9 6 5 4 3 2 1 1
// LC1046: 1
// LC1962: 12
// practical pop order: alert(5) deploy(4) build(3) log(1)
// LC2208: 3
// dijkstra extract order: A(2) C(3) B(5) D(8)
