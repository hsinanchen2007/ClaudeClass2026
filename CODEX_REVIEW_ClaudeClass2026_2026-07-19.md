# Codex 深度審查：ClaudeClass2026

> 審查日期：2026-07-19
> 審查基線：commit <code>3c9aab2</code>
> 審查範圍：repository 內所有 Claude 課程、根層設定與文件；完整排除 <code>Codex AI CUDA課程/</code> 及本次新建的 <code>Codex C++by Examples/</code>。
> 修改邊界：本報告為新增檔案；審查期間沒有修改任何 Claude 原作。
>
> **後續狀態註記：**Claude 已在 <code>7619f30</code> 依本報告修正內容問題，並在
> <code>904538f</code> 移除 195 個 ELF。本文刻意保留 <code>3c9aab2</code> 當時的審查
> 證據與重現方法，不應再把下列數字直接當成目前 HEAD 的缺陷數。

## 1. 結論

教材大部分內容可用，但目前不能宣稱「全部範例皆可依文件命令編譯、執行且安全」。本輪確認：

- 3 組會在正常執行路徑觸發 undefined behavior 的演算法/數值程式。
- 2 組刻意反例直接執行 UB，且其中一處把 UB 簡化成「只會洩漏」。
- 5 個目前會發生真實 Python runtime exception 的 tracked 檔案（含 2 個舊版快照）。
- 5 個多執行緒教材檔不是自包含可編譯程式，其中 2 個是實際 API 用錯。
- 6/125 個 <code>summary.cpp</code> 不符合根層文件宣稱的 C++17 自包含 contract。
- CUDA 課 1.2 對 RTX 5090 thread block cluster 的說明與 NVIDIA 文件、同 repo 黃金大綱互相矛盾。
- 195 個約 10.32 MiB 的 extensionless ELF executable 已被 Git 追蹤，與「只收原始碼與文件」政策矛盾。

建議先修第 3 節的 High，再處理編譯 contract 與 repository hygiene。不要一次做大規模文字重寫；每一類修正後用本報告第 9 節的命令重新驗證。

## 2. 驗證摘要

### 2.1 完整盤點

| 類型 | 數量 | 結果 |
|---|---:|---|
| C++ source | 1,230 | 依檔案主題選 C++11/14/17/20/23，GCC 1,213 過、17 失敗；Clang 同為 1,213/17 |
| 所有 <code>summary.cpp</code> | 125 | 依根層 contract 用 C++17：119 過、6 失敗 |
| Claude Code 範例 sanitizer run | 405 | 391 正常、7 link/compile fail、6 runtime sanitizer fail、1 刻意 race 範例逾時 |
| Python source | 71 | 71/71 通過 <code>py_compile</code>；執行期另見第 3.4 節 |
| CUDA source | 3 | sm_75、sm_89、sm_120 共 9/9 編譯；本機 native 3/3 執行；memcheck 3/3 通過 |
| Markdown | 27 | 27/27 可由 Pandoc parse |
| Markdown local links | 6 | 6/6 有效 |
| PDF | 11 | 11/11 可辨識且可抽取文字 |
| Makefile 宣告目標 | 123 | 123/123 可建置；但 Makefile 沒涵蓋全部 1,230 個 source |

### 2.2 使用環境

- GCC 15.2.0
- Clang 21.1.8
- CUDA 13.3 / Quadro RTX 4000 sm_75
- Python 3.14.4
- AddressSanitizer、UndefinedBehaviorSanitizer、pointer-subtract sanitizer
- Bash syntax、Pandoc、pdftotext、cppcheck（cppcheck 的跨 translation-unit ODR 雜訊已排除）

### 2.3 沒有誤報成 bug 的刻意教材

以下檔案明確是反例或工具教學，因此「被 sanitizer 抓到」本身不是意外：

- <code>C++_MultiThread/14_thread_sanitizer.cpp</code>：刻意保留 race。
- initializer_list 回傳/保存 reference、iterator invalidation 等檔案中，若危險行只留在註解，視為正確教學。
- Parallel STL 三檔需要 <code>-ltbb</code>，檔頭已記載；一般無 <code>-ltbb</code> 的 sanitizer harness link fail 不算 source syntax bug。

不過，若反例會在「正常執行範例」時直接觸發 UB，仍應依第 3.3 節調整交付方式。

## 3. High：應優先修正

### 3.1 <code>std::minmax</code> 回傳 reference，兩處讀到已死亡 temporary

檔案：

- <code>Claude Code C++by Examples/C++_Algorithm/min_max/minmax.cpp:38-41</code>
- 同檔 <code>:99-100</code>
- 同檔 <code>:149-150</code>

問題：

1. 兩參數 overload 回傳 <code>pair&lt;const T&amp;, const T&amp;&gt;</code>。
2. <code>auto p = std::minmax(7, 3)</code> 的 pair 仍然內含 references；statement 結束後兩個 temporary 已死亡。
3. 下一行讀 <code>p.first/p.second</code> 是 stack-use-after-scope。
4. 註解把 <code>auto p = std::minmax(a, b + 1)</code> 標成「安全」也是錯的；<code>auto</code> 不會移除 pair 內層的 reference members。
5. 環狀時間差範例的 <code>mins.front() + 24 * 60</code> 同樣是 temporary。

實測：

    AddressSanitizer: stack-use-after-scope
    minmax.cpp:100

建議：

- 兩值都先存成 lvalue，再呼叫兩參數 overload；或
- 使用 initializer-list overload <code>std::minmax({7, 3})</code>，它回傳 <code>pair&lt;T,T&gt;</code>；或
- 這些情境直接算差值，不建立保存 references 的 pair。

### 3.2 以 lambda 的 by-value 參數地址推算 vector index，屬不相關 pointer 相減

檔案：

- <code>Claude Code C++by Examples/C++_Algorithm/non_modifying/all_any_none_of.cpp:179-184</code>
- <code>Claude Code C++by Examples/C++_Algorithm/non_modifying/find.cpp:209-214</code>

問題：

Lambda 參數是 <code>int x</code>/<code>int y</code>/<code>int v</code>，所以 <code>&amp;x</code> 等地址指向 lambda call stack 上的副本，不是 vector 元素。將它與 <code>&amp;arr[0]</code> 相減沒有合法共同 array object，行為未定義；算出的「index」也沒有意義。

實測（額外開啟 pointer pair 檢查）：

    AddressSanitizer: invalid-pointer-pair
    all_any_none_of.cpp:181

    AddressSanitizer: invalid-pointer-pair
    find.cpp:212

這不只是一個寫法問題，也可能錯誤地把同一元素當成兩個位置。

建議：改用 explicit index loop，或讓外層保留 iterator，再在排除該 iterator 的兩段 range 搜尋。

### 3.3 「反例」在預設 main 直接執行 UB

檔案：

| 檔案與行 | 問題 | 實測 |
|---|---|---|
| <code>C++_OOP/18_VirtualDestructor.cpp:125-129</code> | 透過 non-virtual base destructor 刪 derived object 是 UB，不是標準保證「只呼叫 base destructor並洩漏」 | ASan <code>new-delete-type-mismatch</code> |
| <code>C++_Cast/02_static_cast.cpp:95-98</code> | 把實際 Dog object 的 base pointer 向下轉為 Cat，轉型本身不滿足 downcast 前置條件 | UBSan runtime downcast error |

教材可以解釋錯誤，但不應讓一般 <code>./example</code> 默認觸發 UB。建議：

- 把錯誤程式縮成註解或 <code>#if DEMONSTRATE_UB</code> 的 opt-in sanitizer lab。
- 預設 main 只跑修正版。
- Virtual destructor 說明改為「行為未定義，洩漏只是可能觀察到的結果」，不能承諾析構順序。

另：<code>C++_Cast/02_static_cast.cpp:79-82</code> 同時寫「未定義」與「implementation-defined」。在該檔 C++17 語境，超出 signed destination range 的 integer conversion 是 implementation-defined；不要混稱 UB。

### 3.4 Python tracked 教材有可重現 runtime exception

| 檔案與行 | 問題 | 例外 |
|---|---|---|
| <code>Claude Python課程/summary25-32.py:177,321</code> | <code>scores</code> 先被設為 list，後面呼叫 <code>scores.items()</code> | AttributeError |
| <code>Claude Python課程/第 16 課：運算子優先順序 — 誰先算？/第 16 課：運算子優先順序 — 誰先算？1.py:294-304</code> | <code>d</code>、<code>e</code>、<code>f</code> 使用前未定義 | NameError |
| <code>Claude Python課程/summary1-32.py:905-910</code> | <code>has_ticket</code> 在此 aggregate 執行路徑未定義 | NameError |
| <code>Claude Python課程/summary1-32_old.py:157</code> | 舊快照使用未定義 <code>month</code> | NameError |
| <code>Claude Python課程/summary1-24_old.py:476</code> | 舊快照使用未定義 <code>has_ticket</code> | NameError |

所有 71 檔 syntax 都合法，因此只跑 <code>py_compile</code> 抓不到這些問題。建議為 aggregate summaries 加 non-interactive smoke test；互動教材可將輸入邏輯包進函式並注入固定輸入。

### 3.5 多執行緒教材有兩個真正 API 錯誤

1. <code>Claude C++ STL多線程課程/第二階段：stdthread 基礎/課程 2.3：傳遞參數給執行緒2.cpp:4-14</code>

   <code>modify(int&amp;)</code> 不能由 <code>std::thread t(modify, value)</code> 呼叫。thread 會 decay-copy argument，之後以 rvalue invoke，無法綁定 non-const lvalue reference。這段不是「value 被複製，所以原值維持 1」；它根本無法編譯。

   修法：若要示範 copy，函式改收 <code>int</code>；若要修改原值，使用 <code>std::ref(value)</code>。

2. <code>Claude C++ STL多線程課程/第三階段：執行緒生命週期管理/課程 3.3：std jthread (C++20)4.cpp:13</code>

   <code>get_stop_source()</code> 回傳 value，不能綁到 non-const lvalue reference：

       std::stop_source& source = jt.get_stop_source();

   應改為 <code>auto source = jt.get_stop_source();</code>。

## 4. Medium：正確性與教材 contract

### 4.1 兩個 signed integer overflow

| 檔案與行 | 問題 | 修法 |
|---|---|---|
| <code>C++_Chrono/05_benchmark.cpp:187</code> | <code>i * i</code> 先用 int 計算，i=46341 即 overflow；之後加進 long long 太晚 | <code>1LL * i * i</code> 或一開始用 uint64_t |
| <code>C++_OOP/19_RAII.cpp:229-233</code> | 對 0..99999 做 int running sum，遠超 INT_MAX | vector 改為 int64_t，或縮小資料且說明範圍 |

兩者均由 UBSan 重現。前者會污染 throughput benchmark，後者會讓 RAII 教材在完全無關的位置執行 UB。

### 4.2 根層宣稱所有 <code>summary.cpp</code> 自包含且 C++17 可編譯，但 6/125 失敗

根層 <code>CLAUDE.md:75-81</code> 的 contract 是每個 summary 可用 <code>g++ -std=c++17</code> 編譯。完整測試結果：

| 檔案 | 原因 |
|---|---|
| <code>Claude C++面向對象/第 2 課：C 與 C++ 的關鍵差異/summary.cpp:399</code> | 缺 <code>&lt;cmath&gt;</code>，<code>std::sqrt</code> 未宣告 |
| <code>Claude C++面向對象/第 12 課：struct 與 class 的差異/summary.cpp:73,141</code> | 缺 <code>&lt;cmath&gt;</code>、<code>&lt;cstdint&gt;</code> |
| <code>Claude C++ STL課程/第 15 課：vector 元素刪除：pop_back、erase、clear/summary.cpp:246,253</code> | 使用 C++20 <code>std::erase/erase_if</code> |
| <code>Claude Code C++by Examples/C++_Cpp11/summary.cpp:208</code> | 缺 <code>&lt;cstdint&gt;</code> |
| <code>Claude Code C++by Examples/C++_Cpp17/summary.cpp:75</code> 起 | 缺 <code>&lt;vector&gt;</code>，並會連帶缺演算法宣告 |
| <code>Claude Code C++by Examples/C++_Utility/summary.cpp:329</code> | 缺 <code>&lt;cstdint&gt;</code> |

另外，非單課 summary 的 <code>Claude C++面向對象/summary6-12.cpp:268</code> 也缺 <code>&lt;cstdint&gt;</code>。

處理方式二選一：

- 保持根層 C++17 contract：C++20 區塊加 feature guard/fallback。
- 把 contract 改成「每檔明列最低標準」，並提供可機器讀取的 build manifest。

目前同一份文件既硬寫 C++17，又在 summary 主動執行 C++20 API，兩者不能同時成立。

### 4.3 其他非自包含或標準標示錯誤的 C++ 檔

完整跨 GCC/Clang 的 17 個失敗中，除上節已列者，還有：

| 檔案 | 分類與建議 |
|---|---|
| <code>Claude C++ STL多線程課程/第三階段：執行緒生命週期管理/課程 3.2：執行緒守衛類別設計3.cpp</code> | 缺 <code>&lt;thread&gt;</code>/<code>&lt;utility&gt;</code> 且沒有 main；若只是 snippet，改成 Markdown 或明標不獨立編譯 |
| <code>Claude C++ STL多線程課程/第三階段：執行緒生命週期管理/課程 3.3：std jthread (C++20)6.cpp</code> | API synopsis 使用未限定的 <code>stop_source/stop_token/id/native_handle_type</code> 且無 main |
| <code>Claude C++ STL多線程課程/第三階段：執行緒生命週期管理/課程 3.1：RAII 與執行緒管理5.cpp:258-286</code> | 長篇註解後遺留裸 pseudo-code，<code>work/condition/error/ScopedThread</code> 都未定義 |
| <code>Claude C++ STL課程/第 2 課：泛型編程（Generic Programming）概念/第二課：泛型編程（Generic Programming）概念10.cpp:6</code> | 使用 concept/requires，最低標準是 C++20，不是 C++17 |
| <code>Claude C++ STL課程/第 4 課：迭代器（Iterator）的核心概念/第四課：迭代器（Iterator）的核心概念10.cpp:32</code> | 缺 <code>&lt;algorithm&gt;</code>，<code>std::find</code> 未宣告 |
| <code>Claude C++ STL課程/第 20 課：vector 效能分析與最佳實踐/第 20 課：vector 效能分析與最佳實踐9.cpp:58</code> | <code>std::erase_if</code> 需 C++20 |
| <code>Claude C++ STL課程/第 40 課：list 與 forward_list 的選擇時機/summary.cpp:39</code> | abbreviated function template <code>auto parameter</code> 需 C++20 |
| <code>Claude C++ STL課程/第 15 課：vector 元素刪除：pop_back、erase、clear/第 15 課：vector 元素刪除：pop_back、erase、clear11.cpp:8,16</code> | <code>std::erase/erase_if</code> 需 C++20 |

建議每個 source 都有最低標準 metadata，驗證工具依 metadata 編譯，不再由路徑名稱猜測。

### 4.4 <code>fast_erase</code> 對空 vector 或非法 index 會出錯

重複出現在：

- <code>Claude C++ STL課程/第 15 課…/summary.cpp:263-269</code>
- 同課 <code>…13.cpp:542-548</code>

<code>v.size() - 1</code> 在空 vector 會 unsigned underflow；index 超界時仍執行 <code>pop_back()</code>，可能刪除錯元素；空 vector 的 pop_back 是 UB。

修法：函式進入後先處理 <code>if (index &gt;= v.size())</code>，明確選擇 throw、assert 或回 bool；通過後才做 swap-and-pop。

### 4.5 Filesystem summary 的說明、include 與編譯命令互相矛盾

<code>Claude Code C++by Examples/C++_Filesystem/summary.cpp:5,19-29,46-47</code>：

- 標題與命令宣稱 C++17 <code>&lt;filesystem&gt;</code>。
- 實際卻硬用 <code>&lt;experimental/filesystem&gt;</code>。
- 文件提供的 <code>g++ -std=c++17 ...</code> 命令沒有 <code>-lstdc++fs</code>，本機 link fail。

現代 C++17 工具鏈應直接用 <code>&lt;filesystem&gt;</code>/<code>std::filesystem</code>。若真的保留 legacy fallback，應以 build-system feature test 分流並為 experimental 路徑加入所需 library。

### 4.6 巨大配置不是穩定的 <code>bad_alloc</code> 教學

<code>C++_Exception/02_standard_exceptions.cpp:131-140</code> 用接近 <code>SIZE_MAX/2</code> 的 vector 強迫失敗。不同 library/allocator/sanitizer 可能：

- 丟 <code>length_error</code>；
- 丟 <code>bad_alloc</code>；
- 被 sanitizer 直接 abort；
- 受 OS overcommit 影響。

本輪 ASan 是 <code>allocation-size-too-big</code> 並直接中止。建議用可注入失敗的 allocator 或自訂 operator new test fixture，讓預期行為可重現；至少不要把它放在一般 smoke run。

## 5. CUDA 課筆記

### 5.1 RTX 5090 thread block cluster 說明錯誤

<code>Claude AI CUDA課程/第 01.02 課：GPU 硬體架構總覽/NOTE.md:380</code> 寫：

> RTX 5090 不支援（需 sm_90+），消費級卡沒有。

這句內部已自相矛盾：RTX 5090 是 sm_120，而 sm_120 當然符合「9.0 以上」。同 repo 黃金大綱 <code>CUDA_AI_完整課程大綱_v2.0.6.md:716-722</code> 也明寫 Blackwell 延續 clusters、5090 可跑。

NVIDIA 官方資料：

- [CUDA Programming Guide：CC 9.0+ 支援 thread block clusters](https://docs.nvidia.com/cuda/cuda-programming-guide/01-introduction/programming-model.html#thread-block-clusters)
- [Blackwell Tuning Guide：Blackwell GPUs 支援 thread block clusters](https://docs.nvidia.com/cuda/archive/13.0.3/blackwell-tuning-guide/index.html#thread-block-clusters)

應改為：sm_75 與 sm_89 不支援；Hopper sm_90 與 Blackwell sm_10x/sm_12x 支援，但最大 cluster shape/occupancy 要在目標裝置用 occupancy API 查詢。不能把「consumer」當成排除條件。

### 5.2 Block 排程結果被寫成保證

同一份 NOTE <code>:281-285</code> 把本機一次觀察到 160 blocks 在 40 SM 上剛好 4/SM，解讀為 scheduler round-robin，並寫「這不是巧合」。

CUDA 只保證 blocks 由可用 SM 排程；一般 kernel 無法控制或依賴特定 block-to-SM 順序/均分。官方文件也明說 scheduler 不提供特定順序或 scheme 保證：

- [CUDA Programming Guide：block scheduling 無順序保證](https://docs.nvidia.com/cuda/archive/13.1.0/cuda-programming-guide/02-basics/writing-cuda-kernels.html#kernel-launch-and-occupancy)

可以保留實測結果，但應改成「本次、此 kernel、此 GPU 的觀察」，並說明 residency 受 registers、shared memory、block size、其他工作與 scheduler 決定。

### 5.3 直接把 40,960 resident threads 當 Amdahl/Gustafson 的 N，需加醒目限制

位置：

- 課 1.1 <code>NOTE.md:69-90,180-193</code>
- <code>src/amdahl_gustafson.cpp:37-39,57-79</code>

公式本身正確，但 40,960 CUDA threads 不是 40,960 個等價、獨立、完整的 processors。Warp lockstep、issue width、occupancy、memory stalls、kernel resource usage 與 serial fraction 定義都會影響實際 speedup。Gustafson 的 36,864 倍只能是帶入公式的直覺模型，不能讀成此 GPU 的性能預測。

建議保留數學表，但把標題/輸出改為「假設 N 個等價 processing elements 的理論示意」；另列一段說明 GPU resident thread count 不能直接換算實測加速比。

## 6. Repository 與工具問題

### 6.1 195 個 extensionless ELF executable 已被 Git 追蹤

根層 <code>CLAUDE.md:7</code> 與 <code>README.md:280-285</code> 都宣稱只收原始碼/文件、不提交建置產物，但 <code>Claude Code C++by Examples</code> 內實際追蹤 195 個 PIE ELF executable，共約 10.32 MiB。

| 主題 | 數量 |
|---|---:|
| C++_String | 58 |
| C++_MultiThread | 32 |
| C++_Template | 28 |
| C++_OOP | 28 |
| C++_Container | 17 |
| C++_Iterator | 14 |
| C++_Utility | 13 |
| C++_SmartPointers | 5 |

根因是多個 Makefile 將輸出寫成 source basename（無 extension），而 <code>.gitignore</code> 只擋常見副檔名/build 目錄。結果：

- repository 攜帶平台專屬 binary；
- <code>make clean</code> 會刪 tracked files，立刻造成 dirty worktree；
- 舊 repo 曾因 build outputs 膨脹，但新 repo 又留下同類型資料。

建議：

1. 先由 Claude 確認後，將這 195 個 binary 自 Git 移除。
2. 所有 Makefile 改輸出到 ignored <code>build/</code> 或 <code>/tmp</code>。
3. CI 加入 <code>git ls-files | xargs file</code> guard，拒絕 tracked ELF/PE。
4. 不要只加一條寬泛 ignore pattern，避免誤擋合法 extensionless script/data。

### 6.2 VS Code compiler path 指向不存在的版本

<code>.vscode/settings.json:3</code> 寫死 <code>/usr/bin/g++-16</code>，本機不存在，實際可用的是 GCC 15（以及系統 alternative）。建議用 <code>/usr/bin/g++</code>，或由 workspace/compiler kit 管理；不要把未安裝 minor version 寫入跨機 workspace。

### 6.3 Algorithm 子文件路徑失效，shell loop 也不支援空白

<code>Claude Code C++by Examples/C++_Algorithm/CLAUDE.md:170-176</code>：

- 寫死不存在的 <code>/home/hsinan/C++_Algorithm</code>。
- <code>for f in $(find ...)</code> 會以 IFS 切割含空白/newline 的 path。

建議改成從文件所在 repo 推導 root，並使用 <code>find -print0</code> + NUL-safe loop；輸出建到 mktemp dir。

### 6.4 根層 CLAUDE.md 已與 README/實際目錄漂移

已確認：

- <code>CLAUDE.md:17</code> 寫 OOP 35 課；README 與目錄為 36。
- <code>:38-39</code> 寫兩條 CUDA 軌皆 0/185；README 已有新進度。
- <code>:48</code> 仍指向舊 <code>~/AI/AI_Course/AI_CUDA/...</code>，目前已拆成 <code>AI_CUDA_Claude</code>/<code>AI_CUDA_Codex</code>。
- Python 表仍把不存在的第 2 課當一列；README 已正確說明實際 31 個目錄。
- <code>:131</code> 的總數表因此也過期。

這不影響編譯，但會讓下一個 AI session 從錯誤 inventory 開始。

## 7. Low：值得一併改善

### 7.1 失效 pointer 仍被拿來比較或輸出

- <code>C++_Iterator/11_iterator_invalidation.cpp:85-91</code>
- <code>Claude C++ STL課程/第 17 課：vector 的記憶體重新配置機制/summary.cpp:117-125</code>

vector reallocation 後，舊 pointer 已不再指向其原 object。教材雖沒有解參考，仍在比較/輸出舊 pointer value。這會鼓勵「看地址判定失效」的錯誤模型；iterator/pointer 是否失效由標準規則決定。

建議在 mutation 前輸出舊地址，mutation 後只輸出新地址並文字說明「舊 pointer 不再使用」。若只是想比較 allocator 行為，可在失效前將 representation 轉成僅供診斷的整數，並明說這不是有效性測試。

### 7.2 Parallel STL 的依賴應進入 build metadata

<code>C++_MultiThread/09_parallel_stl.cpp</code>、<code>25_parallel_mergesort.cpp</code>、<code>26_parallel_scan.cpp</code> 的 source 註解都有 <code>-ltbb</code>，這點正確；但一般全庫編譯工具若只用 <code>g++ file.cpp</code> 會 link fail。

建議把 TBB 放進 Makefile/manifest，而不是只留在檔頭。這樣「全庫驗證」能區分 dependency missing 與 source broken。

### 7.3 編譯標準應成為每檔明確資訊

Concepts、<code>std::erase_if</code>、abbreviated template、<code>resize_and_overwrite</code> 等範例跨 C++17/20/23。檔名或章名偶爾有標示，整體沒有一致機器可讀規則。

建議至少在每檔前 20 行使用一致欄位：

    Minimum standard: C++20
    Extra libraries: tbb
    Expected sanitizer failure: no

再由驗證腳本解析或維護 manifest。

## 8. 已驗證為正確、建議保留

- CUDA 課 1.1/1.2 的三個 <code>.cu</code> 均可編成 sm_75、sm_89、sm_120 fat targets；native 執行與 compute-sanitizer memcheck 通過。
- 課 1.1 已改用 <code>cudaDeviceGetAttribute(cudaDevAttrClockRate)</code>，避開 CUDA 13 移除舊 struct member 的問題。
- Python 71 檔全部 syntax 正確，AST 掃描未發現 mutable default、bare except、eval/exec。
- Markdown/PDF 全部可讀，現有 local Markdown links 無斷鏈。
- 多執行緒課對 TSan、<code>-ltbb</code> 與 parallel callback 限制的文字大致清楚。
- 許多危險範例已只放在註解、不在 default path 執行；應把第 3.3 節兩檔也統一成此模式。

## 9. 建議修復順序與回歸門檻

### P0：先修會執行 UB / runtime exception

1. minmax dangling references。
2. 兩個 invalid pointer subtraction。
3. Chrono/RAII signed overflow。
4. Python 3 個 active 檔 runtime exception。
5. thread argument 與 jthread stop_source 編譯錯。
6. virtual destructor/static_cast 反例改為 opt-in。

門檻：targeted GCC/Clang + ASan/UBSan/pointer-subtract 全過。

### P1：恢復教材的可編譯 contract

1. 修 6 個 C++17 summary。
2. snippet 改成 self-contained 或更換副檔名/明標。
3. 每檔最低標準與 TBB 依賴進 manifest。
4. filesystem summary 改標準 API。

門檻：1,230 個 C++ source 依 manifest 編譯；125 summaries 全部符合其宣稱標準。

### P2：文件與 repository hygiene

1. 修 CUDA cluster/scheduler 措辭與 Amdahl 模型邊界。
2. 移除 tracked ELF，改 build output 位置。
3. 修 compiler path、Algorithm 驗證命令與根層 CLAUDE inventory。

門檻：Git tracked files 中無 ELF/PE build output；<code>make clean</code> 後 worktree 仍乾淨。

## 10. 建議的驗證命令

下列是方向示例，Claude 修正時應整合成 repository 工具，不要永久把 binary 寫進 source tree。

    # 單檔一般驗證
    g++ -std=c++20 -Wall -Wextra -Wpedantic -pthread file.cpp -o /tmp/example
    /tmp/example

    # 記憶體與 UB
    g++ -std=c++20 -O1 -g -pthread \
      -fsanitize=address,undefined -fno-omit-frame-pointer \
      file.cpp -o /tmp/example_san
    /tmp/example_san

    # 不相關 pointer 相減
    g++ -std=c++17 -O1 -g \
      -fsanitize=address,pointer-subtract -fno-omit-frame-pointer \
      file.cpp -o /tmp/example_ptr
    ASAN_OPTIONS=detect_invalid_pointer_pairs=2 /tmp/example_ptr

    # Python syntax 只是第一層，仍需 runtime smoke test
    python3 -m py_compile file.py
    python3 file.py

    # 找 tracked native executable
    git ls-files -z |
      while IFS= read -r -d '' file; do
        file --brief -- "$file"
      done

---

本報告只記錄可由原始碼、編譯器、runtime 工具或官方文件支持的問題。沒有要求 Claude 接受 Codex 的教學風格；目標是讓 Claude 原課程本身符合它已宣稱的編譯、執行與正確性 contract。
