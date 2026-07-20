// ============================================================================
// 課題 4：Catch by const reference、slicing 與正確 rethrow
// ============================================================================
//
// `catch(std::exception e)` 會 copy/slice derived exception，失去自訂欄位與 dynamic what()；
// 使用 `catch(const std::exception& e)`。handler 由最具體到最一般，否則 base handler 先
// 攔截，derived handler 永遠到不了。
//
// 在 catch 中要保留原 exception/dynamic type，寫 `throw;`；`throw e;` 會丟一個新的
// static-type copy，可能 slicing，也重設 stack context。要加上下文可 nested_exception
//（第 8 課）或自訂 error chaining。
// ============================================================================

#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

class ConfigError : public std::runtime_error {
public:
    ConfigError(std::string key, std::string message)
        : std::runtime_error(std::move(message)), key_(std::move(key)) {}
    const std::string& key() const noexcept { return key_; }
private:
    std::string key_;
};

void load_config() { throw ConfigError("threads", "must be positive"); }

void intermediate_layer()
{
    try { load_config(); }
    catch (const ConfigError&) { throw; } // 保留 ConfigError dynamic type。
}

void basic_example()
{
    bool preserved = false;
    try { intermediate_layer(); }
    catch (const ConfigError& error) { preserved = error.key() == "threads"; }
    catch (const std::exception&) { assert(false && "specific handler must run first"); }
    assert(preserved);
    std::cout << "[基礎] catch const& + throw; preserves ConfigError fields\n";
}

// LeetCode 8：String to Integer。題目要求只讀合法 numeric prefix 並 clamp；直接 stoi 再
// catch 不能完整表達 parse position，也容易在前導空白負溢位時判錯 sign，因此正式解法
// 在 overflow 發生前比較界線。Exception 教學留在上方 ConfigError 與下方 application boundary。
int my_atoi(const std::string& text)
{
    std::size_t index = 0U;
    while (index < text.size() && text.at(index) == ' ') ++index;

    int sign = 1;
    if (index < text.size() && (text.at(index) == '+' || text.at(index) == '-')) {
        sign = text.at(index) == '-' ? -1 : 1;
        ++index;
    }

    const long long limit = sign > 0
        ? static_cast<long long>(std::numeric_limits<int>::max())
        : static_cast<long long>(std::numeric_limits<int>::max()) + 1LL;
    long long magnitude = 0LL;
    while (index < text.size() && text.at(index) >= '0' && text.at(index) <= '9') {
        const int digit = text.at(index) - '0';
        if (magnitude > (limit - digit) / 10LL) {
            return sign > 0 ? std::numeric_limits<int>::max() : std::numeric_limits<int>::min();
        }
        magnitude = magnitude * 10LL + digit;
        ++index;
    }

    if (sign < 0 && magnitude == limit) return std::numeric_limits<int>::min();
    return sign * static_cast<int>(magnitude);
}

void leetcode_8_example()
{
    assert(my_atoi("42") == 42);
    assert(my_atoi("   -42") == -42);
    assert(my_atoi("4193 with words") == 4'193);
    assert(my_atoi("words and 987") == 0);
    assert(my_atoi("-91283472332") == std::numeric_limits<int>::min());
    assert(my_atoi("999999999999999999999") == std::numeric_limits<int>::max());
    assert(my_atoi("+-12") == 0);
    std::cout << "[LeetCode 8] prefix parsing、sign 與雙向 clamp 完整驗證\n";
}

// 實務：application boundary 可 catch base std::exception 做最後 logging，但不可假成功。
int application_boundary()
{
    try { intermediate_layer(); }
    catch (const std::exception& error) {
        std::cout << "[實務] fatal config error: " << error.what() << '\n';
        return 2;
    }
    return 0;
}

void practical_example()
{
    assert(application_boundary() == 2);
}

int main()
{
    basic_example();
    leetcode_8_example();
    practical_example();
}

// 易錯與面試：以 value catch 會 copy 且可能 slice derived exception；用 `const&`。重新
// 丟同一例外用裸 `throw;`，`throw error;` 會從 static type 建新 exception 並失去資訊。
// 練習：把 `throw;` 改 `throw error;`，觀察 custom type/fields 是否還保留。
// 複雜度：catch by reference 避免複製 exception；重拋成本仍與後續 stack unwinding 深度相關。
// 生命週期：caught reference 只在 handler 有效；bare `throw;` 重拋同一 active exception object。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_catch_by_ref.cpp' -o '/tmp/codex_cpp_C_Exception_04_catch_by_ref' && '/tmp/codex_cpp_C_Exception_04_catch_by_ref'
//
// === 預期輸出（節錄）===
// [LeetCode 8] prefix parsing、sign 與雙向 clamp 完整驗證
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
