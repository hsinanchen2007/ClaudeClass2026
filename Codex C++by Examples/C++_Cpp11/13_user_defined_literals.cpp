/*
 * C++11 教科書：user-defined literals（UDL）
 *
 * UDL 讓 250_ms、4_KiB 之類單位直接進入型別系統。自訂 suffix 必須以下底線開始；
 * 不以下底線的 suffix 保留給標準函式庫。可接 unsigned long long、long double、
 * const char*+size 等形式，並應放在自己的 namespace，避免全域名稱衝突。
 *
 * 【設計】UDL 適合單位與 domain constants，不適合隱藏昂貴 I/O 或難以預期的副作用。
 * 【精度】整數 literal 參數是 unsigned long long，轉成較小型別前必須檢查範圍。
 * 【常見陷阱】UDL 不應隱藏 I/O 或昂貴副作用，也不可使用保留給標準庫的 suffix。
 * 【標準庫】後續標準加入 chrono/string/string_view literals，設計原理相同。
 * 【面試題】為何 1.5_KiB 需要 long double overload，而 2_KiB 走整數 overload？
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace units {
struct Bytes {
    std::size_t count;
};

constexpr Bytes operator""_KiB(unsigned long long value) {
    return (std::numeric_limits<std::size_t>::digits <=
                std::numeric_limits<unsigned long long>::digits &&
            value > static_cast<unsigned long long>(
                        std::numeric_limits<std::size_t>::max() / 1024U))
               ? throw std::overflow_error("KiB literal exceeds size_t")
               : Bytes{static_cast<std::size_t>(value) * std::size_t{1024U}};
}
}  // namespace units

namespace basic {
void demo() {
    using namespace units;
    constexpr Bytes cache = 4_KiB;
    static_assert(cache.count == 4096U, "4 KiB 必須等於 4096 bytes");

    bool rejected = false;
    if (std::numeric_limits<std::size_t>::digits == 64) {
        try {
            static_cast<void>(18014398509481984_KiB); // 2^54 KiB = 2^64 bytes。
        } catch (const std::overflow_error&) {
            rejected = true;
        }
        assert(rejected);
    }
}
}  // namespace basic

namespace leetcode {
// LeetCode 58：Length of Last Word。string 版本保持 C++11 可編譯。
std::size_t length_of_last_word(const std::string& text) {
    const auto last = text.find_last_not_of(' ');
    if (last == std::string::npos) return 0U;
    const auto before = text.find_last_of(' ', last);
    return before == std::string::npos ? last + 1U : last - before;
}

void test() {
    assert(length_of_last_word("Hello World") == 5U);
    assert(length_of_last_word("   fly me   to   the moon  ") == 4U);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
using namespace units;

class BufferBudget {
public:
    explicit constexpr BufferBudget(Bytes capacity) : capacity_(capacity) {}
    constexpr bool fits(Bytes request) const noexcept {
        return request.count <= capacity_.count;
    }

private:
    Bytes capacity_;
};

void test() {
    constexpr BufferBudget budget(64_KiB);
    static_assert(budget.fits(8_KiB), "8 KiB 應放得進 64 KiB");
    static_assert(!budget.fits(128_KiB), "128 KiB 不應放得進 64 KiB");
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "UDL：容量單位、Last Word、buffer budget 測試通過\n";
}
