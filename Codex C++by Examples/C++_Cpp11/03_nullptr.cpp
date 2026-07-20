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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 206. Reverse Linked List（反轉單向鏈結串列）
// 題目：輸入串列頭節點，原地反轉 next 方向並回傳新頭；例如 1->2->3 變成 3->2->1。
// 為何使用本章主題：nullptr 同時表示空串列、走訪終點與新尾節點的 next，清楚取代整數 0 空指標。
// 思路：1. previous 從 nullptr 開始；2. 暫存目前 next；3. 反向連結後將 previous/head 各前進一節點。
// 複雜度：N 為節點數；時間 O(N)、額外空間 O(1)。
// 易錯點：改寫 head->next 前一定先保存 next；這些 raw pointer 不擁有節點，也不能解參考 nullptr。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】可選的 C 風格完成回呼
// 情境：工作完成後可通知呼叫端，但未註冊 callback 時應合法略過通知。
// 為何使用本章主題：nullptr 對 function pointer 明確表示「沒有回呼」，不會像 0/NULL 參與錯誤的整數 overload。
// 設計：1. 接收結果與 Callback；2. 先比較 callback 是否為 nullptr；3. 只有存在時才以結果呼叫。
// 成本：檢查本身時間與空間皆 O(1)；真正成本及副作用取決於 callback 實作。
// 上線注意：函式指標必須仍有效；回呼可能拋例外、重入或跨執行緒觸碰共享狀態，API 需另訂契約。
// -----------------------------------------------------------------------------
using Callback = void (*)(int);
int observed = 0;

void remember(int value) { observed = value; }

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
