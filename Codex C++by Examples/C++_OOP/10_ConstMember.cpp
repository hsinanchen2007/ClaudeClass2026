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

// LeetCode 303：Range Sum Query - Immutable。
// 建構後 prefix_ 不再改；sum_range 是純查詢，所以明確標 const。
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

// 實務案例：logical const cache。計算 checksum 不改 payload 語意，但可快取結果。
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
