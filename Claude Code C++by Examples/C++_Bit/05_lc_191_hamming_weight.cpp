// =============================================================================
//  05_lc_191_hamming_weight.cpp  —  LeetCode 191. Number of 1 Bits
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/numeric/popcount    (C++20 std::popcount)
//    - https://en.cppreference.com/w/cpp/header/bit          (<bit> 標頭)
//    - https://leetcode.com/problems/number-of-1-bits/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 題意                                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  輸入：32-bit unsigned integer
//  輸出：它的二進位表示有多少個 1（"Hamming weight" / popcount）
//
//  範例：
//      n = 11   (0000 1011)        → 3
//      n = 128  (1000 0000)        → 1
//      n = -3 (4294967293) (補碼)  → 31
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 解法 1：逐位掃描                                           │
//  └────────────────────────────────────────────────────────────┘
//
//  最直觀：對 32 個位置一個一個檢查 (n & 1)，再 n >>= 1。
//  時間複雜度：O(32)。
//
//  缺點：即使 n 中只有 1 個位元是 1，還是要跑 32 次。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 解法 2：Brian Kernighan — 用 n & (n-1) 拔最低位 1          │
//  └────────────────────────────────────────────────────────────┘
//
//  每次迴圈把最低位的 1 翻 0、計數一次。迴圈次數 = n 中 1 的個數。
//  最壞 O(32)，平均更快（n 中 1 較少時）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 解法 3：std::popcount (C++20)                              │
//  └────────────────────────────────────────────────────────────┘
//
//  C++20 直接呼叫 CPU popcnt 指令，1 cycle 完成。
//
// =============================================================================

/*
補充筆記：hamming_weight
  - hamming_weight 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - 若使用 while (n) 搭配 n &= n - 1，n 必須是 unsigned，避免負數和符號位讓迴圈語意變複雜。
  - C++20 的 std::popcount 定義在 <bit>，回傳 set bit 數，實務上比手寫迴圈更清楚。
*/
#include <cstdint>
#include <iostream>
#include <vector>

#if __cplusplus >= 202002L
#  include <bit>
#endif

// 前置宣告：附加範例
static void demo_lc_461_hamming_distance();
static void demo_popcount_array_stat();

// 解法 1：逐位
static int hamming1(std::uint32_t n) {
    int cnt = 0;
    for (int i = 0; i < 32; ++i) {
        cnt += (n >> i) & 1u;
    }
    return cnt;
}

// 解法 2：Brian Kernighan
static int hamming2(std::uint32_t n) {
    int cnt = 0;
    while (n) {
        n &= (n - 1);   // 拔掉最低位的 1
        ++cnt;
    }
    return cnt;
}

// 解法 3：std::popcount（C++20）
#if __cplusplus >= 202002L
static int hamming3(std::uint32_t n) {
    return std::popcount(n);
}
#endif

int main() {
    std::uint32_t cases[] = {11u, 128u, 0u, 0xFFFFFFFFu};
    for (auto n : cases) {
        std::cout << "n=" << n
                  << " | hamming1=" << hamming1(n)
                  << " | hamming2=" << hamming2(n)
#if __cplusplus >= 202002L
                  << " | popcount=" << hamming3(n)
#endif
                  << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 LeetCode 上「-3」會變 4294967293？
    //    A：題目說「unsigned integer」，但 C# / Java 用 signed int 接受
    //       輸入；-3 的補碼表示就是 0xFFFFFFFD = 4294967293。它有 31 個 1。
    //
    //  Q2：為什麼 (n >> i) & 1 比 n & (1 << i) 安全？
    //    A：(1 << i) 在 i = 31 對 signed int 上會 UB（C++20 起對 unsigned
    //       無符號才完全 well-defined）。先把 n 右移再 & 1 就避開了這個問題。
    //
    //  Q3：手寫 Kernighan 跟 std::popcount 哪個放面試？
    //    A：兩個都會比較好。面試官通常想看你對 n & (n-1) 的理解；接著問你
    //       「現代 C++ 怎麼一行解」就可以丟出 std::popcount。
    //
    demo_lc_461_hamming_distance();
    demo_popcount_array_stat();
    return 0;
}

// =============================================================================
//  附加 1：LeetCode 461. Hamming Distance（hamming weight 的延伸）
// =============================================================================
//  題意：給整數 x、y，回傳「二進位下不同位的個數」。
//  關鍵觀察：x ^ y 把相同位歸 0、不同位歸 1，問題化簡為「對 x^y 做 popcount」。
//  本題正好把 hamming_weight 當積木使用。
// =============================================================================
static int hammingDistance(int x, int y) {
    unsigned diff = static_cast<unsigned>(x) ^ static_cast<unsigned>(y);
    return hamming2(diff); // 重用本檔的 Brian Kernighan 版本
}
static void demo_lc_461_hamming_distance() {
    std::cout << "[LC461] hamming(1,4) = " << hammingDistance(1, 4) << " (= 2)\n";
    std::cout << "[LC461] hamming(0xFFFF,0) = " << hammingDistance(0xFFFF, 0) << " (= 16)\n";
}

// =============================================================================
//  附加 2：實用範例 — 統計 byte 陣列的 set bit 總數
// =============================================================================
//  工作上常做：把 bitmap / bitset 序列化到 byte buffer 後，需要查總共有幾
//  個 bit 為 1（例如：使用率、布隆過濾器密度估計）。
//  逐 byte popcount 累加即可；如果用 std::popcount 就一行解。
// =============================================================================
static int popcountArray(const std::vector<std::uint8_t>& buf) {
    int total = 0;
    for (auto b : buf) total += hamming2(b);
    return total;
}
static void demo_popcount_array_stat() {
    std::vector<std::uint8_t> buf{0xFF, 0x0F, 0xAA, 0x00}; // 8 + 4 + 4 + 0 = 16
    std::cout << "[popcount_array] total set bits = "
              << popcountArray(buf) << " (= 16)\n";
}
