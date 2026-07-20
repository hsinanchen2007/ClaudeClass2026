/*
 * C++11 教科書：nullptr 與空指標語意
 *
 * nullptr 的型別是 std::nullptr_t，可轉成任何 object/function/member pointer，
 * 但不是整數 0。舊式 NULL 常只是巨集 0，遇到 f(int) 與 f(char*) overload 時
 * 可能選錯或產生歧義；nullptr 能明確選 pointer overload。
 *
 * 【指標狀態】nullptr 只表示「不指向物件」，不代表指標擁有資源。擁有權優先用
 * unique_ptr/shared_ptr；raw pointer 適合 non-owning observer 或與 C API 交界。
 * 【常見陷阱】解參考 nullptr 是 undefined behavior；先檢查不等於 nullptr。
 * 【面試題】delete nullptr 是否安全？是，標準保證 no-op。
 * 【練習】為 reverse_list 加空串列及單節點測試。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace basic {
std::string choose(int) { return "整數 overload"; }
std::string choose(const char*) { return "指標 overload"; }

void demo() {
    static_assert(nullptr == static_cast<int*>(nullptr),
                  "nullptr 應能轉換為 typed null pointer");
    assert(choose(nullptr) == "指標 overload");
    // choose(NULL) 不推薦：NULL 的實作可能是整數常數。
}
}  // namespace basic

namespace leetcode {
// LeetCode 206：Reverse Linked List，時間 O(n)、額外空間 O(1)。
// prev/current/next 都是 observer，不負責 delete；本例節點由 stack 管理。
struct ListNode {
    int value;
    ListNode* next;
};

ListNode* reverse_list(ListNode* head) {
    ListNode* previous = nullptr;
    while (head != nullptr) {
        ListNode* const next = head->next;
        head->next = previous;
        previous = head;
        head = next;
    }
    return previous;
}

void test() {
    ListNode third{3, nullptr};
    ListNode second{2, &third};
    ListNode first{1, &second};
    const ListNode* reversed = reverse_list(&first);
    assert(reversed != nullptr && reversed->value == 3);
    assert(reversed->next != nullptr && reversed->next->value == 2);
    assert(reversed->next->next != nullptr && reversed->next->next->value == 1);
    assert(reversed->next->next->next == nullptr);
}
}  // namespace leetcode

// 【實務案例】C 風格 optional callback：nullptr 明確表示呼叫者不要求完成通知。
namespace practical {
using Callback = void (*)(int);
int observed = 0;

void remember(int value) { observed = value; }

// 實務：模擬 C API 的 optional callback。nullptr 代表呼叫者不需要通知。
void complete_job(int result, Callback callback) {
    if (callback != nullptr) {
        callback(result);
    }
}

void test() {
    complete_job(42, &remember);
    assert(observed == 42);
    complete_job(7, nullptr);  // 合法：沒有 callback，不做事。
    assert(observed == 42);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "nullptr：overload、鏈結串列、optional callback 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_nullptr.cpp' -o '/tmp/codex_cpp_C_Cpp11_03_nullptr' && '/tmp/codex_cpp_C_Cpp11_03_nullptr'
//
// === 預期輸出（節錄）===
// nullptr：overload、鏈結串列、optional callback 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
