/*
 * std::for_each：對範圍每個元素呼叫函式物件
 * ==========================================
 * sequential overload 會對每個元素呼叫一次，O(N)，並回傳移動後的 function object，
 * 因此 stateful functor 可在結束後取累積狀態。若 iterator 非 const，callable 可改值；
 * 但本章放在 non-modifying，是因典型統計用途不改來源。
 *
 * execution policy 版本的順序/執行緒不同，不可用未同步 shared state。callback 不應
 * 對同一容器做會使 iterator 失效的 insert/erase。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1672. Richest Customer Wealth（最富有客戶的資產總量）
// 題目：accounts[i][j] 是第 i 位客戶在第 j 家銀行的資產，回傳任一客戶的最大總額；
// 例如 [[1,5],[7,3],[3,5]] 回 10。
// 為何使用本章主題：for_each 逐客戶執行相同聚合動作，內層 accumulate 求該列總額，
// 外部 capture 更新 richest；一般迴圈同樣可讀，本例展示 callback 聚合。
// 思路：1. richest 初始化為 0；2. 對每列加總 wealth；3. 以 max 更新目前最大值。
// 複雜度：時間 O(T)、額外空間 O(1)，T 為 accounts 中所有元素總數。
// 易錯點：題目資產非負才適合 richest=0；一般有負值資料應由首列或 optional 初始化。
// -----------------------------------------------------------------------------
int leetcode_maximum_wealth(const std::vector<std::vector<int>>& accounts) {
    int richest = 0;
    std::for_each(accounts.begin(), accounts.end(), [&richest](const auto& customer) {
        const int wealth = std::accumulate(customer.begin(), customer.end(), 0);
        richest = std::max(richest, wealth);
    });
    return richest;
}

struct Metric {
    int value;
    bool alert;
};

struct MetricSummary {
    int total;
    int alerts;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】監控 Metric 總量與警報摘要
// 情境：Metric 快照含 value 與 alert；報表要同時累加所有值與警報筆數，不修改來源。
// 為何使用本章主題：for_each 適合在每筆上更新一個局部 Summary 狀態，兩個欄位可在
// 同一趟完成，比各跑一次 accumulate/count_if 少一次掃描。
// 設計：1. summary 初始化為零；2. 每筆加 value；3. alert 為 true 時遞增 alerts；
// 4. 以值回傳摘要。
// 成本：時間 O(N)、額外空間 O(1)，N 為 metric 筆數。
// 上線注意：int total 可能溢位；parallel for_each 不可未同步共享 summary，應改 reduction。
// -----------------------------------------------------------------------------
MetricSummary practical_summarize_metrics(const std::vector<Metric>& metrics) {
    MetricSummary summary{0, 0};
    std::for_each(metrics.begin(), metrics.end(), [&summary](const Metric& metric) {
        summary.total += metric.value;
        if (metric.alert) {
            ++summary.alerts;
        }
    });
    return summary;
}

int main() {
    const std::vector<int> values{1, 2, 3};
    int sum = 0;
    std::for_each(values.begin(), values.end(), [&sum](int value) { sum += value; });
    assert(sum == 6);
    assert((values == std::vector<int>{1, 2, 3}));

    assert(leetcode_maximum_wealth({{1, 2, 3}, {3, 2, 1}}) == 6);
    assert(leetcode_maximum_wealth({{1, 5}, {7, 3}, {3, 5}}) == 10);

    const auto summary = practical_summarize_metrics({{10, false}, {20, true}});
    assert(summary.total == 30 && summary.alerts == 1);
    std::cout << "for_each：LeetCode 1672 與實務 metric 聚合測試通過\n";
}

/*
 * 易錯陷阱：用 for_each 做可由 transform/accumulate 表達的事情，可能降低可讀性；
 * 需要 early break 時 for_each 不適合，改一般 loop 或 find_if。
 *
 * 面試：for_each 的 callable 可否有 state？可以，sequential overload 會回傳它；但
 * capture reference 更直接時不必炫技。LeetCode 本例總成本是所有 account element
 * 數量 O(T)。實務加總可能 overflow，正式 metrics 用 long long。
 * 練習：寫 functor 並從 for_each 回傳值讀取 total/alerts。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'for_each.cpp' -o '/tmp/codex_cpp_C_Algorithm_non_modifying_for_each' && '/tmp/codex_cpp_C_Algorithm_non_modifying_for_each'
//
// === 預期輸出（節錄）===
// for_each：LeetCode 1672 與實務 metric 聚合測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
