# CLAUDE.md

本檔案為 Claude Code 在此專案中工作時的指導原則。請務必遵守以下五點規範。

---

## 專案目標

本專案的目的是學習 C++ 標準函式庫中的「通用工具(General-purpose utilities)」,
參考來源:
- https://en.cppreference.com/cpp/utility#General-purpose_utilities
- https://en.cppreference.com/cpp
- https://cplusplus.com/reference/

每個主題搭配一個簡單實用的 Leetcode 題目,做到「學以致用」。

---

## 五點規範(Claude 必須遵守)

### 1. 語言規範

- **永遠用繁體中文回答問題**,不管使用者是用中文還是英文提問。
- 程式碼註釋也一律使用繁體中文。

### 2. 結合 Leetcode 實作練習

- 每個 C++ utility 主題,都要參考一題**對應的 Leetcode 題目**。
- 題目選擇原則:
  - **簡單實用**(Easy 或 Medium 入門級)。
  - 能自然套用該 utility 的場景。
  - **不選**那種為了考演算法而刁鑽不實用的題目。
- 將 Leetcode 解題程式碼**加到現有的 C++ 程式碼裡**,讓學習能「學以致用」。
- 解題過程要附上**完整詳細的繁體中文註釋**,說明:
  - 題目大意
  - 解題思路
  - 為什麼用這個 utility
  - 每一步在做什麼

### 3. 一個程式一個主題

- **每個 .cpp 檔案只專注於一個主題**(例如 `std::pair`、`std::tuple`、`std::optional` 等)。
- 每個主題都必須包含:
  - **完整詳細的課題介紹**(這個 utility 是什麼、為什麼存在、解決什麼問題)。
  - **觀念解釋**(語法、用法、注意事項、常見陷阱)。
  - **與其他 utility 的比較**(如果有相關的)。
- 註釋要**遠多於程式碼**也沒關係,寧可冗長詳細,也不要簡略。
- 註釋直接寫在程式碼檔案裡面(不另外開 markdown 文件)。

### 4. 範例簡單直接

- 所有 C++ 程式範例都要**簡單直接**。
- **不要**寫艱澀的 C++(避免過度使用模板黑魔法、SFINAE、複雜繼承等)。
- **不要**寫很長的程式碼(每個範例聚焦在一個概念上)。
- 寧可分多個小範例,也不要寫一個又長又複雜的範例。

### 5. 參考來源

所有 C/C++ 的參考一律以下列來源為準:
- https://en.cppreference.com/cpp(主要)
- https://cplusplus.com/reference/(輔助)

如果兩者有差異,以 cppreference 為準(它較貼近現代 C++ 標準)。

### 6. C++ 版本範圍(補充)

- **只涵蓋 C++17 及更早的標準**(C++98 / C++11 / C++14 / C++17)。
- **跳過 C++20 之後**才引入的 utility(例如 `std::cmp_equal`、`std::to_underlying`、
  `std::in_range` 等整數比較工具,以及 C++23 的 `std::unreachable` 等)。
- 編譯時請使用 `-std=c++17` 旗標。

### 7. 範例多樣性(補充)

- 除了 **Leetcode 題目** 之外,**每個主題還要再加一個「日常工作實用範例」**。
- 日常工作範例的選擇方向(取一即可):
  - 設定檔解析 / 環境變數讀取
  - 快取查詢(找得到 vs 找不到)
  - 多回傳值(狀態碼 + 資料)
  - 錯誤處理(成功 vs 失敗)
  - 資料轉換 / 結構封裝
  - 函式參數打包 / 解包
- 實用範例**不要**抽象,要貼近真實開發場景(例如「使用者登入」「商品庫存查詢」)。

---

## 檔案結構建議

```
C++_Utility/
├── CLAUDE.md                    # 本檔案
├── 01_pair.cpp                  # std::pair
├── 02_tuple.cpp                 # std::tuple
├── 03_optional.cpp              # std::optional (C++17)
├── 04_variant.cpp               # std::variant (C++17)
├── 05_any.cpp                   # std::any (C++17)
├── 06_swap.cpp                  # std::swap
├── 07_move.cpp                  # std::move
├── 08_forward.cpp               # std::forward
├── 09_exchange.cpp              # std::exchange
├── 10_as_const.cpp              # std::as_const
└── ... (依 cppreference 的 utility 分類擴充)
```

每個檔案皆為**獨立可編譯的單一主題教材**,可用以下指令編譯執行:

```bash
g++ -std=c++17 -Wall -Wextra 01_pair.cpp -o 01_pair && ./01_pair
```

---

## 註釋風格範本

每個 .cpp 檔案應依下列結構撰寫(全繁體中文):

```cpp
/*
================================================================================
主題:std::pair —— 配對(Pair)
參考:https://en.cppreference.com/w/cpp/utility/pair
================================================================================

【一、課題介紹】
  std::pair 是 C++ 標準函式庫中最基本的「組合型別」,可以把兩個不同型別的
  值打包成一個物件 ...(此處詳細介紹)

【二、觀念解釋】
  1. 標頭檔:<utility>
  2. 語法:std::pair<T1, T2>
  3. 成員:.first / .second
  4. 建立方式:
     - 直接建構 std::pair<int, std::string> p(1, "hi");
     - std::make_pair(1, "hi");
     - C++17 起可用 CTAD:std::pair p(1, "hi");
  ...(此處詳細解釋)

【三、常見陷阱】
  ...

【四、與其他 utility 的比較】
  - vs std::tuple:tuple 可裝 N 個元素,pair 固定 2 個
  - vs struct:struct 可命名欄位,可讀性更高
  ...

【五、Leetcode 對應題目】
  題號:1. Two Sum(兩數之和)
  難度:Easy
  連結:https://leetcode.com/problems/two-sum/
  選用理由:回傳兩個索引,正好用 std::pair 自然表達。

【六、日常工作實用範例】
  情境:解析 "key=value" 設定檔片段,回傳 (鍵, 值)。
================================================================================
*/

#include <iostream>
#include <utility>
// ... 程式碼
```

---

## 工作流程提醒

當使用者要求新增一個主題時,Claude 應:

1. 先確認該主題在 cppreference 的正確分類與最低 C++ 標準版本。
2. 挑選一題簡單實用的 Leetcode 題目作為應用範例。
3. 撰寫獨立的 .cpp 檔案,註釋採用上方範本結構。
4. 程式碼簡短直接,避免艱澀寫法。
5. 全程使用繁體中文。
