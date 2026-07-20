// ============================================================================
// Parallel Scan：prefix sum 的 up-sweep / offset 思維
// ============================================================================
// scan 輸出每個 prefix 的 reduction。inclusive_scan [1,2,3]=>[1,3,6]；exclusive_scan
// 初值 0=>[0,1,3]。它不同於 reduce：每個位置都要結果，因此 block 平行演算法通常：
// 1. 每 block 做 local scan；2. scan 各 block total；3. 將前置 offset 加回 block。
//
// 本例固定兩 block，工作 O(n)、額外空間 O(n)、critical path 約 O(n/2)。大型 GPU/CPU
// 會用樹狀多層 scan。operation 必須可結合；浮點加法重排可能改變末位結果。

#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> parallel_inclusive_scan(const std::vector<int>& input)
{
    if (input.empty()) return {};
    if (input.size() == 1U) return input;

    std::vector<int> output(input.size());
    const std::size_t middle = input.size() / 2U;
    auto local_scan = [&input, &output](std::size_t first, std::size_t last) {
        int running = 0;
        for (std::size_t index = first; index < last; ++index) {
            running += input.at(index);
            output.at(index) = running;
        }
    };

    std::thread left(local_scan, 0U, middle);
    std::thread right(local_scan, middle, input.size());
    left.join();
    right.join();

    const int right_offset = output.at(middle - 1U);
    std::thread add_offset([&] {
        for (std::size_t index = middle; index < output.size(); ++index) {
            output.at(index) += right_offset;
        }
    });
    add_offset.join();
    return output;
}

void basic_demo()
{
    assert((parallel_inclusive_scan({1, 2, 3, 4}) ==
            std::vector<int>{1, 3, 6, 10}));
    assert(parallel_inclusive_scan({}).empty());
}

// ----------------------------------------------------------------------------
// LeetCode 1480：Running Sum of 1d Array
// ----------------------------------------------------------------------------
std::vector<int> running_sum(const std::vector<int>& numbers)
{
    return parallel_inclusive_scan(numbers);
}

void leetcode_demo()
{
    assert((running_sum({1, 1, 1, 1, 1}) == std::vector<int>{1, 2, 3, 4, 5}));
    assert((running_sum({3, 1, 2, 10, 1}) == std::vector<int>{3, 4, 6, 16, 17}));
}

// ----------------------------------------------------------------------------
// 實務：每分鐘 request count 轉成累積計數
// ----------------------------------------------------------------------------
void practical_demo()
{
    const std::vector<int> requests_per_minute{5, 8, 2, 10, 5, 1};
    const auto cumulative = parallel_inclusive_scan(requests_per_minute);
    assert(cumulative.back() == 31);
    assert(cumulative.at(2) == 15);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "parallel scan：兩 block local scan + offset 測試通過\n";
}

// 【陷阱】不能只把陣列分塊各自 scan；右塊缺少所有左塊的 total offset。
// 【陷阱】非 associative operation（如減法）不能任意樹狀重排仍期待相同語意。
// 【面試】scan 是 stream compaction、radix sort、histogram prefix、GPU allocator 的基礎。
// 【練習】擴充成 k blocks，先遞迴 scan block totals 再加 offsets。

/*
 * 【教科書補充：thread 例外與 rollback】
 * - 建立第二個 thread 若失敗，第一個仍 joinable；未先 join/guard 就離開 scope 會 terminate。
 * - worker function 例外不可跨 thread entry 傳回，需 exception_ptr/future 或明確錯誤 channel。
 * - local prefix 與 offset 加法仍可能 signed overflow；平行化不會改變數值型別上限。
 * - production 版本需 join guard/cancellation，並測空、單元素、奇數長度與建立 thread 失敗。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '26_parallel_scan.cpp' -o '/tmp/codex_cpp_C_MultiThread_26_parallel_scan' && '/tmp/codex_cpp_C_MultiThread_26_parallel_scan'
//
// === 預期輸出（節錄）===
// parallel scan：兩 block local scan + offset 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
