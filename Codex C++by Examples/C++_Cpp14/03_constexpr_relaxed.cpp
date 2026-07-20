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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 70. Climbing Stairs（爬樓梯）
// 題目：每次走 1 或 2 階，計算走到第 n 階的方法數；n=5 時答案為 8。
// 為何使用本章主題：C++14 放寬 constexpr body，讓一般 local variable、if 與 loop 的迭代 DP 也可進 static_assert。
// 思路：1. 前兩階直接回傳 n；2. 保存前兩個答案；3. 從第 3 階迭代更新 next 直到 n。
// 複雜度：N 為階數；時間 O(N)、額外空間 O(1)。
// 易錯點：題目限定正整數；較大 N 會使 int 溢位，constexpr 也不代表所有 runtime 呼叫都在編譯期執行。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】編譯期權重查詢表
// 情境：固定四個位置的平方權重要隨程式發布，啟動時不應再重建資料表。
// 為何使用本章主題：relaxed constexpr 允許 loop 修改 local aggregate；內建陣列避開 C++14 std::array 可寫 operator[] 限制。
// 設計：1. 以零值初始化四格；2. loop 將 (i+1)^2 寫入；3. 回傳 Weights 並以 static_assert 驗端點。
// 成本：K=4；編譯期建立時間 O(K)、產物空間 O(K)，runtime 查詢 O(1)。
// 上線注意：這只是平方權重教學表，不是正式 CRC；表變大會增加編譯時間，索引也必須保持在 0..3。
// -----------------------------------------------------------------------------
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
