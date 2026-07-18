/*
好的，陳信安，我們繼續 STL 課程。根據大綱，現在進入：

---

# 第二階段：序列容器 — vector

## 第 9 課：vector 的內部結構與記憶體配置

---

### 核心概念

`std::vector` 是 STL 中最常用的容器，它的本質是一個**動態陣列**（dynamic array）。要真正理解 vector，必須先搞懂它在記憶體中長什麼樣子。

---

### vector 的三個關鍵指標

vector 內部維護三個指標，這是理解其所有行為的基礎：

```cpp
template <typename T, typename Allocator = std::allocator<T>>
class vector {
    // 簡化的內部結構
    T* _begin;    // 指向第一個元素
    T* _end;      // 指向最後一個元素的下一個位置
    T* _cap;      // 指向配置空間的尾端
};
```

用圖來表示：

```
已配置的記憶體區塊
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ A │ B │ C │ D │ E │   │   │   │
└───┴───┴───┴───┴───┴───┴───┴───┘
↑                   ↑           ↑
_begin             _end        _cap

size() = _end - _begin = 5（實際存放的元素數量）
capacity() = _cap - _begin = 8（已配置的空間可容納的元素數量）
```

---

### 連續記憶體的保證

C++ 標準保證 vector 的元素儲存在**連續的記憶體空間**中。這個保證非常重要：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 印出每個元素的位址
    for (size_t i = 0; i < v.size(); ++i) {
        std::cout << "v[" << i << "] 位址: " << &v[i] << std::endl;
    }
    
    // 因為是連續記憶體，可以取得原始指標
    int* ptr = v.data();  // 或 &v[0]
    std::cout << "第三個元素: " << ptr[2] << std::endl;  // 輸出 30
    
    return 0;
}
```

執行後你會看到位址是連續的，每個 int 相差 4 bytes（假設 sizeof(int) == 4）。

---

### 為什麼要分開 size 和 capacity？

這是 vector 效能的關鍵設計。考慮這個情境：

```cpp
std::vector<int> v;

// 連續新增 1000 個元素
for (int i = 0; i < 1000; ++i) {
    v.push_back(i);
}
```

如果每次 `push_back` 都重新配置記憶體：
- 配置 1 個元素的空間，複製 0 個舊元素
- 配置 2 個元素的空間，複製 1 個舊元素
- 配置 3 個元素的空間，複製 2 個舊元素
- ...
- 配置 1000 個元素的空間，複製 999 個舊元素

總複製次數 = 0 + 1 + 2 + ... + 999 = **499,500 次**

這太慢了。所以 vector 採用**預留空間**的策略：當空間不夠時，一次配置更多的空間（通常是原本的 1.5 倍或 2 倍）。

---

### 實際觀察 capacity 的變化

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    size_t prev_cap = 0;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            std::cout << "size: " << v.size() 
                      << ", capacity: " << v.capacity() << std::endl;
            prev_cap = v.capacity();
        }
    }
    
    return 0;
}
```

在不同編譯器上，你可能看到：

**MSVC（通常 1.5 倍成長）：**
```
size: 1, capacity: 1
size: 2, capacity: 2
size: 3, capacity: 3
size: 4, capacity: 4
size: 5, capacity: 6
size: 7, capacity: 9
size: 10, capacity: 13
...
```

**GCC/Clang（通常 2 倍成長）：**
```
size: 1, capacity: 1
size: 2, capacity: 2
size: 3, capacity: 4
size: 5, capacity: 8
size: 9, capacity: 16
size: 17, capacity: 32
...
```

---

### 記憶體配置的代價

當 capacity 不足時，vector 會：

1. **配置新的更大的記憶體區塊**
2. **將所有舊元素移動或複製到新空間**
3. **銷毀舊空間中的元素**
4. **釋放舊的記憶體區塊**

```cpp
#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    Tracker(int i) : id(i) { 
        std::cout << "建構 " << id << std::endl; 
    }
    Tracker(const Tracker& other) : id(other.id) { 
        std::cout << "複製 " << id << std::endl; 
    }
    Tracker(Tracker&& other) noexcept : id(other.id) { 
        std::cout << "移動 " << id << std::endl; 
    }
    ~Tracker() { 
        std::cout << "銷毀 " << id << std::endl; 
    }
};

int main() {
    std::vector<Tracker> v;
    
    std::cout << "--- 加入第 1 個 ---" << std::endl;
    v.emplace_back(1);
    
    std::cout << "--- 加入第 2 個 ---" << std::endl;
    v.emplace_back(2);
    
    std::cout << "--- 加入第 3 個 ---" << std::endl;
    v.emplace_back(3);
    
    std::cout << "--- 程式結束 ---" << std::endl;
    return 0;
}
```

執行這段程式，你會清楚看到每次擴容時的複製/移動和銷毀操作。

---

### 重點整理

| 概念 | 說明 |
|------|------|
| 連續記憶體 | vector 元素保證存放在連續空間，可用指標算術存取 |
| size | 目前實際存放的元素數量 |
| capacity | 目前配置的空間可容納的元素數量 |
| 擴容策略 | 空間不足時，配置 1.5x 或 2x 的新空間並搬移元素 |
| 擴容代價 | 需要配置新空間、搬移所有元素、釋放舊空間 |

---

### 練習題

1. **觀察題**：寫一個程式，建立一個空的 `vector<int>`，連續 push_back 50 個元素，記錄每次 capacity 變化時的值。你的編譯器採用什麼擴容策略？

2. **思考題**：如果你事先知道需要存放 1000 個元素，有什麼方法可以避免多次擴容？（提示：這會在第 11 課詳細講解）

3. **驗證題**：修改 Tracker 類別的程式，把移動建構子的 `noexcept` 拿掉，觀察輸出有什麼變化。為什麼會這樣？

---

下一課我們會講 **vector 的宣告與初始化方式**，涵蓋各種建立 vector 的方法。

有任何問題嗎？或者我們繼續第 10 課？
*/



#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    Tracker(int i) : id(i) { 
        std::cout << "建構 " << id << std::endl; 
    }
    Tracker(const Tracker& other) : id(other.id) { 
        std::cout << "複製 " << id << std::endl; 
    }
    Tracker(Tracker&& other) noexcept : id(other.id) { 
        std::cout << "移動 " << id << std::endl; 
    }
    ~Tracker() { 
        std::cout << "銷毀 " << id << std::endl; 
    }
};

int main() {
    std::vector<Tracker> v;
    
    std::cout << "--- 加入第 1 個 ---" << std::endl;
    v.emplace_back(1);
    
    std::cout << "--- 加入第 2 個 ---" << std::endl;
    v.emplace_back(2);
    
    std::cout << "--- 加入第 3 個 ---" << std::endl;
    v.emplace_back(3);
    
    std::cout << "--- 程式結束 ---" << std::endl;
    return 0;
}
