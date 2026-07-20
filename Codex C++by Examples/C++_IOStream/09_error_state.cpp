// ============================================================================
// 課題 9：good/eof/fail/bad、clear、exceptions mask
// ============================================================================
//
// eofbit：讀取到 end；failbit：格式解析失敗或 operation 未完成；badbit：底層嚴重 I/O
// error。`operator bool` 等價於 !fail()；fail() 只看 failbit/badbit，所以「只有 eofbit」
// 仍可為 true。成功 extraction 也可能在剛好讀到尾端時設 eofbit；再讀一次才會同時
// 設 failbit 並令條件為 false。因此 `while(!eof())` 仍是錯誤迴圈。
//
// clear() 清 flags，不修正輸入位置；互動恢復通常 clear()+ignore bad token。exceptions(mask)
// 可讓設定的 bits 丟 ios_base::failure，適合 fail-fast code，但 destructor/boundary 要小心。
// ============================================================================

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

void basic_example()
{
    std::istringstream exact("10");
    int exact_value = 0;
    assert(exact >> exact_value);
    assert(exact_value == 10 && exact.eof() && !exact.fail());

    std::istringstream input("10 bad 20");
    int value = 0;
    assert(input >> value && value == 10);
    assert(!(input >> value));
    assert(input.fail() && !input.bad());
    input.clear();
    std::string bad_token;
    input >> bad_token >> value;
    assert(bad_token == "bad" && value == 20);
    std::cout << "[基礎] clear resets state; caller must also consume bad token\n";
}

// LeetCode 65：Valid Number。不能只交給 `operator>> double`，因為標準浮點 parser 可能
// 接受題目排除的 `nan`/`inf`。以下明確追蹤 digit、decimal point 與 exponent 狀態。
bool is_number(const std::string& text)
{
    std::size_t begin = 0U;
    while (begin < text.size() && text.at(begin) == ' ') ++begin;
    std::size_t end = text.size();
    while (end > begin && text.at(end - 1U) == ' ') --end;
    if (begin == end) return false;

    bool seen_digit = false;
    bool seen_dot = false;
    bool seen_exponent = false;
    bool digit_after_exponent = true;

    for (std::size_t index = begin; index < end; ++index) {
        const char current = text.at(index);
        if (current >= '0' && current <= '9') {
            seen_digit = true;
            if (seen_exponent) digit_after_exponent = true;
        } else if (current == '+' || current == '-') {
            const bool at_start = index == begin;
            const bool after_exponent = index > begin &&
                (text.at(index - 1U) == 'e' || text.at(index - 1U) == 'E');
            if (!at_start && !after_exponent) return false;
        } else if (current == '.') {
            if (seen_dot || seen_exponent) return false;
            seen_dot = true;
        } else if (current == 'e' || current == 'E') {
            if (seen_exponent || !seen_digit) return false;
            seen_exponent = true;
            digit_after_exponent = false;
        } else {
            return false;
        }
    }
    return seen_digit && digit_after_exponent;
}

void leetcode_65_example()
{
    assert(is_number("2"));
    assert(is_number("-0.1"));
    assert(is_number("2e10"));
    assert(is_number("  +6e-1 "));
    assert(!is_number("1a"));
    assert(!is_number("99e2.5"));
    assert(!is_number("--6"));
    assert(!is_number("inf"));
    assert(!is_number("."));
    std::cout << "[LeetCode 65] decimal/exponent grammar 與非法表示均完整驗證\n";
}

// 實務：完整讀取後，只有 clean EOF 可接受；bad/fail before EOF 要回報 truncated/corrupt。
std::vector<int> read_numbers(std::istream& input)
{
    std::vector<int> values;
    for (int value = 0; input >> value;) values.push_back(value);
    if (!input.eof()) throw std::runtime_error("malformed numeric stream");
    return values;
}

void practical_example()
{
    std::istringstream good("1 2 3");
    assert((read_numbers(good) == std::vector<int>{1, 2, 3}));
    std::istringstream bad("1 x 3");
    bool rejected = false;
    try { (void)read_numbers(bad); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    std::cout << "[實務] malformed stream not mistaken for clean EOF\n";
}

int main()
{
    basic_example();
    leetcode_65_example();
    practical_example();
}

// 易錯與面試：eofbit 表示已碰到輸入尾端，但可以伴隨一次成功讀取；failbit 才讓 fail()
// 與 operator bool 報失敗。clean EOF 與 malformed token 必須分開。clear() 只清 flags，
// 不會移除壞 token。
// 練習：設定 input.exceptions(failbit|badbit)，觀察 EOF loop 應如何改寫。
// 複雜度：查/清 state bits 是 O(1)，真正 retry/ignore 則與丟棄字元數成正比。
// 生命週期：fail/eof/bad 狀態屬於 stream object 並持續存在，直到 clear 或 stream 被解構。

/*
 * 【教科書補充：state bits 可組合】
 * - eofbit、failbit、badbit 不是互斥 enum；只看 eof() 可能把 badbit|eofbit 誤認成正常結束。
 * - 核心 extraction 要先執行再 assert，否則 NDEBUG 會讓教材完全沒測到狀態轉移。
 * - exceptions(mask) 會在對應 bit 被設定時丟 ios_base::failure；仍可用 rdstate() 取得原因組合。
 * - clear(new_state) 只設定狀態，底層錯誤或壞 token 若未處理，下一次讀取仍可能立刻失敗。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_error_state.cpp' -o '/tmp/codex_cpp_C_IOStream_09_error_state' && '/tmp/codex_cpp_C_IOStream_09_error_state'
//
// === 預期輸出（節錄）===
// [實務] malformed stream not mistaken for clean EOF
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
