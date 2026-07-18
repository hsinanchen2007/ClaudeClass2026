/*
# 第二階段：std::thread 基礎

## 課程 2.2：執行緒函式的多種形式

---

### 引言

`std::thread` 可以接受任何**可呼叫物件（Callable）**作為執行緒的入口點。本課介紹四種常見形式。

---

### 一、四種可呼叫物件

```
┌─────────────────────────────────────────┐
│         std::thread 可接受的形式         │
├─────────────────────────────────────────┤
│                                         │
│  1. 一般函式（Function）                 │
│  2. Lambda 表達式                        │
│  3. 成員函式（Member Function）          │
│  4. 函式物件（Functor / Function Object）│
│                                         │
└─────────────────────────────────────────┘
```

---

### 二、一般函式

最基本的形式：

```cpp
#include <iostream>
#include <thread>

void greet() {
    std::cout << "Hello!" << std::endl;
}

int main() {
    std::thread t(greet);
    t.join();
    return 0;
}
```

---

### 三、Lambda 表達式

最靈活且常用的形式：

```cpp
#include <iostream>
#include <thread>

int main() {
    // 無捕獲的 Lambda
    std::thread t1([]() {
        std::cout << "Lambda 1" << std::endl;
    });
    
    // 有捕獲的 Lambda
    int value = 42;
    std::thread t2([value]() {
        std::cout << "Value: " << value << std::endl;
    });
    
    t1.join();
    t2.join();
    return 0;
}
```

輸出：
```
Lambda 1
Value: 42
```

---

### 四、成員函式

需要傳入物件指標和成員函式指標：

```cpp
#include <iostream>
#include <thread>

class Worker {
public:
    void doWork() {
        std::cout << "Worker doing work" << std::endl;
    }
    
    void doWorkWithParam(int id) {
        std::cout << "Worker " << id << " working" << std::endl;
    }
};

int main() {
    Worker worker;
    
    // 成員函式（無參數）
    std::thread t1(&Worker::doWork, &worker);
    
    // 成員函式（有參數）
    std::thread t2(&Worker::doWorkWithParam, &worker, 123);
    
    t1.join();
    t2.join();
    return 0;
}
```

語法：`std::thread(成員函式指標, 物件指標, 參數...)`

---

### 五、函式物件（Functor）

定義了 `operator()` 的類別：

```cpp
#include <iostream>
#include <thread>

class Task {
public:
    void operator()() const {
        std::cout << "Task executed" << std::endl;
    }
};

int main() {
    Task task;
    
    // 方式一：傳入物件
    std::thread t1(task);
    
    // 方式二：傳入臨時物件（注意額外括號）
    std::thread t2((Task()));
    
    // 方式三：使用大括號初始化（推薦）
    std::thread t3{Task()};
    
    t1.join();
    t2.join();
    t3.join();
    return 0;
}
```

**注意**：`std::thread t(Task());` 會被解析為函式宣告（Most Vexing Parse），需要額外括號或大括號。

---

### 六、帶參數的函式物件

```cpp
#include <iostream>
#include <thread>

class Counter {
    int count;
public:
    Counter(int c) : count(c) {}
    
    void operator()() const {
        for (int i = 0; i < count; ++i) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
    }
};

int main() {
    std::thread t{Counter(5)};
    t.join();
    return 0;
}
```

輸出：
```
0 1 2 3 4
```

---

### 七、四種形式對照表

| 形式 | 語法 | 適用場景 |
|------|------|----------|
| 一般函式 | `thread(func)` | 簡單獨立的任務 |
| Lambda | `thread([](){})` | 需要捕獲變數、一次性任務 |
| 成員函式 | `thread(&Class::method, &obj)` | 物件導向設計 |
| 函式物件 | `thread{Functor()}` | 需要保存狀態的可重用任務 |

---

### 八、完整範例

```cpp
// 檔案：lesson_2_2_callable.cpp

#include <iostream>
#include <thread>

// 1. 一般函式
void normalFunction() {
    std::cout << "[Function]" << std::endl;
}

// 2. 類別（成員函式 + 函式物件）
class MyClass {
public:
    void memberFunc() {
        std::cout << "[Member Function]" << std::endl;
    }
    
    void operator()() {
        std::cout << "[Functor]" << std::endl;
    }
};

int main() {
    MyClass obj;
    
    std::thread t1(normalFunction);              // 一般函式
    std::thread t2([]() {                        // Lambda
        std::cout << "[Lambda]" << std::endl; 
    });
    std::thread t3(&MyClass::memberFunc, &obj);  // 成員函式
    std::thread t4{MyClass()};                   // 函式物件
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    return 0;
}
```

---

### 九、本課重點回顧

1. `std::thread` 接受任何可呼叫物件
2. **一般函式**：最簡單，直接傳入函式名稱
3. **Lambda**：最靈活，可捕獲外部變數
4. **成員函式**：需傳入 `&Class::method` 和物件指標
5. **函式物件**：需定義 `operator()`，注意 Most Vexing Parse

---

### 下一課預告

在 **課程 2.3：傳遞參數給執行緒** 中，我們將學習：
- 傳值 vs 傳引用
- `std::ref` 的使用
- 參數傳遞的陷阱

---

準備好繼續嗎？
*/


// 檔案：lesson_2_2_callable.cpp

#include <iostream>
#include <thread>

// 1. 一般函式
void normalFunction() {
    std::cout << "[Function]" << std::endl;
}

// 2. 類別（成員函式 + 函式物件）
class MyClass {
public:
    void memberFunc() {
        std::cout << "[Member Function]" << std::endl;
    }
    
    void operator()() {
        std::cout << "[Functor]" << std::endl;
    }
};

int main() {
    MyClass obj;
    
    std::thread t1(normalFunction);              // 一般函式
    std::thread t2([]() {                        // Lambda
        std::cout << "[Lambda]" << std::endl; 
    });
    std::thread t3(&MyClass::memberFunc, &obj);  // 成員函式
    std::thread t4{MyClass()};                   // 函式物件
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    
    return 0;
}
