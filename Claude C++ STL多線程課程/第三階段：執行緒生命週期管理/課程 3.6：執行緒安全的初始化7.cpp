/*
# 第三階段：執行緒生命週期管理

## 課程 3.6：執行緒安全的初始化

---

### 引言

在多執行緒環境中，某些資源只應該被初始化一次（例如單例物件、全域配置）。本課學習如何確保初始化只發生一次，即使多個執行緒同時嘗試初始化。

---

### 一、問題：不安全的初始化

```cpp
#include <iostream>
#include <thread>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;

void initAndUse() {
    if (db == nullptr) {           // 執行緒 A 檢查
        db = new Database();       // 執行緒 A 和 B 都可能執行這行！
    }
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    
    t1.join();
    t2.join();
    
    delete db;
    return 0;
}
```

可能輸出：
```
Database 初始化
Database 初始化
查詢中...
查詢中...
```

Database 被初始化了兩次！這是競爭條件。

---

### 二、解決方案：std::call_once

```cpp
#include <iostream>
#include <thread>
#include <mutex>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;
std::once_flag initFlag;

void initDatabase() {
    db = new Database();
}

void initAndUse() {
    std::call_once(initFlag, initDatabase);
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    std::thread t3(initAndUse);
    
    t1.join();
    t2.join();
    t3.join();
    
    delete db;
    return 0;
}
```

輸出：
```
Database 初始化
查詢中...
查詢中...
查詢中...
```

不管多少執行緒，Database 只初始化一次。

---

### 三、call_once 工作原理

```
┌─────────────────────────────────────────────────────────────┐
│                std::call_once 機制                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::once_flag flag;                                       │
│  → 記錄是否已經執行過                                        │
│                                                             │
│  std::call_once(flag, func);                                │
│  → 第一個到達的執行緒執行 func                               │
│  → 其他執行緒等待直到 func 完成                              │
│  → 之後的呼叫直接跳過（flag 已設定）                         │
│                                                             │
│  如果 func 拋出例外：                                        │
│  → flag 不會被設定                                          │
│  → 下一個執行緒會再次嘗試執行                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 四、使用 Lambda

更簡潔的寫法：

```cpp
#include <iostream>
#include <thread>
#include <mutex>

class Database {
public:
    Database() { std::cout << "Database 初始化" << std::endl; }
    void query() { std::cout << "查詢中..." << std::endl; }
};

Database* db = nullptr;
std::once_flag initFlag;

void initAndUse() {
    std::call_once(initFlag, []() {
        db = new Database();
    });
    db->query();
}

int main() {
    std::thread t1(initAndUse);
    std::thread t2(initAndUse);
    
    t1.join();
    t2.join();
    
    delete db;
    return 0;
}
```

---

### 五、執行緒安全的單例模式

#### 方法一：使用 call_once

```cpp
#include <iostream>
#include <thread>
#include <mutex>

class Singleton {
    static Singleton* instance;
    static std::once_flag initFlag;
    
    Singleton() { std::cout << "Singleton 建立" << std::endl; }
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static Singleton& getInstance() {
        std::call_once(initFlag, []() {
            instance = new Singleton();
        });
        return *instance;
    }
    
    void doSomething() { std::cout << "工作中" << std::endl; }
};

Singleton* Singleton::instance = nullptr;
std::once_flag Singleton::initFlag;

int main() {
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

#### 方法二：使用 static 區域變數（更簡潔，C++11 保證安全）

```cpp
#include <iostream>
#include <thread>

class Singleton {
    Singleton() { std::cout << "Singleton 建立" << std::endl; }
    
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    static Singleton& getInstance() {
        static Singleton instance;  // C++11 保證執行緒安全
        return instance;
    }
    
    void doSomething() { std::cout << "工作中" << std::endl; }
};

int main() {
    std::thread t1([]() { Singleton::getInstance().doSomething(); });
    std::thread t2([]() { Singleton::getInstance().doSomething(); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

C++11 標準保證：區域 static 變數的初始化是執行緒安全的。

---

### 六、兩種方法比較

```
┌─────────────────────────────────────────────────────────────┐
│            call_once vs static 區域變數                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  std::call_once                static 區域變數              │
│  ──────────────                ─────────────────            │
│  • 明確控制初始化時機          • 更簡潔                     │
│  • 可用於非建構函式的初始化    • 只能用於建構函式            │
│  • 需要 once_flag              • 不需要額外變數              │
│  • 較靈活                      • C++11 自動保證安全          │
│                                                             │
│  建議：單例優先用 static 區域變數                            │
│        複雜初始化用 call_once                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、call_once 與例外

如果初始化函式拋出例外，下一個執行緒會重新嘗試：

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

std::once_flag flag;
std::atomic<int> attempt{0};

void mayFail() {
    int current = ++attempt;
    std::cout << "嘗試 #" << current << std::endl;
    
    if (current < 3) {
        throw std::runtime_error("初始化失敗");
    }
    
    std::cout << "初始化成功！" << std::endl;
}

void worker() {
    try {
        std::call_once(flag, mayFail);
        std::cout << "繼續執行" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲例外: " << e.what() << std::endl;
    }
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    return 0;
}
```

可能輸出：
```
嘗試 #1
捕獲例外: 初始化失敗
嘗試 #2
捕獲例外: 初始化失敗
嘗試 #3
初始化成功！
繼續執行
繼續執行
```

---

### 八、類別成員的延遲初始化

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>

class Service {
    mutable std::once_flag cacheFlag;
    mutable std::unique_ptr<std::string> cache;
    
    void initCache() const {
        std::cout << "初始化快取..." << std::endl;
        cache = std::make_unique<std::string>("快取資料");
    }
    
public:
    const std::string& getCache() const {
        std::call_once(cacheFlag, &Service::initCache, this);
        return *cache;
    }
};

int main() {
    Service service;
    
    std::thread t1([&]() { std::cout << service.getCache() << std::endl; });
    std::thread t2([&]() { std::cout << service.getCache() << std::endl; });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 九、本課重點回顧

1. 多執行緒環境下的初始化可能發生競爭條件
2. `std::call_once` 確保函式只執行一次
3. `std::once_flag` 記錄是否已執行
4. 如果初始化拋出例外，下一個執行緒會重新嘗試
5. C++11 保證區域 static 變數初始化是執行緒安全的
6. 單例模式優先使用區域 static 變數實作

---

### 第三階段完成！

恭喜你完成了執行緒生命週期管理階段！你已經學會：

- ✅ RAII 執行緒管理
- ✅ 執行緒守衛類別設計
- ✅ std::jthread 與 stop_token
- ✅ 執行緒例外處理
- ✅ thread_local 儲存
- ✅ std::call_once 安全初始化

---

### 下一階段預告

**第四階段：共享資料與競爭條件** 將深入探討：
- 課程 4.1：共享資料的問題
- 課程 4.2：不變量與競爭條件
- 課程 4.3：臨界區段概念
- ...

---

準備好進入第四階段嗎？
*/



#include <iostream>
#include <thread>
#include <mutex>
#include <memory>

class Service {
    mutable std::once_flag cacheFlag;
    mutable std::unique_ptr<std::string> cache;
    
    void initCache() const {
        std::cout << "初始化快取..." << std::endl;
        cache = std::make_unique<std::string>("快取資料");
    }
    
public:
    const std::string& getCache() const {
        std::call_once(cacheFlag, &Service::initCache, this);
        return *cache;
    }
};

int main() {
    Service service;
    
    std::thread t1([&]() { std::cout << service.getCache() << std::endl; });
    std::thread t2([&]() { std::cout << service.getCache() << std::endl; });
    
    t1.join();
    t2.join();
    
    return 0;
}
