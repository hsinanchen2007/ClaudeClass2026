/*
 * std::lower_bound：找第一個「不小於 key」的位置
 * =================================================
 * 對升冪範圍，它回傳第一個 >= key 的 iterator；若全都 < key，回 last。
 * 這個位置同時是「插入 key 後仍維持排序」的最左插入點。
 *
 * 複雜度：比較 O(log N)；random-access iterator 位移 O(log N)，其他 iterator
 * 的總位移可到 O(N)。回傳 iterator 仍屬原容器；容器一旦 reallocate/erase，
 * 必須依該容器規則判斷 iterator 是否失效。
 *
 * 比較器必須與排序相同，而且是 strict weak ordering。自訂 heterogeneous
 * 比較器的參數順序是 comp(element, key)。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 35. Search Insert Position（搜尋插入位置）
// 題目：輸入無重複升冪陣列 nums 與 target；若存在回索引，否則回插入後仍有序的
// 位置，例如 [1,3,5,6] 查 2 回 1。
// 為何使用本章主題：std::lower_bound 回第一個不小於 target 的位置，無論 target
// 存不存在都恰好是題目要求的最左插入點。
// 思路：1. 在整個 nums 求 lower_bound；2. 計算 begin 到結果的距離；3. 轉成 size_t 回傳。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：nums 必須升冪；target 大於全部元素時合法回 N；不要解參考只為判斷位置。
// -----------------------------------------------------------------------------
std::size_t leetcode_search_insert(const std::vector<int>& nums, int target) {
    return static_cast<std::size_t>(
        std::distance(nums.begin(),
                      std::lower_bound(nums.begin(), nums.end(), target)));
}

struct Deployment {
    long timestamp;
    std::string version;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】部署時間軸的 ceiling 查詢
// 情境：部署紀錄依 timestamp 升冪保存，給定查詢時間後要找當時或其後第一個版本；
// 若後續沒有部署則回傳 "none"。
// 為何使用本章主題：lower_bound 的異質比較可直接找第一筆 timestamp>=key；相較
// 線性掃描，適合對同一份唯讀時間軸反覆查詢。
// 設計：1. 以 Deployment.timestamp 與 long key 比較；2. 取得第一個不早於查詢時間
// 的 iterator；3. 對 end 回 sentinel，否則回該筆 version。
// 成本：時間 O(log N)、額外空間 O(1)，N 為部署紀錄數；回傳字串會產生值複製。
// 上線注意：載入時需驗證 timestamp 非遞減；正式 API 宜用 optional 取代可能與版本名衝突的 "none"。
// -----------------------------------------------------------------------------
std::string practical_first_deployment_at_or_after(
    const std::vector<Deployment>& log, long timestamp) {
    const auto it = std::lower_bound(
        log.begin(), log.end(), timestamp,
        [](const Deployment& item, long key) { return item.timestamp < key; });
    return it == log.end() ? "none" : it->version;
}

int main() {
    const std::vector<int> values{1, 3, 3, 3, 8};
    const auto first_three = std::lower_bound(values.begin(), values.end(), 3);
    assert(first_three == values.begin() + 1);
    assert(std::lower_bound(values.begin(), values.end(), 9) == values.end());

    // LeetCode 35：存在與不存在都可由同一 API 回答。
    assert(leetcode_search_insert({1, 3, 5, 6}, 5) == 2U);
    assert(leetcode_search_insert({1, 3, 5, 6}, 2) == 1U);
    assert(leetcode_search_insert({1, 3, 5, 6}, 7) == 4U);

    const std::vector<Deployment> log{{100, "v1"}, {250, "v2"}, {400, "v3"}};
    assert(practical_first_deployment_at_or_after(log, 200) == "v2");
    assert(practical_first_deployment_at_or_after(log, 401) == "none");

    std::cout << "lower_bound：最左邊界與插入點測試通過\n";
}

/*
 * 易錯點：不要在取得 iterator 後先 insert 再拿舊 iterator 算距離；vector insert
 * 可能重新配置。正確做法是先保存 index，再執行 insert。
 * 面試題：第一個 >= x、第一個 == x、計算 < x 的個數都可如何由 lower_bound
 * 推導？答案分別是 iterator 本身、再驗 *it==x、distance(begin,it)。
 * 練習：實作 first_deployment_after（嚴格 >），思考應改用哪個演算法。
 *
 * 【選型補充】
 * 若每次查詢前都要 sort，單次任務的總成本其實是 O(N log N)，不再只是查詢的
 * O(log N)。只有資料已排序、或同一批資料會被查很多次時，二分搜尋才真正划算。
 * 資料頻繁插入時可考慮 std::map/set；讀多寫少時 sorted vector 常更省記憶體、
 * cache locality 也更好。lower_bound 回 end 是正常結果，不是錯誤或例外。
 * 實務 API 應把「none」改成 optional/version result，避免字串 sentinel 與真資料衝突。
 */

/*
 * 【教科書補充：lower_bound 的分界模型】
 * - 最小前置條件是範圍依 comp(element,key) 分割；不必完整排序，但違反分割條件是未定義行為。
 * - 答案是第一個 `!comp(element,key)`，演算法不靠 operator== 判斷「相等」。
 * - RandomAccessIterator 有 O(log N) 比較與前進；ForwardIterator 比較仍是 O(log N)，前進可達 O(N)。
 * - 以回傳 iterator 做插入前，仍須遵守該容器 insert/reallocation 的失效規則。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'lower_bound.cpp' -o '/tmp/codex_cpp_C_Algorithm_binary_search_lower_bound' && '/tmp/codex_cpp_C_Algorithm_binary_search_lower_bound'
//
// === 預期輸出（節錄）===
// lower_bound：最左邊界與插入點測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
