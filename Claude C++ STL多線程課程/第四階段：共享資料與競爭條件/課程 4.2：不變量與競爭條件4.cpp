// =============================================================================
//  課程 4.2：不變量與競爭條件4.cpp  —  用 mutex 把「破壞期」變回不可觀察
// =============================================================================
//
// 【本檔性質】本檔是 4.2 系列的【正確版收尾】。
//   前三檔示範了「不變量被破壞 → 被觀察 → 災難」，
//   本檔示範同一個銀行例子加上 std::mutex 之後，audit() 為何永遠印出 2000。
//   檔案上半部是課程講義（大段註解），下半部是可執行的正確範例。
//
// 【主題資訊 Information】
//   主題：    臨界區段如何「重建」單執行緒才有的免費保護
//   語法：    std::lock_guard<std::mutex> lock(bank.mtx);   // RAII，離開作用域自動解鎖
//   標準版本：std::mutex / std::lock_guard 為 C++11；std::scoped_lock 為 C++17
//   標頭檔：  <mutex>、<thread>、<iostream>
//   複雜度：  無競爭時上鎖約數十奈秒（本機實測見課程 5.6）；有競爭時可能進入核心等待
//
// 【詳細解釋 Explanation】
//
// 【1. 鎖真正買到的是什麼】
//   很多人以為 mutex 的功能是「保護變數」。更精準的說法是：
//   【mutex 保護的是「不變量的破壞期」，不是變數本身】。
//   本檔的 transfer() 依然會讓總額暫時不等於 2000 ——
//       bank.accountA -= amount;   // 破壞期開始，總額 = 1999
//       bank.accountB += amount;   // 破壞期結束，總額 = 2000
//   差別在於：audit() 想觀察時必須先取得同一把鎖，
//   而鎖此刻被 transfer() 持有，於是 audit() 被擋在門外，
//   等它進得去時破壞期早已結束。
//   → 破壞照樣發生，只是重新變回「不可觀察」——
//     這正是單執行緒天生免費擁有、而多執行緒必須花錢買回來的性質。
//
// 【2. 為什麼「兩邊都要鎖」不是可選項】
//   互斥不是某一方的屬性，而是所有參與者共同遵守的協定。
//   若只有 transfer() 加鎖、audit() 不加，audit() 完全不會被擋住，
//   照樣讀到破壞期，而且兩者之間仍構成 data race → UB。
//   一把鎖只有在「所有存取該資料的程式碼都經過它」時才有意義。
//   實務上這條規則叫做 lock discipline，是 code review 的重點檢查項。
//
// 【3. 為什麼用 lock_guard 而不是手動 lock()/unlock()】
//   手動配對在下列情形一定會出事：
//     * 中途 return（多個離開點，很容易漏掉一個）
//     * 拋出例外（堆疊展開時直接跳過 unlock）
//     * 後人維護時在中間插入 return
//   lock_guard 是 RAII：解構函式在【任何】離開作用域的路徑上都會執行，
//   包含例外展開。這是 C++ 相對於「手動 lock/unlock」語言的重要優勢。
//   （課程 5.4 的「互斥鎖的常見錯誤9.cpp」示範了例外跳過 unlock 的後果。）
//
// 【4. 鎖的粒度與這個例子的關係】
//   本檔用「一把鎖保護整個 Bank」——粗粒度鎖。
//   對兩個帳戶而言完全合理：不變量橫跨兩個欄位，
//   臨界區段就必須同時涵蓋兩個欄位，拆成兩把鎖反而製造死結風險。
//   但如果是一萬個互不相關的帳戶，一把大鎖就會成為瓶頸，
//   此時要改用分片鎖（每個帳戶或每個桶一把鎖），
//   並用 std::scoped_lock 一次取得多把鎖以避免死結。
//   （粒度取捨的量化比較見課程 5.6：互斥鎖的效能考量3.cpp。）
//
// 【5. 「永遠是 2000」這個註解為什麼這次可以寫】
//   前三檔一律禁止寫「一定會…」，因為那些是 UB。
//   本檔不同：所有存取都在同一把鎖的保護下，
//   程式沒有 data race，行為完全由標準定義。
//   audit() 觀察到的必定是某次 transfer 完成後的一致狀態，
//   所以「總額必定為 2000」是【可以斷言】的。
//   → 判斷一句話能不能寫死，取決於程式有沒有 UB，而不是取決於跑了幾次。
//
// 【概念補充 Concept Deep Dive】
//
// (A) mutex 在本機的實作與成本
//   Linux 上 libstdc++ 的 std::mutex 底層是 pthread_mutex_t，
//   而 glibc 的 pthread mutex 使用 futex（fast userspace mutex）：
//     * 無競爭時：只做一次原子的 compare-and-swap，完全不進核心，
//       本機實測每次 lock+unlock 約 13～25 ns（見課程 5.6 效能考量1.cpp 的實測範圍）。
//     * 有競爭時：才呼叫 futex(FUTEX_WAIT) 進入核心把執行緒掛起，
//       這一趟系統呼叫加上下文切換是數微秒等級，比無競爭貴兩三個數量級。
//   → 這解釋了為什麼「臨界區段要短」：短的臨界區段讓競爭機率下降，
//     大部分 lock 就能走 fast path 不進核心。
//
// (B) 為什麼 audit() 的 lock_guard 也要保護「讀取 + 相加」整段
//   若寫成「鎖起來讀 A、解鎖、鎖起來讀 B、解鎖」，
//   兩次讀取之間 transfer 可以整個跑完，就退化成 4.2-3 的 atomic 對照組：
//   每個值都合法、組合起來卻不合法。
//   一次觀察 = 一個臨界區段，這是保護跨變數不變量的基本要求。
//
// (C) lock_guard / unique_lock / scoped_lock 的選擇
//   * std::lock_guard  (C++11)：最輕，建構即鎖、解構即解，不能中途解鎖。預設選它。
//   * std::unique_lock (C++11)：可延遲上鎖、可中途 unlock、可移動，
//                               condition_variable::wait 必須用它。有少量額外狀態。
//   * std::scoped_lock (C++17)：一次鎖多把，內部用死結避免演算法（類似 std::lock），
//                               鎖兩把以上時的正確選擇。
//
// 【注意事項 Pay Attention】
// 1. 所有存取同一份資料的程式碼都必須經過同一把鎖，讀取端也不例外。
// 2. 一次觀察要在一個臨界區段內完成；拆成多次小鎖等於沒保護跨變數不變量。
// 3. 鎖保護的是「破壞期」，不是變數；不變量橫跨幾個變數就要涵蓋幾個變數。
// 4. 一律用 RAII（lock_guard / scoped_lock），不要手動 lock()/unlock()。
// 5. 臨界區段內不要做 I/O、不要睡眠、不要呼叫可能再上鎖的外部回呼，
//    否則競爭機率與死結風險都會急遽上升。
// 6. 本檔會印出 1000 行「總額: 2000」，這是【確定性】輸出：
//    因為沒有 UB，每次執行都完全相同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 mutex 保護不變量
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 加了 mutex 之後，transfer() 執行到一半時總額還是 1999 嗎?
//     答：還是 1999。鎖沒有消除「破壞」，它消除的是「觀察」。
//         任何多步驟修改都必然產生中間狀態，這是物理事實；
//         mutex 讓想觀察的人必須先拿到同一把鎖，
//         而鎖被持有時它進不來，等它進得來時破壞期已經結束。
//     追問：那如果 audit() 忘記加鎖會怎樣?
//         → 保護完全失效。audit 不會被擋住，照樣讀到破壞期，
//           而且與 transfer 之間仍是 data race → UB。
//           互斥是所有參與者共同遵守的協定，不是單方面的屬性。
//
// 🔥 Q2. 為什麼一律建議用 lock_guard 而不是手動 lock() / unlock()?
//     答：手動配對在「中途 return」和「拋出例外」時一定會漏掉 unlock，
//         鎖從此永遠不會釋放，後續所有取用者全部卡死。
//         lock_guard 是 RAII，解構函式在任何離開作用域的路徑上都會執行，
//         包含例外展開，所以不可能漏解鎖。
//     追問：什麼時候不能用 lock_guard?
//         → 需要中途 unlock、延遲上鎖、或要傳給 condition_variable::wait 時，
//           要改用 unique_lock；一次要鎖兩把以上時用 scoped_lock（C++17）。
//
// ⚠️ 陷阱. 「audit() 只是讀資料，讀取不會改壞東西，應該不用加鎖吧?」——錯在哪?
//     答：data race 的成立條件是「至少一方寫入」，不是「雙方都寫入」。
//         一寫一讀而無同步就已經是 UB。而且就算不談 UB，
//         不加鎖的讀取本來就會讀到破壞期，拿到從未合法存在過的組合。
//     為什麼會錯：把「唯讀」直覺理解成「不會造成傷害」。
//         但並行的問題不在於誰弄壞了資料，而在於誰【看到】了不該看到的狀態；
//         監控、健康檢查、debug dump 這些「只是讀一下」的程式碼，
//         正是實務上最常忘記加鎖、也最常產生假警報的地方。
// ═══════════════════════════════════════════════════════════════════════════

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
#include <vector>
#include <atomic>

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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個堆疊，支援 push / pop / top / getMin，且四者都要 O(1)。
//   為什麼用到本主題：標準解法是「主堆疊 + 最小值堆疊」兩個容器並行維護，
//         不變量是【兩個堆疊必須同步：minStack.top() 永遠是 mainStack 的最小值】。
//         push 要推兩次、pop 要彈兩次 —— 這正是本檔的
//         「橫跨多個變數的不變量 + 多步驟修改」結構。
//         LeetCode 是單執行緒判題，兩次 push 之間沒人看得到；
//         但只要把同一個 MinStack 給兩條執行緒用，
//         就會出現「主堆疊已推、最小堆疊還沒推」的破壞期，
//         getMin() 立刻回傳錯誤答案。本範例用 mutex 讓每個操作成為原子步驟。
//   注意：加了鎖之後每個操作仍是 O(1)（鎖不改變複雜度，只增加常數）。
// -----------------------------------------------------------------------------
class MinStack {
private:
    // 【注意】成員初始化順序依「宣告順序」，與初始化列表順序無關。
    mutable std::mutex mtx;
    std::vector<int> mainStack;
    std::vector<int> minStack;   // 與 mainStack 等高，記錄「到此為止的最小值」

public:
    MinStack() = default;

    void push(int val) {
        std::lock_guard<std::mutex> lock(mtx);
        mainStack.push_back(val);                      // ← 破壞期開始（兩堆疊不等高）
        if (minStack.empty() || val <= minStack.back())
            minStack.push_back(val);
        else
            minStack.push_back(minStack.back());       // ← 破壞期結束
    }

    void pop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (mainStack.empty()) return;
        mainStack.pop_back();
        minStack.pop_back();                           // 兩者必須一起彈
    }

    int top() const {
        std::lock_guard<std::mutex> lock(mtx);
        return mainStack.back();
    }

    int getMin() const {
        std::lock_guard<std::mutex> lock(mtx);
        return minStack.back();
    }

    // 可執行的不變量檢查：兩個堆疊必須等高
    bool invariantHolds() const {
        std::lock_guard<std::mutex> lock(mtx);
        return mainStack.size() == minStack.size();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】連線池：閒置數 + 使用中數必須恆等於總數
//   情境：資料庫連線池對外提供 acquire() / release()，
//         同時要提供 stats() 給監控系統。
//         不變量是【idle + inUse == capacity】，橫跨兩個計數器。
//         若 acquire() 先減 idle、再加 inUse，中間被監控端取樣，
//         就會看到連線「憑空消失」，觸發容量不足的假警報。
//   修法：acquire / release / stats 全部經過同一把鎖。
// -----------------------------------------------------------------------------
struct PoolStats {
    int idle;
    int inUse;
};

class ConnectionPool {
private:
    mutable std::mutex mtx;
    int capacity;
    int idle;
    int inUse = 0;

public:
    explicit ConnectionPool(int cap) : capacity(cap), idle(cap) {}

    bool acquire() {
        std::lock_guard<std::mutex> lock(mtx);
        if (idle == 0) return false;    // check 與 act 同在鎖內
        --idle;                         // ← 破壞期開始
        ++inUse;                        // ← 破壞期結束
        return true;
    }

    void release() {
        std::lock_guard<std::mutex> lock(mtx);
        if (inUse == 0) return;
        --inUse;
        ++idle;
    }

    PoolStats stats() const {
        std::lock_guard<std::mutex> lock(mtx);   // 一次觀察 = 一個臨界區段
        return PoolStats{idle, inUse};
    }

    int cap() const { return capacity; }
};

int main() {
    std::cout << "=== 正確版：mutex 保護的轉帳 + 稽核 ===" << std::endl;
    std::cout << "（transfer 執行到一半時總額確實是 1999，"
                 "但 audit 進不來，所以永遠觀察不到）" << std::endl;

    // ── 第一階段：少量稽核，把觀察結果印出來看 ──
    //   講義版本是跑 1000 次 audit()，會印出 1000 行一模一樣的「總額: 2000」；
    //   這裡只印 8 行做示意，真正的大量驗證交給下面的第二階段（不印、只統計）。
    {
        std::thread t1([]() {
            for (int i = 0; i < 1000; ++i) transfer(1);
        });

        std::thread t2([]() {
            for (int i = 0; i < 8; ++i) audit();
        });

        t1.join();
        t2.join();
    }
    std::cout << "→ 以上 8 行必定全部是 2000（沒有 UB，可以斷言）" << std::endl;

    // ── 第二階段：大量稽核但不印出，只統計有沒有觀察到破壞期 ──
    //   200000 次觀察遠比 1000 次更有說服力，而且不會洗版。
    {
        std::atomic<int> violations{0};
        std::thread w([]() {
            for (int i = 0; i < 200000; ++i) { transfer(1); transfer(-1); }
        });
        std::thread r([&violations]() {
            for (int i = 0; i < 200000; ++i) {
                std::lock_guard<std::mutex> lock(bank.mtx);   // 讀取端也要同一把鎖
                if (bank.accountA + bank.accountB != 2000) violations.fetch_add(1);
            }
        });
        w.join();
        r.join();
        std::cout << "加碼稽核 200000 次（不印出），觀察到總額 != 2000 的次數: "
                  << violations.load() << " (必定為 0)" << std::endl;
    }

    std::cout << "\n=== LeetCode 155. Min Stack ===" << std::endl;
    {
        MinStack st;
        st.push(-2);
        st.push(0);
        st.push(-3);
        std::cout << "getMin() = " << st.getMin() << "  (預期 -3)" << std::endl;
        st.pop();
        std::cout << "top()    = " << st.top()    << "  (預期 0)"  << std::endl;
        std::cout << "getMin() = " << st.getMin() << "  (預期 -2)" << std::endl;
        std::cout << "不變量（兩堆疊等高）: " << std::boolalpha
                  << st.invariantHolds() << std::endl;

        // 多執行緒壓力測試：4 條執行緒同時 push/pop，
        // 因為每個操作都是原子的，兩個堆疊永遠等高。
        MinStack shared;
        std::vector<std::thread> ths;
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&shared, i] {
                for (int k = 0; k < 20000; ++k) {
                    shared.push(i * 100 + k);
                    shared.pop();
                }
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "4 執行緒各 20000 次 push+pop 後，不變量仍成立: "
                  << shared.invariantHolds() << std::endl;
    }

    std::cout << "\n=== 日常實務：連線池的 idle + inUse 恆等於容量 ===" << std::endl;
    {
        ConnectionPool pool(50);
        std::atomic<int> badSamples{0};
        std::vector<std::thread> ths;

        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&] {
                for (int k = 0; k < 20000; ++k) {
                    if (pool.acquire()) pool.release();
                }
            });
        }
        std::thread monitor([&] {
            for (int k = 0; k < 20000; ++k) {
                PoolStats s = pool.stats();
                if (s.idle + s.inUse != pool.cap()) badSamples.fetch_add(1);
            }
        });

        for (auto& t : ths) t.join();
        monitor.join();

        PoolStats fin = pool.stats();
        std::cout << "容量 " << pool.cap()
                  << "，最終 idle=" << fin.idle << ", inUse=" << fin.inUse << std::endl;
        std::cout << "監控取樣 20000 次，看到 idle+inUse != 容量的次數: "
                  << badSamples.load() << " (必定為 0)" << std::endl;
    }

    return 0;
}


// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.2：不變量與競爭條件4.cpp' -o invariant4
//
// 驗證無資料競爭（本檔應【完全乾淨】，與前三檔形成對照）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.2：不變量與競爭條件4.cpp' -o invariant4_tsan
//   ./invariant4_tsan  → 不會有任何 data race 報告

// 註：本檔【沒有】資料競爭，所有存取都在同一把鎖之下，
// 因此輸出是確定性的 —— 每次執行完全相同（本機連續四次實測 md5 一致）。
// 這與 4.2-1/2/3 三檔的 UB 示範形成刻意對照：
// 能不能把結果寫死，取決於程式有沒有 UB，而不是取決於跑了幾次。

// === 預期輸出 ===
// === 正確版：mutex 保護的轉帳 + 稽核 ===
// （transfer 執行到一半時總額確實是 1999，但 audit 進不來，所以永遠觀察不到）
// 總額: 2000
// 總額: 2000
// 總額: 2000
// 總額: 2000
// 總額: 2000
// 總額: 2000
// 總額: 2000
// 總額: 2000
// → 以上 8 行必定全部是 2000（沒有 UB，可以斷言）
// 加碼稽核 200000 次（不印出），觀察到總額 != 2000 的次數: 0 (必定為 0)
//
// === LeetCode 155. Min Stack ===
// getMin() = -3  (預期 -3)
// top()    = 0  (預期 0)
// getMin() = -2  (預期 -2)
// 不變量（兩堆疊等高）: true
// 4 執行緒各 20000 次 push+pop 後，不變量仍成立: true
//
// === 日常實務：連線池的 idle + inUse 恆等於容量 ===
// 容量 50，最終 idle=50, inUse=0
// 監控取樣 20000 次，看到 idle+inUse != 容量的次數: 0 (必定為 0)
