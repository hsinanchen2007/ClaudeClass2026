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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 31. Next Permutation（下一個排列）
// 題目：原地產生 nums 的下一個字典序排列；若已是最大排列則改成最小排列，例如
// [1,1,5] 變 [1,5,1]。
// 為何使用本章主題：std::next_permutation 的完整契約就是題意，包含 wrap-around，
// 可直接取代手寫 pivot/suffix 操作。
// 思路：1. 呼叫 next_permutation；2. 忽略 bool，因題目只要求修改後陣列且 false 時也已 wrap。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：回 false 不代表未修改；最大排列會被重排為最小排列，重複值也由字典序正確處理。
// -----------------------------------------------------------------------------
void leetcode_next_permutation(std::vector<int>& nums) {
    static_cast<void>(std::next_permutation(nums.begin(), nums.end()));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】小型 Feature Flag 順序窮舉測試
// 情境：整合測試要列出少量 flags 的所有不同啟用順序，輸出必須 deterministic，重複
// flag 不可造成重複排列。
// 為何使用本章主題：先排序到最小排列，再 do/while next_permutation，可依字典序列出
// 所有 unique permutations，不需額外 set 去重。
// 設計：1. 排序 flags；2. 先記錄目前排列；3. 反覆產生下一排列直到 wrap 回 false。
// 成本：時間與空間 O(P*N)，N 為字元數、P=N!/prod(count!) 為不同排列數。
// 上線注意：排列數階乘爆炸，必須限制 N；若只需逐一測試應串流處理，避免 materialize 全部結果。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'next_permutation.cpp' -o '/tmp/codex_cpp_C_Algorithm_permutation_next_permutation' && '/tmp/codex_cpp_C_Algorithm_permutation_next_permutation'
//
// === 預期輸出（節錄）===
// next_permutation：LC31 與重複旗標枚舉測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
