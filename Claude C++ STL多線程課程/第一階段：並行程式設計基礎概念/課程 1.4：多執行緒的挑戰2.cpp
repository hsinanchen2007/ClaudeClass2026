/*
# 第一階段：並行程式設計基礎概念

## 課程 1.4：多執行緒的挑戰

---

### 引言

多執行緒程式設計能帶來效能提升，但也引入了單執行緒程式不會遇到的問題。本課介紹三個最常見的挑戰：**競爭條件（Race Condition）**、**死結（Deadlock）**、**飢餓（Starvation）**。

---

### 一、競爭條件（Race Condition）

當多個執行緒同時存取共享資料，且至少一個執行緒進行寫入時，結果取決於執行緒的執行順序，這就是競爭條件。

#### 範例：競爭條件的發生

```cpp
// 檔案：lesson_1_4_race_condition.cpp

#include <iostream>
#include <thread>

int counter = 0;

void incrementManyTimes() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;  // 這不是原子操作！
    }
}

int main() {
    std::thread t1(incrementManyTimes);
    std::thread t2(incrementManyTimes);
    
    t1.join();
    t2.join();
    
    std::cout << "預期: 200000" << std::endl;
    std::cout << "實際: " << counter << std::endl;
    
    return 0;
}
```

#### 為什麼會出錯？

`++counter` 看似一個操作，實際上是三個步驟：

```
1. 讀取 counter 的值到暫存器
2. 暫存器的值 +1
3. 將結果寫回 counter
```

兩個執行緒可能交錯執行：

```
時間   執行緒1              執行緒2           counter
────────────────────────────────────────────────────
 1    讀取 counter (0)                          0
 2                         讀取 counter (0)     0
 3    加 1 (得到 1)                             0
 4                         加 1 (得到 1)        0
 5    寫回 (1)                                  1
 6                         寫回 (1)             1
────────────────────────────────────────────────────
結果：兩次 +1，但 counter 只增加了 1！
```

---

### 二、死結（Deadlock）

當兩個或多個執行緒互相等待對方釋放資源，導致所有執行緒都無法繼續執行。

#### 範例：死結的發生

```cpp
// 檔案：lesson_1_4_deadlock.cpp

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    std::cout << "執行緒1: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();
    std::cout << "執行緒1: 已獲取 mutexA" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒1: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();  // 等待執行緒2釋放 mutexB
    std::cout << "執行緒1: 已獲取 mutexB" << std::endl;
    
    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    std::cout << "執行緒2: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();
    std::cout << "執行緒2: 已獲取 mutexB" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒2: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();  // 等待執行緒1釋放 mutexA
    std::cout << "執行緒2: 已獲取 mutexA" << std::endl;
    
    mutexA.unlock();
    mutexB.unlock();
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    
    t1.join();
    t2.join();
    
    std::cout << "完成（這行永遠不會執行）" << std::endl;
    return 0;
}
```

#### 死結示意圖

```
┌──────────────┐         ┌──────────────┐
│   執行緒 1   │         │   執行緒 2   │
└──────┬───────┘         └───────┬──────┘
       │                         │
       │ 持有 mutexA             │ 持有 mutexB
       │                         │
       │ 等待 mutexB ──────────► │
       │ ◄────────────── 等待 mutexA
       │                         │
       ▼                         ▼
    [阻塞]                    [阻塞]
    
    互相等待，永遠無法繼續！
```

---

### 三、飢餓（Starvation）

某個執行緒長時間無法獲得所需資源，導致無法執行。

#### 常見原因

```
┌─────────────────────────────────────────┐
│            飢餓的常見原因                │
├─────────────────────────────────────────┤
│                                         │
│  1. 優先權不公平                         │
│     高優先權執行緒總是搶先執行            │
│                                         │
│  2. 鎖的獲取不公平                       │
│     某些執行緒總是比其他執行緒先獲得鎖    │
│                                         │
│  3. 資源分配策略問題                     │
│     某些執行緒持有資源時間過長            │
│                                         │
└─────────────────────────────────────────┘
```

#### 簡單示意

```cpp
// 概念示意（非完整程式碼）

// 執行緒 A：非常活躍，頻繁獲取鎖
while (true) {
    lock();
    // 快速完成工作
    unlock();
    // 幾乎立即再次嘗試獲取鎖
}

// 執行緒 B：很少有機會獲取鎖
while (true) {
    lock();    // 總是等待，因為 A 太快了
    // 工作
    unlock();
}
```

---

### 四、三大問題對照表

| 問題 | 原因 | 症狀 | 解決方向 |
|------|------|------|----------|
| 競爭條件 | 非同步存取共享資料 | 結果不可預測 | 互斥鎖、原子操作 |
| 死結 | 循環等待資源 | 程式卡住不動 | 鎖順序、超時機制 |
| 飢餓 | 資源分配不公平 | 某執行緒無法執行 | 公平鎖、優先權調整 |

---

### 五、死結的四個必要條件（Coffman Conditions）

死結發生必須同時滿足以下四個條件：

```
1. 互斥 (Mutual Exclusion)
   資源一次只能被一個執行緒使用

2. 持有並等待 (Hold and Wait)
   執行緒持有資源的同時等待其他資源

3. 不可搶占 (No Preemption)
   已分配的資源不能被強制取走

4. 循環等待 (Circular Wait)
   存在執行緒的循環等待鏈
```

**打破任一條件即可避免死結。**

---

### 六、本課重點回顧

1. **競爭條件**：多執行緒同時存取共享資料導致結果不可預測
2. **死結**：執行緒互相等待對方的資源，導致全部卡住
3. **飢餓**：某執行緒長期無法獲得資源
4. 死結需要四個條件同時成立才會發生
5. 這些問題都有對應的解決方案（後續課程會詳細介紹）

---

### 下一課預告

在 **課程 1.5：C++ 多執行緒發展史** 中，我們將了解：
- C++11 之前如何處理多執行緒
- C++11 標準執行緒函式庫的誕生
- C++14/17/20 的持續演進

---

準備好繼續嗎？
*/

// 檔案：lesson_1_4_deadlock.cpp

#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutexA;
std::mutex mutexB;

void thread1() {
    std::cout << "執行緒1: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();
    std::cout << "執行緒1: 已獲取 mutexA" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒1: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();  // 等待執行緒2釋放 mutexB
    std::cout << "執行緒1: 已獲取 mutexB" << std::endl;
    
    mutexB.unlock();
    mutexA.unlock();
}

void thread2() {
    std::cout << "執行緒2: 嘗試獲取 mutexB..." << std::endl;
    mutexB.lock();
    std::cout << "執行緒2: 已獲取 mutexB" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "執行緒2: 嘗試獲取 mutexA..." << std::endl;
    mutexA.lock();  // 等待執行緒1釋放 mutexA
    std::cout << "執行緒2: 已獲取 mutexA" << std::endl;
    
    mutexA.unlock();
    mutexB.unlock();
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);
    
    t1.join();
    t2.join();
    
    std::cout << "完成（這行永遠不會執行）" << std::endl;
    return 0;
}
