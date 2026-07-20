// ============================================================
// std::make_heap
// 分類 (Category): Heap operations (堆積操作)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/make_heap
//   * https://cplusplus.com/reference/algorithm/make_heap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、什麼是 Heap?                                          │
// └────────────────────────────────────────────────────────────┘
//
// 「堆 (heap)」是一種特殊的「完全二元樹」結構,有兩個關鍵特性:
//
//   1. 「形狀」:完全二元樹 — 從根開始按層由左到右填滿,只有最後一層可能不滿。
//   2. 「順序」:每個父節點都比子節點「大」(max-heap) 或「小」(min-heap)。
//
// STL 用「陣列」表示堆:
//
//   index 0 = 根
//   index i 的左子 = 2i + 1
//   index i 的右子 = 2i + 2
//   index i 的父   = (i - 1) / 2
//
// 因此一個 vector 就足以表示一個 heap — 不必真的建一棵樹。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、std::make_heap 做什麼?                                 │
// └────────────────────────────────────────────────────────────┘
//
//   「把任意排列的 [first, last) 重新排列成合法 heap。」
//
// 預設 max-heap (用 operator<);要 min-heap 傳 std::greater<>。
//
// 它「不是排序」 — 結束後僅保證:
//   * *first 是最大 (或最小) 元素。
//   * 每個父節點 ≥ (或 ≤) 兩個子節點。
//
// 整個範圍的元素仍可能是任意順序 (符合 heap 結構即可)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼是 O(N) 而不是 O(N log N)?                       │
// └────────────────────────────────────────────────────────────┘
//
// 一個常見的誤解是「建堆 = 每個元素 O(log N) → 總共 O(N log N)」 — 錯!
//
// 標準實作用 Floyd 建堆法:從「最後一個非葉節點」往根方向 sift-down。
// 因為大部分節點都接近葉子,sift-down 距離很短;
// 數學上可以證明總成本只有 O(N)。
//
// 對比:
//   * 一個一個 push_heap:O(N log N)
//   * 直接 make_heap:O(N)  (對大 N 差很多)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、heap 的合法操作 (heap operations 家族)                  │
// └────────────────────────────────────────────────────────────┘
//
//   * make_heap   — 建立 heap
//   * push_heap   — 加入新元素 (先 push_back 再 push_heap)
//   * pop_heap    — 取出 root (max/min) (先 pop_heap 再 pop_back)
//   * sort_heap   — 排序 (heap → 排好的範圍)
//   * is_heap     — 檢查是否為合法 heap
//   * is_heap_until — 找第一個破壞 heap property 的位置
//
// 對「優先佇列」需求,直接用 std::priority_queue 通常更乾淨 —
// 它內部就是用這些函式。但若需要「以 vector 暴露 heap」(例如
// 想直接 iterate 看內容、做特殊操作),這套低階 API 就有用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章與需求                                         │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void make_heap(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void make_heap(RandomIt first, RandomIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//   * 需求:RandomAccessIterator (vector / array / deque OK,list 不行)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「heap 不是排序」 — 別期待元素是升/降序。
//   2. 從一個 vector 一次建堆比一個一個 push 更快 (O(N) vs O(N log N))。
//   3. 後續所有 push_heap / pop_heap 必須用「相同的 comp」。
//   4. 想取「第 k 大」可結合 partial_sort / nth_element,有時更直接。
//
// ============================================================

/*
補充筆記：std::make_heap
  - make_heap 會在原本的 random access range 內重排元素，建立 heap property。
  - heap 的底層仍是 vector/deque 這類連續或可隨機存取範圍；list 不能直接使用。
  - 建立後只有 front 保證是極值，其餘元素不能當成排序結果讀。
  - std::make_heap 操作的是 heap 性質，不是 std::priority_queue 物件；資料通常仍放在 vector 這類 random access range 中。
  - heap 預設是 max-heap，front() 會是最大元素；若要 min-heap，要提供反向比較器並在所有 heap 操作中保持一致。
  - make_heap 把既有範圍整理成 heap；push_heap 假設新元素已 push_back 到尾端；pop_heap 會把最大元素換到尾端但不移除它。
  - pop_heap 後通常要再呼叫 container.pop_back() 才真的刪掉元素；漏掉這步會讓已彈出的值仍留在容器裡。
  - heap 只保證父節點和子節點的局部順序，不保證整個陣列排序；不要拿 heap 內部順序當 sorted list 使用。
  - sort_heap 會破壞 heap 結構並產生排序結果；排序後若還要繼續 push_heap，必須重新 make_heap。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::make_heap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. make_heap 做什麼?為什麼它是 O(N) 而不是 O(N log N)?
//     答:把任意排列的 [first, last) 就地重排成合法 heap(預設 max-heap)。
//     它用 Floyd 建堆法 — 從「最後一個非葉節點」往根方向逐一 sift-down。
//     關鍵在於節點深度分布:接近葉子的節點數量最多但下沉距離最短,
//     把「每層節點數 × 該層最大下沉距離」求和會收斂到 O(N)。
//     標準保證比較次數最多 3 * (last - first) 次。
//     追問:那「一個一個 push_heap 建堆」是多少?(答:O(N log N),所以已有整批
//     資料時一律 make_heap,不要逐一 push_heap。)
//
// 🔥 Q2. make_heap 之後,元素的排列是什麼?
//     答:標準「只保證 heap property」(每個父節點不劣於其子節點,因此 *first
//     是極值),**具體的排列順序標準未規定**,不同實作/版本可能不同。
//     所以印出整個 vector 來比對輸出是不可靠的測試方式,只能斷言 is_heap。
//     追問:那 heap 是穩定的嗎?(答:不是,等價元素的相對順序不保證。)
//
// ⚠️ 陷阱. make_heap 之後,vector 就「排好序」了嗎?
//     答:沒有。heap 不是排序 — 它只保證「父子之間」的局部順序,
//     兄弟節點之間、以及跨子樹的元素之間完全沒有順序保證。
//     要得到有序序列必須再呼叫 sort_heap(O(N log N))。
//     為什麼會錯:多數人把「front 是最大值」直覺外推成「整段由大到小」,
//     但 heap 只是一棵完全二元樹的陣列表示,不是線性有序結構。
//
// 🔥 Q. heap 為什麼可以直接蓋在 vector 上，不需要真的建樹？
//     答：binary heap 是**完全二元樹**（complete binary tree）——除了最後一層，
//         每層都填滿，且最後一層靠左對齊、中間沒有洞。
//         沒有洞就代表可以用「層序」把節點一一對應到陣列索引：
//             節點 i 的左子 = 2i+1、右子 = 2i+2、父 = (i-1)/2
//         **父子關係用算術算得出來，就不需要指標**，於是整棵樹塌成一段連續記憶體。
//         這也是 priority_queue 預設用 vector 的真正原因（不只是「演算法需要
//         random access」——那是必要條件，這才是理由）。
//     追問：那為什麼不能用 list？（算不出 2i+1 的位置，得走 n 步；
//         而且失去快取局部性，heapify 會變得非常慢）
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
    // --- 範例 1: 建立 max-heap ---
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};
    std::make_heap(v.begin(), v.end());
    std::cout << "max-heap (root=" << v.front() << "): ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 建立 min-heap ---
    std::vector<int> w{3, 1, 4, 1, 5, 9, 2, 6};
    std::make_heap(w.begin(), w.end(), std::greater<int>{});
    std::cout << "min-heap (root=" << w.front() << "): ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 對結構體建 heap (依 priority) ---
    struct Job { int prio; std::string name; };
    std::vector<Job> jobs{
        {2, "B"}, {5, "E"}, {1, "A"}, {4, "D"}, {3, "C"}
    };
    auto cmp = [](const Job& a, const Job& b){ return a.prio < b.prio; };
    std::make_heap(jobs.begin(), jobs.end(), cmp);
    std::cout << "max-prio job at top: " << jobs.front().name
              << "(" << jobs.front().prio << ")\n";

    // === LeetCode / 實務範例 ===
    void leetcode_215_kth_largest();
    void leetcode_1046_last_stone_weight();
    void practical_vector_to_priority_queue();
    void leetcode_703_kth_largest_in_stream();
    void practical_top_alerts_dashboard();
    leetcode_215_kth_largest();
    leetcode_1046_last_stone_weight();
    practical_vector_to_priority_queue();
    leetcode_703_kth_largest_in_stream();
    practical_top_alerts_dashboard();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 215: 陣列中第 K 大的元素 (用 make_heap + pop_heap)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums 與整數 k,回傳第 k 大的元素。
//
// 為什麼用 std::make_heap:
//   把整個 nums 一次轉成 max-heap (O(N)),比 priority_queue 一個個 push (O(N log N)) 快。
//   接著 pop_heap k-1 次,root 即為第 k 大。
//
// 複雜度:時間 O(N + k log N);空間 O(1)。
void leetcode_215_kth_largest() {
    std::vector<int> nums{3, 2, 1, 5, 6, 4};
    int k = 2;
    std::make_heap(nums.begin(), nums.end());
    for (int i = 0; i < k - 1; ++i) {
        std::pop_heap(nums.begin(), nums.end() - i);
    }
    std::cout << "LC215: " << nums.front() << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1046: 最後一塊石頭的重量 (Last Stone Weight)
// ----------------------------------------------------------------
// 題目:每次取最重兩塊 x ≤ y,若 x == y 兩者皆碎;否則剩 (y-x) 重的石頭放回。
//      重複直到剩 0 或 1 塊。
//
// 為什麼用 std::make_heap + pop/push_heap:
//   「反覆取最大兩個 → 相減 → 放回」是典型 max-heap 操作。
//   make_heap O(N) 一次建堆,後續 pop/push 都是 O(log N)。
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
// 實務範例:把現有 vector 一次轉成 priority queue 結構
// ----------------------------------------------------------------
// 場景:已從 DB / 檔案讀進一批待處理工作,想直接以「最高優先」順序處理。
//      與其一筆筆 push 進 priority_queue (O(N log N)),
//      不如用 std::make_heap 對 vector 就地建堆 (O(N))。
void practical_vector_to_priority_queue() {
    struct Job { int prio; std::string name; };
    std::vector<Job> jobs{
        {2, "backup"}, {5, "alert"}, {1, "log_rotate"},
        {4, "deploy"}, {3, "build"}
    };
    auto cmp = [](const Job& a, const Job& b){ return a.prio < b.prio; };
    std::make_heap(jobs.begin(), jobs.end(), cmp);
    std::cout << "practical: highest-prio job = " << jobs.front().name
              << "(" << jobs.front().prio << ")\n";
}

// ----------------------------------------------------------------
// LeetCode 703: 資料流中的第 K 大元素 (Kth Largest in a Stream)
// ----------------------------------------------------------------
// 題目:設計一個 KthLargest 類,初始化時給定 k 與初始陣列,
//      每次 add(val) 後回傳目前的第 k 大元素。
//
// 為什麼用 std::make_heap (min-heap 維護 size = k):
//   * 維護一個 size = k 的 min-heap (用 std::greater 比較器),root 即為第 k 大。
//   * 加入新元素時:若 heap.size() < k,push;否則若 > root,先 pop root 再 push。
//   * make_heap 一次性把初始陣列前 k 大轉為 min-heap (O(n))。
//
// 複雜度:時間 O(n) 建堆 + O(log k) per add;空間 O(k)。
void leetcode_703_kth_largest_in_stream() {
    int k = 3;
    std::vector<int> nums{4, 5, 8, 2};
    // 取前 k 大,維護 min-heap
    std::sort(nums.begin(), nums.end(), std::greater<int>{});
    std::vector<int> heap(nums.begin(), nums.begin() + std::min((int)nums.size(), k));
    std::make_heap(heap.begin(), heap.end(), std::greater<int>{});
    auto add = [&](int v) {
        if ((int)heap.size() < k) {
            heap.push_back(v);
            std::push_heap(heap.begin(), heap.end(), std::greater<int>{});
        } else if (v > heap.front()) {
            std::pop_heap(heap.begin(), heap.end(), std::greater<int>{});
            heap.back() = v;
            std::push_heap(heap.begin(), heap.end(), std::greater<int>{});
        }
        return heap.front();
    };
    std::cout << "LC703 add(3)=" << add(3)
              << " add(5)=" << add(5)
              << " add(10)=" << add(10)
              << " add(9)=" << add(9)
              << " add(4)=" << add(4) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:儀表板「Top N 嚴重告警」維護
// ----------------------------------------------------------------
// 場景:監控系統持續送進大量告警,儀表板只顯示「目前最嚴重的 N 筆」。
//      用 max-heap 直接 make_heap 一次建堆,後續可從 top 取出最嚴重者。
void practical_top_alerts_dashboard() {
    struct Alert { int severity; std::string msg; };
    std::vector<Alert> alerts{
        {3, "CPU 80%"}, {7, "Disk full"}, {2, "Slow query"},
        {9, "DB down"}, {5, "OOM warning"}
    };
    auto cmp = [](const Alert& a, const Alert& b){ return a.severity < b.severity; };
    std::make_heap(alerts.begin(), alerts.end(), cmp);
    std::cout << "top alerts (by severity):";
    // 連續 pop_heap 取出前 3 名
    for (int i = 0; i < 3 && !alerts.empty(); ++i) {
        std::pop_heap(alerts.begin(), alerts.end(), cmp);
        std::cout << " " << alerts.back().msg << "(" << alerts.back().severity << ")";
        alerts.pop_back();
    }
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra make_heap.cpp -o make_heap

// === 預期輸出 ===
// max-heap (root=9): 9 6 4 1 5 3 2 1
// min-heap (root=1): 1 1 2 3 5 9 4 6
// max-prio job at top: E(5)
// LC215: 5
// LC1046: 1
// practical: highest-prio job = alert(5)
// LC703 add(3)=4 add(5)=5 add(10)=5 add(9)=8 add(4)=8
// top alerts (by severity): DB down(9) Disk full(7) OOM warning(5)
// ⚠️ 部分項目的順序/數值屬 unspecified 或每次執行不同，實際結果可能與上面不同。
