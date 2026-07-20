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

// LeetCode 27：Remove Element。erase-remove idiom 將 logical remove 接 physical erase。
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

// 實務：遍歷時刪除過期 session。erase 回傳下一個有效 iterator，不能再 ++ 舊 iterator。
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
