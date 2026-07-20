// ============================================================
// std::max
// 分類 (Category): Minimum/maximum operations (最值)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/max
//   * https://cplusplus.com/reference/algorithm/max/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::max 是 std::min 的孿生兄弟 — 回傳「較大的值」,行為對稱:
//
//   * 兩值版回傳「const 參考」;ilist 版回傳「值」。
//   * 兩值相等 → 回傳第一個 (a)。
//   * 有「生命期陷阱」 (見 std::min 詳細說明)。
//
// 這是 STL 中極度常用的工具,DP、貪心、競賽程式幾乎處處都用到。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、std::max vs C 巨集 max(a, b) 的關鍵差別                │
// └────────────────────────────────────────────────────────────┘
//
// C 語言常見的巨集寫法:
//
//   #define max(a, b) ((a) > (b) ? (a) : (b))
//
// 這個巨集有 side-effect 問題:
//
//   max(++i, j)   // 巨集展開後 i 可能被遞增兩次!
//
// std::max 是「函式」,參數只求值一次 — 沒有這個問題。
// 寫 C++ 程式時請永遠用 std::max,不要用 C 風格巨集。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、std::max 在 DP 中的核心地位                            │
// └────────────────────────────────────────────────────────────┘
//
// 動態規劃問題經常出現:
//
//   dp[i] = std::max(dp[i-1], something_else);
//
// 這就是「我選最佳選項中收益最大的那個」 — std::max 是 DP 的標配。
// LC 53 (最大子陣列和)、LC 198 (打家劫舍)、LC 322 (零錢兌換) 等都這樣用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、與 std::min 配合做 clamp                                │
// └────────────────────────────────────────────────────────────┘
//
// 「把值限制在 [lo, hi]」的 manual clamp:
//
//   std::min(hi, std::max(lo, x))
//
// C++17 起有 std::clamp 直接寫 — 但底層概念就是 min + max 組合。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class T>
//   const T& max(const T& a, const T& b);
//
//   template <class T, class Compare>
//   const T& max(const T& a, const T& b, Compare comp);
//
//   template <class T>
//   T max(std::initializer_list<T> ilist);
//
//   template <class T, class Compare>
//   T max(std::initializer_list<T> ilist, Compare comp);
//
//   * C++14 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 生命期陷阱 — 不要 const auto& 接,改用 by-value。
//   2. 容器最大值用 std::max_element。
//   3. 永遠用 std::max,不要用 C 巨集 max(a, b)。
//
// ============================================================

/*
補充筆記：std::max
  - std::max 回傳較大值；它不是找整個容器最大元素的工具。
  - 容器範圍要用 max_element，因為 max 只比較你傳入的兩個值或 initializer_list。
  - 若比較 expensive object，注意回傳 reference 與生命週期。
  - std::max 用來取得最小、最大或限制範圍；重點是分清楚回傳的是值、reference 還是 iterator。
  - min/max 處理兩個值，min_element/max_element 處理一段 range；range 版本在空範圍時會回傳 last。
  - clamp(value, lo, hi) 要求 lo 不大於 hi；上下界順序錯誤會違反前置條件。
  - minmax 可避免自己寫兩次比較，minmax_element 則能同時找位置；大量資料時比各跑一次 min_element/max_element 更直接。
  - 比較浮點數時要注意 NaN；一般比較器遇到 NaN 可能讓結果不符合直覺。
  - 若要保留原資料位置，使用 element 版本回傳 iterator；若只要數值，才使用 min/max 的值版本。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::max
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 C++ 要用 std::max 而不是 C 的巨集 `#define max(a,b) ((a)>(b)?(a):(b))`？
//     答：巨集是文字展開，參數會被求值多次——`max(++i, j)` 展開後 `++i` 出現兩次，
//         i 可能被遞增兩次。std::max 是函式，參數只求值一次，沒有 side-effect 問題；
//         而且是 template，型別安全、C++14 起還是 constexpr。
//     追問：巨集還有什麼毛病？(沒有 namespace、會和成員函式/變數名撞名，
//           所以 <windows.h> 的 max/min 巨集才要用 NOMINMAX 關掉)
//
// 🔥 Q2. `std::max(a, b)` 回傳值還是 reference？相等時回傳誰？
//     答：兩值版回傳 **const T&**；initializer_list 版 `max({a,b,c})` 回傳 **T**（值）。
//         相等時回傳 **a**（第一個參數）——實作是 `return a < b ? b : a;`，
//         用嚴格 `<`，相等走 a 那一支。
//     追問：這代表什麼風險？(和 std::min 一樣的 lifetime 陷阱：不要用
//           `const auto&` 接它的回傳值，暫存參數會 dangling)
//
// ⚠️ 陷阱. `std::max(1u, -1)` 會發生什麼事？
//     答：**編譯錯誤**，不是跑出奇怪的值。std::max 是 `template<class T>`，
//         兩個參數必須推導出同一個 T；`unsigned` 和 `int` 推不出共同型別，
//         template argument deduction 失敗。
//         解法：顯式指定 `std::max<int>(1u, -1)`，或先統一型別再比。
//     為什麼會錯：多數人套用「內建運算子 `>` 會做 usual arithmetic conversions」的
//         直覺，以為 std::max 也會自動轉型；但 template 推導不做隱式轉換，
//         這反而幫你擋掉了 unsigned 比較的經典地雷。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 兩值 ---
    std::cout << "max(3, 5) = " << std::max(3, 5) << '\n';

    // --- 範例 2: 字串 (字典序) ---
    std::cout << "max(\"a\", \"b\") = "
              << std::max(std::string("a"), std::string("b")) << '\n';

    // --- 範例 3: 自訂 comp — 比字串長度 ---
    std::cout << "max by length: "
              << std::max(std::string("apple"), std::string("kiwi"),
                          [](const std::string& a, const std::string& b){
                              return a.size() < b.size();
                          })
              << '\n';

    // --- 範例 4: ilist 版多值比較 ---
    std::cout << "max{4, 1, 7, 0, 3} = " << std::max({4, 1, 7, 0, 3}) << '\n';

    // --- 範例 5: 與 min 一起做 manual clamp (C++17 前的寫法) ---
    int x = 12;
    int lo = 0, hi = 10;
    int clipped = std::min(hi, std::max(lo, x));
    std::cout << "clamp(12, 0, 10) = " << clipped << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_53_max_subarray();
    void practical_version_max();
    void leetcode_121_best_time_to_buy_sell_stock();
    void practical_resize_to_min_required();
    leetcode_53_max_subarray();
    practical_version_max();
    leetcode_121_best_time_to_buy_sell_stock();
    practical_resize_to_min_required();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 53: 最大子陣列和 (Maximum Subarray) — Kadane 演算法
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,找具有最大總和的「連續子陣列」,回傳其總和。
//
// 為什麼用 std::max:
//   Kadane 演算法的兩個關鍵更新都是 max:
//     cur = std::max(num, cur + num)   // 接續或重起
//     ans = std::max(ans, cur)          // 全局最大
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_53_max_subarray() {
    std::vector<int> nums{-2, 1, -3, 4, -1, 2, 1, -5, 4};
    int cur = nums[0];
    int ans = nums[0];
    for (size_t i = 1; i < nums.size(); ++i) {
        cur = std::max(nums[i], cur + nums[i]);
        ans = std::max(ans, cur);
    }
    std::cout << "LC53: max subarray sum = " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:取「較新版本號」決定要不要升級
// ----------------------------------------------------------------
// 場景:系統升級時,取「使用者本機版本」與「伺服器最新版本」較大者。
void practical_version_max() {
    int local_version  = 105;
    int server_version = 110;
    int latest = std::max(local_version, server_version);
    std::cout << "latest version = max(local=" << local_version
              << ", server=" << server_version << ") = " << latest << '\n';
}

// ----------------------------------------------------------------
// LeetCode 121: 買賣股票的最佳時機 (Best Time to Buy and Sell Stock)
// ----------------------------------------------------------------
// 題目:給陣列 prices,prices[i] 代表第 i 天股票價格。
//      最多買一次、賣一次,求最大利潤。
//
// 為什麼用 std::max:
//   一邊掃描一邊維護「目前看過的最低買入價 min_price」與「最大利潤 ans」:
//     ans = std::max(ans, prices[i] - min_price);
//     min_price = std::min(min_price, prices[i]);
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_121_best_time_to_buy_sell_stock() {
    std::vector<int> prices{7, 1, 5, 3, 6, 4};
    int min_price = prices[0];
    int ans = 0;
    for (size_t i = 1; i < prices.size(); ++i) {
        ans = std::max(ans, prices[i] - min_price);
        min_price = std::min(min_price, prices[i]);
    }
    std::cout << "LC121: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:緩衝區擴容 (capacity = max(needed, current))
// ----------------------------------------------------------------
// 場景:讀取資料前要先確保 buffer 足夠大。
//      用 std::max 一行決定「保留現有大小,或擴大到所需」 — 不會縮減。
void practical_resize_to_min_required() {
    size_t current_cap = 64;
    size_t needed = 100;
    size_t new_cap = std::max(current_cap, needed);
    std::cout << "buf cap: was=" << current_cap
              << ", need=" << needed
              << " → new=" << new_cap << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra max.cpp -o max

// === 預期輸出 ===
// max(3, 5) = 5
// max("a", "b") = b
// max by length: apple
// max{4, 1, 7, 0, 3} = 7
// clamp(12, 0, 10) = 10
// LC53: max subarray sum = 6
// latest version = max(local=105, server=110) = 110
// LC121: 5
// buf cap: was=64, need=100 → new=100
