# CLAUDE.md — 專案需求說明

本檔案記錄此專案的協作規範與需求,Claude 在每次回覆前都應遵循。

## 0. 核心五大原則 (★ 2026-05-04 新增,優先級最高)

以下五點為本專案最高優先級規範,凌駕於後續所有條款之上:

1. **永遠用繁體中文回答** — 不論使用者用中文還是英文提問,所有對話與回覆一律使用繁體中文 (Traditional Chinese)。
2. **每個主題搭配「簡單實用」的 LeetCode 題目** — 加到對應的 C++ 程式碼裡學以致用。**避開為了考算法而刁鑽不實用的題目**;優先選擇真實工程場景中常用的題型。每個 LeetCode 範例都要附上完整詳細的中文註釋,包含題目簡述、思路、複雜度。
3. **每個程式只聚焦一個主題** — 每個 `.cpp` 檔案只處理一個 container 主題,並提供詳細完整的「課題介紹 + 概念解釋」放在程式碼裡的註釋中。註釋的篇幅可以遠多於程式碼本身,目標是讓讀者單看 source code 就能完整學會該 container 的所有觀念。
4. **程式範例必須簡單直接** — 不要寫艱澀難懂的 C++ template metaprogramming 或冗長的範例。範例應一目了然,讓初學者也能跟上;若要展示進階用法,先用最小可讀範例帶過,再用註釋補充進階說明。
5. **參考來源** — 撰寫時請以下列兩個權威 C/C++ 參考網站為主要依據:
   - <https://en.cppreference.com/w/cpp> (cppreference,標準最權威)
   - <https://cplusplus.com/reference/> (cplusplus.com,範例淺顯易懂)
   兩者擇一或併用,確保 API 行為、複雜度、例外保證等資訊正確。

## 1. 語言要求

- **所有對話與回覆一律使用繁體中文 (Traditional Chinese)**。
- 程式碼中的註解 (comments) 也使用繁體中文,除非是專有名詞 / 標準術語 (如 `iterator`, `noexcept`, `O(1)` 等) 才保留英文。

## 2. 學習目標

本專案為 **C++ STL Container 學習筆記**,目標是讓使用者「直接看 source code 就能完整學會某個 container」。

- 參考標準: <https://en.cppreference.com/cpp/container>
- 每個 STL container 對應 **一個獨立的 `.cpp` 檔案** (例如 `vector.cpp`, `list.cpp`, `map.cpp`...)。
- 每個檔案內必須包含該 container 的 **所有 member functions**,以可執行的 demo 形式呈現。

## 3. 每個檔案應包含的內容

每個 container 的 `.cpp` 檔案請依下列結構撰寫,讓使用者可以直接從原始碼中學習:

1. **檔頭區塊 (file header comment)**
   - container 的概述、底層資料結構 (e.g. dynamic array、doubly linked list、red-black tree、hash table)。
   - 標頭檔 (`#include <...>`)。
   - 所屬類別 (sequence / associative / unordered associative / container adaptor)。
   - 時間複雜度總表 (insert / erase / access / search 的 Big-O)。
   - 與其他相似 container 的比較 (如 `vector` vs `deque` vs `list`)。
   - 適用情境 / 不適用情境。

2. **每一個 member function 一段 demo**
   - 以小段函式或 `// ──── functionName ────` 區塊分隔。
   - 註解說明:
     - 此 function 的用途。
     - 參數、回傳值、複雜度。
     - 是否會 invalidate iterators / references / pointers。
     - 例外保證 (no-throw / strong / basic)。
   - 緊接著一段最小可執行的範例 (含預期輸出)。

3. **Pitfalls / 易錯區段**
   - 容易踩到的雷 (如 `vector::push_back` 後 iterator 失效、`map::operator[]` 會插入 default 元素、`unordered_map` rehash 條件等)。
   - 與其他語言 / container 容易混淆的點。

4. **Best practice / 使用建議**
   - 何時該用、何時不該用。
   - C++11 / 14 / 17 / 20 / 23 後新增的便利 API (如 `emplace`, `try_emplace`, `contains`, `extract`, `merge`)。

5. **LeetCode 實戰範例 (★ 2026-05-04 補強)**
   - 在 best practice 之後另開一節 (`// LeetCode 實戰範例`),示範該 container 在常見演算法題中的真實用法。
   - 選題以「常見 / 經典 / 出題率高」為主,避免冷僻題。每題標註 LeetCode 題號與題名 (如 LC 1: Two Sum),並用註解說明核心思路與時間複雜度。
   - 範例必須能直接編譯執行,並標註 `// 預期輸出: ...`。

6. **`main()` 內含完整可編譯的範例**
   - 使用 `// 預期輸出: ...` 標註執行結果。
   - 編譯指令寫在檔頭註解 (例如 `// g++ -std=c++23 -Wall -Wextra vector.cpp -o vector && ./vector`)。

## 4. 涵蓋的 Container 清單

依 cppreference 分類,以下為已完成項目 (2026-05-03 一次完成 16 個;2026-05-04 全部加入 LeetCode 實戰範例)。

每個檔案最末段都新增了 `// LeetCode 實戰範例` 章節,涵蓋該 container 在面試/競賽中最常見的題型。所列題目均經過 `make run` 驗證可編譯執行。

### Sequence containers
- [x] `array.cpp` — `std::array` (C++11) — LC 242 Anagram / LC 387 First Uniq Char / 9x9 Board
- [x] `vector.cpp` — `std::vector` — LC 121 Stock / LC 283 Move Zeroes / LC 26 Unique / LC 167 Two Sum II
- [x] `deque.cpp` — `std::deque` — LC 239 Sliding Window Max (單調隊列) / LC 933 RecentCounter
- [x] `forward_list.cpp` — `std::forward_list` (C++11) — LC 206 Reverse / LC 876 Middle / LC 83 Dedup
- [x] `list.cpp` — `std::list` — LC 146 LRU Cache (list + unordered_map) / splice 搬移範例

### Associative containers (red-black tree, ordered)
- [x] `set.cpp` — `std::set` — LC 220 ContainsNearbyAlmostDup / LC 729 MyCalendar I
- [x] `multiset.cpp` — `std::multiset` — Sliding Window Max (multiset 版) / LC 480 Sliding Median
- [x] `map.cpp` — `std::map` — LC 981 TimeMap / map 自動排序走訪 / 計次分析
- [x] `multimap.cpp` — `std::multimap` — 學生成績分組 (equal_range) / 行事曆事件

### Unordered associative containers (hash table)
- [x] `unordered_set.cpp` — `std::unordered_set` — LC 217 ContainsDup / LC 128 LongestConsec / LC 202 Happy Number
- [x] `unordered_multiset.cpp` — `std::unordered_multiset` — LC 1207 UniqueOccur / Anagram
- [x] `unordered_map.cpp` — `std::unordered_map` — LC 1 Two Sum / LC 49 GroupAnagrams / LC 560 SubarraySumK / LC 387 FirstUniq
- [x] `unordered_multimap.cpp` — `std::unordered_multimap` — 電話簿 / 字母分組

### Container adaptors
- [x] `stack.cpp` — `std::stack` — LC 20 Valid Parentheses (原有) / LC 739 DailyTemperatures (單調棧) / LC 150 RPN
- [x] `queue.cpp` — `std::queue` — LC 200 NumIslands (BFS) / LC 933 RecentCounter
- [x] `priority_queue.cpp` — `std::priority_queue` — LC 215 KthLargest / LC 347 TopKFrequent / LC 1046 LastStoneWeight

### Views / span (C++20+)
- [x] `span.cpp` — `std::span` (C++20) — 通用 max / subspan 切片 / 滑動視窗 view
- [ ] `mdspan.cpp` — `std::mdspan` (C++23,尚未做 — libstdc++ 13/14 才支援,且常見編譯器 default 不啟用)

### Flat containers (C++23,選做,尚未做)
- [ ] `flat_set.cpp`, `flat_multiset.cpp`, `flat_map.cpp`, `flat_multimap.cpp`
  - 註:這些需要 GCC 15+ 或 Clang 19+ 才能編譯,目前環境 (g++ 預設) 可能不支援。
        待升級編譯器或加上 libc++/-stdlib=libc++ 後再補。

## 4.1 編譯所有檔案

附了 `Makefile`,在專案根目錄執行:

```bash
make            # 編譯所有 .cpp
make run        # 編譯並執行所有 demo
make clean      # 清除執行檔
make <name>     # 只編譯某一支 (e.g. make vector)
```

編譯指令統一為 `g++ -std=c++20 -Wall -Wextra`。

## 4.2 clangd LSP 紅線說明

如果在 VSCode / Neovim 中看到 clangd 的紅線錯誤 (`'vector' file not found` 之類),
那是 LSP 找不到系統 header 的 include path,**並非真正的編譯錯誤** —
g++ 編譯與執行皆正常。要消除紅線可在專案根目錄放置 `.clangd` 設定檔指定:

```yaml
CompileFlags:
  Add: [-std=c++20, -isystem, /usr/include/c++/13, -isystem, /usr/include]
```
(實際路徑因系統而異,可用 `g++ -E -x c++ - -v < /dev/null` 查詢)

## 5. 風格與品質要求

- 使用 **C++17 以上** 為基準語法 (除非該 container 是 C++20/23 才有,則註明)。
- 每個範例都要能 **獨立編譯通過**,不要依賴外部資料。
- 註解要解釋 **「為什麼這樣寫」** 而不只是「在做什麼」,尤其是 iterator 失效規則、複雜度差異這類非從程式碼直接看得出來的資訊。
- 範例輸出用 `std::cout` 列印,並在註解中標註 `// 預期輸出:`。
- **範例程式要簡單直接** — 避免炫技式寫法 (深度 template、SFINAE、複雜 lambda 巢狀)。
- **註釋優先於程式碼** — 寧可程式碼少一點,註釋多寫一點;讀者來這裡是學概念的。

## 6. LeetCode 題目選題標準 (★ 2026-05-04 新增)

加入 LeetCode 範例時,請依下列原則選題:

- **優先**: 「真實工程常見」的題型 — Hash 計數、雙指針、滑動視窗、BFS/DFS、單調棧/隊列、TopK、LRU、區間合併等。
- **避開**: 為了刁難而刁難的演算法題 (極端 DP 狀態壓縮、卡常數的位元魔術、需要冷僻數學定理的題目)。
- **每題註釋**至少包含: LC 題號 / 題名 / 一句話題目簡述 / 核心思路 / 時間複雜度 / 為何用此 container 解最自然。
- **範例輸入要小**,讓讀者能在腦中跟著模擬一次。
