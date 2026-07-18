# C++ Lambda / std::function / std::bind 學習專案

## 使用者需求 / 規則

> 這 5 條是專案永久規則，每次對話都要遵守。

1. **永遠用繁體中文回答** — 不管使用者是用中文還是英文發問，回覆一律用繁體中文。
2. **每個主題加入「簡單實用」的 LeetCode 範例** — 不要那種為了考演算法刻意刁鑽、實務用不到的題目；要選日常開發 / 面試 warm-up 等級的題目，加進對應的 `.cpp` 檔案，且要附**完整詳細的註釋**。
3. **每個程式只談一個主題** — 一個 `.cpp` 檔只聚焦一個 lambda 子主題，並在程式碼內附上**詳細完整的概念解釋**。註釋的份量遠超過程式碼是 OK 的。
4. **範例要簡單直接** — 不要寫艱澀冗長的 C++、不要為了炫技；範例小、好懂、跑得起來最重要。
5. **參考來源** —
   - https://en.cppreference.com/cpp
   - https://cplusplus.com/reference/

## 專案結構

```
C++_Lambda/
├── CLAUDE.md
├── Makefile
├── 01_basic_lambda.cpp           # 基本語法、編譯器產生的 closure type
├── 02_capture_modes.cpp          # [], [=], [&], [this], [x], [&x], 混合捕獲
├── 03_mutable_lambda.cpp         # mutable 關鍵字、修改 by-value 捕獲
├── 04_init_capture.cpp           # C++14 init capture, move-only 物件捕獲
├── 05_generic_lambda.cpp         # C++14 auto 參數, C++20 template lambda
├── 06_std_function.cpp           # std::function 包裝、type erasure 應用
├── 07_function_pointer.cpp       # function pointer、std::mem_fn、無捕獲 lambda 可隱式轉
├── 08_std_bind.cpp               # std::bind、placeholders、為何現代偏好 lambda
├── 09_lambda_in_algorithm.cpp    # 配合 STL 演算法（含 LeetCode 56、215、347）
└── 10_lambda_pitfalls.cpp        # 懸掛引用、this 捕獲危險、return type
```

## 各檔案 LeetCode 對應

| 檔案 | LeetCode |
|---|---|
| 01_basic_lambda | LC1768 Merge Strings Alternately（最簡單的 lambda 排序熱身） |
| 02_capture_modes | LC1431 Kids With the Greatest Number of Candies |
| 03_mutable_lambda | LC1480 Running Sum of 1d Array（用 mutable 累積狀態） |
| 04_init_capture | （說明 unique_ptr 捕獲，無 LC） |
| 05_generic_lambda | LC1108 Defanging an IP Address |
| 06_std_function | LC700 Search in BST（遞迴 lambda 用 std::function） |
| 07_function_pointer | LC負例：捕獲後不能轉 function pointer |
| 08_std_bind | （示範 bind 與 lambda 對照） |
| 09_lambda_in_algorithm | LC56 Merge Intervals / LC215 Kth Largest / LC347 Top K Frequent |
| 10_lambda_pitfalls | LC202 Happy Number（搭配 set 與 lambda 一起用會踩到的坑） |

## 編譯方式

```bash
make            # 編譯全部
make run        # 編譯並依序執行
make 01         # 只編譯 01
make clean      # 清掉執行檔
```
