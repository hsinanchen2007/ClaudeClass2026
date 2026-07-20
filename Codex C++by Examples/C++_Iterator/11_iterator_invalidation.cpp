// ============================================================================
// 課題 11：Iterator invalidation - 容器修改後哪些位置仍可用？
// ============================================================================
//
// 這是 STL 最常造成 production UB 的主題。摘要規則：
//   vector：reallocation 使所有 iterator/reference/pointer 失效；未 reallocate 的
//           insert/erase 仍讓修改點及其後方失效。
//   deque ：規則較細，不應憑直覺；插入常使 iterator 失效，reference 規則另看 API。
//   list/forward_list：只有指向被 erase 元素的位置失效；其他通常穩定。
//   map/set：同上；erase 只使被刪 node 失效。
//   unordered_*：rehash 使 iterator 失效，但既有 element reference/pointer 仍有效；
//                erase 只使被刪元素失效。
//
// 永遠查該 operation 的 invalidation contract。不要靠「這次 capacity 似乎夠」。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

void basic_example()
{
    std::vector<int> values{1, 2, 3};
    values.reserve(8);
    const int* first_address = &values[0];
    values.push_back(4); // capacity 足夠，既有 references/iterators 在此仍有效。
    assert(first_address == &values[0]);

    // 若下一次 push 可能 reallocate，就不可保存 iterator 後再使用；保存 index/key，
    // 修改後重新取得位置是較耐維護的寫法。
    const std::size_t index = 1;
    values.reserve(32);
    assert(values[index] == 2);
    std::cout << "[基礎] stable index was reacquired after vector growth\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element（移除元素）
// 題目：原地移除所有 target 並回傳新長度，前 k 項是保留值；例如 [3,2,2,3] 移除 3 後 k=2。
// 為何使用本章主題：remove 回 logical end，erase 再實際縮短 vector；兩步也展示修改後 iterator 的失效邊界。
// 思路：std::remove 將非 target 穩定搬到前方；接住 new_end；erase [new_end,end)；回傳 size。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為元素數。
// 易錯點：remove 本身不改 size；erase 後 new_end 與後方 iterator 均失效；回傳 int 前應驗證長度範圍。
// -----------------------------------------------------------------------------
int remove_element(std::vector<int>& values, int target)
{
    const auto new_end = std::remove(values.begin(), values.end(), target);
    values.erase(new_end, values.end());
    return static_cast<int>(values.size());
}

void leetcode_27_example()
{
    std::vector<int> values{3, 2, 2, 3};
    assert(remove_element(values, 3) == 2);
    assert((values == std::vector<int>{2, 2}));
    std::cout << "[LeetCode 27] remove followed by erase removed the tail\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】走訪時清理過期 session
// 情境：session 表保存剩餘 TTL；已為 0 的項目要刪除，其餘項目每輪遞減一次。
// 為何使用本章主題：unordered_map::erase(iterator) 回傳下一個有效位置，可避免刪除後遞增 stale iterator。
// 設計：以不自動 ++ 的迴圈走訪；過期時接住 erase 回傳值；存活時減 TTL 並前進。
// 成本：平均時間 O(S)、最壞 O(S^2)，額外空間 O(1)，S 為 session 數。
// 上線注意：TTL 單位與下界要定義；同時插入造成 rehash 會使 iterator 失效；共享表需要鎖或單執行緒 ownership。
// -----------------------------------------------------------------------------
void remove_expired(std::unordered_map<std::string, int>& ttl)
{
    for (auto it = ttl.begin(); it != ttl.end();) {
        if (it->second <= 0) {
            it = ttl.erase(it);
        } else {
            --it->second;
            ++it;
        }
    }
}

void practical_example()
{
    std::unordered_map<std::string, int> sessions{{"alive", 2}, {"expired", 0}};
    remove_expired(sessions);
    assert(sessions.size() == 1 && sessions.at("alive") == 1);
    std::cout << "[實務] erase-return loop safely removed expired sessions\n";
}

int main()
{
    basic_example();
    leetcode_27_example();
    practical_example();
}

// 易錯：reserve 只保證到該 capacity 前不 reallocate，不代表 erase/insert 後方位置穩定。
// 面試自問：unordered_map::rehash 後 iterator 與 reference 哪一個失效？
// 練習：比較 vector、list、map 的 erase loop 寫法與複雜度。

/*
 * 【教科書補充：失效必須分四種 handle】
 * - iterator、reference、pointer 與 past-the-end 不一定同時失效；例如 unordered rehash 使 iterator 失效，元素 reference/pointer 仍有效。
 * - vector 未重配 insert/erase 仍影響操作點後方與舊 end；deque 中間操作通常比端點操作破壞更多 handle。
 * - list splice/insert 通常保留元素 handle，erase 只使被刪節點失效；但 iterator 可能改屬另一 container。
 * - 測試不得比較、解參考或遞增已失效 iterator；一旦失效，連「看看是否還能用」都可能是 UB。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_iterator_invalidation.cpp' -o '/tmp/codex_cpp_C_Iterator_11_iterator_invalidation' && '/tmp/codex_cpp_C_Iterator_11_iterator_invalidation'
//
// === 預期輸出（節錄）===
// [實務] erase-return loop safely removed expired sessions
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
