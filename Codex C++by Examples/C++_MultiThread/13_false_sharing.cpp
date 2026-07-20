// ============================================================================
// False Sharing：不同變數沒有 data race，仍可能爭同一 cache line
// ============================================================================
// CPU cache coherence 通常以 cache line 為單位。兩 core 各寫不同 atomic，但若它們
// 落在同一 line，ownership 反覆轉移造成 invalidation traffic，稱 false sharing。
// 正確性不受影響，效能可能大降。解法包括 per-thread local reduction、padding/alignment、
// 分片；但 64 bytes 只是常見值，不是 C++ 對所有硬體的保證，務必 profile。
//
// 本章不以牆鐘時間 assertion，因 CI/VM 排程使 benchmark 不穩；只展示 layout 與正確性。

#include <atomic>
#include <bit>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>

struct alignas(64) PaddedCounter {
    std::atomic<unsigned int> value{0U};
};

static_assert(alignof(PaddedCounter) >= 64U);

void basic_demo()
{
    PaddedCounter counters[2];
    auto increment = [](PaddedCounter& counter) {
        for (unsigned int i = 0; i < 10'000U; ++i) {
            counter.value.fetch_add(1U, std::memory_order_relaxed);
        }
    };
    std::thread left(increment, std::ref(counters[0]));
    std::thread right(increment, std::ref(counters[1]));
    left.join();
    right.join();
    assert(counters[0].value.load() == 10'000U);
    assert(counters[1].value.load() == 10'000U);
}

// ----------------------------------------------------------------------------
// LeetCode 191：Number of 1 Bits（兩分片 local counters）
// ----------------------------------------------------------------------------
// 對 32 bits 分成兩半，每個 worker 只寫自己的 padded slot。時間固定 O(32)。
int hamming_weight(unsigned int value)
{
    PaddedCounter counters[2];
    auto count_range = [value, &counters](std::size_t slot,
                                          unsigned int first,
                                          unsigned int last) {
        unsigned int local = 0U;
        for (unsigned int bit = first; bit < last; ++bit) {
            local += (value >> bit) & 1U;
        }
        counters[slot].value.store(local, std::memory_order_relaxed);
    };
    std::thread low(count_range, 0U, 0U, 16U);
    std::thread high(count_range, 1U, 16U, 32U);
    low.join();
    high.join();
    return static_cast<int>(counters[0].value.load(std::memory_order_relaxed) +
                            counters[1].value.load(std::memory_order_relaxed));
}

void leetcode_demo()
{
    assert(hamming_weight(0b00000000000000000000000000001011U) == 3);
    assert(hamming_weight(0xFFFFFFFFU) == 32);
}

// ----------------------------------------------------------------------------
// 實務：per-worker metrics，最後才 reduce
// ----------------------------------------------------------------------------
void practical_demo()
{
    PaddedCounter processed[2];
    std::thread a([&] { processed[0].value.store(120U); });
    std::thread b([&] { processed[1].value.store(80U); });
    a.join();
    b.join();
    const unsigned int total = processed[0].value.load() + processed[1].value.load();
    assert(total == 200U);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "false sharing：padding 與 per-thread reduction 測試通過\n";
}

// 【陷阱】padding 增加記憶體 footprint，可能反過來傷 cache；只對熱寫欄位使用。
// 【陷阱】alignas(64) 保證 alignment 至少 64，不證明目標 CPU cache line 正好 64。
// 【面試】false sharing 沒有同址 data race，為何仍慢？coherence 追蹤 line 而非 C++ object。
// 【練習】以 perf stat 比較 padded/unpadded cache miss；不可只看單次 wall clock。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '13_false_sharing.cpp' -o '/tmp/codex_cpp_C_MultiThread_13_false_sharing' && '/tmp/codex_cpp_C_MultiThread_13_false_sharing'
//
// === 預期輸出（節錄）===
// false sharing：padding 與 per-thread reduction 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
