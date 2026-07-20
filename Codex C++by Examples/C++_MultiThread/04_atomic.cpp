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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 136. Single Number（只出現一次的數字）
// 題目：陣列中除一個數只出現一次外，其餘都恰出現兩次，要求找出單獨值；例如 [4,1,2,1,2] 得 4。
// 為何使用本章主題：XOR 可交換且可結合，兩個 thread 先做 local reduction，再以 relaxed atomic fetch_xor 無遺失地合併。
// 思路：1. 將陣列切兩半。2. 每個 worker 在 local int XOR。3. 各自對共享 atomic 做一次 fetch_xor。4. join 後 load。
// 複雜度：總時間 O(N)、額外演算法空間 O(1)，另有兩個 thread 與兩次 atomic RMW 成本。
// 易錯點：relaxed 只適用於此獨立 reduction；join 提供完成同步，小輸入不會比單執行緒 XOR 快，題目資料也須符合配對契約。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】程序內無重複流水號
// 情境：多個 worker 同時為工作配置從 1000 起的流水號，要求每次取得唯一舊值，不要求號碼反映 thread 執行順序。
// 為何使用本章主題：fetch_add 是單一 atomic read-modify-write，比 mutex 保護單一 counter 更直接；不發布其他資料所以可用 relaxed。
// 設計：1. constructor 設定 next。2. acquire 執行 fetch_add(1)。3. 各 thread 保存回傳舊值。4. join 後驗證唯一連續集合。
// 成本：每次配置 O(1) 但所有 thread 爭用同一 cache line；物件空間 O(1)。
// 上線注意：unsigned counter 回繞會重新產生舊 ID，必須定義耗盡策略；跨程序/重啟唯一性需資料庫、租號段或持久化機制。
// -----------------------------------------------------------------------------
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
