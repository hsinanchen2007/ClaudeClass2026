/*
 * std::set_intersection：兩個已排序 multiset 的交集
 * =================================================
 * 某值在兩邊分別出現 m/n 次，輸出 min(m,n) 次。時間 O(N+M)，輸出最多 min(N,M)。
 * 等價元素通常從第一範圍複製，若物件除 key 外還有 payload，這個來源語意很重要。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 350. Intersection of Two Arrays II（兩個陣列的交集 II）
// 題目：回兩陣列交集，每個值出現 min(count1,count2) 次；例如 [1,2,2,1] 與 [2,2]
// 回 [2,2]，順序不限。
// 為何使用本章主題：排序兩個副本後，set_intersection 正好實作 multiset 的最小重複
// 次數規則，輸出自然保持升冪。
// 思路：1. 排序 first/second；2. 預留較小輸入大小；3. 線性求交集。
// 複雜度：時間 O(N log N+M log M)、額外空間 O(N+M)，N/M 為兩輸入大小。
// 易錯點：LC350 要保留 multiplicity，不能先 unique；兩邊必須用相同 comparator 排序。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_intersect(std::vector<int> first,
                                    std::vector<int> second) {
    std::sort(first.begin(), first.end());
    std::sort(second.begin(), second.end());
    std::vector<int> output;
    output.reserve(std::min(first.size(), second.size()));
    std::set_intersection(first.begin(), first.end(), second.begin(), second.end(),
                          std::back_inserter(output));
    return output;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】雙資料中心共同部署版本
// 情境：east 與 west 維護升冪版本 ID，發布系統要取得兩邊都已部署的版本清單，供
// 安全回退選擇。
// 為何使用本章主題：set_intersection 以線性雙指標找共同 ID，輸出仍有序，可直接
// 餵給後續 includes/difference。
// 設計：1. 驗證兩邊排序；2. 求交集並 append 到 common；3. 回共同版本。
// 成本：時間 O(N+M)、額外空間 O(K)，N/M 為兩中心版本數、K 為交集大小。
// 上線注意：版本 ID 理論上應 unique；若有 duplicate，輸出依 multiset min 次數而非單一 ID。
// -----------------------------------------------------------------------------
std::vector<int> practical_common_versions(const std::vector<int>& east,
                                           const std::vector<int>& west) {
    assert(std::is_sorted(east.begin(), east.end()));
    assert(std::is_sorted(west.begin(), west.end()));
    std::vector<int> common;
    std::set_intersection(east.begin(), east.end(), west.begin(), west.end(),
                          std::back_inserter(common));
    return common;
}

int main() {
    assert((leetcode_intersect({1, 2, 2, 1}, {2, 2}) ==
            std::vector<int>{2, 2}));
    assert((leetcode_intersect({4, 9, 5}, {9, 4, 9, 8, 4}) ==
            std::vector<int>{4, 9}));

    assert((practical_common_versions({100, 101, 103}, {99, 101, 103, 104}) ==
            std::vector<int>{101, 103}));

    const std::vector<int> empty;
    assert(practical_common_versions(empty, {1, 2}).empty());

    std::cout << "set_intersection：LC350 與共同部署版本測試通過\n";
}

/*
 * 易錯陷阱：
 * - LC349 要 distinct intersection，LC350 要保留 min multiplicity；先辨清題目。
 * - output 不可覆蓋尚未讀完的 input；一般使用獨立 vector。
 * - 若 comparator 只看 id，兩側等價 Record 的其他欄位可能不同；輸出通常取第一側，
 *   不代表兩份 payload 已一致。需要一致性驗證時應另外比較 payload。
 * - `std::min(size)` 只是 reserve 最佳化，不是 correctness 必需。
 *
 * 面試：若一個集合遠小於另一個且後者 random access，可對小集合逐項 binary_search；
 * 若大小接近，線性雙指標通常更好。資料在資料庫時則應考慮 server-side join。
 *
 * 空範圍與任一範圍交集皆空；完全相同範圍的交集保留原 multiplicity。輸出順序
 * 由共同 comparator 決定，可直接繼續餵給另一個 sorted set algorithm。
 * 需要只判斷「是否有任一共同值」時，手寫雙指標可首個 match 早退，省下輸出。
 *
 * 實務資料若本來是 database sorted cursor，可以直接串流交集；但兩 cursor 的
 * collation/version 必須一致，否則「各自有序」仍不代表共同 comparator 下有序。
 *
 * 練習：實作 galloping intersection，針對 size ratio 極端的 sorted list benchmark；
 * 為 duplicate 定義輸出次數並用 property test 驗證。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'set_intersection.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_set_intersection' && '/tmp/codex_cpp_C_Algorithm_set_operations_set_intersection'
//
// === 預期輸出（節錄）===
// set_intersection：LC350 與共同部署版本測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
