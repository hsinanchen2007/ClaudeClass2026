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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度）
// 題目：忽略句尾空格，計算最後一個單字長度；例如 "fly me to the moon  " 回 4。
// 為何使用本章主題：先找最後非空格，再用 find_last_of(' ', end) 找該單字前的空格，
//       以索引差完成而不配置切片。
// 思路：1. 定位最後有效字元；2. 全空白回 0；3. 往左找空格；4. 依是否找到決定起點。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：find_last_of 的參數是字元集合；npos 分支要先處理，避免無號 `npos+1` wrap。
// -----------------------------------------------------------------------------
int leetcode_length_of_last_word(const std::string& text) {
    const std::size_t end = text.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    const std::size_t separator = text.find_last_of(' ', end);
    return static_cast<int>(end - (separator == std::string::npos ? 0U : separator + 1U) + 1U);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】跨分隔符 basename 顯示
// 情境：診斷介面同時收到 Unix `/` 與 Windows `\` 路徑，只需顯示最後一個 component。
// 為何使用本章主題：find_last_of("/\\") 一次找到兩種分隔符中最右者，再從下一位置建立結果；
//       相較先正規化整條路徑少一次中間修改。
// 設計：1. 找最後一個 slash/backslash；2. 沒找到就從 0 開始；3. 複製尾端 component 回傳。
// 成本：時間 O(N)、額外空間 O(B)，N 是 path 長度，B 是 basename 長度。
// 上線注意：尾端分隔符會得到空名稱，且這不處理 drive root、UNC、`.`/`..`；正式路徑操作用 filesystem。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find_last_of.cpp' -o '/tmp/codex_cpp_C_String_find_last_of' && '/tmp/codex_cpp_C_String_find_last_of'
//
// === 預期輸出（節錄）===
// find_last_of: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
