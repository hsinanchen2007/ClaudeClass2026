/*
 * std::partial_sort：讓前 K 個成為全體最小 K 個，且前 K 已排序
 * ============================================================
 * 呼叫 partial_sort(first,middle,last)，結果 [first,middle) 完整排序；後半順序未指定。
 * 比較約 O(N log K)，適合 K 遠小於 N。K=0（middle==first）與 K=N 都合法。
 * 需要 random-access iterator，原地重排元素。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 973. K Closest Points to Origin（最接近原點的 K 個點）
// 題目：輸入平面 points 與 k，回距離原點最近的 k 點，順序不限；例如 (1,3)、(-2,2)、
// (5,8) 取 2 會包含前兩點。
// 為何使用本章主題：partial_sort 將距離最小 K 點放到前段並排序；題目雖不要求順序，
// 此 API 以 O(N log K) 一次完成選取與前 K 排序。
// 思路：1. 以 long long 計算 squared distance；2. partial_sort 到 begin+k；3. erase 後段。
// 複雜度：時間 O(N log K)、額外空間 O(N)，N 為點數；空間來自按值輸入副本。
// 易錯點：x*x 要先升格；middle 是前 K 的尾後位置；同距離順序未明定。
// -----------------------------------------------------------------------------
using Point = std::vector<int>;

std::vector<Point> leetcode_k_closest(std::vector<Point> points, int k) {
    assert(k >= 0 && static_cast<std::size_t>(k) <= points.size());
    const auto distance2 = [](const Point& point) {
        const long long x = point[0];
        const long long y = point[1];
        return x * x + y * y;
    };
    const auto middle = points.begin() + k;
    std::partial_sort(points.begin(), middle, points.end(),
                      [&distance2](const Point& lhs, const Point& rhs) {
                          return distance2(lhs) < distance2(rhs);
                      });
    points.erase(middle, points.end());
    return points;
}

struct Alert {
    int id;
    int severity;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】最高嚴重度 Alert Top-K
// 情境：告警批次只需前 K 筆；severity 高者先，同 severity 以 id 小者先，超過資料量
// 時回全部告警。
// 為何使用本章主題：partial_sort 只排序所需前 K，K 遠小於 N 時比完整 sort 少工作，
// tie-breaker 讓輸出 deterministic。
// 設計：1. clamp K 到 alerts.size；2. 以 severity 降冪、id 升冪比較；3. partial_sort；
// 4. erase 非 top-K 後段。
// 成本：時間 O(N log K)、額外空間 O(N)，N 為告警數；按值副本保留呼叫端資料。
// 上線注意：K=0 合法且結果空；若 alerts 為無限串流，應改固定大小 heap。
// -----------------------------------------------------------------------------
std::vector<Alert> practical_top_alerts(std::vector<Alert> alerts, std::size_t k) {
    k = std::min(k, alerts.size());
    const auto middle = alerts.begin() + static_cast<std::ptrdiff_t>(k);
    std::partial_sort(alerts.begin(), middle, alerts.end(),
                      [](const Alert& lhs, const Alert& rhs) {
                          if (lhs.severity != rhs.severity) {
                              return lhs.severity > rhs.severity;
                          }
                          return lhs.id < rhs.id;
                      });
    alerts.erase(middle, alerts.end());
    return alerts;
}

int main() {
    std::vector<int> values{9, 1, 8, 2, 7, 3};
    std::partial_sort(values.begin(), values.begin() + 3, values.end());
    assert((std::vector<int>(values.begin(), values.begin() + 3) ==
            std::vector<int>{1, 2, 3}));

    const auto points = leetcode_k_closest({{1, 3}, {-2, 2}, {5, 8}}, 2);
    assert(points.size() == 2U && points[0] == Point({-2, 2}));

    const auto alerts = practical_top_alerts({{2, 5}, {1, 9}, {3, 9}, {4, 1}}, 2U);
    assert(alerts[0].id == 1 && alerts[1].id == 3);

    std::cout << "partial_sort：LC973 與 top-K alerts 測試通過\n";
}

/*
 * 易錯陷阱：
 * - middle 指向「不包含」的尾端，因此 K 個元素是 [first, first+K)。
 * - 後半不是原順序，也不保證排序；不可拿來做額外推論。
 * - 只需第 K 值選 nth_element；需前 K 但不需排序可 nth_element；前 K 要排序才
 *   partial_sort。K 接近 N 時完整 sort 可能更簡單且性能相近。
 * - distance 平方使用 long long，避免 int x*x 溢位。
 *
 * 生命週期：erase 會讓 middle 及其後 iterator 失效，所以先完成所有使用再 erase。
 * 面試應說出 O(N log K) 與 heap intuition；實務 tie-breaker 要 deterministic，避免
 * 同 severity 每次順序不同。練習：大量 streaming alerts 改固定大小 min-heap。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'partial_sort.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_partial_sort' && '/tmp/codex_cpp_C_Algorithm_sorting_partial_sort'
//
// === 預期輸出（節錄）===
// partial_sort：LC973 與 top-K alerts 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
