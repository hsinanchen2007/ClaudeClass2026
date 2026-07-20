/*
 * std::stable_sort：完整排序並保留等價元素原始相對順序
 * =====================================================
 * 時間 O(N log N)（有足夠暫存記憶體）；配置失敗時標準允許更多比較。需要
 * random-access iterator，可能配置 auxiliary buffer。穩定性只針對 comparator
 * 判定等價的元素，不代表 thread-safe 或永遠保留所有欄位順序。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 1122：Relative Sort Array 的簡化 rank comparator，stable 保留未知值順序。
std::vector<int> leetcode_relative_sort(std::vector<int> values,
                                        const std::vector<int>& order) {
    const auto rank_of = [&order](int value) {
        const auto it = std::find(order.begin(), order.end(), value);
        return (it == order.end()) ? order.size()
                                   : static_cast<std::size_t>(std::distance(order.begin(), it));
    };
    std::stable_sort(values.begin(), values.end(), [&rank_of, &order](int lhs, int rhs) {
        const std::size_t left_rank = rank_of(lhs);
        const std::size_t right_rank = rank_of(rhs);
        if (left_rank != right_rank) {
            return left_rank < right_rank;
        }
        return left_rank == order.size() && lhs < rhs;
    });
    return values;
}

struct Message {
    int priority;
    int arrival_sequence;
    std::string text;
};

// 實務：只按 priority 排；同 priority 自動維持 FIFO arrival sequence。
void practical_priority_fifo(std::vector<Message>& messages) {
    std::stable_sort(messages.begin(), messages.end(),
                     [](const Message& lhs, const Message& rhs) {
                         return lhs.priority > rhs.priority;
                     });
}

int main() {
    assert((leetcode_relative_sort({2, 3, 1, 3, 2, 4, 6, 7, 9, 2, 19},
                                   {2, 1, 4, 3, 9, 6}) ==
            std::vector<int>{2, 2, 2, 1, 4, 3, 3, 9, 6, 7, 19}));

    std::vector<Message> messages{{1, 0, "A"}, {3, 1, "B"},
                                   {1, 2, "C"}, {3, 3, "D"}};
    practical_priority_fifo(messages);
    assert(messages[0].text == "B" && messages[1].text == "D");
    assert(messages[2].text == "A" && messages[3].text == "C");

    std::cout << "stable_sort：LC1122 與 priority FIFO 測試通過\n";
}

/*
 * 易錯陷阱：
 * - stable 只在 comparator 等價時保序。若 comparator 加入 arrival_sequence tie-break，
 *   元素不再等價，但結果仍可 deterministic；需理解兩種設計。
 * - rank_of 線性搜尋讓 comparator 昂貴；order 大時預先建 unordered_map rank。
 * - stable_sort 可能配置記憶體，不適合未分析的 hard real-time hot path。
 * - 若只要分兩群保序，stable_partition 比完整 stable_sort 更符合意圖。
 *
 * 面試：多鍵穩定排序可從次要鍵到主要鍵連續 stable_sort；但一次 tuple comparator
 * 通常更快也更直觀。舊系統逐層排序時，穩定性才是關鍵契約。
 *
 * 生命週期與 sort 相同：容器 size 不變但值被搬動。練習：讓 Message 不可複製但
 * 可 noexcept move，驗證 stable_sort；再比較排序 index 的成本。
 */
