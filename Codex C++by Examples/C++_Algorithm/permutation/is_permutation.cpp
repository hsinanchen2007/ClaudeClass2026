/*
 * std::is_permutation：判斷兩範圍是否含相同多重集合
 * ===================================================
 * 順序可不同，但每個等價元素的出現次數必須相同。C++14 起四 iterator overload
 * 可明確傳兩個 end；優先使用，避免第二範圍長度不足造成越界。
 *
 * 最壞時間 O(N^2)、額外空間 O(1)（標準一般演算法）；若已排序可 O(N) 比較，
 * 若可 hash 則 unordered_map 計數通常平均 O(N) 但需額外記憶體。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 242. Valid Anagram（有效的字母異位詞）
// 題目：判斷 lhs 與 rhs 是否含相同字元及相同次數，順序可不同；例如 anagram/nagaram
// 為 true，rat/car 為 false。
// 為何使用本章主題：is_permutation 直接比較兩序列的多重集合與 multiplicity，四
// iterator overload 也會處理長度不同；固定小寫字母表可用 frequency 得到 O(N)。
// 思路：1. 將兩個完整字串範圍傳入；2. 由演算法驗證每個等價字元出現次數相同。
// 複雜度：最壞時間 O(N^2)、額外空間 O(1)，N 為字串長度。
// 易錯點：不是只比 distinct membership；自訂 equality 必須是可傳遞的等價關係。
// -----------------------------------------------------------------------------
bool leetcode_is_anagram(const std::string& lhs, const std::string& rhs) {
    return std::is_permutation(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

struct LineItem {
    int sku;
    int quantity;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單重試 Payload 冪等驗證
// 情境：同一訂單重試時行項順序可能改變，但每個 sku/quantity pair 及其重複次數都
// 必須與首次 request 完全相同。
// 為何使用本章主題：is_permutation 忽略位置但保留 multiplicity，比 sort 後修改來源
// 更直接，適合小型 generic payload。
// 設計：1. 對兩個完整 LineItem 範圍比較；2. equality 同時檢查 sku 與 quantity。
// 成本：最壞時間 O(N^2)、額外空間 O(1)，N 為訂單行數。
// 上線注意：同 sku 拆成多行與合併成一行目前不等價；若業務允許，應先按 sku aggregate 並檢查總量溢位。
// -----------------------------------------------------------------------------
bool practical_same_order_payload(const std::vector<LineItem>& first,
                                  const std::vector<LineItem>& retry) {
    return std::is_permutation(
        first.begin(), first.end(), retry.begin(), retry.end(),
        [](const LineItem& lhs, const LineItem& rhs) {
            return lhs.sku == rhs.sku && lhs.quantity == rhs.quantity;
        });
}

int main() {
    const std::vector<int> a{1, 2, 2, 3};
    const std::vector<int> b{2, 3, 2, 1};
    const std::vector<int> c{1, 2, 3, 3};
    assert(std::is_permutation(a.begin(), a.end(), b.begin(), b.end()));
    assert(!std::is_permutation(a.begin(), a.end(), c.begin(), c.end()));

    assert(leetcode_is_anagram("anagram", "nagaram"));
    assert(!leetcode_is_anagram("rat", "car"));
    assert(!leetcode_is_anagram("aa", "a"));

    const std::vector<LineItem> order{{10, 2}, {20, 1}, {10, 2}};
    assert(practical_same_order_payload(order, {{20, 1}, {10, 2}, {10, 2}}));
    assert(!practical_same_order_payload(order, {{20, 1}, {10, 4}}));

    std::cout << "is_permutation：anagram 與冪等訂單驗證測試通過\n";
}

/*
 * 易錯陷阱：
 * - 它比較 multiplicity，不只是「每種值都出現過」；{1,1,2} != {1,2,2}。
 * - predicate 必須定義等價關係（自反、對稱、傳遞）。用近似浮點 epsilon 可能不
 *   傳遞，破壞演算法推理；先量化或 canonicalize。
 * - 大資料用 O(N^2) 可能太慢；字母表固定的 LC242 用 26 格 frequency 最佳。
 * - 四 iterator overload 會先辨識長度；不要退回只給 second_begin 的舊介面。
 *
 * 面試選型：小型 generic range 可用 is_permutation；可修改副本時 sort+equal 為
 * O(N log N)；hashable 且記憶體足夠可 frequency map 平均 O(N)。
 *
 * 生命週期：比較期間兩範圍不可被修改；predicate capture 的設定也要固定。
 * 若 range 很大且元素昂貴，先比較長度與廉價 fingerprint 可快速拒絕，但 hash
 * collision 代表 fingerprint 只能是 filter，最終仍需精確比較。
 *
 * 練習：訂單比對改成同 sku 的 quantity 可拆成多行，需先 aggregate 後比較，說明
 * 為何目前 pair-level permutation 無法把 {sku10,1}+{sku10,1} 視為 {sku10,2}。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'is_permutation.cpp' -o '/tmp/codex_cpp_C_Algorithm_permutation_is_permutation' && '/tmp/codex_cpp_C_Algorithm_permutation_is_permutation'
//
// === 預期輸出（節錄）===
// is_permutation：anagram 與冪等訂單驗證測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
