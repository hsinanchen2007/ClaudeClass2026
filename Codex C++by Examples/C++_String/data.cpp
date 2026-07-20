/*
 * data()：取得連續儲存區；C++17 起非 const string 可取得 char*
 *
 * const string::data() 與 c_str() 都提供唯讀 NUL 結尾資料。非 const data() 可讓 C API
 * 在既有 size 範圍內寫入；不可寫超過 size，也不可改寫 data()[size()] 的終止 NUL。
 * 若 API 需要更大 buffer，先 resize，再傳 data()，最後依實際長度縮回。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "abcd";
    char* bytes = text.data();
    bytes[0] = 'A';
    assert(text == "Abcd");
    assert(text.data()[text.size()] == '\0');
}

// LeetCode 344（Reverse String）的 pointer 版本，展示 data() 與 size() 必須成對。
void leetcode_reverse_in_place(std::string& text) {
    std::reverse(text.data(), text.data() + static_cast<std::ptrdiff_t>(text.size()));
}

// 實務：模擬 C API 寫入 caller-provided buffer，回傳實際字元數（不含 NUL）。
std::size_t fake_read_hostname(char* output, const std::size_t capacity) {
    constexpr char hostname[] = "build-node";
    constexpr std::size_t length = sizeof(hostname) - 1U;
    if (capacity < length + 1U) {
        return 0U;
    }
    std::copy_n(hostname, length + 1U, output);
    return length;
}

std::string practical_read_hostname() {
    std::string buffer(64U, '\0');
    const std::size_t written = fake_read_hostname(buffer.data(), buffer.size());
    if (written == 0U) {
        return {};
    }
    buffer.resize(written);
    return buffer;
}

}  // namespace

int main() {
    basic_demo();
    std::string word = "stressed";
    leetcode_reverse_in_place(word);
    assert(word == "desserts");
    assert(practical_read_hostname() == "build-node");
    std::cout << "data: tests passed\n";
}

/*
 * 【陷阱】reserve(100) 只改 capacity，不讓 data()[0..99] 成為可寫元素；必須 resize。
 * 【面試】為何不能讓 C API 更新 string 的 size？C API 不知道 string 物件；呼叫後由
 * C++ 端依回傳長度 resize。
 * 【練習】讓 fake API 在空間不足時回報所需容量，實作「查大小→配置→再呼叫」。
 */

/*
 * 【面試速查】non-const data 自 C++17 可寫，但只限現有 [0,size()) 元素；若 C API
 * 會改長度，必須由回傳值同步 resize。任何可能重配的 string 操作都使舊 data pointer
 * 失效。若 API 只讀且要求 NUL，c_str 的意圖通常比 data 更清楚。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'data.cpp' -o '/tmp/codex_cpp_C_String_data' && '/tmp/codex_cpp_C_String_data'
//
// === 預期輸出（節錄）===
// data: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
