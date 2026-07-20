// ============================================================================
// atomic_flag、atomic_ref、fence：低階工具的前置條件與適用邊界
// ============================================================================
// atomic_flag 保證可作原子 test-and-set。純 spinlock 會一直輪詢；本例失敗後改呼叫
// atomic_flag::wait，實作通常先短暫 spin 再由 OS parking，因此是 blocking/hybrid mutex，
// 不能再宣稱等待者只會燒 CPU。標準不保證 fairness，也不指定 spin/park 的實作策略。
// atomic_ref<T> 將既有對齊 T 以 atomic 方式存取；在 ref 存活期間，同一 object 的所有
// 併發存取都必須走相容 atomic_ref，不可混普通 access。
//
// fence 本身不「刷新所有 cache」。它必須與 atomic communication 配對才建立順序；
// 比 acquire/release operation 更難 proof。優先使用 mutex 或直接在 flag 上標 memory order。

#include <atomic>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class AtomicFlagMutex {
public:
    void lock()
    {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            flag_.wait(true, std::memory_order_relaxed);
        }
    }

    void unlock()
    {
        flag_.clear(std::memory_order_release);
        flag_.notify_one();
    }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

void basic_demo()
{
    AtomicFlagMutex lock;
    int counter = 0;
    auto increment = [&] {
        for (int i = 0; i < 1'000; ++i) {
            const std::lock_guard<AtomicFlagMutex> guard(lock);
            ++counter;
        }
    };
    std::thread a(increment);
    std::thread b(increment);
    a.join();
    b.join();
    expect(counter == 2'000, "atomic_flag mutex counter mismatch");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 136. Single Number（只出現一次的數字）
// 題目：其餘值都恰出現兩次，找出唯一值；例如 [2,2,1] 得 1，[4,1,2,1,2] 得 4。
// 為何使用本章主題：兩分片先做 local XOR，再用 atomic_ref 將既有對齊 int 當 atomic RMW 合併，展示不改變儲存型別的原子 view。
// 思路：1. 建立符合 required_alignment 的 shared int。2. 分割輸入。3. 各 worker local XOR 後 fetch_xor。4. join 後 atomic load。
// 複雜度：總工作 O(N)、額外空間 O(1)，另有兩個 thread 與兩次共享 cache-line RMW。
// 易錯點：atomic_ref 不擁有 object且生命期/對齊要有效；ref 存活期間不可混用普通併發存取，relaxed 也不發布旁邊資料。
// -----------------------------------------------------------------------------
int single_number(const std::vector<int>& numbers)
{
    alignas(std::atomic_ref<int>::required_alignment) int shared_result = 0;
    std::atomic_ref<int> result(shared_result);
    const std::size_t middle = numbers.size() / 2U;
    auto reduce = [&](std::size_t first, std::size_t last) {
        int local = 0;
        for (std::size_t index = first; index < last; ++index) local ^= numbers.at(index);
        result.fetch_xor(local, std::memory_order_relaxed);
    };
    std::thread a(reduce, 0U, middle);
    std::thread b(reduce, middle, numbers.size());
    a.join();
    b.join();
    return result.load(std::memory_order_relaxed);
}

void leetcode_demo()
{
    const int first = single_number({2, 2, 1});
    const int second = single_number({4, 1, 2, 1, 2});
    expect(first == 1 && second == 4, "parallel XOR reduction mismatch");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】以 Fence 發布普通 Payload 的低階模式
// 情境：producer 寫入字串 payload 後以 relaxed ready flag 通知 consumer；consumer 看到 true 時必須安全讀到 "ready"。
// 為何使用本章主題：release/acquire fences 透過中間 atomic 的 reads-from 關係建立同步；此模式比直接 release-store/acquire-load 難審查，只作教學。
// 設計：1. producer 寫普通 payload。2. release fence 後 relaxed-store true。3. consumer relaxed-load 到該值。4. acquire fence 後讀 payload。
// 成本：發布/接收 O(1)，但 consumer busy-yield 的 CPU 與延遲無上界。
// 上線注意：consumer load 必須讀到 producer 的 store 才成立；優先改用 flag 上的 release/acquire，並加 wait/notify、生命週期與單 writer 契約。
// -----------------------------------------------------------------------------
void practical_demo()
{
    std::string payload;
    std::atomic<bool> ready{false};
    bool payload_matched = false;
    std::thread producer([&] {
        payload = "ready";
        std::atomic_thread_fence(std::memory_order_release);
        ready.store(true, std::memory_order_relaxed);
    });
    std::thread consumer([&] {
        while (!ready.load(std::memory_order_relaxed)) std::this_thread::yield();
        std::atomic_thread_fence(std::memory_order_acquire);
        payload_matched = payload == "ready";
    });
    producer.join();
    consumer.join();
    expect(payload_matched, "release/acquire fence did not publish payload");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "atomic low-level：flag/ref/fence 測試通過\n";
}

// 【陷阱】atomic_ref 不擁有 object；object 死亡或未達 required_alignment 都是錯誤。
// 【注意】本例 wait 可能先 spin 再 park；若移除 wait 改成空迴圈，才是持續耗 CPU 的純 spin。
// 【面試】fence-fence synchronization 仍需 atomic value 串起 reads-from 關係。
// 【練習】用 atomic<bool> acquire/release 改寫 practical，說明為何更容易 review。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '29_atomic_flag_ref_fence.cpp' -o '/tmp/codex_cpp_C_MultiThread_29_atomic_flag_ref_fence' && '/tmp/codex_cpp_C_MultiThread_29_atomic_flag_ref_fence'
//
// === 預期輸出（節錄）===
// atomic low-level：flag/ref/fence 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
