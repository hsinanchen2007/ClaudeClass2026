// ============================================================================
// 課題 16：virtual function、dynamic dispatch 與 object slicing
// ============================================================================
//
// 經 Base pointer/reference 呼叫 virtual function 時，runtime 依 object 的 dynamic type
// 選 derived override；這叫 runtime polymorphism。Base 要宣告 virtual，Derived 應加
// override，讓 compiler 檢查簽章真的覆寫成功。`final` 可禁止再 override/derive。
//
// constructor/destructor 內的 virtual call 不會派發到尚未建構/已解構的 derived 部分。
// 按值存 Base 會 slicing；多型物件通常用 reference、pointer 或 smart pointer。
//
// 【成本】通常多一次間接呼叫，可能阻礙 inline，但介面邊界的可替換性常更重要；
// hot loop 可考慮 template/variant 等 static polymorphism，仍應 profiling 後決定。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class PriceRule {
public:
    virtual ~PriceRule() = default;
    virtual int final_price(int cents) const = 0;
};

class NoDiscount final : public PriceRule {
public:
    int final_price(int cents) const override { return cents; }
};

class TenPercentOff final : public PriceRule {
public:
    int final_price(int cents) const override { return cents * 90 / 100; }
};

void basic_example()
{
    const TenPercentOff discount;
    const PriceRule& rule = discount;
    assert(rule.final_price(1'000) == 900); // 經 Base& 派發到 derived override。
    std::cout << "[基礎] virtual dispatch final price=900\n";
}

// LeetCode 70：Climbing Stairs。
// 題目本身只需一個演算法；這裡用同一介面提供 iterative/memoized 兩種可替換策略，
// 實際執行並核對答案。面試時不必硬套 class hierarchy，這是教學上的策略示範。
class StairSolver {
public:
    virtual ~StairSolver() = default;
    virtual int climb_stairs(int steps) const = 0;
};

class IterativeStairSolver final : public StairSolver {
public:
    int climb_stairs(int steps) const override
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

class MemoizedStairSolver final : public StairSolver {
public:
    int climb_stairs(int steps) const override
    {
        std::vector<int> memo(static_cast<std::size_t>(steps) + 1U, -1);
        return solve(steps, memo);
    }

private:
    static int solve(int steps, std::vector<int>& memo)
    {
        if (steps <= 1) return 1;
        int& answer = memo.at(static_cast<std::size_t>(steps));
        if (answer < 0) answer = solve(steps - 1, memo) + solve(steps - 2, memo);
        return answer;
    }
};

void leetcode_70_example()
{
    const std::vector<std::unique_ptr<StairSolver>> solvers = [] {
        std::vector<std::unique_ptr<StairSolver>> result;
        result.push_back(std::make_unique<IterativeStairSolver>());
        result.push_back(std::make_unique<MemoizedStairSolver>());
        return result;
    }();
    for (const auto& solver : solvers) {
        assert(solver->climb_stairs(5) == 8);
    }
    std::cout << "[LeetCode 70] 兩個 virtual strategies 都得到 8\n";
}

// 實務案例：通知管線不需知道 Email/Console 的具體型別。
class Notifier {
public:
    virtual ~Notifier() = default;
    virtual std::string send(const std::string& message) const = 0;
};

class ConsoleNotifier final : public Notifier {
public:
    std::string send(const std::string& message) const override
    {
        return "console:" + message;
    }
};

void practical_example()
{
    const ConsoleNotifier console;
    const Notifier& notifier = console;
    assert(notifier.send("build passed") == "console:build passed");
    std::cout << "[實務] " << notifier.send("build passed") << '\n';
}

int main()
{
    basic_example();
    leetcode_70_example();
    practical_example();
}

// 易錯：漏寫 override 時，參數/const 差一點就會變成新 overload 而非覆寫。面試也要說明
// dynamic dispatch 只有透過 base pointer/reference 才發生，value copy 會 slicing。
// 練習：故意把 override 的參數改成 unsigned，觀察 compiler 如何阻止 silent overload。
// 複雜度：一次 virtual call 通常常數時間，但真正 Big-O 由 override body 決定，勿只談 vtable。
// 生命週期：virtual dispatch 需要仍存活的 polymorphic object；constructor/destructor 中 dispatch 受限。
