// ============================================================================
// 課題 5：Microbenchmark 基礎與常見假數據
// ============================================================================
//
// `end-start` 很容易寫，但可信 benchmark 還需：Release optimization、warm-up、多次重複、
// 防止 dead-code elimination、固定/隨機化輸入、統計分布而非只看一次、隔離 I/O/allocation。
// compiler 若證明結果沒被使用，可能刪掉整段；若輸入 compile-time 可知，可能直接算答案。
//
// steady_clock 適合 elapsed，但 clock resolution 不等於 measurement accuracy。真正效能工作
// 優先 Google Benchmark/criterion/perf/Nsight，並報 CPU/GPU、compiler flags、樣本數。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

template<class Function>
std::chrono::nanoseconds measure(Function&& function, int repeats)
{
    const auto start = std::chrono::steady_clock::now();
    for (int repeat = 0; repeat < repeats; ++repeat) function();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

void basic_example()
{
    const std::vector<int> values(10'000U, 1);
    std::int64_t result = 0;
    const auto elapsed = measure([&] {
        result += std::accumulate(values.begin(), values.end(), std::int64_t{0});
    }, 100);
    // result 在 measurement 後被 assert/輸出，compiler 不能把全部 work 當未使用刪掉。
    assert(result == 1'000'000);
    assert(elapsed.count() >= 0);
    std::cout << "[基礎] 100 accumulations took " << elapsed.count() << " ns\n";
}

// LeetCode 704：Binary Search。先驗答案，再示範量測；不可 assertion「一定比 linear 快」，
// 因小資料、cache、optimizer、環境雜訊都可能反轉單次數據。
int binary_search_index(const std::vector<int>& nums, int target)
{
    std::size_t left = 0U;
    std::size_t right = nums.size();
    while (left < right) {
        const std::size_t middle = left + (right - left) / 2U;
        if (nums.at(middle) < target) left = middle + 1U;
        else right = middle;
    }
    return left < nums.size() && nums.at(left) == target ? static_cast<int>(left) : -1;
}

void leetcode_704_example()
{
    std::vector<int> nums(100'000U);
    std::iota(nums.begin(), nums.end(), 0);
    int checksum = 0;
    const auto elapsed = measure([&] { checksum += binary_search_index(nums, 99'999); }, 1'000);
    assert(checksum == 99'999'000);
    std::cout << "[LeetCode 704] 1000 searches took " << elapsed.count() << " ns\n";
}

// 實務：報 median 比只報 minimum/一次測量穩健；本例量多批並排序。
std::chrono::nanoseconds median(std::vector<std::chrono::nanoseconds> samples)
{
    assert(!samples.empty());
    std::sort(samples.begin(), samples.end());
    return samples.at(samples.size() / 2U);
}

void practical_example()
{
    std::vector<std::chrono::nanoseconds> samples;
    int value = 0;
    for (int run = 0; run < 7; ++run) {
        samples.push_back(measure([&] { value += run; }, 1'000));
    }
    assert(value == 21'000);
    std::cout << "[實務] median of 7 batches=" << median(samples).count() << " ns\n";
}

int main()
{
    basic_example();
    leetcode_704_example();
    practical_example();
}

// 練習：改用 Google Benchmark，加入 DoNotOptimize/ClobberMemory 並比較結果。
// 複雜度：量測成本是 O(repetitions × work)，clock calls 也屬 overhead，短工作需批次化。
// 生命週期：被測資料必須涵蓋整段量測；回傳指向已結束 fixture/local buffer 的 view 會失真或 UB。

/*
【本課面試問答】
Q1：benchmark 為何應使用 `steady_clock`？
A：elapsed time 需要 monotonic clock；system_clock 可能因 NTP/人工校時跳動。high_resolution_clock
只代表實作選的高解析 clock，可能就是 system_clock，不能只看名字假設 monotonic。

Q2：極短函式量到 0 ns 或不可思議高速，可能發生什麼？
A：編譯器可能刪除未被觀察的結果，或 clock overhead 大於工作。應用 optimized build、消費結果、
批次重複並扣除/估計 harness 成本；成熟時使用 Google Benchmark 的 DoNotOptimize 等工具。

Q3：只報最好的一次時間有何問題？
A：單次受 warm-up、cache、frequency scaling、scheduler、page fault 影響；minimum 又偏向理想路徑。
應固定環境、量多批，報 median/percentiles/variance 與輸入規模，並確認結果正確後才談速度。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_benchmark.cpp' -o '/tmp/codex_cpp_C_Chrono_05_benchmark' && '/tmp/codex_cpp_C_Chrono_05_benchmark'
//
// === 預期輸出（節錄）===
// [基礎] 100 accumulations took <依本機與當下負載而異> ns
// [LeetCode 704] 1000 searches took <依本機與當下負載而異> ns
// [實務] median of 7 batches=<依本機與當下負載而異> ns
// 注意：時間值會因 CPU、編譯器、系統負載與排程而不同。
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
