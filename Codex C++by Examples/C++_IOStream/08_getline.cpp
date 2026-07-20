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

// LeetCode 58：Length of Last Word。getline 保留整句空格，純函式由尾端略空白後計數。
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

// 實務：讀 blank-line-separated paragraphs；空行是資料，不可用 `line.empty()` 當 EOF。
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
