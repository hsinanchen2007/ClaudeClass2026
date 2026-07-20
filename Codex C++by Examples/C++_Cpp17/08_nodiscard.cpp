/*
 * C++17 教科書：[[nodiscard]]
 *
 * 標在 function 上表示忽略回傳值通常是 bug，compiler 應發出診斷。適合 error/result、
 * resource handle、validation 與 pure computation。C++17 已可標 function、class 與 enum；
 * C++20 再加入理由字串 `[[nodiscard("why")]]`，並擴充 constructor 相關診斷語意。
 * 本檔保持 C++17，只使用無理由字串的 function 形式。
 *
 * 【處理結果】真正不需要時可 `static_cast<void>(call())` 明確表意，但不應濫用來關 warning。
 * attribute 不會改 return type、ABI 或 runtime；compiler diagnostic 也不是安全邊界。
 * 【設計】每個 getter 都 nodiscard 可能產生 warning fatigue；用在忽略會造成錯誤的 API。
 * 【測試】本教材使用 -Werror，所以所有 nodiscard 結果都被消費。
 * 【面試題】nodiscard 能否保證 caller 檢查 error？不能，只要求別無聲丟棄值。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace basic {
[[nodiscard]] bool valid_port(int port) { return port > 0 && port <= 65535; }

void demo() {
    const bool valid = valid_port(443);
    assert(valid && !valid_port(70000));
}
}  // namespace basic

namespace leetcode {
// LeetCode 704：Binary Search。O(log n) time / O(1) space。
[[nodiscard]] int leetcode_binary_search(const std::vector<int>& nums, int target) {
    std::size_t left = 0U;
    std::size_t right = nums.size();
    while (left < right) {
        const std::size_t middle = left + (right - left) / 2U;
        if (nums[middle] < target) left = middle + 1U;
        else right = middle;
    }
    return left < nums.size() && nums[left] == target ? static_cast<int>(left) : -1;
}

void leetcode_test() {
    assert(leetcode_binary_search({-1, 0, 3, 5, 9, 12}, 9) == 4);
    assert(leetcode_binary_search({-1, 0, 3, 5, 9, 12}, 2) == -1);
}
}  // namespace leetcode

// 【實務案例】儲存結果：強迫 caller 面對 path/content 驗證結果，不讓失敗被靜默忽略。
namespace practical {
struct SaveResult {
    bool ok;
    std::string message;
};

[[nodiscard]] SaveResult practical_save(const std::string& path,
                                        const std::string& content) {
    if (path.empty()) return {false, "path 不可為空"};
    if (content.empty()) return {false, "content 不可為空"};
    return {true, "saved"};  // 教材不真的寫檔，維持 deterministic。
}

void practical_test() {
    const SaveResult success = practical_save("report.txt", "ok");
    const SaveResult failure = practical_save("", "ok");
    assert(success.ok && !failure.ok);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "nodiscard：結果契約、Binary Search、save result 測試通過\n";
}

// 【延伸練習】列出可合理忽略與不可忽略的 API，各決定是否 nodiscard，避免 warning fatigue。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_nodiscard.cpp' -o '/tmp/codex_cpp_C_Cpp17_08_nodiscard' && '/tmp/codex_cpp_C_Cpp17_08_nodiscard'
//
// === 預期輸出（節錄）===
// nodiscard：結果契約、Binary Search、save result 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
