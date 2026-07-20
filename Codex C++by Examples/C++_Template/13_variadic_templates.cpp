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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列的動態和）
// 題目：將 [1,2,3,4] 轉為各位置前綴和 [1,3,6,10]。
// 為何使用本章主題：Integers... 接收零到多個異質但可轉 int 的值，sizeof... 預留空間並展開
// 每次累加；原題輸入 vector<int>，這是把短測資改成 parameter pack 的教學版。
// 思路：建立 result 並 reserve pack 大小；依左至右把每個值加到 total；立即追加目前 total。
// 複雜度：時間 O(N)、結果空間 O(N)，N 是 pack 參數數量；空 pack 回空 vector。
// 易錯點：所有值必須可轉 int；窄化與累加可能溢位，逗號 fold 的求值順序不可任意改寫。
// -----------------------------------------------------------------------------
template <typename... Integers>
std::vector<int> leetcode_running_sum_pack(Integers... values) {
    static_assert((std::is_convertible_v<Integers, int> && ...));
    std::vector<int> result;
    result.reserve(sizeof...(Integers));
    int total = 0;
    ((total += static_cast<int>(values), result.push_back(total)), ...);
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】異質欄位稽核紀錄
// 情境：部署稽核要把 action、service、版本與成功旗標等不同型別組成單行紀錄。
// 為何使用本章主題：Fields... 讓 factory 接受任意欄位數與可串流型別，並把 pack 轉交 join_fields；
// 相較固定 overload 組合，不需為每種欄位數量重複實作。
// 設計：join_fields 依序輸出逗號分隔值；factory 在前面加 action 與冒號；回傳 owning AuditEvent。
// 成本：時間與空間 O(L)，L 是完整輸出長度，ostringstream 轉型與配置是主要成本。
// 上線注意：目前未 escaping 分隔符或敏感值，bool 也輸出 0/1；正式 audit 應用結構化 schema。
// -----------------------------------------------------------------------------
struct AuditEvent {
    std::string line;
};

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
