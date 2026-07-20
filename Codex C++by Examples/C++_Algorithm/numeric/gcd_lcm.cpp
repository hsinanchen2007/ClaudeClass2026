/*
 * std::gcd / std::lcm：整數最大公因數與最小公倍數
 * ================================================
 * C++17 起位於 <numeric>。支援整數型別；gcd(0,0)==0，gcd(a,0)==abs(a)。
 * lcm 任一輸入為 0 時回 0。時間約 O(log min(|a|,|b|))，底層使用 Euclid 思想。
 *
 * 注意：若結果無法由共同型別表示，lcm 可能涉及未定義行為；大數先約分再乘，
 * 並在業務層做 overflow 檢查。不要自行寫 `a*b/gcd` 而先乘爆。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1979. Find Greatest Common Divisor of Array（找出陣列的最大公約數）
// 題目：找 nums 中最小值與最大值的最大公因數；例如 [2,5,6,9,10] 回 gcd(2,10)=2。
// 為何使用本章主題：std::gcd 已封裝 Euclid 演算法與符號正規化；題目只需先線性
// 取得兩個極值，不必對所有元素連續求 gcd。
// 思路：1. 以首元素初始化 smallest/largest；2. 掃描更新兩極；3. 回 gcd(smallest,largest)。
// 複雜度：時間 O(N+log V)、額外空間 O(1)，N 為元素數、V 為極值絕對值尺度。
// 易錯點：nums 不可空；若值含最小可表示負數，取絕對值可能超出共同型別。
// -----------------------------------------------------------------------------
int leetcode_find_gcd(const std::vector<int>& nums) {
    assert(!nums.empty());
    int smallest = nums.front();
    int largest = nums.front();
    for (const int value : nums) {
        if (value < smallest) {
            smallest = value;
        }
        if (value > largest) {
            largest = value;
        }
    }
    return std::gcd(smallest, largest);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】多排程週期共同觸發間隔
// 情境：periods 是多個正整數秒數，要找它們下一次同時觸發的 LCM；若結果超過
// 業務 limit，回 0 表示不可排程。
// 為何使用本章主題：每輪先以 gcd 約分 answer，再乘新 period，可降低中間值並在
// 乘法前用 limit/period 檢查溢位。
// 設計：1. answer 以 LCM identity 1 開始；2. 求 divisor；3. 先除再檢查乘法上限；
// 4. 安全時更新 answer。
// 成本：時間 O(P log V)、額外空間 O(1)，P 為週期數、V 為目前數值尺度。
// 上線注意：period 必須大於 0；0 同時可能是合法 LCM 與錯誤 sentinel，正式 API 宜回 optional/expected。
// -----------------------------------------------------------------------------
long long practical_common_interval(const std::vector<int>& periods,
                                    long long limit) {
    long long answer = 1;
    for (const int period : periods) {
        assert(period > 0);
        const long long divisor = std::gcd(answer, static_cast<long long>(period));
        const long long reduced = answer / divisor;
        if (reduced > limit / period) {
            return 0;
        }
        answer = reduced * period;
    }
    return answer;
}

int main() {
    assert(std::gcd(48, 18) == 6);
    assert(std::gcd(-48, 18) == 6);
    assert(std::gcd(0, 0) == 0);
    assert(std::lcm(6, 8) == 24);
    assert(std::lcm(0, 8) == 0);

    assert(leetcode_find_gcd({2, 5, 6, 9, 10}) == 2);
    assert(practical_common_interval({4, 6, 10}, 1000) == 60);
    assert(practical_common_interval({1000, 1001, 1003}, 1'000'000) == 0);

    std::cout << "gcd/lcm：整數契約、LC1979 與排程週期測試通過\n";
}

/*
 * 選擇與易錯：
 * - 約分、比例正規化、分數運算選 gcd；重複週期會合選 lcm。
 * - `%` 對負數的語意容易讓手寫 Euclid 出錯；標準 gcd 已正規化為非負結果。
 * - `std::gcd(INT_MIN, 0)` 若絕對值無法由共同型別表示，同樣不安全；可先升格
 *   到足夠寬的型別，但 long long 的最小值仍需特別處理。
 * - 浮點數不可直接使用 gcd/lcm；先確認可接受的量化尺度，轉成整數單位。
 *
 * 面試：為何 lcm(a,b)=abs(a/gcd(a,b)*b) 要先除後乘？因為數學相等，但機器整數
 * 的中間值範圍有限。仍需檢查最終乘法，這正是 practical_common_interval 所做。
 *
 * 真實用途：音訊 sample rate 對齊、分片大小、排程器 tick、分數約分。若週期很多，
 * LCM 很快成長；production API 應讓「超出上限」有明確型別，而非以 0 混淆合法值。
 * 練習：改用 std::optional<long long> 表達 overflow，並為空 periods 定義 identity=1。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'gcd_lcm.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_gcd_lcm' && '/tmp/codex_cpp_C_Algorithm_numeric_gcd_lcm'
//
// === 預期輸出（節錄）===
// gcd/lcm：整數契約、LC1979 與排程週期測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
