/*
 * C++17 教科書：if constexpr
 *
 * 在 template 中，if constexpr 的 condition 必須可在 compile time 決定。未選分支會被
 * discarded，不需對該 specialization 形成有效程式；這讓一個模板可針對型別能力分流。
 * 普通 `if` 的兩個 branch 都必須編譯合法，不能拿來取代 SFINAE/tag dispatch。
 *
 * 【限制】discarded branch 仍必須能被 parser 理解；完全不依賴 template 的硬語法錯誤
 * 不一定被隱藏。branch 外的 return/type requirement 仍照常檢查。
 * 【選擇】少量局部分流用 if constexpr；公開約束在 C++20 通常用 concepts 更清楚。
 * 【陷阱】把 runtime flag 寫進 condition 會編譯失敗，因它不是 constant expression。
 * 【面試題】if constexpr 是否有 runtime branch cost？選定 specialization 後沒有該分支。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
template <class T>
std::string describe(const T& value) {
    if constexpr (std::is_integral<T>::value) {
        return "integer:" + std::to_string(value);
    } else {
        return "other";
    }
}

void demo() {
    assert(describe(7) == "integer:7");
    assert(describe(std::string("x")) == "other");
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 136. Single Number（只出現一次的數字）
// 題目：除一個值出現一次外，其餘值都恰出現兩次，找出單一值；[4,1,2,1,2] 回傳 4。
// 為何使用本章主題：if constexpr 只為 integral specialization 保留 XOR 分支；static_assert 同時把題目型別契約寫清楚。
// 思路：1. answer 從零開始；2. 逐值 XOR；3. 成對值互相抵消後留下唯一值。
// 複雜度：N 為元素數；時間 O(N)、額外空間 O(1)。
// 易錯點：解法嚴格依賴其餘值成對；if constexpr 在 static_assert 後略顯冗餘，是型別分流的教學示範。
// -----------------------------------------------------------------------------
template <class T>
T leetcode_single_number(const std::vector<T>& nums) {
    static_assert(std::is_integral<T>::value, "Single Number 的 XOR 版本需要 integral type");
    T answer{};
    if constexpr (std::is_integral<T>::value) {
        for (const T value : nums) answer ^= value;
    }
    return answer;
}

void leetcode_test() {
    assert(leetcode_single_number(std::vector<int>{4, 1, 2, 1, 2}) == 4);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】型別導向的簡化值編碼器
// 情境：遙測欄位只允許 std::string 或 arithmetic type，要統一轉成文字表示。
// 為何使用本章主題：if constexpr 在編譯期選擇字串加引號或 std::to_string，未選分支不必對該 T 合法。
// 設計：1. string 分支包雙引號；2. arithmetic 分支呼叫 to_string；3. 其他型別以 static_assert 拒絕。
// 成本：數值轉字串依位數 D 為 O(D)，字串分支 O(L) 並配置 O(L) 結果空間。
// 上線注意：此教學版本未 escape 引號、反斜線或控制字元，不能直接視為完整 JSON encoder。
// -----------------------------------------------------------------------------
template <class T>
std::string practical_encode(const T& value) {
    if constexpr (std::is_same<T, std::string>::value) {
        return "\"" + value + "\"";
    } else if constexpr (std::is_arithmetic<T>::value) {
        return std::to_string(value);
    } else {
        static_assert(std::is_arithmetic<T>::value,
                      "practical_encode 只支援 string 與 arithmetic types");
    }
}

void practical_test() {
    assert(practical_encode(std::string("ok")) == "\"ok\"");
    assert(practical_encode(42) == "42");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "if constexpr：型別分流、Single Number、encode 測試通過\n";
}

// 【延伸練習】以 dependent_false 改善 unsupported type 診斷，並比較 C++20 requires。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_if_constexpr.cpp' -o '/tmp/codex_cpp_C_Cpp17_04_if_constexpr' && '/tmp/codex_cpp_C_Cpp17_04_if_constexpr'
//
// === 預期輸出（節錄）===
// if constexpr：型別分流、Single Number、encode 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
