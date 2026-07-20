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

// ----------------------------------------------------------------------------
// LeetCode 234 風格：Palindrome Linked List
// ----------------------------------------------------------------------------
// 雙向 iterator 可由兩端向中間比較。時間 O(n)、額外空間 O(1)。真正 LeetCode
// ListNode 是單向串列，常用快慢指標反轉後半；本例聚焦 std::list 的雙向能力。
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

// ----------------------------------------------------------------------------
// 實務：容量固定的 LRU cache
// ----------------------------------------------------------------------------
// list 前端代表最近使用；unordered_map 讓 key 平均 O(1) 找到 list 節點。
// get/put 平均 O(1)。iterator 之所以適合放 map，正是 list 節點在 splice 後仍穩定。
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
