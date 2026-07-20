/*
 * Lambda 教科書 02：capture modes 與生命週期
 *
 * [] 不 capture；[x] value copy；[&x] reference；[=]/[&] 對 body 使用到的自動變數採預設；
 * [this] capture pointer，[*this]（C++17）capture object copy。capture 在建立 closure 時發生。
 *
 * 【value】closure 內保存一份，原變數之後改變不影響它；預設 call operator 是 const。
 * 【reference】不擁有對象，closure 若活過被 capture 變數就 dangling。async/callback 最危險。
 * 【this】只複製 pointer，不延長 object lifetime；owner 已析構後 callback 存取成員是 UB。
 * 【建議】列出必要 capture 比 [=]/[&] 更易 code review；跨 scope 要 capture value/owner。
 * 【常見陷阱】reference 或 this capture 不延長 lifetime，長期 callback 最容易 use-after-free。
 * 【面試題】[=] 是否複製 this object？傳統 [=] 隱式 capture this pointer，不是整個 object。
 * 【練習】把 practical_filter 改為 capture 上下限兩個值，測試原值改變不影響 closure。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    int threshold = 10;
    const auto by_value = [threshold](int value) { return value >= threshold; };
    const auto by_reference = [&threshold](int value) { return value >= threshold; };
    threshold = 20;
    assert(by_value(15));       // closure 保存舊值 10
    assert(!by_reference(15)); // reference 看見新值 20
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：輸入 nums 與 target，找出兩個相異位置使其值相加等於 target；[2,7,11,15]、9 回傳 [0,1]。
// 為何使用本章主題：target 以值捕獲固定查詢條件，seen 與 answer 以參考捕獲累積掃描狀態；
// lambda 是為展示捕獲模式的教學拆分，題目本身用一般迴圈即可完成。
// 思路：逐項計算 target-value；先查雜湊表是否已有互補值；沒有才記錄目前值與索引。
// 複雜度：平均時間 O(N)、額外空間 O(N)，N 是 nums 長度；雜湊碰撞最壞時間可退化。
// 易錯點：必須先查再插入以免同一元素配對自己；題目保證唯一答案，本函式無解時回 {-1,-1}。
// -----------------------------------------------------------------------------
std::pair<int, int> leetcode_two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> seen;
    std::pair<int, int> answer{-1, -1};
    const auto inspect = [target, &seen, &answer](int value, int index) {
        if (const auto found = seen.find(target - value); found != seen.end()) {
            answer = {found->second, index};
        } else {
            seen.emplace(value, index);
        }
    };
    for (std::size_t i = 0; i < nums.size() && answer.first < 0; ++i) {
        inspect(nums[i], static_cast<int>(i));
    }
    return answer;
}

void leetcode_test() {
    assert((leetcode_two_sum({2, 7, 11, 15}, 9) == std::pair<int, int>{0, 1}));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】固定門檻的數值篩選
// 情境：一批量測值需要依本次查詢的 minimum 留下合格項目，並維持原輸入順序。
// 為何使用本章主題：以值捕獲 minimum 讓 predicate 擁有建立當下的門檻快照，避免外部變數改動
// 影響執行中的篩選；相較參考捕獲也沒有跨作用域懸空風險。
// 設計：建立空結果；copy_if 逐項呼叫 predicate；將大於等於門檻的值追加至 output。
// 成本：時間 O(N)、結果空間 O(K)，N 是輸入數量、K 是通過篩選的數量，可能發生動態配置。
// 上線注意：需先定義門檻是否含等號及 NaN 等型別規則；大量資料可先 reserve 或改串流處理。
// -----------------------------------------------------------------------------
std::vector<int> practical_filter(const std::vector<int>& values, int minimum) {
    std::vector<int> output;
    std::copy_if(values.begin(), values.end(), std::back_inserter(output),
                 [minimum](int value) { return value >= minimum; });
    return output;
}

void practical_test() {
    assert(practical_filter({2, 9, 5, 12}, 6) == std::vector<int>({9, 12}));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "lambda capture：value/ref、Two Sum、filter 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_capture_modes.cpp' -o '/tmp/codex_cpp_C_Lambda_02_capture_modes' && '/tmp/codex_cpp_C_Lambda_02_capture_modes'
//
// === 預期輸出（節錄）===
// lambda capture：value/ref、Two Sum、filter 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
