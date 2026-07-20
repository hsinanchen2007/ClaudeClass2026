// =============================================================================
//  05_other_distributions.cpp  —  其他常用分佈
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/numeric/random/bernoulli_distribution
//    - https://en.cppreference.com/w/cpp/numeric/random/normal_distribution
//    - https://en.cppreference.com/w/cpp/numeric/random/exponential_distribution
//    - https://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 標準提供的分佈（精選實用）                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  整數類：
//      uniform_int_distribution      均勻 [a, b]
//      bernoulli_distribution        機率 p 的 0/1
//      binomial_distribution         n 次 bernoulli 中成功次數
//      geometric_distribution        第幾次第一次成功
//      poisson_distribution          固定速率的事件數
//      discrete_distribution         有限離散選項，依 weight 分布
//
//  浮點類：
//      uniform_real_distribution     均勻 [a, b)
//      normal_distribution           常態 (mean, sd)
//      exponential_distribution      指數
//      gamma_distribution
//      lognormal_distribution
//      cauchy_distribution
//      chi_squared_distribution
//      student_t_distribution
//      fisher_f_distribution
//
//  本檔挑 4 個工作上最常用：
//   * bernoulli_distribution      抽硬幣
//   * normal_distribution         模擬資料、加噪
//   * exponential_distribution    模擬「事件間隔時間」
//   * discrete_distribution       依機率權重抽（LC528 後篇深入）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：std::other_distributions
  - std::other_distributions 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - normal_distribution、bernoulli_distribution、binomial_distribution 表示不同機率模型，應依資料生成需求選擇。
  - distribution 可能保存內部 cache，例如 normal_distribution；重設狀態可用 reset()。
  - 模擬程式要把模型參數寫清楚，否則亂數看似合理但統計意義錯誤。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】其他常用分佈
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 標準提供哪些常用分佈？各用在什麼場景？
//     答：bernoulli_distribution（以機率 p 回傳 true，用於「要不要發生」）、
//         binomial_distribution（n 次獨立試驗的成功次數）、poisson_distribution
//         （單位時間內的事件數，如請求到達）、normal_distribution（常態，量測誤差、
//         雜訊）、exponential_distribution（事件間隔時間）、discrete_distribution
//         （依權重挑索引，見 LC 528）。選錯分佈比寫錯程式更難發現，模擬前務必先確定
//         你要的機率模型。
//     追問：poisson 與 exponential 的關係？（同一個過程的兩種視角：前者看「單位時間
//           內幾次」，後者看「兩次之間隔多久」）
//
// 🔥 Q2. distribution 是有狀態的嗎？有什麼實務影響？
//     答：是。operator() 不是 const，因為 distribution 可以快取內部狀態。最典型的是
//         normal_distribution：常見實作一次計算會產生「兩個」獨立的常態值，把第二個
//         快取起來下次直接回傳。實務影響：(1) 不能宣告成 const；(2) 每次呼叫都重建
//         distribution 會丟掉快取，效能變差；(3) 想重現結果時只 reseed engine 不夠，
//         還要呼叫 dist.reset() 清掉快取；(4) 複製 distribution 會連狀態一起複製。
//     追問：什麼時候該呼叫 reset()？（切換 engine、重新 seed、或需要嚴格重現時）
//
// Q3. 要「依權重隨機挑一個」有哪些做法？
//     答：最直接是 std::discrete_distribution，傳入權重即可。手寫則是建 prefix sum
//         陣列 + 二分搜尋：建構 O(n)、查詢 O(log n)。若查詢極頻繁且權重固定，可用
//         Alias Method 做到查詢 O(1)，代價是額外空間與較複雜的建構。
//     追問：權重會動態變動怎麼辦？（用 Fenwick tree 維護 prefix sum，更新與查詢
//           都是 O(log n)）
// ═══════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────
// 實用範例 1：A/B test 模擬 — 用 bernoulli 模擬點擊率
//   工作中常見：模擬「對照組 5% 點擊率 vs 實驗組 7% 點擊率」是否有顯著差異。
//   bernoulli_distribution 是最直接的工具。
// ─────────────────────────────────────────────────────────
static void simulate_ab_test(double rateA, double rateB, int N) {
    std::mt19937 rng{2026};
    std::bernoulli_distribution distA{rateA};
    std::bernoulli_distribution distB{rateB};
    int hitsA = 0, hitsB = 0;
    for (int i = 0; i < N; ++i) {
        hitsA += distA(rng);
        hitsB += distB(rng);
    }
    std::cout << "[Practical] A/B test (N=" << N << "):\n";
    std::cout << "  A 命中 " << hitsA << " / " << N
              << " = " << std::fixed << std::setprecision(4)
              << (double)hitsA / N << '\n';
    std::cout << "  B 命中 " << hitsB << " / " << N
              << " = " << (double)hitsB / N << '\n';
}

// ─────────────────────────────────────────────────────────
// 實用範例 2：加權抽獎名稱 — discrete_distribution 名稱對應
//   工作中常見：「依稀有度抽 SSR/SR/R/N 卡片」。
// ─────────────────────────────────────────────────────────
static void simulate_gacha(int draws) {
    std::mt19937 rng{777};
    std::vector<double> weights{1.0, 5.0, 20.0, 74.0};   // SSR/SR/R/N 機率
    std::vector<std::string> names{"SSR", "SR", "R", "N"};
    std::discrete_distribution<int> picker(weights.begin(), weights.end());
    std::map<std::string, int> tally;
    for (int i = 0; i < draws; ++i) ++tally[names[picker(rng)]];
    std::cout << "[Practical] gacha draws=" << draws << ":\n";
    for (auto& [k, v] : tally)
        std::cout << "  " << k << " = " << v << '\n';
}

int main() {
    std::mt19937 rng{2026};

    // ─────────────────────────────────────────────────────────
    // Demo 1：bernoulli_distribution — 機率 0.3 為 true
    // ─────────────────────────────────────────────────────────
    {
        std::bernoulli_distribution coin{0.3};
        int trues = 0;
        for (int i = 0; i < 10000; ++i) trues += coin(rng);
        std::cout << "[Demo1] bernoulli(0.3) 10000 次中 true = " << trues
                  << " (期望 ~3000)\n";
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：normal_distribution — 平均 100、SD 15（IQ-like）
    // ─────────────────────────────────────────────────────────
    {
        std::normal_distribution<double> nd{100.0, 15.0};
        std::vector<double> samples;
        samples.reserve(1000);
        for (int i = 0; i < 1000; ++i) samples.push_back(nd(rng));

        double sum = 0;
        for (double v : samples) sum += v;
        double mean = sum / samples.size();
        double var = 0;
        for (double v : samples) var += (v - mean) * (v - mean);
        var /= samples.size();
        std::cout << "[Demo2] normal(100,15): "
                  << "mean=" << std::fixed << std::setprecision(2) << mean
                  << " sd=" << std::sqrt(var) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：exponential_distribution — 模擬事件間隔
    //   λ=2 表示「平均每秒 2 件事」 → 平均間隔時間 1/2 = 0.5 秒
    // ─────────────────────────────────────────────────────────
    {
        std::exponential_distribution<double> exp{2.0};
        std::cout << "[Demo3] 5 inter-arrival times:";
        for (int i = 0; i < 5; ++i)
            std::cout << ' ' << std::setprecision(3) << exp(rng);
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 4：discrete_distribution — 帶權離散
    //   表達「A: 50%, B: 30%, C: 20%」之類的選擇
    // ─────────────────────────────────────────────────────────
    {
        std::discrete_distribution<int> picker{{50, 30, 20}};
        // 選出的是 0/1/2，對應 A/B/C
        std::map<char, int> tally;
        for (int i = 0; i < 10000; ++i) {
            int idx = picker(rng);
            ++tally["ABC"[idx]];
        }
        std::cout << "[Demo4] discrete 10000 次:";
        for (auto& [k, v] : tally) std::cout << ' ' << k << '=' << v;
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：normal_distribution 的尾巴會不會「太重」？
    //    A：標準分佈尾部是「無限延伸」，理論上會回傳很極端值；實務上只
    //       是極罕見。對某些演算法（如「永遠在 [0,1] 之間」）需要 clip 一下。
    //
    //  Q2：discrete_distribution 內部怎麼做？
    //    A：建立「累積機率表」(walker's alias method 或 binary search) 後
    //       對 uniform 採樣 → 對應到區間 → O(1)/O(log n) 抽出。
    //
    //  Q3：怎麼讓「同一段程式」用「同一序列」？
    //    A：把 engine + distribution 的「狀態」存起來。distribution 物件
    //       本身有 reset()、可 serialize；要嚴謹重現實驗就把 engine 與
    //       seed 都記住。
    //
    simulate_ab_test(0.05, 0.07, 10000);
    simulate_gacha(10000);
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 05_other_distributions.cpp -o 05_other_distributions

// === 預期輸出 ===
// [Demo1] bernoulli(0.3) 10000 次中 true = 3050 (期望 ~3000)
// [Demo2] normal(100,15): mean=100.08 sd=14.69
// [Demo3] 5 inter-arrival times: 0.315 0.063 0.749 0.342 0.385
// [Demo4] discrete 10000 次: A=4907 B=3015 C=2078
// [Practical] A/B test (N=10000):
//   A 命中 502 / 10000 = 0.0502
//   B 命中 750 / 10000 = 0.0750
// [Practical] gacha draws=10000:
//   N = 7411
//   R = 1998
//   SR = 504
//   SSR = 87
