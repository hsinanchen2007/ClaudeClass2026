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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 704. Binary Search（二分搜尋）
// 題目：在已排序陣列中找 target 索引，找不到回 -1；[-1,0,3,5,9,12] 查 9 得 4。
// 為何使用本章主題：函式模板讓元素型別 T 與 Compare 由呼叫端決定，預設 less<> 處理升冪，
// 也能以 greater<> 搜尋依相同規則降冪排列的資料；原題只要求升冪 int。
// 思路：維護半開區間 [left,right)；比較 middle 與 target；縮小其中一半，命中時回索引。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 是 values 長度。
// 易錯點：資料排序規則必須和 Compare 一致；Compare 應形成嚴格弱序，巨大索引轉 int 可能溢位。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】依投影選出最高監控指標
// 情境：監控列包含名稱與數值，呼叫端要選擇比較欄位並取得最大值的原始資料列。
// 為何使用本章主題：Range 與 Projection 都是模板參數，lambda 的具體型別可直接 inline；
// 相較 std::function 沒有固定 type-erasure 邊界，也不必替每種 Metric 欄位寫新函式。
// 設計：以第一列初始化 best；逐列比較投影值；較大時更新指標，最後回傳容器內參考。
// 成本：時間 O(N)、額外空間 O(1)，N 是 values 數量，每筆呼叫 projection 至多兩次。
// 上線注意：Range 必須非空且在回傳參考使用期間存活；projection 應穩定且不可改動比較欄位。
// -----------------------------------------------------------------------------
struct Metric {
    std::string name;
    double value{};
};

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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_function_template_basics.cpp' -o '/tmp/codex_cpp_C_Template_02_function_template_basics' && '/tmp/codex_cpp_C_Template_02_function_template_basics'
//
// === 預期輸出（節錄）===
// 函式模板測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
