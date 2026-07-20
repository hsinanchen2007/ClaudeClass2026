/*
 * std::remove / remove_if：移除演算法其實只做「穩定壓縮」
 * ========================================================
 * 回傳 new_end；[begin,new_end) 是保留元素，[new_end,end) 內容 unspecified 但仍是
 * 容器的一部分。演算法不知道容器，不能改 size。vector/string 的完整刪除需
 * erase(new_end,end)，C++20 可用 std::erase / std::erase_if。
 *
 * 時間 O(N)，保留元素相對順序不變；元素需 MoveAssignable。list 有自己的
 * member remove，能真正移除 node 且不需搬移其他元素。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element（移除元素）
// 題目：原地移除所有等於 val 的元素並回傳剩餘數量 k，前 k 格放保留值；例如
// [3,2,2,3] 移除 3 後 k=2。
// 為何使用本章主題：std::remove 穩定壓縮保留值並回 logical end；本教材再 erase
// 尾段讓 vector 實際縮小，這比原題只要求前 k 格多做一步。
// 思路：1. remove 所有 value；2. 計算 begin 到 new_end 的長度；3. erase 未指定尾段；
// 4. 回傳 k。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：remove 不會改 size；原題不要求保留 k 後內容，本函式則明確 erase 尾端。
// -----------------------------------------------------------------------------
int leetcode_remove_element(std::vector<int>& nums, int value) {
    const auto new_end = std::remove(nums.begin(), nums.end(), value);
    const int length = static_cast<int>(std::distance(nums.begin(), new_end));
    nums.erase(new_end, nums.end());
    return length;
}

struct Session {
    std::string id;
    bool expired;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】過期 Session 批次清理
// 情境：記憶體中的 Session vector 含 expired 標記，要一次刪除所有過期項目並維持
// 未過期 session 的原順序。
// 為何使用本章主題：erase-remove_if 只線性搬移保留元素一次，避免在迴圈逐筆 erase
// 造成反覆位移的 O(N^2) 成本。
// 設計：1. remove_if 將 expired 項目排除並回 new_end；2. erase 尾段真正銷毀 session。
// 成本：時間 O(N)、額外空間 O(1)，N 為 session 數；保留元素可能被 move-assign。
// 上線注意：predicate 不可修改 vector 結構；若 Session 析構會做 I/O，批次延遲與失敗策略要另管控。
// -----------------------------------------------------------------------------
void practical_purge_expired(std::vector<Session>& sessions) {
    const auto new_end = std::remove_if(
        sessions.begin(), sessions.end(),
        [](const Session& session) { return session.expired; });
    sessions.erase(new_end, sessions.end());
}

int main() {
    std::vector<int> raw{1, 2, 2, 3};
    const auto logical_end = std::remove(raw.begin(), raw.end(), 2);
    assert(std::distance(raw.begin(), logical_end) == 2);
    assert(raw.size() == 4U);  // 尚未 erase，物理 size 不變。
    raw.erase(logical_end, raw.end());
    assert((raw == std::vector<int>{1, 3}));

    std::vector<int> nums{3, 2, 2, 3};
    assert(leetcode_remove_element(nums, 3) == 2);
    assert((nums == std::vector<int>{2, 2}));

    std::vector<Session> sessions{{"A", false}, {"B", true}, {"C", false}};
    practical_purge_expired(sessions);
    assert(sessions.size() == 2U && sessions[0].id == "A" && sessions[1].id == "C");
    std::cout << "remove：erase-remove、LC27、session 清理測試通過\n";
}

/*
 * 經典陷阱：只呼叫 remove 後印整個 vector，會看到尾端殘值；把 new_end 當成已
 * 刪除 iterator 也錯。面試：為何 remove 保序但 partition 不保序？remove 逐一
 * 向前覆寫保留元素。練習：改用 C++20 std::erase_if，對照回傳的刪除數量。
 *
 * 【LeetCode 契約】
 * LC27 原題只檢查前 k 格，不要求縮小 vector；本教材額外 erase 是為了讓一般
 * 容器使用者看到完整 idiom。面試時要先說清楚你採哪一種輸出契約。
 *
 * 【實務效能】
 * erase-remove 對 vector 會 move 保留元素並銷毀尾端，沒有逐筆 erase 造成的
 * O(N^2)。若不要求保序，可用 swap-with-back 逐筆移除，搬移更少但順序改變。
 * 易錯陷阱：predicate 不可修改判斷所依賴的 key，否則後續結果難以推理。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'remove.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_remove' && '/tmp/codex_cpp_C_Algorithm_modifying_remove'
//
// === 預期輸出（節錄）===
// remove：erase-remove、LC27、session 清理測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
