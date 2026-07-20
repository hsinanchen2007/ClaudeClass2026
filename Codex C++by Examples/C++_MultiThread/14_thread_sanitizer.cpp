// ============================================================================
// ThreadSanitizer：動態找 data race，不是正確性的形式證明
// ============================================================================
// 建議命令：
//   clang++ -std=c++20 -O1 -g -fsanitize=thread -fno-omit-frame-pointer -pthread 14_thread_sanitizer.cpp -o /tmp/tsan_demo && /tmp/tsan_demo
// TSan 會 instrument memory access 並回報未同步衝突、thread stack 與 lock 關係；成本
// 可達數倍 CPU/大量記憶體，只適合測試。它只能看到本次執行走過的路徑；報告 0 個
// race 不等於證明所有 interleaving 正確，也不抓一般 deadlock/高階邏輯 race。
//
// 故意帶 race 的程式只應在隔離測試用 macro 建立；本教材預設路徑全部 race-free。

#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

void basic_demo()
{
    int counter = 0;
    std::mutex mutex;
    auto increment = [&] {
        for (int i = 0; i < 1'000; ++i) {
            const std::lock_guard<std::mutex> lock(mutex);
            ++counter;
        }
    };
    std::thread a(increment);
    std::thread b(increment);
    a.join();
    b.join();
    assert(counter == 2'000);
}

// ----------------------------------------------------------------------------
// LeetCode 217：Contains Duplicate（安全的平行插入）
// ----------------------------------------------------------------------------
// 共享 unordered_set 的每次操作都鎖住同一 mutex；即使插不同 bucket，rehash 仍修改
// 全域結構，所以不能無鎖併發 insert。結果 atomic 不是必要，因也在同一鎖內。
bool contains_duplicate(const std::vector<int>& values)
{
    std::unordered_set<int> seen;
    std::mutex mutex;
    bool duplicate = false;
    const std::size_t middle = values.size() / 2U;
    auto scan = [&](std::size_t first, std::size_t last) {
        for (std::size_t index = first; index < last; ++index) {
            const std::lock_guard<std::mutex> lock(mutex);
            if (!seen.insert(values.at(index)).second) {
                duplicate = true;
            }
        }
    };
    std::thread left(scan, 0U, middle);
    std::thread right(scan, middle, values.size());
    left.join();
    right.join();
    return duplicate;
}

void leetcode_demo()
{
    assert(contains_duplicate({1, 2, 3, 1}));
    assert(!contains_duplicate({1, 2, 3, 4}));
}

// ----------------------------------------------------------------------------
// 實務：用 message passing 避免共享 vector push_back
// ----------------------------------------------------------------------------
void practical_demo()
{
    std::vector<int> left;
    std::vector<int> right;
    std::thread a([&] { left = {1, 2}; });
    std::thread b([&] { right = {3, 4}; });
    a.join();
    b.join();
    left.insert(left.end(), right.begin(), right.end());  // join 後由主 thread 合併。
    assert((left == std::vector<int>{1, 2, 3, 4}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "TSan 教材：預設 race-free 範例測試通過\n";
}

// 【陷阱】TSan 通常不能與 ASan 同一 binary 同時啟用；分開建置測試。
// 【陷阱】自訂 assembly/synchronization 若 TSan 不理解，可能 false positive/negative。
// 【面試】TSan 與 race detector 的限制：動態 coverage、排程概率、額外成本。
// 【練習】在 /tmp 寫一個未鎖 counter，確認 TSan 報告後刪除，不污染教材原始碼。
