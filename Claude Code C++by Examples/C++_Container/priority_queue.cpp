// ============================================================================
//  priority_queue.cpp — std::priority_queue 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra priority_queue.cpp -o priority_queue && ./priority_queue
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/priority_queue
//  參考 (cplusplus.com): https://cplusplus.com/reference/queue/priority_queue/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::priority_queue 是「最大值優先 (max-heap)」的容器轉接器。
//  每次 top() 都拿到目前「最大」的元素 (預設用 std::less,即 max-heap)。
//
//  ▌ 底層 container
//  template <
//      typename T,
//      typename Container = std::vector<T>,
//      typename Compare   = std::less<typename Container::value_type>
//  > class priority_queue;
//
//  預設底層為 std::vector,搭配 std::make_heap / push_heap / pop_heap 演算法。
//  也可指定為 std::deque。但 list / forward_list 不行 (heap 演算法需要隨機存取)。
//
//  ▌ 所屬類別
//  Container adaptor
//
//  ▌ 時間複雜度
//      push    O(log n)
//      pop     O(log n)
//      top     O(1)
//      size    O(1)
//      empty   O(1)
//
//  ▌ 與其他 container 的比較
//      priority_queue vs queue : 一個是「優先級最高先出」,一個是「先進先出」
//      priority_queue vs set   : set 任意位置查詢/刪除 O(log n);
//                                priority_queue 只能拿「最大值」但 push 較快
//
//  ▌ 適用情境
//      ✅ 需要「動態取最大/最小」:Dijkstra、A*、top-k、event scheduler
//      ✅ 不在乎其他元素的順序
//      ❌ 需要「中間查詢」或「按順序走訪」 → 用 set
//      ❌ 需要 thread-safe → 自己加鎖,或用第三方
//
//  ▌ Min-heap 怎麼做?
//      把 Compare 換成 std::greater:
//          std::priority_queue<int, std::vector<int>, std::greater<int>> minq;
//
// ============================================================================

/*
補充筆記：std::priority_queue
  - priority_queue 預設最大值優先，底層通常是 vector 加 heap 操作。
  - 比較器語意容易反直覺：greater<T> 會讓較小值浮到 top。
  - 它不能直接刪除任意元素；若需要 decrease-key 或刪中間元素，要考慮其他資料結構。
  - std::priority_queue 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::priority_queue
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. priority_queue 的底層預設容器是什麼？為什麼不是 deque？
//     答：預設底層是 std::vector，搭配 std::less<T> 以 heap 演算法
//         （make_heap / push_heap / pop_heap）維護一個 max-heap。
//         選 vector 是因為 heap 演算法需要 random access iterator，且 vector 連續記憶體的 cache locality 最好；
//         stack / queue 預設用 deque 則是為了避免擴容時整塊搬移。
//     追問：top / push / pop 的複雜度？（O(1) / O(log n) / O(log n)）
//
// 🔥 Q2. 如何讓 priority_queue 變成 min-heap？
//     答：`std::priority_queue<int, std::vector<int>, std::greater<int>> pq;`——
//         第三個 template 參數是 comparator，預設 std::less 給出 max-heap，換 std::greater 就是 min-heap。
//         注意必須同時寫出第二個參數（底層容器），不能只寫第三個。
//
// ⚠️ 陷阱. comparator 的語意為什麼看起來是反的？
//     答：comp(a, b) 為 true 表示「a 的優先度低於 b」，低優先度的沉底。
//         所以 std::less 讓大的浮到 top（max-heap）、std::greater 讓小的浮到 top（min-heap）。
//     為什麼會錯：大家把它套到 std::sort 的直覺——sort 用 less 得到「升序」，
//         於是以為 priority_queue 用 less 也會是「小的先出」，實際恰好相反。
// ═══════════════════════════════════════════════════════════════════════════

#include <queue>          // priority_queue 在 <queue>
#include <vector>
#include <deque>
#include <iostream>
#include <string>
#include <functional>     // std::greater
#include <unordered_map>  // for LeetCode TopKFrequent

template <typename PQ>
void dump(PQ pq, const std::string& label = "") {
    if (!label.empty()) std::cout << label << " (top→...): ";
    std::cout << "[ ";
    while (!pq.empty()) {
        std::cout << pq.top() << ' ';
        pq.pop();
    }
    std::cout << "]\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    // 預設 max-heap
    std::priority_queue<int> p1;

    // 從 iterator range 建構 — heapify 是 O(n),比逐一 push (O(n log n)) 快
    std::vector<int> data{3, 1, 4, 1, 5, 9, 2, 6};
    std::priority_queue<int> p2(data.begin(), data.end());
    dump(p2, "p2 (max-heap)       ");

    // Min-heap (用 greater)
    std::priority_queue<int, std::vector<int>, std::greater<int>>
        p3(data.begin(), data.end());
    dump(p3, "p3 (min-heap)       ");

    // 用 deque 為底層 (不常見,但合法)
    std::priority_queue<int, std::deque<int>> p4(data.begin(), data.end());
    dump(p4, "p4 (deque-based)    ");

    // ========================================================================
    //  2. 元素存取 — 只有 top()
    // ========================================================================
    //  ★ top() 對空 priority_queue 是 UB
    //  ★ top() 回傳 const reference,不能修改 (改了會破壞 heap 性質)
    std::priority_queue<int> pq;
    pq.push(3);
    pq.push(1);
    pq.push(4);
    pq.push(1);
    pq.push(5);
    std::cout << "\ntop = " << pq.top() << '\n';        // 5

    // ========================================================================
    //  3. 容量
    // ========================================================================
    std::cout << "size=" << pq.size()
              << ", empty=" << std::boolalpha << pq.empty() << '\n';

    // ========================================================================
    //  4. Modifiers
    // ========================================================================

    // ──── push(value) ──── O(log n)
    pq.push(9);
    pq.push(2);
    std::cout << "after push, top=" << pq.top() << '\n';   // 9

    // ──── emplace(args...) ──── 就地建構,C++17 起 emplace 回傳 reference
    std::priority_queue<std::string> ps;
    ps.emplace(5, 'A');
    ps.emplace("hello");
    ps.emplace("world");
    dump(ps, "emplace             ");

    // ──── pop() ──── O(log n),不回傳值
    std::priority_queue<int> pp;
    pp.push(1); pp.push(3); pp.push(2);
    int t = pp.top();
    pp.pop();
    std::cout << "\npop 取到 " << t << '\n';

    // ──── swap ──── O(1) (轉嫁 vector::swap)
    std::priority_queue<int> a, b;
    a.push(1); a.push(5);
    b.push(10);
    a.swap(b);
    dump(a, "a after swap        ");

    // ========================================================================
    //  5. 清空
    // ========================================================================
    // ★ 沒有 clear()。同樣的 swap idiom:
    std::priority_queue<int> cl;
    cl.push(1); cl.push(2);
    std::priority_queue<int>().swap(cl);
    std::cout << "after swap-clear, empty=" << cl.empty() << '\n';

    // ========================================================================
    //  6. 自訂 Compare — 把元素改成自訂結構
    // ========================================================================
    struct Task {
        int priority;
        std::string name;
    };
    // 自訂比較:priority 大的先出 (max-heap by priority)
    auto cmp = [](const Task& a, const Task& b) {
        return a.priority < b.priority;     // 注意:less 表示「a 較小」→ 較小的下沉
    };
    // C++20 起 lambda 可在 unevaluated context 用,decltype(cmp) 就能寫進 template
    std::priority_queue<Task, std::vector<Task>, decltype(cmp)> tq(cmp);
    tq.push({1, "low"});
    tq.push({5, "high"});
    tq.push({3, "mid"});
    while (!tq.empty()) {
        std::cout << "task: " << tq.top().name
                  << "(p=" << tq.top().priority << ")\n";
        tq.pop();
    }

    // ========================================================================
    //  7. 經典應用:Dijkstra 最短路徑 (片段示意)
    // ========================================================================
    // typedef pair<int,int> P;  // (dist, node)
    // priority_queue<P, vector<P>, greater<P>> pq;     // min-heap
    // pq.push({0, start});
    // while (!pq.empty()) {
    //     auto [d, u] = pq.top(); pq.pop();
    //     if (d > dist[u]) continue;          // 已被更新過
    //     for (auto [v, w] : adj[u]) ...
    // }

    // ========================================================================
    //  8. 常見陷阱 (Pitfalls) ★必看
    // ========================================================================
    //
    //  (1) 預設是 max-heap (用 std::less)
    //      想要 min-heap → std::greater
    //      容易記錯:Compare 是「true 表示 a 應該排在 b 後面 (下沉)」。
    //      std::less<int>{}(3, 5) = true → 3 沉下去 → top 是 5 (max-heap)。
    //
    //  (2) top() 不能修改
    //      回傳 const& — 修改會破壞 heap 性質。
    //      要更新元素 → 整個 pop 再 push,或用 set + manual key-decrease。
    //
    //  (3) pop() 不回傳值
    //      先 top() 取值,再 pop()。
    //
    //  (4) 沒有 iterator
    //      不能走訪所有元素 (除非自己 pop 完)。
    //      要看內容 → 改用 std::set + 手動找最大值,或用底層 vector + make_heap。
    //
    //  (5) 沒有 clear()
    //      用 swap idiom。
    //
    //  (6) Compare 必須 strict weak ordering
    //      若提供等價元素 → heap 行為仍正確,但「等價元素出來的順序」不保證。
    //
    //  (7) 大量初始化 → 用 iterator range constructor 比逐一 push 快
    //      O(n) vs O(n log n)。

    // ========================================================================
    //  9. 最佳實踐
    // ========================================================================
    //
    //  • 需要動態取最大/最小 → priority_queue 是首選
    //  • 想要可走訪、可任意刪除 → 改用 set
    //  • 大量初始建構 → 用 iterator range constructor (heapify 較快)
    //  • Min-heap → std::greater
    //  • 自訂 type → 寫 lambda 或 functor 當 Compare
    //  • 想要「decrease-key」(降低某元素的優先級) →
    //    priority_queue 不直接支援,常見作法:
    //      a) 重複 push 新值,pop 時跳過舊的 (lazy deletion)
    //      b) 改用 set / 手寫 indexed heap
    //
    //  • 與 stack / queue 一樣:不是 thread-safe

    // ========================================================================
    //  10. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // priority_queue 在「Top-K 問題」、「合併 K 個有序資料」、「Dijkstra 最短路」
    // 中是核心工具。下面三題涵蓋了 90% 的 heap 套路。

    // ──── LC 215: Kth Largest Element in an Array (陣列中第 K 大元素) ────
    // 經典 min-heap 套路: 維護「大小固定為 K」的 min-heap,
    // 走完後 heap 頂端 (最小者) 就是「第 K 大」。
    // 時間 O(n log k),空間 O(k) — 比完整排序 O(n log n) 快。
    {
        std::vector<int> nums{3, 2, 1, 5, 6, 4};
        int k = 2;
        std::priority_queue<int, std::vector<int>, std::greater<int>> minq;   // min-heap
        for (int x : nums) {
            minq.push(x);
            if ((int)minq.size() > k) minq.pop();
        }
        std::cout << "\n[LC215 KthLargest k=2] = " << minq.top() << '\n';
        // 預期輸出: 5  (第 2 大是 5)
    }

    // ──── LC 347: Top K Frequent Elements (前 K 個高頻元素) ────
    // 先用 unordered_map 計數,再用「大小為 k 的 min-heap」按頻率挑出前 K 個。
    {
        std::vector<int> nums{1, 1, 1, 2, 2, 3};
        int k = 2;
        std::unordered_map<int, int> cnt;
        for (int x : nums) ++cnt[x];

        // min-heap of (freq, value),小的先被擠出
        using P = std::pair<int, int>;
        std::priority_queue<P, std::vector<P>, std::greater<P>> heap;
        for (const auto& [v, c] : cnt) {
            heap.emplace(c, v);
            if ((int)heap.size() > k) heap.pop();
        }
        std::cout << "[LC347 TopKFrequent k=2] = [ ";
        std::vector<int> ans;
        while (!heap.empty()) { ans.push_back(heap.top().second); heap.pop(); }
        for (int x : ans) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出 (順序可能不同): [ 2 1 ]  (頻率最高的兩個是 1 和 2)
    }

    // ──── LC 1046: Last Stone Weight (最後一塊石頭的重量) ────
    // 每次取兩塊最重的石頭撞一下: 等重 → 都消失;不等重 → 留下重量差。
    // 直到剩 0 或 1 塊。max-heap 自然契合「每次取最大兩塊」的需求。
    {
        std::vector<int> stones{2, 7, 4, 1, 8, 1};
        std::priority_queue<int> maxq(stones.begin(), stones.end());   // O(n) heapify
        while (maxq.size() > 1) {
            int a = maxq.top(); maxq.pop();
            int b = maxq.top(); maxq.pop();
            if (a != b) maxq.push(a - b);
        }
        std::cout << "[LC1046 LastStoneWeight] = "
                  << (maxq.empty() ? 0 : maxq.top()) << '\n';
        // 預期輸出: 1
    }

    // ──── LC 703: Kth Largest Element in a Stream (資料流第 K 大) ────
    // 難度: medium
    // 設計類別:add(val) 後即時回傳「目前所有元素中第 K 大」。
    // 用 size = k 的 min-heap:新元素 add 進去後若 size > k 就 pop,
    // 頂端永遠是第 K 大 (因為比它大的有 k-1 個位於堆中)。
    {
        struct KthLargest {
            int k;
            std::priority_queue<int, std::vector<int>, std::greater<int>> minq;
            KthLargest(int k_, std::vector<int> nums) : k(k_) {
                for (int x : nums) add(x);
            }
            int add(int val) {
                minq.push(val);
                if ((int)minq.size() > k) minq.pop();
                return minq.top();
            }
        };
        KthLargest kl(3, {4, 5, 8, 2});
        std::cout << "[LC703 Stream] "
                  << kl.add(3)  << ' '       // 4
                  << kl.add(5)  << ' '       // 5
                  << kl.add(10) << ' '       // 5
                  << kl.add(9)  << ' '       // 8
                  << kl.add(4)  << '\n';     // 8
    }

    // ========================================================================
    //  11. 實戰範例:任務排程 (Job Scheduler) — 依優先級執行
    // ========================================================================
    // 真實場景:作業系統 / 訊息中介軟體常需依「優先權」執行任務。例如:
    //   • 緊急修復 (priority=10) 必須最先執行
    //   • 一般使用者請求 (priority=5)
    //   • 背景批次 (priority=1)
    // priority_queue 預設 max-heap,把 (priority, task_name) pair 丟進去,
    // top() 永遠是最緊急的任務,直接執行。
    {
        using Job = std::pair<int, std::string>;   // (priority, name)
        std::priority_queue<Job> pq;
        pq.emplace(5,  "User Request A");
        pq.emplace(10, "Critical: DB outage");
        pq.emplace(1,  "Background cleanup");
        pq.emplace(7,  "Send notification");

        std::cout << "[Job Scheduler] 依優先級執行:\n";
        while (!pq.empty()) {
            auto [p, name] = pq.top(); pq.pop();
            std::cout << "  [P=" << p << "] " << name << '\n';
        }
        // 預期輸出: P=10 critical / P=7 notify / P=5 user / P=1 cleanup
    }

    std::cout << "\n=== priority_queue demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：priority_queue 預設是 max-heap,要怎麼變 min-heap?
    //    A：傳第三個 template 參數 std::greater<T>,例如
    //       std::priority_queue<int, std::vector<int>, std::greater<int>> minq;
    //       注意要連底層 container (第二個參數) 也明寫,因為 template 預設參數規則。
    //
    //  Q2：priority_queue 為何沒有 begin / end 也不能走訪所有元素?
    //    A：它是 container adaptor,設計上只暴露 top / push / pop 三個操作,符合
    //       heap 的抽象。底層雖是 vector + heap algorithm,但內部順序不是排序的,
    //       直接走訪沒有意義。要列舉所有元素請改用 multiset 或 sort 過的 vector。
    //
    //  Q3：priority_queue 能修改某個元素的優先權嗎?
    //    A：標準 API 沒辦法。常見替代方案:(1) 「lazy deletion」— 加新值並打標籤,
    //       pop 時若是過期值就跳過;(2) 改用 std::set / multiset 才能 O(log n) 任意刪。
    //       這也是它與 Dijkstra 教科書版實作落差最大的地方。
    //
    return 0;
}

/*
============================================================================
  附錄:std::priority_queue 完整 member function 一覽
============================================================================
  Constructors / destructor / operator=

  Element access:  top   (const&,不可修改)

  Capacity:        empty, size

  Modifiers:       push, push_range (C++23), emplace, pop, swap

  ★ 沒有:iterator、clear、front、back、find、operator[]

  Compare 規則 (★容易記錯):
      Compare(a, b) 為 true 表示「a 應該排在 b 後面 (a 沉下去)」
      → std::less    : 「比較小的沉下去」 → top 是最大值 (max-heap)
      → std::greater : 「比較大的沉下去」 → top 是最小值 (min-heap)
============================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra priority_queue.cpp -o priority_queue

// === 預期輸出 (節錄) ===
// p2 (max-heap)        (top→...): [ 9 6 5 4 3 2 1 1 ]
// p3 (min-heap)        (top→...): [ 1 1 2 3 4 5 6 9 ]
// p4 (deque-based)     (top→...): [ 9 6 5 4 3 2 1 1 ]
//
// top = 5
// size=5, empty=false
// after push, top=9
// emplace              (top→...): [ world hello AAAAA ]
//
// pop 取到 3
// a after swap         (top→...): [ 10 ]
// after swap-clear, empty=true
// task: high(p=5)
// task: mid(p=3)
// task: low(p=1)
//
// [LC215 KthLargest k=2] = 5
// [LC347 TopKFrequent k=2] = [ 2 1 ]
// [LC1046 LastStoneWeight] = 1
// [LC703 Stream] 4 5 5 8 8
// …（後略，完整輸出共 27 行）
