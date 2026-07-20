/*
 * find_first_of(set)：找第一個「屬於字元集合」的元素
 *
 * 它不是找完整 substring：`text.find_first_of("aeiou")` 找任一母音。第二參數 pos
 * 指搜尋起點。找不到回 npos。字元集合很小時簡潔好用；大量分類可用 256-entry table。
 * 直觀複雜度 O(n*m)，n 是被搜尋長度，m 是集合長度。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string text = "rhythm and blues";
    assert(text.find_first_of("aeiou") == 7U);
    assert(text.find_first_of("xyz") == 2U);  // 找到 y，不是尋找 "xyz"。
    assert(text.find_first_of("0123456789") == std::string::npos);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 345. Reverse Vowels of a String（反轉字串中的母音）
// 題目：只反轉英文字串中的母音位置，其他字元不動；例如 "hello" 變成 "holle"。
// 為何使用本章主題：find_first_of 從左找任一母音，find_last_of 從右找任一母音，
//       以字元集合 API 直接跳過所有非母音位置。
// 思路：1. 找最左與最右母音；2. 左小於右時交換；3. 分別從下一個內側位置繼續搜尋。
// 複雜度：時間 O(N*V)、額外空間 O(N)，N 是 text 長度、V=10 是母音集合大小，空間為輸入副本。
// 易錯點：npos 必須先判斷；right==0 時不可做 right-1，且此集合只涵蓋 ASCII 母音。
// -----------------------------------------------------------------------------
std::string leetcode_reverse_vowels(std::string text) {
    constexpr char vowels[] = "aeiouAEIOU";
    std::size_t left = text.find_first_of(vowels);
    std::size_t right = text.find_last_of(vowels);
    while (left != std::string::npos && right != std::string::npos && left < right) {
        const char saved = text[left];
        text[left] = text[right];
        text[right] = saved;
        left = text.find_first_of(vowels, left + 1U);
        if (right == 0U) break;
        right = text.find_last_of(vowels, right - 1U);
    }
    return text;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】混合分隔格式的首個 delimiter 偵測
// 情境：匯入前要找一行中最先出現的逗號、tab 或分號，並同時回報其位置與實際字元。
// 為何使用本章主題：find_first_of(",\t;") 會按輸入順序找集合中的任一字元，正好符合需求；
//       多次分別 find 再取最小值較冗長。
// 設計：1. 搜尋 delimiter 集合；2. npos 時回 `{npos,'\0'}`；3. 命中時以索引讀回字元。
// 成本：時間 O(N*D)、額外空間 O(1)，N 是行長，D=3 是 delimiter 數。
// 上線注意：這只做格式猜測，不解析 quoted CSV；空欄、escape、UTF-8 與錯誤回報仍需正式 parser。
// -----------------------------------------------------------------------------
std::pair<std::size_t, char> practical_first_delimiter(const std::string& line) {
    const std::size_t position = line.find_first_of(",\t;");
    return position == std::string::npos ? std::pair{position, '\0'}
                                         : std::pair{position, line[position]};
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_reverse_vowels("hello") == "holle");
    assert(leetcode_reverse_vowels("leetcode") == "leotcede");
    const auto [position, delimiter] = practical_first_delimiter("name;age,city");
    assert(position == 4U && delimiter == ';');
    const auto none = practical_first_delimiter("plain");
    assert(none.first == std::string::npos && none.second == '\0');
    std::cout << "find_first_of: tests passed\n";
}

/*
 * 【陷阱】find_first_of("cat") 不要求 c-a-t 連續，也不保證先找 c；它按 text 順序找。
 * 【面試】如何把字元集合查找降到 O(n)？預建 bool[256] lookup，逐 byte O(1) 判定。
 * 【Unicode】此 API 比的是 char code units，不理解 UTF-8 code point 或 grapheme。
 * 【練習】找出一行中第一個 JSON structural character `{[,:]}`。
 */

/*
 * 【API 選擇】
 * - find("abc")：完整連續 pattern。
 * - find_first_of("abc")：任一 a/b/c。
 * - regex_search：需要重複、選擇、群組等 pattern 語法。
 * - 256-entry lookup：固定 byte set 且高頻效能敏感。
 * 搜尋起點 pos 可避免建立 substr；pos>=size 且 set 非空時通常找不到。
 * 【面試題】此 API 能找 emoji 集合嗎？它以 char code unit 比對，UTF-8 emoji 是多 bytes。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find_first_of.cpp' -o '/tmp/codex_cpp_C_String_find_first_of' && '/tmp/codex_cpp_C_String_find_first_of'
//
// === 預期輸出（節錄）===
// find_first_of: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
