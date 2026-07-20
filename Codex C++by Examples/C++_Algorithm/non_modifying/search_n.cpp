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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 485. Max Consecutive Ones（最大連續 1 的個數）
// 題目：輸入二進位陣列 nums，回最長連續 1 長度；例如 [1,1,0,1,1,1] 回 3。
// 為何使用本章主題：本教學版反覆用 search_n 詢問是否存在長度 best+1 的 1-run；
// 行為正確但最壞 O(N^2)，正式解應單趟維護 current/best。
// 思路：1. best 從 0 開始；2. 若存在 best+1 個連續 1 就遞增；3. 首次不存在時回 best。
// 複雜度：時間 O(N^2)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：不要把此存在性重複搜尋當最佳解；輸入若不只 0/1，題目語意需先驗證。
// -----------------------------------------------------------------------------
int leetcode_find_max_consecutive_ones(const std::vector<int>& nums) {
    int best = 0;
    while (std::search_n(nums.begin(), nums.end(), best + 1, 1) != nums.end()) {
        ++best;
    }
    return best;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】連續失敗門檻告警定位
// 情境：status 以 0 表失敗、1 表成功；要找第一段至少 threshold 個連續失敗的起點，
// 未達門檻或 threshold=0 時回 status.size()。
// 為何使用本章主題：search_n 正好搜尋固定長度的連續相同值，適合回答「是否已達 K 次」
// 與首段位置，不需手寫 run counter。
// 設計：1. 業務上拒絕零門檻；2. 搜尋 threshold 個連續 0；3. 將 iterator 轉 offset。
// 成本：時間 O(N)、額外空間 O(1)，N 為 status 數。
// 上線注意：threshold 轉 ptrdiff_t 前需確認可表示；跨批次 run 要攜帶上一批尾端狀態。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'search_n.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_search_n' && '/tmp/codex_cpp_C_Algorithm_non_modifying_search_n'
//
// === 預期輸出（節錄）===
// search_n：LeetCode 485 與實務 failure streak 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
