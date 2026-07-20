// =============================================================================
//  07_lc_268_missing_number.cpp  —  LeetCode 268. Missing Number
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/operator_arithmetic  (XOR 運算)
//    - https://en.cppreference.com/w/cpp/algorithm/accumulate          (std::accumulate)
//    - https://leetcode.com/problems/missing-number/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 題意                                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  給一個包含 n 個「介於 0..n」之間互不重複整數的陣列；恰有一個數字缺失。
//  找出那個缺的數。
//
//  範例：
//      n=3, nums=[3,0,1]      → 2
//      n=2, nums=[0,1]        → 2 （缺最後一個）
//      n=9, nums=[9,6,4,2,3,5,7,0,1] → 8
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路 1：等差數列總和                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  期望總和 = n*(n+1)/2
//  實際總和 = sum(nums)
//  缺失值   = 期望 - 實際
//
//  注意 overflow：n 很大時要用 long long；本題 n ≤ 1e4 不會溢位。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路 2：XOR — 不會 overflow、相同寫法套用很多變體         │
//  └────────────────────────────────────────────────────────────┘
//
//  把 0..n 全部 XOR + 把 nums 全部 XOR 結合起來：
//      result = (0^1^2^...^n) ^ (nums[0]^nums[1]^...)
//  「在兩邊都出現」的數值會被消掉，剩下「只在 0..n 出現、不在 nums 中」
//  那個 → 答案。
//
//  優點：XOR 不會 overflow、邏輯一致；缺點：常數因子稍大、可讀性比加總差。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 兩種解法都示範                                             │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：missing_number
  - missing_number 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - XOR 解法要把索引 0..n 和陣列值都 XOR 進去；漏掉 n 是常見 off-by-one。
  - 加總公式版本要用較大型別保存總和，例如 long long，避免 n 大時 int overflow。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】LeetCode 268（缺失的數字）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 0..n 中缺一個數，有哪些解法？各自的取捨是什麼？
//     答：① XOR——res = n; 再對每個 i 做 res ^= i ^ nums[i];，出現兩次的全部抵消，剩下
//     缺失值，O(n) 時間、O(1) 空間，而且完全沒有溢位風險 ② 求和——n*(n+1)/2 - sum，
//     寫起來最短，但 n 大時中間值可能有號溢位（UB）③ 排序 O(n log n) 或 hash set
//     O(n) 額外空間。面試偏好 XOR 解，正是因為它避開了溢位問題。
//     追問：求和法要怎麼補救？（用足夠寬的無號型別如 unsigned long long 累加，或改用
//     每步「先加後減」的方式避免中間值過大——但終究不如 XOR 乾淨）
//
// 🔥 Q2. XOR 解法為什麼一定正確？
//     答：把「0..n 這 n+1 個數」與「陣列中的 n 個數」全部 XOR 在一起，除了缺失的那個
//     以外，每個值都恰好出現兩次而互相抵消（a ^ a == 0），最後與 0 XOR 保持原值
//     （a ^ 0 == a）。這個論證只依賴 XOR 的自逆性與結合交換律，與元素順序無關。
//
// ⚠️ 陷阱. 這題的三種解法都假設了什麼？改變前提還成立嗎？
//     答：都假設「值域恰好是 0..n、每個值最多出現一次、只缺一個」。若陣列可能有重複、
//     或值域不是連續的 0..n，XOR 與求和法都會直接失效（它們靠的是「配對抵消」與
//     「總和已知」）。這時只能退回 hash set 或排序後掃描。
//     為什麼會錯：把 XOR 當成通用的「找不同」工具，忽略它成立的前提是相當嚴格的配對結構。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

// 前置宣告：附加範例
static void demo_lc_461_hamming_distance();
static void demo_checksum_xor();

// 解法 1：總和差
static int missingNumberSum(const std::vector<int>& nums) {
    int n = static_cast<int>(nums.size());
    long long expected = static_cast<long long>(n) * (n + 1) / 2;
    long long actual = std::accumulate(nums.begin(), nums.end(), 0LL);
    return static_cast<int>(expected - actual);
}

// 解法 2：XOR
static int missingNumberXor(const std::vector<int>& nums) {
    int x = 0;
    int n = static_cast<int>(nums.size());
    for (int i = 0; i <= n; ++i) x ^= i;
    for (int v : nums)            x ^= v;
    return x;
}

int main() {
    std::vector<std::vector<int>> cases = {
        {3, 0, 1},
        {0, 1},
        {9, 6, 4, 2, 3, 5, 7, 0, 1},
        {0},                         // n=1, [0] → 缺 1
    };
    for (auto& c : cases) {
        std::cout << "sum=" << missingNumberSum(c)
                  << " | xor=" << missingNumberXor(c) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：總和法跟 XOR 法哪個好？
    //    A：總和法直觀；XOR 法不需要擔心 overflow 且能套用「多缺、多重複」
    //       變體。對「就只缺一個」的題目兩者都 OK。
    //
    //  Q2：能不能 in-place 不用額外加總？
    //    A：可以，「Cyclic Sort」— 把每個值 v 放到 index v 的位置；最後找
    //       第一個 nums[i] != i 的 i，就是缺的。但時間複雜度仍是 O(n)，
    //       且修改原陣列。
    //
    //  Q3：如果題目改成「找重複的數」(LC287)？
    //    A：完全不同 — 那題沒辦法直接 XOR（因為 0..n-1 會跟 nums 中的某個
    //       數消除錯誤）；要用 Floyd 龜兔賽跑或 binary search，請見另一專
    //       題。
    //
    demo_lc_461_hamming_distance();
    demo_checksum_xor();
    return 0;
}

// =============================================================================
//  附加 1：LeetCode 461. Hamming Distance（另一個經典 XOR 題）
// =============================================================================
//  題意：給整數 x、y，回傳「二進位下不同位的個數」。
//  跟 LC 268 共通的思路：「兩個東西的 XOR」是抓差異的最佳工具。
// =============================================================================
static int hammingDistance(int x, int y) {
    unsigned diff = static_cast<unsigned>(x) ^ static_cast<unsigned>(y);
    int cnt = 0;
    while (diff) { diff &= diff - 1; ++cnt; }
    return cnt;
}
static void demo_lc_461_hamming_distance() {
    std::cout << "[LC461] hamming(1,4) = " << hammingDistance(1, 4) << " (= 2)\n";
}

// =============================================================================
//  附加 2：實用範例 — XOR checksum（簡易資料完整性檢查）
// =============================================================================
//  網路 / 嵌入式常用 XOR checksum：把一串 byte 全部 XOR 起來作為校驗碼。
//  傳送：把 data + checksum 一起送出。
//  接收：再把 data + checksum 全 XOR；若無錯誤，結果為 0（成對消除）。
//  注意：XOR checksum 抓單 bit 翻轉很好，但抓不到「兩 bit 同時翻」這類錯誤；
//        實務上強度不夠時要改用 CRC32。
// =============================================================================
static std::uint8_t computeXorChecksum(const std::vector<std::uint8_t>& data) {
    std::uint8_t cs = 0;
    for (auto b : data) cs ^= b;
    return cs;
}
static void demo_checksum_xor() {
    std::vector<std::uint8_t> data{0x12, 0x34, 0x56, 0x78};
    std::uint8_t cs = computeXorChecksum(data);
    std::cout << "[checksum] data XOR = 0x" << std::hex << static_cast<int>(cs) << std::dec << '\n';
    // 把 cs 加進尾巴：再 XOR 一次，整體應為 0
    data.push_back(cs);
    std::cout << "[checksum] verify all XOR = "
              << static_cast<int>(computeXorChecksum(data)) << " (= 0 表示無錯)\n";
}
