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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 704. Binary Search（二分搜尋）
// 題目：在遞增陣列找 target 的索引，找不到回 -1；[-1,0,3,5,9,12] 找 9 回傳 4。
// 為何使用本章主題：[[nodiscard]] 提醒呼叫端不能無聲丟棄搜尋結果，因 -1 與索引都承載控制流程資訊。
// 思路：1. 維持半開區間 [left,right)；2. nums[middle]<target 時移左界，否則收右界；3. 最後驗相等。
// 複雜度：N 為元素數；時間 O(log N)、額外空間 O(1)。
// 易錯點：輸入必須排序；half-open 邊界不可混用，size_t 結果轉 int 前需符合題目長度限制。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】報表儲存前置驗證結果
// 情境：儲存入口要拒絕空 path 或 content，並要求 caller 明確處理成功狀態與訊息。
// 為何使用本章主題：[[nodiscard]] 標在 SaveResult API，忽略驗證失敗時編譯器可發診斷；它不是 runtime 強制。
// 設計：1. 依序驗 path 與 content；2. 失敗回 ok=false 與原因；3. 通過回 saved 結果。
// 成本：空字串檢查為 O(1)，SaveResult 訊息配置依長度 L 為 O(L)；本教材不執行實際 I/O。
// 上線注意：真正寫檔仍要處理權限、close、原子發布與錯誤碼；不要用 cast<void> 普遍壓掉 nodiscard。
// -----------------------------------------------------------------------------
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
