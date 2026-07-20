# ClaudeClass2026

Claude 與 Codex 各自獨立授課的學習筆記與範例程式碼合集。

| 課程 | 內容 |
|---|---|
| `Claude C++面向對象/` | **36 課．已完成**，設計哲學 → 封裝 → 拷貝／移動語意 → 運算子重載 |
| `Claude C++ STL課程/` | **40 課．已完成**，STL 六大組件、vector / deque / list / forward_list |
| `Claude C++ STL多線程課程/` | **5 階段／30 課．已完成**，thread / 生命週期 / 共享資料 / mutex |
| `Claude C++11 到 C++26 課程/` | **15 章．已完成**，型別推導與初始化、右值參考與移動語意 |
| `Claude Code C++by Examples/` | **20 個主題／405 支範例**，容器、多執行緒、智慧指標、模板、工具… |
| `Codex C++by Examples/` | **20 個主題／405 支獨立教材**，與 Claude `.cpp` 路徑 1:1；29 份 summary 含 331 題深挖問答，另補 72 題單課問答 |
| `Claude Python課程/` | **31 課．已完成**，基礎語法、流程控制與四大資料結構 |
| `Claude AI CUDA課程/` | 185 課 / 13 Parts，GPU 硬體到 AI 推論引擎系統（**進行中，見下方進度**） |
| `Codex AI CUDA課程/` | 185 課 / 13 Parts，Codex 獨立版本的 GPU/CUDA/AI 課程（**進行中，見下方進度**） |

## `Claude AI CUDA課程/` 進度

**2 / 185 課**　｜　大綱 `CUDA_AI_完整課程大綱_v2.0.6.md`（185 課 / 13 Parts）
下一課：**課 1.3 — NVIDIA GPU 架構演進史**

編譯方式見 [`Claude AI CUDA課程/BUILD.md`](Claude%20AI%20CUDA%E8%AA%B2%E7%A8%8B/BUILD.md)。
所有 CUDA 範例都已驗證可編到 **sm_75（本機 Quadro RTX 4000）＋ sm_89（4090）＋ sm_120（5090）**；
本機只有 sm_75 實體卡，其餘架構**僅保證編得過、執行結果未經驗證**。

<details>
<summary>展開全部 2 課</summary>

### Part 1 — GPU 與 CUDA 硬體基礎

| 課號 | 課名 | 完成日 | 這堂課證明了什麼 |
|---|---|---|---|
| 1.1 | 第 01.01 課：為什麼需要 GPU | 2026-07-18 | 本機實測 GPU 峰值 **7.07 TFLOPS／384 GB/s／machine balance 18.4 FLOP/byte**；用 Amdahl(10×) vs Gustafson(36864×) 解釋 40960 條執行緒為何用得掉；踩到 CUDA 13.x 移除 `cudaDeviceProp::clockRate` 的真實 API 變更 |
| 1.2 | 第 01.02 課：GPU 硬體架構總覽 | 2026-07-19 | 用 `%smid` 實測 160 個 block 均勻落在全部 **40 個 SM**；暫存器壓力掃描實測 occupancy 兩個轉折點（128 regs → 50%、255 regs → spill 608 B），驗證 `65536 ÷ 1024 = 64 暫存器/thread` 的資源預算公式 |

</details>

## `Codex AI CUDA課程/` 進度

**2 / 185 課**　｜　大綱 `CUDA_AI_完整課程大綱_v2.0.6.md`（185 課 / 13 Parts）
下一課：**課 1.3 — NVIDIA GPU 架構演進史**

編譯方式見 [`Codex AI CUDA課程/BUILD_GUIDE.md`](Codex%20AI%20CUDA%E8%AA%B2%E7%A8%8B/BUILD_GUIDE.md)。
可攜 CUDA 範例都會編譯驗證 **sm_75（本機 Quadro RTX 4000）＋ sm_89（4090）＋ sm_90（Hopper）＋ sm_120（5090）**；
本機只有 sm_75 實體卡，其餘架構**僅保證編得過、執行結果未經驗證**。

<details>
<summary>展開全部 2 課</summary>

### Part 1 — GPU 與 CUDA 硬體基礎

| 課號 | 課名 | 完成日 | 這堂課證明了什麼 |
|---|---|---|---|
| 1.1 | 第 01.01 課：為什麼需要 GPU | 2026-07-18 | 用 Amdahl/Gustafson 定量區分固定與擴張問題；本機 SAXPY 實測顯示大資料的 GPU kernel 最快約為 CPU 的 **12.74×**，但每次跨 PCIe 往返時端到端仍較 CPU 慢，證明 kernel throughput 不等於應用程式加速 |
| 1.2 | 第 01.02 課：GPU 硬體架構總覽 | 2026-07-19 | 用 Runtime、PTX `%laneid/%smid/%nsmid` 探針驗證本機 **40 SM、32-lane warp、384.064 GB/s 理論頻寬**；160 個 sample blocks 本次觀察涵蓋 40 個 SM、validation PASS，並明確區分一次觀察與 CUDA 規格保證 |

</details>

## `Claude C++面向對象/` 進度

**36 課．已完成**　｜　從設計哲學、類別與封裝，到拷貝／移動語意與運算子重載
範例程式 281 支（`.cpp`）

<details>
<summary>展開全部 36 課</summary>

| 課號 | 課名 | 範例數 |
|---|---|---|
| 1 | C++ 的歷史與設計哲學 | 2 |
| 2 | C 與 C++ 的關鍵差異 | 18 |
| 3 | C++ 編譯環境設置（g++、clang++） | 2 |
| 4 | 命名空間（namespace）基礎 | 9 |
| 5 | 輸入輸出流（iostream）入門 | 20 |
| 6 | 什麼是面向對象？核心思想介紹 | 4 |
| 7 | 類別（class）的定義與語法 | 4 |
| 8 | 對象（object）的創建與使用 | 9 |
| 9 | 成員變數（Member Variables） | 9 |
| 10 | 成員函數（Member Functions） | 10 |
| 11 | 存取修飾符——public、private、protected | 10 |
| 12 | struct 與 class 的差異 | 8 |
| 13 | 建構函數（Constructor）基礎 | 9 |
| 14 | 預設建構函數（Default Constructor） | 11 |
| 15 | 帶參數的建構函數 | 13 |
| 16 | 建構函數初始化列表（Member Initializer List） | 12 |
| 17 | 解構函數（Destructor） | 9 |
| 18 | 對象的生命週期（Object Lifetime） | 12 |
| 19 | 動態對象的創建與銷毀（new  delete） | 11 |
| 20 | 封裝（Encapsulation）的意義 | 4 |
| 21 | getter 與 setter 設計模式 | 8 |
| 22 | const 成員函數 | 8 |
| 23 | mutable 關鍵字 | 7 |
| 24 | 類別內的靜態成員變數 | 9 |
| 25 | 類別內的靜態成員函數 | 9 |
| 26 | this 指標 | 12 |
| 27 | 淺拷貝與深拷貝（Shallow Copy vs Deep Copy） | 3 |
| 28 | 拷貝建構函數（Copy Constructor） | 4 |
| 29 | 拷貝賦值運算子（Copy Assignment Operator） | 5 |
| 30 | 三法則（Rule of Three） | 2 |
| 31 | 右值引用（Rvalue Reference）入門 | 5 |
| 32 | 移動建構函數（Move Constructor） | 5 |
| 33 | 移動賦值運算子（Move Assignment Operator） | 3 |
| 34 | 五法則（Rule of Five） | 3 |
| 35 | stdmove 的使用 | 3 |
| 36 | 運算子重載概念與語法 | 3 |

</details>

## `Claude C++ STL課程/` 進度

**40 課．已完成**　｜　六大組件總覽，vector / deque / list / forward_list 逐一深入
範例程式 308 支（`.cpp`）

<details>
<summary>展開全部 40 課</summary>

| 課號 | 課名 | 範例數 |
|---|---|---|
| 1 | STL 的歷史與設計哲學 | 7 |
| 2 | 泛型編程（Generic Programming）概念 | 14 |
| 3 | STL 的六大組件概覽 | 14 |
| 4 | 迭代器（Iterator）的核心概念 | 13 |
| 5 | 迭代器的五種分類 | 12 |
| 6 | 容器（Container）的概念與分類 | 17 |
| 7 | 演算法（Algorithm）與容器的分離設計 | 18 |
| 8 | 函數物件（Function Object）初探 | 17 |
| 9 | vector 的內部結構與記憶體配置 | 4 |
| 10 | vector 的宣告與初始化方式 | 10 |
| 11 | vector 的容量管理：size、capacity、reserve | 12 |
| 12 | vector 元素存取：operator[]、at、front、back | 12 |
| 13 | vector 元素新增：push_back、emplace_back | 11 |
| 14 | vector 元素插入：insert、emplace | 13 |
| 15 | vector 元素刪除：pop_back、erase、clear | 14 |
| 16 | vector 的迭代器操作 | 15 |
| 17 | vector 的記憶體重新配置機制 | 12 |
| 18 | vectorbool 的特殊性與陷阱 | 15 |
| 19 | vector 與原始陣列的互操作 | 17 |
| 20 | vector 效能分析與最佳實踐 | 13 |
| 21 | deque 的內部結構（分段連續空間） | 2 |
| 22 | deque 的宣告與初始化 | 3 |
| 23 | deque 的雙端操作：push_front、push_back、pop_front、pop_back | 3 |
| 24 | deque 與 vector 的比較 | 2 |
| 25 | deque 的迭代器特性 | 2 |
| 26 | deque 的插入與刪除效能分析 | 2 |
| 27 | deque 作為 stackqueue 底層容器 | 6 |
| 28 | deque 的實際應用場景 | 4 |
| 29 | list 的雙向鏈結串列結構 | 2 |
| 30 | list 的宣告與初始化 | 2 |
| 31 | list 的元素操作——push、pop、insert、erase | 2 |
| 32 | list 的特有操作——splice | 2 |
| 33 | list 的特有操作——merge | 2 |
| 34 | list 的特有操作——sort | 2 |
| 35 | list 的特有操作——unique、remove、remove_if | 2 |
| 36 | list 的特有操作——reverse | 2 |
| 37 | list 迭代器失效規則 | 2 |
| 38 | forward_list 單向鏈結串列介紹 | 2 |
| 39 | forward_list 的特殊操作——insert_after、erase_after | 2 |
| 40 | list 與 forward_list 的選擇時機 | 2 |

</details>

## `Claude C++11 到 C++26 課程/` 進度

**15 課．已完成**　｜　型別推導與初始化、右值參考與移動語意（**章節制**，非課號制）
範例程式 82 支（`.cpp`）

<details>
<summary>展開全部 15 課</summary>

| 課號 | 課名 | 範例數 |
|---|---|---|
| 1.1 | auto 關鍵字 — 自動型別推導 | 2 |
| 1.2 | decltype — 查詢表達式的型別 | 4 |
| 1.3 | decltype(auto) — 完美保留型別的自動推導 | 2 |
| 1.4 | 統一初始化 (Uniform Initialization) — 大括號初始化語法 | 3 |
| 1.5 | stdinitializer_list — 初始化列表的底層機制 | 2 |
| 1.6 | 列表初始化的陷阱 — Narrowing Conversion 與優先順序問題 | 2 |
| 2.1 | 左值與右值的定義 — Value Categories 完整解析 | 6 |
| 2.2 | 右值參考 && — 基本語法與用途 | 9 |
| 2.3 | 移動建構子 (Move Constructor) — 實作與原理 | 6 |
| 2.4 | 移動賦值運算子 (Move Assignment Operator) — 實作與原理 | 4 |
| 2.5 | stdmove — 強制轉換為右值 | 6 |
| 2.6 | stdforward — 完美轉發的核心 | 9 |
| 2.7 | 完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術 | 13 |
| 2.8 | Rule of Five — 現代資源管理規則 | 4 |
| 2.9 | 移動語意的效能分析 — 實測比較與最佳實踐 | 8 |

</details>

## `Claude Python課程/` 進度

**31 課．已完成**　｜　基礎語法、流程控制到四大資料結構
範例程式 71 支（`.py`）

> ℹ️ 編號從 1 直接跳到 3：**第 2 課「安裝 Python」是純安裝說明、本來就沒有程式碼**，
> 所以沒有對應目錄（實際 31 個目錄，編號到 32）。這是刻意的，不是缺漏。

<details>
<summary>展開全部 31 課</summary>

| 課號 | 課名 | 範例數 |
|---|---|---|
| 1 | 什麼是 Python？ | 2 |
| 3 | 認識開發工具 | 2 |
| 4 | 第一個程式 —— Hello World | 2 |
| 5 | 變數與命名規則 | 2 |
| 6 | 基本資料型別（一）—— 整數 int 與浮點數 float | 2 |
| 7 | 基本資料型別（二）—— 字串 str | 2 |
| 8 | 基本資料型別（三）— 布林值 bool | 2 |
| 9 | 型別轉換 — int()、str()、float()、bool() | 2 |
| 10 | 輸入與輸出 — print() 與 input() 完全攻略 | 2 |
| 11 | 註解的寫法 — 讓程式碼會說話 | 2 |
| 12 | 算術運算子 — 加減乘除與更多 | 2 |
| 13 | 比較運算子 — 大於、小於、等於、不等於 | 2 |
| 14 | 邏輯運算子 — and、or、not | 2 |
| 15 | 賦值運算子 — =、+=、-= 等 | 2 |
| 16 | 運算子優先順序 — 誰先算？ | 2 |
| 17 | 條件判斷 if — 基本語法 | 2 |
| 18 | if-else 與 if-elif-else — 多重條件 | 2 |
| 19 | 巢狀條件 — if 裡面還有 if | 2 |
| 20 | 迴圈 for — 重複執行的魔法 | 2 |
| 21 | range() 函數 — 產生數字序列 | 2 |
| 22 | 迴圈 while — 條件式重複 | 2 |
| 23 | break 與 continue — 跳出與跳過 | 2 |
| 24 | 巢狀迴圈 — 迴圈中的迴圈 | 2 |
| 25 | 串列 List（一）— 建立與存取 | 2 |
| 26 | 串列 List（二）— 新增、刪除、修改 | 2 |
| 27 | 串列 List（三）— 常用方法與切片 | 2 |
| 28 | 元組 Tuple — 不可變的串列 | 2 |
| 29 | 字典 Dictionary（一）— 鍵值對的概念 | 2 |
| 30 | 字典 Dictionary（二）— 操作與方法 | 2 |
| 31 | 集合 Set — 不重複的元素集合 | 2 |
| 32 | 資料結構比較與選擇 — 什麼時候用什麼？ | 2 |

</details>

## `Claude C++ STL多線程課程/` 進度

**5 階段／30 課．已完成**　｜　並行基礎、`std::thread`、生命週期、共享資料與競爭條件、`std::mutex`
範例程式 153 支（`.cpp`）

| 階段 | 主題 | 課數 | 課號範圍 |
|---|---|---|---|
| 第一階段 | 並行程式設計基礎概念 | 6 | 1.1 – 1.6 |
| 第二階段 | stdthread 基礎 | 7 | 2.1 – 2.7 |
| 第三階段 | 執行緒生命週期管理 | 6 | 3.1 – 3.6 |
| 第四階段 | 共享資料與競爭條件 | 5 | 4.1 – 4.5 |
| 第五階段 | 互斥鎖基礎 (stdmutex) | 6 | 5.1 – 5.6 |

## `Claude Code C++by Examples/` 進度

**20 個主題分類**　｜　依主題分類的可執行範例合集（非課號制）
範例程式 405 支（`.cpp`）

<details>
<summary>展開全部 20 個主題</summary>

| 主題 | 範例數 |
|---|---|
| `C++_Algorithm` | 83 |
| `C++_Bit` | 13 |
| `C++_Cast` | 9 |
| `C++_Chrono` | 10 |
| `C++_Container` | 18 |
| `C++_Cpp11` | 16 |
| `C++_Cpp14` | 8 |
| `C++_Cpp17` | 14 |
| `C++_Exception` | 10 |
| `C++_Filesystem` | 9 |
| `C++_IOStream` | 11 |
| `C++_Iterator` | 15 |
| `C++_Lambda` | 11 |
| `C++_MultiThread` | 31 |
| `C++_OOP` | 29 |
| `C++_Random` | 10 |
| `C++_SmartPointers` | 6 |
| `C++_String` | 59 |
| `C++_Template` | 29 |
| `C++_Utility` | 14 |

</details>

## `Codex C++by Examples/` 進度

**20 個主題分類**　｜　Codex 獨立撰寫、可離線閱讀與執行的 C++ 教科書式範例
範例程式 405 支（`.cpp`），相對路徑與 Claude 版本 1:1，內容不互相複製

編譯與教材稽核方式見 [`Codex C++by Examples/BUILD_GUIDE.md`](Codex%20C++by%20Examples/BUILD_GUIDE.md)。
面試練習方式、29 份速查入口與三層回答法見
[`Codex C++by Examples/INTERVIEW_GUIDE.md`](Codex%20C++by%20Examples/INTERVIEW_GUIDE.md)。

<details>
<summary>展開全部 20 個主題</summary>

| 主題 | 範例數 |
|---|---:|
| `C++_Algorithm` | 83 |
| `C++_Bit` | 13 |
| `C++_Cast` | 9 |
| `C++_Chrono` | 10 |
| `C++_Container` | 18 |
| `C++_Cpp11` | 16 |
| `C++_Cpp14` | 8 |
| `C++_Cpp17` | 14 |
| `C++_Exception` | 10 |
| `C++_Filesystem` | 9 |
| `C++_IOStream` | 11 |
| `C++_Iterator` | 15 |
| `C++_Lambda` | 11 |
| `C++_MultiThread` | 31 |
| `C++_OOP` | 29 |
| `C++_Random` | 10 |
| `C++_SmartPointers` | 6 |
| `C++_String` | 59 |
| `C++_Template` | 29 |
| `C++_Utility` | 14 |

</details>

## 說明

- 本倉庫**只收原始碼與文件**（`.cpp` / `.py` / `.md` / `.pdf`）。
  建置產物（`.exe` / `.obj` / `.pdb` / `.ilk` 等）由 `.gitignore` 擋下，請勿提交。
- 註解與說明一律使用**繁體中文**。
- 2026-07-18 由舊倉庫重建為乾淨版（舊版 `.git` 因混入編譯產物膨脹至 1.9 GB）。
