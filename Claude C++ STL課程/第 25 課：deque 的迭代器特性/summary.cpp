// =============================================================================
//  summary.cpp  —  deque 的迭代器：介面是 random access，實作是「四指標胖迭代器」
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <deque>
//   迭代器類別: LegacyRandomAccessIterator
//               (C++20 起亦滿足 std::random_access_iterator concept)
//   支援操作與複雜度:
//     ++it / --it / it+n / it-n / it+=n / it-=n     O(1)
//     it2 - it1（距離）                              O(1)
//     it[n]（等同 *(it+n)）                          O(1)
//     <, >, <=, >=, ==, !=（全序比較）               O(1)
//   ★ 因為是 random access，std::sort / std::lower_bound / std::nth_element
//     等「需要隨機存取」的演算法可以直接套用在 deque 上（list 不行）。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 deque 的迭代器不能只是一根指標】
// vector 的元素在單一連續區塊裡，所以它的迭代器可以（而且通常就是）
// 一根裸指標 T*：++it 就是位址 +sizeof(T)，it+n 就是位址 +n*sizeof(T)，
// 兩個迭代器相減直接就是指標相減。整組操作都是單一算術指令。
//
// deque 的元素分散在多個 buffer，一根裸指標**走到 buffer 結尾就沒有下一步了**
// ——它不知道下一個 buffer 在哪。要能跨段前進，迭代器必須額外記住
// 「我在哪一段」以及「這一段的邊界在哪」。
//
// 【2. 四個指標:cur / first / last / node】
// libstdc++（承襲 SGI STL）的 deque 迭代器內含四個成員：
//     cur   → 目前指向的元素
//     first → 目前這個 buffer 的起點
//     last  → 目前這個 buffer 的結尾（past-the-end）
//     node  → 指向 map（中控器）中「目前 buffer 的那一格」
//
//     map:  ... [ node-1 ][ node ][ node+1 ] ...
//                            │
//                            ▼
//              buffer:  [ first .......... cur ....... last )
//
// 有了 first/last 才能判斷「是否已經走到本段邊界」，
// 有了 node 才能在越界時往 map 的左右格取得相鄰 buffer 的位址。
// 本機實測 sizeof(std::deque<int>::iterator) = 32 bytes = 4 個指標，
// 而 sizeof(std::vector<int>::iterator) = 8 bytes（就是一根指標）。
// ★ 這是 libstdc++ 的實作，標準只規定行為與複雜度，未規定內部佈局。
//
// 【3. ++ 與 += 各自做了什麼:為什麼比 vector 貴】
//   operator++：
//       ++cur;
//       if (cur == last) { set_node(node + 1); cur = first; }   // 跨段
//   絕大多數情況只是 ++cur，成本與 vector 幾乎相同；
//   只有每 buffer_size 次會觸發一次 set_node（重設 first/last/node）。
//   → 均攤下來很便宜，但每一步都要**多做一次邊界比較**。
//
//   operator+=(n)：不能只是 cur += n，因為可能跨過任意多個 buffer：
//       offset = n + (cur - first);
//       若 offset 落在本段內 → cur += n
//       否則 → 用除法算出要移動幾個 node（注意負數方向要特別處理），
//              set_node 後再定位段內偏移。
//   → 這裡有**除法/取模**，比 vector 的一次加法貴得多。
//
//   operator-(it2, it1)（距離）：
//       buffer_size * (node差 - 1) + (cur - first) + (last - cur)
//   → 也是常數時間，但顯然不只一條指令。
//
// 【4. 實務結論:遍歷用 range-for / iterator，不要用索引迴圈】
//   for (auto& v : dq) ...                      // 好：段內以 ++cur 連續前進
//   for (size_t i = 0; i < dq.size(); ++i) dq[i] // 差：每次都重算 node/偏移
// 兩者複雜度都是 O(n)，但後者每次迭代都做一次完整的「除法 + 兩次記憶體存取」
// 定址，而前者絕大多數步驟只是 ++cur。這是 deque 最常見的效能誤用。
//
// 【5. 失效規則:iterator 與 reference 在 deque 上會「脫鉤」】
// 這是 deque 迭代器最重要、也最常考的性質：
//   * **頭尾 insert（push_front/push_back）**
//       → **所有 iterator 失效**，但 **reference / pointer 仍然有效**。
//   * **中間 insert**
//       → iterator 與 reference / pointer **全部失效**。
//   * **頭尾 erase（pop_front/pop_back）**
//       → 只有**被刪元素**的 iterator/reference 失效
//         （刪尾端時 end() 也失效）。
//   * **中間 erase**
//       → 全部失效。
//
// 為什麼頭尾插入會讓 iterator 死、reference 卻活著？
// 因為 push_front/push_back **不搬移任何既有元素**，元素位址原封不動，
// 所以 T*/T& 依然有效；但迭代器內部快取了 node（指向 map 某一格），
// 而 map 在成長時可能被**重新配置到別的位址**，那個快取就懸空了。
// → deque 的 reference 比 iterator「更耐用」，這與 vector 完全相反。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 deque 迭代器仍算 O(1) 的 random access
//   雖然 += 要算除法，但那是**與 n 無關的常數次運算**，不隨容器大小或
//   跨越幾段而增加（除法本身是常數時間指令）。因此仍滿足 random access
//   iterator 對「it + n 必須是 O(1)」的要求。複雜度相同、常數不同。
//
// (B) 迭代器類別決定你能用哪些演算法
//   std::sort / nth_element / lower_bound（O(log n) 版）都要求
//   random access iterator：
//     std::sort(dq.begin(), dq.end());        // deque：可以
//     std::sort(lst.begin(), lst.end());      // list：編譯失敗（bidirectional）
//   list 只好提供成員函式 lst.sort()（歸併排序，改指標）。
//   這是「容器選型會連帶決定可用演算法」的典型例子。
//
// (C) end() 的特殊性
//   end() 指向「最後一個元素的下一格」。當最後一個 buffer 正好放滿時，
//   end() 的 cur 會等於該段的 last，實作需要小心處理這個邊界，
//   否則 --end() 會跨錯段。使用者層面只要記住 end() 不可解參考。
//
// (D) 反向迭代器 rbegin()/rend()
//   std::reverse_iterator 內部持有一個「比它實際指向位置多一格」的
//   基底迭代器，*rit 實際回傳 *(base - 1)。所以 rbegin() 的基底是 end()。
//   對 deque 同樣適用，且同樣是 random access。
//
// 【注意事項 Pay Attention】
//  1. **頭尾插入 → 所有 iterator 失效，但 reference/pointer 仍有效**。
//     不要在持有 iterator 的迴圈裡 push_front/push_back。
//  2. 中間 insert/erase → iterator 與 reference **全部**失效。
//  3. 遍歷請用 range-for / iterator，不要用 `dq[i]` 索引迴圈（常數大很多）。
//  4. 迭代器比較（<、-）只在**同一個容器**的迭代器之間有意義；
//     跨容器比較是 UB。
//  5. end() 不可解參考；空容器時 begin() == end()。
//  6. sizeof(iterator) = 32 bytes 是 libstdc++ 實測值，非標準保證；
//     這也意味著「複製 deque 迭代器」比複製 vector 迭代器貴。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的迭代器特性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 的迭代器是哪一類?它跟 vector 的迭代器實作差在哪裡?
//     答：都是 random access iterator，介面完全相同。但 vector 的迭代器
//         通常就是一根裸指標（8 bytes）；deque 的迭代器內含四個指標
//         cur/first/last/node（本機實測 32 bytes），因為它必須知道
//         「本段的邊界」與「自己在 map 的哪一格」才能跨 buffer 前進。
//     追問：那 ++it 貴多少?→ 平常只是 ++cur，多一次 `cur == last` 比較；
//         每 buffer_size 次才觸發一次 set_node。真正貴的是 it += n，
//         那裡有除法與取模。
//
// 🔥 Q2. 為什麼 std::sort(dq.begin(), dq.end()) 能編譯，
//        std::sort(lst.begin(), lst.end()) 卻不行?
//     答：std::sort 要求 random access iterator（要能 it+n、it2-it1）。
//         deque 迭代器滿足；list 只有 bidirectional iterator（只能 ++/--），
//         所以編譯期就被擋下。list 因此提供成員函式 lst.sort()，
//         用歸併排序改指標，不需要隨機存取。
//     追問：deque 排序效率跟 vector 比呢?→ 複雜度同為 O(n log n)，
//         但 deque 每次元素存取都多一層間接與邊界判斷，實測會慢一截。
//
// ⚠️ 陷阱. 這段程式為什麼是 UB?
//        auto it = dq.begin();
//        dq.push_back(42);        // 只是在尾端加東西，沒有動到前面
//        std::cout << *it;        // ← 這裡
//     答：deque 在**頭尾插入時所有 iterator 都失效**，包含 it。
//         即使「元素本身沒被搬移」，it 內部快取的 node 指向舊的 map，
//         而 map 可能已被重新配置 → 解參考是 UB。
//     為什麼會錯：多數人推理成「push_back 不影響前面的元素，所以指向
//         前面的迭代器應該還好」。這個推理對 **reference/pointer 成立**
//         （標準明文保證它們有效），但對 **iterator 不成立**——
//         這正是 deque 把兩者拆開的地方。要在插入後續用，
//         請改存 pointer/reference，或存**索引**再重新取得迭代器。
//
// ⚠️ 陷阱2. 「deque 隨機存取是 O(1)，所以用 dq[i] 迴圈遍歷跟 vector 一樣快」
//     答：複雜度相同不代表速度相同。dq[i] 每次都要做
//         「除法算段號 → 讀 map 拿 buffer 位址 → 讀元素」；
//         而 range-for 在段內只是 ++cur。實測索引迴圈明顯較慢。
//     為什麼會錯：把大 O 當成效能的全部。大 O 忽略常數與記憶體階層，
//         而 deque 與 vector 的差距**正好就落在被忽略的那一項**。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 34. Find First and Last Position of Element in Sorted Array
//                        in Sorted Array
//   題目：在已排序陣列中找出 target 的首末位置，要求 O(log n)；找不到回 [-1,-1]。
//   為什麼用到本主題：二分搜尋**必須**有 random access iterator 才能 O(log n)
//     （要能 first + n/2 一步跳到中間）。deque 的迭代器滿足這個要求，
//     所以 std::lower_bound / upper_bound 對 deque 是真正的 O(log n)；
//     若換成 list（bidirectional），lower_bound 仍可編譯但會退化成 O(n)
//     ——這正是「迭代器類別決定演算法效率」的最佳示範。
// -----------------------------------------------------------------------------
std::vector<int> searchRange(const std::deque<int>& nums, int target) {
    auto lo = std::lower_bound(nums.begin(), nums.end(), target);
    if (lo == nums.end() || *lo != target) return {-1, -1};
    auto hi = std::upper_bound(nums.begin(), nums.end(), target);
    return { static_cast<int>(lo - nums.begin()),          // 迭代器相減 = O(1)
             static_cast<int>(hi - nums.begin()) - 1 };
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 933. Number of Recent Calls
//   題目：RecentCounter.ping(t) 回傳「最近 3000 毫秒內」（含本次）的請求數。
//   為什麼用到本主題：時間戳單調遞增，需要「尾端加入、頭端淘汰過期」。
//     這裡刻意用 front()/pop_front() 而不是保存迭代器——因為
//     **每次 push_back 都會使所有 iterator 失效**，
//     在這種迴圈中持有迭代器正是典型的 UB 來源。
// -----------------------------------------------------------------------------
class RecentCounter {
    std::deque<int> q_;
public:
    int ping(int t) {
        q_.push_back(t);                       // ← 此刻所有 iterator 失效
        while (q_.front() < t - 3000)          // 所以用 front()，不用迭代器
            q_.pop_front();
        return static_cast<int>(q_.size());
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】監控系統的移動平均（最近 N 筆取樣）
//   情境：每秒收集一次 CPU 使用率，儀表板要顯示「最近 5 秒移動平均」。
//   為什麼用 deque：新樣本 push_back、最舊的 pop_front，兩端 O(1)。
//   ★ 注意寫法：迴圈中每次 push_back 都會讓 iterator 失效，
//     所以平均值用 range-for **在改動之後**重新遍歷計算，
//     絕不跨越 push_back 保存迭代器。
// -----------------------------------------------------------------------------
class MovingAverage {
    std::deque<double> win_;
    std::size_t n_;
public:
    explicit MovingAverage(std::size_t n) : n_(n) {}
    double add(double sample) {
        win_.push_back(sample);
        if (win_.size() > n_) win_.pop_front();
        double sum = 0;
        for (double v : win_) sum += v;        // range-for：段內連續前進
        return sum / static_cast<double>(win_.size());
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】log 檔的「錯誤前後文」擷取（環形前文緩衝）
//   情境：巡檢 log 時，發現 ERROR 就要把「前 K 行」一起送進告警訊息，
//         方便判斷錯誤發生的脈絡。
//   為什麼用 deque：維持固定長度的前文視窗——新行 push_back，
//     超過 K 行就 pop_front。輸出時用 range-for 依序印出。
// -----------------------------------------------------------------------------
std::vector<std::string> collectErrorContext(const std::vector<std::string>& lines,
                                             std::size_t k) {
    std::deque<std::string> prev;      // 最近 k 行前文
    std::vector<std::string> report;
    for (const auto& line : lines) {
        if (line.find("[ERROR]") != std::string::npos) {
            for (const auto& p : prev) report.push_back("  前文| " + p);
            report.push_back("  錯誤| " + line);
        }
        prev.push_back(line);
        if (prev.size() > k) prev.pop_front();
    }
    return report;
}

int main() {
    std::deque<int> dq = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    // =========================================================================
    std::cout << "=== 1. 迭代器類別與大小 ===\n";
    // =========================================================================
    using DIt = std::deque<int>::iterator;
    using VIt = std::vector<int>::iterator;
    constexpr bool isRandom =
        std::is_same_v<typename std::iterator_traits<DIt>::iterator_category,
                       std::random_access_iterator_tag>;
    std::cout << "  deque iterator 是 random_access? " << isRandom << "\n";
    std::cout << "  sizeof(deque<int>::iterator)  = " << sizeof(DIt)
              << " bytes（= " << sizeof(DIt) / sizeof(void*) << " 個指標:cur/first/last/node）\n";
    std::cout << "  sizeof(vector<int>::iterator) = " << sizeof(VIt)
              << " bytes（就是一根裸指標）\n";
    std::cout << "  ★ 此為 libstdc++ 實測值，標準未規定內部佈局\n";

    // =========================================================================
    std::cout << "\n=== 2. 正向 / 反向遍歷 ===\n";
    // =========================================================================
    std::cout << "  正向:";
    for (auto it = dq.begin(); it != dq.end(); ++it) std::cout << " " << *it;
    std::cout << "\n  反向:";
    for (auto it = dq.rbegin(); it != dq.rend(); ++it) std::cout << " " << *it;
    std::cout << "\n";

    // =========================================================================
    std::cout << "\n=== 3. random access 的四種能力 ===\n";
    // =========================================================================
    auto it = dq.begin();
    it += 3;  std::cout << "  begin()+3 → " << *it << "\n";
    it += 4;  std::cout << "  再 +4     → " << *it << "\n";
    it -= 5;  std::cout << "  再 -5     → " << *it << "\n";
    std::cout << "  距離 end()-begin() = " << (dq.end() - dq.begin()) << "\n";
    auto a = dq.begin() + 2, b = dq.begin() + 7;
    std::cout << "  a→" << *a << " b→" << *b
              << "  a<b? " << (a < b) << "  b-a=" << (b - a) << "\n";
    std::cout << "  下標 begin()[0]=" << dq.begin()[0]
              << " begin()[5]=" << dq.begin()[5] << "（等同 *(it+n)）\n";

    // =========================================================================
    std::cout << "\n=== 4. random access → 可直接套用 std::sort / lower_bound ===\n";
    // =========================================================================
    std::deque<int> messy = {42, 7, 19, 3, 25, 11};
    std::cout << "  排序前:";
    for (int v : messy) std::cout << " " << v;
    std::sort(messy.begin(), messy.end());     // list 在這行會編譯失敗
    std::cout << "\n  排序後:";
    for (int v : messy) std::cout << " " << v;
    auto pos = std::lower_bound(messy.begin(), messy.end(), 19);
    std::cout << "\n  lower_bound(19) 落在索引 " << (pos - messy.begin()) << "\n";
    std::cout << "  ★ list 只有 bidirectional iterator，std::sort 對它無法編譯\n";

    // =========================================================================
    std::cout << "\n=== 5. 失效規則:iterator 死、reference 活 ===\n";
    // =========================================================================
    std::deque<int> d2 = {100, 200, 300};
    int* p200 = &d2[1];                  // pointer 指向值 200 的元素
    std::cout << "  push_front 前 *p200=" << *p200 << "\n";
    for (int i = 0; i < 1000; ++i) d2.push_front(i);
    std::cout << "  push_front 1000 次後 *p200=" << *p200
              << " → pointer 仍有效（標準保證，元素未被搬移）\n";
    std::cout << "  同一元素現在的索引 = 1001，d2[1001]=" << d2[1001] << "\n";
    std::cout << "  ★ 但先前取得的所有 iterator 都已失效，解參考是 UB\n";
    std::cout << "  ★ 中間 insert/erase 則連 reference/pointer 也一起失效\n";

    // =========================================================================
    std::cout << "\n=== 6. LeetCode 34. Find First and Last Position ===\n";
    // =========================================================================
    std::deque<int> sorted = {5, 7, 7, 8, 8, 10};
    auto r1 = searchRange(sorted, 8);
    auto r2 = searchRange(sorted, 6);
    std::cout << "  [5,7,7,8,8,10] target=8 → [" << r1[0] << "," << r1[1] << "]\n";
    std::cout << "  [5,7,7,8,8,10] target=6 → [" << r2[0] << "," << r2[1] << "]\n";

    // =========================================================================
    std::cout << "\n=== 7. LeetCode 933. Number of Recent Calls ===\n";
    // =========================================================================
    RecentCounter rc;
    for (int t : {1, 100, 3001, 3002})
        std::cout << "  ping(" << t << ") = " << rc.ping(t) << "\n";

    // =========================================================================
    std::cout << "\n=== 8. 實務:最近 3 筆的移動平均 ===\n";
    // =========================================================================
    MovingAverage ma(3);
    for (double s : {10.0, 20.0, 30.0, 40.0, 50.0})
        std::cout << "  加入 " << s << " → 移動平均 = " << ma.add(s) << "\n";

    // =========================================================================
    std::cout << "\n=== 9. 實務:log 錯誤前後文擷取（前 2 行）===\n";
    // =========================================================================
    std::vector<std::string> lines = {
        "10:00:01 [INFO]  服務啟動",
        "10:00:02 [INFO]  載入設定檔",
        "10:00:03 [WARN]  連線池即將耗盡",
        "10:00:04 [ERROR] 資料庫連線逾時",
        "10:00:05 [INFO]  重試中"
    };
    for (const auto& r : collectErrorContext(lines, 2)) std::cout << r << "\n";

    // =========================================================================
    std::cout << "\n=== 重點整理 ===\n";
    // =========================================================================
    std::cout << "  1. 介面是 random access，實作是 cur/first/last/node 四指標\n";
    std::cout << "  2. ++it 平常只是 ++cur；it+=n 需要除法，比 vector 貴\n";
    std::cout << "  3. random access → std::sort / lower_bound 可直接用（list 不行）\n";
    std::cout << "  4. 頭尾插入:所有 iterator 失效，但 reference/pointer 有效\n";
    std::cout << "  5. 中間 insert/erase:iterator 與 reference 全部失效\n";
    std::cout << "  6. 遍歷用 range-for，別用 dq[i] 索引迴圈\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

//   ★ 此為 libstdc++ 實測值，標準未規定內部佈局

// === 預期輸出 ===
// === 1. 迭代器類別與大小 ===
//   deque iterator 是 random_access? 1
//   sizeof(deque<int>::iterator)  = 32 bytes（= 4 個指標:cur/first/last/node）
//   sizeof(vector<int>::iterator) = 8 bytes（就是一根裸指標）
//
// === 2. 正向 / 反向遍歷 ===
//   正向: 10 20 30 40 50 60 70 80 90 100
//   反向: 100 90 80 70 60 50 40 30 20 10
//
// === 3. random access 的四種能力 ===
//   begin()+3 → 40
//   再 +4     → 80
//   再 -5     → 30
//   距離 end()-begin() = 10
//   a→30 b→80  a<b? 1  b-a=5
//   下標 begin()[0]=10 begin()[5]=60（等同 *(it+n)）
//
// === 4. random access → 可直接套用 std::sort / lower_bound ===
//   排序前: 42 7 19 3 25 11
//   排序後: 3 7 11 19 25 42
//   lower_bound(19) 落在索引 3
//   ★ list 只有 bidirectional iterator，std::sort 對它無法編譯
//
// === 5. 失效規則:iterator 死、reference 活 ===
//   push_front 前 *p200=200
//   push_front 1000 次後 *p200=200 → pointer 仍有效（標準保證，元素未被搬移）
//   同一元素現在的索引 = 1001，d2[1001]=200
//   ★ 但先前取得的所有 iterator 都已失效，解參考是 UB
//   ★ 中間 insert/erase 則連 reference/pointer 也一起失效
//
// === 6. LeetCode 34. Find First and Last Position ===
//   [5,7,7,8,8,10] target=8 → [3,4]
//   [5,7,7,8,8,10] target=6 → [-1,-1]
//
// === 7. LeetCode 933. Number of Recent Calls ===
//   ping(1) = 1
//   ping(100) = 2
//   ping(3001) = 3
//   ping(3002) = 3
//
// === 8. 實務:最近 3 筆的移動平均 ===
//   加入 10 → 移動平均 = 10
//   加入 20 → 移動平均 = 15
//   加入 30 → 移動平均 = 20
//   加入 40 → 移動平均 = 30
//   加入 50 → 移動平均 = 40
//
// === 9. 實務:log 錯誤前後文擷取（前 2 行）===
//   前文| 10:00:02 [INFO]  載入設定檔
//   前文| 10:00:03 [WARN]  連線池即將耗盡
//   錯誤| 10:00:04 [ERROR] 資料庫連線逾時
//
// === 重點整理 ===
//   1. 介面是 random access，實作是 cur/first/last/node 四指標
//   2. ++it 平常只是 ++cur；it+=n 需要除法，比 vector 貴
//   3. random access → std::sort / lower_bound 可直接用（list 不行）
//   4. 頭尾插入:所有 iterator 失效，但 reference/pointer 有效
//   5. 中間 insert/erase:iterator 與 reference 全部失效
//   6. 遍歷用 range-for，別用 dq[i] 索引迴圈
