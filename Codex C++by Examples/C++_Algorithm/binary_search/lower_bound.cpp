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

// LeetCode 35：Search Insert Position。
std::size_t leetcode_search_insert(const std::vector<int>& nums, int target) {
    return static_cast<std::size_t>(
        std::distance(nums.begin(),
                      std::lower_bound(nums.begin(), nums.end(), target)));
}

struct Deployment {
    long timestamp;
    std::string version;
};

// 實務：找出第一筆時間 >= 查詢時間的部署，常用於時間軸查詢。
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
