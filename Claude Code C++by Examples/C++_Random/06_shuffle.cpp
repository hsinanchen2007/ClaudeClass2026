// =============================================================================
//  06_shuffle.cpp  —  std::shuffle（取代 deprecated random_shuffle）
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/algorithm/random_shuffle
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、std::shuffle vs std::random_shuffle                    │
//  └────────────────────────────────────────────────────────────┘
//
//      std::random_shuffle(first, last);              // C++17 deprecated, C++20 removed
//      std::shuffle(first, last, engine);             // ✅ 現代寫法
//
//  random_shuffle 用全域 rand()（差），shuffle 接受任意 UniformRandomBitGenerator
//  — 你給它一個 mt19937，它就用 mt19937 來洗牌。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、Fisher–Yates 洗牌（shuffle 的內部演算法）              │
//  └────────────────────────────────────────────────────────────┘
//
//  for i from n-1 down to 1:
//      j = uniform_int(0, i)        // 0..i 包含
//      swap(arr[i], arr[j])
//
//  正確 O(n)，每種排列出現機率「都剛好一樣」 (n!) — 這是「無偏洗牌」的
//  教科書答案。錯誤的 naive 寫法（每個 i 都跟 0..n-1 隨機 swap）有偏差。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：對 vector 洗牌、看結果
//   * Demo 2：用同 seed 重現
//   * Demo 3：對任意容器 — 自訂只洗其中一段（subrange）
// =============================================================================

/*
補充筆記：std::shuffle
  - shuffle 使用 UniformRandomBitGenerator 隨機重排整個範圍。
  - 它取代 deprecated 的 random_shuffle，避免 rand 品質與 modulo bias 問題。
  - 如果需要可重現測試，使用固定 seed 的 engine。
  - std::shuffle 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
*/
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────
// 實用範例：sample_k_distinct — 從 N 中隨機抽 k 個不重複
//   工作中常見：「從 100 個 user 中抽 10 個來做問卷」。
//   做法：建 [0,N) 索引 vector → std::shuffle → 取前 k 個。
//   時間 O(N)，正確性「每種 k 元子集等機率」。
// ─────────────────────────────────────────────────────────
static std::vector<int> sample_k_distinct(int N, int k, std::mt19937& rng) {
    std::vector<int> pool(N);
    for (int i = 0; i < N; ++i) pool[i] = i;
    std::shuffle(pool.begin(), pool.end(), rng);
    pool.resize(k);
    return pool;
}

static void demo_practical_sample() {
    std::cout << "[Practical] 從 0..19 抽 5 個：";
    std::mt19937 rng{42};
    auto picked = sample_k_distinct(20, 5, rng);
    for (int x : picked) std::cout << ' ' << x;
    std::cout << '\n';
}

template <class T>
static void print(const std::string& tag, const std::vector<T>& v) {
    std::cout << tag;
    for (auto& x : v) std::cout << ' ' << x;
    std::cout << '\n';
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：洗一副撲克
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> deck;
        for (int i = 1; i <= 13; ++i) deck.push_back(i);   // A=1, 2..K=13

        std::mt19937 rng{std::random_device{}()};
        std::shuffle(deck.begin(), deck.end(), rng);
        print("[Demo1] shuffled:", deck);
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：固定 seed 重現
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> a{1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::vector<int> b{a};
        std::mt19937 rng1{42};
        std::mt19937 rng2{42};
        std::shuffle(a.begin(), a.end(), rng1);
        std::shuffle(b.begin(), b.end(), rng2);
        print("[Demo2] a:", a);
        print("[Demo2] b:", b);
        std::cout << "[Demo2] same? " << std::boolalpha << (a == b) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：洗其中一段
    //   shuffle 跟所有 STL 演算法一樣吃 [first, last)，只洗中間段也行
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> v{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        std::mt19937 rng{1};
        // 洗 index [3, 7) = {3,4,5,6}
        std::shuffle(v.begin() + 3, v.begin() + 7, rng);
        print("[Demo3] partial shuffle:", v);
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：要洗的是 list / forward_list 怎麼辦？
    //    A：std::shuffle 要求 random access iterator — list 不行。可以
    //       (a) 把元素搬到 vector 洗、洗完搬回；(b) 自己寫專屬版（少見）。
    //
    //  Q2：「固定 seed 還能洗出隨機序列」是矛盾嗎？
    //    A：不矛盾 — 「隨機」對「不知道種子的人」是隨機；對「知道種子的
    //       人」是 deterministic。固定 seed 用於測試重現、debug 復現。
    //
    //  Q3：要洗到「特別均勻」(每種 permutation 等概率) 嗎？
    //    A：std::shuffle 的標準保證就是「每種 permutation 等概率」 — 內部
    //       是 Fisher–Yates。能取到的 entropy 上限受 engine 種子 entropy 限制。
    //
    demo_practical_sample();
    return 0;
}
