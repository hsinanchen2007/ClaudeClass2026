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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 206. Reverse Linked List（反轉鏈結串列）
// 題目：反轉單向串列；例如 1->2->3 變成 3->2->1。
// 為何使用本章主題：unique_ptr 將每個 next 表達成獨占資源，reverse 只移轉 ownership，無需裸 new/delete cleanup。
// 思路：move 出 head->next；把 head->next 接到 previous；把 head 移成 previous；繼續處理保存的 next。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為節點數。
// 易錯點：每次覆寫 unique_ptr 前先保存下一段；呼叫會消耗原 head；極長鏈的遞迴式解構深度也需評估。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】預設回滾的記憶體交易 guard
// 情境：在 vector database 尾端暫存多筆 row；scope 未 commit 時恢復原大小，commit 才保留新增資料。
// 為何使用本章主題：RAII destructor 涵蓋正常 return 與例外路徑，比要求每條路徑手動 rollback 更可靠。
// 設計：constructor 記錄原大小；insert 追加；commit 設旗標；未 commit 的 destructor resize 回原大小。
// 成本：insert 攤銷 O(1)；rollback 解構新增的 K 筆為 O(K)；guard 自身空間 O(1)。
// 上線注意：只會撤銷尾端新增，不能回復既有 row 修改；database reference 必須更長壽，並行 writer 需交易同步。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '19_RAII.cpp' -o '/tmp/codex_cpp_C_OOP_19_RAII' && '/tmp/codex_cpp_C_OOP_19_RAII'
//
// === 預期輸出（節錄）===
// [實務] RAII transaction rollback/commit 正確
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
