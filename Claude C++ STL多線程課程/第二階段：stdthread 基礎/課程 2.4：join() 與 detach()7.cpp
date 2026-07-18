/*
# 第二階段：std::thread 基礎

## 課程 2.4：join() 與 detach()

---

### 引言

每個 `std::thread` 物件在解構前，必須明確決定如何處理其執行緒：等待它完成（join）或放手讓它獨立運行（detach）。這是執行緒管理最重要的決策。

---

### 一、兩種選擇

```
┌─────────────────────────────────────────────────────────────┐
│                  join() vs detach()                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  join()                        detach()                     │
│  ──────                        ────────                     │
│  • 阻塞等待執行緒結束           • 讓執行緒獨立運行            │
│  • 確保執行緒完成工作           • 不等待，立即返回            │
│  • 可以安全存取執行緒的結果     • 無法再與該執行緒互動         │
│  • 較常使用                    • 適合背景任務                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、join() 的阻塞行為

`join()` 會阻塞當前執行緒，直到目標執行緒結束：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "1. 建立執行緒" << std::endl;
    
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "3. 執行緒完成" << std::endl;
    });
    
    std::cout << "2. 呼叫 join()，開始等待..." << std::endl;
    t.join();  // 在這裡阻塞 2 秒
    
    std::cout << "4. join() 返回，繼續執行" << std::endl;
    return 0;
}
```

輸出：
```
1. 建立執行緒
2. 呼叫 join()，開始等待...
3. 執行緒完成
4. join() 返回，繼續執行
```

---

### 三、detach() 的獨立運行

`detach()` 讓執行緒在背景獨立執行：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "背景執行緒完成" << std::endl;  // 可能不會印出
    });
    
    t.detach();  // 放手，不等待
    
    std::cout << "主執行緒立即繼續" << std::endl;
    // 主程式結束時，背景執行緒會被強制終止
    return 0;
}
```

輸出（通常）：
```
主執行緒立即繼續
```

---

### 四、detach() 的危險

分離後的執行緒若存取已銷毀的資源，會導致未定義行為：

```cpp
#include <iostream>
#include <thread>
#include <chrono>

void dangerous() {
    int localVar = 42;
    
    std::thread t([&localVar]() {  // 捕獲區域變數的引用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << localVar << std::endl;  // 危險！localVar 可能已被銷毀
    });
    
    t.detach();
}  // localVar 在這裡被銷毀，但執行緒還在運行

int main() {
    dangerous();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
```

**規則**：detach 的執行緒不應存取可能被銷毀的區域變數。

---

### 五、joinable() 狀態檢查

在呼叫 join() 或 detach() 前，可以用 `joinable()` 檢查：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Working" << std::endl;
    });
    
    std::cout << "joinable: " << t.joinable() << std::endl;  // true
    
    t.join();
    
    std::cout << "joinable: " << t.joinable() << std::endl;  // false
    
    return 0;
}
```

輸出：
```
joinable: 1
Working
joinable: 0
```

---

### 六、joinable() 的狀態轉換

```
┌─────────────────────────────────────────────────┐
│           joinable() 狀態變化                   │
├─────────────────────────────────────────────────┤
│                                                 │
│  std::thread t;          → joinable() = false  │
│  std::thread t(func);    → joinable() = true   │
│                                                 │
│  呼叫 join() 後          → joinable() = false  │
│  呼叫 detach() 後        → joinable() = false  │
│  被 move 走後            → joinable() = false  │
│                                                 │
└─────────────────────────────────────────────────┘
```

---

### 七、安全的 join 檢查

避免對 non-joinable 執行緒呼叫 join()：

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {
        std::cout << "Task" << std::endl;
    });
    
    // 安全的寫法
    if (t.joinable()) {
        t.join();
    }
    
    // 再次呼叫是安全的（因為有檢查）
    if (t.joinable()) {
        t.join();  // 不會執行
    }
    
    return 0;
}
```

---

### 八、重複呼叫 join() 會崩潰

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t([]() {});
    
    t.join();
    t.join();  // 崩潰！std::system_error
    
    return 0;
}
```

---

### 九、何時用 join，何時用 detach

| 情況 | 選擇 | 原因 |
|------|------|------|
| 需要執行緒的結果 | join | 必須等待完成 |
| 後續操作依賴執行緒完成 | join | 確保順序 |
| 真正的背景任務（如日誌） | detach | 不需要結果 |
| 執行緒使用區域變數 | join | 避免懸空引用 |
| 不確定時 | join | 較安全 |

---

### 十、完整範例

```cpp
// 檔案：lesson_2_4_join_detach.cpp

#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 範例 1：join
    std::thread t1([]() {
        std::cout << "[t1] 開始工作" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[t1] 完成" << std::endl;
    });
    
    std::cout << "[main] 等待 t1..." << std::endl;
    t1.join();
    std::cout << "[main] t1 已結束" << std::endl;
    
    // 範例 2：detach
    std::thread t2([]() {
        std::cout << "[t2] 背景執行中..." << std::endl;
    });
    
    t2.detach();
    std::cout << "[main] t2 已分離" << std::endl;
    
    // 給 t2 一點時間完成輸出
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return 0;
}
```

---

### 十一、本課重點回顧

1. 每個 `std::thread` 必須呼叫 `join()` 或 `detach()`
2. `join()` 阻塞等待執行緒結束
3. `detach()` 讓執行緒獨立運行，但要小心資源生命週期
4. `joinable()` 檢查執行緒是否可以被 join 或 detach
5. 對 non-joinable 執行緒呼叫 join() 會崩潰
6. 不確定時，優先選擇 `join()`

---

### 下一課預告

在 **課程 2.5：joinable() 狀態檢查** 中，我們將更深入探討：
- 執行緒物件的各種狀態
- 預設建構的執行緒
- 狀態轉換的完整圖解

---

準備好繼續嗎？
*/



// 檔案：lesson_2_4_join_detach.cpp

#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // 範例 1：join
    std::thread t1([]() {
        std::cout << "[t1] 開始工作" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[t1] 完成" << std::endl;
    });
    
    std::cout << "[main] 等待 t1..." << std::endl;
    t1.join();
    std::cout << "[main] t1 已結束" << std::endl;
    
    // 範例 2：detach
    std::thread t2([]() {
        std::cout << "[t2] 背景執行中..." << std::endl;
    });
    
    t2.detach();
    std::cout << "[main] t2 已分離" << std::endl;
    
    // 給 t2 一點時間完成輸出
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    return 0;
}
