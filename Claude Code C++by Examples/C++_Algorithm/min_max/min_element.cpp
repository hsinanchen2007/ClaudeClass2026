// ============================================================
// std::min_element
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/min_element
//   * https://cplusplus.com/reference/algorithm/min_element/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::min_element 解的問題:
//
//   「在這段範圍中,最小的元素在哪裡?」
//
// 它跟 std::min 的關鍵差別:
//
//   * std::min(a, b)              → 兩個值的較小者 (回傳「值」)
//   * std::min({a, b, c, ...})    → ilist 中的最小值 (回傳「值」)
//   * std::min_element(first, last) → 「容器範圍」中最小元素的「迭代器」
//
// min_element 拿到迭代器後,可以:
//   * *it 拿值
//   * it - begin 算索引
//   * it->member 拿成員
//   * 直接 erase(it) 移除最小元素
//
// 「迭代器」比「值」資訊豐富,適合需要進一步操作的場景。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、邊界條件                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 空範圍 → 回傳 last (要先檢查 it != last 才能解參考)。
//   * 多個元素都是最小 → 回傳「第一個」位置 (穩定)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、自訂 comp 的常見用法                                   │
// └────────────────────────────────────────────────────────────┘
//
//   * 找「絕對值最小」: comp = [](a, b){ return abs(a) < abs(b); }
//   * 對結構體找「某欄位最小」: comp = [](a, b){ return a.score < b.score; }
//   * 找「字串最短」: comp = [](a, b){ return a.size() < b.size(); }
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   FwdIt min_element(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class Compare>
//   FwdIt min_element(FwdIt first, FwdIt last, Compare comp);
//
//   * C++14 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 N - 1 次比較 — O(N)
//   空間: O(1)
//
//   1. 空範圍回傳 last,記得先檢查再解參考。
//   2. 多個最小取「第一個」(穩定)。
//   3. 同時要 min/max 用 std::minmax_element 一次掃描。
//   4. 找「值」就好可用 std::min({...}),不必走 min_element。
//
// ============================================================

/*
補充筆記：std::min_element
  - min_element 回傳最小元素的位置，不是最小值本身；這讓你可以修改或取得索引。
  - 空範圍會回傳 last，解參考前一定要判斷。
  - 比較器若只比較部分欄位，回傳的是該欄位最小的第一個元素，tie 的處理要自己定義。
  - std::min_element 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};

    // --- 範例 1: 找最小元素 ---
    auto it = std::min_element(v.begin(), v.end());
    std::cout << "min = " << *it
              << " at index " << (it - v.begin()) << '\n';

    // --- 範例 2: 自訂 comp — 比絕對值 ---
    std::vector<int> w{-7, 3, -1, 5, 4};
    auto it2 = std::min_element(w.begin(), w.end(),
                                [](int a, int b){ return std::abs(a) < std::abs(b); });
    std::cout << "min by |.| = " << *it2 << '\n';

    // --- 範例 3: 對結構體找最低分 ---
    struct Score { std::string name; int val; };
    std::vector<Score> s{{"A", 80}, {"B", 65}, {"C", 90}, {"D", 65}};
    auto it3 = std::min_element(s.begin(), s.end(),
                                [](const Score& a, const Score& b){ return a.val < b.val; });
    std::cout << "lowest score: " << it3->name << "(" << it3->val << ")\n";

    // --- 範例 4: 空範圍 → last ---
    std::vector<int> e;
    std::cout << "empty min_element == end: "
              << std::boolalpha
              << (std::min_element(e.begin(), e.end()) == e.end()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1051_height_checker_min();
    void leetcode_1431_kids_least_candies();
    void practical_worst_score();
    void leetcode_2535_array_element_diff();
    void practical_cheapest_route();
    leetcode_1051_height_checker_min();
    leetcode_1431_kids_least_candies();
    practical_worst_score();
    leetcode_2535_array_element_diff();
    practical_cheapest_route();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1051: 高度檢查器 (Height Checker) — 找最矮者位置
// ----------------------------------------------------------------
// 題目:給高度陣列,找出「最矮的人」的位置 (高度與索引)。
//
// 為什麼用 std::min_element:
//   題目要的就是「最小元素的位置」 — 一行解決。
//   回傳迭代器後可同時拿到值 (*it) 與索引 (it - begin)。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1051_height_checker_min() {
    std::vector<int> heights{1, 1, 4, 2, 1, 3};
    auto it = std::min_element(heights.begin(), heights.end());
    std::cout << "LC1051: shortest height = " << *it
              << " at index " << (it - heights.begin()) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1431 反向版:擁有「最少」糖果的小孩
// ----------------------------------------------------------------
// 題目反向:LC 1431 原題找最多糖果者;這裡示範對偶概念 — 找最少者的索引。
void leetcode_1431_kids_least_candies() {
    std::vector<int> candies{2, 3, 5, 1, 3};
    auto it = std::min_element(candies.begin(), candies.end());
    std::cout << "LC1431-min: kid with least candies = "
              << *it << " at index " << (it - candies.begin()) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:成績表中找「最低分學生」(用於補救教學)
// ----------------------------------------------------------------
// 場景:老師需要找出「最低分學生」(姓名 + 分數),
//      安排補救教學或一對一輔導。
//
// 為什麼用 std::min_element:
//   回傳迭代器,可同時拿姓名與分數 — 比把兩件事拆開做更乾淨。
void practical_worst_score() {
    struct Student { std::string name; int score; };
    std::vector<Student> cls{
        {"Alice", 88}, {"Bob", 72}, {"Cathy", 95}, {"David", 55}, {"Eve", 80}
    };
    auto it = std::min_element(cls.begin(), cls.end(),
        [](const Student& a, const Student& b){ return a.score < b.score; });
    std::cout << "worst student: " << it->name
              << " (" << it->score << ")\n";
}

// ----------------------------------------------------------------
// LeetCode 2535: 陣列元素和與數位和的絕對差
// ----------------------------------------------------------------
// 題目:給陣列 nums,計算「元素和」與「數位和」的絕對差。
//      (這裡示範用 min_element 找其中「位數最少的」當小附帶 demo。)
//
// 為什麼用 std::min_element (作為衍生統計):
//   雖然主要計算用 accumulate,但「找位數最少的元素」 (有時做 debug 報告用)
//   就是 min_element + 自訂 comp 的標準用法。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2535_array_element_diff() {
    std::vector<int> nums{1, 15, 6, 3};
    int element_sum = 0, digit_sum = 0;
    for (int x : nums) {
        element_sum += x;
        int t = x;
        while (t) { digit_sum += t % 10; t /= 10; }
    }
    auto it = std::min_element(nums.begin(), nums.end(),
        [](int a, int b){
            auto d = [](int x){ int c = 0; do { ++c; x /= 10; } while (x); return c; };
            return d(a) < d(b);
        });
    std::cout << "LC2535 diff=" << std::abs(element_sum - digit_sum)
              << ", fewest-digit elem=" << *it << '\n';
}

// ----------------------------------------------------------------
// 實務範例:從候選路線中選「最便宜的」
// ----------------------------------------------------------------
// 場景:訂機票時系統提供多個 (轉乘方式, 價錢) 候選,
//      要找出「最便宜」的方案並回傳其名字。
void practical_cheapest_route() {
    struct Route { std::string name; int price; };
    std::vector<Route> routes{
        {"直飛", 8800}, {"轉香港", 6500}, {"轉曼谷", 7200}, {"轉新加坡", 6900}
    };
    auto it = std::min_element(routes.begin(), routes.end(),
        [](const Route& a, const Route& b){ return a.price < b.price; });
    std::cout << "cheapest route: " << it->name
              << " (" << it->price << ")\n";
}

// === 預期輸出 (Expected output) ===
// min = 1 at index 1
// min by |.| = -1
// lowest score: B(65)
// empty min_element == end: true
// LC1051: shortest height = 1 at index 0
// LC1431-min: kid with least candies = 1 at index 3
// worst student: David (55)
// LC2535 diff=9, fewest-digit elem=1
// cheapest route: 轉香港 (6500)
