/*
 * std::set_difference：已排序 A 減去已排序 B（multiset 差集）
 * ==========================================================
 * 某值在 A 出現 m 次、B 出現 n 次，輸出 max(m-n,0) 次。時間 O(N+M)，輸出最多
 * N 項。輸出保持 A 的排序與相對來源語意。兩輸入需相同 comparator 排序。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// LeetCode 2215：Find the Difference of Two Arrays；題目是 distinct set，先 sort+unique。
std::vector<int> leetcode_only_in_first(std::vector<int> first,
                                        std::vector<int> second) {
    std::sort(first.begin(), first.end());
    first.erase(std::unique(first.begin(), first.end()), first.end());
    std::sort(second.begin(), second.end());
    second.erase(std::unique(second.begin(), second.end()), second.end());
    std::vector<int> output;
    std::set_difference(first.begin(), first.end(), second.begin(), second.end(),
                        std::back_inserter(output));
    return output;
}

// 實務：expected job IDs 減去 completed IDs，得到尚未完成清單。
std::vector<int> practical_pending_jobs(const std::vector<int>& expected,
                                        const std::vector<int>& completed) {
    assert(std::is_sorted(expected.begin(), expected.end()));
    assert(std::is_sorted(completed.begin(), completed.end()));
    std::vector<int> pending;
    pending.reserve(expected.size());
    std::set_difference(expected.begin(), expected.end(), completed.begin(),
                        completed.end(), std::back_inserter(pending));
    return pending;
}

int main() {
    const std::vector<int> a{1, 1, 2, 3, 5};
    const std::vector<int> b{1, 2, 2, 4};
    std::vector<int> difference;
    std::set_difference(a.begin(), a.end(), b.begin(), b.end(),
                        std::back_inserter(difference));
    assert((difference == std::vector<int>{1, 3, 5}));

    assert((leetcode_only_in_first({1, 2, 3, 3}, {2, 4, 6}) ==
            std::vector<int>{1, 3}));
    assert((practical_pending_jobs({10, 11, 12, 13}, {11, 13}) ==
            std::vector<int>{10, 12}));

    std::cout << "set_difference：LC2215 與 pending jobs 測試通過\n";
}

/*
 * 易錯陷阱：
 * - A-B 不對稱；交換輸入會得到不同結果。
 * - multiset 次數會相減。若業務是 distinct set，要先 unique 或使用真正 set 容器。
 * - completed 含 expected 不存在的 ID 不會出現在輸出；若要偵測異常，另算 B-A。
 * - 未排序輸入與 comparator 不一致都違反前置條件。
 *
 * 面試：要同時得到 only-A、only-B、both，可做三次 set operation，或單一雙指標
 * 一次分三流以減少掃描。清楚度與效能按資料量選擇。
 *
 * output 的生命週期由呼叫者持有；back_inserter 允許擴容。若保留 output iterator
 * 或 reference，後續 reallocation 仍會失效。練習：讓 pending jobs 保留 Job 結構，
 * comparator 只看 id，並思考等價元素輸出取自哪一側。
 *
 * 邊界速記：A-空=A、空-B=空、A-A=空（在同一等價規則下）。若只要計數而不要
 * materialize，可用手寫雙指標累計，避免配置輸出容器。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'set_difference.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_set_difference' && '/tmp/codex_cpp_C_Algorithm_set_operations_set_difference'
//
// === 預期輸出（節錄）===
// set_difference：LC2215 與 pending jobs 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
