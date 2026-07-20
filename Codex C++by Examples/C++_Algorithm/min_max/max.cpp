/*
 * std::max：取較大值，注意 reference 與 tie 規則
 * ================================================
 * 兩參數版通常回 const T&，比較一次；等價時回第一個參數。
 * initializer_list 版回值並做 O(N) 比較。不要把兩個 temporary 的回傳參考保存
 * 到下一個 statement；也不要用 <= 當 comparator（會破壞嚴格弱序）。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

// LeetCode 53：Maximum Subarray（Kadane）。
int leetcode_max_sub_array(const std::vector<int>& nums) {
    assert(!nums.empty());
    int ending_here = nums.front();
    int best = nums.front();
    for (std::size_t i = 1; i < nums.size(); ++i) {
        ending_here = std::max(nums[i], ending_here + nums[i]);
        best = std::max(best, ending_here);
    }
    return best;
}

// 實務：服務的 worker 數量至少為安全下限。
int practical_effective_workers(int configured, int minimum_safe) {
    return std::max(configured, minimum_safe);
}

int main() {
    const int left = 4;
    const int right = 9;
    assert(std::max(left, right) == 9);
    assert(std::max({-7, -2, -10}) == -2);

    assert(leetcode_max_sub_array({-2, 1, -3, 4, -1, 2, 1, -5, 4}) == 6);
    assert(leetcode_max_sub_array({-3}) == -3);
    assert(practical_effective_workers(2, 4) == 4);

    std::cout << "max：基本、Kadane、worker 下限測試通過\n";
}

/*
 * Kadane 面試關鍵：ending_here 是「必須以目前元素結尾」的最佳和；best 是全域
 * 最佳。初始化為 0 會讓全負數案例錯答 0，因此必須從 nums.front() 開始。
 * 練習：擴充回傳最佳子陣列的 [begin,end) index。
 *
 * 【LeetCode 複雜度】
 * Kadane 只掃一次，時間 O(N)、額外空間 O(1)。若加總可能超出 int，要將
 * ending_here/best 升為 long long；型別安全不能靠 std::max 自動解決。
 *
 * 【實務選擇】
 * worker 下限只是一側約束，若還有硬體上限應再用 clamp。configured 的負值究竟
 * 是錯誤還是「自動」也需先正規化，不能讓 max 默默掩蓋壞設定而不記錄告警。
 *
 * 易錯陷阱：`const auto& result=std::max(make_a(),make_b())` 會在 statement 後懸空；
 * 直接用 value 接收。若型別 move/copy 很昂貴且來源為 lvalue，reference 才有價值。
 *
 * 面試延伸：Maximum Product Subarray 不能只保留最大乘積，因負數會讓最小乘積翻成
 * 最大；需同時維護 current_min/current_max。這也說明選演算法前先分析轉移性質。
 *
 * LeetCode 全負測試是 Kadane 必測；實務 worker 還要測 0、負值與超過硬體上限。
 *
 * 練習：回傳最大子陣列的起訖 index，並在相同總和時選最短區間。
 * 延伸：若資料是 stream，Kadane 可持續更新 best，但無法在丟棄歷史後重建 index。
 * 實務監控要保留足夠 metadata，而不是只有單一最大值。
 * 完成後再以 std::max_element 當小型資料 oracle，交叉驗證最佳值。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'max.cpp' -o '/tmp/codex_cpp_C_Algorithm_min_max_max' && '/tmp/codex_cpp_C_Algorithm_min_max_max'
//
// === 預期輸出（節錄）===
// max：基本、Kadane、worker 下限測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
