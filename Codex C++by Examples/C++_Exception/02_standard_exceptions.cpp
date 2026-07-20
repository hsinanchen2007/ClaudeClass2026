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

// LeetCode 20：Valid Parentheses。題目只有 bracket chars；production 版對未知輸入丟
// invalid_argument，配對不合法則正常回 false（這是預期答案，不是 exceptional failure）。
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

// 實務：設定錯誤用 invalid_argument；I/O failure 則會是 runtime/system error 類。
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
