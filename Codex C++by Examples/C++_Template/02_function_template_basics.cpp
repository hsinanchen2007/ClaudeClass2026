/*
 * 第 02 章：函式模板基礎
 *
 * 函式模板不是函式，而是產生函式的藍圖。呼叫 binary_search_index(values, key)
 * 時，編譯器由引數推導 T；若需求不成立（例如 T 不可比較），編譯會失敗。
 * C++20 可用 concept 提供更漂亮的限制，本章先掌握推導、明確指定與參考參數。
 */

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

template <typename T>
T square(T value) {
    return value * value;
}

// LeetCode 704：Binary Search。
// Compare 預設為 less<>；只要資料依相同規則排序，就能替換成 greater<>。
template <typename T, typename Compare = std::less<>>
int leetcode_binary_search_index(const std::vector<T>& values,
                                 const T& target,
                                 Compare before = {}) {
    std::size_t left = 0;
    std::size_t right = values.size(); // 半開區間 [left, right)
    while (left < right) {
        const std::size_t mid = left + (right - left) / 2;
        if (before(values[mid], target)) {
            left = mid + 1;
        } else if (before(target, values[mid])) {
            right = mid;
        } else {
            return static_cast<int>(mid);
        }
    }
    return -1;
}

struct Metric {
    std::string name;
    double value{};
};

// 實務：以投影函式找最大值；資料型別與取值方式都由呼叫端決定。
template <typename Range, typename Projection>
const typename Range::value_type& practical_max_by(const Range& values, Projection project) {
    assert(!values.empty());
    const auto* best = &values.front();
    for (const auto& value : values) {
        if (project(*best) < project(value)) {
            best = &value;
        }
    }
    return *best;
}

int main() {
    // 基礎推導：square<int> 與 square<double> 是不同實體。
    assert(square(5) == 25);
    assert(square(1.5) == 2.25);
    assert(square<long long>(9) == 81); // 也可明確指定 T

    // LeetCode 式測試：O(log n) 時間、O(1) 空間。
    const std::vector<int> ascending{-1, 0, 3, 5, 9, 12};
    assert(leetcode_binary_search_index(ascending, 9) == 4);
    assert(leetcode_binary_search_index(ascending, 2) == -1);

    const std::vector<int> descending{9, 7, 4, 1};
    assert(leetcode_binary_search_index(descending, 4, std::greater<>{}) == 2);

    // 實務：lambda 是投影，模板保留其具體型別，通常可被 inline。
    const std::vector<Metric> metrics{{"cpu", 42.0}, {"gpu", 91.5}, {"disk", 67.0}};
    const Metric& hottest = practical_max_by(metrics, [](const Metric& m) { return m.value; });
    assert(hottest.name == "gpu");

    std::cout << "函式模板測試完成\n";
}

/*
 * 【陷阱】
 * - 傳值參數 T value 會移除 top-level const/reference；T& 或 forwarding reference 規則不同。
 * - 回傳 typename Range::value_type& 前必須確保容器非空且活得比 reference 久。
 * - 二分搜尋的比較規則必須與排序規則一致，否則結果無意義。
 *
 * 【面試】函式模板可否只靠回傳型別推導 T？一般不行，因回傳型別不參與引數推導。
 * 【練習】讓 binary_search_index 接受 vector 以外的 random-access range。
 */
