/*
好的，讓我先查看我們之前的課程進度和大綱。找到了課程大綱。讓我再確認第五階段的完整內容：很好，我確認了課程進度。你已經完成了第四階段（共享資料與競爭條件），現在正式開始第五階段。

---

# 第五階段：互斥鎖基礎 (std::mutex)

## 課程 5.1：std::mutex 基本操作

---

### 引言

在第四階段，我們深入了解了競爭條件的危害。現在，我們要學習解決這個問題的第一個武器：**互斥鎖（Mutex）**。Mutex 是 **Mut**ual **Ex**clusion 的縮寫，意思是「互相排斥」，確保同一時間只有一個執行緒能存取共享資源。

---

### 一、std::mutex 基本概念

```
┌─────────────────────────────────────────────────────────────┐
│                    互斥鎖（Mutex）概念                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  想像一間只能容納一人的更衣室：                               │
│                                                             │
│    ┌──────────┐                                             │
│    │  更衣室  │  ← 共享資源（如：全域變數）                   │
│    │  🔒門鎖  │  ← 互斥鎖                                    │
│    └──────────┘                                             │
│                                                             │
│  規則：                                                      │
│  • 進入前必須鎖門（lock）                                    │
│  • 離開後必須開門（unlock）                                  │
│  • 門鎖著時，其他人必須在外等待                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、std::mutex 的三個基本操作

```cpp
#include <mutex>

std::mutex mtx;  // 建立互斥鎖

mtx.lock();      // 🔒 獲取鎖（若已被鎖定，則阻塞等待）
// ... 臨界區段：存取共享資源 ...
mtx.unlock();    // 🔓 釋放鎖
```

| 方法 | 功能 | 阻塞行為 |
|------|------|----------|
| `lock()` | 獲取鎖 | 若鎖已被持有，**阻塞等待**直到獲取成功 |
| `unlock()` | 釋放鎖 | 非阻塞 |
| `try_lock()` | 嘗試獲取鎖 | **非阻塞**，立即返回成功或失敗 |

---

### 三、解決競爭條件：完整範例

#### 問題回顧（無保護的程式碼）

```cpp
// 檔案：lesson_5_1_no_mutex.cpp
// 說明：沒有互斥鎖保護的計數器（有競爭條件）

#include <iostream>
#include <thread>

int counter = 0;  // 共享資源

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;  // 危險！非原子操作
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;  // 通常小於 200000
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o no_mutex lesson_5_1_no_mutex.cpp
./no_mutex
```

#### 可能的輸出

```
預期: 200000
實際: 143287
```

---

#### 解決方案（使用 std::mutex）

```cpp
// 檔案：lesson_5_1_with_mutex.cpp
// 說明：使用互斥鎖保護共享資源

#include <iostream>
#include <thread>
#include <mutex>

int counter = 0;      // 共享資源
std::mutex mtx;       // 保護 counter 的互斥鎖

void increment() {
    for (int i = 0; i < 100000; ++i) {
        mtx.lock();       // 🔒 進入臨界區段前獲取鎖
        ++counter;        // 安全！一次只有一個執行緒能執行
        mtx.unlock();     // 🔓 離開臨界區段後釋放鎖
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;  // 保證是 200000
    
    return 0;
}
```

#### 編譯與執行

```bash
g++ -std=c++17 -pthread -o with_mutex lesson_5_1_with_mutex.cpp
./with_mutex
```

#### 輸出

```
預期: 200000
實際: 200000
```

---

### 四、執行流程圖解

```
時間軸 ──────────────────────────────────────────────────────────►

執行緒 1:   lock()─────[++counter]─────unlock()   lock()─────...
                                                    │
                                                    │ 獲取鎖
                                                    ▼
執行緒 2:   lock()  ════════等待════════════════  [++counter]─unlock()
              │                                      ▲
              │ 嘗試獲取鎖，但被阻塞                   │
              └──────────────────────────────────────┘
                    執行緒 1 釋放鎖後，執行緒 2 獲取鎖

════════ 表示阻塞等待
─────── 表示持有鎖執行
```

---

### 五、多個操作的保護

當臨界區段包含多個操作時：

```cpp
// 檔案：lesson_5_1_multiple_ops.cpp
// 說明：保護多個相關操作

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

std::vector<int> data;
std::mutex mtx;

void addAndPrint(int value) {
    mtx.lock();
    
    // 以下三個操作必須作為一個整體執行
    data.push_back(value);                    // 操作 1：新增元素
    std::cout << "Added: " << value;          // 操作 2：輸出訊息
    std::cout << ", Size: " << data.size();   // 操作 3：輸出大小
    std::cout << std::endl;
    
    mtx.unlock();
}

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 5; ++i) {
            addAndPrint(i);
        }
    });
    
    std::thread t2([]() {
        for (int i = 100; i < 105; ++i) {
            addAndPrint(i);
        }
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

#### 輸出（順序可能不同，但每行是完整的）

```
Added: 0, Size: 1
Added: 100, Size: 2
Added: 1, Size: 3
Added: 101, Size: 4
Added: 2, Size: 5
...
```

若沒有互斥鎖，輸出可能會交錯混亂：

```
Added: 0Added: 100, Size: , Size: 21
```

---

### 六、lock() 與 unlock() 的配對規則

```
┌─────────────────────────────────────────────────────────────┐
│                    配對規則（重要！）                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✓ 正確：每個 lock() 都有對應的 unlock()                    │
│                                                             │
│      mtx.lock();                                            │
│      // 工作                                                │
│      mtx.unlock();  // ✓ 配對                               │
│                                                             │
│  ✗ 錯誤 1：忘記 unlock()                                    │
│                                                             │
│      mtx.lock();                                            │
│      // 工作                                                │
│      // 沒有 unlock() → 其他執行緒永遠等待（死結）           │
│                                                             │
│  ✗ 錯誤 2：重複 lock()（同一執行緒）                        │
│                                                             │
│      mtx.lock();                                            │
│      mtx.lock();  // 💀 未定義行為！（標準 mutex 不支援）    │
│                                                             │
│  ✗ 錯誤 3：unlock() 未鎖定的互斥鎖                          │
│                                                             │
│      mtx.unlock();  // 💀 未定義行為！                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、忘記 unlock() 的危險

```cpp
// 檔案：lesson_5_1_forget_unlock.cpp
// 說明：忘記 unlock() 導致的問題（示範用，不要在實際專案這樣寫）

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void badFunction() {
    mtx.lock();
    std::cout << "Thread " << std::this_thread::get_id() << " acquired lock\n";
    
    // 假設這裡發生早期返回或拋出例外
    if (true) {
        return;  // 💀 忘記 unlock()！
    }
    
    mtx.unlock();  // 永遠不會執行
}

int main() {
    std::thread t1(badFunction);
    t1.join();
    
    std::cout << "Thread 1 finished\n";
    
    std::thread t2(badFunction);  // 💀 永遠卡住！因為鎖沒被釋放
    t2.join();
    
    std::cout << "Thread 2 finished\n";  // 永遠不會執行
    
    return 0;
}
```

這就是為什麼我們需要 RAII 鎖管理器（下一階段會學習）。

---

### 八、std::mutex 的特性

```
┌─────────────────────────────────────────────────────────────┐
│                   std::mutex 特性                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  • 不可複製、不可移動                                        │
│    std::mutex mtx1;                                         │
│    std::mutex mtx2 = mtx1;  // ✗ 編譯錯誤                   │
│                                                             │
│  • 不支援遞迴鎖定                                           │
│    同一執行緒 lock() 兩次 → 未定義行為                       │
│    （需要遞迴鎖定時，使用 std::recursive_mutex）             │
│                                                             │
│  • lock() 可能拋出例外                                      │
│    std::system_error（如：資源不足）                        │
│                                                             │
│  • 預設建構，無需參數                                        │
│    std::mutex mtx;  // 即可使用                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 九、本課重點回顧

1. **std::mutex** 是最基本的同步機制，確保臨界區段的互斥存取
2. 三個基本操作：`lock()`、`unlock()`、`try_lock()`
3. `lock()` 會阻塞等待直到獲取鎖
4. **每個 `lock()` 必須有對應的 `unlock()`**
5. 忘記 `unlock()` 會導致其他執行緒永遠等待
6. std::mutex 不支援同一執行緒重複鎖定

---

### 十、練習題

**練習 1**：修改以下程式碼，使用 std::mutex 解決競爭條件

```cpp
#include <iostream>
#include <thread>

int balance = 1000;

void withdraw(int amount) {
    if (balance >= amount) {
        balance -= amount;
        std::cout << "Withdrew: " << amount << ", Balance: " << balance << std::endl;
    }
}

int main() {
    std::thread t1(withdraw, 800);
    std::thread t2(withdraw, 800);
    
    t1.join();
    t2.join();
    
    std::cout << "Final balance: " << balance << std::endl;
    return 0;
}
```

---

### 下一課預告

在 **課程 5.2：互斥鎖的工作原理** 中，我們將深入了解：
- 互斥鎖在作業系統層面如何運作
- 自旋鎖 vs 睡眠鎖
- 鎖的獲取與釋放過程

---

準備好繼續嗎？
*/



// 檔案：lesson_5_1_forget_unlock.cpp
// 說明：忘記 unlock() 導致的問題（示範用，不要在實際專案這樣寫）

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;

void badFunction() {
    mtx.lock();
    std::cout << "Thread " << std::this_thread::get_id() << " acquired lock\n";
    
    // 假設這裡發生早期返回或拋出例外
    if (true) {
        return;  // 💀 忘記 unlock()！
    }
    
    mtx.unlock();  // 永遠不會執行
}

int main() {
    std::thread t1(badFunction);
    t1.join();
    
    std::cout << "Thread 1 finished\n";
    
    std::thread t2(badFunction);  // 💀 永遠卡住！因為鎖沒被釋放
    t2.join();
    
    std::cout << "Thread 2 finished\n";  // 永遠不會執行
    
    return 0;
}
