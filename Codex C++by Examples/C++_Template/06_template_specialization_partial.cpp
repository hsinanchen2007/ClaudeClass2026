/*
 * 第 06 章：偏特化（partial specialization）
 *
 * 偏特化不是鎖定一個確切型別，而是匹配一整族型別，例如 T*、pair<A,B>。
 * 只有類別模板與變數模板能偏特化；函式模板請使用 overload、SFINAE 或 concepts。
 * 選擇順序：先找能匹配的特化，再選「最特殊」者；無匹配才用主模板。
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct TypeShape {
    static constexpr const char* name = "一般值";
    static constexpr int depth = 0;
};

template <typename T>
struct TypeShape<T*> {
    static constexpr const char* name = "指標";
    static constexpr int depth = 1 + TypeShape<T>::depth;
};

template <typename First, typename Second>
struct TypeShape<std::pair<First, Second>> {
    static constexpr const char* name = "二元組";
    static constexpr int depth = 1;
};

// LeetCode 283：Move Zeroes。Trait 判定「是否為 vector」，讓演算法只在預期容器啟用。
template <typename T>
struct IsVector : std::false_type {};

template <typename Value, typename Allocator>
struct IsVector<std::vector<Value, Allocator>> : std::true_type {};

template <typename Container>
void leetcode_move_zeroes(Container& values) {
    static_assert(IsVector<Container>::value, "本教學版本要求 std::vector");
    std::size_t write = 0;
    for (const auto value : values) {
        if (value != 0) {
            values[write++] = value;
        }
    }
    while (write < values.size()) {
        values[write++] = 0;
    }
}

// 實務：針對一般值與 pair 提供不同顯示格式。
template <typename T>
struct FieldFormatter {
    static std::string format(const T& value) { return std::to_string(value); }
};

template <typename A, typename B>
struct FieldFormatter<std::pair<A, B>> {
    static std::string format(const std::pair<A, B>& value) {
        return "(" + std::to_string(value.first) + "," + std::to_string(value.second) + ")";
    }
};

std::string practical_format_coordinate(int x, int y) {
    return FieldFormatter<std::pair<int, int>>::format({x, y});
}

int main() {
    static_assert(std::is_same_v<decltype(TypeShape<int>::depth), const int>);
    static_assert(TypeShape<int*>::depth == 1);
    static_assert(TypeShape<int**>::depth == 2);
    static_assert(TypeShape<std::pair<int, double>>::depth == 1);

    std::vector<int> values{0, 1, 0, 3, 12};
    leetcode_move_zeroes(values);
    assert((values == std::vector<int>{1, 3, 12, 0, 0}));

    assert(FieldFormatter<int>::format(7) == "7");
    using IntPair = std::pair<int, int>;
    // 先實例化再驗證；若把唯一用途放在 assert，-DNDEBUG 會讓 alias 變成未使用。
    const std::string formatted_pair = FieldFormatter<IntPair>::format({3, 9});
    if (formatted_pair != "(3,9)") {
        throw std::logic_error("pair partial specialization selected incorrectly");
    }
    assert(formatted_pair == "(3,9)");
    assert(practical_format_coordinate(4, 8) == "(4,8)");

    std::cout << TypeShape<int*>::name << '\n';
}

/*
 * 【陷阱】若兩個偏特化同樣適合而無法判斷誰更特殊，程式會 ambiguous。
 * 【陷阱】assert 是巨集；assert(Trait<pair<int,int>>::value) 的逗號會切開巨集參數。
 *         可用型別別名，或把整個運算式再包一層括號。
 * 【設計】不要用偏特化偷偷改變所有權語意；T 與 T* 的 API 若差太多，應拆成不同型別。
 * 【面試】std::vector<bool> 為何常被提到？它是標準庫的完整特化，operator[] 回 proxy 而非 bool&。
 * 【練習】新增 TypeShape<const T> 並思考 const int* 與 int* const 分別匹配何者。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_template_specialization_partial.cpp' -o '/tmp/codex_cpp_C_Template_06_template_specialization_partial' && '/tmp/codex_cpp_C_Template_06_template_specialization_partial'
//
// === 預期輸出（節錄）===
// 指標
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
