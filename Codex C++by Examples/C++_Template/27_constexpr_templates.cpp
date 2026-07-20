/*
 * 第 27 章：constexpr templates
 *
 * constexpr 函式「可以」在編譯期執行，但只有輸入與上下文符合 constant expression 規則時
 * 才一定發生；同一函式也可在執行期使用。consteval 則強制編譯期。模板配 constexpr
 * 可讓型別/尺寸參與預先計算，減少 runtime 工作並用 static_assert 驗證不變量。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

template <typename T>
constexpr T power(T base, unsigned exponent) {
    T result{1};
    while (exponent > 0U) {
        if ((exponent & 1U) != 0U) {
            result *= base;
        }
        exponent >>= 1U;
        if (exponent > 0U) {
            base *= base;
        }
    }
    return result;
}

// LeetCode 70：Climbing Stairs。N 是編譯期參數，回傳可用於 static_assert。
template <std::size_t N>
consteval std::uint64_t leetcode_climb_stairs() {
    if constexpr (N <= 1U) {
        return 1U;
    } else {
        std::uint64_t previous = 1U;
        std::uint64_t current = 1U;
        for (std::size_t step = 2; step <= N; ++step) {
            const std::uint64_t next = previous + current;
            previous = current;
            current = next;
        }
        return current;
    }
}

template <typename Character, std::size_t N>
constexpr std::uint32_t checksum(const std::array<Character, N>& bytes) {
    std::uint32_t hash = 2166136261U;
    for (Character byte : bytes) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 16777619U;
    }
    return hash;
}

// 【實務情境】協定 magic 的 checksum 在編譯期建立固定測試向量，runtime 再驗收封包。
void practical_protocol_test() {
    constexpr std::array<char, 4> magic{'C', 'U', 'D', 'A'};
    constexpr std::uint32_t expected = checksum(magic);
    // FNV-1a("CUDA") 的固定測試向量；若演算法被誤改，編譯立即失敗。
    static_assert(expected == 3379756726U);

    // runtime 仍可呼叫同一 constexpr template，例如資料來自網路封包。
    const std::array<char, 4> received{'C', 'U', 'D', 'A'};
    assert(checksum(received) == expected);
}

int main() {
    static_assert(power(2, 10U) == 1024);
    const int runtime_base = 3;
    assert(power(runtime_base, 4U) == 81);

    static_assert(leetcode_climb_stairs<2>() == 2U);
    static_assert(leetcode_climb_stairs<5>() == 8U);
    static_assert(leetcode_climb_stairs<10>() == 89U);

    practical_protocol_test();
    std::cout << "constexpr templates 測試完成\n";
}

/*
 * 【陷阱】constexpr 不等於一定 compile-time；要以 constexpr 變數、static_assert 或 consteval 強制。
 * 【限制】大量編譯期運算會增加編譯時間，且編譯器有 constexpr steps/depth 上限。
 * 【溢位】climb_stairs 仍受 uint64_t 上限；編譯期計算不會自動提供任意精度。
 * 【面試】constinit 只保證靜態物件初始化期，不代表物件之後不可修改。
 * 【練習】用 constexpr 建立 0..255 的 byte parity lookup table。
 */
