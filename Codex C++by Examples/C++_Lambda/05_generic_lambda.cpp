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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate（存在重複元素）
// 題目：判斷整數陣列是否有任一值至少出現兩次；[1,2,3,1] 為 true，[1,2,3,4] 為 false。
// 為何使用本章主題：generic lambda 把原題的 int 解法泛化到任何可雜湊型別，方便同一 closure
// 處理 int 與 string；這是教學擴充，LeetCode 原始介面只要求整數。
// 思路：建立空 unordered_set；依序插入每個元素；首次插入失敗時由 any_of 立即回傳 true。
// 複雜度：平均時間 O(N)、額外空間 O(N)，N 是 values 長度；惡劣雜湊下時間可能退化。
// 易錯點：T 必須可雜湊且可比較相等；空陣列沒有重複值，predicate 參考捕獲的 seen 只在呼叫中有效。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】依投影欄位找最大監控指標
// 情境：監控資料含指標名稱與數值，呼叫端要指定比較欄位並取出數值最大的完整資料列。
// 為何使用本章主題：泛型 Projection 可接受不同 lambda，不必為每種資料列或欄位另寫比較函式；
// 相較固定 comparator，投影介面把「取 key」與 max_element 的排序流程分離。
// 設計：接收 range 與 projection；比較兩列的投影結果；解參考最大元素並以值回傳。
// 成本：時間 O(N)、額外空間 O(1)，N 是 rows 數量；目前回傳值會複製一個最大元素。
// 上線注意：rows 必須非空，否則會解參考 end；昂貴元素宜回 iterator，並記錄其失效規則。
// -----------------------------------------------------------------------------
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

// 【延伸練習】讓 practical_max_by 回 iterator/optional，正確處理空 range 並避免不必要複製。

/*
 * 【教科書補充：泛型回傳與空範圍】
 * - identity(temporary) 可回 T&&，但 temporary 在完整運算式結束時死亡；不得保存該 reference。
 * - perfect forwarding 保留 value category，並不延長任何來源物件生命週期。
 * - practical_max_by 對空 range 會解參考 end()；目前契約是 non-empty，正式 API 應回 iterator/optional。
 * - 目前 auto return 會複製最大元素並移除 cv/ref；若元素昂貴，回 iterator 並明列失效規則更好。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_generic_lambda.cpp' -o '/tmp/codex_cpp_C_Lambda_05_generic_lambda' && '/tmp/codex_cpp_C_Lambda_05_generic_lambda'
//
// === 預期輸出（節錄）===
// generic lambda：template call operator、duplicate、projection 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
