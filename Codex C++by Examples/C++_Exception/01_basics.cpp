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

// LeetCode 150：Evaluate Reverse Polish Notation。
// 題目保證合法；production 版仍檢查 stack/除零/未知 token，讓 contract failure 可診斷。
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

// 實務：在 application boundary 才 catch，轉成明確 exit/result；library 內保留型別資訊。
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
