/*
 * C++17 教科書：structured bindings
 *
 * `auto [a,b] = object` 可拆 array、tuple-like、或具合適 public members 的 aggregate。
 * 對 value elements，auto 建立 hidden object 的副本；auto& 綁原物件；const auto&
 * 只讀免複製。但 tuple-like element 本身若是 reference（例如 tuple<int&>），即使
 * `auto [x]` 複製 tuple，x 仍可指向原 referent。decltype(name) 也有特殊規則。
 *
 * 【map】`for (const auto& [key,value] : map)` 清楚表達 pair；要改 mapped value 用 auto&。
 * 【生命週期】`const auto& [a,b] = temporary_tuple()` 可延長 hidden object 生命週期；
 * 但從 temporary 內部取得 view/pointer 仍需各自檢查。
 * 【陷阱】漏寫 & 會複製大型 value，修改 binding 不會回原容器。
 * 【面試題】`auto [x,y]=pair` 與 `auto& [x,y]=pair` 修改效果有何不同？
 * 【練習】把 practical_index 改成允許同一 owner 多個 service。
 */

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    std::pair<std::string, int> item{"apple", 3};
    auto [copied_name, copied_count] = item;
    copied_count = 9;
    assert(item.second == 3 && copied_name == "apple");

    auto& [name, count] = item;
    count = 7;
    assert(name == "apple" && item.second == 7);

    int original = 10;
    auto references = std::tie(original);
    auto [still_a_reference] = references;
    static_assert(std::is_same<decltype(still_a_reference), int&>::value,
                  "tuple<int&> 的 element binding 仍是 int&");
    still_a_reference = 12;
    assert(original == 12);
}
}  // namespace basic

namespace leetcode {
// LeetCode 1：Two Sum。structured binding 拆出 map 中的 value/index（測試也拆答案）。
// 平均 O(n) time / O(n) space。
std::pair<int, int> leetcode_two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<int, int> index_by_value;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        const auto found = index_by_value.find(target - nums[i]);
        if (found != index_by_value.end()) {
            const auto& [value, index] = *found;
            static_cast<void>(value);
            return {index, static_cast<int>(i)};
        }
        index_by_value.emplace(nums[i], static_cast<int>(i));
    }
    return {-1, -1};
}

void leetcode_test() {
    const auto [left, right] = leetcode_two_sum({2, 7, 11, 15}, 9);
    assert(left == 0 && right == 1);
}
}  // namespace leetcode

// 【實務案例】service-owner 索引：結構化綁定直接命名 pair 的 service 與 owner。
namespace practical {
using ServiceOwner = std::pair<std::string, std::string>;

std::map<std::string, std::string> practical_index(const std::vector<ServiceOwner>& rows) {
    std::map<std::string, std::string> result;
    for (const auto& [service, owner] : rows) {
        result.emplace(service, owner);
    }
    return result;
}

void practical_test() {
    const auto index = practical_index({{"api", "alice"}, {"db", "bob"}});
    assert(index.at("api") == "alice" && index.at("db") == "bob");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "structured bindings：copy/ref、Two Sum、service index 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_structured_bindings.cpp' -o '/tmp/codex_cpp_C_Cpp17_01_structured_bindings' && '/tmp/codex_cpp_C_Cpp17_01_structured_bindings'
//
// === 預期輸出（節錄）===
// structured bindings：copy/ref、Two Sum、service index 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
