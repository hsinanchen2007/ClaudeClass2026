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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 191. Number of 1 Bits（位元 1 的個數）
// 題目：計算無號整數 binary representation 中 1 的個數；例如 0b1011 得 3、0 得 0。
// 為何使用本章主題：演算法不需要 affinity；本例只把 Brian Kernighan 計數放進 worker，並嘗試 pin 到 allowed mask 第一顆 CPU 作可選示範。
// 思路：1. worker 嘗試設定 affinity。2. 反覆執行 value&=value-1 清最低 set bit。3. 累加 count。4. join 後讀結果。
// 複雜度：B 個 set bits 的時間 O(B)、空間 O(1)，但 thread/OS affinity 系統呼叫成本遠高於小型計數。
// 易錯點：affinity 失敗不能改變正確性；value 必須 unsigned，relaxed store 的完成可見性在此由 join 保證。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】具可選 CPU Pinning 的分片工作
// 情境：兩個 worker 各自擁有一個 partial slot，分別產生 20、22；平台允許時可 pin CPU，但總和 42 不得依賴 pin 成功。
// 為何使用本章主題：獨立 shard 才是正確性基礎，affinity 只可能改善 cache/NUMA locality；相較強制 CPU 0，先讀 allowed mask 可適應 cgroup。
// 設計：1. 每 worker 嘗試 pin。2. 各自寫唯一 slot。3. join 建立完成關係。4. 主執行緒合併 partial。
// 成本：資料操作 O(1)，另有每 worker 一次平台 affinity 查詢/設定與 thread 排程成本。
// 上線注意：非 Linux 或 sandbox 可合法失敗；pin 多 worker 到同 CPU 反而變慢，且 affinity 不等於 NUMA memory placement。
// -----------------------------------------------------------------------------
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
