// ============================================================================
// 課題 4：sync_with_stdio、cin.tie 與 fast I/O
// ============================================================================
//
// `ios::sync_with_stdio(false)` 允許 C++ streams 不與 stdio 同步，通常加速大量 I/O；
// `cin.tie(nullptr)` 取消每次 input 前自動 flush cout。兩者應在任何 I/O 前設定。
// 取消同步後混用 cout/printf 或 cin/scanf 的相對順序/行為不可再依賴。
//
// 效能瓶頸也可能是 endl 每行 flush、逐 token allocation、演算法本身。先 benchmark；
// online judge 常用 fast I/O，但一般 interactive CLI 需要 prompt flush/tie。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】批次程式 fast I/O 初始化
// 情境：大量標準輸入/輸出的 batch main 不混用 stdio，希望取消 C/C++ 同步與 cin 對 cout 的自動 tie。
// 為何使用本章主題：sync_with_stdio(false) 與 cin.tie(nullptr) 是程序級 buffering policy；
//       相較每個 parser 個別調整，應由 application entry 統一決定。
// 設計：1. 在任何標準 I/O 前關閉同步；2. 解除 cin tie；3. library parser 只借用 stream，不改全域政策。
// 成本：設定本身時間/空間 O(1)；整體收益取決於 token formatting、flush 與底層裝置成本。
// 上線注意：本教材既有 main 在 basic/LeetCode 輸出後才呼叫此 helper，只能作 API 示意；正式程式
//       必須移到第一次 I/O 前，且之後不可無規則混用 printf/scanf，互動 prompt 要自行 flush。
// -----------------------------------------------------------------------------
void configure_fast_io()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
}

void basic_example()
{
    // 真正 global 設定只在 main 一開始做；此教材先用 stringstream 示範解析正確性。
    std::istringstream input("3 10 20 30");
    int count = 0;
    input >> count;
    int sum = 0;
    for (int index = 0; index < count; ++index) { int value = 0; input >> value; sum += value; }
    assert(sum == 60);
    std::cout << "[基礎] fast-I/O flags change buffering, not parsing semantics\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1672. Richest Customer Wealth（最富有客戶資產）
// 題目：accounts 每列是一位客戶各帳戶金額，回傳最大列總和；{{1,5},{7,3},{3,5}} 回 10。
// 為何使用本章主題：maximum_wealth 是純演算法且不依賴全域 I/O 模式；本例說明 fast I/O
//       只影響 transport/buffering，不應改變計算語意。
// 思路：1. 逐客戶將列內金額加總；2. 與目前 maximum 比較；3. 掃完回最大值。
// 複雜度：時間 O(C)、額外空間 O(1)，C 是 accounts 中所有整數格數。
// 易錯點：本版 maximum 初值 0 依賴原題金額非負；大額總和一般應使用較寬型別防 overflow。
// -----------------------------------------------------------------------------
int maximum_wealth(const std::vector<std::vector<int>>& accounts)
{
    int maximum = 0;
    for (const auto& customer : accounts) {
        int total = 0;
        for (const int value : customer) total += value;
        maximum = std::max(maximum, total);
    }
    return maximum;
}

void leetcode_1672_example()
{
    assert(maximum_wealth({{1, 2, 3}, {3, 2, 1}}) == 6);
    assert(maximum_wealth({{1, 5}, {7, 3}, {3, 5}}) == 10);
    std::cout << "[LeetCode 1672] pure algorithm independent of global I/O mode\n";
}

void practical_example()
{
    configure_fast_io();
    assert(std::cin.tie() == nullptr);
    std::cout << "[實務] fast I/O configured once at application entry\n";
}

int main()
{
    basic_example();
    leetcode_1672_example();
    practical_example();
}

// 易錯與面試：
//   * 這兩個設定是 process-wide standard-stream policy，應在第一次 standard I/O 前決定。
//   * `cin.tie(nullptr)` 後 interactive prompt 要自行 flush，例如 `cout << "> " << flush`。
//   * 關閉同步不是演算法優化，也不會讓 stringstream/file stream 自動變成非同步 I/O。
//   * 取消同步後別交錯 C stdio 與 C++ iostream 並期待固定順序。
// 複雜度仍由讀寫 token/字元數決定；設定本身近似 O(1)，只改 buffering/synchronization。
// 生命週期：global stream 設定持續到 process 結束，library 不應暗中改變 caller 的政策。
// 練習：benchmark `\n` 與 std::endl 寫 100k 行到 ostringstream/file 的差異。

/*
 * 【教科書補充：sync_with_stdio 是程序級初始化決策】
 * - 必須在第一次 C/C++ standard I/O 前設定；已做 I/O 後再切換，行為由實作決定。
 * - 關閉同步後不要依賴 printf 與 cout 的交錯順序，除非自行 flush 並了解實作。
 * - 它是 process-global 狀態，不適合在一般 library function 或可重複測試函式中偷偷修改。
 * - 效能瓶頸若在格式化、磁碟或鎖，關閉同步未必有幫助；應量測完整工作負載。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_sync_with_stdio.cpp' -o '/tmp/codex_cpp_C_IOStream_04_sync_with_stdio' && '/tmp/codex_cpp_C_IOStream_04_sync_with_stdio'
//
// === 預期輸出（節錄）===
// [實務] fast I/O configured once at application entry
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
