/*
 * std::partial_sort_copy：保留來源，把最小 K 個排序後複製到固定輸出
 * ================================================================
 * K 由輸出範圍長度決定；回傳實際 output end。輸出比來源大時只寫 N 項，尾端原值
 * 保持不變，必須依回傳 iterator 截取。時間約 O(N log K)，來源不被修改。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode 1337：The K Weakest Rows in a Matrix。
std::vector<int> leetcode_k_weakest_rows(const std::vector<std::vector<int>>& matrix,
                                         int k) {
    struct Row {
        int soldiers;
        int index;
    };
    std::vector<Row> rows;
    rows.reserve(matrix.size());
    for (std::size_t i = 0; i < matrix.size(); ++i) {
        rows.push_back({static_cast<int>(std::count(matrix[i].begin(), matrix[i].end(), 1)),
                        static_cast<int>(i)});
    }
    std::vector<Row> weakest(static_cast<std::size_t>(k));
    const auto out_end = std::partial_sort_copy(
        rows.begin(), rows.end(), weakest.begin(), weakest.end(),
        [](const Row& lhs, const Row& rhs) {
            return (lhs.soldiers != rhs.soldiers) ? lhs.soldiers < rhs.soldiers
                                                   : lhs.index < rhs.index;
        });
    weakest.erase(out_end, weakest.end());
    std::vector<int> answer;
    for (const auto& row : weakest) {
        answer.push_back(row.index);
    }
    return answer;
}

// 實務：從 immutable latency snapshot 複製最快 K 筆，不破壞原報表順序。
std::vector<int> practical_fastest_samples(const std::vector<int>& latency_ms,
                                           std::size_t k) {
    std::vector<int> output(std::min(k, latency_ms.size()));
    const auto end = std::partial_sort_copy(latency_ms.begin(), latency_ms.end(),
                                            output.begin(), output.end());
    output.erase(end, output.end());
    return output;
}

int main() {
    const std::vector<int> source{9, 1, 8, 2, 7, 3};
    assert((practical_fastest_samples(source, 3U) == std::vector<int>{1, 2, 3}));
    assert((source == std::vector<int>{9, 1, 8, 2, 7, 3}));

    const std::vector<std::vector<int>> matrix{
        {1, 1, 0, 0, 0}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0},
        {1, 1, 0, 0, 0}, {1, 1, 1, 1, 1}};
    assert((leetcode_k_weakest_rows(matrix, 3) == std::vector<int>{2, 0, 3}));

    std::cout << "partial_sort_copy：LC1337 與 immutable top-K 測試通過\n";
}

/*
 * 易錯陷阱：
 * - output 必須先有 size；reserve 不夠。它不是 back_insert API。
 * - 回傳 end 可能早於 output.end；輸入少於 K 時不可讀未寫尾端。
 * - output 若長度 0 合法，回 begin；K 要先受業務/記憶體上限限制。
 * - source/output 不應不安全重疊；要原地請用 partial_sort。
 *
 * 面試比較：partial_sort_copy 適合 const source 與小固定 buffer；partial_sort 省輸出
 * copy 但破壞來源；priority_queue 適合 streaming。複雜度都常見 N log K，但常數與
 * memory access 不同，production 需 benchmark。
 *
 * 練習：輸出固定 std::array<Sample,10>，使用回傳 iterator 得到實際筆數；為 ties
 * 加 sequence number，確保 deterministic 結果。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'partial_sort_copy.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_partial_sort_copy' && '/tmp/codex_cpp_C_Algorithm_sorting_partial_sort_copy'
//
// === 預期輸出（節錄）===
// partial_sort_copy：LC1337 與 immutable top-K 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
