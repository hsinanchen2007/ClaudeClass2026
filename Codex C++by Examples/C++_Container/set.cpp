// ============================================================================
// set：有序、唯一值集合
// ============================================================================
// set<Key> 可視為只有 key、沒有 mapped value 的 map。元素依 Compare 排序且唯一；
// find/contains/insert/bounds 為 O(log n)；erase(pos) 是 amortized O(1)，erase(key) 為
// O(log n + count(key))，erase(first,last) 為 O(log n + N)。iterator 解參考得到
// const Key，不能原地修改，因為改 key 會破壞樹的排序 invariant；要用 node handle
// extract/修改/insert，或 erase 後重新 insert。

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

void basic_demo()
{
    std::set<int> ids{30, 10, 20, 20};
    assert(ids.size() == 3U);
    assert((std::vector<int>(ids.begin(), ids.end()) ==
            std::vector<int>{10, 20, 30}));
    assert(ids.contains(20));
    assert(*ids.lower_bound(15) == 20);
}

// ----------------------------------------------------------------------------
// LeetCode 217：Contains Duplicate
// ----------------------------------------------------------------------------
// insert 回傳 pair<iterator,bool>；bool=false 表示值已存在。時間 O(n log n)、空間 O(n)。
// 若不需要排序，unordered_set 平均 O(n)；set 則提供穩定最差 O(log n) 與有序輸出。
bool contains_duplicate(const std::vector<int>& values)
{
    std::set<int> seen;
    for (const int value : values) {
        const auto [position, inserted] = seen.insert(value);
        static_cast<void>(position);
        if (!inserted) {
            return true;
        }
    }
    return false;
}

void leetcode_demo()
{
    assert(contains_duplicate({1, 2, 3, 1}));
    assert(!contains_duplicate({1, 2, 3, 4}));
}

// ----------------------------------------------------------------------------
// 實務：權限集合的差集
// ----------------------------------------------------------------------------
// set_difference 要求兩個輸入已排序；set 天然滿足。時間 O(n+m)。
std::set<std::string> missing_permissions(
    const std::set<std::string>& required,
    const std::set<std::string>& granted)
{
    std::set<std::string> missing;
    std::set_difference(required.begin(), required.end(),
                        granted.begin(), granted.end(),
                        std::inserter(missing, missing.end()));
    return missing;
}

void practical_demo()
{
    const std::set<std::string> required{"read", "write"};
    const std::set<std::string> granted{"read", "share"};
    assert((missing_permissions(required, granted) ==
            std::set<std::string>{"write"}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "set：唯一有序值、去重與權限差集測試通過\n";
}

// 【陷阱】自訂比較若只比較部分欄位，該部分等價的兩物件會被視為同一 key。
// 【面試】set 的「相等」由 !comp(a,b)&&!comp(b,a) 定義，不一定呼叫 operator==。
// 【練習】用 set_intersection 算兩使用者共同權限。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'set.cpp' -o '/tmp/codex_cpp_C_Container_set' && '/tmp/codex_cpp_C_Container_set'
//
// === 預期輸出（節錄）===
// set：唯一有序值、去重與權限差集測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
