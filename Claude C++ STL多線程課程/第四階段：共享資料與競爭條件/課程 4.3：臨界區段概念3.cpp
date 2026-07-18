/*
# 第四階段：共享資料與競爭條件

## 課程 4.3：臨界區段概念

---

### 引言

既然我們知道共享資料存取會造成問題，下一步就是識別哪些程式碼需要保護。這些需要保護的程式碼區域稱為**臨界區段（Critical Section）**。

---

### 一、臨界區段的定義

```
┌─────────────────────────────────────────────────────────────┐
│                 臨界區段（Critical Section）                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  定義：存取共享資源的程式碼區段                              │
│                                                             │
│  特性：                                                      │
│  • 同一時間只能有一個執行緒執行                              │
│  • 其他執行緒必須等待                                        │
│  • 應該盡量短小                                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、圖解臨界區段

```
執行緒 A        執行緒 B        執行緒 C
    │               │               │
    ▼               ▼               ▼
[一般程式碼]    [一般程式碼]    [一般程式碼]
    │               │               │
    ▼               │               │
┌────────┐          │               │
│ 臨界   │  等待    │               │
│ 區段   │◄─────────┤               │
│        │          │      等待     │
└────────┘          │◄──────────────┤
    │               ▼               │
    │          ┌────────┐           │
    │          │ 臨界   │           │
    │          │ 區段   │           │
    │          └────────┘           │
    │               │               ▼
    │               │          ┌────────┐
    │               │          │ 臨界   │
    │               │          │ 區段   │
    │               │          └────────┘
    ▼               ▼               ▼
[一般程式碼]    [一般程式碼]    [一般程式碼]
```

---

### 三、識別臨界區段

```cpp
#include <iostream>
#include <thread>

int sharedData = 0;

void worker() {
    int localVar = 0;        // 不是臨界區段：區域變數
    
    localVar = 42;           // 不是臨界區段：只存取區域變數
    
    sharedData = localVar;   // ← 臨界區段：寫入共享資料
    
    int temp = sharedData;   // ← 臨界區段：讀取共享資料（如有寫入者）
    
    std::cout << temp;       // 可能是臨界區段：cout 也是共享資源
}
```

---

### 四、臨界區段的範圍

#### 範例：太大的臨界區段（不好）

```cpp
void badExample() {
    lock();
    
    // 臨界區段開始
    int result = complexCalculation();  // 不需要鎖！
    sharedData = result;                // 這才需要鎖
    logToFile(result);                  // 不需要鎖！
    // 臨界區段結束
    
    unlock();
}
```

#### 範例：精確的臨界區段（好）

```cpp
void goodExample() {
    int result = complexCalculation();  // 在鎖外計算
    
    lock();
    sharedData = result;                // 只保護必要的部分
    unlock();
    
    logToFile(result);                  // 在鎖外記錄
}
```

---

### 五、臨界區段設計原則

```
┌─────────────────────────────────────────────────────────────┐
│                 臨界區段設計原則                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 最小化原則                                               │
│     只保護真正需要保護的程式碼                               │
│                                                             │
│  2. 快進快出                                                 │
│     在臨界區段內不做耗時操作                                 │
│                                                             │
│  3. 不要巢狀                                                 │
│     避免在臨界區段內進入另一個臨界區段（易死結）             │
│                                                             │
│  4. 不要等待                                                 │
│     不要在臨界區段內等待外部事件                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 六、識別共享資源

```cpp
#include <iostream>
#include <thread>
#include <vector>

// 共享資源
int globalCounter = 0;                    // 全域變數
static int staticCounter = 0;             // 靜態變數
std::vector<int> sharedVector;            // 全域容器

class MyClass {
    int memberVar;                        // 若多執行緒存取同一物件
    static int staticMember;              // 靜態成員
    
public:
    void method() {
        // memberVar 是否為共享資源？
        // 取決於是否多執行緒存取同一個 MyClass 物件
    }
};

void function() {
    int localVar = 0;                     // 不是共享資源
    static int localStatic = 0;           // 是共享資源！
    thread_local int tlVar = 0;           // 不是共享資源
}
```

---

### 七、常見的共享資源

| 資源類型 | 是否共享 | 需要保護？ |
|----------|----------|------------|
| 全域變數 | 是 | 是 |
| 函式內 static 變數 | 是 | 是 |
| 類別 static 成員 | 是 | 是 |
| 堆積上的物件（共用指標）| 是 | 是 |
| 區域變數 | 否 | 否 |
| thread_local 變數 | 否 | 否 |
| 函式參數（傳值）| 否 | 否 |
| const 全域變數 | 是 | 否（唯讀）|

---

### 八、程式碼標記練習

```cpp
#include <thread>

int shared = 0;

void example(int param) {
    int local = param;           // A
    local += 10;                 // B
    shared = local;              // C
    local = shared;              // D
    int result = local * 2;      // E
    shared += result;            // F
}
```

哪些是臨界區段？

```
A: 不是（只存取區域變數和參數）
B: 不是（只存取區域變數）
C: 是（寫入共享資料）
D: 是（讀取共享資料，且有其他寫入者）
E: 不是（只存取區域變數）
F: 是（讀取 + 寫入共享資料）
```

---

### 九、臨界區段與效能

```
┌─────────────────────────────────────────────────────────────┐
│              臨界區段長度 vs 效能                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  臨界區段太長：                                              │
│  • 其他執行緒等待時間長                                      │
│  • 並行度降低                                                │
│  • 效能接近單執行緒                                          │
│                                                             │
│  臨界區段太短（分散）：                                      │
│  • 頻繁加鎖解鎖                                              │
│  • 鎖的開銷累積                                              │
│  • 可能無法保護完整操作                                      │
│                                                             │
│  平衡：保護完整的邏輯操作，但不包含無關的程式碼               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十、實際案例分析

```cpp
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::vector<int> data;
std::mutex mtx;

// 不好：臨界區段太大
void badPush(int value) {
    std::lock_guard<std::mutex> lock(mtx);
    int processed = value * value;        // 不需要鎖
    data.push_back(processed);            // 需要鎖
    std::cout << "Added: " << processed;  // 可能不需要鎖
}

// 好：臨界區段精確
void goodPush(int value) {
    int processed = value * value;        // 鎖外計算
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(processed);        // 只保護必要操作
    }
    
    std::cout << "Added: " << processed;  // 鎖外輸出
}
```

---

### 十一、本課重點回顧

1. **臨界區段**是存取共享資源的程式碼區域
2. 同一時間只能有一個執行緒執行臨界區段
3. 臨界區段應該**盡量短小**
4. 只保護真正需要保護的程式碼
5. 避免在臨界區段內做耗時操作
6. 區域變數和 thread_local 變數不需要保護

---

### 下一課預告

在 **課程 4.4：資料競爭範例分析** 中，我們將：
- 分析更多真實世界的資料競爭案例
- 學習如何發現潛在的競爭條件
- 探討競爭條件的除錯技巧

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::vector<int> data;
std::mutex mtx;

// 不好：臨界區段太大
void badPush(int value) {
    std::lock_guard<std::mutex> lock(mtx);
    int processed = value * value;        // 不需要鎖
    data.push_back(processed);            // 需要鎖
    std::cout << "Added: " << processed;  // 可能不需要鎖
}

// 好：臨界區段精確
void goodPush(int value) {
    int processed = value * value;        // 鎖外計算
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(processed);        // 只保護必要操作
    }
    
    std::cout << "Added: " << processed;  // 鎖外輸出
}

int main() {
    std::thread t1([]{
        for (int i = 0; i < 5; ++i) {
            goodPush(i);
        }
    });
    
    std::thread t2([]{
        for (int i = 5; i < 10; ++i) {
            goodPush(i);
        }
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
