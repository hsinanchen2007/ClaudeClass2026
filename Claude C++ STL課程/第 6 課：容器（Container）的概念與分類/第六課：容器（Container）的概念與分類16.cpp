/*
# 第六課：容器（Container）的概念與分類

---

## 6.1 什麼是容器？

**容器**（Container）是用來**儲存和管理一組物件**的類別模板。

你可以把容器想像成「資料的收納盒」：

```
┌─────────────────────────────────────────────────────────────────┐
│                      容器的基本概念                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   容器負責：                                                    │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ • 儲存元素                                              │  │
│   │ • 管理記憶體（自動配置和釋放）                          │  │
│   │ • 提供存取元素的方法                                    │  │
│   │ • 提供迭代器讓演算法可以操作                            │  │
│   │ • 追蹤元素數量                                          │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│   你只需要：                                                    │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ • 決定要存什麼型別                                      │  │
│   │ • 選擇適合的容器                                        │  │
│   │ • 使用容器提供的介面操作                                │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.2 容器的分類總覽

STL 容器分為四大類：

```
┌─────────────────────────────────────────────────────────────────┐
│                       STL 容器分類                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  序列容器（Sequence Containers）                        │   │
│  │  • 元素按插入順序排列                                   │   │
│  │  • array, vector, deque, list, forward_list             │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  關聯容器（Associative Containers）                     │   │
│  │  • 元素按鍵值自動排序                                   │   │
│  │  • set, multiset, map, multimap                         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  無序容器（Unordered Containers）                       │   │
│  │  • 使用雜湊表，無特定順序                               │   │
│  │  • unordered_set, unordered_multiset,                   │   │
│  │    unordered_map, unordered_multimap                    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  容器配接器（Container Adapters）                       │   │
│  │  • 包裝其他容器，提供特定介面                           │   │
│  │  • stack, queue, priority_queue                         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.3 序列容器（Sequence Containers）

序列容器的特點：**元素按照插入的順序排列**。

### 6.3.1 array — 固定大小陣列

```cpp
#include <iostream>
#include <array>

int main() {
    // array：大小在編譯期固定
    std::array<int, 5> arr = {10, 20, 30, 40, 50};
    
    std::cout << "=== std::array ===" << std::endl;
    std::cout << "大小: " << arr.size() << std::endl;
    std::cout << "第一個: " << arr.front() << std::endl;
    std::cout << "最後一個: " << arr.back() << std::endl;
    std::cout << "arr[2]: " << arr[2] << std::endl;
    std::cout << "arr.at(2): " << arr.at(2) << std::endl;  // 有邊界檢查
    
    // 遍歷
    std::cout << "所有元素: ";
    for (int n : arr) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::array ===
大小: 5
第一個: 10
最後一個: 50
arr[2]: 30
arr.at(2): 30
所有元素: 10 20 30 40 50 
```

**特性：**
- 大小固定，編譯期決定
- 連續記憶體
- 零額外開銷（跟 C 陣列一樣快）
- Random Access Iterator

### 6.3.2 vector — 動態陣列

```cpp
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== std::vector ===" << std::endl;
    
    std::vector<int> vec;
    
    // 動態新增
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    std::cout << "大小: " << vec.size() << std::endl;
    std::cout << "容量: " << vec.capacity() << std::endl;
    
    // 繼續新增
    vec.push_back(40);
    vec.push_back(50);
    
    std::cout << "新增後大小: " << vec.size() << std::endl;
    std::cout << "新增後容量: " << vec.capacity() << std::endl;
    
    // 存取
    std::cout << "vec[2]: " << vec[2] << std::endl;
    std::cout << "front: " << vec.front() << std::endl;
    std::cout << "back: " << vec.back() << std::endl;
    
    // 刪除最後一個
    vec.pop_back();
    std::cout << "pop_back 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::vector ===
大小: 3
容量: 4
新增後大小: 5
新增後容量: 8
vec[2]: 30
front: 10
back: 50
pop_back 後: 10 20 30 40 
```

**特性：**
- 大小可動態變化
- 連續記憶體
- 尾端操作 O(1)
- 中間插入/刪除 O(n)
- Random Access Iterator

### 6.3.3 deque — 雙端佇列

```cpp
#include <iostream>
#include <deque>

int main() {
    std::cout << "=== std::deque ===" << std::endl;
    
    std::deque<int> deq;
    
    // 可以在兩端操作
    deq.push_back(30);
    deq.push_front(20);
    deq.push_back(40);
    deq.push_front(10);
    
    std::cout << "元素: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;
    
    // 隨機存取
    std::cout << "deq[2]: " << deq[2] << std::endl;
    
    // 兩端刪除
    deq.pop_front();
    deq.pop_back();
    
    std::cout << "刪除頭尾後: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::deque ===
元素: 10 20 30 40 
deq[2]: 30
刪除頭尾後: 20 30 
```

**特性：**
- 兩端操作都是 O(1)
- 支援隨機存取
- 分段連續記憶體（不是完全連續）
- Random Access Iterator

### 6.3.4 list — 雙向鏈結串列

```cpp
#include <iostream>
#include <list>

int main() {
    std::cout << "=== std::list ===" << std::endl;
    
    std::list<int> lst = {20, 40};
    
    // 任意位置插入都是 O(1)（前提是已有迭代器）
    lst.push_front(10);
    lst.push_back(50);
    
    // 在中間插入
    auto it = lst.begin();
    ++it;  // 指向 20
    ++it;  // 指向 40
    lst.insert(it, 30);  // 在 40 前面插入 30
    
    std::cout << "元素: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list 特有的操作
    std::list<int> lst2 = {100, 200, 300};
    
    // splice：將 lst2 的元素移動到 lst
    it = lst.begin();
    std::advance(it, 2);  // 指向 30
    lst.splice(it, lst2);  // 在 30 前面插入 lst2 的所有元素
    
    std::cout << "splice 後 lst: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "splice 後 lst2 大小: " << lst2.size() << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::list ===
元素: 10 20 30 40 50 
splice 後 lst: 10 20 100 200 300 30 40 50 
splice 後 lst2 大小: 0
```

**特性：**
- 任意位置插入/刪除 O(1)
- 不支援隨機存取
- 每個元素有額外的指標開銷
- Bidirectional Iterator

### 6.3.5 forward_list — 單向鏈結串列

```cpp
#include <iostream>
#include <forward_list>

int main() {
    std::cout << "=== std::forward_list ===" << std::endl;
    
    std::forward_list<int> flst = {20, 30, 40};
    
    // 只能在前端插入
    flst.push_front(10);
    
    std::cout << "元素: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 在某個位置「之後」插入
    auto it = flst.begin();  // 指向 10
    flst.insert_after(it, 15);  // 在 10 之後插入 15
    
    std::cout << "insert_after 後: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 注意：forward_list 沒有 size() 成員函數！
    // 需要用 std::distance 計算
    auto count = std::distance(flst.begin(), flst.end());
    std::cout << "元素個數: " << count << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::forward_list ===
元素: 10 20 30 40 
insert_after 後: 10 15 20 30 40 
元素個數: 5
```

**特性：**
- 最小記憶體開銷的鏈結串列
- 只能往前遍歷
- 沒有 size()、push_back()、back()
- Forward Iterator

### 序列容器比較

```
┌────────────────────────────────────────────────────────────────────────────┐
│                          序列容器比較表                                     │
├──────────────┬─────────┬─────────┬─────────┬─────────┬─────────────────────┤
│    操作      │  array  │ vector  │  deque  │  list   │    forward_list     │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 隨機存取     │  O(1)   │  O(1)   │  O(1)   │  O(n)   │       O(n)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 頭部插入     │   N/A   │  O(n)   │  O(1)   │  O(1)   │       O(1)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 尾部插入     │   N/A   │ O(1)*   │  O(1)   │  O(1)   │       O(n)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 中間插入     │   N/A   │  O(n)   │  O(n)   │  O(1)   │       O(1)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 記憶體連續   │    ✓    │    ✓    │   部分  │   ✗     │        ✗            │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 迭代器類別   │ Random  │ Random  │ Random  │  Bidir  │     Forward         │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 額外開銷     │   無    │   低    │   中    │   高    │        中           │
└──────────────┴─────────┴─────────┴─────────┴─────────┴─────────────────────┘
  * vector 尾部插入 O(1) 是均攤時間複雜度，可能觸發重新配置
```

---

## 6.4 關聯容器（Associative Containers）

關聯容器的特點：**元素按鍵值自動排序**，內部通常用紅黑樹實作。

### 6.4.1 set — 有序集合

```cpp
#include <iostream>
#include <set>

int main() {
    std::cout << "=== std::set ===" << std::endl;
    
    // 元素自動排序，不允許重複
    std::set<int> s;
    
    s.insert(30);
    s.insert(10);
    s.insert(50);
    s.insert(20);
    s.insert(40);
    s.insert(30);  // 重複，會被忽略
    
    std::cout << "元素（自動排序）: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "大小: " << s.size() << std::endl;
    
    // 查找
    auto it = s.find(30);
    if (it != s.end()) {
        std::cout << "找到 30" << std::endl;
    }
    
    // count：因為不重複，只會回傳 0 或 1
    std::cout << "30 的數量: " << s.count(30) << std::endl;
    std::cout << "100 的數量: " << s.count(100) << std::endl;
    
    // 刪除
    s.erase(30);
    std::cout << "刪除 30 後: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::set ===
元素（自動排序）: 10 20 30 40 50 
大小: 5
找到 30
30 的數量: 1
100 的數量: 0
刪除 30 後: 10 20 40 50 
```

### 6.4.2 multiset — 允許重複的有序集合

```cpp
#include <iostream>
#include <set>

int main() {
    std::cout << "=== std::multiset ===" << std::endl;
    
    // 允許重複元素
    std::multiset<int> ms;
    
    ms.insert(30);
    ms.insert(10);
    ms.insert(30);  // 重複，會被保留
    ms.insert(20);
    ms.insert(30);  // 第三個 30
    
    std::cout << "元素: ";
    for (int n : ms) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "30 的數量: " << ms.count(30) << std::endl;
    
    // equal_range：取得所有等於某值的元素範圍
    auto range = ms.equal_range(30);
    std::cout << "所有 30：";
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::multiset ===
元素: 10 20 30 30 30 
30 的數量: 3
所有 30：30 30 30 
```

### 6.4.3 map — 有序鍵值對

```cpp
#include <iostream>
#include <map>
#include <string>

int main() {
    std::cout << "=== std::map ===" << std::endl;
    
    std::map<std::string, int> ages;
    
    // 插入方式一：operator[]
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    
    // 插入方式二：insert
    ages.insert({"Charlie", 35});
    ages.insert(std::make_pair("Diana", 28));
    
    // 按鍵排序
    std::cout << "所有人（按名字排序）:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 查找
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }
    
    // operator[] 的陷阱：會插入預設值！
    std::cout << "Eve 的年齡: " << ages["Eve"] << std::endl;  // 插入 Eve = 0
    std::cout << "現在的大小: " << ages.size() << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::map ===
所有人（按名字排序）:
  Alice: 25
  Bob: 30
  Charlie: 35
  Diana: 28
Bob 的年齡: 30
Eve 的年齡: 0
現在的大小: 5
```

### 6.4.4 multimap — 允許重複鍵的有序鍵值對

```cpp
#include <iostream>
#include <map>
#include <string>

int main() {
    std::cout << "=== std::multimap ===" << std::endl;
    
    // 一個鍵可以對應多個值
    std::multimap<std::string, std::string> phonebook;
    
    phonebook.insert({"Alice", "0912-345-678"});
    phonebook.insert({"Alice", "02-1234-5678"});  // Alice 有兩個號碼
    phonebook.insert({"Bob", "0923-456-789"});
    phonebook.insert({"Alice", "03-9876-5432"});  // Alice 第三個號碼
    
    std::cout << "電話簿:" << std::endl;
    for (const auto& entry : phonebook) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }
    
    // 查找 Alice 的所有號碼
    std::cout << "\nAlice 的所有號碼:" << std::endl;
    auto range = phonebook.equal_range("Alice");
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << "  " << it->second << std::endl;
    }
    
    std::cout << "\nAlice 有 " << phonebook.count("Alice") << " 個號碼" << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::multimap ===
電話簿:
  Alice: 0912-345-678
  Alice: 02-1234-5678
  Alice: 03-9876-5432
  Bob: 0923-456-789

Alice 的所有號碼:
  0912-345-678
  02-1234-5678
  03-9876-5432

Alice 有 3 個號碼
```

### 關聯容器比較

```
┌────────────────────────────────────────────────────────────────────────────┐
│                          關聯容器比較表                                     │
├──────────────┬────────────────┬────────────────┬───────────────────────────┤
│    容器      │    允許重複    │   儲存內容     │        典型用途           │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│    set       │      否        │     鍵         │ 去重、有序集合            │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│  multiset    │      是        │     鍵         │ 排序並保留重複            │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│    map       │    鍵不重複    │   鍵值對       │ 字典、索引                │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│  multimap    │    鍵可重複    │   鍵值對       │ 一對多關係                │
├──────────────┴────────────────┴────────────────┴───────────────────────────┤
│                                                                            │
│  共同特性：                                                                 │
│  • 內部使用紅黑樹                                                          │
│  • 元素按鍵自動排序                                                        │
│  • 查找、插入、刪除都是 O(log n)                                           │
│  • Bidirectional Iterator                                                  │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## 6.5 無序容器（Unordered Containers）

無序容器的特點：**使用雜湊表**，無特定順序，但查找速度更快。

### 6.5.1 unordered_set

```cpp
#include <iostream>
#include <unordered_set>

int main() {
    std::cout << "=== std::unordered_set ===" << std::endl;
    
    std::unordered_set<int> us;
    
    us.insert(30);
    us.insert(10);
    us.insert(50);
    us.insert(20);
    us.insert(40);
    
    // 順序不固定（取決於雜湊）
    std::cout << "元素（順序不固定）: ";
    for (int n : us) std::cout << n << " ";
    std::cout << std::endl;
    
    // 查找：平均 O(1)
    if (us.find(30) != us.end()) {
        std::cout << "找到 30" << std::endl;
    }
    
    // 雜湊表資訊
    std::cout << "桶的數量: " << us.bucket_count() << std::endl;
    std::cout << "負載因子: " << us.load_factor() << std::endl;
    
    return 0;
}
```

**可能的輸出：**
```
=== std::unordered_set ===
元素（順序不固定）: 40 20 50 10 30 
找到 30
桶的數量: 7
負載因子: 0.714286
```

### 6.5.2 unordered_map

```cpp
#include <iostream>
#include <unordered_map>
#include <string>

int main() {
    std::cout << "=== std::unordered_map ===" << std::endl;
    
    std::unordered_map<std::string, int> ages;
    
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    ages["Diana"] = 28;
    
    // 順序不固定
    std::cout << "所有人:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 查找：平均 O(1)
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }
    
    return 0;
}
```

**可能的輸出：**
```
=== std::unordered_map ===
所有人:
  Diana: 28
  Charlie: 35
  Bob: 30
  Alice: 25
Bob 的年齡: 30
```

### 有序 vs 無序容器

```
┌────────────────────────────────────────────────────────────────────────────┐
│                      有序 vs 無序容器比較                                   │
├──────────────────────┬─────────────────────┬───────────────────────────────┤
│        面向          │     有序（set/map）  │    無序（unordered_*）        │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 內部結構             │      紅黑樹         │         雜湊表                │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 查找時間             │     O(log n)        │      O(1) 平均               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 插入時間             │     O(log n)        │      O(1) 平均               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 刪除時間             │     O(log n)        │      O(1) 平均               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 元素順序             │      有序           │         無序                 │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 範圍查詢             │       支援          │        不支援                │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 迭代器類別           │   Bidirectional     │        Forward               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 最差情況             │     O(log n)        │      O(n)*                   │
└──────────────────────┴─────────────────────┴───────────────────────────────┘
  * 雜湊碰撞嚴重時可能退化
```

---

## 6.6 容器配接器（Container Adapters）

容器配接器**不是獨立的容器**，而是包裝其他容器來提供特定介面。

### 6.6.1 stack — 後進先出（LIFO）

```cpp
#include <iostream>
#include <stack>

int main() {
    std::cout << "=== std::stack ===" << std::endl;
    
    std::stack<int> stk;
    
    // 只能從頂端操作
    stk.push(10);
    stk.push(20);
    stk.push(30);
    
    std::cout << "頂端: " << stk.top() << std::endl;
    std::cout << "大小: " << stk.size() << std::endl;
    
    // 依序取出（後進先出）
    std::cout << "依序取出: ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::stack ===
頂端: 30
大小: 3
依序取出: 30 20 10 
```

### 6.6.2 queue — 先進先出（FIFO）

```cpp
#include <iostream>
#include <queue>

int main() {
    std::cout << "=== std::queue ===" << std::endl;
    
    std::queue<int> que;
    
    // 從尾端加入
    que.push(10);
    que.push(20);
    que.push(30);
    
    std::cout << "前端: " << que.front() << std::endl;
    std::cout << "後端: " << que.back() << std::endl;
    
    // 依序取出（先進先出）
    std::cout << "依序取出: ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();  // 從前端移除
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::queue ===
前端: 10
後端: 30
依序取出: 10 20 30 
```

### 6.6.3 priority_queue — 優先佇列

```cpp
#include <iostream>
#include <queue>
#include <vector>

int main() {
    std::cout << "=== std::priority_queue ===" << std::endl;
    
    // 預設是最大堆（最大的在頂端）
    std::priority_queue<int> max_pq;
    
    max_pq.push(30);
    max_pq.push(10);
    max_pq.push(50);
    max_pq.push(20);
    max_pq.push(40);
    
    std::cout << "最大堆依序取出: ";
    while (!max_pq.empty()) {
        std::cout << max_pq.top() << " ";
        max_pq.pop();
    }
    std::cout << std::endl;
    
    // 最小堆
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
    
    min_pq.push(30);
    min_pq.push(10);
    min_pq.push(50);
    min_pq.push(20);
    min_pq.push(40);
    
    std::cout << "最小堆依序取出: ";
    while (!min_pq.empty()) {
        std::cout << min_pq.top() << " ";
        min_pq.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::priority_queue ===
最大堆依序取出: 50 40 30 20 10 
最小堆依序取出: 10 20 30 40 50 
```

### 配接器與底層容器

```
┌─────────────────────────────────────────────────────────────────┐
│                   容器配接器的底層容器                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   配接器           預設底層容器      可用的底層容器             │
│   ─────────────────────────────────────────────────────────     │
│   stack            deque            vector, deque, list         │
│   queue            deque            deque, list                 │
│   priority_queue   vector           vector, deque               │
│                                                                 │
│   指定底層容器：                                                │
│   std::stack<int, std::vector<int>> stk;                        │
│   std::queue<int, std::list<int>> que;                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.7 容器的共同介面

所有容器都提供一些共同的成員函數：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <set>

template <typename Container>
void show_container_info(const std::string& name, const Container& c) {
    std::cout << name << ":" << std::endl;
    std::cout << "  size(): " << c.size() << std::endl;
    std::cout << "  empty(): " << (c.empty() ? "true" : "false") << std::endl;
    // max_size(): 容器理論上能容納的最大元素數
    std::cout << "  max_size(): " << c.max_size() << std::endl;
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::list<int> lst = {1, 2, 3};
    std::set<int> s = {1, 2};
    std::vector<int> empty_vec;
    
    show_container_info("vector", vec);
    show_container_info("list", lst);
    show_container_info("set", s);
    show_container_info("empty_vector", empty_vec);
    
    // 其他共同操作
    std::cout << "=== 其他共同操作 ===" << std::endl;
    
    // swap
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {10, 20};
    v1.swap(v2);
    
    std::cout << "swap 後 v1: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << std::endl;
    
    // clear
    v1.clear();
    std::cout << "clear 後 v1.size(): " << v1.size() << std::endl;
    
    return 0;
}
```

**輸出：**
```
vector:
  size(): 5
  empty(): false
  max_size(): 4611686018427387903

list:
  size(): 3
  empty(): false
  max_size(): 768614336404564650

set:
  size(): 2
  empty(): false
  max_size(): 461168601842738790

empty_vector:
  size(): 0
  empty(): true
  max_size(): 4611686018427387903

=== 其他共同操作 ===
swap 後 v1: 10 20 
clear 後 v1.size(): 0
```

### 共同成員函數一覽

```
┌─────────────────────────────────────────────────────────────────┐
│                    容器共同成員函數                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  大小相關：                                                     │
│  • size()       → 元素數量                                     │
│  • empty()      → 是否為空                                     │
│  • max_size()   → 最大可容納數量                               │
│                                                                 │
│  迭代器：                                                       │
│  • begin() / end()           → 正向迭代器                      │
│  • cbegin() / cend()         → const 迭代器                    │
│  • rbegin() / rend()         → 反向迭代器（部分容器）          │
│                                                                 │
│  修改：                                                         │
│  • clear()      → 清空所有元素                                 │
│  • swap()       → 交換兩個容器的內容                           │
│                                                                 │
│  比較（同型別容器）：                                           │
│  • ==, !=       → 相等比較                                     │
│  • <, <=, >, >= → 字典序比較                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.8 如何選擇容器？

```
┌─────────────────────────────────────────────────────────────────┐
│                      容器選擇決策流程                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 需要鍵值對嗎？                                              │
│     ├─ 是 → 需要排序嗎？                                       │
│     │       ├─ 是 → map / multimap                             │
│     │       └─ 否 → unordered_map / unordered_multimap         │
│     │                                                           │
│     └─ 否 → 繼續                                               │
│                                                                 │
│  2. 元素需要唯一嗎？                                            │
│     ├─ 是 → 需要排序嗎？                                       │
│     │       ├─ 是 → set                                        │
│     │       └─ 否 → unordered_set                              │
│     │                                                           │
│     └─ 否 → 繼續                                               │
│                                                                 │
│  3. 大小固定嗎？                                                │
│     ├─ 是 → array                                              │
│     └─ 否 → 繼續                                               │
│                                                                 │
│  4. 主要操作是什麼？                                            │
│     ├─ 隨機存取多 → vector                                     │
│     ├─ 頭尾操作多 → deque                                      │
│     ├─ 中間插刪多 → list                                       │
│     ├─ 只需前端操作 + 最小開銷 → forward_list                  │
│     └─ 後進先出 → stack                                        │
│     └─ 先進先出 → queue                                        │
│     └─ 優先順序 → priority_queue                               │
│                                                                 │
│  5. 不確定？先用 vector                                         │
│     → 它在大多數情況下效能最好                                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 實際案例

```cpp
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <string>

int main() {
    // 案例 1：儲存學生成績列表 → vector
    std::cout << "=== 案例 1：學生成績 ===" << std::endl;
    std::vector<int> scores = {85, 92, 78, 95, 88};
    std::cout << "第三個學生: " << scores[2] << std::endl;
    
    // 案例 2：去重的標籤 → set 或 unordered_set
    std::cout << "\n=== 案例 2：去重標籤 ===" << std::endl;
    std::set<std::string> tags;
    tags.insert("C++");
    tags.insert("Programming");
    tags.insert("C++");  // 重複，被忽略
    tags.insert("STL");
    for (const auto& tag : tags) std::cout << tag << " ";
    std::cout << std::endl;
    
    // 案例 3：字數統計 → map 或 unordered_map
    std::cout << "\n=== 案例 3：字數統計 ===" << std::endl;
    std::string text = "hello world hello c++ world world";
    std::unordered_map<std::string, int> word_count;
    
    // 簡化的分詞
    std::string word;
    for (char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                word_count[word]++;
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) word_count[word]++;
    
    for (const auto& pair : word_count) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    
    // 案例 4：任務排程 → priority_queue
    std::cout << "\n=== 案例 4：任務排程 ===" << std::endl;
    std::priority_queue<std::pair<int, std::string>> tasks;
    
    tasks.push({3, "一般任務"});
    tasks.push({5, "緊急任務"});
    tasks.push({1, "低優先任務"});
    tasks.push({4, "重要任務"});
    
    std::cout << "按優先順序處理:" << std::endl;
    while (!tasks.empty()) {
        auto task = tasks.top();
        std::cout << "  優先級 " << task.first << ": " << task.second << std::endl;
        tasks.pop();
    }
    
    return 0;
}
```

**輸出：**
```
=== 案例 1：學生成績 ===
第三個學生: 78

=== 案例 2：去重標籤 ===
C++ Programming STL 

=== 案例 3：字數統計 ===
c++: 1
world: 3
hello: 2

=== 案例 4：任務排程 ===
按優先順序處理:
  優先級 5: 緊急任務
  優先級 4: 重要任務
  優先級 3: 一般任務
  優先級 1: 低優先任務
```

---

## 6.9 本課重點整理

```
┌─────────────────────────────────────────────────────────────────┐
│                      第六課 重點整理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 容器分類                                                     │
│     • 序列容器：array, vector, deque, list, forward_list        │
│     • 關聯容器：set, multiset, map, multimap                    │
│     • 無序容器：unordered_set, unordered_map 等                 │
│     • 容器配接器：stack, queue, priority_queue                  │
│                                                                 │
│  2. 序列容器特點                                                 │
│     • 元素按插入順序排列                                        │
│     • vector 最常用，連續記憶體，尾端操作快                     │
│     • list 適合頻繁中間插刪                                     │
│                                                                 │
│  3. 關聯容器特點                                                 │
│     • 元素按鍵自動排序（紅黑樹）                                │
│     • 查找、插入、刪除都是 O(log n)                             │
│     • multi 版本允許重複鍵                                      │
│                                                                 │
│  4. 無序容器特點                                                 │
│     • 使用雜湊表，無特定順序                                    │
│     • 平均 O(1) 的查找、插入、刪除                              │
│     • 不支援範圍查詢                                            │
│                                                                 │
│  5. 容器配接器特點                                               │
│     • 包裝其他容器，提供特定介面                                │
│     • stack（LIFO）、queue（FIFO）、priority_queue              │
│                                                                 │
│  6. 選擇原則                                                     │
│     • 不確定時先用 vector                                       │
│     • 需要鍵值對用 map 系列                                     │
│     • 需要去重用 set 系列                                       │
│     • 重視查找速度用 unordered 系列                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.10 課後練習

1. **思考題**：為什麼 `vector` 通常比 `list` 效能更好，即使在需要中間插入的情況下？（提示：考慮快取效應）

2. **實作題**：設計一個簡單的「最近瀏覽記錄」系統，要求：
   - 保持最多 5 筆記錄
   - 新記錄加在最前面
   - 如果記錄已存在，移到最前面
   - 你會選擇哪種容器？為什麼？

---

準備好進入**第七課：演算法（Algorithm）與容器的分離設計**了嗎？下一課我們會深入探討 STL 演算法的設計哲學，以及它如何透過迭代器實現與容器的解耦。
*/



#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <string>

int main() {
    // 案例 1：儲存學生成績列表 → vector
    std::cout << "=== 案例 1：學生成績 ===" << std::endl;
    std::vector<int> scores = {85, 92, 78, 95, 88};
    std::cout << "第三個學生: " << scores[2] << std::endl;
    
    // 案例 2：去重的標籤 → set 或 unordered_set
    std::cout << "\n=== 案例 2：去重標籤 ===" << std::endl;
    std::set<std::string> tags;
    tags.insert("C++");
    tags.insert("Programming");
    tags.insert("C++");  // 重複，被忽略
    tags.insert("STL");
    for (const auto& tag : tags) std::cout << tag << " ";
    std::cout << std::endl;
    
    // 案例 3：字數統計 → map 或 unordered_map
    std::cout << "\n=== 案例 3：字數統計 ===" << std::endl;
    std::string text = "hello world hello c++ world world";
    std::unordered_map<std::string, int> word_count;
    
    // 簡化的分詞
    std::string word;
    for (char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                word_count[word]++;
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) word_count[word]++;
    
    for (const auto& pair : word_count) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    
    // 案例 4：任務排程 → priority_queue
    std::cout << "\n=== 案例 4：任務排程 ===" << std::endl;
    std::priority_queue<std::pair<int, std::string>> tasks;
    
    tasks.push({3, "一般任務"});
    tasks.push({5, "緊急任務"});
    tasks.push({1, "低優先任務"});
    tasks.push({4, "重要任務"});
    
    std::cout << "按優先順序處理:" << std::endl;
    while (!tasks.empty()) {
        auto task = tasks.top();
        std::cout << "  優先級 " << task.first << ": " << task.second << std::endl;
        tasks.pop();
    }
    
    return 0;
}
