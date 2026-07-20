/*
 * Lambda 教科書 05：generic lambda 與 C++20 template parameter list
 *
 * C++14 的 `[](auto x)` 讓 call operator 成為 function template。C++20 可寫
 * `[]<class T>(T x)`，便於同一 T 出現多次、使用 type traits/concepts 或明確 template 名稱。
 * `auto&&` 參數是 forwarding reference；若要把 value category 傳下去，搭 std::forward。
 *
 * 【編譯模型】每種 argument type 產生 specialization，通常可 inline，但可能增加 code size。
 * 【限制】泛型不代表接受一切；body 對某型別不合法時才會在 instantiation 報錯。C++20
 * concepts/`requires` 能提前表達 contract，本例保持語法集中而不引入完整 concepts 章。
 * 【陷阱】`auto value` 複製；處理大型物件或 polymorphic object 時可能 slicing/昂貴。
 * 【面試題】generic lambda 與 function template 的核心差異？前者是 closure object member template。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    const auto add = []<class T>(T left, T right) { return left + right; };
    assert(add(2, 3) == 5);
    assert(add(std::string("C++"), std::string("20")) == "C++20");

    const auto identity = [](auto&& value) -> decltype(auto) {
        return std::forward<decltype(value)>(value);
    };
    int number = 7;
    static_assert(std::is_same_v<decltype(identity(number)), int&>);
}
}  // namespace basic

namespace leetcode {
// LeetCode 217：Contains Duplicate。generic lambda 讓相同演算法可測 int/string。
const auto leetcode_contains_duplicate = []<class T>(const std::vector<T>& values) {
    std::unordered_set<T> seen;
    return std::any_of(values.begin(), values.end(),
                       [&seen](const T& value) { return !seen.insert(value).second; });
};

void leetcode_test() {
    assert(leetcode_contains_duplicate(std::vector<int>{1, 2, 3, 1}));
    assert(!leetcode_contains_duplicate(std::vector<std::string>{"a", "b"}));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Metric {
    std::string name;
    double value;
};

template <class Range, class Projection>
auto practical_max_by(const Range& rows, Projection projection) {
    return *std::max_element(rows.begin(), rows.end(),
        [&](const auto& left, const auto& right) {
            return projection(left) < projection(right);
        });
}

void practical_test() {
    const std::vector<Metric> metrics{{"cpu", 70.0}, {"gpu", 91.0}};
    const Metric hottest = practical_max_by(metrics, [](const Metric& metric) { return metric.value; });
    assert(hottest.name == "gpu");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "generic lambda：template call operator、duplicate、projection 測試通過\n";
}
