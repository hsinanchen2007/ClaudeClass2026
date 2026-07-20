/*
 * 第 02 章：std::tuple
 *
 * tuple<Ts...> 是固定長度、異質型別集合。可用 get<I>、get<T>（該型別需唯一）、
 * structured binding、tie、apply、tuple_cat。它適合短生命週期的資料搬運；若 tuple
 * 有很多欄、跨模組傳遞，讀者無法記住第 3 欄是什麼，應改 named struct。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using Point = std::tuple<int, int>;

std::tuple<std::string, int, bool> basic_user_record() {
    return {"Ada", 42, true};
}

// LeetCode 1637：Widest Vertical Area Between Two Points。
// y 不影響答案，但 tuple 仍保留完整座標。排序 O(n log n)、額外空間 O(n)。
int leetcode_max_vertical_width(std::vector<Point> points) {
    std::sort(points.begin(), points.end(), [](const Point& left, const Point& right) {
        return std::get<0>(left) < std::get<0>(right);
    });
    int widest = 0;
    for (std::size_t i = 1; i < points.size(); ++i) {
        widest = std::max(widest, std::get<0>(points[i]) - std::get<0>(points[i - 1U]));
    }
    return widest;
}

using DatabaseRow = std::tuple<int, std::string, double>;

// 實務：模擬 DB row -> 顯示文字。std::apply 將 tuple 展開成 callable 引數。
std::string practical_render_database_row(const DatabaseRow& row) {
    return std::apply([](int id, const std::string& sku, double price) {
        return "id=" + std::to_string(id) + ",sku=" + sku +
               ",price=" + std::to_string(price);
    }, row);
}

int main() {
    const auto user = basic_user_record();
    assert(std::get<0>(user) == "Ada");
    assert(std::get<int>(user) == 42); // int 在 tuple 中唯一，才可按型別 get

    const auto& [name, id, active] = user;
    assert(name == "Ada" && id == 42 && active);
    static_assert(std::tuple_size_v<decltype(user)> == 3U);

    assert(leetcode_max_vertical_width({{8, 7}, {9, 9}, {7, 4}, {9, 7}}) == 1);
    assert(leetcode_max_vertical_width({{3, 1}, {9, 0}, {1, 0}, {4, 2}}) == 5);

    const DatabaseRow row{7, "RTX", 1599.0};
    assert(practical_render_database_row(row).starts_with("id=7,sku=RTX"));

    // tie 可把 tuple 指定到既有變數；ignore 跳過欄位。
    int row_id = 0;
    std::string sku;
    std::tie(row_id, sku, std::ignore) = row;
    assert(row_id == 7 && sku == "RTX");

    const auto extended = std::tuple_cat(row, std::tuple{true});
    static_assert(std::tuple_size_v<decltype(extended)> == 4U);
    assert(std::get<3>(extended));

    std::cout << "tuple 測試完成\n";
}

/*
 * 【常見陷阱】
 * - get<T> 在 T 出現 0 次或多次都編譯失敗；穩定介面通常用 index 或 structured binding。
 * - forward_as_tuple 可能保存 rvalue references；若 tuple 活過完整 expression 會懸空。
 * - tie 建立 reference tuple，不擁有資料；不要從函式回傳指向區域變數的 tie。
 * - tuple comparison 是逐欄 lexicographical，欄位順序就是排序語意。
 *
 * 【面試段落】tuple 與 variant 差異？tuple 同時擁有所有欄；variant 同一時間只擁有其中一型。
 * 【練習】用 index_sequence 寫自己的 apply，並說明 std::invoke 的必要性。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_tuple.cpp' -o '/tmp/codex_cpp_C_Utility_02_tuple' && '/tmp/codex_cpp_C_Utility_02_tuple'
//
// === 預期輸出（節錄）===
// tuple 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
