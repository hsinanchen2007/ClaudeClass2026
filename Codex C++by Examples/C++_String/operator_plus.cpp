/*
 * operator+：建立新的字串結果
 *
 * + 不修改左右運算元，而是回傳新 std::string。可組合 string、C 字串與 char；
 * 至少一側要能讓 overload 找到 std::string。連續 `a+b+c+d` 可能建立中間結果，
 * 熱路徑通常用 reserve + +=，或 C++20 format（若格式化需求較複雜）。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const std::string user = "alice";
    const std::string message = std::string("hello ") + user + '!';
    assert(message == "hello alice!");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 67. Add Binary（二進位字串相加）
// 題目：輸入兩個非空二進位字串，回傳其和；"1010" 加 "1011" 得 "10101"。
// 為何使用本章主題：核心迴圈實際用 += char 建 reversed，沒有使用 operator+ 連接大字串；
//       這是刻意對照，因熱迴圈用可重用 builder 比連鎖 `a+b+c` 更合適。
// 思路：1. 從兩輸入尾端逐位相加；2. 加入 carry；3. 把結果低位附加到 reversed；4. 反轉。
// 複雜度：時間 O(max(N,M))、額外空間 O(max(N,M))，N、M 是兩輸入位數。
// 易錯點：迴圈條件要包含最後 carry；輸入必須只含 0/1，且倒序索引用 size_t 時要防下溢。
// -----------------------------------------------------------------------------
std::string leetcode_add_binary(const std::string& a, const std::string& b) {
    std::string reversed;
    reversed.reserve(std::max(a.size(), b.size()) + 1U);
    std::size_t i = a.size();
    std::size_t j = b.size();
    int carry = 0;
    while (i > 0U || j > 0U || carry != 0) {
        int sum = carry;
        if (i > 0U) {
            sum += a[--i] - '0';
        }
        if (j > 0U) {
            sum += b[--j] - '0';
        }
        reversed += static_cast<char>('0' + (sum % 2));
        carry = sum / 2;
    }
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】物件儲存資源鍵組裝
// 情境：把 tenant 與 object id 組成 `tenant/<tenant>/objects/<id>`，欄位數固定且很少。
// 為何使用本章主題：operator+ 讓固定少量片段的結構一眼可讀；相較 stream/format 較輕量，
//       但大量迴圈或長片段應改用 reserve+append 以控制中間配置。
// 設計：1. 先以 std::string literal 啟動字串加法；2. 依固定順序加入 tenant、路徑與 id。
// 成本：總輸出長度 L 的時間至少 O(L)，額外空間 O(L)；連鎖加法可能建立中間結果。
// 上線注意：tenant/id 若來自外部要驗空值與 `/` 等保留字元；此鍵組裝不等於路徑安全或 URL encoding。
// -----------------------------------------------------------------------------
std::string practical_make_object_key(const std::string& tenant, const std::string& id) {
    return std::string("tenant/") + tenant + "/objects/" + id;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_add_binary("11", "1") == "100");
    assert(leetcode_add_binary("1010", "1011") == "10101");
    assert(practical_make_object_key("acme", "42") == "tenant/acme/objects/42");
    std::cout << "operator+: tests passed\n";
}

/*
 * 【陷阱】`"prefix" + number` 是指標位移，不是字串串接；先轉成 std::string。
 * 【面試】大量串接如何避免 O(n^2) 複製？預估總長度、reserve，再使用 append/+=。
 * 【練習】把 practical_make_object_key 改成先 reserve，並比較兩版可讀性與配置次數。
 */

/*
 * 【考前速查】
 * - + 建立新結果，不改左右 operands；+=/append 修改既有 string。
 * - 至少一側必須是 std::string，否則兩個 C pointer/literal 沒有字串加法。
 * - C++11 後 rvalue overload 可重用暫時字串 buffer，但不要據此假設零配置。
 * - 少量欄位優先可讀性；大量迴圈用 reserve+append，並以 profiler 驗證。
 * - 數字不會自動轉字串；使用 to_chars、format 或 to_string。
 * 【面試題】`"abc" + 1` 是什麼？array 退化 pointer 後位移，得到指向 "bc" 的 pointer。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'operator_plus.cpp' -o '/tmp/codex_cpp_C_String_operator_plus' && '/tmp/codex_cpp_C_String_operator_plus'
//
// === 預期輸出（節錄）===
// operator+: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
