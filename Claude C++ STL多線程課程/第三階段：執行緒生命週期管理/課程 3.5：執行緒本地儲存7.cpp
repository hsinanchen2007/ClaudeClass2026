/*
# 第三階段：執行緒生命週期管理

## 課程 3.5：執行緒本地儲存

---

### 引言

有時我們希望每個執行緒都有自己獨立的變數副本，而不是共享同一個變數。C++11 引入的 `thread_local` 關鍵字正是為此設計。

---

### 一、thread_local 基本概念

```
┌─────────────────────────────────────────────────────────────┐
│                    變數儲存類型比較                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  全域/static 變數        thread_local 變數                  │
│  ─────────────────      ─────────────────                   │
│  所有執行緒共享一份       每個執行緒各有一份                  │
│  需要同步保護            不需要同步（各自獨立）               │
│  程式啟動時初始化         執行緒啟動時初始化                  │
│  程式結束時銷毀          執行緒結束時銷毀                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、基本用法

```cpp
#include <iostream>
#include <thread>

thread_local int counter = 0;  // 每個執行緒有獨立的 counter

void increment(const std::string& name) {
    for (int i = 0; i < 3; ++i) {
        ++counter;
        std::cout << name << ": counter = " << counter << std::endl;
    }
}

int main() {
    std::thread t1(increment, "Thread A");
    std::thread t2(increment, "Thread B");
    
    t1.join();
    t2.join();
    
    return 0;
}
```

輸出（順序可能不同）：
```
Thread A: counter = 1
Thread A: counter = 2
Thread A: counter = 3
Thread B: counter = 1
Thread B: counter = 2
Thread B: counter = 3
```

注意：兩個執行緒的 counter 各自獨立從 0 開始。

---

### 三、對比：有無 thread_local

```cpp
#include <iostream>
#include <thread>

int globalCounter = 0;              // 共享
thread_local int localCounter = 0;  // 各自獨立

void work(const std::string& name) {
    ++globalCounter;
    ++localCounter;
    
    std::cout << name 
              << " global=" << globalCounter 
              << " local=" << localCounter << std::endl;
}

int main() {
    std::thread t1(work, "A");
    std::thread t2(work, "B");
    std::thread t3(work, "C");
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}
```

可能的輸出：
```
A global=1 local=1
B global=2 local=1
C global=3 local=1
```

`globalCounter` 累加，`localCounter` 每個執行緒都是 1。

---

### 四、thread_local 的位置

可以用於三種地方：

```cpp
#include <iostream>
#include <thread>

// 1. 全域變數
thread_local int global_tl = 0;

void func() {
    // 2. 函式內的 static 變數
    thread_local static int local_tl = 0;
    
    ++global_tl;
    ++local_tl;
    
    std::cout << "global_tl=" << global_tl 
              << " local_tl=" << local_tl << std::endl;
}

class MyClass {
public:
    // 3. 類別的 static 成員
    thread_local static int member_tl;
};

thread_local int MyClass::member_tl = 0;

int main() {
    std::thread t1([]() { func(); func(); });
    std::thread t2([]() { func(); func(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 五、常見用途

#### 用途一：錯誤碼（如 errno）

```cpp
#include <iostream>
#include <thread>

thread_local int lastError = 0;

void setError(int code) {
    lastError = code;
}

int getError() {
    return lastError;
}

void worker(int id) {
    setError(id * 100);
    std::cout << "Thread " << id << " error: " << getError() << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

#### 用途二：快取

```cpp
#include <iostream>
#include <thread>
#include <string>

thread_local std::string cache;

std::string expensiveCompute(int id) {
    if (cache.empty()) {
        // 模擬耗時計算
        cache = "Result for thread " + std::to_string(id);
        std::cout << "Computing..." << std::endl;
    }
    return cache;
}

void worker(int id) {
    std::cout << expensiveCompute(id) << std::endl;
    std::cout << expensiveCompute(id) << std::endl;  // 使用快取
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

輸出：
```
Computing...
Result for thread 1
Result for thread 1
Computing...
Result for thread 2
Result for thread 2
```

---

#### 用途三：隨機數產生器

```cpp
#include <iostream>
#include <thread>
#include <random>

thread_local std::mt19937 rng{std::random_device{}()};

int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void worker(int id) {
    std::cout << "Thread " << id << ": " 
              << randomInt(1, 100) << ", "
              << randomInt(1, 100) << ", "
              << randomInt(1, 100) << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 六、生命週期

```
┌─────────────────────────────────────────────────────────────┐
│              thread_local 變數的生命週期                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  執行緒啟動                                                  │
│      ↓                                                      │
│  首次存取時初始化（或執行緒開始時）                           │
│      ↓                                                      │
│  執行緒執行期間持續存在                                      │
│      ↓                                                      │
│  執行緒結束時銷毀（呼叫解構函式）                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、注意事項

```cpp
#include <iostream>
#include <thread>

class Resource {
public:
    Resource() { std::cout << "Resource 建構" << std::endl; }
    ~Resource() { std::cout << "Resource 銷毀" << std::endl; }
};

thread_local Resource res;  // 每個執行緒有自己的 Resource

void worker(int id) {
    std::cout << "Thread " << id << " 使用資源" << std::endl;
}

int main() {
    std::cout << "=== 主執行緒 ===" << std::endl;
    
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    std::cout << "=== 結束 ===" << std::endl;
    return 0;
}
```

輸出：
```
=== 主執行緒 ===
Resource 建構
Thread 1 使用資源
Resource 建構
Thread 2 使用資源
Resource 銷毀
Resource 銷毀
=== 結束 ===
```

---

### 八、thread_local vs 其他方案

| 方案 | 優點 | 缺點 |
|------|------|------|
| thread_local | 語法簡潔、自動管理 | 無法跨執行緒存取 |
| 傳遞參數 | 明確、可控 | 需要到處傳遞 |
| 執行緒 ID map | 可跨執行緒存取 | 需要同步、較複雜 |

---

### 九、本課重點回顧

1. `thread_local` 讓每個執行緒有獨立的變數副本
2. 不需要同步，因為各執行緒存取自己的副本
3. 變數在執行緒首次存取時初始化，結束時銷毀
4. 常見用途：錯誤碼、快取、隨機數產生器
5. 可用於全域變數、函式內 static、類別 static 成員

---

### 下一課預告

在 **課程 3.6：執行緒安全的初始化** 中，我們將學習：
- `std::call_once` 確保只初始化一次
- `std::once_flag` 的使用
- 單例模式的執行緒安全實作

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>

class Resource {
public:
    Resource() { std::cout << "Resource 建構" << std::endl; }
    ~Resource() { std::cout << "Resource 銷毀" << std::endl; }
};

thread_local Resource res;  // 每個執行緒有自己的 Resource

void worker(int id) {
    std::cout << "Thread " << id << " 使用資源" << std::endl;
}

int main() {
    std::cout << "=== 主執行緒 ===" << std::endl;
    
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    std::cout << "=== 結束 ===" << std::endl;
    return 0;
}
