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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1047. Remove All Adjacent Duplicates In String（移除相鄰重複字元）
// 題目：持續刪除相鄰相同的一對字元直到穩定；"abbaca" 最後得到 "ca"。
// 為何使用本章主題：string 作為 stack，保留字元用 push_back，與尾端相同時改用 pop_back；
//       預留 N 容量可降低 builder 成長配置。
// 思路：1. 逐字元掃描；2. stack 非空且尾端相同就刪除；3. 否則 push；4. 回傳 stack。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 input 長度。
// 易錯點：back 前必須判空；刪除可能形成新的相鄰對，stack 模型會自然在後續比較中處理。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】CSV 欄位雙引號 escaping
// 情境：把單一欄位包在雙引號中，內容每個 `"` 要輸出成兩個 `"`；Ada "A" 需安全編碼。
// 為何使用本章主題：push_back 精確加入外層 quote 與每個 byte，遇 quote 再多推一次；
//       相較反覆 replace，單向 builder 不需移動已輸出的尾段。
// 設計：1. 推入開 quote；2. 掃欄位，quote 先額外推一次；3. 推原字元；4. 推閉 quote。
// 成本：時間 O(N)、額外空間 O(N+Q)，N 是欄位長度，Q 是其中 quote 數。
// 上線注意：完整 CSV 還涉及何時必須加引號、delimiter/newline、編碼與整筆 record 組裝；需限制輸出長度。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'push_back.cpp' -o '/tmp/codex_cpp_C_String_push_back' && '/tmp/codex_cpp_C_String_push_back'
//
// === 預期輸出（節錄）===
// push_back: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
