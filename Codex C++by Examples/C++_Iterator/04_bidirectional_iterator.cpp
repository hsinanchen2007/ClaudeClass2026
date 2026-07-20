// ============================================================================
// 課題 4：Bidirectional iterator - ++ 與 --
// ============================================================================
//
// BidirectionalIterator 在 forward 基礎加 --，list/set/map 提供；仍不能 O(1) `it+n` 或
// 相減。`--end()` 在 non-empty bidirectional range 得最後元素；不可 `--begin()`。
// reverse_iterator 正是把 ++ 轉為 underlying --（第 6 課）。
//
// list 中 insert/erase 通常不使其他 iterators 失效，適合需要 stable positions 的資料結構。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include <vector>

void basic_example()
{
    const std::list<int> values{1, 2, 3};
    auto iterator = values.end();
    --iterator;
    assert(*iterator == 3);
    --iterator;
    assert(*iterator == 2);
    std::cout << "[基礎] list iterator walks backward from end\n";
}

// LeetCode 234：Palindrome Linked List；用 bidirectional list 由兩端比較。
bool is_palindrome(const std::list<int>& values)
{
    auto left = values.begin();
    auto right = values.end();
    for (std::size_t compared = 0U; compared < values.size() / 2U; ++compared) {
        --right;
        if (*left != *right) return false;
        ++left;
    }
    return true;
}

void leetcode_234_example()
{
    assert(is_palindrome({1, 2, 2, 1}));
    assert(!is_palindrome({1, 2}));
    std::cout << "[LeetCode 234] bidirectional endpoints detect palindrome\n";
}

// 實務：stable iterator 保存 queue 中 job，插入其他 jobs 後仍有效。
void practical_example()
{
    std::list<int> jobs{1, 3};
    auto tracked = jobs.begin();
    ++tracked; // 指 job 3。
    jobs.insert(tracked, 2);
    assert(*tracked == 3);
    std::vector<int> snapshot(jobs.begin(), jobs.end());
    assert((snapshot == std::vector<int>{1, 2, 3}));
    std::cout << "[實務] list insertion preserved tracked iterator\n";
}

int main() { basic_example(); leetcode_234_example(); practical_example(); }

// 易錯與面試：non-empty range 才能 `--end()`，而 `--begin()` 永遠非法。bidirectional
// 只有逐步 --，不代表 `it - 3` 或 iterator subtraction；這正是它和 random access 的界線。
//
// 工作選型：需要 stable node position 與中間 splice 時 list 有價值；若只是反向遍歷，
// vector 本身已有 reverse_iterator，通常有更好的 cache locality。不要因支援 -- 就選 list。
// 對 set/map 應優先用成員 find/lower_bound，因它們能沿樹 O(logN)，自由 algorithm 只會走訪。
// 練習：比較 vector insert 對 tracked iterator 的 invalidation。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_bidirectional_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_04_bidirectional_iterator' && '/tmp/codex_cpp_C_Iterator_04_bidirectional_iterator'
//
// === 預期輸出（節錄）===
// [實務] list insertion preserved tracked iterator
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
