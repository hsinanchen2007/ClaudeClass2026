/*
 * std::inplace_merge：把同一範圍內相鄰兩段已排序序列合成一段
 * =============================================================
 * 前置條件：[first,middle) 與 [middle,last) 各自按同一 comparator 排序。
 * 合併是 stable：等價元素中，左半元素仍位於右半元素之前。
 *
 * 有足夠暫存記憶體時 O(N) 比較/搬移；無 buffer 時比較可到 O(N log N)。需要
 * bidirectional iterator。它不改容器 size，但會重新排列元素。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 88：Merge Sorted Array；這版先把第二段放入同一 vector，再 inplace_merge。
std::vector<int> leetcode_merge_sorted(std::vector<int> first,
                                       const std::vector<int>& second) {
    const std::size_t middle_index = first.size();
    first.insert(first.end(), second.begin(), second.end());
    std::inplace_merge(first.begin(), first.begin() +
                                      static_cast<std::ptrdiff_t>(middle_index),
                       first.end());
    return first;
}

struct Event {
    int timestamp;
    std::string source;
};

// 實務：buffer 前半是昨日已排序事件，後半是今日已排序事件；穩定合併同時戳。
void practical_merge_event_runs(std::vector<Event>& events, std::size_t middle) {
    assert(middle <= events.size());
    const auto less_time = [](const Event& lhs, const Event& rhs) {
        return lhs.timestamp < rhs.timestamp;
    };
    assert(std::is_sorted(events.begin(), events.begin() +
                                         static_cast<std::ptrdiff_t>(middle), less_time));
    assert(std::is_sorted(events.begin() + static_cast<std::ptrdiff_t>(middle),
                          events.end(), less_time));
    std::inplace_merge(events.begin(),
                       events.begin() + static_cast<std::ptrdiff_t>(middle),
                       events.end(), less_time);
}

int main() {
    assert((leetcode_merge_sorted({1, 2, 4}, {1, 3, 4}) ==
            std::vector<int>{1, 1, 2, 3, 4, 4}));

    std::vector<Event> events{{1, "old-A"}, {3, "old-B"},
                              {1, "new-A"}, {2, "new-B"}};
    practical_merge_event_runs(events, 2U);
    assert(events[0].source == "old-A" && events[1].source == "new-A");
    assert(events[2].timestamp == 2 && events[3].timestamp == 3);

    std::cout << "inplace_merge：LC88 與穩定事件 run 合併測試通過\n";
}

/*
 * 易錯陷阱：
 * - middle 是 iterator，不是數量；它必須位於 [first,last] 且兩側各自 sorted。
 * - 「兩段加起來大致有序」不夠；任何一側內部逆序都破壞契約。
 * - iterator 未因 size 改變而失效，但元素位置/identity 已變，外部 position cache 過期。
 * - 演算法可能配置暫存 buffer，hard real-time 需專門策略。
 *
 * 面試：merge sort 的 combine 步驟可用 inplace_merge；整體穩定性來自每次 stable
 * merge。LC88 正統 O(1) 解從尾端寫入，本例重點是 API 契約而非題目最省空間解。
 *
 * 練習：手寫從尾端 merge 的 LC88，再比較插入第二段造成的 reallocation 與
 * inplace_merge buffer 成本；大型物件可考慮 indirect index merge。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'inplace_merge.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_inplace_merge' && '/tmp/codex_cpp_C_Algorithm_set_operations_inplace_merge'
//
// === 預期輸出（節錄）===
// inplace_merge：LC88 與穩定事件 run 合併測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
