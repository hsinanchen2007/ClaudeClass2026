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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接）
// 題目：建立長度 2N 的 answer，使前後兩半皆為 nums；例如 [1,2,1] 得 [1,2,1,1,2,1]。
// 為何使用本章主題：back_insert_iterator 把 algorithm 的輸出賦值轉成 push_back，連續寫入兩份 range。
// 思路：預留 2N 容量；第一次 copy 原陣列；第二次 copy 同一 range；回傳 answer。
// 複雜度：時間 O(N)、額外空間 O(N)（不含必要輸出則為 O(1)），N 為 nums 長度。
// 易錯點：nums.size()*2 可能 size_t 溢位；reserve 不改 size；直接寫 begin 前必須先 resize。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】逐行輸出的識別碼報表
// 情境：下游工具要求每行一個 id，輸入 [10,20,30] 必須輸出三行且保留最後換行。
// 為何使用本章主題：ostream_iterator 將每次 assignment 直接格式化到 stream，無需手寫索引與分隔判斷。
// 設計：建立輸出 stream；以換行作 delimiter；copy 所有 id 到 output iterator；檢查結果。
// 成本：時間 O(N) 加序列化/I/O，額外記憶體依 stream 緩衝；N 為 id 數。
// 上線注意：delimiter 也出現在最後一筆後；必須檢查 stream state，寫檔若需原子性應先寫暫存檔再替換。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_output_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_02_output_iterator' && '/tmp/codex_cpp_C_Iterator_02_output_iterator'
//
// === 預期輸出（節錄）===
// [實務] output iterator wrote deterministic line records
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
