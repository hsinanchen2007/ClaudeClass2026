/*
 * std::all_of / any_of / none_of：對範圍做量詞判斷
 * =================================================
 * all_of：全部符合；any_of：至少一個符合；none_of：沒有任何符合。三者最壞 O(N)，
 * 可短路。空範圍遵循數學 vacuous truth：all_of=true、any_of=false、none_of=true。
 * 演算法唯讀；predicate 應無副作用，不能依賴被呼叫幾次或順序。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 896. Monotonic Array（單調陣列）
// 題目：判斷 nums 是否整體非遞減或非遞增；例如 [1,2,2,3] 與 [6,5,4,4] 為 true，
// [1,3,2] 為 false。
// 為何使用本章主題：本教學版用 all_of 分別驗證所有相鄰 pair 的 <= 或 >=；為了套用
// API 先建立 index vector，正式單迴圈可省下 O(N) 空間。
// 思路：1. 長度小於 2 直接成立；2. iota 產生 0..N-2；3. all_of 檢查非遞減；
// 4. 再檢查非遞增，任一成立即回 true。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 nums 的元素數。
// 易錯點：相等元素在兩種單調性都合法；空與單元素也應回 true。
// -----------------------------------------------------------------------------
bool leetcode_is_monotonic(const std::vector<int>& nums) {
    if (nums.size() < 2U) {
        return true;
    }
    std::vector<std::size_t> indexes(nums.size() - 1U);
    std::iota(indexes.begin(), indexes.end(), 0U);
    const bool nondecreasing = std::all_of(
        indexes.begin(), indexes.end(),
        [&nums](std::size_t i) { return nums[i] <= nums[i + 1U]; });
    const bool nonincreasing = std::all_of(
        indexes.begin(), indexes.end(),
        [&nums](std::size_t i) { return nums[i] >= nums[i + 1U]; });
    return nondecreasing || nonincreasing;
}

struct Check {
    bool valid;
    bool warning;
    bool fatal;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】健康檢查量詞摘要
// 情境：每項 Check 提供 valid、warning、fatal；摘要要回答是否全部有效、是否至少
// 一項警告、以及是否完全沒有致命錯誤。
// 為何使用本章主題：all_of/any_of/none_of 直接對應三個量詞，短路語意清楚，避免
// 手寫旗標忘記空範圍的數學結果。
// 設計：1. all_of 聚合 valid；2. any_of 聚合 warning；3. none_of 排除 fatal；4. 回三值。
// 成本：時間 O(N)、額外空間 O(1)，N 為檢查數，最壞共掃描三次。
// 上線注意：空輸入得到 {true,false,true}；若業務要 unknown，必須在呼叫演算法前另建模。
// -----------------------------------------------------------------------------
std::vector<bool> practical_health_summary(const std::vector<Check>& checks) {
    const bool all_valid = std::all_of(checks.begin(), checks.end(),
                                       [](const Check& item) { return item.valid; });
    const bool any_warning = std::any_of(
        checks.begin(), checks.end(), [](const Check& item) { return item.warning; });
    const bool no_fatal = std::none_of(checks.begin(), checks.end(),
                                       [](const Check& item) { return item.fatal; });
    return {all_valid, any_warning, no_fatal};
}

int main() {
    const std::vector<int> values{2, 4, 6};
    assert(std::all_of(values.begin(), values.end(), [](int v) { return v > 0; }));
    assert(std::any_of(values.begin(), values.end(), [](int v) { return v == 4; }));
    assert(std::none_of(values.begin(), values.end(), [](int v) { return v < 0; }));

    assert(leetcode_is_monotonic({1, 2, 2, 3}));
    assert(leetcode_is_monotonic({6, 5, 4, 4}));
    assert(!leetcode_is_monotonic({1, 3, 2}));

    assert((practical_health_summary({{true, false, false}, {true, true, false}}) ==
            std::vector<bool>{true, true, true}));
    assert((practical_health_summary({}) == std::vector<bool>{true, false, true}));
    std::cout << "量詞演算法：LeetCode 896 與實務健康摘要測試通過\n";
}

/*
 * 易錯陷阱：空 checks 的 all_valid=true 不一定符合「沒有檢查資料應未知」的業務；
 * STL 給的是數學結果，實務 API 可先特判空並回 optional/status。
 *
 * 面試：all_of 可用 !any_of(not predicate) 表示，none_of 等於 !any_of；但直接 API
 * 更能表意。LeetCode 本例為教 all_of 建 index vector，最佳實作可單 loop 且 O(1)
 * 空間。練習：用 ranges::views::iota 移除 index 配置，並處理 C++20 lifetime。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'all_any_none_of.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_all_any_none_of' && '/tmp/codex_cpp_C_Algorithm_non_modifying_all_any_none_of'
//
// === 預期輸出（節錄）===
// 量詞演算法：LeetCode 896 與實務健康摘要測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
