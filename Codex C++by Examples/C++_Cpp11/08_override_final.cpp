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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses（有效括號）
// 題目：判斷只含 ()[]{} 的字串是否依正確種類與順序成對；"()[]{}" 為 true，"([)]" 為 false。
// 為何使用本章主題：題解本身不需要多型；本檔以 Validator 介面與 final 實作示範可替換驗證器的教學架構。
// 思路：1. 左括號壓入 stack；2. 右括號比對 stack 頂端；3. 不符或缺左括號即失敗，最後要求 stack 為空。
// 複雜度：N 為字串長度；時間 O(N)、額外空間 O(N)。
// 易錯點：空 stack 不可取 top；本實作把任何非左括號都當右括號，依賴題目限定輸入字元。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】結帳折扣策略切換
// 情境：結帳流程接收一組分價金額，部署時可在 runtime 注入不同 PriceRule 計價策略。
// 為何使用本章主題：override 讓編譯器核對 apply 簽章，final 固定折扣實作，virtual destructor 保證經介面安全析構。
// 設計：1. 定義 PriceRule 契約；2. 十趴折扣回傳 cents*90/100；3. checkout 逐價套規則並累加。
// 成本：N 為價格筆數；時間 O(N) 且每筆一次 virtual dispatch，額外空間 O(1)。
// 上線注意：整數除法會捨去小數分且總額可能溢位；策略物件須活過 checkout，金額也必須拒絕負值。
// -----------------------------------------------------------------------------
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

// 【延伸練習】故意漏掉 const 觀察 override 診斷，再以 composition 重寫 PriceRule 比較取捨。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_override_final.cpp' -o '/tmp/codex_cpp_C_Cpp11_08_override_final' && '/tmp/codex_cpp_C_Cpp11_08_override_final'
//
// === 預期輸出（節錄）===
// override/final：介面、括號驗證、定價規則測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
