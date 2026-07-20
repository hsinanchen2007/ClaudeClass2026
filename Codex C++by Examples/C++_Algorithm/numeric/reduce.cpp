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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1832. Check if the Sentence Is Pangram（判斷句子是否為全字母句）
// 題目：sentence 只含小寫英文字母，判斷 a..z 是否都至少出現一次；例如
// "thequickbrownfoxjumpsoverthelazydog" 回 true。
// 為何使用本章主題：每個字元先映成單一 bit mask，再由 reduce 以具結合/交換律的
// bitwise OR 合併；中間 vector 是教學用，手寫單趟可省空間。
// 思路：1. 每字元產生 1<<(ch-'a')；2. 以 OR reduce 全部 mask；3. 與低 26 bit 全 1 比較。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 sentence 長度，空間來自 masks。
// 易錯點：只接受 'a'..'z'；未驗證字元可能造成非法 shift count；OR 才適合任意重排。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】分片事件計數歸約
// 情境：每個 shard 已輸出 long long event count，中央報表只需合併成總事件數，輸入
// 次序沒有業務意義。
// 為何使用本章主題：整數加法在不溢位前提下可安全分組，符合 reduce 容許重排與未來
// 向量化/平行化的模型；若要求固定順序則 accumulate 更保守。
// 設計：1. 以 0LL 為 identity；2. 使用 plus 合併所有 shard_counts。
// 成本：時間 O(N)、額外資源依 reduce 實作，N 為 shard 數；本無 policy 版本通常為常數空間。
// 上線注意：有號 overflow 是 UB；需 checked/saturating 計數，平行版本 callback 不可有共享副作用。
// -----------------------------------------------------------------------------
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
