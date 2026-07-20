/*
 * 第 12 章：std::hash 與 unordered containers
 *
 * hash 將 key 映射成 size_t；unordered_map 再以 bucket 與 equality 找實際元素。
 * 核心契約：若 KeyEqual(a,b) 為 true，Hash(a) 必須等於 Hash(b)。反向不必成立，
 * 因不同 key 可以 collision。平均查找 O(1)，最壞 O(n)；不要把平均值當安全上限。
 */

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
    // 常見混合公式；不是密碼學 hash，也不承諾跨版本穩定值。
    return seed ^ (value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U));
}

struct CacheKey {
    std::string service;
    int version{};

    friend bool operator==(const CacheKey&, const CacheKey&) = default;
};

struct CacheKeyHash {
    std::size_t operator()(const CacheKey& key) const noexcept {
        std::size_t seed = std::hash<std::string>{}(key.service);
        return hash_combine(seed, std::hash<int>{}(key.version));
    }
};

// LeetCode 1：Two Sum。平均 O(n)；先查 complement 再 insert，避免同一元素重用。
std::pair<std::size_t, std::size_t>
leetcode_two_sum(const std::vector<int>& values, int target) {
    std::unordered_map<int, std::size_t> index;
    index.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (const auto found = index.find(target - values[i]); found != index.end()) {
            return {found->second, i};
        }
        index.emplace(values[i], i);
    }
    return {values.size(), values.size()};
}

// 【實務情境】以 service+version 複合 key 查部署 artifact cache。
using Cache = std::unordered_map<CacheKey, std::string, CacheKeyHash>;

std::string practical_cache_lookup(const Cache& cache, const CacheKey& key) {
    const auto found = cache.find(key);
    return found == cache.end() ? "miss" : found->second;
}

int main() {
    assert(std::hash<int>{}(42) == std::hash<int>{}(42)); // 同一執行中的相等輸入一致

    const auto answer = leetcode_two_sum({3, 2, 4}, 6);
    assert(answer.first == 1U && answer.second == 2U);

    Cache cache;
    cache.reserve(8U);
    cache.emplace(CacheKey{"search", 7}, "artifact-A");
    cache.emplace(CacheKey{"billing", 2}, "artifact-B");
    assert(practical_cache_lookup(cache, CacheKey{"search", 7}) == "artifact-A");
    assert(practical_cache_lookup(cache, CacheKey{"search", 8}) == "miss");

    // load_factor 過高會增加 collision；max_load_factor 影響何時 rehash。
    cache.max_load_factor(0.75F);
    cache.reserve(cache.size()); // setter 本身不保證立即 rehash，reserve 後才檢查
    assert(cache.load_factor() <= cache.max_load_factor());

    std::cout << "hash 測試完成\n";
}

/*
 * 【常見陷阱】
 * - std::hash 不是密碼學 hash，不可拿來存密碼、驗完整性或抵抗惡意 collision。
 * - 標準不保證 hash 值跨 process/版本/平台固定；不可持久化成檔案協定。
 * - rehash 使 iterator 失效；對元素的 reference/pointer 在標準 unordered container 中仍有效，
 *   但 erase 該元素當然會失效。
 * - 修改作為 key 的內容會破壞 bucket 不變量；unordered_set 元素視為 const key。
 *
 * 【面試段落】collision 怎麼處理？實作以 bucket 鏈結/其他結構再用 KeyEqual 判定。
 * 【練習】設計 case-insensitive string hash/equal，證明兩者對大小寫使用同一正規化。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_hash.cpp' -o '/tmp/codex_cpp_C_Utility_12_hash' && '/tmp/codex_cpp_C_Utility_12_hash'
//
// === 預期輸出（節錄）===
// hash 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
