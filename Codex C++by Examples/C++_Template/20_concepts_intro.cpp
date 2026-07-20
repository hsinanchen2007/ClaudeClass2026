/*
 * 第 20 章：Concepts 入門（C++20）
 *
 * concept 是「一組編譯期需求的命名」。它不只縮短 SFINAE，還把 API 契約放進簽名。
 * 可用 `template <Concept T>`、`requires Concept<T>` 或 abbreviated template `Concept auto`。
 * Concept 判斷 expression 是否有效及其型別性質，不保證執行期語意（例如比較是否真為全序）。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. Concept 是命名過的 constraint predicate，不是 class、interface 物件，也沒有 runtime 狀態。
 * 2. 可寫 `template<Addable T>`、尾端 `requires Addable<T>`，或參數 `const Metric auto&`。
 * 3. requires-expression 內的 expression 位於 unevaluated context，只驗證能否形成及回傳型別。
 * 4. `same_as<T>` 要求精確型別；`convertible_to<T>` 較寬，接受語意一致的隱式/顯式轉換。
 * 5. 本例 Addable 選 same_as，所以 short+short 產生 int 而不符合，避免悄悄窄化回 short。
 * 6. 限制應描述演算法最小需求；限制過寬會讓錯誤掉進本體，過窄則拒絕合法 proxy/view。
 * 7. 標準 ranges concept 優先於手刻「有 size 與 operator[]」，因為它表達正式 iterator 契約。
 * 8. BinarySearchableRange 要求 const range 可 random access、可取大小，元素值具 total ordering。
 * 9. 額外 expression constraint 確認實際 reference/proxy 能與 value_type 雙向做 `<` 比較。
 * 10. Concept 仍不能證明輸入已排序；二分搜尋的 sorted invariant 必須寫成 runtime 前置條件。
 * 11. 二分搜尋時間 O(log n)、額外空間 O(1)，使用半開區間 [left,right) 避免 size-1 下溢。
 * 12. 空 range 合法並回 -1；有重複值時回任一匹配位置，不承諾第一個或最後一個。
 * 13. 回傳 range 的 difference_type，避免巨大 range 索引硬轉 int；-1 是明訂的 not-found sentinel。
 * 14. LeetCode 契約還要求比較關係穩定且符合嚴格全序；concept 只能檢查語法近似條件。
 * 15. add 按值取得兩個 T，對 string 會有參數複製與結果配置；concept 本身沒有 runtime 成本。
 * 16. Metric 只借用輸入，先轉成 owning string/double，再建立 owning 回傳字串，不保存引用。
 * 17. 每種滿足 constraint 的 T/Range 仍會實體化函式，可能增加 code size 與 compile time。
 * 18. Concepts 可讓不合格候選提早退出，常比 enable_if 少產生級聯錯誤，但複雜 constraint 仍昂貴。
 * 19. GCC 可提高 `-fconcepts-diagnostics-depth` 看失敗 requirement；Clang 通常逐層列 constraint。
 * 20. 面試追問：concept 能否保證 operator< 無副作用？不能，語意公理仍由型別作者遵守。
 * 21. 面試追問：何時不用模板 concept？需要 runtime 異質集合或穩定 ABI 時考慮 virtual/type erasure。
 */

#include <cassert>
#include <concepts>
#include <cstddef>
#include <forward_list>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

template <typename T>
concept Addable = requires(const T& left, const T& right) {
    { left + right } -> std::same_as<T>;
};

template <Addable T>
T add(T left, T right) {
    return left + right;
}

template <typename Range>
concept BinarySearchableRange =
    std::ranges::random_access_range<const Range> &&
    std::ranges::sized_range<const Range> &&
    std::totally_ordered<std::ranges::range_value_t<Range>> &&
    requires(const Range& values, const std::ranges::range_value_t<Range>& target) {
        { *std::ranges::begin(values) < target } -> std::convertible_to<bool>;
        { target < *std::ranges::begin(values) } -> std::convertible_to<bool>;
    };

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 704. Binary Search（二分搜尋）
// 題目：在升冪陣列找 target 索引，找不到回 -1；[-1,0,3,5,9,12] 查 9 得 4。
// 為何使用本章主題：BinarySearchableRange 明列 random access、sized、排序與雙向比較需求，
// 讓 forward_list 在呼叫邊界被拒絕；原題固定 vector<int>，此處泛化為 range。
// 思路：以 difference_type 維護半開區間；由 first+mid 讀值；比較後縮左或右界，命中回 mid。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 是 range 長度。
// 易錯點：concept 無法證明資料真的已排序或比較無副作用；重複值時不保證回第一筆。
// -----------------------------------------------------------------------------
template <BinarySearchableRange Range>
auto leetcode_binary_search_index(
    const Range& values, const std::ranges::range_value_t<Range>& target)
    -> std::ranges::range_difference_t<const Range> {
    using Difference = std::ranges::range_difference_t<const Range>;
    const auto first = std::ranges::begin(values);
    Difference left = 0;
    Difference right = std::ranges::distance(values);
    while (left < right) {
        const Difference mid = left + (right - left) / 2;
        const auto& current = *(first + mid);
        if (current < target) {
            left = mid + 1;
        } else if (target < current) {
            right = mid;
        } else {
            return mid;
        }
    }
    return static_cast<Difference>(-1);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】受約束的監控指標顯示
// 情境：顯示層只接受具有可轉字串 name 與可轉 double value 的監控資料。
// 為何使用本章主題：Metric concept 把結構需求命名並放在 abbreviated template 參數上；
// 相較無限制模板，傳入 int 時會在 API 邊界得到 constraint 診斷。
// 設計：concept 檢查兩成員 expression；Gauge 滿足契約；renderer 建立 owning 名稱與數值字串。
// 成本：constraint 僅在編譯期，runtime 時間與空間 O(L)，L 是輸出字串長度。
// 上線注意：concept 不驗 name 非空、value 有限或單位；浮點格式與 locale 也需明訂。
// -----------------------------------------------------------------------------
template <typename T>
concept Metric = requires(const T& metric) {
    { metric.name } -> std::convertible_to<std::string>;
    { metric.value } -> std::convertible_to<double>;
};

struct Gauge {
    std::string name;
    double value{};
};

std::string practical_render_metric(const Metric auto& metric) {
    return std::string{metric.name} + "=" +
           std::to_string(static_cast<double>(metric.value));
}

int main() {
    static_assert(Addable<int>);
    static_assert(!Addable<short>);
    assert(add(2, 3) == 5);
    assert(add(std::string{"C++"}, std::string{"20"}) == "C++20");

    const std::vector<int> values{-1, 0, 3, 5, 9, 12};
    static_assert(BinarySearchableRange<decltype(values)>);
    static_assert(!BinarySearchableRange<std::forward_list<int>>);
    assert(leetcode_binary_search_index(values, -1) == 0);
    assert(leetcode_binary_search_index(values, 9) == 4);
    assert(leetcode_binary_search_index(values, 12) == 5);
    assert(leetcode_binary_search_index(values, 2) == -1);
    assert(leetcode_binary_search_index(std::vector<int>{}, 2) == -1);

    const Gauge gpu{"gpu", 87.5};
    static_assert(Metric<Gauge>);
    static_assert(!Metric<int>);
    assert(practical_render_metric(gpu).starts_with("gpu=87.5"));

    std::cout << practical_render_metric(gpu) << '\n';
}

/*
 * 【陷阱】requires 中 `metric.name` 若只檢查存在，private/access 或轉型需求仍要明列。
 * 【陷阱】過度限制會拒絕原本合法的 proxy/view；應限制演算法真正需要的最小契約。
 * 【面試】concept 是型別嗎？不是；它是可被布林求值的編譯期 predicate。
 * 【練習】改用 std::ranges::random_access_range 與 std::totally_ordered 約束二分搜尋。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '20_concepts_intro.cpp' -o '/tmp/codex_cpp_C_Template_20_concepts_intro' && '/tmp/codex_cpp_C_Template_20_concepts_intro'
//
// === 預期輸出（節錄）===
// gpu=87.500000
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
