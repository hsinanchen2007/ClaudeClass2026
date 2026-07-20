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

// LeetCode 67（Add Binary）：由低位相加，再反轉。
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

// 實務：產生可讀的資源鍵。少量欄位用 + 清楚；大量迴圈不要如此串接。
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
