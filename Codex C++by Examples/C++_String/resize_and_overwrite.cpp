/*
 * C++23 resize_and_overwrite：由 callback 直接填入 string buffer 並回報新長度
 *
 * 這不是 C++20 API。callback 取得 char* 與可寫容量 n，必須只寫 [0,n)，再回傳
 * 不超過 n 的最終 size。適合編碼、C API 輸出、filter，讓實作有機會減少額外配置或
 * 複製；標準不保證「一定只配置一次」，配置次數仍取決於原容量與 library 實作。
 * callback 期間不可再對同一 string 做一般操作，也不可讓 pointer 逃出 callback。
 * callback 不得丟例外；而且回傳長度涵蓋的每個 byte 都必須已被寫成確定值，留下
 * indeterminate byte 再讓 string 讀取會違反前置條件。
 *
 * 本檔必須在 C++20 驗證，因此用 feature-test macro：有 C++23 API 時走真正版本，
 * 否則走語意等價 fallback。這不是把 API 誤稱為 C++20。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

std::string uppercase_ascii(std::string text) {
#if defined(__cpp_lib_string_resize_and_overwrite) && \
    __cpp_lib_string_resize_and_overwrite >= 202110L
    text.resize_and_overwrite(text.size(), [](char* buffer, const std::size_t size) noexcept {
        for (std::size_t i = 0U; i < size; ++i) {
            if (buffer[i] >= 'a' && buffer[i] <= 'z') {
                buffer[i] = static_cast<char>(buffer[i] - 'a' + 'A');
            }
        }
        return size;
    });
#else
    for (char& ch : text) {
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
    }
#endif
    return text;
}

// LeetCode 709（To Lower Case）的相反變形；實際可執行且兩路結果一致。
std::string leetcode_upper(const std::string& text) {
    return uppercase_ascii(text);
}

// 實務：移除 ASCII 空白；C++23 路徑在同一 buffer 原地壓縮。
std::string practical_remove_ascii_spaces(std::string text) {
#if defined(__cpp_lib_string_resize_and_overwrite) && \
    __cpp_lib_string_resize_and_overwrite >= 202110L
    text.resize_and_overwrite(text.size(), [](char* buffer, const std::size_t size) noexcept {
        std::size_t output = 0U;
        for (std::size_t i = 0U; i < size; ++i) {
            if (buffer[i] != ' ') buffer[output++] = buffer[i];
        }
        return output;
    });
#else
    std::size_t output = 0U;
    for (const char ch : text) {
        if (ch != ' ') text[output++] = ch;
    }
    text.resize(output);
#endif
    return text;
}

}  // namespace

int main() {
    assert(leetcode_upper("Hello-123") == "HELLO-123");
    assert(practical_remove_ascii_spaces("a b  c") == "abc");
    assert(practical_remove_ascii_spaces("").empty());
    std::cout << "resize_and_overwrite: tests passed\n";
}

/*
 * 【面試】callback 為何回傳 size？實際輸出可能小於預留空間，string 需知道最終長度。
 * 【陷阱】回傳大於傳入容量、讓 callback 丟例外、或在回傳長度內留下未初始化 byte，
 * 都違反契約；保存 buffer 指標供 callback 後使用也錯。
 * 【練習】用此模式把每 byte 轉兩位十六進位（預留 input.size()*2）。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'resize_and_overwrite.cpp' -o '/tmp/codex_cpp_C_String_resize_and_overwrite' && '/tmp/codex_cpp_C_String_resize_and_overwrite'
//
// === 預期輸出（節錄）===
// resize_and_overwrite: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
