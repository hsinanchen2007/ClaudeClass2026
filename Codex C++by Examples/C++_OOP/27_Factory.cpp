// ============================================================================
// 課題 27：Factory pattern（工廠模式）
// ============================================================================
//
// Factory 把「根據設定選 concrete type + 建構細節」集中，呼叫端只依賴 interface。
// 回傳 unique_ptr 表達單一 ownership；若只是簡單 object，named factory 回 value 即可，
// 不必所有建構都做成 hierarchy。
//
// Factory 適合 plugin/parser/backend 選擇。避免巨大 switch 無限增長；大型系統可用
// registration map。但 registry 也增加隱性控制流，兩三個固定類型時 switch 最清楚。
//
// 【面試】Factory Method 常由 derived override creation；Abstract Factory 建立一族相容
// objects；日常口語的 factory 通常只是集中 creation 的 function/class。
// 【陷阱】factory 回 raw owning pointer 讓責任不清，優先 value/unique_ptr。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

class Parser {
public:
    virtual ~Parser() = default;
    virtual std::string parse_name(const std::string& input) const = 0;
};

class CsvParser final : public Parser {
public:
    std::string parse_name(const std::string& input) const override
    {
        return input.substr(0U, input.find(','));
    }
};

class KeyValueParser final : public Parser {
public:
    std::string parse_name(const std::string& input) const override
    {
        const std::size_t equal = input.find('=');
        return equal == std::string::npos ? std::string{} : input.substr(equal + 1U);
    }
};

std::unique_ptr<Parser> make_parser(std::string_view format)
{
    if (format == "csv") return std::make_unique<CsvParser>();
    if (format == "key-value") return std::make_unique<KeyValueParser>();
    throw std::invalid_argument("unsupported parser format");
}

void basic_example()
{
    const auto csv = make_parser("csv");
    const auto key_value = make_parser("key-value");
    assert(csv->parse_name("Ada,95") == "Ada");
    assert(key_value->parse_name("name=Bjarne") == "Bjarne");
    std::cout << "[基礎] factory 建立兩種 Parser implementations\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 70. Climbing Stairs（爬樓梯）
// 題目：每次爬 1 或 2 階，求到第 n 階方法數；例如 n=5 得 8，n=20 得 10946。
// 為何使用本章主題：題目不需 Factory；本例教學改寫成依 n 選小型遞迴或迭代 solver 的建構邏輯。
// 思路：factory 對 n<=10 建 recursive solver，否則建 iterative；兩者依 Fibonacci recurrence 求答案。
// 複雜度：遞迴版 O(2^N) 時間/O(N) stack，迭代版 O(N) 時間/O(1) 空間。
// 易錯點：steps 應符合題目正數限制；int 結果會溢位；factory 回 unique_ptr，interface destructor 必須 virtual。
// -----------------------------------------------------------------------------
class StairSolver {
public:
    virtual ~StairSolver() = default;
    virtual int solve(int steps) const = 0;
};

class IterativeSolver final : public StairSolver {
public:
    int solve(int steps) const override
    {
        int previous = 0;
        int current = 1;
        for (int step = 1; step <= steps; ++step) {
            const int next = previous + current;
            previous = current;
            current = next;
        }
        return current;
    }
};

class SmallRecursiveSolver final : public StairSolver {
public:
    int solve(int steps) const override
    {
        return steps <= 1 ? 1 : solve(steps - 1) + solve(steps - 2);
    }
};

std::unique_ptr<StairSolver> make_stair_solver(int steps)
{
    if (steps <= 10) return std::make_unique<SmallRecursiveSolver>();
    return std::make_unique<IterativeSolver>();
}

void leetcode_70_example()
{
    assert(make_stair_solver(5)->solve(5) == 8);
    assert(make_stair_solver(20)->solve(20) == 10'946);
    std::cout << "[LeetCode 70] factory 選 solver，n=20 answer=10946\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依部署模式建立重試策略值
// 情境：interactive 要低延遲 3 次/100ms，batch 可容忍 10 次/1000ms，未知模式必須明確拒絕。
// 為何使用本章主題：簡單 RetryPolicy 適合 factory 回 value，不需要 virtual hierarchy 或 heap allocation。
// 設計：比較 mode；建立對應 attempts/delay；未知值丟 invalid_argument；caller 只讀策略欄位。
// 成本：固定兩次字串比較，時間 O(M)、額外空間 O(1)，M 為 mode 長度。
// 上線注意：應用 enum/validated config 避免拼字；delay 單位要明確，重試還需 jitter、上限與可取消性。
// -----------------------------------------------------------------------------
class RetryPolicy {
public:
    RetryPolicy(int attempts, int delay_ms) : attempts_(attempts), delay_ms_(delay_ms) {}
    int attempts() const { return attempts_; }
    int delay_ms() const { return delay_ms_; }
private:
    int attempts_;
    int delay_ms_;
};

RetryPolicy make_retry_policy(std::string_view mode)
{
    if (mode == "interactive") return RetryPolicy(3, 100);
    if (mode == "batch") return RetryPolicy(10, 1'000);
    throw std::invalid_argument("unsupported mode");
}

void practical_example()
{
    const RetryPolicy policy = make_retry_policy("batch");
    assert(policy.attempts() == 10 && policy.delay_ms() == 1'000);
    std::cout << "[實務] value factory 建 batch retry policy\n";
}

int main()
{
    basic_example();
    leetcode_70_example();
    practical_example();
}

// 練習：新增 JsonParser，說明 Open/Closed Principle 與 switch factory 的取捨。
// 複雜度：switch factory O(1)，registry lookup 常 O(log N)/平均 O(1)，建立成本含 allocation。
// 生命週期：回傳 unique_ptr 讓 caller 成為 owner；factory 不應留下指向已移交 object 的 raw pointer。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '27_Factory.cpp' -o '/tmp/codex_cpp_C_OOP_27_Factory' && '/tmp/codex_cpp_C_OOP_27_Factory'
//
// === 預期輸出（節錄）===
// [實務] value factory 建 batch retry policy
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
