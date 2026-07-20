// ============================================================================
// 課題 10：const member function 與 const correctness
// ============================================================================
//
// `int size() const` 的尾端 const 修飾隱含 this，表示函式不可修改一般 member，
// 因而可由 const object 呼叫。const correctness 讓「唯讀承諾」進入型別系統：
// compiler 可阻止查詢函式偷偷改 state，也讓 API 使用者知道呼叫是否有副作用。
//
// 可依 const qualification overload：non-const object 取得 `T&`，const object 取得
// `const T&`。`mutable` 允許 const function 修改不屬於 logical state 的 cache/mutex；
// 它不是逃避 const 的工具，必須確保外界觀察到的語意仍不變。
//
// 【面試】const object 只能呼叫 const member（constructor/destructor 除外）。
// 【陷阱】const member 回傳內部 non-const pointer/reference 會破壞唯讀承諾。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

class Notebook {
public:
    explicit Notebook(std::vector<std::string> pages) : pages_(std::move(pages)) {}

    std::string& at(std::size_t index) { return pages_.at(index); }
    const std::string& at(std::size_t index) const { return pages_.at(index); }
    std::size_t size() const { return pages_.size(); }

private:
    std::vector<std::string> pages_;
};

void basic_example()
{
    Notebook editable({"C++", "CUDA"});
    editable.at(0) = "Modern C++"; // non-const overload 回 string&。
    const Notebook& read_only = editable;
    assert(read_only.at(0) == "Modern C++" && read_only.size() == 2U);
    std::cout << "[基礎] const Notebook 只能唯讀 pages\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 303. Range Sum Query - Immutable（區域和檢索：不可變）
// 題目：預處理整數陣列後，多次查 [left,right] 總和；例如 [-2,0,3,-5,2,-1] 的 [0,2] 為 1。
// 為何使用本章主題：prefix_ 建構後不變，sum_range 標 const，讓同一唯讀 NumArray 可重複查詢。
// 思路：建立前置零的 prefix；prefix[i+1] 累加 nums[i]；查詢以 prefix[right+1]-prefix[left]。
// 複雜度：建構時間/空間 O(N)，每次查詢 O(1)，N 為元素數。
// 易錯點：left<=right 且 right<N；right+1 要防溢位；int 前綴和可能溢位，正式版本宜用較寬型別。
// -----------------------------------------------------------------------------
class NumArray {
public:
    explicit NumArray(const std::vector<int>& nums) : prefix_(nums.size() + 1U, 0)
    {
        for (std::size_t index = 0U; index < nums.size(); ++index) {
            prefix_.at(index + 1U) = prefix_.at(index) + nums.at(index);
        }
    }

    int sum_range(std::size_t left, std::size_t right) const
    {
        return prefix_.at(right + 1U) - prefix_.at(left);
    }

private:
    std::vector<int> prefix_;
};

void leetcode_303_example()
{
    const NumArray sums({-2, 0, 3, -5, 2, -1});
    assert(sums.sum_range(0U, 2U) == 1);
    assert(sums.sum_range(2U, 5U) == -1);
    assert(sums.sum_range(0U, 5U) == -3);
    std::cout << "[LeetCode 303] const sum_range 可安全重複查詢\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】封包 checksum 的 logical-const 快取
// 情境：不可變 Packet 可能多次查 checksum，首次線性計算後要重用結果而不改變外界看到的 payload 語意。
// 為何使用本章主題：checksum() 可維持 const API，mutable 只允許更新內部 cache，避免把快取暴露成 domain state。
// 設計：cached_ 為 false 時 accumulate bytes；保存結果並設旗標；後續直接回 cached_checksum_。
// 成本：首次時間 O(N)、後續 O(1)，額外空間 O(1)，N 為 byte 數。
// 上線注意：目前 mutable cache 有 data race；多執行緒需同步，總和要防溢位，若 payload 可改則必須失效 cache。
// -----------------------------------------------------------------------------
class Packet {
public:
    explicit Packet(std::vector<int> bytes) : bytes_(std::move(bytes)) {}

    int checksum() const
    {
        if (!cached_) {
            cached_checksum_ = std::accumulate(bytes_.begin(), bytes_.end(), 0);
            cached_ = true;
        }
        return cached_checksum_;
    }

private:
    std::vector<int> bytes_;
    mutable bool cached_ = false;
    mutable int cached_checksum_ = 0;
};

void practical_example()
{
    const Packet packet({1, 2, 3, 4});
    assert(packet.checksum() == 10);
    assert(packet.checksum() == 10); // 第二次使用 cache，observable result 相同。
    std::cout << "[實務] const checksum=10，cache 屬內部實作細節\n";
}

int main()
{
    basic_example();
    leetcode_303_example();
    practical_example();
}

// 練習：故意把 const at() 改回 std::string&，觀察如何由 const object 改到內部資料。
// 複雜度：const qualification 沒有 runtime 成本；at/find 的 Big-O 仍由內部 container 決定。
// 生命週期：const reference getter 仍是 borrow，不能超過 object 或會使 container reallocate 的修改。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_ConstMember.cpp' -o '/tmp/codex_cpp_C_OOP_10_ConstMember' && '/tmp/codex_cpp_C_OOP_10_ConstMember'
//
// === 預期輸出（節錄）===
// [實務] const checksum=10，cache 屬內部實作細節
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
