/*
# 第五階段：互斥鎖基礎 (std::mutex)

## 課程 5.3：try_lock() 非阻塞鎖定

---

### 引言

在前兩課中，我們學習了 `lock()` 會阻塞等待直到獲取鎖。但有時候，我們不希望執行緒傻傻地等待，而是想「試試看能不能拿到鎖，拿不到就做別的事」。這就是 `try_lock()` 的用途。

---

### 一、lock() vs try_lock()

```
┌─────────────────────────────────────────────────────────────┐
│                  lock() vs try_lock()                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  lock()                           try_lock()                │
│  ──────                           ──────────                │
│                                                             │
│  「我一定要拿到鎖」                「試試看能不能拿到」        │
│                                                             │
│  ┌─────────┐                     ┌─────────┐               │
│  │ lock()  │                     │try_lock│               │
│  └────┬────┘                     └────┬────┘               │
│       │                               │                     │
│       ▼                        ┌──────┴──────┐              │
│  ┌─────────┐                   │             │              │
│  │鎖可用？ │                 成功          失敗              │
│  └────┬────┘               (true)        (false)            │
│       │                       │             │               │
│  ┌────┴────┐                  ▼             ▼               │
│  │         │             進入臨界區段    立即返回             │
│ 是        否                            做其他事             │
│  │         │                                                │
│  ▼         ▼                                                │
│ 獲得鎖   阻塞等待                                            │
│         （可能很久）                                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

| 方法 | 行為 | 返回值 | 阻塞 |
|------|------|--------|------|
| `lock()` | 獲取鎖，失敗則等待 | 無（void） | 是 |
| `try_lock()` | 嘗試獲取鎖，失敗則立即返回 | `true`/`false` | 否 |
| `unlock()` | 釋放鎖 | 無（void） | 否 |

---

### 二、try_lock() 基本用法

```cpp
// 檔案：lesson_5_3_try_lock_basic.cpp
// 說明：try_lock() 的基本使用方式

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

void worker(int id) {
    // 嘗試獲取鎖
    if (mtx.try_lock()) {
        // 成功獲取鎖
        std::cout << "執行緒 " << id << "：成功獲取鎖，開始工作..." << std::endl;
        
        // 模擬工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "執行緒 " << id << "：工作完成，釋放鎖" << std::endl;
        mtx.unlock();  // 記得解鎖！
    } else {
        // 獲取失敗，立即返回
        std::cout << "執行緒 " << id << "：無法獲取鎖，執行其他任務" << std::endl;
    }
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o try_lock_basic lesson_5_3_try_lock_basic.cpp
./try_lock_basic
```

#### 可能的輸出

```
執行緒 1：成功獲取鎖，開始工作...
執行緒 2：無法獲取鎖，執行其他任務
執行緒 3：無法獲取鎖，執行其他任務
執行緒 1：工作完成，釋放鎖
```

---

### 三、重複嘗試模式

有時我們想重複嘗試，但不想無限等待：

```cpp
// 檔案：lesson_5_3_retry_pattern.cpp
// 說明：使用 try_lock() 實現有限次數的重試

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;
int shared_data = 0;

bool tryUpdateWithRetry(int id, int maxRetries) {
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        if (mtx.try_lock()) {
            // 成功獲取鎖
            std::cout << "執行緒 " << id << "：第 " << attempt 
                      << " 次嘗試成功" << std::endl;
            
            ++shared_data;
            
            // 模擬一些工作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            mtx.unlock();
            return true;
        }
        
        std::cout << "執行緒 " << id << "：第 " << attempt 
                  << " 次嘗試失敗，稍後重試..." << std::endl;
        
        // 等待一小段時間再重試
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "執行緒 " << id << "：達到最大重試次數，放棄" << std::endl;
    return false;
}

int main() {
    std::thread t1(tryUpdateWithRetry, 1, 5);
    std::thread t2(tryUpdateWithRetry, 2, 5);
    
    t1.join();
    t2.join();
    
    std::cout << "最終 shared_data = " << shared_data << std::endl;
    
    return 0;
}
```

#### 可能的輸出

```
執行緒 1：第 1 次嘗試成功
執行緒 2：第 1 次嘗試失敗，稍後重試...
執行緒 2：第 2 次嘗試失敗，稍後重試...
執行緒 2：第 3 次嘗試失敗，稍後重試...
執行緒 2：第 4 次嘗試失敗，稍後重試...
執行緒 2：第 5 次嘗試失敗，稍後重試...
執行緒 1：（解鎖）
執行緒 2：第 5 次嘗試成功
最終 shared_data = 2
```

---

### 四、避免死結的應用

`try_lock()` 最重要的應用之一是**避免死結**。

#### 死結情境回顧

```
┌─────────────────────────────────────────────────────────────┐
│                      死結情境                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Thread A                      Thread B                     │
│  ─────────                     ─────────                    │
│  lock(mutex1);                 lock(mutex2);                │
│  lock(mutex2);  ← 等待 B       lock(mutex1);  ← 等待 A      │
│                                                             │
│              互相等待 → 死結！                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 使用 try_lock() 避免死結

```cpp
// 檔案：lesson_5_3_avoid_deadlock.cpp
// 說明：使用 try_lock() 避免死結

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mutex1;
std::mutex mutex2;

void safeOperation(int id, std::mutex& first, std::mutex& second, 
                   const char* firstName, const char* secondName) {
    while (true) {
        // 先鎖定第一個互斥鎖
        first.lock();
        std::cout << "執行緒 " << id << "：獲得 " << firstName << std::endl;
        
        // 嘗試鎖定第二個互斥鎖
        if (second.try_lock()) {
            // 成功獲得兩個鎖
            std::cout << "執行緒 " << id << "：獲得 " << secondName 
                      << "，執行操作" << std::endl;
            
            // 執行需要兩個鎖的操作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // 釋放鎖（注意順序：後獲取的先釋放）
            second.unlock();
            first.unlock();
            
            std::cout << "執行緒 " << id << "：操作完成" << std::endl;
            return;  // 成功完成
        }
        
        // 無法獲得第二個鎖，釋放第一個鎖，避免死結
        std::cout << "執行緒 " << id << "：無法獲得 " << secondName 
                  << "，釋放 " << firstName << " 並重試" << std::endl;
        first.unlock();
        
        // 稍等一下再重試（避免活鎖）
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main() {
    // 兩個執行緒以不同順序請求鎖
    std::thread t1([](){ 
        safeOperation(1, mutex1, mutex2, "mutex1", "mutex2"); 
    });
    std::thread t2([](){ 
        safeOperation(2, mutex2, mutex1, "mutex2", "mutex1"); 
    });
    
    t1.join();
    t2.join();
    
    std::cout << "兩個執行緒都成功完成，沒有死結！" << std::endl;
    
    return 0;
}
```

#### 輸出

```
執行緒 1：獲得 mutex1
執行緒 2：獲得 mutex2
執行緒 1：無法獲得 mutex2，釋放 mutex1 並重試
執行緒 2：無法獲得 mutex1，釋放 mutex2 並重試
執行緒 1：獲得 mutex1
執行緒 1：獲得 mutex2，執行操作
執行緒 1：操作完成
執行緒 2：獲得 mutex2
執行緒 2：獲得 mutex1，執行操作
執行緒 2：操作完成
兩個執行緒都成功完成，沒有死結！
```

---

### 五、活鎖（Livelock）問題

```
┌─────────────────────────────────────────────────────────────┐
│                      活鎖警告！                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  使用 try_lock() 可能導致「活鎖」：                          │
│                                                             │
│    Thread A              Thread B                           │
│    ────────              ────────                           │
│    lock(m1)              lock(m2)                           │
│    try m2 失敗           try m1 失敗                         │
│    unlock(m1)            unlock(m2)                         │
│    lock(m1)              lock(m2)        ← 立刻重試          │
│    try m2 失敗           try m1 失敗     ← 又失敗            │
│    ...                   ...             ← 無限循環！        │
│                                                             │
│  解決方案：加入隨機延遲                                      │
│                                                             │
│    // 重試前等待隨機時間                                     │
│    std::this_thread::sleep_for(                             │
│        std::chrono::milliseconds(rand() % 10)               │
│    );                                                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 避免活鎖的改進版本

```cpp
// 檔案：lesson_5_3_avoid_livelock.cpp
// 說明：使用隨機退避避免活鎖

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

std::mutex mutex1;
std::mutex mutex2;

void robustOperation(int id) {
    // 隨機數產生器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);
    
    int attempts = 0;
    const int maxAttempts = 100;
    
    while (attempts < maxAttempts) {
        ++attempts;
        
        mutex1.lock();
        
        if (mutex2.try_lock()) {
            // 成功獲得兩個鎖
            std::cout << "執行緒 " << id << "：成功（嘗試 " 
                      << attempts << " 次）" << std::endl;
            
            // 執行操作
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            mutex2.unlock();
            mutex1.unlock();
            return;
        }
        
        // 釋放並等待隨機時間（指數退避的簡化版）
        mutex1.unlock();
        
        int waitTime = dis(gen);  // 1-10 毫秒的隨機等待
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    
    std::cout << "執行緒 " << id << "：達到最大嘗試次數" << std::endl;
}

int main() {
    std::thread t1(robustOperation, 1);
    std::thread t2(robustOperation, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 六、實用場景：非阻塞資源存取

```cpp
// 檔案：lesson_5_3_non_blocking_cache.cpp
// 說明：非阻塞的快取存取模式

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <chrono>
#include <vector>

class NonBlockingCache {
private:
    std::map<std::string, std::string> cache;
    std::mutex mtx;
    
public:
    // 嘗試從快取獲取值（非阻塞）
    bool tryGet(const std::string& key, std::string& value) {
        if (mtx.try_lock()) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                value = it->second;
                mtx.unlock();
                return true;
            }
            mtx.unlock();
        }
        // 無法獲取鎖或找不到 key
        return false;
    }
    
    // 嘗試設定值（非阻塞）
    bool trySet(const std::string& key, const std::string& value) {
        if (mtx.try_lock()) {
            cache[key] = value;
            mtx.unlock();
            return true;
        }
        return false;
    }
    
    // 阻塞版本（保證成功）
    void set(const std::string& key, const std::string& value) {
        mtx.lock();
        cache[key] = value;
        mtx.unlock();
    }
    
    std::string get(const std::string& key) {
        mtx.lock();
        std::string result = cache[key];
        mtx.unlock();
        return result;
    }
};

NonBlockingCache cache;

void fastReader(int id) {
    std::string value;
    
    for (int i = 0; i < 5; ++i) {
        if (cache.tryGet("data", value)) {
            std::cout << "Reader " << id << "：快速讀取成功 = " << value << std::endl;
        } else {
            std::cout << "Reader " << id << "：快取忙碌，使用預設值" << std::endl;
            // 使用預設值或其他邏輯
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void writer() {
    for (int i = 0; i < 5; ++i) {
        std::string value = "value_" + std::to_string(i);
        cache.set("data", value);
        std::cout << "Writer：寫入 " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

int main() {
    cache.set("data", "initial");
    
    std::thread w(writer);
    std::thread r1(fastReader, 1);
    std::thread r2(fastReader, 2);
    
    w.join();
    r1.join();
    r2.join();
    
    return 0;
}
```

---

### 七、try_lock() 的注意事項

```
┌─────────────────────────────────────────────────────────────┐
│                  try_lock() 注意事項                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 成功時必須 unlock()                                     │
│     ─────────────────────                                   │
│     if (mtx.try_lock()) {                                   │
│         // 工作...                                          │
│         mtx.unlock();  // ← 不要忘記！                      │
│     }                                                       │
│                                                             │
│  2. 可能產生虛假失敗（Spurious Failure）                    │
│     ────────────────────────────────────                    │
│     即使鎖是空閒的，try_lock() 也可能返回 false             │
│     （實作相關，較少見）                                     │
│                                                             │
│  3. 不保證公平性                                            │
│     ──────────────                                          │
│     頻繁呼叫 try_lock() 的執行緒可能一直搶不到鎖            │
│                                                             │
│  4. 不適合替代 lock()                                       │
│     ─────────────────                                       │
│     如果一定要獲取鎖，就用 lock()                           │
│     try_lock() 是用於「可以不獲取」的場景                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 八、try_lock() 與 RAII

手動管理 `try_lock()` 容易忘記 `unlock()`，可以搭配 `std::unique_lock` 使用：

```cpp
// 檔案：lesson_5_3_try_lock_raii.cpp
// 說明：使用 unique_lock 搭配 try_lock

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int counter = 0;

void safeIncrement(int id) {
    // std::try_to_lock 讓 unique_lock 使用 try_lock() 而非 lock()
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    
    if (lock.owns_lock()) {
        // 成功獲取鎖
        ++counter;
        std::cout << "執行緒 " << id << "：成功遞增，counter = " 
                  << counter << std::endl;
        // lock 離開作用域時自動 unlock
    } else {
        // 獲取失敗
        std::cout << "執行緒 " << id << "：無法獲取鎖" << std::endl;
    }
    // 無論成功與否，都不需要手動 unlock
}

int main() {
    std::thread t1(safeIncrement, 1);
    std::thread t2(safeIncrement, 2);
    std::thread t3(safeIncrement, 3);
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "最終 counter = " << counter << std::endl;
    
    return 0;
}
```

---

### 九、何時使用 try_lock()

```
┌─────────────────────────────────────────────────────────────┐
│               try_lock() 適用場景                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✓ 適合使用 try_lock() 的情況：                             │
│                                                             │
│    • 有替代方案時（如使用預設值、快取）                      │
│    • 避免死結時（嘗試多個鎖）                                │
│    • 實現超時機制時                                         │
│    • 輪詢式設計時（定期檢查資源）                            │
│    • 效能敏感且可接受偶爾失敗時                              │
│                                                             │
│  ✗ 不適合使用 try_lock() 的情況：                           │
│                                                             │
│    • 操作必須完成時                                         │
│    • 沒有合理的失敗處理邏輯時                                │
│    • 會導致活鎖時（頻繁重試）                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十、完整範例：帶超時的鎖獲取

```cpp
// 檔案：lesson_5_3_try_lock_timeout.cpp
// 說明：實現自訂的超時鎖獲取

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

// 嘗試在指定時間內獲取鎖
bool tryLockFor(std::mutex& m, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (m.try_lock()) {
            return true;  // 成功獲取
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;  // 超時
        }
        
        // 短暫睡眠避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void worker(int id, int workTime) {
    std::cout << "執行緒 " << id << "：嘗試獲取鎖（超時 100ms）" << std::endl;
    
    if (tryLockFor(mtx, std::chrono::milliseconds(100))) {
        std::cout << "執行緒 " << id << "：獲得鎖，工作 " 
                  << workTime << "ms" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(workTime));
        
        mtx.unlock();
        std::cout << "執行緒 " << id << "：完成並釋放鎖" << std::endl;
    } else {
        std::cout << "執行緒 " << id << "：超時，放棄等待" << std::endl;
    }
}

int main() {
    // 執行緒 1 持有鎖 200ms，執行緒 2 和 3 只等待 100ms
    std::thread t1(worker, 1, 200);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::thread t2(worker, 2, 50);
    std::thread t3(worker, 3, 50);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
```

#### 輸出

```
執行緒 1：嘗試獲取鎖（超時 100ms）
執行緒 1：獲得鎖，工作 200ms
執行緒 2：嘗試獲取鎖（超時 100ms）
執行緒 3：嘗試獲取鎖（超時 100ms）
執行緒 2：超時，放棄等待
執行緒 3：超時，放棄等待
執行緒 1：完成並釋放鎖
```

---

### 十一、本課重點回顧

1. **try_lock() 是非阻塞的**：立即返回 `true`（成功）或 `false`（失敗）
2. **成功時必須 unlock()**：與 `lock()` 相同的責任
3. **可用於避免死結**：嘗試獲取多個鎖，失敗就釋放已持有的鎖
4. **注意活鎖問題**：加入隨機延遲避免無限重試
5. **搭配 std::unique_lock 使用更安全**：`std::try_to_lock` 標籤
6. **適用於有替代方案的場景**：不是用來替代 `lock()`

---

### 下一課預告

在 **課程 5.4：互斥鎖的常見錯誤** 中，我們將學習：
- 忘記解鎖的後果
- 重複鎖定的問題
- 在錯誤的執行緒解鎖
- 例外安全問題

這些都是實際開發中最常見的錯誤，學會避免它們非常重要！

---

準備好繼續嗎？
*/



// 檔案：lesson_5_3_try_lock_timeout.cpp
// 說明：實現自訂的超時鎖獲取

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

std::mutex mtx;

// 嘗試在指定時間內獲取鎖
bool tryLockFor(std::mutex& m, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (m.try_lock()) {
            return true;  // 成功獲取
        }
        
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false;  // 超時
        }
        
        // 短暫睡眠避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void worker(int id, int workTime) {
    std::cout << "執行緒 " << id << "：嘗試獲取鎖（超時 100ms）" << std::endl;
    
    if (tryLockFor(mtx, std::chrono::milliseconds(100))) {
        std::cout << "執行緒 " << id << "：獲得鎖，工作 " 
                  << workTime << "ms" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(workTime));
        
        mtx.unlock();
        std::cout << "執行緒 " << id << "：完成並釋放鎖" << std::endl;
    } else {
        std::cout << "執行緒 " << id << "：超時，放棄等待" << std::endl;
    }
}

int main() {
    // 執行緒 1 持有鎖 200ms，執行緒 2 和 3 只等待 100ms
    std::thread t1(worker, 1, 200);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::thread t2(worker, 2, 50);
    std::thread t3(worker, 3, 50);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
