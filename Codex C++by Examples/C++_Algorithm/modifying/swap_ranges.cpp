/*
 * std::swap_ranges：逐一交換兩個等長範圍
 * ======================================
 * swap_ranges(first1,last1,first2) 交換 N 個元素並回第二範圍的尾後 iterator。
 * 第二範圍必須至少 N 格且可寫；時間 O(N)。C++20 ranges 版本可同時給兩個 end，
 * 但 classic overload 無法替你檢查目的容量。
 *
 * 兩個交換範圍不得重疊；完全相同的範圍也屬重疊，不能把它當成標準保證的 no-op。
 * 若程式已知兩者相同，應直接略過呼叫。不同容器可交換，前提是元素型別可互換；
 * 演算法不改兩容器 size。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 344：用兩個不重疊半區與 reverse_iterator 反轉字串。
void leetcode_reverse_with_swap_ranges(std::vector<char>& text) {
    const auto half = static_cast<std::ptrdiff_t>(text.size() / 2U);
    std::swap_ranges(text.begin(), text.begin() + half, text.rbegin());
}

// 實務：blue/green buffer 同時切換前 count 個 slot，剩餘資料保留。
void practical_swap_active_slots(std::vector<std::string>& blue,
                                 std::vector<std::string>& green,
                                 std::size_t count) {
    assert(count <= blue.size() && count <= green.size());
    const auto count_as_difference = static_cast<std::ptrdiff_t>(count);
    const auto result = std::swap_ranges(blue.begin(), blue.begin() + count_as_difference,
                                         green.begin());
    assert(result == green.begin() + count_as_difference);
}

int main() {
    std::vector<int> first{1, 2};
    std::vector<int> second{8, 9};
    assert(std::swap_ranges(first.begin(), first.end(), second.begin()) == second.end());
    assert((first == std::vector<int>{8, 9}));
    assert((second == std::vector<int>{1, 2}));

    std::vector<char> text{'h', 'e', 'l', 'l', 'o'};
    leetcode_reverse_with_swap_ranges(text);
    assert((text == std::vector<char>{'o', 'l', 'l', 'e', 'h'}));

    std::vector<std::string> blue{"b0", "b1", "b2"};
    std::vector<std::string> green{"g0", "g1", "g2"};
    practical_swap_active_slots(blue, green, 2U);
    assert((blue == std::vector<std::string>{"g0", "g1", "b2"}));
    assert((green == std::vector<std::string>{"b0", "b1", "g2"}));
    std::cout << "swap_ranges：LeetCode 反轉與實務 blue/green 切換測試通過\n";
}

/*
 * 易錯陷阱：swap_ranges 不知道 second.end()，容量不足會越界；assert 是本例契約，
 * 對外 API 應回錯誤而非只在 debug 擋下。reverse_iterator 的 base 語意容易混淆，
 * 本例只使用 rbegin 作交換起點。
 *
 * 面試：與 std::swap(vectorA,vectorB) 差異？整個 vector swap 通常 O(1) 交換內部
 * buffer；swap_ranges 是 O(N) 交換元素，且可只交換子範圍。實務 whole-buffer
 * publish 應優先整體 swap。練習：支援 count 超界時回 bool 且完全不修改。
 *
 * LeetCode 反轉版本對奇數長度留下中間元素不動；half=floor(N/2) 正是所需。
 * 實務 blue/green 若元素 swap 會丟例外，可能完成部分切換；真正部署發布應交換
 * 不會丟例外的 handle/shared_ptr，或先建立 transaction log。
 *
 * 測試要含 count=0、count=完整長度、兩邊長度不同、超界拒絕。此範例用 assert
 * 表達內部前置條件，公開函式應回 expected/error。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'swap_ranges.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_swap_ranges' && '/tmp/codex_cpp_C_Algorithm_modifying_swap_ranges'
//
// === 預期輸出（節錄）===
// swap_ranges：LeetCode 反轉與實務 blue/green 切換測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
