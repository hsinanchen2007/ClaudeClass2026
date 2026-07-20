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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列的動態和）
// 題目：回傳 answer[i]=nums[0]+...+nums[i]；例如 [1,2,3,4] 回 [1,3,6,10]。
// 為何使用本章主題：std::partial_sum 的預設 inclusive prefix 正好就是 running sum，
// 並按左到右順序寫入等長輸出。
// 思路：1. 建立與 nums 等長 answer；2. 對完整範圍做 partial_sum；3. 回傳輸出。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 nums 的元素數。
// 易錯點：輸出範圍必須先有 N 格；累加型別跟隨輸入 int，長序列可能溢位。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_running_sum(const std::vector<int>& nums) {
    std::vector<int> answer(nums.size());
    std::partial_sum(nums.begin(), nums.end(), answer.begin());
    return answer;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】每日流量前綴索引
// 情境：daily 流量在索引建立後唯讀，服務要反覆回答半開區間 [begin,end) 的總量，
// 包含空區間也要回 0。
// 為何使用本章主題：建 n+1 格 prefix 並以 partial_sum 從 prefix[1] 開始寫，可讓每次
// 查詢只做 prefix[end]-prefix[begin]，適合查多改少資料。
// 設計：1. prefix[0]=0；2. partial_sum 建累積值；3. 驗證 half-open 邊界；4. 兩前綴相減。
// 成本：建置時間/空間 O(N)，每次查詢時間 O(1)，N 為 daily 筆數。
// 上線注意：daily 任一更新都使後續 prefix 過期；高頻更新應改 Fenwick/segment tree，外部索引需 runtime 驗證。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'partial_sum.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_partial_sum' && '/tmp/codex_cpp_C_Algorithm_numeric_partial_sum'
//
// === 預期輸出（節錄）===
// partial_sum：running sum 與 O(1) 區間查詢測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
