// ============================================================================
//  multimap.cpp — std::multimap 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra multimap.cpp -o multimap && ./multimap
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/multimap
//  參考 (cplusplus.com): https://cplusplus.com/reference/map/multimap/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::multimap<K, V> 與 std::map 幾乎完全相同,差別只在:
//      ★ 允許「同一個 key 對應多個 value」(也就是允許重複 key)。
//
//  ▌ 與 std::map 的差異
//      • 沒有 operator[] 和 at()  ★ 因為「key→value」不再是 1 對 1
//      • 沒有 insert_or_assign / try_emplace
//      • insert 永遠成功,回傳 iterator (不是 pair<iterator,bool>)
//      • count(k) 可能 > 1
//      • erase(k) 會刪除「所有」此 key 的元素,回傳數量
//      • equal_range(k) 真正有意義
//
//  ▌ 時間複雜度 (與 map 同)
//      insert / erase(by it) / find / lower_bound / upper_bound  O(log n)
//      erase(by key)                                             O(log n + k)
//      count(key)                                                O(log n + k)
//
//  ▌ 適用情境
//      ✅ 一個 key 對多個 value:e.g. 多語字典、tag→items
//      ✅ 想要按 key 排序保留所有重複項
//      ❌ 不需要順序 → unordered_multimap
//      ❌ 一對多但 value 是固定大小 → 也可以用 map<K, vector<V>>,通常更直觀
//
//  ▌ Iterator 失效規則
//      與 map / set 同 — insert 不影響;erase 只影響被刪元素。
//
// ============================================================================

/*
補充筆記：std::multimap
  - multimap 允許多個相同 key，不能用 operator[] 表示唯一值。
  - 查同一 key 的所有元素時使用 equal_range。
  - 插入順序不等於輸出順序；主要順序仍由 key comparison 決定。
  - std::multimap 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::multimap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. multimap 和 map 的差別？為什麼 multimap 沒有 operator[] 和 at()？
//     答：multimap 允許重複 key，所以 insert 永遠成功（回傳 iterator 而非 pair<iterator,bool>）。
//         因為一個 key 可能對應多個 value，`m[k]` 語意不明（要回哪一個？），
//         因此標準干脆不提供 operator[] 與 at()，也沒有 insert_or_assign / try_emplace。
//     追問：什麼時候改用 map<K, vector<V>> 更好？（需要把同 key 的 value 當一組整體操作時）
//
// 🔥 Q2. 如何取出 multimap 中某個 key 的所有 value？
//     答：用 equal_range(key)，回傳 pair<iterator,iterator> 即該 key 的整個區間；
//         也可用 lower_bound / upper_bound 自己組。count(key) 回傳個數，複雜度 O(log n + k)。
//         C++11 起標準要求等值 key 的元素維持插入順序（相對順序穩定）。
//
// ⚠️ 陷阱. multimap::erase(key) 會刪掉幾個元素？
//     答：全部等值元素，並回傳刪除個數。只想刪一個必須傳 iterator（例如 find(key) 的結果）。
//     為什麼會錯：把它跟 map::erase(key)（最多只會刪 1 個）混為一談，
//         實務上這會一次吹掉整組資料。
// ═══════════════════════════════════════════════════════════════════════════

#include <map>            // multimap 與 map 共用 <map>
#include <iostream>
#include <string>
#include <vector>         // for LeetCode 範例
#include <unordered_map>  // for LC 692 計數

template <typename K, typename V, typename C = std::less<K>>
void print(const std::multimap<K, V, C>& m, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& [k, v] : m) std::cout << '(' << k << ':' << v << ") ";
    std::cout << "} (size=" << m.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::multimap<std::string, int> m1;
    std::multimap<std::string, int> m2{
        {"apple", 1}, {"banana", 2}, {"apple", 3}, {"cherry", 4}, {"apple", 5}};
    std::multimap<std::string, int> m3(m2);
    std::multimap<std::string, int, std::greater<>> m4{{"a",1},{"a",2},{"b",3}};

    print(m2, "m2 (含重複 key)     ");
    print(m4, "m4 (greater)        ");

    // ========================================================================
    //  2. 元素存取 ★multimap 沒有 operator[] / at !
    // ========================================================================
    //  原因:一個 key 可能對應多個 value,沒辦法用 m[k] 唯一回傳一個值。
    //  想存取某 key 的所有 value → 用 equal_range。

    // ========================================================================
    //  3. Iterators
    // ========================================================================
    //  順序為「key 的順序」,相同 key 之間為「插入順序」(C++11 stable)
    std::cout << "\n[正向] ";
    for (auto it = m2.begin(); it != m2.end(); ++it)
        std::cout << '(' << it->first << ',' << it->second << ") ";
    std::cout << '\n';

    // ========================================================================
    //  4. 容量
    // ========================================================================
    std::cout << "\n[Capacity] size=" << m2.size()
              << ", empty=" << std::boolalpha << m2.empty() << '\n';

    // max_size():理論上限
    std::cout << "max_size = " << m2.max_size() << '\n';

    // get_allocator()
    auto mma = m2.get_allocator();
    (void)mma;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  5. Modifiers
    // ========================================================================

    // ──── insert ──── 永遠成功,回傳 iterator
    std::multimap<std::string, int> m;
    auto it1 = m.insert({"a", 1});
    auto it2 = m.insert({"a", 2});      // 同 key 也會插入
    std::cout << "\ninsert a:1 it1=" << it1->second
              << ", insert a:2 it2=" << it2->second << '\n';
    m.insert({{"b", 10}, {"b", 20}, {"c", 30}});
    print(m, "after insert        ");

    // 帶 hint
    m.insert(m.end(), {"d", 40});
    print(m, "insert hint         ");

    // ──── emplace / emplace_hint ────
    std::multimap<int, std::string> em;
    em.emplace(1, "one");
    em.emplace(1, "uno");               // 同 key
    em.emplace_hint(em.end(), 2, "two");
    print(em, "emplace             ");

    // ──── erase ────
    // by key:刪除「所有」此 key 的元素,回傳數量
    std::multimap<std::string, int> e{
        {"a",1},{"a",2},{"a",3},{"b",4},{"c",5}};
    std::cout << "\nerase(\"a\") → " << e.erase("a") << " 個\n";
    print(e, "after erase(a)      ");

    // by iterator: 只刪這一個
    e.erase(e.begin());
    print(e, "after erase(begin)  ");

    // by range: 結合 equal_range 可只刪某 key 的「一部分」
    std::multimap<std::string,int> e2{
        {"a",1},{"a",2},{"a",3},{"b",4}};
    auto [lo, hi] = e2.equal_range("a");
    e2.erase(lo, std::next(lo));      // 只刪 "a" 群中的第一個
    print(e2, "erase 1 of \"a\"    ");

    // ──── extract / merge ────
    std::multimap<std::string, int> ma{{"a",1},{"b",2}};
    std::multimap<std::string, int> mb{{"a",10},{"c",20}};
    ma.merge(mb);                      // multimap 的 merge 全部搬過來,不去重
    print(ma, "ma after merge      ");
    print(mb, "mb (掏空)           ");

    // swap / clear
    std::multimap<int,int> sw1{{1,1}}, sw2{{2,2},{2,3}};
    sw1.swap(sw2);
    print(sw1, "after swap          ");
    sw1.clear();
    print(sw1, "after clear         ");

    // ========================================================================
    //  6. Lookup
    // ========================================================================
    std::multimap<std::string, int> q{
        {"a",1},{"a",2},{"a",3},{"b",4},{"c",5}};

    std::cout << "\ncount(\"a\") = " << q.count("a") << '\n';   // 3

    // find: 回傳「某一個」== key 的 iterator (通常為第一個,但不保證)
    auto fi = q.find("a");
    if (fi != q.end()) std::cout << "find: " << fi->second << '\n';

    std::cout << "contains(\"d\") = " << q.contains("d") << '\n';  // C++20

    // ──── equal_range ──── multimap 的核心查詢方式
    auto [b1, e1] = q.equal_range("a");
    std::cout << "all \"a\"s: ";
    for (auto it_ = b1; it_ != e1; ++it_) std::cout << it_->second << ' ';
    std::cout << '\n';

    // lower_bound / upper_bound
    auto lb = q.lower_bound("a");
    auto ub = q.upper_bound("b");
    std::cout << "[a..b]: ";
    for (auto it_ = lb; it_ != ub; ++it_) std::cout << it_->first << ' ';
    std::cout << '\n';

    // ========================================================================
    //  7. Observers
    // ========================================================================
    auto kc = q.key_comp();
    std::cout << "\nkey_comp(\"a\",\"b\") = " << kc("a","b") << '\n';

    // value_comp():對「整個 pair<const K, V>」的比較器
    // 內部其實是用 key_comp 比較兩個 pair 的 first
    auto vc = q.value_comp();
    std::cout << "value_comp((a,1),(b,2)) = "
              << vc({"a", 1}, {"b", 2}) << '\n';

    // ========================================================================
    //  8. C++20 free functions
    // ========================================================================
    std::multimap<int,int> n1{{1,1},{2,2},{2,3},{3,4}};
    std::erase_if(n1, [](const auto& p){ return p.second % 2 == 0; });
    print(n1, "erase_if(even val)  ");

    // ========================================================================
    //  9. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) 沒有 operator[] / at!
    //      因為 key 不唯一,無法回傳唯一 value reference。
    //
    //  (2) 沒有 try_emplace / insert_or_assign
    //      原因相同 — 「該 key 已有元素」沒有意義。
    //
    //  (3) erase(key) 會刪掉「所有」此 key 的元素
    //      只想刪一個 → 用 erase(find(key)) 或 erase(lower_bound(key))。
    //
    //  (4) find(key) 不保證回傳第一個
    //      要明確的「第一個」用 lower_bound(key)。
    //
    //  (5) 修改 value 可以,但修改 key 不行
    //      it->first 是 const,需 extract → 改 key() → re-insert。
    //
    //  (6) 想做「key→list of values」,有時候 map<K, vector<V>> 比 multimap 直觀
    //      尤其當需要對某 key 的 values 做整體操作時。

    // ========================================================================
    //  10. 最佳實踐
    // ========================================================================
    //
    //  • 取某 key 的所有值 → equal_range,而非 find + 走 ++it 直到 key 變
    //  • 只刪一個 → erase(find(key))
    //  • 想做整批 batch 處理 → 考慮 map<K, vector<V>>
    //  • 不需要順序 → unordered_multimap

    // ========================================================================
    //  11. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // multimap 的典型應用是「一個 key 有多個 value」的「索引/分群」,
    // 配合 equal_range 做範圍查詢。在面試題裡常見於日程、行事曆、事件分組。

    // ──── 學生成績分組 (常見筆試題) ────
    // 把學生成績 (60..100) 各對應到名字,一個分數可能有多人,
    // 要查詢某分數所有同學、或前 K 高分。
    {
        std::multimap<int, std::string, std::greater<int>> scores;   // 由高到低
        scores.emplace(85, "Alice");
        scores.emplace(92, "Bob");
        scores.emplace(85, "Cathy");
        scores.emplace(70, "Dave");
        scores.emplace(92, "Eve");

        std::cout << "\n[multimap 高分到低分排名]\n";
        int rank = 1;
        for (const auto& [score, name] : scores) {
            std::cout << "  #" << rank++ << ' ' << name << " (" << score << ")\n";
        }
        // 預期輸出順序: Bob/Eve (92) → Alice/Cathy (85) → Dave (70)

        // 查詢「85 分」所有學生 → equal_range
        auto [lo, hi] = scores.equal_range(85);
        std::cout << "[score == 85 的學生] ";
        for (auto it = lo; it != hi; ++it) std::cout << it->second << ' ';
        std::cout << '\n';
    }

    // ──── ★ emplace_hint 套路:從「已排序」資料建 multimap ────
    // 若題目給定的 events 已按 key 順序到達 (e.g. LC 1996 比賽分數依到場順序),
    // 用 hint = end() 可達攤銷 O(1) 插入,整體 O(n) 而非 O(n log n)。
    //
    // 範例情境: 學生分數已按到達順序「降序」推送,直接 hint = end() 即正確位置。
    {
        std::vector<std::pair<int, std::string>> sorted_arrivals{
            {95, "Tom"}, {95, "Jerry"}, {88, "Mary"}, {72, "Sam"}, {60, "Lee"}
        };  // 已按分數降序到達

        std::multimap<int, std::string, std::greater<int>> mm;
        for (const auto& [score, name] : sorted_arrivals) {
            mm.emplace_hint(mm.end(), score, name);    // 攤銷 O(1)
        }
        std::cout << "[multimap emplace_hint(end)] ";
        for (const auto& [k, v] : mm) std::cout << '(' << k << ',' << v << ") ";
        std::cout << '\n';
        // 預期輸出: (95,Tom) (95,Jerry) (88,Mary) (72,Sam) (60,Lee)
    }

    // ──── 行事曆: 同一天可有多個事件 ────
    // 用 multimap<date, event> 表示, equal_range 取出某天全部事件。
    {
        std::multimap<int, std::string> calendar;     // key 是 day (1..31)
        calendar.emplace(5,  "Meeting");
        calendar.emplace(5,  "Lunch with HR");
        calendar.emplace(10, "Code review");
        calendar.emplace(5,  "Gym");

        std::cout << "[5 號的事件]: ";
        auto [b, e] = calendar.equal_range(5);
        for (auto it = b; it != e; ++it) std::cout << '[' << it->second << "] ";
        std::cout << '\n';
        // 預期輸出: [Meeting] [Lunch with HR] [Gym]
    }

    // ──── LC 692 變形: Top K Frequent Words (前 K 個高頻詞) ────
    // 難度: medium
    // 給一堆單字,求出現次數前 K 高的單字 (相同次數按字母順序)。
    // 思路:先用 unordered_map 計數,再把 (count, word) 倒進 multimap<int,string,greater>,
    // 取前 K 個即可。multimap 自動依次數降序、相同次數依字母升序。
    {
        std::vector<std::string> words{"i","love","leetcode","i","love","coding"};
        int k = 2;
        std::unordered_map<std::string,int> freq;
        for (auto& w : words) ++freq[w];

        // multimap<count desc, word asc>:利用排序自動完成 top-k
        std::multimap<int, std::string, std::greater<int>> mm;
        for (auto& [w, c] : freq) mm.emplace(c, w);

        std::cout << "[LC692 TopK Words] = ";
        int cnt = 0;
        for (auto& [c, w] : mm) {
            if (cnt++ >= k) break;
            std::cout << w << '(' << c << ") ";
        }
        std::cout << '\n';
        // 預期輸出: i(2) love(2)  (note: 相同次數時順序依 hash 而非字典序,真實題目需後處理)
    }

    // ========================================================================
    //  12. 實戰範例:日誌系統 — 同時間多筆訊息查詢
    // ========================================================================
    // 真實場景:後端服務的日誌以「時間戳 -> 訊息」儲存,同一毫秒可能有多筆訊息。
    // 需求:取「某時間範圍內所有訊息」做檢索,或「最近 N 筆」做監控。
    // multimap 的優勢:
    //   • key 重複自然支援 (同毫秒多訊息)
    //   • 自動依時間排序 → lower_bound/upper_bound 做時間區間 O(log n) 查詢
    //   • rbegin()/rend() 反向走訪 → 取最新訊息很自然
    {
        std::multimap<long long, std::string> logs;
        logs.emplace(1000, "service started");
        logs.emplace(1050, "user login: alice");
        logs.emplace(1050, "user login: bob");      // 同毫秒兩筆
        logs.emplace(1100, "DB query");
        logs.emplace(1200, "user logout: alice");

        // 取出 [1050, 1100] 範圍的所有訊息
        auto lo = logs.lower_bound(1050);
        auto hi = logs.upper_bound(1100);
        std::cout << "[LogSystem] 時間 [1050,1100] 的訊息:\n";
        for (auto it = lo; it != hi; ++it)
            std::cout << "  t=" << it->first << " | " << it->second << '\n';
        // 預期輸出: 1050 alice / 1050 bob / 1100 DB query
    }

    std::cout << "\n=== multimap demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：multimap 為什麼沒有 operator[] 與 at()?
    //    A：因為同一個 key 可以對應到多個 value,operator[] 無法決定要回傳哪一筆,
    //       語意不明確,標準直接拿掉這個 API。要查多筆請改用 equal_range 取得區間。
    //
    //  Q2：multimap 的 equal_range(key) 與 lower_bound/upper_bound 是同樣的東西嗎?
    //    A：是的,equal_range 等價於 {lower_bound, upper_bound},都是 O(log n)。
    //       它回傳一個 pair,代表「所有等於 key 的元素」的半開區間 [first, last)。
    //
    //  Q3：multimap 內相同 key 的元素彼此順序如何決定?
    //    A：C++11 起標準保證「插入順序」(insertion order) 在相同 key 內被保留 —
    //       先插入者走訪時也會在前。這對「分組記錄」型應用 (例如行事曆事件) 很重要。
    //
    return 0;
}

/*
============================================================================
  附錄:std::multimap 完整 member function 一覽
============================================================================
  介面與 std::map 基本相同,主要差異:
      ★ 沒有 at, operator[], insert_or_assign, try_emplace
      ★ insert 回傳 iterator (不是 pair<iterator, bool>)
      ★ count(key) 可能 > 1
      ★ erase(key) 回傳刪除數量

  Constructors / destructor / operator= / get_allocator
  begin/end/cbegin/cend/rbegin/rend/crbegin/crend
  empty, size, max_size
  clear, insert, insert_range (C++23), emplace, emplace_hint,
  erase, swap, extract (C++17), merge (C++17)
  count, find, contains (C++20), equal_range, lower_bound, upper_bound
  key_comp, value_comp
  operator==, <=> (C++20), std::swap, std::erase_if (C++20)
============================================================================
*/
