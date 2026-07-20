/*
 * std::min：比較兩值或 initializer_list 的最小值
 * ================================================
 * std::min(a,b) 比較一次，等價時回第一個參數；兩參數版本回 const T&。
 * 因此不要長期保存 std::min(temporary1, temporary2) 的 reference：暫時物件在
 * 該完整運算式結束後銷毀。最安全是把結果複製成值：const int m=std::min(1,2)。
 * initializer_list 版本回 T 值，成本 O(N)。比較器必須是嚴格弱序。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode 121：Best Time to Buy and Sell Stock。
// 每天更新截至目前最低買價，再計算今天賣出的最佳利潤，O(N)/O(1)。
int leetcode_max_profit(const std::vector<int>& prices) {
    if (prices.empty()) {
        return 0;
    }
    int lowest = prices.front();
    int best = 0;
    for (int price : prices) {
        lowest = std::min(lowest, price);
        best = std::max(best, price - lowest);
    }
    return best;
}

// 實務：多個限額取最嚴格者；直接存值，不保存可能懸空的 reference。
int practical_effective_upload_limit(int account, int system, int request) {
    return std::min({account, system, request});
}

int main() {
    const int a = 8;
    const int b = 3;
    assert(std::min(a, b) == 3);
    assert(std::min({9, 4, 7, 2}) == 2);

    assert(leetcode_max_profit({7, 1, 5, 3, 6, 4}) == 5);
    assert(leetcode_max_profit({7, 6, 4, 3, 1}) == 0);
    assert(practical_effective_upload_limit(100, 80, 120) == 80);

    std::cout << "min：基本、LC121、限額合併測試通過\n";
}

/*
 * 易錯點：std::min(1, 1.5) 不會自動推導共同型別，兩參數 T 必須一致；可顯式
 * std::min<double>(1,1.5)。NaN 也會讓一般大小直覺失效。
 * 面試：為何 std::min({a,b,c}) 可能複製？initializer_list overload 回值。
 * 練習：把 max_profit 改成同時回傳買入與賣出日。
 *
 * 【LeetCode 不變量】
 * lowest 是目前日以前（含目前日）的最低價格；best 是已看區間內合法的一買一賣
 * 最大利潤。先更新 lowest 再算當日差也安全，因同日買賣只得到 0。
 *
 * 【實務陷阱】
 * 多層限額先確認單位一致（MB/s、MiB/s 不可混用），也要決定 0 是「禁止」還是
 * 「不限速」。std::min 只比較值，不理解業務 sentinel。配置會熱更新時，先取得
 * 同一版本 snapshot，避免三個參數來自不同時刻。
 *
 * 面試延伸：running minimum 也可解「到每個位置為止的最低值」；若要滑動視窗
 * minimum，單純 std::min 不夠，通常用 monotonic deque 將總成本維持 O(N)。
 *
 * 測試要含空 prices、單日、嚴格遞增、嚴格遞減與重複價。LeetCode 題允許空輸入
 * 與否由題目決定；函式契約不可只靠 assert 猜測。
 *
 * 易錯：initializer_list 中各元素仍需同型別，且大物件版本會產生回傳值複製。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'min.cpp' -o '/tmp/codex_cpp_C_Algorithm_min_max_min' && '/tmp/codex_cpp_C_Algorithm_min_max_min'
//
// === 預期輸出（節錄）===
// min：基本、LC121、限額合併測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
