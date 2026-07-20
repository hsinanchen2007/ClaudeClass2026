/*
 * std::partition：原地分成 predicate=true 與 false 兩區
 * ======================================================
 * 回傳第一個 false 的 iterator，也就是分界。時間 O(N)，交換次數有線性上界。
 * 不保證任一區內原始順序；若順序有業務意義，改用 stable_partition。
 *
 * 需要至少 forward iterator。交換會改變元素位置，所有指向元素的 iterator/reference
 * 雖未必失效，卻可能指向「另一個值」；外部索引或 position-based invariant 要重建。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode 905：Sort Array By Parity；題目不要求群組內順序，partition 正適合。
std::vector<int> leetcode_sort_array_by_parity(std::vector<int> nums) {
    std::partition(nums.begin(), nums.end(),
                   [](int value) { return value % 2 == 0; });
    return nums;
}

struct Task {
    int id;
    bool ready;
};

// 實務：把 ready task 移到前半，回傳可立即 dispatch 的數量。
std::size_t practical_group_ready_tasks(std::vector<Task>& tasks) {
    const auto boundary = std::partition(
        tasks.begin(), tasks.end(),
        [](const Task& task) { return task.ready; });
    return static_cast<std::size_t>(std::distance(tasks.begin(), boundary));
}

int main() {
    std::vector<int> values{1, 2, 3, 4, 5, 6};
    const auto boundary = std::partition(values.begin(), values.end(),
                                         [](int value) { return value < 4; });
    assert(std::all_of(values.begin(), boundary,
                       [](int value) { return value < 4; }));
    assert(std::none_of(boundary, values.end(),
                        [](int value) { return value < 4; }));

    const auto parity = leetcode_sort_array_by_parity({3, 1, 2, 4});
    assert(std::is_partitioned(parity.begin(), parity.end(),
                               [](int value) { return value % 2 == 0; }));

    std::vector<Task> tasks{{1, false}, {2, true}, {3, false}, {4, true}};
    assert(practical_group_ready_tasks(tasks) == 2U);
    assert(std::is_partitioned(tasks.begin(), tasks.end(),
                               [](const Task& task) { return task.ready; }));

    std::cout << "partition：原地分區、LC905 與任務 dispatch 測試通過\n";
}

/*
 * 易錯陷阱：
 * - partition 不是 sort；true 區內順序未指定，測試不可硬寫某個排列。
 * - 回傳 boundary 可把範圍視為 [begin,boundary) 與 [boundary,end)。不要解參考
 *   boundary，除非先確認 boundary != end。
 * - predicate 必須能反覆一致分類；依隨機數或會變動時鐘分類會破壞後續推理。
 * - 若物件 swap 很昂貴，partition 仍可能大量搬動；可改 partition_copy 產生新 view。
 *
 * 面試比較：partition 平均/保證線性分類；stable_partition 保留順序但通常需要
 * 額外記憶體，無緩衝實作可能較昂貴。題目不要求順序時不要支付穩定成本。
 *
 * 練習：把 ready tasks 前半直接交給 worker。思考 dispatch 後 erase 前半會使哪些
 * iterator 失效；較安全做法是保存 boundary offset，再 erase 或 move 到另一容器。
 */
