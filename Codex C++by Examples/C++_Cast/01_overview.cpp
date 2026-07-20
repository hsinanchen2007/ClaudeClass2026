// ============================================================================
// 課題 1：C++ casts 全覽與選擇順序
// ============================================================================
//
// C++ named casts 把意圖寫進語法：
//   static_cast      編譯期已知的數值/繼承/void* 等轉換；不做 runtime type check。
//   dynamic_cast     polymorphic hierarchy 的 runtime checked down/cross cast。
//   const_cast       只改 cv-qualification；若原物件真是 const，修改仍是 UB。
//   reinterpret_cast 低階 pointer/integer/reinterpretation；不保證可安全 dereference。
//   std::bit_cast    C++20，等大小 trivially-copyable object representation copy。
//
// 選擇順序：先問能否不 cast（改 API/type）；必要時用最窄語意的 named cast。不要用
// C-style `(T)x`，因它可能依序嘗試多種 casts，reviewer 看不出真正意圖。
//
// cast 不會自動驗證 range/lifetime/alignment/ownership；「compiler 接受」不是安全證明。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

std::optional<int> size_to_int(std::size_t size)
{
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) return std::nullopt;
    return static_cast<int>(size);
}

void basic_example()
{
    const std::vector<int> values{10, 20, 30};
    assert(size_to_int(values.size()).value() == 3);
    const double average = static_cast<double>(values[0] + values[1] + values[2]) /
                           static_cast<double>(values.size());
    assert(average == 20.0);
    std::cout << "[基礎] checked size cast=3，floating average=20\n";
}

// LeetCode 1929：Concatenation of Array。
// vector index 是 size_t；題目回 vector，不需要把 size 強轉 int。這也是重要觀念：
// 最安全的 cast 往往是讓 loop/index 型別跟 container API 一致，完全不 cast。
std::vector<int> get_concatenation(const std::vector<int>& nums)
{
    std::vector<int> answer(nums.size() * 2U);
    for (std::size_t index = 0U; index < nums.size(); ++index) {
        answer.at(index) = nums.at(index);
        answer.at(index + nums.size()) = nums.at(index);
    }
    return answer;
}

void leetcode_1929_example()
{
    assert((get_concatenation({1, 2, 1}) == std::vector<int>{1, 2, 1, 1, 2, 1}));
    std::cout << "[LeetCode 1929] size_t indexing 不需 cast\n";
}

// 實務：percentage 計算若先做 integer division，之後 cast 已來不及救回精度。
double completion_percentage(std::size_t done, std::size_t total)
{
    if (total == 0U) return 0.0;
    return static_cast<double>(done) * 100.0 / static_cast<double>(total);
}

void practical_example()
{
    assert(completion_percentage(1U, 4U) == 25.0);
    std::cout << "[實務] 1/4 completion=25%（除法前轉 double）\n";
}

int main()
{
    basic_example();
    leetcode_1929_example();
    practical_example();
}

// 易錯與面試：cast 只能改型別解讀/轉換，不能補回已在 integer division 遺失的資訊。
// C-style cast 會依序嘗試多種強力轉型，review 看不出意圖；production 優先具名 cast。
// 練習：解釋 `double result = 1 / 4;` 為何得到 0.0。
// 複雜度：numeric/pointer cast 通常是 O(1)，但 user-defined conversion 可執行任意 constructor。
// 生命週期：cast 不會自動延長來源生命；由 temporary 轉出的 pointer/reference 仍可能懸空。
