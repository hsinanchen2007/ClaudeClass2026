/*
# 第三階段：執行緒生命週期管理

## 課程 3.3：std::jthread (C++20)

---

### 引言

C++20 引入了 `std::jthread`（joining thread），它是 `std::thread` 的改良版本，內建 RAII 自動 join 和協作式取消機制。

---

### 一、std::jthread vs std::thread

```
┌─────────────────────────────────────────────────────────────┐
│            std::thread vs std::jthread                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::thread                    std::jthread                │
│  ────────────                   ─────────────               │
│  • 必須手動 join/detach         • 解構時自動 join            │
│  • 忘記會導致 terminate         • 不會忘記                   │
│  • 無取消機制                   • 內建 stop_token 取消機制   │
│  • C++11 起可用                 • C++20 起可用               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、基本用法

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([]() {
        std::cout << "Hello from jthread!" << std::endl;
    });
    
    // 不需要呼叫 join()！
    // 離開作用域時自動 join
    
    return 0;
}
```

編譯（需要 C++20）：
```bash
g++ -std=c++20 -pthread -o jthread_demo jthread_demo.cpp
```

---

### 三、自動 join 的安全性

即使發生例外，也能正確處理：

```cpp
#include <iostream>
#include <thread>
#include <stdexcept>

void riskyFunction() {
    std::jthread jt([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "執行緒完成" << std::endl;
    });
    
    throw std::runtime_error("發生錯誤！");
    
    // 不需要擔心！jt 解構時會自動 join
}

int main() {
    try {
        riskyFunction();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
```

輸出：
```
執行緒完成
捕獲: 發生錯誤！
```

---

### 四、stop_token 取消機制

`std::jthread` 最強大的功能是內建的協作式取消：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        int count = 0;
        while (!stoken.stop_requested()) {
            std::cout << "工作中... " << ++count << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "收到停止請求，結束" << std::endl;
    });
    
    // 讓執行緒跑一下
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 請求停止
    jt.request_stop();
    
    // jt 解構時會等待執行緒結束
    return 0;
}
```

輸出：
```
工作中... 1
工作中... 2
工作中... 3
工作中... 4
工作中... 5
收到停止請求，結束
```

---

### 五、stop_token 詳解

```
┌─────────────────────────────────────────────────────────────┐
│                   stop_token 機制                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::stop_source   →  發出停止請求的來源                    │
│  std::stop_token    →  檢查是否有停止請求                    │
│  std::stop_callback →  停止時自動執行的回調                  │
│                                                             │
│  jthread 內部自動管理 stop_source                            │
│  執行緒函式可接收 stop_token 作為第一個參數                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、手動取得 stop_source 和 stop_token

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::cout << "Running..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });
    
    // 取得 stop_source
    std::stop_source& source = jt.get_stop_source();
    
    // 取得 stop_token
    std::stop_token token = jt.get_stop_token();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 兩種方式都可以請求停止
    // source.request_stop();
    jt.request_stop();
    
    return 0;
}
```

---

### 七、stop_callback 自動回調

當停止被請求時，自動執行回調：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        // 註冊停止時的回調
        std::stop_callback callback(stoken, []() {
            std::cout << "停止回調被觸發！" << std::endl;
        });
        
        while (!stoken.stop_requested()) {
            std::cout << "工作中..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        
        std::cout << "執行緒結束" << std::endl;
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    jt.request_stop();
    
    return 0;
}
```

輸出：
```
工作中...
工作中...
工作中...
停止回調被觸發！
執行緒結束
```

---

### 八、jthread 的完整介面

```cpp
class jthread {
public:
    // 建構
    jthread() noexcept;
    
    template<typename F, typename... Args>
    explicit jthread(F&& f, Args&&... args);
    
    // 解構：自動 request_stop() + join()
    ~jthread();
    
    // 移動（不可複製）
    jthread(jthread&&) noexcept;
    jthread& operator=(jthread&&) noexcept;
    
    // 停止機制
    stop_source get_stop_source() noexcept;
    stop_token get_stop_token() const noexcept;
    bool request_stop() noexcept;
    
    // 與 std::thread 相同的介面
    bool joinable() const noexcept;
    void join();
    void detach();
    id get_id() const noexcept;
    native_handle_type native_handle();
    
    static unsigned int hardware_concurrency() noexcept;
};
```

---

### 九、何時用 jthread vs thread

| 情況 | 選擇 |
|------|------|
| C++20 可用 | 優先使用 `jthread` |
| 需要取消機制 | 使用 `jthread` |
| 需要 detach | 使用 `thread` |
| 舊專案相容性 | 使用 `thread` |

---

### 十、完整範例：可取消的工作執行緒

```cpp
#include <iostream>
#include <thread>
#include <vector>

void worker(std::stop_token stoken, int id) {
    std::cout << "Worker " << id << " 啟動" << std::endl;
    
    int count = 0;
    while (!stoken.stop_requested()) {
        ++count;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Worker " << id << " 結束，執行了 " << count << " 次" << std::endl;
}

int main() {
    std::vector<std::jthread> workers;
    
    // 建立 3 個工作執行緒
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back(worker, i);
    }
    
    std::cout << "主執行緒等待 1 秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "請求所有執行緒停止" << std::endl;
    for (auto& w : workers) {
        w.request_stop();
    }
    
    // vector 解構時，每個 jthread 自動 join
    return 0;
}
```

---

### 十一、本課重點回顧

1. `std::jthread` 是 C++20 新增的執行緒類別
2. **自動 join**：解構時自動等待執行緒結束
3. **stop_token**：內建協作式取消機制
4. 執行緒函式可接收 `std::stop_token` 作為第一個參數
5. `request_stop()` 請求執行緒停止
6. `stop_callback` 可在停止時自動執行回調
7. 優先使用 `jthread`，除非需要 detach 或舊版相容

---

### 下一課預告

在 **課程 3.4：執行緒例外處理** 中，我們將學習：
- 執行緒中的例外如何處理
- 例外如何在執行緒間傳遞
- 安全的錯誤處理模式

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <vector>

void worker(std::stop_token stoken, int id) {
    std::cout << "Worker " << id << " 啟動" << std::endl;
    
    int count = 0;
    while (!stoken.stop_requested()) {
        ++count;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Worker " << id << " 結束，執行了 " << count << " 次" << std::endl;
}

int main() {
    std::vector<std::jthread> workers;
    
    // 建立 3 個工作執行緒
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back(worker, i);
    }
    
    std::cout << "主執行緒等待 1 秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "請求所有執行緒停止" << std::endl;
    for (auto& w : workers) {
        w.request_stop();
    }
    
    // vector 解構時，每個 jthread 自動 join
    return 0;
}
