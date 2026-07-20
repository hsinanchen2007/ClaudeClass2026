/*
 * std::string 建構子：從「字元資料」建立擁有內容的字串
 *
 * 【先建立正確模型】
 * std::string 是擁有連續字元記憶體的容器；複製建構會複製內容，移動建構則
 * 可接管資源。常見建構方式如下：
 *   std::string a;                   // 空字串
 *   std::string b("hello");          // 從 NUL 結尾 C 字串建立
 *   std::string c(4, 'x');           // 四個 x
 *   std::string d(b, 1, 3);          // b[1..3] 的子字串
 *   std::string e(first, last);      // 從 iterator 範圍建立
 *   std::string f(ptr, byte_count);  // 精確複製 byte_count 個 char，可含 '\0'
 *
 * 【複雜度與生命週期】
 * 除移動建構通常為 O(1) 外，建立 n 個字元一般是 O(n)，而且可能配置記憶體。
 * 建構完成後，string 擁有自己的副本；來源陣列之後失效不會影響它。
 *
 * 【常見陷阱】
 * 1. std::string(nullptr) 是未定義行為，空指標不是「空字串」。
 * 2. std::string("a\0b") 只得到 "a"；要保留內嵌 NUL，必須給長度或用 s literal。
 * 3. std::string(5, 'A') 與 std::string("ABCDE", 5) 的第二參數語意不同。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    const std::string empty;
    const std::string repeated(3U, 'A');
    const std::string source = "012345";
    const std::string middle(source, 2U, 3U);
    const std::vector<char> letters{'C', '+', '+'};
    const std::string from_range(letters.begin(), letters.end());

    const char binary[] = {'A', '\0', 'B'};
    const std::string owns_all_bytes(binary, sizeof(binary));

    assert(empty.empty());
    assert(repeated == "AAA");
    assert(middle == "234");
    assert(from_range == "C++");
    assert(owns_all_bytes.size() == 3U && owns_all_bytes[1] == '\0');
}

// LeetCode 58（Length of Last Word）：建構子將輸入複製進本地資料，接著倒序掃描。
int leetcode_length_of_last_word(const std::string& text) {
    std::size_t end = text.size();
    while (end > 0U && text[end - 1U] == ' ') {
        --end;
    }
    std::size_t begin = end;
    while (begin > 0U && text[begin - 1U] != ' ') {
        --begin;
    }
    return static_cast<int>(end - begin);
}

// 實務：網路封包或檔案欄位不保證 NUL 結尾，必須用 pointer + length。
std::string practical_copy_wire_field(const char* bytes, const std::size_t length) {
    if (bytes == nullptr) {
        assert(length == 0U);
        return {};
    }
    return std::string(bytes, length);
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_length_of_last_word("Hello World") == 5);
    assert(leetcode_length_of_last_word("   fly me   to   the moon  ") == 4);

    const char field[] = {'O', 'K', '\0', 'X'};
    const std::string copied = practical_copy_wire_field(field, sizeof(field));
    assert(copied.size() == 4U && copied[2] == '\0');

    std::cout << "constructor: tests passed\n";
}

/*
 * 【面試快問】為何 pointer + length 版本能保存 '\0'？
 * 因為它不靠 strlen 尋找終止字元，而是精確複製指定數量的 char。
 *
 * 【練習】新增一個從 std::array<char, N> 建立字串的函式，並測試陣列含 '\0'。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'constructor.cpp' -o '/tmp/codex_cpp_C_String_constructor' && '/tmp/codex_cpp_C_String_constructor'
//
// === 預期輸出（節錄）===
// constructor: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
