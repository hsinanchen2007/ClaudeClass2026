// ============================================================================
// 課題 7：stringstream - in-memory formatting/parsing
// ============================================================================
//
// istringstream 讀 string、ostringstream 寫 string、stringstream 兩者兼具。適合小型 line
// parsing/formatting/tests；大量高效 numeric conversion 可用 from_chars/to_chars，避免 locale/
// allocation。重用 stringstream 時，`str("")` 清 buffer、`clear()` 清 error flags，兩者都要。
//
// extraction 成功不代表整串合法；strict parser 讀完值後再消耗 whitespace，確認 EOF。
// ============================================================================

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

int parse_int_exact(const std::string& text)
{
    std::istringstream input(text);
    int value = 0;
    if (!(input >> value)) throw std::invalid_argument("integer required");
    input >> std::ws;
    if (!input.eof()) throw std::invalid_argument("trailing data");
    return value;
}

void basic_example()
{
    assert(parse_int_exact(" 42 ") == 42);
    bool rejected = false;
    try { (void)parse_int_exact("42xyz"); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::ostringstream output;
    output << "id=" << 7 << ",ok=" << std::boolalpha << true;
    assert(output.str() == "id=7,ok=true");
    std::cout << "[基礎] strict parse rejects trailing data\n";
}

// LeetCode 150：RPN tokens 本來已切好；此例由一行 stringstream 切 token 再求值。
int eval_rpn_line(const std::string& line)
{
    std::istringstream input(line);
    std::vector<int> stack;
    for (std::string token; input >> token;) {
        if (token != "+" && token != "-") stack.push_back(parse_int_exact(token));
        else {
            if (stack.size() < 2U) throw std::invalid_argument("missing operand");
            const int right = stack.back(); stack.pop_back();
            const int left = stack.back(); stack.pop_back();
            stack.push_back(token == "+" ? left + right : left - right);
        }
    }
    if (stack.size() != 1U) throw std::invalid_argument("malformed RPN");
    return stack.back();
}

void leetcode_150_example()
{
    assert(eval_rpn_line("2 1 + 3 +") == 6);
    assert(eval_rpn_line("10 4 -") == 6);
    std::cout << "[LeetCode 150] line tokenization + RPN evaluation answers 6\n";
}

// 實務：CSV 只在「沒有 quote/escape/newline」的簡化格式可用 getline delimiter；
// production CSV 應用成熟 parser。
std::vector<std::string> split_simple_csv(const std::string& line)
{
    std::istringstream input(line);
    std::vector<std::string> fields;
    for (std::string field; std::getline(input, field, ',');) fields.push_back(field);
    if (!line.empty() && line.back() == ',') fields.emplace_back();
    return fields;
}

void practical_example()
{
    assert((split_simple_csv("a,b,") == std::vector<std::string>{"a", "b", ""}));
    std::cout << "[實務] simplified CSV preserves trailing empty field\n";
}

int main()
{
    basic_example();
    leetcode_150_example();
    practical_example();
}

// 易錯與面試：用 delimiter getline 切 CSV 不處理 quoted comma/newline/escape，只適合已限制
// 的簡化格式。stringstream 方便但會配置且受 locale 影響，高效 numeric parser 看 from_chars。
// 練習：改用 from_chars 寫不受 locale 影響且不配置的 parse_int_exact。
// 複雜度：parse/format 是 O(characters)，str() 通常還會複製完整 buffer。
// 生命週期：stringstream 擁有內部 string；取得的 view/pointer 在修改 buffer 或 stream 解構後失效。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_stringstream.cpp' -o '/tmp/codex_cpp_C_IOStream_07_stringstream' && '/tmp/codex_cpp_C_IOStream_07_stringstream'
//
// === 預期輸出（節錄）===
// [實務] simplified CSV preserves trailing empty field
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
