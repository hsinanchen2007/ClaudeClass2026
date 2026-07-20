# Codex C++ 面試題庫使用指南

這份指南把教材中的面試問答組成一套可反覆練習的系統。題目不是從網路題庫原文拼貼：
Codex 先以 C++ 標準草案、cppreference、C++ Core Guidelines 與 ISO C++ FAQ 核對技術事實，
再以美國／英語圈、中國與其他地區的面試紀錄判斷頻率；Claude、Gemini、Grok 的研究只作為
候選題與缺口來源，最後全部重新去重、改寫並放回最適合的主題。

## 題庫規模

- 20 個頂層主題均有自己的 `summary.cpp`；Algorithm 再細分 10 類，共 29 份總整理。
- 29 份 `summary.cpp` 新增 331 題深挖問答。
- 另為 24 個原先沒有明確問答段落的單課檔案新增 72 題。
- 其餘單課原有的面試問題、LeetCode 與實務案例全部保留。

數字只代表覆蓋範圍，不代表背得愈多愈好。真正目標是能說清楚「標準保證、前置條件、
生命週期、失敗模式、效能取捨與如何驗證」。

## 三層回答法

### 第一層：30 秒直接回答

先給定義與結論，不要從歷史故事開始。例如：

> `std::move` 本身不搬資料，它是產生 xvalue 的 cast；是否真的 move，取決於 overload resolution
> 與被選到的 constructor/assignment。

面試官應先聽到正確核心，而不是等兩分鐘才知道你是否理解。

### 第二層：2 分鐘補足邊界

依序補：

1. 前置條件與 ownership/lifetime。
2. 標準保證和常見實作的區別。
3. 複雜度、iterator/reference invalidation 或同步關係。
4. 一個常見誤答或會造成 UB 的反例。
5. 一個本教材可執行的工作案例。

### 第三層：資深追問

把答案接到工程決策：替代方案為何、何時不該用、如何測試、如何觀測 production failure。
例如回答 `shared_mutex` 時，不能停在「讀多寫少」；還要談 reader bookkeeping、writer
starvation、公平性不保證、critical section 大小，以及以 benchmark 證明是否值得。

## 面試官真正檢查的七件事

| 面向 | 應回答的問題 |
|---|---|
| 標準語意 | 這是標準保證，還是 libstdc++／ABI 的常見實作？ |
| 前置條件 | range 要排序、partition、有效，還是 comparator 要滿足特定契約？ |
| 所有權 | 誰建立、誰釋放、誰只是借用？borrow 可活多久？ |
| 失敗模型 | 會丟例外、回空值、回 error code、terminate，還是形成 UB？ |
| 複雜度 | 除了 Big-O，allocation、cache、iterator movement、contention 是什麼？ |
| 替代方案 | 為何選它而非另一個容器、同步原語或 vocabulary type？ |
| 驗證 | 單元測試、sanitizer、stress、benchmark、failure injection 如何分工？ |

## 29 份速查入口

| 領域 | 面試前主要入口 |
|---|---|
| C++11／14／17 | `C++_Cpp11/summary.cpp`、`C++_Cpp14/summary.cpp`、`C++_Cpp17/summary.cpp` |
| OOP／資源 | `C++_OOP/summary.cpp`、`C++_SmartPointers/summary.cpp`、`C++_Exception/summary.cpp` |
| 泛型／可呼叫物件 | `C++_Template/summary.cpp`、`C++_Lambda/summary.cpp`、`C++_Utility/summary.cpp` |
| STL 結構 | `C++_Container/summary.cpp`、`C++_Iterator/summary.cpp` |
| Algorithm | `C++_Algorithm/*/summary.cpp`（binary search 到 sorting，共 10 份） |
| 資料與 I/O | `C++_String/summary.cpp`、`C++_IOStream/summary.cpp`、`C++_Filesystem/summary.cpp` |
| 數值工具 | `C++_Bit/summary.cpp`、`C++_Chrono/summary.cpp`、`C++_Random/summary.cpp` |
| 型別與錯誤 | `C++_Cast/summary.cpp`、`C++_Exception/summary.cpp` |
| 並行 | `C++_MultiThread/summary.cpp` |

## 建議練習週期

1. **第一次**：逐課執行範例，能用自己的話解釋 assertions 為何成立。
2. **第二次**：只看問題，口頭回答 30 秒版，再對照檔案答案補漏。
3. **第三次**：每題再回答一個反例、替代方案與 production 使用情境。
4. **面試前一週**：每天讀 4–5 份 `summary.cpp`，把答不完整的題標成 weak list。
5. **面試前一天**：只複習 weak list、UB/lifetime、container invalidation 與 memory model；不再擴題。

## 去重規則

同一知識點只在一處當主問題，其餘主題以交叉角度追問。例如：

- `std::move` 的 value category 主體放 C++11；資源 class 的 Rule of Five 放 OOP。
- `shared_ptr` control block 放 SmartPointers；跨執行緒 publication 放 MultiThread。
- strict weak ordering 主體放 Algorithm；map key equivalence 放 Container。
- `string_view` lifetime 放 String；ranges 的 borrowed/dangling 放 Iterator。

這樣可避免背到兩份措辭不同、甚至彼此矛盾的答案，也能在面試時清楚指出問題的真正 ownership。

## 自我評分

每題 0–4 分：

- `0`：不會或核心結論錯誤。
- `1`：只背 API 名稱，說不出前置條件。
- `2`：定義正確，能說出一個陷阱。
- `3`：能比較替代方案，說清生命週期／複雜度／失敗模式。
- `4`：能連到 production 案例、測試與觀測，並區分標準保證和實作細節。

目標不是每題都背成 4 分，而是高頻核心題至少 3 分，且任何 ownership、UB、data race 題不得答錯。
