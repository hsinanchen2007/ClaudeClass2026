/*
# 第三階段：執行緒生命週期管理

## 課程 3.4：執行緒例外處理

---

### 引言

執行緒中拋出的例外不會自動傳遞到建立它的執行緒。如果例外未被捕獲，程式會直接終止。本課學習如何正確處理執行緒中的例外。

---

### 一、問題：例外不會跨執行緒傳遞

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    throw std::runtime_error("執行緒內的錯誤！");
}

int main() {
    std::thread t(worker);
    
    try {
        t.join();
    } catch (const std::exception& e) {
        // 這裡捕獲不到！
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

輸出：
```
terminate called after throwing an instance of 'std::runtime_error'
Aborted
```

例外在執行緒內未被捕獲，程式直接終止。

---

### 二、解決方案一：執行緒內部捕獲

最簡單的方法是在執行緒函式內部處理例外：

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>

void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (const std::exception& e) {
        std::cout << "執行緒內捕獲: " << e.what() << std::endl;
    }
}

int main() {
    std::thread t(worker);
    t.join();
    
    std::cout << "程式正常結束" << std::endl;
    return 0;
}
```

輸出：
```
執行緒內捕獲: 執行緒內的錯誤！
程式正常結束
```

---

### 三、解決方案二：使用 std::exception_ptr 傳遞例外

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>
#include <exception>

std::exception_ptr threadException = nullptr;

void worker() {
    try {
        throw std::runtime_error("執行緒內的錯誤！");
    } catch (...) {
        // 捕獲並儲存例外
        threadException = std::current_exception();
    }
}

int main() {
    std::thread t(worker);
    t.join();
    
    // 檢查是否有例外
    if (threadException) {
        try {
            std::rethrow_exception(threadException);
        } catch (const std::exception& e) {
            std::cout << "主執行緒捕獲: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
```

輸出：
```
主執行緒捕獲: 執行緒內的錯誤！
```

---

### 四、exception_ptr 關鍵函式

```
┌─────────────────────────────────────────────────────────────┐
│              例外傳遞相關函式                                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::current_exception()                                   │
│  → 捕獲當前例外，回傳 exception_ptr                          │
│                                                             │
│  std::rethrow_exception(ptr)                                │
│  → 重新拋出 exception_ptr 指向的例外                         │
│                                                             │
│  std::make_exception_ptr(e)                                 │
│  → 從例外物件建立 exception_ptr                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 五、封裝成類別

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>
#include <exception>
#include <functional>

class SafeThread {
    std::thread t;
    std::exception_ptr exPtr = nullptr;
    
public:
    template<typename Func, typename... Args>
    explicit SafeThread(Func&& f, Args&&... args) {
        t = std::thread([this, f = std::forward<Func>(f), 
                         ...args = std::forward<Args>(args)]() mutable {
            try {
                f(args...);
            } catch (...) {
                exPtr = std::current_exception();
            }
        });
    }
    
    void join() {
        t.join();
        if (exPtr) {
            std::rethrow_exception(exPtr);
        }
    }
    
    ~SafeThread() {
        if (t.joinable()) {
            t.join();
        }
    }
};

int main() {
    try {
        SafeThread st([]() {
            throw std::runtime_error("錯誤！");
        });
        st.join();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

---

### 六、解決方案三：使用 std::future（推薦）

`std::async` 和 `std::future` 自動處理例外傳遞：

```cpp
#include <iostream>
#include <future>
#include <stdexcept>

int worker() {
    throw std::runtime_error("非同步任務的錯誤！");
    return 42;
}

int main() {
    std::future<int> result = std::async(std::launch::async, worker);
    
    try {
        int value = result.get();  // 例外在這裡被重新拋出
        std::cout << "結果: " << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

輸出：
```
捕獲: 非同步任務的錯誤！
```

---

### 七、使用 std::promise 傳遞例外

```cpp
#include <iostream>
#include <thread>
#include <future>
#include <stdexcept>

void worker(std::promise<int> prom) {
    try {
        throw std::runtime_error("Worker 錯誤！");
        prom.set_value(42);
    } catch (...) {
        prom.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    
    std::thread t(worker, std::move(prom));
    
    try {
        int value = fut.get();
        std::cout << "結果: " << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    t.join();
    return 0;
}
```

---

### 八、各種方法比較

| 方法 | 優點 | 缺點 |
|------|------|------|
| 執行緒內捕獲 | 簡單 | 無法傳遞給呼叫者 |
| exception_ptr | 可傳遞任何例外 | 需要手動管理 |
| std::future | 自動傳遞 | 需要用 async 或 promise |
| promise | 靈活控制 | 較複雜 |

---

### 九、最佳實踐

```cpp
#include <iostream>
#include <future>
#include <vector>
#include <stdexcept>

int task(int id) {
    if (id == 2) {
        throw std::runtime_error("Task 2 失敗");
    }
    return id * 10;
}

int main() {
    std::vector<std::future<int>> futures;
    
    // 啟動多個任務
    for (int i = 0; i < 5; ++i) {
        futures.push_back(std::async(std::launch::async, task, i));
    }
    
    // 收集結果，處理例外
    for (int i = 0; i < 5; ++i) {
        try {
            int result = futures[i].get();
            std::cout << "Task " << i << " 結果: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Task " << i << " 例外: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
```

輸出：
```
Task 0 結果: 0
Task 1 結果: 10
Task 2 例外: Task 2 失敗
Task 3 結果: 30
Task 4 結果: 40
```

---

### 十、本課重點回顧

1. 執行緒中的例外**不會自動傳遞**到其他執行緒
2. 未捕獲的例外會導致 `std::terminate()`
3. `std::exception_ptr` 可以儲存和傳遞例外
4. `std::current_exception()` 捕獲當前例外
5. `std::rethrow_exception()` 重新拋出例外
6. **推薦使用 `std::future`**，它自動處理例外傳遞

---

### 下一課預告

在 **課程 3.5：執行緒本地儲存** 中，我們將學習：
- `thread_local` 關鍵字
- 每個執行緒獨立的變數
- 使用場景與注意事項

---

準備好繼續嗎？
*/



#include <iostream>
#include <future>
#include <vector>
#include <stdexcept>

int task(int id) {
    if (id == 2) {
        throw std::runtime_error("Task 2 失敗");
    }
    return id * 10;
}

int main() {
    std::vector<std::future<int>> futures;
    
    // 啟動多個任務
    for (int i = 0; i < 5; ++i) {
        futures.push_back(std::async(std::launch::async, task, i));
    }
    
    // 收集結果，處理例外
    for (int i = 0; i < 5; ++i) {
        try {
            int result = futures[i].get();
            std::cout << "Task " << i << " 結果: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Task " << i << " 例外: " << e.what() << std::endl;
        }
    }
    
    return 0;
}
