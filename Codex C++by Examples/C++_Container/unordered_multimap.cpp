// ============================================================================
// unordered_multimap：hash-based 重複鍵索引
// ============================================================================
// unordered_multimap 允許同 key 多 value。equal_range(key) 取得同 key 的連續局部
// 區間；查找平均 O(1 + matches)、最差 O(n)。它沒有 operator[]/at，因單一 key
// 不對應唯一 value。erase(key) 刪除全部匹配；只刪一筆要 erase(iterator)。

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::vector<std::string> values_for(
    const std::unordered_multimap<std::string, std::string>& index,
    const std::string& key)
{
    const auto [first, last] = index.equal_range(key);
    std::vector<std::string> values;
    for (auto it = first; it != last; ++it) {
        values.push_back(it->second);
    }
    std::ranges::sort(values);  // hash 容器順序不保證，測試前明確排序。
    return values;
}

void basic_demo()
{
    std::unordered_multimap<std::string, std::string> tags;
    tags.emplace("cpp", "vector");
    tags.emplace("cpp", "map");
    tags.emplace("python", "dict");
    assert(tags.count("cpp") == 2U);
    assert((values_for(tags, "cpp") == std::vector<std::string>{"map", "vector"}));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 49. Group Anagrams（字母異位詞分組）
// 題目：把字母組成相同、排列不同的字串分組；例如 eat、tea、ate 應在同一組。
// 為何使用本章主題：這是 unordered_multimap 教學改寫，用相同 signature 的重複 key 收納成員；常見解用 map 到 vector。
// 思路：排序每個 word 得 signature；插入重複鍵索引；以 processed 防重複處理；equal_range 收集一組。
// 複雜度：平均時間 O(N*K log K + N)、額外空間 O(N*K)，N 為字數、K 為最長字串長度。
// 易錯點：hash 遍歷使群組順序未指定；processed 與主索引皆可能 rehash；測試不可依賴碰巧的 bucket 次序。
// -----------------------------------------------------------------------------
std::vector<std::vector<std::string>> group_anagrams(
    const std::vector<std::string>& words)
{
    std::unordered_multimap<std::string, std::string> by_signature;
    for (const std::string& word : words) {
        std::string signature = word;
        std::ranges::sort(signature);
        by_signature.emplace(std::move(signature), word);
    }

    std::vector<std::vector<std::string>> groups;
    // processed 避免對同 signature 重複輸出；unordered_set 章會再詳談。
    std::unordered_map<std::string, bool> processed;
    for (const auto& [signature, word] : by_signature) {
        static_cast<void>(word);
        if (!processed.emplace(signature, true).second) {
            continue;
        }
        const auto [first, last] = by_signature.equal_range(signature);
        std::vector<std::string> group;
        for (auto it = first; it != last; ++it) {
            group.push_back(it->second);
        }
        groups.push_back(std::move(group));
    }
    return groups;
}

void leetcode_demo()
{
    const auto groups = group_anagrams({"eat", "tea", "ate", "bat"});
    assert(groups.size() == 2U);
    std::size_t total = 0;
    for (const auto& group : groups) {
        total += group.size();
    }
    assert(total == 4U);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】部門到員工的次要索引
// 情境：每位員工隸屬一個部門，查詢 compiler 部門時需取回 Ada、Grace 等全部成員。
// 為何使用本章主題：unordered_multimap 允許 department 重複且平均快速定位整組，適合獨立節點式次要索引。
// 設計：逐員工 emplace department->name；查詢時 equal_range；複製名稱；排序後提供 deterministic 結果。
// 成本：插入平均 O(1)；查詢平均 O(1+M) 加結果排序 O(M log M)，空間 O(N)。
// 上線注意：刪除單一員工不可用 erase(key)；rehash 後區間 iterator 失效，主資料與索引更新也需具交易一致性。
// -----------------------------------------------------------------------------
void practical_demo()
{
    std::unordered_multimap<std::string, std::string> employees;
    employees.emplace("compiler", "Ada");
    employees.emplace("compiler", "Grace");
    employees.emplace("kernel", "Linus");
    assert((values_for(employees, "compiler") ==
            std::vector<std::string>{"Ada", "Grace"}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "unordered_multimap：重複 hash key 與 secondary index 測試通過\n";
}

// 【陷阱】equal_range 回傳區間只在下一次可能 rehash 的修改前可靠。
// 【面試】何時用 unordered_multimap，何時用 unordered_map<Key, vector<Value>>？
//         前者單筆節點插刪方便；後者整組取得與連續儲存通常更直觀。
// 【練習】精確刪除 compiler->Ada，不可刪掉 compiler 的其他員工。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'unordered_multimap.cpp' -o '/tmp/codex_cpp_C_Container_unordered_multimap' && '/tmp/codex_cpp_C_Container_unordered_multimap'
//
// === 預期輸出（節錄）===
// unordered_multimap：重複 hash key 與 secondary index 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
