// ============================================================================
// 課題 20：std::unique_ptr（獨占 ownership）
// ============================================================================
//
// unique_ptr 表示「同一時間只有一個 owner」。它不可 copy、可以 move；owner 解構時
// 自動 delete。請用 std::make_unique 建立，避免裸 new 與 ownership 尚未交接時發生
// leak。`get()` 只借出 non-owning pointer，不轉移 ownership；`release()` 放棄 ownership
// 且不 delete，除非立即交給另一 owner，否則很容易 leak。
//
// function 介面語意：
//   * `unique_ptr<T>` by value：callee 接管 ownership。
//   * `const T&` / `T*`：只借用，不接管。
//   * `unique_ptr<T>&`：可修改 owner，本身較少見，應清楚說明。
//
// 【效能】預設 deleter 的 unique_ptr 通常與 raw pointer 同大小，沒有 reference count。
// 【陷阱】`unique_ptr<Base>` 刪 Derived 時，Base 仍需要 virtual destructor。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

class Session {
public:
    explicit Session(std::string id) : id_(std::move(id)) {}
    const std::string& id() const { return id_; }

private:
    std::string id_;
};

std::unique_ptr<Session> open_session(std::string id)
{
    return std::make_unique<Session>(std::move(id));
}

std::string consume_session(std::unique_ptr<Session> session)
{
    if (session == nullptr) {
        throw std::invalid_argument("consume_session requires an owner");
    }
    // 回傳一份 value 供 audit/log 使用；函式結束時 session 仍會被銷毀，ownership 已消費。
    return session->id();
}

void basic_example()
{
    std::unique_ptr<Session> owner = open_session("build-42");
    Session* borrowed = owner.get();
    if (borrowed == nullptr || borrowed->id() != "build-42") {
        throw std::logic_error("open_session broke its ownership contract");
    }
    assert(borrowed->id() == "build-42");
    const std::string consumed_id = consume_session(std::move(owner));
    assert(consumed_id == "build-42");
    assert(owner == nullptr); // moved-from unique_ptr 明確為 null。
    std::cout << "[基礎] ownership 已由 caller move 給 consumer\n";
}

// LeetCode 707：Design Linked List。
// 每個 node 獨占 next；刪節點只需 move ownership，整條 list 由 head_ 自動清理。
class MyLinkedList {
public:
    int get(int index) const
    {
        const Node* node = node_at(index);
        return node == nullptr ? -1 : node->value;
    }

    void addAtHead(int value)
    {
        auto node = std::make_unique<Node>(value);
        node->next = std::move(head_);
        head_ = std::move(node);
        ++size_;
    }

    void addAtTail(int value)
    {
        std::unique_ptr<Node>* link = &head_;
        while (*link != nullptr) link = &((*link)->next);
        *link = std::make_unique<Node>(value);
        ++size_;
    }

    void addAtIndex(int index, int value)
    {
        if (index < 0) index = 0;
        if (index > static_cast<int>(size_)) return;
        std::unique_ptr<Node>* link = link_at(index);
        auto node = std::make_unique<Node>(value);
        node->next = std::move(*link);
        *link = std::move(node);
        ++size_;
    }

    void deleteAtIndex(int index)
    {
        if (index < 0 || index >= static_cast<int>(size_)) return;
        std::unique_ptr<Node>* link = link_at(index);
        *link = std::move((*link)->next); // 被取代的 node 立即解構。
        --size_;
    }

private:
    struct Node {
        explicit Node(int node_value) : value(node_value) {}
        int value;
        std::unique_ptr<Node> next;
    };

    const Node* node_at(int index) const
    {
        if (index < 0 || index >= static_cast<int>(size_)) return nullptr;
        const Node* node = head_.get();
        for (int current = 0; current < index; ++current) node = node->next.get();
        return node;
    }

    std::unique_ptr<Node>* link_at(int index)
    {
        std::unique_ptr<Node>* link = &head_;
        for (int current = 0; current < index; ++current) link = &((*link)->next);
        return link;
    }

    std::unique_ptr<Node> head_;
    std::size_t size_ = 0U;
};

void leetcode_707_example()
{
    MyLinkedList list;
    list.addAtHead(1);
    list.addAtTail(3);
    list.addAtIndex(1, 2);
    assert(list.get(1) == 2);
    list.deleteAtIndex(1);
    assert(list.get(1) == 3);
    std::cout << "[LeetCode 707] unique_ptr linked list: 1->3\n";
}

// 實務案例：Service owns Repository；Repository 不可意外被另一 Service 共享。
class Repository {
public:
    explicit Repository(std::string endpoint) : endpoint_(std::move(endpoint)) {}
    const std::string& endpoint() const { return endpoint_; }
private:
    std::string endpoint_;
};

class Service {
public:
    explicit Service(std::unique_ptr<Repository> repository)
        : repository_(std::move(repository)) {}
    std::string health() const { return "ok:" + repository_->endpoint(); }
private:
    std::unique_ptr<Repository> repository_;
};

void practical_example()
{
    auto repository = std::make_unique<Repository>("db-primary");
    Service service(std::move(repository));
    assert(repository == nullptr && service.health() == "ok:db-primary");
    std::cout << "[實務] Service 獨占 db-primary repository\n";
}

int main()
{
    basic_example();
    leetcode_707_example();
    practical_example();
}

// 練習：為 MyLinkedList 加 reverse()，只用 std::move 改接 ownership links。
// 複雜度：head insert O(1)、遍歷 O(N)、整串解構 O(N)；move unique_ptr 本身 O(1)。

/*
【本課面試問答】
Q1：`unique_ptr<Derived>` 可轉成 `unique_ptr<Base>` 的必要安全條件是什麼？
A：型別轉換本身可成立，但若最後經 Base pointer 刪除 Derived，Base destructor 必須 virtual，或 deleter
必須保留正確具體刪除方式。否則刪除行為未定義；「smart pointer」不會自動修補錯誤多型介面。

Q2：linked list 用 `unique_ptr<Node> next` 表達了什麼？
A：每個 node 唯一擁有下一個 node，head 唯一擁有整條鏈；reverse 其實是逐段轉移 ownership。
prev/observer 若不擁有節點可用 raw pointer，但其生命期不得超過 owner。

Q3：為何 `release()` 很危險？
A：它只交出 raw pointer，不呼叫 deleter；若接收端沒有立即納入另一個 owner，資源就洩漏。大多數
ownership 轉移應直接 move `unique_ptr`，只有必須把責任交給明確接管的 C API 時才用 release。
*/
