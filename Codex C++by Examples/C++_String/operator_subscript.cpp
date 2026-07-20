/*
 * operator[]：可證明索引合法時的 O(1) 存取
 *
 * text[i] 不做邊界檢查；i < size() 才能讀寫元素。不要把 size() 當最後一個索引：
 * 空字串沒有元素，非空字串最後索引是 size()-1。C++ 規格對 pos==size() 有特殊
 * 終止字元語意，但不可把它當一般可寫元素；教學與實務一律只存取 pos < size()。
 */

#include <array>
#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "map";
    assert(text[1U] == 'a');
    text[0U] = 'c';
    assert(text == "cap");
}

// LeetCode 387（First Unique Character in a String）；題目保證小寫英文字母。
int leetcode_first_unique_char(const std::string& text) {
    std::array<int, 26U> counts{};
    for (const char ch : text) {
        ++counts[static_cast<std::size_t>(ch - 'a')];
    }
    for (std::size_t i = 0U; i < text.size(); ++i) {
        if (counts[static_cast<std::size_t>(text[i] - 'a')] == 1) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// 實務：解析固定格式 "L:payload"，先驗證長度與分隔符再用 []。
bool practical_is_valid_record(const std::string& line) {
    if (line.size() < 3U) {
        return false;
    }
    const char level = line[0U];
    return (level == 'I' || level == 'W' || level == 'E') && line[1U] == ':';
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_first_unique_char("leetcode") == 0);
    assert(leetcode_first_unique_char("loveleetcode") == 2);
    assert(leetcode_first_unique_char("aabb") == -1);
    assert(practical_is_valid_record("I:ready"));
    assert(!practical_is_valid_record("X:no"));
    assert(!practical_is_valid_record("E"));
    std::cout << "operator[]: tests passed\n";
}

/*
 * 【陷阱】`for (i=0; i<=s.size(); ++i) s[i]` 的 <= 會多跑一次。
 * 【面試】何時選 []？當迴圈條件或前置驗證已證明索引合法，且越界代表程式 bug。
 * 【練習】擴充 practical_is_valid_record，要求冒號後至少一個非空白字元。
 */

/*
 * 【[] 與 at 決策】
 * - 迴圈條件已證明 `i<size`：[] 清楚且無例外路徑。
 * - index 來自使用者/檔案且越界是可恢復錯誤：先驗證，或 at 轉成例外。
 * - 越界代表程式不變量被破壞：assert 前置條件，再用 []。
 * non-const [] 回 char& 可修改；任何重配後舊 reference 可能失效。
 * 【面試題】為何 size_type 與 int 混用有風險？負 int 轉無號會變巨大索引。
 */

/*
 * 【教科書補充：operator[] 的 size 邊界】
 * - 在本教材 C++20 基準，pos<size() 才是一般元素；pos>size() 為未定義行為。
 * - pos==size() 可讀到 CharT{} 的特殊終止值，但它不是字串元素；寫入非 CharT{} 是 UB。
 * - 因此空字串 s[0] 也只可當上述終止值讀取，不能拿來寫第一個元素。
 * - at(size()) 不採這個特殊規則，會丟 out_of_range；外部索引優先使用 at 或先驗證。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'operator_subscript.cpp' -o '/tmp/codex_cpp_C_String_operator_subscript' && '/tmp/codex_cpp_C_String_operator_subscript'
//
// === 預期輸出（節錄）===
// operator[]: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
