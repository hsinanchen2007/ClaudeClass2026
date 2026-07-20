/*
 * C++17 教科書：[[fallthrough]]
 *
 * switch case 未寫 break 會接著執行下一個 case；這常是 bug。C++17 的 [[fallthrough]]
 * 放在 null statement（`[[fallthrough]];`）上，明確標示貫穿是刻意設計並抑制 warning。
 * 它只能出現在 switch 中，且下一個 statement 必須是同一 switch 的 case/default label。
 *
 * 【選擇】多個 case 共用同一段程式可直接連續寫 labels，不需要 fallthrough；只有先執行
 * 本 case 部分工作、再延續下一 case 時才使用。複雜累進邏輯常改成 function/table 更清楚。
 * 【陷阱】attribute 不會執行 jump，也不會阻止控制流程；它只是向人與 compiler 表意。
 * 【面試題】空 case label `case 1: case 2:` 與 [[fallthrough]] 的使用時機差在哪？
 * 【練習】將 basic::privileges 改成資料表，討論可讀性。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace basic {
int privileges(int level) {
    int count = 0;
    switch (level) {
        case 3:
            ++count;  // admin 特權
            [[fallthrough]];
        case 2:
            ++count;  // editor 特權
            [[fallthrough]];
        case 1:
            ++count;  // reader 特權
            break;
        default:
            break;
    }
    return count;
}

void demo() {
    assert(privileges(1) == 1);
    assert(privileges(3) == 3);
}
}  // namespace basic

namespace leetcode {
// LeetCode 13：Roman to Integer。右側較大時相減，否則相加，O(n)/O(1)。
int roman_value(char symbol) {
    switch (symbol) {
        case 'I': return 1;
        case 'V': return 5;
        case 'X': return 10;
        case 'L': return 50;
        case 'C': return 100;
        case 'D': return 500;
        case 'M': return 1000;
        default: return 0;
    }
}

int leetcode_roman_to_int(const std::string& roman) {
    int total = 0;
    for (std::size_t i = 0; i < roman.size(); ++i) {
        const int current = roman_value(roman[i]);
        const int next = i + 1U < roman.size() ? roman_value(roman[i + 1U]) : 0;
        total += current < next ? -current : current;
    }
    return total;
}

void leetcode_test() {
    assert(leetcode_roman_to_int("III") == 3);
    assert(leetcode_roman_to_int("MCMXCIV") == 1994);
}
}  // namespace leetcode

// 【實務案例】告警升級：error 依序 page、記 metric、寫 log，刻意貫穿由 attribute 說清楚。
namespace practical {
enum class Severity { info, warning, error };

std::vector<std::string> practical_actions(Severity severity) {
    std::vector<std::string> actions;
    switch (severity) {
        case Severity::error:
            actions.push_back("page-oncall");
            [[fallthrough]];
        case Severity::warning:
            actions.push_back("record-metric");
            [[fallthrough]];
        case Severity::info:
            actions.push_back("write-log");
            break;
    }
    return actions;
}

void practical_test() {
    assert(practical_actions(Severity::info).size() == 1U);
    assert(practical_actions(Severity::error).size() == 3U);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "fallthrough：累進權限、Roman numeral、告警動作測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_fallthrough.cpp' -o '/tmp/codex_cpp_C_Cpp17_10_fallthrough' && '/tmp/codex_cpp_C_Cpp17_10_fallthrough'
//
// === 預期輸出（節錄）===
// fallthrough：累進權限、Roman numeral、告警動作測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
