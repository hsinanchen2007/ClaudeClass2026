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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（尋找首次出現位置，bool 變形）
// 題目：原題輸入 haystack、needle 並回傳第一個索引或 -1；本例只回答是否出現，
//       "sadbutsad" 中有 "but"，因此回 true，並非原題完整回傳介面。
// 為何使用本章主題：contains_text 直接表達存在性；C++23 走 contains，C++20 則以 find
//       fallback 保持相同語意，避免為了 bool 暴露位置細節。
// 思路：1. 把兩輸入視為唯讀範圍；2. 呼叫存在性搜尋；3. 直接回傳是否命中。
// 複雜度：直觀最壞時間 O(N*K)、額外空間 O(1)，N 是 text 長度，K 是 pattern 長度。
// 易錯點：這個 wrapper 不回原題索引；空 pattern 依標準為 true，產品若不允許要先驗證。
// -----------------------------------------------------------------------------
bool leetcode_has_occurrence(const std::string& text, const std::string& pattern) {
    return contains_text(text, pattern);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】禁止片段快速篩查
// 情境：在設定文字中檢查任一非空黑名單片段，例如 `secret`，命中便拒絕後續處理。
// 為何使用本章主題：只需 yes/no 時 contains 比 find+npos 更直接；逐一掃小型規則清單也比
//       regex 容易審核，但它只是第一層粗略篩選。
// 設計：1. 略過空規則；2. 逐一做 substring 存在性檢查；3. 首次命中立即回 true。
// 成本：令 B 為規則數、K 為平均規則長度，直觀最壞時間 O(B*N*K)、額外空間 O(1)。
// 上線注意：必須定義大小寫、Unicode 正規化與 token 邊界；安全判定不可只靠 substring，
//       並應限制規則與輸入長度以避免 CPU 濫用。
// -----------------------------------------------------------------------------
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
