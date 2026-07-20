// ============================================================================
// 課題 1：為什麼不優先使用 std::rand()
// ============================================================================
//
// rand/srand 有 hidden global state、quality/range 小且 implementation-dependent、難安全
// 並行，也把 engine 與 distribution 混在一起。`rand()%n` 在 RAND_MAX+1 不能整除 n 時
// 有 modulo bias。<random> 把「產生均勻 bits 的 engine」與「映射成需求分布」分開。
//
// 一般 simulation/test：mt19937 + 明確 seed。非決定性 seed：random_device（但某些平台
// 可能 deterministic，查 entropy）。密碼/token/session key 不應假設 mt19937/random_device
// 足夠，使用 OS CSPRNG/安全函式庫。
// ============================================================================

#include <array>
#include <cassert>
#include <iostream>
#include <random>

void basic_example()
{
    std::mt19937 engine(42U);
    std::uniform_int_distribution<int> die(1, 6); // inclusive [1,6]
    for (int roll = 0; roll < 100; ++roll) {
        const int value = die(engine);
        assert(value >= 1 && value <= 6);
    }
    std::cout << "[基礎] mt19937 + uniform_int_distribution produced valid dice\n";
}

// LeetCode 470：Implement Rand10() Using Rand7()。
// 兩次 rand7 形成 [0,48] 的 49 個等機率值；只接受前 40 個，mod 10 才無 bias。
class Rand10 {
public:
    explicit Rand10(unsigned seed) : engine_(seed) {}
    int rand10()
    {
        while (true) {
            const int value = (rand7() - 1) * 7 + (rand7() - 1); // [0,48]
            if (value < 40) return value % 10 + 1;
        }
    }
private:
    int rand7()
    {
        return std::uniform_int_distribution<int>(1, 7)(engine_);
    }
    std::mt19937 engine_;
};

void leetcode_470_example()
{
    Rand10 generator(123U);
    std::array<int, 10> counts{};
    for (int sample = 0; sample < 10'000; ++sample) {
        const int value = generator.rand10();
        assert(value >= 1 && value <= 10);
        ++counts.at(static_cast<std::size_t>(value - 1));
    }
    for (const int count : counts) assert(count > 800 && count < 1'200); // 寬鬆 sanity check。
    std::cout << "[LeetCode 470] rejection sampling avoids modulo bias\n";
}

// 實務：A/B bucket 用 deterministic hash 比 runtime PRNG 更穩定，使用者重啟後不換組。
int stable_bucket(unsigned long user_id, int buckets)
{
    assert(buckets > 0);
    const unsigned long mixed = user_id * 1'140'071'481'932'319'849UL;
    return static_cast<int>(mixed % static_cast<unsigned long>(buckets));
}

void practical_example()
{
    assert(stable_bucket(42UL, 100) == stable_bucket(42UL, 100));
    std::cout << "[實務] A/B assignment uses stable hash, not rand global state\n";
}

int main()
{
    basic_example();
    leetcode_470_example();
    practical_example();
}

// 練習：數學推導為何直接 `(rand7()-1)%10+1` 不可能產生 8/9/10。
// 複雜度：rejection sampling 每輪 O(1)，期望輪數有限但最壞理論上無上界。
// 生命週期：PRNG state 應由 object/fixture 長期擁有；global rand state 使測試互相干擾。

/*
【本課面試問答】
Q1：`rand()%N` 為何可能有 modulo bias？
A：若 `RAND_MAX+1` 不是 N 的倍數，某些餘數會對應較多輸入，因此機率不同。可用標準 distribution，
或只接受落在最大完整倍數區間內的值再取餘數（rejection sampling）。

Q2：固定 seed 是 bug 還是功能？
A：測試與模擬需要固定 seed 才能重播；production 若需要每次不同，可由外部 entropy 建 seed，並把
seed 記進 log 以便重現。不要在每次 draw 前用目前秒數重播種，短時間內會得到重複序列。

Q3：`mt19937` 可否產生密碼、token 或 session ID？
A：不可。它是可預測的非密碼學 PRNG，觀察足夠輸出可推回狀態。安全 token 應使用作業系統 CSPRNG
或經審核的密碼函式庫；`random_device` 的品質也必須依平台文件確認。
*/
