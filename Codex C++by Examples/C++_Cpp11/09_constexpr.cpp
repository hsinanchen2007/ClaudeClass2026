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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 70. Climbing Stairs（爬樓梯）
// 題目：每次可爬 1 或 2 階，求到第 n 階的方法數；n=5 時共有 8 種。
// 為何使用本章主題：C++11 constexpr 的單一 return 限制促使本例用尾遞迴，讓小 n 可由 static_assert 編譯期驗證。
// 思路：1. 前兩階答案為 1、2；2. 每階答案等於前兩階相加；3. 以 previous/current 尾遞迴推到剩餘階數。
// 複雜度：N 為階數；時間 O(N)，標準未保證尾呼叫消除，因此額外呼叫堆疊 O(N)。
// 易錯點：題目契約是正整數階數；較大 n 會使 int 溢位，runtime 大輸入宜改 iterative O(1) 空間版本。
// -----------------------------------------------------------------------------
constexpr int climb_impl(int remaining, int previous, int current) {
    return remaining <= 2 ? current
                          : climb_impl(remaining - 1, current, previous + current);
}

constexpr int climb_stairs(int steps) {
    return steps <= 2 ? steps : climb_impl(steps, 1, 2);
}

void test() {
    static_assert(climb_stairs(5) == 8, "五階樓梯應有八種走法");
    assert(climb_stairs(10) == 89);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】固定 worker 記憶體預算
// 情境：服務映像檔在編譯時已知 worker 數與每個 worker 的 KiB 配額，要產生總 byte 預算。
// 為何使用本章主題：constexpr 讓單位換算與乘法可在常數語境求值，再由 static_assert 鎖住部署常數。
// 設計：1. 保存 workers 與 kib_per_worker；2. 將 KiB 乘 1024；3. 再乘 worker 數取得 total_bytes。
// 成本：compile-time 或 runtime 都是 O(1) 值運算，物件空間 O(1)。
// 上線注意：目前未拒絕負值或乘法溢位；常數求值成功只證明算式可算，不代表資源真的可配置。
// -----------------------------------------------------------------------------
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
