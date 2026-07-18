// ============================================================
// std::shuffle   (C++11 起,取代被移除的 std::random_shuffle)
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/random_shuffle
//   * https://cplusplus.com/reference/algorithm/shuffle/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::shuffle 把 [first, last) 範圍中的元素「均勻隨機重新排列」。
// 「均勻」是關鍵 — 任何排列出現的機率相同。
//
// 標準實作通常是「Fisher-Yates 洗牌演算法」:
//
//   for i = N-1 down to 1:
//       j = uniform random integer in [0, i]
//       swap(a[i], a[j])
//
// 這個演算法 O(N),每次抽 1 次隨機數,每對 swap 1 次 — 簡潔且最佳。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼不用 std::random_shuffle?                        │
// └────────────────────────────────────────────────────────────┘
//
// std::random_shuffle 在 C++14 被「棄用 (deprecated)」、C++17 被「移除」。
// 原因:
//   * 它依賴 rand() — 品質差且不執行緒安全。
//   * 介面允許傳入「自製 RNG 函式」,但這些自製 RNG 多半很爛。
//
// 取而代之的 std::shuffle 強制要求使用者傳入「URBG」 —
// 一個「均勻隨機位元產生器」(像 std::mt19937)。這保證了隨機品質。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、URBG 的選擇                                            │
// └────────────────────────────────────────────────────────────┘
//
// C++ 標準提供多個 URBG:
//   * std::mt19937              — Mersenne Twister 32-bit,最常用
//   * std::mt19937_64           — 64-bit 版本
//   * std::ranlux24 / ranlux48  — 較慢但更高品質
//   * std::random_device        — 真隨機 (品質依硬體與 OS)
//
// 一般用法:
//   * 想「可重現」(測試環境):用 std::mt19937(固定 seed)
//   * 想「真隨機」(實際遊戲、安全):用 std::random_device 提供 seed
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、shuffle 與 sample 的差別                               │
// └────────────────────────────────────────────────────────────┘
//
//   * std::shuffle(first, last, g)         打亂整個範圍 (size 不變)
//   * std::sample(first, last, out, n, g)  從中抽 n 個 (size 為 n)
//
// 「我要打亂順序」用 shuffle;「我要抽幾個」用 sample。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt, class URBG>
//   void shuffle(RandomIt first, RandomIt last, URBG&& g);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N) — Fisher-Yates
//   空間: O(1)
//   需求: RandomAccessIterator (對 list 不能用,要先複製到 vector)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 必須提供 URBG;不要再用 random_shuffle 或 rand()。
//   2. PRNG 用同一個 seed 多次初始化會給同樣序列 — 用於可重現測試。
//   3. 對 list 不能直接用 — RandomAccess 需求。先複製到 vector 再洗。
//   4. RNG 不是執行緒安全的;多執行緒環境每個執行緒應有獨立 RNG。
//
// ============================================================

/*
補充筆記：std::shuffle
  - shuffle 使用 UniformRandomBitGenerator 隨機重排整個範圍。
  - 它取代 deprecated 的 random_shuffle，避免 rand 品質與 modulo bias 問題。
  - 如果需要可重現測試，使用固定 seed 的 engine。
  - std::shuffle 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // --- 範例 1: 用 mt19937 + 固定 seed (可重現) ---
    std::mt19937 rng(42);
    std::shuffle(v.begin(), v.end(), rng);
    std::cout << "shuffled (seed=42): ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 再洗一次,結果不同 (因為 rng 已前進過) ---
    std::shuffle(v.begin(), v.end(), rng);
    std::cout << "shuffled again:     ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 真隨機 seed (每次執行都不同) ---
    std::random_device rd;
    std::mt19937 rng2(rd());
    std::vector<int> w{1, 2, 3, 4, 5};
    std::shuffle(w.begin(), w.end(), rng2);
    std::cout << "true random shuffle: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_384_shuffle_an_array();
    void practical_card_deck_shuffle();
    void leetcode_1470_shuffle_array_pairs();
    void practical_randomize_quiz_questions();
    leetcode_384_shuffle_an_array();
    practical_card_deck_shuffle();
    leetcode_1470_shuffle_array_pairs();
    practical_randomize_quiz_questions();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 384: 打亂陣列 (Shuffle an Array)
// ----------------------------------------------------------------
// 題目:設計一個類別,提供:
//   1) reset()   — 還原為原始陣列
//   2) shuffle() — 回傳一個「均勻隨機」洗亂的陣列
//
// 為什麼用 std::shuffle:
//   題目要求「均勻隨機」洗牌 — std::shuffle 即 Fisher-Yates,完全對應。
//   只需 O(n) 時間,且品質有保證。
//
// 解法步驟:
//   1. 建構時保存 original 與工作 work。
//   2. reset 把 work 還原為 original。
//   3. shuffle 對 work 呼叫 std::shuffle。
//
// 複雜度:時間 O(n);空間 O(n) (保存 original)。
class Solution384 {
public:
    Solution384(std::vector<int> nums)
        : original_(std::move(nums)), work_(original_), rng_(2026) {}
    const std::vector<int>& reset() {
        work_ = original_;
        return work_;
    }
    const std::vector<int>& shuffle() {
        std::shuffle(work_.begin(), work_.end(), rng_);
        return work_;
    }
private:
    std::vector<int> original_;
    std::vector<int> work_;
    std::mt19937 rng_;
};

void leetcode_384_shuffle_an_array() {
    Solution384 sol({1, 2, 3, 4, 5});
    auto& s = sol.shuffle();
    std::cout << "LC384 shuffle (size=" << s.size() << "): ";
    for (int x : s) std::cout << x << ' ';
    std::cout << '\n';
    auto& r = sol.reset();
    std::cout << "LC384 reset:   ";
    for (int x : r) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:撲克牌洗牌
// ----------------------------------------------------------------
// 場景:牌局開始前需洗一副 52 張的撲克牌。
//
// 為什麼用 std::shuffle:
//   * Fisher-Yates 是賭場/公平遊戲的標準洗牌演算法。
//   * O(n) 時間,均勻機率,品質保證。
void practical_card_deck_shuffle() {
    std::vector<std::string> deck;
    const std::string suits = "SHDC";
    const std::string ranks[] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
    for (char s : suits) {
        for (auto& r : ranks) deck.push_back(r + s);
    }
    std::mt19937 rng(123);
    std::shuffle(deck.begin(), deck.end(), rng);
    std::cout << "Deck size=" << deck.size()
              << ", top 5: ";
    for (int i = 0; i < 5; ++i) std::cout << deck[i] << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1470 概念:打亂陣列 (Shuffle the Array) — 用 std::shuffle 做隨機驗證
// ----------------------------------------------------------------
// 題目原意:把 [x1, x2, ..., xn, y1, y2, ..., yn] 重排為 [x1, y1, x2, y2, ...]。
//          這原本不是隨機題,但我們可示範:洗牌後驗證「隨機性」 — 一個常見的單元測試模式。
//
// 為什麼用 std::shuffle:
//   單元測試裡常需要產生「無特定順序」的測試輸入,確認程式碼對任意輸入皆正確。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1470_shuffle_array_pairs() {
    std::vector<int> v{2, 5, 1, 3, 4, 7};   // n = 3,前 3 個是 x,後 3 個是 y
    int n = 3;
    std::vector<int> ans(2 * n);
    for (int i = 0; i < n; ++i) {
        ans[2*i]     = v[i];
        ans[2*i + 1] = v[i + n];
    }
    std::cout << "LC1470 reshuffled:";
    for (int x : ans) std::cout << ' ' << x;
    std::cout << '\n';
    // 再做一次隨機洗牌作為「驗證測試輸入」
    std::mt19937 rng(0);
    std::shuffle(ans.begin(), ans.end(), rng);
    std::cout << "LC1470 random verify size=" << ans.size() << '\n';
}

// ----------------------------------------------------------------
// 實務範例:測驗系統 — 把題目順序隨機化
// ----------------------------------------------------------------
// 場景:線上測驗系統要避免「相鄰考生答案抄襲」,每位考生看到的題目順序不同。
//      std::shuffle 配以「考生 ID」為 seed,即可生成可重現但隨機化的題序。
void practical_randomize_quiz_questions() {
    std::vector<std::string> questions{"Q1", "Q2", "Q3", "Q4", "Q5"};
    int student_id = 12345;
    std::mt19937 rng(student_id);
    std::shuffle(questions.begin(), questions.end(), rng);
    std::cout << "student " << student_id << " sees:";
    for (auto& q : questions) std::cout << " " << q;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// shuffled (seed=42): ...    (mt19937(42) 對 1..10 的洗牌,實際序列依實作)
// shuffled again:     ...
// true random shuffle: ...   (每次執行皆不同)
// LC384 shuffle (size=5): ...
// LC384 reset:   1 2 3 4 5
// Deck size=52, top 5: ...
// LC1470 reshuffled: 2 3 5 4 1 7
// LC1470 random verify size=6
// student 12345 sees: ... (依 seed 固定序列)
