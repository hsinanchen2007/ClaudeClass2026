// ============================================================================
// SPSC Ring Buffer：單 producer、單 consumer 的 bounded queue
// ============================================================================
// 固定陣列以 head 指下一寫入槽、tail 指下一讀取槽。保留一格區分 full/empty：
// empty: head==tail；full: next(head)==tail，可用容量為 N-1。producer 唯一寫 head，
// consumer 唯一寫 tail；雙方才可不用 CAS。把它拿給 MPMC 使用會 data race/遺失資料。
//
// producer 先寫元素，再 release-store head；consumer acquire-load head 後才讀元素。
// consumer 讀完再 release-store tail；producer acquire-load tail 後才重用槽位。
// try_push/try_pop O(1)、無配置；滿/空時立即 false，由上層決定 spin/drop/block。

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

template <std::size_t StorageSize>
class SpscRing {
    static_assert(StorageSize >= 2U);

public:
    bool try_push(int value)
    {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = increment(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        values_[head] = value;
        head_.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(int& value)
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        value = values_[tail];
        tail_.store(increment(tail), std::memory_order_release);
        return true;
    }

    static constexpr std::size_t capacity() { return StorageSize - 1U; }

private:
    static constexpr std::size_t increment(std::size_t index)
    {
        return (index + 1U) % StorageSize;
    }

    std::array<int, StorageSize> values_{};
    alignas(64) std::atomic<std::size_t> head_{0U};
    alignas(64) std::atomic<std::size_t> tail_{0U};
};

void basic_demo()
{
    SpscRing<4> ring;
    const bool pushed_10 = ring.try_push(10);
    const bool pushed_20 = ring.try_push(20);
    assert(pushed_10);
    assert(pushed_20);
    int value = 0;
    const bool popped_10 = ring.try_pop(value);
    assert(popped_10 && value == 10);
    const bool popped_20 = ring.try_pop(value);
    assert(popped_20 && value == 20);
    const bool empty = !ring.try_pop(value);
    assert(empty);
}

// ----------------------------------------------------------------------------
// LeetCode 622：Design Circular Queue
// ----------------------------------------------------------------------------
void leetcode_demo()
{
    SpscRing<4> queue;  // 可用容量 3。
    const bool pushed_1 = queue.try_push(1);
    const bool pushed_2 = queue.try_push(2);
    const bool pushed_3 = queue.try_push(3);
    const bool full = !queue.try_push(4);
    assert(pushed_1);
    assert(pushed_2);
    assert(pushed_3);
    assert(full);
    int value = 0;
    const bool popped_1 = queue.try_pop(value);
    assert(popped_1 && value == 1);
    const bool wrapped_push = queue.try_push(4);
    assert(wrapped_push);
    const bool popped_2 = queue.try_pop(value);
    assert(popped_2 && value == 2);
}

// ----------------------------------------------------------------------------
// 實務：telemetry producer/consumer；busy wait 只用於短小教材
// ----------------------------------------------------------------------------
void practical_demo()
{
    SpscRing<8> ring;
    std::vector<int> received;
    received.reserve(100U);
    std::thread producer([&] {
        for (int value = 1; value <= 100; ++value) {
            while (!ring.try_push(value)) {
                std::this_thread::yield();
            }
        }
    });
    std::thread consumer([&] {
        while (received.size() < 100U) {
            int value = 0;
            if (ring.try_pop(value)) {
                received.push_back(value);  // 只有 consumer 存取 received。
            } else {
                std::this_thread::yield();
            }
        }
    });
    producer.join();
    consumer.join();
    assert(received.front() == 1 && received.back() == 100);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "SPSC ring：bounded FIFO 與 release/acquire 測試通過\n";
}

// 【陷阱】容量是 StorageSize-1，不是 StorageSize；若要全用需額外 count/sequence。
// 【陷阱】泛型 T 若非 trivial，slot 建構/銷毀與 exception 需額外設計。
// 【面試】為何 head/tail 分別只有單一 writer？這正是能不用 CAS 的核心前提。
// 【練習】增加 close flag，讓 consumer 在排空後自然離開而非依賴固定 100 筆。
