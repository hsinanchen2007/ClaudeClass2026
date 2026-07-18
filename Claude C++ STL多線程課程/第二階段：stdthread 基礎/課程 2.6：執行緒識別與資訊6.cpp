/*
# 第二階段：std::thread 基礎

## 課程 2.6：執行緒識別與資訊

---

### 引言

有時我們需要識別不同的執行緒，或查詢系統的硬體資訊來決定建立多少執行緒。本課介紹這些實用的工具。

---

### 一、取得執行緒 ID

每個執行緒都有唯一的識別碼：

```cpp
#include <iostream>
#include <thread>

int main() {
    // 取得主執行緒 ID
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << std::endl;
    
    std::thread t([]() {
        // 在執行緒內部取得自己的 ID
        std::cout << "子執行緒 ID: " << std::this_thread::get_id() << std::endl;
    });
    
    // 從外部取得執行緒 ID
    std::cout << "t 的 ID: " << t.get_id() << std::endl;
    
    t.join();
    return 0;
}
```

輸出：
```
主執行緒 ID: 140234567891520
子執行緒 ID: 140234567891521
t 的 ID: 140234567891521
```

---

### 二、兩種取得 ID 的方式

```
┌─────────────────────────────────────────────────────────────┐
│                取得執行緒 ID 的兩種方式                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::this_thread::get_id()                                 │
│  → 在執行緒內部呼叫                                          │
│  → 取得「當前執行緒」的 ID                                   │
│                                                             │
│  thread_object.get_id()                                     │
│  → 在執行緒外部呼叫                                          │
│  → 取得「指定執行緒物件」的 ID                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 三、std::thread::id 類型

執行緒 ID 是特殊類型，可以比較和輸出：

```cpp
#include <iostream>
#include <thread>

std::thread::id mainThreadId;

void checkThread() {
    if (std::this_thread::get_id() == mainThreadId) {
        std::cout << "這是主執行緒" << std::endl;
    } else {
        std::cout << "這是子執行緒" << std::endl;
    }
}

int main() {
    mainThreadId = std::this_thread::get_id();
    
    checkThread();  // 在主執行緒呼叫
    
    std::thread t(checkThread);  // 在子執行緒呼叫
    t.join();
    
    return 0;
}
```

輸出：
```
這是主執行緒
這是子執行緒
```

---

### 四、hardware_concurrency()

查詢系統支援的並行執行緒數（通常等於 CPU 核心數）：

```cpp
#include <iostream>
#include <thread>

int main() {
    unsigned int cores = std::thread::hardware_concurrency();
    
    std::cout << "硬體支援的並行執行緒數: " << cores << std::endl;
    
    // 可能回傳 0 表示無法偵測
    if (cores == 0) {
        std::cout << "無法偵測，使用預設值" << std::endl;
        cores = 2;
    }
    
    return 0;
}
```

---

### 五、實際應用：根據核心數分配工作

```cpp
#include <iostream>
#include <thread>
#include <vector>

int main() {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    
    std::cout << "建立 " << numThreads << " 個執行緒" << std::endl;
    
    std::vector<std::thread> threads;
    
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i]() {
            std::cout << "執行緒 " << i 
                      << " (ID: " << std::this_thread::get_id() << ")" 
                      << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

---

### 六、std::this_thread 命名空間總覽

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 1. get_id() - 取得當前執行緒 ID
    std::cout << "ID: " << std::this_thread::get_id() << std::endl;
    
    // 2. sleep_for() - 休眠指定時間
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 3. sleep_until() - 休眠到指定時間點
    auto wakeTime = std::chrono::steady_clock::now() 
                  + std::chrono::milliseconds(100);
    std::this_thread::sleep_until(wakeTime);
    
    // 4. yield() - 讓出 CPU 時間給其他執行緒
    std::this_thread::yield();
    
    return 0;
}
```

---

### 七、yield() 的用途

`yield()` 提示作業系統讓其他執行緒先執行：

```cpp
#include <iostream>
#include <thread>
#include <atomic>

std::atomic<bool> ready{false};

void waitForReady() {
    while (!ready) {
        std::this_thread::yield();  // 避免忙等待浪費 CPU
    }
    std::cout << "Ready!" << std::endl;
}

int main() {
    std::thread t(waitForReady);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ready = true;
    
    t.join();
    return 0;
}
```

---

### 八、常用功能對照表

| 功能 | 語法 | 說明 |
|------|------|------|
| 取得當前執行緒 ID | `std::this_thread::get_id()` | 回傳 `std::thread::id` |
| 取得執行緒物件的 ID | `t.get_id()` | 回傳 `std::thread::id` |
| 查詢 CPU 核心數 | `std::thread::hardware_concurrency()` | 回傳 `unsigned int` |
| 休眠一段時間 | `std::this_thread::sleep_for(duration)` | 阻塞當前執行緒 |
| 休眠到指定時間 | `std::this_thread::sleep_until(time_point)` | 阻塞當前執行緒 |
| 讓出 CPU | `std::this_thread::yield()` | 提示排程器 |

---

### 九、本課重點回顧

1. `std::this_thread::get_id()` 取得當前執行緒的 ID
2. `t.get_id()` 取得執行緒物件 t 的 ID
3. `std::thread::id` 類型可以比較和輸出
4. `hardware_concurrency()` 回傳 CPU 核心數（可能為 0）
5. `sleep_for()` 和 `sleep_until()` 用於休眠
6. `yield()` 讓出 CPU 時間，避免忙等待

---

### 下一課預告

在 **課程 2.7：執行緒的移動語意** 中，我們將學習：
- 為什麼 `std::thread` 不能複製
- 如何使用 `std::move` 轉移執行緒所有權
- 執行緒容器的管理

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <atomic>

std::atomic<bool> ready{false};

void waitForReady() {
    while (!ready) {
        std::this_thread::yield();  // 避免忙等待浪費 CPU
    }
    std::cout << "Ready!" << std::endl;
}

int main() {
    std::thread t(waitForReady);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ready = true;
    
    t.join();
    return 0;
}
