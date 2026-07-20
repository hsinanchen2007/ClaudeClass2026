// ============================================================================
// 課題 7：Seeding 策略 - replay 與 entropy 是不同目標
// ============================================================================
//
// 固定 seed：tests/experiments 可重播。random_device seed：每次可能不同，適合一般非安全
// simulation，但 random_device::entropy()==0 可能表示 deterministic implementation。
// seed_seq 可把多個 32-bit seed words 擴散進 engine state；單一 32-bit seed 只覆蓋
// mt19937 巨大 state space 的很小部分，但對一般測試通常足夠。
//
// 若 failure 無法重播，randomized testing 價值大減：每次印/保存 seed。不要用目前秒數
// 當唯一 seed，並行 processes 可能同秒取得相同值。安全 token 請用 OS CSPRNG。
// ============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

std::mt19937 make_engine_from_words(const std::array<std::uint32_t, 4>& words)
{
    std::seed_seq sequence(words.begin(), words.end());
    return std::mt19937(sequence);
}

void basic_example()
{
    const std::array<std::uint32_t, 4> seed{1U, 2U, 3U, 4U};
    auto first = make_engine_from_words(seed);
    auto replay = make_engine_from_words(seed);
    for (int index = 0; index < 100; ++index) assert(first() == replay());
    std::cout << "[基礎] seed_seq words reproduce raw mt19937 state\n";
}

// LeetCode 380：RandomizedSet 測試注入 seed，讓 getRandom failure 可重播。
class RandomizedSet {
public:
    explicit RandomizedSet(std::uint32_t seed) : engine_(seed) {}
    bool insert(int value)
    {
        if (std::find(values_.begin(), values_.end(), value) != values_.end()) return false;
        values_.push_back(value);
        return true;
    }
    int getRandom()
    {
        const std::size_t index =
            std::uniform_int_distribution<std::size_t>(0U, values_.size() - 1U)(engine_);
        return values_.at(index);
    }
private:
    std::vector<int> values_;
    std::mt19937 engine_;
};

std::vector<int> sample_sequence(std::uint32_t seed)
{
    RandomizedSet set(seed);
    set.insert(10);
    set.insert(20);
    set.insert(30);
    std::vector<int> result;
    for (int count = 0; count < 20; ++count) result.push_back(set.getRandom());
    return result;
}

void leetcode_380_example()
{
    assert(sample_sequence(99U) == sample_sequence(99U));
    for (const int value : sample_sequence(99U)) assert(value == 10 || value == 20 || value == 30);
    std::cout << "[LeetCode 380] injected seed replays getRandom calls\n";
}

// 實務：取得 nondeterministic seed 時保存它；random_device 不是密碼學保證。
std::uint32_t runtime_seed()
{
    std::random_device device;
    return device();
}

void practical_example()
{
    const std::uint32_t seed = runtime_seed();
    std::mt19937 engine(seed);
    (void)engine();
    std::cout << "[實務] record this seed for replay: " << seed << '\n';
}

int main()
{
    basic_example();
    leetcode_380_example();
    practical_example();
}

// 易錯與面試：每次抽樣都重新 seed 會降低品質且破壞 replay；engine 應保存狀態。random_device
// 也不保證每個平台都有 non-deterministic entropy，密碼用途需專門 CSPRNG，不用 mt19937。
// 練習：接受 CLI `--seed`；未給才 random_device，並把實際 seed 寫進 test report。
// 複雜度：seed_seq 初始化與 engine state 大小成正比；每次 draw 才是 engine 的常態成本。
// 生命週期：seed 只初始化 state，不需保活；engine 必須保活，否則每次重建都回序列起點。

/*
 * 【教科書補充：seed 與空集合契約】
 * - random_device 不保證硬體熵或 non-deterministic；安全 token 應使用平台 CSPRNG，而非 mt19937。
 * - 對空集合建立 [0,size-1] distribution 會下溢；getRandom 應回 optional 或丟明確例外。
 * - 若資料結構名稱對應 LeetCode 380，應明示線性搜尋版本只是 seed 教材，不具平均 O(1) insert/remove。
 * - seed_seq 可展開多個 seed word；測試保存固定 seed，production 則記錄 seed 以利重現診斷。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_seeding.cpp' -o '/tmp/codex_cpp_C_Random_07_seeding' && '/tmp/codex_cpp_C_Random_07_seeding'
//
// === 預期輸出（節錄）===
// [LeetCode 380] injected seed replays getRandom calls
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
