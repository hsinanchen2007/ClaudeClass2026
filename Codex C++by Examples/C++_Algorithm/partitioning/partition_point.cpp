/*
 * std::partition_point：在已分區範圍以對數次 predicate 找分界
 * =============================================================
 * 前置條件：範圍必須已依同一 predicate partitioned；違反時結果不可信。
 * 對 random-access iterator 為 O(log N) predicate/iterator 操作；一般 forward
 * iterator 雖比較次數 O(log N)，iterator 前進總成本可到 O(N)。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode 278：First Bad Version 的一般化；false 表示 bad，找第一個 false。
int leetcode_first_bad_version(int version_count, int first_bad) {
    std::vector<int> versions(static_cast<std::size_t>(version_count));
    for (int version = 1; version <= version_count; ++version) {
        versions[static_cast<std::size_t>(version - 1)] = version;
    }
    const auto boundary = std::partition_point(
        versions.begin(), versions.end(),
        [first_bad](int version) { return version < first_bad; });
    return (boundary == versions.end()) ? -1 : *boundary;
}

struct Deployment {
    int id;
    bool healthy;
};

// 實務：輸入 invariant 為 healthy 前綴；找到第一個需 rollback 的 deployment。
int practical_first_unhealthy(const std::vector<Deployment>& deployments) {
    assert(std::is_partitioned(deployments.begin(), deployments.end(),
                               [](const Deployment& item) { return item.healthy; }));
    const auto point = std::partition_point(
        deployments.begin(), deployments.end(),
        [](const Deployment& item) { return item.healthy; });
    return (point == deployments.end()) ? -1 : point->id;
}

int main() {
    const std::vector<int> values{2, 4, 6, 1, 3, 5};
    const auto first_odd = std::partition_point(
        values.begin(), values.end(), [](int value) { return value % 2 == 0; });
    assert(first_odd == values.begin() + 3);
    assert(*first_odd == 1);

    assert(leetcode_first_bad_version(5, 4) == 4);
    assert(leetcode_first_bad_version(5, 1) == 1);
    assert(practical_first_unhealthy({{1, true}, {2, true}, {3, false}}) == 3);
    assert(practical_first_unhealthy({{1, true}, {2, true}}) == -1);

    std::cout << "partition_point：分界二分、LC278 與部署診斷測試通過\n";
}

/*
 * 易錯陷阱：
 * - 未 partitioned 就呼叫不是「比較慢」，而是前置條件破壞；debug 可先 assert
 *   is_partitioned，release 則由資料結構 API 維護 invariant。
 * - predicate 必須與建立分區時完全一致，包含 threshold/capture 的值。
 * - 回傳 end 表示全部 true；回傳 begin 表示第一項就 false，兩者都合法。
 * - forward_list 上不會神奇得到 O(log N) wall time，因 std::advance 本身是線性。
 *
 * 面試比較：partition_point 對布林單調區間做 binary search；lower_bound 對已排序
 * 值做第一個不小於 target。後者本質也利用 monotonic predicate。
 *
 * 實務 invariant 若被 concurrent writer 改變，檢查與搜尋間存在 TOCTOU；要鎖定、
 * snapshot 或接受 eventual result。練習：不用配置 versions vector，手寫 LC278 的
 * 整數索引二分，並避免 mid=(lo+hi)/2 overflow。
 */

/*
 * 【教科書補充：partition_point 的 invariant】
 * - 輸入必須是「true 前綴、false 後綴」，而且使用與查詢完全相同的 predicate。
 * - 若資料未 partitioned，或 predicate capture 在查詢期間改變，行為未定義而非單純答案不可信。
 * - 併發 writer 即使沒有改 size，也可能破壞 predicate 分界並造成 data race；查詢要看一致快照。
 * - ForwardIterator 的比較可為 O(log N)，但 iterator increments 仍可能 O(N)。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'partition_point.cpp' -o '/tmp/codex_cpp_C_Algorithm_partitioning_partition_point' && '/tmp/codex_cpp_C_Algorithm_partitioning_partition_point'
//
// === 預期輸出（節錄）===
// partition_point：分界二分、LC278 與部署診斷測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
