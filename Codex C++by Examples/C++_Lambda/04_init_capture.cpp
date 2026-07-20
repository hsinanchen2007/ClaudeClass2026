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
// LeetCode 724：Find Pivot Index。init-capture 保存 total，reference capture 更新 left sum。
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

// 【實務案例】背景工作 ownership：init-capture 將 unique_ptr 移入 callback，離開 factory 仍安全。
namespace practical {
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
