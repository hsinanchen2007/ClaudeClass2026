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

// LeetCode 1512：Number of Good Pairs。
// 教學版對每個位置 count 前綴相同值，O(N^2)；hash frequency 可最佳化到平均 O(N)。
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

// 實務：報告 5xx 與慢請求數；一次 count_if 各掃一遍，清楚但總共 2N。
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
