// ============================================================================
// 課題 2：Data member 與 member function
// ============================================================================
//
// data member 保存物件 state；member function 查詢或修改 state。短函式可在 class
// 內定義；較長實作常在 class 外以 `ClassName::method` 定義。class 內定義的函式
// 隱含 inline 語意是「可在多個 translation unit 有相同定義」，不保證機器碼一定
// 展開；是否 inline 最佳化由編譯器決定。
//
// const member function 的隱含 this 是 pointer-to-const，不可改一般 member，且可由
// const object 呼叫。純查詢 method 應標 const。大型輸入常以 const reference 傳入；
// 若 method 要保存一份，可按值接收再 move。
//
// 【面試】`void show() const` 與 `void show()` 可 overload，因 this cv-qualification 不同。
// 【陷阱】只提供機械式 getter/setter 不等於良好 OOP；method 應表達 domain intent。
// ============================================================================

#include <cassert>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

class BankAccount {
public:
    BankAccount(std::string owner, int balance)
        : owner_(std::move(owner)), balance_(balance) {}

    void deposit(int amount);  // declaration；定義放在 class 外。
    int balance() const { return balance_; }
    const std::string& owner() const { return owner_; }

private:
    std::string owner_;
    int balance_;
};

void BankAccount::deposit(int amount)
{
    assert(amount > 0);
    balance_ += amount;
}

void basic_example()
{
    BankAccount account("Ada", 100);
    account.deposit(50);
    const BankAccount& read_only = account;
    assert(read_only.balance() == 150);
    std::cout << "[基礎] " << read_only.owner() << " balance=150\n";
}

// LeetCode 1929：method 對 class 保存的 vector 產生兩份串接結果。
class ArrayDuplicator {
public:
    explicit ArrayDuplicator(std::vector<int> values) : values_(std::move(values)) {}

    std::vector<int> concatenate() const
    {
        std::vector<int> result;
        result.reserve(values_.size() * 2U);
        result.insert(result.end(), values_.begin(), values_.end());
        result.insert(result.end(), values_.begin(), values_.end());
        return result;
    }

private:
    std::vector<int> values_;
};

void leetcode_1929_example()
{
    const ArrayDuplicator solution({1, 2, 1});
    assert((solution.concatenate() == std::vector<int>{1, 2, 1, 1, 2, 1}));
    std::cout << "[LeetCode 1929] class method 回傳 2n 陣列\n";
}

// 工作案例：BatchStats 讓 count/total/average 一律由同一份資料計算。
class BatchStats {
public:
    void add(int sample) { samples_.push_back(sample); }
    std::size_t count() const { return samples_.size(); }
    double average() const
    {
        assert(!samples_.empty());
        const int total = std::accumulate(samples_.begin(), samples_.end(), 0);
        return static_cast<double>(total) / static_cast<double>(samples_.size());
    }

private:
    std::vector<int> samples_;
};

void practical_example()
{
    BatchStats stats;
    stats.add(10);
    stats.add(20);
    assert(stats.count() == 2U && stats.average() == 15.0);
    std::cout << "[實務] samples=2 average=15\n";
}

int main()
{
    basic_example();
    leetcode_1929_example();
    practical_example();
}

// 實務提醒：query member 應標 const；修改 invariant 的 member 集中驗證，別讓 caller 直改欄位。
// 練習：讓 BatchStats 以 O(1) 回 average，不每次重新 accumulate，並維持 invariant。
// 生命週期：method 中的 this 只在 member call 有效；回傳 member reference 會受 owning object 約束。
