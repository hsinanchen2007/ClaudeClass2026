// ============================================================================
// std::thread 入門：啟動、參數傳遞、join 與生命週期
// ============================================================================
// thread 建構後，callable 可能立刻在另一執行緒開始。thread 物件本身是 OS thread
// handle 的擁有者，但不擁有以 reference 傳入的外部資料。join() 等待完成並回收；
// detach() 放棄 join 權，生命週期很難證明，應避免用於一般 application code。
// 可 join 的 std::thread 在 destructor 執行時會 std::terminate，因此所有控制流
// （包含 exception）都必須 join；C++20 std::jthread 會自動 join，見第 07 章。
//
// 建立/排程 thread 的成本遠高於一次加法。平行化前要有足夠工作量，並避免多個
// thread 寫同一物件。不同 thread 寫不同 vector 元素在元素不重疊時是安全的；
// vector 結構本身不可同時 push_back。

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

void basic_demo()
{
    int answer = 0;
    std::thread worker([&answer] {
        // 主執行緒在 join 前不讀 answer，所以沒有同時存取。
        answer = 6 * 7;
    });
    assert(worker.joinable());
    worker.join();  // join 建立 happens-before；之後可安全看到 42。
    assert(answer == 42);
    assert(!worker.joinable());
}

// ----------------------------------------------------------------------------
// LeetCode 977：Squares of a Sorted Array（雙執行緒教學版）
// ----------------------------------------------------------------------------
// 兩執行緒各寫 result 的不重疊一半，完成後排序。重點是 ownership partition；
// 最佳單執行緒雙指標可 O(n)，本例刻意展示如何分割寫入，不宣稱更快。
std::vector<int> sorted_squares(const std::vector<int>& numbers)
{
    std::vector<int> result(numbers.size());
    const std::size_t middle = numbers.size() / 2U;

    auto square_range = [&numbers, &result](std::size_t first, std::size_t last) {
        for (std::size_t index = first; index < last; ++index) {
            result.at(index) = numbers.at(index) * numbers.at(index);
        }
    };

    std::thread left(square_range, 0U, middle);
    std::thread right(square_range, middle, numbers.size());
    left.join();
    right.join();
    std::ranges::sort(result);
    return result;
}

void leetcode_demo()
{
    assert((sorted_squares({-4, -1, 0, 3, 10}) ==
            std::vector<int>{0, 1, 9, 16, 100}));
}

// ----------------------------------------------------------------------------
// 實務：兩個互不相依資料分片各自加總
// ----------------------------------------------------------------------------
long long parallel_sum(const std::vector<int>& values)
{
    const std::size_t middle = values.size() / 2U;
    long long left_sum = 0;
    long long right_sum = 0;
    std::thread left([&] {
        left_sum = std::accumulate(values.begin(),
                                   values.begin() + static_cast<std::ptrdiff_t>(middle),
                                   0LL);
    });
    std::thread right([&] {
        right_sum = std::accumulate(
            values.begin() + static_cast<std::ptrdiff_t>(middle), values.end(), 0LL);
    });
    left.join();
    right.join();
    return left_sum + right_sum;
}

void practical_demo()
{
    assert(parallel_sum({1, 2, 3, 4, 5, 6}) == 21LL);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "thread：啟動、join 與資料分片測試通過\n";
}

// 【陷阱】把區域 reference 交給 detached thread，函式回傳後會懸空。
// 【陷阱】thread 只能 move、不可 copy；join/detach 只能成功執行一次。
// 【面試】join 提供什麼同步保證？worker 結束前的副作用 happens-before join 返回。
// 【練習】用 RAII wrapper 保證 exception 發生時也 join，之後比較 jthread。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_hello_thread.cpp' -o '/tmp/codex_cpp_C_MultiThread_01_hello_thread' && '/tmp/codex_cpp_C_MultiThread_01_hello_thread'
//
// === 預期輸出（節錄）===
// thread：啟動、join 與資料分片測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
