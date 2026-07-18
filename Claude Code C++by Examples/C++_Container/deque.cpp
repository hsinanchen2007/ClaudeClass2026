// ============================================================================
//  deque.cpp — std::deque 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra deque.cpp -o deque && ./deque
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/deque
//  參考 (cplusplus.com): https://cplusplus.com/reference/deque/deque/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::deque (double-ended queue,雙端佇列) 是支援「兩端」O(1) 插入/刪除的
//  序列容器,同時也提供隨機存取。
//
//  ▌ 底層資料結構
//  典型實作為「分塊陣列 (chunked array / array of arrays)」:
//
//      map (block 指標表)
//      ┌───┬───┬───┬───┬───┐
//      │ ▼ │ ▼ │ ▼ │ ▼ │ ▼ │
//      └─┬─┴─┬─┴─┬─┴─┬─┴─┬─┘
//        ↓   ↓   ↓   ↓   ↓
//      ┌───┐┌───┐┌───┐┌───┐┌───┐
//      │...││...││...││...││...│   每塊固定大小 (e.g. 512 bytes)
//      └───┘└───┘└───┘└───┘└───┘
//
//  因此 deque 的元素「不是」連續記憶體 — 跨 block 的指標運算是不合法的。
//
//  ▌ 所屬類別
//  Sequence container
//
//  ▌ 時間複雜度
//      隨機存取 (operator[])             O(1)  但常數比 vector 大
//      push_front / push_back            O(1)  攤銷
//      pop_front  / pop_back             O(1)
//      中間 insert / erase               O(n)
//      size / empty                      O(1)
//
//  ▌ 與其他 container 的比較
//      vector vs deque  : vector 連續記憶體 (data() 可用),deque 不連續
//                         vector 只有尾端 O(1),deque 兩端皆 O(1)
//      list   vs deque  : list 任意位置 O(1),但無隨機存取且 cache 差
//      queue / stack    : 容器轉接器,預設底層就是 deque
//
//  ▌ 適用情境
//      ✅ 需要兩端高效 insert/erase (FIFO buffer / 滑動視窗 / 雙端佇列)
//      ✅ 想要 vector 的隨機存取,但又要 push_front
//      ❌ 需要連續記憶體傳給 C API → deque 沒有 data(),改用 vector
//      ❌ 中間頻繁 insert/erase → 改用 list
//
//  ▌ Iterator 失效規則 ★
//      • insert / erase 在「兩端」  → references 不失效,但 iterator 全部失效
//      • insert / erase 在「中間」  → 全部失效
//      • push_back / push_front     → iterator 全失效,但 references 不失效
//      • pop_back  / pop_front      → 只有被刪元素的 iterator/reference 失效
//      ★ 這個 references 不失效的特性是 deque 與 vector 的關鍵差異之一
//
// ============================================================================

/*
補充筆記：std::deque
  - deque 支援頭尾快速插入，內部通常是分段連續儲存，不等同 vector。
  - 隨機存取可用，但快取區域性通常不如 vector。
  - 頭尾 push/pop 的失效規則和 vector 不同，保存 iterator 前要查清楚。
  - std::deque 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/
#include <deque>
#include <iostream>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <vector>     // for LeetCode 範例

template <typename T>
void print(const std::deque<T>& d, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (const auto& e : d) std::cout << e << ' ';
    std::cout << "] (size=" << d.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::deque<int> d1;                            // (1) 空 deque
    std::deque<int> d2(5);                         // (2) 5 個 0
    std::deque<int> d3(5, 42);                     // (3) 5 個 42
    std::deque<int> d4{1, 2, 3, 4, 5};             // (4) initializer_list
    std::deque<int> d5(d4.begin(), d4.end());      // (5) iterator range
    std::deque<int> d6(d4);                        // (6) copy
    std::deque<int> d7(std::move(d6));             // (7) move

    print(d1, "d1                  ");
    print(d2, "d2 (size=5)         ");
    print(d3, "d3 (5 個 42)        ");
    print(d4, "d4                  ");
    print(d5, "d5 (range)          ");
    print(d7, "d7 (move 來)        ");

    // ========================================================================
    //  2. assign
    // ========================================================================
    std::deque<int> a;
    a.assign(3, 99);
    print(a, "assign(3,99)        ");
    a.assign({7, 8, 9});
    print(a, "assign({...})       ");

    // ========================================================================
    //  3. 元素存取
    // ========================================================================
    //  operator[] / at / front / back
    //  ★ deque 沒有 data(),因為記憶體不連續

    std::deque<int> e{10, 20, 30, 40};
    std::cout << "\n[Access]\n";
    std::cout << "e[1]      = " << e[1] << '\n';
    std::cout << "e.at(2)   = " << e.at(2) << '\n';
    std::cout << "e.front() = " << e.front() << '\n';
    std::cout << "e.back()  = " << e.back() << '\n';

    try { e.at(99); }
    catch (const std::out_of_range& ex) {
        std::cout << "at(99) 例外: " << ex.what() << '\n';
    }

    // ========================================================================
    //  4. Iterators
    // ========================================================================
    // begin/end/cbegin/cend/rbegin/rend/crbegin/crend
    // 為 random access iterator,但「跨 block」的 + - 運算成本比 vector 高。

    std::cout << "\n[正向] ";
    for (auto it = e.begin(); it != e.end(); ++it) std::cout << *it << ' ';
    std::cout << "\n[反向] ";
    for (auto it = e.rbegin(); it != e.rend(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  5. 容量
    // ========================================================================
    //  empty / size / max_size / shrink_to_fit
    //  ★ deque 沒有 capacity / reserve — 因為它的成長策略是配置新 block
    //     而非 reallocate 整段記憶體。

    std::cout << "\n[Capacity] size=" << e.size()
              << ", empty=" << std::boolalpha << e.empty() << '\n';
    e.shrink_to_fit();   // 釋放未使用的 block (實作可選)
    std::cout << "after shrink_to_fit, size=" << e.size() << '\n';

    // max_size():理論上限 (size_t / sizeof(T) 量級)
    std::cout << "max_size = " << e.max_size() << '\n';

    // get_allocator():取得目前使用的 allocator
    auto alloc = e.get_allocator();
    int* tmp = alloc.allocate(2);
    alloc.deallocate(tmp, 2);
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  6. Modifiers
    // ========================================================================

    // clear: 清空,size → 0
    std::deque<int> m{1,2,3,4,5};
    m.clear();
    print(m, "after clear         ");

    // insert: 中間插入
    std::deque<int> m1{1, 2, 5};
    m1.insert(m1.begin() + 2, 3);             // [1,2,3,5]
    m1.insert(m1.begin() + 3, 2, 4);          // [1,2,3,4,4,5]
    m1.insert(m1.end(), {6, 7});              // [1,2,3,4,4,5,6,7]
    print(m1, "after insert        ");

    // emplace
    std::deque<std::string> m2;
    m2.emplace(m2.end(), 5, 'A');
    print(m2, "after emplace       ");

    // erase
    std::deque<int> m3{1,2,3,2,4,2,5};
    m3.erase(std::remove(m3.begin(), m3.end(), 2), m3.end());
    print(m3, "erase-remove all 2  ");

    // ──── push_back / push_front ────  ★ deque 獨有 push_front
    std::deque<int> m4;
    m4.push_back(2);
    m4.push_back(3);
    m4.push_front(1);                          // [1,2,3]
    m4.push_front(0);                          // [0,1,2,3]
    print(m4, "push_back/front     ");

    // ──── emplace_back / emplace_front ────
    std::deque<std::pair<int,std::string>> m5;
    m5.emplace_back(1, "back");
    m5.emplace_front(0, "front");
    std::cout << "emplace_back/front: ";
    for (auto& [k,v] : m5) std::cout << '(' << k << ',' << v << ") ";
    std::cout << '\n';

    // ──── pop_back / pop_front ────
    m4.pop_back();
    m4.pop_front();
    print(m4, "after pop_b/pop_f   ");

    // resize
    std::deque<int> m6{1, 2, 3};
    m6.resize(5);                              // [1,2,3,0,0]
    print(m6, "resize(5)           ");
    m6.resize(7, 9);
    print(m6, "resize(7,9)         ");
    m6.resize(2);
    print(m6, "resize(2)           ");

    // swap: deque 的 swap 是 O(1) (與 vector 相同,只交換內部指標表)
    std::deque<int> s1{1,2,3}, s2{9,8};
    s1.swap(s2);
    print(s1, "s1 after swap       ");
    print(s2, "s2 after swap       ");

    // ========================================================================
    //  7. 比較
    // ========================================================================
    std::deque<int> c1{1,2,3}, c2{1,2,4};
    std::cout << "\n[比較] c1 < c2 ? " << (c1 < c2) << '\n';

    // ========================================================================
    //  8. C++20 free functions
    // ========================================================================
    std::deque<int> n1{1,2,3,4,5,6};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(偶數)      ");

    // ========================================================================
    //  9. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) 沒有 data() 函式
    //      deque 元素不連續,不能傳給 C API。需要連續 → 改用 vector。
    //
    //  (2) push_back / push_front 會使所有 iterator 失效
    //      但 reference 不會失效 — 這跟 vector 不同。
    //
    //  (3) 隨機存取 O(1) 但常數較大
    //      operator[] 要先算「在哪個 block」再算「block 內 offset」,
    //      cache 命中率比 vector 差。性能敏感時優先 benchmark。
    //
    //  (4) shrink_to_fit 行為實作定義
    //      可能釋放不到記憶體,別當銀彈。
    //
    //  (5) 別假設 block size
    //      libstdc++ 為 512 bytes 一塊,但這非標準規定。
    //
    //  (6) deque<bool> 是真的 bool deque
    //      不像 vector<bool> 是 bit-packed proxy,deque<bool> 行為正常。

    // ========================================================================
    //  10. 最佳實踐
    // ========================================================================
    //
    //  • 雙端佇列 / FIFO buffer / 滑動視窗 → deque 是首選
    //  • 想要 vector 的特性又要 push_front → deque
    //  • stack / queue 的底層預設就是 deque,不用自己再包一層
    //  • 巨量元素且只在尾端 push → vector 通常更快 (cache 友善)
    //  • 與 C API 互動 → 用 vector,不用 deque

    // ========================================================================
    //  11. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // deque 在演算法題的最重要應用是「單調隊列 (monotonic deque)」,
    // 它能在 O(n) 時間內解決「滑動視窗最大/最小值」這類問題。

    // ──── LC 239: Sliding Window Maximum (滑動視窗最大值) ────
    // 給陣列 nums 與視窗大小 k,回傳每個視窗的最大值。
    // 核心思路: 維護一個「下標的單調遞減 deque」,
    //   1. 視窗右移時,從尾部彈掉所有「比新元素小」的舊下標 (它們永遠不會是最大值)
    //   2. 從頭部彈掉「不在視窗內」的下標 (i - k)
    //   3. 此時 deque.front() 就是當前視窗的最大值
    // 時間複雜度 O(n) — 每個元素最多進出 deque 一次。
    {
        std::vector<int> nums{1, 3, -1, -3, 5, 3, 6, 7};
        int k = 3;
        std::deque<int> dq;          // 存放下標,nums[dq] 嚴格遞減
        std::vector<int> ans;
        for (int i = 0; i < (int)nums.size(); ++i) {
            while (!dq.empty() && dq.front() <= i - k) dq.pop_front();      // 過期
            while (!dq.empty() && nums[dq.back()] < nums[i]) dq.pop_back(); // 維持遞減
            dq.push_back(i);
            if (i >= k - 1) ans.push_back(nums[dq.front()]);
        }
        std::cout << "\n[LC239 Sliding Window Max] = [ ";
        for (int x : ans) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 3 3 5 5 6 7 ]
    }

    // ──── LC 933: Number of Recent Calls (最近的請求次數) ────
    // 設計一個 RecentCounter 類別,ping(t) 回傳 [t-3000, t] 區間內的請求數。
    // 用 deque 把「過期的請求」從頭部 pop_front 掉就行,十分自然。
    {
        struct RecentCounter {
            std::deque<int> q;
            int ping(int t) {
                q.push_back(t);
                while (q.front() < t - 3000) q.pop_front();
                return (int)q.size();
            }
        } rc;
        std::cout << "[LC933 RecentCounter] ";
        std::cout << rc.ping(1)    << ' ';     // 1
        std::cout << rc.ping(100)  << ' ';     // 2
        std::cout << rc.ping(3001) << ' ';     // 3
        std::cout << rc.ping(3002) << '\n';    // 3 (t=1 已過期)
        // 預期輸出: 1 2 3 3
    }

    // ──── LC 950: Reveal Cards In Increasing Order (按遞增順序顯示卡片) ────
    // 難度: medium
    // 題意:給一組已排序的牌,要重排成「翻一張、把下一張塞到底」的順序執行時
    // 會剛好依遞增順序出現。
    // 思路:「逆向操作」用 deque 模擬。把排序後的牌由大到小逐張放回:
    //   1. 先把 deque 的尾部移到頭部 (對應反向的「移底到頂」)
    //   2. 再把當前最大張塞到 deque 頭部
    // 為何用 deque:同時需要 push_front 與 pop_back,兩端都是 O(1)。
    {
        std::vector<int> deck{17, 13, 11, 2, 3, 5, 7};
        std::sort(deck.begin(), deck.end());     // 1 2 3 5 7 11 13 17 -> 排序後 [2,3,5,7,11,13,17]
        std::deque<int> dq;
        for (auto it = deck.rbegin(); it != deck.rend(); ++it) {
            if (!dq.empty()) {
                dq.push_front(dq.back());
                dq.pop_back();
            }
            dq.push_front(*it);
        }
        std::cout << "[LC950 RevealCards] = [ ";
        for (int x : dq) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 2 13 3 11 5 17 7 ]
    }

    // ========================================================================
    //  12. 實戰範例:命令歷史 / Undo-Redo Buffer
    // ========================================================================
    // 真實場景:文字編輯器、繪圖軟體常要維護「最近 N 個操作」的歷史紀錄,
    // 並支援以下操作:
    //   • 新增操作 → push_back  (O(1))
    //   • 撤銷 (Undo)  → pop_back  (O(1))
    //   • 超過容量丟掉最舊 → pop_front (O(1))
    // 這正是 deque 雙端 O(1) 的最佳應用。vector 只有單端,list 又沒有索引。
    {
        const size_t MAX_HISTORY = 5;
        std::deque<std::string> history;
        auto do_cmd = [&](const std::string& cmd) {
            history.push_back(cmd);
            if (history.size() > MAX_HISTORY) history.pop_front();  // 超量丟最舊
        };
        for (auto& c : {"open", "edit", "save", "copy", "paste", "cut", "exit"}) do_cmd(c);
        std::cout << "[Undo Buffer] 最近 " << MAX_HISTORY << " 個命令 = [ ";
        for (auto& c : history) std::cout << c << ' ';
        std::cout << "]\n";
        // 預期輸出: [ save copy paste cut exit ]  (open / edit 已被擠掉)
    }

    std::cout << "\n=== deque demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：deque 的 push_front 為何也是 O(1)?vector 不能也這樣做嗎?
    //    A：deque 底層是「分塊陣列」(chunked array),由多個固定大小的 buffer 拼接,
    //       兩端各有一個指標可獨立成長。vector 只有一塊連續記憶體,push_front 必須
    //       搬移所有元素 (O(n)) 才能空出位置。
    //
    //  Q2：deque 既然兩端都 O(1),為什麼大家還愛用 vector?
    //    A：deque 元素「不保證連續」,無法傳給 C API (data() 不存在);分塊存取
    //       cache locality 較差,operator[] 多一次間接;每個 chunk 的 heap 配置
    //       比一整塊大記憶體的 vector 更耗時。多數情境 vector 仍是首選。
    //
    //  Q3：deque 的 iterator 在 push_front / push_back 後會失效嗎?
    //    A：只要做 push_front 或 push_back,所有 iterator 都「可能」失效 (因可能
    //       要分配新的 chunk 並更新中央索引);但 reference / pointer 仍有效。
    //       這點和 vector 完全相反,vector 是 reference 與 iterator 一起失效。
    //
    return 0;
}

/*
============================================================================
  附錄:std::deque 完整 member function 一覽
============================================================================
  Constructors / destructor / operator=

  assign / assign_range (C++23) / get_allocator

  Element access:  at, operator[], front, back

  Iterators:       begin/end/cbegin/cend/rbegin/rend/crbegin/crend

  Capacity:        empty, size, max_size, shrink_to_fit
                   (沒有 capacity / reserve)

  Modifiers:       clear, insert, insert_range (C++23), emplace, erase,
                   push_front, emplace_front, pop_front,
                   push_back,  emplace_back,  pop_back,
                   prepend_range / append_range (C++23),
                   resize, swap

  Non-member:      operator==, !=, <, <=, >, >=, <=> (C++20)
                   std::swap, std::erase, std::erase_if (C++20)
============================================================================
*/
