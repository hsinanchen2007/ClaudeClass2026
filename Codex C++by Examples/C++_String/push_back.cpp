/*
 * push_back(char)：在尾端加入一個字元
 *
 * 只接受單一 char；平均/攤銷 O(1)，但 capacity 不足時重新配置並搬移全部字元 O(n)。
 * 它比 `+= ch` 更明確表達 stack/builder 行為。重新配置會讓舊 iterator/pointer 失效。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "C+";
    text.push_back('+');
    assert(text == "C++");
}

// LeetCode 1047（Remove All Adjacent Duplicates In String）。
std::string leetcode_remove_adjacent_duplicates(const std::string& input) {
    std::string stack;
    stack.reserve(input.size());
    for (const char ch : input) {
        if (!stack.empty() && stack.back() == ch) {
            stack.pop_back();
        } else {
            stack.push_back(ch);
        }
    }
    return stack;
}

// 實務：CSV 欄位若含 quote，輸出時要把一個 quote 變兩個。
std::string practical_quote_csv_field(const std::string& field) {
    std::string result;
    result.reserve(field.size() + 2U);
    result.push_back('"');
    for (const char ch : field) {
        if (ch == '"') result.push_back('"');
        result.push_back(ch);
    }
    result.push_back('"');
    return result;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_adjacent_duplicates("abbaca") == "ca");
    assert(practical_quote_csv_field("Ada \"A\"") == "\"Ada \"\"A\"\"\"");
    std::cout << "push_back: tests passed\n";
}

/*
 * 【陷阱】push_back("x") 錯，因為 "x" 是 const char[2]；單字元要寫 'x'。
 * 【面試】攤銷 O(1) 是什麼？少數成長很貴，但長序列操作的平均每次成本為常數。
 * 【練習】擴充 CSV quoting：只有含逗號、quote 或換行才加外層 quote。
 */

/*
 * 【考前速查】
 * - 只接受一個 char；字串片段用 append 或 +=。
 * - 單次可能因重配 O(n)，長序列攤銷 O(1)。
 * - 已知最終長度先 reserve，降低配置與 iterator 失效次數。
 * - push 後新的 back() 就是剛加入字元；end() 會改變。
 * - capacity growth 是實作細節，不能假設固定倍數。
 * 【面試題】攤銷分析不代表每次延遲固定；低延遲系統仍可能預留上限。
 */
