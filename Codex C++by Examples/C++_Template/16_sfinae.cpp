/*
 * 第 16 章：SFINAE
 *
 * Substitution Failure Is Not An Error：模板引數代換若在「立即語境」失敗，該候選
 * 從 overload set 移除，而不是整個編譯終止。它是 C++20 concepts 以前的核心限制工具。
 * 但函式本體內才出錯通常不屬 SFINAE，因此限制要放在宣告可見的位置。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. SFINAE 只把「模板引數代換期間、立即語境內」形成失敗的候選移出 overload set。
 * 2. 它不是 try/catch：語法解析錯誤、非相依名稱錯誤、函式本體實體化失敗仍是硬錯誤。
 * 3. detection idiom 以主模板表示 false，再讓 void_t<合法 expression> 選到偏特化 true。
 * 4. declval<const T&>() 只在 unevaluated context 模擬運算式，不建構物件也沒有生命週期。
 * 5. 本例不只檢查 size() 存在，也檢查結果可轉為 size_t，讓宣告限制與本體一致。
 * 6. enable_if_t<條件, R> 可放回傳型別；建構子沒有回傳型別時常改放模板參數或 requires。
 * 7. 兩個 overload 的條件應互斥，否則同時成立會 ambiguous、同時不成立會 no matching function。
 * 8. C++20 新 API 優先 concept，因契約在簽名可見且診斷較短；維護 C++17 程式仍需 SFINAE。
 * 9. measured_size 對有 size() 型別是呼叫本身的成本，常見容器為 O(1)，但泛型契約不保證。
 * 10. 無 size() fallback 固定回 1 是本教材政策，不代表所有 scalar 或 range 的通用語意。
 * 11. Jewels and Stones 的雙迴圈時間 O(|J|*|S|)、額外空間 O(1)，適合小型通用序列。
 * 12. 若字元集固定可用 bitset；若元素可 hash 可用 unordered_set，把平均成本降為 O(J+S)。
 * 13. LeetCode 契約要求兩個 range 可迭代，且 stone==jewel 可轉成 bool；空 range 合法並回 0。
 * 14. range 與元素都只以 const reference 借用，函式不保存 iterator，呼叫結束後沒有懸空引用。
 * 15. practical_encode 的字串 overload 用 forwarding reference，但立即建立 owning string 回傳。
 * 16. 因此傳入暫時 std::string 也安全；forward 只避免不必要複製，不會把 reference 帶出函式。
 * 17. arithmetic trait 也包含 bool 與 char；正式序列化協定通常要另訂格式，不能盲信 to_string。
 * 18. 每個成功型別仍會產生函式 specialization；SFINAE 不會自動降低 code size。
 * 19. 多層 void_t/enable_if 會增加 template instantiation 與 compile time，錯誤回溯也較長。
 * 20. 診斷技巧：先把條件獨立成 trait 並 static_assert，再檢查錯誤是否真的位於立即語境。
 * 21. 面試追問：為何本體內 `value.size()` 出錯不算 SFINAE？候選早已完成代換並被選中。
 * 22. 面試追問：void_t 為何回 void 仍有用？價值在形成模板引數前是否能成功代換 expression。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
using SizeAsSizeTExpression =
    decltype(static_cast<std::size_t>(std::declval<const T&>().size()));

template <typename T, typename = void>
struct HasSize : std::false_type {};

template <typename T>
struct HasSize<T, std::void_t<SizeAsSizeTExpression<T>>> : std::true_type {};

template <typename T>
std::enable_if_t<HasSize<T>::value, std::size_t> measured_size(const T& value) {
    return static_cast<std::size_t>(value.size());
}

template <typename T>
std::enable_if_t<!HasSize<T>::value, std::size_t> measured_size(const T&) {
    return 1U;
}

template <typename Range>
using ConstBegin = decltype(std::declval<const Range&>().begin());

template <typename Range>
using ConstEnd = decltype(std::declval<const Range&>().end());

// LeetCode 771：Jewels and Stones。以 SFINAE 檢查 range 與元素比較所需語法。
template <typename Jewels, typename Stones,
          typename = std::void_t<ConstBegin<Jewels>, ConstEnd<Jewels>,
                                 ConstBegin<Stones>, ConstEnd<Stones>,
                                 decltype(++std::declval<ConstBegin<Jewels>&>()),
                                 decltype(++std::declval<ConstBegin<Stones>&>()),
                                 decltype(static_cast<bool>(
                                     std::declval<ConstBegin<Jewels>&>() !=
                                     std::declval<ConstEnd<Jewels>&>())),
                                 decltype(static_cast<bool>(
                                     std::declval<ConstBegin<Stones>&>() !=
                                     std::declval<ConstEnd<Stones>&>())),
                                 decltype(static_cast<bool>(
                                     *std::declval<ConstBegin<Stones>&>() ==
                                     *std::declval<ConstBegin<Jewels>&>()))>>
int leetcode_count_jewels(const Jewels& jewels, const Stones& stones) {
    int count = 0;
    for (const auto stone : stones) {
        for (const auto jewel : jewels) {
            if (stone == jewel) {
                ++count;
                break;
            }
        }
    }
    return count;
}

// 實務：依型別類別選序列化 overload；失敗型別不會誤走數值版。
template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, std::string> practical_encode(const T& value) {
    return std::to_string(value);
}

template <typename T>
std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>, std::string>
practical_encode(T&& value) {
    return '"' + std::forward<T>(value) + '"';
}

struct VoidSized {
    void size() const {}
};

int main() {
    static_assert(HasSize<std::vector<int>>::value);
    static_assert(!HasSize<int>::value);
    static_assert(!HasSize<VoidSized>::value);
    assert(measured_size(std::string{"cuda"}) == 4U);
    assert(measured_size(std::vector<int>{}) == 0U);
    assert(measured_size(42) == 1U);

    assert(leetcode_count_jewels(std::string{"aA"}, std::string{"aAAbbbb"}) == 3);
    assert(leetcode_count_jewels(std::vector<char>{'z'}, std::vector<char>{'Z', 'z'}) == 1);
    assert(leetcode_count_jewels(std::string{}, std::string{"abc"}) == 0);
    assert(leetcode_count_jewels(std::string{"a"}, std::string{}) == 0);

    assert(practical_encode(42) == "42");
    assert(practical_encode(std::string{"gpu"}) == "\"gpu\"");

    std::cout << "SFINAE 測試完成\n";
}

/*
 * 【陷阱】兩個 overload 若只有預設 template argument 不同，可能被視為同一宣告而重定義。
 * 【陷阱】SFINAE 診斷常冗長；新程式優先 concepts，但讀舊函式庫仍必須懂。
 * 【面試】void_t 為何有效？只要內部 expressions 都可形成型別，它就是 void；否則代換失敗。
 * 【練習】建立 HasReserve trait，只有容器支援 reserve 時才預先配置。
 */
