// ============================================================================
// 課題 0：Iterator 與 half-open range [first,last)
// ============================================================================
//
// Iterator 把 traversal 與 container 分離，algorithm 只要求 operations/category，不必知道
// vector/list/stream。`begin` 指第一元素，`end` 是 one-past sentinel，不可 dereference。
// Empty range 滿足 begin==end；half-open ranges 可無重疊切割且 distance 就是元素數。
//
// Iterator 可能失效：container erase/reallocation、object destruction、temporary range 都會
// 影響。拿到 iterator 後先比 end，再 dereference。C++20 ranges 可把 iterator+sentinel
// 包成 range，但 lifetime 規則仍存在。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

void basic_example()
{
    std::vector<int> values{10, 20, 30};
    const auto found = std::find(values.begin(), values.end(), 20);
    assert(found != values.end() && *found == 20);
    assert(std::distance(values.begin(), values.end()) == 3);
    std::cout << "[基礎] find returns iterator; end is checked before dereference\n";
}

// LeetCode 704：Binary Search；lower_bound 回第一個 >= target 的 iterator。
int search(const std::vector<int>& nums, int target)
{
    const auto found = std::lower_bound(nums.begin(), nums.end(), target);
    return found != nums.end() && *found == target ? static_cast<int>(found - nums.begin()) : -1;
}

void leetcode_704_example()
{
    assert(search({-1, 0, 3, 5, 9, 12}, 9) == 4);
    assert(search({-1, 0, 3, 5, 9, 12}, 2) == -1);
    std::cout << "[LeetCode 704] lower_bound iterator yields index 4/miss\n";
}

// 實務：同一 function 接任意 iterators，計算 [first,last) 中正值數。
template<class Iterator>
std::size_t positive_count(Iterator first, Iterator last)
{
    return static_cast<std::size_t>(std::count_if(first, last, [](int value) { return value > 0; }));
}

void practical_example()
{
    const std::vector<int> values{-1, 0, 2, 3};
    assert(positive_count(values.begin(), values.end()) == 2U);
    std::cout << "[實務] generic iterator range counts two positive values\n";
}

int main() { basic_example(); leetcode_704_example(); practical_example(); }

// 易錯與面試：`end()` 是 sentinel，不是最後元素；先檢查 found != end 才能 `*found`。
// 面試常追問 iterator 何時 dangling：owner 析構、vector reallocation、erase 與 temporary
// lifetime 都可能造成；正確答案必須同時指出「依容器與 operation 查規格」。
//
// 速查：
//   `*it`      讀/寫目前元素（是否可寫取決於 iterator/reference constness）。
//   `++it`     前進；通常優先 pre-increment，避免舊值副本。
//   `it == end` 判斷是否走完，不可用元素值充當 sentinel。
//   `std::begin(x)` 可同時支援容器與 C array，是泛型 helper 的好入口。
//
// 工作設計：iterator pair 不擁有資料，呼叫期間 owner 必須活著。若 API 要延後執行，
// 應傳 owning container、shared ownership，或明確保證 lifetime，而非偷偷保存 iterators。
// 練習：把 temporary vector 的 begin 存起來後再用，說明 dangling 的原因（不要執行 UB）。
