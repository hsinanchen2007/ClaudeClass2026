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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 34. Find First and Last Position of Element in Sorted Array（尋找元素的首尾位置）
// 題目：在升冪陣列找 target 的第一與最後 index，找不到回 [-1,-1]；例如 [5,7,7,8,8,10] 找 8 得 [3,4]。
// 為何使用本章主題：lower_bound 與 upper_bound 彼此獨立，教學上以兩個 launch::async future 傳結果/例外；小型二分搜尋不會因此更快。
// 思路：1. 非同步求第一個 >=target。2. 同時求第一個 >target。3. get 兩個 iterator。4. 驗證命中後轉成 [first,after-1]。
// 複雜度：總工作 O(log N)、額外共享狀態 O(1)，但包含兩個 async task 的啟動、排程與同步成本。
// 易錯點：必須明寫 launch::async 才保證非 deferred；numbers 要活到兩個 future 完成，distance 轉 int 也需符合題目大小範圍。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】合併兩個獨立設定來源
// 情境：服務啟動時可同時讀 endpoint 與 timeout 來源，兩者都成功後才組成 Configuration。
// 為何使用本章主題：兩個 async future 表達獨立的一次性結果，get 會等待並把 loader exception 傳回組裝端，比手動 thread/promise 簡潔。
// 設計：1. 以 launch::async 啟動 endpoint loader。2. 啟動 timeout loader。3. 各 get 一次。4. 以兩個值建構 config。
// 成本：wall latency 接近較慢來源加排程成本；另有兩個 async shared states，實際 I/O 成本由 loader 決定。
// 上線注意：需要總 timeout、取消與部分失敗政策；future get 次序不應讓另一工作失去回收，敏感設定也不可直接寫入錯誤日誌。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_async_future.cpp' -o '/tmp/codex_cpp_C_MultiThread_06_async_future' && '/tmp/codex_cpp_C_MultiThread_06_async_future'
//
// === 預期輸出（節錄）===
// future：結果、exception 與 async 合併測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
