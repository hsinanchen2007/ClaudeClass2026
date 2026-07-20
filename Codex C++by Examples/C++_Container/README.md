# Container：先由操作需求選型

容器選擇不是背「vector 快、list 慢」。你要先問：資料是否連續、主要查找方式、是否要求排序、插入位置、iterator/reference 必須穩定多久。

## 常用預設

### vector

一般情況的第一選擇。連續記憶體帶來 cache locality、可與 C API/Span 互通，也支援 O(1) random access。尾端加入 amortized O(1)，中間插刪 O(n)。擴容會讓全部 pointer/reference/iterator 失效。

### deque 與 list

deque 適合兩端頻繁加入移除，不保證整體連續；中間操作仍昂貴。list 只有在你已持有位置、需要 O(1) splice/erase，且穩定 iterator 的價值高於配置與 cache 成本時才合理。不要為了「插入 O(1)」忽略先找位置的 O(n)。

### map 與 unordered_map

map 維持 key 排序，查找 O(log n)，可做 lower_bound/range query。unordered_map 平均 O(1)，最壞 O(n)，iteration order 不穩定；rehash 使 iterator 失效，但元素 reference/pointer 仍有效。

## 失效規則是 API contract

修改容器前要知道：

- vector reallocation：全部失效。
- vector erase：被刪位置及其後方失效。
- list erase：只有被刪元素失效。
- unordered container rehash：iterator 失效。

不能靠「地址這次剛好沒變」判定有效；失效是語意，不是觀察結果。

## 範例索引

本章共 18 個可獨立編譯執行的 `.cpp`；`summary.cpp` 是面試前速查。

<details>
<summary>展開全部檔案</summary>

- [`array.cpp`](array.cpp)
- [`deque.cpp`](deque.cpp)
- [`forward_list.cpp`](forward_list.cpp)
- [`list.cpp`](list.cpp)
- [`map.cpp`](map.cpp)
- [`multimap.cpp`](multimap.cpp)
- [`multiset.cpp`](multiset.cpp)
- [`priority_queue.cpp`](priority_queue.cpp)
- [`queue.cpp`](queue.cpp)
- [`set.cpp`](set.cpp)
- [`span.cpp`](span.cpp)
- [`stack.cpp`](stack.cpp)
- [`summary.cpp`](summary.cpp)
- [`unordered_map.cpp`](unordered_map.cpp)
- [`unordered_multimap.cpp`](unordered_multimap.cpp)
- [`unordered_multiset.cpp`](unordered_multiset.cpp)
- [`unordered_set.cpp`](unordered_set.cpp)
- [`vector.cpp`](vector.cpp)

</details>

## 練習

1. 為工作佇列比較 vector、deque 的前端刪除成本。
2. 用 map 的 lower_bound 找第一個不小於指定時間的事件。
3. 在 vector 插入後重新取得 iterator，而不是保存舊 iterator。
