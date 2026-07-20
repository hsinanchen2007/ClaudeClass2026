/*
 * C++17 教科書：nested namespace definition
 *
 * C++17 可把 `namespace company { namespace service { ... } }` 寫成
 * `namespace company::service { ... }`。這只是較短語法，不改 name lookup、linkage 或 ABI。
 * namespace 用來組織名稱與 API ownership，不是 access control；內容預設仍可被外部使用。
 *
 * 【版本 API】inline namespace 可讓 v2 名稱同時成為父 namespace 的預設版本；這與本章
 * nested syntax 是不同功能。大量 `using namespace` 尤其在 header 會污染使用者 lookup。
 * 【ADL】function 與 argument type 同 namespace 時可能由 argument-dependent lookup 找到。
 * 【陷阱】namespace 名稱重構會改 mangled symbol，library ABI 可能破壞。
 * 【面試題】anonymous namespace 在 source file 中提供 internal linkage，與 nested 無關。
 *
 * 【實務命名守則】
 * 1. 由穩定到具體排列：company::product::component，而不是依當前資料夾隨意命名。
 * 2. public header 不放 `using namespace`，避免呼叫端的 overload/name lookup 被污染。
 * 3. 不要用 namespace 模擬 Java class；無狀態 free functions 才適合放 namespace。
 * 4. API version 若需要預設版本，研究 inline namespace，而非只把名稱寫成 v2。
 * 5. test helper 可放 production namespace 的子 namespace test，但不要洩漏到正式 header。
 * 【練習】建立 company::billing::v1 舊計價 API，說明為何不能直接把 v2 改名成 v1。
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <string>

namespace textbook::basic {
int add(int left, int right) { return left + right; }
}  // namespace textbook::basic

namespace textbook::leetcode {
// LeetCode 242：Valid Anagram。固定小寫英文，O(n) time / O(1) alphabet space。
bool leetcode_is_anagram(const std::string& first, const std::string& second) {
    if (first.size() != second.size()) return false;
    std::array<int, 26> counts{};
    for (const char ch : first) ++counts[static_cast<std::size_t>(ch - 'a')];
    for (const char ch : second) --counts[static_cast<std::size_t>(ch - 'a')];
    return std::all_of(counts.begin(), counts.end(), [](int count) { return count == 0; });
}

void leetcode_test() {
    assert(leetcode_is_anagram("anagram", "nagaram"));
    assert(!leetcode_is_anagram("rat", "car"));
}
}  // namespace textbook::leetcode

namespace company::billing::v2 {
struct Invoice {
    int cents;
};

int practical_total(const std::array<Invoice, 3>& invoices) {
    int total = 0;
    for (const Invoice& invoice : invoices) total += invoice.cents;
    return total;
}

void practical_test() {
    const std::array<Invoice, 3> invoices{{{100}, {250}, {50}}};
    assert(practical_total(invoices) == 400);
}
}  // namespace company::billing::v2

int main() {
    assert(textbook::basic::add(2, 3) == 5);
    textbook::leetcode::leetcode_test();
    company::billing::v2::practical_test();
    std::cout << "nested namespace：名稱組織、anagram、billing API 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_nested_namespace.cpp' -o '/tmp/codex_cpp_C_Cpp17_07_nested_namespace' && '/tmp/codex_cpp_C_Cpp17_07_nested_namespace'
//
// === 預期輸出（節錄）===
// nested namespace：名稱組織、anagram、billing API 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
