/*
# 第四階段：共享資料與競爭條件

## 課程 4.2：不變量與競爭條件

---

### 引言

要理解競爭條件為何危險，必須先理解**不變量（Invariant）**的概念。不變量是資料結構必須永遠保持為真的條件，競爭條件的危害在於它會破壞不變量。

---

### 一、什麼是不變量

```
┌─────────────────────────────────────────────────────────────┐
│                    不變量（Invariant）                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  定義：資料結構在任何「可觀察」時刻都必須滿足的條件           │
│                                                             │
│  例子：                                                      │
│  • 雙向鏈結串列：A.next = B  則  B.prev = A                 │
│  • 銀行帳戶：轉帳前後總金額不變                              │
│  • 二元搜尋樹：左子節點 < 父節點 < 右子節點                  │
│  • 堆疊：size 等於實際元素數量                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、不變量在操作中的暫時破壞

進行複合操作時，不變量可能**暫時**被破壞：

```cpp
// 雙向鏈結串列插入節點
// 不變量：A.next->prev == A

// 初始狀態：A <-> C
// 目標：A <-> B <-> C

// 步驟 1：B.next = C
//   A -> C    B -> C    不變量暫時破壞！
//   A <- C              B.prev 還沒設定

// 步驟 2：B.prev = A
//   A -> C    B -> C
//   A <- C    B <- A    不變量仍破壞！

// 步驟 3：A.next = B
//   A -> B -> C
//   A <- C    B <- A    不變量仍破壞！

// 步驟 4：C.prev = B
//   A -> B -> C
//   A <- B <- C         不變量恢復 ✓
```

---

### 三、單執行緒下沒問題

```cpp
#include <iostream>

struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
};

void insertAfter(Node* a, Node* newNode, Node* c) {
    // 不變量暫時被破壞，但沒關係
    // 因為沒有其他人會看到中間狀態
    newNode->next = c;
    newNode->prev = a;
    a->next = newNode;
    c->prev = newNode;
    // 不變量恢復
}

int main() {
    Node a{1}, b{2}, c{3};
    a.next = &c;
    c.prev = &a;
    
    insertAfter(&a, &b, &c);  // 安全：單執行緒
    
    std::cout << a.next->data << std::endl;  // 2
    return 0;
}
```

---

### 四、多執行緒下的災難

```cpp
#include <iostream>
#include <thread>

struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
};

Node a{1}, b{2}, c{3};

void writer() {
    // 插入 b 到 a 和 c 之間
    b.next = &c;
    b.prev = &a;
    // ← 此時另一個執行緒可能讀取！
    a.next = &b;
    c.prev = &b;
}

void reader() {
    // 嘗試遍歷鏈結串列
    Node* current = &a;
    while (current != nullptr) {
        std::cout << current->data << " ";
        current = current->next;
    }
    std::cout << std::endl;
}

int main() {
    a.next = &c;
    c.prev = &a;
    
    std::thread t1(writer);
    std::thread t2(reader);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

可能的輸出：
```
1 3           // 正常（在修改前讀取）
1 2 3         // 正常（在修改後讀取）
1             // 異常！讀到中間狀態
```

---

### 五、銀行帳戶的不變量

```cpp
#include <iostream>
#include <thread>

struct Bank {
    int accountA = 1000;
    int accountB = 1000;
    // 不變量：accountA + accountB == 2000
};

Bank bank;

void transfer(int amount) {
    // 不變量暫時破壞
    bank.accountA -= amount;
    // ← 此刻總額不是 2000！
    bank.accountB += amount;
    // 不變量恢復
}

void audit() {
    int total = bank.accountA + bank.accountB;
    if (total != 2000) {
        std::cout << "警告！總額異常: " << total << std::endl;
    }
}

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 1000; ++i) transfer(1);
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 1000; ++i) audit();
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

可能輸出：
```
警告！總額異常: 1999
警告！總額異常: 2001
警告！總額異常: 1999
```

---

### 六、競爭條件的本質

```
┌─────────────────────────────────────────────────────────────┐
│                   競爭條件的本質                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  競爭條件 = 其他執行緒看到了不變量被破壞的中間狀態           │
│                                                             │
│  時間線：                                                    │
│                                                             │
│  執行緒A:  [不變量成立] → [破壞] → [恢復] → [不變量成立]    │
│                              ↑                              │
│  執行緒B:              在此讀取 = 看到不一致的資料！         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 七、常見的不變量破壞場景

#### 場景一：容器的 size

```cpp
#include <vector>
#include <thread>

std::vector<int> vec;
// 不變量：vec.size() == 實際元素數量

void unsafeAdd(int value) {
    vec.push_back(value);  // 可能破壞內部不變量
}
```

#### 場景二：物件的狀態

```cpp
class Connection {
    bool connected = false;
    int socket = -1;
    // 不變量：connected == true 時，socket >= 0
    
public:
    void connect() {
        socket = openSocket();  // socket 已設定
        // ← 此刻 connected 還是 false！
        connected = true;
    }
};
```

---

### 八、如何保護不變量

```
┌─────────────────────────────────────────────────────────────┐
│                   保護不變量的方法                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 互斥鎖                                                   │
│     讓整個操作成為原子，其他執行緒看不到中間狀態             │
│                                                             │
│  2. 原子操作                                                 │
│     對於簡單資料，使用硬體保證的原子操作                     │
│                                                             │
│  3. 事務性操作                                               │
│     先準備好所有資料，最後一步原子切換                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 九、使用互斥鎖保護（預覽）

```cpp
#include <iostream>
#include <thread>
#include <mutex>

struct Bank {
    int accountA = 1000;
    int accountB = 1000;
    std::mutex mtx;
};

Bank bank;

void transfer(int amount) {
    std::lock_guard<std::mutex> lock(bank.mtx);
    // 不變量暫時破壞，但沒人看得到
    bank.accountA -= amount;
    bank.accountB += amount;
    // 不變量恢復
}

void audit() {
    std::lock_guard<std::mutex> lock(bank.mtx);
    int total = bank.accountA + bank.accountB;
    std::cout << "總額: " << total << std::endl;  // 永遠是 2000
}
```

---

### 十、本課重點回顧

1. **不變量**是資料結構必須保持為真的條件
2. 複合操作會**暫時破壞**不變量
3. 單執行緒下暫時破壞沒問題，因為沒人看到
4. 多執行緒下，其他執行緒可能看到破壞的中間狀態
5. **競爭條件**的本質是看到了不一致的資料
6. 解決方案：讓整個操作對外呈現為原子

---

### 下一課預告

在 **課程 4.3：臨界區段概念** 中，我們將學習：
- 什麼是臨界區段（Critical Section）
- 如何識別需要保護的程式碼
- 臨界區段的設計原則

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <mutex>

struct Bank {
    int accountA = 1000;
    int accountB = 1000;
    std::mutex mtx;
};

Bank bank;

void transfer(int amount) {
    std::lock_guard<std::mutex> lock(bank.mtx);
    // 不變量暫時破壞，但沒人看得到
    bank.accountA -= amount;
    bank.accountB += amount;
    // 不變量恢復
}

void audit() {
    std::lock_guard<std::mutex> lock(bank.mtx);
    int total = bank.accountA + bank.accountB;
    std::cout << "總額: " << total << std::endl;  // 永遠是 2000
}

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 1000; ++i) transfer(1);
    });
    
    std::thread t2([]() {
        for (int i = 0; i < 1000; ++i) audit();
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
