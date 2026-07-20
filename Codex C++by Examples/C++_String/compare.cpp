/*
 * std::string::compare：回傳負數、0、正數的字典序三向結果
 *
 * compare 支援整串、C 字串、substring 對 substring 等 overload。只可依結果的符號判斷，
 * 不可假設差值恰為 -1/0/1。比較逐 char code unit，最壞 O(min(n,m))；不提供 locale、
 * 自然數字排序或 Unicode collation。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    const std::string a = "apple";
    assert(a.compare("apple") == 0);
    assert(a.compare("banana") < 0);
    assert(a.compare(0U, 3U, "app") == 0);
}

// LeetCode 1455（Check If a Word Occurs As a Prefix of Any Word in a Sentence）。
int leetcode_is_prefix_of_word(const std::string& sentence, const std::string& search) {
    std::size_t begin = 0U;
    int index = 1;
    while (begin <= sentence.size()) {
        const std::size_t end = sentence.find(' ', begin);
        const std::size_t length = (end == std::string::npos ? sentence.size() : end) - begin;
        if (length >= search.size() && sentence.compare(begin, search.size(), search) == 0) {
            return index;
        }
        if (end == std::string::npos) break;
        begin = end + 1U;
        ++index;
    }
    return -1;
}

// 實務：依 namespace prefix 分流，避免先配置 substr。
std::string practical_route_key(const std::string& key) {
    if (key.size() >= 5U && key.compare(0U, 5U, "user.") == 0) return "user-service";
    if (key.size() >= 4U && key.compare(0U, 4U, "job.") == 0) return "job-service";
    return "unknown";
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_is_prefix_of_word("i love eating burger", "burg") == 4);
    assert(leetcode_is_prefix_of_word("this problem is easy", "pro") == 2);
    assert(leetcode_is_prefix_of_word("hello world", "x") == -1);
    assert(practical_route_key("user.name") == "user-service");
    assert(practical_route_key("job.state") == "job-service");
    std::cout << "compare: tests passed\n";
}

/*
 * 【選擇】只需相等用 ==；排序用 <=>/relational operators；比較 substring 才特別適合 compare。
 * 【陷阱】`if (a.compare(b))` 代表「不相等」，可讀性差；應明寫 `!= 0`。
 * 【面試】compare(0,prefix.size(),prefix)==0 可避免什麼？避免 substr 暫時字串配置。
 * 【練習】寫 ASCII case-insensitive compare，並說明為何不等於 Unicode case folding。
 */

/*
 * 【compare overload 選擇】
 * - compare(other)：整串三向比較。
 * - compare(pos,count,other)：不配置地比較本字串片段。
 * - compare(pos,count,other,other_pos,other_count)：兩側都取片段。
 * - pos 超出本字串範圍會丟 out_of_range；count 過長則截到尾端。
 * 【面試題】結果為何不能判 `== -1`？標準只保證負、零、正。
 * 【實務】自然語言、版本、數字排序都應先定義語意，不能直接套 compare。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'compare.cpp' -o '/tmp/codex_cpp_C_String_compare' && '/tmp/codex_cpp_C_String_compare'
//
// === 預期輸出（節錄）===
// compare: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
