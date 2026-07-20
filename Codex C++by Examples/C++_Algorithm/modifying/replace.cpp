/*
 * std::replace / replace_if / replace_copy：依條件替換元素
 * =======================================================
 * replace 在原範圍把等於 old_value 的元素指定為 new_value；replace_if 用 predicate。
 * replace_copy 系列保留來源、寫到目的端。皆為 O(N)，不改容器 size。
 * 若 new_value 參考到範圍內某元素，而該元素先被改寫，可能造成令人意外結果；
 * 正式程式宜先複製 replacement 值。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1299. Replace Elements with Greatest Element on Right Side（將每個元素替換為右側最大元素）
// 題目：原題要求每格換成其右側最大值且最後一格為 -1；本 helper 只把既有 -1
// sentinel 換成固定 default，例如 [8,-1,3] 變 [8,0,3]，不是完整 LC1299 解法。
// 為何使用本章主題：replace_copy 適合「符合固定舊值就換固定新值」並保留來源；
// 原題每格 replacement 不同，正式解法應由右往左維護 running maximum。
// 思路：1. 預留輸出容量；2. 掃描 readings；3. 遇 -1 輸出 default，其餘原樣複製。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 readings 筆數。
// 易錯點：不得把此固定替換 helper 當作 LC1299 題解；若 -1 可能是合法值，sentinel 模型本身不成立。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_replace_missing(const std::vector<int>& readings,
                                          int default_value) {
    std::vector<int> result;
    result.reserve(readings.size());
    std::replace_copy(readings.begin(), readings.end(),
                      std::back_inserter(result), -1, default_value);
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】公開錯誤計數正規化
// 情境：內部統計用負數表示採集錯誤，但公開報表只允許非負 count；匯出時將所有
// 負值改為 0，同時保留原 internal 資料供診斷。
// 為何使用本章主題：replace_copy_if 能依 predicate 寫入固定替代值且不修改來源；
// 相較原地 replace_if，可保留內外兩份不同語意的快照。
// 設計：1. 建立與來源等長輸出；2. predicate 判斷 value<0；3. 負值寫 0，其餘複製。
// 成本：時間 O(N)、額外空間 O(N)，N 為 internal 計數筆數。
// 上線注意：把不同錯誤都壓成 0 會失去原因，應另帶 status/metric；目的範圍必須先建立 N 個元素。
// -----------------------------------------------------------------------------
std::vector<int> practical_public_error_counts(
    const std::vector<int>& internal) {
    std::vector<int> result(internal.size());
    std::replace_copy_if(internal.begin(), internal.end(), result.begin(),
                         [](int value) { return value < 0; }, 0);
    return result;
}

int main() {
    std::vector<int> values{1, 2, 2, 3};
    std::replace(values.begin(), values.end(), 2, 9);
    assert((values == std::vector<int>{1, 9, 9, 3}));
    std::replace_if(values.begin(), values.end(),
                    [](int value) { return value > 5; }, 5);
    assert((values == std::vector<int>{1, 5, 5, 3}));

    assert((leetcode_replace_missing({8, -1, 3}, 0) ==
            std::vector<int>{8, 0, 3}));
    const std::vector<int> internal{5, -2, 7};
    assert((practical_public_error_counts(internal) ==
            std::vector<int>{5, 0, 7}));
    assert((internal == std::vector<int>{5, -2, 7}));
    std::cout << "replace：原地/複製替換與資料清理測試通過\n";
}

/*
 * 面試：replace 與 transform 差別？replace 只在符合條件時放固定值；transform
 * 可由每個輸入計算不同輸出。練習：將 out-of-range sensor 值換成 optional，
 * 思考固定 sentinel 可能與合法資料衝突的問題。
 *
 * 【LeetCode-style 設計】
 * leetcode_replace_missing 回新 vector，適合題目要求 immutable input；若允許原地
 * 修改，可直接 replace 並省一份 O(N) 空間。兩者時間都是 O(N)。
 *
 * 【實務資料契約】
 * 把所有負數換 0 可能掩蓋不同錯誤原因，只有公開 API 契約明確要求時才做。
 * 更完整設計會保留 status/error enum，顯示層才決定文字或預設值。
 * replace_copy 目的範圍仍需容量，本例先依來源 size 建好結果。
 *
 * 易錯陷阱：replace 不會改變容器長度，也不會重新排序；若替換的是排序 key，原本
 * sorted 的前置條件可能失效。面試需指出 replace_if predicate 接收元素且應無副作用。
 * LeetCode-style 測試還應含沒有 -1 與全部 -1。實務匯出則測來源完全未修改。
 * 練習：比較 replace_copy_if 與 transform 回傳 optional 的資料建模差異。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'replace.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_replace' && '/tmp/codex_cpp_C_Algorithm_modifying_replace'
//
// === 預期輸出（節錄）===
// replace：原地/複製替換與資料清理測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
