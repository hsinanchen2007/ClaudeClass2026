/*
 * std::find / find_if / find_if_not：線性尋找第一個符合元素
 * =======================================================
 * find 用 ==，find_if 用 predicate，find_if_not 找第一個不符合者。最壞 O(N)，找到
 * 即短路。回傳 iterator；找不到回 last，解參考前必須檢查。它不是 binary search，
 * 不要求排序，也不利用 hash/index。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 217：Contains Duplicate。教學版在每個前綴 find，O(N^2)。
// 正式大資料用 unordered_set 平均 O(N)。
bool leetcode_contains_duplicate(const std::vector<int>& nums) {
    for (auto it = nums.begin(); it != nums.end(); ++it) {
        if (std::find(nums.begin(), it, *it) != it) {
            return true;
        }
    }
    return false;
}

struct Command {
    std::string name;
    bool enabled;
};

// 實務：依名稱找第一個 enabled command，回 index sentinel=size。
std::size_t practical_find_enabled_command(
    const std::vector<Command>& commands, const std::string& name) {
    const auto it = std::find_if(
        commands.begin(), commands.end(),
        [&name](const Command& command) {
            return command.enabled && command.name == name;
        });
    return static_cast<std::size_t>(std::distance(commands.begin(), it));
}

int main() {
    const std::vector<int> values{4, 8, 15};
    assert(std::find(values.begin(), values.end(), 8) == values.begin() + 1);
    assert(std::find(values.begin(), values.end(), 9) == values.end());
    assert(std::find_if_not(values.begin(), values.end(),
                            [](int value) { return value % 2 == 0; }) ==
           values.begin() + 2);

    assert(leetcode_contains_duplicate({1, 2, 3, 1}));
    assert(!leetcode_contains_duplicate({1, 2, 3, 4}));

    const std::vector<Command> commands{{"build", false}, {"test", true}};
    assert(practical_find_enabled_command(commands, "test") == 1U);
    assert(practical_find_enabled_command(commands, "build") == commands.size());
    std::cout << "find：LeetCode 217 與實務 command lookup 測試通過\n";
}

/*
 * 易錯陷阱：find 回 end 仍拿 `*it`；或將 O(N) find 放進 loop 形成 O(N^2)。如果資料
 * 已排序選 lower_bound，若頻繁 key lookup 選 map/unordered_map。
 *
 * 面試：predicate 捕捉 name by reference 在同步呼叫期間安全；若把 predicate 保存
 * 到異步工作，name lifetime 需重新設計。實務 index sentinel 可改 optional<size_t>
 * 更明確。練習：回 const Command*，說明 vector reallocation 後 pointer 失效。
 * LeetCode 測試再加空 vector 與重複在尾端；實務 command name 是否大小寫敏感要列契約。
 * 若 command 數量大且頻繁查詢，建 unordered_map，而不是每次線性 find_if。
 * 練習：比較線性表與 hash index 在更新頻繁時的維護成本。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'find.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_find' && '/tmp/codex_cpp_C_Algorithm_non_modifying_find'
//
// === 預期輸出（節錄）===
// find：LeetCode 217 與實務 command lookup 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
