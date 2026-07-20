// ============================================================================
// 課題 2：Output iterator - 只寫入的 destination abstraction
// ============================================================================
//
// OutputIterator 支援 `*out=value; ++out`，不保證可讀或 multi-pass。back_inserter、
// ostream_iterator 是典型；algorithm 不必先知道 output size。輸出容器若可預估大小，
// reserve 可減少 realloc，但 back_inserter 本身仍安全。
//
// ostream_iterator delimiter 會在每個 element 後輸出，包含最後一個；需要無 trailing
// delimiter 時自行 join/loop 或 ranges join。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

void basic_example()
{
    const std::vector<int> source{1, 2, 3};
    std::vector<int> doubled;
    std::transform(source.begin(), source.end(), std::back_inserter(doubled),
                   [](int value) { return value * 2; });
    assert((doubled == std::vector<int>{2, 4, 6}));
    std::cout << "[基礎] transform writes through back_insert_iterator\n";
}

// LeetCode 1929：兩次 copy 到同一 back inserter，形成 nums concatenation。
std::vector<int> get_concatenation(const std::vector<int>& nums)
{
    std::vector<int> answer;
    answer.reserve(nums.size() * 2U);
    std::copy(nums.begin(), nums.end(), std::back_inserter(answer));
    std::copy(nums.begin(), nums.end(), std::back_inserter(answer));
    return answer;
}

void leetcode_1929_example()
{
    assert((get_concatenation({1, 2, 1}) == std::vector<int>{1, 2, 1, 1, 2, 1}));
    std::cout << "[LeetCode 1929] output iterator appended two ranges\n";
}

// 實務：ostream_iterator 產生 line-oriented report，每筆一行無歧義。
void practical_example()
{
    std::ostringstream output;
    const std::vector<int> ids{10, 20, 30};
    std::copy(ids.begin(), ids.end(), std::ostream_iterator<int>(output, "\n"));
    assert(output.str() == "10\n20\n30\n");
    std::cout << "[實務] output iterator wrote deterministic line records\n";
}

int main() { basic_example(); leetcode_1929_example(); practical_example(); }

// 易錯與面試：OutputIterator 只承諾可寫，不能期待 `*out` 能讀回值，也不能把複本當成
// 兩個獨立輸出位置。ostream_iterator 的 delimiter 包含最後一筆，是常見格式陷阱。
//
// 速查：
//   back_inserter(v)  -> v.push_back(value)，v 不需先 resize。
//   ostream_iterator  -> out << value << delimiter，錯誤存在 stream state。
//   raw `v.begin()`   -> algorithm 會覆寫既有 slots；output range 太短就是 UB。
//
// 實務選擇：已知精確輸出數量時可 resize 後寫 begin；未知數量用 back_inserter，若只知
// 上界則 reserve + back_inserter，兼顧安全與配置效率。
// output adapter 的 assignment 可能觸發配置或 stream 錯誤，並非天然 noexcept。
// 若輸出失敗會危害一致性，應先寫暫存結果、驗證後再提交，而非假設 algorithm 原子化。
// 練習：改用 front_inserter(list) 觀察輸出順序為何反轉。
