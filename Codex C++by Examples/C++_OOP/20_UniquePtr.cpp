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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List（設計鏈結串列）
// 題目：支援 get、頭尾插入、指定索引插入/刪除；例如 1->3 中索引 1 插 2，再刪 2 復原 1->3。
// 為何使用本章主題：每個 Node 以 unique_ptr 獨占 next，link_at 可藉移動 ownership 安全插刪且自動釋放節點。
// 思路：head 操作改第一個 owning link；尾/索引操作走到 link pointer；插入重接兩段；刪除以 next 取代目前 owner。
// 複雜度：頭插 O(1)，get/尾插/索引操作 O(N)，空間 O(N)，N 為節點數。
// 易錯點：負插入索引視為 0、越界依題意忽略；size_ 轉 int 可能超範圍；移動 link 後不可再用舊 owner。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】服務獨占資料儲存庫
// 情境：每個 Service 接管一個指向 db-primary 的 Repository，不允許另一服務意外共享同一可變連線物件。
// 為何使用本章主題：constructor 按值接 unique_ptr，型別明確表示 ownership transfer，Service 生命結束時自動清理 repository。
// 設計：caller 以 make_unique 建 owner；move 進 Service；原 pointer 變 null；health 透過唯一 owner 查 endpoint。
// 成本：ownership move O(1)，health 字串組合 O(E)，E 為 endpoint 長度；資源清理成本由 Repository 決定。
// 上線注意：Service constructor 應拒絕 null owner；不得保存 move 前的 raw borrow，跨執行緒使用仍需同步。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '20_UniquePtr.cpp' -o '/tmp/codex_cpp_C_OOP_20_UniquePtr' && '/tmp/codex_cpp_C_OOP_20_UniquePtr'
//
// === 預期輸出（節錄）===
// [實務] Service 獨占 db-primary repository
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
