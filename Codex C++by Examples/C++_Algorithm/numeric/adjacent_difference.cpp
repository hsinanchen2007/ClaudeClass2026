/*
 * std::adjacent_difference：相鄰元素差分與前綴狀態轉換
 * =====================================================
 * 輸出第一項等於輸入第一項；之後預設為 current - previous。
 * 自訂 binary op 時，之後輸出 op(current, previous)，參數次序不能寫反。
 *
 * 時間 O(N)、額外空間 O(1)（不計輸出）。input 與 output iterator 即可。
 * 輸出範圍必須至少容納 N 項；可合法原地轉換，但原地寫法較難閱讀。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <numeric>
#include <vector>

// LeetCode 1109：Corporate Flight Bookings 的差分陣列核心。
std::vector<int> leetcode_flight_bookings(
    const std::vector<std::vector<int>>& bookings, int flight_count) {
    std::vector<int> difference(static_cast<std::size_t>(flight_count + 1), 0);
    for (const auto& booking : bookings) {
        const int first = booking[0] - 1;
        const int last_exclusive = booking[1];
        difference[static_cast<std::size_t>(first)] += booking[2];
        difference[static_cast<std::size_t>(last_exclusive)] -= booking[2];
    }

    std::vector<int> answer(static_cast<std::size_t>(flight_count));
    std::partial_sum(difference.begin(), difference.begin() + flight_count,
                     answer.begin());
    return answer;
}

// 實務：累積電表讀數轉為每小時用量，並保留負值供上層判定感測器重置。
std::vector<int> practical_hourly_usage(const std::vector<int>& meter) {
    std::vector<int> usage(meter.size());
    std::adjacent_difference(meter.begin(), meter.end(), usage.begin());
    if (!usage.empty()) {
        usage.front() = 0;  // 第一項是基準讀數，不是「第一小時用量」。
    }
    return usage;
}

int main() {
    const std::vector<int> readings{100, 108, 121, 121};
    std::vector<int> delta(readings.size());
    std::adjacent_difference(readings.begin(), readings.end(), delta.begin());
    assert((delta == std::vector<int>{100, 8, 13, 0}));

    // 自訂乘法產生 current * previous；第一項仍原樣複製。
    std::vector<int> products(readings.size());
    std::adjacent_difference(readings.begin(), readings.end(), products.begin(),
                             std::multiplies<>{});
    assert((products == std::vector<int>{100, 10800, 13068, 14641}));

    assert((leetcode_flight_bookings({{1, 2, 10}, {2, 3, 20}, {2, 5, 25}}, 5) ==
            std::vector<int>{10, 55, 45, 25, 25}));
    assert((practical_hourly_usage({100, 108, 121, 121}) ==
            std::vector<int>{0, 8, 13, 0}));

    std::cout << "adjacent_difference：差分與區間更新測試通過\n";
}

/*
 * 易錯陷阱：
 * - 第一個輸出不是 0；若業務要 delta，必須明確覆寫或另外處理基準。
 * - callback 是 op(current, previous)，不是 op(previous, current)。減法寫反會變號。
 * - 寫到同一 vector 雖被標準支援，但除錯時看不到原始輸入，教材與 production
 *   通常用獨立輸出，除非記憶體壓力已量測證明值得原地做。
 * - LeetCode 差分技巧要配置 n+1 哨兵，才能在 right+1 安全扣回；索引轉換最常
 *   發生 off-by-one。先在紙上區分 1-based 題目與 0-based vector。
 *
 * 面試：adjacent_difference 與 partial_sum 是近似反操作；前者壓成變化量，後者
 * 還原累積值。實務中可用於時間序列壓縮、事件 sourcing、區間批次更新。
 *
 * 練習：加入時間戳，若兩筆間隔不是一小時，輸出每分鐘平均用量；要處理時間倒退
 * 與計量器 overflow，並決定錯誤是回 optional、expected 還是拋例外。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'adjacent_difference.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_adjacent_difference' && '/tmp/codex_cpp_C_Algorithm_numeric_adjacent_difference'
//
// === 預期輸出（節錄）===
// adjacent_difference：差分與區間更新測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
