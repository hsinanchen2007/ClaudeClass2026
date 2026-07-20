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
// LeetCode 1672：Richest Customer Wealth。時間 O(rows*cols)，額外空間 O(1)。
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

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Service {
    std::string name;
    int retry_count;
};

// 實務：部署前把負值重試次數正規化。需要改元素，所以使用 auto&。
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
