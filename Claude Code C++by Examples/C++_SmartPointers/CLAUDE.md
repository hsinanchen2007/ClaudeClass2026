# CLAUDE.md — 專案規範

本專案專注於 **C++ STL Smart Pointers (智慧指標)** 的學習。
跳過 C++20 之後新增, 以及不常用或不實用的智慧指標
(例如已被移除的 `std::auto_ptr`)。

## 協作規則 (Claude 必須遵守)

### 1. 語言

- **永遠使用「繁體中文」回答使用者的問題**, 無論使用者以中文或英文提問。
- 程式碼註釋同樣以「繁體中文」撰寫。

### 2. Leetcode 應用題目

- 每個主題都要參考一道相對應的 **Leetcode 題目**, 並把題解整合進該主題的 C++ 程式裡, 達到「學以致用」。
- 題目挑選原則:
  - **簡單、實用** 為主 (Easy / 偶爾 Medium)。
  - 避開為了考演算法而設計的刁鑽、不實用題目。
  - 題目本身要能自然體現該智慧指標的特性 (例如 `weak_ptr` 配合圖/樹的循環引用)。
- 必須附上「**完整詳細的中文註釋**」, 包含:
  - 題目描述
  - 解題思路
  - 為什麼這題適合用該智慧指標展示
  - 程式碼逐段解釋

### 3. 一個程式只談一個主題

- 每個 `.cpp` 檔案只專注於 **一個主題**。
- 每個主題在程式檔案最上方都必須有 **詳盡完整的課題介紹與觀念解釋**, 放在區塊註釋中:
  - 該智慧指標的定義、用途、語意 (ownership semantics)
  - 何時該用、何時不該用
  - 與其他智慧指標的差異
  - 常見誤用與陷阱
  - 底層實作概念 (例如 `shared_ptr` 的 control block)
- **註釋遠多過程式碼是被允許且鼓勵的** — 重點是清楚的觀念傳達。

### 4. 程式範例風格

- 所有程式範例都要 **簡單直接**。
- **不要** 寫艱澀的 C++ 範例, 也不要寫很長的程式碼。
- 寧可分多個小範例, 也不要塞一個大範例。

### 5. 實用日常範例

- 除了 Leetcode 題目, 若有「每天工作中可能會用到」的實用情境, 也鼓勵加入程式範例。
- 同樣需要 **簡潔簡單**, 並附上 **完整詳細的中文解說註釋**。
- 範例情境例如: 資源管理、PIMPL idiom、工廠函式、觀察者模式、快取等。

### 6. 參考來源

- 所有 C/C++ 相關參考, 以以下兩個官方/權威網站為準:
  - <https://en.cppreference.com/w/cpp/memory>  (智慧指標主題)
  - <https://en.cppreference.com/w/cpp>          (C++ 全主題)
  - <https://cplusplus.com/reference/>
- 在註釋中如有引用, 註明來源連結方便使用者查證。

## 主題清單

| 編號 | 主題          | 檔案                              | Leetcode 對應 / 實用情境 |
| ---- | ------------- | --------------------------------- | ----------------------- |
| 01   | `unique_ptr`  | `01_unique_ptr.cpp`               | 21. Merge Two Sorted Lists |
| 02   | `shared_ptr`  | `02_shared_ptr.cpp`               | 138. Copy List with Random Pointer |
| 03   | `weak_ptr`    | `03_weak_ptr.cpp`                 | 133. Clone Graph (循環引用情境) |
| 04   | `enable_shared_from_this` | `04_enable_shared_from_this.cpp` | 非同步 callback / 觀察者 / Builder (實務情境) |
| 05   | Custom Deleter | `05_custom_deleter.cpp`          | FILE* / 物件池 / aliasing constructor (實務情境) |

## 編譯方式

```bash
g++ -std=c++17 -Wall -Wextra -o demo 01_unique_ptr.cpp
./demo
```
