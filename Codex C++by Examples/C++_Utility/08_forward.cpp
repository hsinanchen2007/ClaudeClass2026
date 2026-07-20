/*
 * 第 08 章：std::forward
 *
 * std::forward<T>(value) 依 T 的推導結果保留原呼叫端 value category。它只應搭配
 * forwarding reference 使用。`std::move` 無條件轉 rvalue；`std::forward` 有條件轉。
 * 常見位置：factory、emplace、wrapper，將引數原樣交給下一層 constructor/callable。
 */

#include <cassert>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

template <typename T, typename... Args>
std::unique_ptr<T> make_object(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

struct ListNode {
    int value{};
    std::unique_ptr<ListNode> next;
    explicit ListNode(int initial) : value(initial) {}
};

std::unique_ptr<ListNode> make_list(std::initializer_list<int> values) {
    auto sentinel = make_object<ListNode>(0);
    ListNode* tail = sentinel.get();
    for (int value : values) {
        tail->next = make_object<ListNode>(value);
        tail = tail->next.get();
    }
    return std::move(sentinel->next);
}

// LeetCode 2：Add Two Numbers；factory 以 forward 將 int 原樣交給 ListNode constructor。
std::unique_ptr<ListNode> leetcode_add_two_numbers(const ListNode* left,
                                                   const ListNode* right) {
    auto sentinel = make_object<ListNode>(0);
    ListNode* tail = sentinel.get();
    int carry = 0;
    while (left != nullptr || right != nullptr || carry != 0) {
        const int sum = (left == nullptr ? 0 : left->value) +
                        (right == nullptr ? 0 : right->value) + carry;
        tail->next = make_object<ListNode>(sum % 10);
        tail = tail->next.get();
        carry = sum / 10;
        left = left == nullptr ? nullptr : left->next.get();
        right = right == nullptr ? nullptr : right->next.get();
    }
    return std::move(sentinel->next);
}

// 【實務情境】事件 factory 同時接受 lvalue 名稱與 rvalue payload，保留各自 value category。
struct Event {
    std::string name;
    std::vector<int> payload;
    Event(std::string event_name, std::vector<int> values)
        : name(std::move(event_name)), payload(std::move(values)) {}
};

template <typename... Args>
Event practical_make_event(Args&&... args) {
    return Event(std::forward<Args>(args)...);
}

int main() {
    std::string name = "audit";
    Event event = practical_make_event(name, std::vector<int>{1, 2, 3});
    assert(name == "audit"); // lvalue 未被無條件 move
    assert(event.payload.size() == 3U);

    const auto left = make_list({2, 4, 3});
    const auto right = make_list({5, 6, 4});
    const auto answer = leetcode_add_two_numbers(left.get(), right.get());
    assert(answer->value == 7);
    assert(answer->next->value == 0);
    assert(answer->next->next->value == 8);

    std::cout << "forward 測試完成\n";
}

/*
 * 【常見陷阱】
 * - class<T> 成員參數 T&& 若 T 已在 class 實體化時固定，就不是 forwarding reference。
 * - named `args` 一律是 lvalue；若漏掉 forward，rvalue 也會走 copy overload。
 * - 同一 args 不要 forward 到兩個消費者；第一個可能已把資源取走。
 * - braced initializer list 通常無法直接推導 forwarding parameter，需明寫 initializer_list。
 *
 * 【面試段落】何謂 perfect forwarding？盡可能保留型別 cv/ref 與 value category，
 * 不是保證零複製，也不會延長 temporary 的生命週期。
 * 【練習】寫 invoke_and_log(F&&, Args&&...)；回傳型別用 decltype(auto)，並處理 void。
 */
