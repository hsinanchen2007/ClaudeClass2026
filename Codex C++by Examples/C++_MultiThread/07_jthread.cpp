// ============================================================================
// jthread / stop_token：RAII join 與合作式取消
// ============================================================================
// std::jthread destructor 會 request_stop() 再 join()，降低遺漏 join 的 terminate 風險。
// 若 callable 第一參數可接 stop_token，jthread 會自動傳入自己的 token。取消是合作式：
// request_stop 不會強制殺 thread；worker 必須定期檢查 token 或使用 stop-aware wait。
// stop_token 可複製、只觀察；stop_source 才能 request_stop。

#include <atomic>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <vector>

namespace {

// 教材測試必須在 -DNDEBUG 下照常執行，不能用 assert 承擔控制流程。
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

void basic_demo()
{
    std::atomic<int> iterations{0};
    std::jthread worker([&iterations](std::stop_token stop) {
        while (!stop.stop_requested()) {
            iterations.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::yield();
        }
    });
    while (iterations.load(std::memory_order_relaxed) < 10) {
        std::this_thread::yield();
    }

    // 【契約】request_stop 是讓 worker 離開迴圈的必要副作用，必須在 release 也執行。
    const bool first_stop_request = worker.request_stop();
    expect(first_stop_request, "第一次停止請求應成功");
    expect(!worker.request_stop(), "重複停止請求應回傳 false");
    worker.join();
    expect(iterations.load(std::memory_order_relaxed) >= 10,
           "worker 應至少完成十次迭代");
}

// ----------------------------------------------------------------------------
// LeetCode 1480：Running Sum（jthread 作為 RAII worker）
// ----------------------------------------------------------------------------
// prefix sum 本身有前後依賴，這裡不硬做假平行化；以 jthread 安全承接工作並在 scope
// 內 join。演算法 O(n) 時間、O(n) 輸出。
std::vector<int> running_sum(const std::vector<int>& numbers)
{
    std::vector<int> result(numbers.size());
    std::jthread worker([&] {
        int total = 0;
        for (std::size_t index = 0; index < numbers.size(); ++index) {
            total += numbers.at(index);
            result.at(index) = total;
        }
    });
    worker.join();
    return result;
}

void leetcode_demo()
{
    expect((running_sum({1, 2, 3, 4}) == std::vector<int>{1, 3, 6, 10}),
           "running_sum 結果錯誤");
    expect(running_sum({}).empty(), "空輸入應產生空輸出");
}

// ----------------------------------------------------------------------------
// 實務：可取消搜尋；找到目標後共享 stop_source 通知其他分片
// ----------------------------------------------------------------------------
int cancellable_find(const std::vector<int>& values, int target)
{
    std::stop_source cancellation;
    std::atomic<int> found{-1};
    const std::size_t middle = values.size() / 2U;
    auto search = [&](std::stop_token token, std::size_t first, std::size_t last) {
        for (std::size_t index = first; index < last && !token.stop_requested(); ++index) {
            if (values.at(index) == target) {
                int expected = -1;
                static_cast<void>(found.compare_exchange_strong(
                    expected, static_cast<int>(index), std::memory_order_relaxed));
                cancellation.request_stop();
                return;
            }
        }
    };

    const std::stop_token token = cancellation.get_token();
    std::jthread left([&] { search(token, 0U, middle); });
    std::jthread right([&] { search(token, middle, values.size()); });
    left.join();
    right.join();
    return found.load(std::memory_order_relaxed);
}

void practical_demo()
{
    const int index = cancellable_find({10, 20, 30, 40, 50}, 40);
    expect(index == 3, "應找到右半部的目標");
    expect(cancellable_find({1, 2, 3}, 1) == 0, "應找到第一個元素");
    expect(cancellable_find({1, 2, 3}, 9) == -1, "缺少目標時應回傳 -1");
    expect(cancellable_find({}, 9) == -1, "空輸入不應找到目標");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "jthread：RAII join、取消與搜尋測試通過\n";
}

// 【陷阱】request_stop 回 false 可能表示已有人請求，不表示 worker 已停止。
// 【陷阱】worker 若卡在不支援 stop 的 blocking I/O，token 本身無法把它喚醒。
// 【面試】jthread destructor 順序為 request_stop 再 join，但 shared state 必須比 jthread
//         活得久；類別 member 宣告順序要讓 jthread 先被銷毀。
// 【練習】用 condition_variable_any::wait(lock, token, predicate) 寫 stop-aware queue。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_jthread.cpp' -o '/tmp/codex_cpp_C_MultiThread_07_jthread' && '/tmp/codex_cpp_C_MultiThread_07_jthread'
//
// === 預期輸出（節錄）===
// jthread：RAII join、取消與搜尋測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
