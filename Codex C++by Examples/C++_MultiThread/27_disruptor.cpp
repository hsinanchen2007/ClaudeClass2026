// ============================================================================
// Disruptor-style Sequenced Ring：預配置 slot、sequence 發布與 consumer gating
// ============================================================================
// LMAX Disruptor 的核心不是「另一個 queue 名稱」，而是 sequence-based ring、預配置事件、
// producer claim/publish、consumer gating、batching 與可選 wait strategy。完整 Disruptor
// 支援複雜 topology；本檔只做 SPSC 教學縮影，不能宣稱等同 production implementation。
//
// producer 的 next sequence 若超過 consumer N 格就等待，避免覆寫尚未消費 slot。
// 寫 payload 後 release-store published；consumer acquire-load 後讀，再 release-store
// consumed。sequence 以 unsigned modulo arithmetic 前進，index 再映射 ring slot；容量
// 小於序號空間一半時，producer-consumer 距離即使跨 uint64_t wrap 仍不含 signed UB。

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

template <std::size_t Capacity>
class SequencedRing {
    static_assert(Capacity > 0U);

public:
    using sequence_type = std::uint64_t;
    static_assert(Capacity <= std::numeric_limits<sequence_type>::max() / 2U,
                  "Capacity must keep modular sequence distance unambiguous");

    explicit SequencedRing(sequence_type initial_sequence = 0U)
        : next_publish_(initial_sequence),
          next_consume_(initial_sequence),
          published_(initial_sequence),
          consumed_(initial_sequence)
    {
    }

    void publish(int value)
    {
        const sequence_type sequence = next_publish_;
        while (sequence - consumed_.load(std::memory_order_acquire) >= capacity_value) {
            std::this_thread::yield();
        }
        slots_[slot_index(sequence)] = value;
        const sequence_type next = sequence + 1U;  // unsigned wrap 有完整定義。
        published_.store(next, std::memory_order_release);
        next_publish_ = next;
    }

    int consume()
    {
        const sequence_type expected = next_consume_;
        while (published_.load(std::memory_order_acquire) == expected) {
            std::this_thread::yield();
        }
        const int value = slots_[slot_index(expected)];
        const sequence_type next = expected + 1U;
        next_consume_ = next;
        consumed_.store(next, std::memory_order_release);
        return value;
    }

private:
    static constexpr sequence_type capacity_value = static_cast<sequence_type>(Capacity);

    static constexpr std::size_t slot_index(sequence_type sequence) noexcept
    {
        return static_cast<std::size_t>(sequence % capacity_value);
    }

    std::array<int, Capacity> slots_{};
    sequence_type next_publish_;  // 僅 producer 使用；代表下一個要 claim 的 position。
    sequence_type next_consume_;  // 僅 consumer 使用；代表下一個要讀的 position。
    alignas(64) std::atomic<sequence_type> published_;  // 已發布到哪個 exclusive position。
    alignas(64) std::atomic<sequence_type> consumed_;   // 已消費到哪個 exclusive position。
};

void basic_demo()
{
    SequencedRing<4> ring;
    std::thread producer([&] { ring.publish(10); ring.publish(20); });
    int first = 0;
    int second = 0;
    std::thread consumer([&] { first = ring.consume(); second = ring.consume(); });
    producer.join();
    consumer.join();
    expect(first == 10 && second == 20, "SPSC basic order mismatch");
}

// ----------------------------------------------------------------------------
// LeetCode 622：Design Circular Queue（sequence 版測試）
// ----------------------------------------------------------------------------
void leetcode_demo()
{
    SequencedRing<3> queue;
    std::thread producer([&] {
        for (int value : {1, 2, 3, 4, 5}) queue.publish(value);
    });
    std::vector<int> received;
    std::thread consumer([&] {
        for (int i = 0; i < 5; ++i) received.push_back(queue.consume());
    });
    producer.join();
    consumer.join();
    expect((received == std::vector<int>{1, 2, 3, 4, 5}),
           "circular queue order mismatch");
}

// ----------------------------------------------------------------------------
// 實務：預配置 telemetry pipeline，ring 很小仍由 gating 防覆寫
// ----------------------------------------------------------------------------
void practical_demo()
{
    SequencedRing<2> ring;
    int total = 0;
    std::thread producer([&] { for (int value = 1; value <= 20; ++value) ring.publish(value); });
    std::thread consumer([&] { for (int i = 0; i < 20; ++i) total += ring.consume(); });
    producer.join();
    consumer.join();
    expect(total == 210, "telemetry total mismatch");

    // 從 UINT64_MAX 前一格開始，三筆資料會跨過 max -> 0；舊 signed long long 寫法
    // 到 LLONG_MAX++ 是 UB，這個可注入起始序號讓 wrap path 能在測試中立即執行。
    SequencedRing<2> wrapping_ring(std::numeric_limits<std::uint64_t>::max() - 1U);
    std::vector<int> wrapped;
    std::thread wrapping_producer([&] {
        for (int value : {7, 8, 9}) wrapping_ring.publish(value);
    });
    std::thread wrapping_consumer([&] {
        for (int index = 0; index < 3; ++index) wrapped.push_back(wrapping_ring.consume());
    });
    wrapping_producer.join();
    wrapping_consumer.join();
    expect((wrapped == std::vector<int>{7, 8, 9}), "unsigned sequence wrap failed");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Disruptor-style ring：sequence、publish 與 gating 測試通過\n";
}

// 【陷阱】本例 published 是單一 sequence，只適用單 producer 依序發布；MPMC 需 per-slot
//         sequence、claim protocol，不能只把 next_publish 改 atomic 就完成。
// 【陷阱】busy-spin 低 latency 但吃 CPU；blocking/yield/sleep wait strategy 要依 workload。
// 【面試】為何用 unsigned position 而非 signed++？unsigned wrap 有定義，模距可表示 lag。
// 【練習】加入 batch consume：一次讀到 published snapshot 的所有可用序號。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '27_disruptor.cpp' -o '/tmp/codex_cpp_C_MultiThread_27_disruptor' && '/tmp/codex_cpp_C_MultiThread_27_disruptor'
//
// === 預期輸出（節錄）===
// Disruptor-style ring：sequence、publish 與 gating 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
