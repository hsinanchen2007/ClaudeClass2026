/*
 * std::string 到 std::string_view 的轉換：零拷貝唯讀視窗
 *
 * C++17 起，string 可隱式轉成 string_view。view 只保存 pointer + length，不擁有資料；
 * 建立是 O(1)，非常適合唯讀參數與切片。代價是呼叫者必須確保底層字串活得夠久，
 * 且修改 string 導致重新配置時，既有 view 會懸空。
 */

#include <cassert>
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>

namespace {

std::size_t count_ascii_digits(const std::string_view text) {
    std::size_t count = 0U;
    for (const char ch : text) {
        if (ch >= '0' && ch <= '9') {
            ++count;
        }
    }
    return count;
}

void basic_demo() {
    const std::string owned = "item-2048";
    const std::string_view view = owned;  // O(1)，沒有字元複製。
    const std::string_view suffix = view.substr(5U);
    assert(suffix == "2048");
    assert(count_ascii_digits(owned) == 4U);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 125. Valid Palindrome（驗證回文）
// 題目：忽略非英數與大小寫後判斷字串是否回文；Panama 句為 true，race a car 為 false。
// 為何使用本章主題：string 可隱式轉成 string_view，函式以零拷貝 view 接收 string、literal 等來源；
//       雙索引只讀原 buffer，不需要 owning 副本。
// 思路：1. 左右索引夾住 view；2. 跳過非英數；3. 將兩端轉小寫比較；4. 相同就向內縮。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 的 byte 數。
// 易錯點：cctype 前先轉 unsigned char；此解法只承諾 ASCII，且 view 不能比來源活得更久。
// -----------------------------------------------------------------------------
bool leetcode_is_ascii_palindrome(const std::string_view text) {
    std::size_t left = 0U;
    std::size_t right = text.size();
    while (left < right) {
        while (left < right && std::isalnum(static_cast<unsigned char>(text[left])) == 0) {
            ++left;
        }
        while (left < right &&
               std::isalnum(static_cast<unsigned char>(text[right - 1U])) == 0) {
            --right;
        }
        if (left < right) {
            const int a = std::tolower(static_cast<unsigned char>(text[left]));
            const int b = std::tolower(static_cast<unsigned char>(text[right - 1U]));
            if (a != b) {
                return false;
            }
            ++left;
            --right;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP header 名稱零拷貝切片
// 情境：從 `Content-Type: text/plain` 取得冒號前的名稱，找不到冒號時回空 view。
// 為何使用本章主題：string_view::substr 只回 pointer+length，相較 std::string substr 不配置，
//       適合在同一 request buffer 生命週期內做解析。
// 設計：1. find 第一個冒號；2. npos 回空；3. 否則回 [0, colon) 借用片段。
// 成本：搜尋時間 O(N)、額外空間 O(1)，N 是 line 長度。
// 上線注意：回傳空 view 無法區分缺冒號與空 header 名；還要驗 token grammar、trim 規則，
//       且不得保存 temporary string 所產生的結果。
// -----------------------------------------------------------------------------
std::string_view practical_header_name(const std::string_view line) {
    const std::size_t colon = line.find(':');
    return colon == std::string_view::npos ? std::string_view{} : line.substr(0U, colon);
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_is_ascii_palindrome("A man, a plan, a canal: Panama"));
    assert(!leetcode_is_ascii_palindrome("race a car"));

    const std::string line = "Content-Type: text/plain";
    assert(practical_header_name(line) == "Content-Type");
    std::cout << "operator string_view: tests passed\n";
}

/*
 * 【禁止】`return std::string_view(std::string("temporary"));`：函式回傳時即懸空。
 * 【陷阱】view 的 data 不保證在 view 結尾有 NUL，不能直接傳給只收 C 字串的 API。
 * 【面試】何時參數用 const string&，何時用 string_view？若只讀且接受多種字元來源，
 * 用 view；若需 NUL 結尾、保存參考或與擁有權互動，const string& 更合適。
 * 【練習】寫 value_part(line)，回傳冒號後略過前導空白的 view。
 */

/*
 * 【教科書補充：view 失效不只發生在 reallocation】
 * - owner 的 clear/erase/resize 可能讓 view 涵蓋已不再存在的元素，即使 capacity 與 data() 沒變。
 * - owner 原地修改字元時，view 會看到新內容，但 view 自己的長度不會跟著 owner 自動調整。
 * - `practical_header_name(std::string{"X: y"})` 回傳後立即懸空；參數改 const string& 也不會延長到 call 之後。
 * - 回傳借用 view 的 API 應在名稱/文件標示 owner 契約；需要保存時回傳 owning string。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'operator_string_view.cpp' -o '/tmp/codex_cpp_C_String_operator_string_view' && '/tmp/codex_cpp_C_String_operator_string_view'
//
// === 預期輸出（節錄）===
// operator string_view: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
