// =============================================================================
//  03_bit_header_cpp20.cpp  —  <bit> header 的 C++20 工具
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/header/bit
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ <bit> 提供哪些工具？                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  C++20 把先前要靠 GCC __builtin_* 或 MSVC __popcnt 等 intrinsics 才能做的
//  事情，全部標準化進 <bit>：
//
//      std::popcount(x)       ─ 計算 1 的個數
//      std::countl_zero(x)    ─ 從最高位算起連續 0 個數 (count leading zeros)
//      std::countr_zero(x)    ─ 從最低位算起連續 0 個數 (count trailing zeros)
//      std::countl_one(x)     ─ 從最高位算起連續 1 個數
//      std::countr_one(x)     ─ 從最低位算起連續 1 個數
//      std::bit_width(x)      ─ 表達 x 至少要幾 bit （= 1 + log2(x)）
//      std::bit_floor(x)      ─ ≤ x 的最大 2 次方
//      std::bit_ceil(x)       ─ ≥ x 的最小 2 次方
//      std::has_single_bit(x) ─ 是不是 2 的次方
//      std::rotl(x, k)        ─ 循環左移
//      std::rotr(x, k)        ─ 循環右移
//      std::bit_cast<T>(src)  ─ bit-pattern 安全轉型（見 04_bit_cast.cpp）
//
//  上面工具「只接受 unsigned 整數」 — 對 signed 用會編譯失敗，這是好事
//  （位元運算對 signed 容易出 UB）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 實務應用                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * popcount：LC191 一行解
//   * bit_ceil：要 round up 到 2 的次方（hash table 容量、buffer 對齊）
//   * countr_zero：找最低位 1 的位置（== Fenwick tree lowbit 的 index）
//   * has_single_bit：判斷是否為 2 的次方（替代 (n & (n-1)) == 0 的可讀性）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：bit_header_cpp20
  - bit_header_cpp20 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - <bit> 把常見位元操作標準化，例如 popcount、countl_zero、countr_zero、has_single_bit，減少手寫未定義行為。
  - std::has_single_bit(x) 比 x && !(x & (x - 1)) 更直接表達 power-of-two 判斷。
  - <bit> 函式多半要求 unsigned integer；傳入 signed 值前先確認轉型語意。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】<bit> header（C++20）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++20 的 <bit> 提供哪些函式？有什麼共同限制？
//     答：popcount（set bit 個數）、countl_zero／countl_one（從最高位起連續 0／1 的
//     個數）、countr_zero／countr_one（從最低位起）、has_single_bit（是否為 2 的冪）、
//     bit_width（表示 x 所需的位數，x == 0 回 0）、bit_ceil／bit_floor（不小於／不大於
//     x 的 2 的冪）、rotl／rotr（循環位移）、bit_cast、以及 std::endian。共同限制是：
//     引數必須是 unsigned integer type（不含 bool、char、char8_t 等）。全部 constexpr，
//     且通常直接映射到 POPCNT／LZCNT／TZCNT／ROL 等硬體指令。
//     追問：為什麼 std::popcount(5) 編譯失敗？（5 是 int，constraint 要求無號型別；
//     要寫 std::popcount(5u) 或 static_cast<unsigned>(n)）
//
// 🔥 Q2. std::rotl 相對手寫 (x << s) | (x >> (N - s)) 的優勢是什麼？
//     答：手寫版在 s == 0 時會變成 x >> N（例如 32 位型別的 x >> 32），而「位移量 >=
//     型別寬度」在任何標準版本都是 UB——這個邊界條件極容易漏掉，而且平常測試不會踩到。
//     std::rotl／rotr 對任意 s 都良好定義，s 為負數代表反向旋轉，也不是 UB。
//
// Q3. std::popcount 和 __builtin_popcount／_mm_popcnt_u32 差在哪？
//     答：__builtin_popcount 是 GCC/Clang 擴充、不可攜（MSVC 用 __popcnt）；
//     _mm_popcnt_u32 是 intrinsic，需要 CPU 支援且要開對應的編譯旗標，在不支援的機器上
//     會是非法指令。std::popcount 是標準、constexpr、可攜的，實作會在硬體支援時使用
//     POPCNT 指令、否則退回軟體實作。現代 C++ 一律優先用 std::popcount。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#if __cplusplus >= 202002L
#  include <bit>
#endif
#include <cstdint>

// 前置宣告：附加範例（C++20）
#if __cplusplus >= 202002L
static void demo_lc_190_reverse_bits();
static void demo_align_up();
#endif

int main() {
#if __cplusplus < 202002L
    std::cout << "[skipped] need C++20\n";
    return 0;
#else
    std::uint32_t x = 0b0001'0110;   // 22 = 10110

    std::cout << "x = 22 (binary 0001'0110)\n";
    std::cout << "popcount        = " << std::popcount(x)        << " (= 3)\n";
    std::cout << "countl_zero     = " << std::countl_zero(x)     << " (= 27 over 32-bit)\n";
    std::cout << "countr_zero     = " << std::countr_zero(x)     << " (= 1 — 最低位 1 在 index 1)\n";
    std::cout << "bit_width       = " << std::bit_width(x)       << " (= 5 — 表達 22 要 5 bit)\n";
    std::cout << "bit_floor       = " << std::bit_floor(x)       << " (= 16)\n";
    std::cout << "bit_ceil        = " << std::bit_ceil(x)        << " (= 32)\n";
    std::cout << "has_single_bit  = " << std::has_single_bit(x)  << " (= 0)\n";
    std::cout << "has_single_bit(8) = " << std::has_single_bit(8u) << " (= 1)\n";

    // rotl / rotr：循環移位（不像 << 會把推出去的 bit 丟掉）
    std::uint32_t r = std::rotl(static_cast<std::uint32_t>(0xF000'0001), 4);
    std::cout << "rotl(0xF0000001, 4) = 0x" << std::hex << r << std::dec << '\n';
    // 預期：0x0000001F（最高 4 bit 跑回最低）

    // ─────────────────────────────────────────────────────────
    // 實用範例：把容量 round up 到 2 的次方
    //   hash table、ring buffer 常用 power-of-2 容量讓 modulo 變 AND
    // ─────────────────────────────────────────────────────────
    auto roundUp = [](std::uint32_t want) {
        return std::bit_ceil(want);
    };
    std::cout << "roundUp(7)   = " << roundUp(7)   << '\n'; // 8
    std::cout << "roundUp(8)   = " << roundUp(8)   << '\n'; // 8 （恰好就是）
    std::cout << "roundUp(100) = " << roundUp(100) << '\n'; // 128

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：popcount 怎麼這麼快？
    //    A：現代 x86 有 popcnt 指令；ARM 有 cnt（NEON）。compiler 把
    //       std::popcount 直接 lower 到該指令 → 1 個 cycle。
    //
    //  Q2：countl_zero(0) 是 UB 嗎？
    //    A：定義良好。countl_zero(0) = sizeof(T)*8（全是 0 → 全是 leading
    //       zero）。但底層 CPU 指令（如 lzcnt、bsr）對 0 行為不一致，C++20
    //       已幫你把這個邊界吃掉。
    //
    //  Q3：bit_ceil(0)、bit_ceil(1)？
    //    A：bit_ceil(0) 結果是 1；bit_ceil(1) 結果是 1。不會有 UB（只是要
    //       注意值 1）。
    demo_lc_190_reverse_bits();
    demo_align_up();
#endif
    return 0;
}

#if __cplusplus >= 202002L
// =============================================================================
//  附加 1：LeetCode 190. Reverse Bits
// =============================================================================
//  題意：把 32-bit unsigned 整數的位元前後顛倒回傳。
//  例：0b00000010100101000001111010011100 (43261596)
//      → 0b00111001011110000010100101000000 (964176192)
//  思路：逐位掃描，把第 i 位塞到第 (31 - i) 位。<bit> 沒有 reverse_bits 直接
//        對應，但搭配 popcount/位移可以實作；也可分而治之做 16/8/4/2/1 配對交換。
// =============================================================================
static std::uint32_t reverseBits(std::uint32_t n) {
    // 分而治之：每次把相鄰大小相同的兩段交換
    n = (n >> 16) | (n << 16);
    n = ((n & 0xFF00FF00u) >> 8) | ((n & 0x00FF00FFu) << 8);
    n = ((n & 0xF0F0F0F0u) >> 4) | ((n & 0x0F0F0F0Fu) << 4);
    n = ((n & 0xCCCCCCCCu) >> 2) | ((n & 0x33333333u) << 2);
    n = ((n & 0xAAAAAAAAu) >> 1) | ((n & 0x55555555u) << 1);
    return n;
}
static void demo_lc_190_reverse_bits() {
    std::uint32_t n = 43261596u;
    std::cout << "[LC190] reverseBits(43261596) = " << reverseBits(n)
              << " (= 964176192)\n";
}

// =============================================================================
//  附加 2：實用範例 — align_up：把位址 / 大小對齊到 2 的次方
// =============================================================================
//  記憶體配置、SIMD buffer、檔案區塊大小都常需要「向上對齊到 align 的倍數」。
//  典型公式：(x + align - 1) & ~(align - 1)，需先確認 align 是 2 的次方。
//  std::has_single_bit 正好用來做這個前置檢查；找不到對齊 size 時 bit_ceil 可
//  直接給出最近的 2 次方容量。
// =============================================================================
static std::uint32_t align_up(std::uint32_t x, std::uint32_t align) {
    // 要求 align 是 2 的次方
    return (x + align - 1) & ~(align - 1);
}
static void demo_align_up() {
    std::uint32_t align = 16;
    std::cout << "[align_up] has_single_bit(16) = "
              << std::has_single_bit(align) << '\n';
    std::cout << "[align_up] align_up(1000, 16) = " << align_up(1000, align)
              << " (= 1008)\n";
    std::cout << "[align_up] align_up(1024, 16) = " << align_up(1024, align)
              << " (= 1024，本來就對齊)\n";
}
#endif

// 編譯: g++ -std=c++20 -Wall -Wextra 03_bit_header_cpp20.cpp -o 03_bit_header_cpp20

// === 預期輸出 ===
// x = 22 (binary 0001'0110)
// popcount        = 3 (= 3)
// countl_zero     = 27 (= 27 over 32-bit)
// countr_zero     = 1 (= 1 — 最低位 1 在 index 1)
// bit_width       = 5 (= 5 — 表達 22 要 5 bit)
// bit_floor       = 16 (= 16)
// bit_ceil        = 32 (= 32)
// has_single_bit  = 0 (= 0)
// has_single_bit(8) = 1 (= 1)
// rotl(0xF0000001, 4) = 0x1f
// roundUp(7)   = 8
// roundUp(8)   = 8
// roundUp(100) = 128
// [LC190] reverseBits(43261596) = 964176192 (= 964176192)
// [align_up] has_single_bit(16) = 1
// [align_up] align_up(1000, 16) = 1008 (= 1008)
// [align_up] align_up(1024, 16) = 1024 (= 1024，本來就對齊)
