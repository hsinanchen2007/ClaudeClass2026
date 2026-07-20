// ============================================================================
// C++20 同步工具：latch、barrier、semaphore、atomic wait/notify
// ============================================================================
// latch：一次性倒數門；count 到 0 後永久開啟，不能重設。
// barrier：可重複 phase，每輪所有 participant arrive 後執行 completion 並進下一輪。
// counting_semaphore：管理 N 個 permit；binary_semaphore 是上限 1 的特化。
// atomic::wait(old)：值仍等於 old 時阻塞，notify 喚醒後仍要重查值。
//
// 選擇準則：等待 state predicate 用 cv/atomic wait；固定一批初始化完成用 latch；
// 同一批 worker 多輪對齊用 barrier；限制同時使用資源數量用 semaphore。

#include <atomic>
#include <barrier>
#include <cassert>
#include <iostream>
#include <latch>
#include <semaphore>
#include <string>
#include <thread>

void basic_demo()
{
    std::latch ready(2);
    int left = 0;
    int right = 0;
    std::thread a([&] { left = 20; ready.count_down(); });
    std::thread b([&] { right = 22; ready.count_down(); });
    ready.wait();
    a.join();
    b.join();
    assert(left + right == 42);

    std::atomic<int> state{0};
    std::thread waiter([&] {
        state.wait(0, std::memory_order_acquire);
        assert(state.load(std::memory_order_acquire) == 1);
    });
    state.store(1, std::memory_order_release);
    state.notify_one();
    waiter.join();
}

// ----------------------------------------------------------------------------
// LeetCode 1114：binary_semaphore 排序
// ----------------------------------------------------------------------------
class Foo {
public:
    void first()
    {
        output_ += "first";
        first_done_.release();
    }

    void second()
    {
        first_done_.acquire();
        output_ += "second";
        second_done_.release();
    }

    void third()
    {
        second_done_.acquire();
        output_ += "third";
    }

    const std::string& result() const { return output_; }

private:
    std::binary_semaphore first_done_{0};
    std::binary_semaphore second_done_{0};
    std::string output_;  // semaphore 建立順序；每次只有一個 writer。
};

void leetcode_demo()
{
    Foo foo;
    std::thread c([&] { foo.third(); });
    std::thread b([&] { foo.second(); });
    std::thread a([&] { foo.first(); });
    a.join();
    b.join();
    c.join();
    assert(foo.result() == "firstsecondthird");
}

// ----------------------------------------------------------------------------
// 實務：兩 worker 進行兩階段計算，barrier 保證 phase 1 全完才讀彼此結果
// ----------------------------------------------------------------------------
void practical_demo()
{
    int partial[2]{0, 0};
    int combined[2]{0, 0};
    std::barrier phase_end(2);
    auto worker = [&](int slot, int input) {
        partial[slot] = input * input;
        phase_end.arrive_and_wait();
        combined[slot] = partial[0] + partial[1];
        phase_end.arrive_and_wait();
    };
    std::thread a(worker, 0, 4);
    std::thread b(worker, 1, 5);
    a.join();
    b.join();
    assert(combined[0] == 41 && combined[1] == 41);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "C++20 sync：latch/atomic wait/semaphore/barrier 測試通過\n";
}

// 【陷阱】binary_semaphore 重複 release 超過 max 是前置條件違反，不是無限 counter。
// 【陷阱】barrier participant 提早退出要 arrive_and_drop，否則其他人永遠等待。
// 【面試】latch 與 barrier 差異：一次性 vs 可重複 phase；semaphore 不綁 thread ownership。
// 【練習】用 counting_semaphore<2> 驗證同時進入 critical resource 最多兩人。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_cpp20_sync.cpp' -o '/tmp/codex_cpp_C_MultiThread_12_cpp20_sync' && '/tmp/codex_cpp_C_MultiThread_12_cpp20_sync'
//
// === 預期輸出（節錄）===
// C++20 sync：latch/atomic wait/semaphore/barrier 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
