// ============================================================================
// 課題 5：bernoulli、normal、binomial、poisson、discrete distributions
// ============================================================================
//
// Distribution 應匹配模型，不只是產生不同形狀：bernoulli 單次成功、binomial 固定次數
// 成功數、poisson 固定時間事件數、normal 連續測量誤差、exponential 到下一事件時間、
// discrete_distribution 依離散 weights 選 index。
//
// 參數是模型假設，不能因資料看起來像鐘形就盲套 normal。固定 seed 可重播測試，但
// statistical test 要寬容 sampling noise；不要要求平均值精確等於理論值。
// ============================================================================

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

void basic_example()
{
    std::mt19937 engine(42U);
    std::bernoulli_distribution coin(0.25);
    int successes = 0;
    for (int trial = 0; trial < 10'000; ++trial) if (coin(engine)) ++successes;
    assert(successes > 2'200 && successes < 2'800);

    std::normal_distribution<double> noise(0.0, 1.0);
    double sum = 0.0;
    for (int sample = 0; sample < 10'000; ++sample) sum += noise(engine);
    assert(std::abs(sum / 10'000.0) < 0.1);
    std::cout << "[基礎] Bernoulli/normal samples pass broad sanity checks\n";
}

// LeetCode 528：Random Pick with Weight。discrete_distribution 直接依 weights 回傳 index，
// 因此功能契約完整；第 9 課再手寫 prefix-sum + binary search，方便面試說明複雜度。
class WeightedPicker {
public:
    WeightedPicker(const std::vector<double>& weights, unsigned seed)
        : distribution_(weights.begin(), weights.end()), engine_(seed) {}
    std::size_t pickIndex() { return distribution_(engine_); }
private:
    std::discrete_distribution<std::size_t> distribution_;
    std::mt19937 engine_;
};

void leetcode_528_example()
{
    WeightedPicker picker({1.0, 3.0}, 12U);
    std::array<int, 2> counts{};
    for (int sample = 0; sample < 10'000; ++sample) ++counts.at(picker.pickIndex());
    assert(counts[1] > counts[0] * 2 && counts[1] < counts[0] * 4);
    std::cout << "[LeetCode 528] pickIndex 依 1:3 權重抽樣\n";
}

// 實務：每天平均 4 incidents 的 Poisson simulation；只驗非負與合理 sample mean。
void practical_example()
{
    std::mt19937 engine(88U);
    std::poisson_distribution<int> incidents(4.0);
    int total = 0;
    constexpr int days = 10'000;
    for (int day = 0; day < days; ++day) {
        const int count = incidents(engine);
        assert(count >= 0);
        total += count;
    }
    const double average = static_cast<double>(total) / static_cast<double>(days);
    assert(average > 3.8 && average < 4.2);
    std::cout << "[實務] simulated incident mean=" << average << '\n';
}

int main()
{
    basic_example();
    leetcode_528_example();
    practical_example();
}

// 易錯與面試：distribution 參數代表統計模型，不是「換個函式讓數字看起來隨機」。
// 測試統計性質要容許波動並固定 seed；不要用單次樣本 assert 平均/常態分布成立。
// 練習：用 binomial_distribution 模擬 100 requests、每個成功率 0.9 的成功數。
// 複雜度：不同 distribution 的 sampling algorithm/成本由實作與參數決定，不能一概當 O(1)。
// 生命週期：某些 distribution 可保存 cached state；改參數/重播時要理解 param() 與 reset()。
