/*
 * 第 17 章：type traits
 *
 * type traits 在編譯期回答型別問題或轉換型別：is_integral_v<T> 是布林常數，
 * remove_cvref_t<T> 產生新型別。它們支撐 generic code、SFINAE、concepts 與最佳化分支。
 * 注意 trait 描述「型別性質」，不會在執行期檢查某個物件內容。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. Predicate trait 如 is_integral<T> 繼承 integral_constant，提供 value、value_type 與轉型。
 * 2. Transformation trait 如 remove_cvref<T> 以巢狀 type 回傳型別；`_t` 是 typename ...::type 捷徑。
 * 3. `_v` 與 `_t` 都只是 API 便利形式，不會在 runtime 查 RTTI，也不會檢查物件內容。
 * 4. 泛型分類前通常先 remove_cvref_t，否則 const int& 並不滿足 is_integral_v<const int&>。
 * 5. remove_cvref 保留 array/function 本體；decay 另會把 array 轉 pointer、function 轉函式指標。
 * 6. 選型時優先標準 trait；自訂 trait 應描述穩定的型別事實，不應偷讀 runtime 狀態。
 * 7. 若目的是限制公開 API，C++20 concept 診斷較佳；若要計算新型別，trait 仍是核心工具。
 * 8. category_of 的判斷全在編譯期，但回傳 string_view 只是借用字面值，無配置且為 O(1)。
 * 9. 每個 T 仍會實體化函式；if constexpr 可刪未選分支，卻不保證不同 T 會合併機器碼。
 * 10. 大量巢狀 trait 會增加 compile time；以命名好的中間 alias 可改善診斷與重複運算。
 * 11. LeetCode palindrome 採半開區間 [left,right)，避免空字串時 right-1 先下溢。
 * 12. 每個字元最多被兩端掃過，時間 O(n)、額外空間 O(1)，不建立正規化字串副本。
 * 13. cctype 對負 signed char 的呼叫未定義；轉 unsigned char 後再傳 isalnum/tolower 才安全。
 * 14. LeetCode 此題按 ASCII 英數解讀；cctype 會受目前 C locale 影響，不等同 Unicode 分詞。
 * 15. string_view 不擁有字元；本函式同步讀完不保存 view，但呼叫端仍須保證呼叫期間有效。
 * 16. practical_metric_value 借用 const reference 並回傳 owning string，因此結果不依賴輸入壽命。
 * 17. bool 在標準中確實是 integral，若先判 arithmetic 就會被 to_string 格式化成 0/1。
 * 18. 診斷常見錯誤是忘記正規化 T，或把 is_base_of 誤當成「可公開隱式轉型」。
 * 19. Trait 只能證明編譯期可觀察性質；例如 is_copy_constructible 不保證複製不丟例外。
 * 20. 面試追問：remove_reference 與 remove_cvref 差異？前者留下被引用型別本身的 cv。
 * 21. 面試追問：何時用 decay？模擬按值傳參或儲存值；保留 array 長度時不可用。
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

template <typename T>
constexpr bool is_text_v = std::is_same_v<std::remove_cvref_t<T>, std::string> ||
                           std::is_same_v<std::remove_cvref_t<T>, std::string_view>;

template <typename T>
std::string_view category_of() {
    using Raw = std::remove_cvref_t<T>;
    if constexpr (std::is_integral_v<Raw>) {
        return "integral";
    } else if constexpr (std::is_floating_point_v<Raw>) {
        return "floating";
    } else if constexpr (is_text_v<Raw>) {
        return "text";
    } else {
        return "other";
    }
}

// LeetCode 125：Valid Palindrome。以 traits 接受 string/string_view，忽略非英數與大小寫。
template <typename Text>
bool leetcode_is_palindrome(const Text& text) {
    static_assert(is_text_v<Text>);
    std::size_t left = 0;
    std::size_t right = text.size();
    while (left < right) {
        while (left < right && !std::isalnum(static_cast<unsigned char>(text[left]))) {
            ++left;
        }
        while (left < right && !std::isalnum(static_cast<unsigned char>(text[right - 1U]))) {
            --right;
        }
        if (left < right) {
            const int a = std::tolower(static_cast<unsigned char>(text[left]));
            const int b = std::tolower(static_cast<unsigned char>(text[right - 1U]));
            if (a != b) {
                return false;
            }
            ++left;
            --right;
        }
    }
    return true;
}

// 實務：避免 bool 被當作一般整數指標輸出；bool 在型別系統中確實是 integral。
template <typename T>
std::string practical_metric_value(const T& value) {
    using Raw = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Raw, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<Raw>) {
        return std::to_string(value);
    } else {
        return "unsupported";
    }
}

int main() {
    using RawArray = int[3];
    static_assert(std::is_same_v<std::remove_cvref_t<const int&>, int>);
    static_assert(std::is_same_v<std::remove_cvref_t<RawArray&>, RawArray>);
    static_assert(std::is_same_v<std::decay_t<RawArray&>, int*>);
    static_assert(is_text_v<const std::string&>);
    assert(category_of<int>() == "integral");
    assert(category_of<const int&>() == "integral");
    assert(category_of<double>() == "floating");
    assert(category_of<const std::string&>() == "text");

    assert(leetcode_is_palindrome(std::string{"A man, a plan, a canal: Panama"}));
    assert(!leetcode_is_palindrome(std::string_view{"race a car"}));
    assert(leetcode_is_palindrome(std::string_view{}));
    assert(leetcode_is_palindrome(std::string_view{".,"}));

    assert(practical_metric_value(true) == "true");
    assert(practical_metric_value(42) == "42");

    std::cout << "type traits 測試完成\n";
}

/*
 * 【常用速查】decay_t 模擬傳值參數轉換；remove_cvref_t 只移除 cv/ref，不把 array 變 pointer。
 * 【陷阱】is_base_of 可對 private/ambiguous base 為 true，不代表可隱式轉型；另看 is_convertible。
 * 【面試】trait 的結果何時決定？模板實體化時的編譯期；通常可被 if constexpr 消除分支。
 * 【練習】以 is_nothrow_move_constructible_v 決定容器 relocation 策略。
 */
