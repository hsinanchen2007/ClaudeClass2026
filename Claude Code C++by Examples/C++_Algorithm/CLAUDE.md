# C++ STL Algorithm 學習專案

## 專案目標
本專案用於學習 C++ STL `<algorithm>` 標頭檔中提供的演算法函式。學習方式以「閱讀原始碼」為主 — 每一個主要演算法函式皆獨立成一個 C++ 原始碼檔案,內含完整註解、說明與範例,讓使用者可以直接從原始碼中學到所有必要的資訊。

## 使用者需求 (Requirements)

### 語言設定 ⭐ 強制規範
- **永遠用繁體中文回答使用者問題,不論使用者是用中文或英文提問,一律使用繁體中文 (Traditional Chinese) 回覆**。
- 程式碼註解亦一律使用繁體中文,讓學習者直接從原始碼中理解每個函式的用途與細節。

### 參考資料
- C++ STL `<algorithm>` 主要參考:<https://en.cppreference.com/cpp/algorithm>
- C/C++ 全部標準函式庫參考:
  - <https://en.cppreference.com/cpp> (主要、最新、最權威)
  - <https://cplusplus.com/reference/> (輔助參考、範例豐富)
- 必須對照 cppreference 上的函式簽章、複雜度、需求 (requirements) 與行為描述。

### 主題與檔案規範 ⭐ 強制規範
- **每個 .cpp 檔案只能有「一個主題」(一個演算法函式或極相近的多載群)**,不要混雜多個概念。
- 每個主題都必須包含「詳細完整的課題介紹與概念解釋」,寫在程式碼最上方的大段註解區內。
  - 介紹必須涵蓋:該演算法的用途、所屬分類、底層原理、適用情境、與其他相似函式的差異等。
  - **註解的份量遠多過實際程式碼也完全沒關係 — 教學的價值在註解裡**。
- LeetCode 題目選擇原則:
  - **必須簡單、實用、貼近真實開發場景**;不要選那種為了考算法而刁鑽不實用的題目。
  - 例如優先選 LC 1 (Two Sum)、LC 26 (Remove Duplicates)、LC 88 (Merge Sorted Array)、LC 217 (Contains Duplicate) 這類常見且能展現 STL 用法的題目。
  - 避免那種需要複雜資料結構/動態規劃變體/數學技巧的題目,除非 STL 本身就是為了那個情境而設計。
  - 每個 LeetCode 題目範例都要附上完整詳細的繁中註釋,讓學習者能「學以致用」。

### 範例風格規範 ⭐ 強制規範
- **所有程式範例都要簡單、直接、易讀**。
- 不要寫艱澀的 C++ 模板技巧、SFINAE、複雜的 lambda 巢狀,除非主題本身就是介紹那項技巧。
- 不要寫過長的程式碼 — 範例越短越能凸顯該演算法本身的用法。
- 「教學價值來自註解」這條原則優先於「展示語言進階特性」。

### 檔案組織原則
- **每一個主要 (major) 演算法函式 → 一個獨立的 .cpp 檔案**。
- 檔名以該演算法的名稱命名,例如:`for_each.cpp`、`find.cpp`、`sort.cpp`、`transform.cpp` 等。
- 同一群相近的多載 (overloads) 可放在同一個檔案中,以避免過度切割。

### 每個原始碼檔案必須包含
1. **函式簡介** — 該演算法的用途、所屬分類 (Non-modifying / Modifying / Sorting / ...)。
2. **函式簽章** — 包含所有 C++ 標準版本 (C++11 / C++17 / C++20 / C++26) 的差異、`constexpr`、執行策略 (execution policy) 等。
3. **參數說明** — 每個參數的型別與意義。
4. **回傳值** — 回傳什麼、代表什麼意義。
5. **複雜度 (Complexity)** — 時間/空間複雜度。
6. **需求 (Requirements)** — 對迭代器類別 (Iterator category)、可呼叫物件 (Callable)、比較函式等的要求。
7. **基本範例 (Examples)** — 一個簡單但清楚、可直接編譯執行的 `main()` 範例,並有預期輸出。
8. **注意事項 (Pitfalls / Things to pay attention)** — 常見錯誤、未定義行為、與其他類似函式的差異。
9. **LeetCode / 實務範例 (Practical Examples)** — 至少 1~2 個 LeetCode 題與 1 個實務情境,
   範例皆寫成獨立函式 (`leetcode_<num>_<slug>()`、`practical_<slug>()`) 並在 `main()` 末尾呼叫。
   每個 LeetCode 範例註解需包含:題號 + 中英文題名、題目簡述、為何此 STL 適用、
   解法核心、詳細步驟、時間/空間複雜度。

### 程式碼風格
- 使用 C++17 或更新的標準 (除非示範特定版本之差異)。
- `#include` 必要的標頭檔,並在註解中標明。
- 範例必須能用 `g++ -std=c++17 file.cpp -o file && ./file` 直接編譯執行。
- 在程式碼內以 `// ====` 或 `/* */` 區塊清楚分隔「說明」與「範例」兩個部分。

### 教學風格
- 註解必須詳細,適合「直接讀原始碼即可學會」的學習者。
- 對於容易混淆的細節 (例如 `find` vs `find_if`、`remove` 不會真的刪除元素、`unique` 只移除「相鄰」重複元素) 必須特別強調。
- 盡量在範例中加入「典型用法」與「邊界案例」(空容器、找不到的情況等)。

## 專案結構建議
```
C++_Algorithm/
├── CLAUDE.md                    # 本檔案
├── non_modifying/               # 非修改型序列演算法
│   ├── for_each.cpp
│   ├── find.cpp
│   ├── count.cpp
│   └── ...
├── modifying/                   # 修改型序列演算法
│   ├── copy.cpp
│   ├── transform.cpp
│   ├── replace.cpp
│   └── ...
├── partitioning/                # 分割演算法
├── sorting/                     # 排序相關演算法
│   ├── sort.cpp
│   ├── stable_sort.cpp
│   └── ...
├── binary_search/               # 二分搜尋 (需已排序)
├── set_operations/              # 集合操作 (需已排序)
├── heap/                        # heap 操作
├── min_max/                     # 最大最小值
├── permutation/                 # 排列
└── numeric/                     # 數值演算法 (<numeric>)
```

## 工作流程
1. 使用者提出某個演算法 (或一個分類) 的學習需求。
2. Claude 對照 cppreference 撰寫對應的 .cpp 檔案。
3. 若有疑問或建議分類,Claude 主動以繁體中文詢問或說明。

## 完成狀況 (Status)

| 分類 | 檔案數 | 狀態 |
|------|-------|------|
| non_modifying  | 12 | ✓ 完整詳註版 + LeetCode/實務範例 |
| modifying      | 15 | ✓ 完整詳註版 + LeetCode/實務範例 |
| partitioning   |  5 | ✓ 完整詳註版 + LeetCode/實務範例 |
| sorting        |  6 | ✓ 完整詳註版 + LeetCode/實務範例 |
| binary_search  |  4 | ✓ 完整詳註版 + LeetCode/實務範例 |
| set_operations |  7 | ✓ 完整詳註版 + LeetCode/實務範例 |
| heap           |  5 | ✓ 完整詳註版 + LeetCode/實務範例 |
| min_max        |  7 | ✓ 完整詳註版 + LeetCode/實務範例 |
| permutation    |  4 | ✓ 完整詳註版 + LeetCode/實務範例 |
| numeric        |  8 | ✓ 完整詳註版 + LeetCode/實務範例 (含 gcd/lcm) |
| **總計**       | **73** | **全數通過 g++ -std=c++17 編譯與執行** |

每個 .cpp 檔皆包含: 簡介、函式簽章 (含 C++11/14/17/20 差異)、參數說明、回傳值、
複雜度、需求 (Requirements)、基本範例、注意事項 (Pitfalls)、
**LeetCode / 實務範例 (Practical Examples)**、預期輸出。

### LeetCode / 實務範例 一覽 (節選)

| 演算法 | LeetCode 題目 | 實務情境 |
|--------|---------------|----------|
| `find`              | LC 1 Two Sum                                 | 依 ID 在容器中搜尋使用者 |
| `adjacent_find`     | LC 217 Contains Duplicate (排序後)            | 偵測連續重複登入 |
| `mismatch`          | LC 14 Longest Common Prefix                  | 兩份設定檔的差異點 |
| `equal`             | LC 242 Valid Anagram                         | 校驗兩 buffer 內容 |
| `search`            | LC 28 Find First Occurrence (strStr)         | 在 log 中找錯誤序列 |
| `search_n`          | LC 485 Max Consecutive Ones (變體)           | 偵測連續 N 次錯誤 |
| `count` / `count_if`| LC 387 First Unique Char, LC 1365             | 計票系統 |
| `all/any/none_of`   | LC 1346 N and Its Double, LC 941 Mountain    | 表單驗證 |
| `for_each` / `_n`   | LC 1672 Richest Customer Wealth, LC 1480     | log 處理 / 取前 N 筆訂單 |
| `transform`         | LC 1672, LC 1365                             | 雙感測器讀數平均 |
| `unique`            | LC 26 / LC 80 Remove Duplicates              | 壓縮 log 連續重複 |
| `remove`            | LC 27 Remove Element / LC 283 Move Zeroes    | 過濾無效訂單 |
| `replace`           | LC 1351 變化版                               | 字串敏感字元替換 |
| `reverse`           | LC 344 Reverse String, LC 189 Rotate Array   | 事件清單反序顯示 |
| `rotate`            | LC 189 Rotate Array                          | VIP 任務插入隊頭 |
| `partition`         | LC 905 Sort Array By Parity, LC 75 Sort Colors | 錯誤 vs 正常 log 分區 |
| `stable_partition`  | LC 75 (穩定版), LC 2161                      | VIP 用戶前置且保留註冊順序 |
| `partition_point`   | LC 278 First Bad Version, LC 852 Mountain    | 系統狀態轉換點 |
| `sort`              | LC 56 Merge Intervals, LC 252, LC 179        | 事件 timeline 排序 |
| `partial_sort`      | LC 215 Kth Largest, LC 347 Top K Frequent    | 遊戲排行榜前 N 名 |
| `nth_element`       | LC 215 (O(N) 平均), LC 973 K Closest Points  | 中位數計算 |
| `is_sorted`         | LC 941 Valid Mountain, LC 1827               | 時間戳完整性檢查 |
| `binary_search`     | LC 704 Binary Search                         | 排序設定檔關鍵字驗證 |
| `lower_bound`       | LC 35 Search Insert, LC 300 LIS, LC 34       | 維持插入排序的插入點 |
| `upper_bound`       | LC 34 Find Range, LC 875 二分答案概念         | 統計區間內元素個數 |
| `equal_range`       | LC 34 (一次拿 first/last), LC 350             | 依時戳查詢區間事件 log |
| `merge` / `inplace_merge` | LC 88 Merge Sorted Array               | 合併兩份已排序 log |
| `set_intersection`  | LC 349 / LC 350                              | 找兩名單共同好友 |
| `set_difference`    | LC 2215                                      | 權限差異 (新增/移除) |
| `set_union`         | LC 2215 概念延伸                             | 合併設定鍵清單去重 |
| `set_symmetric_difference` | LC 2215                               | 報名/出席異常名單 |
| `includes`          | 子集判斷                                     | RBAC 權限子集檢查 |
| `make/push/pop_heap`| LC 215 Kth Largest, LC 1046 Last Stone, LC 295 Median | priority queue 任務佇列 |
| `sort_heap`         | LC 912 Sort an Array (heapsort)              | 任務佇列最終排序輸出 |
| `min/max_element`   | LC 1431 Kids With Greatest Candies, LC 1051  | 找最高分玩家 / 最差成績位置 |
| `minmax_element`    | LC 414 Third Maximum, LC 561                 | 一次取統計 max/min |
| `clamp`             | LC 2419 概念                                 | 字級 / HP / MP 範圍限制 |
| `next_permutation`  | LC 31 Next Permutation, LC 46 Permutations   | 字典序枚舉 ID 組合 |
| `is_permutation`    | LC 242, LC 567                               | server/client 集合一致性 |
| `lexicographical_compare` | LC 179 Largest Number, LC 1370          | 版本字串比較 |
| `accumulate`        | LC 1672, LC 1431                             | 訂單總金額 |
| `partial_sum` (含 inclusive_scan / exclusive_scan) | LC 1480 Running Sum, LC 303 Range Sum | 累計營收 / 平行寫入 offset |
| `adjacent_difference` | LC 121 Buy/Sell Stock, LC 1991              | 累積病例 → 每日新增 |
| `inner_product`     | LC 1313 Decompress RLE, LC 1773              | 加權平均 (學分制) |
| `iota`              | LC 287 Find Duplicate, argsort 模式           | 連續 ID 生成 |
| `transform_reduce` (含 transform_inclusive_scan / transform_exclusive_scan) | LC 1672 優雅版, LC 1480 變化 | 購物車總額 / 字串拼接 offset |
| `gcd` / `lcm`       | LC 1071 GCD of Strings, LC 914 X of a Kind   | 分數約分 / 螢幕比例 |

### 一次驗證所有檔案
```bash
# 從本檔所在位置推導根目錄，不要硬編絕對路徑（原版寫死 /home/hsinan/C++_Algorithm，
# 那個路徑並不存在）。find -print0 + read -d '' 才能正確處理含空白/換行的檔名。
cd "$(dirname "${BASH_SOURCE[0]:-$0}")" || exit 1
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
find . -name "*.cpp" -print0 |
while IFS= read -r -d '' f; do
  if g++ -std=c++17 -Wall "$f" -o "$out/_t" 2>/dev/null && "$out/_t" >/dev/null 2>&1; then
    echo "OK:   $f"
  else
    echo "FAIL: $f"
  fi
done
```

### LSP/clangd 警告說明
專案中可能會看到 clangd 報「'algorithm' file not found」與 std 識別子未宣告等錯誤,
這是 clangd 找不到系統 include 路徑造成的「假陽性」 — 實際以 g++ 編譯皆通過。
若要消除,可在專案根目錄建立 `compile_commands.json` 或 `.clangd` 設定 include 路徑。
