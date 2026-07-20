// =============================================================================
//  11_lc_78_subsets.cpp  —  LeetCode 78. Subsets（用 bitmask 枚舉）
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/operator_arithmetic  (位元運算子)
//    - https://en.cppreference.com/w/cpp/container/vector              (std::vector)
//    - https://leetcode.com/problems/subsets/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 題意                                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  給互不相同的整數陣列 nums，回傳「所有可能的子集合」(power set)。
//
//  範例：nums = [1,2,3]
//        → [[], [1], [2], [1,2], [3], [1,3], [2,3], [1,2,3]]
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路：bitmask 枚舉                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  n 個元素 → 共有 2^n 個子集合。每個子集合可以用一個「n 位二進位數」表
//  示：第 i 位是 1 → 第 i 個元素被選入。
//
//      mask = 000   → []
//      mask = 001   → [nums[0]]
//      mask = 010   → [nums[1]]
//      mask = 011   → [nums[0], nums[1]]
//      ...
//      mask = 111   → [nums[0], nums[1], nums[2]]
//
//  迭代從 0 到 2^n - 1 即可枚舉全部子集合，沒有遞迴、沒有回溯，邏輯極簡。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼這寫法值得學？                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  bitmask 枚舉是「狀態壓縮 DP」的入門 — 把「集合的選擇狀態」用整數表達
//  後，許多問題（旅行推銷員、彩色塗格、背包子集...）都能用同樣的迴圈結構
//  解決。
//
//  時間複雜度：O(n · 2^n)（每個 mask 內要花 O(n) 把元素寫進子集合）
//
// =============================================================================

/*
補充筆記：subsets
  - subsets 屬於位元操作；位元運算適合旗標、遮罩、集合狀態、低階資料格式和某些整數技巧。
  - 位移前要確認位數合法；位移負數或位移量大於等於型別寬度會造成未定義行為。
  - signed integer 的位元表示和右移行為有細節；做遮罩和位元技巧時通常使用 unsigned 型別較清楚。
  - x & (x - 1) 可清掉最低位的 1，常用於計算 set bits 或判斷 power of two；但 x=0 要另外處理。
  - C++20 <bit> 提供 popcount、has_single_bit、rotl、bit_width 等標準工具，能取代許多手寫技巧。
  - bit_cast 是按位元重新解讀且要求大小相同與 trivially copyable；它不是任意型別轉換。
  - 若 n 接近 int bit 數，1 << n 會溢位或未定義；枚舉上限應使用 size_t{1} << n。
  - bitmask 產生的 subset 順序通常不是字典序；若輸出順序有要求，需要額外排序或改用回溯。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】LeetCode 78（bitmask 枚舉子集）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 怎麼用位運算枚舉一個集合的所有子集？
//     答：n 個元素共 2^n 個子集，用 0 .. (1<<n)-1 的每個整數當 bitmask，第 i 位為 1
//     就代表選了第 i 個元素：
//     for (int mask = 0; mask < (1 << n); ++mask)
//         for (int i = 0; i < n; ++i) if (mask >> i & 1) sub.push_back(nums[i]);
//     時間 O(n · 2^n)。內層可改成只走 set bit：
//     while (m) { int i = std::countr_zero(static_cast<unsigned>(m)); ...; m &= m - 1; }
//     追問：n 最多能多大？（1 << n 用 int 時 n 最多 30——n == 31 就會踩到有號左移溢位；
//     更大要用 1ll << n 或無號型別。這正是「位遮罩一律用無號字面量」的實際理由）
//
// 🔥 Q2. 怎麼枚舉某個 mask 的所有子遮罩（submask）？複雜度是多少？
//     答：for (int s = mask; s; s = (s - 1) & mask) { ... }——(s-1) & mask 會產生字典序
//     上一個子遮罩，遞減枚舉，每步只要兩個運算（空集需另外處理，因為 s == 0 時迴圈終止）。
//     複雜度的漂亮結論是：對所有 mask 都枚舉一次其子遮罩，總量是 3^n 而不是 4^n——
//     每個 bit 在 (mask, submask) 配對中恰有三種狀態：不在 mask、在 mask 但不在 submask、
//     兩者都在。這是 bitmask DP（TSP、集合覆蓋）的核心。
//
// ⚠️ 陷阱. 子遮罩枚舉為什麼不能直接寫 s = s - 1？
//     答：那會枚舉到「不在 mask 內」的位元組合，產生根本不是子集的東西。必須用
//     (s - 1) & mask 把減法借位產生的多餘位元遮掉。另外若忘了處理 s == 0 的終止條件，
//     (0 - 1) & mask 會等於 mask，直接變成無窮迴圈。
//     為什麼會錯：把它想成單純的「倒數」，忽略了要一直維持「s 是 mask 的子集」這個不變式。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

// 前置宣告：附加範例
static void demo_lc_89_gray_code();
static void demo_role_permission_subset();

static std::vector<std::vector<int>> subsets(const std::vector<int>& nums) {
    int n = static_cast<int>(nums.size());
    std::vector<std::vector<int>> result;
    result.reserve(1u << n);

    for (int mask = 0; mask < (1 << n); ++mask) {
        std::vector<int> sub;
        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {        // 第 i 位是 1 → 選 nums[i]
                sub.push_back(nums[i]);
            }
        }
        result.push_back(std::move(sub));
    }
    return result;
}

int main() {
    std::vector<int> nums{1, 2, 3};
    auto all = subsets(nums);
    std::cout << "subsets count = " << all.size() << " (= 2^3 = 8)\n";
    for (auto& s : all) {
        std::cout << "[ ";
        for (int x : s) std::cout << x << ' ';
        std::cout << "]\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：mask 跟 1 << i 的型別陷阱？
    //    A：n ≥ 30 時 (1 << n) 會 overflow int。寫 (1u << n) 或 (1LL << n)
    //       讓 shift 在較大型別上做。本題 n ≤ 10 → 2^10 = 1024，不會出事。
    //
    //  Q2：怎麼遍歷 mask 的所有 1-位元（不空跑 i 從 0..n-1）？
    //    A：用「Brian Kernighan」風：
    //         for (int m = mask; m; m &= m - 1) {
    //             int idx = __builtin_ctz(m);   // 最低位 1 的位置
    //             ...
    //         }
    //       適合 mask 大多稀疏的情境。
    //
    //  Q3：要按「字典序」輸出怎麼辦？
    //    A：題目沒要求順序；如果要，把 result 整體排序，或改用回溯法產生
    //       字典序輸出。bitmask 法的順序是「按 mask 遞增」，跟字典序不同。
    //
    demo_lc_89_gray_code();
    demo_role_permission_subset();
    return 0;
}

// =============================================================================
//  附加 1：LeetCode 89. Gray Code  // 難度: medium
// =============================================================================
//  題意：產生 n 位元的 Gray code 序列，相鄰兩個只差一個 bit，第一個是 0。
//  解法：第 i 個 = i ^ (i >> 1)，2^n 個逐個產生即可。
//  跟 LC 78 的連結：兩題都是「按 mask 枚舉 2^n 種狀態」 — LC 78 不在意順序，
//  LC 89 要求「相鄰只差一 bit」，所以用 XOR 把線性序列「重新編號」。
// =============================================================================
static std::vector<int> grayCode(int n) {
    std::vector<int> ans;
    int total = 1 << n;
    ans.reserve(total);
    for (int i = 0; i < total; ++i) ans.push_back(i ^ (i >> 1));
    return ans;
}
static void demo_lc_89_gray_code() {
    auto g = grayCode(3);
    std::cout << "[LC89] gray(3) =";
    for (int x : g) std::cout << ' ' << x;
    std::cout << "  (相鄰只差一 bit)\n";
}

// =============================================================================
//  附加 2：實用範例 — 角色權限枚舉
// =============================================================================
//  工作上：一個使用者可能有 N 個權限旗標（READ、WRITE、ADMIN...），
//  測試 / 列舉「所有可能的權限組合」就是「枚舉 N bit 的所有 mask」。
//  下面範例：3 個權限位元，列出所有 8 種組合的名稱集合。
// =============================================================================
static void demo_role_permission_subset() {
    const char* names[] = {"READ", "WRITE", "ADMIN"};
    int n = 3;
    std::cout << "[permission] 所有 " << (1 << n) << " 種權限組合:\n";
    for (int mask = 0; mask < (1 << n); ++mask) {
        std::cout << "  mask=" << mask << " -> { ";
        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) std::cout << names[i] << ' ';
        }
        std::cout << "}\n";
    }
}
