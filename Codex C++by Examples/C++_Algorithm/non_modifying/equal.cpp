/*
 * std::equal：逐元素判斷兩範圍是否相等
 * ====================================
 * 四 iterator overload 能驗兩邊長度；舊的三 iterator overload 只知道第二範圍起點，
 * 呼叫者必須保證至少和第一範圍一樣長。時間 O(min length)，遇第一個不等可短路。
 * predicate 應表達 equivalence relation。演算法唯讀、不保存 iterator。
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 242. Valid Anagram（有效的字母異位詞）
// 題目：判斷 s 與 t 是否由相同字元及相同次數組成；例如 anagram/nagaram 為 true，
// rat/car 為 false。
// 為何使用本章主題：先排序兩份副本，把多重集合相等轉為逐位置相等，再由四 iterator
// equal 同時驗內容與長度；固定字母表 frequency 會是更快的 O(N) 解。
// 思路：1. 長度不同立即 false；2. 各自排序；3. equal 比較完整兩範圍。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 為字串長度，參數副本會被排序。
// 易錯點：不能只比較 unique 字元集合；Unicode anagram 還需 normalization 與字素定義。
// -----------------------------------------------------------------------------
bool leetcode_is_anagram(std::string first, std::string second) {
    if (first.size() != second.size()) {
        return false;
    }
    std::sort(first.begin(), first.end());
    std::sort(second.begin(), second.end());
    return std::equal(first.begin(), first.end(), second.begin(), second.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Protocol Signature 完整比對
// 情境：解析器要確認 payload 是否整段等於 expected signature，長度較長或較短都算
// 不匹配。
// 為何使用本章主題：四 iterator 的 std::equal 可同時處理內容與長度，避免舊三 iterator
// overload 在第二範圍過短時越界。
// 設計：1. 傳入 payload 與 expected 的完整 begin/end；2. 遇第一差異或長度不同即 false。
// 成本：時間 O(min(N,M))、額外空間 O(1)，N/M 為兩字串長度；可在第一差異短路。
// 上線注意：真正檔案 magic 常只比 prefix；秘密值不可使用會短路的 equal，應採 constant-time primitive。
// -----------------------------------------------------------------------------
bool practical_matches_signature(const std::string& payload,
                                 const std::string& expected) {
    return std::equal(payload.begin(), payload.end(), expected.begin(), expected.end());
}

int main() {
    const std::string a = "Hello";
    const std::string b = "hello";
    assert(!std::equal(a.begin(), a.end(), b.begin(), b.end()));
    assert(std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char lhs, char rhs) {
                          return std::tolower(static_cast<unsigned char>(lhs)) ==
                                 std::tolower(static_cast<unsigned char>(rhs));
                      }));

    assert(leetcode_is_anagram("anagram", "nagaram"));
    assert(!leetcode_is_anagram("rat", "car"));
    assert(practical_matches_signature("PNG", "PNG"));
    assert(!practical_matches_signature("PNG-extra", "PNG"));
    assert(!practical_matches_signature("PN", "PNG"));
    std::cout << "equal：LeetCode 242 與實務 signature 比較測試通過\n";
}

/*
 * 易錯陷阱：對 signed char 直接傳 std::tolower 可能未定義，需先轉 unsigned char。
 * case-insensitive Unicode 不是逐 byte tolower，實務要使用 Unicode normalization/case
 * folding library。安全 signature 也不等於密碼學 constant-time compare；std::equal
 * 會在第一個差異短路，可能洩漏 timing。
 *
 * 面試：equal 與 is_permutation 差別？equal 要同位置相等；is_permutation 只要求
 * 多重集合相同。LeetCode anagram 可用 26-count 降到 O(N)，但需字元集約束。
 *
 * 【LeetCode 測試策略】
 * 必測長度不同、同字母不同次數、空字串與完全相同。sort 會改 local copies，不會
 * 改 caller；若參數改成 const reference 就要另建副本或採 frequency table。
 *
 * 【實務 signature 邊界】
 * 檔案 magic 通常只比較 payload prefix，不是整份 payload 等於 magic。本例函式刻意
 * 比整段，是為展示四 iterator equal；真實 parser 應先驗 payload.size>=magic.size，
 * 再比較 prefix。版本、endianness 與 checksum 仍需額外驗證。
 *
 * 練習：實作 practical_has_prefix，使用 equal 且安全處理短 payload。
 * 再加入空 magic、完全相等、payload 較短與較長四組測試。
 * 面試回答時先指出比較的是 sequence equality，不是物件 identity。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'equal.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_equal' && '/tmp/codex_cpp_C_Algorithm_non_modifying_equal'
//
// === 預期輸出（節錄）===
// equal：LeetCode 242 與實務 signature 比較測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
