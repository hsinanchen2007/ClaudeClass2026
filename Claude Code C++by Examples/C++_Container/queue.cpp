// ============================================================================
//  queue.cpp — std::queue 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra queue.cpp -o queue && ./queue
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/queue
//  參考 (cplusplus.com): https://cplusplus.com/reference/queue/queue/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::queue 是「先進先出 (FIFO)」的容器轉接器。
//  與 stack 一樣是 wrapper,只暴露 queue 介面 (push/pop/front/back)。
//
//  ▌ 底層 container
//  template <typename T, typename Container = std::deque<T>>
//  class queue;
//
//  預設底層為 std::deque (兩端 O(1) 是 queue 必要條件)。
//  也可指定為 std::list。
//  ★ 不能用 std::vector — 因為 vector 沒有 pop_front。
//
//  底層 container 必須支援:
//      front(), back(), push_back(), pop_front(), empty(), size()
//
//  ▌ 所屬類別
//  Container adaptor
//
//  ▌ 時間複雜度
//      push / pop / front / back / size / empty   全部 O(1)
//
//  ▌ 與其他 container 的比較
//      queue vs stack          : queue FIFO,stack LIFO
//      queue vs priority_queue : priority_queue 是「最大值先出」,queue 是「先進先出」
//      queue vs deque          : queue 限制介面,deque 是通用雙端
//
//  ▌ 適用情境
//      ✅ FIFO 場景:BFS、message queue、producer-consumer 緩衝
//      ✅ 想限制介面避免誤操作
//      ❌ 需要走訪所有元素 → queue 沒有 iterator,改用 deque
//
//  ▌ 沒有的功能 (與 stack 類似)
//      • 沒有 begin / end / iterator
//      • 沒有 operator[] / at / find
//      • 沒有 clear
//
// ============================================================================

/*
補充筆記：std::queue
  - queue 是 FIFO adaptor，只暴露 front/back/push/pop。
  - pop 不回傳元素，若要取值要先 front 再 pop。
  - 它適合表達排隊語意；需要 priority 時改用 priority_queue。
  - std::queue 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::queue
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. queue 的底層預設容器是什麼？為什麼不能用 std::vector？
//     答：queue 是 container adaptor，預設底層是 std::deque，也可換成 std::list。
//         它要求底層提供 front() / back() / push_back() / pop_front() / empty() / size()，
//         而 std::vector 沒有 pop_front()（從頭刪除是 O(n)，標準沒提供這個介面），所以不能當 queue 底層。
//
// 🔥 Q2. queue 為什麼沒有 iterator、也沒有 clear()？
//     答：因為 adaptor 的設計目的就是「只暴露 FIFO 介面」，避免使用者繞過 push / pop 直接動資料；
//         所以沒有 begin / end（不能用 range-for）、沒有 operator[] / find、也沒有 clear。
//         要清空只能 `while (!q.empty()) q.pop();`，或直接指派一個新的空 queue。
//     追問：需要走訪所有元素時怎麼辦？（不要用 adaptor，直接用 std::deque）
//
// Q3. queue 和 priority_queue 的差別？
//     答：queue 是嚴格 FIFO（先進先出），push / pop / front / back 都是 O(1)；
//         priority_queue 是「優先度最高者先出」，底層是 heap，push / pop 為 O(log n)、top 為 O(1)，
//         且只能看 top（沒有 back）。BFS 用 queue；Dijkstra / Top-K 用 priority_queue。
// ═══════════════════════════════════════════════════════════════════════════

#include <queue>
#include <deque>
#include <list>
#include <iostream>
#include <string>

template <typename Q>
void dump(Q q, const std::string& label = "") {
    if (!label.empty()) std::cout << label << " (front→back): ";
    std::cout << "[ ";
    while (!q.empty()) {
        std::cout << q.front() << ' ';
        q.pop();
    }
    std::cout << "]\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::queue<int> q1;                                // 預設 deque
    std::queue<int, std::list<int>> q2;                // 用 list 為底層
    std::deque<int> d{1, 2, 3};
    std::queue<int> q3(d);                             // 由 container 建構
    std::queue<int> q4(std::move(d));

    q3.push(4);
    dump(q3, "q3                  ");

    // ========================================================================
    //  2. 元素存取
    // ========================================================================
    //  front() : 取最前面的元素 (FIFO 中下一個要被 pop 的)
    //  back()  : 取最後面的元素 (剛剛 push 的)
    //  ★ 對空 queue 呼叫是 UB
    std::queue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);
    std::cout << "\nfront = " << q.front()
              << ", back = " << q.back() << '\n';      // 10, 30

    // 可以修改
    q.front() = 100;
    q.back()  = 300;
    dump(q, "modified            ");

    // ========================================================================
    //  3. 容量
    // ========================================================================
    std::cout << "size=" << q.size()
              << ", empty=" << std::boolalpha << q.empty() << '\n';

    // ========================================================================
    //  4. Modifiers
    // ========================================================================

    // ──── push(value) ────  從尾端加入
    std::queue<int> p;
    p.push(1);
    p.push(2);
    p.push(3);
    dump(p, "after push          ");

    // ──── emplace(args...) ────  尾端就地建構,C++17 起回傳 reference
    std::queue<std::string> ps;
    ps.emplace(5, 'A');                                // "AAAAA"
    ps.emplace("hi");
    dump(ps, "emplace             ");

    // ──── pop() ────  ★ 從前端移除,「不回傳值」
    std::queue<int> pp;
    pp.push(1); pp.push(2); pp.push(3);
    int front_val = pp.front();
    pp.pop();
    std::cout << "\npop 取到 " << front_val << '\n';

    // ──── swap ────
    std::queue<int> a, b;
    a.push(1); a.push(2);
    b.push(9);
    a.swap(b);
    dump(a, "a after swap        ");

    // ========================================================================
    //  5. 清空 queue
    // ========================================================================
    // queue 也沒有 clear()。同樣三種寫法:
    //   while (!q.empty()) q.pop();
    //   q = std::queue<int>();
    //   std::queue<int>().swap(q);

    std::queue<int> cl;
    cl.push(1); cl.push(2);
    std::queue<int>().swap(cl);
    std::cout << "after swap-clear, empty=" << cl.empty() << '\n';

    // ========================================================================
    //  6. 比較
    // ========================================================================
    std::queue<int> c1, c2;
    c1.push(1); c1.push(2);
    c2.push(1); c2.push(3);
    std::cout << "c1 < c2 ? " << (c1 < c2) << '\n';

    // ========================================================================
    //  7. 經典應用:BFS
    // ========================================================================
    // 這裡示意:層序印出二元樹 (用 vector 模擬)
    std::vector<int> tree{1, 2, 3, 4, 5, 6, 7};   // 1
    std::queue<size_t> bfs;                       //  ↙ ↘
    bfs.push(0);                                  // 2  3
    std::cout << "\nBFS: ";
    while (!bfs.empty()) {
        size_t i = bfs.front();
        bfs.pop();
        if (i >= tree.size()) continue;
        std::cout << tree[i] << ' ';
        bfs.push(2 * i + 1);
        bfs.push(2 * i + 2);
    }
    std::cout << '\n';

    // ========================================================================
    //  8. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) 不能用 vector 為底層
    //      因為 vector 沒有 pop_front。
    //
    //  (2) pop() 不回傳值
    //      要取值要先 front() 再 pop()。
    //
    //  (3) front() / back() 對空 queue 是 UB
    //
    //  (4) 沒有 iterator
    //      要走訪 → 用底層 container (deque / list)。
    //
    //  (5) 沒有 clear()
    //      用 swap idiom 或 while-pop。
    //
    //  (6) std::queue 不是執行緒安全
    //      多執行緒場景請用 mutex 或考慮第三方 lock-free queue。

    // ========================================================================
    //  9. 最佳實踐
    // ========================================================================
    //
    //  • FIFO 場景 → 用 queue 限制介面
    //  • BFS / producer-consumer → queue 是首選
    //  • 預設 deque 已是最佳底層選擇
    //  • 想要 priority (取最大/最小) → 改用 priority_queue
    //  • 想要走訪所有元素 → 直接用 deque

    // ========================================================================
    //  10. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // queue 是 BFS (廣度優先搜尋) 的標準容器,從矩陣搜尋、最短路徑、二元樹層序
    // 走訪、拓撲排序到網路爬蟲,都是同一個套路: 「進佇列 → 取出 → 處理 → 鄰居入佇列」。

    // ──── LC 200: Number of Islands (島嶼數量) ────
    // 給 0/1 矩陣,「上下左右」相連的 1 為一座島,問共幾座島。
    // BFS: 遇到 '1' 就 ++count,從該點開始 BFS 把整座島標記為 visited。
    {
        std::vector<std::vector<char>> grid{
            {'1','1','0','0','0'},
            {'1','1','0','0','0'},
            {'0','0','1','0','0'},
            {'0','0','0','1','1'}
        };
        int rows = grid.size(), cols = grid[0].size();
        int islands = 0;
        constexpr int dr[4] = {-1, 1, 0, 0};
        constexpr int dc[4] = {0, 0, -1, 1};
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (grid[r][c] != '1') continue;
                ++islands;
                std::queue<std::pair<int,int>> bfs;
                bfs.emplace(r, c);
                grid[r][c] = '0';                       // 標記為已訪問
                while (!bfs.empty()) {
                    auto [cr, cc] = bfs.front(); bfs.pop();
                    for (int d = 0; d < 4; ++d) {
                        int nr = cr + dr[d], nc = cc + dc[d];
                        if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                        if (grid[nr][nc] != '1') continue;
                        grid[nr][nc] = '0';
                        bfs.emplace(nr, nc);
                    }
                }
            }
        }
        std::cout << "\n[LC200 NumIslands] = " << islands << '\n';
        // 預期輸出: 3
    }

    // ──── LC 933: Number of Recent Calls (最近的請求次數) ────
    // ping(t) 回傳 [t-3000, t] 區間內的請求數。經典「視窗 + queue」題。
    {
        struct RecentCounter {
            std::queue<int> q;
            int ping(int t) {
                q.push(t);
                while (q.front() < t - 3000) q.pop();
                return (int)q.size();
            }
        } rc;
        std::cout << "[LC933 RecentCounter] "
                  << rc.ping(1) << ' '       // 1
                  << rc.ping(100) << ' '     // 2
                  << rc.ping(3001) << ' '    // 3
                  << rc.ping(3002) << '\n';  // 3
    }

    // ──── LC 225: Implement Stack using Queues (用 queue 實作 stack) ────
    // 用兩個 queue 模擬 stack:push 時把新元素丟進空 queue,再把另一個 queue 全部
    // 倒進來,就讓「最新元素」位於 front,正好對應 stack.top()。
    // 重點在理解 queue 與 stack 的雙向轉換。
    {
        struct MyStack {
            std::queue<int> q1, q2;
            void push(int x) {
                q2.push(x);
                while (!q1.empty()) { q2.push(q1.front()); q1.pop(); }
                std::swap(q1, q2);   // q1 永遠是「stack 視角的」隊列
            }
            int  top() { return q1.front(); }
            void pop() { q1.pop(); }
            bool empty() { return q1.empty(); }
        };
        MyStack s;
        s.push(1); s.push(2); s.push(3);
        std::cout << "[LC225 Stack via Queue] top=" << s.top() << '\n';   // 3
        s.pop();
        std::cout << "[LC225 Stack via Queue] top=" << s.top() << '\n';   // 2
    }

    // ========================================================================
    //  12. 實戰範例:訊息佇列 (Message Queue / Producer-Consumer)
    // ========================================================================
    // 真實場景:生產者-消費者模式。Producer 把任務 push 到 queue 尾端,
    // Consumer 從 front 取出處理。queue 嚴格的 FIFO 語意保證「公平處理」 —
    // 先到的任務先被執行,避免任務餓死。
    // (多執行緒場景要加 mutex / condition_variable,此處示範單執行緒邏輯。)
    {
        std::queue<std::string> mq;

        // Producer:三個任務
        mq.push("send_email:alice@example.com");
        mq.push("resize_image:photo_001.jpg");
        mq.push("backup_database");

        // Consumer:處理直到佇列空
        std::cout << "[Message Queue] 處理順序:\n";
        int n = 1;
        while (!mq.empty()) {
            std::cout << "  #" << n++ << " 處理: " << mq.front() << '\n';
            mq.pop();
        }
        // 預期輸出: 依 FIFO 順序處理三個任務
    }

    std::cout << "\n=== queue demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::queue 是 container 還是 adaptor?底層預設用什麼?
    //    A：是 container adaptor (容器轉接器),預設底層 container 為 std::deque。
    //       deque 兩端 push/pop 都是 O(1) 且不需要 realloc,適合 queue 的 FIFO 操作。
    //       也可以指定 std::list,但不能用 vector (vector 沒有 push_front)。
    //
    //  Q2：queue 為何不暴露 begin / end?要走訪元素該怎麼辦?
    //    A：FIFO 抽象只允許「在尾端進、頭端出」,中間不該存取。如果你需要走訪,
    //       代表用錯容器了 — 改用 std::deque 或 std::vector 直接操作。
    //
    //  Q3：BFS 為何幾乎必用 queue?用 stack 會發生什麼?
    //    A：queue 保證「先入先出」,層序展開節點,因此每次 pop 出的都是距離起點
    //       最近的節點,對「無權圖最短路徑」自然成立。改用 stack 會變成 DFS,
    //       搜出的路徑可能不是最短,且需要顯式記錄距離才能模擬層序。
    //
    return 0;
}

/*
============================================================================
  附錄:std::queue 完整 member function 一覽
============================================================================
  Constructors / destructor / operator=

  Element access:  front, back

  Capacity:        empty, size

  Modifiers:       push, push_range (C++23), emplace, pop, swap

  Non-member:      operator==, !=, <, <=, >, >=, <=> (C++20)
                   std::swap

  ★ 沒有 iterator、clear、capacity、reserve
============================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra queue.cpp -o queue

// === 預期輸出 ===
// q3                   (front→back): [ 1 2 3 4 ]
//
// front = 10, back = 30
// modified             (front→back): [ 100 20 300 ]
// size=3, empty=false
// after push           (front→back): [ 1 2 3 ]
// emplace              (front→back): [ AAAAA hi ]
//
// pop 取到 1
// a after swap         (front→back): [ 9 ]
// after swap-clear, empty=true
// c1 < c2 ? true
//
// BFS: 1 2 3 4 5 6 7
//
// [LC200 NumIslands] = 3
// [LC933 RecentCounter] 1 2 3 3
// [LC225 Stack via Queue] top=3
// [LC225 Stack via Queue] top=2
// [Message Queue] 處理順序:
//   #1 處理: send_email:alice@example.com
//   #2 處理: resize_image:photo_001.jpg
//   #3 處理: backup_database
//
// === queue demo 結束 ===
