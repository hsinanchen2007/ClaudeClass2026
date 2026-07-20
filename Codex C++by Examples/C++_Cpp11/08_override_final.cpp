/*
 * C++11 教科書：override 與 final
 *
 * virtual 提供 runtime polymorphism。衍生類函式加 override 後，compiler 必須確認它真的
 * 覆寫 base virtual function；const、reference qualifier 或參數寫錯都會立即報錯。
 * final 可放在 virtual member（不可再覆寫）或 class（不可再繼承）。
 *
 * 【必要規則】透過 Base* 刪除 Derived 時，Base destructor 必須 virtual；否則是 UB。
 * 【設計】繼承代表 is-a 與可替換性，不要只為了重用幾行 code 濫用。多數實務先考慮
 * composition、variant 或 template；需要 runtime plugin interface 時才用 virtual。
 * 【成本】virtual dispatch 通常一次間接呼叫，並可能妨礙 inline；先量測再優化。
 * 【面試題】overload、override、name hiding 有何不同？
 */

#include <cassert>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace basic {
class Formatter {
public:
    virtual ~Formatter() = default;
    virtual std::string format(int value) const = 0;
};

class DecimalFormatter final : public Formatter {
public:
    std::string format(int value) const override {
        return std::to_string(value);
    }
};

void demo() {
    std::unique_ptr<Formatter> formatter(new DecimalFormatter());
    assert(formatter->format(42) == "42");
}
}  // namespace basic

namespace leetcode {
// LeetCode 20：Valid Parentheses。stack 解法時間 O(n)、空間 O(n)。
class Validator {
public:
    virtual ~Validator() = default;
    virtual bool valid(const std::string& text) const = 0;
};

class BracketValidator final : public Validator {
public:
    bool valid(const std::string& text) const override {
        std::stack<char> opens;
        for (const char ch : text) {
            if (ch == '(' || ch == '[' || ch == '{') {
                opens.push(ch);
            } else {
                if (opens.empty() || !matches(opens.top(), ch)) {
                    return false;
                }
                opens.pop();
            }
        }
        return opens.empty();
    }

private:
    static bool matches(char open, char close) {
        return (open == '(' && close == ')') || (open == '[' && close == ']') ||
               (open == '{' && close == '}');
    }
};

void test() {
    const BracketValidator validator;
    assert(validator.valid("()[]{}"));
    assert(!validator.valid("([)]"));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
class PriceRule {
public:
    virtual ~PriceRule() = default;
    virtual int apply(int cents) const = 0;
};

class TenPercentDiscount final : public PriceRule {
public:
    int apply(int cents) const override { return cents * 90 / 100; }
};

int checkout(const std::vector<int>& prices, const PriceRule& rule) {
    int total = 0;
    for (const int price : prices) {
        total += rule.apply(price);
    }
    return total;
}

void test() {
    const TenPercentDiscount rule;
    assert(checkout({1000, 500}, rule) == 1350);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "override/final：介面、括號驗證、定價規則測試通過\n";
}
