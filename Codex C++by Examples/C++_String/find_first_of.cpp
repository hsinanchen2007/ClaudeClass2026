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

// LeetCode 345（Reverse Vowels of a String）。
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

// 實務：找 CSV/TSV 中最先出現的 delimiter，回傳 delimiter 本身與位置。
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
