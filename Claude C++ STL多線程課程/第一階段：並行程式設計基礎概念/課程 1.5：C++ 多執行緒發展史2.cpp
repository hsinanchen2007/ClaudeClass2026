/*
# 第一階段：並行程式設計基礎概念

## 課程 1.5：C++ 多執行緒發展史

---

### 引言

C++ 直到 2011 年才在語言標準中正式支援多執行緒。在此之前，程式設計師只能依賴平台特定的 API。了解這段歷史，能幫助你理解為什麼現代 C++ 的多執行緒設計是這樣的。

---

### 一、C++11 之前的黑暗時代

#### 平台特定的解決方案

```
┌─────────────────────────────────────────────────────────────┐
│                C++11 之前的多執行緒                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Windows                    POSIX (Linux/macOS/Unix)        │
│  ─────────────────         ─────────────────────────        │
│  CreateThread()            pthread_create()                 │
│  WaitForSingleObject()     pthread_join()                   │
│  EnterCriticalSection()    pthread_mutex_lock()             │
│  LeaveCriticalSection()    pthread_mutex_unlock()           │
│                                                             │
│  完全不同的 API，程式碼無法跨平台！                           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### POSIX Threads 範例（舊式寫法）

```cpp
// C++11 之前的 POSIX 多執行緒寫法
#include <pthread.h>
#include <stdio.h>

void* threadFunction(void* arg) {
    int* value = (int*)arg;
    printf("執行緒收到值: %d\n", *value);
    return NULL;
}

int main() {
    pthread_t thread;
    int value = 42;
    
    // 建立執行緒
    pthread_create(&thread, NULL, threadFunction, &value);
    
    // 等待執行緒結束
    pthread_join(thread, NULL);
    
    return 0;
}
```

**問題**：
- 需要手動管理記憶體與資源
- 型別不安全（使用 `void*`）
- 無法跨平台
- 容易出錯

---

### 二、C++11：標準多執行緒的誕生

C++11 引入了完整的多執行緒支援，這是 C++ 歷史上最重要的更新之一。

#### C++11 新增的標頭檔

```cpp
#include <thread>              // std::thread
#include <mutex>               // std::mutex, std::lock_guard
#include <condition_variable>  // std::condition_variable
#include <future>              // std::future, std::promise, std::async
#include <atomic>              // std::atomic
```

#### 現代 C++ 寫法對比

```cpp
// C++11 的現代寫法
#include <iostream>
#include <thread>

void threadFunction(int value) {
    std::cout << "執行緒收到值: " << value << std::endl;
}

int main() {
    std::thread t(threadFunction, 42);  // 型別安全！
    t.join();
    return 0;
}
```

**優點**：
- 型別安全
- 跨平台
- 支援 Lambda
- RAII 風格的資源管理

---

### 三、C++ 標準演進時間線

```
┌────────────────────────────────────────────────────────────────┐
│                    C++ 多執行緒演進史                           │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  C++11 (2011) ─── 奠基                                         │
│  │  • std::thread                                              │
│  │  • std::mutex, std::lock_guard                              │
│  │  • std::condition_variable                                  │
│  │  • std::future, std::promise, std::async                    │
│  │  • std::atomic                                              │
│  │  • 記憶體模型 (Memory Model)                                 │
│  │                                                             │
│  C++14 (2014) ─── 小幅改進                                     │
│  │  • std::shared_timed_mutex（讀寫鎖）                         │
│  │  • std::shared_lock                                         │
│  │                                                             │
│  C++17 (2017) ─── 重要擴展                                     │
│  │  • std::shared_mutex                                        │
│  │  • std::scoped_lock（多重鎖）                                │
│  │  • 平行演算法（Parallel Algorithms）                         │
│  │                                                             │
│  C++20 (2020) ─── 大幅增強                                     │
│  │  • std::jthread（自動 join）                                 │
│  │  • std::counting_semaphore                                  │
│  │  • std::latch, std::barrier                                 │
│  │  • std::stop_token（協作式取消）                             │
│  │  • 協程（Coroutines）                                       │
│  │                                                             │
│  C++23 (2023) ─── 持續完善                                     │
│     • 更多原子操作改進                                          │
│     • std::expected（錯誤處理）                                 │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

### 四、各版本關鍵功能速覽

#### C++11：基礎建設

```cpp
// 執行緒
std::thread t(func);

// 互斥鎖
std::mutex mtx;
std::lock_guard<std::mutex> lock(mtx);

// 非同步
auto future = std::async(std::launch::async, func);
```

#### C++17：便利性提升

```cpp
// 同時鎖定多個互斥鎖（避免死結）
std::scoped_lock lock(mutex1, mutex2, mutex3);

// 平行演算法
std::sort(std::execution::par, vec.begin(), vec.end());
```

#### C++20：現代化完善

```cpp
// 自動 join 的執行緒
std::jthread t(func);  // 解構時自動 join

// 信號量
std::counting_semaphore<10> sem(5);

// 協作式取消
std::jthread t([](std::stop_token token) {
    while (!token.stop_requested()) {
        // 工作...
    }
});
t.request_stop();
```

---

### 五、為什麼要了解這段歷史？

```
┌─────────────────────────────────────────────────────────────┐
│                 了解歷史的實際價值                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 閱讀舊程式碼                                             │
│     許多專案仍使用 POSIX 或 Windows API                      │
│                                                             │
│  2. 理解設計決策                                             │
│     知道為什麼 C++ 標準庫是這樣設計的                        │
│                                                             │
│  3. 選擇正確工具                                             │
│     根據編譯器支援程度選擇適當的功能                         │
│                                                             │
│  4. 漸進式升級                                               │
│     了解如何將舊程式碼遷移到現代 C++                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、編譯器支援參考

| 功能 | GCC | Clang | MSVC |
|------|-----|-------|------|
| C++11 多執行緒 | 4.8+ | 3.3+ | 2012+ |
| C++17 平行演算法 | 9+ | 10+ | 2017+ |
| C++20 jthread | 10+ | 12+ | 2019+ |
| C++20 協程 | 10+ | 12+ | 2019+ |

---

### 七、本課重點回顧

1. C++11 之前，多執行緒只能依賴平台特定 API（POSIX、Windows）
2. C++11 首次將多執行緒納入語言標準，提供跨平台支援
3. C++14/17/20 持續增強，加入讀寫鎖、平行演算法、協程等功能
4. 現代 C++ 的多執行緒設計強調型別安全與 RAII
5. 本課程將涵蓋 C++11 到 C++20 的所有重要功能

---

### 下一課預告

在 **課程 1.6：標準函式庫總覽** 中，我們將：
- 完整介紹 C++ 多執行緒相關的標頭檔
- 預覽各個重要類別與函式
- 建立整個課程的學習地圖

---

準備好繼續嗎？
*/


// C++11 的現代寫法
#include <iostream>
#include <thread>

void threadFunction(int value) {
    std::cout << "執行緒收到值: " << value << std::endl;
}

int main() {
    std::thread t(threadFunction, 42);  // 型別安全！
    t.join();
    return 0;
}
