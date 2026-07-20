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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接）
// 題目：輸出長度 2N 的 answer，前後兩半都等於 nums；例如 [1,2,1] 得 [1,2,1,1,2,1]。
// 為何使用本章主題：題目不要求 class state；本例教學改寫成 const member function，讀取物件保存的 vector。
// 思路：建立結果並 reserve 2N；插入 values_ 一次；再插入同一範圍一次；回傳新 vector。
// 複雜度：時間 O(N)、必要輸出空間 O(N)，N 為輸入元素數。
// 易錯點：reserve 不改 size；2*N 的容量計算可能溢位；concatenate 標 const 才能由唯讀物件呼叫。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】批次樣本統計物件
// 情境：依序加入量測樣本，查詢 count 與 average 時必須永遠來自同一份資料，避免外部計數不同步。
// 為何使用本章主題：member functions 封裝 samples_ 的修改與唯讀查詢，const average 不會改變批次內容。
// 設計：add 追加樣本；count 讀 size；average 累加全部樣本後除以筆數。
// 成本：add 攤銷 O(1)，count O(1)，average O(N) 時間與 O(1) 額外空間。
// 上線注意：空批次不可求平均；int total 可能溢位；高頻查詢可同步維護寬型別總和，但要守住 invariant。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '2_MemberAndMethod.cpp' -o '/tmp/codex_cpp_C_OOP_2_MemberAndMethod' && '/tmp/codex_cpp_C_OOP_2_MemberAndMethod'
//
// === 預期輸出（節錄）===
// [實務] samples=2 average=15
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
