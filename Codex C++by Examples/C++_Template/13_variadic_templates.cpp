/*
 * 第 13 章：可變參數模板（variadic templates）
 *
 * `typename... Ts` 是 type parameter pack，`Ts...` 展開成零到多個型別。
 * 函式參數也可寫 `Ts&&... args`。C++17 前常以遞迴拆 pack；現在多數聚合運算
 * 可用 fold expression（下一章）。本章先看 sizeof...、展開與異質參數。
 */

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

template <typename... Ts>
constexpr std::size_t argument_count(const Ts&...) noexcept {
    return sizeof...(Ts);
}

template <typename... Ts>
std::string join_fields(const Ts&... values) {
    std::ostringstream output;
    std::size_t index = 0;
    // initializer_list 讓每個展開項依左至右執行；C++17 fold 也可做到。
    ((output << (index++ == 0U ? "" : ",") << values), ...);
    return output.str();
}

// LeetCode 1480 的 pack 版本：輸入編譯期常數，產生 running sum vector。
template <typename... Integers>
std::vector<int> leetcode_running_sum_pack(Integers... values) {
    static_assert((std::is_convertible_v<Integers, int> && ...));
    std::vector<int> result;
    result.reserve(sizeof...(Integers));
    int total = 0;
    ((total += static_cast<int>(values), result.push_back(total)), ...);
    return result;
}

struct AuditEvent {
    std::string line;
};

// 實務：不同欄位型別直接串成稽核紀錄；正式系統還應處理 escaping 與結構化格式。
template <typename... Fields>
AuditEvent practical_make_audit_event(const std::string& action, const Fields&... fields) {
    return {action + ":" + join_fields(fields...)};
}

int main() {
    static_assert(argument_count(1, 2.0, "three") == 3U);
    static_assert(argument_count() == 0U);
    assert(join_fields("gpu", 75, 8.5) == "gpu,75,8.5");

    const std::vector<int> sums = leetcode_running_sum_pack(1, 2, 3, 4);
    assert((sums == std::vector<int>{1, 3, 6, 10}));

    const AuditEvent event = practical_make_audit_event("deploy", "search", 42, true);
    assert(event.line == "deploy:search,42,1");

    std::cout << event.line << '\n';
}

/*
 * 【術語】pack expansion 必須出現在允許展開的 pattern 中；`f(args...)` 是常見形式。
 * 【陷阱】空 pack 對某些 unary fold 不合法；binary fold 可提供 identity 初值。
 * 【陷阱】異質 logging 若依賴 operator<<，遇到不支援的型別會在編譯期失敗。
 * 【面試】sizeof...(Ts) 是編譯期常數，不等於物件所占 byte 數。
 * 【練習】以 index_sequence 將 tuple 每個欄位輸出；比較與 std::apply 的差異。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '13_variadic_templates.cpp' -o '/tmp/codex_cpp_C_Template_13_variadic_templates' && '/tmp/codex_cpp_C_Template_13_variadic_templates'
//
// === 預期輸出（節錄）===
// deploy:search,42,1
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
