/*
 * std::string::assign：以新內容「整批取代」舊內容
 *
 * assign 與建構子有相似 overload：另一個 string、substring、C 字串、
 * pointer+count、count+char、iterator range、initializer_list。差別是物件已存在。
 * 回傳值為 *this，因此可以串接，但實務上分行通常更容易讀。
 *
 * 複雜度約 O(新字串長度)；若既有 capacity 足夠，實作可重用配置，但標準不保證
 * 一定不配置。assign 可能使原本指向字串的 iterator/reference/pointer 失效。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    std::string text = "old";
    text.assign(4U, 'x');
    assert(text == "xxxx");

    const std::string source = "prefix:value:suffix";
    text.assign(source, 7U, 5U);
    assert(text == "value");

    const std::vector<char> bytes{'A', 'P', 'I'};
    text.assign(bytes.begin(), bytes.end());
    assert(text == "API");
}

// LeetCode 344（Reverse String）：題目通常收 vector<char>；這裡展示 string 版本。
void leetcode_reverse_string(std::string& value) {
    std::size_t left = 0U;
    std::size_t right = value.size();
    while (left < right) {
        --right;
        if (left >= right) {
            break;
        }
        const char saved = value[left];
        value[left] = value[right];
        value[right] = saved;
        ++left;
    }
}

// 實務：重用工作緩衝區，明確表示「舊請求內容全部作廢」。
void practical_load_request_body(std::string& buffer, const char* data, const std::size_t size) {
    if (data == nullptr) {
        assert(size == 0U);
        buffer.clear();
        return;
    }
    buffer.assign(data, size);
}

}  // namespace

int main() {
    basic_demo();

    std::string word = "hello";
    leetcode_reverse_string(word);
    assert(word == "olleh");

    std::string request = "stale data";
    const char payload[] = {'a', '\0', 'b'};
    practical_load_request_body(request, payload, sizeof(payload));
    assert(request.size() == 3U && request[1] == '\0');

    std::cout << "assign: tests passed\n";
}

/*
 * 【易錯】assign(ptr) 要求 ptr 指向有效 NUL 結尾字串；二進位資料請用 assign(ptr,n)。
 * 【面試】assign 與 operator= 怎麼選？單一完整來源用 = 最自然；要 substring、重複
 * 字元或 iterator 範圍時 assign 的意圖較精準。
 * 【練習】用 assign(source, pos, count) 取出 URL 的 host，並處理 pos 超界例外。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'assign.cpp' -o '/tmp/codex_cpp_C_String_assign' && '/tmp/codex_cpp_C_String_assign'
//
// === 預期輸出（節錄）===
// assign: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
