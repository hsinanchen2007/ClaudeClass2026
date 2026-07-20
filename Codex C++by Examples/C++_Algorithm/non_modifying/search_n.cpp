/*
 * std::search_n：尋找連續 count 個相同/符合條件的元素
 * ===================================================
 * search_n(first,last,count,value) 回第一段長度 count 的連續 value 起點；找不到回 last。
 * count<=0 視為立即匹配 first（實務最好拒絕模糊輸入）。最壞時間 O(N)，演算法唯讀。
 * predicate 版本對每個元素與 value 比較，適合 tolerance/category 判斷。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

// LeetCode 485：Max Consecutive Ones。
// 教學版逐步詢問是否存在長度 best+1 的 run，最壞 O(N^2)；單 loop 可 O(N)。
int leetcode_find_max_consecutive_ones(const std::vector<int>& nums) {
    int best = 0;
    while (std::search_n(nums.begin(), nums.end(), best + 1, 1) != nums.end()) {
        ++best;
    }
    return best;
}

// 實務：偵測是否有 threshold 次連續失敗，回第一段 index；無則 size。
std::size_t practical_first_failure_streak(const std::vector<int>& status,
                                           std::size_t threshold) {
    if (threshold == 0U) {
        return status.size();  // 業務上拒絕「零次失敗即告警」。
    }
    const auto it = std::search_n(status.begin(), status.end(),
                                  static_cast<std::ptrdiff_t>(threshold), 0);
    return static_cast<std::size_t>(std::distance(status.begin(), it));
}

int main() {
    const std::vector<int> values{0, 1, 1, 1, 0};
    assert(std::search_n(values.begin(), values.end(), 3, 1) == values.begin() + 1);
    assert(std::search_n(values.begin(), values.end(), 4, 1) == values.end());

    assert(leetcode_find_max_consecutive_ones({1, 1, 0, 1, 1, 1}) == 3);
    assert(leetcode_find_max_consecutive_ones({0, 0}) == 0);

    assert(practical_first_failure_streak({1, 0, 0, 0, 1}, 3U) == 1U);
    assert(practical_first_failure_streak({1, 0, 1}, 2U) == 3U);
    assert(practical_first_failure_streak({1, 1}, 0U) == 2U);
    std::cout << "search_n：LeetCode 485 與實務 failure streak 測試通過\n";
}

/*
 * 易錯陷阱：size_t threshold 轉 iterator difference_type 前要確認可表示；本例資料小。
 * 對浮點近似連續值可傳 predicate，但 tolerance 必須對稱且資料契約清楚。
 *
 * 面試：LC485 最佳解一次 loop 維持 current/best，O(N)/O(1)；本例刻意以 search_n
 * 教 API，不應用於大資料。實務告警還需 hysteresis/cooldown，否則長 failure run
 * 每次輪詢都重複告警。練習：回所有 maximal runs 的起點與長度。
 *
 * 【LeetCode 最佳不變量】
 * 單趟版本遇 1 就 ++current，遇 0 就 current=0，每步 best=max(best,current)。這與
 * 本教材 search_n 重複詢問相比，才是面試應交付的 O(N) 解。
 *
 * 【實務告警】
 * status 以 0 表失敗只是範例；HTTP 0、false、error enum 的語意需正規化。若資料
 * 跨 batch，failure streak 狀態要從上一批延續，單獨 search_n 每批會漏跨界 run。
 *
 * 測試 threshold=1、等於 size、大於 size、零門檻、run 在頭尾與跨 batch。
 * 練習：寫單趟最佳版，並用隨機 0/1 vector 與本教材版交叉驗證。
 * 面試時清楚區分「是否存在長度 k」與「求最大長度」兩種需求。
 * 實務跨批狀態至少要保存上一批尾端 run 長度與是否已告警。
 */
