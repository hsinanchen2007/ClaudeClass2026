/*
 * std::is_sorted / std::is_sorted_until：驗證排序 invariant
 * ======================================================
 * is_sorted 回 bool；is_sorted_until 回第一個破壞非遞減順序的 iterator，若全排序
 * 則回 end。時間 O(N)，最壞 N-1 次比較，額外空間 O(1)。空/單元素皆已排序。
 *
 * comparator 必須與建立排序時相同，且滿足 strict weak ordering。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 896. Monotonic Array（單調陣列）
// 題目：判斷 nums 是否全程非遞減或非遞增；例如 [1,2,2,3] 與 [6,5,4,4] 為 true，
// [1,3,2] 為 false。
// 為何使用本章主題：is_sorted 以預設 less 驗非遞減，再以 greater 驗非遞增；任一
// sorted invariant 成立即可，不需修改輸入。
// 思路：1. 檢查升冪非遞減；2. 若不成立，再檢查降冪非遞增；3. 回兩者 OR。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 的元素數，兩次檢查都可早退。
// 易錯點：相等鄰居合法；descending 必須傳 greater，而不是沿用預設 comparator。
// -----------------------------------------------------------------------------
bool leetcode_is_monotonic(const std::vector<int>& nums) {
    return std::is_sorted(nums.begin(), nums.end()) ||
           std::is_sorted(nums.begin(), nums.end(), std::greater<>{});
}

struct Sample {
    int timestamp;
    double value;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】Time Series 第一個時間倒退位置
// 情境：匯入 Sample 前要驗 timestamp 非遞減，並回第一筆小於前一筆的索引；完全
// 合法時回 samples.size() 供診斷報表使用。
// 為何使用本章主題：is_sorted_until 不只回 bool，還直接指出第一個破壞排序的後元素，
// 比再次掃描定位錯誤更有效率。
// 設計：1. comparator 只比 timestamp；2. 找 sorted prefix 尾端；3. 將 iterator 轉 index。
// 成本：時間 O(N)、額外空間 O(1)，N 為 Sample 數，可在首個逆序早退。
// 上線注意：相同 timestamp 目前合法；若要求唯一需另做 duplicate 檢查，size sentinel 不可解參考。
// -----------------------------------------------------------------------------
std::size_t practical_first_time_regression(const std::vector<Sample>& samples) {
    const auto point = std::is_sorted_until(
        samples.begin(), samples.end(),
        [](const Sample& lhs, const Sample& rhs) {
            return lhs.timestamp < rhs.timestamp;
        });
    return static_cast<std::size_t>(std::distance(samples.begin(), point));
}

int main() {
    const std::vector<int> sorted{1, 2, 2, 4};
    const std::vector<int> broken{1, 4, 3, 5};
    assert(std::is_sorted(sorted.begin(), sorted.end()));
    const auto point = std::is_sorted_until(broken.begin(), broken.end());
    assert(point == broken.begin() + 2 && *point == 3);

    assert(leetcode_is_monotonic({1, 2, 2, 3}));
    assert(leetcode_is_monotonic({6, 5, 4, 4}));
    assert(!leetcode_is_monotonic({1, 3, 2}));

    assert(practical_first_time_regression({{10, 1.0}, {20, 2.0}, {20, 3.0}}) == 3U);
    assert(practical_first_time_regression({{10, 1.0}, {30, 2.0}, {20, 3.0}}) == 2U);

    std::cout << "is_sorted：單調陣列與時間序列 invariant 測試通過\n";
}

/*
 * 易錯陷阱：
 * - 預設「sorted」是非遞減，duplicate 合法；不是嚴格遞增。
 * - is_sorted_until 回的是破壞順序的後一元素。例如 [1,4,3] 回指 3。
 * - comparator 用 <= 會讓相等元素互相 less，破壞 strict weak ordering。
 * - 時間戳 comparator 只看 timestamp，因此相同 timestamp 等價且合法；若業務要求
 *   唯一時間戳，要另做 adjacent_find duplicate 驗證。
 *
 * 面試：若資料宣稱 sorted，debug assert 可及早抓 producer bug；production 是否
 * 每次 O(N) 驗證取決於 trust boundary。跨服務輸入通常值得驗證或附帶 checksum。
 *
 * descending 驗證不是把 iterator 反過來，而是傳 std::greater；自訂 Record 則應
 * 重用真正排序時的 comparator 物件，避免兩處規則日後漂移。
 *
 * 測試要包含 empty、single、duplicate、第一對即逆序與最後一對才逆序，確保
 * is_sorted_until 的 end/begin 邊界都被覆蓋。
 *
 * 生命週期：回傳 iterator 只在容器未被重配置/銷毀時有效；實務函式回 index，較
 * 適合跨 API 邊界。練習：回傳 regression 前後兩筆及錯誤訊息，處理空範圍。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'is_sorted.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_is_sorted' && '/tmp/codex_cpp_C_Algorithm_sorting_is_sorted'
//
// === 預期輸出（節錄）===
// is_sorted：單調陣列與時間序列 invariant 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
