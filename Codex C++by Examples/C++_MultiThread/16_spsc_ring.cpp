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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 622. Design Circular Queue（設計循環佇列）
// 題目：固定容量 queue 要支援入列、出列、Front/Rear 與滿空判斷；本檔只借題驗證 FIFO、滿佇列及 wrap-around 核心，並非完整介面。
// 為何使用本章主題：固定 ring 避免配置，單 producer/consumer 各自唯一寫 head/tail；release/acquire 在槽位交接時發布元素。
// 思路：1. 保留一格區分滿空。2. push 寫槽後發布 head。3. pop 讀槽後發布 tail。4. index 以 modulo 回繞。
// 複雜度：try_push/try_pop 時間 O(1)，固定空間 O(S)，可用容量為 S-1。
// 易錯點：此實作只允許恰一 producer、一 consumer；滿/空回 false，泛型非 trivial T 還需建構、析構與例外協定。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】單來源 Telemetry 的有界傳輸管線
// 情境：一個 producer 依序送出 1..100，一個 consumer 必須 FIFO 收完；ring 只有 7 個可用槽，滿/空時暫時讓出 CPU。
// 為何使用本章主題：SPSC ring 無每筆 mutex/配置，適合固定單寫單讀的低延遲 telemetry；相較 unbounded queue 可限制記憶體。
// 設計：1. producer 對每值重試 try_push。2. consumer 重試 try_pop。3. consumer 獨占寫 received。4. 雙方 join 後驗首尾。
// 成本：成功傳送 N 筆總工作 O(N)，固定 queue 空間 O(S)、接收結果 O(N)；busy wait 的 CPU/延遲無上界。
// 上線注意：應加 close/EOF 而非依賴固定筆數，並用 atomic wait/semaphore 或 backoff 取代長時間 spin；角色數量不可動態變成 MPMC。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '16_spsc_ring.cpp' -o '/tmp/codex_cpp_C_MultiThread_16_spsc_ring' && '/tmp/codex_cpp_C_MultiThread_16_spsc_ring'
//
// === 預期輸出（節錄）===
// SPSC ring：bounded FIFO 與 release/acquire 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
