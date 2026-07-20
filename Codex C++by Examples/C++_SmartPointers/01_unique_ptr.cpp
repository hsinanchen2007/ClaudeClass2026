// ============================================================================
// 課題 1：std::unique_ptr - 預設的 owning pointer
// ============================================================================
//
// unique_ptr<T> 表示獨占 ownership：不可 copy、可 move，owner 解構時 delete T。
// `make_unique` 把 allocation 與 owner 建立放在同一 expression，避免 exception window。
// array 使用 unique_ptr<T[]>，operator[] 可用，但通常 vector 更有 size/iterator API。
//
// API 語意：by value=接管 ownership；T*/T&=borrow；unique_ptr&=修改 owner。
// `get()` 借用但不釋放；`release()` 放棄且不 delete；`reset()` 刪舊物件再接新物件。
// 不可讓借出的 pointer 活過 owner，也不可用兩個 unique_ptr 擁有同一 raw pointer。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Resource {
public:
    explicit Resource(std::string name) : name_(std::move(name)) {}
    const std::string& name() const { return name_; }
private:
    std::string name_;
};

std::unique_ptr<Resource> create_resource()
{
    return std::make_unique<Resource>("GPU-buffer");
}

void basic_example()
{
    auto first_owner = create_resource();
    Resource* borrowed = first_owner.get();
    assert(borrowed->name() == "GPU-buffer");
    std::unique_ptr<Resource> second_owner = std::move(first_owner);
    assert(first_owner == nullptr && second_owner->name() == "GPU-buffer");
    std::cout << "[基礎] ownership moved; source unique_ptr is null\n";
}

// LeetCode 206：Reverse Linked List；unique_ptr 版本以 move 重接 ownership。
struct ListNode {
    explicit ListNode(int node_value) : value(node_value) {}
    int value;
    std::unique_ptr<ListNode> next;
};

std::unique_ptr<ListNode> reverse_list(std::unique_ptr<ListNode> head)
{
    std::unique_ptr<ListNode> previous;
    while (head != nullptr) {
        auto next = std::move(head->next);
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
    assert(head->value == 3 && head->next->value == 2 && head->next->next->value == 1);
    std::cout << "[LeetCode 206] unique ownership reversed 3->2->1\n";
}

// 實務：Controller 明確獨占 Backend，呼叫端 move 後不能再使用 owner。
class Backend {
public:
    std::string status() const { return "ready"; }
};
class Controller {
public:
    explicit Controller(std::unique_ptr<Backend> backend) : backend_(std::move(backend)) {}
    std::string health() const { return backend_->status(); }
private:
    std::unique_ptr<Backend> backend_;
};

void practical_example()
{
    auto backend = std::make_unique<Backend>();
    Controller controller(std::move(backend));
    assert(backend == nullptr && controller.health() == "ready");
    std::cout << "[實務] Controller owns exactly one Backend\n";
}

int main()
{
    basic_example();
    leetcode_206_example();
    practical_example();
}

// 練習：為 linked list 加 insert_after；列出哪些 pointers 是 owner、哪些只是 borrow。
// 複雜度：move/release/get 通常 O(1)；reset/destructor 還要加上 pointee deleter 的成本。
// 生命週期：最後且唯一的 owner 解構或 reset 時釋放；由 get() 取得的 borrow 不得活得更久。
