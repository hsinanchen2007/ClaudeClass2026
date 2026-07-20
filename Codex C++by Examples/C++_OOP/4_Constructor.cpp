// ============================================================================
// 課題 4：Constructor 讓物件一出生就合法
// ============================================================================
//
// constructor 名稱與 class 相同、沒有回傳型別，在 object 建立時自動執行。它的核心
// 任務是建立 invariant，不是把所有工作都塞入。若 constructor 失敗可 throw；物件
// 不會以半建構狀態交給呼叫端。
//
// 一旦宣告任何 user constructor，compiler 不一定再產生 default constructor；需要
// 時可寫 `Type() = default`。單參數 constructor 通常標 explicit，防止 `f(5)` 偷偷
// 建 Type。brace initialization 可防 narrowing，且避免 `Type x();` 被解析成函式宣告。
//
// member 真正依 class 宣告順序初始化，不依 initializer list 書寫順序；第 7 課深入。
// 【面試】constructor 可 virtual 嗎？不可；物件尚未建立完成，dynamic type 機制不適用。
// ============================================================================

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Port {
public:
    explicit Port(int value) : value_(value)
    {
        if (value_ < 1 || value_ > 65'535) {
            throw std::invalid_argument("invalid port");
        }
    }
    int value() const { return value_; }

private:
    int value_;
};

void basic_example()
{
    const Port https{443};
    assert(https.value() == 443);
    [[maybe_unused]] bool rejected = false;
    try {
        const Port invalid{70'000};
        (void)invalid;
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
    std::cout << "[基礎] Port{443} 合法，70000 在 constructor 被拒\n";
}

// LeetCode 1656：Ordered Stream。constructor 依 n 配好 1-based 儲存空間。
class OrderedStream {
public:
    explicit OrderedStream(int size)
        : values_(validated_storage_size(size)), next_(1) {}

    std::vector<std::string> insert(int id, std::string value)
    {
        values_.at(static_cast<std::size_t>(id)) = std::move(value);
        std::vector<std::string> result;
        while (next_ < values_.size() && !values_.at(next_).empty()) {
            result.push_back(values_.at(next_));
            ++next_;
        }
        return result;
    }

private:
    static std::size_t validated_storage_size(int size)
    {
        // 不可先把負數 cast 成 size_t；它會變成極大的正數並嘗試配置巨量記憶體。
        if (size <= 0) {
            throw std::invalid_argument("stream size must be positive");
        }
        return static_cast<std::size_t>(size) + 1U;
    }

    std::vector<std::string> values_;
    std::size_t next_;
};

void leetcode_1656_example()
{
    OrderedStream stream(5);
    // insert 會寫入 stream 並改變 next_；先執行，避免 -DNDEBUG 把操作本身刪除。
    [[maybe_unused]] const auto waiting = stream.insert(3, "ccccc");
    assert(waiting.empty());
    [[maybe_unused]] const auto first_chunk = stream.insert(1, "aaaaa");
    assert((first_chunk == std::vector<std::string>{"aaaaa"}));
    [[maybe_unused]] const auto second_chunk = stream.insert(2, "bbbbb");
    assert((second_chunk ==
            std::vector<std::string>{"bbbbb", "ccccc"}));
    std::cout << "[LeetCode 1656] constructor 預配置 5 個 stream slots\n";
}

// 工作案例：ConnectionOptions 沒有「未設 host/port」的半成品狀態。
class ConnectionOptions {
public:
    ConnectionOptions(std::string host, Port port)
        : host_(std::move(host)), port_(port)
    {
        if (host_.empty()) {
            throw std::invalid_argument("empty host");
        }
    }
    std::string endpoint() const
    {
        return host_ + ":" + std::to_string(port_.value());
    }

private:
    std::string host_;
    Port port_;
};

void practical_example()
{
    const ConnectionOptions options("localhost", Port{8080});
    assert(options.endpoint() == "localhost:8080");
    std::cout << "[實務] endpoint=" << options.endpoint() << '\n';
}

int main()
{
    basic_example();
    leetcode_1656_example();
    practical_example();
}

// 實務提醒：constructor 完成時 object invariant 必須成立；不能建立「稍後記得 init」的半成品。
// 練習：移除 explicit，觀察 `void connect(Port); connect(80);` 為何會偷偷轉型。
// 複雜度：constructor 成本是各 base/member 建構成本總和；vector sizing 例如是 O(N)。
// 生命週期：base 與 members 依宣告順序先完成，之後才進 constructor body；失敗則反向解構。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '4_Constructor.cpp' -o '/tmp/codex_cpp_C_OOP_4_Constructor' && '/tmp/codex_cpp_C_OOP_4_Constructor'
//
// === 預期輸出（節錄）===
// [LeetCode 1656] constructor 預配置 5 個 stream slots
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
