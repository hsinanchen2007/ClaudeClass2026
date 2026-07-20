// ============================================================================
// 課題 1：Exception 基礎 - throw、try、catch、stack unwinding
// ============================================================================
//
// throw 建立 exception object，runtime 沿 call stack 尋找相容 catch；途中已完整建構的
// automatic objects 依反序解構（stack unwinding）。找不到 handler 則 std::terminate。
// Exception 適合函式無法履行契約且呼叫端不一定能就地處理的 failure；預期分支（找不到、
// queue full）常用 optional/expected/error code 更清楚。
//
// catch 由具體到一般，通常 `catch(const T&)` 避免 copy/slicing。處理不了就讓它傳上去，
// 不要每層 catch 後只印訊息又回假成功。RAII 保證 unwinding 仍清理資源。
// ============================================================================

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

double divide(double numerator, double denominator)
{
    if (denominator == 0.0) throw std::domain_error("division by zero");
    return numerator / denominator;
}

void basic_example()
{
    assert(divide(6.0, 2.0) == 3.0);
    bool caught = false;
    try {
        (void)divide(1.0, 0.0);
    } catch (const std::domain_error& error) {
        caught = std::string(error.what()) == "division by zero";
    }
    assert(caught);
    std::cout << "[基礎] domain_error caught by const reference\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 150. Evaluate Reverse Polish Notation（計算逆波蘭表示式）
// 題目：輸入合法 token 序列，以整數運算計算後綴表示式；例如 [2,1,+,3,*] 得 9，[4,13,5,/,+] 得 6。
// 為何使用本章主題：原題保證合法；可重用版本仍用 typed exception 回報缺 operand、除以零或 malformed expression，示範違約時的 stack unwinding。
// 思路：1. 數字轉 int 後 push。2. 運算子取出右、左 operand。3. 計算並 push 結果。4. 結尾要求 stack 恰有一值。
// 複雜度：N 個 token 的時間 O(N)、stack 額外空間 O(N)。
// 易錯點：減法與除法的 operand 順序不能反；stoi 與 signed arithmetic 仍需完整格式及 overflow 檢查，catch 不能補救 UB。
// -----------------------------------------------------------------------------
int eval_rpn(const std::vector<std::string>& tokens)
{
    std::vector<int> stack;
    for (const std::string& token : tokens) {
        if (token != "+" && token != "-" && token != "*" && token != "/") {
            stack.push_back(std::stoi(token));
            continue;
        }
        if (stack.size() < 2U) throw std::invalid_argument("missing operands");
        const int right = stack.back(); stack.pop_back();
        const int left = stack.back(); stack.pop_back();
        if (token == "+") stack.push_back(left + right);
        else if (token == "-") stack.push_back(left - right);
        else if (token == "*") stack.push_back(left * right);
        else {
            if (right == 0) throw std::domain_error("division by zero");
            stack.push_back(left / right);
        }
    }
    if (stack.size() != 1U) throw std::invalid_argument("malformed expression");
    return stack.back();
}

void leetcode_150_example()
{
    assert(eval_rpn({"2", "1", "+", "3", "*"}) == 9);
    assert(eval_rpn({"4", "13", "5", "/", "+"}) == 6);
    std::cout << "[LeetCode 150] valid RPN answers 9 and 6\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】計算服務邊界的錯誤轉譯
// 情境：底層 divide 以 domain_error 表示分母為零，但 application 對呼叫端只需要明確的成功或失敗結果。
// 為何使用本章主題：library 保留具體例外型別供中間層傳遞，直到能採取行動的邊界才 catch 並轉成 bool，避免每層吞錯。
// 設計：1. 邊界呼叫 divide。2. 正常完成回 true。3. 只捕捉 domain_error。4. 將該失敗轉成 false。
// 成本：正常路徑是一次除法；失敗路徑另付 throw 與 unwind 成本，取決於堆疊深度。
// 上線注意：bool 會丟失錯誤細節，真實 API 可改 expected/status 並在邊界記錄 context；不可 catch-all 後回假成功。
// -----------------------------------------------------------------------------
bool run_calculation(double denominator)
{
    try {
        (void)divide(10.0, denominator);
        return true;
    } catch (const std::domain_error&) {
        return false;
    }
}

void practical_example()
{
    assert(run_calculation(2.0));
    assert(!run_calculation(0.0));
    std::cout << "[實務] boundary translates exception to failure result\n";
}

int main()
{
    basic_example();
    leetcode_150_example();
    practical_example();
}

// 易錯與面試：exception 只處理已定義程式路徑，不會把 signed overflow、dangling 等 UB
// 變成可 catch 錯誤。catch 應在能採取行動的 boundary，不能每層 catch 後靜默忽略。
// 練習：加入 overflow 檢查；不要讓 signed arithmetic UB 發生後才期待 exception。
// 複雜度：正常路徑的 try 通常低成本；真正 throw/unwind 成本與展開 stack frames、destructors 有關。
// 生命週期：unwinding 會依反向建構順序解構已完成物件；尚未建構完成的 object 不會解構。

/*
 * 【教科書補充：例外抓不到 undefined behavior】
 * - checked parser 要驗 stoi consumed position；"12x" 不應因成功解析前綴 12 就算合法 token。
 * - 有號 +、-、* 溢位與 INT_MIN/-1 除法都是 UB，不會自動變成可 catch 的 exception。
 * - 先用寬型別/邊界公式做 checked arithmetic，再執行運算；catch(...) 不是數值安全措施。
 * - 外部輸入錯誤是預期 control flow 時，也可用 expected/error code，避免每個壞 token 都靠 throw。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_basics.cpp' -o '/tmp/codex_cpp_C_Exception_01_basics' && '/tmp/codex_cpp_C_Exception_01_basics'
//
// === 預期輸出（節錄）===
// [實務] boundary translates exception to failure result
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
