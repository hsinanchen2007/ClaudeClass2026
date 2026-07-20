/*
 * empty()：以意圖清楚且 O(1) 的方式判斷字串是否沒有元素
 *
 * 【基本模型】empty 只回答 size 是否為零，不掃描內容，也不判斷文字是否有業務意義。
 * 【API】成員函式簽名概念上是 bool empty() const noexcept；const 與非 const 物件都能呼叫。
 * 【選型】已知型別是 string 時用 member empty；泛型 range 可考慮 std::ranges::empty。
 * 【複雜度】對 basic_string 固定為 O(1)，不是從頭尋找第一個非 NUL 字元。
 * 【生命週期】回傳 bool 是獨立值，不借用 buffer；呼叫本身不造成 iterator/reference 失效。
 * 【例外安全】empty 不配置、不修改且 noexcept，適合放在 front/back/pop_back 的 guard。
 * 【易錯】"   " 的 size 不為零，所以不是 empty；空白驗證是另一份領域規則。
 * 【易錯】empty 不代表「未初始化」；一個正常建構的 string 永遠是有效物件。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string none;
    const std::string spaces = "   ";
    assert(none.empty());
    assert(!spaces.empty());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses（有效括號）
// 題目：輸入只含三種括號，判斷每個閉括號是否依正確類型與順序配對；"([]{})" 合法，"([)]" 不合法。
// 為何使用本章主題：string 作為 stack 時，empty() 同時保護 back/pop_back 不發生 underflow，
//       並在掃描結束確認沒有未閉合括號。
// 思路：1. 開括號 push；2. 閉括號先判 stack 非空；3. pop 並核對類型；4. 最後 stack 必須為空。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 text 長度，最壞全部都是開括號。
// 易錯點：閉括號到來時要先 empty guard；本版也明確拒絕題目契約外的非括號字元。
// -----------------------------------------------------------------------------
bool leetcode_valid_parentheses(const std::string& text) {
    std::string stack;
    for (const char ch : text) {
        if (ch == '(' || ch == '[' || ch == '{') {
            stack.push_back(ch);
            continue;
        }
        if (ch != ')' && ch != ']' && ch != '}') {
            return false;
        }
        if (stack.empty()) {
            return false;
        }
        const char open = stack.back();
        stack.pop_back();
        if ((ch == ')' && open != '(') || (ch == ']' && open != '[') ||
            (ch == '}' && open != '{')) {
            return false;
        }
    }
    return stack.empty();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】必填設定值檢查
// 情境：表單或設定欄位不能是空字串，也不能只含 space、tab、LF、CR。
// 為何使用本章主題：empty() 只能回答元素數是否為零，本例逐 byte 補上業務層的 ASCII
//       whitespace 規則；相較 `text != ""`，能拒絕看似有長度但沒有內容的值。
// 設計：1. 逐字元掃描；2. 找到任一非列舉空白立即回 true；3. 掃完則回 false。
// 成本：時間 O(N)、額外空間 O(1)，N 是 text 長度，可在首個有效字元提前結束。
// 上線注意：只處理四種 ASCII 空白；Unicode whitespace 需要明定編碼與文字函式庫，不能逐 byte 猜測。
// -----------------------------------------------------------------------------
bool practical_has_non_space(const std::string& text) {
    for (const char ch : text) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_valid_parentheses(""));
    assert(leetcode_valid_parentheses("([]{})"));
    assert(!leetcode_valid_parentheses("([)]"));
    assert(!leetcode_valid_parentheses(")"));
    assert(!leetcode_valid_parentheses("(x)"));
    assert(!practical_has_non_space(""));
    assert(!practical_has_non_space(" \t\n"));
    assert(practical_has_non_space("  value "));
    std::cout << "empty: tests passed\n";
}

/*
 * 【LeetCode 契約】空字串視為合法；任何提早閉合、錯配、殘留開括號或非括號字元皆失敗。
 * 【LeetCode 成本】每個字元至多 push/pop 一次，時間 O(n)，最壞額外空間 O(n)。
 * 【實務契約】practical_has_non_space 將空格、tab、LF、CR 視為空白，不承諾完整 Unicode 分類。
 * 【實務選型】若規格要求 Unicode whitespace，必須先決定編碼與文字函式庫，不能只看 byte。
 * 【面試追問】empty 是否比 size()==0 快？兩者皆 O(1)，empty 的優勢是直接表達意圖。
 * 【陷阱】不要用 `text == ""` 代替必填欄位驗證；它同樣放過只含空白的輸入。
 * 【練習】實作 trim 後再 empty 的版本，並比較是否需要配置新字串。
 */

/*
 * 【面試速查】empty 是 O(1) 且不修改字串，只回答元素數量是否為零。
 * 【前置條件】front/back/pop_back 必須在緊鄰呼叫處用 empty guard，避免空容器 UB。
 * 【泛型程式】不要假設所有 range 都有 size；empty/ranges::empty 更能表達真正需求。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'empty.cpp' -o '/tmp/codex_cpp_C_String_empty' && '/tmp/codex_cpp_C_String_empty'
//
// === 預期輸出（節錄）===
// empty: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
