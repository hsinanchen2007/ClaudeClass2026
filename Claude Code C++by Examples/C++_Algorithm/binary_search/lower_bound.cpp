// ============================================================
// std::lower_bound
// 分類 (Category): Binary search operations (二分搜尋,需已排序)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/lower_bound
//   * https://cplusplus.com/reference/algorithm/lower_bound/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::lower_bound 解的問題:
//
//   「在已排序的範圍中,找『第一個不小於 value』的位置。」
//
// 即 *it >= value 的最小 it。換句話說,這是「value 應該插入的位置」 —
// 把 value 插在這裡,範圍仍然保持排序。
//
// 範例:在 {1, 3, 3, 5, 7, 9} 中:
//
//   lower_bound(4) → 指向 5    (第一個 >= 4)
//   lower_bound(3) → 指向第一個 3  (第一個 >= 3,跳過任何 < 3)
//   lower_bound(0) → 指向 1   (第一個 >= 0)
//   lower_bound(99) → end()   (沒有任何 >= 99)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、lower_bound 是「插入點」                                │
// └────────────────────────────────────────────────────────────┘
//
// 「插入點」是 lower_bound 最有用的應用 — 想維護一個「動態排序的 vector」
// 時,每次新元素 x 進來:
//
//   auto pos = std::lower_bound(v.begin(), v.end(), x);
//   v.insert(pos, x);   // 找到位置 + 插入,vector 仍保持排序
//
// 這比每次 push_back 後 sort 整個容器快得多 (但 insert 仍是 O(N) 搬移)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、lower_bound vs upper_bound vs equal_range              │
// └────────────────────────────────────────────────────────────┘
//
//   ┌────────────────┬─────────────────────────────────────┐
//   │ lower_bound    │ 第一個 >= value 的位置                │
//   │ upper_bound    │ 第一個 >  value 的位置                │
//   │ equal_range    │ 一次取 (lower, upper),回傳 pair       │
//   └────────────────┴─────────────────────────────────────┘
//
// 這三者是「二分搜尋三劍客」,搭配使用可解決所有「在排序範圍找位置/個數」的問題:
//
//   * 想找「第一個 == value 的位置」 → lower_bound + 驗證 *it == value
//   * 想找「最後一個 == value 的位置」 → upper_bound - 1
//   * 想知道「value 出現了幾次」 → upper_bound - lower_bound
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   FwdIt lower_bound(FwdIt first, FwdIt last, const T& value);
//
//   template <class FwdIt, class T, class Compare>
//   FwdIt lower_bound(FwdIt first, FwdIt last, const T& value, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間:
//     RandomAccessIt → 真 O(log N)
//     ForwardIt      → O(log N) 比較,但步進總計 O(N)
//   空間: O(1)
//
//   需求:範圍依 comp 已排序;否則 UB。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 範圍必須已排序!否則 UB。
//   2. 「不小於」是 >= 不是 > — 別跟 upper_bound 搞混。
//   3. 對 set / map 應用容器自帶的 .lower_bound() — 不必走步進。
//   4. comp 必須與排序時相同。
//   5. lower_bound 配合「自定 comp」可做「在已排序的物件序列中依某欄位查詢」。
//
// ============================================================

/*
補充筆記：std::lower_bound
  - lower_bound 回傳第一個「不小於 value」的位置，這個位置通常就是保持排序時的插入點。
  - 回傳 end 是合法結果，代表所有元素都小於 value；在解參考 iterator 前必須先檢查。
  - 自訂比較器要和排序時使用的比較器一致，否則二分搜尋會在錯誤的邊界上停止。
  - std::lower_bound 屬於二分搜尋家族，前提是輸入範圍已依同一個比較規則排序；未排序資料上使用結果沒有意義。
  - 這類函式通常回傳位置或範圍，不一定回傳 bool；拿到 iterator 後要先檢查是否等於 end()。
  - lower_bound 找第一個不小於 value 的位置，upper_bound 找第一個大於 value 的位置，兩者相減可得到重複值數量。
  - binary_search 只回答是否存在；若後續需要插入位置、索引或重複區間，lower_bound/equal_range 更實用。
  - 比較器必須和排序時使用的規則一致；用不同規則搜尋同一批資料，結果會像資料沒排序一樣不可信。
  - 在 vector 上 iterator 相減可得到索引，在 list 上不行；iterator category 會影響你能不能做 O(1) 距離計算。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::lower_bound
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lower_bound、upper_bound、equal_range 的差別?
//     答:三者都要求區間已依同一準則排序(嚴格說是對該述詞已 partitioned)。
//         lower_bound 回傳第一個 >= value 的位置;upper_bound 回傳第一個 > value
//         的位置;equal_range 一次回傳 {lower_bound, upper_bound}。
//         「等於 value 的元素個數」= upper_bound - lower_bound。
//         lower_bound 的位置同時也是「插入 value 後仍保持有序」的最左插入點。
//     追問:找不到 value 時 lower_bound 回傳什麼?
//           (答:第一個大於 value 的位置,或 last — 見下方陷阱)
//
// 🔥 Q2. 複雜度是 O(log N) 嗎?對 std::list 呢?
//     答:要分「比較次數」與「總時間」。比較次數恆為 O(log N)。
//         但只有 random access iterator 能 O(1) 跳到中點,總時間才是 O(log N);
//         對 std::list 這種 bidirectional iterator,跳中點要一步步 ++,
//         iterator 前進的總步數是 O(N),總時間因此退化成 O(N)。
//     追問:那 lower_bound 最低需要哪種 iterator?
//           (答:LegacyForwardIterator 即可,所以 forward_list 也能編譯過,只是慢)
//
// ⚠️ 陷阱1. 為什麼不該對 std::set 用 std::lower_bound?
//     答:因為通用演算法版本會退化。std::lower_bound 的比較次數雖是 O(log N),
//         但 set 的 iterator 只是 bidirectional,每次「跳到中點」都得一步步走,
//         iterator 前進總次數是 O(N);而成員函式 set::lower_bound() 直接沿紅黑樹
//         下降,是真正的 O(log N)。通則:關聯容器一律優先用同名成員函式。
//     為什麼會錯:大家記得的是「二分搜尋 = O(log n)」,卻忘了那句話成立的前提
//         是「能 O(1) 定址中點」。同理 std::find 對 set 是 O(n)、set::find() 才是
//         O(log n);std::count 對 set 也是同樣的退化。
//
// ⚠️ 陷阱2. it != end() 就代表找到了嗎?
//     答:不代表。lower_bound 回傳的是「第一個不小於 value」的位置,value
//         不存在時它會指向下一個更大的元素 — 這是合法且有意義的插入點,
//         不是 end()。必須額外檢查:it != v.end() && *it == value。
//     為什麼會錯:大家把它類比成 std::find / map::find 的「找不到就回 end()」
//         慣例,但 lower_bound 的語意是「邊界」不是「查詢」,只有在 value
//         大於所有元素時才會回 end()。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 3, 3, 5, 7, 9, 9, 11};

    // --- 範例 1: 找 5 的下界 (>= 5 的第一個位置) ---
    auto it = std::lower_bound(v.begin(), v.end(), 5);
    std::cout << "lower_bound(5) at index " << (it - v.begin())
              << ", val=" << *it << '\n';

    // --- 範例 2: 找 4 (不存在) → 指向 5 (插入點) ---
    auto it2 = std::lower_bound(v.begin(), v.end(), 4);
    std::cout << "lower_bound(4) at index " << (it2 - v.begin())
              << ", val=" << *it2 << "  (插入點)\n";

    // --- 範例 3: 找 100 (大於所有元素) → 回傳 end ---
    auto it3 = std::lower_bound(v.begin(), v.end(), 100);
    std::cout << "lower_bound(100) is end? "
              << std::boolalpha << (it3 == v.end()) << '\n';

    // --- 範例 4: 維護插入排序 — 插入 6 ---
    auto pos = std::lower_bound(v.begin(), v.end(), 6);
    v.insert(pos, 6);
    std::cout << "after insert 6: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 5: 自訂比較 (降序排列陣列,自訂 comp) ---
    std::vector<int> desc{9, 7, 5, 3, 1};
    auto d = std::lower_bound(desc.begin(), desc.end(), 4,
                              [](int a, int b){ return a > b; });
    std::cout << "lower_bound in desc(4) at index "
              << (d - desc.begin()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_35_search_insert_position();
    void leetcode_300_longest_increasing_subsequence();
    void practical_online_sorted_insert();
    void leetcode_1011_capacity_to_ship();
    void practical_level_threshold_lookup();
    leetcode_35_search_insert_position();
    leetcode_300_longest_increasing_subsequence();
    practical_online_sorted_insert();
    leetcode_1011_capacity_to_ship();
    practical_level_threshold_lookup();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 35: 搜尋插入位置 (Search Insert Position)
// ----------------------------------------------------------------
// 題目:給已排序整數陣列 nums (無重複) 與 target,
//      若存在則回傳其索引;否則回傳「應插入位置」。
//
// 為什麼用 std::lower_bound:
//   題意 100% 等於 lower_bound 的定義 — 「第一個 >= target 的位置」。
//   一行解決,O(log n)。
//
// 複雜度:時間 O(log n);空間 O(1)。
void leetcode_35_search_insert_position() {
    std::vector<int> nums{1, 3, 5, 6};
    auto it = std::lower_bound(nums.begin(), nums.end(), 5);
    std::cout << "LC35(5): " << (it - nums.begin()) << '\n';
    auto it2 = std::lower_bound(nums.begin(), nums.end(), 0);
    std::cout << "LC35(0): " << (it2 - nums.begin()) << '\n';
    auto it3 = std::lower_bound(nums.begin(), nums.end(), 7);
    std::cout << "LC35(7): " << (it3 - nums.begin()) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 300: 最長遞增子序列 (Longest Increasing Subsequence)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,回傳其最長嚴格遞增子序列的長度。
//
// 為什麼用 std::lower_bound:
//   經典「耐心排序 (patience sorting)」解法 —
//   維護 tails[i] = 「長度為 i+1 的遞增子序列目前最小的尾元素」。
//   對每個 x:
//     * 用 lower_bound 在 tails 找第一個 >= x 的位置。
//     * 找到 → 取代 (用較小的 x 改善該長度的尾)。
//     * 找不到 (== end) → append (產生更長的子序列)。
//   tails 始終嚴格遞增,所以可以 lower_bound (才 O(log n))。
//   tails.size() 即為答案。
//
// 複雜度:時間 O(n log n);空間 O(n)。
void leetcode_300_longest_increasing_subsequence() {
    std::vector<int> nums{10, 9, 2, 5, 3, 7, 101, 18};
    std::vector<int> tails;
    for (int x : nums) {
        auto it = std::lower_bound(tails.begin(), tails.end(), x);
        if (it == tails.end()) tails.push_back(x);
        else                   *it = x;
    }
    std::cout << "LC300: " << tails.size() << '\n';
}

// ----------------------------------------------------------------
// 實務範例:online insertion (動態維護排序 vector)
// ----------------------------------------------------------------
// 場景:串流資料逐筆進來,要動態維護一個已排序陣列方便查詢。
//
// 為什麼用 std::lower_bound:
//   每筆資料用 lower_bound 找插入位置,再 vector::insert。
//   找位置 O(log n),搬移 O(n),總體 O(n) per insert — 對小規模適用。
void practical_online_sorted_insert() {
    std::vector<int> sorted;
    std::vector<int> stream{4, 1, 7, 2, 5, 3};
    for (int x : stream) {
        auto pos = std::lower_bound(sorted.begin(), sorted.end(), x);
        sorted.insert(pos, x);
    }
    std::cout << "online sorted:";
    for (int v : sorted) std::cout << ' ' << v;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1011: 在 D 天內送達包裹的能力 (Capacity To Ship Packages Within D Days)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給包裹重量 weights 與天數 days,求最小船的「載重容量」,
//      使得依序裝箱可以在 days 天內全部送完。
//
// 為什麼用 std::lower_bound (二分答案):
//   把「答案空間」視為一條已排序的數值線:[max_weight, sum_weights]。
//   對任一容量 cap,可以 O(n) 模擬計算「需要幾天」。
//   「需要的天數 <= days」是單調遞減的 (cap 越大、天數越小) — 滿足二分前提。
//   我們要找的是「第一個讓 needed_days(cap) <= days 成立的 cap」 → lower_bound 風格。
//
// 解法步驟:
//   1. 構造索引序列 [max_weight, sum_weights]。
//   2. 對其用 lower_bound + 自訂 predicate 找答案 (透過 lambda 中模擬天數)。
//
// 複雜度:時間 O(n log(sum));空間 O(1)。
void leetcode_1011_capacity_to_ship() {
    std::vector<int> weights{1,2,3,4,5,6,7,8,9,10};
    int days = 5;
    int lo_cap = *std::max_element(weights.begin(), weights.end());
    int hi_cap = 0;
    for (int w : weights) hi_cap += w;

    // 模擬:給定容量 cap,計算需要幾天
    auto need_days = [&](int cap) {
        int d = 1, cur = 0;
        for (int w : weights) {
            if (cur + w > cap) { ++d; cur = 0; }
            cur += w;
        }
        return d;
    };

    // 構造容量候選 [lo_cap, hi_cap],二分找第一個滿足 need_days(cap) <= days 的 cap
    int lo = lo_cap, hi = hi_cap + 1;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (need_days(mid) <= days) hi = mid;
        else lo = mid + 1;
    }
    std::cout << "LC1011: " << lo << '\n';
}

// ----------------------------------------------------------------
// 實務範例:遊戲等級門檻查詢 (依經驗值決定等級)
// ----------------------------------------------------------------
// 場景:遊戲設計用「等級經驗門檻陣列」決定玩家現在的等級。
//      例如 thresholds = [0, 100, 300, 600, 1000, 1500] 代表
//      lv1=[0,100), lv2=[100,300), ..., lv6=[1500, ∞)。
//      給玩家經驗值 xp,要快速算出等級。
//
// 為什麼用 std::lower_bound:
//   找「第一個 > xp 的門檻索引」(其實就是 upper_bound) 即可,
//   但這裡示範 lower_bound 的另一個用法:找「第一個 >= xp+1」也等價,
//   或更直觀的:用 upper_bound 取得 idx,等級 = idx (因為 1-based)。
void practical_level_threshold_lookup() {
    std::vector<int> thresholds{0, 100, 300, 600, 1000, 1500};
    std::vector<int> players_xp{50, 100, 250, 800, 1500, 9999};
    for (int xp : players_xp) {
        // 用 upper_bound 找第一個 > xp 的位置;等級 = 位置 (1-based)
        auto it = std::upper_bound(thresholds.begin(), thresholds.end(), xp);
        int level = static_cast<int>(it - thresholds.begin());
        std::cout << "xp=" << xp << " level=" << level << '\n';
    }
}

// === 預期輸出 (Expected output) ===
// lower_bound(5) at index 3, val=5
// lower_bound(4) at index 3, val=5  (插入點)
// lower_bound(100) is end? true
// after insert 6: 1 3 3 5 6 7 9 9 11
// lower_bound in desc(4) at index 3
// LC35(5): 2
// LC35(0): 0
// LC35(7): 4
// LC300: 4
// online sorted: 1 2 3 4 5 7
// LC1011: 15
// xp=50 level=1
// xp=100 level=2
// xp=250 level=2
// xp=800 level=4
// xp=1500 level=6
// xp=9999 level=6
