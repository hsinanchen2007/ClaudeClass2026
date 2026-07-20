// ============================================================================
// 課題 5：Random-access iterator - O(1) 跳躍、距離與排序
// ============================================================================
//
// RandomAccessIterator 支援 `it+n`, `it[n]`, subtraction/order comparison，vector/deque/array
// 提供。std::sort/lower_bound 的高效實作依此能力；list 不能 std::sort（用 list::sort）。
// `end-begin` O(1)，但只有同一 range（或 one-past）的 iterator 可相減/比較，跨容器是 UB。
//
// C++20 contiguous_iterator（vector/array/string）更進一步保證 memory contiguous；deque 是
// random access 但不 contiguous。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

void basic_example()
{
    const std::vector<int> values{10, 20, 30, 40};
    const auto begin = values.begin();
    assert(begin[2] == 30);
    assert(*(begin + 3) == 40);
    assert(values.end() - begin == 4);
    std::cout << "[基礎] vector iterator supports indexing and O(1) distance\n";
}

// LeetCode 704：binary search 用 iterator arithmetic，O(log N)。
int search(const std::vector<int>& nums, int target)
{
    auto first = nums.begin();
    auto last = nums.end();
    while (first < last) {
        const auto middle = first + (last - first) / 2;
        if (*middle < target) first = middle + 1;
        else last = middle;
    }
    return first != nums.end() && *first == target ? static_cast<int>(first - nums.begin()) : -1;
}

void leetcode_704_example()
{
    assert(search({-1, 0, 3, 5, 9, 12}, 9) == 4);
    assert(search({-1, 0, 3, 5, 9, 12}, 2) == -1);
    std::cout << "[LeetCode 704] iterator binary search returns 4/-1\n";
}

// 實務：partition sorted records at lower_bound insertion point。
void practical_example()
{
    std::vector<int> ids{10, 20, 40};
    const auto position = std::lower_bound(ids.begin(), ids.end(), 30);
    ids.insert(position, 30); // position 在 insert 後不可再用；vector 可能 reallocate。
    assert((ids == std::vector<int>{10, 20, 30, 40}));
    std::cout << "[實務] lower_bound found sorted insertion position\n";
}

int main() { basic_example(); leetcode_704_example(); practical_example(); }

// 易錯與面試：只能相減/排序同一 range 的 iterators；即使兩個 vector 位址看似相近，
// 跨容器比較仍沒有合法語意。面試要能說出 deque 是 random-access 但不是 contiguous。
//
// 速查：`last-first`、`first+n`、`first[n]` 都要求合法範圍，能力強不代表自動 bounds check。
// `std::sort` 要 random access；`std::stable_sort` 也一樣但承諾 equal keys 的相對順序。
// `std::lower_bound` 在 vector 是 O(logN) iterator movements；在 list 比較雖少，走訪仍 O(N)。
//
// 實務選擇：需要送入 C API/GPU buffer 時還要 contiguous，不能把 deque 的 random-access
// 誤當連續記憶體保證；用 vector/array/span 並明確維護 lifetime。
// 若資料本身未排序，binary-search iterator arithmetic 再漂亮也不正確；前置條件優先。
// 練習：證明 deque iterator random-access，但 &deque[0]+n 不保證跨 block contiguous。
