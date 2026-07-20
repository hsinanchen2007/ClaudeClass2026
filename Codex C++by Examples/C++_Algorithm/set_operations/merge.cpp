/*
 * std::merge：把兩個已排序輸入穩定合併到獨立輸出
 * =================================================
 * 兩輸入必須用同一 comparator 排序；輸出大小最多 N+M，回傳 output end。
 * 時間最多 N+M-1 次比較，額外空間由輸出容器承擔。等價元素時先輸出第一範圍，
 * 且各來源內相對順序保留。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 21. Merge Two Sorted Lists（合併兩個有序鏈結串列）
// 題目：原題合併兩條升冪 linked list；例如 [1,2,4] 與 [1,3,4] 得
// [1,1,2,3,4,4]。本 helper 使用 vector，是 iterator API 對應版。
// 為何使用本章主題：std::merge 以雙 iterator 穩定合併兩個獨立 sorted ranges，完整
// 保留兩邊 duplicate，正好對應 linked-list merge 的值語意。
// 思路：1. 驗證兩輸入升冪；2. 預留 N+M；3. merge 到 back_inserter。
// 複雜度：時間 O(N+M)、額外空間 O(N+M)，N/M 為兩序列長度。
// 易錯點：這不是原題 ListNode 原地 relink 解；reserve 不建立元素，因此輸出使用 back_inserter。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_merge_two_sorted(const std::vector<int>& lhs,
                                           const std::vector<int>& rhs) {
    assert(std::is_sorted(lhs.begin(), lhs.end()));
    assert(std::is_sorted(rhs.begin(), rhs.end()));
    std::vector<int> output;
    output.reserve(lhs.size() + rhs.size());
    std::merge(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
               std::back_inserter(output));
    return output;
}

struct Record {
    int timestamp;
    std::string source;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】Immutable Shard 事件時間線合併
// 情境：first/second 是兩個不可修改的 shard snapshot，各自依 timestamp 排序；下游
// 要一條完整合併時間線供查詢，來源仍保留給 audit。
// 為何使用本章主題：std::merge 線性讀兩個 sorted ranges 並穩定輸出；相同 timestamp
// 時第一 shard 先出，無需重排來源副本。
// 設計：1. 預留兩邊總數；2. comparator 只比 timestamp；3. merge 到新 output。
// 成本：時間 O(N+M)、額外空間 O(N+M)，N/M 為兩 shard 筆數。
// 上線注意：若 tie 順序不能依來源，需加入全域 sequence；載入邊界要驗證兩邊確實按同 comparator 排序。
// -----------------------------------------------------------------------------
std::vector<Record> practical_merge_shards(const std::vector<Record>& first,
                                           const std::vector<Record>& second) {
    std::vector<Record> output;
    output.reserve(first.size() + second.size());
    std::merge(first.begin(), first.end(), second.begin(), second.end(),
               std::back_inserter(output),
               [](const Record& lhs, const Record& rhs) {
                   return lhs.timestamp < rhs.timestamp;
               });
    return output;
}

int main() {
    assert((leetcode_merge_two_sorted({1, 2, 4}, {1, 3, 4}) ==
            std::vector<int>{1, 1, 2, 3, 4, 4}));

    const std::vector<Record> left{{1, "L1"}, {3, "L3"}};
    const std::vector<Record> right{{1, "R1"}, {2, "R2"}};
    const auto merged = practical_merge_shards(left, right);
    assert(merged[0].source == "L1" && merged[1].source == "R1");
    assert(merged[2].source == "R2" && merged[3].source == "L3");
    assert(left.size() == 2U && right.size() == 2U);

    std::cout << "merge：LC21 與穩定 shard 合併測試通過\n";
}

/*
 * 易錯陷阱：
 * - output 不可與 input 不安全重疊；要同一容器相鄰 run 請用 inplace_merge。
 * - reserve 後不能直接傳 output.begin()，因 size 仍為 0；用 back_inserter 或 resize。
 * - comparator 不得使用 <=，且兩輸入必須按完全相同規則排序。
 * - merge 保留 duplicate；若要 set union 的 max multiplicity，使用 set_union。
 *
 * 面試：merge 的穩定 tie 規則對 event ordering 很重要；相同 timestamp 時第一來源
 * 先出。若業務需要全域 sequence，應把 sequence 納入 comparator，而非依來源順序。
 *
 * 生命週期：output 若 reallocate，先前指向 output 的 iterator 失效；back_inserter
 * 自己會追蹤容器 append。練習：用 priority_queue 合併 K 個 sorted shards，分析
 * O(total log K)，並保留 deterministic tie-breaker。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'merge.cpp' -o '/tmp/codex_cpp_C_Algorithm_set_operations_merge' && '/tmp/codex_cpp_C_Algorithm_set_operations_merge'
//
// === 預期輸出（節錄）===
// merge：LC21 與穩定 shard 合併測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
