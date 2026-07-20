/*
 * std::inner_product：兩序列的配對映射再依序聚合
 * =================================================
 * 預設計算 init + a[0]*b[0] + ...。只以第一範圍的 end 決定長度，因此第二範圍
 * 必須至少同長；否則越界是未定義行為。時間 O(N)、額外空間 O(1)。
 *
 * 與 transform_reduce 的關鍵差別：inner_product 保證左到右累積，可處理不具
 * 結合律的 operation；transform_reduce 可重排，換取向量化/平行化機會。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <numeric>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1572. Matrix Diagonal Sum（矩陣對角線元素的和）
// 題目：輸入 n*n 矩陣，回主對角線與副對角線總和，中央重疊格只算一次；例如
// 1..9 的 3*3 矩陣回 25。
// 為何使用本章主題：先以 iota 建 row index，再用 inner_product 的自訂 pair-op 把
// 每列兩個對角值映射後依序加總；第二個 index 範圍僅為符合二元 API，值未使用。
// 思路：1. 產生 0..N-1；2. 每列加 matrix[row][row] 與副對角值；3. N 為奇數時扣掉中央重複。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為矩陣邊長，index vector 佔主要額外空間。
// 易錯點：輸入必須每列至少 N 格；奇數矩陣中央只能算一次；一般迴圈可省 index 配置。
// -----------------------------------------------------------------------------
int leetcode_diagonal_sum(const std::vector<std::vector<int>>& matrix) {
    const std::size_t n = matrix.size();
    std::vector<std::size_t> index(n);
    std::iota(index.begin(), index.end(), std::size_t{0});
    const int both = std::inner_product(
        index.begin(), index.end(), index.begin(), 0, std::plus<>{},
        [&matrix, n](std::size_t row, std::size_t) {
            return matrix[row][row] + matrix[row][n - 1U - row];
        });
    return (n % 2U == 0U) ? both : both - matrix[n / 2U][n / 2U];
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單單價與數量點積
// 情境：unit_cents 與 quantities 以相同索引描述品項，需計算 sum(unit_cents[i]*quantity[i])
// 作訂單整數分幣總額。
// 為何使用本章主題：inner_product 直接表達兩個等長序列的逐項乘法再加總；0LL 初值
// 將 accumulator 與乘積結果提升為 long long。
// 設計：1. 驗證兩範圍等長；2. 以 quantities.begin 作第二序列起點；3. 使用預設乘加。
// 成本：時間 O(N)、額外空間 O(1)，N 為品項數。
// 上線注意：assert 不能驗證外部訂單；數量負值、幣別、乘法與總和溢位都需 runtime 檢查。
// -----------------------------------------------------------------------------
long long practical_order_total(const std::vector<long long>& unit_cents,
                                const std::vector<int>& quantities) {
    assert(unit_cents.size() == quantities.size());
    return std::inner_product(unit_cents.begin(), unit_cents.end(),
                              quantities.begin(), 0LL);
}

int main() {
    const std::vector<int> lhs{1, 2, 3};
    const std::vector<int> rhs{4, 5, 6};
    assert(std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0) == 32);

    // 自訂兩個 operation：外層聚合取最大，內層配對做絕對差。
    const int max_gap = std::inner_product(
        lhs.begin(), lhs.end(), rhs.begin(), 0,
        [](int best, int gap) { return (gap > best) ? gap : best; },
        [](int a, int b) { return (a > b) ? a - b : b - a; });
    assert(max_gap == 3);

    assert(leetcode_diagonal_sum({{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}) == 25);
    assert(practical_order_total({125, 250, 99}, {2, 1, 3}) == 797);

    std::cout << "inner_product：點積、矩陣對角線與訂單總額測試通過\n";
}

/*
 * 易錯陷阱：
 * 1. API 沒有 second_last；長度不符無法被 inner_product 自動發現。C++20 ranges
 *    演算法也未替這個舊 API 補長度檢查，production 應先 assert/validate。
 * 2. init=0 讓乘積與總和都可能在 int 溢位；金額使用 0LL。
 * 3. 自訂版本的參數順序是 accumulate_op(acc, transformed) 與 pair_op(a,b)。
 * 4. 浮點 dot product 對順序敏感；高精度需求考慮補償加總或專門線代函式庫。
 *
 * 面試：inner_product 不只「乘再加」；兩個 operation 讓它成為 zip-transform-fold。
 * 但可讀性若變差，一般 for loop 可能更好。演算法名稱清楚時才是抽象的價值。
 *
 * iterator 生命週期：回傳的是值，不會保存兩個範圍；然而演算法執行期間兩個範圍
 * 都必須存在且不可被重配置。若資料來自不同服務，先驗證長度與版本屬同一快照。
 *
 * 練習：實作 cosine similarity，處理零向量、型別升格與 sqrt；再比較單次掃描
 * 同時計算 dot/norm，和三次 inner_product 在 cache locality 上的差異。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'inner_product.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_inner_product' && '/tmp/codex_cpp_C_Algorithm_numeric_inner_product'
//
// === 預期輸出（節錄）===
// inner_product：點積、矩陣對角線與訂單總額測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
