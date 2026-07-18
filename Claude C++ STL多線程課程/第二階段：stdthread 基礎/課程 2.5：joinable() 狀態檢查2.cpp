/*
# 第二階段：std::thread 基礎

## 課程 2.5：joinable() 狀態檢查

---

### 引言

`joinable()` 是判斷 `std::thread` 物件狀態的關鍵方法。理解何時執行緒是 joinable，能幫助你避免程式崩潰。

---

### 一、什麼是 joinable？

一個 `std::thread` 物件是 **joinable** 代表：
- 它關聯著一個真正的執行緒
- 該執行緒尚未被 join 或 detach

```
┌─────────────────────────────────────────┐
│         joinable 的意義                 │
├─────────────────────────────────────────┤
│                                         │
│  joinable() == true                     │
│  → 有一個執行緒正在運行（或已結束但未回收）│
│  → 必須呼叫 join() 或 detach()          │
│                                         │
│  joinable() == false                    │
│  → 沒有關聯的執行緒                      │
│  → 不需要（也不能）呼叫 join/detach      │
│                                         │
└─────────────────────────────────────────┘
```

---

### 二、何時 joinable 為 false？

```cpp
#include <iostream>
#include <thread>

int main() {
    // 情況 1：預設建構
    std::thread t1;
    std::cout << "預設建構: " << t1.joinable() << std::endl;
    
    // 情況 2：已經 join
    std::thread t2([]() {});
    t2.join();
    std::cout << "join 後: " << t2.joinable() << std::endl;
    
    // 情況 3：已經 detach
    std::thread t3([]() {});
    t3.detach();
    std::cout << "detach 後: " << t3.joinable() << std::endl;
    
    // 情況 4：被 move 走
    std::thread t4([]() {});
    std::thread t5 = std::move(t4);
    std::cout << "move 後(原): " << t4.joinable() << std::endl;
    std::cout << "move 後(新): " << t5.joinable() << std::endl;
    t5.join();
    
    return 0;
}
```

輸出：
```
預設建構: 0
join 後: 0
detach 後: 0
move 後(原): 0
move 後(新): 1
```

---

### 三、狀態轉換圖

```
                    ┌─────────────────┐
                    │  預設建構        │
                    │  joinable=false │
                    └────────┬────────┘
                             │ 賦值帶有執行緒的物件
                             ▼
┌──────────────┐    ┌─────────────────┐    ┌──────────────┐
│   join()     │◄───│   執行中         │───►│   detach()   │
│              │    │  joinable=true  │    │              │
└──────┬───────┘    └────────┬────────┘    └──────┬───────┘
       │                     │                    │
       │                     │ std::move()        │
       ▼                     ▼                    ▼
┌─────────────────────────────────────────────────────────┐
│                    joinable=false                        │
│               （不再關聯任何執行緒）                       │
└─────────────────────────────────────────────────────────┘
```

---

### 四、安全的執行緒管理模式

```cpp
#include <iostream>
#include <thread>

class SafeThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    void start(Func&& f, Args&&... args) {
        if (t.joinable()) {
            t.join();  // 先結束舊的
        }
        t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
    }
    
    void join() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    ~SafeThread() {
        join();  // 解構時自動 join
    }
};

int main() {
    SafeThread st;
    
    st.start([]() {
        std::cout << "任務 1" << std::endl;
    });
    
    st.start([]() {  // 自動 join 前一個
        std::cout << "任務 2" << std::endl;
    });
    
    // 解構時自動 join
    return 0;
}
```

---

### 五、常見錯誤與修正

#### 錯誤：不檢查就 join

```cpp
// 錯誤
std::thread t;
t.join();  // 崩潰！t 不是 joinable

// 正確
std::thread t;
if (t.joinable()) {
    t.join();
}
```

#### 錯誤：條件式建立後忘記處理

```cpp
// 錯誤
std::thread t;
if (someCondition) {
    t = std::thread(func);
}
t.join();  // 若條件為 false，崩潰！

// 正確
std::thread t;
if (someCondition) {
    t = std::thread(func);
}
if (t.joinable()) {
    t.join();
}
```

---

### 六、joinable 檢查的時機

| 情境 | 是否需要檢查 |
|------|-------------|
| 剛建立的執行緒，立即 join | 不需要（一定是 joinable）|
| 可能被 move 走的執行緒 | 需要 |
| 條件式建立的執行緒 | 需要 |
| 迴圈中重複使用的執行緒變數 | 需要 |
| 類別解構函式中 | 需要 |

---

### 七、本課重點回顧

1. `joinable()` 檢查執行緒物件是否關聯著一個執行緒
2. 預設建構、join 後、detach 後、move 後都是 non-joinable
3. 對 non-joinable 呼叫 `join()` 或 `detach()` 會拋出例外
4. 在不確定狀態時，先用 `joinable()` 檢查
5. 類別解構函式中應該檢查並 join

---

### 下一課預告

在 **課程 2.6：執行緒識別與資訊** 中，我們將學習：
- `get_id()` 取得執行緒 ID
- `hardware_concurrency()` 查詢 CPU 核心數
- `std::this_thread` 命名空間

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>

class SafeThread {
    std::thread t;
    
public:
    template<typename Func, typename... Args>
    void start(Func&& f, Args&&... args) {
        if (t.joinable()) {
            t.join();  // 先結束舊的
        }
        t = std::thread(std::forward<Func>(f), std::forward<Args>(args)...);
    }
    
    void join() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    ~SafeThread() {
        join();  // 解構時自動 join
    }
};

int main() {
    SafeThread st;
    
    st.start([]() {
        std::cout << "任務 1" << std::endl;
    });
    
    st.start([]() {  // 自動 join 前一個
        std::cout << "任務 2" << std::endl;
    });
    
    // 解構時自動 join
    return 0;
}
