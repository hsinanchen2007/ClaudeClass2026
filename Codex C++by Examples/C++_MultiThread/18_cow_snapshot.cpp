// ============================================================================
// Copy-on-Write Snapshot：讀者讀 immutable 版本，寫者複製後原子發布
// ============================================================================
// C++20 支援 atomic<shared_ptr<T>>。讀者 atomic load 取得 shared ownership，之後不需
// 鎖即可讀 immutable object；寫者 load 舊版、copy、修改，再 compare_exchange 發布。
// 優點是讀路徑簡單且 snapshot 一致；代價是每次寫 O(n) copy、reference counting、
// 舊版由慢讀者延長生命。適合讀極多、寫少、資料量可接受的 config/routing table。

#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <utility>
#include <vector>

template <class T>
class CowSnapshot {
public:
    explicit CowSnapshot(T initial)
        : current_(std::make_shared<const T>(std::move(initial))) {}

    std::shared_ptr<const T> read() const
    {
        return current_.load(std::memory_order_acquire);
    }

    template <class Mutator>
    void update(Mutator mutate)
    {
        auto expected = current_.load(std::memory_order_acquire);
        for (;;) {
            auto next_mutable = std::make_shared<T>(*expected);
            mutate(*next_mutable);
            std::shared_ptr<const T> next = std::move(next_mutable);
            if (current_.compare_exchange_weak(expected, next,
                                               std::memory_order_release,
                                               std::memory_order_acquire)) {
                return;
            }
            // CAS 失敗時 expected 已更新；必須基於新版重做 mutator。
        }
    }

private:
    std::atomic<std::shared_ptr<const T>> current_;
};

void basic_demo()
{
    CowSnapshot<std::vector<int>> values({1, 2});
    const auto old = values.read();
    values.update([](std::vector<int>& next) { next.push_back(3); });
    const auto current = values.read();
    assert(((*old) == std::vector<int>{1, 2}));
    assert(((*current) == std::vector<int>{1, 2, 3}));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 303. Range Sum Query - Immutable（區域和檢索）
// 題目：建構後重複查詢 nums[left..right] 的總和；例如 [-2,0,3,-5,2,-1] 的 [0,2] 為 1、[2,5] 為 -1。
// 為何使用本章主題：本例刻意改成 COW snapshot 並加 replace 擴充，讓每次 query 讀一致版本；它用 accumulate O(R)，不是原題 prefix-sum O(1) 最佳解。
// 思路：1. read 取得 shared_ptr snapshot。2. 驗證左右邊界。3. 在該 immutable 版本累加閉區間。4. replace 複製並 CAS 發布新版。
// 複雜度：區間長 R 的查詢 O(R)、replace 複製 O(N)，每個活躍 snapshot 可能保留 O(N) 空間。
// 易錯點：邊界不能只靠 assert；mutator 可能因 CAS 失敗重跑且不可含外部副作用，總和也要防 int overflow。
// -----------------------------------------------------------------------------
class NumArray {
public:
    explicit NumArray(std::vector<int> values) : values_(std::move(values)) {}

    int sum_range(std::size_t left, std::size_t right) const
    {
        const auto snapshot = values_.read();
        assert(left <= right && right < snapshot->size());
        return std::accumulate(
            snapshot->begin() + static_cast<std::ptrdiff_t>(left),
            snapshot->begin() + static_cast<std::ptrdiff_t>(right + 1U), 0);
    }

    void replace(std::size_t index, int value)
    {
        values_.update([index, value](std::vector<int>& next) { next.at(index) = value; });
    }

private:
    CowSnapshot<std::vector<int>> values_;
};

void leetcode_demo()
{
    NumArray numbers({-2, 0, 3, -5, 2, -1});
    assert(numbers.sum_range(0U, 2U) == 1);
    assert(numbers.sum_range(2U, 5U) == -1);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】無鎖讀取的路由表版本切換
// 情境：reader 正在使用只含 api-v1 的舊路由時，writer 加入 api-v2；舊 request 必須繼續看到穩定舊版，新 request 讀到新版。
// 為何使用本章主題：atomic<shared_ptr<const T>> 讓 reader 取得 owning immutable snapshot 後不持鎖，writer 則 copy-on-write 後原子發布。
// 設計：1. reader load 並保留舊 shared_ptr。2. writer 複製目前 routes。3. 修改副本並 CAS。4. 驗證舊/新 owner 各見一致版本。
// 成本：讀取有 atomic shared_ptr/refcount 成本；每次更新 O(N) 複製，慢 reader 會延長舊版記憶體生命。
// 上線注意：多 writer 可能反覆重試而飢餓，mutator 必須可重放；atomic shared_ptr 不保證 lock-free，需量測讀寫比例與峰值記憶體。
// -----------------------------------------------------------------------------
void practical_demo()
{
    CowSnapshot<std::vector<std::string>> routes({"api-v1"});
    auto reader_snapshot = routes.read();
    std::thread writer([&] {
        routes.update([](auto& next) { next.push_back("api-v2"); });
    });
    writer.join();
    assert(reader_snapshot->size() == 1U);
    assert(routes.read()->size() == 2U);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "COW snapshot：immutable 版本與原子發布測試通過\n";
}

// 【陷阱】mutator 必須可重試且不可有外部一次性副作用，因 CAS 失敗會重跑。
// 【陷阱】atomic<shared_ptr> 操作本身可能非 lock-free；正確性不代表零成本。
// 【面試】COW 適合讀多寫少；寫多時 copy amplification 與舊版本滯留會失控。
// 【練習】加入 version number，讓讀者記錄自己讀到哪一版。

/*
 * 【教科書補充：copy-on-write snapshot 的重試語意】
 * - CAS 失敗會重做 mutator；因此 mutator 必須無外部副作用，否則 log/I/O/計數可能重複執行。
 * - 舊 snapshot 由 reader 的 shared_ptr 延長生命，換得 lock-free read，但更新頻繁時記憶體峰值可能上升。
 * - sum_range 需持續驗證兩端邊界；assert 消失後形成非法 iterator 不是可接受的錯誤處理。
 * - 多 writer 競爭可能反覆 CAS 失敗而飢餓；lock-free 不等於 wait-free 或公平。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '18_cow_snapshot.cpp' -o '/tmp/codex_cpp_C_MultiThread_18_cow_snapshot' && '/tmp/codex_cpp_C_MultiThread_18_cow_snapshot'
//
// === 預期輸出（節錄）===
// COW snapshot：immutable 版本與原子發布測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
