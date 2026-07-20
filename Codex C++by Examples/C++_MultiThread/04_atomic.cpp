// ============================================================================
// atomic：單一物件的不可分割操作，不等於整個演算法 thread-safe
// ============================================================================
// std::atomic<T> 提供 load/store/exchange/fetch_add/compare_exchange。lock-free 與否依
// 型別/平台，可用 is_lock_free 查詢但不應假設。atomic 適合 counter、flag、state；
// 多欄位 invariant 通常仍需 mutex 或封裝成單一不可分割狀態。
//
// memory_order_relaxed 只保證該 atomic 的修改順序與不可分割性，不發布其他資料；
// 預設 seq_cst 最容易推理。release/acquire 與 memory model 在第 11 章詳談。

#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

void basic_demo()
{
    std::atomic<int> next_id{100};
    const int first = next_id.fetch_add(1, std::memory_order_relaxed);
    const int second = next_id.fetch_add(1, std::memory_order_relaxed);
    assert(first == 100 && second == 101);

    int expected = 102;
    const bool changed = next_id.compare_exchange_strong(expected, 200);
    assert(changed && next_id.load() == 200);
}

// ----------------------------------------------------------------------------
// LeetCode 136：Single Number（平行 XOR 教學版）
// ----------------------------------------------------------------------------
// XOR 具結合/交換律，兩分片先算 thread-local，再以 atomic fetch_xor 合併。
// O(n) 時間、O(1) 共享狀態。實務上小輸入 thread overhead 會比單執行緒慢。
int single_number(const std::vector<int>& numbers)
{
    const std::size_t middle = numbers.size() / 2U;
    std::atomic<int> result{0};
    auto reduce = [&numbers, &result](std::size_t first, std::size_t last) {
        int local = 0;
        for (std::size_t index = first; index < last; ++index) {
            local ^= numbers.at(index);
        }
        result.fetch_xor(local, std::memory_order_relaxed);
    };
    std::thread left(reduce, 0U, middle);
    std::thread right(reduce, middle, numbers.size());
    left.join();
    right.join();
    return result.load(std::memory_order_relaxed);
}

void leetcode_demo()
{
    assert(single_number({4, 1, 2, 1, 2}) == 4);
}

// ----------------------------------------------------------------------------
// 實務：無重複流水號產生器
// ----------------------------------------------------------------------------
class Sequence {
public:
    explicit Sequence(unsigned long long first) : next_(first) {}

    unsigned long long acquire()
    {
        return next_.fetch_add(1ULL, std::memory_order_relaxed);
    }

private:
    std::atomic<unsigned long long> next_;
};

void practical_demo()
{
    Sequence sequence(1'000ULL);
    std::vector<unsigned long long> ids(4U);
    std::thread a([&] { ids.at(0) = sequence.acquire(); ids.at(1) = sequence.acquire(); });
    std::thread b([&] { ids.at(2) = sequence.acquire(); ids.at(3) = sequence.acquire(); });
    a.join();
    b.join();
    std::ranges::sort(ids);
    assert((ids == std::vector<unsigned long long>{1'000, 1'001, 1'002, 1'003}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "atomic：RMW、XOR reduce 與 sequence 測試通過\n";
}

// 【陷阱】if (!ready.load()) ready.store(true) 不是不可分割 check-then-set；用 CAS/exchange。
// 【陷阱】兩個 atomic 欄位各自正確，不代表兩者組合 invariant 是一致 snapshot。
// 【面試】compare_exchange 失敗時 expected 會被寫成目前值；weak 可 spurious fail。
// 【練習】用 compare_exchange_weak 迴圈實作 atomic max。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_atomic.cpp' -o '/tmp/codex_cpp_C_MultiThread_04_atomic' && '/tmp/codex_cpp_C_MultiThread_04_atomic'
//
// === 預期輸出（節錄）===
// atomic：RMW、XOR reduce 與 sequence 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
