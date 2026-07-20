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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 278. First Bad Version（第一個錯誤版本）
// 題目：版本 1..n 從某一版起全為 bad，找第一個 bad；n=5、first_bad=4 時回傳 4。
// 為何使用本章主題：binary search 是替代 API；線性 legacy 實作標 deprecated，示範以警告導向較快版本而非直接刪除。
// 思路：1. 維持包含答案的 [left,right]；2. middle 為 bad 就收縮右界；3. 否則把左界移到 middle+1。
// 複雜度：N 為版本數；新 API 時間 O(log N)、額外空間 O(1)；legacy 時間 O(N)。
// 易錯點：middle 用 left+(right-left)/2 避免加法溢位；deprecated 只發診斷，不會阻止舊函式被呼叫。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】事件傳送 API 漸進遷移
// 情境：舊 send_legacy 無法回報失敗，要讓呼叫端逐步改用會回傳成功狀態的新 practical_send。
// 為何使用本章主題：[[deprecated]] 保留相容期並在編譯時指出替代函式，比突然移除 API 更利於分批遷移。
// 設計：1. 保留舊函式並附替代訊息；2. 新函式驗 payload 非空；3. 呼叫端消費 bool 結果。
// 成本：本教材驗證 payload.empty() 為 O(1)；真正傳送成本取決於網路與重試，attribute 無 runtime 成本。
// 上線注意：需透過 telemetry 確認舊呼叫歸零並排 removal 版本；bool 太弱時應回傳帶原因的 result。
// -----------------------------------------------------------------------------
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

// 【延伸練習】規劃兩階段 API 遷移：先警告與替代訊息，再於 major release 移除舊函式。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_deprecated_attribute.cpp' -o '/tmp/codex_cpp_C_Cpp14_07_deprecated_attribute' && '/tmp/codex_cpp_C_Cpp14_07_deprecated_attribute'
//
// === 預期輸出（節錄）===
// deprecated：安全遷移、First Bad Version、send API 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
