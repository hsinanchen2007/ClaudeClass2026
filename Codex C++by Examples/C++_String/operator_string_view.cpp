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

// LeetCode 125（Valid Palindrome）：view 避免複製輸入；僅處理題目要求的 ASCII。
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

// 實務：切出 HTTP header 的名稱，不配置；回傳 view 仍依賴 line 的生命週期。
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
