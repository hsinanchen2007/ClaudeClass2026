// =============================================================================
//  08_lc_384_shuffle_array.cpp  —  LeetCode 384. Shuffle an Array
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/algorithm/random_shuffle (std::shuffle)
//    - https://en.cppreference.com/w/cpp/numeric/random/mt19937
//    - https://leetcode.com/problems/shuffle-an-array/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 題意                                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  實作一個 class Solution：
//    Solution(vector<int>& nums)   建構（記下原 nums）
//    vector<int> reset()           回到原 nums
//    vector<int> shuffle()         回傳一個「均勻隨機洗牌後」的 nums copy
//
//  範例：
//    nums = [1,2,3]
//    shuffle 多次每次回傳 [1,2,3] 的 6 種排列之一（理論上等機率）
//    reset 回到 [1,2,3]
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路：Fisher–Yates                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  shuffle() 內：拷貝 original_，跑 Fisher–Yates。或直接用 std::shuffle。
//  reset() 內：回傳 original_ 的 copy。
//
//  時間：shuffle O(n)，reset O(n)（拷貝）；空間：O(n)（保留 original）。
//
//  本檔示範兩種寫法：
//   (A) 自己寫 Fisher–Yates 迴圈（教學）
//   (B) 直接呼叫 std::shuffle（實務）
//
// =============================================================================

/*
補充筆記：std::shuffle_array
  - std::shuffle_array 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - reset() 應回傳原始順序的副本，不能回傳目前被 shuffle 過的同一份資料。
  - shuffle 若用 std::shuffle，亂數引擎應是類別成員或外部注入，避免每次重設 seed。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】LeetCode 384. Shuffle an Array
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 這題的標準解法是什麼？複雜度？
//     答：Fisher-Yates（Knuth shuffle）：複製一份原陣列，從後往前對每個位置 i，用
//         uniform_int_distribution<int>(0, i) 取 j（「含 i」是關鍵），交換 a[i] 與 a[j]。
//         時間 O(n)、額外空間 O(n)（因為 reset() 需要保留原始順序）。迴圈跑到 i > 0
//         即可，i == 0 時只能跟自己交換。
//     追問：engine 該放哪？（成員變數或 thread_local，只建立一次；放在 shuffle() 內
//           每次重建會讓結果失去隨機性）
//
// 🔥 Q2. 如何驗證你的洗牌真的是均勻的？
//     答：取小 n（例如 3），跑上百萬次，統計 n! = 6 種排列各自出現的頻率是否都接近
//         1/6，再用卡方檢定判斷偏差是否顯著。這個方法能直接抓出「隨機挑 j 的範圍寫錯」
//         這類 bug——錯誤版本在 n = 3 時頻率差異就已經肉眼可見。
//     追問：為什麼要挑小 n？（n 稍大時 n! 就爆炸，無法為每種排列收集到足夠樣本；
//           小 n 才能做有統計意義的頻率比較）
//
// ⚠️ 陷阱. 寫成 swap(a[i], a[rand() % n]) 為什麼是錯的？
//     答：兩個錯誤疊在一起。(1) 取值範圍應該是 [0, i] 而不是 [0, n-1]：後者產生 n^n
//         條等機率路徑，而 n^n 無法被 n! 整除，所以各排列的機率不可能相等。(2) rand()
//         % n 本身還有 modulo bias。結果是「看起來很亂但統計上有偏」的洗牌。
//     為什麼會錯：把「亂」等同於「均勻」。均勻性是可以被量化檢驗的性質，靠肉眼看
//         輸出永遠檢查不出來——這也是為什麼面試官特別愛考這題的驗證方法。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Solution {
public:
    explicit Solution(std::vector<int> nums)
        : original_(std::move(nums)),
          rng_(std::random_device{}()) {}

    std::vector<int> reset() const { return original_; }

    // 寫法 A：手寫 Fisher–Yates
    std::vector<int> shuffleManual() {
        std::vector<int> a = original_;
        for (int i = static_cast<int>(a.size()) - 1; i > 0; --i) {
            std::uniform_int_distribution<int> pick{0, i};
            int j = pick(rng_);
            std::swap(a[i], a[j]);
        }
        return a;
    }

    // 寫法 B：直接 std::shuffle
    std::vector<int> shuffleStd() {
        std::vector<int> a = original_;
        std::shuffle(a.begin(), a.end(), rng_);
        return a;
    }

private:
    std::vector<int> original_;
    std::mt19937 rng_;
};

// ─────────────────────────────────────────────────────────
// LeetCode 519. Random Flip Matrix  (難度: medium，不同題)
// 題意：給 m x n 矩陣，flip() 在所有「目前是 0」的格子中等機率選一個翻成 1。
//
// 思路：把已 flip 的格子位置記在 unordered_map<int,int>。
//   - 設 remaining = 還有幾格是 0
//   - rand 一個 r ∈ [0, remaining)
//   - 如果 map 裡有 r 對應 → 用對應值；否則就是 r 本身
//   - 把 r 對應到 remaining-1 (最後一格)，再 --remaining
//   類似於「動態縮減版」的洗牌 — Fisher-Yates 思想。
//
// 與 LC384 的差異：384 一次性洗整個 array；519 是「延遲式洗牌」，只需要
//   k 次 flip 時不必預先 O(m*n) 配置整個矩陣。
//
// 時間：flip O(1)；空間 O(k)，k = flip 次數。
// ─────────────────────────────────────────────────────────
class RandomFlipMatrix {
public:
    RandomFlipMatrix(int m, int n)
        : m_(m), n_(n), total_(static_cast<long long>(m) * n),
          remaining_(total_), rng_(std::random_device{}()) {}

    std::pair<int, int> flip() {
        std::uniform_int_distribution<long long> d{0, remaining_ - 1};
        long long r = d(rng_);
        // 查 map：如果 r 已被換到別處，拿替代值
        auto it = mapping_.find(r);
        long long picked = (it == mapping_.end()) ? r : it->second;
        // 把 r 對應到「目前最後一格」
        long long last = remaining_ - 1;
        auto it2 = mapping_.find(last);
        mapping_[r] = (it2 == mapping_.end()) ? last : it2->second;
        --remaining_;
        return {(int)(picked / n_), (int)(picked % n_)};
    }

    void reset() {
        mapping_.clear();
        remaining_ = total_;
    }

private:
    int m_, n_;
    long long total_, remaining_;
    std::unordered_map<long long, long long> mapping_;
    std::mt19937 rng_;
};

static void demo_lc519_random_flip() {
    std::cout << "[LC519] random flip matrix 3x3 翻 5 次：\n";
    RandomFlipMatrix m{3, 3};
    for (int i = 0; i < 5; ++i) {
        auto [r, c] = m.flip();
        std::cout << "  flipped (" << r << "," << c << ")\n";
    }
}

static void print(const std::vector<int>& v, const std::string& tag = "") {
    std::cout << tag;
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';
}

int main() {
    Solution s({1, 2, 3, 4, 5});

    print(s.shuffleManual(), "[manual]");
    print(s.shuffleStd(),    "[std]   ");
    print(s.reset(),         "[reset] ");

    // ─────────────────────────────────────────────────────────
    // 額外：跑 60000 次，驗證均勻性
    //   對 n=3 共 6 種排列；每種應該約 10000 次
    // ─────────────────────────────────────────────────────────
    Solution t({1, 2, 3});
    std::map<std::vector<int>, int> tally;
    for (int i = 0; i < 60'000; ++i) ++tally[t.shuffleStd()];
    std::cout << "[uniformity] each permutation count (期望 ~10000):\n";
    for (auto& [perm, cnt] : tally) {
        std::cout << "  ";
        for (int x : perm) std::cout << x;
        std::cout << " -> " << cnt << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 Fisher–Yates 從尾巴往前？
    //    A：每次「決定當前 index 的最終值」 — i 位置從 [0..i] 中均勻挑一
    //       個放入。這樣保證「每個 permutation 等機率」。從前往後也行，
    //       但要對應地把範圍改成 [i..n-1]。
    //
    //  Q2：rng 應該每次重建嗎？
    //    A：不應該。rng 持續使用、狀態才是 PRNG 的精髓；每次重建還會付
    //       random_device 的成本。把 rng 當 member 持有最佳。
    //
    //  Q3：每次 shuffle 後不存「shuffle 過的版本」可以嗎？
    //    A：可以 — 題目只要求「shuffle() 回傳一個」。我們從 original 重
    //       新洗，狀態零汙染、好測。如果題目要連續洗，就把 array 設成
    //       member 並 in-place 洗。
    //
    demo_lc519_random_flip();
    return 0;
}
