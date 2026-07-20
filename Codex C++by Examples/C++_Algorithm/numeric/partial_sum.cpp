/*
 * std::partial_sum：建立 prefix scan（含目前元素的前綴聚合）
 * ========================================================
 * 預設輸出 [a0, a0+a1, ...]；自訂 op 依序聚合。時間 O(N)，輸出需 N 格。
 * 它保證左到右；C++17 inclusive_scan 另提供 execution policy 與顯式 init。
 * 原地輸出合法，但獨立輸出通常更容易保留原始資料與除錯。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

// LeetCode 1480：Running Sum of 1d Array。
std::vector<int> leetcode_running_sum(const std::vector<int>& nums) {
    std::vector<int> answer(nums.size());
    std::partial_sum(nums.begin(), nums.end(), answer.begin());
    return answer;
}

// 實務：每日流量轉累積值，再以 prefix[right]-prefix[left] 做 O(1) 區間查詢。
class PracticalTrafficPrefix {
public:
    explicit PracticalTrafficPrefix(const std::vector<long long>& daily)
        : prefix_(daily.size() + 1U, 0) {
        std::partial_sum(daily.begin(), daily.end(), prefix_.begin() + 1);
    }

    long long range_sum(std::size_t begin, std::size_t end) const {
        assert(begin <= end);
        assert(end <= prefix_.size() - 1U);
        return prefix_[end] - prefix_[begin];
    }

private:
    std::vector<long long> prefix_;
};

int main() {
    const std::vector<int> values{2, 3, 5, 7};
    std::vector<int> product(values.size());
    std::partial_sum(values.begin(), values.end(), product.begin(),
                     [](int lhs, int rhs) { return lhs * rhs; });
    assert((product == std::vector<int>{2, 6, 30, 210}));

    assert((leetcode_running_sum({1, 2, 3, 4}) ==
            std::vector<int>{1, 3, 6, 10}));

    const PracticalTrafficPrefix traffic({10, 20, 5, 7});
    assert(traffic.range_sum(0, 4) == 42);
    assert(traffic.range_sum(1, 3) == 25);
    assert(traffic.range_sum(2, 2) == 0);

    std::cout << "partial_sum：running sum 與 O(1) 區間查詢測試通過\n";
}

/*
 * 易錯陷阱：
 * 1. partial_sum 自身沒有前置 identity 0；若要方便 [l,r) 查詢，像實務類別一樣
 *    額外配置 n+1 並把輸出寫到 begin()+1。
 * 2. 多次 query 才值得 O(N) 預處理；只查一次直接 accumulate 該範圍較簡單。
 * 3. 原資料更新後 prefix 全部過期。高頻更新/查詢應改 Fenwick tree 或 segment tree。
 * 4. int prefix 很容易溢位；由輸出容器元素型別控制結果，使用 long long。
 *
 * 面試比較：prefix sum 是 immutable/batch 問題的首選；difference array 擅長批次
 * 區間更新；Fenwick tree 提供 O(log N) 單點更新與 prefix query。
 *
 * 練習：擴充為二維 prefix sum，回答矩形總和；清楚定義四個邊界是 inclusive 還是
 * half-open，並為空矩形與第一列/欄寫測試。
 */
