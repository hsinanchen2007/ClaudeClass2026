/*
 * C++11 教科書：static_assert 編譯期契約
 *
 * static_assert(condition, message) 在編譯期檢查條件；失敗時 translation unit 無法完成。
 * 它適合檢查 type traits、模板前置條件、常數設定及 ABI 假設，不取代 runtime assert：
 * 使用者輸入、檔案內容等 runtime 資料仍需 exception/error code/validation。
 *
 * 【好訊息】message 應說明「需要什麼」，而不只是「錯了」。C++17 可省略 message。
 * 【陷阱】不要用 sizeof(struct)==N 當 portable serialization；padding/endian/ABI 會不同。
 * 【模板】依賴 template parameter 的 static_assert 只在 instantiation 時檢查。
 * 【面試題】assert 在 NDEBUG 下可能消失，static_assert 永遠是 compile-time gate。
 */

#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace basic {
template <class Integer>
constexpr Integer safe_double(Integer value) {
    static_assert(std::is_integral<Integer>::value, "safe_double 只接受整數型別");
    static_assert(!std::is_same<Integer, bool>::value, "bool 不是數值計算輸入");
    return value > std::numeric_limits<Integer>::max() / 2 ||
                   value < std::numeric_limits<Integer>::min() / 2
               ? throw std::overflow_error("safe_double overflow")
               : static_cast<Integer>(value * 2);
}

void demo() {
    static_assert(std::numeric_limits<unsigned char>::digits >= 8,
                  "此教材需要至少 8-bit byte");
    static_assert(safe_double(6) == 12, "safe_double 的編譯期結果錯誤");
    assert(safe_double(9L) == 18L);

    bool rejected = false;
    try {
        static_cast<void>(safe_double(std::numeric_limits<int>::max()));
    } catch (const std::overflow_error&) {
        rejected = true;
    }
    assert(rejected);
}
}  // namespace basic

namespace leetcode {
// LeetCode 1929：Concatenation of Array 的固定大小版本。
// N 在 compile time 已知，static_assert 可拒絕空輸入的教材規格。
template <std::size_t N>
std::array<int, N * 2U> concatenate(const std::array<int, N>& nums) {
    static_assert(N > 0U, "題目輸入至少要有一個元素");
    std::array<int, N * 2U> answer{};
    for (std::size_t i = 0; i < N; ++i) {
        answer[i] = nums[i];
        answer[i + N] = nums[i];
    }
    return answer;
}

void test() {
    constexpr std::array<int, 3> input{1, 2, 1};
    const auto output = concatenate(input);
    assert((output == std::array<int, 6>{1, 2, 1, 1, 2, 1}));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
template <class Counter>
class MetricsCounter {
    static_assert(std::is_unsigned<Counter>::value, "計數器必須是 unsigned，避免負計數");

public:
    void add(Counter value) { total_ += value; }
    Counter total() const noexcept { return total_; }

private:
    Counter total_{};
};

void test() {
    MetricsCounter<unsigned long> requests;
    requests.add(4UL);
    requests.add(6UL);
    assert(requests.total() == 10UL);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "static_assert：型別契約、陣列串接、計數器測試通過\n";
}
