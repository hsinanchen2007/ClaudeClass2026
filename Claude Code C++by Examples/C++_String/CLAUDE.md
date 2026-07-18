# CLAUDE.md

本檔案為 Claude Code 在此專案中工作時的指引文件。

## 專案目標 (Project Goal)

本專案的目標是參考 cppreference 上的 `std::basic_string` 文件
(https://en.cppreference.com/cpp/string/basic_string),建立一個完整的
C++ STL string 類別實作教學範例,供學習者深入理解 STL 字串類別的內部運作。

## 五大核心規則 (Five Core Rules) - 最高優先

以下五點為本專案的最高層級規範,優先於下方所有細節說明。
若下方規則與此處衝突,以此處為準。

### 規則 1:語言 (Language)
- **永遠使用繁體中文 (Traditional Chinese) 回覆使用者,不論使用者是用中文或英文提問**
- 程式碼中的所有註解亦必須使用繁體中文
- 技術名詞 (iterator、template、constructor、reference 等) 可保留英文原文

### 規則 2:LeetCode 範例 (LeetCode Examples)
- 每個主題 (每支程式) 必須附帶一個對應的 LeetCode 題目示範
- 題目選擇原則:**簡單實用、貼近日常開發**,而非「為了考演算法的刁鑽題目」
  - 優先 Easy / Medium 難度
  - 必須是該函式「自然」會用到的題目,而非硬湊
  - 例如:`find` → LeetCode 28 (strStr)、`back` → LeetCode 20 (Valid Parentheses)
- LeetCode 範例需直接寫進現有的 C++ 原始碼中,並有詳盡的繁體中文註解
- 註解中必須包含:題目敘述、為何選用此函式、解題思路、複雜度

### 規則 3:每程式一主題 + 註解詳盡 (One Topic per File + Detailed Comments)
- **每個 `.cpp` 檔案只聚焦在一個主題 (一個 STL 函式 / 一個概念)**
- 每個主題必須在程式碼註解中提供:
  - 完整的課題介紹 (這個函式是什麼、為何存在)
  - 概念解釋 (底層機制、設計理念、與其他函式的關係)
  - 邊界條件、例外安全、效能複雜度
  - C++ 標準版本差異 (C++11/14/17/20/23)
- **註解的份量遠多過實際程式碼也完全沒問題**,本專案的目的是「教學」,
  程式碼是輔助理解概念的範例,概念說明才是主軸

### 規則 4:範例簡單直接 (Simple and Direct Examples)
- **所有程式範例必須簡單、直接、易讀**
- **不要為了展示能力而寫艱澀的 C++**(例如不必要的 template metaprogramming、
  晦澀的 SFINAE、過度泛型化)
- **不要寫過長的程式碼**,範例的目的是把概念講清楚,不是炫技
- 寧可多用幾個小範例,也不要寫一個很長的綜合範例
- 範例應讓初學者也能讀懂

### 規則 5:參考來源 (Reference Sources)
- 所有 C/C++ 內容皆需參考下列兩個權威網站:
  - **cppreference**: https://en.cppreference.com/cpp
  - **cplusplus.com**: https://cplusplus.com/reference/
- 每個檔案的標頭註解中,必須同時列出兩個網站對應的連結
- 內容若兩網站描述有差異,以 cppreference 為主、cplusplus.com 為輔

---

## 程式碼結構需求 (Code Structure Requirements)

1. **每個函式 (member function) 一個獨立的 C++ 原始碼檔案**
   - 檔名應清楚反映函式名稱,例如:`constructor.cpp`、`append.cpp`、`substr.cpp`
   - 每個檔案需可獨立編譯與執行(包含 `main()` 範例)

2. **每個檔案必須包含以下內容(以註解形式)**
   - **參考連結 (References)**:同時列出 cppreference 與 cplusplus.com 連結
   - **函式資訊 (Information)**:函式原型、參數、回傳型別、所屬類別
   - **詳細解釋 (Explanation)**:此函式的功能、行為、底層運作機制、設計理念
     (此區塊內容應該很「厚」,把概念講透徹)
   - **使用範例 (Usage)**:實際程式碼範例,展示如何呼叫此函式
   - **注意事項 (Things to Pay Attention)**:常見錯誤、邊界條件、效能考量、
     例外安全性 (exception safety)、與其他函式的互動、版本差異 (C++11/17/20/23) 等
   - **LeetCode 範例 (LeetCode Example)**:
     - 每個檔案需包含至少一個 LeetCode 題目示範 (見規則 2)
     - 範例可放在同一個 `main()` 中以區塊區隔,或寫成獨立的小函式
     - 必須有清楚的題目描述註解、解題思路、為何用此函式

   - **日常實務範例 (Daily-Work Examples)**:
     - 每個檔案除了 LeetCode 題目外,**還需要一段「日常工作會用到的實務範例」**
     - 內容應貼近後端 / DevOps / 系統開發的真實任務,例如:
       - SQL placeholder 產生、IN 子句構造
       - HTTP request/response header 解析 (Authorization、Content-Type、charset)
       - URL / path 處理 (host 抽取、絕對相對路徑判斷、副檔名)
       - Log 處理 (chomp、CRLF 去除、log 警示偵測、敏感資料遮蔽)
       - 設定檔解析 (.env、INI、CSV)
       - 檔案大小 / bytes 顯示、進度條
       - 編碼轉換 (BOM 處理、hex dump)
       - 系統呼叫接合 (write、getenv、固定欄位寫入)
       - Buffer 重用 / double-buffering / snapshot 模式
     - 標記為 `// 【日常實務範例 Daily Work】` 區塊,並在 `main()` 加上
       `=== 日常實務: ... ===` 標題區隔
     - 必須說明「為何用這個函式」(在註解中清楚交代)
     - 範例不需要太長,重點是「實務有用」與「展示函式的實際應用情境」

3. **註解風格**
   - 註解必須非常詳盡 (very detailed),且份量可以多過程式碼
   - 適合作為教學材料,讓初學者也能理解
   - 包含 Big-O 複雜度說明
   - 標註 `noexcept`、`constexpr` 等修飾符的意義
   - 必須使用繁體中文

## 涵蓋範圍 (Scope - 參考 cppreference basic_string)

需逐步建立涵蓋以下類別成員的範例檔案:

- 建構子 / 解構子 / 賦值運算子 (Constructors / Destructor / operator=)
- 元素存取 (Element access):`at`、`operator[]`、`front`、`back`、`data`、`c_str`
- 迭代器 (Iterators):`begin`、`end`、`rbegin`、`rend`、`cbegin`、`cend` 等
- 容量 (Capacity):`empty`、`size`、`length`、`max_size`、`reserve`、
  `capacity`、`shrink_to_fit`
- 修改器 (Modifiers):`clear`、`insert`、`erase`、`push_back`、`pop_back`、
  `append`、`operator+=`、`compare`、`replace`、`substr`、`copy`、`resize`、`swap`
- 搜尋 (Search):`find`、`rfind`、`find_first_of`、`find_last_of`、
  `find_first_not_of`、`find_last_not_of`
- 操作 (Operations):`starts_with`、`ends_with`、`contains` (C++20/23)
- 非成員函式 (Non-member functions):`operator+`、比較運算子、`std::swap`、
  `getline`、`stoi/stol/stof` 等轉換函式
- I/O 運算子:`operator>>`、`operator<<`

## 工作方式 (Working Style)

- 使用者請求建立特定函式檔案時,直接在工作目錄下建立 `.cpp` 檔
- 若使用者未指定起點,可從建構子 (constructor) 開始,逐步往下推進
- 每個檔案建立後,簡要回報檔名與該函式重點

## 編譯標準 (Compile Standard)

預設使用 C++20 (`-std=c++20`),若範例使用較新功能 (C++23) 會在註解中註明。
