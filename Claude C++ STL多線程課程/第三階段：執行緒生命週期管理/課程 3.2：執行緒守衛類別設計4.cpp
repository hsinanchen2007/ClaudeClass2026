/*
# 第三階段：執行緒生命週期管理

## 課程 3.2：執行緒守衛類別設計

---

### 引言

上一課介紹了基本的 RAII 執行緒包裝。本課將設計一個更完善、更實用的執行緒守衛類別。

---

### 一、設計目標

```
┌─────────────────────────────────────────────────────────────┐
│              理想的執行緒守衛應具備                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✓ 自動在解構時 join 或 detach                              │
│  ✓ 支援移動語意                                             │
│  ✓ 禁止複製                                                 │
│  ✓ 可以手動提前釋放                                         │
│  ✓ 可以取得底層執行緒的資訊                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、完整的 JoiningThread 類別

```cpp
#include <iostream>
#include <thread>
#include <utility>

class JoiningThread {
    std::thread t;
    
public:
    // 預設建構
    JoiningThread() noexcept = default;
    
    // 從 std::thread 移動建構
    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}
    
    // 直接建構執行緒（完美轉發）
    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    // 移動建構
    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}
    
    // 移動賦值
    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {
            if (t.joinable()) {
                t.join();  // 先處理當前執行緒
            }
            t = std::move(other.t);
        }
        return *this;
    }
    
    // 禁止複製
    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;
    
    // 解構：自動 join
    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 工具方法
    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }
    
    void join() {
        t.join();
    }
    
    void detach() {
        t.detach();
    }
    
    std::thread& get() noexcept { return t; }
};
```

---

### 三、使用範例

```cpp
#include <iostream>
#include <vector>

int main() {
    // 方式一：直接建構
    JoiningThread jt1([]() {
        std::cout << "執行緒 1" << std::endl;
    });
    
    // 方式二：從 std::thread 移動
    std::thread t([]() {
        std::cout << "執行緒 2" << std::endl;
    });
    JoiningThread jt2(std::move(t));
    
    // 方式三：帶參數
    JoiningThread jt3([](int x) {
        std::cout << "執行緒 3: " << x << std::endl;
    }, 42);
    
    // 不需要手動 join！
    return 0;
}
```

輸出：
```
執行緒 1
執行緒 2
執行緒 3: 42
```

---

### 四、在容器中使用

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<JoiningThread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Worker " << i << std::endl;
        });
    }
    
    std::cout << "所有執行緒已建立" << std::endl;
    
    // 離開作用域時，vector 解構
    // 每個 JoiningThread 解構時自動 join
    return 0;
}
```

---

### 五、DetachingThread 變體

如果預設行為是 detach：

```cpp
class DetachingThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit DetachingThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    DetachingThread(DetachingThread&&) = default;
    DetachingThread& operator=(DetachingThread&&) = default;
    
    DetachingThread(const DetachingThread&) = delete;
    DetachingThread& operator=(const DetachingThread&) = delete;
    
    ~DetachingThread() {
        if (t.joinable()) {
            t.detach();
        }
    }
};
```

---

### 六、策略模式：可選擇的行為

```cpp
#include <iostream>
#include <thread>

enum class ThreadAction { Join, Detach };

template<ThreadAction action>
class ManagedThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit ManagedThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    ManagedThread(ManagedThread&&) = default;
    ManagedThread& operator=(ManagedThread&&) = default;
    
    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;
    
    ~ManagedThread() {
        if (t.joinable()) {
            if constexpr (action == ThreadAction::Join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
};

// 型別別名
using AutoJoinThread = ManagedThread<ThreadAction::Join>;
using AutoDetachThread = ManagedThread<ThreadAction::Detach>;

int main() {
    AutoJoinThread jt([]() {
        std::cout << "會被 join" << std::endl;
    });
    
    return 0;
}
```

---

### 七、與 std::jthread 的比較

```
┌────────────────────────────────────────────────────────────┐
│          自訂 JoiningThread vs std::jthread               │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  JoiningThread (自訂)           std::jthread (C++20)      │
│  ─────────────────────         ─────────────────────      │
│  • 可用於 C++11/14/17          • 需要 C++20               │
│  • 可自訂行為                   • 標準化，跨平台一致        │
│  • 需要自己維護                 • 內建支援取消機制          │
│                                • 經過充分測試              │
│                                                            │
│  建議：C++20 可用時，優先使用 std::jthread                 │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

### 八、本課重點回顧

1. 完善的執行緒守衛需要支援**移動語意**
2. 移動賦值前要先處理當前的 joinable 執行緒
3. 使用**完美轉發**支援直接建構執行緒
4. 可用模板參數在編譯期選擇 join 或 detach 行為
5. 執行緒守衛可以安全地放入容器
6. C++20 有標準的 `std::jthread`，功能更完整

---

### 下一課預告

在 **課程 3.3：std::jthread (C++20)** 中，我們將學習：
- 標準化的自動 join 執行緒
- 內建的取消機制（stop_token）
- 為什麼它比自訂類別更好

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>

enum class ThreadAction { Join, Detach };

template<ThreadAction action>
class ManagedThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    explicit ManagedThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    ManagedThread(ManagedThread&&) = default;
    ManagedThread& operator=(ManagedThread&&) = default;
    
    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;
    
    ~ManagedThread() {
        if (t.joinable()) {
            if constexpr (action == ThreadAction::Join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
};

// 型別別名
using AutoJoinThread = ManagedThread<ThreadAction::Join>;
using AutoDetachThread = ManagedThread<ThreadAction::Detach>;

int main() {
    AutoJoinThread jt([]() {
        std::cout << "會被 join" << std::endl;
    });
    
    return 0;
}
