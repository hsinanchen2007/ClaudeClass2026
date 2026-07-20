/*
 * std::is_partitioned：檢查 true 區塊是否全部位於 false 區塊之前
 * =============================================================
 * 所謂 partitioned 不代表已排序，只要求 predicate 為 true 的元素形成前綴，之後
 * 全為 false。空範圍與單一元素都回 true。時間 O(N)，最多 N 次 predicate。
 *
 * 此演算法不修改元素、不保存 iterator；predicate 不可改變元素或容器結構。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode 905：Sort Array By Parity 的驗證器；偶數必須全部在奇數之前。
bool leetcode_is_parity_partitioned(const std::vector<int>& nums) {
    return std::is_partitioned(nums.begin(), nums.end(),
                               [](int value) { return value % 2 == 0; });
}

struct Request {
    int id;
    bool urgent;
};

// 實務：批次發送前驗證 urgent queue 沒有被普通請求插入中間。
bool practical_validate_priority_batch(const std::vector<Request>& requests) {
    return std::is_partitioned(requests.begin(), requests.end(),
                               [](const Request& request) {
                                   return request.urgent;
                               });
}

int main() {
    const auto positive = [](int value) { return value > 0; };
    const std::vector<int> empty;
    assert(std::is_partitioned(empty.begin(), empty.end(), positive));

    const std::vector<int> valid{2, 4, 8, 1, 3};
    const std::vector<int> invalid{2, 1, 4, 3};
    assert(std::is_partitioned(valid.begin(), valid.end(),
                               [](int value) { return value % 2 == 0; }));
    assert(!std::is_partitioned(invalid.begin(), invalid.end(),
                                [](int value) { return value % 2 == 0; }));

    assert(leetcode_is_parity_partitioned({2, 4, 1, 3}));
    assert(!leetcode_is_parity_partitioned({2, 1, 4, 3}));
    assert(practical_validate_priority_batch({{1, true}, {2, true}, {3, false}}));
    assert(!practical_validate_priority_batch({{1, true}, {2, false}, {3, true}}));

    std::cout << "is_partitioned：奇偶與請求批次驗證通過\n";
}

/*
 * 易錯陷阱：範圍的 begin/end 必須來自同一個仍存活的容器。main 特別用具名 empty，
 * 不從兩個內容相同、實際上卻不同物件的 temporary 分別取得 iterator。
 *
 * `true,false,false` 是 partitioned；`false,false` 也是；`false,true` 才違規。
 * predicate 必須在整次檢查保持同一語意。若讀取會變動的 atomic 狀態，結果可能
 * 只反映混合時間點，不是可信 snapshot。
 *
 * 面試：如何一趟手寫？先跳過 true，看到第一個 false 後，只要再遇到 true 就
 * 回 false。標準函式已把這個意圖表達清楚，通常優先使用。
 *
 * 實務選擇：若只驗證 invariants 用 is_partitioned；若要修正資料用 partition 或
 * stable_partition；若還要得到 boundary，用 partition_point，但前提必須已分區。
 *
 * 練習：讓 Request 增加 `cancelled`，定義 urgent 且未取消才在前；思考 predicate
 * 是業務分類還是排序規則，以及相同群組內是否需要穩定順序。
 */
