/*
# 第二階段：std::thread 基礎

## 課程 2.7：執行緒的移動語意

---

### 引言

`std::thread` 是**只能移動、不能複製**的類型。理解這個特性對於正確管理執行緒至關重要。

---

### 一、為什麼不能複製？

```
┌─────────────────────────────────────────────────────────────┐
│              std::thread 不能複製的原因                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  一個執行緒只能有一個擁有者                                   │
│                                                             │
│  如果允許複製：                                              │
│  • 兩個 thread 物件指向同一個執行緒                          │
│  • 誰負責 join？兩個都 join 會崩潰                           │
│  • 一個 detach 後另一個怎麼辦？                              │
│                                                             │
│  所以：複製被禁止，只能移動                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、複製會編譯錯誤

```cpp
#include <thread>

int main() {
    std::thread t1([]() {});
    
    // std::thread t2 = t1;        // 編譯錯誤！
    // std::thread t3(t1);         // 編譯錯誤！
    
    t1.join();
    return 0;
}
```

錯誤訊息：
```
error: use of deleted function 'std::thread::thread(const std::thread&)'
```

---

### 三、使用 std::move 轉移所有權

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread t1([]() {
        std::cout << "執行中" << std::endl;
    });
    
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    
    // 移動所有權
    std::thread t2 = std::move(t1);
    
    std::cout << "移動後:" << std::endl;
    std::cout << "t1 joinable: " << t1.joinable() << std::endl;
    std::cout << "t2 joinable: " << t2.joinable() << std::endl;
    
    t2.join();  // 由 t2 負責 join
    return 0;
}
```

輸出：
```
t1 joinable: 1
移動後:
t1 joinable: 0
t2 joinable: 1
執行中
```

---

### 四、移動語意圖解

```
移動前：
┌─────────┐         ┌─────────────┐
│   t1    │────────►│  執行緒資源  │
│joinable │         │             │
└─────────┘         └─────────────┘

std::move(t1) 後：
┌─────────┐         ┌─────────────┐
│   t1    │    ┌───►│  執行緒資源  │
│  空的   │    │    │             │
└─────────┘    │    └─────────────┘
               │
┌─────────┐    │
│   t2    │────┘
│joinable │
└─────────┘
```

---

### 五、函式回傳執行緒

函式可以回傳 `std::thread`（自動移動）：

```cpp
#include <iostream>
#include <thread>

std::thread createThread(int id) {
    return std::thread([id]() {
        std::cout << "執行緒 " << id << std::endl;
    });
}

int main() {
    std::thread t = createThread(42);
    t.join();
    return 0;
}
```

---

### 六、函式接受執行緒參數

```cpp
#include <iostream>
#include <thread>

void takeOwnership(std::thread t) {
    std::cout << "取得執行緒所有權" << std::endl;
    t.join();
}

int main() {
    std::thread t([]() {
        std::cout << "工作中" << std::endl;
    });
    
    takeOwnership(std::move(t));  // 必須 move
    
    // t 現在是空的
    std::cout << "t joinable: " << t.joinable() << std::endl;
    
    return 0;
}
```

---

### 七、執行緒容器

使用 `std::vector` 管理多個執行緒：

```cpp
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::vector<std::thread> threads;
    
    // 建立多個執行緒
    for (int i = 0; i < 5; ++i) {
        threads.push_back(std::thread([i]() {
            std::cout << "執行緒 " << i << std::endl;
        }));
        // 或使用 emplace_back
        // threads.emplace_back([i]() { ... });
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

---

### 八、移動賦值的陷阱

移動到一個 joinable 的執行緒會導致程式終止：

```cpp
#include <thread>

int main() {
    std::thread t1([]() {});
    std::thread t2([]() {});
    
    // 危險！t1 還是 joinable，程式會呼叫 std::terminate()
    // t1 = std::move(t2);
    
    // 正確做法：先 join 或 detach
    t1.join();
    t1 = std::move(t2);  // 現在安全了
    t1.join();
    
    return 0;
}
```

---

### 九、安全的執行緒包裝類別

```cpp
#include <iostream>
#include <thread>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
    
    // 允許移動
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;
};

int main() {
    ScopedThread st(std::thread([]() {
        std::cout << "自動管理的執行緒" << std::endl;
    }));
    
    // 離開作用域時自動 join
    return 0;
}
```

---

### 十、本課重點回顧

1. `std::thread` **不能複製**，只能移動
2. 使用 `std::move()` 轉移執行緒所有權
3. 移動後原物件變成 non-joinable
4. 函式可以回傳 `std::thread`（自動移動）
5. `std::vector<std::thread>` 可管理多個執行緒
6. 不可移動到仍是 joinable 的執行緒物件

---

### 第二階段完成！

恭喜你完成了 `std::thread` 基礎階段！你已經學會：

- ✅ 建立與啟動執行緒
- ✅ 各種可呼叫物件的使用
- ✅ 參數傳遞與 std::ref
- ✅ join() 與 detach() 的選擇
- ✅ joinable() 狀態檢查
- ✅ 執行緒 ID 與硬體資訊
- ✅ 移動語意與執行緒容器

---

### 下一階段預告

**第三階段：執行緒生命週期管理** 將深入探討：
- 課程 3.1：RAII 與執行緒管理
- 課程 3.2：執行緒守衛類別設計
- 課程 3.3：std::jthread (C++20)
- ...

---

準備好進入第三階段嗎？
*/



#include <iostream>
#include <thread>

class ScopedThread {
    std::thread t;
public:
    explicit ScopedThread(std::thread thread) 
        : t(std::move(thread)) {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }
    
    ~ScopedThread() {
        t.join();
    }
    
    // 禁止複製
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
    
    // 允許移動
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;
};

int main() {
    ScopedThread st(std::thread([]() {
        std::cout << "自動管理的執行緒" << std::endl;
    }));
    
    // 離開作用域時自動 join
    return 0;
}
