/*
 * C++17 教科書：inline variables
 *
 * header 中定義 non-inline global variable 會在多個 translation units 造成 ODR/link error。
 * inline variable 允許相同定義出現在多個 TU，program 中視為同一 entity/address；這是
 * inline function 規則對變數的延伸，不代表 compiler 一定把資料「內嵌」到使用處。
 *
 * 【常見用途】header-only library constants、class 的 inline static data member。
 * namespace-scope constexpr 原本有 internal linkage；`inline constexpr` 明確建立共享常數。
 * 【初始化】動態初始化順序問題仍存在；inline 不會神奇修好跨 TU initialization fiasco。
 * 優先 constexpr constant 或 function-local static。
 * 【常見陷阱】inline 解決 ODR 定義問題，不解決動態初始化順序或共享 mutable state race。
 * 【面試題】inline 的核心語意是 ODR/linkage，不是效能 hint。
 * 【練習】將 practical_limits 改成 struct inline static members。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace basic {
inline constexpr int default_port = 8080;

class Counter {
public:
    Counter() { ++instances; }
    ~Counter() { --instances; }
    Counter(const Counter&) = delete;
    Counter& operator=(const Counter&) = delete;
    inline static int instances = 0;
};

void demo() {
    assert(default_port == 8080 && Counter::instances == 0);
    {
        Counter first;
        Counter second;
        assert(Counter::instances == 2);
    }
    assert(Counter::instances == 0);
}
}  // namespace basic

namespace leetcode {
inline constexpr int maximum_fibonacci_input = 46;  // int 不 overflow 的教材上限。

// LeetCode 509：Fibonacci Number。O(n) time / O(1) space。
int leetcode_fibonacci(int value) {
    assert(value >= 0 && value <= maximum_fibonacci_input);
    int previous = 0;
    int current = 1;
    for (int index = 0; index < value; ++index) {
        const int next = previous + current;
        previous = current;
        current = next;
    }
    return previous;
}

void leetcode_test() {
    assert(leetcode_fibonacci(0) == 0);
    assert(leetcode_fibonacci(10) == 55);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
inline const std::vector<std::string> supported_regions{"us-west", "us-east"};
inline constexpr std::size_t maximum_batch_size = 500U;

bool practical_supported(const std::string& region, std::size_t batch_size) {
    const bool known = region == supported_regions[0] || region == supported_regions[1];
    return known && batch_size <= maximum_batch_size;
}

void practical_test() {
    assert(practical_supported("us-west", 100U));
    assert(!practical_supported("eu", 100U));
    assert(!practical_supported("us-east", 501U));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "inline variables：ODR、Fibonacci、共享設定測試通過\n";
}
