/*
 * std::prev_permutation：原地變成字典序的前一個排列
 * =================================================
 * 有前一個排列回 true；目前為最小排列時，重排成最大排列並回 false。
 * 它是 next_permutation 的鏡像，時間 O(N)、額外空間 O(1)，需雙向 iterator。
 *
 * 要反向枚舉全部排列，先依 comparator 排成最大排列，再 do/while 呼叫。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 1053：Previous Permutation With One Swap 的基準版。
// 題目限制只能一次 swap；prev_permutation 可能反轉 suffix，故僅作正確前序 oracle。
std::vector<int> leetcode_previous_permutation_oracle(std::vector<int> nums) {
    static_cast<void>(std::prev_permutation(nums.begin(), nums.end()));
    return nums;
}

// 實務：從最高優先的 rollout 順序逐步回退，列出 deterministic fallback 順序。
std::vector<std::string> practical_fallback_orders(std::string nodes) {
    std::sort(nodes.begin(), nodes.end(), std::greater<>{});
    std::vector<std::string> result;
    do {
        result.push_back(nodes);
    } while (std::prev_permutation(nodes.begin(), nodes.end()));
    return result;
}

int main() {
    std::vector<int> values{3, 2, 1};
    assert(std::prev_permutation(values.begin(), values.end()));
    assert((values == std::vector<int>{3, 1, 2}));

    values = {1, 2, 3};
    assert(!std::prev_permutation(values.begin(), values.end()));
    assert((values == std::vector<int>{3, 2, 1}));

    assert((leetcode_previous_permutation_oracle({3, 1, 1, 3}) ==
            std::vector<int>{1, 3, 3, 1}));

    const auto orders = practical_fallback_orders("ABC");
    assert(orders.size() == 6U);
    assert(orders.front() == "CBA" && orders.back() == "ABC");

    std::cout << "prev_permutation：前序 oracle 與 fallback 枚舉測試通過\n";
}

/*
 * 易錯陷阱：
 * - LC1053 要「一次 swap 後最大但小於原值」，不能直接把 prev_permutation 當提交解；
 *   本例明確命名 oracle，用於驗證手寫一次-swap 最佳解。
 * - 回 false 時已 wrap 到最大排列，容器不是保持不變。
 * - 反向完整枚舉要從最大排列開始；否則只列當前到最小的 suffix。
 * - N! 爆炸與 next_permutation 相同，只適合小型 exhaustive test/search。
 *
 * 面試推導：找最長非遞減 suffix，pivot 是前一項；交換 suffix 中剛小於 pivot 的
 * 最右元素，再反轉 suffix 使結果儘可能大但仍較小。
 *
 * 實務使用前先問是否真的需要 materialize 全排列。常見排程可用 backtracking
 * 邊生成邊剪枝，避免建立所有結果。練習：實作 LC1053 一次 swap 解，並以本 oracle
 * 對所有短陣列驗證，但注意重複值時應交換哪個最左/最右候選。
 *
 * 空範圍與單一元素回 false；自訂 comparator 的 descending 起點仍必須依同一個
 * comparator 的「最大排列」定義建立，不能混用預設 std::less 的排序結果。
 * 元素 swap 若拋例外，演算法不提供回滾保證，production 型別宜提供 noexcept swap。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'prev_permutation.cpp' -o '/tmp/codex_cpp_C_Algorithm_permutation_prev_permutation' && '/tmp/codex_cpp_C_Algorithm_permutation_prev_permutation'
//
// === 預期輸出（節錄）===
// prev_permutation：前序 oracle 與 fallback 枚舉測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
