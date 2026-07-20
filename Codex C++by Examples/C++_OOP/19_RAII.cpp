// ============================================================================
// 課題 19：RAII（Resource Acquisition Is Initialization）
// ============================================================================
//
// RAII 把 resource lifetime 綁到 object lifetime：constructor/acquire 成功後物件代表
//「持有資源」，destructor/release 無條件清理。所有 return 路徑與 exception unwinding
// 都會解構 stack objects，因此比散落的 cleanup calls 穩定。
//
// resource 不只 heap memory：file、socket、mutex、GPU buffer、transaction、暫時設定都可
// 用 RAII。標準例子是 vector/string/unique_ptr/lock_guard。destructor 應 noexcept；
// 需要回報 close/commit 失敗時提供明確 operation，destructor 只做 best-effort rollback。
//
// 【面試】RAII 與 garbage collection 不同：RAII 在 deterministic scope exit 釋放所有
// resource，GC 主要追蹤 memory，回收時間通常不確定。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class ScopedFlag {
public:
    explicit ScopedFlag(bool& flag) : flag_(flag) { flag_ = true; }
    ~ScopedFlag() noexcept { flag_ = false; }
    ScopedFlag(const ScopedFlag&) = delete;
    ScopedFlag& operator=(const ScopedFlag&) = delete;

private:
    bool& flag_;
};

void basic_example()
{
    bool busy = false;
    {
        ScopedFlag guard(busy);
        assert(busy);
    }
    assert(!busy);
    std::cout << "[基礎] scope exit 自動把 busy 還原 false\n";
}

// LeetCode 206：Reverse Linked List。
// unique_ptr 表達每個 node 獨占 next；reverse 只移轉 ownership，不用 new/delete，
// 最終 head 離開 scope 時整串自動解構。
struct ListNode {
    explicit ListNode(int node_value) : value(node_value) {}
    int value;
    std::unique_ptr<ListNode> next;
};

std::unique_ptr<ListNode> reverse_list(std::unique_ptr<ListNode> head)
{
    std::unique_ptr<ListNode> previous;
    while (head != nullptr) {
        std::unique_ptr<ListNode> next = std::move(head->next);
        head->next = std::move(previous);
        previous = std::move(head);
        head = std::move(next);
    }
    return previous;
}

void leetcode_206_example()
{
    auto head = std::make_unique<ListNode>(1);
    head->next = std::make_unique<ListNode>(2);
    head->next->next = std::make_unique<ListNode>(3);
    head = reverse_list(std::move(head));
    assert(head->value == 3);
    assert(head->next->value == 2);
    assert(head->next->next->value == 1);
    std::cout << "[LeetCode 206] unique_ptr list reversed: 3->2->1\n";
}

// 實務案例：Transaction 預設 rollback；只有顯式 commit 才保留 staged changes。
class Transaction {
public:
    explicit Transaction(std::vector<std::string>& database)
        : database_(database), original_size_(database.size()) {}

    ~Transaction() noexcept
    {
        if (!committed_) database_.resize(original_size_);
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    void insert(std::string row) { database_.push_back(std::move(row)); }
    void commit() noexcept { committed_ = true; }

private:
    std::vector<std::string>& database_;
    std::size_t original_size_;
    bool committed_ = false;
};

void practical_example()
{
    std::vector<std::string> rows{"stable"};
    {
        Transaction transaction(rows);
        transaction.insert("temporary");
        // 未 commit，scope exit rollback。
    }
    assert((rows == std::vector<std::string>{"stable"}));
    {
        Transaction transaction(rows);
        transaction.insert("committed");
        transaction.commit();
    }
    assert(rows.size() == 2U && rows.back() == "committed");
    std::cout << "[實務] RAII transaction rollback/commit 正確\n";
}

int main()
{
    basic_example();
    leetcode_206_example();
    practical_example();
}

// 易錯與面試：RAII 的重點是資源 lifetime 綁 object lifetime，不是只對 heap。lock、file、
// transaction 都適用；destructor cleanup 應 noexcept，ownership 要用型別表達。
// 練習：用 std::lock_guard<std::mutex> 改寫一個 thread-safe counter。
// 複雜度：wrapper 通常 O(1)，但 acquire/release 可能 syscall、lock contention 或 I/O。
// 生命週期：resource validity 精確綁 owner scope；move 可轉移，raw handle borrow 不得超過 owner。
