/*
 * std::includes：判斷已排序 superset 是否包含另一個已排序 multiset
 * ================================================================
 * 兩邊必須以相同 comparator 排序。它考慮重複次數：{1,1,2} 包含 {1,1}，但
 * {1,2} 不包含 {1,1}。時間最多 O(N+M)，額外空間 O(1)。空需求永遠被包含。
 *
 * 演算法只讀範圍，不保存 iterator；比較期間不可修改來源。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1431. Kids With the Greatest Number of Candies（擁有最多糖果的孩子）
// 題目：原題給 candies 與 extraCandies；例如 [2,3,5,1,3] 加 3 回
// [true,true,true,false,true]。本 helper 改查 inventory 是否包含 requested multiset。
// 為何使用本章主題：排序後 std::includes 可驗證每種糖果值及其重複次數都包含於庫存；
// 這是以糖果情境展示 multiset containment 的教學延伸。
// 思路：1. 排序 inventory；2. 排序 requested；3. includes 驗每個需求 occurrence 都有供應。
// 複雜度：時間 O(N log N+M log M)、額外空間 O(N+M)，N/M 為庫存/需求筆數，參數按值複製。
// 易錯點：不得當作 LC1431 提交解；includes 會計算 duplicate multiplicity，兩邊排序規則必須相同。
// -----------------------------------------------------------------------------
bool leetcode_can_supply_candies(std::vector<int> inventory,
                                 std::vector<int> requested) {
    std::sort(inventory.begin(), inventory.end());
    std::sort(requested.begin(), requested.end());
    return std::includes(inventory.begin(), inventory.end(),
                         requested.begin(), requested.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Endpoint 權限包含檢查
// 情境：granted 與 required permissions 都以 canonical 升冪字串保存；只有角色包含
// endpoint 的全部需求權限時才授權。
// 為何使用本章主題：includes 對兩個已排序序列做線性 subset 檢查，可在首個缺項早退，
// 不需建立額外 hash set。
// 設計：1. 開發期驗證兩邊排序；2. 以 required 作第二個 multiset；3. 回包含結果。
// 成本：時間 O(N+M)、額外空間 O(1)，N/M 為 granted/required 數量。
// 上線注意：權限通常應 unique；大小寫、Unicode normalization 與 comparator 必須由同一 canonicalizer 保證。
// -----------------------------------------------------------------------------
bool practical_authorized(const std::vector<std::string>& granted,
                          const std::vector<std::string>& required) {
    assert(std::is_sorted(granted.begin(), granted.end()));
    assert(std::is_sorted(required.begin(), required.end()));
    return std::includes(granted.begin(), granted.end(),
                         required.begin(), required.end());
}

int main() {
    const std::vector<int> all{1, 1, 2, 3, 5};
    const std::vector<int> need{1, 1, 5};
    assert(std::includes(all.begin(), all.end(), need.begin(), need.end()));
    const std::vector<int> too_many{1, 1, 1};
    assert(!std::includes(all.begin(), all.end(), too_many.begin(), too_many.end()));

    assert(leetcode_can_supply_candies({4, 1, 2, 2}, {2, 4}));
    assert(!leetcode_can_supply_candies({4, 1, 2}, {2, 2}));

    assert(practical_authorized({"deploy", "read", "write"}, {"read", "write"}));
    assert(!practical_authorized({"read"}, {"read", "write"}));

    std::cout << "includes：multiset 包含、供應與權限驗證通過\n";
}

/*
 * 易錯陷阱：
 * - 未排序輸入違反前置條件；debug assert 只在開發期，production 邊界要 canonicalize。
 * - comparator 必須相同。若 granted 用 case-insensitive、required 用預設排序，結果
 *   無法推理。最好由同一型別封裝排序 invariant。
 * - 權限通常是 set，不應有 duplicate；若重複代表資料錯誤，先 unique/validate。
 * - includes 不是 substring 或 subsequence；它比較排序 multiset。
 *
 * 面試：為何 O(N+M)？兩個 iterator 單向前進，每項最多被比較常數次。若 required
 * 很小、inventory 支援 random access，也可逐項 binary_search，成本 M log N。
 *
 * 邊界：required 空時為 true；inventory 空但 required 非空時為 false；兩者都空
 * 仍為 true。這些集合 identity 應列入測試。若只需要 bool，includes 可遇到缺項
 * 立即早退，不必像 set_difference 建立完整缺項清單。
 *
 * 資料量很大且來自磁碟時，排序成本可能主導；若檔案已維護 sorted invariant，
 * includes 可 streaming 執行，不需要把所有元素同時載入記憶體。
 *
 * 練習：把 permission 改為 enum class，避免字串拼字與 locale 問題；建立
 * SortedPermissions 型別，constructor 排序去重，讓 includes 前置條件由型別保證。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'includes.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_includes' && '/tmp/codex_cpp_C_Algorithm_set_operations_includes'
//
// === 預期輸出（節錄）===
// includes：multiset 包含、供應與權限驗證通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
