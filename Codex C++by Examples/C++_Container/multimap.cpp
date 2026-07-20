// ============================================================================
// multimap：依 key 排序、允許重複鍵的關聯容器
// ============================================================================
// multimap 與 map 都是 ordered tree，但同一 key 可有多筆 value。它沒有 operator[]；
// 查一組重複鍵用 equal_range(key)，回傳 [first,last) 半開區間。insert O(log n)，
// count 通常 O(log n + matches)，走訪匹配 O(matches)。同 key 元素彼此的相對順序
// 自 C++11 起，等價 key 的相對順序按插入順序保留；若業務還有 priority 等明確規則，仍應放進 composite key。

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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 49. Group Anagrams（字母異位詞分組）
// 題目：將由相同字母重排而成的字串放在同組；例如 eat、tea、ate 同組，tan、nat 同組。
// 為何使用本章主題：這是 multimap 教學改寫，以排序後字串作重複 key；常見最佳實作會用 unordered_map<string,vector<string>>。
// 思路：排序每個 word 得 signature；插入 signature->word；依 equal_range 收集相同 signature；跳到下一組。
// 複雜度：時間 O(N*K log K + N log N)、額外空間 O(N*K)，N 為字數、K 為最長字串長度。
// 易錯點：不可假設不同 key 的插入順序；空字串 signature 合法；群組與群內順序若有契約需另行排序。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】同一時間點的排程事件索引
// 情境：排程表允許同一 timestamp 同時有 log rotation 與 health check，查詢時需取回全部事件。
// 為何使用本章主題：multimap 直接保存重複時間鍵且保持排序，equal_range 可精確取得該時間的連續區間。
// 設計：對 timestamp 呼叫 equal_range；走訪半開區間；複製每筆事件名稱到結果。
// 成本：查找與收集為 O(log N + M)，額外空間 O(M)，N 為總事件數、M 為命中事件數。
// 上線注意：同鍵相對順序不應代替明確 priority；修改可能使保存的區間邊界失效，取消單筆要用 iterator。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'multimap.cpp' -o '/tmp/codex_cpp_C_Container_multimap' && '/tmp/codex_cpp_C_Container_multimap'
//
// === 預期輸出（節錄）===
// multimap：重複鍵、equal_range 與排程索引測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
