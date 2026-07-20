// ============================================================================
// 課題 10：iterator operations - advance / distance / next / prev
// ============================================================================
//
// std::advance(it, n) 原地移動 it；std::next/prev 回傳移動後副本；std::distance 算距離。
// 複雜度取決於 category：
//   random-access (vector/deque/array) -> O(1)
//   bidirectional/forward (list/forward_list) -> O(|n|)
// forward/input iterator 不可負向 advance；做了是違反前置條件。
//
// Iterator 表示位置，不自帶 range 邊界。next(it, 100) 若越過 end 是 UB；C++20 ranges
// 有帶 bound 的 advance overload，更適合不可信距離。
// ============================================================================

#include <cassert>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

void basic_example()
{
    const std::vector<int> values{10, 20, 30, 40};
    assert(*std::next(values.begin(), 2) == 30);
    assert(*std::prev(values.end()) == 40);
    assert(std::distance(values.begin(), values.end()) == 4);

    const std::list<int> linked{1, 2, 3};
    auto it = linked.begin();
    std::advance(it, 2); // 合法但要做兩次 ++，不是 O(1)。
    assert(*it == 3);
    std::cout << "[基礎] iterator movement obeys category-specific complexity\n";
}

// LeetCode 876：Middle of the Linked List。slow/fast 只要求 forward iterator。
int middle_value(const std::forward_list<int>& values)
{
    auto slow = values.begin();
    auto fast = values.begin();
    while (fast != values.end()) {
        fast = std::next(fast);
        if (fast == values.end()) break;
        fast = std::next(fast);
        slow = std::next(slow);
    }
    return *slow;
}

void leetcode_876_example()
{
    assert(middle_value({1, 2, 3, 4, 5}) == 3);
    assert(middle_value({1, 2, 3, 4, 5, 6}) == 4);
    std::cout << "[LeetCode 876] slow/fast iterators found the middle\n";
}

// 實務：從 vector 取一頁資料。先 clamp 範圍，避免 iterator 越界。
std::vector<int> page(const std::vector<int>& rows, std::size_t offset, std::size_t limit)
{
    if (offset >= rows.size()) return {};
    const std::size_t count = std::min(limit, rows.size() - offset);
    const auto first = std::next(rows.begin(), static_cast<std::ptrdiff_t>(offset));
    const auto last = std::next(first, static_cast<std::ptrdiff_t>(count));
    return {first, last};
}

void practical_example()
{
    const std::vector<int> rows{100, 101, 102, 103, 104};
    assert((page(rows, 2, 2) == std::vector<int>{102, 103}));
    assert((page(rows, 4, 20) == std::vector<int>{104}));
    assert(page(rows, 9, 2).empty());
    std::cout << "[實務] bounded iterator arithmetic produced safe pagination\n";
}

int main()
{
    basic_example();
    leetcode_876_example();
    practical_example();
}

// 易錯：iterator 不知道 range 邊界；next/advance 超過 end 不會自動 clamp，而是 UB。
// 面試自問：為何 distance(list.begin(), list.end()) 放在每次 loop 會造成 O(N²)？
// 練習：用 std::ranges::advance(it, n, end) 重寫分頁邊界（C++20）。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_iterator_operations.cpp' -o '/tmp/codex_cpp_C_Iterator_10_iterator_operations' && '/tmp/codex_cpp_C_Iterator_10_iterator_operations'
//
// === 預期輸出（節錄）===
// [實務] bounded iterator arithmetic produced safe pagination
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
