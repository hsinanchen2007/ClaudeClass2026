// ============================================================================
//  map.cpp — std::map 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra map.cpp -o map && ./map
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/map
//  參考 (cplusplus.com): https://cplusplus.com/reference/map/map/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::map<K, V> 是「有序、key 唯一」的關聯容器,儲存 key→value 對應關係。
//  典型實作為紅黑樹。
//  元素型別是 std::pair<const Key, T> — key 是 const 的 (改 key 會破壞排序)。
//
//  ▌ 底層資料結構
//  與 std::set 相同的紅黑樹,只是節點存的是 pair<const K, V>。
//
//  ▌ 所屬類別
//  Associative container
//
//  ▌ 時間複雜度
//      insert / erase / find / operator[] / at   O(log n)
//      lower_bound / upper_bound                 O(log n)
//      size / empty                              O(1)
//      iterator ++/--                            攤銷 O(1)
//
//  ▌ 與其他 container 的比較
//      map vs unordered_map : map 有序 O(log n);unordered_map 無序、平均 O(1)
//      map vs multimap      : multimap 允許多個相同 key
//      map vs vector<pair>  : 小資料量 + 查詢多 → vector + sort 更快
//
//  ▌ 適用情境
//      ✅ 需要 key→value 對應 + 自動排序
//      ✅ 需要範圍查詢 (e.g. 找所有 key 在 [a,b) 的項目)
//      ❌ 不需要順序 → unordered_map 通常更快
//
//  ▌ Iterator 失效規則
//      與 set 相同:insert 不影響;erase 只影響被刪元素。
//
// ============================================================================

/*
補充筆記：std::map
  - map 依 key 排序，通常以紅黑樹實作，查找/插入/刪除是 O(log n)。
  - operator[] 查不到 key 會插入預設值；純查詢請用 find、contains 或 at。
  - iterator 按 key 順序走，適合需要有序輸出的資料。
  - std::map 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/
#include <map>
#include <iostream>
#include <string>
#include <vector>            // for LeetCode 範例
#include <unordered_map>     // for TimeMap 範例
#include <iterator>          // std::prev

template <typename K, typename V, typename C = std::less<K>>
void print(const std::map<K, V, C>& m, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& [k, v] : m) std::cout << '(' << k << ':' << v << ") ";
    std::cout << "} (size=" << m.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::map<std::string, int> m1;
    std::map<std::string, int> m2{{"apple", 1}, {"banana", 2}, {"cherry", 3}};
    std::map<std::string, int> m3(m2.begin(), m2.end());
    std::map<std::string, int> m4(m2);
    std::map<std::string, int> m5(std::move(m4));
    std::map<std::string, int, std::greater<>> m6{{"a",1}, {"b",2}};

    print(m2, "m2                  ");
    print(m6, "m6 (greater)        ");

    // ========================================================================
    //  2. 元素存取
    // ========================================================================

    // ──── operator[] ──── ★ 經典陷阱
    // 若 key 不存在,會「插入一個 default value 並回傳 reference」
    // 因此 operator[] 不能用在 const map 上。
    std::map<std::string, int> m;
    m["apple"] = 10;
    m["banana"] = 20;
    std::cout << "\nm[\"apple\"] = " << m["apple"] << '\n';
    std::cout << "m[\"unknown\"] = " << m["unknown"] << '\n';   // 自動插入 unknown:0
    print(m, "after operator[]    ");

    // ──── at(key) ──── 不會自動插入,key 不存在丟 std::out_of_range
    std::cout << "at(\"apple\") = " << m.at("apple") << '\n';
    try { m.at("xxx"); }
    catch (const std::out_of_range& ex) {
        std::cout << "at exception: " << ex.what() << '\n';
    }

    // ========================================================================
    //  3. Iterators
    // ========================================================================
    // 元素為 std::pair<const Key, Value>,順序為「key 的順序」(預設升冪)
    std::cout << "\n[正向] ";
    for (auto it = m2.begin(); it != m2.end(); ++it)
        std::cout << '(' << it->first << ',' << it->second << ") ";
    std::cout << "\n[反向] ";
    for (auto it = m2.rbegin(); it != m2.rend(); ++it)
        std::cout << '(' << it->first << ',' << it->second << ") ";
    std::cout << '\n';

    // ========================================================================
    //  4. 容量
    // ========================================================================
    std::cout << "\n[Capacity] size=" << m2.size()
              << ", empty=" << std::boolalpha << m2.empty() << '\n';

    // max_size():紅黑樹節點理論上限
    std::cout << "max_size = " << m2.max_size() << '\n';

    // get_allocator():取得 allocator (供 generic 程式碼使用)
    auto malloc_ = m2.get_allocator();
    (void)malloc_;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  5. Modifiers
    // ========================================================================

    // ──── insert ────
    // 回傳 pair<iterator, bool>;bool=false 表示「key 已存在,沒寫入」
    std::map<std::string, int> ins;
    auto [it1, ok1] = ins.insert({"a", 1});
    auto [it2, ok2] = ins.insert({"a", 999});   // 已存在,不會覆蓋!
    std::cout << "\ninsert a:1 → ok=" << ok1 << '\n';
    std::cout << "insert a:999 → ok=" << ok2 << ", value=" << it2->second << '\n';

    // 想覆蓋值 → 直接用 operator[] 或 insert_or_assign (C++17)
    ins["a"] = 999;                                          // 直接覆蓋
    print(ins, "after operator[]    ");

    // ──── insert_or_assign (C++17) ────
    // key 不在 → 插入;key 已在 → 覆寫值。回傳 pair<iterator, bool>
    auto [io_it, io_ok] = ins.insert_or_assign("a", 100);    // 覆寫
    std::cout << "insert_or_assign(\"a\",100) inserted? " << io_ok
              << ", value=" << io_it->second << '\n';

    // ──── try_emplace (C++17) ────
    // key 已在 → 不做任何事 (★ 不會建構 value);key 不在 → 就地建構 value
    // 比 operator[] = 更安全,且比 insert 更省一次拷貝
    std::map<std::string, std::string> te;
    te.try_emplace("hello", 5, '!');   // value = string(5,'!') = "!!!!!"
    te.try_emplace("hello", 3, '?');   // 已存在,不執行 — value 不變
    std::cout << "try_emplace hello = " << te["hello"] << '\n';

    // ──── emplace ────
    // 把參數 forward 給 pair<K,V> 的建構子
    std::map<int, std::string> em;
    em.emplace(1, "one");
    em.emplace(std::piecewise_construct,
               std::forward_as_tuple(2),
               std::forward_as_tuple(3, 'X'));     // value = "XXX"
    print(em, "emplace             ");

    // ──── emplace_hint ────
    // 給 hint iterator,若 hint 正好接近正確位置 → 攤銷 O(1)
    // (hint 不對也不會錯,只是回退為 O(log n))
    em.emplace_hint(em.end(), 3, "three");
    em.emplace_hint(em.lower_bound(0), 0, "zero");   // 在開頭
    print(em, "after emplace_hint  ");

    // ──── erase ────
    std::map<std::string,int> e{{"a",1},{"b",2},{"c",3}};
    e.erase("a");                       // by key,回傳刪除數量 (0 或 1)
    e.erase(e.begin());                 // by iterator
    print(e, "after erase         ");

    // ──── extract / merge (C++17) ────
    std::map<std::string,int> ma{{"a",1},{"b",2}};
    std::map<std::string,int> mb{{"b",20},{"c",30}};
    ma.merge(mb);            // ★ key 衝突的不會被搬 → b 仍在 mb
    print(ma, "ma after merge      ");
    print(mb, "mb (留下衝突的)     ");

    // extract:可改 key (因為已不在容器中,不破壞排序)
    std::map<std::string,int> ex{{"old",1}};
    auto node = ex.extract("old");
    if (!node.empty()) {
        node.key() = "new";
        ex.insert(std::move(node));
    }
    print(ex, "after rename key    ");

    // swap / clear
    std::map<int,int> sw1{{1,1}}, sw2{{9,9},{8,8}};
    sw1.swap(sw2);
    print(sw1, "after swap          ");
    sw1.clear();
    print(sw1, "after clear         ");

    // ========================================================================
    //  6. Lookup
    // ========================================================================
    std::map<std::string,int> q{{"apple",1},{"banana",2},{"cherry",3}};

    std::cout << "\ncount(\"apple\") = " << q.count("apple") << '\n';

    auto fi = q.find("banana");
    if (fi != q.end())
        std::cout << "find: " << fi->first << '=' << fi->second << '\n';

    std::cout << "contains(\"x\") = " << q.contains("x") << '\n';   // C++20

    // 範圍查詢
    auto lb = q.lower_bound("b");
    auto ub = q.upper_bound("c");
    std::cout << "range [b..c]: ";
    for (auto it_ = lb; it_ != ub; ++it_)
        std::cout << it_->first << ' ';
    std::cout << '\n';

    // ──── equal_range(key) ──── 等同 (lower_bound, upper_bound)
    // 對 map 而言區間長度 ≤ 1 (key 唯一),用法主要是寫 generic code 與 multimap 共用。
    auto er = q.equal_range("banana");
    std::cout << "equal_range(\"banana\") size = "
              << std::distance(er.first, er.second) << '\n';
    if (er.first != er.second)
        std::cout << "  → " << er.first->first << ':' << er.first->second << '\n';

    // ========================================================================
    //  7. Observers
    // ========================================================================
    auto kc = q.key_comp();
    std::cout << "\nkey_comp(\"a\",\"b\") = " << kc("a","b") << '\n';

    auto vc = q.value_comp();
    std::cout << "value_comp((a,1),(b,2)) = "
              << vc({"a",1}, {"b",2}) << '\n';

    // ========================================================================
    //  8. 透明比較 (C++14)
    // ========================================================================
    // std::less<> 允許 find / lower_bound 直接吃 const char* 而非 string
    std::map<std::string, int, std::less<>> trans{{"hello",1},{"world",2}};
    auto t = trans.find("hello");          // 直接 const char* 查
    if (t != trans.end()) std::cout << "transparent find: " << t->second << '\n';

    // ========================================================================
    //  9. C++20 free functions
    // ========================================================================
    std::map<int,int> n1{{1,1},{2,2},{3,3},{4,4}};
    std::erase_if(n1, [](const auto& p){ return p.first % 2 == 0; });
    print(n1, "erase_if(even key)  ");

    // ========================================================================
    //  10. 常見陷阱 (Pitfalls) ★必看
    // ========================================================================
    //
    //  (1) operator[] 在 key 不存在時「會插入」!
    //      想單純查詢 → 用 find / at / contains。
    //      const map 不能用 operator[]。
    //
    //  (2) insert 不會覆寫已存在的 key
    //      想覆寫 → operator[] = / insert_or_assign。
    //
    //  (3) emplace 仍可能建構暫時物件
    //      若 key 已存在,emplace 也已經建構了 pair → 浪費。
    //      想避免 → 用 try_emplace,key 已存在時完全不建構 value。
    //
    //  (4) Key 是 const,iterator 不能改 key
    //      it->first = "x";     // 編譯錯誤
    //      it->second = 100;    // OK,可以改 value
    //      改 key 要走 extract → modify key() → re-insert。
    //
    //  (5) merge 不會覆寫衝突 key
    //      衝突的元素留在原本的 map 中。
    //
    //  (6) operator[] 要求 V 是 default-constructible
    //      若 V 沒有 default constructor → 編譯錯誤,要用 try_emplace。
    //
    //  (7) for (auto [k,v] : m) 是「拷貝」每個 pair
    //      想避免拷貝 → for (auto& [k,v] : m)。

    // ========================================================================
    //  11. 最佳實踐
    // ========================================================================
    //
    //  • 安全查詢用 find / contains / at,別用 operator[]
    //  • 想插入「若不存在」→ try_emplace (C++17),最高效
    //  • 想插入「無論存不存在都覆寫」→ insert_or_assign (C++17)
    //  • 想批次處理 → 範圍 for + structured binding (auto& [k,v] : m)
    //  • 想避免 string 重複建構 → std::less<> 透明比較
    //  • 不需要順序 → 直接換 unordered_map,通常快很多

    // ========================================================================
    //  12. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // map 在面試題的最常見用法是「按 key 排序的計數 / 累積」, 以及利用
    // lower_bound / upper_bound 做「最近一個比 x 小/大」的範圍查詢。

    // ──── LC 729 變形: 用 map 紀錄事件並查詢「目前最大值」 ────
    // map 走訪自動按 key 升序,reverse iterator 取得最大 key 是 O(1)。
    {
        std::map<int, std::string> events;
        events[100] = "Buy";
        events[50]  = "Sell";
        events[200] = "Hold";
        std::cout << "\n[map] 自動排序走訪: ";
        for (const auto& [k, v] : events) std::cout << '(' << k << ',' << v << ") ";
        std::cout << "\n  最大 key = " << events.rbegin()->first
                  << ", 最小 key = " << events.begin()->first << '\n';
        // 預期輸出順序: (50,Sell) (100,Buy) (200,Hold)
    }

    // ──── LC 981: Time Based Key-Value Store (基於時間的鍵值存儲) ────
    // 設計一個 set(key, val, ts) / get(key, ts) 的資料結構。
    // get 要回傳「時間 <= ts」中最大的 ts 對應的 val。
    // map<int, string> 配合 upper_bound 即可在 O(log n) 完成。
    {
        struct TimeMap {
            std::unordered_map<std::string, std::map<int, std::string>> mp;
            void set(const std::string& key, const std::string& val, int ts) {
                mp[key][ts] = val;
            }
            std::string get(const std::string& key, int ts) {
                auto& tm = mp[key];
                auto it = tm.upper_bound(ts);
                if (it == tm.begin()) return "";
                return std::prev(it)->second;
            }
        };
        TimeMap tm;
        tm.set("foo", "bar",  1);
        tm.set("foo", "bar2", 4);
        std::cout << "[LC981 TimeMap] get(foo, 1) = " << tm.get("foo", 1) << '\n';   // bar
        std::cout << "[LC981 TimeMap] get(foo, 3) = " << tm.get("foo", 3) << '\n';   // bar
        std::cout << "[LC981 TimeMap] get(foo, 5) = " << tm.get("foo", 5) << '\n';   // bar2
    }

    // ──── 經典題: 找出陣列中第 K 大的元素出現次數 ────
    // map 排序好的特性可以快速做 Top-K 統計分析。
    {
        std::vector<int> nums{3, 1, 1, 2, 2, 2, 4};
        std::map<int, int> cnt;
        for (int x : nums) ++cnt[x];
        std::cout << "[map count by value] ";
        for (const auto& [k, c] : cnt) std::cout << k << ':' << c << ' ';
        std::cout << '\n';
        // 預期輸出: 1:2 2:3 3:1 4:1
    }

    // ──── ★ emplace_hint 套路:從「已排序」資料建 map ────
    // 真實 LC 場景: 題目給的 events / intervals / pairs 已先排序 (依題意或前置 sort),
    // 此時依序插入 map 時用 hint = end() 可達「攤銷 O(1)」,整體 O(n)。
    // 若用 emplace 或 operator[] 則為 O(n log n)。
    //
    // 例題場景: LC 350 變形 — 給兩個「已排序」陣列,要建立 sorted intersection map。
    //          LC 1086 High Five 變形 — 學生成績已按學號排序時的彙總。
    // 下例直接示範: 已排序輸入 → emplace_hint(end(), ...) 建立 map<int, count>。
    {
        std::vector<std::pair<int, int>> sorted_input{
            {1, 10}, {2, 20}, {3, 30}, {5, 50}, {8, 80}, {13, 130}
        };  // 已按 key 升序

        std::map<int, int> built;
        for (const auto& [k, v] : sorted_input) {
            // hint = end() 精確指向「未來該插入的位置」,標準保證攤銷 O(1)
            built.emplace_hint(built.end(), k, v);
        }
        std::cout << "[map emplace_hint(end)] ";
        for (const auto& [k, v] : built) std::cout << k << ':' << v << ' ';
        std::cout << '\n';
        // 預期輸出: 1:10 2:20 3:30 5:50 8:80 13:130
    }

    // ──── LC 56: Merge Intervals (合併重疊區間) ────
    // 難度: medium
    // 給一堆 [start, end] 區間,合併重疊。用 std::map<int,int> 以 start 為 key、
    // end 為 value 自動排序,然後線性掃描合併。
    // 為何用 map:輸入未排序時,map 自動依 start 排序;若已排序則退化為線性掃描,
    //              但程式碼意圖更清楚。
    {
        std::vector<std::pair<int,int>> intervals{{1,3},{2,6},{8,10},{15,18}};
        std::map<int,int> mp;       // start -> end (自動依 start 排序)
        for (auto [s,e] : intervals) {
            auto it = mp.find(s);
            if (it != mp.end()) it->second = std::max(it->second, e);
            else                mp.emplace(s, e);
        }
        std::vector<std::pair<int,int>> merged;
        for (auto [s, e] : mp) {
            if (!merged.empty() && s <= merged.back().second)
                merged.back().second = std::max(merged.back().second, e);
            else
                merged.push_back({s, e});
        }
        std::cout << "[LC56 MergeIntervals] = ";
        for (auto [s, e] : merged) std::cout << '[' << s << ',' << e << "] ";
        std::cout << '\n';
        // 預期輸出: [1,6] [8,10] [15,18]
    }

    // ========================================================================
    //  12. 實戰範例:訂單簿 (Order Book) 最佳買賣價查詢
    // ========================================================================
    // 真實場景:交易系統的「買單簿 / 賣單簿」要快速取得「最高出價」和「最低叫價」。
    // map 自動依 key (價格) 排序,begin() 取最小、rbegin() 取最大,皆 O(log n) 級別,
    // 配合 erase / insert 也都是 O(log n),非常適合動態維護有序價位表。
    {
        // 買單簿:價格 -> 數量 (買方希望「最高價」優先)
        std::map<double, int> bids;
        bids[101.5] = 100;
        bids[101.7] = 50;
        bids[101.3] = 200;

        // 賣單簿:價格 -> 數量 (賣方希望「最低價」優先)
        std::map<double, int> asks;
        asks[101.9] = 80;
        asks[102.1] = 150;
        asks[101.8] = 30;

        // 最佳買價 = bids 中最大 key  →  rbegin
        // 最佳賣價 = asks 中最小 key  →  begin
        auto best_bid = bids.rbegin();
        auto best_ask = asks.begin();
        std::cout << "[OrderBook] Best Bid = " << best_bid->first
                  << " x" << best_bid->second
                  << ",Best Ask = " << best_ask->first
                  << " x" << best_ask->second << '\n';
        // 預期輸出: Best Bid = 101.7 x50,Best Ask = 101.8 x30
    }

    std::cout << "\n=== map demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：map vs unordered_map,實務上怎麼選?
    //    A：需要「自動排序走訪」「lower_bound / upper_bound 區間查詢」就用 map
    //       (紅黑樹,O(log n));單純 key→value 查找且 hash 容易設計就用 unordered_map
    //       (O(1) 平均,O(n) 最壞)。資料量小時 map 常數小可能更快,大資料量則 hash 勝出。
    //
    //  Q2：m[key] 與 m.at(key) 的差別?哪個比較危險?
    //    A：operator[] 找不到 key 時會「自動插入 default 值」並回傳 reference
    //       (對 const map 不可用);at(key) 找不到會丟 std::out_of_range。
    //       純查詢時用 at 或 find,避免 operator[] 不小心污染 map。
    //
    //  Q3：try_emplace 與 emplace 在 map 上有何差別?
    //    A：emplace 不論 key 是否存在,都會「先建構 value 物件」再判斷要不要插入;
    //       try_emplace (C++17) 在 key 已存在時「完全不建構 value」,可避免昂貴
    //       建構 (例如 value 是 unique_ptr / 大 string / 容器) 的多餘開銷。
    //
    return 0;
}

/*
============================================================================
  附錄:std::map 完整 member function 一覽
============================================================================
  Constructors / destructor / operator= / get_allocator

  Element access:
      at, operator[]

  Iterators:       begin/end/cbegin/cend/rbegin/rend/crbegin/crend
  Capacity:        empty, size, max_size

  Modifiers:
      clear, insert, insert_range (C++23), insert_or_assign (C++17),
      emplace, emplace_hint, try_emplace (C++17),
      erase, swap, extract (C++17), merge (C++17)

  Lookup:
      count, find, contains (C++20), equal_range, lower_bound, upper_bound

  Observers:
      key_comp, value_comp

  Non-member:
      operator==, <=> (C++20), std::swap, std::erase_if (C++20)
============================================================================
*/
