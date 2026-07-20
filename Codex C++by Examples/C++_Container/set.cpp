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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate（存在重複元素）
// 題目：輸入整數陣列，任兩索引值相同即回 true；例如 [1,2,3,1] 為 true。
// 為何使用本章主題：set::insert 的 bool 直接指出唯一值是否已存在，並提供穩定 O(log N) 上界與排序。
// 思路：建立空 seen；逐值 insert；首次遇到 inserted=false 就回 true；掃完則回 false。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 為輸入長度。
// 易錯點：空或單元素應回 false；不要忽略 insert 回傳值；若不需排序，unordered_set 平均可達 O(N)。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】部署前檢查缺少的權限
// 情境：比較服務 required 權限與帳號 granted 權限，輸出尚未授予的項目供部署閘門使用。
// 為何使用本章主題：set 天然唯一且有序，正好滿足 set_difference 的排序前置條件並產生可重現結果。
// 設計：同步走訪 required 與 granted；只輸出前者獨有項目；透過 inserter 寫入 missing set。
// 成本：時間 O(R+G)、額外空間 O(M)，R/G 為兩集合大小、M 為缺少權限數。
// 上線注意：兩邊 comparator 必須語意一致；權限名稱需先正規化，且結果只表示名稱差集、不驗證授權範圍。
// -----------------------------------------------------------------------------
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
