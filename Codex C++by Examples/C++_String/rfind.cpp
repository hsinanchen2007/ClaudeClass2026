/*
 * std::string::rfind：由右向左找「最後一個」匹配
 *
 * `rfind(needle, pos)` 尋找起始索引不大於 pos 的最後匹配；預設 pos=npos 表示從尾端。
 * 對 char 常用來找最後一個點或斜線。它回傳的仍是正向索引，不是 reverse_iterator。
 * 找不到同樣回傳 npos。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void basic_demo() {
    const std::string text = "abc-abc-abc";
    assert(text.rfind("abc") == 8U);
    assert(text.rfind("abc", 7U) == 4U);
    assert(text.rfind('x') == std::string::npos);
}

// LeetCode 58（Length of Last Word）：rfind 找最後一個非空白後，再找前一個空白。
int leetcode_length_of_last_word(const std::string& text) {
    const std::size_t end = text.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    const std::size_t space = text.rfind(' ', end);
    const std::size_t begin = space == std::string::npos ? 0U : space + 1U;
    return static_cast<int>(end - begin + 1U);
}

// 實務：只認最後一個 path component 中的點；`.bashrc` 視為沒有副檔名。
std::string practical_extension(const std::string_view path) {
    const std::size_t slash = path.rfind('/');
    const std::size_t dot = path.rfind('.');
    const std::size_t name_begin = slash == std::string_view::npos ? 0U : slash + 1U;
    if (dot == std::string_view::npos || dot <= name_begin || dot + 1U == path.size()) {
        return {};
    }
    return std::string(path.substr(dot + 1U));
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_length_of_last_word("Hello World") == 5);
    assert(leetcode_length_of_last_word("   ") == 0);
    assert(practical_extension("/tmp/archive.tar.gz") == "gz");
    assert(practical_extension("/home/user/.bashrc").empty());
    assert(practical_extension("name.").empty());
    std::cout << "rfind: tests passed\n";
}

/*
 * 【易錯】rfind 的 pos 是「可接受匹配起點的最右位置」，不是反向偏移量。
 * 【選擇】找最後一個完整 substring 用 rfind；找集合中任一字元用 find_last_of。
 * 【複雜度】最壞會檢查多個候選起點，每個再比 needle；熱路徑需量測。
 * 【面試】rfind("",pos) 的空 needle 有特殊規則，產品程式最好先拒絕空 pattern。
 * 【練習】實作 basename(path)，同時支援尾端 slash 的規格。
 */

/*
 * 【rfind 邊界速查】
 * - 預設 pos=npos，實作會從最後可行起點搜尋。
 * - pattern 比字串長直接找不到。
 * - 回傳 pattern 的起始正向 index，不是末端 index。
 * - 找上一個匹配可從 `previous_pos-1` 繼續，但 previous_pos==0 時先停止以免 underflow。
 * - 空 pattern 的標準語意細緻；parser 通常應先禁止空 pattern。
 * 【面試題】最後一次出現且必須貼齊尾端，為何 ends_with 比 rfind 更直接？意圖與邊界
 * 都由 API 表達，不必再驗 `pos+pattern.size()==size()`。
 */
