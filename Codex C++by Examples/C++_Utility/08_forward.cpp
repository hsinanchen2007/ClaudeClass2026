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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2. Add Two Numbers（兩數相加）
// 題目：兩條反向儲存十進位數字的 linked list 相加；[2,4,3]+[5,6,4] 產生 [7,0,8]。
// 為何使用本章主題：make_object 以 forwarding reference 與 std::forward 把節點值交給 constructor；
// 對 int 並無效能必要，這是把通用 factory 套入題目的 perfect-forwarding 教學示範。
// 思路：以 sentinel 管理結果頭；逐位加上兩邊值與 carry；建立個位數節點並更新進位。
// 複雜度：時間 O(max(M,N))、結果空間 O(max(M,N))，M、N 是兩條 list 的節點數。
// 易錯點：最後 carry 非零時仍要建節點；輸入 digit 應為 0..9，unique_ptr 負責結果串列所有權。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】保留引數類別的事件 factory
// 情境：呼叫端要保留仍會使用的 lvalue 事件名稱，同時把臨時 payload 高效移入 Event。
// 為何使用本章主題：practical_make_event 以 forwarding references 接收參數，再用 std::forward
// 保留 lvalue/rvalue；相較無條件 std::move，不會意外搬空 name。
// 設計：factory 推導每個 Args；原樣轉送給 Event constructor；constructor 複製 lvalue name、移動 rvalue payload。
// 成本：name 複製 O(L)，payload 移動通常 O(1)，L 是名稱長度；Event 本身保存 payload 空間。
// 上線注意：同一參數不可 forward 給多個消費者；braced list 推導受限，且 temporary 生命週期不會被延長。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_forward.cpp' -o '/tmp/codex_cpp_C_Utility_08_forward' && '/tmp/codex_cpp_C_Utility_08_forward'
//
// === 預期輸出（節錄）===
// forward 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
