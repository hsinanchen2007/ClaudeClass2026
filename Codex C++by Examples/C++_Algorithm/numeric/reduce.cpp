/*
 * std::reduce：容許重排的歸約，目標是向量化或平行化
 * ===================================================
 * C++17 <numeric>。無 execution policy 也允許以未指定分組順序結合元素，與
 * accumulate 的固定左摺疊不同。operation 應具結合律，平行時通常也需交換律。
 * 時間 O(N)；額外資源依實作與 execution policy 而定。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// LeetCode 1832：Check if the Sentence Is Pangram；以 bit mask reduce。
bool leetcode_check_pangram(const std::string& sentence) {
    std::vector<unsigned int> masks;
    masks.reserve(sentence.size());
    for (const char ch : sentence) {
        masks.push_back(1U << static_cast<unsigned int>(ch - 'a'));
    }
    const unsigned int all = std::reduce(masks.begin(), masks.end(), 0U,
                                         [](unsigned int lhs, unsigned int rhs) {
                                             return lhs | rhs;
                                         });
    return all == ((1U << 26U) - 1U);
}

// 實務：分片統計可安全合併的整數計數；加法具結合/交換律（不考慮 overflow）。
long long practical_total_events(const std::vector<long long>& shard_counts) {
    return std::reduce(shard_counts.begin(), shard_counts.end(), 0LL,
                       std::plus<>{});
}

int main() {
    const std::vector<int> values{1, 2, 3, 4, 5};
    assert(std::reduce(values.begin(), values.end(), 0) == 15);
    assert(std::reduce(values.begin(), values.end(), 1, std::multiplies<>{}) == 120);

    assert(leetcode_check_pangram("thequickbrownfoxjumpsoverthelazydog"));
    assert(!leetcode_check_pangram("leetcode"));
    assert(practical_total_events({100, 200, 50}) == 350);

    // 反例只作說明：減法不具結合律，不可期待 reduce 等於左到右結果。
    const int ordered = std::accumulate(values.begin(), values.end(), 0,
                                        [](int a, int b) { return a - b; });
    assert(ordered == -15);

    std::cout << "reduce：可重排歸約、pangram 與分片計數測試通過\n";
}

/*
 * 易錯陷阱：
 * - 浮點加法數學上近似結合，機器上不是；reduce 的末位可能與 accumulate 不同。
 * - callback 不得依賴呼叫次序、外部 mutable state 或「一定從 init 開始」。
 * - `par`/`par_unseq` 不保證一定更快；小資料的排程成本可能大於收益。
 * - 標準平行演算法的後端與連結需求依工具鏈；本例使用無 policy overload，避免
 *   把 oneTBB 等實作相依性偷偷塞進最小教學編譯指令。
 * - 整數加法在有號 overflow 時是 UB；大量計數仍需更寬型別或 checked arithmetic。
 *
 * 面試選擇：需要 deterministic order 或不具結合律 -> accumulate；operation 可安全
 * 分組且已量測平行收益 -> reduce。不是看到「加總」就無條件換成 reduce。
 *
 * API 版本：`reduce(first,last)` 使用 value_type{} 當初值；明確傳 init 通常更好，
 * 因為空範圍語意與升格型別都一眼可見。execution policy overload 可能 terminate
 * 而非正常傳遞 callback 例外，撰寫 throwing operation 前要查該 overload 契約。
 *
 * 練習：做 min/max pair reduction，設計 associative combine；證明任意分組仍得到
 * 同結果，並處理空範圍 identity。
 */

/*
 * 【教科書補充：reduce 能重排，契約比 accumulate 嚴格】
 * - binary operation 必須能接受 T/value 的各種分組結果，且每次結果都可轉回 T。
 * - 因分組與順序可變，想要可重現結果時 operation 應具結合律；浮點加法通常做不到 bitwise 重現。
 * - 位元 mask 的 shift count 必須小於型別寬度；字元先驗證在 'a'..'z'，不能把任意 byte 拿來位移。
 * - 標準 execution-policy overload 中 callback 拋出的例外可能導致 terminate；平行路徑另需無 data race。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'reduce.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_reduce' && '/tmp/codex_cpp_C_Algorithm_numeric_reduce'
//
// === 預期輸出（節錄）===
// reduce：可重排歸約、pangram 與分片計數測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
