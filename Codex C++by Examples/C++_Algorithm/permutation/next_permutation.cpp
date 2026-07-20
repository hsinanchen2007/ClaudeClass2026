/*
 * std::next_permutation：原地變成字典序的下一個排列
 * =================================================
 * 若存在下一個排列回 true；若目前是最大排列，重排成最小排列並回 false。
 * 時間 O(N)，最多約 N/2 次 swap。需要 bidirectional iterator。
 *
 * 要枚舉全部 unique permutations，先排序到最小排列，再 do/while 呼叫。重複元素
 * 會自然只產生不同字典序排列，不需 set 去重。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 31：Next Permutation；標準函式就是題意，原地 O(1) 額外空間。
void leetcode_next_permutation(std::vector<int>& nums) {
    static_cast<void>(std::next_permutation(nums.begin(), nums.end()));
}

// 實務：列出小型 feature flags 的所有啟用順序，用於 deterministic 測試。
std::vector<std::string> practical_all_orders(std::string flags) {
    std::sort(flags.begin(), flags.end());
    std::vector<std::string> orders;
    do {
        orders.push_back(flags);
    } while (std::next_permutation(flags.begin(), flags.end()));
    return orders;
}

int main() {
    std::vector<int> values{1, 2, 3};
    assert(std::next_permutation(values.begin(), values.end()));
    assert((values == std::vector<int>{1, 3, 2}));

    values = {3, 2, 1};
    assert(!std::next_permutation(values.begin(), values.end()));
    assert((values == std::vector<int>{1, 2, 3}));

    std::vector<int> lc{1, 1, 5};
    leetcode_next_permutation(lc);
    assert((lc == std::vector<int>{1, 5, 1}));

    const auto orders = practical_all_orders("AAB");
    assert((orders == std::vector<std::string>{"AAB", "ABA", "BAA"}));

    std::cout << "next_permutation：LC31 與重複旗標枚舉測試通過\n";
}

/*
 * 易錯陷阱：
 * - 從未排序的中間排列開始，只會枚舉到最大排列，不會自動繞回後繼續。
 * - 回 false 時容器已被改成最小排列；不能把 false 當成「完全沒修改」。
 * - 全排列是 N!；即使每次 O(N)，N=12 已非常龐大，production 要限制輸入。
 * - 自訂 comparator 必須和初始 sort 使用同一規則，且滿足 strict weak ordering。
 *
 * 面試可解釋演算法：找最長非遞增 suffix；找 pivot；在 suffix 找剛好大於 pivot
 * 的元素交換；反轉 suffix 成最小。總體 O(N)、額外 O(1)。
 *
 * 生命週期：只在原容器交換，不改 size，vector iterator 不失效但值位置會變。
 * 對空範圍與單一元素會回 false，範圍維持原狀；這是合法邊界，不需特判。
 * 若元素 swap 會拋例外，範圍可能處於部分重排狀態，不能當成 transaction。
 *
 * 測試策略：除檢查期望下一排列，也要確認輸出仍是輸入的 is_permutation；對最大
 * 排列則同時驗回傳 false 與 wrap 到最小排列，才能抓到只看 bool 的測試缺口。
 *
 * 練習：手寫上述四步並以 std::next_permutation 作 oracle，property-test 所有長度
 * <=8 且含重複值的輸入。
 */
