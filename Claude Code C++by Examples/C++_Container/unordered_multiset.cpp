// ============================================================================
//  unordered_multiset.cpp — std::unordered_multiset 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra unordered_multiset.cpp -o unordered_multiset && ./unordered_multiset
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/unordered_multiset
//  參考 (cplusplus.com): https://cplusplus.com/reference/unordered_set/unordered_multiset/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::unordered_multiset 與 unordered_set 幾乎完全相同,差別:
//      ★ 允許重複元素。
//
//  ▌ 與 unordered_set 的差異
//      • insert 永遠成功,回傳 iterator (不是 pair<iterator,bool>)
//      • count(k) 可能 > 1
//      • erase(k) 刪除「所有」此 key,回傳數量
//      • equal_range(k) 真正有意義
//      • merge 全部搬過來,不去重
//
//  ▌ 時間複雜度
//      與 unordered_set 完全相同,額外:
//      count(key)        平均 O(k),k 為相等元素數
//      erase(key)        平均 O(k)
//
//  ▌ 適用情境
//      ✅ 需要平均 O(1) 計數 + 允許重複 (e.g. word count、流量統計的中間表)
//      ✅ 需要快速「某個 key 出現幾次」
//      ❌ 需要排序 → multiset
//
//  ▌ Iterator 失效規則
//      與 unordered_set 完全相同 (rehash 全失效, erase 局部失效)。
//
// ============================================================================

/*
補充筆記：std::unordered_multiset
  - unordered_multiset 允許重複 key，適合不需要排序的多重集合。
  - count(key) 會回傳該 key 的出現次數，但成本和該 bucket 內容相關。
  - erase(key) 會移除所有等價 key；只刪一個要用 iterator。
  - std::unordered_multiset 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::unordered_multiset
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. unordered_multiset 和 unordered_set 差在哪？
//     答：它允許重複元素。因此 insert 永遠成功並回傳 iterator（不是 pair<iterator,bool>）；
//         count(k) 可能 > 1（平均 O(k)，k 為相等元素數）；equal_range(k) 才真正有意義；
//         merge 不去重、全部搬過來。其餘時間複雜度與 unordered_set 完全相同。
//     追問：需要排序時該用什麼？（multiset，底層是 red-black tree）
//
// 🔥 Q2. erase(key) 會刪掉幾個元素？
//     答：全部等值元素，並回傳刪除數量（平均 O(k)）。
//         只想刪一個必須傳 iterator，例如 erase(ms.find(key))。
//
// ⚠️ 陷阱. rehash 之後，舊的 iterator 和 reference 還能用嗎？
//     答：規則與 unordered_set 完全相同：
//         **所有 iterator 失效，但 reference / pointer 永遠不會失效**。
//         因為 separate chaining 的元素住在獨立配置的節點上，rehash 只重接 bucket array 與 next 指標，
//         節點本體沒有搬家。erase 則只使被刪元素的 iterator / reference 失效。
//     為什麼會錯：直覺以為「bucket 重新分配 = 元素被搬到新位置」，
//         但那只發生在 open addressing。
// ═══════════════════════════════════════════════════════════════════════════

#include <unordered_set>      // unordered_multiset 也在 <unordered_set>
#include <iostream>
#include <string>
#include <vector>             // for LeetCode 範例
#include <unordered_map>

template <typename T>
void print(const std::unordered_multiset<T>& s, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& e : s) std::cout << e << ' ';
    std::cout << "} (size=" << s.size() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::unordered_multiset<int> m1;
    std::unordered_multiset<int> m2{3,1,4,1,5,9,2,6,5,3};   // 重複保留
    std::unordered_multiset<int> m3(m2);
    std::unordered_multiset<int> m4(100);                  // 100 buckets

    print(m2, "m2 (含重複)         ");

    // ========================================================================
    //  2. Iterators (forward,無反向)
    // ========================================================================
    std::cout << "\n[walk] ";
    for (auto e : m2) std::cout << e << ' ';
    std::cout << '\n';

    // ========================================================================
    //  3. 容量
    // ========================================================================
    std::cout << "\n[Capacity] size=" << m2.size() << ", max_size="
              << m2.max_size()
              << ", empty=" << std::boolalpha << m2.empty() << '\n';

    // get_allocator()
    auto uma = m2.get_allocator();
    (void)uma;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  4. Modifiers
    // ========================================================================

    // insert: 永遠成功 (★ 與 unordered_set 不同,不會去重)
    //   - 單一值      : 回傳 iterator 指向新插入的元素
    //   - initializer : 多筆一次插入
    //   - hint 版本   : 對 hash table 而言 hint 通常無效益 (與 set/map 不同),
    //                   但 API 一致提供以利模板程式碼共用
    //   - range 版本  : insert(first, last) 也支援
    std::unordered_multiset<int> m;
    m.insert(10);
    m.insert(10);                 // 第二個 10 也會留
    m.insert({1,2,2,3,3,3});
    auto hint_it = m.insert(m.end(), 99);   // hint 版本 (hash 表中 hint 被忽略)
    std::cout << "insert(end, 99) → *it=" << *hint_it << '\n';
    std::vector<int> more{7, 8, 8};
    m.insert(more.begin(), more.end());     // range 版本
    print(m, "after insert        ");

    // emplace
    std::unordered_multiset<std::string> em;
    em.emplace(5, 'A');
    em.emplace(5, 'A');
    print(em, "emplace             ");

    // emplace_hint:hint 對 hash table 無效益,但 API 一致提供
    auto eh_it = em.emplace_hint(em.end(), "Z");
    std::cout << "emplace_hint(\"Z\") → " << *eh_it << '\n';

    // erase
    std::unordered_multiset<int> e{1,2,2,2,3,4};
    std::cout << "\nerase(2) → " << e.erase(2) << " 個\n";
    print(e, "after erase(2)      ");
    e.erase(e.begin());
    print(e, "erase(begin)        ");

    // merge: ★ multiset 版的 merge 不去重,全部搬過來
    std::unordered_multiset<int> a{1,2,3}, b{2,3,4};
    a.merge(b);
    print(a, "after merge         ");
    print(b, "b (掏空)            ");

    // swap / clear
    std::unordered_multiset<int> sw1{1}, sw2{9,9};
    sw1.swap(sw2);
    print(sw1, "after swap          ");
    sw1.clear();

    // ========================================================================
    //  4. Lookup
    // ========================================================================
    std::unordered_multiset<int> q{1,2,2,3,3,3,4};
    std::cout << "\ncount(3) = " << q.count(3) << '\n';
    std::cout << "contains(2) = " << q.contains(2) << '\n';

    auto fi = q.find(3);
    if (fi != q.end()) std::cout << "find(3) → " << *fi << '\n';

    auto [b1, e1] = q.equal_range(3);
    std::cout << "all 3s: ";
    for (auto it = b1; it != e1; ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  6. Bucket interface (完整版)
    // ========================================================================
    // bucket_count / max_bucket_count / bucket_size / bucket(key) / begin(n) / end(n)
    std::cout << "\nbuckets=" << q.bucket_count()
              << ", max_bucket_count=" << q.max_bucket_count() << '\n';
    for (size_t i = 0; i < q.bucket_count() && i < 10; ++i) {     // 只看前 10 格避免太長
        std::cout << "  bucket " << i << " (size=" << q.bucket_size(i) << "): ";
        for (auto it_ = q.begin(i); it_ != q.end(i); ++it_) std::cout << *it_ << ' ';
        std::cout << '\n';
    }
    std::cout << "  bucket(3) = " << q.bucket(3) << '\n';

    // ========================================================================
    //  7. Hash policy (load_factor / max_load_factor / rehash / reserve)
    // ========================================================================
    std::cout << "\nload_factor = " << q.load_factor()
              << ", max_load_factor = " << q.max_load_factor() << '\n';
    q.reserve(100);
    std::cout << "after reserve(100), buckets = " << q.bucket_count() << '\n';
    q.rehash(50);
    std::cout << "after rehash(50), buckets = " << q.bucket_count() << '\n';
    q.max_load_factor(0.75f);
    std::cout << "max_load_factor(0.75) → " << q.max_load_factor() << '\n';

    // ========================================================================
    //  8. Observers (hash_function / key_eq)
    // ========================================================================
    auto h = q.hash_function();
    std::cout << "hash(3) = " << h(3) << '\n';
    auto eq2 = q.key_eq();
    std::cout << "key_eq(3,3) = " << eq2(3, 3) << '\n';

    // ========================================================================
    //  6. C++20 free functions
    // ========================================================================
    std::unordered_multiset<int> n1{1,2,2,3,3,3,4};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  7. 常見陷阱
    // ========================================================================
    //
    //  (1) erase(key) 刪「所有」相等 key,要小心用
    //  (2) find 不保證回傳哪一個
    //  (3) 走訪順序無保證,且因為碰撞可能在每次插入後改變
    //  (4) 需要計算「某 key 出現幾次」 → 不如用 unordered_map<K,int> 自己累加,
    //      因為 multiset 的 count(k) 是 O(k) (走完整個 bucket),
    //      而 map 的 count 是 O(1)。

    // ========================================================================
    //  8. 最佳實踐
    // ========================================================================
    //
    //  • 想要 O(1) 計數 → 用 unordered_map<K, int>,而非 unordered_multiset
    //  • 真的有「保留每個重複實例」的需求 → 才用 multiset
    //  • 大量插入前先 reserve(n) 避免反覆 rehash

    // ========================================================================
    //  9. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // unordered_multiset 適合「兩個多重集合是否相等」這類題目 — 不在意順序,
    // 但在意每個元素的「出現次數」。注意大多數情況用 unordered_map 計數更快。

    // ──── LC 1207: Unique Number of Occurrences (出現次數是否唯一) ────
    // 給陣列, 計算每個元素的出現次數,然後判斷這些「次數」彼此是否唯一。
    // 第二步剛好就是 multiset 的「等於某 key 個數是否 == 1」判斷。
    {
        std::vector<int> arr{1, 2, 2, 1, 1, 3};
        std::unordered_map<int, int> cnt;
        for (int x : arr) ++cnt[x];                  // 1→3, 2→2, 3→1
        std::unordered_multiset<int> freqs;
        for (const auto& [k, c] : cnt) freqs.insert(c);
        bool unique = true;
        for (int f : freqs) if (freqs.count(f) > 1) { unique = false; break; }
        std::cout << "\n[LC1207 UniqueOccur] " << std::boolalpha << unique << '\n';
        // 預期輸出: true (出現次數分別為 3, 2, 1,彼此不同)
    }

    // ──── LC 242: Valid Anagram (字母異位詞判斷) — 用 multiset<char> ────
    // 題目簡述:
    //   給兩個字串 s, t,判斷 t 是不是 s 的「字母異位詞」 — 也就是兩者字母
    //   出現次數完全相同,但順序可不同。例如 "anagram" 與 "nagaram" → true。
    //
    // 為什麼 unordered_multiset 自然解:
    //   兩字串若是 anagram ⇔ 把字母拆成「多重集合 (bag)」會完全相等。
    //   unordered_multiset 內建 == 運算子比較「每個元素出現次數是否都一樣」,
    //   一行就解決。注意走訪順序不同沒關係 — operator== 會逐元素比對個數。
    //
    // 時間複雜度: 平均 O(n + m),n、m 為兩字串長度。
    // 空間複雜度: O(n + m)。
    //
    // 註: 競賽常用 array<int,26> 計數,常數更小;這裡用 multiset 是為了學習
    //      「容器 == 比較」的語意。
    {
        auto is_anagram = [](const std::string& s, const std::string& t) {
            return std::unordered_multiset<char>(s.begin(), s.end())
                == std::unordered_multiset<char>(t.begin(), t.end());
        };
        std::cout << "[LC242 ValidAnagram] "
                  << is_anagram("anagram", "nagaram") << ' '  // 1 (true)
                  << is_anagram("listen",  "silent")  << ' '  // 1 (true)
                  << is_anagram("rat",     "car")     << '\n';// 0 (false)
    }

    // ──── 實務範例: 兩個訂單明細 (含重複品項) 是否內容相同 ────
    // 場景: 兩位倉管各自掃描出貨清單,品項重複時數量也要一致才算對得起來。
    // 用 unordered_multiset<string> 比較,順序無關但個數有關。
    // 比起寫雙層迴圈 O(n*m),平均 O(n+m) 直接完成。
    {
        std::vector<std::string> picker_a{"apple", "banana", "apple", "cherry"};
        std::vector<std::string> picker_b{"banana", "cherry", "apple", "apple"};
        std::vector<std::string> picker_c{"apple", "banana", "cherry"};       // 少一個 apple

        std::unordered_multiset<std::string> a(picker_a.begin(), picker_a.end());
        std::unordered_multiset<std::string> b(picker_b.begin(), picker_b.end());
        std::unordered_multiset<std::string> c(picker_c.begin(), picker_c.end());
        std::cout << "[訂單明細比對] a==b? " << (a == b)        // true (順序不同但個數同)
                  << ", a==c? " << (a == c) << '\n';            // false (apple 個數不同)
    }

    // ──── LC 383: Ransom Note (贖金信) ────
    // 給 ransomNote 和 magazine 兩字串,判斷能否用 magazine 的字母組成 ransomNote
    // (每個字母只能用一次)。
    // 思路:把 magazine 字母倒進 multiset,然後 ransomNote 每個字母都嘗試 erase 一個;
    //        若中途 find 不到代表字母不夠,直接 false。
    // 為何用 multiset:erase(iterator) 一次只刪一個,完美模擬「字母用一次少一次」。
    {
        auto can_construct = [](const std::string& note, const std::string& mag) {
            std::unordered_multiset<char> bag(mag.begin(), mag.end());
            for (char c : note) {
                auto it = bag.find(c);
                if (it == bag.end()) return false;
                bag.erase(it);    // 只移除一個
            }
            return true;
        };
        std::cout << "[LC383 RansomNote] "
                  << std::boolalpha
                  << can_construct("a", "b")        << ' '   // false
                  << can_construct("aa", "aab")     << ' '   // true
                  << can_construct("aab", "aaab")   << '\n'; // true
    }

    // ========================================================================
    //  12. 實戰範例:庫存比對 — 兩個倉庫貨品清單差異
    // ========================================================================
    // 真實場景:零售系統有「總倉」與「分店」兩份貨品清單 (含重複品項,代表庫存數量)。
    // 需求:找出「分店缺少」的品項。用 unordered_multiset 對「總倉」建模,
    // 然後對分店每個品項 erase 一個 — 最後 multiset 內剩下的就是分店缺貨。
    // 時間平均 O(n + m),空間 O(n)。
    {
        std::vector<std::string> warehouse{"A","A","A","B","B","C","D","D"};
        std::vector<std::string> store    {"A","A","B","D"};

        std::unordered_multiset<std::string> miss(warehouse.begin(), warehouse.end());
        for (auto& item : store) {
            auto it = miss.find(item);
            if (it != miss.end()) miss.erase(it);
        }
        std::cout << "[Inventory Diff] 分店缺貨: [ ";
        for (auto& x : miss) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: 缺貨 = 1xA, 1xB, 1xC, 1xD (順序不保證)
    }

    std::cout << "\n=== unordered_multiset demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unordered_multiset.count(x) 的複雜度是多少?
    //    A：平均 O(k)、最壞 O(n),k 是與 x 相等的元素數量。實作必須走訪該 key 的
    //       所有 bucket 元素計數。如果只想知道「至少有一個」,用 contains(x)
    //       (C++20) 或 find(x) != end() 更快。
    //
    //  Q2：unordered_multiset 的 == 運算子怎麼判斷兩個容器相等?
    //    A：忽略走訪順序,只要「每個 key 的出現次數相同」就視為相等 — 完美符合
    //       「multiset 是 multiset of values」的數學定義。底層不需要排序,先建 hash
    //       表計次再比對。LC1207 唯一出現次數類題目可用此特性簡化判斷。
    //
    //  Q3：什麼時候用 unordered_multiset 而不是 unordered_map<K, int> 計次?
    //    A：multiset 直接把每筆「值」存下來,適合保留「插入紀錄」(可逐筆 erase
    //       單一個);map<K, int> 只記次數,刪除單一筆要手動 --count。資料量大且
    //       純粹計次時 map<K, int> 更省記憶體;需要存樣本本身則 multiset 更直觀。
    //
    return 0;
}

/*
============================================================================
  附錄:std::unordered_multiset member functions
============================================================================
  介面與 std::unordered_set 相同,差異:
      • insert 回傳 iterator
      • count / erase 可能 > 1
      • equal_range 真正有用
      • merge 不去重
============================================================================
*/

// 編譯: g++ -std=c++20 -Wall -Wextra unordered_multiset.cpp -o unordered_multiset

// === 預期輸出 (節錄) ===
// m2 (含重複)         : { 6 2 9 5 5 4 1 1 3 3 } (size=10)
//
// [walk] 6 2 9 5 5 4 1 1 3 3
//
// [Capacity] size=10, max_size=1152921504606846975, empty=false
// get_allocator() OK
// insert(end, 99) → *it=99
// after insert        : { 7 8 8 99 3 3 3 2 2 1 10 10 } (size=12)
// emplace             : { AAAAA AAAAA } (size=2)
// emplace_hint("Z") → Z
//
// erase(2) → 3 個
// after erase(2)      : { 4 3 1 } (size=3)
// erase(begin)        : { 3 1 } (size=2)
// after merge         : { 4 1 2 2 3 3 } (size=6)
// b (掏空)            : { } (size=0)
// after swap          : { 9 9 } (size=2)
//
// count(3) = 3
// contains(2) = true
// …（後略，完整輸出共 48 行）
