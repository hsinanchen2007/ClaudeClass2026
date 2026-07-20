/*
 * 第 19 章：if constexpr（C++17）
 *
 * if constexpr 的條件在編譯期決定；未選分支在該模板實體中被 discard，因此可放入
 * 只對另一類型合法的程式碼。普通 if 兩邊仍須能編譯。這讓許多 tag dispatch/SFINAE
 * 的實作更直觀，但介面是否可用仍適合用 concept 限制。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. if constexpr 的 condition 必須可在編譯期轉成 bool；每個模板實體只採用其中一條路徑。
 * 2. 未採用分支是 discarded statement，依賴 T 的不合法操作不會在該 specialization 實體化。
 * 3. 普通 if 即使條件永遠為 false，兩個分支仍須對 T 都是合法程式；這是核心差異。
 * 4. 分類前用 remove_cvref_t 可避免 const/reference 讓 is_arithmetic 等 trait 意外失敗。
 * 5. requires-expression 可直接放進 condition；它只做編譯期有效性檢查，不執行 size()。
 * 6. 只檢查 `value.size()` 存在仍不足，本例再要求結果 convertible_to<size_t> 才使用。
 * 7. 若不支援型別應在 API 邊界消失，選 concept/overload；若要在一個實作內選算法，選 if constexpr。
 * 8. 分支代表可擴充的外部策略時，tag dispatch 或 policy class 通常比長串 else-if 好維護。
 * 9. describe 的型別選擇沒有 runtime branch，回傳 string 的配置與格式化才是執行期成本。
 * 10. 每個 T 仍會實體化一份函式；discard 可減少無用機器碼，但型別種類多仍會放大 code size。
 * 11. requires 與各分支也需由編譯器解析及代換，條件層級過深會增加 compile time。
 * 12. LeetCode integer 路徑用 XOR，時間 O(n)、空間 O(1)，依賴「恰一個值一次，其餘恰兩次」。
 * 13. XOR 對有號整數做位元運算合法且不產生算術 overflow；空輸入卻不符合題目契約。
 * 14. 非整數示範用 equality 雙迴圈，時間 O(n^2)、空間 O(1)，並要求 T 可預設建構。
 * 15. 找不到唯一值時回 T{} 可能與真資料混淆；正式 API 應回 optional 或明確錯誤。
 * 16. 函式全程借用 vector 的 const reference，非整數成功時複製一個 T 作為 owning 回傳值。
 * 17. practical_report 要求 temperature 可轉 int、healthy 可轉 bool，再轉成固定輸出格式。
 * 18. 回傳 std::string 自有生命週期，不保存 Health 的成員 reference，也可安全接暫時物件。
 * 19. 診斷技巧：若 discarded branch 仍報錯，檢查錯誤是否與 T 無關而在第一階段解析就成立。
 * 20. 非相依的 `static_assert(false)` 也可能立刻失敗；泛型 fallback 應讓 assertion 相依於 T。
 * 21. 面試追問：if constexpr 是否保證零指令？分派本身通常消失，但被選分支的工作仍存在。
 * 22. 面試追問：它能取代所有 SFINAE 嗎？不能，候選是否參與 overload resolution 仍需 constraint。
 */

#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

template <typename T>
std::string describe(const T& value) {
    using Raw = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Raw, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<Raw>) {
        return std::to_string(value);
    } else if constexpr (requires {
                             { value.size() } -> std::convertible_to<std::size_t>;
                         }) {
        return "size=" + std::to_string(static_cast<std::size_t>(value.size()));
    } else {
        return "unknown";
    }
}

// LeetCode 136：Single Number。整數走 XOR；字串示範以計數找唯一值。
template <typename T>
T leetcode_single_number(const std::vector<T>& values) {
    if constexpr (std::is_integral_v<T>) {
        T answer{};
        for (T value : values) {
            answer ^= value;
        }
        return answer;
    } else {
        for (const T& candidate : values) {
            std::size_t count = 0;
            for (const T& value : values) {
                count += value == candidate ? 1U : 0U;
            }
            if (count == 1U) {
                return candidate;
            }
        }
        return {};
    }
}

struct Health {
    int temperature{};
    bool healthy{};
};

// 實務：同一入口依型別產生可讀報告；requires expression 只檢查成員存在。
template <typename T>
std::string practical_report(const T& value) {
    if constexpr (requires {
                      { value.temperature } -> std::convertible_to<int>;
                      { value.healthy } -> std::convertible_to<bool>;
                  }) {
        return "temp=" + std::to_string(static_cast<int>(value.temperature)) +
               ",healthy=" + describe(static_cast<bool>(value.healthy));
    } else {
        return describe(value);
    }
}

struct VoidSize {
    void size() const {}
};

int main() {
    assert(describe(true) == "true");
    assert(describe(42) == "42");
    assert(describe(std::string{"cuda"}) == "size=4");
    assert(describe(VoidSize{}) == "unknown");

    assert(leetcode_single_number(std::vector<int>{4, 1, 2, 1, 2}) == 4);
    assert(leetcode_single_number(std::vector<int>{-7}) == -7);
    assert(leetcode_single_number(std::vector<std::string>{"a", "b", "a"}) == "b");

    assert(practical_report(Health{72, true}) == "temp=72,healthy=true");
    std::cout << practical_report(Health{72, true}) << '\n';
}

/*
 * 【陷阱】discarded branch 中完全不依賴 T 的語法錯誤仍可能在解析時報錯。
 * 【陷阱】if constexpr 分支過多通常表示抽象邊界不佳，應考慮 overload/policy。
 * 【面試】是否有 runtime branch？條件是常數表達式，未選分支不進該實體，通常沒有。
 * 【練習】讓 describe 支援有 begin/end 的 range，並避免 string 被當一般 range。
 */
