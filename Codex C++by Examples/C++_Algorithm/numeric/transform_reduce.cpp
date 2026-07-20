/*
 * std::transform_reduce：融合 transform 與 reduce，避免中間容器
 * ============================================================
 * 一元版本先 map 每項再 reduce；二元版本把兩範圍配對 map，再 reduce。
 * 時間 O(N)，通常額外空間 O(1)。reduce 部分可重排，聚合 operation 必須適合
 * 未指定分組；第二範圍仍沒有 end 參數，呼叫者要保證長度足夠。
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

// LeetCode 977：Squares of a Sorted Array；此處示範平方總和這個衍生查詢。
long long leetcode_sum_of_squares(const std::vector<int>& nums) {
    return std::transform_reduce(
        nums.begin(), nums.end(), 0LL, std::plus<>{},
        [](int value) {
            const long long wide = value;
            return wide * wide;
        });
}

struct Sample {
    double predicted;
    double actual;
};

// 實務：不配置 error vector，直接計算 mean squared error。
double practical_mean_squared_error(const std::vector<Sample>& samples) {
    if (samples.empty()) {
        return 0.0;
    }
    const double total = std::transform_reduce(
        samples.begin(), samples.end(), 0.0, std::plus<>{},
        [](const Sample& sample) {
            const double error = sample.predicted - sample.actual;
            return error * error;
        });
    return total / static_cast<double>(samples.size());
}

int main() {
    const std::vector<int> lhs{1, 2, 3};
    const std::vector<int> rhs{4, 5, 6};
    const int dot = std::transform_reduce(lhs.begin(), lhs.end(), rhs.begin(), 0);
    assert(dot == 32);

    assert(leetcode_sum_of_squares({-4, -1, 0, 3, 10}) == 126);
    const double mse = practical_mean_squared_error({{2.0, 1.0}, {4.0, 5.0}});
    assert(std::abs(mse - 1.0) < 1e-12);
    assert(practical_mean_squared_error({}) == 0.0);

    std::cout << "transform_reduce：融合 map/reduce、平方和與 MSE 測試通過\n";
}

/*
 * 易錯與效能：
 * 1. 融合可省一個暫存 vector 與一次 memory traffic，但不代表一定更快；量測為準。
 * 2. 二元版本的預設是乘法 transform、加法 reduce；自訂版本的兩個 operation
 *    參數位置容易顛倒，應靠具名 lambda 與單元測試固定語意。
 * 3. 浮點 reduce 可重排；若訓練指標要求 bitwise reproducibility，使用固定順序
 *    或專門 reproducible reduction，而不是假設 execution::seq 等於 accumulate。
 * 4. callback 不可丟例外給 unsequenced policy，也不可有資料競爭。
 *
 * 面試：transform + accumulate 需要中間容器嗎？不一定，可手寫迴圈；
 * transform_reduce 的價值是明確表達可融合、可平行的 map-reduce 意圖。
 *
 * 練習：實作 cosine similarity，一次取得 dot product 與兩個 norm；比較三次
 * transform_reduce、單次結構歸約及數值穩定性的取捨。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'transform_reduce.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_transform_reduce' && '/tmp/codex_cpp_C_Algorithm_numeric_transform_reduce'
//
// === 預期輸出（節錄）===
// transform_reduce：融合 map/reduce、平方和與 MSE 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
