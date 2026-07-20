/*
 * std::count / count_if：計算範圍中符合值/條件的元素數量
 * ======================================================
 * count 使用 ==；count_if 使用 unary predicate。時間一定 O(N)，不像 associative
 * container::count 可利用索引。回傳 iterator_traits<It>::difference_type，可能不是
 * int；大容器不要盲目窄化。演算法唯讀且不保存 iterator。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1512. Number of Good Pairs（好數對的數目）
// 題目：計算 i<j 且 nums[i]==nums[j] 的索引對數；例如 [1,2,3,1,1,3] 回 4。
// 為何使用本章主題：本教學版對每個新元素以 count 計算先前相同值數量，該數量正是
// 以目前位置為右端的新 pair；正式大資料宜用 frequency map 降到平均 O(N)。
// 思路：1. 逐位置 it 掃描；2. count [begin,it) 中等於 *it 的值；3. 加到 pairs。
// 複雜度：時間 O(N^2)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：只數前綴才能避免同一 pair 重複；大 N 時 pair 數可能超過 int。
// -----------------------------------------------------------------------------
int leetcode_num_identical_pairs(const std::vector<int>& nums) {
    int pairs = 0;
    for (auto it = nums.begin(); it != nums.end(); ++it) {
        pairs += static_cast<int>(std::count(nums.begin(), it, *it));
    }
    return pairs;
}

struct Request {
    int status_code;
    int latency_ms;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 錯誤與慢請求計數
// 情境：Request 快照包含 status_code 與 latency_ms，報表要分別統計 5xx 數量與
// latency>1000ms 的數量，同一筆可同時屬於兩者。
// 為何使用本章主題：兩次 count_if 清楚表達兩個獨立 predicate；指標種類少時可讀性
// 高，種類很多時應改單趟聚合以減少記憶體頻寬。
// 設計：1. 計數 status_code>=500；2. 計數 latency_ms>1000；3. 將 difference_type
// 安全轉成 size_t 回傳。
// 成本：時間 O(N)、額外空間 O(1)，N 為 request 數，實際掃描 2N 次元素。
// 上線注意：需驗證負 latency 與非法 status；只有 count 沒有總分母，無法直接表示 error rate。
// -----------------------------------------------------------------------------
std::vector<std::size_t> practical_request_counts(
    const std::vector<Request>& requests) {
    const auto errors = std::count_if(
        requests.begin(), requests.end(),
        [](const Request& request) { return request.status_code >= 500; });
    const auto slow = std::count_if(
        requests.begin(), requests.end(),
        [](const Request& request) { return request.latency_ms > 1000; });
    return {static_cast<std::size_t>(errors), static_cast<std::size_t>(slow)};
}

int main() {
    const std::vector<int> values{1, 2, 2, 3, 2};
    assert(std::count(values.begin(), values.end(), 2) == 3);
    assert(std::count_if(values.begin(), values.end(),
                         [](int value) { return value % 2 == 1; }) == 2);

    assert(leetcode_num_identical_pairs({1, 2, 3, 1, 1, 3}) == 4);
    assert(leetcode_num_identical_pairs({1, 1, 1, 1}) == 6);

    const std::vector<Request> requests{{200, 20}, {503, 50}, {200, 1500}, {500, 2000}};
    assert((practical_request_counts(requests) == std::vector<std::size_t>{2U, 2U}));
    assert((practical_request_counts({}) == std::vector<std::size_t>{0U, 0U}));
    std::cout << "count：LeetCode 1512 與實務 request 指標測試通過\n";
}

/*
 * 易錯陷阱：O(N) count 放進 N 次 loop 會變 O(N^2)，LeetCode 大資料可能超時；本例
 * 故意保留作 API 教學並明說最佳化。實務若同時要十種指標，一次 loop 聚合通常比
 * 十次 count_if 更省 memory bandwidth。
 *
 * 面試：vector<bool> count(true) 仍可用；浮點直接 count(value) 有精度問題，改用
 * count_if(abs(a-b)<=eps) 但要定義尺度。練習：用 unordered_map 重寫 good pairs。
 *
 * 測試再補充：全相同 n 個值應有 n*(n-1)/2 good pairs；計算式要用 long long 防
 * 大 n 溢位。實務 request 指標也要分母，只有 count 無法算 error rate。
 * 若資料是 input stream，只能走一次，應在同一 loop 同時累積全部 counters。
 * 練習完成後比較兩版結果，並用不同 N 量測 O(N^2) 與 O(N) 成長差距。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'count.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_count' && '/tmp/codex_cpp_C_Algorithm_non_modifying_count'
//
// === 預期輸出（節錄）===
// count：LeetCode 1512 與實務 request 指標測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
