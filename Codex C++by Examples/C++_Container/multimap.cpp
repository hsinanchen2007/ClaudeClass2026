// ============================================================================
// multimap：依 key 排序、允許重複鍵的關聯容器
// ============================================================================
// multimap 與 map 都是 ordered tree，但同一 key 可有多筆 value。它沒有 operator[]；
// 查一組重複鍵用 equal_range(key)，回傳 [first,last) 半開區間。insert O(log n)，
// count 通常 O(log n + matches)，走訪匹配 O(matches)。同 key 元素彼此的相對順序
// 不應拿來當業務契約；需要明確排序時把次要欄位放進 composite key。

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

void basic_demo()
{
    std::multimap<std::string, int> scores;
    scores.emplace("Ada", 90);
    scores.emplace("Ada", 95);
    scores.emplace("Linus", 88);

    const auto [first, last] = scores.equal_range("Ada");
    std::vector<int> ada_scores;
    for (auto it = first; it != last; ++it) {
        ada_scores.push_back(it->second);
    }
    std::ranges::sort(ada_scores);
    assert((ada_scores == std::vector<int>{90, 95}));
}

// ----------------------------------------------------------------------------
// LeetCode 49 風格：Group Anagrams
// ----------------------------------------------------------------------------
// 排序後字串作 key，相同字母組合落在同一 equal_range。建立時每字 O(k log k)，
// 最後依 ordered key 輸出可重現。真正解題也常用 unordered_map<string,vector<string>>。
std::vector<std::vector<std::string>> group_anagrams(
    const std::vector<std::string>& words)
{
    std::multimap<std::string, std::string> by_signature;
    for (const std::string& word : words) {
        std::string signature = word;
        std::ranges::sort(signature);
        by_signature.emplace(std::move(signature), word);
    }

    std::vector<std::vector<std::string>> groups;
    for (auto it = by_signature.begin(); it != by_signature.end();) {
        const auto [first, last] = by_signature.equal_range(it->first);
        std::vector<std::string> group;
        for (auto member = first; member != last; ++member) {
            group.push_back(member->second);
        }
        groups.push_back(std::move(group));
        it = last;
    }
    return groups;
}

void leetcode_demo()
{
    const auto groups = group_anagrams({"eat", "tea", "tan", "ate", "nat", "bat"});
    assert(groups.size() == 3U);
    std::size_t total = 0;
    for (const auto& group : groups) {
        total += group.size();
    }
    assert(total == 6U);
}

// ----------------------------------------------------------------------------
// 實務：一個時間點可有多個排程事件
// ----------------------------------------------------------------------------
std::vector<std::string> events_at(
    const std::multimap<int, std::string>& timeline, int timestamp)
{
    const auto [first, last] = timeline.equal_range(timestamp);
    std::vector<std::string> result;
    for (auto it = first; it != last; ++it) {
        result.push_back(it->second);
    }
    return result;
}

void practical_demo()
{
    const std::multimap<int, std::string> timeline{
        {10, "backup"}, {20, "rotate-log"}, {20, "health-check"}};
    auto events = events_at(timeline, 20);
    std::ranges::sort(events);
    assert((events == std::vector<std::string>{"health-check", "rotate-log"}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "multimap：重複鍵、equal_range 與排程索引測試通過\n";
}

// 【陷阱】erase(key) 會刪除該 key 的所有元素；只刪一筆要 erase(iterator)。
// 【面試】multimap 與 map<Key, vector<Value>> 差異：局部更新、配置、整組存取介面。
// 【練習】將事件 key 改為 pair<timestamp,priority>，讓同時間事件有明確順序。
