// =============================================================================
//  04_uniform_real.cpp  —  uniform_real_distribution
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、定義                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      std::uniform_real_distribution<double> d{a, b};   // 半開區間 [a, b)
//      double x = d(engine);
//
//  與整數版差異：
//   * 區間是 [a, b)（不含 b）
//   * 值域是浮點 — 會回傳「無限多」可能值（受浮點精度限制）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、常見應用                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 模擬機率事件：if (d(rng) < 0.3) ...
//   * Monte Carlo 演算法（積分估計、隨機抽樣）
//   * 圖形 / 動畫的隨機擾動
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：基本 [0, 1) 抽 N 個，算平均
//   * Demo 2：範圍 [a, b) 中抽
//   * Demo 3：用 [0, 1) 估算圓周率（Monte Carlo π）
// =============================================================================

/*
補充筆記：std::uniform_real
  - std::uniform_real 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - uniform_real_distribution 常見語意是產生 [a,b) 的浮點數，不保證包含上界。
  - 浮點亂數的離散精度受引擎和浮點型別限制，不是連續數學實數。
  - 若要產生整數索引，不要先產生 real 再乘長度，應直接用 uniform_int_distribution。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】uniform_real_distribution
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. uniform_real_distribution<double>(a, b) 的區間是什麼？
//     答：標準規定是半開區間 [a, b)，不含上界。這與整數版 uniform_int_distribution
//         的閉區間 [a, b] 恰好相反，是面試最愛考的不對稱點。實務上若上界絕對不能出現
//         （例如要當作陣列索引的比例、或後續會取倒數），應該自己再加一道檢查，不要
//         把區間端點的假設埋在程式深處。
//     追問：想要 [a, b] 閉區間怎麼辦？（用 std::nextafter(b, 上界之外) 稍微擴大上界，
//           或直接改用整數 distribution 再映射）
//
// 🔥 Q2. 要產生 [0, 1) 的隨機浮點數，為什麼不直接寫 gen() / (double)gen.max()？
//     答：因為那樣會踩到多個問題：除法把整數的均勻分布映射到浮點數時，因浮點表示的
//         間距不均勻，各個可表示值的機率並不相等；而且 gen() / max() 會包含 1.0
//         （閉區間），與慣例不符。uniform_real_distribution 已處理這些細節，直接用它。
//     追問：浮點的均勻分布本來就不可能完美嗎？（是。[0,1) 中可表示的 double 分布本身
//           就不均勻，任何實作都只能在「數值均勻」與「每個可表示值等機率」之間取捨）
//
// Q3. 為什麼比較浮點亂數時不該用 ==？
//     答：因為浮點運算有捨入誤差，而且亂數幾乎不可能剛好落在某個特定值上——
//         d(gen) == 0.5 的機率趨近於零，這種判斷等於恆為 false。要判斷落點請用區間
//         比較（x < 0.5），要做「以 p 機率發生」請直接用 std::bernoulli_distribution(p)，
//         語意更清楚也不會寫錯。
//     追問：bernoulli_distribution 與自己寫 d(gen) < p 差在哪？（結果等價，但前者意圖
//           明確、且把邊界條件的處理交給標準庫）
// ═══════════════════════════════════════════════════════════════════════════

#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

// ─────────────────────────────────────────────────────────
// 實用範例 1：random_jitter — 給時間戳「加上 ±N% 的隨機抖動」
//   工作中常見：重試機制 / 排程器要避免「打包瞬間爆量」，會對 sleep 時間
//   加上隨機 jitter (例如 ±10%)。uniform_real_distribution 一行解決。
// ─────────────────────────────────────────────────────────
static double random_jitter(double base, double pct, std::mt19937& rng) {
    std::uniform_real_distribution<double> d{-pct, pct};
    return base * (1.0 + d(rng));
}

static void demo_practical_jitter() {
    std::cout << "[Practical] random_jitter (base=1000ms, ±10%):\n";
    std::mt19937 rng{2026};
    for (int i = 0; i < 5; ++i) {
        std::cout << "  attempt " << i + 1 << " sleep "
                  << std::fixed << std::setprecision(1)
                  << random_jitter(1000.0, 0.10, rng) << " ms\n";
    }
}

// ─────────────────────────────────────────────────────────
// 實用範例 2：random_unit_vector_2d — 在圓盤內均勻取樣 (2D)
//   遊戲開發 / 物理模擬常見：在單位圓內均勻撒點。
//   注意「先取 r 再取 θ」會偏中心 — 要用拒絕法或開根號。
// ─────────────────────────────────────────────────────────
static std::pair<double, double> random_point_in_disk(std::mt19937& rng) {
    std::uniform_real_distribution<double> d{-1.0, 1.0};
    while (true) {
        double x = d(rng), y = d(rng);
        if (x * x + y * y <= 1.0) return {x, y};      // 拒絕落在圓外的點
    }
}

static void demo_practical_disk_sampling() {
    std::cout << "[Practical] disk sampling (5 points):\n";
    std::mt19937 rng{99};
    for (int i = 0; i < 5; ++i) {
        auto [x, y] = random_point_in_disk(rng);
        std::cout << "  (" << std::setprecision(3) << x << ", " << y << ")\n";
    }
}

int main() {
    std::mt19937 rng{12345};

    // ─────────────────────────────────────────────────────────
    // Demo 1：[0, 1) 平均應該接近 0.5
    // ─────────────────────────────────────────────────────────
    {
        std::uniform_real_distribution<double> d{0.0, 1.0};
        constexpr int N = 100'000;
        double sum = 0;
        for (int i = 0; i < N; ++i) sum += d(rng);
        std::cout << "[Demo1] N=" << N << " mean = "
                  << std::fixed << std::setprecision(4) << (sum / N) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：[10, 20) 抽 5 個觀察
    // ─────────────────────────────────────────────────────────
    {
        std::uniform_real_distribution<double> d{10.0, 20.0};
        std::cout << "[Demo2] 5 samples:";
        for (int i = 0; i < 5; ++i) std::cout << ' ' << d(rng);
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：Monte Carlo 估 π
    //   單位正方形 [0,1)x[0,1) 投點，落在四分之一圓內的比例 ≈ π/4
    // ─────────────────────────────────────────────────────────
    {
        std::uniform_real_distribution<double> d{0.0, 1.0};
        constexpr int N = 100'000;
        int inside = 0;
        for (int i = 0; i < N; ++i) {
            double x = d(rng), y = d(rng);
            if (x*x + y*y <= 1.0) ++inside;
        }
        double pi_est = 4.0 * inside / N;
        std::cout << "[Demo3] Monte Carlo π estimate = "
                  << std::setprecision(4) << pi_est
                  << " (true ≈ " << M_PI << ")\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 real 版是半開 [a, b)？
    //    A：跟「機率密度函式」的數學慣例一致；且做 (b - a) * rng / max 公
    //       式時、b 邏輯上不被取到。實作上保證 result < b。
    //
    //  Q2：精度有上限嗎？
    //    A：double 有 53-bit 尾數 → 約 15~17 位十進位精度。實作通常用
    //       generate_canonical 配合 engine 多次採樣以填滿尾數。
    //
    //  Q3：怎麼產生「常態分佈」的 double？
    //    A：用 std::normal_distribution（見 05 號檔）。內部基於
    //       Box-Muller 或 Ziggurat 演算法。
    //
    demo_practical_jitter();
    demo_practical_disk_sampling();
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 04_uniform_real.cpp -o 04_uniform_real

// === 預期輸出 ===
// [Demo1] N=100000 mean = 0.4991
// [Demo2] 5 samples: 12.9088 19.4849 15.8843 16.9668 19.5808
// [Demo3] Monte Carlo π estimate = 3.1436 (true ≈ 3.1416)
// [Practical] random_jitter (base=1000ms, ±10%):
//   attempt 1 sleep 1053.3 ms
//   attempt 2 sleep 1092.2 ms
//   attempt 3 sleep 1092.6 ms
//   attempt 4 sleep 1041.4 ms
//   attempt 5 sleep 903.9 ms
// [Practical] disk sampling (5 points):
//   (0.445, 0.664)
//   (0.468, 0.402)
//   (0.155, 0.028)
//   (0.703, -0.238)
//   (-0.452, 0.281)
// ⚠️ 上面的位址／執行緒 id／耗時每次執行都不同，數值僅供對照，不是固定結果。
