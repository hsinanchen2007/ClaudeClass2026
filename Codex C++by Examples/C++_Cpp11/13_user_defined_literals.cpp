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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word（最後一個單字的長度）
// 題目：輸入由單字與空白組成的字串，回傳最後一個單字長度；"Hello World" 回傳 5。
// 為何使用本章主題：此字串題不需要 UDL；它作為同檔演算法案例，UDL 的真正角色留在後方容量單位實務範例。
// 思路：1. 從尾端找最後一個非空白；2. 再向前找分隔空白；3. 以兩個位置差計算長度。
// 複雜度：N 為字串長度；最壞時間 O(N)、額外空間 O(1)。
// 易錯點：全空白輸入會得到 npos；第一個單字前沒有空白時長度要用 last+1。
// -----------------------------------------------------------------------------
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

namespace practical {
using namespace units;

// -----------------------------------------------------------------------------
// 【日常實務範例】網路緩衝區容量配額
// 情境：服務有固定 64 KiB buffer，要在編譯期判斷 8 KiB 或 128 KiB 請求是否可容納。
// 為何使用本章主題：_KiB 把單位直接寫在呼叫點並回傳 Bytes，避免裸整數究竟代表 byte 或 KiB 的歧義。
// 設計：1. UDL 將 literal 安全乘 1024；2. BufferBudget 持有容量；3. fits 比較 request 與 capacity 的 byte 數。
// 成本：literal 轉換與比較皆 O(1)，物件空間 O(1)，常數輸入可完全在編譯期求值。
// 上線注意：UDL 已檢查 size_t 乘法上限；不同單位仍應維持強型別，且 UDL 不應隱藏配置或 I/O。
// -----------------------------------------------------------------------------
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

// 【延伸練習】加入 _MiB 並做 overflow 檢查；說明為何 literal 不應偷偷讀檔或查網路。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '13_user_defined_literals.cpp' -o '/tmp/codex_cpp_C_Cpp11_13_user_defined_literals' && '/tmp/codex_cpp_C_Cpp11_13_user_defined_literals'
//
// === 預期輸出（節錄）===
// UDL：容量單位、Last Word、buffer budget 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
