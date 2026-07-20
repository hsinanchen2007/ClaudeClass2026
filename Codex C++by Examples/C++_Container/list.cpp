// ============================================================================
// list：雙向鏈結串列、穩定節點與 O(1) splice
// ============================================================================
// std::list<T> 的每個元素是獨立節點，保存前後指標。已知 iterator 時插入/刪除
// O(1)，但「先找到第 i 個位置」要 O(n)。它不支援 operator[]，記憶體配置多、
// cache locality 差，所以不應只因為 erase 是 O(1) 就取代 vector。
//
// 插入、splice 不使既有元素的 iterator/reference 失效；erase 只使被刪節點失效。
// splice 可在相同型別且 allocator 相容的 list 間轉移節點，不複製/搬移元素，常用於
// LRU cache。整串或單節點 splice 是 O(1)；range splice 在同一 list 內 O(1)，但跨
// list 時需計算轉移元素數，複雜度為 O(distance(first,last))，不可一概說全是 O(1)。
// list::sort 必須使用成員函式，因 std::sort 需要 random-access iterator。

#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

void basic_demo()
{
    std::list<int> values{3, 1, 2};
    values.sort();
    assert((std::vector<int>(values.begin(), values.end()) ==
            std::vector<int>{1, 2, 3}));

    auto two = std::next(values.begin());
    values.insert(two, 10);  // two 仍指向值 2。
    assert(*two == 2);
    values.erase(two);       // 此後 two 不可再使用。
    assert((std::vector<int>(values.begin(), values.end()) ==
            std::vector<int>{1, 10, 3}));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 234. Palindrome Linked List（回文鏈結串列）
// 題目：判斷鏈結串列由前後讀是否相同；例如 [1,2,3,2,1] 為 true，[1,2,3] 為 false。
// 為何使用本章主題：本例是 std::list 雙向 iterator 的教學改寫；原題 ListNode 是單向串列，通常反轉後半。
// 思路：left 從 begin 前進、right 從 rbegin 後退；比較前半數對應元素；任一不同即回 false。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為節點數；list::size 在 C++11 起為 O(1)。
// 易錯點：只比較 N/2 組；空與單節點皆為回文；不可把本解法誤稱為原題單向串列最佳解。
// -----------------------------------------------------------------------------
bool is_palindrome(const std::list<int>& values)
{
    auto left = values.begin();
    auto right = values.rbegin();
    for (std::size_t checked = 0; checked < values.size() / 2U; ++checked) {
        if (*left != *right) {
            return false;
        }
        ++left;
        ++right;
    }
    return true;
}

void leetcode_demo()
{
    assert(is_palindrome({1, 2, 3, 2, 1}));
    assert(!is_palindrome({1, 2, 3}));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定容量的 LRU 快取
// 情境：快取 key/value，命中時提升為最近使用，新增超過容量時淘汰最久未使用項目。
// 為何使用本章主題：list 的節點 iterator 在 splice 後仍穩定，可配 unordered_map 達成平均常數時間定位與重排。
// 設計：list 前端代表最新；map 保存 key 到節點；get/更新時 splice 到前端；超量時刪除尾端與索引。
// 成本：get/put 平均 O(1)，空間 O(C)，C 為容量；hash 最壞情況與配置失敗另計。
// 上線注意：容量不得為零；list 與 map 必須維持一一對應，跨兩容器更新需處理例外回滾與併發同步。
// -----------------------------------------------------------------------------
class LruCache {
public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity)
    {
        assert(capacity_ > 0U);
    }

    int get(int key)
    {
        const auto found = positions_.find(key);
        if (found == positions_.end()) {
            return -1;
        }
        entries_.splice(entries_.begin(), entries_, found->second);
        return found->second->second;
    }

    void put(int key, int value)
    {
        if (const auto found = positions_.find(key); found != positions_.end()) {
            found->second->second = value;
            entries_.splice(entries_.begin(), entries_, found->second);
            return;
        }
        entries_.emplace_front(key, value);
        positions_[key] = entries_.begin();
        if (entries_.size() > capacity_) {
            positions_.erase(entries_.back().first);
            entries_.pop_back();
        }
    }

private:
    using Entry = std::pair<int, int>;
    using Iterator = std::list<Entry>::iterator;
    std::size_t capacity_;
    std::list<Entry> entries_;
    std::unordered_map<int, Iterator> positions_;
};

void practical_demo()
{
    LruCache cache(2U);
    cache.put(1, 10);
    cache.put(2, 20);
    // get 命中會 splice 節點、改變 LRU 次序；先呼叫，避免 NDEBUG 刪掉必要操作。
    [[maybe_unused]] const int one = cache.get(1);  // 1 成為最近使用。
    assert(one == 10);
    cache.put(3, 30);            // 淘汰 2。
    [[maybe_unused]] const int two = cache.get(2);
    [[maybe_unused]] const int three = cache.get(3);
    assert(two == -1);
    assert(three == 30);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "list：穩定 iterator、回文與 LRU 測試通過\n";
}

// 【陷阱】std::distance(list.begin(), it) 是 O(n)，不要藏在熱迴圈中。
// 【陷阱】splice 的 iterator 會繼續指向被搬到新 list 的同一節點，而非失效。
// 【面試】list 與 vector 中間插入各自何時更快？必須把「定位」與 cache 成本算入。
// 【練習】為 LruCache 加 contains，但不可用 operator[] 意外插入 key。

/*
 * 【教科書補充：兩個容器組成一個 invariant】
 * - LRU 的 list 與 index map 必須一一對應；list emplace 成功、map 配置失敗時會留下 orphan node。
 * - production 寫法需 rollback guard，或先完成可失敗步驟再 commit 兩邊 metadata。
 * - splice 可常數時間移動節點且保持元素 reference，但 iterator 的 container ownership 語意會改變。
 * - 例外安全要針對「list+map 整體」說明，不能只引用單一容器的保證。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'list.cpp' -o '/tmp/codex_cpp_C_Container_list' && '/tmp/codex_cpp_C_Container_list'
//
// === 預期輸出（節錄）===
// list：穩定 iterator、回文與 LRU 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
