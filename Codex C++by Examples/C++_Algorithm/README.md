# Algorithm：讓程式碼直接表達意圖

標準演算法的優勢不只是少寫 for。<code>find_if</code>、<code>sort</code>、<code>transform</code>、<code>accumulate</code> 把「走訪機制」封裝起來，呼叫端只描述目的與規則。

## 選擇流程

1. 只查是否存在：<code>any_of</code>。
2. 還要位置：<code>find</code>/<code>find_if</code>。
3. 要計數：<code>count_if</code>。
4. 要改每個元素：<code>transform</code>。
5. 要移除符合條件元素：C++20 <code>erase_if</code>，或 erase-remove idiom。
6. 要聚合：<code>accumulate</code>；平行/可重排才考慮 reduce。

## Predicate 與 comparator contract

sort comparator 必須形成 strict weak ordering。不要使用 <code>&lt;=</code>，不要讓比較結果隨呼叫次數改變。演算法可用任意順序、任意次數呼叫 predicate；除非 API 明確保證，避免依賴副作用。

## 複雜度不是唯一成本

sort 是 O(n log n)，binary_search 是 O(log n)，但 binary search 的前置條件是 range 已依相同比較規則排序。若資料只查一次，先排序可能比線性 find 更貴。

## ranges

C++20 ranges 版本可直接接受 range，並支援 projection。Iterator 回傳值仍受來源生命週期約束；對 temporary range 呼叫時要留意 dangling protection 與實際回傳型別。

## 範例索引

本章共 83 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`binary_search/binary_search.cpp`](binary_search/binary_search.cpp)
- [`binary_search/equal_range.cpp`](binary_search/equal_range.cpp)
- [`binary_search/lower_bound.cpp`](binary_search/lower_bound.cpp)
- [`binary_search/summary.cpp`](binary_search/summary.cpp)
- [`binary_search/upper_bound.cpp`](binary_search/upper_bound.cpp)
- [`heap/is_heap.cpp`](heap/is_heap.cpp)
- [`heap/make_heap.cpp`](heap/make_heap.cpp)
- [`heap/pop_heap.cpp`](heap/pop_heap.cpp)
- [`heap/push_heap.cpp`](heap/push_heap.cpp)
- [`heap/sort_heap.cpp`](heap/sort_heap.cpp)
- [`heap/summary.cpp`](heap/summary.cpp)
- [`min_max/clamp.cpp`](min_max/clamp.cpp)
- [`min_max/max.cpp`](min_max/max.cpp)
- [`min_max/max_element.cpp`](min_max/max_element.cpp)
- [`min_max/min.cpp`](min_max/min.cpp)
- [`min_max/min_element.cpp`](min_max/min_element.cpp)
- [`min_max/minmax.cpp`](min_max/minmax.cpp)
- [`min_max/minmax_element.cpp`](min_max/minmax_element.cpp)
- [`min_max/summary.cpp`](min_max/summary.cpp)
- [`modifying/copy.cpp`](modifying/copy.cpp)
- [`modifying/fill.cpp`](modifying/fill.cpp)
- [`modifying/generate.cpp`](modifying/generate.cpp)
- [`modifying/iter_swap.cpp`](modifying/iter_swap.cpp)
- [`modifying/move.cpp`](modifying/move.cpp)
- [`modifying/remove.cpp`](modifying/remove.cpp)
- [`modifying/replace.cpp`](modifying/replace.cpp)
- [`modifying/reverse.cpp`](modifying/reverse.cpp)
- [`modifying/rotate.cpp`](modifying/rotate.cpp)
- [`modifying/sample.cpp`](modifying/sample.cpp)
- [`modifying/shuffle.cpp`](modifying/shuffle.cpp)
- [`modifying/summary.cpp`](modifying/summary.cpp)
- [`modifying/swap.cpp`](modifying/swap.cpp)
- [`modifying/swap_ranges.cpp`](modifying/swap_ranges.cpp)
- [`modifying/transform.cpp`](modifying/transform.cpp)
- [`modifying/unique.cpp`](modifying/unique.cpp)
- [`non_modifying/adjacent_find.cpp`](non_modifying/adjacent_find.cpp)
- [`non_modifying/all_any_none_of.cpp`](non_modifying/all_any_none_of.cpp)
- [`non_modifying/count.cpp`](non_modifying/count.cpp)
- [`non_modifying/equal.cpp`](non_modifying/equal.cpp)
- [`non_modifying/find.cpp`](non_modifying/find.cpp)
- [`non_modifying/find_end.cpp`](non_modifying/find_end.cpp)
- [`non_modifying/find_first_of.cpp`](non_modifying/find_first_of.cpp)
- [`non_modifying/for_each.cpp`](non_modifying/for_each.cpp)
- [`non_modifying/for_each_n.cpp`](non_modifying/for_each_n.cpp)
- [`non_modifying/mismatch.cpp`](non_modifying/mismatch.cpp)
- [`non_modifying/search.cpp`](non_modifying/search.cpp)
- [`non_modifying/search_n.cpp`](non_modifying/search_n.cpp)
- [`non_modifying/summary.cpp`](non_modifying/summary.cpp)
- [`numeric/accumulate.cpp`](numeric/accumulate.cpp)
- [`numeric/adjacent_difference.cpp`](numeric/adjacent_difference.cpp)
- [`numeric/gcd_lcm.cpp`](numeric/gcd_lcm.cpp)
- [`numeric/inner_product.cpp`](numeric/inner_product.cpp)
- [`numeric/iota.cpp`](numeric/iota.cpp)
- [`numeric/partial_sum.cpp`](numeric/partial_sum.cpp)
- [`numeric/reduce.cpp`](numeric/reduce.cpp)
- [`numeric/summary.cpp`](numeric/summary.cpp)
- [`numeric/transform_reduce.cpp`](numeric/transform_reduce.cpp)
- [`partitioning/is_partitioned.cpp`](partitioning/is_partitioned.cpp)
- [`partitioning/partition.cpp`](partitioning/partition.cpp)
- [`partitioning/partition_copy.cpp`](partitioning/partition_copy.cpp)
- [`partitioning/partition_point.cpp`](partitioning/partition_point.cpp)
- [`partitioning/stable_partition.cpp`](partitioning/stable_partition.cpp)
- [`partitioning/summary.cpp`](partitioning/summary.cpp)
- [`permutation/is_permutation.cpp`](permutation/is_permutation.cpp)
- [`permutation/lexicographical_compare.cpp`](permutation/lexicographical_compare.cpp)
- [`permutation/next_permutation.cpp`](permutation/next_permutation.cpp)
- [`permutation/prev_permutation.cpp`](permutation/prev_permutation.cpp)
- [`permutation/summary.cpp`](permutation/summary.cpp)
- [`set_operations/includes.cpp`](set_operations/includes.cpp)
- [`set_operations/inplace_merge.cpp`](set_operations/inplace_merge.cpp)
- [`set_operations/merge.cpp`](set_operations/merge.cpp)
- [`set_operations/set_difference.cpp`](set_operations/set_difference.cpp)
- [`set_operations/set_intersection.cpp`](set_operations/set_intersection.cpp)
- [`set_operations/set_symmetric_difference.cpp`](set_operations/set_symmetric_difference.cpp)
- [`set_operations/set_union.cpp`](set_operations/set_union.cpp)
- [`set_operations/summary.cpp`](set_operations/summary.cpp)
- [`sorting/is_sorted.cpp`](sorting/is_sorted.cpp)
- [`sorting/nth_element.cpp`](sorting/nth_element.cpp)
- [`sorting/partial_sort.cpp`](sorting/partial_sort.cpp)
- [`sorting/partial_sort_copy.cpp`](sorting/partial_sort_copy.cpp)
- [`sorting/sort.cpp`](sorting/sort.cpp)
- [`sorting/stable_sort.cpp`](sorting/stable_sort.cpp)
- [`sorting/summary.cpp`](sorting/summary.cpp)

</details>

## 練習

1. 依姓名再依分數排序，確認 comparator 的等價關係。
2. 把 Celsius vector 轉成 Fahrenheit。
3. 用 prefix sum 回答多個區間總和查詢。
