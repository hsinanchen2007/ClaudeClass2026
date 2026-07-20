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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 383. Ransom Note（贖金信）
// 題目：判斷 note 的每個字元能否由 magazine 中的字元各使用至多一次組成；"aa"、"aab" 為 true。
// 為何使用本章主題：unordered_map<char,int> 以 hash table 保存可用次數；它沒有直接使用
//       std::hash<string>，且原題小寫字母固定時 array<26> 常有更低常數成本。
// 思路：1. 掃 magazine 累加次數；2. 掃 note 查 key；3. 缺 key 或次數為零即失敗；4. 否則遞減。
// 複雜度：平均時間 O(N+M)、最壞 O((N+M)^2)，額外空間 O(K)，K 是不同字元數。
// 易錯點：hash 碰撞不代表 key 相等；本解法會保留歸零 entry，且固定小字母題目可改用 array。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】程序內暫時工作分片
// 情境：單次程序生命週期內把工作 key 分配到 shard_count 個 worker，不要求重啟後維持同一分片。
// 為何使用本章主題：std::hash<string> 能直接產生 unordered 容器同類的 size_t hash；相較
//       自訂持久演算法適合短生命週期用途，但不提供跨版本穩定性。
// 設計：1. 確認 shard_count 非零；2. 計算 key hash；3. 取模映射到 [0, shard_count)。
// 成本：hash 時間 O(N)、額外空間 O(1)，N 是 key 長度。
// 上線注意：assert 在 release 不會防 `%0`；正式 API 必須持續驗證，且不可將結果寫成永久 shard ID。
// -----------------------------------------------------------------------------
std::size_t practical_ephemeral_shard(const std::string& key, const std::size_t shard_count) {
    assert(shard_count > 0U);
    return std::hash<std::string>{}(key) % shard_count;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】字串計數表查詢
// 情境：程序內統計表以 std::string 為 key，查詢存在時回 count，不存在時回 0。
// 為何使用本章主題：unordered_map 使用 std::hash<string> 定位 bucket，再以字串相等解決碰撞；
//       相較線性掃描，正常負載下查詢平均為常數時間。
// 設計：1. 呼叫 find 一次；2. 與 end 比較；3. 命中回 value，未命中回領域預設 0。
// 成本：平均時間 O(1)、最壞 O(K)，額外空間 O(1)，K 是容器元素數；字串 hash 仍需 O(N) bytes。
// 上線注意：0 若也是有意義的已存值，caller 不能據此判存在性；高頻 string_view 查詢可考慮
//       transparent hash/equality，並監控 load factor 與惡意碰撞風險。
// -----------------------------------------------------------------------------
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
