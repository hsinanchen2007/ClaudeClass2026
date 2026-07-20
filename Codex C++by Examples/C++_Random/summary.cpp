// ============================================================================
// C++ <random> 總複習：engine、distribution、seed、shuffle 與可重播測試
// ============================================================================
//
// 【本章地圖：對應 01～09】
//   why not rand / engines / uniform_int / uniform_real / other distributions /
//   shuffle / seeding / LeetCode 384 Shuffle Array / LeetCode 528 Weighted Pick
//
// 【兩層模型：不要混為一談】
//   engine       產生固定型別的 pseudo-random bit stream；有 state，可 seed/copy/serialize。
//   distribution 把 engine 輸出映射為所需機率模型；可能也有 cache state（例如 normal）。
//   正確形式：`distribution(engine)`，不要自行 `engine() % n`。
//
// 【engine 選型】
//   mt19937       32-bit Mersenne Twister；週期大、可重播、state 大，非密碼安全。
//   mt19937_64    64-bit 輸出；同樣非 CSPRNG。
//   minstd_rand   state 小、快，但統計品質/週期用途有限。
//   random_device 可能接 OS entropy，也允許 deterministic 實作；查 entropy() 仍非安全 API 契約。
//   安全 token/key/nonce：使用作業系統 CSPRNG 或經審核 crypto library，不用 <random> engine。
//
// 【distribution 選型與邊界】
//   uniform_int_distribution<int>(a,b)   離散整數 [a,b]，兩端 inclusive
//   uniform_real_distribution<double>(a,b) 通常 [a,b)，不要以 ==b 當 invariant
//   bernoulli_distribution(p)            bool，true 機率 p
//   binomial_distribution(n,p)           n 次 Bernoulli 的成功數
//   normal_distribution(mean,stddev)     常態；可 cache 下一樣本，param/reset 要留意
//   exponential_distribution(lambda)     事件間隔；lambda > 0
//   poisson_distribution(rate)           固定期間內的非負事件計數；rate > 0
//   discrete_distribution(weights...)    類別機率與非負 weights 成比例
//
// 【seed policy】
//   單元測試/模擬：固定且記錄 seed，failure 可重播。
//   production 非安全抽樣：可用 random_device 組 seed_seq；仍要把 seed 寫 log 便於診斷。
//   同 seed + 同 engine type + 同標準 engine 呼叫序列可重播；distribution 產生的精確序列
//   不一定跨所有 standard library 實作相同，所以跨平台 golden test 不綁 distribution 值。
//   不要每次抽樣都以目前時間重建 engine：解析度碰撞、品質差、也失去 state。
//
// 【常用 API / 複雜度】
//   engine.seed(s)          重設 state；成本依 engine
//   engine.discard(z)       丟棄 z 次結果，通常 O(z)
//   distribution.reset()   清內部 cache，不改 parameter
//   std::shuffle(first,last,engine) O(n)，需要 random-access iterator
//   weighted prefix build  O(n)，每次 lower_bound pick O(log n)
//   reservoir sampling     單 pass O(n) time；抽 1 筆為 O(1) extra，抽 k 筆為 O(k)
//
// 【經典陷阱】
//   - `rand()%n` 可能 modulo bias，且 rand 全域 state/品質/範圍都差。
//   - `std::random_shuffle` 已移除；用 std::shuffle 並明確傳 engine。
//   - uniform_int 上界 inclusive；產生 vector index 要用 [0,size-1] 且先檢查非空。
//   - 權重累加可能 overflow；先檢查正值與 total 的型別範圍。
//   - 隨機測試不要 assertion「每桶剛好相等」；驗 range/invariant 或寬鬆統計界線。
//   - shared engine 多執行緒 data race；每 thread engine 或外部同步，並規劃 seed stream。
//
// 【面試快問快答】
//   Q: 為何 modulo bias？ A: engine 範圍大小若不是 n 的倍數，某些餘數多一個來源值。
//   Q: rejection sampling 怎麼修？ A: 拒絕無法均分的尾端 states，再取 modulo。
//   Q: Fisher-Yates 為何均勻？ A: 第 i 步從尚未固定的 i+1 個位置等機率選一個，
//      每個 permutation 機率乘積皆為 1/n!。
//   Q: mt19937 可否產生密碼？ A: 不可；state 可由足夠輸出推回，且無前向安全性。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

void basic_distribution_demo()
{
    std::mt19937 first(2026U);
    std::mt19937 second(2026U);
    for (int sample = 0; sample < 20; ++sample) {
        assert(first() == second()); // engine 本身相同 seed 可精確重播
    }

    std::mt19937 engine(7U);
    std::uniform_int_distribution<int> die(1, 6);
    std::uniform_real_distribution<double> ratio(0.0, 1.0);
    std::bernoulli_distribution failed(0.2);
    std::poisson_distribution<int> requests_per_second(4.0);
    for (int sample = 0; sample < 100; ++sample) {
        const int face = die(engine);
        const double value = ratio(engine);
        assert(face >= 1 && face <= 6);
        assert(value >= 0.0 && value < 1.0);
        static_cast<void>(failed(engine));
        assert(requests_per_second(engine) >= 0);
    }

    std::vector<int> values{1, 2, 3, 4, 5};
    std::shuffle(values.begin(), values.end(), engine);
    std::sort(values.begin(), values.end());
    assert((values == std::vector<int>{1, 2, 3, 4, 5})); // shuffle 保留 permutation
}

// ---------------------------------------------------------------------------
// LeetCode 528：Random Pick with Weight
// prefix=[1,4,6] 時，在整數 [1,6] 均勻抽 target，再 lower_bound 找第一個 >= target。
// constructor O(n)，pick O(log n)，space O(n)。權重必須正值。
// ---------------------------------------------------------------------------
class WeightedPicker {
public:
    WeightedPicker(const std::vector<std::uint64_t>& weights, std::uint32_t seed)
        : engine_(seed)
    {
        if (weights.empty()) throw std::invalid_argument("empty weights");
        prefix_.reserve(weights.size());
        std::uint64_t total = 0U;
        for (const std::uint64_t weight : weights) {
            if (weight == 0U || total > UINT64_MAX - weight) {
                throw std::invalid_argument("invalid/overflowing weight");
            }
            total += weight;
            prefix_.push_back(total);
        }
    }

    std::size_t pick_index()
    {
        std::uniform_int_distribution<std::uint64_t> target(1U, prefix_.back());
        const auto iterator = std::lower_bound(prefix_.begin(), prefix_.end(), target(engine_));
        return static_cast<std::size_t>(std::distance(prefix_.begin(), iterator));
    }

private:
    std::vector<std::uint64_t> prefix_;
    std::mt19937 engine_;
};

void leetcode_528_demo()
{
    WeightedPicker picker({1U, 3U, 2U}, 99U);
    std::vector<int> counts(3U, 0);
    for (int sample = 0; sample < 6'000; ++sample) {
        const std::size_t index = picker.pick_index();
        assert(index < counts.size());
        ++counts[index];
    }
    // 不把有限樣本的桶數大小順序寫成 correctness assertion：合法亂數仍可能偏離期望，
    // 且 distribution 的精確序列不具跨標準庫可攜性。這裡只驗 API invariant。
    assert(std::accumulate(counts.begin(), counts.end(), 0) == 6'000);
    assert(std::all_of(counts.begin(), counts.end(), [](int count) {
        return count >= 0;
    }));
}

// ---------------------------------------------------------------------------
// 實務：可重播的服務故障注入。
// production 測試失敗時記 seed；同 seed 會得到相同 latency/failure plan。
// ---------------------------------------------------------------------------
struct RequestOutcome {
    bool failed;
    int latency_ms;
    friend bool operator==(const RequestOutcome&, const RequestOutcome&) = default;
};

std::vector<RequestOutcome> simulate_requests(std::uint32_t seed, std::size_t count)
{
    std::mt19937 engine(seed);
    std::bernoulli_distribution failure(0.1);
    std::normal_distribution<double> latency(40.0, 5.0);

    std::vector<RequestOutcome> outcomes;
    outcomes.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
        const bool failed = failure(engine);
        const int milliseconds = std::max(1, static_cast<int>(std::lround(latency(engine))));
        outcomes.push_back(RequestOutcome{failed, milliseconds});
    }
    return outcomes;
}

void practical_simulation_demo()
{
    const auto first = simulate_requests(0xC0FFEEU, 200U);
    const auto replay = simulate_requests(0xC0FFEEU, 200U);
    assert(first == replay);
    assert(first.size() == 200U);
    assert(std::all_of(first.begin(), first.end(), [](const RequestOutcome& outcome) {
        return outcome.latency_ms > 0;
    }));
}

int main()
{
    basic_distribution_demo();
    leetcode_528_demo();
    practical_simulation_demo();
    std::cout << "Random summary: all assertions passed\n";
}
