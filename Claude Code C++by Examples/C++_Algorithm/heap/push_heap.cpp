// ============================================================
// std::push_heap
// 分類 (Category): Heap operations (堆積操作)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/push_heap
//   * https://cplusplus.com/reference/algorithm/push_heap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::push_heap 解的問題:
//
//   「我有一個合法 heap [first, last - 1),剛剛 push_back 了一個新元素到 last - 1。
//    請你把這個新元素『調整到正確的位置』,讓 [first, last) 重新成為合法 heap。」
//
// 內部做的事是「sift-up (上浮)」 — 從新位置往根方向比較,
// 若新元素比父大 (max-heap) 就和父交換,直到不再比父大。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、標準的「加入 heap」流程                                 │
// └────────────────────────────────────────────────────────────┘
//
// 加入新元素的標準寫法是兩步:
//
//   v.push_back(x);                          // step 1: 容器加長
//   std::push_heap(v.begin(), v.end());      // step 2: 把新元素 sift-up
//
// 順序「不能顛倒」 — 必須先 push_back,再 push_heap。
// 否則 push_heap 看到的範圍不正確,行為未定義。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼是 O(log N)?                                     │
// └────────────────────────────────────────────────────────────┘
//
// 新元素從某個葉子位置 sift-up 到根,最多走樹的高度 = log2(N) 步。
// 每步是常數時間 (一次比較 + 可能一次 swap)。所以 O(log N)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * priority queue 加入新任務
//   * 流式 Top-K (LC 215 KthLargest 串流版)
//   * 雙堆維護中位數 (LC 295)
//   * 任何「動態維護最大/最小元素」的場景
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章與需求                                         │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void push_heap(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void push_heap(RandomIt first, RandomIt last, Compare comp);
//
//   * 需求:RandomAccessIterator;[first, last - 1) 必須已是合法 heap;
//          新元素位於 last - 1。
//   * comp 必須與 make_heap 用的相同。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 必須「先 push_back 再 push_heap」 — 順序顛倒 UB。
//   2. comp 必須與 make_heap 用的相同。
//   3. 一般專案直接用 std::priority_queue 更乾淨。
//
// ============================================================

/*
補充筆記：std::push_heap
  - push_heap 假設新元素已經被 push_back 到 range 尾端，然後只修復 heap property。
  - 如果忘記先把元素放到尾端，push_heap 沒有新資料可以納入 heap。
  - 容器大小的變化由 push_back 負責，push_heap 只負責排列。
  - std::push_heap 操作的是 heap 性質，不是 std::priority_queue 物件；資料通常仍放在 vector 這類 random access range 中。
  - heap 預設是 max-heap，front() 會是最大元素；若要 min-heap，要提供反向比較器並在所有 heap 操作中保持一致。
  - make_heap 把既有範圍整理成 heap；push_heap 假設新元素已 push_back 到尾端；pop_heap 會把最大元素換到尾端但不移除它。
  - pop_heap 後通常要再呼叫 container.pop_back() 才真的刪掉元素；漏掉這步會讓已彈出的值仍留在容器裡。
  - heap 只保證父節點和子節點的局部順序，不保證整個陣列排序；不要拿 heap 內部順序當 sorted list 使用。
  - sort_heap 會破壞 heap 結構並產生排序結果；排序後若還要繼續 push_heap，必須重新 make_heap。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::push_heap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要把一個新元素加進 heap,正確的兩步驟是什麼?為什麼不能顛倒?
//     答:先 v.push_back(x) 讓容器長出新位置,再 std::push_heap(v.begin(),
//     v.end()) 把位於 last - 1 的新元素 sift-up 到正確位置。
//     push_heap 的前置條件是「[first, last - 1) 已是合法 heap,而待插入的
//     元素就放在 last - 1」;若先呼叫 push_heap 再 push_back,push_heap 看到
//     的是舊範圍(不含新元素),而之後 push_back 進來的元素沒有任何人幫它
//     sift-up,heap property 就被破壞了。違反前置條件是 UB。
//     追問:為什麼 STL 不乾脆幫我做 push_back?(答:演算法只拿到一對迭代器,
//     根本不知道也碰不到背後的容器,改變大小只有容器成員函式做得到。)
//
// 🔥 Q2. push_heap 是 O(log N),內部做了什麼?
//     答:sift-up(上浮)。新元素位於某個葉子位置,反覆與父節點
//     (index (i-1)/2) 比較,只要「比父更該在上面」就交換,直到不再需要交換
//     或到達根。路徑長度最多是樹高 log2(N),每步常數時間。
//     標準保證比較次數最多 log(last - first) 次。
//     追問:那 pop_heap 為什麼是 sift-down 而且上限是 2*log N 次比較?
//     (答:下沉時每層要先比較兩個子節點誰較大,再拿贏家和父比,每層 2 次。)
//
// ⚠️ 陷阱. 對一個「還沒 make_heap 的 vector」逐一 push_back + push_heap,
//     結果會是合法 heap 嗎?
//     答:會 — 但前提是「從空容器開始」逐一累積。空範圍與單元素範圍本身就是
//     合法 heap,每次 push_heap 都維持不變式,歸納下來全程合法。
//     真正錯的是「先塞滿一堆元素,才對整段呼叫一次 push_heap」:此時
//     [first, last - 1) 根本不是 heap,前置條件不成立 → UB。這種情況要用
//     make_heap。順帶一提逐一 push 建堆是 O(N log N),比 make_heap 的 O(N) 差。
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
    // --- 範例 1: 逐一 push 元素到 max-heap ---
    std::vector<int> v;
    for (int x : {3, 1, 4, 1, 5, 9}) {
        v.push_back(x);
        std::push_heap(v.begin(), v.end());
        std::cout << "after push " << x << ", heap top = "
                  << v.front() << '\n';
    }

    // --- 範例 2: min-heap 版本 ---
    std::vector<int> w;
    for (int x : {3, 1, 4, 1, 5, 9}) {
        w.push_back(x);
        std::push_heap(w.begin(), w.end(), std::greater<int>{});
    }
    std::cout << "min-heap top = " << w.front() << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_215_kth_largest_stream();
    void leetcode_295_find_median();
    void practical_task_queue_push();
    void leetcode_1845_seat_manager();
    void practical_event_loop_schedule();
    leetcode_215_kth_largest_stream();
    leetcode_295_find_median();
    practical_task_queue_push();
    leetcode_1845_seat_manager();
    practical_event_loop_schedule();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 703: 資料流中的第 K 大元素 (Kth Largest Element in a Stream)
// ----------------------------------------------------------------
// 題目:設計類別,持續加入新值,任意時刻能回傳「目前第 k 大」。
//
// 為什麼用 std::push_heap (min-heap, size = k):
//   * 維護一個「大小恰好 k」的 min-heap,root 即為「目前第 k 大」。
//   * 新值來時 push_back + push_heap 加入;若 size > k,pop_heap + pop_back 拋掉最小。
//   * 每次操作 O(log k)。
//
// 複雜度:每次 add O(log k);總 N 次 O(N log k)。
void leetcode_215_kth_largest_stream() {
    int k = 3;
    std::vector<int> stream{4, 5, 8, 2, 3, 5, 10, 9, 4};
    std::vector<int> heap;
    for (int x : stream) {
        heap.push_back(x);
        std::push_heap(heap.begin(), heap.end(), std::greater<int>{});
        if (static_cast<int>(heap.size()) > k) {
            std::pop_heap(heap.begin(), heap.end(), std::greater<int>{});
            heap.pop_back();
        }
    }
    std::cout << "LC703-stream: " << heap.front() << '\n';
}

// ----------------------------------------------------------------
// LeetCode 295: 資料流的中位數 (Find Median from Data Stream)
// ----------------------------------------------------------------
// 題目:設計類別 addNum / findMedian,持續加入新值,任意時刻回傳中位數。
//
// 為什麼用 std::push_heap (兩個 heap):
//   * low  = max-heap (較小一半,root = 較小一半的最大)
//   * high = min-heap (較大一半,root = 較大一半的最小)
//   * 維持 |low.size - high.size| ≤ 1。
//   * 中位數 = sizes 不等時 → 較大堆 root;否則 → 兩 root 平均。
//
// 複雜度:addNum O(log N);findMedian O(1)。
void leetcode_295_find_median() {
    std::vector<int> low;   // max-heap (預設)
    std::vector<int> high;  // min-heap (greater)
    auto add = [&](int x){
        if (low.empty() || x <= low.front()) {
            low.push_back(x);
            std::push_heap(low.begin(), low.end());
        } else {
            high.push_back(x);
            std::push_heap(high.begin(), high.end(), std::greater<int>{});
        }
        // 平衡兩堆 size
        if (low.size() > high.size() + 1) {
            std::pop_heap(low.begin(), low.end());
            high.push_back(low.back()); low.pop_back();
            std::push_heap(high.begin(), high.end(), std::greater<int>{});
        } else if (high.size() > low.size()) {
            std::pop_heap(high.begin(), high.end(), std::greater<int>{});
            low.push_back(high.back()); high.pop_back();
            std::push_heap(low.begin(), low.end());
        }
    };
    auto median = [&](){
        return low.size() > high.size()
            ? static_cast<double>(low.front())
            : (low.front() + high.front()) / 2.0;
    };
    add(1); add(2);
    std::cout << "LC295: median after {1,2} = " << median() << '\n';
    add(3);
    std::cout << "LC295: median after {1,2,3} = " << median() << '\n';
}

// ----------------------------------------------------------------
// 實務範例:任務佇列加入新任務
// ----------------------------------------------------------------
// 場景:依「優先度」排程的任務佇列,新任務隨時到達。
//
// 為什麼用 std::push_heap:
//   標準的「priority queue 加入新元素」流程 — push_back + push_heap,O(log N)。
void practical_task_queue_push() {
    struct Task { int prio; std::string name; };
    auto cmp = [](const Task& a, const Task& b){ return a.prio < b.prio; };
    std::vector<Task> q;

    auto push_task = [&](Task t){
        q.push_back(t);
        std::push_heap(q.begin(), q.end(), cmp);
    };
    push_task({3, "build"});
    push_task({5, "alert"});
    push_task({1, "log"});
    push_task({4, "deploy"});
    std::cout << "practical: top task = " << q.front().name
              << "(" << q.front().prio << ")\n";
}

// ----------------------------------------------------------------
// LeetCode 1845: 座位預訂管理 (Seat Reservation Manager)
// ----------------------------------------------------------------
// 題目:設計座位管理器,n 個座位 (1..n) 初始皆可用。
//      reserve() 回傳「目前可用的最小編號」並標記為已預訂;
//      unreserve(x) 釋放座位 x。
//
// 為什麼用 std::push_heap (min-heap):
//   * 維護「可用座位編號」的 min-heap,root 就是最小編號。
//   * reserve = pop_heap + pop_back;unreserve = push_back + push_heap。
//   * 每次 O(log n)。
//
// 複雜度:reserve/unreserve 各 O(log n);初始化 O(n)。
void leetcode_1845_seat_manager() {
    int n = 5;
    std::vector<int> avail;
    for (int i = 1; i <= n; ++i) avail.push_back(i);
    std::make_heap(avail.begin(), avail.end(), std::greater<int>{});

    auto reserve = [&]() {
        std::pop_heap(avail.begin(), avail.end(), std::greater<int>{});
        int s = avail.back(); avail.pop_back();
        return s;
    };
    auto unreserve = [&](int s) {
        avail.push_back(s);
        std::push_heap(avail.begin(), avail.end(), std::greater<int>{});
    };

    std::cout << "LC1845: reserve=" << reserve()
              << " reserve=" << reserve()
              << " reserve=" << reserve();
    unreserve(2);
    std::cout << " (unreserve 2) reserve=" << reserve() << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Event Loop 排程 (依執行時間 push 進 min-heap)
// ----------------------------------------------------------------
// 場景:類 Node.js 的 event loop — 各 timer 帶「到期時間 (毫秒)」,
//      event loop 永遠先處理「最近到期」的 timer。
//      新註冊的 timer 用 push_heap 加入,O(log N)。
void practical_event_loop_schedule() {
    struct Timer { long long deadline; std::string name; };
    auto cmp = [](const Timer& a, const Timer& b){ return a.deadline > b.deadline; };
    std::vector<Timer> q;

    auto schedule = [&](Timer t) {
        q.push_back(t);
        std::push_heap(q.begin(), q.end(), cmp);
    };
    schedule({500, "auth_check"});
    schedule({100, "heartbeat"});
    schedule({300, "log_flush"});
    schedule({200, "metrics"});
    std::cout << "next timer to fire: " << q.front().name
              << "(deadline=" << q.front().deadline << ")\n";
}

// 編譯: g++ -std=c++20 -Wall -Wextra push_heap.cpp -o push_heap

// === 預期輸出 ===
// after push 3, heap top = 3
// after push 1, heap top = 3
// after push 4, heap top = 4
// after push 1, heap top = 4
// after push 5, heap top = 5
// after push 9, heap top = 9
// min-heap top = 1
// LC703-stream: 8
// LC295: median after {1,2} = 1.5
// LC295: median after {1,2,3} = 2
// practical: top task = alert(5)
// LC1845: reserve=1 reserve=2 reserve=3 (unreserve 2) reserve=2
// next timer to fire: heartbeat(deadline=100)
