// ============================================================================
// 課題 8：std::getline、delimiter 與 `>>` 混用陷阱
// ============================================================================
//
// `operator>> int` 讀數字後把 newline 留在 stream；下一個 getline 會先讀到空行。解法依
// policy：`getline(input >> std::ws,line)` 會跳過所有 leading whitespace（不適合保留前導
// 空白）；或 `ignore(max,'\n')` 只丟到本行末端。
//
// getline 去掉 delimiter，不把它放入結果；空行成功得到 empty string。正確 loop 是
// while(getline(input,line))。含 quote/escape 的 CSV 不可只用 delimiter overload。
// ============================================================================

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

void basic_example()
{
    std::istringstream input("42\nAda Lovelace\n");
    int id = 0;
    input >> id;
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string name;
    std::getline(input, name);
    assert(id == 42 && name == "Ada Lovelace");
    std::cout << "[基礎] ignore consumed leftover newline before getline\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度）
// 題目：忽略句尾空格後回傳最後一個單字長度；`Hello World` 回 5，moon 例回 4。
// 為何使用本章主題：example 先以 getline 保留整句及其中空格，再交給純函式由尾端計數；
//       相較 operator>>，不會在 transport 階段丟失行的原始空白結構。
// 思路：1. getline 讀完整一行；2. end 向左略過空格；3. begin 向左走過最後一字；4. 回邊界差。
// 複雜度：讀取與計算時間 O(N)、額外空間 O(N)，N 是行長，空間來自 line 字串。
// 易錯點：空字串不可從 size()-1 起算；`at` 仍需先以 end>0 保護，getline 失敗也要檢查。
// -----------------------------------------------------------------------------
int length_of_last_word(const std::string& text)
{
    std::size_t end = text.size();
    while (end > 0U && text.at(end - 1U) == ' ') --end;
    std::size_t begin = end;
    while (begin > 0U && text.at(begin - 1U) != ' ') --begin;
    return static_cast<int>(end - begin);
}

void leetcode_58_example()
{
    std::istringstream input("Hello World\n");
    std::string line;
    std::getline(input, line);
    assert(length_of_last_word(line) == 5);
    assert(length_of_last_word("   fly me   to   the moon  ") == 4);
    std::cout << "[LeetCode 58] getline preserves sentence; last lengths 5/4\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】保留空白行的逐行匯入
// 情境：讀取 `first\n\nthird\n` 時，中間空行是段落資料，結果必須是 first、空字串、third。
// 為何使用本章主題：以 getline 本身作迴圈條件可區分「成功讀到空行」與「沒有下一行」；
//       相較檢查 line.empty() 或 `!eof()` 不會誤丟資料或重複舊值。
// 設計：1. 每輪建立 line；2. getline 成功就 push，包括 empty；3. 讀取失敗時結束。
// 成本：時間 O(T)、額外空間 O(T)，T 是所有行內容總 bytes，vector 擁有每行副本。
// 上線注意：結束後應分辨 clean EOF 與 badbit；還要設定總大小/單行上限並明定 CRLF 正規化。
// -----------------------------------------------------------------------------
std::vector<std::string> read_lines(std::istream& input)
{
    std::vector<std::string> lines;
    for (std::string line; std::getline(input, line);) lines.push_back(line);
    return lines;
}

void practical_example()
{
    std::istringstream input("first\n\nthird\n");
    assert((read_lines(input) == std::vector<std::string>{"first", "", "third"}));
    std::cout << "[實務] blank middle line preserved as data\n";
}

int main()
{
    basic_example();
    leetcode_58_example();
    practical_example();
}

// 成本與生命週期：每次 getline 是 O(L)，L 為讀到 delimiter/EOF 的字元數；string 以值
// 擁有該行。不要保存指向迴圈內 `line.data()` 的 pointer，下一次 getline 可能 reallocate。
// 面試快問：`while (!input.eof())` 為何錯？EOF 只在「讀取失敗後」才設，會多處理一次
// stale/empty data；應把讀取操作本身放在 while condition。
// 易錯：Windows CRLF 經 text stream 不一定跨所有環境自動去掉 `\r`，wire parser 要明定。
// 練習：支援 Windows CRLF 時觀察 getline 留下的 `\r`，並設計 normalization。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_getline.cpp' -o '/tmp/codex_cpp_C_IOStream_08_getline' && '/tmp/codex_cpp_C_IOStream_08_getline'
//
// === 預期輸出（節錄）===
// [實務] blank middle line preserved as data
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
