/*
 * find_first_not_of(set)：找第一個「不屬於排除集合」的字元
 *
 * 最常用於略過前導空白、零或 delimiter。若所有字元都在集合中，回傳 npos。
 * 注意 set 是 byte/code-unit 集合；`" \t\r\n"` 只涵蓋列出的 ASCII whitespace。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view ascii_space = " \t\r\n";

void basic_demo() {
    const std::string text = "\t  value";
    assert(text.find_first_not_of(ascii_space) == 3U);
    assert(std::string(" \n").find_first_not_of(ascii_space) == std::string::npos);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度，前置 helper）
// 題目：原題要忽略尾端空格後計算最後一字；本函式只計算開頭空格數，用於辨認全空白，
//       例如 "   fly" 回 3，因此只是教學拆出的前置步驟，不是完整原題答案。
// 為何使用本章主題：find_first_not_of(' ') 一次定位第一個非空格，比手寫逐字元前進直接，
//       並以 npos 明確表示全字串都在排除集合內。
// 思路：1. 搜尋第一個非空格；2. npos 時以整體長度作為前導空格數；3. 否則回傳該索引。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：此 helper 沒有計算最後一字；只認 ASCII space，不含 tab/newline，且 size 轉 int 要驗範圍。
// -----------------------------------------------------------------------------
int leetcode_count_leading_spaces(const std::string& text) {
    const std::size_t first = text.find_first_not_of(' ');
    return first == std::string::npos ? static_cast<int>(text.size()) : static_cast<int>(first);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】設定行零拷貝左側 trim
// 情境：parser 要略過 space、tab、CR、LF 後繼續讀同一行，但不想配置一份新字串。
// 為何使用本章主題：find_first_not_of 固定集合可直接找保留區段起點，再由 string_view::substr
//       建立 O(1) 借用視窗；相較 cctype 不受 locale 影響且規格明確。
// 設計：1. 找第一個不在 ASCII 空白集合的 byte；2. 全空白回空 view；3. 否則回起點到尾端。
// 成本：搜尋時間 O(N)、額外空間 O(1)，N 是 input 長度，結果不複製字元。
// 上線注意：回傳 view 依賴來源生命週期；temporary string、owner clear/erase/reallocate 後都不能沿用。
// -----------------------------------------------------------------------------
std::string_view practical_ltrim(const std::string_view input) {
    const std::size_t first = input.find_first_not_of(ascii_space);
    return first == std::string_view::npos ? std::string_view{} : input.substr(first);
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_count_leading_spaces("   fly") == 3);
    assert(leetcode_count_leading_spaces("   ") == 3);
    const std::string line = "\t  key=value  ";
    assert(practical_ltrim(line) == "key=value  ");
    assert(practical_ltrim(" \n\t").empty());
    std::cout << "find_first_not_of: tests passed\n";
}

/*
 * 【選擇表】find_first_of 找「第一個在集合內」；find_first_not_of 找「第一個在集合外」。
 * 【陷阱】npos 不能直接拿來 substr；本例全空白時回空 view。
 * 【生命週期】ltrim 回 view，不擁有資料；不要傳 temporary std::string 並把 view 保存。
 * 【面試】為何不用 std::isspace？集合版本規格固定、無 locale；isspace 要處理 unsigned char。
 * 【練習】讓 whitespace 集合可由呼叫者傳入，並測試只 trim 空格、不 trim tab。
 */

/*
 * 【四個集合搜尋 API 對照】
 * - find_first_of：左起第一個在 set 內。
 * - find_last_of：右起第一個在 set 內。
 * - find_first_not_of：左起第一個不在 set 內。
 * - find_last_not_of：右起第一個不在 set 內。
 * 它們都把參數當「char 集合」，不是 regex character class，也不是 substring。
 *
 * 【trim 左側的安全寫法】
 * 若結果 npos，代表全部需移除，回空；否則 substr(pos) 或 remove_prefix(pos)。
 * owning substr 配置，view remove_prefix 不配置但依賴來源生命週期。
 *
 * 【面試題】傳入空 set 會如何？沒有字元屬於空集合，因此 first_not_of 從 pos 即成功
 * （只要 pos 在範圍）；產品程式若不允許空 set 要先驗證。
 * 【練習】用回傳位置直接 erase 前綴，並對全排除字元輸入明確 clear。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find_first_not_of.cpp' -o '/tmp/codex_cpp_C_String_find_first_not_of' && '/tmp/codex_cpp_C_String_find_first_not_of'
//
// === 預期輸出（節錄）===
// find_first_not_of: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
