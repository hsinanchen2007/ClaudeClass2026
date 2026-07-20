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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 150. Evaluate Reverse Polish Notation（計算逆波蘭表示式）
// 題目：原題輸入已切好的 tokens 並支援 +、-、*、/；本例改讀單一文字行且只實作 +、-，
//       `2 1 + 3 +` 得 6，因此是 stringstream 教學子集，不能直接提交原題。
// 為何使用本章主題：istringstream 依空白切 token，再由嚴格 parse_int_exact 解析操作數；
//       相較手寫 lexer 適合這個受限展示，但原題 vector<string> 不需要 stream。
// 思路：1. 逐 token；2. 數字 push stack；3. operator 取右、左 operand 計算後 push；4. 最後須恰一值。
// 複雜度：時間 O(T)、額外空間 O(T)，T 是 token/輸入總字元數，stack 最壞保存所有操作數。
// 易錯點：減法 operand 順序不可反；要驗 stack 至少兩格，且此版缺乘除與除零/overflow 規則。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】受限 CSV 單行切欄
// 情境：格式契約保證沒有 quote、escape 或欄內 newline，只需以逗號切欄，且 `a,b,` 要保留尾端空欄。
// 為何使用本章主題：istringstream 搭配 getline(delimiter) 可重用 stream API 逐欄讀取；相較 regex
//       簡單且可讀，但只適用明確受限格式。
// 設計：1. 以 line 建 input；2. getline 逗號迴圈加入欄位；3. 原行尾為逗號時補一個空欄。
// 成本：時間 O(N)、額外空間 O(N)，N 是 line 長度，所有欄位合計擁有內容副本。
// 上線注意：一般 CSV 的 quoted comma、雙引號 escaping 與多行欄位必須用成熟 parser；也要限制行長。
// -----------------------------------------------------------------------------
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
