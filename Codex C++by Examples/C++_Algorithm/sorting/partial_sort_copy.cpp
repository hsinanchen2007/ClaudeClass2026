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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1337. The K Weakest Rows in a Matrix（矩陣中戰鬥力最弱的 K 行）
// 題目：每列 1 表士兵且都在 0 前，按士兵數升冪、相同時按 row index，回最弱 K 行；
// 範例矩陣取 3 行可得 [2,0,3]。
// 為何使用本章主題：先把每列轉成 {soldiers,index}，partial_sort_copy 再把最小 K 個
// 排序寫到固定輸出，不修改 rows 來源。
// 思路：1. count 每列士兵；2. 建 K 格 weakest；3. 依士兵數/index 選取排序；4. 抽出 index。
// 複雜度：時間 O(R*C+R log K)、額外空間 O(R+K)，R/C 為列數/欄數。
// 易錯點：k 必須非負且不超過列數；回傳 output end 可能早於預配置尾端，需 erase 未寫區。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】Immutable Latency Snapshot 最快 K 筆
// 情境：原 latency_ms 排列是報表觀測順序，不可重排；另需一份升冪的最快 K 筆供
// quick view，K 可大於樣本數。
// 為何使用本章主題：partial_sort_copy 同時保留來源、選取並排序固定大小輸出，比先
// 複製全部再 partial_sort 更節省目的空間。
// 設計：1. 輸出大小=min(K,N)；2. partial_sort_copy 到完整輸出；3. 依回傳 end 清掉未寫尾段。
// 成本：時間 O(N log K)、額外空間 O(min(K,N))，N 為樣本數。
// 上線注意：output 必須 resize 而非只 reserve；相同 latency 的先後不應被視為穩定契約。
// -----------------------------------------------------------------------------
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
