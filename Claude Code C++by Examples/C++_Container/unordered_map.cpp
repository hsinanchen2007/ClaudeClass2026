// ============================================================================
//  unordered_map.cpp — std::unordered_map 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra unordered_map.cpp -o unordered_map && ./unordered_map
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/unordered_map
//  參考 (cplusplus.com): https://cplusplus.com/reference/unordered_map/unordered_map/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::unordered_map<K, V> (C++11) 是「無序、key 唯一」的關聯容器,
//  底層為 hash table。
//  元素型別為 std::pair<const Key, T>。
//
//  ▌ 底層資料結構
//  與 unordered_set 相同的 chained hash table,只是節點存 pair<const K, V>。
//
//  ▌ 所屬類別
//  Unordered associative container
//
//  ▌ 時間複雜度
//      operator[] / at / find / insert / erase   平均 O(1),最差 O(n)
//      bucket / bucket_count / load_factor       O(1)
//      rehash / reserve                          平均 O(n)
//      size / empty                              O(1)
//
//  ▌ 與其他 container 的比較
//      unordered_map vs map  : map 有序 O(log n) 穩定;這個無序、平均 O(1)
//      unordered_map vs unordered_multimap : 後者允許多個相同 key
//      unordered_map vs vector<pair<K,V>>  : 元素少時 vector 更快
//
//  ▌ 適用情境
//      ✅ 大量 key→value 查詢、不在乎順序 (e.g. 字典、index、cache)
//      ✅ 追求平均 O(1)
//      ❌ 需要排序或範圍查詢 → 用 map
//      ❌ 需要絕對最差時間保證 → 用 map
//
//  ▌ Iterator 失效規則
//      ★ rehash 會使「所有 iterator」失效,但 reference / pointer 不失效
//      • erase: 只影響被刪元素的 iterator
//      • insert: 若不觸發 rehash,iterator 都不失效
//
// ============================================================================

/*
補充筆記：std::unordered_map
  - unordered_map 以 hash bucket 組織 key，平均查找 O(1)，但最壞情況可能退化。
  - operator[] 查不到會插入預設 value；統計頻率時方便，純查詢時可能造成意外新增 key。
  - rehash 會讓 iterator 失效；保存 iterator 後又大量插入時要特別小心。
  - std::unordered_map 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>            // for LeetCode 範例
#include <algorithm>         // std::sort

template <typename K, typename V>
void print(const std::unordered_map<K, V>& m, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& [k, v] : m) std::cout << '(' << k << ':' << v << ") ";
    std::cout << "} (size=" << m.size()
              << ", buckets=" << m.bucket_count() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::unordered_map<std::string, int> m1;
    std::unordered_map<std::string, int> m2{
        {"apple", 1}, {"banana", 2}, {"cherry", 3}};
    std::unordered_map<std::string, int> m3(m2);
    std::unordered_map<std::string, int> m4(100);   // 100 buckets

    print(m2, "m2                  ");

    // ========================================================================
    //  2. 元素存取
    // ========================================================================

    // ──── operator[] ──── ★ 與 map 一樣:key 不存在會插入 default!
    std::unordered_map<std::string, int> m;
    m["apple"] = 10;
    m["banana"] = 20;
    std::cout << "\nm[\"apple\"] = " << m["apple"] << '\n';
    std::cout << "m[\"unknown\"] = " << m["unknown"] << '\n';   // 自動插入 0
    print(m, "after operator[]    ");

    // at(): 不會插入,key 不存在丟 std::out_of_range
    try { m.at("xxx"); }
    catch (const std::out_of_range& ex) {
        std::cout << "at exception: " << ex.what() << '\n';
    }

    // ========================================================================
    //  3. Iterators (forward,無反向)
    // ========================================================================
    std::cout << "\n[walk] ";
    for (auto& [k, v] : m2) std::cout << '(' << k << ',' << v << ") ";
    std::cout << '\n';

    // 容量補充: max_size + get_allocator
    std::cout << "max_size = " << m2.max_size() << '\n';
    auto uma = m2.get_allocator();
    (void)uma;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  4. Modifiers
    // ========================================================================

    // ──── insert ──── 不會覆寫已存在的 key
    std::unordered_map<std::string, int> ins;
    auto [it1, ok1] = ins.insert({"a", 1});
    auto [it2, ok2] = ins.insert({"a", 999});   // 已存在,ok=false
    std::cout << "\ninsert ok=" << ok1 << ", again ok=" << ok2 << '\n';

    // ──── insert_or_assign (C++17) ──── 不存在則插入,存在則覆寫
    ins.insert_or_assign("a", 100);
    print(ins, "after insert_or_a   ");

    // ──── try_emplace (C++17) ──── 已存在不做事 (★ 不會建構 V)
    std::unordered_map<std::string, std::string> te;
    te.try_emplace("hello", 5, '!');           // value = "!!!!!"
    te.try_emplace("hello", 3, '?');           // 不執行
    std::cout << "try_emplace hello = " << te["hello"] << '\n';

    // ──── emplace ────
    std::unordered_map<int, std::string> em;
    em.emplace(1, "one");
    em.emplace(std::piecewise_construct,
               std::forward_as_tuple(2),
               std::forward_as_tuple(3, 'X'));
    print(em, "emplace             ");

    // ──── emplace_hint ────
    // hash table 中 hint 通常無效益 (位置由 hash 決定),但 API 為一致而提供。
    em.emplace_hint(em.end(), 4, "four");
    print(em, "after emplace_hint  ");

    // ──── erase ────
    std::unordered_map<std::string, int> e{{"a",1},{"b",2},{"c",3}};
    e.erase("a");
    e.erase(e.begin());
    print(e, "after erase         ");

    // ──── extract / merge (C++17) ────
    std::unordered_map<std::string,int> ma{{"a",1},{"b",2}};
    std::unordered_map<std::string,int> mb{{"b",20},{"c",30}};
    ma.merge(mb);                  // 衝突的 key 不會搬
    print(ma, "after merge         ");
    print(mb, "mb (留下衝突)       ");

    // swap / clear
    std::unordered_map<int,int> sw1{{1,1}}, sw2{{9,9}};
    sw1.swap(sw2);
    print(sw1, "after swap          ");
    sw1.clear();

    // ========================================================================
    //  5. Lookup
    // ========================================================================
    std::unordered_map<std::string,int> q{
        {"apple",1},{"banana",2},{"cherry",3}};

    std::cout << "\ncount(\"apple\") = " << q.count("apple") << '\n';

    auto fi = q.find("banana");
    if (fi != q.end()) std::cout << "find: " << fi->second << '\n';

    std::cout << "contains(\"x\") = " << q.contains("x") << '\n';   // C++20

    // ★ 沒有 lower_bound / upper_bound

    // ========================================================================
    //  6. Bucket interface
    // ========================================================================
    std::unordered_map<int,int> bk{{1,1},{2,2},{3,3},{4,4},{5,5}};
    std::cout << "\nbuckets=" << bk.bucket_count()
              << ", max_bucket_count=" << bk.max_bucket_count() << '\n';
    for (size_t i = 0; i < bk.bucket_count(); ++i) {
        std::cout << "  bucket " << i << " (size=" << bk.bucket_size(i) << "): ";
        for (auto it = bk.begin(i); it != bk.end(i); ++it)
            std::cout << '(' << it->first << ',' << it->second << ") ";
        std::cout << '\n';
    }
    std::cout << "  bucket(3) = " << bk.bucket(3) << '\n';

    // ========================================================================
    //  7. Hash policy
    // ========================================================================
    std::cout << "\nload_factor = " << bk.load_factor()
              << ", max_load_factor = " << bk.max_load_factor() << '\n';
    bk.reserve(1000);
    std::cout << "after reserve, buckets=" << bk.bucket_count() << '\n';
    bk.rehash(50);
    std::cout << "after rehash(50), buckets=" << bk.bucket_count() << '\n';
    bk.max_load_factor(0.5f);
    std::cout << "max_load_factor(0.5) → " << bk.max_load_factor() << '\n';

    // ========================================================================
    //  8. Observers
    // ========================================================================
    auto h = q.hash_function();
    std::cout << "\nhash(\"apple\") = " << h("apple") << '\n';
    auto eq = q.key_eq();
    std::cout << "key_eq(\"a\",\"a\") = " << eq("a","a") << '\n';

    // ========================================================================
    //  9. 自訂型別當 key
    // ========================================================================
    struct Point { int x, y; };
    struct PointHash {
        size_t operator()(const Point& p) const noexcept {
            return std::hash<int>{}(p.x) * 31 + std::hash<int>{}(p.y);
        }
    };
    struct PointEq {
        bool operator()(const Point& a, const Point& b) const noexcept {
            return a.x == b.x && a.y == b.y;
        }
    };
    std::unordered_map<Point, std::string, PointHash, PointEq> pm{
        {{1,2}, "p12"}, {{3,4}, "p34"}};
    std::cout << "\ncustom-key map p12 = " << pm[{1,2}] << '\n';

    // ========================================================================
    //  10. C++20 free functions
    // ========================================================================
    std::unordered_map<int,int> n1{{1,1},{2,2},{3,3},{4,4}};
    std::erase_if(n1, [](const auto& p){ return p.first % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  11. 常見陷阱 (Pitfalls) ★必看
    // ========================================================================
    //
    //  (1) operator[] 在 key 不存在時「會插入」!
    //      想單純查詢 → 用 find / contains / at。
    //      const map 不能用 operator[]。
    //
    //  (2) Rehash 後 iterator 失效
    //      reference / pointer 不失效 (節點本身不搬),但 iterator 全失效。
    //      大量插入前先 reserve(n)。
    //
    //  (3) 自訂型別當 key 必須提供好的 hash function
    //      壞的 hash 會把 unordered_map 退化為 linked list。
    //
    //  (4) 元素遍歷順序「沒有保證」且「可能改變」
    //      不要在輸出中依賴特定順序;不要在迭代中插入 (可能 rehash)。
    //
    //  (5) 沒有 lower_bound / upper_bound / 反向 iterator
    //      要範圍查詢 → 用 map。
    //
    //  (6) operator[] 要求 V 是 default-constructible
    //      若 V 沒 default constructor → 改用 try_emplace。
    //
    //  (7) for (auto [k,v] : m) 是「拷貝」每個 pair
    //      想避免拷貝 → for (auto& [k,v] : m)。
    //
    //  (8) 對「相同 key 的指標」是穩定的,但對「目前位置」不是
    //      若你存了 it (iterator),rehash 後就死了;若存的是 &v[k] 仍然有效。

    // ========================================================================
    //  12. 最佳實踐
    // ========================================================================
    //
    //  • 安全查詢用 find / contains / at,不要用 operator[]
    //  • 大量插入前先 reserve(n)
    //  • 想插入「若不存在」→ try_emplace (C++17)
    //  • 想插入「無論存不存在都覆寫」→ insert_or_assign (C++17)
    //  • 自訂 key → 寫好 hash function (均勻分布、快速、noexcept)
    //  • 不要依賴遍歷順序
    //  • 元素數量少 (< 數十) → vector<pair<K,V>> 通常更快

    // ========================================================================
    //  13. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // unordered_map 是 LeetCode 出現率最高的 container — 題目只要看到「找關係」
    // 「計次」「前綴和」就十有八九用 hash map。下面四題涵蓋了 90% 的套路。

    // ──── LC 1: Two Sum (兩數之和) — 面試最經典題 ────
    // 「掃過陣列時,看看有沒有看過 target - x」 → 把 value→index 存進 hash map。
    // 時間 O(n),空間 O(n)。
    {
        std::vector<int> nums{2, 7, 11, 15};
        int target = 9;
        std::unordered_map<int, int> seen;     // value → index
        std::pair<int, int> ans{-1, -1};
        for (int i = 0; i < (int)nums.size(); ++i) {
            auto it = seen.find(target - nums[i]);
            if (it != seen.end()) { ans = {it->second, i}; break; }
            seen[nums[i]] = i;
        }
        std::cout << "\n[LC1 Two Sum] indices = ("
                  << ans.first << ',' << ans.second << ")\n";
        // 預期輸出: (0, 1)
    }

    // ──── LC 49: Group Anagrams (字母異位詞分組) ────
    // 相同字母組成的字串 → 排序後 key 相同 → 用 unordered_map<string, vector<string>>。
    // 經典「特徵指紋」hash 套路。
    {
        std::vector<std::string> strs{"eat", "tea", "tan", "ate", "nat", "bat"};
        std::unordered_map<std::string, std::vector<std::string>> groups;
        for (const auto& s : strs) {
            std::string key = s;
            std::sort(key.begin(), key.end());
            groups[key].push_back(s);
        }
        std::cout << "[LC49 Group Anagrams]\n";
        for (const auto& [k, vs] : groups) {
            std::cout << "  [" << k << "] -> ";
            for (const auto& s : vs) std::cout << s << ' ';
            std::cout << '\n';
        }
        // 預期: {eat,tea,ate}, {tan,nat}, {bat}
    }

    // ──── LC 560: Subarray Sum Equals K (和為 K 的子陣列) ────
    // 計算「前綴和 == prefix - k 在歷史中出現過幾次」 → 累加。
    // 經典「前綴和 + 計數」題,O(n)。
    {
        std::vector<int> nums{1, 1, 1};
        int k = 2;
        std::unordered_map<int, int> prefix_cnt{{0, 1}};   // 前綴和 0 出現一次 (空陣列)
        int sum = 0, ans = 0;
        for (int x : nums) {
            sum += x;
            if (prefix_cnt.count(sum - k)) ans += prefix_cnt[sum - k];
            ++prefix_cnt[sum];
        }
        std::cout << "[LC560 SubarraySumK] count = " << ans << '\n';
        // 預期輸出: 2  ([1,1] 在 index 0..1 與 1..2 各一次)
    }

    // ──── LC 387: First Unique Character (第一個不重複字元) ────
    // 兩遍掃描: 第一遍計數; 第二遍找第一個 cnt == 1 的字元。
    {
        auto first_uniq = [](const std::string& s) -> int {
            std::unordered_map<char, int> cnt;
            for (char c : s) ++cnt[c];
            for (int i = 0; i < (int)s.size(); ++i) {
                if (cnt[s[i]] == 1) return i;
            }
            return -1;
        };
        std::cout << "[LC387 FirstUniq] \"loveleetcode\" → "
                  << first_uniq("loveleetcode") << '\n';
        // 預期輸出: 2  (字元 'v')
    }

    // ──── LC 13: Roman to Integer (羅馬數字轉整數) ────
    // 用 unordered_map<char,int> 做查表;若下一字大於本字則減,否則加。O(n)。
    // 這是「查表 + 簡單規則」的典型工程題,unordered_map 的 O(1) 查找最自然。
    {
        std::unordered_map<char,int> val{
            {'I',1},{'V',5},{'X',10},{'L',50},
            {'C',100},{'D',500},{'M',1000}
        };
        auto roman_to_int = [&](const std::string& s) {
            int total = 0;
            for (size_t i = 0; i < s.size(); ++i) {
                int cur = val[s[i]];
                int nxt = (i + 1 < s.size()) ? val[s[i+1]] : 0;
                total += (cur < nxt) ? -cur : cur;
            }
            return total;
        };
        std::cout << "[LC13 Roman] MCMXCIV = " << roman_to_int("MCMXCIV") << '\n';
        // 預期輸出: 1994
    }

    // ========================================================================
    //  12. 實戰範例:Web 服務的 Session 快取
    // ========================================================================
    // 真實場景:Web Server 收到請求時要快速根據 session_id (字串 token) 取得
    // 使用者資料 (UserInfo)。一個高流量服務每秒可能查上萬次,O(log n) 都嫌慢,
    // 需要 hash table 的 O(1) 平均查找。同時要支援:
    //   • set / get / erase 都 O(1)
    //   • 已知容量上限時先 reserve 避免 rehash
    {
        struct UserInfo { std::string name; int level; };
        std::unordered_map<std::string, UserInfo> sessions;
        sessions.reserve(1024);   // 預估同時上線 1000 人,先配 bucket

        sessions["sid_abc123"] = {"alice", 5};
        sessions["sid_def456"] = {"bob",   3};
        sessions["sid_ghi789"] = {"cathy", 8};

        // 查詢:O(1) 平均
        auto it = sessions.find("sid_def456");
        if (it != sessions.end())
            std::cout << "[Session] " << it->first << " -> "
                      << it->second.name << " (Lv." << it->second.level << ")\n";

        // 使用者登出:erase
        sessions.erase("sid_abc123");
        std::cout << "[Session] 目前線上 " << sessions.size() << " 人\n";
        // 預期輸出: sid_def456 -> bob (Lv.3) / 目前線上 2 人
    }

    std::cout << "\n=== unordered_map demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unordered_map 為何「平均 O(1)」但「最壞 O(n)」?
    //    A：hash table 採鏈式 (chaining) 處理碰撞,理想下每個 bucket 元素數為常數,
    //       插入/查詢 O(1)。但若所有 key hash 到同一個 bucket (惡意輸入或差勁
    //       hash function),退化成單一 linked list,變成 O(n)。
    //
    //  Q2：要為自訂型別作 key,該怎麼提供 hash function?
    //    A：兩種選擇:(1) 特化 std::hash<MyType> (放在 namespace std 內);(2) 直接
    //       傳 functor 當第三個 template 參數。同時必須提供 operator==。
    //       常見做法:把成員逐一 hash 後用 boost::hash_combine 或 XOR + 移位混合。
    //
    //  Q3：什麼是 rehash?何時觸發?如何避免效能跳水?
    //    A：當 load_factor (元素數 / bucket 數) 超過 max_load_factor (預設 1.0) 時,
    //       container 會自動配置更大的 bucket array 並重新 hash 所有元素 — 此操作
    //       為 O(n)。已知資料量時可先呼叫 reserve(n) 避免多次 rehash。
    //       注意:rehash 會使所有 iterator 失效,但 reference 仍有效。
    //
    return 0;
}

/*
============================================================================
  附錄:std::unordered_map 完整 member function 一覽
============================================================================
  Constructors / destructor / operator= / get_allocator

  Element access:  at, operator[]

  Iterators:       begin/end/cbegin/cend  (沒有 rbegin / rend)
  Capacity:        empty, size, max_size

  Modifiers:
      clear, insert, insert_range (C++23), insert_or_assign (C++17),
      emplace, emplace_hint, try_emplace (C++17),
      erase, swap, extract (C++17), merge (C++17)

  Lookup:          count, find, contains (C++20), equal_range
                   (沒有 lower_bound / upper_bound)

  Bucket:          begin(n), end(n), bucket_count, max_bucket_count,
                   bucket_size, bucket

  Hash policy:     load_factor, max_load_factor, rehash, reserve

  Observers:       hash_function, key_eq

  Non-member:      operator==, !=, std::swap, std::erase_if (C++20)
============================================================================
*/
