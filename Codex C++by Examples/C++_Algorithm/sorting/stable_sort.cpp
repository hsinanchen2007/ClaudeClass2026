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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1122. Relative Sort Array（陣列的相對排序）
// 題目：arr1 中出現在 arr2 的值依 arr2 順序排列，其餘值升冪放尾端；範例輸出為
// [2,2,2,1,4,3,3,9,6,7,19]。
// 為何使用本章主題：stable_sort 依 rank comparator 排列；未知值另以數值升冪比較。
// 本版 rank_of 每次線性找 order，行為正確但大 order 應預建 hash rank。
// 思路：1. rank_of 回 arr2 index 或尾端 rank；2. rank 不同者按 rank；3. 兩者未知時
// 按值升冪；4. stable_sort 全部 values。
// 複雜度：時間 O(N log N*M)、額外空間 O(N)，N/M 為 values/order 大小。
// 易錯點：未知值必須升冪，不是保留原順序；comparator 相等時 stable 才保留輸入次序。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】訊息優先權排序且同級 FIFO
// 情境：messages 已按 arrival_sequence 到達，需讓高 priority 先處理，同 priority 保持
// 原始到達順序。
// 為何使用本章主題：stable_sort comparator 只比 priority，使同級訊息彼此等價並自動
// 保留輸入 FIFO，不必把 sequence 再寫入 comparator。
// 設計：1. comparator 以 priority 降冪；2. 穩定排序完整訊息列；3. 同級沿用原順序。
// 成本：時間 O(N log N)、額外空間可 O(N)，N 為訊息數；配置失敗 fallback 比較成本較高。
// 上線注意：輸入必須本來就是 arrival 順序；stable_sort 可能配置，且併發 queue 需同步。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：stable_sort 的保證與成本】
 * - 有足夠額外記憶體時比較次數 O(N log N)；配置不到 buffer 時仍可完成，但可達 O(N log^2 N)。
 * - 標準沒有承諾固定 O(N) 額外空間；實作可嘗試配置再退回原地演算法。
 * - comparator 必須是 strict weak ordering；含 NaN 的一般 `<` 需先定義 domain ordering。
 * - execution-policy 版本要求 callable 無資料競爭；標準 policy 下未處理例外可能終止程序。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'stable_sort.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_stable_sort' && '/tmp/codex_cpp_C_Algorithm_sorting_stable_sort'
//
// === 預期輸出（節錄）===
// stable_sort：LC1122 與 priority FIFO 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
