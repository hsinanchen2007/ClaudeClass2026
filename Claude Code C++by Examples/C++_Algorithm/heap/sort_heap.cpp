// ============================================================
// std::sort_heap
// 分類 (Category): Heap operations (堆積操作)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/sort_heap
//   * https://cplusplus.com/reference/algorithm/sort_heap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::sort_heap 解的問題:
//
//   「把『合法 heap』的範圍轉換為已排序範圍。」
//
// 對 max-heap 操作 → 升序;對 min-heap (greater comp) 操作 → 降序。
// 排序完成後,範圍不再是 heap (而是排好的)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼這是 Heapsort?                                  │
// └────────────────────────────────────────────────────────────┘
//
// Heapsort 的標準兩步流程:
//
//   std::make_heap(first, last);    // step 1: 任意陣列 → max-heap, O(N)
//   std::sort_heap(first, last);    // step 2: max-heap → 升序排列, O(N log N)
//
// 第二步內部就是反覆 pop_heap + 縮小範圍 — 把每次的 max 放到「當前範圍尾端」,
// 然後縮小範圍繼續找下一個 max。最後尾端就排好了。
//
// 整體時間 O(N log N),空間 O(1) — heapsort 是經典的「就地 + 最壞情況保證」演算法。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼通常 std::sort 比 heapsort 還快?                │
// └────────────────────────────────────────────────────────────┘
//
// std::sort 的 introsort 是 quicksort + heapsort + insertion sort 混合;
// 平均情況下 quicksort 比 heapsort 快 (cache friendly),
// heapsort 只在 quicksort 退化時當保險。
//
// 結論:單純排序需求,直接用 std::sort 通常更快。
// 但若你「已經有一個 heap」(因為其他原因維護成 heap),
// 用 sort_heap 比丟掉再 sort 划算。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 整個程式期間維護 heap,結束時一次輸出排序結果
//   * 教學 / 演算法示範:展示 heapsort
//   * LC 912 Sort an Array (其中一種解法)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章與需求                                         │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void sort_heap(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void sort_heap(RandomIt first, RandomIt last, Compare comp);
//
//   * 需求:RandomAccessIterator;範圍必須已是合法 heap (用同一個 comp)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N log N)
//   空間: O(1)
//
//   1. 對「非 heap」呼叫 → UB。先 make_heap 才能 sort_heap。
//   2. 對 max-heap 排出來是「升序」(從大到小依次 pop 到尾端)。
//   3. 想要降序 → 用 min-heap (greater) 配 sort_heap (greater)。
//   4. 排序後不再是 heap;若要繼續用 heap 操作,要重新 make_heap。
//
// ============================================================

/*
補充筆記：std::sort_heap
  - sort_heap 只能用在已經是 heap 的 range，否則前置條件不成立。
  - 排序後 range 不再保持 heap property，而是成為一般有序序列。
  - 若只是想排序 vector，直接 sort 通常更直觀；sort_heap 是 heap 流程的收尾。
  - std::sort_heap 操作的是 heap 性質，不是 std::priority_queue 物件；資料通常仍放在 vector 這類 random access range 中。
  - heap 預設是 max-heap，front() 會是最大元素；若要 min-heap，要提供反向比較器並在所有 heap 操作中保持一致。
  - make_heap 把既有範圍整理成 heap；push_heap 假設新元素已 push_back 到尾端；pop_heap 會把最大元素換到尾端但不移除它。
  - pop_heap 後通常要再呼叫 container.pop_back() 才真的刪掉元素；漏掉這步會讓已彈出的值仍留在容器裡。
  - heap 只保證父節點和子節點的局部順序，不保證整個陣列排序；不要拿 heap 內部順序當 sorted list 使用。
  - sort_heap 會破壞 heap 結構並產生排序結果；排序後若還要繼續 push_heap，必須重新 make_heap。
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: heapsort 完整流程 (max-heap → 升序) ---
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};
    std::make_heap(v.begin(), v.end());
    std::sort_heap(v.begin(), v.end());
    std::cout << "heapsort asc: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 用 min-heap 排出降序 ---
    std::vector<int> w{3, 1, 4, 1, 5, 9, 2, 6};
    std::make_heap(w.begin(), w.end(), std::greater<int>{});
    std::sort_heap(w.begin(), w.end(), std::greater<int>{});
    std::cout << "heapsort desc: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_912_sort_array();
    void practical_heap_then_sort_output();
    void leetcode_1985_kth_largest_string();
    void practical_export_sorted_report();
    leetcode_912_sort_array();
    practical_heap_then_sort_output();
    leetcode_1985_kth_largest_string();
    practical_export_sorted_report();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 912: 排序陣列 (Sort an Array) — heapsort 解法
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,升序排序後回傳。要求 O(N log N)。
//
// 為什麼用 std::make_heap + std::sort_heap:
//   經典 heapsort 兩步驟。雖然 std::sort 通常更快,但這裡示範 sort_heap 的
//   標準用法 — 也是 LC 912 「自選排序演算法」這類題目的展示之一。
//
// 複雜度:時間 O(N log N);空間 O(1)。
void leetcode_912_sort_array() {
    std::vector<int> nums{5, 2, 3, 1, 4, 9, 7, 6, 8};
    std::make_heap(nums.begin(), nums.end());
    std::sort_heap(nums.begin(), nums.end());
    std::cout << "LC912 sorted:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:任務佇列「結束時一次輸出排序結果」
// ----------------------------------------------------------------
// 場景:整個程式運行期間都把任務維護成 max-heap (因為要快速取最高優先),
//      結束前需要把整批任務「按優先度由低到高」輸出 (例如報表)。
//
// 為什麼用 std::sort_heap:
//   * 容器已經是 heap → 直接 sort_heap 一行完成,不必重排。
//   * O(N log N),與 std::sort 同等;但語意更明確 — 「把現有 heap 排好」。
void practical_heap_then_sort_output() {
    struct Task { int prio; std::string name; };
    auto less_by_prio = [](const Task& a, const Task& b){ return a.prio < b.prio; };
    std::vector<Task> q{
        {3, "build"}, {5, "alert"}, {1, "log"}, {4, "deploy"}, {2, "backup"}
    };
    std::make_heap(q.begin(), q.end(), less_by_prio);   // max-heap by prio
    // ...期間程式可能執行多次 push_heap / pop_heap...
    // 結束時統一輸出排序結果 (升序 by prio)
    std::sort_heap(q.begin(), q.end(), less_by_prio);
    std::cout << "practical sorted output (asc):";
    for (const auto& t : q) {
        std::cout << " " << t.name << "(" << t.prio << ")";
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1985: 找出字串陣列中第 K 大的整數 (簡化)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給字串陣列 nums (每個字串表示一個非負整數,可能很大),
//      回傳「第 k 大」的字串 (依數字大小,非字典序)。
//      簡化:這裡值都可裝下 long long,直接用 long long 比較。
//
// 為什麼用 std::make_heap + std::sort_heap:
//   一次性排好整個陣列 (升序),倒數第 k 個即為第 k 大。
//   sort_heap 與 std::sort 同為 O(N log N),這裡作為示範。
//
// 複雜度:時間 O(N log N);空間 O(N) (轉成 long long)。
void leetcode_1985_kth_largest_string() {
    std::vector<std::string> nums{"3", "6", "7", "10"};
    int k = 4;
    std::vector<long long> values;
    for (const auto& s : nums) values.push_back(std::stoll(s));
    std::make_heap(values.begin(), values.end());
    std::sort_heap(values.begin(), values.end());
    long long ans = values[values.size() - k];
    std::cout << "LC1985: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:資料報表輸出 — 依分數由低到高排序
// ----------------------------------------------------------------
// 場景:評測系統運行期間以 max-heap 維護「分數」,
//      最終要產生「由低分到高分」的報表,直接呼叫 sort_heap。
void practical_export_sorted_report() {
    std::vector<int> scores{77, 92, 60, 85, 100, 50};
    std::make_heap(scores.begin(), scores.end());
    std::sort_heap(scores.begin(), scores.end());
    std::cout << "report (asc):";
    for (int s : scores) std::cout << ' ' << s;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// heapsort asc: 1 1 2 3 4 5 6 9
// heapsort desc: 9 6 5 4 3 2 1 1
// LC912 sorted: 1 2 3 4 5 6 7 8 9
// practical sorted output (asc): log(1) backup(2) build(3) deploy(4) alert(5)
// LC1985: 3
// report (asc): 50 60 77 85 92 100
