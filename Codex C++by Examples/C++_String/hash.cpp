/*
 * std::hash<std::string>：把字串映射成 size_t，供 unordered 容器使用
 *
 * 相等字串在同一次實作中必須有相等 hash；不同字串可能碰撞，因此 hash 相等不代表
 * 字串相等。演算法與數值不具跨標準庫/版本/程序持久化保證，不可當檔案格式、網路
 * 協定、密碼學 hash 或長期 shard 規則。unordered_map 仍用 == 解決碰撞。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void basic_demo() {
    const std::hash<std::string> hasher;
    const std::string first = "same";
    const std::string second = "same";
    assert(first == second);
    assert(hasher(first) == hasher(second));
    // 不可 assert hash("a") != hash("b")：碰撞始終允許。
}

// LeetCode 383（Ransom Note）：hash table 計數；此題小寫字母其實 array 更快。
bool leetcode_can_construct(const std::string& note, const std::string& magazine) {
    std::unordered_map<char, int> counts;
    for (const char ch : magazine) ++counts[ch];
    for (const char ch : note) {
        auto found = counts.find(ch);
        if (found == counts.end() || found->second == 0) return false;
        --found->second;
    }
    return true;
}

// 實務：程序內暫時分 shard 可用 std::hash；若資料需重啟後留在同 shard，改用明定 hash。
std::size_t practical_ephemeral_shard(const std::string& key, const std::size_t shard_count) {
    assert(shard_count > 0U);
    return std::hash<std::string>{}(key) % shard_count;
}

// 透明 lookup 是進階優化；一般 unordered_map<string,...> 查 string_view 可能需建 string。
int practical_lookup_count(const std::unordered_map<std::string, int>& table,
                           const std::string& key) {
    const auto found = table.find(key);
    return found == table.end() ? 0 : found->second;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_can_construct("aa", "aab"));
    assert(!leetcode_can_construct("aa", "ab"));
    const std::size_t shard = practical_ephemeral_shard("user-42", 8U);
    assert(shard < 8U);  // 不檢查精確值，避免依賴實作。
    const std::unordered_map<std::string, int> table{{"ok", 3}};
    assert(practical_lookup_count(table, "ok") == 3);
    assert(practical_lookup_count(table, "missing") == 0);
    std::cout << "hash: tests passed\n";
}

/*
 * 【面試速查】
 * - 平均查找 O(1)，最壞碰撞退化 O(n)。reserve 可降低 rehash。
 * - hash collision 不是 bug；容器必須再用 equality 比較 key。
 * - 密碼/完整性請用專用 cryptographic hash；std::hash 沒安全承諾。
 * - 對外可重現 shard 應指定演算法、seed、byte order 與版本。
 * 【練習】比較 array<26> 與 unordered_map<char,int> 解 Ransom Note 的空間/常數成本。
 */

/*
 * 【教科書補充：hash table 的必要 invariant】
 * - shard_count 必須大於零；assert 在 release 會消失，正式 API 要用持續存在的驗證避免 `% 0` UB。
 * - KeyEqual(a,b)==true 必須推出 hash(a)==hash(b)，否則查找可能永遠找不到已存在的 key。
 * - rehash 會使 iterator 失效，但未被 erase 元素的 reference/pointer 仍有效；兩者不要混為一談。
 * - std::hash 結果不承諾跨程序、版本或平台穩定，不能直接當持久化 shard/協定格式。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'hash.cpp' -o '/tmp/codex_cpp_C_String_hash' && '/tmp/codex_cpp_C_String_hash'
//
// === 預期輸出（節錄）===
// hash: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
