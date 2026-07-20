/*
 * Numeric algorithms 面試前總複習
 * ================================
 * 本檔涵蓋同目錄八個 API：
 *   accumulate、adjacent_difference、gcd/lcm、inner_product、iota、
 *   partial_sum、reduce、transform_reduce。
 * 目標是五分鐘先用「順序、輸入/輸出、結合律」選對工具，再回想細節。
 *
 * 【選擇表】
 * 需求                                      首選
 * 固定左到右 fold                           accumulate
 * 兩序列 zip 後固定順序聚合                 inner_product
 * 可重排、可能向量化/平行聚合               reduce
 * map 後可重排聚合且不想建中間容器           transform_reduce
 * 產生連續值或索引                          iota
 * 建立 prefix query                         partial_sum
 * 累積值轉相鄰變化、difference encoding      adjacent_difference
 * 約分/整數週期會合                          gcd / lcm
 *
 * 【複雜度與 iterator】
 * - 上述掃描類全部時間 O(N)；gcd/lcm 約 O(log min(a,b))。
 * - accumulate/reduce/partial_sum 等不自行配置結果容器。
 * - 需要輸出的演算法，呼叫者必須預先配置足夠空間。
 * - inner_product/二元 transform_reduce 只收到第二個 begin，必須自行驗長度。
 *
 * 【生命週期與失效】
 * 演算法只在呼叫期間使用 iterator，不延長容器生命週期。callback 若 push_back 到
 * 同一 vector，可能重配置並使正在使用的 iterator 全失效；也不可回傳指向區域暫存
 * 的 view。輸出與輸入重疊是否安全要逐 API 查契約，勿由「剛好能跑」推論。
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct DailyMetric {
    int requests;
    int failures;
    double latency_ms;
};

struct NumericReport {
    long long total_requests;
    long long worst_daily_increase;
    std::optional<double> request_weighted_latency_ms;
    std::vector<long long> request_prefix;
};

// 本檔報告 API 的執行期契約：計數非負、failures<=requests、latency 為有限非負值。
// 若違反就丟 invalid_argument；所有整數 aggregate 另要求可表示為 long long。
void validate_daily_metrics(const std::vector<DailyMetric>& days) {
    for (const auto& day : days) {
        if (day.requests < 0 || day.failures < 0 ||
            day.failures > day.requests || !std::isfinite(day.latency_ms) ||
            day.latency_ms < 0.0) {
            throw std::invalid_argument("invalid DailyMetric");
        }
    }
}

// 基礎整合：一次展示 accumulate、adjacent_difference、partial_sum、inner_product。
NumericReport basic_numeric_report(const std::vector<DailyMetric>& days) {
    validate_daily_metrics(days);

    // partial_sum 的 accumulator 跟隨 input value_type；只把 output 設為 long long
    // 不會讓 int 累加自動升格。因此在任何加法前先把來源轉成 long long。
    std::vector<long long> requests;
    std::vector<double> latency;
    requests.reserve(days.size());
    latency.reserve(days.size());
    for (const auto& day : days) {
        requests.push_back(static_cast<long long>(day.requests));
        latency.push_back(day.latency_ms);
    }

    const long long total = std::accumulate(requests.begin(), requests.end(), 0LL);
    std::vector<long long> changes(requests.size());
    std::adjacent_difference(requests.begin(), requests.end(), changes.begin());
    if (!changes.empty()) {
        changes.front() = 0;
    }
    const long long worst = changes.empty()
                                ? 0LL
                                : *std::max_element(changes.begin(), changes.end());

    std::vector<long long> prefix(requests.size() + 1U, 0LL);
    std::partial_sum(requests.begin(), requests.end(), prefix.begin() + 1);

    // requests 才是權重；除以總請求數後才是 request-weighted mean latency。
    const double weighted_latency_total = std::inner_product(
        latency.begin(), latency.end(), requests.begin(), 0.0);
    if (!std::isfinite(weighted_latency_total)) {
        throw std::overflow_error("weighted latency is not finite");
    }
    std::optional<double> weighted_latency;
    if (total > 0) {
        weighted_latency = weighted_latency_total / static_cast<double>(total);
    }
    return {total, worst, weighted_latency, std::move(prefix)};
}

// LeetCode 1732：Find the Highest Altitude。
int leetcode_largest_altitude(const std::vector<int>& gain) {
    std::vector<int> altitude(gain.size() + 1U, 0);
    std::partial_sum(gain.begin(), gain.end(), altitude.begin() + 1);
    return *std::max_element(altitude.begin(), altitude.end());
}

struct UnhealthyReport {
    long long total_failures;
    std::vector<std::size_t> ranked_day_indices;
};

// 實務：以索引排序保留 metrics，並把 transform_reduce 的失敗總量納入回傳報告。
UnhealthyReport practical_rank_unhealthy(
    const std::vector<DailyMetric>& days) {
    validate_daily_metrics(days);
    const long long total_failures = std::transform_reduce(
        days.begin(), days.end(), 0LL, std::plus<>{},
        [](const DailyMetric& day) {
            return static_cast<long long>(day.failures);
        });

    std::vector<std::size_t> order(days.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&days](std::size_t lhs,
                                                        std::size_t rhs) {
        return days[lhs].failures > days[rhs].failures;
    });
    return {total_failures, std::move(order)};
}

int main() {
    const std::vector<DailyMetric> days{
        {100, 2, 10.0}, {120, 8, 12.0}, {90, 1, 9.0}};
    const NumericReport report = basic_numeric_report(days);
    assert(report.total_requests == 310);
    assert(report.worst_daily_increase == 20);
    assert(report.request_weighted_latency_ms.has_value());
    assert(std::abs(*report.request_weighted_latency_ms - 3250.0 / 310.0) <
           1e-12);
    assert((report.request_prefix == std::vector<long long>{0, 100, 220, 310}));

    assert(leetcode_largest_altitude({-5, 1, 5, 0, -7}) == 1);
    const UnhealthyReport unhealthy = practical_rank_unhealthy(days);
    assert(unhealthy.total_failures == 11);
    assert((unhealthy.ranked_day_indices ==
            std::vector<std::size_t>{1U, 0U, 2U}));

    const NumericReport no_traffic = basic_numeric_report({});
    assert(no_traffic.total_requests == 0);
    assert(!no_traffic.request_weighted_latency_ms.has_value());
    assert((no_traffic.request_prefix == std::vector<long long>{0}));

    bool rejected_invalid_metric = false;
    try {
        static_cast<void>(basic_numeric_report({{10, 11, 1.0}}));
    } catch (const std::invalid_argument&) {
        rejected_invalid_metric = true;
    }
    assert(rejected_invalid_metric);

    assert(std::gcd(84, 30) == 6);
    assert(std::lcm(6, 8) == 24);
    const std::vector<int> empty;
    assert(std::reduce(empty.begin(), empty.end(), 0) == 0);

    std::cout << "Numeric summary：八類演算法整合測試通過\n";
}

/*
 * 【重要陷阱清單】
 * 1. init 決定累加型別：long long 資料配 0 仍可能用 int；寫 0LL。
 * 2. reduce/transform_reduce 可重排；減法、字串串接、依賴順序的 state machine
 *    不可使用。浮點加法也會因分組產生末位差異。
 * 3. partial_sum 第一輸出就是第一輸入；若要 prefix[0]=0，配置 n+1。它的內部
 *    accumulator 使用 input value_type，寬 output 不會補救窄型別 overflow；先轉型，
 *    或改用帶 0LL init 的 inclusive_scan。
 * 4. adjacent_difference 第一輸出同樣是第一輸入，不是 0；operation 參數為
 *    op(current, previous)。
 * 5. inner_product 與二元 transform_reduce 不知道第二範圍終點；長度不足是 UB。
 * 6. iota 需要既有可寫元素；reserve 不等於 resize。
 * 7. lcm 先除 gcd 再乘只能降低 overflow 風險，不能保證結果一定可表示。
 * 8. callback 不可讓來源 iterator 失效，也不能在 par_unseq 做未同步副作用。
 * 9. 有號整數 overflow 是 UB；統計不只要換 long long，也要估算上界。
 * 10. 金額用浮點會有精度問題；優先以最小貨幣單位整數表示。
 *
 * 【面試快問快答】
 * Q: accumulate 與 reduce 差在哪？
 * A: 前者固定左到右；後者允許重排，operation 必須可安全分組。
 * Q: partial_sum 與 inclusive_scan？
 * A: 都做 inclusive prefix；partial_sum 沒有 init overload，accumulator 跟 input
 *    value_type。inclusive_scan 可給 0LL init，且 scan 家族支援 execution policy。
 * Q: difference array 為何可做區間更新？
 * A: 區間 [l,r] 只在 diff[l]+=x、diff[r+1]-=x，最後一次 prefix 還原全部值。
 * Q: transform_reduce 為何可能比 transform + reduce 好？
 * A: 可融合，避免中間配置與額外 memory pass；實際速度仍需 benchmark。
 * Q: 何時不用 numeric algorithm？
 * A: 邏輯有多個早退、複雜錯誤處理或一個 loop 同時維護多個狀態時，清楚的 for
 *    loop 往往更可維護。演算法是表達意圖，不是消滅所有迴圈。
 *
 * 【選型決策流程】
 * 先問是否需要每個 prefix/差分輸出；若是選 partial_sum/adjacent_difference。
 * 若只要單值，再問是否 zip 兩範圍或先 map；依序選 inner_product 或
 * transform_reduce。最後問 operation 是否可重排；不可就 accumulate，可才 reduce。
 * 產生索引是 iota；整數數論關係是 gcd/lcm。
 *
 * 【本檔報告契約】
 * basic_numeric_report 對非法 DailyMetric 丟 invalid_argument；整數總和須可放進
 * long long。request_weighted_latency_ms = sum(latency_ms*requests)/sum(requests)，
 * 總 requests 為 0 時回 nullopt，不把「無流量」偽裝成 0 ms。practical_rank_unhealthy
 * 回傳 total_failures 與完整 index 排名；按 failures 降序，同值因 stable_sort 保留日期
 * 原順序。inner_product 的兩範圍在本檔由同一個 days 建立，長度必然相同。
 *
 * 【面試前自我檢查】
 * - 我能說出每個 API 對空範圍的結果嗎？
 * - 我能解釋 output iterator 容量、第二範圍長度與 iterator 失效嗎？
 * - 我能舉出浮點不具結合律的後果嗎？
 * - 我能從 LC1109 推導差分，再由 partial_sum 還原嗎？
 * - 我能證明自訂 reduce combine 是 associative 嗎？
 *
 * 練習：
 * 1. 為 NumericReport 加 p95 latency；注意它不是 numeric fold，通常需 nth_element。
 * 2. 實作 checked_lcm 回 optional，測 0、負數、最大值。
 * 3. 實作二維 prefix sum 並回答 half-open 矩形查詢。
 * 4. 用高精度基準比較 accumulate 與 reduce 的浮點誤差。
 * 5. 將 rank_unhealthy 改成 partial top-k，避免完整排序。
 *
 * 最後提醒：範圍的 begin/end 必須來自同一個活著的物件。不要分別建立兩個
 * temporary vector 再取 iterator；即使內容相同，它們仍屬不同範圍。main 中的
 * 空範圍測試特別使用具名 `empty`，讓 iterator 的來源與生命週期清楚可見。
 */
