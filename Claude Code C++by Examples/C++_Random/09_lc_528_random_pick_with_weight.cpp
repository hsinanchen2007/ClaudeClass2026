// =============================================================================
//  09_lc_528_random_pick_with_weight.cpp  —  LeetCode 528. Random Pick with Weight
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
//    - https://en.cppreference.com/w/cpp/algorithm/partial_sum
//    - https://en.cppreference.com/w/cpp/algorithm/lower_bound
//    - https://leetcode.com/problems/random-pick-with-weight/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 題意                                                       │
//  └────────────────────────────────────────────────────────────┘
//
//  輸入：weights（正整數陣列）。例如 [1, 3]
//  pickIndex()：每次回傳一個 index i，且 P(i) = weights[i] / sum(weights)
//
//  例：[1, 3]
//   → P(0) = 1/4, P(1) = 3/4
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 思路：累積前綴和 + 二分搜                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  weights = [1, 3, 2]
//  prefix  = [1, 4, 6]                  (sum = 6)
//
//  抽 r = uniform(1, 6)（注意：不是 [0, 5]，是 [1, 6] 包含；或用 [0, 5] 配
//  upper_bound 搜尋一致都行）。本實作用 [1, sum] 配 lower_bound 的「第一
//  個 >= r 的 index」 — 概念最清楚。
//
//  時間：建構 O(n)；pickIndex O(log n)；空間 O(n)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 對照：std::discrete_distribution 一行解                    │
//  └────────────────────────────────────────────────────────────┘
//
//  C++ 標準有現成的：
//
//      std::discrete_distribution<int> dist{weights.begin(), weights.end()};
//      int idx = dist(rng);
//
//  我們同時示範手寫版（教學）與標準版。
//
// =============================================================================

/*
補充筆記：std::random_pick_with_weight
  - std::random_pick_with_weight 使用 <random>；現代 C++ 把亂數引擎和分佈分開，引擎產生位元，分佈把位元轉成需要的機率模型。
  - std::rand 品質與範圍有限，也常被錯誤取模造成偏差；新程式應優先使用 <random>。
  - seed 決定可重現性；測試常用固定 seed，正式隨機可用 random_device 或更完整的 seed sequence。
  - uniform_int_distribution 端點通常包含上下界，uniform_real_distribution 常見語意是 [a,b)；使用前要確認範圍。
  - shuffle 應搭配亂數引擎；不要使用 random_shuffle，因為它已被移除。
  - 加權抽樣、常態分佈、伯努力分佈等應使用對應 distribution，不要手寫容易有偏差的算法。
  - prefix sum 陣列必須單調遞增，lower_bound 才能把亂數落點對應到正確 index。
  - 若總權重很大，uniform_int_distribution 的型別也要跟著使用 long long 或對應整數型別。
*/
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <vector>

// ─────────────────────────────────────────────────────────
// LeetCode 478. Generate Random Point in a Circle  (難度: medium，不同題)
// 題意：給圓心 (x_center, y_center) 與半徑 r，randPoint() 在圓內均勻取樣。
//
// 思路（拒絕取樣，最直觀正確）：
//   1. 對 [-r, r] x [-r, r] 正方形均勻採樣 (x, y)
//   2. 如果 x² + y² > r² 就丟棄重抽
//   3. 否則平移加圓心、回傳
// 期望嘗試次數 = (4 / π) ≈ 1.27 次，效能足夠。
//
// 為什麼適合本主題：和 528 同樣是「以均勻分布為基礎，組出特殊分布」。
// 528 用累積前綴 + binary search；478 用拒絕取樣 — 兩種互補的隨機抽樣技巧。
// ─────────────────────────────────────────────────────────
class CirclePicker {
public:
    CirclePicker(double radius, double x_center, double y_center)
        : r_(radius), cx_(x_center), cy_(y_center),
          rng_(std::random_device{}()),
          dist_(-radius, radius) {}

    std::pair<double, double> randPoint() {
        while (true) {
            double x = dist_(rng_);
            double y = dist_(rng_);
            if (x * x + y * y <= r_ * r_)
                return {x + cx_, y + cy_};
        }
    }

private:
    double r_, cx_, cy_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
};

static void demo_lc478_circle_point() {
    std::cout << "[LC478] 圓心 (0,0) 半徑 1，取 5 個點：\n";
    CirclePicker cp{1.0, 0.0, 0.0};
    for (int i = 0; i < 5; ++i) {
        auto [x, y] = cp.randPoint();
        double d = std::sqrt(x * x + y * y);
        std::cout << "  (" << x << ", " << y << ")  dist=" << d << '\n';
    }
}

class WeightedPickerManual {
public:
    explicit WeightedPickerManual(const std::vector<int>& weights)
        : prefix_(weights.size()),
          rng_(std::random_device{}())
    {
        std::partial_sum(weights.begin(), weights.end(), prefix_.begin());
        // 抽 r ∈ [1, prefix_.back()]
        dist_ = std::uniform_int_distribution<int>{1, prefix_.back()};
    }

    int pickIndex() {
        int r = dist_(rng_);
        // 第一個 prefix[i] >= r → 就是答案 index
        auto it = std::lower_bound(prefix_.begin(), prefix_.end(), r);
        return static_cast<int>(it - prefix_.begin());
    }

private:
    std::vector<int> prefix_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
};

class WeightedPickerStd {
public:
    explicit WeightedPickerStd(const std::vector<int>& weights)
        : rng_(std::random_device{}()),
          dist_(weights.begin(), weights.end()) {}

    int pickIndex() { return dist_(rng_); }

private:
    std::mt19937 rng_;
    std::discrete_distribution<int> dist_;
};

int main() {
    std::vector<int> weights{1, 3, 2};   // 期望比例 1:3:2 = 16.67%, 50%, 33.33%

    auto runTrial = [&](auto& picker, const std::string& tag) {
        std::map<int, int> tally;
        constexpr int N = 60'000;
        for (int i = 0; i < N; ++i) ++tally[picker.pickIndex()];
        std::cout << tag << " counts (期望: ~" << N * weights[0] / 6
                  << ' ' << N * weights[1] / 6
                  << ' ' << N * weights[2] / 6 << "):";
        for (auto& [k, v] : tally) std::cout << " idx" << k << '=' << v;
        std::cout << '\n';
    };

    WeightedPickerManual manual{weights};
    WeightedPickerStd    std_  {weights};
    runTrial(manual, "[manual]");
    runTrial(std_,   "[std]   ");

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼用 lower_bound 不是 upper_bound？
    //    A：題目意義是「r 落在 weight[i] 對應的區間」。我們把 r 設成
    //       [1, sum]：
    //         weights=[1,3,2] prefix=[1,4,6]
    //         r=1 → idx 0 (lower_bound 找到 1, idx 0)
    //         r=2 → idx 1 (lower_bound 找到 4, idx 1)
    //         r=4 → idx 1
    //         r=5 → idx 2
    //         r=6 → idx 2
    //       lower_bound 對應「第一個 >= r 的 prefix index」，剛好是答案。
    //
    //  Q2：浮點 weight 怎麼辦？
    //    A：std::discrete_distribution 接受 double 等浮點 weight，內部會自
    //       行 normalize。手寫的話用 partial_sum + uniform_real_distribution。
    //
    //  Q3：能不能用 alias method 達 O(1) pick？
    //    A：可以；alias method 預處理 O(n)，每次抽 O(1)。標準的
    //       discrete_distribution 部分實作就用 alias method。手寫成本較高，
    //       面試說「用 prefix + binary search O(log n)」就夠用。
    //
    demo_lc478_circle_point();
    return 0;
}
