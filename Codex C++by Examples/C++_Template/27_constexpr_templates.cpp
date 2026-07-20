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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 70. Climbing Stairs（爬樓梯）
// 題目：每次走 1 或 2 階，計算到第 n 階的方法數；n=5 時有 8 種。
// 為何使用本章主題：N 是 NTTP，consteval 強制在編譯期計算並讓結果可進 static_assert；
// 原題 n 是 runtime 參數，這是只接受編譯期階數的教學改寫。
// 思路：N<=1 回 1；由 step=2 起迭代 Fibonacci 狀態 previous/current；最後回 current。
// 複雜度：編譯期時間 O(N)、額外空間 O(1)，runtime 沒有這段迴圈成本，但會增加編譯工作。
// 易錯點：uint64_t 僅能正確容納到 N=92；N=0 回 1 是泛化定義，原題從 n=1 開始。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】協定 Magic Checksum 驗收
// 情境：協定固定 magic `CUDA` 的 checksum 要在編譯時鎖定，runtime 收到封包後再用同演算法驗證。
// 為何使用本章主題：checksum 是 constexpr function template，可同時處理編譯期 array 測試向量與
// runtime received 資料；static_assert 能立即發現常數或演算法被誤改。
// 設計：逐 byte 執行 FNV-1a xor/乘法；編譯期產生 expected；runtime 重算 received 並比較。
// 成本：時間 O(N)、額外空間 O(1)，N 是 magic byte 數；固定測試向量的計算由編譯器負擔。
// 上線注意：這不是密碼學完整性保護；Character 會截為 byte，真正封包還需長度、版本與認證驗證。
// -----------------------------------------------------------------------------
template <typename Character, std::size_t N>
constexpr std::uint32_t checksum(const std::array<Character, N>& bytes) {
    std::uint32_t hash = 2166136261U;
    for (Character byte : bytes) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 16777619U;
    }
    return hash;
}

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

/*
 * 【教科書補充：編譯期運算仍受整數規則限制】
 * - uint64_t climb-stairs 只有 N<=92 才容納正確結果；consteval 不會自動提供任意精度。
 * - power<int> 超出 int 範圍仍是 signed-overflow UB；constant evaluation 會拒絕該表達式，runtime 更需驗證。
 * - checksum 將 Character 截為 byte，只適合 byte-oriented protocol；寬字元需另定 encoding。
 * - FNV 使用 unsigned wrap 是演算法刻意的一部分，應和 signed overflow 明確區分。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '27_constexpr_templates.cpp' -o '/tmp/codex_cpp_C_Template_27_constexpr_templates' && '/tmp/codex_cpp_C_Template_27_constexpr_templates'
//
// === 預期輸出（節錄）===
// constexpr templates 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
