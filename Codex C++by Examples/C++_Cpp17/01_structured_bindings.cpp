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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：在 nums 中找兩個不同索引使其值相加為 target；[2,7,11,15] 與 9 回傳 (0,1)。
// 為何使用本章主題：structured binding 將雜湊表 entry 拆成 value/index，也將回傳 pair 拆成兩個測試值。
// 思路：1. 逐項計算所需補數；2. 在已看過的值中查找；3. 命中就回舊索引與目前索引，否則插入目前值。
// 複雜度：N 為元素數；平均時間 O(N)、雜湊最壞 O(N^2)，額外空間 O(N)。
// 易錯點：const auto& 避免複製 map entry；先查後插可防同一元素配自己，失敗 pair 需有明確契約。
// -----------------------------------------------------------------------------
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

namespace practical {
using ServiceOwner = std::pair<std::string, std::string>;

// -----------------------------------------------------------------------------
// 【日常實務範例】服務負責人索引建置
// 情境：輸入多筆 (service, owner) 清單，建立可依 service 查 owner 的排序索引。
// 為何使用本章主題：`const auto& [service, owner]` 直接命名 pair 欄位，避免 first/second 語意不清與字串複製。
// 設計：1. 逐筆解構 service/owner；2. emplace 到 map；3. 回傳自行擁有字串的索引。
// 成本：N 為資料筆數；時間 O(N log N)，結果空間 O(N)。
// 上線注意：重複 service 的 emplace 會保留第一筆且靜默忽略後者；需明訂衝突、空值與大小寫政策。
// -----------------------------------------------------------------------------
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
