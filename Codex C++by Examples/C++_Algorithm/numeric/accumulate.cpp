/*
 * std::accumulate：依固定次序做左摺疊（left fold）
 * =================================================
 * 標頭 <numeric>。形式可視為 init = op(init, *first)，直到 first == last。
 * 預設 op 是加法；自訂 op 可做乘積、字串串接、結構彙總。
 *
 * 契約與成本：
 * - 輸入範圍可為 input iterator；空範圍直接回 init。
 * - 時間 O(N)，額外空間 O(1)，呼叫順序固定，所以適合不可交換的運算。
 * - 回傳型別由 init 決定，不是由容器元素決定。init 寫 0 會得到 int 累加。
 * - 演算法不保存 iterator；但執行期間不可讓 callback 使範圍失效。
 */

#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// LeetCode 1672：Richest Customer Wealth。
int leetcode_maximum_wealth(const std::vector<std::vector<int>>& accounts) {
    int best = 0;
    for (const auto& row : accounts) {
        const int wealth = std::accumulate(row.begin(), row.end(), 0);
        if (wealth > best) {
            best = wealth;
        }
    }
    return best;
}

struct Invoice {
    std::string customer;
    long long cents;
};

// 實務：財務資料用整數分幣，避免 double 的二進位小數誤差。
long long practical_total_cents(const std::vector<Invoice>& invoices) {
    return std::accumulate(
        invoices.begin(), invoices.end(), 0LL,
        [](long long total, const Invoice& invoice) {
            return total + invoice.cents;
        });
}

int main() {
    const std::vector<int> values{1, 2, 3, 4};
    assert(std::accumulate(values.begin(), values.end(), 10) == 20);

    // 順序固定，故可安全示範非交換的字串串接。
    const std::string csv = std::accumulate(
        values.begin(), values.end(), std::string{},
        [](std::string out, int value) {
            if (!out.empty()) {
                out += ',';
            }
            return out + std::to_string(value);
        });
    assert(csv == "1,2,3,4");

    assert(leetcode_maximum_wealth({{1, 2, 3}, {3, 2, 1}, {9}}) == 9);
    assert(practical_total_cents({{"A", 1250}, {"B", -200}, {"C", 50}}) == 1100);

    std::cout << "accumulate：左摺疊、LeetCode 與帳務彙總測試通過\n";
}

/*
 * 易錯與面試速記：
 * 1. `accumulate(v.begin(), v.end(), 0)` 在 int 內運算；即使 v 是 long long，
 *    大數仍可能先溢位。應寫 `0LL` 或明確型別的初值。
 * 2. callback 不應修改正在遍歷的 vector 大小，push_back 可能使 iterator 失效。
 * 3. 要保證左到右順序選 accumulate；可交換且想容許平行重排才考慮 reduce。
 * 4. 浮點加法不具結合律；不同順序可能有不同末位。財務通常改用整數分幣。
 * 5. C++23 有 fold_left/fold_right；C++20 專案仍常以 accumulate 表達左摺疊。
 *
 * 面試題：空範圍回什麼？答案是 init。為何初值是 API 的一部分？因為它同時
 * 定義 identity、輸出型別與空集合語意。實務上應為空輸入寫測試，而非只測正常表。
 *
 * 練習：把 practical_total_cents 改成同時產生總額與發票筆數的 Summary 結構；
 * 再思考 callback 每次複製 Summary 的成本，以及能否用 move 或一般 for loop 改善。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'accumulate.cpp' -o '/tmp/codex_cpp_C_Algorithm_numeric_accumulate' && '/tmp/codex_cpp_C_Algorithm_numeric_accumulate'
//
// === 預期輸出（節錄）===
// accumulate：左摺疊、LeetCode 與帳務彙總測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
