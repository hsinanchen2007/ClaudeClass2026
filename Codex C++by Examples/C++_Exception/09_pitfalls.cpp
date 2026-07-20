// ============================================================================
// 課題 9：Exception pitfalls 與 anti-patterns
// ============================================================================
//
//   * destructor 丟 exception：unwinding 中再丟會 terminate；destructor noexcept cleanup。
//   * catch by value：slicing/copy；catch const reference。
//   * `throw e` rethrow：可能 slicing；用 `throw;`。
//   * catch(...) 後忽略：把失敗變成 silent corruption。
//   * exception 當 loop/lookup 正常控制流：API/效能難懂；用 optional/bool/iterator。
//   * signed overflow/null dereference 等 UB 不會自動變 exception，必須事前檢查。
//   * 跨 C ABI boundary 讓 C++ exception 逃出通常不安全，boundary catch 並轉 error code。
// ============================================================================

#include <cassert>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::optional<int> find_value(const std::unordered_map<std::string, int>& values,
                              const std::string& key)
{
    const auto found = values.find(key);
    if (found == values.end()) return std::nullopt;
    return found->second;
}

void basic_example()
{
    const std::unordered_map<std::string, int> values{{"threads", 8}};
    assert(find_value(values, "threads").value() == 8);
    assert(!find_value(values, "missing").has_value());
    std::cout << "[基礎] expected lookup miss uses optional, not throw/catch\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：找出兩個不同 index，使 nums[i]+nums[j]=target；例如 [2,7,11,15] 與 9 回 (0,1)，原題保證恰有一解。
// 為何使用本章主題：通用版本不採原題保證，找不到時回 optional empty，避免用 exception 表示正常搜尋結果。
// 思路：1. 從左到右掃描。2. 在 hash map 尋找 target-current。3. 命中就回舊 index 與 current。4. 否則記錄 current。
// 複雜度：N 個元素平均時間 O(N)、額外空間 O(N)；hash table 最壞情況可能退化。
// 易錯點：要先查 complement 再插入以避免同元素重用；target-current 可能 signed overflow，production 版需先做 checked arithmetic。
// -----------------------------------------------------------------------------
std::optional<std::pair<int, int>> two_sum(const std::vector<int>& nums, int target)
{
    if (nums.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("too many values for int indices");
    }
    std::unordered_map<int, int> index;
    for (std::size_t current = 0U; current < nums.size(); ++current) {
        const auto found = index.find(target - nums.at(current));
        if (found != index.end()) return std::pair<int, int>{found->second, static_cast<int>(current)};
        index[nums.at(current)] = static_cast<int>(current);
    }
    return std::nullopt;
}

void leetcode_1_example()
{
    assert((two_sum({2, 7, 11, 15}, 9).value() == std::pair<int, int>(0, 1)));
    assert(!two_sum({1, 2, 3}, 100).has_value());
    std::cout << "[LeetCode 1] no solution is optional empty\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】C ABI callback 的例外防火牆
// 情境：C library 呼叫 C++ callback，負輸入要回穩定錯誤碼 -1，任何 C++ exception 都不能穿越語言 ABI。
// 為何使用本章主題：extern "C" noexcept boundary 在內部 catch-all 並轉狀態碼，是 catch(...) 合理的最後防線用途。
// 設計：1. 在 try 內驗證輸入。2. 合法值乘 2 回傳。3. 任一 exception 在邊界捕捉。4. 統一回 -1。
// 成本：正常路徑 O(1)；失敗另有 throw/unwind，實際 logging 若加入則有 I/O 成本。
// 上線注意：value*2 仍須在運算前檢查 overflow，因 UB 無法被 catch；錯誤碼契約也需避免與合法結果衝突並提供診斷通道。
// -----------------------------------------------------------------------------
extern "C" int safe_callback(int value) noexcept
{
    try {
        if (value < 0) throw std::invalid_argument("negative");
        return value * 2;
    } catch (...) {
        return -1;
    }
}

void practical_example()
{
    assert(safe_callback(4) == 8);
    assert(safe_callback(-1) == -1);
    std::cout << "[實務] C ABI boundary translates exception to -1\n";
}

int main()
{
    basic_example();
    leetcode_1_example();
    practical_example();
}

// 易錯與面試：禁止 exception 穿越 C ABI、destructor 或 noexcept callback boundary；先在
// C++ 端 catch-all 並轉成明確 error code。catch(...) 只能在這類最後防線，不能拿來失憶。
// 練習：讓 callback 同時寫入 thread-local error message，但避免 global data race。
// 複雜度：destructor/callback 的錯誤轉譯通常 O(1)，但 logging/cleanup I/O 成本另計。
// 生命週期：destructor 執行時物件已在結束生命；不可讓 callback 保存指向即將消失成員的 pointer。

/*
 * 【教科書補充：noexcept 邊界仍須先排除 UB】
 * - `target-nums[i]` 與 `value*2` 可能 signed overflow；UB 發生後，任何 catch-all 都無法提供恢復保證。
 * - C ABI/noexcept wrapper 應先做 checked arithmetic，再把可預期錯誤轉成狀態碼。
 * - 測試需包含 INT_MIN/INT_MAX，而不只一般正數；邊界值才能驗證運算前檢查真的有效。
 * - `noexcept` 的承諾是例外不得逃出；若內部 throw 未捕捉，結果是 terminate，不是自動錯誤碼。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_pitfalls.cpp' -o '/tmp/codex_cpp_C_Exception_09_pitfalls' && '/tmp/codex_cpp_C_Exception_09_pitfalls'
//
// === 預期輸出（節錄）===
// [實務] C ABI boundary translates exception to -1
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
