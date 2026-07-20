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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 88. Merge Sorted Array（合併兩個有序陣列）
// 題目：合併兩個升冪序列；例如 [1,2,3] 與 [2,5,6] 得 [1,2,2,3,5,6]。
// 為何使用本章主題：這是 insert iterator 教學改寫，以 merge+back_inserter 建新結果；原題要求寫回 nums1。
// 思路：預留兩邊總容量；merge 同步比較兩個 range；由 back_inserter 逐項追加。
// 複雜度：時間 O(N+M)、額外空間 O(N+M)，N/M 為兩輸入長度；原題尾端雙指標可用 O(1) 額外空間。
// 易錯點：兩輸入必須已排序；本函式不符合原題原地限制；容量加法需防 size_t 溢位。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】合併新舊資料集的唯一識別碼
// 情境：舊 id [1,3,5] 與新 id [2,3,4] 要合成排序且去除跨集合重複的 [1,2,3,4,5]。
// 為何使用本章主題：set_union 無法事先確定重複數，back_inserter 可安全按實際輸出擴充 vector。
// 設計：提供兩個排序半開區間；set_union 比較並略過共同值的額外份數；結果逐項 push_back。
// 成本：時間 O(N+M)、結果空間 O(U)，U 為聯集大小。
// 上線注意：兩輸入必須依同一 comparator 排序；back_inserter 不會 reserve，大資料應先預留上界以減少配置。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_insert_iterators.cpp' -o '/tmp/codex_cpp_C_Iterator_07_insert_iterators' && '/tmp/codex_cpp_C_Iterator_07_insert_iterators'
//
// === 預期輸出（節錄）===
// [實務] set union grew output without pre-sizing
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
