/*
 * 第 07 章：預設模板引數
 *
 * 預設模板引數可降低常見用法的噪音，又保留進階客製能力。像 vector<T, Allocator>
 * 的 Allocator 幾乎都用預設。類別模板從第一個有預設值的參數起，後方通常也要有
 * 預設值；函式模板較彈性，且多數參數可由函式引數推導。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
#include <utility>
#include <vector>

template <typename Key = std::string, typename Value = int>
struct Entry {
    Key key{};
    Value value{};
};

// LeetCode 703：Kth Largest Element in a Stream。
// Compare 有預設值，想改成其他排序規則時仍可覆寫。
template <typename T = int, typename Compare = std::greater<T>>
class KthLargest {
public:
    KthLargest(std::size_t k, std::vector<T> initial) : k_(k) {
        for (T& value : initial) {
            add(std::move(value));
        }
    }

    T add(T value) {
        heap_.push(std::move(value));
        if (heap_.size() > k_) {
            heap_.pop();
        }
        return heap_.top();
    }

private:
    std::size_t k_;
    std::priority_queue<T, std::vector<T>, Compare> heap_;
};

struct NoRetry {
    static constexpr int attempts = 1;
};

struct ThreeAttempts {
    static constexpr int attempts = 3;
};

// 實務：Client 的資料型別與 retry policy 都有安全預設；測試可替換 policy。
template <typename Response = std::string, typename RetryPolicy = NoRetry>
class Client {
public:
    template <typename Operation>
    Response request(Operation operation) const {
        for (int attempt = 0; attempt < RetryPolicy::attempts; ++attempt) {
            if (auto result = operation(attempt)) {
                return *result;
            }
        }
        return {};
    }
};

void leetcode_kth_largest_test() {
    KthLargest<> kth(3U, {4, 5, 8, 2});
    const int after_3 = kth.add(3);
    const int after_5 = kth.add(5);
    const int after_10 = kth.add(10);
    const int after_9 = kth.add(9);
    assert(after_3 == 4);
    assert(after_5 == 5);
    assert(after_10 == 5);
    assert(after_9 == 8);
}

void practical_retry_test() {
    Client<std::string, ThreeAttempts> client;
    const std::string response = client.request([](int attempt) -> std::optional<std::string> {
        return attempt == 2 ? std::optional<std::string>{"ok"} : std::nullopt;
    });
    assert(response == "ok");
}

int main() {
    const Entry<> default_entry{"retries", 3};
    const Entry<int, std::string> custom_entry{404, "not found"};
    assert(default_entry.value == 3);
    assert(custom_entry.key == 404);

    leetcode_kth_largest_test();

    practical_retry_test();

    std::cout << "預設模板引數測試完成\n";
}

/*
 * 【複雜度】KthLargest::add 為 O(log k)，空間 O(k)，而不是保存全部資料。
 * 【陷阱】預設值是 API 契約；公開函式庫任意改預設型別可能改變效能或 ABI。
 * 【陷阱】priority_queue 的 Compare 方向常令人混淆：greater<T> 讓最小值位於 top。
 * 【面試】預設函式引數與預設模板引數何時決定？兩者皆在編譯期，但參與規則不同。
 * 【練習】替 KthLargest 加 static_assert 或例外，拒絕 k == 0。
 */

/*
 * 【教科書補充：預設參數不取代 runtime 契約】
 * - KthLargest 的 k 是 runtime constructor argument；k==0 時會 pop 後再 top 空 heap，正式版須丟例外。
 * - static_assert 只能檢查模板常數，不能驗證這個 runtime k；兩種檢查不要混用。
 * - Compare 必須和「第 k 大/小」的定義成對；換 comparator 可能同時翻轉 heap top 的語意。
 * - 改公開模板的預設型別會改呼叫端實體化出的型別，可能影響 ABI、效能與序列化名稱。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_default_template_arguments.cpp' -o '/tmp/codex_cpp_C_Template_07_default_template_arguments' && '/tmp/codex_cpp_C_Template_07_default_template_arguments'
//
// === 預期輸出（節錄）===
// 預設模板引數測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
