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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接自身）
// 題目：輸入 nums，回傳 nums 後再接一次 nums；例如 [1,2,1] 變成 [1,2,1,1,2,1]。
// 為何使用本章主題：教材改用固定大小 std::array，使 N 成為模板參數並可用 static_assert 拒絕空陣列規格。
// 思路：1. 建立大小 2N 的 answer；2. 將 nums[i] 寫到 i；3. 同時寫到 i+N。
// 複雜度：N 為輸入長度；時間 O(N)，結果空間 O(N)、除結果外額外工作空間 O(1)。
// 易錯點：這是固定大小教學改寫，不是 LeetCode 的 vector 介面；第二份索引必須偏移 N。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】無負值的指標計數器
// 情境：請求數等 metrics 只能增加非負數量，希望錯用 signed counter 時直接無法建置。
// 為何使用本章主題：類別內 static_assert 檢查 Counter type trait，把型別契約提前到模板實體化階段。
// 設計：1. 驗 Counter 為 unsigned；2. add 將批次值累加；3. total 以相同型別回傳目前值。
// 成本：每次 add/total 時間 O(1)，每個計數器空間 O(1)。
// 上線注意：unsigned 仍會模數溢位，且多執行緒更新有 data race；需依需求加入飽和、atomic 或鎖。
// -----------------------------------------------------------------------------
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

// 【延伸練習】為固定 wire header 寫型別契約，但不要以 sizeof(struct) 取代逐欄 serialization。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_static_assert.cpp' -o '/tmp/codex_cpp_C_Cpp11_10_static_assert' && '/tmp/codex_cpp_C_Cpp11_10_static_assert'
//
// === 預期輸出（節錄）===
// static_assert：型別契約、陣列串接、計數器測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
