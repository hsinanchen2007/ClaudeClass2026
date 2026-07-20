// ============================================================================
// 課題 1：Input iterator - 單向、single-pass 讀取
// ============================================================================
//
// InputIterator 支援讀取、++、比較；只保證 single pass。複製 iterator 後推進其中一份，
// 另一份不一定仍代表獨立位置（istream_iterator 共用 stream）。不可要求反向、隨機跳躍，
// 也不可反覆跑兩遍同一 input range。
//
// 典型：istream_iterator。Algorithm 若只需讀一次應宣告最弱 category，適用來源最多。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <vector>

void basic_example()
{
    std::istringstream input("1 2 3 4");
    const std::istream_iterator<int> first(input);
    const std::istream_iterator<int> last;
    assert(std::accumulate(first, last, 0) == 10);
    assert(input.eof());
    std::cout << "[基礎] istream input range consumed exactly once\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列的動態和）
// 題目：輸出 answer[i]=nums[0]+...+nums[i]；例如 1 2 3 4 得 [1,3,6,10]。
// 為何使用本章主題：本例把題目輸入改為 stream，展示 input iterator single-pass 讀取後 materialize 再計算。
// 思路：建立 stream 的 first/end；一次消耗成 vector；用 partial_sum 原地累加；回傳結果。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為成功解析的整數數量。
// 易錯點：istream_iterator 不能重走；格式錯誤與 EOF 都會結束 range；int 累加仍可能溢位。
// -----------------------------------------------------------------------------
std::vector<int> running_sum_from_stream(std::istream& input)
{
    // 先把 iterator 命名，可避開 `vector(first, iterator{})` 被解析成函式宣告的
    // most-vexing-parse；也讓 single-pass 的消耗範圍更清楚。
    const std::istream_iterator<int> first(input);
    const std::istream_iterator<int> last;
    std::vector<int> values(first, last);
    std::partial_sum(values.begin(), values.end(), values.begin());
    return values;
}

void leetcode_1480_example()
{
    std::istringstream input("1 2 3 4");
    assert((running_sum_from_stream(input) == std::vector<int>{1, 3, 6, 10}));
    std::cout << "[LeetCode 1480] stream input -> running sums\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】串流量測值的一次性正數過濾
// 情境：從管線讀取空白分隔量測值，只保存有效的正數，例如 -2 3 0 5 產生 [3,5]。
// 為何使用本章主題：input iterator 可直接消耗大型 stream，back_inserter 只配置實際通過篩選的結果。
// 設計：以 istream_iterator 定義輸入；copy_if 判斷 value>0；命中值追加到 vector。
// 成本：時間 O(N)、結果空間 O(P)，N 為解析值數、P 為正數數量；另有 stream I/O 成本。
// 上線注意：要在完成後區分正常 EOF 與 failbit；single-pass 來源不可先 distance，且需限制結果記憶體。
// -----------------------------------------------------------------------------
void practical_example()
{
    std::istringstream input("-2 3 0 5");
    std::vector<int> positives;
    std::copy_if(std::istream_iterator<int>(input), std::istream_iterator<int>(),
                 std::back_inserter(positives), [](int value) { return value > 0; });
    assert((positives == std::vector<int>{3, 5}));
    std::cout << "[實務] one-pass stream filter produced 3,5\n";
}

int main() { basic_example(); leetcode_1480_example(); practical_example(); }

// 易錯與面試：InputIterator 的複本不保證是兩個獨立 cursor；對 stream 先呼叫 distance
// 可能已消耗輸入。面試若問「為何不能 sort」，答案是 sort 需要 random-access 的跳躍、
// 比較位置與交換能力，而 input iterator 只有 single-pass 讀取與 ++。
//
// 工作選型：一次掃大型檔案時，input iterator 可避免先載入全部資料；若流程需排序、
// 回頭重試或多個 consumer，就先 materialize 到 vector，再清楚承擔記憶體成本。
// `istream_iterator<int>` 遇格式錯誤會停在 fail state；production parser 必須另查 fail/eof。
// 同一 iterator 的 `*it` 可重讀目前值，但一旦 ++，舊的 stream position 就不可回復。
// 練習：解釋為何 std::sort 不能接受 input iterators。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_input_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_01_input_iterator' && '/tmp/codex_cpp_C_Iterator_01_input_iterator'
//
// === 預期輸出（節錄）===
// [實務] one-pass stream filter produced 3,5
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
