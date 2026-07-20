// ============================================================================
// 課題 3：std::uniform_int_distribution
// ============================================================================
//
// uniform_int_distribution<Int>(a,b) 對每個整數 a..b 給相同機率，兩端都包含。它處理
// rejection/mapping，避免 `% range` bias。每次呼叫會修改 engine；distribution 可重用，
// 也可透過 param_type 暫時換 range。
//
// a>b 不符合前置條件；動態空容器不可建立 [0,size-1]。選 index 使用 size_t distribution，
// 避免先把大 size narrowing 成 int。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

void basic_example()
{
    std::mt19937 engine(11U);
    std::uniform_int_distribution<int> die(1, 6);
    for (int count = 0; count < 1'000; ++count) {
        const int roll = die(engine);
        assert(roll >= 1 && roll <= 6);
    }
    std::cout << "[基礎] integer distribution is inclusive [1,6]\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 398. Random Pick Index（隨機選取索引）
// 題目：給定可含重複值的陣列，每次 pick(target) 要等機率回傳任一 target 索引；例如三個 3 位於 2、3、4。
// 為何使用本章主題：uniform_int_distribution(1,seen) 實作 reservoir sampling，使第 k 個命中以 1/k 機率取代，無須另存索引表。
// 思路：1. 掃描陣列。2. 每遇到 target 遞增 seen。3. 均勻抽 1..seen，抽到 1 就更新 chosen。4. 回傳結果。
// 複雜度：每次 pick 時間 O(N)、演算法額外空間 O(1)；物件另持有輸入陣列 O(N)，N 為 nums 長度。
// 易錯點：分布上界是 inclusive；target 不存在時本教材回 -1，而原題保證存在，且 engine 必須跨 pick 呼叫保留狀態。
// -----------------------------------------------------------------------------
class Solution {
public:
    Solution(std::vector<int> nums, unsigned seed)
        : nums_(std::move(nums)), engine_(seed) {}
    int pick(int target)
    {
        int chosen = -1;
        int seen = 0;
        for (std::size_t index = 0U; index < nums_.size(); ++index) {
            if (nums_.at(index) != target) continue;
            ++seen;
            if (std::uniform_int_distribution<int>(1, seen)(engine_) == 1) {
                chosen = static_cast<int>(index);
            }
        }
        return chosen;
    }
private:
    std::vector<int> nums_;
    std::mt19937 engine_;
};

void leetcode_398_example()
{
    Solution solution({1, 2, 3, 3, 3}, 42U);
    for (int trial = 0; trial < 100; ++trial) {
        const int index = solution.pick(3);
        assert(index >= 2 && index <= 4);
    }
    assert(solution.pick(9) == -1);
    std::cout << "[LeetCode 398] reservoir picks only target indices 2..4\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】健康後端的等機率選擇
// 情境：負載平衡器已過濾出 endpoint_count 個健康節點，要在 [0,count) 中均勻選一個索引。
// 為何使用本章主題：uniform_int_distribution<size_t> 正確處理閉區間映射，比 engine()%count 少 modulo bias 與 narrowing 風險。
// 設計：1. 先拒絕空清單。2. 建立 [0,count-1] 分布。3. 以呼叫端長期持有的 engine 取樣。4. 回傳 size_t index。
// 成本：單次抽樣通常為期望 O(1)、額外空間 O(1)；拒絕映射的確切成本由實作與 engine 範圍決定。
// 上線注意：健康清單與 count 必須來自同一 snapshot；共享 engine 需同步或每執行緒獨立，且安全流量分配另需權重與熔斷策略。
// -----------------------------------------------------------------------------
std::size_t choose_endpoint(std::size_t endpoint_count, std::mt19937& engine)
{
    if (endpoint_count == 0U) throw std::invalid_argument("no endpoints");
    return std::uniform_int_distribution<std::size_t>(0U, endpoint_count - 1U)(engine);
}

void practical_example()
{
    std::mt19937 engine(7U);
    for (int trial = 0; trial < 20; ++trial) assert(choose_endpoint(3U, engine) < 3U);
    std::cout << "[實務] endpoint selection stays within [0,count)\n";
}

int main()
{
    basic_example();
    leetcode_398_example();
    practical_example();
}

// 練習：統計 60,000 次骰子，檢查每面約 10,000，但不要把窄區間當永久單元測試。
// 複雜度：單次 distribution sample 通常期望 O(1)；reservoir scan 是 O(N)、空間 O(1)。
// 生命週期：distribution 可臨時建立，但 engine state 必須跨 draws 保留，才形成連續序列。

/*
【本課面試問答】
Q1：`uniform_int_distribution(a,b)` 的端點是否包含？
A：整數分布是閉區間 `[a,b]`。要抽 vector index 應使用 `[0,size-1]`，並先拒絕空容器；把上界寫成
`size()` 會偶爾越界，這類低機率 bug 很難由少量測試抓到。

Q2：為何 engine 通常以 non-const reference 傳入？
A：每次生成都會更新 engine state；按值傳入會複製狀態，可能讓每次呼叫得到同一段序列。以 reference
傳入也讓 ownership 清楚：fixture/service 擁有 state，演算法只推進它。

Q3：相同 engine type 與 seed 是否保證所有平台產生完全相同最終樣本？
A：標準 engine（如 `mt19937`）的序列有規格，但 distribution 將 engine bits 映射到目標分布的演算法
可因標準庫實作不同。跨平台 bit-for-bit 重播必須固定整個演算法/版本，而不只 seed。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_uniform_int.cpp' -o '/tmp/codex_cpp_C_Random_03_uniform_int' && '/tmp/codex_cpp_C_Random_03_uniform_int'
//
// === 預期輸出（節錄）===
// [實務] endpoint selection stays within [0,count)
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
