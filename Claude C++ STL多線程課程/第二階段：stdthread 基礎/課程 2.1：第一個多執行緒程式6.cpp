/*
# 第二階段：std::thread 基礎

## 課程 2.1：第一個多執行緒程式

---

### 引言

從這一課開始，我們正式動手撰寫多執行緒程式。`std::thread` 是 C++ 多執行緒程式設計的核心類別，掌握它是一切的起點。

---

### 一、std::thread 基本概念

`std::thread` 代表一個執行緒物件。建立它時，執行緒就開始執行。

```
┌─────────────────────────────────────────┐
│           std::thread 生命週期          │
├─────────────────────────────────────────┤
│                                         │
│  建構 ──► 執行中 ──► join() 或 detach() │
│                                         │
│  • 建構時立即開始執行                    │
│  • 必須在解構前呼叫 join() 或 detach()  │
│  • 否則程式會呼叫 std::terminate()      │
│                                         │
└─────────────────────────────────────────┘
```

---

### 二、最簡單的多執行緒程式

```cpp
// 檔案：lesson_2_1_first_thread.cpp

#include <iostream>
#include <thread>

void sayHello() {
    std::cout << "Hello from thread!" << std::endl;
}

int main() {
    std::thread t(sayHello);  // 建立並啟動執行緒
    t.join();                 // 等待執行緒結束
    
    std::cout << "Back in main" << std::endl;
    return 0;
}
```

```bash
g++ -std=c++17 -pthread -o lesson_2_1 lesson_2_1_first_thread.cpp
./lesson_2_1
```

輸出：
```
Hello from thread!
Back in main
```

---

### 三、join() vs detach()

#### join() — 等待執行緒結束

```cpp
#include <iostream>
#include <thread>

void work() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Work done!" << std::endl;
}

int main() {
    std::thread t(work);
    std::cout << "Waiting..." << std::endl;
    t.join();  // 阻塞，直到 t 完成
    std::cout << "Thread finished" << std::endl;
    return 0;
}
```

輸出：
```
Waiting...
Work done!
Thread finished
```

#### detach() — 分離執行緒

```cpp
#include <iostream>
#include <thread>

void background() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Background task" << std::endl;  // 可能不會印出！
}

int main() {
    std::thread t(background);
    t.detach();  // 分離，不等待
    
    std::cout << "Main exits" << std::endl;
    return 0;  // 主程式結束，背景執行緒可能還沒完成
}
```

輸出（不確定）：
```
Main exits
```

**警告**：detach 後的執行緒在主程式結束時會被強制終止，要小心使用。

---

### 四、忘記 join/detach 的後果

```cpp
#include <iostream>
#include <thread>

void work() {
    std::cout << "Working..." << std::endl;
}

int main() {
    std::thread t(work);
    // 忘記呼叫 join() 或 detach()
    return 0;  // 程式崩潰！呼叫 std::terminate()
}
```

執行結果：
```
Working...
terminate called without an active exception
Aborted (core dumped)
```

---

### 五、使用 Lambda 建立執行緒

Lambda 是建立執行緒最簡潔的方式：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Hello from lambda!" << std::endl;
    });
    
    t.join();
    return 0;
}
```

---

### 六、多個執行緒同時運行

```cpp
// 檔案：lesson_2_1_multiple.cpp

#include <iostream>
#include <thread>

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 3; ++i)
            std::cout << "A";
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 3; ++i)
            std::cout << "B";
    });
    
    t1.join();
    t2.join();
    
    std::cout << std::endl;
    return 0;
}
```

輸出（順序不確定）：
```
AABBAB
```

每次執行結果可能不同，這就是多執行緒的非確定性。

---

### 七、關鍵要點圖解

```
┌──────────────────────────────────────────────────────────┐
│                   std::thread 使用要點                    │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  ✓ 建構時傳入可呼叫物件（函式、Lambda、函式物件）          │
│                                                          │
│  ✓ 執行緒建立後立即開始執行                               │
│                                                          │
│  ✓ 必須呼叫 join() 或 detach()，否則程式崩潰             │
│                                                          │
│  ✓ join() = 等待結束，detach() = 放手不管                │
│                                                          │
│  ✓ 多執行緒的輸出順序是不確定的                           │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

### 八、本課重點回顧

1. `std::thread` 建構時傳入函式，執行緒立即開始執行
2. `join()` 會阻塞等待執行緒結束
3. `detach()` 讓執行緒在背景獨立運行
4. 必須在 thread 物件解構前呼叫 join() 或 detach()
5. Lambda 是建立執行緒最方便的方式
6. 多執行緒的執行順序是不確定的

---

### 下一課預告

在 **課程 2.2：執行緒函式的多種形式** 中，我們將學習：
- 一般函式
- Lambda 表達式
- 成員函式
- 函式物件（Functor）

---

準備好繼續嗎？
*/

// 檔案：lesson_2_1_multiple.cpp

#include <iostream>
#include <thread>

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 3; ++i)
            std::cout << "A";
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 3; ++i)
            std::cout << "B";
    });
    
    t1.join();
    t2.join();
    
    std::cout << std::endl;
    return 0;
}
