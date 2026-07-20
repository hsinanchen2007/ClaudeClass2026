// ============================================================================
//  unordered_set.cpp — std::unordered_set 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra unordered_set.cpp -o unordered_set && ./unordered_set
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/unordered_set
//  參考 (cplusplus.com): https://cplusplus.com/reference/unordered_set/unordered_set/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::unordered_set (C++11 引入) 是「無序、不允許重複」的關聯容器。
//  底層為 hash table,平均 O(1) 操作,但最差仍是 O(n) (碰撞極端時)。
//
//  ▌ 底層資料結構
//  典型實作為「鏈結式 hash table」 (chaining):
//
//      bucket array (N 格)
//      ┌──────┐
//      │  ●─→ key1 ─→ key3
//      ├──────┤
//      │ null │
//      ├──────┤
//      │  ●─→ key2
//      ├──────┤
//      │  ●─→ key4 ─→ key5
//      └──────┘
//
//  Hash 函式把 key 映射到 bucket index,同 bucket 用單向 linked list 串起來。
//
//  ▌ 所屬類別
//  Unordered associative container
//
//  ▌ 時間複雜度
//      insert / erase / find / count       平均 O(1),最差 O(n)
//      bucket interface (bucket / bucket_count / load_factor)  O(1)
//      rehash / reserve                    平均 O(n)
//      size / empty                        O(1)
//
//  ▌ 與其他 container 的比較
//      unordered_set vs set            : set 有序、O(log n) 穩定;這個無序、平均 O(1)
//      unordered_set vs unordered_multi: 後者允許重複
//      unordered_set vs vector<T>+find : 元素少時 vector 更快 (cache 友善)
//
//  ▌ 適用情境
//      ✅ 大量「存在性查詢」(set membership) — 是否包含某個元素
//      ✅ 不在乎順序、追求平均 O(1)
//      ❌ 需要排序或範圍查詢 → 用 set
//      ❌ 需要絕對的最差 O(log n) 保證 → 用 set
//
//  ▌ Iterator 失效規則 ★
//      ★ 任何「rehash」(因 insert 觸發或主動呼叫) 都會使「所有 iterator」失效
//      ★ rehash 不影響「reference / pointer」(節點本身位置不變)
//      • erase: 只有被刪元素的 iterator 失效
//      • clear: 全部失效
//
//  ▌ 何時會 rehash?
//      當 size() > max_load_factor() * bucket_count() 時。
//      預設 max_load_factor 為 1.0。可透過 reserve / rehash 預先擴容。
//
// ============================================================================

/*
補充筆記：std::unordered_set
  - unordered_set 只保存唯一 key，順序不穩定也不應依賴。
  - 效能依賴 hash 與 key_equal 品質；自訂型別必須同時提供一致的 hash 與相等判斷。
  - rehash 會使 iterator 失效，但對元素本身的 reference 通常仍有效。
  - std::unordered_set 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::unordered_set
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. unordered_set 的元素為什麼不能修改？
//     答：因為元素本身就是雜湊的依據，修改它會讓元素落在錯的 bucket，使容器內部結構不一致
//         （之後就可能永遠查不到它）。因此 iterator 與 const_iterator 都指向 const 元素。
//         要改就 erase 舊值再 insert 新值，或 C++17 起用 extract() 取出 node handle、
//         改完 value() 再 insert 回去（不需重新配置記憶體）。
//
// 🔥 Q2. load_factor / max_load_factor / rehash / reserve 各是什麼？
//     答：load_factor() = size() / bucket_count()。max_load_factor()（預設 1.0）是觸發 rehash 的門檻——
//         插入後若將超過它，容器自動增加 bucket 數並重新分配所有元素。
//         reserve(n) 等價於 rehash(ceil(n / max_load_factor()))，事先呼叫可避免多次 rehash。
//
// Q3. unordered_set 的 iterator 為什麼不能 `--`？
//     答：因為它是 forward iterator。libstdc++ 把所有元素串在一條 singly linked list 上，
//         bucket array 只存指向該 bucket 前一節點的指標；singly linked list 天生無法反向走訪。
//         所以 unordered 容器沒有 rbegin() / rend()，也不能用需要 bidirectional 的演算法。
//
// ⚠️ 陷阱. rehash 之後，舊的 iterator 和 reference 還能用嗎？
//     答：**所有 iterator 失效，但 reference / pointer 永遠不會失效**。
//         因為 separate chaining 的元素住在獨立配置的節點上，rehash 只重接 bucket array 與 next 指標，
//         節點本體完全沒搬家。erase 則只使被刪元素的 iterator / reference 失效。
//     為什麼會錯：拿 vector 擴容「整塊搬家、iterator 與 reference 一起失效」的模型套到 hash table；
//         但只有 open addressing 才會搬動元素本體。
// ═══════════════════════════════════════════════════════════════════════════

#include <unordered_set>
#include <iostream>
#include <string>
#include <vector>           // for LeetCode 範例
#include <algorithm>        // std::max

template <typename T>
void print(const std::unordered_set<T>& s, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "{ ";
    for (const auto& e : s) std::cout << e << ' ';
    std::cout << "} (size=" << s.size()
              << ", buckets=" << s.bucket_count() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::unordered_set<int> s1;
    std::unordered_set<int> s2{3, 1, 4, 1, 5, 9, 2, 6};   // 重複自動去重
    std::unordered_set<int> s3(s2.begin(), s2.end());
    std::unordered_set<int> s4(s2);
    std::unordered_set<int> s5(std::move(s4));
    // 指定初始 bucket 數
    std::unordered_set<int> s6(100);                     // 至少 100 個 bucket

    print(s2, "s2 (去重)           ");
    print(s6, "s6 (100 buckets)    ");

    // ========================================================================
    //  2. 元素存取
    // ========================================================================
    //  unordered_set 沒有 operator[] / at — 元素本身即 key,且為 const。

    // ========================================================================
    //  3. Iterators (forward iterator,只能 ++)
    // ========================================================================
    //  ★ 沒有 reverse iterator,因為 hash table 沒有「順序」概念。
    std::cout << "\n[walk] ";
    for (auto it = s2.begin(); it != s2.end(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // ========================================================================
    //  4. 容量
    // ========================================================================
    std::cout << "\n[Capacity] size=" << s2.size()
              << ", empty=" << std::boolalpha << s2.empty() << '\n';

    // max_size():理論上限
    std::cout << "max_size = " << s2.max_size() << '\n';

    // get_allocator()
    auto usa = s2.get_allocator();
    (void)usa;
    std::cout << "get_allocator() OK\n";

    // ========================================================================
    //  5. Modifiers
    // ========================================================================

    // ──── insert ────
    // 回傳 pair<iterator, bool>;bool=false 表示已存在
    std::unordered_set<int> m;
    auto [it1, ok1] = m.insert(10);
    auto [it2, ok2] = m.insert(10);   // 已存在
    std::cout << "\ninsert 10 ok=" << ok1 << ", again ok=" << ok2 << '\n';
    m.insert({1,2,3,4,5});
    print(m, "after insert        ");

    // emplace
    std::unordered_set<std::string> em;
    em.emplace(5, 'A');
    em.emplace("hello");
    print(em, "emplace             ");

    // emplace_hint:hint 對 hash table 通常無效益 (位置由 hash 決定),
    // 但 API 為了與 set/map 一致仍提供。回傳 iterator (沒有 bool)。
    auto eh_it = em.emplace_hint(em.end(), "world");
    std::cout << "emplace_hint(\"world\") → " << *eh_it << '\n';

    // ──── erase ────
    std::unordered_set<int> e{1,2,3,4,5};
    e.erase(3);                       // by key,回傳 0 或 1
    e.erase(e.begin());               // by iterator
    print(e, "after erase         ");

    // extract / merge (C++17)
    std::unordered_set<int> ma{1,2,3}, mb{2,3,4};
    ma.merge(mb);                     // 衝突的不會搬
    print(ma, "after merge         ");
    print(mb, "mb (留下衝突)       ");

    // swap / clear
    std::unordered_set<int> sw1{1}, sw2{9,8};
    sw1.swap(sw2);
    print(sw1, "after swap          ");
    sw1.clear();
    print(sw1, "after clear         ");

    // ========================================================================
    //  6. Lookup
    // ========================================================================
    std::unordered_set<int> q{1,3,5,7,9};

    std::cout << "\ncount(3) = " << q.count(3) << '\n';

    auto fi = q.find(7);
    if (fi != q.end()) std::cout << "find(7) → " << *fi << '\n';

    std::cout << "contains(8) = " << q.contains(8) << '\n';  // C++20

    auto [b1, e1] = q.equal_range(5);
    std::cout << "equal_range(5): ";
    for (auto it_ = b1; it_ != e1; ++it_) std::cout << *it_ << ' ';
    std::cout << '\n';

    // ★ 注意:沒有 lower_bound / upper_bound (無序)

    // ========================================================================
    //  7. Bucket interface (★ 獨有,set 沒有的)
    // ========================================================================
    //  bucket_count()        : 目前 bucket 總數
    //  max_bucket_count()    : 上限
    //  bucket_size(n)        : 第 n 個 bucket 的元素數量
    //  bucket(key)           : key 落在哪個 bucket
    //  begin(n) / end(n)     : 走訪第 n 個 bucket 的元素

    std::unordered_set<int> bk{1,2,3,4,5,6,7,8,9,10};
    std::cout << "\n[Bucket] count=" << bk.bucket_count()
              << ", max_bucket_count=" << bk.max_bucket_count() << '\n';
    for (size_t i = 0; i < bk.bucket_count(); ++i) {
        std::cout << "  bucket " << i << " (size=" << bk.bucket_size(i) << "): ";
        for (auto it = bk.begin(i); it != bk.end(i); ++it) std::cout << *it << ' ';
        std::cout << '\n';
    }
    std::cout << "  bucket(5) = " << bk.bucket(5) << '\n';

    // ========================================================================
    //  8. Hash policy (★ unordered 系列獨有)
    // ========================================================================
    //  load_factor()           : size / bucket_count (越高越擁擠)
    //  max_load_factor()       : 觸發 rehash 的閾值,預設 1.0
    //  max_load_factor(f)      : 設定新閾值
    //  rehash(n)               : 強制將 bucket 數設為 ≥ n
    //  reserve(n)              : 預留至少能放 n 個元素的空間 (= rehash(ceil(n/max_load)))

    std::cout << "\nload_factor = " << bk.load_factor() << '\n';
    std::cout << "max_load_factor = " << bk.max_load_factor() << '\n';

    bk.reserve(1000);
    std::cout << "after reserve(1000), bucket_count = " << bk.bucket_count() << '\n';

    bk.rehash(50);
    std::cout << "after rehash(50), bucket_count = " << bk.bucket_count() << '\n';

    // max_load_factor 也可以「設定」 — 把閾值降低 → 更早觸發 rehash → 碰撞少
    bk.max_load_factor(0.5f);
    std::cout << "after max_load_factor(0.5), value = " << bk.max_load_factor() << '\n';

    // ========================================================================
    //  9. Observers
    // ========================================================================
    auto h = q.hash_function();
    std::cout << "\nhash(7) = " << h(7) << '\n';

    auto eq = q.key_eq();
    std::cout << "key_eq(7,7) = " << eq(7,7) << '\n';

    // ========================================================================
    //  10. 自訂型別當 key — 必須提供 hash + equality
    // ========================================================================
    struct Point { int x, y; };
    struct PointHash {
        size_t operator()(const Point& p) const noexcept {
            // 簡單版 hash combine — 實務上請用更好的 (boost::hash_combine 之類)
            return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
        }
    };
    struct PointEq {
        bool operator()(const Point& a, const Point& b) const noexcept {
            return a.x == b.x && a.y == b.y;
        }
    };

    std::unordered_set<Point, PointHash, PointEq> ps{
        {1,2}, {3,4}, {1,2}                // 第二個 {1,2} 會被去重
    };
    std::cout << "\ncustom-key set size = " << ps.size() << '\n';

    // ========================================================================
    //  11. C++20 free functions
    // ========================================================================
    std::unordered_set<int> n1{1,2,3,4,5,6};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(even)      ");

    // ========================================================================
    //  12. 常見陷阱 (Pitfalls) ★必看
    // ========================================================================
    //
    //  (1) Iterator 在 rehash 後失效
    //      插入大量元素時務必小心,可先 reserve() 避免多次 rehash。
    //
    //  (2) 自訂型別當 key 必須提供 hash function
    //      預設只支援基本型別 + std::string + 指標等。
    //      自訂 → 寫個 functor / 特化 std::hash<MyType>。
    //
    //  (3) Hash 函式應該「均勻分布且快速」
    //      壞的 hash → 大量碰撞 → 退化到 O(n)。
    //      永遠不要回傳 0 / 永遠回傳同一值 → 整個 unordered_set 變 linked list。
    //
    //  (4) 元素遍歷順序「沒有保證」
    //      不同編譯器 / 不同次執行可能不同,千萬別依賴。
    //
    //  (5) 沒有 lower_bound / upper_bound / 反向 iterator
    //      hash table 沒有順序概念。需要範圍查詢 → 用 set。
    //
    //  (6) 大量 reserve 不會是萬靈丹
    //      reserve 可避免 rehash,但太大會浪費記憶體。
    //
    //  (7) max_load_factor 設太低 → 浪費空間;設太高 → 碰撞多
    //      預設 1.0 通常是合理選擇。

    // ========================================================================
    //  13. 最佳實踐
    // ========================================================================
    //
    //  • 已知元素數量大概 → 先 reserve(n) 避免多次 rehash
    //  • 自訂 key 一定要寫好 hash function (均勻分布)
    //  • 不在乎順序、需要平均 O(1) → unordered_set 是首選
    //  • 元素數量很小 (< 數十個) → 直接用 vector 線性查找可能更快 (cache)
    //  • 想要「最差 O(log n)」保證或排序 → 用 set
    //  • 與 set 一樣,merge 不會覆寫衝突;extract 可以節點移交

    // ========================================================================
    //  14. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // unordered_set 在面試題的最常見場景: 「O(1) 判斷某元素是否出現過」。
    // 可把 O(n²) 暴力解優化為 O(n) — 是 hash 三大殺手之一 (set/map/array)。

    // ──── LC 217: Contains Duplicate (是否含重複元素) ────
    // 一行解: 把陣列丟進 set,size 變了就有重複。
    {
        auto contains_dup = [](const std::vector<int>& nums) {
            std::unordered_set<int> seen;
            for (int x : nums) {
                if (!seen.insert(x).second) return true;     // insert 回傳 (it, inserted)
            }
            return false;
        };
        std::cout << "\n[LC217 ContainsDup] ";
        std::cout << std::boolalpha << contains_dup({1, 2, 3, 1}) << ' '   // true
                  << contains_dup({1, 2, 3, 4}) << '\n';                   // false
    }

    // ──── LC 128: Longest Consecutive Sequence (最長連續序列) ────
    // 找出未排序陣列中最長的「連續整數序列」長度。
    // 思路: 把所有數丟進 set; 對每個「序列起點」(即 x-1 不在 set 中) 才開始往上延伸,
    //        確保每個元素只被延伸過一次 → 整體 O(n)。
    {
        auto longest = [](std::vector<int> nums) {
            std::unordered_set<int> s(nums.begin(), nums.end());
            int best = 0;
            for (int x : s) {
                if (s.count(x - 1)) continue;     // 不是序列起點
                int len = 1, y = x;
                while (s.count(y + 1)) ++y, ++len;
                best = std::max(best, len);
            }
            return best;
        };
        std::cout << "[LC128 LongestConsec] ";
        std::cout << longest({100, 4, 200, 1, 3, 2}) << '\n';   // 4 (1,2,3,4)
    }

    // ──── LC 202: Happy Number (快樂數) ────
    // 反覆「各位數平方和」直到變成 1 (快樂) 或進入循環 (不快樂)。
    // 用 set 記錄看過的數字,一旦重複出現就代表進入循環。
    {
        auto is_happy = [](int n) {
            auto next_num = [](int x) {
                int s = 0;
                while (x) { s += (x % 10) * (x % 10); x /= 10; }
                return s;
            };
            std::unordered_set<int> seen;
            while (n != 1 && !seen.count(n)) { seen.insert(n); n = next_num(n); }
            return n == 1;
        };
        std::cout << "[LC202 Happy] is_happy(19)=" << is_happy(19)
                  << ", is_happy(2)=" << is_happy(2) << '\n';
        // 預期輸出: is_happy(19)=1, is_happy(2)=0
    }

    // ──── LC 349: Intersection of Two Arrays (兩陣列交集) ────
    // 給兩個陣列,回傳「不重複」的交集元素。
    // 思路:把 nums1 全部丟進 unordered_set,然後走訪 nums2,在 set 中存在的就加入結果。
    // 為何用 unordered_set:O(1) 平均查找,整體 O(n+m);若用 sort + 雙指針則為 O(n log n)。
    {
        std::vector<int> nums1{4, 9, 5};
        std::vector<int> nums2{9, 4, 9, 8, 4};
        std::unordered_set<int> s1(nums1.begin(), nums1.end());
        std::unordered_set<int> ans;
        for (int x : nums2) if (s1.count(x)) ans.insert(x);
        std::cout << "[LC349 Intersect] = [ ";
        for (int x : ans) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 4 9 ] (順序不保證)
    }

    // ========================================================================
    //  15. 實戰範例:URL 去重 (Web Crawler Visited Set)
    // ========================================================================
    // 真實場景:網路爬蟲必須避免重複抓取同一頁面,需要一個「已訪問 URL 集合」:
    //   • insert / contains 都 O(1) 平均 — 否則千萬筆 URL 會慢到爆
    //   • 不需要排序 → 直接用 unordered_set,比 set 快 3~5 倍
    //   • 已知資料量先 reserve,減少 rehash
    {
        std::unordered_set<std::string> visited;
        visited.reserve(10000);

        std::vector<std::string> incoming{
            "https://a.com/p1",
            "https://a.com/p2",
            "https://a.com/p1",   // 重複
            "https://b.com/x",
            "https://a.com/p2"    // 重複
        };
        int new_count = 0;
        for (auto& url : incoming) {
            if (visited.insert(url).second) {
                ++new_count;
                std::cout << "  抓取: " << url << '\n';
            } else {
                std::cout << "  跳過 (已抓): " << url << '\n';
            }
        }
        std::cout << "[Web Crawler] 新抓取 " << new_count
                  << " 筆,集合內共 " << visited.size() << " 個 URL\n";
        // 預期輸出: 新抓取 3 筆,集合共 3 個 URL
    }

    std::cout << "\n=== unordered_set demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unordered_set 與 set 的選擇原則?
    //    A：只判斷「存在 / 不存在」、要求 O(1) 平均查找就用 unordered_set;
    //       需要排序走訪、區間查詢 (lower_bound/upper_bound) 就用 set (O(log n))。
    //       LC128 最長連續序列、LC202 快樂數這類純存在性判斷,unordered_set 最合適。
    //
    //  Q2：unordered_set 的 iterator 在 insert / rehash 後是否失效?
    //    A：insert 若觸發 rehash,所有 iterator 失效;沒觸發 rehash 則仍有效。
    //       reference / pointer 在 rehash 後仍然有效 (元素物件本身不會被搬移,
    //       只是 bucket 重新排列)。預先 reserve(n) 可避免 rehash。
    //
    //  Q3：為何不能用「pair<int,int>」直接當 unordered_set 的元素?
    //    A:標準函式庫沒有為 std::pair 提供 std::hash 特化。要自己寫,例如:
    //       struct PH { size_t operator()(pair<int,int> const& p) const noexcept {
    //         return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1); } };
    //       std::unordered_set<pair<int,int>, PH> s;
    //
    return 0;
}

/*
============================================================================
  附錄:std::unordered_set 完整 member function 一覽
============================================================================
  Constructors / destructor / operator= / get_allocator

  Iterators:       begin/end/cbegin/cend  (沒有 rbegin / rend)
  Capacity:        empty, size, max_size

  Modifiers:       clear, insert, insert_range (C++23), emplace, emplace_hint,
                   erase, swap, extract (C++17), merge (C++17)

  Lookup:          count, find, contains (C++20), equal_range
                   (沒有 lower_bound / upper_bound)

  Bucket interface:
      begin(n), end(n), cbegin(n), cend(n),
      bucket_count, max_bucket_count, bucket_size, bucket

  Hash policy:
      load_factor, max_load_factor, rehash, reserve

  Observers:       hash_function, key_eq

  Non-member:      operator==, !=, std::swap, std::erase_if (C++20)
============================================================================
*/
