/*
 * std::set_symmetric_difference：只在其中一側出現的 multiset 元素
 * =================================================================
 * 數學上是 (A-B) union (B-A)。某值出現 m/n 次，輸出 |m-n| 次。時間 O(N+M)，
 * 最多輸出 N+M 項。兩輸入必須依同一 comparator 排序。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2215. Find the Difference of Two Arrays（找出兩陣列的不同）
// 題目：原題要分開回只在 nums1 與只在 nums2 的 distinct 值；本 helper 將兩側合併
// 成不標來源的 symmetric difference，例如 [1,2,3] 與 [2,4,6] 回 [1,3,4,6]。
// 為何使用本章主題：sort+unique 後，set_symmetric_difference 一趟取得兩側 exclusive
// 值；因輸出不帶來源，僅適合作為原題的合併衍生結果。
// 思路：1. 兩邊排序去重；2. 求 symmetric difference；3. 依共同排序輸出。
// 複雜度：時間 O(N log N+M log M)、額外空間 O(N+M)，N/M 為兩輸入大小。
// 易錯點：不是原題要求的二維回傳；若需知道來源，必須分別做 A-B/B-A 或手寫 tagged merge。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_symmetric_difference(std::vector<int> first,
                                               std::vector<int> second) {
    std::sort(first.begin(), first.end());
    first.erase(std::unique(first.begin(), first.end()), first.end());
    std::sort(second.begin(), second.end());
    second.erase(std::unique(second.begin(), second.end()), second.end());
    std::vector<int> output;
    std::set_symmetric_difference(first.begin(), first.end(), second.begin(),
                                  second.end(), std::back_inserter(output));
    return output;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Replication Manifest ID Drift 偵測
// 情境：primary 與 replica manifest 依 file ID 升冪；要列出只出現在其中一側的所有
// ID，空結果表示兩份 ID multiset 一致。
// 為何使用本章主題：set_symmetric_difference 一趟取得雙向差異，比各自掃描 membership
// 更有效率，適合監控是否 drift。
// 設計：1. 驗兩邊 sorted；2. 預留 N+M 上界；3. 輸出兩側 multiplicity 差的絕對值。
// 成本：時間 O(N+M)、額外空間 O(K)，K 為 drift 輸出數，最壞 N+M。
// 上線注意：結果不標來源且 comparator 只看 ID；checksum 改變不會被偵測，修復計畫需更完整 diff。
// -----------------------------------------------------------------------------
std::vector<int> practical_manifest_drift(const std::vector<int>& primary,
                                          const std::vector<int>& replica) {
    assert(std::is_sorted(primary.begin(), primary.end()));
    assert(std::is_sorted(replica.begin(), replica.end()));
    std::vector<int> drift;
    drift.reserve(primary.size() + replica.size());
    std::set_symmetric_difference(primary.begin(), primary.end(), replica.begin(),
                                  replica.end(), std::back_inserter(drift));
    return drift;
}

int main() {
    assert((leetcode_symmetric_difference({1, 2, 3}, {2, 4, 6}) ==
            std::vector<int>{1, 3, 4, 6}));

    const std::vector<int> a{1, 1, 2, 5};
    const std::vector<int> b{1, 3, 5, 5};
    assert((practical_manifest_drift(a, b) ==
            std::vector<int>{1, 2, 3, 5}));
    assert(practical_manifest_drift(a, a).empty());

    std::cout << "set_symmetric_difference：LC2215 與 manifest drift 測試通過\n";
}

/*
 * 易錯陷阱：
 * - 結果不標示元素來自 A 還是 B；修復 replication 時通常仍需分別算 A-B/B-A。
 * - multiset 是 |m-n|；若 manifest 理論上 unique，先 validate duplicate，否則差異
 *   可能是重複筆數而非缺檔。
 * - 輸出最大 N+M，reserve 相加前對敵意尺寸需考慮 size_t overflow/配置失敗。
 * - comparator 等價不代表 operator==；key 相同但 payload 不同不會被列為 drift。
 *
 * 面試：symmetric difference 適合「是否一致」的快速明細；若只要 bool，可同步雙
 * 指標遇第一差異即早退，避免建立整個輸出。
 *
 * 數學性質：操作交換 A/B 結果值集合相同，但有 payload 且 comparator 只看 key 時，
 * 等價項目的來源選擇仍需查標準語意，不能由交換律推論 payload 完全一致。
 * 空範圍與 A 的 symmetric difference 就是 A，適合做 boundary test。
 *
 * 監控只看 drift 是否為空時，不一定要保存所有 ID；然而保留前幾個 sample 常能
 * 大幅改善診斷。可設輸出上限，超過後只累計 count，避免異常時耗盡記憶體。
 *
 * 練習：回傳 tagged drift `{id, only_primary}`，一次雙指標完成；再加入相同 id 但
 * checksum 不同的 changed 狀態，形成實際檔案同步 plan。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'set_symmetric_difference.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_set_symmetric_difference' && '/tmp/codex_cpp_C_Algorithm_set_operations_set_symmetric_difference'
//
// === 預期輸出（節錄）===
// set_symmetric_difference：LC2215 與 manifest drift 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
