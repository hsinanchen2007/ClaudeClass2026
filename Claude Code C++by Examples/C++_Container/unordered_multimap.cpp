// ============================================================================
//  unordered_multimap.cpp — std::unordered_multimap 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra unordered_multimap.cpp -o unordered_multimap && ./unordered_multimap
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/unordered_multimap
//  參考 (cplusplus.com): https://cplusplus.com/reference/unordered_map/unordered_multimap/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::unordered_multimap<K, V> 與 unordered_map 幾乎相同,差別:
//      ★ 允許「同一個 key 對應多個 value」(允許重複 key)
//
//  ▌ 與 unordered_map 的差異
//      • 沒有 operator[] 和 at()  ★
//      • 沒有 insert_or_assign / try_emplace
//      • insert 永遠成功,回傳 iterator
//      • count(k) 可能 > 1
//      • erase(k) 刪除「所有」此 key 的元素,回傳數量
//      • equal_range(k) 真正有意義
//      • merge 不去重,全部搬過來
//
//  ▌ 時間複雜度
//      與 unordered_map 同,額外:
//      count(key)        平均 O(k)
//      erase(key)        平均 O(k)
//
//  ▌ 適用情境
//      ✅ 一個 key 多個 value + 不在乎順序 (e.g. tag→items, multi-index)
//      ✅ 需要平均 O(1) 取得「同 key 的所有 values」
//      ❌ 想做計數 → 用 unordered_map<K, int> 更直觀
//
//  ▌ Iterator 失效規則
//      與 unordered_map 同。
//
// ============================================================================

/*
補充筆記：std::unordered_multimap
  - unordered_multimap 允許相同 key 對應多筆 value；查群組時用 equal_range。
  - bucket 順序不是語意順序，輸出順序不可當成穩定結果。
  - 自訂 hash 不良會讓同 key 或碰撞資料集中到少數 bucket。
  - std::unordered_multimap 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::unordered_multimap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. unordered_multimap 和 unordered_map 差在哪？
//     答：它允許同一個 key 對應多個 value。因此：
//         沒有 operator[] 與 at()（key 不再是 1 對 1，語意不明）、也沒有 insert_or_assign / try_emplace；
//         insert 永遠成功並回傳 iterator；count(k) 可能 > 1；
//         erase(k) 會刪除該 key 的「全部」元素並回傳數量；merge 不去重、全部搬過來。
//     追問：什麼時候改用 unordered_map<K, vector<V>>？（需要把同 key 的 value 當一組整體操作時）
//
// 🔥 Q2. 如何取出某個 key 的所有 value？輸出順序可以依賴嗎？
//     答：用 equal_range(key) 取得整組區間，count(key) 與 erase(key) 的平均複雜度為 O(k)。
//         但 bucket 順序不是語意順序——走訪得到的輸出順序不能當成穩定結果，
//         需要排序請改用 multimap。
//
// ⚠️ 陷阱. rehash 之後，舊的 iterator 和 reference 還能用嗎？
//     答：規則與 unordered_map 完全相同：
//         **所有 iterator 失效，但 reference / pointer 永遠不會失效**。
//         因為 separate chaining 的元素住在獨立節點上，rehash 只重接 bucket array 與 next 指標。
//     為什麼會錯：拿 vector 擴容「iterator 與 reference 一起失效」的模型套到 hash table。
// ═══════════════════════════════════════════════════════════════════════════

#include <unordered_map>
#include <unordered_set>     // for LC49 GroupAnagrams 範例 (記錄已印過的 key)
#include <iostream>
#include <string>
#include <vector>            // for LeetCode 範例
#include <algorithm>         // std::sort (LC49 要把字串字母排序當 key)

template <typename K, typename V>
void print(const std::unordered_multimap<K, V>& m, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& [k, v] : m) std::cout << '(' << k << ':' << v << ") ";
    std::cout << "} (size=" << m.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::unordered_multimap<std::string, int> m1;
    std::unordered_multimap<std::string, int> m2{
        {"apple",1},{"banana",2},{"apple",3},{"apple",5}};
    std::unordered_multimap<std::string, int> m3(m2);
    std::unordered_multimap<std::string, int> m4(100);

    print(m2, "m2 (含重複 key)     ");

    // ========================================================================
    //  2. Iterators
    // ========================================================================
    std::cout << "\n[walk] ";
    for (auto& [k, v] : m2) std::cout << '(' << k << ',' << v << ") ";
    std::cout << '\n';

    // 容量補充:max_size + get_allocator
    std::cout << "max_size = " << m2.max_size() << '\n';
    auto umma = m2.get_allocator();
    (void)umma;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  3. Modifiers
    // ========================================================================

    // insert: 永遠成功
    std::unordered_multimap<std::string, int> m;
    m.insert({"a", 1});
    m.insert({"a", 2});
    m.insert({"b", 3});
    print(m, "after insert        ");

    // emplace
    std::unordered_multimap<int, std::string> em;
    em.emplace(1, "one");
    em.emplace(1, "uno");
    em.emplace(2, "two");
    print(em, "emplace             ");

    // emplace_hint:hash table 中 hint 通常無效益,但 API 一致提供
    auto eh_it = em.emplace_hint(em.end(), 3, "three");
    std::cout << "emplace_hint(3, \"three\") → ("
              << eh_it->first << ',' << eh_it->second << ")\n";

    // erase by key (刪所有)
    std::unordered_multimap<std::string,int> e{
        {"a",1},{"a",2},{"a",3},{"b",4}};
    std::cout << "\nerase(\"a\") → " << e.erase("a") << " 個\n";
    print(e, "after erase(a)      ");

    // erase by iterator
    e.erase(e.begin());
    print(e, "erase(begin)        ");

    // erase by range (利用 equal_range 只刪一部分)
    std::unordered_multimap<std::string,int> e2{
        {"a",1},{"a",2},{"a",3},{"b",4}};
    auto [lo, hi] = e2.equal_range("a");
    e2.erase(lo, std::next(lo));
    print(e2, "erase 1 of \"a\"    ");

    // merge (不去重)
    std::unordered_multimap<std::string,int> ma{{"a",1},{"b",2}};
    std::unordered_multimap<std::string,int> mb{{"a",10},{"c",20}};
    ma.merge(mb);
    print(ma, "after merge         ");
    print(mb, "mb (掏空)           ");

    // ========================================================================
    //  4. Lookup
    // ========================================================================
    std::unordered_multimap<std::string,int> q{
        {"a",1},{"a",2},{"a",3},{"b",4}};

    std::cout << "\ncount(\"a\") = " << q.count("a") << '\n';

    auto fi = q.find("a");
    if (fi != q.end()) std::cout << "find: " << fi->second << '\n';

    std::cout << "contains(\"c\") = " << q.contains("c") << '\n';

    // 主力查詢方式: equal_range
    auto [b1, e1] = q.equal_range("a");
    std::cout << "all \"a\"s: ";
    for (auto it_ = b1; it_ != e1; ++it_) std::cout << it_->second << ' ';
    std::cout << '\n';

    // ========================================================================
    //  5. Bucket interface (完整版)
    // ========================================================================
    // bucket_count / max_bucket_count / bucket_size / bucket(key) / begin(n)/end(n)
    std::cout << "\nbuckets=" << q.bucket_count()
              << ", max_bucket_count=" << q.max_bucket_count() << '\n';
    for (size_t i = 0; i < q.bucket_count() && i < 10; ++i) {
        std::cout << "  bucket " << i << " (size=" << q.bucket_size(i) << "): ";
        for (auto it_ = q.begin(i); it_ != q.end(i); ++it_)
            std::cout << '(' << it_->first << ',' << it_->second << ") ";
        std::cout << '\n';
    }
    std::cout << "  bucket(\"a\") = " << q.bucket("a") << '\n';

    // ========================================================================
    //  6. Hash policy (load_factor / max_load_factor / rehash / reserve)
    // ========================================================================
    std::cout << "\nload_factor=" << q.load_factor()
              << ", max_load_factor=" << q.max_load_factor() << '\n';
    q.reserve(100);
    std::cout << "after reserve(100), buckets=" << q.bucket_count() << '\n';
    q.rehash(50);
    std::cout << "after rehash(50), buckets=" << q.bucket_count() << '\n';
    q.max_load_factor(0.75f);
    std::cout << "max_load_factor(0.75) → " << q.max_load_factor() << '\n';

    // ========================================================================
    //  7. Observers (hash_function / key_eq)
    // ========================================================================
    auto h = q.hash_function();
    std::cout << "hash(\"a\") = " << h("a") << '\n';
    auto ke = q.key_eq();
    std::cout << "key_eq(\"a\",\"a\") = " << ke("a", "a") << '\n';

    // ========================================================================
    //  6. C++20 free functions
    // ========================================================================
    std::unordered_multimap<int,int> n1{{1,1},{2,2},{2,3},{3,4}};
    std::erase_if(n1, [](const auto& p){ return p.second % 2 == 0; });
    print(n1, "erase_if(even val)  ");

    // ========================================================================
    //  7. 常見陷阱
    // ========================================================================
    //
    //  (1) 沒有 operator[] / at — key 不唯一,沒有單一 value
    //  (2) 沒有 try_emplace / insert_or_assign
    //  (3) erase(key) 刪「所有」相等 key
    //  (4) 想計數 → 用 unordered_map<K,int>,別用 unordered_multimap
    //  (5) Rehash 使 iterator 失效但 reference 不失效

    // ========================================================================
    //  8. 最佳實踐
    // ========================================================================
    //
    //  • 取某 key 的所有 value → equal_range
    //  • 計數場景 → unordered_map<K,int>
    //  • 想要「key→list of values」的 batch 處理 → unordered_map<K, vector<V>>
    //  • 只是想要「重複插入 key」 → 才用 unordered_multimap

    // ========================================================================
    //  9. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // unordered_multimap 在演算法題中相對少見 (大多數情況用 unordered_map<K, vector<V>>
    // 更直觀)。但「電話簿、tag 索引、不關心順序的 group」場景很適合。

    // ──── 電話簿: 一人多號碼 ────
    // unordered_multimap<姓名, 電話> 自然表達「一個 key 多個 value」。
    // 查詢用 equal_range,O(平均 1 + k) 取得某人所有電話。
    {
        std::unordered_multimap<std::string, std::string> phonebook;
        phonebook.emplace("Alice", "0912-111-111");
        phonebook.emplace("Bob",   "0987-222-222");
        phonebook.emplace("Alice", "0922-333-333");   // Alice 第二支
        phonebook.emplace("Cathy", "0900-444-444");

        std::cout << "\n[unordered_multimap PhoneBook] Alice 的電話: ";
        auto [b, e] = phonebook.equal_range("Alice");
        for (auto it = b; it != e; ++it) std::cout << it->second << ' ';
        std::cout << '\n';
    }

    // ──── 字母分組: 用 hash 打散字母,聚合單字 ────
    // 每個單字「首字母」作為 key,所有單字會自動分群。
    {
        std::vector<std::string> words{"apple", "ant", "banana", "berry", "cherry", "blueberry"};
        std::unordered_multimap<char, std::string> groups;
        for (const auto& w : words) groups.emplace(w[0], w);
        // 走訪 'b' 群
        std::cout << "[group by first letter, key='b']: ";
        auto [lo, hi] = groups.equal_range('b');
        for (auto it = lo; it != hi; ++it) std::cout << it->second << ' ';
        std::cout << '\n';
    }

    // ──── LC 49 風格: Group Anagrams (字母重排分組) ────
    // 題目簡述:
    //   給字串陣列, 把「字母重排相同」(anagram) 的字串歸為同一組。
    //   例如 ["eat","tea","tan","ate","nat","bat"] →
    //        [["eat","tea","ate"], ["tan","nat"], ["bat"]]
    //
    // 為什麼用 unordered_multimap 自然:
    //   把每個字串「排序後的字母」當 key,原字串當 value:
    //     "eat" → "aet" → group "aet"
    //     "tea" → "aet" → 同一個 group
    //   多個字串對應同一個 key (排序後字母),正是 multimap 的本意。
    //   要列出某 group → equal_range(sorted_key)。
    //
    // 時間複雜度: O(n * k log k),k 為字串平均長度 (排序成本)
    // 空間: O(n * k)
    //
    // 註: 標準解法常用 unordered_map<string, vector<string>>,程式碼略短;
    //      這裡用 multimap 強調「一 key 多 value」的容器原生語意。
    {
        std::vector<std::string> strs{"eat", "tea", "tan", "ate", "nat", "bat"};
        std::unordered_multimap<std::string, std::string> groups;
        for (const auto& s : strs) {
            std::string key = s;
            std::sort(key.begin(), key.end());     // 排序字母當 key
            groups.emplace(std::move(key), s);
        }

        // 列出每個 group (用 set 紀錄已印過的 key 避免重複列出)
        std::cout << "[LC49 GroupAnagrams]\n";
        std::unordered_set<std::string> printed;
        for (const auto& [k, v] : groups) {
            if (printed.count(k)) continue;
            printed.insert(k);
            std::cout << "  key='" << k << "': [ ";
            auto [lo, hi] = groups.equal_range(k);
            for (auto it = lo; it != hi; ++it) std::cout << it->second << ' ';
            std::cout << "]\n";
        }
        // 預期輸出 (各 group 內順序可能不同):
        //   key='aet': [ eat tea ate ]
        //   key='ant': [ tan nat ]
        //   key='abt': [ bat ]
    }

    // ──── LC 風格:Tag 索引 — 一個 tag 對應多個文章 ID ────
    // 部落格、購物網站常見:「依標籤檢索文章 / 商品」。每篇文章可能有多個 tag,
    // 但「以 tag 為 key 反查文章」是一對多關係 → unordered_multimap 自然契合。
    // 時間:emplace O(1) 平均;equal_range O(1 + k) 平均 (k 為該 tag 的命中數)。
    {
        std::unordered_multimap<std::string, int> tag2post;
        // 文章 1 標籤: cpp, stl
        tag2post.emplace("cpp", 1);
        tag2post.emplace("stl", 1);
        // 文章 2 標籤: cpp, algorithm
        tag2post.emplace("cpp", 2);
        tag2post.emplace("algorithm", 2);
        // 文章 3 標籤: stl, container
        tag2post.emplace("stl", 3);
        tag2post.emplace("container", 3);

        std::cout << "[Tag Index] tag='cpp' 的文章: ";
        auto [b, e] = tag2post.equal_range("cpp");
        for (auto it = b; it != e; ++it) std::cout << it->second << ' ';
        std::cout << '\n';
        // 預期輸出: 1 2  (順序不保證)
    }

    // ========================================================================
    //  12. 實戰範例:錯誤碼分類器 — 同類錯誤多條訊息
    // ========================================================================
    // 真實場景:後端服務有多種錯誤碼 (4xx / 5xx),每種錯誤碼下會記錄多筆訊息。
    // 需求:依錯誤碼快速取出所有訊息做監控與告警。
    //   • emplace 平均 O(1)
    //   • 不需要排序 (錯誤碼通常是離散整數,順序無意義)
    //   • count(code) 可立刻得到該錯誤發生次數
    {
        std::unordered_multimap<int, std::string> error_log;
        error_log.emplace(404, "/api/user not found");
        error_log.emplace(500, "DB connection timeout");
        error_log.emplace(404, "/api/order not found");
        error_log.emplace(503, "service unavailable");
        error_log.emplace(500, "internal error: null ptr");

        std::cout << "[Error Classifier] 404 共 " << error_log.count(404) << " 筆\n";
        std::cout << "[Error Classifier] 500 共 " << error_log.count(500) << " 筆\n";
        auto [b, e] = error_log.equal_range(500);
        std::cout << "  500 訊息: ";
        for (auto it = b; it != e; ++it) std::cout << '[' << it->second << "] ";
        std::cout << '\n';
        // 預期輸出: 404 共 2 筆 / 500 共 2 筆 / 訊息列表
    }

    std::cout << "\n=== unordered_multimap demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unordered_multimap 同 key 多筆的相對順序有保證嗎?
    //    A：標準「不保證」走訪順序 — 既然是 unordered,不要依賴。即使是同一個 key
    //       取 equal_range,實作之間可能不同;若需要穩定順序請改用 multimap (紅黑樹)。
    //
    //  Q2：為何 unordered_multimap 沒有 operator[] 也沒有 at()?
    //    A：和 multimap 同樣理由 — 一個 key 對應多筆 value,operator[] 取哪一個
    //       語意不明確,標準選擇直接拿掉。要查 / 列舉同 key 全部資料請用 equal_range。
    //
    //  Q3：什麼時候用 unordered_multimap 而不是 unordered_map<K, vector<V>>?
    //    A：unordered_multimap 適合「插入頻繁、不在意走訪順序、不常需要列舉同 key
    //       全部 value」的情境;若常常要拿出某個 key 的所有 value 一起處理 (例如
    //       group anagrams),unordered_map<K, vector<V>> 通常 cache 友善且更直觀。
    //
    return 0;
}

/*
============================================================================
  附錄:std::unordered_multimap member functions
============================================================================
  介面與 std::unordered_map 相同,主要差異:
      ★ 沒有 at, operator[], insert_or_assign, try_emplace
      ★ insert 回傳 iterator
      ★ count / erase 可能 > 1
      ★ equal_range 真正有用
      ★ merge 不去重
============================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra unordered_multimap.cpp -o unordered_multimap

// === 預期輸出 (節錄) ===
// m2 (含重複 key)     : { (banana:2) (apple:1) (apple:5) (apple:3) } (size=4)
//
// [walk] (banana,2) (apple,1) (apple,5) (apple,3)
// max_size = 329406144173384850
// get_allocator() OK
// after insert        : { (b:3) (a:1) (a:2) } (size=3)
// emplace             : { (2:two) (1:uno) (1:one) } (size=3)
// emplace_hint(3, "three") → (3,three)
//
// erase("a") → 3 個
// after erase(a)      : { (b:4) } (size=1)
// erase(begin)        : { } (size=0)
// erase 1 of "a"    : { (b:4) (a:3) (a:2) } (size=3)
// after merge         : { (c:20) (a:10) (a:1) (b:2) } (size=4)
// mb (掏空)           : { } (size=0)
//
// count("a") = 3
// find: 1
// contains("c") = 0
// all "a"s: 1 3 2
// …（後略，完整輸出共 49 行）
