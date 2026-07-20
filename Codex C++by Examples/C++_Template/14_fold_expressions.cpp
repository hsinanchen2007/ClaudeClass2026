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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接）
// 題目：回傳 nums 接上自己；[1,2,1] 變成 [1,2,1,1,2,1]。
// 為何使用本章主題：本例泛化為 first 加任意數量 rest vectors，fold 計算總長度並依序 insert；
// 原題只需插入同一陣列兩次，這是 variadic fold 的教學擴充。
// 思路：折疊所有 size 得總長度；一次 reserve；先插 first，再以逗號 fold 依序插入 rest。
// 複雜度：時間 O(S)、結果空間 O(S)，S 是所有輸入 vector 元素總數。
// 易錯點：rest 必須精確為 vector<T>；輸入不可與 result alias，總長度相加也可能 size_t 溢位。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】請求前置驗證鏈
// 情境：請求進入服務前要依序確認登入、配額與 schema，任一失敗便停止後續驗證。
// 為何使用本章主題：`validators(request) && ...` 把任意數量 validator 折疊並保留 && 短路；
// 相較手動 if 鏈，新增規則不需修改 practical_validate 本體。
// 設計：Request 保存三項狀態；呼叫端建立各 validator；fold 由左至右執行並回傳總結果。
// 成本：最多 O(V) 次驗證、額外空間 O(1)，V 是 validator 數；每項內部 I/O 成本另計。
// 上線注意：validator 順序會影響副作用與成本；空 pack 回 true，需確認是否符合安全政策。
// -----------------------------------------------------------------------------
struct Request {
    bool authenticated{};
    bool quota_available{};
    bool schema_valid{};
};

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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '14_fold_expressions.cpp' -o '/tmp/codex_cpp_C_Template_14_fold_expressions' && '/tmp/codex_cpp_C_Template_14_fold_expressions'
//
// === 預期輸出（節錄）===
// fold expression 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
