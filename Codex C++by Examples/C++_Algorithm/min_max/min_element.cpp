/*
 * std::min_element：回傳範圍中第一個最小元素的 iterator
 * ====================================================
 * 空範圍回 last；非空範圍做 N-1 次比較，時間 O(N)、額外空間 O(1)。相同最小值
 * 回第一個。演算法不修改元素，但回傳 iterator 的有效期由容器決定。
 *
 * 解參考前永遠先檢查 it != end。自訂 comparator 比較的是 element/element。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vector>

// LeetCode 153：Find Minimum in Rotated Sorted Array 的線性基準解。
// 題目最佳解是 O(log N)，這裡刻意用 min_element 建立正確性基線。
int leetcode_find_min_linear(const std::vector<int>& nums) {
    assert(!nums.empty());
    return *std::min_element(nums.begin(), nums.end());
}

struct Server {
    int id;
    int active_requests;
};

// 實務：挑負載最低 server；tie 時 min_element 保留第一台，結果可預測。
int practical_least_loaded_server(const std::vector<Server>& servers) {
    assert(!servers.empty());
    const auto it = std::min_element(
        servers.begin(), servers.end(),
        [](const Server& lhs, const Server& rhs) {
            return lhs.active_requests < rhs.active_requests;
        });
    return it->id;
}

int main() {
    const std::vector<int> values{8, 2, 5, 2};
    const auto it = std::min_element(values.begin(), values.end());
    assert(it == values.begin() + 1);
    assert(std::min_element(values.end(), values.end()) == values.end());

    assert(leetcode_find_min_linear({3, 4, 5, 1, 2}) == 1);
    assert(practical_least_loaded_server({{1, 10}, {2, 3}, {3, 3}}) == 2);

    std::cout << "min_element：位置、LC153 基線、負載平衡測試通過\n";
}

/*
 * 面試追問：既然 rotated sorted 可 O(log N)，為何實務仍可能先寫 O(N)？它是簡單
 * 可驗證的 oracle，可用於 property test 驗證最佳化版本；但正式大資料仍應二分。
 * 練習：比較 server 時加入 id 作第二鍵，明確定義 tie-breaking。
 *
 * 【LeetCode 最佳化方向】
 * LC153 利用 rotated sorted 的單調結構可二分到 O(log N)；min_element 完全不利用
 * 排序資訊，所以是 O(N)。面試應先交付正確解，再說明如何利用題目條件最佳化。
 *
 * 【實務一致性】
 * active_requests 若會被其他 thread 同時更新，掃描期間可能看見不同時間點資料。
 * 要求嚴格決策時需 snapshot 或同步；容許近似負載平衡時可接受 eventual consistency。
 *
 * 易錯陷阱：把 comparator 寫成 `lhs.load-lhs.load` 或回 int 差值再轉 bool，不僅
 * 拼字易錯，也可能溢位；直接使用 `<`。LeetCode 二分版則要另測無 rotation。
 * 面試若問 tie，明確回答 min_element 回第一個最小值。
 * 實務回傳 server id 前，也要確認該 server 沒有被健康檢查標為 unavailable。
 * 練習：回傳 optional<int> 取代空表 assert，並加入 health 欄位過濾。
 */
