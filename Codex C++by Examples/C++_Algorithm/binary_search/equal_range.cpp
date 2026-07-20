/*
 * std::equal_range：一次取得所有等價元素的半開區間
 * ==================================================
 * 回傳 pair{lower_bound, upper_bound}。空範圍 first==second，且兩者仍指出合法
 * 插入位置。它不依賴 operator==，等價性仍由排序比較器定義。
 *
 * 比較次數 O(log N)。計算元素個數時，vector 的 iterator 相減 O(1)；一般
 * forward iterator 的 std::distance 可能 O(K)。回傳 iterator 不延長容器生命。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 34. Find First and Last Position of Element in Sorted Array（在排序陣列中找元素的第一與最後位置）
// 題目：輸入升冪陣列 nums 與 target，回傳 target 的首尾索引；不存在時回
// [-1,-1]，例如 [5,7,7,8,8,10] 查 8 得 [3,4]。
// 為何使用本章主題：std::equal_range 一次表達 target 的完整等價半開區間，等同
// lower_bound 與 upper_bound 的組合，正好對應左右邊界。
// 思路：1. 取得 [first,last)；2. 空區間表示不存在；3. first 為左界，last-1 為右界。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：輸入必須升冪；last 是尾後 iterator；空區間不可做 last-1 或解參考。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_search_range_equal(const std::vector<int>& nums,
                                              int target) {
    const auto [first, last] = std::equal_range(nums.begin(), nums.end(), target);
    if (first == last) {
        return {-1, -1};
    }
    return {static_cast<int>(std::distance(nums.begin(), first)),
            static_cast<int>(std::distance(nums.begin(), last) - 1)};
}

struct Event {
    int user_id;
    std::string action;
};

struct EventUserLess {
    bool operator()(const Event& event, int key) const {
        return event.user_id < key;
    }
    bool operator()(int key, const Event& event) const {
        return key < event.user_id;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】依使用者擷取事件動作清單
// 情境：事件快照已按 user_id 升冪，同一使用者可有多筆 login/view 等動作；查詢要
// 複製指定 user_id 的全部 action，不存在時回空清單。
// 為何使用本章主題：equal_range 可直接定位連續同鍵區段；相較掃描整份事件表，只需
// 對數搜尋加上實際輸出筆數的線性複製。
// 設計：1. EventUserLess 支援 Event 與 int 異質比較；2. 找出 [first,last)；3. 依
// distance 預留容量並按原事件順序複製 action。
// 成本：時間 O(log N + K)、額外空間 O(K)，N 為事件數、K 為該使用者事件數。
// 上線注意：事件必須維持 user_id 排序；回傳值只含 action，若需時間順序要確保同鍵區段已按時間建立。
// -----------------------------------------------------------------------------
std::vector<std::string> practical_actions_for(
    const std::vector<Event>& events, int user_id) {
    const auto [first, last] = std::equal_range(
        events.begin(), events.end(), user_id, EventUserLess{});
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(std::distance(first, last)));
    for (auto it = first; it != last; ++it) {
        result.push_back(it->action);
    }
    return result;
}

int main() {
    const std::vector<int> values{1, 2, 2, 2, 5};
    const auto [first, last] = std::equal_range(values.begin(), values.end(), 2);
    assert(first == values.begin() + 1);
    assert(last == values.begin() + 4);

    assert((leetcode_search_range_equal({5, 7, 7, 8, 8, 10}, 8) ==
            std::vector<int>{3, 4}));
    assert((leetcode_search_range_equal({1, 2, 3}, 9) ==
            std::vector<int>{-1, -1}));

    const std::vector<Event> events{{7, "login"}, {7, "view"}, {9, "logout"}};
    assert((practical_actions_for(events, 7) ==
            std::vector<std::string>{"login", "view"}));
    assert(practical_actions_for(events, 8).empty());

    std::cout << "equal_range：重複值與事件群組查詢測試通過\n";
}

/*
 * 易錯點：四參數 heterogeneous overload 對比較器要求較細；比較器必須能表達
 * element/key 的排序關係。若編譯器或型別設計不合，分別呼叫 lower_bound 與
 * upper_bound 並提供正確方向的 lambda 會更清楚。
 * 面試題：count 為何不一定是 O(1)？因為 distance 的成本取決於 iterator 類別。
 * LeetCode 解題時，equal_range 可把 LC34 的兩次邊界搜尋包成一個語意單位。
 * 實務查詢時，先確認索引真的是依 user_id 排序，不能只看測試資料剛好有序。
 * 練習：讓 Event 先按 user_id、再按 timestamp 排序，查某人指定時間區間。
 */

/*
 * 【教科書補充：equal_range 是兩個分界的組合契約】
 * - 它等價於 lower_bound/upper_bound 的半開區間，因此異質 comparator 要支援兩種參數方向。
 * - 範圍須同時滿足兩側 partition 且比較不可讓 comp(a,b) 與 comp(b,a) 同時為 true；否則 UB。
 * - 對 ForwardIterator，即使比較約 O(log N)，逐步前進仍可能 O(N)；不要只看 distance 的成本。
 * - 回傳空區間仍有兩種語意：key 不存在但可插入於該位置，或查詢在 begin/end 邊界。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'equal_range.cpp' -o '/tmp/codex_cpp_C_Algorithm_binary_search_equal_range' && '/tmp/codex_cpp_C_Algorithm_binary_search_equal_range'
//
// === 預期輸出（節錄）===
// equal_range：重複值與事件群組查詢測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
