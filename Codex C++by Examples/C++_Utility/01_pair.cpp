/*
 * 第 01 章：std::pair
 *
 * pair<T,U> 把兩個可能不同型別的值組成一個物件，成員是 first/second。
 * 適合「兩者天然成對且語意簡單」：座標、key/value、成功旗標+結果。
 * 若欄位有明確領域名稱或不變量，struct 通常比 pair.first/pair.second 更易維護。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::pair<std::string, int> basic_lookup() {
    // make_pair 可推導型別；C++17 也可直接 std::pair{"gpu", 75} 使用 CTAD。
    return std::make_pair(std::string{"gpu"}, 75);
}

// LeetCode 1：Two Sum。pair 很適合回傳兩個 index。
// 平均 O(n) 時間、O(n) 額外空間；找不到時回傳 size,size 作 sentinel。
std::pair<std::size_t, std::size_t>
leetcode_two_sum(const std::vector<int>& values, int target) {
    std::unordered_map<int, std::size_t> seen;
    seen.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        const int wanted = target - values[i];
        if (const auto found = seen.find(wanted); found != seen.end()) {
            return {found->second, i};
        }
        seen.emplace(values[i], i);
    }
    return {values.size(), values.size()};
}

// 實務：解析 HTTP 狀態。這裡 pair 的 first/second 尚可理解；大型 API 應改 named struct。
std::pair<bool, std::string> practical_validate_http_status(int status) {
    if (status >= 200 && status < 300) {
        return {true, "request accepted"};
    }
    if (status == 404) {
        return {false, "resource not found"};
    }
    return {false, "unexpected status=" + std::to_string(status)};
}

int main() {
    const auto metric = basic_lookup();
    assert(metric.first == "gpu");
    assert(metric.second == 75);

    // structured binding 依 pair 的 tuple-like protocol 解包；此處用 const auto& 避免複製。
    const auto& [name, value] = metric;
    assert(name == "gpu" && value == 75);

    const auto answer = leetcode_two_sum({2, 7, 11, 15}, 9);
    assert((answer == std::pair<std::size_t, std::size_t>(0U, 1U)));
    const auto absent = leetcode_two_sum({1, 2, 3}, 99);
    assert(absent.first == 3U && absent.second == 3U);

    const auto [ok, message] = practical_validate_http_status(404);
    assert(!ok);
    assert(message == "resource not found");

    // pair 具有 lexicographical comparison：先比 first，相等才比 second。
    assert((std::pair{1, 9} < std::pair{2, 0}));

    std::cout << "pair 測試完成\n";
}

/*
 * 【常見陷阱】
 * - `auto [a,b] = pair` 會複製；要修改原物件用 `auto& [a,b]`。
 * - pair 的比較順序不一定符合領域需求，例如版本字串/自然排序需自訂 comparator。
 * - `std::pair<int&,int&>` 保存 reference；referent 死亡後 pair 仍在但已懸空。
 * - assert(pair<int,int>{...} == ...) 會被逗號干擾，使用型別別名或額外括號。
 *
 * 【面試段落】pair 與 struct 怎麼選？
 * 臨時、局部、兩欄語意明顯可用 pair；跨 API 或欄位需要名稱/驗證時選 struct。
 * 【練習】把 practical 回傳值改為 HttpValidation struct，比較呼叫端可讀性。
 */
