// ============================================================================
// 課題 3：std::weak_ptr - 不延長生命的 observer 與 cycle breaker
// ============================================================================
//
// weak_ptr 指向 shared control block但不增加 strong count。`lock()` 成功回 shared_ptr，
// object 已死則回 empty；不可先 `expired()` 再假設 lock 必成功，兩步間可能有 race，
// 直接 `if (auto owner=weak.lock())`。
//
// 雙向 parent/child 或 graph edges 全是 shared_ptr 會 cycle。決定主要 ownership direction：
// parent strong-owns child，child weak-observes parent。Cache 可 weakly index objects，讓
// cache 本身不阻止回收。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Node {
public:
    explicit Node(std::string name) : name_(std::move(name)) {}
    void add_child(const std::shared_ptr<Node>& child)
    {
        child->parent_ = shared_from_this();
        children_.push_back(child);
    }
    std::string parent_name() const
    {
        const auto parent = parent_.lock();
        return parent == nullptr ? "none" : parent->name_;
    }
    const std::string& name() const { return name_; }

private:
    // 為縮小範例，手動用內部 weak self；第 4 課改用 enable_shared_from_this 正規 API。
    std::shared_ptr<Node> shared_from_this()
    {
        return self_.lock();
    }
    void bind_self(const std::shared_ptr<Node>& self) { self_ = self; }
    std::string name_;
    std::vector<std::shared_ptr<Node>> children_;
    std::weak_ptr<Node> parent_;
    std::weak_ptr<Node> self_;

public:
    static std::shared_ptr<Node> create_bound(std::string name)
    {
        auto node = std::shared_ptr<Node>(new Node(std::move(name)));
        node->bind_self(node);
        return node;
    }
};

void basic_example()
{
    auto parent = Node::create_bound("root");
    auto child = Node::create_bound("leaf");
    parent->add_child(child);
    assert(child->parent_name() == "root");
    parent.reset(); // child 的 weak parent 不延長 root 生命。
    assert(child->parent_name() == "none");
    std::cout << "[基礎] weak parent expires after root owner reset\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 138. Copy List with Random Pointer（複製帶隨機指標的鏈結串列）
// 題目：深複製 next/random 指標；例如第二節點 random 指回第一節點時，副本也要指回副本第一節點。
// 為何使用本章主題：next 以 shared_ptr 維持主 ownership chain，random 用 weak_ptr 表示 observer，避免反向 cycle。
// 思路：第一趟為每個來源節點建 clone map；第二趟重接 clone next；lock random 後映射到 clone；回 clone head。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 next chain 節點數。
// 易錯點：random 必須為 null 或指向同一 chain；lock 可能失敗；副本 random 不可仍指來源節點。
// -----------------------------------------------------------------------------
struct RandomNode {
    explicit RandomNode(int node_value) : value(node_value) {}
    int value;
    std::shared_ptr<RandomNode> next;
    std::weak_ptr<RandomNode> random;
};

std::shared_ptr<RandomNode> copy_random_list(const std::shared_ptr<RandomNode>& head)
{
    if (head == nullptr) return nullptr;
    std::unordered_map<const RandomNode*, std::shared_ptr<RandomNode>> clones;
    for (auto current = head; current != nullptr; current = current->next) {
        clones.emplace(current.get(), std::make_shared<RandomNode>(current->value));
    }
    for (auto current = head; current != nullptr; current = current->next) {
        auto& clone = clones.at(current.get());
        if (current->next != nullptr) clone->next = clones.at(current->next.get());
        if (const auto random = current->random.lock()) clone->random = clones.at(random.get());
    }
    return clones.at(head.get());
}

void leetcode_138_example()
{
    auto first = std::make_shared<RandomNode>(7);
    auto second = std::make_shared<RandomNode>(13);
    first->next = second;
    second->random = first;
    const auto copy = copy_random_list(first);
    assert(copy.get() != first.get());
    assert(copy->value == 7 && copy->next->value == 13);
    assert(copy->next.get() != second.get());
    assert(copy->next->random.lock().get() == copy.get());

    first->value = 99;
    assert(copy->value == 7); // deep copy 不受來源修改影響。
    std::cout << "[LeetCode 138] next/random 關係與 deep-copy independence 均驗證\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】不阻止資產回收的 weak cache entry
// 情境：cache 想快速觀察 Asset 42，但 caller 釋放最後 owner 後，cache 不應獨自延長大型資產生命。
// 為何使用本章主題：weak_ptr 可保存 control-block observer 而不增加 strong count，比 shared_ptr cache 更符合可回收語意。
// 設計：caller 建 shared Asset；cache_entry 接 weak observer；使用時 lock；caller scope 結束後 entry 變 expired。
// 成本：指定/expired/lock 為常數 control-block 操作，真正載入資產成本另計。
// 上線注意：不要先 expired 再 lock；多執行緒下只能以 lock 結果判斷，cache 還需清理 expired keys 與防重複載入。
// -----------------------------------------------------------------------------
class Asset { public: explicit Asset(int id) : id_(id) {} int id() const { return id_; } private: int id_; };

void practical_example()
{
    std::weak_ptr<Asset> cache_entry;
    {
        auto asset = std::make_shared<Asset>(42);
        cache_entry = asset;
        assert(cache_entry.lock()->id() == 42);
    }
    assert(cache_entry.expired());
    std::cout << "[實務] weak cache entry expires when callers release asset\n";
}

int main()
{
    basic_example();
    leetcode_138_example();
    practical_example();
}

// 練習：移除 self_ workaround，使用下一課的 enable_shared_from_this。
// 複雜度：lock/expired/reset 通常 O(1)，但 lock 涉及同步的 control-block 狀態轉換。
// 生命週期：weak_ptr 本身可比 pointee 活更久；只有 lock 成功得到的 shared_ptr 才延長存活。

/*
【本課面試問答】
Q1：為何不要先 `expired()` 再使用，而應直接 `lock()`？
A：兩次呼叫間最後一個 strong owner 可能消失；`expired()==false` 只是瞬間觀察。`lock()` 以單一
同步動作嘗試取得 strong owner，成功後該物件至少活到所得 `shared_ptr` 離開 scope。

Q2：物件銷毀後，為何 `weak_ptr` 還能存在？
A：strong count 歸零時 managed object 被銷毀，但 control block 必須保留 weak count 與 expired
狀態；等最後一個 weak owner 也消失才回收 control block。`weak_ptr` 不延長 pointee 生命。

Q3：`weak_ptr` 只用來打破 cycle 嗎？
A：不是。它也適合 cache、observer、parent back-reference 等「能用就用、消失也合法」的非擁有
關係。若業務規則要求 dependency 必須存在，應保存 strong owner 或重新設計 ownership，而非每次
`lock()` 失敗後默默略過。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_weak_ptr.cpp' -o '/tmp/codex_cpp_C_SmartPointers_03_weak_ptr' && '/tmp/codex_cpp_C_SmartPointers_03_weak_ptr'
//
// === 預期輸出（節錄）===
// [實務] weak cache entry expires when callers release asset
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
