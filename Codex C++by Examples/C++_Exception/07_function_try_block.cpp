// ============================================================================
// 課題 7：Function try block，特別是 constructor initializer failures
// ============================================================================
//
// 語法 `Ctor(...) try : member_(...) { ... } catch (...) { ... }` 能捕捉 base/member
// initialization 或 constructor body 丟出的 exception；普通 body 內 try 已來不及包住
// initializer list。constructor function-try handler 結束時若不明寫 throw，exception 仍
// 自動 rethrow；object 沒有完整建成，不能從 handler「恢復成成功」。
//
// 常用於加 context/log/exception translation。handler 中存取已解構或未建成 members 危險，
// 不應依賴 object state。一般 function 也可用 function-try-block，但普通 try 更清楚。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class PositiveSize {
public:
    explicit PositiveSize(int size)
    {
        if (size <= 0) throw std::invalid_argument("size must be positive");
        value_ = static_cast<std::size_t>(size);
    }
    std::size_t value() const { return value_; }
private:
    std::size_t value_ = 0U;
};

class Buffer {
public:
    Buffer(int size, int& failures)
    try : size_(size), values_(size_.value(), 0)
    {
    }
    catch (const std::exception&)
    {
        ++failures; // 只用 constructor parameter，不碰未完整建成的 members。
        throw;      // 明寫保留原 exception type。
    }

    std::size_t size() const { return values_.size(); }
private:
    PositiveSize size_;
    std::vector<int> values_;
};

void basic_example()
{
    int failures = 0;
    [[maybe_unused]] bool rejected = false;
    try { const Buffer invalid(0, failures); (void)invalid; }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected && failures == 1);
    const Buffer valid(4, failures);
    assert(valid.size() == 4U && failures == 1);
    std::cout << "[基礎] initializer failure logged then original error rethrown\n";
}

// LeetCode 1656：Ordered Stream。validated_size 在 vector initializer 前驗證 n；
// function-try-block 可為 allocation/validation failure 增加 construction counter/context。
class OrderedStream {
public:
    OrderedStream(int count, int& failures)
    try : values_(validated_size(count)), next_(1U)
    {
    }
    catch (...)
    {
        ++failures;
        throw;
    }
    std::vector<std::string> insert(int id, std::string value)
    {
        values_.at(static_cast<std::size_t>(id)) = std::move(value);
        std::vector<std::string> result;
        while (next_ < values_.size() && !values_.at(next_).empty()) {
            result.push_back(values_.at(next_++));
        }
        return result;
    }
private:
    static std::size_t validated_size(int count)
    {
        if (count <= 0) throw std::invalid_argument("count");
        return static_cast<std::size_t>(count) + 1U;
    }
    std::vector<std::string> values_;
    std::size_t next_;
};

void leetcode_1656_example()
{
    int failures = 0;
    OrderedStream stream(3, failures);
    // insert 具有狀態副作用；assert 在 NDEBUG 下整段消失，因此只能檢查已保存的結果。
    [[maybe_unused]] const auto waiting = stream.insert(2, "bb");
    assert(waiting.empty());
    [[maybe_unused]] const auto ready = stream.insert(1, "aa");
    assert((ready == std::vector<std::string>{"aa", "bb"}));
    assert(failures == 0);
    std::cout << "[LeetCode 1656] valid constructor and ordered chunk pass\n";
}

// 實務：真正需要 translation 時，catch 原 error 後 throw_with_nested（下一課）。
void practical_example()
{
    int failures = 0;
    try { const Buffer bad(-1, failures); (void)bad; }
    catch (const std::exception& error) {
        assert(std::string(error.what()) == "size must be positive");
    }
    assert(failures == 1);
    std::cout << "[實務] constructor metrics counted failed initialization\n";
}

int main()
{
    basic_example();
    leetcode_1656_example();
    practical_example();
}

// 易錯與面試：constructor body 開始前 members/bases 已完成建構；普通 body try 抓不到其
// 失敗。function-try-block 可觀察/轉譯，但 catch 結束後 constructor 仍必須丟出，不能復活物件。
// 練習：移除 function-try-block，說明 constructor body try 為何抓不到 member failure。
// 複雜度：function-try-block 不改成功路徑 Big-O；失敗成本包含已建 base/member 的反向解構。
// 生命週期：進入 constructor handler 時 object 未完成，不能把 `this` 當完整物件繼續使用。
