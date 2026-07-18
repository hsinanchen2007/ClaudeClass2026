/*
# 第三課：STL 的六大組件概覽

---

## 3.1 STL 的整體架構

STL 由六大組件構成，它們彼此協作，形成一個強大而靈活的系統：

```
┌─────────────────────────────────────────────────────────────────┐
│                      STL 六大組件架構圖                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      ┌─────────────┐                            │
│                      │  配置器      │                            │
│                      │ (Allocator) │                            │
│                      └──────┬──────┘                            │
│                             │ 負責記憶體配置                     │
│                             ▼                                   │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│   │   演算法     │◄───│   迭代器     │───►│    容器     │        │
│   │ (Algorithm) │    │ (Iterator)  │    │ (Container) │        │
│   └──────┬──────┘    └─────────────┘    └──────┬──────┘        │
│          │                                      │               │
│          │ 使用                                 │ 可被包裝      │
│          ▼                                      ▼               │
│   ┌─────────────┐                       ┌─────────────┐        │
│   │  函數物件    │                       │   配接器    │        │
│   │ (Functor)   │                       │  (Adapter)  │        │
│   └─────────────┘                       └─────────────┘        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

讓我們逐一認識這六大組件。

---

## 3.2 組件一：容器（Containers）

**容器**是用來儲存和管理資料的類別模板。

### 容器的分類

```
┌─────────────────────────────────────────────────────────────────┐
│                        容器分類總覽                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  序列容器（Sequence Containers）                                 │
│  ├── array          固定大小陣列                                │
│  ├── vector         動態陣列（最常用）                          │
│  ├── deque          雙端佇列                                    │
│  ├── list           雙向鏈結串列                                │
│  └── forward_list   單向鏈結串列                                │
│                                                                 │
│  關聯容器（Associative Containers）                              │
│  ├── set            有序集合（不重複）                          │
│  ├── multiset       有序集合（可重複）                          │
│  ├── map            有序鍵值對（不重複鍵）                      │
│  └── multimap       有序鍵值對（可重複鍵）                      │
│                                                                 │
│  無序容器（Unordered Containers）                                │
│  ├── unordered_set       雜湊集合（不重複）                     │
│  ├── unordered_multiset  雜湊集合（可重複）                     │
│  ├── unordered_map       雜湊鍵值對（不重複鍵）                 │
│  └── unordered_multimap  雜湊鍵值對（可重複鍵）                 │
│                                                                 │
│  容器配接器（Container Adapters）                                │
│  ├── stack          後進先出（LIFO）                            │
│  ├── queue          先進先出（FIFO）                            │
│  └── priority_queue 優先佇列                                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 容器快速示範

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>

int main() {
    // 序列容器：vector
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 序列容器：list
    std::list<int> lst = {10, 20, 30};
    lst.push_front(5);  // list 可以高效地在前端插入
    std::cout << "list: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 關聯容器：set（自動排序、不重複）
    std::set<int> s = {30, 10, 20, 10, 30};  // 重複的會被忽略
    std::cout << "set: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    // 關聯容器：map（鍵值對）
    std::map<std::string, int> ages;
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    std::cout << "map: ";
    for (const auto& pair : ages) {
        std::cout << pair.first << "=" << pair.second << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
vector: 1 2 3 4 5 
list: 5 10 20 30 
set: 10 20 30 
map: Alice=25 Bob=30 Charlie=35 
```

---

## 3.3 組件二：迭代器（Iterators）

**迭代器**是連接容器與演算法的橋樑，它提供了一種統一的方式來遍歷容器中的元素。

### 迭代器的概念

你可以把迭代器想像成「泛化的指標」：

```
┌─────────────────────────────────────────────────────────────────┐
│                      迭代器概念圖解                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   容器內容：  [ 10 | 20 | 30 | 40 | 50 ]                        │
│                 ▲                    ▲                          │
│                 │                    │                          │
│              begin()              end()                         │
│              (指向第一個)        (指向最後一個的下一位)          │
│                                                                 │
│   遍歷過程：                                                    │
│                                                                 │
│   it = begin()  →  *it = 10                                    │
│   ++it          →  *it = 20                                    │
│   ++it          →  *it = 30                                    │
│   ++it          →  *it = 40                                    │
│   ++it          →  *it = 50                                    │
│   ++it          →  it == end()  (結束)                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 迭代器基本操作

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 方法一：使用迭代器遍歷
    std::cout << "使用迭代器: ";
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";  // *it 取得迭代器指向的值
    }
    std::cout << std::endl;
    
    // 方法二：使用 auto 簡化
    std::cout << "使用 auto: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 方法三：範圍 for（底層也是迭代器）
    std::cout << "範圍 for: ";
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
使用迭代器: 10 20 30 40 50 
使用 auto: 10 20 30 40 50 
範圍 for: 10 20 30 40 50 
```

### 迭代器的五種類別

```
┌─────────────────────────────────────────────────────────────────┐
│                     迭代器類別層次                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   功能最強                                                      │
│      ▲                                                          │
│      │   Random Access Iterator（隨機存取迭代器）               │
│      │   • 支援 +n, -n, [], <, >                                │
│      │   • 代表：vector, deque, array                           │
│      │                                                          │
│      │   Bidirectional Iterator（雙向迭代器）                   │
│      │   • 支援 ++, --                                          │
│      │   • 代表：list, set, map                                 │
│      │                                                          │
│      │   Forward Iterator（前向迭代器）                         │
│      │   • 只支援 ++                                            │
│      │   • 代表：forward_list, unordered_set                    │
│      │                                                          │
│      │   Input Iterator（輸入迭代器）                           │
│      │   • 只能讀取，只能前進一次                               │
│      │   • 代表：istream_iterator                               │
│      │                                                          │
│      │   Output Iterator（輸出迭代器）                          │
│      │   • 只能寫入，只能前進一次                               │
│      │   • 代表：ostream_iterator                               │
│      ▼                                                          │
│   功能最弱                                                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 不同迭代器的能力差異

```cpp
#include <iostream>
#include <vector>
#include <list>

int main() {
    // vector 有 Random Access Iterator
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto vit = vec.begin();
    
    std::cout << "vector 迭代器可以：" << std::endl;
    std::cout << "  vit[2] = " << vit[2] << std::endl;      // 隨機存取
    std::cout << "  *(vit + 3) = " << *(vit + 3) << std::endl;  // 算術運算
    
    // list 只有 Bidirectional Iterator
    std::list<int> lst = {10, 20, 30, 40, 50};
    auto lit = lst.begin();
    
    std::cout << "\nlist 迭代器可以：" << std::endl;
    ++lit;  // 前進
    std::cout << "  ++lit → *lit = " << *lit << std::endl;
    --lit;  // 後退
    std::cout << "  --lit → *lit = " << *lit << std::endl;
    
    // 但 list 迭代器不能：
    // lit[2];      // 編譯錯誤！
    // lit + 3;     // 編譯錯誤！
    
    return 0;
}
```

**輸出：**
```
vector 迭代器可以：
  vit[2] = 30
  *(vit + 3) = 40

list 迭代器可以：
  ++lit → *lit = 20
  --lit → *lit = 10
```

---

## 3.4 組件三：演算法（Algorithms）

**演算法**是一系列對容器元素進行操作的函數模板。它們透過迭代器來操作容器，與容器本身解耦。

### 演算法的分類

```
┌─────────────────────────────────────────────────────────────────┐
│                       演算法分類總覽                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  非修改序列操作                                                  │
│  ├── find, find_if        查找元素                             │
│  ├── count, count_if      計數                                 │
│  ├── for_each             對每個元素執行操作                    │
│  └── all_of, any_of       條件判斷                             │
│                                                                 │
│  修改序列操作                                                    │
│  ├── copy, copy_if        複製元素                             │
│  ├── transform            轉換元素                             │
│  ├── fill, fill_n         填充                                 │
│  ├── replace              替換                                 │
│  └── remove, remove_if    移除                                 │
│                                                                 │
│  排序相關操作                                                    │
│  ├── sort, stable_sort    排序                                 │
│  ├── partial_sort         部分排序                             │
│  ├── nth_element          找第 n 個元素                        │
│  └── binary_search        二分搜尋                             │
│                                                                 │
│  數值操作（<numeric>）                                          │
│  ├── accumulate           累加                                 │
│  ├── inner_product        內積                                 │
│  └── partial_sum          部分和                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 演算法示範

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 排序
    std::sort(vec.begin(), vec.end());
    std::cout << "排序後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 查找
    auto it = std::find(vec.begin(), vec.end(), 7);
    if (it != vec.end()) {
        std::cout << "找到 7，位置: " << (it - vec.begin()) << std::endl;
    }
    
    // 計數
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2, 5};
    int count = std::count(data.begin(), data.end(), 2);
    std::cout << "2 出現次數: " << count << std::endl;
    
    // 累加
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;
    
    // 最大最小
    auto minmax = std::minmax_element(vec.begin(), vec.end());
    std::cout << "最小值: " << *minmax.first << std::endl;
    std::cout << "最大值: " << *minmax.second << std::endl;
    
    return 0;
}
```

**輸出：**
```
排序後: 1 2 3 4 5 6 7 8 9 
找到 7，位置: 6
2 出現次數: 4
總和: 45
最小值: 1
最大值: 9
```

### 演算法與容器的獨立性

同一個演算法可以用在不同容器上：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <algorithm>

template <typename Container>
void print_container(const std::string& name, const Container& c) {
    std::cout << name << ": ";
    for (const auto& elem : c) std::cout << elem << " ";
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {3, 1, 4, 1, 5};
    std::list<int> lst = {9, 2, 6, 5, 3};
    std::deque<int> deq = {5, 8, 9, 7, 9};
    
    // 同一個 sort 演算法用在 vector 和 deque
    std::sort(vec.begin(), vec.end());
    std::sort(deq.begin(), deq.end());
    
    // list 有自己的 sort（因為 std::sort 需要 Random Access Iterator）
    lst.sort();
    
    print_container("vector", vec);
    print_container("list", lst);
    print_container("deque", deq);
    
    // 同一個 find 演算法用在所有容器
    std::cout << "\n尋找元素 5：" << std::endl;
    
    auto vit = std::find(vec.begin(), vec.end(), 5);
    std::cout << "  vector: " << (vit != vec.end() ? "找到" : "沒找到") << std::endl;
    
    auto lit = std::find(lst.begin(), lst.end(), 5);
    std::cout << "  list: " << (lit != lst.end() ? "找到" : "沒找到") << std::endl;
    
    auto dit = std::find(deq.begin(), deq.end(), 5);
    std::cout << "  deque: " << (dit != deq.end() ? "找到" : "沒找到") << std::endl;
    
    return 0;
}
```

**輸出：**
```
vector: 1 1 3 4 5 
list: 2 3 5 6 9 
deque: 5 7 8 9 9 

尋找元素 5：
  vector: 找到
  list: 找到
  deque: 找到
```

---

## 3.5 組件四：函數物件（Function Objects / Functors）

**函數物件**是重載了 `operator()` 的類別實例，它可以像函數一樣被呼叫。

### 為什麼需要函數物件？

演算法常常需要一個「策略」來決定如何操作。例如：
- `sort` 需要知道如何比較兩個元素
- `find_if` 需要知道什麼條件算「找到」
- `transform` 需要知道如何轉換元素

### 函數物件 vs 函數指標

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// 方法一：普通函數
bool is_even_func(int n) {
    return n % 2 == 0;
}

// 方法二：函數物件
class IsEven {
public:
    bool operator()(int n) const {
        return n % 2 == 0;
    }
};

// 方法三：帶狀態的函數物件
class IsDivisibleBy {
    int divisor;
public:
    IsDivisibleBy(int d) : divisor(d) {}
    bool operator()(int n) const {
        return n % divisor == 0;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 使用普通函數
    int count1 = std::count_if(vec.begin(), vec.end(), is_even_func);
    std::cout << "偶數個數（函數）: " << count1 << std::endl;
    
    // 使用函數物件
    int count2 = std::count_if(vec.begin(), vec.end(), IsEven());
    std::cout << "偶數個數（函數物件）: " << count2 << std::endl;
    
    // 使用帶狀態的函數物件
    int count3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3的倍數個數: " << count3 << std::endl;
    
    int count4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(5));
    std::cout << "5的倍數個數: " << count4 << std::endl;
    
    return 0;
}
```

**輸出：**
```
偶數個數（函數）: 5
偶數個數（函數物件）: 5
3的倍數個數: 3
5的倍數個數: 2
```

### STL 內建的函數物件

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>  // 內建函數物件

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    
    // 預設排序（升序）
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 使用 greater<int> 降序排序
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 其他內建函數物件
    std::cout << "\n內建函數物件示範：" << std::endl;
    std::cout << "plus<int>()(3, 4) = " << std::plus<int>()(3, 4) << std::endl;
    std::cout << "minus<int>()(10, 3) = " << std::minus<int>()(10, 3) << std::endl;
    std::cout << "multiplies<int>()(5, 6) = " << std::multiplies<int>()(5, 6) << std::endl;
    std::cout << "logical_and<bool>()(true, false) = " 
              << std::logical_and<bool>()(true, false) << std::endl;
    
    return 0;
}
```

**輸出：**
```
升序: 1 2 5 8 9 
降序: 9 8 5 2 1 

內建函數物件示範：
plus<int>()(3, 4) = 7
minus<int>()(10, 3) = 7
multiplies<int>()(5, 6) = 30
logical_and<bool>()(true, false) = 0
```

### Lambda 表達式：現代的函數物件

C++11 引入了 Lambda，讓函數物件的撰寫更簡潔：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Lambda 表達式
    int count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; }  // Lambda
    );
    std::cout << "偶數個數: " << count << std::endl;
    
    // 帶捕獲的 Lambda
    int divisor = 3;
    int count3 = std::count_if(vec.begin(), vec.end(),
        [divisor](int n) { return n % divisor == 0; }  // 捕獲外部變數
    );
    std::cout << "3的倍數個數: " << count3 << std::endl;
    
    // 用 Lambda 做 transform
    std::vector<int> squared;
    squared.resize(vec.size());
    std::transform(vec.begin(), vec.end(), squared.begin(),
        [](int n) { return n * n; }
    );
    std::cout << "平方: ";
    for (int n : squared) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
偶數個數: 5
3的倍數個數: 3
平方: 1 4 9 16 25 36 49 64 81 100 
```

---

## 3.6 組件五：配接器（Adapters）

**配接器**是用來修改或包裝其他組件介面的工具。STL 有三類配接器：

### 容器配接器

```cpp
#include <iostream>
#include <stack>
#include <queue>

int main() {
    // stack：後進先出（LIFO）
    std::stack<int> stk;
    stk.push(1);
    stk.push(2);
    stk.push(3);
    
    std::cout << "stack (LIFO): ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;
    
    // queue：先進先出（FIFO）
    std::queue<int> que;
    que.push(1);
    que.push(2);
    que.push(3);
    
    std::cout << "queue (FIFO): ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();
    }
    std::cout << std::endl;
    
    // priority_queue：優先佇列（預設最大值優先）
    std::priority_queue<int> pq;
    pq.push(30);
    pq.push(10);
    pq.push(50);
    pq.push(20);
    
    std::cout << "priority_queue: ";
    while (!pq.empty()) {
        std::cout << pq.top() << " ";
        pq.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
stack (LIFO): 3 2 1 
queue (FIFO): 1 2 3 
priority_queue: 50 30 20 10 
```

### 迭代器配接器

```cpp
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // reverse_iterator：反向迭代
    std::cout << "反向迭代: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;
    
    // back_inserter：自動在尾端插入
    std::vector<int> src = {10, 20, 30};
    std::vector<int> dest;
    
    std::copy(src.begin(), src.end(), std::back_inserter(dest));
    std::cout << "back_inserter 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;
    
    // ostream_iterator：輸出到串流
    std::cout << "ostream_iterator: ";
    std::copy(vec.begin(), vec.end(), 
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
反向迭代: 5 4 3 2 1 
back_inserter 結果: 10 20 30 
ostream_iterator: 1 2 3 4 5 
```

### 函數配接器（C++11 後主要用 Lambda 取代）

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // std::bind：綁定參數
    auto is_greater_than_5 = std::bind(std::greater<int>(), 
                                        std::placeholders::_1, 5);
    
    int count = std::count_if(vec.begin(), vec.end(), is_greater_than_5);
    std::cout << "大於5的個數: " << count << std::endl;
    
    // 等價的 Lambda 寫法（更推薦）
    int count2 = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n > 5; }
    );
    std::cout << "大於5的個數（Lambda）: " << count2 << std::endl;
    
    return 0;
}
```

**輸出：**
```
大於5的個數: 5
大於5的個數（Lambda）: 5
```

---

## 3.7 組件六：配置器（Allocators）

**配置器**負責容器的記憶體配置與釋放。大多數情況下，你不需要直接使用它。

### 預設配置器

```cpp
#include <iostream>
#include <vector>
#include <memory>

int main() {
    // 所有容器都有一個預設的配置器
    // vector<int> 實際上是 vector<int, allocator<int>>
    
    std::vector<int> vec1;  // 使用預設配置器
    std::vector<int, std::allocator<int>> vec2;  // 明確指定（效果相同）
    
    vec1.push_back(1);
    vec2.push_back(2);
    
    std::cout << "vec1[0] = " << vec1[0] << std::endl;
    std::cout << "vec2[0] = " << vec2[0] << std::endl;
    
    // allocator 的基本用法
    std::allocator<int> alloc;
    
    // 配置記憶體（但不建構物件）
    int* ptr = alloc.allocate(5);  // 配置 5 個 int 的空間
    
    // 建構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::construct(alloc, ptr + i, i * 10);
    }
    
    std::cout << "手動配置的陣列: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << ptr[i] << " ";
    }
    std::cout << std::endl;
    
    // 解構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::destroy(alloc, ptr + i);
    }
    
    // 釋放記憶體
    alloc.deallocate(ptr, 5);
    
    return 0;
}
```

**輸出：**
```
vec1[0] = 1
vec2[0] = 2
手動配置的陣列: 0 10 20 30 40 
```

配置器在進階應用中很有用，例如：
- 記憶體池（Memory Pool）
- 追蹤記憶體使用
- 特殊的記憶體區域（如共享記憶體）

目前你只需要知道它的存在，我們在第十六階段會深入探討。

---

## 3.8 六大組件的協作範例

讓我們用一個完整的例子展示六大組件如何協同工作：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>

int main() {
    // 【容器】儲存資料
    std::vector<int> numbers = {64, 25, 12, 22, 11, 90, 42};
    
    std::cout << "原始資料: ";
    // 【迭代器配接器】ostream_iterator 輸出
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】sort + 【函數物件】greater
    std::sort(numbers.begin(), numbers.end(), std::greater<int>());
    
    std::cout << "降序排序: ";
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】copy_if + 【Lambda（函數物件）】
    std::vector<int> even_numbers;
    std::copy_if(numbers.begin(), numbers.end(),
                 std::back_inserter(even_numbers),  // 【迭代器配接器】
                 [](int n) { return n % 2 == 0; }); // 【Lambda】
    
    std::cout << "偶數: ";
    std::copy(even_numbers.begin(), even_numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】transform + 【Lambda】
    std::vector<int> doubled;
    doubled.resize(even_numbers.size());
    std::transform(even_numbers.begin(), even_numbers.end(),
                   doubled.begin(),
                   [](int n) { return n * 2; });
    
    std::cout << "偶數加倍: ";
    std::copy(doubled.begin(), doubled.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
原始資料: 64 25 12 22 11 90 42 
降序排序: 90 64 42 25 22 12 11 
偶數: 90 64 42 22 12 
偶數加倍: 180 128 84 44 24 
```

### 組件關係圖

```
┌─────────────────────────────────────────────────────────────────┐
│                    上例中的組件協作                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   vector<int>                                                   │
│   (容器)                                                        │
│       │                                                         │
│       │ 提供                                                    │
│       ▼                                                         │
│   begin() / end()                                               │
│   (迭代器)                                                      │
│       │                                                         │
│       │ 傳給                                                    │
│       ▼                                                         │
│   sort / copy_if / transform      ◄──── greater<int> / Lambda  │
│   (演算法)                               (函數物件)              │
│       │                                                         │
│       │ 輸出到                                                  │
│       ▼                                                         │
│   back_inserter / ostream_iterator                              │
│   (迭代器配接器)                                                │
│                                                                 │
│   ※ 整個過程中，allocator 在背後默默配置記憶體                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3.9 本課重點整理

```
┌─────────────────────────────────────────────────────────────────┐
│                      第三課 重點整理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  STL 六大組件：                                                  │
│                                                                 │
│  1. 容器（Containers）                                          │
│     → 儲存資料：vector, list, set, map...                       │
│                                                                 │
│  2. 迭代器（Iterators）                                         │
│     → 遍歷容器的泛化指標                                        │
│     → 五種類別：Input, Output, Forward, Bidirectional, Random   │
│                                                                 │
│  3. 演算法（Algorithms）                                        │
│     → 操作資料：sort, find, copy, transform...                  │
│     → 透過迭代器與容器解耦                                      │
│                                                                 │
│  4. 函數物件（Functors）                                        │
│     → 可呼叫的物件：重載 operator()                             │
│     → 現代 C++ 常用 Lambda 取代                                 │
│                                                                 │
│  5. 配接器（Adapters）                                          │
│     → 容器配接器：stack, queue, priority_queue                  │
│     → 迭代器配接器：reverse_iterator, back_inserter             │
│                                                                 │
│  6. 配置器（Allocators）                                        │
│     → 管理記憶體配置與釋放                                      │
│     → 通常使用預設，進階才需自訂                                │
│                                                                 │
│  核心設計：                                                      │
│  → 容器與演算法透過迭代器連接，達到正交性                       │
│  → 函數物件讓演算法行為可客製化                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3.10 課後思考

1. **思考題**：為什麼 `std::sort` 不能直接用在 `std::list` 上，而 `list` 要有自己的 `sort` 成員函數？

2. **觀察題**：在你目前或過去的專案中，有哪些地方可以用 STL 的容器和演算法來簡化程式碼？

---

準備好進入**第四課：迭代器（Iterator）的核心概念**了嗎？下一課我們會深入探討迭代器這個「連接容器與演算法的橋樑」，理解它的設計精髓。
*/



#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>

int main() {
    // 【容器】儲存資料
    std::vector<int> numbers = {64, 25, 12, 22, 11, 90, 42};
    
    std::cout << "原始資料: ";
    // 【迭代器配接器】ostream_iterator 輸出
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】sort + 【函數物件】greater
    std::sort(numbers.begin(), numbers.end(), std::greater<int>());
    
    std::cout << "降序排序: ";
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】copy_if + 【Lambda（函數物件）】
    std::vector<int> even_numbers;
    std::copy_if(numbers.begin(), numbers.end(),
                 std::back_inserter(even_numbers),  // 【迭代器配接器】
                 [](int n) { return n % 2 == 0; }); // 【Lambda】
    
    std::cout << "偶數: ";
    std::copy(even_numbers.begin(), even_numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】transform + 【Lambda】
    std::vector<int> doubled;
    doubled.resize(even_numbers.size());
    std::transform(even_numbers.begin(), even_numbers.end(),
                   doubled.begin(),
                   [](int n) { return n * 2; });
    
    std::cout << "偶數加倍: ";
    std::copy(doubled.begin(), doubled.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    return 0;
}
