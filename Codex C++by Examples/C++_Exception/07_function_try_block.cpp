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

// -----------------------------------------------------------------------------
// 【日常實務範例】建構失敗的指標計數
// 情境：Buffer 的 size member 驗證或 vector 配置失敗時，要遞增外部 failures metric，同時保留原始 exception 給呼叫端。
// 為何使用本章主題：constructor function-try-block 能涵蓋 initializer list，普通 constructor body 內的 try 無法捕捉先發生的 member failure。
// 設計：1. 先建 PositiveSize。2. 依驗證後大小建 vector。3. 任一 initializer 失敗便進 handler。4. 只更新外部 counter 並 bare rethrow。
// 成本：成功路徑仍由 N 個 int 的 vector 配置/初始化主導；失敗另有已建 member 解構與 unwinding 成本。
// 上線注意：handler 中 object 尚未完整建成，不可讀 members 或復活 this；metric 更新本身也應不拋且具備併發安全。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1656. Design an Ordered Stream（設計有序流）
// 題目：以 1-based id 插入字串，每次回傳從目前 pointer 起連續可用的 chunk；先插 id=2 無輸出，再插 id=1 回 [aa,bb]。
// 為何使用本章主題：validated_size 在 initializer 前驗證 n，function-try-block 讓建構驗證或配置失敗可計數後原樣重拋；這是教學擴充。
// 思路：1. 配置 n+1 的 1-based vector。2. insert 將值放到 id。3. 從 next 收集連續非空字串。4. 同步推進 next。
// 複雜度：建構時間/空間 O(N)；所有 insert 合計輸出每項一次，單次另加回傳 chunk 大小 O(K)。
// 易錯點：id 必須在 1..N 且每 id 只插一次；本實作收集結果配置失敗時 next_ 可能已前進，未提供 strong guarantee。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：prepare-then-commit】
 * - `values_.at(next_++)` 可能先推進 next_，之後配置/格式化拋出時便永久跳過資料。
 * - 強保證做法是用暫存 index 準備完整 chunk，所有可失敗工作成功後才 commit next_。
 * - function-try-block 適合攔 constructor initializer 的例外，不會自動替一般 state mutation rollback。
 * - 若輸出 sink 也可失敗，要把「來源 cursor」與「已發布結果」視為同一交易設計。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_function_try_block.cpp' -o '/tmp/codex_cpp_C_Exception_07_function_try_block' && '/tmp/codex_cpp_C_Exception_07_function_try_block'
//
// === 預期輸出（節錄）===
// [實務] constructor metrics counted failed initialization
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
