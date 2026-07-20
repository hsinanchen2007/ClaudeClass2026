/*
 * C++11 教科書：std::initializer_list
 *
 * initializer_list<T> 是輕量 view，指向 compiler 建立的一段唯讀連續元素；複製它只複製
 * pointer/size，不複製元素。元素型別視為 const T，因此不能透過 iterator 修改。
 * 大括號呼叫若能匹配 initializer_list constructor，該 overload 具有很高優先權。
 *
 * 【生命週期】直接綁定到具名 initializer_list 變數時，底層陣列可活到該變數 scope 結束；
 * 不要回傳指向函式參數 initializer_list 元素的 pointer/iterator，呼叫結束即可能懸空。
 * 【效能】不適合搬移 move-only 元素，因元素是 const；vector<unique_ptr<T>> 應用 emplace。
 * 【常見陷阱】保存 initializer_list 的 pointer/iterator 到呼叫結束後會形成 dangling。
 * 【面試題】為什麼 vector<int>{10, 3} 與 vector<int>(10, 3) 結果不同？
 * 【練習】替 RetryPolicy 驗證 delay 必須遞增。
 */

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace basic {
int sum(std::initializer_list<int> values) {
    int total = 0;
    for (const int value : values) {
        total += value;
    }
    return total;
}

void demo() {
    assert(sum({1, 2, 3, 4}) == 10);
    const std::vector<int> elements{10, 3};
    const std::vector<int> repeated(10, 3);
    assert(elements.size() == 2U && repeated.size() == 10U);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate（是否存在重複元素）
// 題目：輸入整數序列，只要任一值出現至少兩次就回 true；例如 [1,2,3,1] 回 true。
// 為何使用本章主題：initializer_list 讓教材直接以大括號傳測資；這是教學介面，正式大量輸入通常改用容器或 view。
// 思路：1. 建立 seen 集合；2. 逐值插入；3. insert 回報已存在時立刻回 true，走完則回 false。
// 複雜度：N 為元素數；std::set 版本時間 O(N log N)、額外空間 O(N)。
// 易錯點：initializer_list 元素唯讀且生命週期只涵蓋呼叫；不可保存其 iterator，且它不適合 move-only 元素。
// -----------------------------------------------------------------------------
bool contains_duplicate(std::initializer_list<int> nums) {
    std::set<int> seen;
    for (const int number : nums) {
        if (!seen.insert(number).second) {
            return true;
        }
    }
    return false;
}

void test() {
    assert(contains_duplicate({1, 2, 3, 1}));
    assert(!contains_duplicate({1, 2, 3, 4}));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】服務重試延遲策略
// 情境：呼叫端用 {100,500,2000} 設定每次重試等待毫秒數，策略物件需長期保存並供索引查詢。
// 為何使用本章主題：initializer_list 讓短固定序列易讀；constructor 立即複製到 vector，取得獨立 ownership。
// 設計：1. 從大括號序列建構 delays_；2. 拒絕空序列或非正值；3. 以 at 依嘗試次數取延遲。
// 成本：K 為重試階段數；建構與驗證時間 O(K)、持有空間 O(K)，單次 delay 查詢 O(1)。
// 上線注意：還需限制最大延遲與總等待時間；不能保存 initializer_list 指標，attempt 越界會丟例外。
// -----------------------------------------------------------------------------
class RetryPolicy {
public:
    RetryPolicy(std::initializer_list<int> delays_ms) : delays_(delays_ms) {
        if (delays_.empty() ||
            std::any_of(delays_.begin(), delays_.end(), [](int value) { return value <= 0; })) {
            throw std::invalid_argument("重試延遲必須是正整數");
        }
    }

    int delay(std::size_t attempt) const { return delays_.at(attempt); }
    std::size_t attempts() const noexcept { return delays_.size(); }

private:
    std::vector<int> delays_;  // 必須複製持有，不能保存 initializer_list iterator。
};

void test() {
    const RetryPolicy policy{100, 500, 2000};
    assert(policy.attempts() == 3U && policy.delay(1) == 500);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "initializer_list：overload、重複偵測、重試策略測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_initializer_list.cpp' -o '/tmp/codex_cpp_C_Cpp11_06_initializer_list' && '/tmp/codex_cpp_C_Cpp11_06_initializer_list'
//
// === 預期輸出（節錄）===
// initializer_list：overload、重複偵測、重試策略測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
