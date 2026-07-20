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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 709. To Lower Case（轉成小寫，本檔反向改寫為大寫）
// 題目：原題將 ASCII 大寫轉成小寫；本例反向把小寫轉大寫，例如 Hello-123 得 HELLO-123，
//       用來展示相同逐 byte 轉換骨架，不能直接提交原題。
// 為何使用本章主題：C++23 路徑以 resize_and_overwrite 在同一 string buffer 逐 byte 覆寫並回原長度；
//       C++20 fallback 用 range-for 保持相同行為。
// 思路：1. 以原 size 作可寫上限；2. callback 掃描每格；3. 只轉 `a..z`；4. 回傳原 size。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 text 長度；按值參數先建立一份結果副本。
// 易錯點：這是方向相反的教學改寫；callback 不得丟例外、保存 buffer 或回傳大於 writable 的長度。
// -----------------------------------------------------------------------------
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

std::string leetcode_upper(const std::string& text) {
    return uppercase_ascii(text);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】識別碼 ASCII 空格原地壓縮
// 情境：輸入 `a b  c`，只移除 byte 0x20 並得到 `abc`，tab 與其他 whitespace 不在規格內。
// 為何使用本章主題：resize_and_overwrite 讓讀游標與寫游標共用同一 buffer，callback 回報壓縮後長度；
//       相較先建第二份字串可省一個輸出配置，C++20 fallback 明確 resize 收尾。
// 設計：1. output 索引從 0；2. 掃原範圍；3. 非空格搬到 buffer[output++]；4. 回 output。
// 成本：時間 O(N)、額外空間 O(N)，N 是輸入長度；按值參數持有結果，壓縮本身 O(1) 額外工作區。
// 上線注意：只移除 ASCII space，可能改變有意義內容；callback 內來源與目的重疊順序不可反向，
//       且所有回傳長度內 bytes 都必須已初始化。
// -----------------------------------------------------------------------------
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
