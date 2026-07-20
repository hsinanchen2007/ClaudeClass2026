/*
 * std::stable_partition：原地分區且保留各群組原始相對順序
 * ==========================================================
 * 回傳第一個 false。時間複雜度依額外記憶體：有足夠 buffer 通常 O(N)，配置失敗
 * 時可退化到 O(N log N) 次 swap；標準不保證固定記憶體成本。
 *
 * 相較 partition，穩定性是明確業務需求才支付的成本，例如 FIFO、時間序與 UI 排列。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 283. Move Zeroes（移動零）
// 題目：原地把所有 0 移到尾端並維持非零值順序；例如 [0,1,0,3,12] 變成
// [1,3,12,0,0]。
// 為何使用本章主題：stable_partition 以 value!=0 將非零穩定移到前段，語意正確；
// 但實作可能配置 buffer，不保證原題追問的 O(1) 額外空間，雙指標更適合正式面試解。
// 思路：1. 將非零分類為 true；2. 穩定分區整個 nums；3. 零自然集中到後段。
// 複雜度：有 buffer 時間 O(N)、額外空間可 O(N)；無 buffer 時交換成本可 O(N log N)。
// 易錯點：不要宣稱標準 stable_partition 必為 O(1) 空間；stable 只保證各群組內相對次序。
// -----------------------------------------------------------------------------
void leetcode_move_zeroes(std::vector<int>& nums) {
    std::stable_partition(nums.begin(), nums.end(),
                          [](int value) { return value != 0; });
}

struct Ticket {
    std::string name;
    bool vip;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】客服 Ticket VIP 優先且維持 FIFO
// 情境：Ticket 依到達順序保存；要把 VIP 移到前半，但 VIP 與普通兩群各自都必須
// 保留原到達順序，並回 VIP 數量。
// 為何使用本章主題：stable_partition 同時提供二分組與群內穩定性，比不穩定 partition
// 更符合可觀察的 FIFO 契約。
// 設計：1. 以 ticket.vip 分類；2. 穩定分區；3. 由 boundary distance 算 VIP 數。
// 成本：常見時間 O(N)、額外空間 O(N) buffer；配置失敗 fallback 可能 O(N log N) 次 swap。
// 上線注意：演算法可能配置且不是 thread-safe；hard real-time 路徑需預配置或採其他資料結構。
// -----------------------------------------------------------------------------
std::size_t practical_prioritize_vip(std::vector<Ticket>& tickets) {
    const auto point = std::stable_partition(
        tickets.begin(), tickets.end(),
        [](const Ticket& ticket) { return ticket.vip; });
    return static_cast<std::size_t>(std::distance(tickets.begin(), point));
}

int main() {
    std::vector<int> values{5, 2, 7, 4, 9, 6};
    const auto point = std::stable_partition(
        values.begin(), values.end(), [](int value) { return value % 2 == 0; });
    assert((values == std::vector<int>{2, 4, 6, 5, 7, 9}));
    assert(point == values.begin() + 3);

    std::vector<int> nums{0, 1, 0, 3, 12};
    leetcode_move_zeroes(nums);
    assert((nums == std::vector<int>{1, 3, 12, 0, 0}));

    std::vector<Ticket> tickets{{"A", false}, {"B", true}, {"C", false},
                                 {"D", true}};
    assert(practical_prioritize_vip(tickets) == 2U);
    assert(tickets[0].name == "B" && tickets[1].name == "D");
    assert(tickets[2].name == "A" && tickets[3].name == "C");

    std::cout << "stable_partition：穩定分區、LC283 與 FIFO 優先權測試通過\n";
}

/*
 * 易錯陷阱：
 * - stable 指「相同群組內相對次序」，不是整體排序，也不是 thread-safe。
 * - 演算法可能配置暫存記憶體；hard real-time 或 no-allocation 區段不能想當然使用。
 * - 移動元素後，舊 position/iterator 所代表的業務項目可能改變，應以 stable id 查找。
 * - LC283 只要求 O(1) 額外空間；標準 stable_partition 的實作可能配置，因此面試
 *   最佳解通常手寫雙指標，而本例用它展示語意與穩定性。
 *
 * 面試：如果只需把 0 移後且保留非零，remove + fill 也可 O(N) 且語意清楚；
 * stable_partition 更一般，但未必滿足題目的空間限制。要主動說出這個取捨。
 *
 * 例外安全也取決於元素 move/swap 與 predicate；若其中拋例外，容器仍有效，但排列
 * 未必維持原狀。需要交易語意時先在副本操作，成功後再 swap 發布。
 *
 * 練習：以 rotate 寫無額外配置的穩定分區，分析最壞 O(N log N)；再 benchmark
 * 大型 movable object，觀察 partition 與 stable_partition 的搬移差異。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'stable_partition.cpp' -o '/tmp/codex_cpp_C_Algorithm_partitioning_stable_partition' && '/tmp/codex_cpp_C_Algorithm_partitioning_stable_partition'
//
// === 預期輸出（節錄）===
// stable_partition：穩定分區、LC283 與 FIFO 優先權測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
