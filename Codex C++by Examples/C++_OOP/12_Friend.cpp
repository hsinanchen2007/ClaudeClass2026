// ============================================================================
// 課題 12：friend function / friend class
// ============================================================================
//
// friend 宣告讓特定 non-member function 或 class 存取 private/protected。它不是 member、
// 不會被繼承，也不具傳遞性：A 是 B 的 friend、B 是 C 的 friend，不代表 A 是 C 的。
// 常見合理用途是對稱 binary operator、stream output、緊密合作的 serializer/test probe。
//
// friend 會擴大封裝的 trusted boundary，因此應精準授權一個 function，避免為方便把
// 大型 class 整個設 friend。若需求可由 public API 完成，就不必 friend。
//
// 【面試】friend declaration 可出現在 class 的 public/private 任一區，效果相同。
// 【陷阱】friend 不代表 ownership；取得 private pointer 後仍要遵守生命週期。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <stack>
#include <string>
#include <utility>

class Temperature {
public:
    explicit Temperature(double celsius) : celsius_(celsius) {}

    friend bool operator==(const Temperature& left, const Temperature& right)
    {
        return left.celsius_ == right.celsius_;
    }

    friend std::ostream& operator<<(std::ostream& output, const Temperature& value)
    {
        return output << value.celsius_ << " C";
    }

private:
    double celsius_;
};

void basic_example()
{
    const Temperature first(25.0);
    const Temperature second(25.0);
    assert(first == second);
    std::cout << "[基礎] friend operator<<: " << first << '\n';
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack（最小棧）
// 題目：push、pop、top、getMin 都須 O(1)；例如 push -2,0,-3 時最小為 -3，pop 後為 -2。
// 為何使用本章主題：friend probe 只為測試檢查雙 stack 同步，不是解題必要；production public API 仍維持封裝。
// 思路：values_ 保存值；minimums_ 每層保存截至該層最小值；push/pop 同步；get_min 讀第二個 top。
// 複雜度：所有操作時間 O(1)，額外空間 O(N)，N 為 stack 元素數。
// 易錯點：空 stack 不可 pop/top；重複最小值每層都要保存；friend 擴大 trusted boundary 但不授予 ownership。
// -----------------------------------------------------------------------------
class MinStack;

class MinStackProbe {
public:
    static bool invariant_holds(const MinStack& stack);
};

class MinStack {
public:
    void push(int value)
    {
        values_.push(value);
        minimums_.push(minimums_.empty() ? value : std::min(value, minimums_.top()));
    }
    void pop()
    {
        values_.pop();
        minimums_.pop();
    }
    int top() const { return values_.top(); }
    int get_min() const { return minimums_.top(); }

private:
    friend class MinStackProbe;
    std::stack<int> values_;
    std::stack<int> minimums_;
};

bool MinStackProbe::invariant_holds(const MinStack& stack)
{
    return stack.values_.size() == stack.minimums_.size();
}

void leetcode_155_example()
{
    MinStack stack;
    stack.push(-2);
    stack.push(0);
    stack.push(-3);
    assert(stack.get_min() == -3);
    stack.pop();
    assert(stack.top() == 0 && stack.get_min() == -2);
    assert(MinStackProbe::invariant_holds(stack));
    std::cout << "[LeetCode 155] friend test probe 驗證雙 stack invariant\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】API token 的受控 wire serialization
// 情境：domain caller 只需 issue token，但 wire serializer 必須讀 private owner/serial 產生 build-agent#42。
// 為何使用本章主題：friend class 精準授權緊密合作的 serializer，避免為所有呼叫端公開內部 wire 欄位。
// 設計：ApiToken 以 private constructor 建合法 state；issue 建立值；TokenSerializer 直接讀兩欄並組字串。
// 成本：序列化時間與空間 O(O+D)，O/D 為 owner 與 serial 字串長度。
// 上線注意：owner 中的 # 需 escaping 或結構化格式；token 可能敏感，日誌要遮罩，friend 不會延長 token 生命週期。
// -----------------------------------------------------------------------------
class ApiToken {
public:
    static ApiToken issue(std::string owner, unsigned long serial)
    {
        return ApiToken(std::move(owner), serial);
    }

private:
    friend class TokenSerializer;
    ApiToken(std::string owner, unsigned long serial)
        : owner_(std::move(owner)), serial_(serial) {}
    std::string owner_;
    unsigned long serial_;
};

class TokenSerializer {
public:
    static std::string serialize(const ApiToken& token)
    {
        return token.owner_ + "#" + std::to_string(token.serial_);
    }
};

void practical_example()
{
    const ApiToken token = ApiToken::issue("build-agent", 42UL);
    assert(TokenSerializer::serialize(token) == "build-agent#42");
    std::cout << "[實務] friend serializer=build-agent#42\n";
}

int main()
{
    basic_example();
    leetcode_155_example();
    practical_example();
}

// 練習：改成只 friend 一個 free serialize function，比較授權範圍。
// 複雜度：friend 只改 compile-time access，沒有 runtime dispatch；函式成本由其實作決定。
// 生命週期：friend 不獲得 ownership 特權；保存 private member reference 仍受原 object 存活約束。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_Friend.cpp' -o '/tmp/codex_cpp_C_OOP_12_Friend' && '/tmp/codex_cpp_C_OOP_12_Friend'
//
// === 預期輸出（節錄）===
// [實務] friend serializer=build-agent#42
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
