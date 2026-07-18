/*
# 第一階段：並行程式設計基礎概念

## 課程 1.6：標準函式庫總覽

---

### 引言

在正式開始撰寫多執行緒程式之前，讓我們先鳥瞰整個 C++ 多執行緒標準函式庫的全貌。這將成為你後續學習的地圖。

---

### 一、標頭檔總覽

```
┌─────────────────────────────────────────────────────────────┐
│              C++ 多執行緒標準函式庫                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  <thread>          執行緒的建立與管理                        │
│                                                             │
│  <mutex>           互斥鎖與鎖管理器                          │
│                                                             │
│  <condition_variable>  條件變數，執行緒間的等待與通知        │
│                                                             │
│  <future>          非同步操作與結果傳遞                      │
│                                                             │
│  <atomic>          原子操作，無鎖程式設計                    │
│                                                             │
│  <semaphore>       信號量 (C++20)                           │
│                                                             │
│  <latch>           閂鎖 (C++20)                             │
│                                                             │
│  <barrier>         屏障 (C++20)                             │
│                                                             │
│  <stop_token>      協作式取消 (C++20)                        │
│                                                             │
│  <coroutine>       協程支援 (C++20)                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、各標頭檔核心內容

#### `<thread>` — 執行緒管理

```cpp
#include <thread>

std::thread              // 執行緒類別
std::this_thread         // 當前執行緒的操作命名空間
  ::get_id()             //   取得執行緒 ID
  ::sleep_for()          //   休眠指定時間
  ::sleep_until()        //   休眠到指定時間點
  ::yield()              //   讓出 CPU 時間

std::jthread             // 自動 join 的執行緒 (C++20)
```

#### `<mutex>` — 互斥鎖

```cpp
#include <mutex>

// 互斥鎖類型
std::mutex                    // 基本互斥鎖
std::timed_mutex              // 支援超時的互斥鎖
std::recursive_mutex          // 可遞迴鎖定的互斥鎖
std::recursive_timed_mutex    // 遞迴 + 超時
std::shared_mutex             // 讀寫鎖 (C++17)

// 鎖管理器（RAII）
std::lock_guard              // 簡單的 RAII 鎖
std::unique_lock             // 靈活的 RAII 鎖
std::scoped_lock             // 多重鎖 (C++17)
std::shared_lock             // 共享鎖 (C++14)

// 輔助函式
std::lock()                  // 同時鎖定多個互斥鎖
std::try_lock()              // 嘗試同時鎖定
std::call_once               // 確保只執行一次
std::once_flag               // call_once 的旗標
```

#### `<condition_variable>` — 條件變數

```cpp
#include <condition_variable>

std::condition_variable      // 條件變數（搭配 unique_lock）
std::condition_variable_any  // 通用條件變數（任意鎖）

// 主要操作
.wait()                      // 等待通知
.wait_for()                  // 等待指定時間
.wait_until()                // 等待到指定時間點
.notify_one()                // 喚醒一個等待者
.notify_all()                // 喚醒所有等待者
```

#### `<future>` — 非同步操作

```cpp
#include <future>

std::future                  // 非同步結果的接收端
std::promise                 // 非同步結果的發送端
std::shared_future           // 可複製的 future
std::packaged_task           // 包裝可呼叫物件為非同步任務
std::async                   // 高階非同步執行函式

// 啟動策略
std::launch::async           // 強制新執行緒
std::launch::deferred        // 延遲執行
```

#### `<atomic>` — 原子操作

```cpp
#include <atomic>

std::atomic<T>               // 原子類型模板
std::atomic_flag             // 最基本的原子布林
std::atomic_ref              // 對既有物件的原子操作 (C++20)

// 記憶體順序
std::memory_order_relaxed
std::memory_order_acquire
std::memory_order_release
std::memory_order_acq_rel
std::memory_order_seq_cst
```

#### `<semaphore>` — 信號量 (C++20)

```cpp
#include <semaphore>

std::counting_semaphore<N>   // 計數信號量
std::binary_semaphore        // 二元信號量（等同 counting_semaphore<1>）
```

#### `<latch>` 與 `<barrier>` — 同步屏障 (C++20)

```cpp
#include <latch>
#include <barrier>

std::latch                   // 一次性倒數閂鎖
std::barrier                 // 可重複使用的屏障
```

---

### 三、學習路線圖

```
                        ┌─────────────┐
                        │  你在這裡   │
                        └──────┬──────┘
                               │
            ┌──────────────────┼──────────────────┐
            ▼                  ▼                  ▼
     ┌────────────┐    ┌─────────────┐    ┌─────────────┐
     │  <thread>  │    │   <mutex>   │    │  <atomic>   │
     │  執行緒基礎 │    │   同步機制   │    │   原子操作   │
     └─────┬──────┘    └──────┬──────┘    └──────┬──────┘
           │                  │                  │
           ▼                  ▼                  ▼
     ┌────────────┐    ┌─────────────┐    ┌─────────────┐
     │  <future>  │    │ <condition_ │    │  無鎖資料   │
     │  非同步操作 │    │  variable>  │    │    結構     │
     └─────┬──────┘    └──────┬──────┘    └──────┬──────┘
           │                  │                  │
           └──────────────────┼──────────────────┘
                              ▼
                    ┌──────────────────┐
                    │  C++20 進階功能   │
                    │  semaphore/latch │
                    │  barrier/jthread │
                    │    coroutine     │
                    └──────────────────┘
```

---

### 四、快速參考表

| 我想要... | 使用... | 標頭檔 |
|-----------|---------|--------|
| 建立執行緒 | `std::thread` | `<thread>` |
| 保護共享資料 | `std::mutex` + `std::lock_guard` | `<mutex>` |
| 等待某個條件 | `std::condition_variable` | `<condition_variable>` |
| 執行非同步任務 | `std::async` | `<future>` |
| 無鎖計數器 | `std::atomic<int>` | `<atomic>` |
| 限制並發數量 | `std::counting_semaphore` | `<semaphore>` |
| 等待多執行緒到達同一點 | `std::barrier` | `<barrier>` |

---

### 五、最小可運行範例

將所有核心功能濃縮在一個範例中：

```cpp
// 檔案：lesson_1_6_overview.cpp

#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>

std::mutex mtx;
std::atomic<int> atomicCounter{0};

int main() {
    // 1. std::thread - 建立執行緒
    std::thread t([]() {
        std::lock_guard<std::mutex> lock(mtx);  // 2. std::mutex - 保護輸出
        std::cout << "Hello from thread!" << std::endl;
    });
    
    // 3. std::async - 非同步執行
    auto future = std::async(std::launch::async, []() {
        return 42;
    });
    
    // 4. std::atomic - 原子操作
    atomicCounter++;
    
    t.join();
    std::cout << "Async result: " << future.get() << std::endl;
    std::cout << "Atomic counter: " << atomicCounter << std::endl;
    
    return 0;
}
```

```bash
g++ -std=c++17 -pthread -o lesson_1_6 lesson_1_6_overview.cpp
./lesson_1_6
```

---

### 六、本課重點回顧

1. C++ 多執行緒功能分布在多個標頭檔中
2. `<thread>` 負責執行緒的建立與管理
3. `<mutex>` 提供各種互斥鎖和 RAII 鎖管理器
4. `<condition_variable>` 實現執行緒間的等待與通知
5. `<future>` 支援非同步程式設計
6. `<atomic>` 提供無鎖的原子操作
7. C++20 新增了信號量、閂鎖、屏障等同步原語

---

### 第一階段完成！

恭喜你完成了第一階段的學習！你已經建立了多執行緒程式設計的概念基礎：

- ✅ 並發與並行的區別
- ✅ 多執行緒的價值與適用場景
- ✅ 執行緒與程序的差異
- ✅ 常見的多執行緒問題
- ✅ C++ 多執行緒的發展歷程
- ✅ 標準函式庫的整體架構

---

### 下一階段預告

**第二階段：std::thread 基礎** 將帶你實際動手：
- 課程 2.1：第一個多執行緒程式
- 課程 2.2：執行緒函式的多種形式
- 課程 2.3：傳遞參數給執行緒
- ...

準備好進入第二階段了嗎？
*/


// 檔案：lesson_1_6_overview.cpp

#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>

std::mutex mtx;
std::atomic<int> atomicCounter{0};

int main() {
    // 1. std::thread - 建立執行緒
    std::thread t([]() {
        std::lock_guard<std::mutex> lock(mtx);  // 2. std::mutex - 保護輸出
        std::cout << "Hello from thread!" << std::endl;
    });
    
    // 3. std::async - 非同步執行
    auto future = std::async(std::launch::async, []() {
        return 42;
    });
    
    // 4. std::atomic - 原子操作
    atomicCounter++;
    
    t.join();
    std::cout << "Async result: " << future.get() << std::endl;
    std::cout << "Atomic counter: " << atomicCounter << std::endl;
    
    return 0;
}
