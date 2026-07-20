// ============================================================================
// 課題 7：back/front/inserter adapters
// ============================================================================
//
// back_inserter 呼叫 push_back（vector/deque/list），front_inserter 呼叫 push_front（會反轉
// 逐項輸出順序），inserter(container,pos) 呼叫 insert 並更新內部位置。它們把需要 grow
// 的 container 包成 OutputIterator，避免先 resize 後寫入。
//
// associative container 可用 inserter；duplicate key 是否插入由 container 規則決定。
// vector back insertion 可能 reallocate，使外部 iterators/references 失效。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <vector>

void basic_example()
{
    const std::vector<int> source{1, 2, 3};
    std::vector<int> destination;
    std::copy(source.begin(), source.end(), std::back_inserter(destination));
    assert(destination == source);
    std::set<int> unique;
    std::copy(source.begin(), source.end(), std::inserter(unique, unique.end()));
    assert(unique.size() == 3U);
    std::cout << "[基礎] insert adapters grow sequence/associative containers\n";
}

// LeetCode 88：Merge Sorted Array。以 std::merge + back_inserter 寫清楚版本；題目正式要求
// in-place nums1，面試可再用尾端雙指標達 O(1) extra space。
std::vector<int> merge_sorted(const std::vector<int>& left, const std::vector<int>& right)
{
    std::vector<int> result;
    result.reserve(left.size() + right.size());
    std::merge(left.begin(), left.end(), right.begin(), right.end(), std::back_inserter(result));
    return result;
}

void leetcode_88_example()
{
    assert((merge_sorted({1, 2, 3}, {2, 5, 6}) == std::vector<int>{1, 2, 2, 3, 5, 6}));
    std::cout << "[LeetCode 88] merge writes six sorted values via back inserter\n";
}

// 實務：set_union output 大小未知，用 back_inserter。
void practical_example()
{
    const std::vector<int> old_ids{1, 3, 5};
    const std::vector<int> new_ids{2, 3, 4};
    std::vector<int> all;
    std::set_union(old_ids.begin(), old_ids.end(), new_ids.begin(), new_ids.end(),
                   std::back_inserter(all));
    assert((all == std::vector<int>{1, 2, 3, 4, 5}));
    std::cout << "[實務] set union grew output without pre-sizing\n";
}

int main() { basic_example(); leetcode_88_example(); practical_example(); }

// 易錯與面試：front_inserter 逐項 push_front 會反轉順序；back_inserter 不會替你 reserve，
// 大量輸出已知上界時先 reserve 可減少配置。set 的重複值則會由 container 規則丟棄。
//
// `inserter(container, hint)` 保存並更新 insertion position；對 sequence container 可在中間
// 連續插入，對 associative container 則由 key ordering 決定實際位置。adapter 自身不擁有
// container，container 必須活到 algorithm 結束。面試也要分清 resize（建立元素）與
// reserve（只配置容量）：直接寫 begin 前需要 resize，back_inserter 前通常只需 reserve。
// 若 algorithm 的 input 與 output 是同一 vector，成長造成的 reallocation 可能使 input
// iterators 失效；除非 API 明確支援 overlap，否則先用獨立 output 或預先設計容量與索引。
// 練習：用 front_inserter(deque) copy 1,2,3，預測並驗證結果順序。
