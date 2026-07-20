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

// LeetCode 27：Remove Element，題目只要求回傳新長度。
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

// 實務：刪除過期 session。predicate 不可在同時修改被遍歷容器。
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
