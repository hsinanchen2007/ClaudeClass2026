// =============================================================================
//  08_lc_338_counting_bits.cpp  —  LeetCode 338. Counting Bits
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/numeric/popcount   (C++20 std::popcount)
//    - https://en.cppreference.com/w/cpp/language/operator_arithmetic (位元運算子)
//    - https://leetcode.com/problems/counting-bits/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 題意                                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  輸入：非負整數 n
//  輸出：長度 n+1 的陣列 ans，ans[i] 是 i 的 popcount
//
//  範例：
//      n=2 → [0,1,1]
//      n=5 → [0,1,1,2,1,2]
//
//  限制：要求 O(n) 時間、O(1) 額外空間（不算輸出）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路 1：對每個 i 個別 popcount                              │
//  └────────────────────────────────────────────────────────────┘
//
//  時間 O(n log n)。可行但沒達到 O(n) 要求。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路 2：DP — 用之前算過的結果推當前                        │
//  └────────────────────────────────────────────────────────────┘
//
//  關鍵觀察：i 跟 (i >> 1) 的關係 —
//
//      ans[i] = ans[i >> 1] + (i & 1)
//
//  證明：「右移一位」把最低位丟掉、其餘位數的 1 個數不變。所以 i 的 1
//  個數 = (i>>1) 的 1 個數 + i 的最低位是不是 1。
//
//  時間 O(n)，因為每個 i 只做一次 O(1) 計算。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路 3：DP — 用 Brian Kernighan 推                          │
//  └────────────────────────────────────────────────────────────┘
//
//      ans[i] = ans[i & (i - 1)] + 1
//
//  原理：i & (i-1) 把 i 的最低位 1 拔掉，所以 ans[i & (i-1)] 是「比 i 少
//  一個 1 的 ans」 → 再 +1 即得 ans[i]。
//
// =============================================================================

/*
補充筆記：counting_bits
  - counting_bits 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - DP 表應從 0 開始，dp[0] = 0 是所有遞推的基底。
  - 時間複雜度 O(n)，空間 O(n)；若只查單一數字，直接 popcount 不需要建立整張表。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】LeetCode 338（Counting Bits DP）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 求 0..n 每個數的 popcount，有哪些 O(n) 遞推式？
//     答：三種經典寫法：① 看最低位——dp[i] = dp[i >> 1] + (i & 1)（去掉最低位後的結果
//     已算過）② 看最高位——dp[i] = dp[i - highestPowerOfTwo(i)] + 1 ③ Brian Kernighan
//     ——dp[i] = dp[i & (i - 1)] + 1，最簡潔。三者都是 O(n)，優於逐個呼叫 popcount 的
//     O(n log n)（若沒有硬體指令的話）。
//     追問：dp[i & (i-1)] + 1 為什麼正確？（i & (i-1) 是把 i 清掉一個 set bit 的結果，
//     它必定小於 i，所以在遞推順序中已經算過；而它比 i 恰好少一個 1）
//
// 🔥 Q2. 既然 C++20 有 std::popcount，這題的 DP 還有意義嗎？
//     答：有。std::popcount 在有 POPCNT 指令的機器上是單指令，直接迴圈呼叫也是 O(n)
//     且常數更小——所以實務上就用它。DP 的價值在於：① 展示「用已算過的子問題」這個
//     思路 ② 在沒有硬體 popcount 的平台上仍然快 ③ 面試官想看的是你能不能找出
//     i 與 i >> 1 或 i & (i-1) 之間的關係。
//
// ⚠️ 陷阱. dp[i] = dp[i >> 1] + (i & 1) 中，i >> 1 對有號 int 安全嗎？
//     答：本題 i 從 0 遞增到 n，都是非負數，所以沒問題。但這個寫法一旦被複製到「可能
//     出現負數」的情境就會出錯：有號負數右移在 C++17 及以前是 implementation-defined、
//     C++20 起才規定為算術右移（補符號位），無論哪個版本，對負數右移都不會是你想要的
//     「邏輯右移」。習慣上位運算一律用無號型別，就能完全避開這類討論。
//     為什麼會錯：在一個「剛好都非負」的題目裡養成的寫法，被當成通用慣例帶走。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

// 前置宣告：附加範例
static void demo_lc_477_total_hamming_distance();
static void demo_subset_size_distribution();

// 思路 2：i = (i>>1) + (i&1)
static std::vector<int> countingBitsShift(int n) {
    std::vector<int> ans(n + 1, 0);
    for (int i = 1; i <= n; ++i) {
        ans[i] = ans[i >> 1] + (i & 1);
    }
    return ans;
}

// 思路 3：i = (i & (i-1)) + 1
static std::vector<int> countingBitsKernighan(int n) {
    std::vector<int> ans(n + 1, 0);
    for (int i = 1; i <= n; ++i) {
        ans[i] = ans[i & (i - 1)] + 1;
    }
    return ans;
}

static void print(const std::vector<int>& v) {
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';
}

int main() {
    int n = 8;
    std::cout << "shift     n=" << n << ": ";  print(countingBitsShift(n));
    std::cout << "kernighan n=" << n << ": ";  print(countingBitsKernighan(n));

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：兩個 DP 解的差別？
    //    A：思路 2 看右移、跟二進位結構直觀對應；思路 3 用 Brian Kernighan
    //       的觀察，跟其他位元題（如 LC191）共享思維。兩者都是 O(n)。
    //
    //  Q2：為什麼不直接 std::popcount？
    //    A：可以！C++20：
    //         for (int i = 0; i <= n; ++i) ans[i] = std::popcount(unsigned(i));
    //       一行解。但 LeetCode 介面題目本身想考的就是 DP 思路，理解 DP
    //       推導比直接用 intrinsic 更有教學價值。
    //
    //  Q3：能省記憶體嗎？
    //    A：題目要求回傳整個陣列，所以 O(n) 空間是必須。但程式中沒有額外
    //       的暫存陣列 — 已經是最小空間。
    //
    demo_lc_477_total_hamming_distance();
    demo_subset_size_distribution();
    return 0;
}

// =============================================================================
//  附加 1：LeetCode 477. Total Hamming Distance  // 難度: medium
// =============================================================================
//  題意：給陣列 nums，回傳所有「兩兩元素」漢明距離的總和。
//  暴力 O(n^2 * 32) 會超時；關鍵觀察是「按位獨立統計」：
//    對第 k 位，假設有 c 個元素該位為 1，有 (n - c) 個為 0；
//    這一位對總距離的貢獻 = c * (n - c)。
//  時間 O(32 n)，空間 O(1)。
// =============================================================================
static int totalHammingDistance(const std::vector<int>& nums) {
    int n = static_cast<int>(nums.size());
    int total = 0;
    for (int k = 0; k < 32; ++k) {
        int ones = 0;
        for (int x : nums) ones += (static_cast<unsigned>(x) >> k) & 1u;
        total += ones * (n - ones);
    }
    return total;
}
static void demo_lc_477_total_hamming_distance() {
    std::vector<int> nums{4, 14, 2};
    std::cout << "[LC477] total hamming [4,14,2] = "
              << totalHammingDistance(nums) << " (= 6)\n";
}

// =============================================================================
//  附加 2：實用範例 — 統計 0..n 之間「位元 1 個數的分布」
// =============================================================================
//  常用於：分析 hash 結果均勻性、估算 bitmap 平均密度、debug 觀察。
//  直接重用 countingBitsShift，把結果聚合成「k 個 1 出現了幾次」。
// =============================================================================
static void demo_subset_size_distribution() {
    int n = 15; // 0..15
    auto bits = countingBitsShift(n);
    int dist[5] = {0, 0, 0, 0, 0}; // 最多 4 個 1（log2(15)+1）
    for (int x : bits) dist[x]++;
    std::cout << "[distribution] 0..15 中 popcount 分布: ";
    for (int k = 0; k <= 4; ++k) std::cout << k << "->" << dist[k] << ' ';
    std::cout << '\n';
}
