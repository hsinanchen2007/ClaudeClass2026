/*
 * C++14 教科書：[[deprecated]] attribute
 *
 * `[[deprecated("改用 new_api")]]` 可標在 function、class、variable、enum 等宣告上；
 * 使用它時 compiler 通常發 warning 並顯示 migration message。它不改 ABI、不阻止呼叫，
 * 也不保證 runtime 行為；真正移除前仍需版本政策與相容期。
 *
 * 【遷移策略】先提供替代 API -> 標 deprecated + 明確訊息 -> 修 call sites ->
 * telemetry 確認無使用 -> major release 才刪除。不要只寫「deprecated」而不給替代方案。
 * 【教材驗證】本檔以 -Werror 編譯，因此不真的呼叫舊 API；註解中的呼叫若解除會得到診斷。
 * 【可攜性】attribute 可被 implementation 忽略，不能把它當 security/safety enforcement。
 * 【面試題】deprecated 與 delete 差異：前者仍可用但警告，後者 compile-time 禁止。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace basic {
[[deprecated("請改用 parse_number_checked")]]
int parse_number_legacy(const std::string& text) {
    return std::stoi(text);
}

bool parse_number_checked(const std::string& text, int& output) {
    if (text.empty() ||
        !std::all_of(text.begin(), text.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
        return false;
    }
    int candidate = 0;
    for (const char ch : text) {
        const int digit = ch - '0';
        if (candidate > (std::numeric_limits<int>::max() - digit) / 10) {
            return false;
        }
        candidate = candidate * 10 + digit;
    }
    output = candidate;
    return true;
}

void demo() {
    int value = 0;
    const bool parsed_42 = parse_number_checked("42", value);
    assert(parsed_42 && value == 42);
    const bool malformed_rejected = !parse_number_checked("4x", value);
    assert(malformed_rejected);
    const bool parsed_max = parse_number_checked(
        std::to_string(std::numeric_limits<int>::max()), value);
    assert(parsed_max);
    assert(value == std::numeric_limits<int>::max());
    const bool overflow_rejected = !parse_number_checked(std::string(100U, '9'), value);
    assert(overflow_rejected);
    // parse_number_legacy("42");  // 解除註解會產生 deprecated warning；-Werror 會阻止 build。
}
}  // namespace basic

namespace leetcode {
// LeetCode 278：First Bad Version。binary search O(log n)，API replacement 保留清楚 contract。
int leetcode_first_bad_version(int versions, int first_bad) {
    int left = 1;
    int right = versions;
    while (left < right) {
        const int middle = left + (right - left) / 2;
        if (middle >= first_bad) right = middle;
        else left = middle + 1;
    }
    return left;
}

[[deprecated("線性掃描太慢，請改用 leetcode_first_bad_version")]]
int first_bad_linear_legacy(int versions, int first_bad) {
    for (int version = 1; version <= versions; ++version) {
        if (version >= first_bad) return version;
    }
    return -1;
}

void leetcode_test() {
    assert(leetcode_first_bad_version(5, 4) == 4);
    assert(leetcode_first_bad_version(1, 1) == 1);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Request {
    std::string payload;
};

[[deprecated("請改用 practical_send，會回傳明確成功狀態")]]
void send_legacy(const Request&) {}

bool practical_send(const Request& request) {
    return !request.payload.empty();
}

void practical_test() {
    assert(practical_send(Request{"event"}));
    assert(!practical_send(Request{""}));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "deprecated：安全遷移、First Bad Version、send API 測試通過\n";
}
