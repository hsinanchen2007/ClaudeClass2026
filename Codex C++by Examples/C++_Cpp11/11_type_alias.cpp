/*
 * C++11 教科書：using type alias
 *
 * using Name = ExistingType; 與 typedef 都能替型別取別名，但 using 由左至右較易讀，
 * 且能建立 alias template。別名不創造新型別：UserId 與 int 仍可互相混用；若需要
 * 防止 OrderId 傳給 UserId，應建立 wrapper class/strong typedef。
 *
 * 【用途】縮短 callback/container 型別、隔離 implementation detail、建立 generic alias。
 * 【維護】別名應表達 domain intent，不要只把 vector<int> 改名成 VecInt 而沒有語意。
 * 【陷阱】using Ptr = int*; const Ptr 是 int* const，不是 const int*。
 * 【面試題】alias template 為何比 template typedef 容易？typedef 本身不能直接模板化。
 * 【練習】把 Graph alias 改成帶權重的 adjacency list。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace basic {
using Scores = std::vector<int>;
template <class T>
using Matrix = std::vector<std::vector<T>>;

void demo() {
    const Scores scores{80, 90};
    const Matrix<int> grid{{1, 2}, {3, 4}};
    static_assert(std::is_same<Scores, std::vector<int> >::value,
                  "using alias 不應建立不同型別");
    assert(scores.size() == 2U && grid[1][0] == 3);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 141. Linked List Cycle（鏈結串列是否成環）
// 題目：輸入鏈結串列頭節點，判斷沿 next 是否會重訪節點；例如尾節點連回第二節點時回 true。
// 為何使用本章主題：NodePtr 縮短重複的 ListNode* 宣告並表達 observer 角色，但 alias 不會建立新型別或 ownership。
// 思路：1. slow 每輪走一步；2. fast 每輪走兩步；3. 相遇代表有環，fast 到 nullptr 則無環。
// 複雜度：N 為可達節點數；時間 O(N)、額外空間 O(1)。
// 易錯點：先確認 fast 與 fast->next 非空再走兩步；NodePtr 不負責 delete，也不延長節點生命週期。
// -----------------------------------------------------------------------------
struct ListNode {
    int value;
    ListNode* next;
};
using NodePtr = ListNode*;

bool has_cycle(NodePtr head) {
    NodePtr slow = head;
    NodePtr fast = head;
    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            return true;
        }
    }
    return false;
}

void test() {
    ListNode a{1, nullptr};
    ListNode b{2, nullptr};
    ListNode c{3, nullptr};
    a.next = &b;
    b.next = &c;
    c.next = &b;
    assert(has_cycle(&a));
    c.next = nullptr;
    assert(!has_cycle(&a));
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】使用者權限查詢表
// 情境：小型 ACL 以使用者 ID 對應權限集合，要判斷指定使用者是否具 read 權限。
// 為何使用本章主題：UserId、PermissionSet、AccessTable aliases 把巢狀 STL 型別換成領域名稱，降低簽章閱讀負擔。
// 設計：1. 線性尋找 user；2. 命中後查 PermissionSet 的 read；3. 找不到使用者即回 false。
// 成本：U 為使用者數、P 為單人權限數；平均時間 O(U+1)，雜湊最壞受 P 影響，儲存空間 O(U*P)。
// 上線注意：using 不防 UserId 與普通 string 混用；大型 ACL 應以 user 為 key 建索引並處理同步更新。
// -----------------------------------------------------------------------------
using UserId = std::string;
using PermissionSet = std::unordered_set<std::string>;
using AccessTable = std::vector<std::pair<UserId, PermissionSet>>;

bool can_read(const AccessTable& table, const UserId& user) {
    for (AccessTable::const_iterator it = table.begin(); it != table.end(); ++it) {
        const UserId& id = it->first;
        const PermissionSet& permissions = it->second;
        if (id == user) {
            return permissions.count("read") != 0U;
        }
    }
    return false;
}

void test() {
    const AccessTable access{{"alice", {"read", "write"}}, {"bob", {"write"}}};
    assert(can_read(access, "alice"));
    assert(!can_read(access, "bob"));
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "type alias：alias template、cycle detection、ACL 測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_type_alias.cpp' -o '/tmp/codex_cpp_C_Cpp11_11_type_alias' && '/tmp/codex_cpp_C_Cpp11_11_type_alias'
//
// === 預期輸出（節錄）===
// type alias：alias template、cycle detection、ACL 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
