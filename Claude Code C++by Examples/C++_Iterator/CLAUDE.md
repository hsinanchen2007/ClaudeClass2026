# C++ Iterator 學習專案

## 使用者需求 / 規則 (Project Requirements)

> 這 5 條是專案永久規則，每次對話都要遵守。

1. **永遠用繁體中文回答** — 不管使用者是用中文還是英文發問，回覆一律用繁體中文。
2. **每個主題加入「簡單實用」的 LeetCode 範例** — 不要那種為了考演算法刻意刁鑽、實務用不到的題目；要選日常開發 / 面試 warm-up 等級的題目，加進對應的 `.cpp` 檔案，且要附**完整詳細的註釋**，把「為什麼這題對應這個 iterator」說清楚，目的是學以致用。
3. **每個程式只談一個主題** — 一個 `.cpp` 檔只聚焦一個 iterator 主題，並在程式碼內附上**詳細完整的課題介紹與概念解釋**。註釋的份量遠超過程式碼是 OK 的、甚至應該如此。
4. **範例要簡單直接** — 不要寫艱澀冗長的 C++、不要為了炫技而寫；範例小、好懂、跑得起來最重要。
5. **參考來源** — 所有 C/C++ 的 API 定義、複雜度、版本資訊以下面兩個網站為主：
   - https://en.cppreference.com/cpp
   - https://cplusplus.com/reference/

   每個 `.cpp` 檔的標頭註解都要列出對應的 cppreference 連結，方便快速查找。

### 附帶限制

- **C++ 版本**：忽略 C++20 與之後新增的 iterator（例如 `contiguous_iterator`、ranges / views、`std::iter_value_t` 等 concepts）。專案聚焦 **C++17 (含) 以前** 的傳統 iterator 體系。
- **流程**：每次新需求進來，先把需求寫進 CLAUDE.md，再開始實作。

## 專案結構

```
C++_Iterator/
├── CLAUDE.md                       # 本檔案
├── 00_overview.cpp                 # iterator 五大類別總覽 + iterator_traits
├── 01_input_iterator.cpp           # InputIterator
├── 02_output_iterator.cpp          # OutputIterator
├── 03_forward_iterator.cpp         # ForwardIterator
├── 04_bidirectional_iterator.cpp   # BidirectionalIterator
├── 05_random_access_iterator.cpp   # RandomAccessIterator
├── 06_reverse_iterator.cpp         # std::reverse_iterator
├── 07_insert_iterators.cpp         # back_inserter / front_inserter / inserter
├── 08_stream_iterators.cpp         # istream_iterator / ostream_iterator
├── 08b_streambuf_iterators.cpp     # istreambuf_iterator / ostreambuf_iterator
├── 09_move_iterator.cpp            # std::move_iterator (C++11) + make_move_iterator
├── 10_iterator_operations.cpp      # advance/distance/next/prev + cbegin/cend +
│                                   # std::size/empty/data + iter_swap/swap_ranges
├── 11_iterator_invalidation.cpp    # 各容器失效規則 + erase-while-iterate 慣用語
├── 12_custom_iterator.cpp          # 從零實作一個 random_access iterator
└── Makefile                        # 一鍵編譯所有範例
```

## 內建知識速查表 (位於 00_overview.cpp 開頭註解)

1. **速查表 1**：操作 (++/--/+n/[n] 等) vs Iterator 類別
2. **速查表 2**：常見演算法 (sort / find / unique...) 需要的最低 Iterator 等級
3. **速查表 3**：容器 (vector / list / set ...) → Iterator 等級對照
4. **速查表 4**：Iterator Adaptor 速查 (含工廠函式 / C++ 版本)

## 各檔案內含的 LeetCode 範例

| 檔案 | LeetCode |
|---|---|
| 01_input_iterator | LC1480 Running Sum / LC136 Single Number |
| 02_output_iterator | LC88 Merge Sorted Array / LC1929 Concatenation of Array |
| 03_forward_iterator | LC141 Linked List Cycle / LC206 Reverse Linked List / LC14 Longest Common Prefix (std::mismatch) |
| 04_bidirectional_iterator | LC125 Valid Palindrome / LC977 Squares of a Sorted Array |
| 05_random_access_iterator | LC704 Binary Search / LC215 Kth Largest / LC35 Search Insert Position |
| 06_reverse_iterator | LC344 Reverse String / LC7 Reverse Integer |
| 07_insert_iterators | LC1002 (set intersect) / LC217 Contains Duplicate / LC349 Intersection of Two Arrays / LC2215 Find Difference of Two Arrays (set_difference) |
| 08_stream_iterators | LC1773-style Token Split / LC412 Fizz Buzz |
| 08b_streambuf_iterators | LC387 First Unique Char / LC771 Jewels and Stones |
| 09_move_iterator | LC1470 Shuffle the Array |
| 10_iterator_operations | LC876 Middle of Linked List / LC26 Remove Duplicates from Sorted Array |
| 11_iterator_invalidation | LC27 Remove Element / LC283 Move Zeroes |
| 12_custom_iterator | LC268 Missing Number (含 std::iterator C++17 deprecated 基底類別歷史對照) |

## 課程知識補充段落 (在各檔案 main() 末段)

每個檔案除了標頭詳註外，main 結尾還埋了「課程知識補充」段落，方便學習：

| 檔案 | 補充重點 |
|---|---|
| 01 | 為什麼 InputIterator 「這麼弱」(設計初衷 = 串流) |
| 02 | `*it = x` 是「整體寫入」、為什麼 OutputIterator 不需 == |
| 03 | multi-pass 的真正意涵 (拷貝後還能重走) |
| 04 | 為什麼 std::sort 對 list/set 不能用 (introsort vs mergesort) |
| 05 | `std::deque` 是 random_access 但不連續、`vector<bool>` 的 proxy 陷阱 |
| 07 | 三種 inserter 的「方向感」差異、set 用 inserter(c, c.end()) 的 hint 用法 |
| 08 | Most Vexing Parse 三種解法 |
| 11 | std::remove / std::unique 的「假象」(不真刪，要搭 erase) |

## 學習路徑建議

```
00 (總覽 + 速查表)
  ↓
01 → 02 → 03 → 04 → 05  (五大類別，由弱到強)
  ↓
06 → 07 → 08 → 08b → 09  (五大 adaptor + 兩種 stream + streambuf + move)
  ↓
10 (iterator 工具函式)
  ↓
11 (失效規則 — 進階實務)
  ↓
12 (自製 iterator — 全面理解)
```

## 編譯方式

```bash
# 全部編譯
make

# 編譯單一檔案
g++ -std=c++17 -Wall -Wextra -O2 01_input_iterator.cpp -o 01_input_iterator
./01_input_iterator
```

## Iterator 類別階層 (由弱到強)

```
InputIterator   ─┐
                 ├─→ ForwardIterator ─→ BidirectionalIterator ─→ RandomAccessIterator
OutputIterator  ─┘
```

- 上層 iterator 都滿足下層的所有需求（強的可以當弱的用）。
- 容器與其 iterator 類別對應：
  | 容器 | Iterator 類別 |
  |------|---------------|
  | `vector`, `array`, `deque` | RandomAccess |
  | `list`, `set`, `map` | Bidirectional |
  | `forward_list`, `unordered_*` | Forward |
  | `istream_iterator` | Input |
  | `ostream_iterator`, `back_inserter` 等 | Output |
