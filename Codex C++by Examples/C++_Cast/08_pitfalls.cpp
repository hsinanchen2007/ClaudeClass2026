// ============================================================================
// 課題 8：Cast 常見陷阱與 review checklist
// ============================================================================
//
// 看到 cast 時逐項問：
//   * numeric value 是否在目的 range？round/truncate policy 是什麼？
//   * pointer 的 dynamic type、alignment、lifetime 是否成立？誰 owns？
//   * const_cast 後原 object 是否真的 mutable？callee 是否可能寫入？
//   * reinterpret/bit_cast 的 representation 是否 portable/有效？endian 呢？
//   * cast 是否只是掩蓋 API type 設計錯誤或 compiler warning？
//
// 最危險的 pattern 是「為消警告加 cast」：warning 消失，但資料遺失仍存在。先驗證，
// 再在最小範圍 cast；可用 optional/expected 回報無法表示，而非偷偷 wrap/truncate。
// ============================================================================

#include <cassert>
#include <cctype>
#include <climits>
#include <iostream>
#include <limits>
#include <optional>
#include <string>

std::optional<unsigned char> checked_byte(int value)
{
    if (value < 0 || value > static_cast<int>(std::numeric_limits<unsigned char>::max())) {
        return std::nullopt;
    }
    return static_cast<unsigned char>(value);
}

void basic_example()
{
    assert(checked_byte(255).value() == static_cast<unsigned char>(255));
    assert(!checked_byte(256).has_value());
    assert(!checked_byte(-1).has_value());
    std::cout << "[基礎] checked cast accepts 255, rejects 256/-1\n";
}

// LeetCode 8：String to Integer (atoi)。
// 逐 digit 前先用 unsigned char 呼叫 cctype，並在乘 10/加 digit 前做 range clamp；
// 最後 cast 回 int 已有數學證明，不是用 cast 隱藏 overflow。
int my_atoi(const std::string& input)
{
    std::size_t index = 0U;
    while (index < input.size() &&
           std::isspace(static_cast<unsigned char>(input.at(index))) != 0) ++index;
    int sign = 1;
    if (index < input.size() && (input.at(index) == '+' || input.at(index) == '-')) {
        if (input.at(index) == '-') sign = -1;
        ++index;
    }
    long long value = 0;
    while (index < input.size() &&
           std::isdigit(static_cast<unsigned char>(input.at(index))) != 0) {
        value = value * 10 + (input.at(index) - '0');
        const long long signed_value = sign == 1 ? value : -value;
        if (signed_value > INT_MAX) return INT_MAX;
        if (signed_value < INT_MIN) return INT_MIN;
        ++index;
    }
    return static_cast<int>(sign == 1 ? value : -value);
}

void leetcode_8_example()
{
    assert(my_atoi("42") == 42);
    assert(my_atoi("   -042") == -42);
    assert(my_atoi("4193 with words") == 4'193);
    assert(my_atoi("91283472332") == INT_MAX);
    std::cout << "[LeetCode 8] parsing clamps before final int cast\n";
}

// 實務：duration API 從 unsigned external field 進 signed internal type前檢查上界。
std::optional<int> timeout_from_wire(unsigned long milliseconds)
{
    if (milliseconds > static_cast<unsigned long>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return static_cast<int>(milliseconds);
}

void practical_example()
{
    assert(timeout_from_wire(5'000UL).value() == 5'000);
    assert(!timeout_from_wire(std::numeric_limits<unsigned long>::max()).has_value());
    std::cout << "[實務] wire timeout checked before signed narrowing\n";
}

int main()
{
    basic_example();
    leetcode_8_example();
    practical_example();
}

// 面試快問快答：
// Q：static_cast 可以防止整數窄化造成資料遺失嗎？
// A：不可以。它只明確表達轉型意圖；若來源值超出目的型別可表示範圍，資料仍會改變。
//    正確流程是先用 numeric_limits 驗證數學範圍，再做最小範圍的 static_cast。
// Q：為什麼不能把所有 conversion warning 都用 cast 消掉？
// A：warning 往往揭露 API 型別或邊界檢查不足；cast 只讓編譯器停止提醒，不會建立 range、
//    alignment、dynamic type、object lifetime 或 ownership 的正確性證明。

// 練習：開啟 -Wconversion/-Wsign-conversion，逐一判斷哪些 warning 應改 type 而非加 cast。
// 複雜度：checked parsing 是 O(D)，D 為輸入 digits；最後的 cast 通常 O(1)，不可取代驗證。
// 生命週期：任何 cast 得到的 view/pointer 都受來源 storage 約束，compiler 不會替你追 owner。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_pitfalls.cpp' -o '/tmp/codex_cpp_C_Cast_08_pitfalls' && '/tmp/codex_cpp_C_Cast_08_pitfalls'
//
// === 預期輸出（節錄）===
// [實務] wire timeout checked before signed narrowing
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
