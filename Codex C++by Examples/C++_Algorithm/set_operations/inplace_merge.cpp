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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 88. Merge Sorted Array（合併兩個有序陣列）
// 題目：原題要求把 nums2 合併進 nums1；例如 [1,2,4]+[1,3,4] 得 [1,1,2,3,4,4]。
// 本版先 append second 再 inplace_merge，未使用原題從尾端 O(1) 額外空間技巧。
// 為何使用本章主題：append 後同一 vector 形成兩段相鄰 sorted runs，正是
// inplace_merge 的前置形狀，且等價值保持左段先出。
// 思路：1. 保存原 first.size 作 middle；2. append second；3. 以 middle 分隔兩段並穩定合併。
// 複雜度：常見時間 O(N+M)、額外空間 O(N+M)，N/M 為兩輸入大小；無 buffer merge 可更昂貴。
// 易錯點：兩段必須各自升冪；insert 後舊 iterator 可能失效，所以保存 index 而非 iterator。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】昨日與今日事件 Run 原地合併
// 情境：同一 events buffer 的 [0,middle) 是昨日已排序事件，後半是今日已排序事件；
// 要依 timestamp 合成一條時間線，相同時間讓昨日事件先出。
// 為何使用本章主題：inplace_merge 專為相鄰 sorted runs 設計，穩定 tie 規則符合舊資料
// 先於新資料，且不改容器大小。
// 設計：1. 驗 middle 合法；2. 以同一 less_time 驗證兩半；3. 對完整 buffer 穩定合併。
// 成本：有 buffer 時間 O(N)、額外空間可 O(N)，N 為事件總數；無 buffer 時可 O(N log N)。
// 上線注意：timestamp 相同但需全域次序時應加入 sequence；演算法可能配置，不適合未評估的即時路徑。
// -----------------------------------------------------------------------------
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
