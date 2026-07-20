# Codex C++ by Examples

這是一套以「先建立正確模型，再用短程式驗證」為主軸的繁體中文 C++ 教材。課程與
`Claude Code C++by Examples/` 的 405 個 `.cpp` 保持相同相對路徑，方便逐題對照；
章節內容、程式、說明與驗證流程均由 Codex 獨立撰寫，不是另一軌的文字副本。

## 適合誰

- 剛開始學 C++，手邊沒有其他教科書或網路資料，希望直接從可執行檔案學習。
- 已會基本 C/C++ 語法，想補齊現代 C++ 標準函式庫與工程習慣。
- 想理解「為什麼要選這個型別/API」，而不只背語法。
- 未來要學 CUDA C++，需要先把 RAII、泛型、iterator、並行與生命週期打穩。

## 閱讀方法

每個 `.cpp` 同時是講義與可執行實驗，固定包含原理、API/複雜度、基礎用法、真正可執行
的短 LeetCode 題、真正可執行的工作案例、易錯處、面試問題與練習。建議依序：

1. 先讀該主題 README，建立章節地圖與學習順序。
2. 逐檔讀繁體中文講義，再編譯、執行其中 assertions。
3. 修改輸入資料，先預測結果再重跑。
4. 完成檔尾練習，並回答面試追問。
5. 章節結束後讀 `summary.cpp`；它是可獨立使用的面試前速查，不是簡短索引。

## 課程地圖

| 順序 | 目錄 | 核心問題 | 標準 |
|---:|---|---|---|
| 1 | [C++_Cpp11](C++_Cpp11/README.md) | 現代 C++ 的 RAII、型別安全與移動語意從哪裡開始 | C++11 |
| 2 | [C++_Cpp14](C++_Cpp14/README.md) | 如何把 C++11 寫得更簡潔、更泛型 | C++14 |
| 3 | [C++_Cpp17](C++_Cpp17/README.md) | 如何用 vocabulary types 表達「可能沒有／多種結果」 | C++17 |
| 4 | [C++_OOP](C++_OOP/README.md) | 何時用值語意，何時才需要多型 | C++20 |
| 5 | [C++_Template](C++_Template/README.md) | 如何把共同規則抽成可檢查的泛型程式 | C++20 |
| 6 | [C++_Container](C++_Container/README.md) | 如何依存取、插入、排序與失效規則選容器 | C++20 |
| 7 | [C++_Iterator](C++_Iterator/README.md) | 演算法與容器之間的共同語言 | C++20 |
| 8 | [C++_Algorithm](C++_Algorithm/README.md) | 如何用意圖清楚的演算法取代手寫迴圈 | C++20 |
| 9 | [C++_Lambda](C++_Lambda/README.md) | 如何安全地把行為傳給演算法與非同步工作 | C++20 |
| 10 | [C++_String](C++_String/README.md) | 擁有字串、字串 view 與 UTF-8 bytes 的界線 | C++20 |
| 11 | [C++_SmartPointers](C++_SmartPointers/README.md) | 如何明確表達所有權 | C++20 |
| 12 | [C++_Exception](C++_Exception/README.md) | 失敗時如何維持物件不變式與資源安全 | C++20 |
| 13 | [C++_Utility](C++_Utility/README.md) | optional、variant、tuple 等小型組合工具 | C++20 |
| 14 | [C++_Cast](C++_Cast/README.md) | 哪種轉型表達哪種意圖，哪些轉型應避免 | C++20 |
| 15 | [C++_Bit](C++_Bit/README.md) | 位元旗標、bitset 與 C++20 bit utilities | C++20 |
| 16 | [C++_Chrono](C++_Chrono/README.md) | 如何用型別安全的單位表示時間 | C++20 |
| 17 | [C++_Random](C++_Random/README.md) | 引擎、分布與可重現測試如何分工 | C++20 |
| 18 | [C++_Filesystem](C++_Filesystem/README.md) | 如何安全組合路徑、巡覽與原子儲存 | C++20 |
| 19 | [C++_IOStream](C++_IOStream/README.md) | 格式化、解析與檔案 I/O 的狀態模型 | C++20 |
| 20 | [C++_MultiThread](C++_MultiThread/README.md) | 執行緒生命週期、同步與任務抽象 | C++20 |

## 全課驗證

完整命令與編譯器需求見 [BUILD_GUIDE.md](BUILD_GUIDE.md)。最快的檢查方式：

    ./tools/audit_textbook.sh
    ./tools/check_all.sh
    ./tools/check_all.sh --run
    CXX=clang++ ./tools/check_all.sh

`audit_textbook.sh` 驗證 405 路徑一對一與教材完整度；編譯工具將每個 executable 建在
一次性暫存目錄，離開時自動刪除，絕不把輸出寫回 source tree。

## 本教材刻意不做的事

- 不把每個標準庫 API 都列成百科；重點是建立可遷移的選擇能力。
- 不用可執行 UB 當示範。錯誤模式會以註解、型別系統或 sanitizer 練習說明。
- 不假設「在某台機器印出某結果」就是標準保證。
- 不以大量巨型範例取代小型、可驗證的核心模型。
