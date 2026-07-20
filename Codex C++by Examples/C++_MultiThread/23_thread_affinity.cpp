// ============================================================================
// Thread Affinity：限制可執行 CPU，不是保證專用 core 或更快
// ============================================================================
// affinity 是 OS-specific，不屬標準 C++。Linux pthread_setaffinity_np 可限制 thread 的
// CPU mask；scheduler 仍決定何時執行，CPU 也可能與其他 process 共用。好處可能是 cache/
// NUMA locality；代價是負載不平衡、熱點、容器 cpuset 衝突與 portability。先 profile。
//
// 本檔只選目前允許 mask 的第一顆 CPU，不寫死 CPU 0；容器/cgroup 可能不允許 CPU 0。
// affinity 設定失敗是環境能力差異，函式回 false 而非把教材測試判成演算法錯誤。

#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <thread>

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

bool pin_current_thread_to_first_allowed_cpu()
{
#ifdef __linux__
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if (pthread_getaffinity_np(pthread_self(), sizeof(allowed), &allowed) != 0) {
        return false;
    }
    int selected = -1;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &allowed)) {
            selected = cpu;
            break;
        }
    }
    if (selected < 0) return false;
    cpu_set_t one;
    CPU_ZERO(&one);
    CPU_SET(selected, &one);
    return pthread_setaffinity_np(pthread_self(), sizeof(one), &one) == 0;
#else
    return false;
#endif
}

void basic_demo()
{
    bool affinity_supported = false;
    std::thread worker([&] { affinity_supported = pin_current_thread_to_first_allowed_cpu(); });
    worker.join();
    // Linux 正常環境通常 true；非 Linux/受限 sandbox 可 false，兩者都屬合法結果。
    static_cast<void>(affinity_supported);
}

// ----------------------------------------------------------------------------
// LeetCode 191：Number of 1 Bits，在 worker 上執行
// ----------------------------------------------------------------------------
int hamming_weight(unsigned int value)
{
    std::atomic<int> answer{0};
    std::thread worker([&] {
        static_cast<void>(pin_current_thread_to_first_allowed_cpu());
        int count = 0;
        while (value != 0U) {
            value &= value - 1U;
            ++count;
        }
        answer.store(count, std::memory_order_relaxed);
    });
    worker.join();
    return answer.load(std::memory_order_relaxed);
}

void leetcode_demo()
{
    assert(hamming_weight(0b1011U) == 3);
    assert(hamming_weight(0U) == 0);
}

// ----------------------------------------------------------------------------
// 實務：每 worker 獨立 shard；affinity 只是可選最佳化，不影響正確性
// ----------------------------------------------------------------------------
void practical_demo()
{
    int partial[2]{0, 0};
    std::thread a([&] {
        static_cast<void>(pin_current_thread_to_first_allowed_cpu());
        partial[0] = 20;
    });
    std::thread b([&] {
        static_cast<void>(pin_current_thread_to_first_allowed_cpu());
        partial[1] = 22;
    });
    a.join();
    b.join();
    assert(partial[0] + partial[1] == 42);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "thread affinity：可攜 fallback 與 Linux mask 路徑測試通過\n";
}

// 【陷阱】硬編 CPU 0 在 cgroup/container 可能 EINVAL；先讀 allowed mask。
// 【陷阱】pin 多個 compute threads 到同一 CPU 可能比不 pin 更慢。
// 【面試】affinity 與 NUMA memory placement 是兩件事；只 pin CPU 不保證資料 locality。
// 【練習】列出 allowed CPU mask，做 opt-in CLI，而不是 library 默默改 caller affinity。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '23_thread_affinity.cpp' -o '/tmp/codex_cpp_C_MultiThread_23_thread_affinity' && '/tmp/codex_cpp_C_MultiThread_23_thread_affinity'
//
// === 預期輸出（節錄）===
// thread affinity：可攜 fallback 與 Linux mask 路徑測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
