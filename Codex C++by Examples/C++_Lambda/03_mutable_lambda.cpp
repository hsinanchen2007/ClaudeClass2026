/*
 * Lambda 教科書 03：mutable closure state
 *
 * value-captured members 在預設 `operator() const` 中不可修改；加 mutable 後 call operator
 * 不再是 const，可修改 closure 自己的副本。原變數不會被改。每份 closure copy 各有獨立狀態，
 * 除非 capture 的本身是 shared pointer/reference 等共享物件。
 *
 * 【用途】小型 generator、計數 callback、backoff sequence。若狀態是核心 domain object，
 * 命名 class 通常比 stateful lambda 更好測試/序列化/同步。
 * 【const】mutable 不表示 thread-safe，也不允許修改 value-captured const object 的底層 const。
 * 【並行】同一 closure 被多執行緒同時呼叫並寫狀態會 data race，需 mutex/atomic/每執行緒副本。
 * 【面試題】mutable lambda copy 後兩個計數器是否共享 count？一般不共享，各自有副本。
 * 【練習】替 practical_backoff 增加 maximum delay，避免整數 overflow。
 */

#include <cassert>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace basic {
void demo() {
    int original = 0;
    auto counter = [original]() mutable { return ++original; };
    assert(counter() == 1 && counter() == 2);
    assert(original == 0);

    auto copied_counter = counter;
    assert(copied_counter() == 3);
    assert(counter() == 3);  // 各自從 copy 時的 2 往上加，互不影響。
}
}  // namespace basic

namespace leetcode {
// LeetCode 202：Happy Number。mutable lambda 封裝「平方各位數」的 running total。
bool leetcode_is_happy(int number) {
    std::unordered_set<int> seen;
    const auto next = [](int value) {
        auto add_square = [total = 0](int digit) mutable {
            total += digit * digit;
            return total;
        };
        int total = 0;
        while (value > 0) {
            total = add_square(value % 10);
            value /= 10;
        }
        return total;
    };
    while (number != 1 && seen.insert(number).second) number = next(number);
    return number == 1;
}

void leetcode_test() {
    assert(leetcode_is_happy(19));
    assert(!leetcode_is_happy(2));
}
}  // namespace leetcode

// 【實務案例】退避序列：mutable closure 保存每次呼叫後更新的 delay 狀態。
namespace practical {
auto practical_backoff(int initial_ms, int factor) {
    return [delay = initial_ms, factor]() mutable {
        const int current = delay;
        delay *= factor;
        return current;
    };
}

void practical_test() {
    auto next_delay = practical_backoff(100, 2);
    assert(next_delay() == 100);
    assert(next_delay() == 200);
    assert(next_delay() == 400);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "mutable lambda：獨立狀態、Happy Number、backoff 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_mutable_lambda.cpp' -o '/tmp/codex_cpp_C_Lambda_03_mutable_lambda' && '/tmp/codex_cpp_C_Lambda_03_mutable_lambda'
//
// === 預期輸出（節錄）===
// mutable lambda：獨立狀態、Happy Number、backoff 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
