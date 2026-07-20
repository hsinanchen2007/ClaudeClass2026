/*
 * std::for_each_n（C++17）：從 first 起處理恰好 n 個元素
 * =====================================================
 * 回傳處理後的尾後 iterator。呼叫者必須保證至少有 n 個可解參考元素；classic API
 * 不知道 end。時間 O(n)。n=0 不做事並回 first。sequential overload 適合固定 batch；
 * 若 n 來自外部輸入，先 clamp/check，不能越界。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列的動態和）
// 題目：回傳 runningSum[i]=nums[0]+...+nums[i]；例如 [1,2,3,4] 回 [1,3,6,10]。
// 為何使用本章主題：本教學版以 sequential for_each_n 恰好處理 N 項，capture 維持
// running sum 並原地覆寫；partial_sum/inclusive_scan 會更符合前綴和語意。
// 思路：1. running 初始化為 0；2. 依序處理每個 value；3. 先加到 running，再把
// value 改為 running。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 元素數，不計按值回傳的陣列。
// 易錯點：正確性依賴 sequential 順序，不可換平行 policy；總和可能 int 溢位。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_running_sum(std::vector<int> nums) {
    int running = 0;
    std::for_each_n(nums.begin(), nums.size(), [&running](int& value) {
        running += value;
        value = running;
    });
    return nums;
}

struct Job {
    int id;
    bool processed;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】固定上限工作批次標記
// 情境：jobs 本輪最多接受 batch_size 筆，要把實際前 C 筆標為 processed，並回下一
// 批起始 index；batch_size 可大於剩餘筆數。
// 為何使用本章主題：for_each_n 精確處理 C 筆並回尾後 iterator；先以 min 限制 C，
// 補足 classic API 不知道 end 的安全責任。
// 設計：1. C=min(batch_size,jobs.size())；2. 對前 C 筆設 processed=true；3. 將回傳
// iterator 轉成 index。
// 成本：時間 O(C)、額外空間 O(1)，C=min(batch_size,N)。
// 上線注意：processed bool 無法表達執行失敗或重試；真正工作狀態應有 pending/running/done/error。
// -----------------------------------------------------------------------------
std::size_t practical_mark_batch(std::vector<Job>& jobs, std::size_t batch_size) {
    const std::size_t count = std::min(batch_size, jobs.size());
    const auto next = std::for_each_n(jobs.begin(), count,
                                      [](Job& job) { job.processed = true; });
    return static_cast<std::size_t>(std::distance(jobs.begin(), next));
}

int main() {
    std::vector<int> values{1, 2, 3, 4};
    const auto next = std::for_each_n(values.begin(), 2,
                                      [](int& value) { value *= 10; });
    assert(next == values.begin() + 2);
    assert((values == std::vector<int>{10, 20, 3, 4}));

    assert((leetcode_running_sum({1, 2, 3, 4}) ==
            std::vector<int>{1, 3, 6, 10}));
    assert(leetcode_running_sum({}).empty());

    std::vector<Job> jobs{{1, false}, {2, false}, {3, false}};
    assert(practical_mark_batch(jobs, 2U) == 2U);
    assert(jobs[0].processed && jobs[1].processed && !jobs[2].processed);
    assert(practical_mark_batch(jobs, 99U) == jobs.size());
    std::cout << "for_each_n：LeetCode 1480 與實務 batch 處理測試通過\n";
}

/*
 * 易錯陷阱：直接傳使用者給的 n 而沒有和 distance 比較會越界。對 input iterator，
 * 預先 distance 可能消耗 stream；API 應改接 counted iterator/ranges 或自行 loop 到 end。
 *
 * 面試：running sum 更應使用 partial_sum/inclusive_scan，語意明確且可最佳化；本例
 * 只為展示 for_each_n。LeetCode 複雜度 O(N)/O(1) 額外（不計回傳）。實務 batch
 * 真正執行工作若會失敗，單一 processed bool 不足，需 pending/running/done/error。
 *
 * 面試延伸：for_each_n 常和 counting iterator/parallel execution 搭配；平行 callable
 * 不可直接修改同一 running sum。prefix sum 應選 scan 家族。
 *
 * 測試 batch_size=0、剛好 size、大於 size；practical 函式先 min，確保永不越界。
 * 練習：讓函式接 start index，處理多輪 batch 且不重複處理。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'for_each_n.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_for_each_n' && '/tmp/codex_cpp_C_Algorithm_non_modifying_for_each_n'
//
// === 預期輸出（節錄）===
// for_each_n：LeetCode 1480 與實務 batch 處理測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
