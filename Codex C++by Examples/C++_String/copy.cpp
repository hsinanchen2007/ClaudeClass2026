/*
 * std::string::copy：把子區段複製到 caller-provided char buffer
 *
 * `copied = text.copy(dest,count,pos)` 最多複製 count 個字元，回傳實際數量；pos 超界
 * 丟 out_of_range。重要：它不會自動加 '\0'。目的 buffer 必須至少有 count 空間，
 * 這是低階 interop API；一般 C++ 切片更適合 substr/string_view。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string text = "abcdef";
    std::array<char, 4U> buffer{};
    const std::size_t copied = text.copy(buffer.data(), 3U, 2U);
    buffer[copied] = '\0';  // copy 本身不做這件事。
    assert(copied == 3U);
    assert(std::string(buffer.data()) == "cde");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（尋找首次出現位置）
// 題目：輸入 haystack、needle，回傳 needle 首次出現的索引，找不到回 -1；"sadbutsad"、"sad" 回 0。
// 為何使用本章主題：此題解實際只用 find，沒有呼叫 string::copy；它是章內對照案例，
//       copy 適合已知位置後寫入外部 buffer，而不是取得搜尋索引，真正用途在下方固定欄位。
// 思路：1. 尋找 needle；2. npos 映射成 -1；3. 命中則回傳起始索引。
// 複雜度：直觀最壞時間 O(N*K)、額外空間 O(1)，N、K 是 haystack、needle 長度。
// 易錯點：不可把 npos 先轉 int；一般資料若位置可能超過 INT_MAX，窄化前要檢查。
// -----------------------------------------------------------------------------
int leetcode_find_first(const std::string& haystack, const std::string& needle) {
    const std::size_t position = haystack.find(needle);
    return position == std::string::npos ? -1 : static_cast<int>(position);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定八位元文字欄位編碼
// 情境：舊式檔案格式要求恰好 8 bytes，短值右側補空格，過長值目前截到 8 bytes，且不使用 NUL。
// 為何使用本章主題：string::copy 可直接寫入 caller-owned array 並回實際量，相較 substr
//       不建立中間字串，也不會誤加第九個終止字元。
// 設計：1. 先把整個欄位填空格；2. 計算 min(value.size, 8)；3. 從位置 0 複製該數量。
// 成本：欄寬固定時時間與空間皆 O(1)；一般寬度 W 則填充與複製為 O(W)。
// 上線注意：靜默截斷可能破壞識別碼，正式 encoder 應回報錯誤；copy 不補 NUL，consumer 必須按寬度讀。
// -----------------------------------------------------------------------------
std::array<char, 8U> practical_fixed_width_field(const std::string& value) {
    std::array<char, 8U> field{};
    field.fill(' ');
    const std::size_t count = value.size() < field.size() ? value.size() : field.size();
    static_cast<void>(value.copy(field.data(), count));
    return field;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_find_first("sadbutsad", "sad") == 0);
    assert(leetcode_find_first("leetcode", "leeto") == -1);
    const auto field = practical_fixed_width_field("Ada");
    assert(std::string(field.begin(), field.end()) == "Ada     ");
    std::cout << "copy: tests passed\n";
}

/*
 * 【陷阱】配置 char dest[count] 後再寫 dest[count]='\0' 會越界；需要 count+1。
 * 【面試】copy 與 memcpy 差異？copy 知道 string 長度並處理 pos/count；兩者都需目的容量。
 * 【練習】practical_fixed_width_field 遇到過長輸入時改成回傳 optional，避免靜默截斷。
 */

/*
 * 【copy 的完整契約】
 * 1. dest 指向至少 count 個可寫 char；API 不知道 buffer 容量。
 * 2. pos 必須 <= size；pos==size 合法且複製 0，pos>size 丟 out_of_range。
 * 3. 實際量是 min(count,size-pos)，以回傳值為準。
 * 4. 不自動 NUL terminate；若要 C 字串，buffer 至少 count+1 並手動補 NUL。
 * 5. 二進位/fixed-width 欄位通常不需要 NUL，應保留長度或固定寬度。
 *
 * 【API 選擇】owning slice 用 substr；零拷貝唯讀 slice 用 string_view；必須寫入
 * 既有 C buffer 才用 copy。
 * 【面試題】copy 會改來源嗎？不會，它是 const member。
 * 【練習】讓 fixed-width encoder 回報截斷，而不是默默丟掉尾端資料。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'copy.cpp' -o '/tmp/codex_cpp_C_String_copy' && '/tmp/codex_cpp_C_String_copy'
//
// === 預期輸出（節錄）===
// copy: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
