/*
 * 第 14 章：fold expression（C++17）
 *
 * Fold 將 parameter pack 與二元運算子折疊：
 *   (... + args)       unary left fold
 *   (args + ...)       unary right fold
 *   (init + ... + args) binary left fold
 * 左右差異在非結合運算（減法、字串組合）尤其重要。&& 的 identity 是 true，
 * || 的 identity 是 false，逗號運算子也可對空 pack 使用；其他 unary fold 空 pack 多半不合法。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

template <typename... Numbers>
constexpr auto sum(Numbers... values) {
    return (0 + ... + values); // binary left fold，空 pack 也得到 0
}

template <typename... Conditions>
constexpr bool all(Conditions... conditions) {
    return (... && static_cast<bool>(conditions));
}

// LeetCode 1929：Concatenation of Array 的 pack 示範版。
template <typename T, typename... Vectors>
std::vector<T> leetcode_concatenate(const std::vector<T>& first, const Vectors&... rest) {
    static_assert((std::is_same_v<std::vector<T>, Vectors> && ...));
    std::size_t total = first.size() + (std::size_t{0} + ... + rest.size());
    std::vector<T> result;
    result.reserve(total);
    result.insert(result.end(), first.begin(), first.end());
    (result.insert(result.end(), rest.begin(), rest.end()), ...);
    return result;
}

struct Request {
    bool authenticated{};
    bool quota_available{};
    bool schema_valid{};
};

// 實務：把任意數量的驗證器折疊；&& 具有短路語意，前一項 false 後不再呼叫後面。
template <typename... Validators>
bool practical_validate(const Request& request, Validators... validators) {
    return (validators(request) && ...);
}

int main() {
    static_assert(sum() == 0);
    static_assert(sum(1, 2, 3, 4) == 10);
    static_assert(all(true, 1, 2.5));

    const std::vector<int> joined = leetcode_concatenate(std::vector<int>{1, 2, 1},
                                                         std::vector<int>{1, 2, 1});
    assert((joined == std::vector<int>{1, 2, 1, 1, 2, 1}));

    const Request good{true, true, true};
    const Request bad{true, false, true};
    const auto auth = [](const Request& r) { return r.authenticated; };
    const auto quota = [](const Request& r) { return r.quota_available; };
    const auto schema = [](const Request& r) { return r.schema_valid; };
    assert(practical_validate(good, auth, quota, schema));
    assert(!practical_validate(bad, auth, quota, schema));

    std::cout << "fold expression 測試完成\n";
}

/*
 * 【陷阱】`(args - ...)` 與 `(... - args)` 結果不同，先在紙上展開再選方向。
 * 【陷阱】逗號 fold 常用於副作用，但若能回傳轉換後容器，通常更容易測試。
 * 【面試】fold 是否保證順序？&&、||、逗號保有其語意；其他運算要看運算子求值規則。
 * 【練習】寫 print_with_separator(separator, args...)，避免最後多一個 separator。
 */
