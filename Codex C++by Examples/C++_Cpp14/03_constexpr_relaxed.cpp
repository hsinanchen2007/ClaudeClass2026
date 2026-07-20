/*
 * C++14 教科書：放寬的 constexpr function
 *
 * C++11 constexpr function 幾乎只能有單一 return；C++14 開始允許 local variable、if、
 * switch、loop 與修改 local state（仍不能執行不允許 constant expression 的操作）。
 * 因此正常 iterative algorithm 可同時服務 compile-time 與 runtime，不必刻意寫遞迴。
 *
 * 【限制】constexpr 不保證每次 compile-time；只有 constant-expression context 才強制。
 * 動態配置到 C++20 才有更多支援。throw 路徑若在 compile-time 被走到會使常數求值失敗。
 * 【成本】大量 constexpr table 會增加編譯時間；先確認 startup/runtime 收益值得。
 * 【常見陷阱】函式標 constexpr 不代表每次都常數求值；runtime 輸入仍走 runtime。
 * 【面試題】何時 static_assert(f(x)) 能證明 f 真正在 compile time 執行？x 也須為常數。
 * 【練習】寫 constexpr gcd，並以 static_assert 驗證 48/18 的最大公因數。
 */

#include <cassert>
#include <cstddef>
#include <iostream>

namespace basic {
constexpr int factorial(int value) {
    int result = 1;
    for (int current = 2; current <= value; ++current) {
        result *= current;
    }
    return result;
}

void demo() {
    static_assert(factorial(5) == 120, "C++14 loop constexpr 應在編譯期工作");
    int runtime = 6;
    assert(factorial(runtime) == 720);
}
}  // namespace basic

namespace leetcode {
// LeetCode 70：Climbing Stairs。iterative DP，O(n) time、O(1) space。
constexpr int leetcode_climb_stairs(int steps) {
    if (steps <= 2) return steps;
    int previous = 1;
    int current = 2;
    for (int step = 3; step <= steps; ++step) {
        const int next = previous + current;
        previous = current;
        current = next;
    }
    return current;
}

void leetcode_test() {
    static_assert(leetcode_climb_stairs(5) == 8, "五階樓梯應有八種走法");
    assert(leetcode_climb_stairs(10) == 89);
}
}  // namespace leetcode

// 【實務案例】編譯期 lookup table：用 C++14 loop 建表，並避開當版 std::array 的 constexpr 限制。
namespace practical {
// 實務：編譯期建立簡單 CRC-like lookup（教學版，不是正式 CRC 規格）。
// C++14 標準尚未要求 std::array 的 mutable operator[] 為 constexpr；該能力由 C++17
// 的 P0031 加入。為讓本檔嚴格維持 C++14，這裡使用內建陣列。
struct Weights {
    unsigned values[4];
    constexpr unsigned operator[](std::size_t index) const { return values[index]; }
};

constexpr Weights practical_weights() {
    Weights weights{{0U, 0U, 0U, 0U}};
    for (std::size_t i = 0; i < 4U; ++i) {
        weights.values[i] = static_cast<unsigned>((i + 1U) * (i + 1U));
    }
    return weights;
}

void practical_test() {
    constexpr Weights weights = practical_weights();
    static_assert(weights[0] == 1U && weights[3] == 16U,
                  "lookup table 必須在 compile time 正確建立");
    assert(weights[2] == 9U);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "C++14 constexpr：loop、Climbing Stairs、lookup table 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_constexpr_relaxed.cpp' -o '/tmp/codex_cpp_C_Cpp14_03_constexpr_relaxed' && '/tmp/codex_cpp_C_Cpp14_03_constexpr_relaxed'
//
// === 預期輸出（節錄）===
// C++14 constexpr：loop、Climbing Stairs、lookup table 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
