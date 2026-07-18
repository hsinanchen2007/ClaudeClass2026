/*
# 第三階段：執行緒生命週期管理

## 課程 3.1：RAII 與執行緒管理

---

### 引言

RAII（Resource Acquisition Is Initialization）是 C++ 最重要的資源管理慣用法。將它應用於執行緒管理，可以避免忘記 join 或 detach 導致的程式崩潰。

---

### 一、問題：例外導致的資源洩漏

```cpp
#include <iostream>
#include <thread>

void riskyFunction() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    // 如果這裡拋出例外...
    throw std::runtime_error("發生錯誤！");
    
    t.join();  // 永遠不會執行！
}  // t 解構時仍是 joinable → std::terminate()

int main() {
    try {
        riskyFunction();
    } catch (...) {
        std::cout << "捕獲例外" << std::endl;
    }
    return 0;
}
```

這段程式會崩潰，因為例外導致 `join()` 被跳過。

---

### 二、RAII 的核心概念

```
┌─────────────────────────────────────────────────────────────┐
│                    RAII 原則                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  建構函式：獲取資源                                          │
│  解構函式：釋放資源                                          │
│                                                             │
│  優點：                                                      │
│  • 無論正常返回或例外，解構函式都會被呼叫                     │
│  • 資源自動管理，不會忘記釋放                                │
│  • 程式碼更簡潔、更安全                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 三、簡單的 RAII 執行緒包裝

```cpp
#include <iostream>
#include <thread>

class ThreadGuard {
    std::thread& t;
public:
    explicit ThreadGuard(std::thread& thread) : t(thread) {}
    
    ~ThreadGuard() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 禁止複製
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

int main() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    ThreadGuard guard(t);  // 保證 t 會被 join
    
    // 即使這裡拋出例外，guard 的解構函式仍會執行
    // throw std::runtime_error("測試");
    
    return 0;
}  // guard 解構 → 自動 join
```

---

### 四、擁有執行緒的 RAII 類別

上面的 `ThreadGuard` 只持有引用。更好的設計是擁有執行緒：

```cpp
#include <iostream>
#include <thread>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) 
    {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();  // 一定是 joinable，不用檢查
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

int main() {
    ScopedThread st(std::thread([]() {
        std::cout << "安全的執行緒" << std::endl;
    }));
    
    // 自動管理，不需要手動 join
    return 0;
}
```

---

### 五、選擇 join 或 detach 的 RAII

```cpp
#include <iostream>
#include <thread>

class FlexibleThread {
public:
    enum class Action { join, detach };
    
private:
    std::thread t;
    Action action;
    
public:
    FlexibleThread(std::thread thread, Action a) 
        : t(std::move(thread)), action(a) {}
    
    ~FlexibleThread() {
        if (t.joinable()) {
            if (action == Action::join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
    
    FlexibleThread(const FlexibleThread&) = delete;
    FlexibleThread& operator=(const FlexibleThread&) = delete;
};

int main() {
    FlexibleThread ft1(
        std::thread([]() { std::cout << "Join 我" << std::endl; }),
        FlexibleThread::Action::join
    );
    
    FlexibleThread ft2(
        std::thread([]() { std::cout << "Detach 我" << std::endl; }),
        FlexibleThread::Action::detach
    );
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
```

---

### 六、RAII vs 手動管理對比

```cpp
// ❌ 手動管理：容易出錯
void manual() {
    std::thread t(work);
    
    if (condition1) {
        t.join();
        return;       // 要記得 join
    }
    
    if (condition2) {
        t.join();
        throw error;  // 要記得 join
    }
    
    t.join();         // 正常路徑也要 join
}

// ✅ RAII：自動管理
void automatic() {
    ScopedThread st(std::thread(work));
    
    if (condition1) {
        return;       // 自動 join
    }
    
    if (condition2) {
        throw error;  // 自動 join
    }
    
    // 自動 join
}
```

---

### 七、本課重點回顧

1. **RAII** 利用解構函式自動釋放資源
2. 解構函式在正常返回和例外時都會執行
3. `ThreadGuard` 持有引用，保證執行緒被 join
4. `ScopedThread` 擁有執行緒，更安全
5. RAII 讓程式碼更簡潔，避免遺漏 join/detach
6. C++20 的 `std::jthread` 就是標準化的 RAII 執行緒

---

### 下一課預告

在 **課程 3.2：執行緒守衛類別設計** 中，我們將：
- 完善 ThreadGuard 的設計
- 處理更多邊界情況
- 加入移動語意支援

---

準備好繼續嗎？
*/



// ❌ 手動管理：容易出錯
void manual() {
    std::thread t(work);
    
    if (condition1) {
        t.join();
        return;       // 要記得 join
    }
    
    if (condition2) {
        t.join();
        throw error;  // 要記得 join
    }
    
    t.join();         // 正常路徑也要 join
}

// ✅ RAII：自動管理
void automatic() {
    ScopedThread st(std::thread(work));
    
    if (condition1) {
        return;       // 自動 join
    }
    
    if (condition2) {
        throw error;  // 自動 join
    }
    
    // 自動 join
}
