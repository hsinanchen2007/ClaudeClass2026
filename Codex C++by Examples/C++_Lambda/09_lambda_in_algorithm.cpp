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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 56. Merge Intervals（合併區間）
// 題目：合併所有重疊區間；例如 [[1,3],[2,6],[8,10]] 變成 [[1,6],[8,10]]。
// 為何使用本章主題：std::sort 使用 lambda 就地定義依起點排序的 comparator，讓後續線性掃描
// 只需比較目前區間與最後一個合併結果。
// 思路：先依 start 遞增排序；遇到不重疊區間便追加；重疊時把最後 end 擴張到兩者最大值。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 是區間數；排序主導時間，回傳結果最壞 N 筆。
// 易錯點：comparator 必須滿足 strict weak ordering，不能用 <=；端點相接在本實作視為重疊。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】慢工作診斷清單
// 情境：從工作名稱與耗時資料中，依毫秒門檻列出所有需要調查的工作，並維持原順序。
// 為何使用本章主題：for_each 管理走訪，lambda 以值捕獲固定門檻、以參考捕獲輸出；
// 相較手寫迴圈，篩選副作用被集中在呼叫點，但 copy_if 再 transform 也可提供更純的流程。
// 設計：先為最壞結果預留空間；逐筆比較 duration_ms；達門檻便把 name 追加到結果。
// 成本：時間 O(N)、結果空間 O(K)，N 是工作數、K 是慢工作數；目前 reserve 會先配置 N 個容量。
// 上線注意：需拒絕負耗時與不合理門檻；多執行緒收集不可共享未同步的 names，並應保留追蹤 ID。
// -----------------------------------------------------------------------------
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

// 【延伸練習】把收集改成 copy_if+transform，並比較單次走訪與兩階段 pipeline。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_lambda_in_algorithm.cpp' -o '/tmp/codex_cpp_C_Lambda_09_lambda_in_algorithm' && '/tmp/codex_cpp_C_Lambda_09_lambda_in_algorithm'
//
// === 預期輸出（節錄）===
// algorithm lambda：sort/filter、Merge Intervals、job filter 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
