/*
 * find_last_of(set)：從右側找最後一個屬於集合的字元
 *
 * 常見用途是找最後一個 `/` 或 `\\`、最後一個 delimiter、最後一個數字。它與 rfind
 * 的差別：rfind 尋找完整 pattern，find_last_of 尋找集合中任一 char。pos 表示最右
 * 候選索引；找不到回 npos。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void basic_demo() {
    const std::string text = "a/b\\c.txt";
    assert(text.find_last_of("/\\") == 3U);
    assert(text.find_last_of("0123456789") == std::string::npos);
}

// LeetCode 58（Length of Last Word），用 find_last_of 找前一個空白。
int leetcode_length_of_last_word(const std::string& text) {
    const std::size_t end = text.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    const std::size_t separator = text.find_last_of(' ', end);
    return static_cast<int>(end - (separator == std::string::npos ? 0U : separator + 1U) + 1U);
}

// 實務：跨 Windows/Unix 顯示 basename；不修改輸入、不配置中間 substring。
std::string practical_basename(const std::string_view path) {
    const std::size_t separator = path.find_last_of("/\\");
    const std::size_t begin = separator == std::string_view::npos ? 0U : separator + 1U;
    return std::string(path.substr(begin));
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_length_of_last_word("fly me to the moon  ") == 4);
    assert(practical_basename("/srv/data/report.csv") == "report.csv");
    assert(practical_basename("C:\\temp\\x.txt") == "x.txt");
    assert(practical_basename("plain") == "plain");
    std::cout << "find_last_of: tests passed\n";
}

/*
 * 【陷阱】path 以 separator 結尾時 basename 會是空；這是合理規格還是要先去尾 slash，
 * 必須由應用定義。正式 path 操作優先 std::filesystem::path。
 * 【面試】find_last_of(".gz") 是找最後的 '.', 'g' 或 'z'，不是找副檔名 ".gz"。
 * 【練習】找最後一個十六進位字元；說明大小寫集合如何列出。
 */

/*
 * 【考前速查】
 * - 預設 pos=npos，等同從可行的最右端開始。
 * - 回傳正向 index；結果不是 reverse_iterator。
 * - set 內重複字元沒有額外語意。
 * - 找 path separator 可列 `/\\`，但正式 path 操作仍以 filesystem::path 為準。
 * - 找副檔名的點時要處理 hidden file、尾點、directory 中的點與大小寫。
 *
 * 【面試題】find_last_of("abc") 與 rfind("abc") 最小反例？字串 "axc"：前者找到 c，
 * 後者找不到連續 "abc"。
 * 【練習】找最後一個非英數 delimiter，考慮用 find_last_not_of。
 *
 * 【複雜度與失效】搜尋本身不修改 string，既有 iterator/reference 保持有效；直觀
 * 最壞 O(n*m)。取得位置後若接著 erase/replace，修改操作才可能造成失效。
 * pos 不是「倒數第幾個」，而是正向索引上界；這是常見 off-by-one 來源。
 */
