/*
 * std::string::find：由左至右尋找 substring、C 字串或 char
 *
 * 回傳第一個匹配的起始索引；找不到回傳 string::npos。第二參數 pos 是「從哪個
 * 索引開始考慮匹配」。不要把結果轉 int 再與 -1 比；npos 是 size_type 的最大值。
 * 標準只給上界，直觀最壞可視為 O((n-pos)*m)，實作可能使用更好的搜尋法。
 */

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace {

void basic_demo() {
    const std::string text = "one two two";
    assert(text.find("two") == 4U);
    assert(text.find("two", 5U) == 8U);
    assert(text.find('x') == std::string::npos);
    assert(text.find("") == 0U);  // 空 needle 在合法 pos 立即匹配。
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（尋找首次出現位置）
// 題目：輸入 haystack 與 needle，回傳第一次完整匹配的起始索引，找不到回 -1；空 needle 回 0。
// 為何使用本章主題：string::find 直接提供題目需要的第一個位置與 npos sentinel，不必手寫雙層比較。
// 思路：1. 呼叫 find 搜完整 pattern；2. 先判斷是否為 npos；3. 依題目介面轉成 -1 或 int 索引。
// 複雜度：直觀最壞時間 O(N*K)、額外空間 O(1)，N、K 是 haystack、needle 長度。
// 易錯點：不可寫 `if (find(...))`，位置 0 與 npos 會被顛倒解讀；窄化成 int 前一般程式要驗範圍。
// -----------------------------------------------------------------------------
int leetcode_str_str(const std::string& haystack, const std::string& needle) {
    const std::size_t position = haystack.find(needle);
    return position == std::string::npos ? -1 : static_cast<int>(position);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】單行 key=value 設定切割
// 情境：解析第一個等號，key 不得為空，而 value 可以繼續包含 `=`；`token=a=b` 應得到 token 與 a=b。
// 為何使用本章主題：find('=') 只定位第一個分隔點，較 split-all 或 regex 更符合此格式，
//       且搜尋本身不配置。
// 設計：1. 找第一個等號；2. 拒絕找不到或空 key；3. 以該位置切出 owning key/value。
// 成本：時間 O(N)、額外空間 O(N)，N 是 line 長度，兩個 substr 合計複製輸入內容。
// 上線注意：要再決定空 value、空白、escape、重複 key 與最大行長；npos 未檢查前不可做 +1。
// -----------------------------------------------------------------------------
std::optional<std::pair<std::string, std::string>>
practical_parse_assignment(const std::string& line) {
    const std::size_t equal = line.find('=');
    if (equal == std::string::npos || equal == 0U) {
        return std::nullopt;
    }
    return std::pair{line.substr(0U, equal), line.substr(equal + 1U)};
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_str_str("sadbutsad", "sad") == 0);
    assert(leetcode_str_str("leetcode", "leeto") == -1);
    assert(leetcode_str_str("abc", "") == 0);

    const auto parsed = practical_parse_assignment("token=a=b");
    assert(parsed.has_value());
    assert(parsed->first == "token" && parsed->second == "a=b");
    assert(!practical_parse_assignment("=missing-key").has_value());
    std::cout << "find: tests passed\n";
}

/*
 * 【npos 安全規則】
 * - 先判 `pos != npos`，才可做 substr(pos) 或 pos+needle.size()。
 * - `npos + 1` 會無號 wrap 成 0，可能把「找不到」誤當字串開頭。
 * - 找所有不重疊匹配：成功後從 pos + needle.size() 繼續；needle 空時另訂規格。
 *
 * 【面試】find 的 pos 是限制結果必須 >= pos，不是把字串先 substr；因此不需配置。
 * 【練習】實作 count_non_overlapping(text,needle)，明確處理空 needle。
 */

/*
 * 【搜尋所有匹配模板】
 * 1. `pos=find(needle,search_from)`。
 * 2. 若 npos 結束；否則處理 pos。
 * 3. 不重疊搜尋從 pos+needle.size()；重疊搜尋從 pos+1。
 * 4. needle 為空時必須另訂規格，否則游標可能永遠不前進。
 * 【陷阱】`if (text.find(x))` 把位置 0 當 false、npos 當 true，邏輯正好顛倒。
 * 【面試題】為何不先 substr(pos).find？那會建立副本並讓回傳索引變相對位置。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find.cpp' -o '/tmp/codex_cpp_C_String_find' && '/tmp/codex_cpp_C_String_find'
//
// === 預期輸出（節錄）===
// find: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
