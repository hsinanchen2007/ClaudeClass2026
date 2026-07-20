/*
 * std::partition_copy：單次掃描，把 true/false 分流到兩個輸出
 * ===========================================================
 * 輸入不被修改；回傳 pair<true_output_end,false_output_end>。時間 O(N)，每項只分類
 * 一次。兩個輸出範圍都必須有足夠容量，或使用 back_inserter 動態 append。
 *
 * 輸出範圍不可與輸入或彼此產生不安全重疊。分流後各自保留輸入相對順序。
 */

#include <algorithm>
#include <cassert>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2160. Minimum Sum of Four Digit Number After Splitting Digits（拆分四位數字後的最小和）
// 題目：原題將四個 digit 重組為兩數並最小化總和，例如 2932 回 52；本 helper 只把
// 任意 digits 穩定分成偶數與奇數兩流，因此不是完整 LC2160 解法。
// 為何使用本章主題：partition_copy 展示一次 predicate 掃描產生兩個輸出；它可作
// 後續教學管線的分流步驟，但 parity 並非原題最佳組合的核心規則。
// 思路：1. 建立 even/odd 輸出；2. digit%2==0 進 true 流；3. 其餘進 false 流。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 digit 數，兩個輸出合計 N 項。
// 易錯點：不得宣稱此函式解完 LC2160；完整題解需排序四個 digit 並平衡十位權重。
// -----------------------------------------------------------------------------
std::pair<std::vector<int>, std::vector<int>> leetcode_split_digits(
    const std::vector<int>& digits) {
    std::vector<int> even;
    std::vector<int> odd;
    std::partition_copy(digits.begin(), digits.end(), std::back_inserter(even),
                        std::back_inserter(odd),
                        [](int digit) { return digit % 2 == 0; });
    return {even, odd};
}

struct LogEntry {
    std::string text;
    bool error;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】錯誤與一般日誌雙路 Routing
// 情境：LogEntry 快照要同時送往 error 與 normal channel，來源供 audit 保留，兩條
// channel 內都要維持原到達順序。
// 為何使用本章主題：partition_copy 一次分類並穩定寫入兩個獨立輸出，比複製後各自
// erase 更省掃描與更清楚。
// 設計：1. 兩個輸出先 reserve 上界；2. 以 entry.error 分類；3. back_inserter 分別 append。
// 成本：時間 O(N)、額外空間 O(N)，N 為 log 數，輸出合計複製 N 筆。
// 上線注意：predicate 或配置丟例外會留下部分輸出；需要交易語意時應 staging 後一次發布。
// -----------------------------------------------------------------------------
std::pair<std::vector<LogEntry>, std::vector<LogEntry>> practical_route_logs(
    const std::vector<LogEntry>& logs) {
    std::vector<LogEntry> errors;
    std::vector<LogEntry> normal;
    errors.reserve(logs.size());
    normal.reserve(logs.size());
    std::partition_copy(logs.begin(), logs.end(), std::back_inserter(errors),
                        std::back_inserter(normal),
                        [](const LogEntry& entry) { return entry.error; });
    return {errors, normal};
}

int main() {
    const auto [even, odd] = leetcode_split_digits({4, 1, 2, 9, 6});
    assert((even == std::vector<int>{4, 2, 6}));
    assert((odd == std::vector<int>{1, 9}));

    const std::vector<LogEntry> logs{{"start", false}, {"disk", true},
                                     {"retry", false}, {"timeout", true}};
    const auto [errors, normal] = practical_route_logs(logs);
    assert(errors.size() == 2U && errors[0].text == "disk");
    assert(normal.size() == 2U && normal[1].text == "retry");
    assert(logs.size() == 4U);  // input 未改。

    std::cout << "partition_copy：穩定雙路分流與 log routing 測試通過\n";
}

/*
 * 易錯陷阱：
 * 1. 預配置 vector(size) 時可直接傳 begin；只有 reserve 時必須用 back_inserter。
 * 2. 兩個 output iterator 寫到同一容器可能因 reallocation 互相使 iterator 失效。
 * 3. back_inserter 方便但可能多次配置；已知上界可 reserve，卻不應把兩邊都 resize(N)
 *    後忘記依回傳 end erase 未使用尾端。
 * 4. predicate 丟例外時可能已有部分輸出，API 不提供 transaction rollback。
 *
 * 面試：為何 partition_copy 算穩定？每個輸出以輸入掃描順序 append，因此同類元素
 * 的相對順序保留；這和原地 partition 的未指定區內順序不同。
 *
 * 實務：雙路輸出適合 accepted/rejected、hot/cold、success/retry。若每類很多種，
 * 一次 partition_copy 不夠，考慮 map<key,vector> 的單次 routing loop。
 * 練習：回傳 rejected reason，避免只分流後失去失敗原因。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'partition_copy.cpp' -o '/tmp/codex_cpp_C_Algorithm_partitioning_partition_copy' && '/tmp/codex_cpp_C_Algorithm_partitioning_partition_copy'
//
// === 預期輸出（節錄）===
// partition_copy：穩定雙路分流與 log routing 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
