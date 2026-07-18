/*
# 第四階段：共享資料與競爭條件

## 課程 4.4：資料競爭範例分析

---

### 引言

本課透過多個實際案例，分析資料競爭如何發生，以及如何識別程式碼中潛在的競爭條件。

---

### 一、案例一：Check-Then-Act 競爭

最常見的競爭模式：先檢查條件，再根據結果行動。

```cpp
#include <iostream>
#include <thread>
#include <map>

std::map<int, std::string> cache;

// 危險！Check-Then-Act 競爭
std::string getValue(int key) {
    if (cache.find(key) == cache.end()) {  // 檢查
        // ← 另一執行緒可能在此插入相同 key！
        cache[key] = "computed_" + std::to_string(key);  // 行動
    }
    return cache[key];
}

int main() {
    std::thread t1([]() { getValue(1); });
    std::thread t2([]() { getValue(1); });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

**問題**：兩個執行緒都可能認為 key 不存在，然後都嘗試插入。

---

### 二、案例二：Read-Modify-Write 競爭

讀取、修改、寫回的複合操作。

```cpp
#include <iostream>
#include <thread>

int counter = 0;

void increment() {
    // 這三步不是原子的！
    // 1. 讀取 counter
    // 2. +1
    // 3. 寫回
    for (int i = 0; i < 10000; ++i) {
        counter++;  // 非原子操作
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    
    t1.join();
    t2.join();
    
    // 預期 20000，實際可能更小
    std::cout << "counter = " << counter << std::endl;
    
    return 0;
}
```

---

### 三、案例三：複合操作競爭

看似獨立的操作，實際上有關聯。

```cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> vec;

void unsafeAppend(int value) {
    if (vec.size() < 10) {        // 檢查
        // ← 另一執行緒可能在此改變 size！
        vec.push_back(value);      // 可能超過限制
    }
}

void unsafeAccess() {
    if (!vec.empty()) {            // 檢查
        // ← 另一執行緒可能清空 vec！
        int last = vec.back();     // 可能崩潰
        std::cout << last << std::endl;
    }
}
```

---

### 四、案例四：迭代器失效

```cpp
#include <iostream>
#include <thread>
#include <vector>

std::vector<int> data = {1, 2, 3, 4, 5};

void reader() {
    for (auto it = data.begin(); it != data.end(); ++it) {
        // ← 另一執行緒可能修改 data，導致迭代器失效！
        std::cout << *it << " ";
    }
}

void writer() {
    data.push_back(6);  // 可能導致重新配置，所有迭代器失效
}

int main() {
    std::thread t1(reader);
    std::thread t2(writer);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

---

### 五、案例五：延遲初始化

```cpp
#include <iostream>
#include <thread>

class Singleton {
    static Singleton* instance;
    
public:
    // 危險！雙重檢查鎖定的錯誤實作
    static Singleton* getInstance() {
        if (instance == nullptr) {          // 第一次檢查
            // ← 多執行緒可能同時通過這裡！
            instance = new Singleton();     // 可能建立多個實例
        }
        return instance;
    }
};

Singleton* Singleton::instance = nullptr;
```

---

### 六、案例六：共享物件的部分更新

```cpp
#include <iostream>
#include <thread>
#include <string>

struct Person {
    std::string firstName;
    std::string lastName;
    int age;
};

Person person{"John", "Doe", 30};

void writer() {
    // 更新不是原子的
    person.firstName = "Jane";
    // ← 此刻資料不一致！
    person.lastName = "Smith";
    person.age = 25;
}

void reader() {
    // 可能讀到 "Jane Doe 30" 這種不一致狀態
    std::cout << person.firstName << " " 
              << person.lastName << " " 
              << person.age << std::endl;
}
```

---

### 七、競爭條件識別清單

```
┌─────────────────────────────────────────────────────────────┐
│                 競爭條件警示信號                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ⚠️ if (condition) { action }                              │
│     條件和行動之間狀態可能改變                               │
│                                                             │
│  ⚠️ variable++, variable += x                              │
│     Read-Modify-Write 不是原子的                            │
│                                                             │
│  ⚠️ 讀取多個相關變數                                        │
│     可能讀到不一致的組合                                     │
│                                                             │
│  ⚠️ 迭代容器時修改容器                                      │
│     迭代器可能失效                                          │
│                                                             │
│  ⚠️ 物件狀態的部分更新                                      │
│     可能讀到中間狀態                                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 八、如何發現競爭條件

#### 方法一：程式碼審查

問自己：
- 這個變數是否被多執行緒存取？
- 有沒有寫入操作？
- 操作是否原子？

#### 方法二：使用工具

```bash
# ThreadSanitizer（你已熟悉）
g++ -fsanitize=thread -g -o program program.cpp
./program
```

#### 方法三：壓力測試

```cpp
// 大量重複執行，增加發現問題的機會
for (int i = 0; i < 10000; ++i) {
    runTest();
}
```

---

### 九、ThreadSanitizer 輸出範例

```cpp
// 編譯：g++ -fsanitize=thread -g -o race race.cpp -pthread

#include <thread>

int counter = 0;

int main() {
    std::thread t1([]() { counter++; });
    std::thread t2([]() { counter++; });
    
    t1.join();
    t2.join();
    
    return 0;
}
```

TSan 輸出：
```
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x000000601040 by thread T2:
    #0 main::$_1::operator()() race.cpp:8
    
  Previous write of size 4 at 0x000000601040 by thread T1:
    #0 main::$_0::operator()() race.cpp:7
    
  Location is global 'counter' of size 4 at 0x000000601040
```

---

### 十、競爭條件的特性

```
┌─────────────────────────────────────────────────────────────┐
│               競爭條件為何難以除錯                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 非確定性                                                 │
│     同樣的程式碼，每次執行結果可能不同                       │
│                                                             │
│  2. 時機敏感                                                 │
│     只在特定時序下發生                                       │
│                                                             │
│  3. 難以重現                                                 │
│     加上 printf 除錯可能改變時序，問題消失                   │
│                                                             │
│  4. 環境依賴                                                 │
│     在某台機器正常，換台機器就出錯                           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十一、本課重點回顧

1. **Check-Then-Act**：檢查和行動之間狀態可能改變
2. **Read-Modify-Write**：複合操作不是原子的
3. **複合操作**：多個相關操作必須一起保護
4. **迭代器失效**：修改容器時迭代器可能無效
5. **部分更新**：物件可能處於不一致狀態
6. **TSan** 是發現競爭條件的有力工具

---

### 下一課預告

在 **課程 4.5：競爭條件的檢測** 中，我們將深入學習：
- ThreadSanitizer 的詳細使用
- 靜態分析工具
- 除錯策略與技巧

---

準備好繼續嗎？
*/



#include <iostream>
#include <thread>
#include <string>

struct Person {
    std::string firstName;
    std::string lastName;
    int age;
};

Person person{"John", "Doe", 30};

void writer() {
    // 更新不是原子的
    person.firstName = "Jane";
    // ← 此刻資料不一致！
    person.lastName = "Smith";
    person.age = 25;
}

void reader() {
    // 可能讀到 "Jane Doe 30" 這種不一致狀態
    std::cout << person.firstName << " " 
              << person.lastName << " " 
              << person.age << std::endl;
}
int main() {
    std::thread t1(writer);
    std::thread t2(reader);
    
    t1.join();
    t2.join();
    
    return 0;
}