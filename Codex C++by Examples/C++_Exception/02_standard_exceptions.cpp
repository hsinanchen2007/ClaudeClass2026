// ============================================================================
// 課題 2：標準 exception hierarchy 與選擇
// ============================================================================
//
// std::exception 提供 virtual what()。常用分類：
//   logic_error：程式/前置條件問題；invalid_argument、domain_error、out_of_range。
//   runtime_error：執行環境失敗；range_error、overflow_error、underflow_error。
//   bad_alloc、bad_cast、system_error、filesystem_error 等有專門資訊。
//
// 分類不是絕對哲學，重點是 caller 能按型別做合理處理。message 應包含 operation/context，
// 但不要依 what() 字串解析控制流程。容器 `.at()` 丟 out_of_range；operator[] 不檢查。
// ============================================================================

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int checked_at(const std::vector<int>& values, std::size_t index)
{
    return values.at(index);
}

void basic_example()
{
    const std::vector<int> values{10, 20};
    assert(checked_at(values, 1U) == 20);
    bool out_of_range = false;
    try {
        (void)checked_at(values, 2U);
    } catch (const std::out_of_range&) {
        out_of_range = true;
    }
    assert(out_of_range);
    std::cout << "[基礎] vector::at reports out_of_range\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses（有效的括號）
// 題目：輸入只含 ()[]{}，判斷括號種類與順序是否正確；例如 ()[]{} 為 true，(] 與 ([)] 為 false。
// 為何使用本章主題：配對失敗是題目正常答案而回 false；只有 production 版收到契約外字元時才丟 invalid_argument。
// 思路：1. 開括號 push。2. 閉括號先確認 stack 非空。3. pop 並比對種類。4. 掃描後要求 stack 為空。
// 複雜度：字串長度 N 的時間 O(N)、最壞額外空間 O(N)。
// 易錯點：不能用 exception 表示普通 mismatch；閉括號遇空 stack 要立即 false，未知字元與不匹配括號是不同契約狀態。
// -----------------------------------------------------------------------------
bool is_valid(const std::string& text)
{
    std::vector<char> stack;
    for (const char token : text) {
        if (token == '(' || token == '[' || token == '{') stack.push_back(token);
        else if (token == ')' || token == ']' || token == '}') {
            if (stack.empty()) return false;
            const char open = stack.back();
            stack.pop_back();
            if ((token == ')' && open != '(') || (token == ']' && open != '[') ||
                (token == '}' && open != '{')) return false;
        } else {
            throw std::invalid_argument("unsupported character");
        }
    }
    return stack.empty();
}

void leetcode_20_example()
{
    assert(is_valid("()[]{}"));
    assert(!is_valid("(]"));
    assert(!is_valid("([)]"));
    std::cout << "[LeetCode 20] false is normal mismatch; unknown chars throw\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Worker 數量設定驗證
// 情境：服務接受 worker_count 設定，但部署允許範圍只有 1..256；0 或過大值代表呼叫端輸入違約。
// 為何使用本章主題：invalid_argument 精確表達參數不符合前置條件，比籠統 runtime_error 或 sentinel 0 更方便 caller 分類。
// 設計：1. 接收 requested。2. 檢查上下界。3. 違約時丟 invalid_argument。4. 合法時原值回傳。
// 成本：成功判斷時間 O(1)、空間 O(1)；失敗另有建立 exception 與 unwinding 成本。
// 上線注意：訊息可供人讀但控制流程應看型別；若設定來自檔案，還要在較高層附加 key、來源與安全過濾後的值。
// -----------------------------------------------------------------------------
int worker_count(int requested)
{
    if (requested < 1 || requested > 256) throw std::invalid_argument("workers must be 1..256");
    return requested;
}

void practical_example()
{
    assert(worker_count(8) == 8);
    bool rejected = false;
    try { (void)worker_count(0); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::cout << "[實務] invalid configuration has a typed failure\n";
}

int main()
{
    basic_example();
    leetcode_20_example();
    practical_example();
}

// 易錯與面試：依 meaning 選型別，而非所有失敗都 runtime_error。invalid_argument 表示
// caller 輸入違約；out_of_range 表示索引/數值超界；system_error 可攜 error_code。
// 練習：比較 stoi 可能丟的 invalid_argument/out_of_range，分別提供不同錯誤訊息。
// 複雜度：選 exception subclass 不改解題 Big-O；throw 成本仍取決於 stack 深度與 cleanup。
// 生命週期：catch(const exception&) 在 handler 期間引用 exception object，離開 handler 不得保存 reference。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_standard_exceptions.cpp' -o '/tmp/codex_cpp_C_Exception_02_standard_exceptions' && '/tmp/codex_cpp_C_Exception_02_standard_exceptions'
//
// === 預期輸出（節錄）===
// [實務] invalid configuration has a typed failure
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
