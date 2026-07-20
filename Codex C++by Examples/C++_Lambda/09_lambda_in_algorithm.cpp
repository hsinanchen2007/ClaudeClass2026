/*
 * Lambda 教科書 09：STL algorithms 中的 lambda
 *
 * 【基本模型】algorithm 管理走訪，lambda closure 保存局部規則與 capture，兩者責任分離。
 * 【API】predicate 回傳可轉 bool 的值；comparator 接受兩元素；transform callable 產生輸出值。
 * 【參數選型】小型 scalar 可傳值；大型唯讀元素通常以 const T& 接收，避免不必要複製。
 * 【capture 選型】固定門檻用 value capture；輸出或共享狀態才用 reference capture。
 * 【callable 生命週期】algorithm 可複製 callable，不能假設最後檢查原 closure 就看到所有狀態。
 * 【capture 生命週期】reference capture 不延長物件壽命；若 callback 被保存，來源必須活得更久。
 * 【元素失效】lambda 本身不決定失效規則；sort/erase 等實際容器操作才會重排或失效 iterator。
 * 【選型】規則只在呼叫點使用時 lambda 最清楚；跨模組重用或需命名 invariant 時用函式物件。
 *
 * 【comparator】sort comparator 必須滿足 strict weak ordering：comp(x,x)==false，且關係具有
 * 非對稱/傳遞性。寫 `<=` 幾乎一定錯，可能導致 undefined behavior。
 * 【複雜度】lambda 不改 algorithm 保證：sort O(n log n)、find_if O(n)、transform O(n)。
 * 【例外安全】callable 若拋例外，sort/remove 類演算法不保證回復原排列；不要假設交易回滾。
 * 【平行版本】execution-policy overload 可能並行且複製 callable，共享可變 capture 需避免資料競爭。
 * 【面試題】為何在 comparator 裡改被比較元素或外部排序 key 很危險？破壞排序不變量。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    std::vector<int> values{5, 2, 8, 1};
    std::sort(values.begin(), values.end(), [](int left, int right) { return left < right; });
    const auto logical_end = std::remove_if(values.begin(), values.end(),
                                            [](int value) { return value % 2 == 0; });
    values.erase(logical_end, values.end());
    assert(values == std::vector<int>({1, 5}));
}
}  // namespace basic

namespace leetcode {
// LeetCode 56：Merge Intervals。sort O(n log n)，merge O(n)。
using Interval = std::pair<int, int>;

std::vector<Interval> leetcode_merge(std::vector<Interval> intervals) {
    std::sort(intervals.begin(), intervals.end(),
              [](const Interval& left, const Interval& right) {
                  return left.first < right.first;
              });
    std::vector<Interval> merged;
    for (const Interval& interval : intervals) {
        if (merged.empty() || merged.back().second < interval.first) {
            merged.push_back(interval);
        } else {
            merged.back().second = std::max(merged.back().second, interval.second);
        }
    }
    return merged;
}

void leetcode_test() {
    const std::vector<Interval> answer = leetcode_merge({{1, 3}, {2, 6}, {8, 10}, {15, 18}});
    assert(answer == std::vector<Interval>({{1, 6}, {8, 10}, {15, 18}}));
    assert(leetcode_merge({}).empty());
    assert(leetcode_merge({{1, 4}, {4, 5}}) == std::vector<Interval>({{1, 5}}));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Job {
    std::string name;
    int duration_ms;
};

std::vector<std::string> practical_slow_jobs(const std::vector<Job>& jobs, int threshold_ms) {
    std::vector<std::string> names;
    names.reserve(jobs.size());
    std::for_each(jobs.begin(), jobs.end(), [&names, threshold_ms](const Job& job) {
        if (job.duration_ms >= threshold_ms) names.push_back(job.name);
    });
    return names;
}

void practical_test() {
    const std::vector<Job> jobs{{"parse", 5}, {"train", 900}, {"checkpoint", 100}};
    assert(practical_slow_jobs(jobs, 100) ==
           std::vector<std::string>({"train", "checkpoint"}));
    assert(practical_slow_jobs({}, 100).empty());
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "algorithm lambda：sort/filter、Merge Intervals、job filter 測試通過\n";
}

/*
 * 【remove-erase】remove_if 只把保留元素搬到前段並回傳 logical end；真正縮短 vector 要再 erase。
 * 【易錯】在 comparator 使用 <=、隨呼叫改變結果，或修改排序 key，都會破壞 strict weak ordering。
 * 【LeetCode 契約】每個 interval 滿足 start<=end；端點相接視為重疊，輸入副本可被排序。
 * 【LeetCode 成本】排序 O(n log n)、合併 O(n)，結果最壞 O(n)；原 caller 容器不被修改。
 * 【實務契約】duration_ms>=threshold_ms 才輸出名稱，保持 jobs 原順序，空輸入回空結果。
 * 【實務例外安全】names 是區域 candidate；配置失敗會拋出，但不會修改 jobs 或外部輸出容器。
 * 【面試追問】為何 stateful lambda 結果難讀？algorithm 可能複製 closure，狀態分散在副本中。
 * 【面試追問】何時避免 reference capture？callback 可能逃離 scope，或平行執行會共享可變狀態時。
 * 【面試追問】lambda 是否讓 sort 更快？不必然；它只利於 inline，總成本仍含比較器每次工作的成本。
 */
