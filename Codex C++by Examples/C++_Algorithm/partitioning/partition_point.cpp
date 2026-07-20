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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 278. First Bad Version（第一個錯誤的版本）
// 題目：版本 1..N 中，從 first_bad 起皆為壞版，要找第一個壞版；例如 N=5、first_bad=4
// 回 4。本例以已知 first_bad 模擬 isBadVersion。
// 為何使用本章主題：version<first_bad 形成 true 前綴，partition_point 可找第一個 false；
// 但本教學先配置 1..N vector，總成本為 O(N)，正式題解應直接對整數索引二分。
// 思路：1. 建立版本序列；2. predicate 將好版分類為 true；3. 找分界；4. end 回 -1，
// 否則回該版本值。
// 複雜度：時間 O(N)、額外空間 O(N)，N=version_count；其中分界搜尋本身為 O(log N)。
// 易錯點：這不是原題最佳空間解；predicate 必須單調，且 first_bad 模擬值要符合版本範圍。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】部署序列第一個不健康版本
// 情境：Deployment rollout 依 id 排列，契約為 healthy 前綴後接 unhealthy 後綴；要找
// 第一個需要 rollback 的 deployment id，全部健康則回 -1。
// 為何使用本章主題：partition_point 利用健康狀態的單調分界做對數 predicate 查詢；
// debug assert 先以 is_partitioned 驗證 producer invariant。
// 設計：1. 開發期驗證 healthy 分區；2. 以同 predicate 找第一個 false；3. end 回 -1。
// 成本：含 assert 時間 O(N)、關閉 assert 後 O(log N)，額外空間 O(1)，N 為部署數。
// 上線注意：assert 在 release 消失；外部資料需 runtime 驗證或由型別維持 invariant，併發修改需 snapshot。
// -----------------------------------------------------------------------------
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
