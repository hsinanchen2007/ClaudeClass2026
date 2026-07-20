/*
 * C++11 教科書：constexpr 編譯期計算
 *
 * constexpr 表示「若輸入與語境允許，可在 compile time 求值」；它不是保證每次都編譯期。
 * constexpr variable 必須由 constant expression 初始化，並隱含 const。constexpr function
 * 也能在 runtime 接受 runtime 值。C++11 對 function body 限制很嚴，後續標準逐步放寬。
 *
 * 【用途】array size、non-type template argument、查表、單位轉換與 compile-time validation。
 * 【陷阱】把大型計算搬到 compile time 可能增加編譯時間與 binary；不要只為炫技使用。
 * 【consteval/constinit】是 C++20；前者強制編譯期呼叫，後者保證 static 初始化時機。
 * 【面試題】const 與 constexpr 差異：const 是不可修改；constexpr 是 constant expression 能力。
 */

#include <array>
#include <cassert>
#include <iostream>

namespace basic {
constexpr int square(int value) noexcept { return value * value; }
constexpr int kibibytes(int value) noexcept { return value * 1024; }

void demo() {
    constexpr int area = square(6);
    std::array<int, static_cast<std::size_t>(square(3))> grid{};
    static_assert(area == 36, "square 必須可在編譯期計算");
    static_assert(square(3) == 9, "array extent 必須是 constant expression");
    assert(grid.size() == 9U);

    int runtime = 5;
    assert(square(runtime) == 25);  // 同一函式也能在 runtime 執行。
}
}  // namespace basic

namespace leetcode {
// LeetCode 70：Climbing Stairs。為符合 C++11 constexpr 的單一 return 限制，這裡採
// tail-recursive recurrence：O(n) time、O(n) call stack；標準不保證 tail-call elimination。
constexpr int climb_impl(int remaining, int previous, int current) {
    return remaining <= 2 ? current
                          : climb_impl(remaining - 1, current, previous + current);
}

// Runtime 大 n 應改 iterative O(1) 額外空間；本例只用小 n 示範 compile-time 求值。
constexpr int climb_stairs(int steps) {
    return steps <= 2 ? steps : climb_impl(steps, 1, 2);
}

void test() {
    static_assert(climb_stairs(5) == 8, "五階樓梯應有八種走法");
    assert(climb_stairs(10) == 89);
}
}  // namespace leetcode

// 【實務案例】worker 記憶體預算：在編譯期完成 KiB 換算並用 static_assert 鎖住結果。
namespace practical {
struct MemoryBudget {
    int workers;
    int kib_per_worker;

    constexpr int total_bytes() const noexcept {
        return workers * basic::kibibytes(kib_per_worker);
    }
};

void test() {
    constexpr MemoryBudget budget{4, 256};
    static_assert(budget.total_bytes() == 1048576, "記憶體預算換算錯誤");
    assert(budget.total_bytes() == 1048576);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "constexpr：編譯期計算、爬樓梯、記憶體預算測試通過\n";
}

// 【延伸練習】讓 MemoryBudget 拒絕負數與乘法溢位，分別測 compile-time 與 runtime 輸入。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_constexpr.cpp' -o '/tmp/codex_cpp_C_Cpp11_09_constexpr' && '/tmp/codex_cpp_C_Cpp11_09_constexpr'
//
// === 預期輸出（節錄）===
// constexpr：編譯期計算、爬樓梯、記憶體預算測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
