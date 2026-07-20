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

// LeetCode 1480：以 input iterators 建 vector，再做 running sum。
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

// 實務：copy_if 可從 stream single-pass 過濾正數到 vector。
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
