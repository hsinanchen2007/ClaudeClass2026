/*
 * Lambda 教科書 04：init-capture（generalized lambda capture）
 *
 * C++14 起可在 capture list 建立新 closure member：`[name = expression]`。用途包括重新命名、
 * 型別轉換、計算衍生值，以及用 `p = std::move(unique)` 把 move-only ownership 交給 callback。
 * init-capture 的 `name` 只在 lambda body 可見；右側 expression 在外層 scope 求值。
 *
 * 【ownership】move capture 後，原 unique_ptr 變 null，closure 成為唯一 owner；closure 本身
 * 也因含 unique_ptr 而不可複製。std::function 到 C++23 仍要求 target 可複製；C++23 是
 * 另外新增 std::move_only_function 來承接 move-only callable，並未放寬 std::function。
 * 【evaluation】建立 lambda 時 expression 就執行，不是每次 call 才重新計算。
 * 【陷阱】`[&alias = local]` 仍是 reference，沒有延長 local lifetime。
 * 【面試題】init-capture 與 body 內 local 變數差異？前者是 closure persistent member。
 * 【練習】讓 practical_task 接收 vector<int> ownership 並回傳總和。
 */

#include <cassert>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    int base = 5;
    const auto scaled = [factor = base * 2](int value) { return factor * value; };
    base = 100;
    assert(scaled(3) == 30);  // factor 在建立 closure 時已算成 10。
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 724. Find Pivot Index（尋找陣列的中心下標）
// 題目：找最左側索引，使其左側總和等於右側總和；[1,7,3,6,5,6] 的答案是 3。
// 為何使用本章主題：lambda 以值捕獲預先算好的 total、以參考捕獲 left；目前語法是一般捕獲，
// 並未使用 `[total = expression]` init-capture，因此只是展示「建立時保存衍生值」的相近概念。
// 思路：先算全陣列總和；由左至右比較 left 與 total-left-value；比較後才把 value 加入 left。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 nums 長度；第一次走訪求總和，第二次找索引。
// 易錯點：必須在更新 left 前比較；找不到回 -1，空陣列也回 -1，整數總和可能溢位。
// -----------------------------------------------------------------------------
int leetcode_pivot_index(const std::vector<int>& nums) {
    const int total = std::accumulate(nums.begin(), nums.end(), 0);
    int left = 0;
    const auto is_pivot = [total, &left](int value) {
        const bool answer = left == total - left - value;
        left += value;
        return answer;
    };
    for (std::size_t i = 0; i < nums.size(); ++i) {
        if (is_pivot(nums[i])) return static_cast<int>(i);
    }
    return -1;
}

void leetcode_test() {
    assert(leetcode_pivot_index({1, 7, 3, 6, 5, 6}) == 3);
    assert(leetcode_pivot_index({1, 2, 3}) == -1);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】背景工作移交 payload 所有權
// 情境：factory 收到唯一擁有的字串 payload，回傳可延後執行的工作以取得其長度。
// 為何使用本章主題：init-capture 用 `owned = std::move(payload)` 把 unique_ptr 移入 closure；
// 相較捕獲裸指標，工作離開 factory 後仍擁有有效資料且所有權明確。
// 設計：呼叫端移交 unique_ptr；closure 成為唯一 owner；執行時判空並讀取 owned 字串長度。
// 成本：建立工作是 O(1) 的指標移動，呼叫 O(1)、closure 持有一份 payload 生命週期成本。
// 上線注意：回傳 closure 是 move-only，C++20 不能直接放入 std::function；也應明訂空 payload 行為。
// -----------------------------------------------------------------------------
auto practical_task(std::unique_ptr<std::string> payload) {
    return [owned = std::move(payload)]() {
        return owned == nullptr ? std::size_t{0} : owned->size();
    };
}

void practical_test() {
    auto payload = std::make_unique<std::string>("event");
    auto task = practical_task(std::move(payload));
    assert(payload == nullptr);
    assert(task() == 5U);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "init-capture：計算/move ownership、Pivot Index、task 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_init_capture.cpp' -o '/tmp/codex_cpp_C_Lambda_04_init_capture' && '/tmp/codex_cpp_C_Lambda_04_init_capture'
//
// === 預期輸出（節錄）===
// init-capture：計算/move ownership、Pivot Index、task 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
