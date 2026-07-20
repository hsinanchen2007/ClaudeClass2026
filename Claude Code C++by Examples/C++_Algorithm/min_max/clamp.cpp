// ============================================================
// std::clamp    (C++17 起)
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/clamp
//   * https://cplusplus.com/reference/algorithm/clamp/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::clamp 解的問題:
//
//   「把 v 限制在 [lo, hi] 範圍內 — 如果超出就拉回邊界。」
//
// 三種情況:
//
//   * v < lo  → lo
//   * v > hi  → hi
//   * 否則    → v (本身)
//
// 等價於:
//
//   std::max(lo, std::min(hi, v))
//
// 但 std::clamp 一行寫完,語意更清晰。C++17 之前都用 max + min 組合,
// 現在直接用 clamp 就好。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、典型應用 — 「邊界裁剪」(saturation)                     │
// └────────────────────────────────────────────────────────────┘
//
//   * UI 字級限制:8 ≤ size ≤ 72
//   * 遊戲血量:0 ≤ HP ≤ MAX_HP
//   * 影像 RGB 值:0 ≤ pixel ≤ 255
//   * 音量旋鈕:0 ≤ volume ≤ 100
//   * 任何「使用者輸入要落在合法範圍」的情境
//
// 寫成 clamp 比寫 if (v < lo) v = lo; else if (v > hi) v = hi; 簡潔太多。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、生命期陷阱 (與 min/max 一樣)                            │
// └────────────────────────────────────────────────────────────┘
//
// std::clamp 也回傳 const T& — 一樣有 dangling 風險:
//
//   const auto& m = std::clamp(x, 0, 10);   // 不安全 (若 x 是暫存)
//   auto m = std::clamp(x, 0, 10);           // 安全
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、lo > hi 是 UB                                          │
// └────────────────────────────────────────────────────────────┘
//
// 使用前要確保 lo ≤ hi,否則行為未定義。
// 這個前提通常是程式設計常識,但動態傳入的範圍要小心驗證。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class T>
//   constexpr const T& clamp(const T& v, const T& lo, const T& hi);
//
//   template <class T, class Compare>
//   constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(1) (常數 — 1~2 次比較)
//   空間: O(1)
//
//   1. lo > hi → UB,使用前驗證。
//   2. 用 by-value 接,避免生命期陷阱。
//   3. 三個型別必須一致 (或可隱式轉換)。
//   4. 對浮點數 NaN 行為依平台,小心。
//
// ============================================================

/*
補充筆記：std::clamp
  - std::clamp 把值限制在 low 到 high 之間；它不會修正反過來的上下界。
  - low 必須不大於 high，否則行為不符合你想像的「夾住」。
  - clamp 可能回傳傳入物件的 reference，不要把暫時值的 reference 長期保存。
  - std::clamp 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::clamp
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::clamp 是什麼？哪個標準加入的？和 min/max 組合等價嗎？
//     答：`std::clamp(v, lo, hi)` 是 **C++17** 加入的，把 v 夾在 [lo, hi]：
//         v < lo 回 lo、hi < v 回 hi、否則回 v 本身。語意上等價於
//         `std::max(lo, std::min(v, hi))`，但只寫一次 v、意圖明確、
//         至多 2 次比較，而且是 constexpr。
//     追問：C++17 之前怎麼寫？(就是本檔範例 5 的 `std::min(hi, std::max(lo, x))`
//           ——正確，但讀者得自己推敲那是在做 clamp)
//
// 🔥 Q2. `std::clamp(v, 10, 0)`（lo > hi）會怎樣？
//     答：**UB**。標準把 `!(hi < lo)` 訂為前置條件（precondition），不是執行期
//         檢查——編譯器不會報錯，libstdc++ 也不會丟例外，你只會拿到無意義的結果。
//         範圍若來自設定檔或使用者輸入，必須自己先驗證 lo <= hi。
//     追問：那浮點 NaN 呢？(NaN < lo 與 hi < NaN 都是 false，兩個分支都不成立
//           → 回傳 v 本身，也就是 NaN 原封不動穿過去；想過濾要自己先判斷)
//
// ⚠️ 陷阱. `const auto& x = std::clamp(compute(), 0, 10);` 安全嗎？
//     答：**不安全**。clamp 回傳 `const T&`，可能是對 v（也就是 compute() 那個
//         暫存值）的 reference；全表達式結束後暫存物銷毀，x 成為 dangling。
//         改成 `auto x = std::clamp(compute(), 0, 10);`（by value）就安全。
//     為什麼會錯：很多人記得「clamp 會回傳邊界值 lo 或 hi」，就假設回來的一定是
//         那兩個活得夠久的變數；但 v 落在範圍內時回傳的正是 v 自己——
//         恰恰是最短命的那一個。「回傳 reference 而非 value」是為了省下大型物件的
//         複製，代價就是這個 lifetime 陷阱。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 基本 clamp ---
    std::cout << "clamp(5, 0, 10) = "  << std::clamp(5, 0, 10)  << '\n';
    std::cout << "clamp(-3, 0, 10) = " << std::clamp(-3, 0, 10) << '\n';
    std::cout << "clamp(99, 0, 10) = " << std::clamp(99, 0, 10) << '\n';

    // --- 範例 2: 浮點 ---
    std::cout << "clamp(0.7, 0.0, 1.0) = "
              << std::clamp(0.7, 0.0, 1.0) << '\n';

    // --- 範例 3: 字串 (字典序) ---
    std::cout << "clamp(\"m\", \"a\", \"f\") = "
              << std::clamp(std::string("m"),
                            std::string("a"),
                            std::string("f")) << '\n';

    // --- 範例 4: 自訂 comp — 用「絕對值」做比較 ---
    int v = -20, lo = -5, hi = 5;
    auto by_abs = [](int a, int b){ return std::abs(a) < std::abs(b); };
    std::cout << "clamp(-20, -5, 5, by_abs) = "
              << std::clamp(v, lo, hi, by_abs) << '\n';

    // --- 範例 5: 與 min/max 等價 ---
    int x = 12, mn = 0, mx = 10;
    int eq = std::min(mx, std::max(mn, x));
    std::cout << "manual clamp = " << eq << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_2419_longest_max_and_subarray();
    void practical_font_size_clamp();
    void practical_hp_clamp();
    void leetcode_1759_count_homogeneous_capped();
    void practical_volume_slider_clamp();
    leetcode_2419_longest_max_and_subarray();
    practical_font_size_clamp();
    practical_hp_clamp();
    leetcode_1759_count_homogeneous_capped();
    practical_volume_slider_clamp();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 2419 概念:最長最大值連續段 (Longest Subarray With Maximum Bitwise AND)
// ----------------------------------------------------------------
// 題目:找最大 AND 對應的「最長連續子陣列長度」 — 等價於找「最大值連續段」。
//
// 為什麼用 std::clamp:
//   主要邏輯是 cur 累計 + ans 取 max;這裡示範若有業務上限制
//   「單段最長」時,用 clamp 把 cur 卡在 [0, CAP] 內 — 一行表達。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2419_longest_max_and_subarray() {
    std::vector<int> nums{1, 2, 3, 3, 2, 2};
    int mx = *std::max_element(nums.begin(), nums.end());
    int cur = 0, ans = 0;
    const int CAP = 100;
    for (int x : nums) {
        cur = (x == mx) ? (cur + 1) : 0;
        cur = std::clamp(cur, 0, CAP);
        ans = std::max(ans, cur);
    }
    std::cout << "LC2419: max=" << mx << ", longest run = " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例 1:UI 字級限制在 [8, 72]
// ----------------------------------------------------------------
// 場景:UI 設定中,使用者輸入的字級要被限制在合法範圍。
//
// 為什麼用 std::clamp:
//   一行寫完「邊界裁剪」邏輯,可讀性極高 — 比 if/else 短得多。
void practical_font_size_clamp() {
    std::vector<int> inputs{4, 8, 16, 24, 99, 72, 100};
    std::cout << "font sizes: ";
    for (int v : inputs) std::cout << std::clamp(v, 8, 72) << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:遊戲角色 HP 不超過上限,也不低於 0
// ----------------------------------------------------------------
// 場景:HP 受傷或補血後,要限制在 [0, max_hp] 範圍內。
//
// 為什麼用 std::clamp:
//   把「不超過 max_hp」和「不低於 0」一行寫完,程式語意清晰。
void practical_hp_clamp() {
    int max_hp = 100;
    int hp = 80;
    int heal = 50;
    hp = std::clamp(hp + heal, 0, max_hp);
    std::cout << "after heal: hp = " << hp << '\n';
    int dmg = 200;
    hp = std::clamp(hp - dmg, 0, max_hp);
    std::cout << "after big damage: hp = " << hp << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1759 概念:相同字元的同質子字串計數 (帶上限版)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給字串 s,計算「字元全相同的子字串」總數 (1 + 2 + ... + k 對長度 k 的同質段)。
//      帶上限版:結果太大時,clamp 在 [0, MOD]。
//
// 為什麼用 std::clamp:
//   累加完總數後,用 clamp 控制在合法範圍內 (例如 0 ~ 1e9+7);
//   也示範 clamp 在「結果上下界」常見的使用法。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1759_count_homogeneous_capped() {
    std::string s = "abbcccaa";
    long long total = 0;
    int run = 1;
    for (size_t i = 1; i <= s.size(); ++i) {
        if (i < s.size() && s[i] == s[i-1]) {
            ++run;
        } else {
            total += static_cast<long long>(run) * (run + 1) / 2;
            run = 1;
        }
    }
    const long long CAP = 1000000007;
    total = std::clamp<long long>(total, 0, CAP);
    std::cout << "LC1759: " << total << '\n';
}

// ----------------------------------------------------------------
// 實務範例:音量 / 亮度滑桿 (Volume Slider) clamp
// ----------------------------------------------------------------
// 場景:UI 滑桿讓使用者調整數值,即使外部程式 (例如快捷鍵 +/-) 觸發
//      也要保證數值不會跑出範圍。clamp 一行解決。
void practical_volume_slider_clamp() {
    int vol = 50;
    auto adjust = [&](int delta) {
        vol = std::clamp(vol + delta, 0, 100);
    };
    adjust(30);  std::cout << "vol=" << vol << '\n';  // 80
    adjust(50);  std::cout << "vol=" << vol << '\n';  // 100
    adjust(-200);std::cout << "vol=" << vol << '\n';  // 0
}

// === 預期輸出 (Expected output) ===
// clamp(5, 0, 10) = 5
// clamp(-3, 0, 10) = 0
// clamp(99, 0, 10) = 10
// clamp(0.7, 0.0, 1.0) = 0.7
// clamp("m", "a", "f") = f
// clamp(-20, -5, 5, by_abs) = -5
// manual clamp = 10
// LC2419: max=3, longest run = 2
// font sizes: 8 8 16 24 72 72 72
// after heal: hp = 100
// after big damage: hp = 0
// LC1759: 13
// vol=80
// vol=100
// vol=0
