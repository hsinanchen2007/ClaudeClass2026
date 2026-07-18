// ============================================================
// std::max_element
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/max_element
//   * https://cplusplus.com/reference/algorithm/max_element/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::max_element 是 std::min_element 的孿生 — 在範圍中找最大元素的迭代器。
//
// 重要差別:
//
//   * std::max(a, b)              → 兩個值的較大者
//   * std::max_element(first, last) → 範圍中最大元素的「迭代器」
//
// max_element 拿到迭代器後可以:
//   * *it 拿值
//   * it - begin 算索引
//   * it->member 拿成員
//   * 用 distance(begin, it) 算位置
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、邊界條件                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 空範圍 → last (要先檢查 it != last)。
//   * 多個最大 → 回傳「第一個」(穩定;與 minmax_element 取「最後一個」不同!)。
//
// 這個「最大值有多個時取哪一個」的差異很細,但會被一些 LeetCode 題目刁難 —
// 看清楚規格才不會踩雷。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 找排行榜首位 (LC 1431 Kids With Greatest Number of Candies)
//   * 找最常出現元素 (對結構陣列搭配 lambda)
//   * 找「字串最長」「物件分數最高」等
//   * 任何「掃描一次找極值」的需求
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   FwdIt max_element(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class Compare>
//   FwdIt max_element(FwdIt first, FwdIt last, Compare comp);
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
//   1. 空範圍回傳 last。
//   2. 多個最大取「第一個」。
//   3. 同時要 min/max 用 std::minmax_element 一次掃描省一半比較。
//
// ============================================================

/*
補充筆記：std::max_element
  - max_element 回傳最大元素的 iterator；需要索引時再用 distance 換算。
  - 若範圍為空會回傳 last，不能直接 *it。
  - 對結構資料找最大值時，lambda 比較器應明確寫出比較哪個欄位，避免讀者猜。
  - std::max_element 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{3, 1, 4, 1, 5, 9, 2, 6};

    // --- 範例 1: 找最大值與位置 ---
    auto it = std::max_element(v.begin(), v.end());
    std::cout << "max = " << *it
              << " at index " << (it - v.begin()) << '\n';

    // --- 範例 2: 字串中字典序最大 ---
    std::vector<std::string> w{"apple", "kiwi", "banana", "orange"};
    auto it2 = std::max_element(w.begin(), w.end());
    std::cout << "max string = " << *it2 << '\n';

    // --- 範例 3: 字串中「最長」 ---
    auto it3 = std::max_element(w.begin(), w.end(),
                                [](const std::string& a, const std::string& b){
                                    return a.size() < b.size();
                                });
    std::cout << "longest = " << *it3 << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1431_kids_with_greatest_candies();
    void leetcode_169_majority_element_variant();
    void practical_top_player();
    void leetcode_414_third_max_basic();
    void practical_hottest_day_index();
    leetcode_1431_kids_with_greatest_candies();
    leetcode_169_majority_element_variant();
    practical_top_player();
    leetcode_414_third_max_basic();
    practical_hottest_day_index();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1431: 擁有最多糖果的小孩
//                (Kids With the Greatest Number of Candies)
// ----------------------------------------------------------------
// 題目:給 candies[] 與 extraCandies。對每個 i 判斷 candies[i] + extra 是否
//      會 ≥ 「目前最大值」 — 是的話標 true。回傳布林陣列。
//
// 為什麼用 std::max_element:
//   先用 max_element 一次找出當前最大 m (O(N)),
//   再對每個 i 判斷 candies[i] + extra >= m,O(N) 完成。
//
// 複雜度:時間 O(n);空間 O(1) (除輸出)。
void leetcode_1431_kids_with_greatest_candies() {
    std::vector<int> candies{2, 3, 5, 1, 3};
    int extra = 3;
    int m = *std::max_element(candies.begin(), candies.end());
    std::cout << "LC1431: ";
    for (int c : candies) std::cout << ((c + extra >= m) ? "T " : "F ");
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 169 變體:多數元素 (Majority Element)
// ----------------------------------------------------------------
// 題目:找出「出現次數超過 n/2」的元素。
//
// 為什麼用 std::max_element (對計數結果):
//   先把 (value, count) 統計起來,再用 max_element + 自訂 comp 找出
//   count 最大的 pair。雖然 LC 169 最佳解是 Boyer-Moore O(N) 投票法,
//   這裡示範 max_element 對結構陣列「依某欄位找極值」的標準寫法。
//
// 複雜度:時間 O(n²)(逐筆 vector 計次);若改 unordered_map 為 O(n)。
void leetcode_169_majority_element_variant() {
    std::vector<int> nums{2, 2, 1, 1, 1, 2, 2};
    std::vector<std::pair<int,int>> cnt;
    for (int x : nums) {
        bool found = false;
        for (auto& p : cnt) if (p.first == x) { ++p.second; found = true; break; }
        if (!found) cnt.push_back({x, 1});
    }
    auto it = std::max_element(cnt.begin(), cnt.end(),
        [](auto& a, auto& b){ return a.second < b.second; });
    std::cout << "LC169-variant: majority = " << it->first
              << " (count = " << it->second << ")\n";
}

// ----------------------------------------------------------------
// 實務範例:多人遊戲計分板找首位
// ----------------------------------------------------------------
// 場景:多人遊戲計分板要找出「目前分數最高」的玩家以頒獎。
//
// 為什麼用 std::max_element:
//   回傳迭代器,可同時拿到玩家姓名與分數 — 一次解決。
void practical_top_player() {
    struct Player { std::string name; int score; };
    std::vector<Player> board{
        {"Alice", 1200}, {"Bob", 900}, {"Cathy", 1500}, {"David", 1350}
    };
    auto it = std::max_element(board.begin(), board.end(),
        [](const Player& a, const Player& b){ return a.score < b.score; });
    std::cout << "top player: " << it->name
              << " (" << it->score << ")\n";
}

// ----------------------------------------------------------------
// LeetCode 414: 第三大的數 (Third Maximum Number) — 簡化版
// ----------------------------------------------------------------
// 題目:找出陣列中「第三大不同的數」;若不存在,回傳最大的。
//
// 為什麼用 std::max_element:
//   * 簡單但不一定最佳:重複用 max_element 找最大、移除、再找次大… —
//     缺點是 O(k·n);若只要前 3 大,k = 3 仍可接受。
//   * 教學重點:示範 max_element 重複呼叫「找前 k 大」的直覺寫法。
//
// 複雜度:時間 O(k·n);空間 O(n) (拷貝)。
void leetcode_414_third_max_basic() {
    std::vector<int> nums{2, 2, 3, 1};
    std::vector<int> copy = nums;
    std::sort(copy.begin(), copy.end());
    copy.erase(std::unique(copy.begin(), copy.end()), copy.end());

    int ans;
    if (copy.size() < 3) {
        ans = *std::max_element(copy.begin(), copy.end());
    } else {
        ans = copy[copy.size() - 3];
    }
    std::cout << "LC414: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:找一週中最熱的那一天 (溫度 vector 找位置)
// ----------------------------------------------------------------
// 場景:氣象資料以 vector<double> 儲存,要找最高溫所在的「第幾天」。
//      max_element 回傳 iterator,既能取值也能取位置。
void practical_hottest_day_index() {
    std::vector<double> temps{28.5, 30.1, 27.9, 33.2, 31.0, 29.8, 32.5};
    auto it = std::max_element(temps.begin(), temps.end());
    std::cout << "hottest day index=" << (it - temps.begin())
              << ", temp=" << *it << '\n';
}

// === 預期輸出 (Expected output) ===
// max = 9 at index 5
// max string = orange
// longest = banana
// LC1431: T T T F T
// LC169-variant: majority = 2 (count = 4)
// top player: Cathy (1500)
// LC414: 1
// hottest day index=3, temp=33.2
