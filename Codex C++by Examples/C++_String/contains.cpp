/*
 * C++23 contains：判斷是否包含 substring 或 char
 *
 * `text.contains(x)` 等價於 `text.find(x) != npos`，但意圖更直接。它不是 C++20 API；
 * 為符合本教材 C++20 雙編譯器驗證，本檔以 feature-test macro 在 C++23 使用真正 API，
 * C++20 使用 find fallback。空 pattern 視為包含。比較區分大小寫、按 code units。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool contains_text(const std::string_view text, const std::string_view needle) {
#if defined(__cpp_lib_string_contains) && __cpp_lib_string_contains >= 202011L
    return text.contains(needle);
#else
    return text.find(needle) != std::string_view::npos;
#endif
}

bool contains_char(const std::string_view text, const char needle) {
#if defined(__cpp_lib_string_contains) && __cpp_lib_string_contains >= 202011L
    return text.contains(needle);
#else
    return text.find(needle) != std::string_view::npos;
#endif
}

void basic_demo() {
    assert(contains_text("C++20 textbook", "text"));
    assert(contains_text("abc", ""));
    assert(!contains_text("abc", "z"));
    assert(contains_char("abc", 'b'));
    assert(!contains_char("abc", 'z'));
}

// LeetCode 28 的 bool 變形：是否至少出現一次。
bool leetcode_has_occurrence(const std::string& text, const std::string& pattern) {
    return contains_text(text, pattern);
}

// 實務：非常簡化的敏感字檢查；真實系統需處理大小寫、Unicode 與 token 邊界。
bool practical_has_blocked_fragment(const std::string_view input,
                                    const std::vector<std::string>& blocked) {
    for (const std::string& fragment : blocked) {
        if (!fragment.empty() && contains_text(input, fragment)) return true;
    }
    return false;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_has_occurrence("sadbutsad", "but"));
    assert(!leetcode_has_occurrence("leetcode", "xyz"));
    assert(practical_has_blocked_fragment("token=secret", {"password=", "secret"}));
    assert(!practical_has_blocked_fragment("public-data", {"secret"}));
    std::cout << "contains: tests passed\n";
}

/*
 * 【版本】若用 `-std=c++20` 直接寫 string::contains 應編譯失敗；請勿靠編譯器 extension。
 * 【陷阱】contains("admin") 也匹配 "not-admin-like"；權限判定不能只做 substring 搜尋。
 * 【面試】contains 有沒有改變搜尋複雜度？沒有，它主要改善可讀性。
 * 【練習】實作以 delimiter 分詞後的完整 token 黑名單檢查。
 */

/*
 * 【contains 選擇表】
 * - C++23 且只需 bool：contains 最清楚。
 * - C++20 或需要位置：find。
 * - 只需前綴/後綴：starts_with/ends_with，避免一般搜尋。
 * - 字元集合：find_first_of，不是 contains("abc") 的同義詞。
 *
 * contains 不回位置、不計次數、不處理重疊匹配，也不做 case folding。空 needle
 * 為 true；若產品把空 pattern 視為錯誤，應在業務層先拒絕。
 * 【面試題】contains 是否比 find 快？沒有額外複雜度承諾，主要改善可讀性。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'contains.cpp' -o '/tmp/codex_cpp_C_String_contains' && '/tmp/codex_cpp_C_String_contains'
//
// === 預期輸出（節錄）===
// contains: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
