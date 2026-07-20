// ============================================================================
// 課題 3：Forward iterator - 可重複 multi-pass 的單向 iterator
// ============================================================================
//
// ForwardIterator 比 input 多 multi-pass guarantee：複製的 iterators 可各自推進，range 可
// 重走；仍只有 ++，不能 -- 或 +n。forward_list/unordered containers 提供 forward iterator。
// `std::distance` 對它是 O(N)，反覆在 loop 內計算會變 O(N²)。
//
// forward_list 沒 size（歷史/complexity 設計）且 erase_after/insert_after 以「前一位置」操作。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <forward_list>
#include <iostream>
#include <iterator>

void basic_example()
{
    const std::forward_list<int> values{1, 2, 3};
    auto first = values.begin();
    auto copy = first;
    ++first;
    assert(*first == 2 && *copy == 1); // multi-pass copies retain independent positions。
    assert(std::distance(values.begin(), values.end()) == 3);
    std::cout << "[基礎] forward iterator copies support independent passes\n";
}

// LeetCode 876：Middle of Linked List；slow/fast 都只需 forward increments。
int middle_value(const std::forward_list<int>& values)
{
    auto slow = values.begin();
    auto fast = values.begin();
    while (fast != values.end()) {
        ++fast;
        if (fast == values.end()) break;
        ++fast;
        ++slow;
    }
    return *slow;
}

void leetcode_876_example()
{
    assert(middle_value({1, 2, 3, 4, 5}) == 3);
    assert(middle_value({1, 2, 3, 4, 5, 6}) == 4);
    std::cout << "[LeetCode 876] forward slow/fast yields middle 3/4\n";
}

// 實務：std::unique 只需 forward iterator；erase_after 完成 physical erase。
void practical_example()
{
    std::forward_list<int> values{1, 1, 2, 2, 3};
    values.unique();
    // expected 必須是具名物件。不能從兩個不同的 temporary initializer_list
    // 分別取得 begin/end；兩個 iterator 不屬於同一個 range，拿來比較會是 UB。
    const std::forward_list<int> expected{1, 2, 3};
    assert(values == expected);
    std::cout << "[實務] forward_list::unique removed adjacent duplicates\n";
}

int main() { basic_example(); leetcode_876_example(); practical_example(); }

// 易錯與面試：multi-pass 不代表 random access；std::distance(forward_list) 仍是 O(N)。
// 面試若問 forward 比 input 多什麼，核心是同一 range 可重走、iterator copies 可各自推進。
//
// Algorithm 關係：std::unique、std::search 與 std::lower_bound 的最低傳統要求可到
// forward iterator；但在不能 O(1) 跳躍的容器上，實際 iterator increments 可能是 O(N)。
// 實務若需要頻繁 size/index，forward_list 通常不是適合容器，不能只看單次插入 O(1)。
// 練習：改成只移除連續重複的字串，並解釋為何未排序資料需先 sort 才能全域去重。
