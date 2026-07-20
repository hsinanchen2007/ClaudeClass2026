/*
 * <cctype>：以 C locale 分類/轉換單一 byte 字元
 *
 * 常用函式：isalpha/isdigit/isalnum/isspace/islower/isupper/isxdigit，以及
 * tolower/toupper。它們回傳 int（零或非零），不是保證回 bool。最重要的前置條件：
 * 參數必須是 EOF 或可表示成 unsigned char 的值；直接傳負的 signed char 是 UB。
 * 這套 API 不理解 UTF-8 多 byte 字元，也不是完整 Unicode case folding。
 */

#include <cassert>
#include <cctype>
#include <iostream>
#include <string>

namespace {

unsigned char as_uchar(const char ch) {
    return static_cast<unsigned char>(ch);
}

void basic_demo() {
    assert(std::isdigit(as_uchar('7')) != 0);
    assert(std::isalpha(as_uchar('A')) != 0);
    assert(std::isspace(as_uchar('\n')) != 0);
    const int lower = std::tolower(as_uchar('Q'));
    const int upper = std::toupper(as_uchar('m'));
    assert(lower == 'q');
    assert(upper == 'M');
}

// LeetCode 125（Valid Palindrome）：題目使用英數 ASCII，適合 cctype。
bool leetcode_valid_palindrome(const std::string& text) {
    std::size_t left = 0U;
    std::size_t right = text.size();
    while (left < right) {
        while (left < right && std::isalnum(as_uchar(text[left])) == 0) ++left;
        while (left < right && std::isalnum(as_uchar(text[right - 1U])) == 0) --right;
        if (left < right) {
            if (std::tolower(as_uchar(text[left])) !=
                std::tolower(as_uchar(text[right - 1U]))) {
                return false;
            }
            ++left;
            --right;
        }
    }
    return true;
}

// 實務：產生 ASCII slug；連續非英數合併成一個 dash，並移除尾端 dash。
std::string practical_make_slug(const std::string& title) {
    std::string slug;
    for (const char ch : title) {
        const unsigned char byte = as_uchar(ch);
        if (std::isalnum(byte) != 0) {
            slug.push_back(static_cast<char>(std::tolower(byte)));
        } else if (!slug.empty() && slug.back() != '-') {
            slug.push_back('-');
        }
    }
    if (!slug.empty() && slug.back() == '-') slug.pop_back();
    return slug;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_valid_palindrome("A man, a plan, a canal: Panama"));
    assert(!leetcode_valid_palindrome("race a car"));
    assert(practical_make_slug("C++ String: 101") == "c-string-101");
    assert(practical_make_slug("  Hello---World  ") == "hello-world");
    std::cout << "cctype: tests passed\n";
}

/*
 * 【面試速查】
 * 【陷阱】ctype 函式的合法輸入限制比函式名稱更重要，負的 signed char 不能直接傳入。
 * - 為何 `std::tolower(ch)` 可能 UB？char 若 signed，0x80..FF 會成負值。
 * - 安全模式：`std::tolower(static_cast<unsigned char>(ch))`，再轉回 char。
 * - locale 版本 `<locale>` 的 facet 與 C `<cctype>` overload 不要混淆。
 * - ASCII slug 會略過/破壞非 ASCII 語言；產品若支援 Unicode，使用 ICU/utf8proc 等。
 *
 * 【練習】實作只接受 `[A-Za-z_][A-Za-z0-9_]*` 的 identifier validator。
 */
