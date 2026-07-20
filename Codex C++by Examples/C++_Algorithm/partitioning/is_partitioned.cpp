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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 905. Sort Array By Parity（按奇偶排序陣列）
// 題目：原題要重排 nums，使所有偶數位於所有奇數之前；例如 [2,4,1,3] 合法。
// 本 helper 只驗證候選輸出是否符合該分區，不負責修改陣列。
// 為何使用本章主題：is_partitioned 正好檢查「偶數 true 前綴、奇數 false 後綴」；
// 可作 LC905 解法的 postcondition/assert，而非提交解本身。
// 思路：1. predicate 將偶數分類為 true；2. 驗證整段是否只出現 true...false。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：群組內不需排序；[2,1,4,3] 在 false 後又出現 true，因此驗證失敗。
// -----------------------------------------------------------------------------
bool leetcode_is_parity_partitioned(const std::vector<int>& nums) {
    return std::is_partitioned(nums.begin(), nums.end(),
                               [](int value) { return value % 2 == 0; });
}

struct Request {
    int id;
    bool urgent;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】緊急請求批次順序驗證
// 情境：派送批次契約要求所有 urgent Request 在普通請求之前；送出前只需驗證，不可
// 擅自重排已簽章或待稽核的來源。
// 為何使用本章主題：is_partitioned 唯讀檢查 urgent true 前綴，比完整 sort 更符合
// 二分類 invariant，也不改變群組內順序。
// 設計：1. 以 request.urgent 分類；2. 掃描到第一個普通請求後，確認後續不再有 urgent。
// 成本：時間 O(N)、額外空間 O(1)，N 為 request 數，可在首個反例早退。
// 上線注意：空批次會回 true；若 urgent 狀態可併發改變，需先取得一致 snapshot。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'is_partitioned.cpp' -o '/tmp/codex_cpp_C_Algorithm_partitioning_is_partitioned' && '/tmp/codex_cpp_C_Algorithm_partitioning_is_partitioned'
//
// === 預期輸出（節錄）===
// is_partitioned：奇偶與請求批次驗證通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
