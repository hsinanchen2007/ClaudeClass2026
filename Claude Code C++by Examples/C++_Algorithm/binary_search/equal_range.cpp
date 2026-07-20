// ============================================================
// std::equal_range
// 分類 (Category): Binary search operations (二分搜尋,需已排序)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/equal_range
//   * https://cplusplus.com/reference/algorithm/equal_range/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::equal_range 解的問題:
//
//   「在已排序的範圍中,『等於 value』的所有元素構成的子範圍是哪一段?」
//
// 它一次性回傳一個 pair { lo, hi }:
//
//   lo = lower_bound(first, last, value)
//   hi = upper_bound(first, last, value)
//
// 等於 value 的元素就是 [lo, hi) — 個數 = hi - lo。
// 若沒有任何匹配,lo == hi (兩者皆指向「應插入的位置」)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼比「分別呼叫 lower + upper」好?                  │
// └────────────────────────────────────────────────────────────┘
//
// 雖然兩種寫法結果相同,但 equal_range 在實作上可以「共用搜尋路徑」 —
// 第一次走訪到 value 的位置後,可同時往左找 lower、往右找 upper,
// 比兩次獨立 binary search 快 (常數因子小)。
//
// 對於只想知道「個數」或「子範圍」的場景,直接用 equal_range 更乾淨。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * LC 34:在排序陣列中找元素的第一個與最後一個位置 — 一行解決。
//   * 計算「multiset 中某 value 出現幾次」 — hi - lo。
//   * 在「按時戳排序的事件 log」中,撈出「同一時戳的所有事件」。
//   * 在「按 key 排序的物件 vector」中,撈出「同一 key 的所有物件」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   std::pair<FwdIt, FwdIt>
//   equal_range(FwdIt first, FwdIt last, const T& value);
//
//   template <class FwdIt, class T, class Compare>
//   std::pair<FwdIt, FwdIt>
//   equal_range(FwdIt first, FwdIt last, const T& value, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(log N) 比較
//   空間: O(1)
//   需求: 範圍依 comp 已排序;否則 UB
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 範圍必須已排序!
//   2. 子範圍長度 = hi - lo;== 0 表示「找不到」。
//   3. comp 必須與排序時相同。
//   4. 對 multiset / multimap,改用容器自帶的 .equal_range() — 它走樹結構,常數更小。
//
// ============================================================

/*
補充筆記：std::equal_range
  - equal_range 一次回傳 lower_bound 與 upper_bound，適合處理重複 key 的範圍。
  - 回傳的 pair 可能是兩個相同 iterator，代表沒有找到但仍給出插入位置。
  - 在 multiset、multimap 或排序 vector 中，equal_range 是掃描同 key 群組的標準入口。
  - std::equal_range 屬於二分搜尋家族，前提是輸入範圍已依同一個比較規則排序；未排序資料上使用結果沒有意義。
  - 這類函式通常回傳位置或範圍，不一定回傳 bool；拿到 iterator 後要先檢查是否等於 end()。
  - lower_bound 找第一個不小於 value 的位置，upper_bound 找第一個大於 value 的位置，兩者相減可得到重複值數量。
  - binary_search 只回答是否存在；若後續需要插入位置、索引或重複區間，lower_bound/equal_range 更實用。
  - 比較器必須和排序時使用的規則一致；用不同規則搜尋同一批資料，結果會像資料沒排序一樣不可信。
  - 在 vector 上 iterator 相減可得到索引，在 list 上不行；iterator category 會影響你能不能做 O(1) 距離計算。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::equal_range
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. equal_range 和 lower_bound / upper_bound 的關係?
//     答:equal_range 回傳 std::pair<It, It>,定義上就等於
//         { lower_bound(first, last, v), upper_bound(first, last, v) } —
//         也就是所有「等價於 v」的元素構成的半開區間 [lo, hi)。
//         個數 = hi - lo;lo == hi 代表沒有任何匹配。
//         三者共用同一個前置條件:區間必須依同一 comp 已排序,否則 UB。
//     追問:那分開呼叫 lower_bound 和 upper_bound 有什麼差?
//           (答:結果相同,但 equal_range 可共用一次搜尋路徑,常數因子較小;
//            兩者比較次數都是 O(log N) 等級)
//
// 🔥 Q2. 判斷「有沒有找到」該怎麼寫?
//     答:比較 lo != hi(區間非空)即可,不需要跟 end() 比。
//         沒找到時 lo == hi,而且這個共同位置仍然有意義 — 它就是
//         「插入 v 後仍保持有序」的插入點,可以直接拿來做 insert。
//     追問:為什麼不能只檢查 lo != end()?
//           (答:v 不存在但小於某些元素時,lo 會指向下一個更大的元素而非 end())
//
// ⚠️ 陷阱. 對 std::multiset / std::multimap 也用 std::equal_range 嗎?
//     答:不要,改用同名成員函式 multiset::equal_range()。自由函式版的比較
//         次數雖是 O(log N),但這些容器的 iterator 只是 bidirectional,
//         跳中點得逐步 ++,iterator 前進總步數是 O(N),整體退化成 O(N);
//         成員版沿紅黑樹下降才是真正的 O(log N)。
//     為什麼會錯:「二分搜尋 = O(log n)」被記成了無條件成立的事實,
//         但它成立的前提是能 O(1) 定址中點。另外成員版還多一項好處:
//         C++14 起搭配透明比較器(如 std::less<>)可做異質鍵查找,
//         自由函式版沒有這個能力。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 3, 3, 4, 5, 5, 7};

    // --- 範例 1: 找出所有 3 ---
    auto [lo, hi] = std::equal_range(v.begin(), v.end(), 3);
    std::cout << "3 found at indices [" << (lo - v.begin())
              << ", " << (hi - v.begin()) << "), count="
              << (hi - lo) << '\n';
    std::cout << "elements: ";
    for (auto it = lo; it != hi; ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // --- 範例 2: 找不到 (lo == hi → 表示沒有,且兩者都指向插入點) ---
    auto p = std::equal_range(v.begin(), v.end(), 6);
    std::cout << "6 found? " << std::boolalpha << (p.first != p.second) << '\n';
    std::cout << "  insertion point at index " << (p.first - v.begin()) << '\n';

    // --- 範例 3: 對結構體用自訂 comp (依 key 排序) ---
    struct Item { int key; std::string name; };
    std::vector<Item> items{
        {1, "a"}, {2, "b"}, {2, "c"}, {2, "d"}, {3, "e"}
    };
    auto cmp = [](const Item& x, const Item& y){ return x.key < y.key; };
    Item probe{2, ""};
    auto r = std::equal_range(items.begin(), items.end(), probe, cmp);
    std::cout << "items with key=2:";
    for (auto it = r.first; it != r.second; ++it)
        std::cout << " " << it->name;
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_34_search_range_via_equal_range();
    void leetcode_350_intersection_via_equal_range();
    void practical_log_query_by_timestamp();
    void leetcode_2070_richest_within_budget();
    void practical_product_search_by_category();
    leetcode_34_search_range_via_equal_range();
    leetcode_350_intersection_via_equal_range();
    practical_log_query_by_timestamp();
    leetcode_2070_richest_within_budget();
    practical_product_search_by_category();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 34: 在排序陣列中查詢元素的第一個與最後一個位置
// ----------------------------------------------------------------
// 題目:給已排序陣列 nums 與 target,回傳 [first, last]。不存在 → [-1, -1]。
//
// 為什麼用 std::equal_range:
//   一次取 (lower, upper),不用呼叫兩次。lo == hi → 不存在。
//   lo - begin = first index;hi - begin - 1 = last index。
//
// 複雜度:時間 O(log n);空間 O(1)。
void leetcode_34_search_range_via_equal_range() {
    std::vector<int> nums{5, 7, 7, 8, 8, 10};
    int target = 8;
    auto [lo, hi] = std::equal_range(nums.begin(), nums.end(), target);
    int first = -1, last = -1;
    if (lo != hi) {
        first = static_cast<int>(lo - nums.begin());
        last  = static_cast<int>(hi - nums.begin()) - 1;
    }
    std::cout << "LC34: [" << first << ',' << last << "]\n";
    std::cout << "LC34 count(8): " << (hi - lo) << '\n';

    auto [lo2, hi2] = std::equal_range(nums.begin(), nums.end(), 6);
    int f2 = -1, l2 = -1;
    if (lo2 != hi2) {
        f2 = static_cast<int>(lo2 - nums.begin());
        l2 = static_cast<int>(hi2 - nums.begin()) - 1;
    }
    std::cout << "LC34(miss): [" << f2 << ',' << l2 << "]\n";
}

// ----------------------------------------------------------------
// LeetCode 350: 兩個陣列的交集 II (Intersection of Two Arrays II)
// ----------------------------------------------------------------
// 題目:給兩陣列 nums1, nums2,回傳兩者交集 (含重複次數,以 min(出現次數) 計)。
//
// 為什麼用 std::equal_range:
//   把 nums2 排序後,對 nums1 中每個 x 用 equal_range 找 x 在 nums2 中的範圍。
//   * 範圍非空 → x 是交集元素之一,同時把該 x 從 nums2 移除一個 (避免重複配對)。
//
// 複雜度:時間 O((n + m) log m);空間 O(m)。
void leetcode_350_intersection_via_equal_range() {
    std::vector<int> nums1{4, 9, 5};
    std::vector<int> nums2{9, 4, 9, 8, 4};
    std::sort(nums2.begin(), nums2.end());
    std::vector<int> ans;
    for (int x : nums1) {
        auto [lo, hi] = std::equal_range(nums2.begin(), nums2.end(), x);
        if (lo != hi) {
            ans.push_back(x);
            nums2.erase(lo);   // 消耗一個配對
        }
    }
    std::sort(ans.begin(), ans.end());
    std::cout << "LC350:";
    for (int v : ans) std::cout << ' ' << v;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:在排好的事件 log 中撈出「同一時戳」的所有事件
// ----------------------------------------------------------------
// 場景:事件 log 已依時戳排序,要查詢時戳 == t 的所有事件 (例如同一秒
//      內發生的多筆 log,做關聯分析)。
//
// 為什麼用 std::equal_range:
//   對結構陣列 + 自訂 comp (只比 timestamp),一次取出整段「同 ts」的事件。
void practical_log_query_by_timestamp() {
    struct Event { long long ts; std::string msg; };
    std::vector<Event> log{
        {100, "boot"},
        {120, "login(alice)"},
        {120, "login(bob)"},
        {120, "login(carol)"},
        {130, "tick"},
        {200, "shutdown"},
    };
    auto cmp = [](const Event& a, const Event& b){ return a.ts < b.ts; };
    Event probe{120, ""};
    auto [lo, hi] = std::equal_range(log.begin(), log.end(), probe, cmp);
    std::cout << "events at ts=120 (count=" << (hi - lo) << "):\n";
    for (auto it = lo; it != hi; ++it)
        std::cout << "  " << it->msg << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2070 (簡化版): 預算內最大美麗值
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:給多個 query 預算 budget 與商品 (price, beauty) 清單,
//          回傳預算內可買到的最高 beauty 值。
//
// 為什麼用 std::equal_range / upper_bound 概念:
//   * 排序商品依價格;對每個 query budget,要找「price <= budget」的最後一個。
//   * 用 upper_bound(budget) 拿到「第一個 > budget」的位置;前面這段就是可買的。
//   * 預先把「前綴最大 beauty」存好,直接查表即可。
//   * 也可以理解為 equal_range 的特例 (邊界查詢的應用)。
//
// 複雜度:時間 O(n log n + q log n);空間 O(n)。
void leetcode_2070_richest_within_budget() {
    struct Item { int price; int beauty; };
    std::vector<Item> items{{1,2},{3,2},{2,4},{5,6},{3,5}};
    std::sort(items.begin(), items.end(),
              [](const Item& a, const Item& b){ return a.price < b.price; });
    // 預先計算「價格 <= prices[i]」的前綴最大 beauty
    std::vector<int> prices, max_beauty;
    int cur_max = 0;
    for (const auto& it : items) {
        prices.push_back(it.price);
        cur_max = std::max(cur_max, it.beauty);
        max_beauty.push_back(cur_max);
    }
    std::vector<int> queries{1, 2, 3, 4, 5};
    std::cout << "LC2070:";
    for (int q : queries) {
        auto up = std::upper_bound(prices.begin(), prices.end(), q);
        if (up == prices.begin()) std::cout << " 0";
        else std::cout << " " << max_beauty[up - prices.begin() - 1];
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:依分類欄位查詢所有商品
// ----------------------------------------------------------------
// 場景:商品已依 category_id 排序,要快速取出「某分類底下的所有商品」 —
//      typical 的 multimap / database GROUP BY 模式,可用 equal_range 一次取段。
void practical_product_search_by_category() {
    struct Product { int cat; std::string name; };
    std::vector<Product> products{
        {1, "蘋果"}, {1, "香蕉"}, {2, "毛巾"}, {2, "肥皂"}, {2, "牙刷"}, {3, "鉛筆"}
    };
    auto cmp = [](const Product& a, const Product& b){ return a.cat < b.cat; };
    Product probe{2, ""};
    auto [lo, hi] = std::equal_range(products.begin(), products.end(), probe, cmp);
    std::cout << "category 2 products (count=" << (hi - lo) << "):";
    for (auto it = lo; it != hi; ++it) std::cout << " " << it->name;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// 3 found at indices [2, 5), count=3
// elements: 3 3 3
// 6 found? false
//   insertion point at index 8
// items with key=2: b c d
// LC34: [3,4]
// LC34 count(8): 2
// LC34(miss): [-1,-1]
// LC350: 4 9
// events at ts=120 (count=3):
//   login(alice)
//   login(bob)
//   login(carol)
// LC2070: 2 4 5 5 6
// category 2 products (count=3): 毛巾 肥皂 牙刷
