// ============================================================================
// 課題 6：std::shuffle 與 Fisher-Yates
// ============================================================================
//
// std::shuffle(first,last,engine) 產生均勻隨機 permutation（假設 URBG 品質良好），時間
// O(N)。舊 random_shuffle 使用 global rand/implementation callback，C++17 已移除。
// 正確 Fisher-Yates 在 i 從 n-1 降到 1 時，從 [0,i] 均勻選 j 並 swap；若每次都從
// [0,n-1] 選，permutations 不是等機率。
//
// shuffle 修改原容器；需要 reset 時保留 original copy。測試應驗「是原資料排列」，
// 不應斷言必定改變順序（原排列本身也是合法隨機結果）。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

bool is_permutation_1_to_n(const std::vector<int>& values)
{
    std::vector<int> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    std::vector<int> expected(values.size());
    std::iota(expected.begin(), expected.end(), 1);
    return sorted == expected;
}

void basic_example()
{
    std::vector<int> cards(52U);
    std::iota(cards.begin(), cards.end(), 1);
    std::mt19937 engine(42U);
    std::shuffle(cards.begin(), cards.end(), engine);
    assert(cards.size() == 52U);
    assert(is_permutation_1_to_n(cards));
    std::cout << "[基礎] shuffled deck remains a 1..52 permutation\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 384. Shuffle an Array（打亂陣列）
// 題目：保存原陣列，shuffle 回傳等機率排列且 reset 可還原；此檔只示範不改原輸入的 shuffle 核心，完整類別在第 8 課。
// 為何使用本章主題：std::shuffle 以呼叫端 engine 實作 Fisher-Yates 類均勻排列，取代已移除且依賴全域狀態的 random_shuffle。
// 思路：1. 複製 input。2. 對副本呼叫 std::shuffle。3. 回傳副本。4. 測試 permutation 且確認 input 未變。
// 複雜度：N 個元素的時間 O(N)、回傳副本空間 O(N)，N 為陣列長度。
// 易錯點：原排列也是合法結果，不能斷言一定改變；engine 必須持續推進，且本 helper 本身沒有實作 reset API。
// -----------------------------------------------------------------------------
std::vector<int> shuffled_copy(const std::vector<int>& input, std::mt19937& engine)
{
    std::vector<int> output = input;
    std::shuffle(output.begin(), output.end(), engine);
    return output;
}

void leetcode_384_example()
{
    const std::vector<int> original{1, 2, 3};
    std::mt19937 engine(7U);
    const auto result = shuffled_copy(original, engine);
    assert(std::is_permutation(result.begin(), result.end(), original.begin(), original.end()));
    assert((original == std::vector<int>{1, 2, 3}));
    std::cout << "[LeetCode 384] shuffle returns permutation without mutating original\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可重播的測試執行順序
// 情境：測試框架要隨機化 0..test_count-1 的執行順序，以暴露順序依賴，失敗時用 seed=2026 完整重播。
// 為何使用本章主題：固定 seed 的 mt19937 加 std::shuffle 同時提供隨機排列與診斷重現，比全域 rand 更容易隔離測試。
// 設計：1. 驗證 count 非負。2. 用 iota 建立所有測試 ID。3. 由 seed 建 engine。4. 原地 shuffle 後回傳。
// 成本：建立與洗牌時間 O(T)、輸出空間 O(T)，T 為測試數；engine state 為固定額外成本。
// 上線注意：報告必須記 seed、測試清單版本與工具鏈；同 seed 的 distribution/shuffle 細節未必跨標準庫完全一致。
// -----------------------------------------------------------------------------
std::vector<int> randomized_test_order(int test_count, unsigned seed)
{
    if (test_count < 0) throw std::invalid_argument("negative test count");
    std::vector<int> order(static_cast<std::size_t>(test_count));
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 engine(seed);
    std::shuffle(order.begin(), order.end(), engine);
    return order;
}

void practical_example()
{
    const auto first = randomized_test_order(10, 2026U);
    const auto replay = randomized_test_order(10, 2026U);
    assert(first == replay);
    std::cout << "[實務] seed 2026 exactly replays test order\n";
}

int main()
{
    basic_example();
    leetcode_384_example();
    practical_example();
}

// 易錯與面試：`random_shuffle` 已移除，因其隨機來源不可控；std::shuffle 要傳 URBG。
// 用 `sort` 搭隨機 key 不是無偏 Fisher-Yates，collision 與排序代價也讓它不合適。
// 練習：手寫 Fisher-Yates，與 std::shuffle 比較 permutation invariant。
// 生命週期：shuffle 原地修改 range；iterators 必須在操作期間有效，engine state 由 caller 持有。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_shuffle.cpp' -o '/tmp/codex_cpp_C_Random_06_shuffle' && '/tmp/codex_cpp_C_Random_06_shuffle'
//
// === 預期輸出（節錄）===
// [實務] seed 2026 exactly replays test order
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
