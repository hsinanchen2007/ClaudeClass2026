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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 876. Middle of the Linked List（鏈結串列的中間節點）
// 題目：回傳中間節點，偶數長度取第二個中點；例如 [1,2,3,4,5,6] 取值 4。
// 為何使用本章主題：slow/fast 只需複製後各自前進，正好由 multi-pass forward iterator 支援。
// 思路：兩者從 begin 出發；fast 每輪走兩步；每完成兩步 slow 走一步；fast 到 end 時 slow 在中點。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為節點數。
// 易錯點：空串列會解參考 end，本 API 需非空；每次 fast 第二步前要查 end；偶數長度須回後中點。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】移除相鄰的重複事件代碼
// 情境：單向事件序列已按群組排列，需將 [1,1,2,2,3] 壓成 [1,2,3] 並保留首次出現順序。
// 為何使用本章主題：forward_list::unique 只需 forward iterator，就能直接解除相鄰重複節點而不搬移元素。
// 設計：建立單向串列；呼叫 unique 比較相鄰值；以具名 expected range 驗證結果。
// 成本：時間 O(N)、額外空間 O(1)，N 為節點數；刪節點另有解配置成本。
// 上線注意：unique 只移除相鄰重複，未排序資料不會全域去重；temporary ranges 的 iterator 不可混用。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_forward_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_03_forward_iterator' && '/tmp/codex_cpp_C_Iterator_03_forward_iterator'
//
// === 預期輸出（節錄）===
// [實務] forward_list::unique removed adjacent duplicates
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
