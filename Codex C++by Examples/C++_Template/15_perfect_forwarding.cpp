/*
 * 第 15 章：完美轉發（perfect forwarding）
 *
 * 在型別推導情境，T&& 可能是 forwarding reference：傳 lvalue 時 T 推導為 U&，
 * reference collapsing 讓 T&& 變 U&；傳 rvalue 時 T=U，T&& 保持 rvalue reference。
 * std::forward<T>(arg) 依推導結果保留 value category。直接 std::move(arg) 會把 lvalue
 * 也強制移走，不叫完美轉發。
 */

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

template <typename T, typename... Args>
std::unique_ptr<T> make_owned(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

struct ListNode {
    int value{};
    std::unique_ptr<ListNode> next;

    explicit ListNode(int initial) : value(initial) {}
};

// LeetCode 2：Add Two Numbers。節點以 make_owned 完美轉發建構參數。
std::unique_ptr<ListNode> leetcode_add_two_numbers(const ListNode* left, const ListNode* right) {
    auto sentinel = make_owned<ListNode>(0);
    ListNode* tail = sentinel.get();
    int carry = 0;
    while (left != nullptr || right != nullptr || carry != 0) {
        const int sum = (left == nullptr ? 0 : left->value) +
                        (right == nullptr ? 0 : right->value) + carry;
        tail->next = make_owned<ListNode>(sum % 10);
        tail = tail->next.get();
        carry = sum / 10;
        left = left == nullptr ? nullptr : left->next.get();
        right = right == nullptr ? nullptr : right->next.get();
    }
    return std::move(sentinel->next);
}

std::unique_ptr<ListNode> make_list(std::initializer_list<int> digits) {
    auto sentinel = make_owned<ListNode>(0);
    ListNode* tail = sentinel.get();
    for (int digit : digits) {
        tail->next = make_owned<ListNode>(digit);
        tail = tail->next.get();
    }
    return std::move(sentinel->next);
}

struct Event {
    std::string name;
    std::vector<int> payload;

    Event(std::string event_name, std::vector<int> event_payload)
        : name(std::move(event_name)), payload(std::move(event_payload)) {}
};

// 實務：直接在 vector 儲存區內建構，避免先建立 Event 再搬移。
class EventQueue {
public:
    template <typename... Args>
    Event& emplace(Args&&... args) {
        return events_.emplace_back(std::forward<Args>(args)...);
    }

    const Event& last() const { return events_.back(); }

private:
    std::vector<Event> events_;
};

void practical_event_queue_test() {
    EventQueue queue;
    std::vector<int> payload{7, 8, 9};
    queue.emplace("audit", std::move(payload));
    assert(queue.last().name == "audit");
    assert(queue.last().payload.size() == 3U);
}

int main() {
    std::string name = "deploy";
    auto event = make_owned<Event>(name, std::vector<int>{1, 2});
    assert(name == "deploy");       // lvalue 被保留，不應遭移走
    assert(event->payload.size() == 2U);

    const auto left = make_list({2, 4, 3});
    const auto right = make_list({5, 6, 4});
    const auto result = leetcode_add_two_numbers(left.get(), right.get());
    assert(result->value == 7);
    assert(result->next->value == 0);
    assert(result->next->next->value == 8);

    practical_event_queue_test();

    std::cout << "perfect forwarding 測試完成\n";
}

/*
 * 【陷阱】
 * - forwarding reference 必須是「正在推導的 T&&」；class<T> 裡的 T&& 通常只是 rvalue reference。
 * - named rvalue reference 變數本身是 lvalue，所以往下一層傳時仍需 std::forward。
 * - 同一參數不要 forward 兩次：第一次可能已移走內容。
 * 【面試】std::forward 做了什麼？條件式 cast，不會自行搬資料；是否搬移由接收端決定。
 * 【練習】為 EventQueue 加 constrained emplace，要求 Event 可由 Args... 建構。
 */

/*
 * 【教科書補充：哪些 T&& 才是 forwarding reference】
 * - 必須是當下正在推導、未加 const 的 `T&&`；`const T&&` 與 class-template 已固定 T 的 member `T&&` 都不是。
 * - braced-init-list、overload set、bit-field 等常無法直接推導，需明確型別、cast 或先存具名物件。
 * - forward 只做條件式 cast，不延長 lifetime；將 temporary 轉發給會保存 reference 的 API 仍會懸空。
 * - EventQueue::emplace/last 回傳的 Event& 會在 vector reallocation 後失效，不可跨後續 emplace 保存。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '15_perfect_forwarding.cpp' -o '/tmp/codex_cpp_C_Template_15_perfect_forwarding' && '/tmp/codex_cpp_C_Template_15_perfect_forwarding'
//
// === 預期輸出（節錄）===
// perfect forwarding 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
