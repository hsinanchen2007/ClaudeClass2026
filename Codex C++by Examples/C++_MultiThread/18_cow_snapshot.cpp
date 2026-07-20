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

// ----------------------------------------------------------------------------
// LeetCode 303：Range Sum Query - Immutable（snapshot 版本）
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// 實務：讀者持有舊 routing snapshot，writer 發布新版不破壞舊讀取
// ----------------------------------------------------------------------------
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
