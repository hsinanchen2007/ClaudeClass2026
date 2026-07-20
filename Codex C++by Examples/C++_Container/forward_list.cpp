// ============================================================================
// forward_list：單向鏈結串列與「前一個位置」API
// ============================================================================
// std::forward_list<T> 每個節點只保存 next 指標，空間通常少於 list；只能往前走，
// 沒有 size()、back()、push_back()。insert_after/erase_after 需要「目標前一節點」，
// 因此提供 before_begin() 這個位於第一元素之前的虛擬 iterator。
//
// 已知前一 iterator 時插刪 O(1)；搜尋仍 O(n)。插入不使其他 iterator 失效，erase
// 只使被刪節點失效。適合 free-list、bucket chain、只需單向掃描的低額外成本節點。

#include <cassert>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <vector>

std::vector<int> snapshot(const std::forward_list<int>& values)
{
    return {values.begin(), values.end()};
}

void basic_demo()
{
    std::forward_list<int> values{2, 3};
    values.push_front(1);
    auto before_two = values.begin();
    values.insert_after(before_two, 10);
    assert((snapshot(values) == std::vector<int>{1, 10, 2, 3}));

    values.erase_after(before_two);  // 刪除 10；before_two 仍有效。
    assert((snapshot(values) == std::vector<int>{1, 2, 3}));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 206. Reverse Linked List（反轉鏈結串列）
// 題目：輸入單向鏈結串列並反轉節點次序；例如 1->2->3->4->5 變成 5->4->3->2->1。
// 為何使用本章主題：forward_list 就是單向鏈結結構；本例以成員 reverse 教學展示重接 next，而非手寫 ListNode。
// 思路：以傳值取得串列 ownership；呼叫 reverse 重接所有節點；將反轉後容器移回呼叫端。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為節點數，不計傳值參數在呼叫端可能造成的複製。
// 易錯點：原題常要求手寫三指標；本 API 會消耗或複製輸入；空串列與單節點也必須成立。
// -----------------------------------------------------------------------------
std::forward_list<int> reverse_values(std::forward_list<int> values)
{
    values.reverse();
    return values;
}

void leetcode_demo()
{
    const auto reversed = reverse_values({1, 2, 3, 4, 5});
    assert((snapshot(reversed) == std::vector<int>{5, 4, 3, 2, 1}));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】清理 allow-list 中的過期權杖
// 情境：記憶體內白名單保存 token id 與到期時間，定期移除 expires_at 不晚於目前時間的項目。
// 為何使用本章主題：只需單向掃描與節點刪除，forward_list 節點負擔較低，remove_if 會正確維護前驅。
// 設計：逐節點檢查 expires_at；符合過期條件即解除該節點；保留其餘 token 原有次序。
// 成本：時間 O(N)、額外空間 O(1)，N 為 token 數；每個被刪節點另有解配置成本。
// 上線注意：now 與 expires_at 必須使用一致時基；若手寫 erase_after，刪除後不能誤增前驅 iterator。
// -----------------------------------------------------------------------------
struct Token {
    int id;
    int expires_at;
};

void remove_expired(std::forward_list<Token>& tokens, int now)
{
    tokens.remove_if([now](const Token& token) {
        return token.expires_at <= now;
    });
}

void practical_demo()
{
    std::forward_list<Token> tokens{{1, 100}, {2, 20}, {3, 200}};
    remove_expired(tokens, 50);
    std::vector<int> ids;
    for (const Token& token : tokens) {
        ids.push_back(token.id);
    }
    assert((ids == std::vector<int>{1, 3}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "forward_list：after API、反轉與過期清理測試通過\n";
}

// 【陷阱】沒有 size() 是設計選擇：維持 size 會增加 splice_after 等操作成本/狀態。
// 【陷阱】before_begin() 不可解參考，它只作為第一元素的前驅位置。
// 【面試】為何 erase_after 而非 erase？單向節點無法由目前節點 O(1) 找到前驅。
// 【練習】手寫 erase_after 迴圈移除偶數，和 remove_if 結果比對。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'forward_list.cpp' -o '/tmp/codex_cpp_C_Container_forward_list' && '/tmp/codex_cpp_C_Container_forward_list'
//
// === 預期輸出（節錄）===
// forward_list：after API、反轉與過期清理測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
