// ============================================================================
// 課題 2：static_cast - 明確的 compile-time conversion
// ============================================================================
//
// 常見用途：numeric conversion、enum↔underlying、Derived*→Base* upcast、已知來源的
// void* 還原。Base*→Derived* 的 static downcast 不做 runtime check；若 Base subobject
// 並不屬於 Derived（或其衍生類別）物件，執行該 downcast 本身就產生 UB，不是等到
// 解參考才出事。無法由程式 invariant 證明時，應改 dynamic_cast 或重新設計。
//
// numeric static_cast 也不等於 checked cast：floating→integer 截斷小數，超出目的範圍
// 會是 UB。C++20 的 integer→integer 轉換則得到與來源值模 2^N 同餘、且目的型別唯一
// 可表示的值；雖已不是舊教材常寫的 implementation-defined，通常仍不是業務想要的
// clipping/error policy。外部資料仍應先 range-check，再 cast。
// ============================================================================

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>

enum class Status : unsigned char { idle = 0U, running = 1U, failed = 2U };

void basic_example()
{
    const Status status = Status::running;
    const auto code = static_cast<std::underlying_type_t<Status>>(status);
    assert(code == 1U);

    const double measurement = 3.9;
    const int truncated = static_cast<int>(measurement);
    assert(truncated == 3); // static_cast 不做 round；要四捨五入請用 lround 等 API。
    std::cout << "[基礎] enum code=1，3.9 truncates to 3\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 69. Sqrt(x)（整數平方根）
// 題目：回傳非負整數 x 的平方根向下取整；x=8 回傳 2，INT_MAX 回傳 46,340。
// 為何使用本章主題：以 long long 計算 middle^2 防 int 溢位，最後在範圍已證明後才 static_cast 回 int。
// 思路：1. 在 [0,x] 二分；2. middle^2<=x 時記錄並往右；3. 否則往左，最後回最佳 answer。
// 複雜度：X 為輸入值；時間 O(log X)、額外空間 O(1)。
// 易錯點：負值要拒絕；static_cast 不是 range check，安全性來自 answer<=sqrt(INT_MAX) 的先行證明。
// -----------------------------------------------------------------------------
int integer_sqrt(int value)
{
    if (value < 0) throw std::invalid_argument("sqrt input must be nonnegative");
    long long left = 0;
    long long right = value;
    long long answer = 0;
    while (left <= right) {
        const long long middle = left + (right - left) / 2;
        if (middle * middle <= value) {
            answer = middle;
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    return static_cast<int>(answer);
}

void leetcode_69_example()
{
    assert(integer_sqrt(4) == 2);
    assert(integer_sqrt(8) == 2);
    assert(integer_sqrt(std::numeric_limits<int>::max()) == 46'340);
    std::cout << "[LeetCode 69] sqrt(INT_MAX)=46340 without overflow\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】秒數轉毫秒的受檢窄化
// 情境：外部 double 秒數要轉成 long long 毫秒，負值、NaN、無限值或超範圍都回失敗。
// 為何使用本章主題：static_cast 明確採截斷政策，但只有在 isfinite 與數值上界檢查後才合法可靠。
// 設計：1. 驗 finite 且非負；2. 以 max/1000 驗乘法後可表示；3. 乘 1000 後 cast 並包 optional。
// 成本：固定次數檢查與運算，時間、額外空間皆 O(1)。
// 上線注意：需確認截斷而非四捨五入符合需求；浮點邊界與單位來源也要在 API 契約明訂。
// -----------------------------------------------------------------------------
std::optional<long long> seconds_to_milliseconds(double seconds)
{
    if (!std::isfinite(seconds) || seconds < 0.0) return std::nullopt;
    constexpr double max_ms = static_cast<double>(std::numeric_limits<long long>::max());
    if (seconds > max_ms / 1'000.0) return std::nullopt;
    return static_cast<long long>(seconds * 1'000.0);
}

void practical_example()
{
    assert(seconds_to_milliseconds(1.25).value() == 1'250LL);
    assert(!seconds_to_milliseconds(-1.0).has_value());
    std::cout << "[實務] 1.25 seconds -> 1250ms after range check\n";
}

int main()
{
    basic_example();
    leetcode_69_example();
    practical_example();
}

// 易錯與面試：static_cast 做 numeric narrowing 不會替你檢查 range，也不會丟例外；
// float→integer 超出可表示範圍尤其危險。先驗 domain，再明定 rounding/truncation policy。
// 練習：寫 checked_narrow<T>(U)，同時檢查 signedness 與上下界。
// 複雜度與生命週期：built-in static_cast 是 O(1) 值轉換；轉成新 value 不借用來源，
// 但 downcast pointer 只改 static type，既不驗 dynamic object 也不延長 pointee 生命。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_static_cast.cpp' -o '/tmp/codex_cpp_C_Cast_02_static_cast' && '/tmp/codex_cpp_C_Cast_02_static_cast'
//
// === 預期輸出（節錄）===
// [實務] 1.25 seconds -> 1250ms after range check
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
