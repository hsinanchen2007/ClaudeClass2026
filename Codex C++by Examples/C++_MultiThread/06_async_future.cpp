// ============================================================================
// async / future / promise：一次性結果與 exception 傳遞
// ============================================================================
// future<T> 是 shared state 的讀取端，get() 等待並取走一次結果；第二次 get 無效。
// promise<T> 是寫入端，可 set_value 或 set_exception。std::async 建立 shared state，
// 但若不指定 policy，實作可選 async 或 deferred；需要真正並行時明寫 launch::async。
//
// async callable 丟出的 exception 會保存並於 future::get 重新拋出。若 async future
// 是最後一個 handle，其 destructor 在某些條件會等待 task，不能把它當 fire-and-forget。

#include <algorithm>
#include <cassert>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void basic_demo()
{
    auto answer = std::async(std::launch::async, [] { return 6 * 7; });
    assert(answer.valid());
    // get 會等待並消耗 unique future；NDEBUG 會移除整個 assert expression，故先取值。
    [[maybe_unused]] const int answer_value = answer.get();
    assert(answer_value == 42);
    assert(!answer.valid());

    auto failure = std::async(std::launch::async, []() -> int {
        throw std::runtime_error("worker failed");
    });
    try {
        static_cast<void>(failure.get());
        assert(false);
    } catch (const std::runtime_error& error) {
        assert(std::string{error.what()} == "worker failed");
    }
}

// ----------------------------------------------------------------------------
// LeetCode 34：Find First and Last Position（兩個獨立 binary searches）
// ----------------------------------------------------------------------------
// lower_bound 與 upper_bound 可獨立計算，分別交給 async。每個 O(log n)；小輸入不會
// 比單 thread 快，本例教的是 result/exception channel。
std::vector<int> search_range(const std::vector<int>& numbers, int target)
{
    auto first_task = std::async(std::launch::async, [&numbers, target] {
        return std::lower_bound(numbers.begin(), numbers.end(), target);
    });
    auto after_task = std::async(std::launch::async, [&numbers, target] {
        return std::upper_bound(numbers.begin(), numbers.end(), target);
    });

    const auto first = first_task.get();
    const auto after = after_task.get();
    if (first == numbers.end() || *first != target) {
        return {-1, -1};
    }
    return {static_cast<int>(std::distance(numbers.begin(), first)),
            static_cast<int>(std::distance(numbers.begin(), after) - 1)};
}

void leetcode_demo()
{
    // search_range 內會建立 async tasks；先執行，assert 只比對回傳結果。
    [[maybe_unused]] const auto found = search_range({5, 7, 7, 8, 8, 10}, 8);
    [[maybe_unused]] const auto missing = search_range({5, 7, 7, 8, 8, 10}, 6);
    assert((found == std::vector<int>{3, 4}));
    assert((missing == std::vector<int>{-1, -1}));
}

// ----------------------------------------------------------------------------
// 實務：同時讀兩個獨立設定來源，再合併
// ----------------------------------------------------------------------------
struct Configuration {
    std::string endpoint;
    int timeout_ms;
};

Configuration load_configuration()
{
    auto endpoint = std::async(std::launch::async, [] {
        return std::string{"https://service.local"};
    });
    auto timeout = std::async(std::launch::async, [] { return 2'000; });
    return {endpoint.get(), timeout.get()};
}

void practical_demo()
{
    const Configuration config = load_configuration();
    assert(config.endpoint == "https://service.local");
    assert(config.timeout_ms == 2'000);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "future：結果、exception 與 async 合併測試通過\n";
}

// 【陷阱】不指定 launch policy 就不能保證另開 thread；deferred 到 get 才在呼叫端跑。
// 【陷阱】future 不是可複製 broadcast；多讀者要 shared_future。
// 【面試】promise 被銷毀但沒 set 時，future.get 會收到 broken_promise future_error。
// 【練習】用 promise/future 手寫 basic_demo，不使用 async。
