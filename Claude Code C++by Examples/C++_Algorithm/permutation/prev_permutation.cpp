// ============================================================
// std::prev_permutation
// 分類 (Category): Permutation operations (排列)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/prev_permutation
//   * https://cplusplus.com/reference/algorithm/prev_permutation/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::prev_permutation 是 std::next_permutation 的孿生 — 把容器
// 重排為「字典序的『前一個』排列」。
//
// 行為對稱:
//   * next_permutation: 從小到大推進,到最大後 wrap around 回最小。
//   * prev_permutation: 從大到小推進,到最小後 wrap around 回最大。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、什麼時候會用到 prev_permutation?                       │
// └────────────────────────────────────────────────────────────┘
//
//   * 「字典序由大到小」枚舉所有排列 (從遞減開始反覆呼叫)
//   * 「Undo」 —「上一個排列狀態」可以由 prev_permutation 回退
//   * 與 next_permutation 對稱的演算法練習
//
// 大多數情況用 next_permutation 就夠了 — prev_permutation 的場景比較少。
// 但「字典序往小走」這個語意,只有它能直接表達。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、演算法原理 (對稱於 next_permutation)                   │
// └────────────────────────────────────────────────────────────┘
//
//   1. 從右往左找「第一個 a[i] > a[i+1]」 (即遞減點) — 稱為 pivot。
//   2. 從右往左找「第一個 a[j] < a[i]」(剛好小於 pivot)。
//   3. swap(a[i], a[j])。
//   4. reverse a[i+1 .. end] (這段原本遞增,反轉後變遞減)。
//
// 與 next_permutation 唯一差別就是大小比較反向。整體 O(N)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class BidirIt>
//   bool prev_permutation(BidirIt first, BidirIt last);
//
//   template <class BidirIt, class Compare>
//   bool prev_permutation(BidirIt first, BidirIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 平均 O(N) 次比較與 swap
//   空間: O(1) (原地)
//
//   1. 從遞減狀態開始反覆呼叫,可逆向枚舉所有排列。
//   2. 含重複元素時,只枚舉相異排列。
//   3. comp 必須符合嚴格弱序;與 next_permutation 是「同一個 comp 的反方向」。
//
// ============================================================

/*
補充筆記：std::prev_permutation
  - std::prev_permutation 處理排列順序或字典序比較；它關心元素相對排列，不一定關心容器型別。
  - next_permutation/prev_permutation 會直接修改範圍，並回傳是否成功產生下一個排列。
  - 若一開始不是排序後的最小排列，next_permutation 只會從目前排列往後走，不會列出全部排列。
  - lexicographical_compare 像字典查單字一樣逐元素比較，常用於自訂排序或比較序列。
  - is_permutation 判斷兩段資料是否由相同元素組成，重複值也會納入計算。
  - 排列數量成長極快，n! 很快不可接受；教材範例小，工作上要先估算資料量。
  - std::prev_permutation 處理的是序列排列，不是集合；相同元素出現多次時，排列與比較都會把重複值算進去。
  - std::prev_permutation 常和字典序有關，字典序比較會從第一個不同元素決定大小，前面元素相同才比較長度。
  - std::prev_permutation 可能直接重排原範圍；呼叫後若還需要原順序，應先保留副本或改用不修改輸入的判斷函式。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::prev_permutation
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. prev_permutation 和 next_permutation 的關係是什麼?演算法差在哪?
//     答:兩者是鏡像操作 — next 走字典序的下一個,prev 走上一個。演算法四步完全
//         對稱,只是把大小比較反向:
//         (1) 從右往左找第一個 a[i] > a[i+1](pivot);
//         (2) 從右往左找第一個 a[j] < a[i];
//         (3) swap(a[i], a[j]);
//         (4) reverse [i+1, end)(該段原本遞增,反轉後變遞減)。
//         同樣是原地、額外空間 O(1),單次呼叫均攤 O(N)。
//     追問:傳一個反向的 comp 給 next_permutation,是不是就等於 prev_permutation?
//           (答:效果上可以達成「往反方向走」,但要求 comp 仍需是嚴格弱序,
//            而且與你排序時用的準則一致 — 直接用 prev_permutation 語意清楚得多)
//
// 🔥 Q2. 要用 prev_permutation 逆向枚舉全部排列,起點該怎麼設?
//     答:起點必須是「最大排列」(完全降序),否則只會走到最小排列就停,
//         前面的排列拿不到。做法是 sort 後 reverse,或直接
//         std::sort(v.begin(), v.end(), std::greater<>{}),再配 do-while。
//         這正是 next_permutation「先 sort 成升序」的鏡像要求。
//     追問:兩個方向的走訪順序會互為顛倒嗎?(答:會 — 同一組元素的全部相異排列,
//           用 next 由小到大、用 prev 由大到小,是同一條字典序鏈的兩個方向)
//
// ⚠️ 陷阱. prev_permutation 回傳 false 時,容器維持原樣嗎?
//     答:不是。回傳 false 代表「呼叫前已是最小排列」,而此時容器已經被
//         wrap around 重排成「最大排列」(完全降序) — 值已經被改掉了。
//         next_permutation 對稱:回 false 時容器被重排回最小排列。
//     為什麼會錯:多數人把回傳值讀成「失敗 / 沒做事」,於是在 if (!ok) 分支裡
//         繼續沿用容器內容,結果拿到的是 wrap 之後的排列。
//         要保留原狀就得自己先複製一份。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 從遞減狀態起,逆向枚舉 ---
    std::vector<int> v{3, 2, 1};
    std::cout << "reverse-order permutations of {1,2,3}:\n";
    do {
        std::cout << "  ";
        for (int x : v) std::cout << x << ' ';
        std::cout << '\n';
    } while (std::prev_permutation(v.begin(), v.end()));

    // --- 範例 2: 已最小 → 回 false 並重排為最大 ---
    std::vector<int> w{1, 2, 3};
    bool more = std::prev_permutation(w.begin(), w.end());
    std::cout << "after pp on {1,2,3}: more=" << std::boolalpha << more << ", v=";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_31_prev_permutation();
    void leetcode_46_permutations_desc();
    void practical_undo_permutation();
    void leetcode_2last_permutation_string();
    void practical_step_backward_player_order();
    std::cout << "\n--- LeetCode / Practical Examples ---\n";
    leetcode_31_prev_permutation();
    leetcode_46_permutations_desc();
    practical_undo_permutation();
    leetcode_2last_permutation_string();
    practical_step_backward_player_order();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 31 反向版:上一個排列
// ----------------------------------------------------------------
// 題目:LC 31 原題求「下一個排列」,本範例為對稱概念 — 求「上一個」字典序排列。
//
// 為什麼用 std::prev_permutation:
//   題意 100% 對應 — 直接呼叫,O(n) 解決。
void leetcode_31_prev_permutation() {
    std::vector<int> nums{1, 3, 2};
    std::prev_permutation(nums.begin(), nums.end());
    std::cout << "LC31-rev ({1,3,2} -> prev): ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';

    std::vector<int> nums2{1, 2, 3};
    bool more = std::prev_permutation(nums2.begin(), nums2.end());
    std::cout << "LC31-rev ({1,2,3} -> prev): more=" << std::boolalpha << more << ", v=";
    for (int x : nums2) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 46 反向:字典序「降冪」枚舉所有排列
// ----------------------------------------------------------------
// 題目:給不重複整數陣列 nums,以「字典序由大到小」回傳所有排列。
//
// 為什麼用 std::prev_permutation:
//   先 sort 再 reverse 使其為最大排列,反覆 prev_permutation 即可降冪枚舉。
void leetcode_46_permutations_desc() {
    std::vector<int> nums{1, 2, 3};
    std::sort(nums.begin(), nums.end(), std::greater<int>());
    std::vector<std::vector<int>> ans;
    do {
        ans.push_back(nums);
    } while (std::prev_permutation(nums.begin(), nums.end()));
    std::cout << "LC46-desc ({1,2,3} desc): total=" << ans.size() << '\n';
    for (const auto& p : ans) {
        std::cout << "  ";
        for (int x : p) std::cout << x << ' ';
        std::cout << '\n';
    }
}

// ----------------------------------------------------------------
// 實務範例:Undo 系統 — 回到上一個排列狀態
// ----------------------------------------------------------------
// 場景:編輯器把某段元素的順序視為「排列狀態」,使用者按 Ctrl+Z 想回到字典序的
//      上一個排列。回到最小排列後就再無 undo 可做。
//
// 為什麼用 std::prev_permutation:
//   * 一次操作 O(n),語意明確。
//   * 回傳 false 即代表已是最小排列,無法再 undo。
void practical_undo_permutation() {
    std::string state = "BCA";
    std::cout << "Practical (Undo system, start state=\"" << state << "\"):\n";
    for (int step = 1; step <= 5; ++step) {
        std::string before = state;
        bool ok = std::prev_permutation(state.begin(), state.end());
        if (!ok) {
            std::cout << "  step " << step << ": \"" << before
                      << "\" is already the smallest, no undo available\n";
            break;
        }
        std::cout << "  step " << step << ": \"" << before
                  << "\" -> \"" << state << "\"\n";
    }
}

// ----------------------------------------------------------------
// LeetCode 概念題:取得字典序「最後一個」排列
// ----------------------------------------------------------------
// 題目:給字串 s,回傳其「字典序最大的排列」 (即 s 的字元降冪排列)。
//
// 為什麼用 std::prev_permutation:
//   只要 sort 升冪後再 prev_permutation 一次 false (即還原 max),
//   或更乾脆 sort(greater) 即可。這裡用 prev_permutation 示範對「邊界」的識別。
//
// 複雜度:時間 O(n log n);空間 O(1)。
void leetcode_2last_permutation_string() {
    std::string s = "bdca";
    std::sort(s.begin(), s.end());
    // 反覆 next_permutation 到最後一個 — 直接 sort(greater) 才高效;
    // 這裡示範 prev_permutation 從降冪起點回看為何回 false
    std::sort(s.begin(), s.end(), std::greater<char>());
    bool more = std::prev_permutation(s.begin(), s.end());
    std::cout << "last perm of \"bdca\": " << s << " more=" << std::boolalpha << more << '\n';
    // 注意:呼叫 prev_permutation 後 s 已不是最大排列;再 sort(greater) 即可還原
}

// ----------------------------------------------------------------
// 實務範例:遊戲玩家輪換 — 上一個玩家順序
// ----------------------------------------------------------------
// 場景:多人遊戲玩家順序按字典序循環,每回合「退一個排列」(復古回上一輪 layout)。
//      prev_permutation 直接表達。
void practical_step_backward_player_order() {
    std::vector<std::string> players{"Bob", "Alice", "Carol"};
    std::sort(players.begin(), players.end());   // 起始最小
    std::next_permutation(players.begin(), players.end());
    std::next_permutation(players.begin(), players.end());
    // 現在處於第 3 排列;呼叫 prev_permutation 回到第 2 排列
    std::prev_permutation(players.begin(), players.end());
    std::cout << "after step-back:";
    for (auto& p : players) std::cout << " " << p;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// reverse-order permutations of {1,2,3}:
//   3 2 1
//   3 1 2
//   2 3 1
//   2 1 3
//   1 3 2
//   1 2 3
// after pp on {1,2,3}: more=false, v=3 2 1
//
// --- LeetCode / Practical Examples ---
// LC31-rev ({1,3,2} -> prev): 1 2 3
// LC31-rev ({1,2,3} -> prev): more=false, v=3 2 1
// LC46-desc ({1,2,3} desc): total=6
//   3 2 1
//   3 1 2
//   2 3 1
//   2 1 3
//   1 3 2
//   1 2 3
// Practical (Undo system, start state="BCA"):
//   step 1: "BCA" -> "BAC"
//   step 2: "BAC" -> "ACB"
//   step 3: "ACB" -> "ABC"
//   step 4: "ABC" is already the smallest, no undo available
// last perm of "bdca": dcab more=true
// after step-back: Alice Carol Bob
