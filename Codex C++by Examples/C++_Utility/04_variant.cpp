/*
 * 第 04 章：std::variant
 *
 * variant<Ts...> 是 type-safe union：任一時間恰好保存其中一種 alternative，並知道目前 index。
 * get<T>/get<I> 取錯會丟 bad_variant_access；get_if 則回 pointer/null。std::visit 是主要工具，
 * 編譯器會要求 visitor 對所有可能 alternative 都可呼叫。
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

template <typename... Functions>
struct Overloaded : Functions... {
    using Functions::operator()...;
};
template <typename... Functions>
Overloaded(Functions...) -> Overloaded<Functions...>;

using Token = std::variant<int, char>;

// LeetCode 150：Evaluate Reverse Polish Notation。
// int 代表數字，char 代表 + - * /；時間 O(n)、stack 空間 O(n)。
int leetcode_eval_rpn(const std::vector<Token>& tokens) {
    std::vector<int> stack;
    for (const Token& token : tokens) {
        std::visit(Overloaded{
            [&](int value) { stack.push_back(value); },
            [&](char operation) {
                assert(stack.size() >= 2U);
                const int right = stack.back();
                stack.pop_back();
                const int left = stack.back();
                stack.pop_back();
                switch (operation) {
                    case '+': stack.push_back(left + right); break;
                    case '-': stack.push_back(left - right); break;
                    case '*': stack.push_back(left * right); break;
                    case '/':
                        if (right == 0) {
                            throw std::domain_error("RPN division by zero");
                        }
                        stack.push_back(left / right);
                        break;
                    default: throw std::invalid_argument("unknown RPN operator");
                }
            }
        }, token);
    }
    assert(stack.size() == 1U);
    return stack.back();
}

// 【實務情境】通知型別集合封閉，variant 讓 routing 必須窮舉 Email/Sms/Push。
struct Email { std::string address; std::string body; };
struct Sms { std::string number; std::string body; };
struct Push { int user_id{}; std::string body; };
using Notification = std::variant<Email, Sms, Push>;

std::string practical_route_notification(const Notification& notification) {
    return std::visit(Overloaded{
        [](const Email& value) { return "email:" + value.address; },
        [](const Sms& value) { return "sms:" + value.number; },
        [](const Push& value) { return "push:" + std::to_string(value.user_id); }
    }, notification);
}

int main() {
    Token token = 42;
    assert(std::holds_alternative<int>(token));
    assert(std::get<int>(token) == 42);
    token = '+';
    assert(std::get_if<char>(&token) != nullptr);

    assert(leetcode_eval_rpn({2, 1, '+', 3, '*'}) == 9);
    assert(leetcode_eval_rpn({4, 13, 5, '/', '+'}) == 6);

    const Notification message = Email{"ada@example.com", "build complete"};
    assert(practical_route_notification(message) == "email:ada@example.com");
    assert(practical_route_notification(Push{7, "alert"}) == "push:7");

    std::cout << "variant 測試完成\n";
}

/*
 * 【常見陷阱】
 * - variant 的第一個 alternative 必須 default-constructible 才能預設建構；可把 monostate 放第一。
 * - 型別重複時 get<T> 不可用，改用 get<I>。
 * - emplace/assignment 若建構丟例外，極少數情況 variant 可能 valueless_by_exception。
 * - visitor 捕捉 reference 時仍受外部生命週期限制。
 * - 題目雖承諾合法 RPN，production parser 仍應防 division by zero 與 stack underflow。
 *
 * 【面試段落】variant 與 virtual hierarchy？variant 適合封閉型別集合且 exhaustive visit；
 * virtual 適合開放擴充，但操作分散在各 derived class。
 * 【練習】為 Notification visitor 回傳實際 payload 長度，確保三種型別都有測試。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_variant.cpp' -o '/tmp/codex_cpp_C_Utility_04_variant' && '/tmp/codex_cpp_C_Utility_04_variant'
//
// === 預期輸出（節錄）===
// variant 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
