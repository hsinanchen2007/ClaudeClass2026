/*
 * C++11 教科書：統一大括號初始化（brace initialization）
 *
 * T{args...} 可初始化 scalar、aggregate、container 與 class，並避免 most-vexing parse。
 * 優點是阻止 narrowing，例如 int{3.14} 編譯失敗。空大括號 T{} 通常做 value
 * initialization：數值歸零、pointer 為 nullptr。
 *
 * 【選擇規則】若 class 同時有 initializer_list constructor，它通常優先；
 * vector<int>(3, 9) 是三個 9，vector<int>{3, 9} 是兩個元素 3、9，語意完全不同。
 * 【常見陷阱】initializer_list overload 優先，不能只看大括號內數字猜 constructor 語意。
 * 【aggregate】公開資料成員、無阻礙 aggregate 條件時可依序初始化；欄位順序改變可能
 * 破壞呼叫端，正式 API 可改用具名 constructor/builder。
 * 【面試題】為何 auto x{1} 與 auto x = {1} 推導可能不同？後者是 initializer_list。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace basic {
struct Point {
    int x;
    int y;
};

void demo() {
    int zero{};
    Point origin{};
    Point target{3, 4};
    std::vector<int> values{1, 2, 3};
    assert(zero == 0 && origin.x == 0 && origin.y == 0);
    assert(target.x == 3 && target.y == 4 && values.size() == 3U);

    // int narrowed{2.5};  // 正確地編譯失敗，避免靜默遺失小數。
}
}  // namespace basic

namespace leetcode {
// LeetCode 121：Best Time to Buy and Sell Stock。O(n) time / O(1) space。
int max_profit(const std::vector<int>& prices) {
    if (prices.empty()) {
        return 0;
    }
    int minimum{prices.front()};
    int answer{};
    for (const int price : prices) {
        minimum = std::min(minimum, price);
        answer = std::max(answer, price - minimum);
    }
    return answer;
}

void test() {
    assert(max_profit({7, 1, 5, 3, 6, 4}) == 5);
    assert(max_profit({7, 6, 4, 3, 1}) == 0);
}
}  // namespace leetcode

// 【實務案例】ServerConfig aggregate：以具名欄位型別表達 host、port、TLS 的完整設定。
namespace practical {
struct ServerConfig {
    std::string host;
    unsigned short port;
    bool tls;
};

bool valid(const ServerConfig& config) {
    return !config.host.empty() && config.port != 0U;
}

void test() {
    const ServerConfig production{"api.example.test", 443, true};
    const ServerConfig invalid{};
    assert(valid(production) && production.tls);
    assert(!valid(invalid));
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "brace init：零初始化、股票題、伺服器設定測試通過\n";
}

// 【延伸練習】列出 vector<int>(3, 9) 與 vector<int>{3, 9} 的內容，再設計不易誤讀的 factory。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_brace_init.cpp' -o '/tmp/codex_cpp_C_Cpp11_05_brace_init' && '/tmp/codex_cpp_C_Cpp11_05_brace_init'
//
// === 預期輸出（節錄）===
// brace init：零初始化、股票題、伺服器設定測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
