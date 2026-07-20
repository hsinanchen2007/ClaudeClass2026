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

// LeetCode 896：Monotonic Array。建立相鄰 index，檢查全遞增或全遞減。
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

// 實務：一次報告三種聚合狀態，避免手寫容易漏空範圍語意的 flags。
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
