/*
 * std::set_union：兩個已排序 multiset 的聯集
 * ============================================
 * 某值在 A/B 各 m/n 次，輸出 max(m,n) 次，不是 m+n。時間 O(N+M)，輸出上限 N+M。
 * 等價元素優先取第一範圍，並保持排序。兩輸入要用相同 comparator。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 349. Intersection of Two Arrays（兩個陣列的交集）
// 題目：原題要求兩陣列的 distinct 交集；本 helper 改求 distinct union，例如
// [1,2,2] 與 [2,3,3] 回 [1,2,3]，是集合操作對照而非 LC349 解法。
// 為何使用本章主題：兩邊 sort+unique 後，set_union 取每個值一次；用來和原題應用的
// set_intersection 比較 union/intersection 選型。
// 思路：1. 各自排序去重；2. set_union 合併兩個 distinct ranges；3. 回升冪結果。
// 複雜度：時間 O(N log N+M log M)、額外空間 O(N+M)，N/M 為兩輸入大小。
// 易錯點：不得把 union 當 LC349 提交結果；若不先 unique，單側 duplicate 仍會保留。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_distinct_union(std::vector<int> first,
                                         std::vector<int> second) {
    std::sort(first.begin(), first.end());
    first.erase(std::unique(first.begin(), first.end()), first.end());
    std::sort(second.begin(), second.end());
    second.erase(std::unique(second.begin(), second.end()), second.end());
    std::vector<int> output;
    std::set_union(first.begin(), first.end(), second.begin(), second.end(),
                   std::back_inserter(output));
    return output;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Blue/Green Rollout Audience 合併
// 情境：blue 與 green 都是升冪且理論上 unique 的 user IDs；發布系統要取得至少在
// 任一 audience 的完整名單，共同使用者只出現一次。
// 為何使用本章主題：set_union 以線性雙指標合併兩個 sorted set，重疊 ID 只輸出一份，
// 並保持結果升冪。
// 設計：1. 驗兩邊排序；2. 預留 N+M 上界；3. union 到 combined。
// 成本：時間 O(N+M)、額外空間 O(K)，K 為聯集大小，最壞 N+M。
// 上線注意：若單側含 duplicate，set_union 會保留該側 multiplicity；載入時應驗證 unique invariant。
// -----------------------------------------------------------------------------
std::vector<int> practical_combined_audience(const std::vector<int>& blue,
                                             const std::vector<int>& green) {
    assert(std::is_sorted(blue.begin(), blue.end()));
    assert(std::is_sorted(green.begin(), green.end()));
    std::vector<int> combined;
    combined.reserve(blue.size() + green.size());
    std::set_union(blue.begin(), blue.end(), green.begin(), green.end(),
                   std::back_inserter(combined));
    return combined;
}

int main() {
    assert((leetcode_distinct_union({1, 2, 2}, {2, 3, 3}) ==
            std::vector<int>{1, 2, 3}));

    const std::vector<int> a{1, 1, 2, 5};
    const std::vector<int> b{1, 3, 5, 5};
    std::vector<int> multiset_union;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                   std::back_inserter(multiset_union));
    assert((multiset_union == std::vector<int>{1, 1, 2, 3, 5, 5}));

    assert((practical_combined_audience({10, 20, 40}, {20, 30}) ==
            std::vector<int>{10, 20, 30, 40}));

    std::cout << "set_union：distinct/multiset 聯集與 rollout audience 測試通過\n";
}

/*
 * 易錯陷阱：
 * - std::merge 會保留 m+n 個 duplicate；set_union 只保留 max(m,n)。先選語意。
 * - 業務若要求真正 distinct set，輸入要先 unique；set_union 不會把單側自身重複
 *   自動壓成一份。
 * - comparator 只看 key 時，等價 payload 通常取第一範圍；不要以為會 merge fields。
 * - output 不能覆寫仍待讀的 input，通常使用新容器。
 *
 * 面試：已排序 vector 做一次 union 是線性且 cache-friendly；需要頻繁插入/查詢時
 * std::set 可能更合適，但 node allocation 與 locality 成本不同。
 *
 * 邊界：A union 空=A；A union A 仍保留 A 原本 multiplicity，不會倍增。輸出本身
 * 已排序，所以可直接串接 includes/intersection，不需再 sort。
 *
 * 練習：Audience 改為結構 `{id, cohort}`，同 id 衝突時不能靠 set_union 解決；
 * 寫明確 conflict resolver，並測試來源優先級。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'set_union.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_set_union' && '/tmp/codex_cpp_C_Algorithm_set_operations_set_union'
//
// === 預期輸出（節錄）===
// set_union：distinct/multiset 聯集與 rollout audience 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
