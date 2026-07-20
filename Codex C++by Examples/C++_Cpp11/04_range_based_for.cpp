/*
 * C++11 教科書：range-based for
 *
 * for (declaration : range) 會取得 begin(range)/end(range) 並逐項走訪。
 * - auto item：複製元素，修改 item 不影響容器。
 * - auto& item：直接修改元素。
 * - const auto& item：只讀且避免昂貴複製，日常最安全。
 * - auto&& item：泛型 range 常用，可接 proxy 與不同 value category。
 *
 * 複雜度是 O(n)，每次迭代成本取決於是否複製元素。走訪中改變 vector 容量、
 * erase 當前容器通常使 iterator 失效；需要刪除時用 erase-remove 或 iterator 迴圈。
 * 【常見陷阱】省略 `&` 會改到副本；迭代時 reallocation 會使 iterator/reference 失效。
 * 【生命週期】C++23 延長許多 range-initializer 內 temporary 的生命週期，但不是
 * 「所有 temporary 都安全」：若 temporary 是函式的 by-value 參數，callee 回傳其
 * reference，參數仍會在函式返回時死亡。初學階段先把複雜來源存成具名變數。
 * 【面試題】為何 vector<bool> 常用 auto&& 而非 auto&？元素是 proxy。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace basic {
void demo() {
    std::vector<int> values{1, 2, 3};
    int copied_total = 0;
    for (auto value : values) {
        value *= 10;  // 只改副本
        copied_total += value;
    }
    assert(copied_total == 60 && values == std::vector<int>({1, 2, 3}));

    for (auto& value : values) {
        value *= 10;  // 改原元素
    }
    assert(values == std::vector<int>({10, 20, 30}));
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1672. Richest Customer Wealth（最富有客戶的資產總和）
// 題目：accounts 每列是一位客戶在各銀行的餘額，回傳最大的列總和；例如 [[1,5],[7,3],[3,5]] 回傳 10。
// 為何使用本章主題：外層 range-for 以 const auto& 借用每列，避免複製整個 vector<int> 後再累加。
// 思路：1. 逐列走訪客戶；2. accumulate 該列所有帳戶；3. 持續更新目前最大資產。
// 複雜度：R 為客戶數、C 為每列平均帳戶數；時間 O(R*C)、額外空間 O(1)。
// 易錯點：漏寫 & 會逐列複製；若餘額或欄數很大，int 累加初值可能溢位，需改較寬型別。
// -----------------------------------------------------------------------------
int maximum_wealth(const std::vector<std::vector<int>>& accounts) {
    int best = 0;
    for (const auto& customer : accounts) {
        const int wealth = std::accumulate(customer.begin(), customer.end(), 0);
        best = std::max(best, wealth);
    }
    return best;
}

void test() {
    assert(maximum_wealth({{1, 2, 3}, {3, 2, 1}}) == 6);
    assert(maximum_wealth({{1, 5}, {7, 3}, {3, 5}}) == 10);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】部署服務重試次數正規化
// 情境：部署清單中的 retry_count 可能來自錯誤設定，要統一限制在 0 到 5 次。
// 為何使用本章主題：range-for 的 auto& 直接綁定原 Service；若用 auto，修正只會落在暫時副本。
// 設計：1. 逐一取得可寫 Service reference；2. 先套上限 5；3. 再套下限 0 並寫回原容器。
// 成本：N 為服務數；時間 O(N)、額外空間 O(1)。
// 上線注意：迴圈期間不可造成 vector reallocation；實際系統還應記錄被修正的非法設定以利觀測。
// -----------------------------------------------------------------------------
struct Service {
    std::string name;
    int retry_count;
};

void normalize(std::vector<Service>& services) {
    for (auto& service : services) {
        service.retry_count = std::max(0, std::min(service.retry_count, 5));
    }
}

void test() {
    std::vector<Service> services{{"api", -1}, {"db", 9}, {"cache", 2}};
    normalize(services);
    assert(services[0].retry_count == 0);
    assert(services[1].retry_count == 5);
    assert(services[2].retry_count == 2);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "range-for：複製/參考、財富統計、設定正規化測試通過\n";
}

// 【延伸練習】對 vector<bool>、vector<string> 各比較 auto、auto&、const auto& 與 auto&& 的型別/效果。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_range_based_for.cpp' -o '/tmp/codex_cpp_C_Cpp11_04_range_based_for' && '/tmp/codex_cpp_C_Cpp11_04_range_based_for'
//
// === 預期輸出（節錄）===
// range-for：複製/參考、財富統計、設定正規化測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
