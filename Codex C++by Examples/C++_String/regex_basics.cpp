/*
 * <regex> 基礎：regex_match、regex_search、regex_replace、sregex_iterator
 *
 * regex_match 要整串符合；regex_search 只需某段符合。std::regex 預設 ECMAScript grammar，
 * 反斜線在 C++ literal 與 regex 各有一層，raw string `R"(...)"` 可減少 escaping。
 * 建構 regex 可能丟 regex_error，且效能/最壞輸入風險需量測；簡單 delimiter/token
 * 通常 find/from_chars 更清楚、更快。
 */

#include <cassert>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    const std::regex identifier(R"([A-Za-z_][A-Za-z0-9_]*)");
    assert(std::regex_match("valid_name2", identifier));
    assert(!std::regex_match("2bad", identifier));

    const std::regex digits(R"(\d+)");
    assert(std::regex_search("id=42", digits));
    assert(std::regex_replace("a1b22", digits, "#") == "a#b#");
}

// LeetCode 2042（Check if Numbers Are Ascending in a Sentence）。
bool leetcode_numbers_ascending(const std::string& sentence) {
    const std::regex number(R"(\d+)");
    int previous = -1;
    for (std::sregex_iterator it(sentence.begin(), sentence.end(), number), end; it != end; ++it) {
        const int value = std::stoi(it->str());
        if (value <= previous) return false;
        previous = value;
    }
    return true;
}

// 實務：驗證非常簡化的 `LEVEL: message`；真正 log 格式應有明確 parser/schema。
struct ParsedLog {
    std::string level;
    std::string message;
};

bool practical_parse_log(const std::string& line, ParsedLog& output) {
    static const std::regex pattern(R"((INFO|WARN|ERROR): ([^\r\n]+))");
    std::smatch match;
    if (!std::regex_match(line, match, pattern)) return false;
    output = ParsedLog{match[1].str(), match[2].str()};
    return true;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_numbers_ascending("1 box has 3 blue 4 red 6 green and 12 yellow marbles"));
    assert(!leetcode_numbers_ascending("hello world 5 x 5"));
    ParsedLog log;
    const bool parsed = practical_parse_log("ERROR: disk full", log);
    assert(parsed);
    assert(log.level == "ERROR" && log.message == "disk full");
    const bool rejected = practical_parse_log("DEBUG: hidden", log);
    assert(!rejected);
    std::cout << "regex basics: tests passed\n";
}

/*
 * 【陷阱與面試】
 * - `regex_match("xx42yy", "\\d+")` 為 false；search 才找局部。
 * - 每次函式呼叫重建 regex 很貴；固定 pattern 可 static const，但初始化仍可能丟例外。
 * - 不可信複雜 pattern/input 可能造成高 CPU；安全邊界需長度限制、測試或替代引擎。
 * - capture group 從 match[1] 開始，match[0] 是整體匹配。
 * 【練習】加入可選 timestamp capture，並驗證缺 timestamp 與錯格式。
 */

/*
 * 【教科書補充：regex 結果也有生命週期】
 * - smatch/sub_match 保存來源字串的 iterator；來源銷毀或修改失效後，不可再讀 capture。
 * - 需要跨 scope 保存 capture 時立即呼叫 `.str()` 取得 owning string；sregex_iterator 也依賴輸入與 regex。
 * - 標準 regex 沒有適合作為安全邊界的可攜最壞時間保證；不可信輸入要限長、限 pattern 或用線性引擎。
 * - static regex 省去重複建構，但初始化仍可能丟 regex_error，錯誤策略必須在 API 邊界決定。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'regex_basics.cpp' -o '/tmp/codex_cpp_C_String_regex_basics' && '/tmp/codex_cpp_C_String_regex_basics'
//
// === 預期輸出（節錄）===
// regex basics: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
