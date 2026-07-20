// ============================================================================
// 課題 6：reverse_iterator 與 base() 的 off-by-one
// ============================================================================
//
// rbegin→最後元素，rend→第一元素之前的 reverse sentinel。reverse_iterator::base() 回
//「對應元素之後」的 forward iterator：若 *rit 是 x，則 *(rit.base()-1) 才是 x。
// 這讓 [rbegin,rend) 仍符合 half-open range，但 erase 轉換常出現 off-by-one。
//
// 要 erase reverse iterator 指向元素：`container.erase(std::next(rit).base())` 或
// `erase(rit.base()-1)`（後者只適 random access）。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>

void basic_example()
{
    const std::string text = "abc";
    assert(*text.rbegin() == 'c');
    assert(text.rbegin().base() == text.end());
    std::string reversed(text.rbegin(), text.rend());
    assert(reversed == "cba");
    std::cout << "[基礎] reverse range constructs cba\n";
}

// LeetCode 344：Reverse String，std::reverse 接 normal bidirectional iterators。
void reverse_string(std::string& text) { std::reverse(text.begin(), text.end()); }

void leetcode_344_example()
{
    std::string text = "hello";
    reverse_string(text);
    assert(text == "olleh");
    std::cout << "[LeetCode 344] reverse algorithm outputs olleh\n";
}

// 實務：從尾端找最後一個 slash，再正確轉 forward index。
void practical_example()
{
    const std::string path = "dir/sub/file.txt";
    const auto slash = std::find(path.rbegin(), path.rend(), '/');
    assert(slash != path.rend());
    const std::size_t index = static_cast<std::size_t>(std::distance(path.begin(), slash.base()) - 1);
    assert(index == 7U && path.substr(index + 1U) == "file.txt");
    std::cout << "[實務] reverse search found final separator index 7\n";
}

int main() { basic_example(); leetcode_344_example(); practical_example(); }

// 易錯與面試：`rit.base()` 不指向 `*rit`，而是正向序列的下一格。這不是 API 瑕疵，
// 而是讓反向 `[rbegin,rend)` 仍保持半開區間所必需的設計；erase 時最常 off-by-one。
//
// 轉換圖（資料 a b c，rit 指 c）：
//   forward: [a][b][c][end]
//                       ^ rit.base()
//                  ^ *rit
// 因此 random-access 容器刪 `*rit` 可用 erase(std::prev(rit.base()))；泛型寫法常用
// erase(std::next(rit).base())。轉換後原 rit 是否有效仍依 erase 的容器失效規則。
//
// 工作用途：找最後一個 delimiter、最後一筆符合條件的 log、由新到舊掃事件。若只要
// 最大/最後元素，先看容器是否已有 back()/rbegin()，不必手寫 index 倒數以免 unsigned underflow。
// reverse_iterator 只是 view/cursor，不會真的反轉 container；要永久改順序才用 std::reverse。
// const container 的 rbegin() 產生 const reverse iterator，不能藉反向介面繞過 const。
// `rend()` 和 `end()` 一樣只可比較，不可解參考；空 range 時 rbegin()==rend()。
// 面試若要求回正向位置，先畫半開區間，再決定要 base() 還是 prev(base())。
// 對 list 可用 std::next(rit).base()，不可寫 base()-1，因其不是 random-access。
// 練習：以 reverse_iterator 找最後一個偶數並 erase，驗證 base conversion。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_reverse_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_06_reverse_iterator' && '/tmp/codex_cpp_C_Iterator_06_reverse_iterator'
//
// === 預期輸出（節錄）===
// [實務] reverse search found final separator index 7
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
