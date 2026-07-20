/*
 * C++20 std::erase / std::erase_if：把 erase-remove 慣用法包成單一呼叫
 *
 * std::erase(string, value) 刪除所有等於 value 的字元；std::erase_if 依 predicate 刪除。
 * 回傳刪除數量。兩者皆線性 O(n)，會保持未刪元素的相對順序。它們是 <string>
 * 提供的非成員函式，不是 text.erase_if(...）。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "banana";
    const std::size_t removed = std::erase(text, 'a');
    assert(removed == 3U);
    assert(text == "bnn");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element（移除元素，string 教學改寫）
// 題目：原題在 vector<int> 原地移除所有等於 val 的元素並回傳新長度；本例改成 char 字串，
//       移除 unwanted 後直接回傳新字串，例如 "leetcode" 刪 'e' 得 "ltcod"。
// 為何使用本章主題：C++20 std::erase 把 erase-remove 慣用法包成一次呼叫；這是容器與回傳介面
//       都經改寫的示範，不能直接當原題提交。
// 思路：1. 按值取得可修改副本；2. std::erase 刪除所有目標 char；3. 回傳縮短後字串。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 text 長度，空間來自輸入副本／回傳值。
// 易錯點：原題要求回新長度且不在意尾端內容；本版回 string，語意不同，且 iterator 全部要重取。
// -----------------------------------------------------------------------------
std::string leetcode_remove_character(std::string text, const char unwanted) {
    static_cast<void>(std::erase(text, unwanted));
    return text;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】單行 log 控制字元清理
// 情境：紀錄欄位不可含換行等 ASCII control bytes，但允許 tab 作為欄位間隔，並需回報刪除數量。
// 為何使用本章主題：std::erase_if 以 predicate 線性壓縮字串並直接回刪除數，相較手寫
//       erase-remove 較不會忘記真正縮短容器。
// 設計：1. 每個 char 轉 unsigned byte；2. 判定 `<0x20` 且不是 tab；3. 一次刪除並回 count。
// 成本：時間 O(N)、額外空間 O(1)，N 是 text 長度；保留字元可能在同一 buffer 內搬移。
// 上線注意：這不是完整 log escaping，DEL、ANSI escape、Unicode line separator 與欄位注入政策
//       仍需定義；修改後所有舊 view／pointer 都不得沿用。
// -----------------------------------------------------------------------------
std::size_t practical_sanitize_log_line(std::string& text) {
    return std::erase_if(text, [](const char ch) {
        const unsigned char byte = static_cast<unsigned char>(ch);
        return byte < 0x20U && ch != '\t';
    });
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_character("leetcode", 'e') == "ltcod");
    std::string log("ok\nnext\tfield", 13U);
    const std::size_t removed = practical_sanitize_log_line(log);
    assert(removed == 1U);
    assert(log == "oknext\tfield");
    std::cout << "std::erase: tests passed\n";
}

/*
 * 【面試】std::erase_if 會保留順序嗎？會，效果等同 erase(remove_if(...),end())。
 * 【陷阱】predicate 內把負的 char 直接交給 std::iscntrl 會 UB；先轉 unsigned char。
 * 【練習】刪除所有 ASCII whitespace，但保留一般 Unicode UTF-8 bytes 不動。
 */

/*
 * 【C++20 前後比較】
 * C++17 慣用法：
 *   s.erase(std::remove(s.begin(),s.end(),ch),s.end());
 * C++20：
 *   std::erase(s,ch);
 * 後者不會忘記第二步，也直接回傳刪除數量。
 *
 * 【predicate 規則】
 * - 不應修改元素或依賴呼叫次數/順序。
 * - 必須能接受所有 char 值；呼叫 cctype 前轉 unsigned char。
 * - erase_if 穩定保留未刪字元相對順序，但 iterator/reference 仍會失效。
 *
 * 【面試題】std::remove 是否真的縮短容器？不會，只把保留元素移到前面並回新 logical end。
 * 【練習】讓 sanitizer 回傳「刪除數 + 清理後內容」的結果 struct。
 * 【注意】刪除後任何保存的 element reference 都不應繼續使用。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'erase_free.cpp' -o '/tmp/codex_cpp_C_String_erase_free' && '/tmp/codex_cpp_C_String_erase_free'
//
// === 預期輸出（節錄）===
// std::erase: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
