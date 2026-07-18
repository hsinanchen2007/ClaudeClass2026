// =============================================================================
//  04_bit_cast.cpp  —  std::bit_cast (C++20) 安全的 bit-pattern 轉型
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/bit_cast
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼需要 bit_cast？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  常見需求：「把 float 看成 32-bit 整數」、「把 double 看成 uint64_t」
//  ── 用來看浮點數的內部表示，或做 hash、序列化、優化技巧。
//
//  老作法（C++17 之前）：
//
//    A) reinterpret_cast<std::uint32_t&>(f)            ❌ 違反 strict aliasing
//    B) memcpy(&i, &f, 4)                              ✅ 標準允許但醜
//    C) union { float f; uint32_t i; }                 ❌ C 允許、C++ 是 UB
//
//  C++20 引入 std::bit_cast：
//
//      std::uint32_t bits = std::bit_cast<std::uint32_t>(f);
//
//  優點：
//   * 沒有 UB（標準保證 byte-by-byte 拷貝）
//   * constexpr（編譯期就能算）
//   * 編譯器能優化成「零成本」（多半就是一個 mov 指令）
//
//  限制：
//   * sizeof(From) == sizeof(To)
//   * 兩邊都必須是 trivially_copyable
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、IEEE 754 float 的解剖                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  32-bit float：
//
//    s eeeeeeee fffffffffffffffffffffff
//    │    │              │
//    │    │              └─ 23-bit 尾數 (mantissa)
//    │    └──────────────── 8-bit 指數 (exponent, 偏移 127)
//    └───────────────────── 1-bit 符號
//
//  例：1.0f
//    s = 0
//    e = 127     (偏移後)
//    f = 0
//    bits = 0x3F800000
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：float → uint32_t 看 IEEE 754 結構
//   * Demo 2：以 bit pattern 翻轉符號位 → 變相反數
//   * Demo 3：和 memcpy 寫法的對照
// =============================================================================

/*
補充筆記：bit_cast
  - bit_cast 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - std::bit_cast 複製物件的位元表示到另一個同大小型別，不會改變來源物件生命週期。
  - bit_cast 要求來源和目標都是 trivially copyable 且大小相同；它不是 reinterpret_cast 的萬用安全版。
  - 浮點數 bit_cast 成整數常用於觀察 IEEE 754 表示，但不能假設所有平台浮點格式都完全一樣。
*/
#include <cstdint>
#include <cstring>
#include <iostream>

#if __cplusplus >= 202002L
#  include <bit>
#endif

#if __cplusplus >= 202002L
// 前置宣告：附加範例
static void demo_lc_762_prime_set_bits();
static void demo_float_is_nan();
#endif

int main() {
#if __cplusplus < 202002L
    std::cout << "[skipped] need C++20\n";
    return 0;
#else
    // ─────────────────────────────────────────────────────────
    // Demo 1：float 1.0 的 bit pattern
    // ─────────────────────────────────────────────────────────
    float f = 1.0f;
    auto bits = std::bit_cast<std::uint32_t>(f);
    std::cout << "[Demo1] float 1.0 → 0x"
              << std::hex << bits << std::dec << " (預期 0x3f800000)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 bit_cast 翻符號位
    //   翻第 31 位（最高位）讓正負號反轉
    // ─────────────────────────────────────────────────────────
    float g = 3.14f;
    std::uint32_t gbits = std::bit_cast<std::uint32_t>(g);
    gbits ^= 0x8000'0000u;             // 反轉符號位
    float neg_g = std::bit_cast<float>(gbits);
    std::cout << "[Demo2] flip sign of 3.14 → " << neg_g << " (預期 -3.14)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：對照 memcpy 寫法（C++17 也可用）
    // ─────────────────────────────────────────────────────────
    float h = 2.5f;
    std::uint32_t hbits;
    std::memcpy(&hbits, &h, sizeof(h));
    std::cout << "[Demo3] memcpy(2.5f) bits = 0x"
              << std::hex << hbits << std::dec
              << " (vs bit_cast " << std::hex
              << std::bit_cast<std::uint32_t>(h) << std::dec << ")\n";

    // ─────────────────────────────────────────────────────────
    // Demo 4：double → uint64_t
    // ─────────────────────────────────────────────────────────
    double d = -0.0;
    auto dbits = std::bit_cast<std::uint64_t>(d);
    std::cout << "[Demo4] -0.0 bits = 0x"
              << std::hex << dbits << std::dec
              << " (符號位是 1，其它都 0)\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 reinterpret_cast 會 UB？
    //    A：C++ 的「strict aliasing」規則：只能透過「相容型別 / char / byte」
    //       的指標讀取一個物件。透過 uint32_t* 讀 float 物件，編譯器在 -O2
    //       下可能假設兩者沒互相影響、產生奇怪優化。
    //
    //  Q2：bit_cast 比 memcpy 快嗎？
    //    A：執行成本一樣 — 編譯器會把兩者都優化成同樣的 mov / load。bit_cast
    //       的優勢在於 (a) 強制檢查大小、(b) constexpr、(c) 表達意圖更清楚。
    //
    //  Q3：兩個物件大小不一樣怎麼辦？
    //    A：bit_cast 編譯失敗。如果你真的想做不同大小的 bit pattern 操作，
    //       要先 memcpy 進一個中介 buffer，標準沒提供「截斷或補 0」的 cast。
    demo_lc_762_prime_set_bits();
    demo_float_is_nan();
#endif
    return 0;
}

#if __cplusplus >= 202002L
// =============================================================================
//  附加 1：LeetCode 762. Prime Number of Set Bits in Binary Representation
// =============================================================================
//  題意：在 [left, right] 範圍內，回傳「二進位中 set bit 個數為質數」的數的個數。
//  思路：對每個 i 用 std::popcount 計位元 1 個數，再查它是否為質數。
//  範圍上限 <= 1e6 → popcount 最多 20，質數預先列出即可。
// =============================================================================
static int countPrimeSetBits(int left, int right) {
    auto isPrime = [](int p) {
        // 範圍 [2..20] 內質數：2, 3, 5, 7, 11, 13, 17, 19
        constexpr std::uint32_t primeMask =
            (1u<<2) | (1u<<3) | (1u<<5) | (1u<<7) |
            (1u<<11) | (1u<<13) | (1u<<17) | (1u<<19);
        return p >= 2 && p < 32 && ((primeMask >> p) & 1u);
    };
    int cnt = 0;
    for (int x = left; x <= right; ++x) {
        if (isPrime(std::popcount(static_cast<unsigned>(x)))) ++cnt;
    }
    return cnt;
}
static void demo_lc_762_prime_set_bits() {
    std::cout << "[LC762] [6,10] = " << countPrimeSetBits(6, 10) << " (= 4)\n";
    std::cout << "[LC762] [10,15] = " << countPrimeSetBits(10, 15) << " (= 5)\n";
}

// =============================================================================
//  附加 2：實用範例 — 用 bit_cast 檢測 NaN
// =============================================================================
//  IEEE 754 規定 NaN 的指數位全 1、尾數非 0。雖然 std::isnan 已有，但這個範
//  例展示 bit_cast 在「分析浮點 bit pattern」的標準工程做法。
//  float 的 bit 配置：1 sign + 8 exponent + 23 mantissa。
//    指數位 mask = 0x7F800000，尾數 mask = 0x007FFFFF。
// =============================================================================
static bool isNanByBits(float f) {
    auto bits = std::bit_cast<std::uint32_t>(f);
    bool expAllOne = (bits & 0x7F800000u) == 0x7F800000u;
    bool mantNonZero = (bits & 0x007FFFFFu) != 0u;
    return expAllOne && mantNonZero;
}
static void demo_float_is_nan() {
    float nan_val = std::bit_cast<float>(0x7FC00000u); // quiet NaN
    float inf_val = std::bit_cast<float>(0x7F800000u); // +inf（指數全 1、尾數 0）
    float normal  = 1.5f;
    std::cout << "[is_nan_bits] nan -> " << isNanByBits(nan_val)
              << ", inf -> " << isNanByBits(inf_val)
              << ", 1.5 -> " << isNanByBits(normal) << '\n';
}
#endif
