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

// LeetCode 34 的另一種寫法：equal_range 同時取得左右邊界。
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

// 實務：事件已按 user_id 排序，擷取單一使用者的所有 action。
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
