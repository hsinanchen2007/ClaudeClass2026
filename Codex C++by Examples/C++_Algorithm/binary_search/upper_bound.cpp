/*
 * std::upper_bound：找第一個「大於 key」的位置
 * ===============================================
 * 升冪範圍回第一個 > key；沒有則回 last。重複資料中：
 * [lower_bound(key), upper_bound(key)) 正好涵蓋所有等價元素。
 * upper_bound 的自訂比較器參數順序是 comp(key, element)，與 lower_bound 相反，
 * 這是 heterogeneous lookup 最容易寫反之處。
 *
 * 比較 O(log N)；非 random-access iterator 的位移仍可能 O(N)。演算法唯讀。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

// LeetCode 34：找 target 的首尾位置。
std::pair<int, int> leetcode_search_range(const std::vector<int>& nums,
                                          int target) {
    const auto first = std::lower_bound(nums.begin(), nums.end(), target);
    if (first == nums.end() || *first != target) {
        return {-1, -1};
    }
    const auto after = std::upper_bound(first, nums.end(), target);
    const int left = static_cast<int>(std::distance(nums.begin(), first));
    const int right = static_cast<int>(std::distance(nums.begin(), after) - 1);
    return {left, right};
}

struct Limit {
    int maximum;
    int price;
};

// 實務：階梯費率表用「第一個 maximum > usage」選級距。
int practical_rate_for_usage(const std::vector<Limit>& limits, int usage) {
    const auto it = std::upper_bound(
        limits.begin(), limits.end(), usage,
        [](int key, const Limit& item) { return key < item.maximum; });
    return it == limits.end() ? -1 : it->price;
}

int main() {
    const std::vector<int> values{1, 3, 3, 3, 8};
    assert(std::upper_bound(values.begin(), values.end(), 3) == values.begin() + 4);
    assert(std::upper_bound(values.begin(), values.end(), 8) == values.end());

    assert((leetcode_search_range({5, 7, 7, 8, 8, 10}, 8) ==
            std::pair{3, 4}));
    assert((leetcode_search_range({5, 7, 7, 8, 8, 10}, 6) ==
            std::pair{-1, -1}));

    const std::vector<Limit> limits{{100, 10}, {500, 8}, {1000, 6}};
    assert(practical_rate_for_usage(limits, 99) == 10);
    assert(practical_rate_for_usage(limits, 100) == 8);
    assert(practical_rate_for_usage(limits, 1000) == -1);

    std::cout << "upper_bound：最右邊界、LC34、階梯費率測試通過\n";
}

/*
 * 面試速記：<= key 的元素數量 = distance(begin, upper_bound(key))；
 * > key 的數量 = distance(upper_bound(key), end)。
 * 陷阱：若需求是「usage <= maximum 屬於本級」，上面的 strict 邊界規則要重新
 * 定義，不能只換函式名稱。先用具體邊界測試 99/100/101 再決定。
 * 練習：由 upper_bound 實作「最後一個 <= x 的 iterator」，處理空容器與 begin。
 *
 * 【邊界設計】
 * 商業規則最容易錯在等號。先把「100 屬於前級還是後級」寫成測試，再選
 * lower_bound 或 upper_bound。不要先選 API 再反推需求。若 upper_bound 回 begin，
 * 表示沒有元素 <= key；此時直接 --it 會越界。若回 end，則所有元素都 <= key。
 * LeetCode 測試要包含 target 位於頭、尾、重複區與不存在四種情況。
 * 實務費率表還應在載入時驗證 maximum 嚴格遞增，否則二分前置條件不成立。
 */
