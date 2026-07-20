// ============================================================================
// Data race：C++ 中不是「偶爾算錯」，而是 Undefined Behavior
// ============================================================================
// 當兩執行緒未同步地存取同一 memory location、至少一方寫入，且存取不是適當 atomic，
// 即形成 data race，整個程式具有 UB。`counter++` 是 read-modify-write，不是原子操作。
// volatile 只影響某些最佳化/硬體 I/O，不能修競態，也不建立 happens-before。
//
// 危險示意（不可執行）：
//   int counter = 0;
//   thread a([&]{ ++counter; }); thread b([&]{ ++counter; }); // UB
// 本章可執行路徑只展示三種正確 ownership：thread-local、partition、atomic。

#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

void basic_demo()
{
    constexpr int iterations = 5'000;
    std::atomic<int> counter{0};
    std::thread first([&] {
        for (int i = 0; i < iterations; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::thread second([&] {
        for (int i = 0; i < iterations; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    });
    first.join();
    second.join();
    assert(counter.load(std::memory_order_relaxed) == 2 * iterations);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接）
// 題目：輸入 nums，建立長度 2N 的 ans，使 ans[i]=ans[i+N]=nums[i]；例如 [1,2,1] 得 [1,2,1,1,2,1]。
// 為何使用本章主題：預配置 vector 後按 input index 分區，兩個 thread 只寫不同 int memory locations，示範以 ownership 避免 data race。
// 思路：1. 配置 2N 輸出。2. 將 input index 切成左右段。3. 每個 worker 同時填 i 與 i+N。4. join 後回傳。
// 複雜度：總工作 O(N)、輸出空間 O(N)，另有兩個 thread 的建立、排程與 join 成本。
// 易錯點：不可改成並行 push_back；要先檢查 2*N 的 size_t/容器上限，vector<bool> 也不具不同 index 即不同 memory location 的保證。
// -----------------------------------------------------------------------------
std::vector<int> get_concatenation(const std::vector<int>& numbers)
{
    std::vector<int> result(numbers.size() * 2U);
    const std::size_t middle = numbers.size() / 2U;
    auto copy_part = [&numbers, &result](std::size_t first, std::size_t last) {
        for (std::size_t index = first; index < last; ++index) {
            result.at(index) = numbers.at(index);
            result.at(index + numbers.size()) = numbers.at(index);
        }
    };
    std::thread left(copy_part, 0U, middle);
    std::thread right(copy_part, middle, numbers.size());
    left.join();
    right.join();
    return result;
}

void leetcode_demo()
{
    assert((get_concatenation({1, 2, 1}) ==
            std::vector<int>{1, 2, 1, 1, 2, 1}));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】事件批次的偶數計數 Map/Reduce
// 情境：兩個 worker 掃描事件值的不同分片，各自計算偶數筆數，最後由主執行緒合併結果。
// 為何使用本章主題：每個 worker 只寫自己的 partial slot，比共享 counter 每次加鎖或 atomic RMW 降低同步與爭用。
// 設計：1. 以 middle 分割 index。2. 每段逐值判斷偶數。3. 只遞增指定 slot。4. join 後相加兩個 partial。
// 成本：總工作 O(N)、額外空間 O(1)，但相鄰 partial slots 可能產生 false sharing，且 thread overhead 對小批次不划算。
// 上線注意：partial 與 values 必須活到 join；若 worker 數動態化應使用 padded per-worker counters，並確保每個 slot 唯一擁有。
// -----------------------------------------------------------------------------
std::size_t parallel_count_even(const std::vector<int>& values)
{
    const std::size_t middle = values.size() / 2U;
    std::size_t partial[2]{0U, 0U};
    auto count = [&values, &partial](std::size_t slot,
                                     std::size_t first,
                                     std::size_t last) {
        for (std::size_t index = first; index < last; ++index) {
            if (values.at(index) % 2 == 0) {
                ++partial[slot];  // slot 不重疊，無共享 read-modify-write。
            }
        }
    };
    std::thread left(count, 0U, 0U, middle);
    std::thread right(count, 1U, middle, values.size());
    left.join();
    right.join();
    return partial[0] + partial[1];
}

void practical_demo()
{
    assert(parallel_count_even({1, 2, 4, 7, 8}) == 3U);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "data race：atomic 與 ownership partition 測試通過\n";
}

// 【陷阱】「測一百次沒錯」不能證明沒有 data race；最佳化層級會改變症狀。
// 【陷阱】vector<bool> 不同 index 可能共享同一 machine word，不適合上述 partition 假設。
// 【面試】data race 與 race condition 不同：前者有標準定義且為 UB；後者泛指時序
//         影響邏輯結果，即使全用 mutex 仍可能有高階 race condition。
// 【練習】用 ThreadSanitizer 在私人壞例子驗證報告，再還原成正確版本。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_data_race.cpp' -o '/tmp/codex_cpp_C_MultiThread_02_data_race' && '/tmp/codex_cpp_C_MultiThread_02_data_race'
//
// === 預期輸出（節錄）===
// data race：atomic 與 ownership partition 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
