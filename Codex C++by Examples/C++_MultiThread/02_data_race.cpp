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

// ----------------------------------------------------------------------------
// LeetCode 1929：Concatenation of Array（安全 partition 寫入）
// ----------------------------------------------------------------------------
// 兩 thread 各處理不重疊 index；同一 vector 的不同 int 元素是不同 memory location。
// 不可在 worker 裡 push_back，因那會同時修改 size/capacity 共享狀態。
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

// ----------------------------------------------------------------------------
// 實務：Map 階段只寫 thread-local，Reduce 階段由主 thread 合併
// ----------------------------------------------------------------------------
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
