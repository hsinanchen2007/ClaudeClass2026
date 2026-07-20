/*
 * 第 13 章：std::initializer_list
 *
 * 花括號 `{1,2,3}` 可形成 initializer_list<T>，它只是指向編譯器產生之 const array 的
 * 輕量 view；複製 initializer_list 不複製元素。元素是 const，不能從中 move 出資源。
 * 建構子 overload resolution 對 initializer_list 有高度優先權，這是著名陷阱。
 */

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

int basic_sum(std::initializer_list<int> values) {
    int total = 0;
    for (int value : values) {
        total += value;
    }
    return total;
}

// LeetCode 217：Contains Duplicate。initializer_list 讓短測資呼叫自然。
template <typename T>
bool leetcode_contains_duplicate(std::initializer_list<T> values) {
    std::unordered_set<T> seen;
    seen.reserve(values.size());
    for (const T& value : values) {
        if (!seen.insert(value).second) {
            return true;
        }
    }
    return false;
}

// 【實務情境】短小固定 endpoint 清單用 braces 建立，再複製到 owned vector。
struct Endpoint {
    std::string host;
    int port{};
};

class ClusterConfig {
public:
    ClusterConfig(std::initializer_list<Endpoint> endpoints)
        : endpoints_(endpoints) {}

    std::size_t size() const noexcept { return endpoints_.size(); }
    const Endpoint& primary() const { return endpoints_.front(); }

private:
    // 必須複製進擁有型容器；不可保存 initializer_list.begin() pointer。
    std::vector<Endpoint> endpoints_;
};

ClusterConfig practical_make_cluster() {
    return ClusterConfig{{{"db-a", 5432}, {"db-b", 5432}}};
}

int main() {
    assert(basic_sum({1, 2, 3, 4}) == 10);
    assert(leetcode_contains_duplicate<int>({1, 2, 3, 1}));
    assert(!leetcode_contains_duplicate<std::string>({"api", "db", "cache"}));

    const ClusterConfig cluster = practical_make_cluster();
    assert(cluster.size() == 2U);
    assert(cluster.primary().host == "db-a");

    // 重要對比：vector(10,20) 是 10 個 20；vector{10,20} 是兩個元素。
    const std::vector<int> parentheses(10U, 20);
    const std::vector<int> braces{10, 20};
    assert(parentheses.size() == 10U);
    assert(braces.size() == 2U);

    // narrowing 會被 braces 拒絕：`int bad{3.14};` 無法編譯，提升安全性。
    const int safe{42};
    assert(safe == 42);

    std::cout << "initializer_list 測試完成\n";
}

/*
 * 【常見陷阱】
 * - initializer_list 元素為 const，vector<unique_ptr<T>> 無法從 list 搬移 move-only 元素。
 * - 保存 begin/end 超過 backing array 生命週期會懸空；class 應複製到 owned container。
 * - `auto x{1}` 在現代 C++ 是 int；`auto x = {1}` 是 initializer_list<int>，語法一字之差。
 * - 混合型別 `{1, 2.0}` 無法推導單一 initializer_list element type。
 *
 * 【面試段落】為何 initializer_list constructor 常「搶走」其他 overload？
 * list-initialization 的 overload resolution 對可行 initializer_list constructor 給優先級。
 * 【練習】設計支援 move-only Endpoint 的 variadic factory，避免 initializer_list const 元素限制。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '13_initializer_list.cpp' -o '/tmp/codex_cpp_C_Utility_13_initializer_list' && '/tmp/codex_cpp_C_Utility_13_initializer_list'
//
// === 預期輸出（節錄）===
// initializer_list 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
