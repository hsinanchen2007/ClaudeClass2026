/*
# 第二階段：序列容器 — vector

## 第 11 課：vector 的容量管理：size、capacity、reserve

---

### 核心概念

上一課我們學了如何建立 vector，這一課要深入理解如何**控制**它的記憶體行為。掌握容量管理，是寫出高效能程式的關鍵。

---

### size 與 capacity 的區別（複習）

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    
    std::cout << "size: " << v.size() << std::endl;         // 5（實際元素數量）
    std::cout << "capacity: " << v.capacity() << std::endl; // >= 5（配置的空間）
    std::cout << "empty: " << v.empty() << std::endl;       // 0（false）
    
    return 0;
}
```

記住這個關係：**size ≤ capacity**，永遠成立。

---

### 一、reserve：預先配置空間

當你知道大約需要多少元素時，用 `reserve` 預先配置空間可以避免多次擴容：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    std::cout << "初始 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    v.reserve(100);  // 預先配置至少 100 個元素的空間
    
    std::cout << "reserve(100) 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    // size 仍是 0，capacity >= 100
    
    // 現在連續 push_back 100 次都不會觸發擴容
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    
    std::cout << "100 次 push_back 後 - size: " << v.size() 
              << ", capacity: " << v.capacity() << std::endl;
    
    return 0;
}
```

**重點**：`reserve` 只改變 capacity，不改變 size，也不會建立任何元素。

---

### 二、效能差異實測

```cpp
#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 1000000;
    
    // 不使用 reserve
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v1;
        for (int i = 0; i < N; ++i) {
            v1.push_back(i);
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 使用 reserve
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        std::vector<int> v2;
        v2.reserve(N);
        for (int i = 0; i < N; ++i) {
            v2.push_back(i);
        }
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);
    
    std::cout << "不使用 reserve: " << duration1.count() << " 微秒" << std::endl;
    std::cout << "使用 reserve:   " << duration2.count() << " 微秒" << std::endl;
    
    return 0;
}
```

你會發現使用 `reserve` 的版本明顯快很多，尤其當元素是需要複製的複雜物件時。

---

### 三、resize：改變實際大小

`resize` 和 `reserve` 不同，它會**實際改變元素數量**：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::cout << "初始: size=" << v.size() << std::endl;  // 5
    
    // 擴大：新元素用預設值（0）填充
    v.resize(8);
    std::cout << "resize(8): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0
    std::cout << std::endl;
    
    // 擴大並指定填充值
    v.resize(10, 99);
    std::cout << "resize(10, 99): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3 4 5 0 0 0 99 99
    std::cout << std::endl;
    
    // 縮小：多餘的元素被銷毀
    v.resize(3);
    std::cout << "resize(3): ";
    for (int x : v) std::cout << x << " ";  // 1 2 3
    std::cout << std::endl;
    
    // 注意：縮小後 capacity 通常不變
    std::cout << "縮小後 capacity: " << v.capacity() << std::endl;  // 仍然 >= 10
    
    return 0;
}
```

---

### 四、reserve vs resize 對照

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1, v2;
    
    v1.reserve(5);
    v2.resize(5);
    
    std::cout << "reserve(5): size=" << v1.size() 
              << ", capacity=" << v1.capacity() << std::endl;
    // size=0, capacity>=5
    
    std::cout << "resize(5):  size=" << v2.size() 
              << ", capacity=" << v2.capacity() << std::endl;
    // size=5, capacity>=5
    
    // reserve 後不能直接用 operator[]
    // v1[0] = 10;  // 未定義行為！size 是 0
    
    // resize 後可以直接用 operator[]
    v2[0] = 10;  // 合法，因為 size 是 5
    
    return 0;
}
```

| 函數 | 改變 size | 改變 capacity | 建立元素 |
|------|-----------|---------------|----------|
| `reserve(n)` | 否 | 是（若 n > capacity） | 否 |
| `resize(n)` | 是 | 可能（若 n > capacity） | 是（擴大時） |

---

### 五、shrink_to_fit：釋放多餘空間

C++11 引入的 `shrink_to_fit` 可以請求釋放多餘的記憶體：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    v.reserve(1000);
    
    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }
    
    std::cout << "shrink 前: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity=1000
    
    v.shrink_to_fit();
    
    std::cout << "shrink 後: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=10, capacity 約等於 10（不保證）
    
    return 0;
}
```

**注意**：`shrink_to_fit` 是**非強制性的請求**，編譯器可以忽略它。實務上大多數實作會照做。

---

### 六、max_size：理論最大容量

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v;
    
    std::cout << "max_size: " << v.max_size() << std::endl;
    
    // 在 64 位元系統上，這個數字通常非常大
    // 例如：4611686018427387903（約 4.6 × 10^18）
    
    return 0;
}
```

這是理論上的最大值，實際上受限於可用記憶體。

---

### 七、清空 vector 的方式比較

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    std::cout << "初始: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    
    // 方法一：clear（保留 capacity）
    v.clear();
    std::cout << "clear(): size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity 不變
    
    // 重新填入資料
    v = {1, 2, 3, 4, 5};
    
    // 方法二：swap 技巧（C++11 前常用，釋放記憶體）
    std::vector<int>().swap(v);
    std::cout << "swap 技巧: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity=0
    
    // 重新填入資料
    v = {1, 2, 3, 4, 5};
    v.reserve(100);
    
    // 方法三：clear + shrink_to_fit（C++11 後推薦）
    v.clear();
    v.shrink_to_fit();
    std::cout << "clear + shrink: size=" << v.size() 
              << ", capacity=" << v.capacity() << std::endl;
    // size=0, capacity 約為 0
    
    return 0;
}
```

---

### 八、實際應用場景

#### 場景一：讀取已知大小的資料

```cpp
#include <vector>
#include <iostream>

int main() {
    int n;
    std::cout << "請輸入資料筆數: ";
    std::cin >> n;
    
    std::vector<int> data;
    data.reserve(n);  // 預先配置，避免擴容
    
    std::cout << "請輸入 " << n << " 筆整數: ";
    for (int i = 0; i < n; ++i) {
        int x;
        std::cin >> x;
        data.push_back(x);
    }
    
    // 處理資料...
    
    return 0;
}
```

#### 場景二：建立固定大小的緩衝區

```cpp
#include <vector>
#include <cstdint>

int main() {
    // 建立 1MB 的緩衝區
    std::vector<uint8_t> buffer(1024 * 1024);  // resize，直接可用
    
    // 或者只配置空間，稍後填入
    std::vector<uint8_t> buffer2;
    buffer2.reserve(1024 * 1024);  // 只 reserve，size 仍為 0
    
    return 0;
}
```

#### 場景三：收集不確定數量的結果

```cpp
#include <vector>
#include <iostream>

std::vector<int> find_even_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    
    // 最多有 input.size() 個偶數，所以這是合理的上界
    result.reserve(input.size());
    
    for (int x : input) {
        if (x % 2 == 0) {
            result.push_back(x);
        }
    }
    
    // 可選：釋放多餘空間
    result.shrink_to_fit();
    
    return result;
}

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = find_even_numbers(nums);
    
    for (int x : evens) {
        std::cout << x << " ";  // 2 4 6 8 10
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### 容量管理函數一覽

| 函數 | 說明 |
|------|------|
| `size()` | 回傳目前元素數量 |
| `capacity()` | 回傳目前配置的空間可容納的元素數量 |
| `empty()` | 回傳 size() == 0 |
| `max_size()` | 回傳理論最大容量 |
| `reserve(n)` | 確保 capacity >= n，不改變 size |
| `resize(n)` | 改變 size 為 n，必要時擴大 capacity |
| `resize(n, val)` | 同上，新元素用 val 填充 |
| `shrink_to_fit()` | 請求釋放多餘的 capacity |
| `clear()` | 移除所有元素，size 變 0，capacity 不變 |

---

### 練習題

1. **預測題**：以下程式碼執行後，v 的 size 和 capacity 分別是多少？
   ```cpp
   std::vector<int> v;
   v.reserve(10);
   v.resize(5);
   v.push_back(100);
   v.resize(3);
   v.shrink_to_fit();
   ```

2. **實作題**：寫一個函數，接收一個 `vector<int>`，回傳一個新的 vector，只包含原本的奇數。要求：使用 reserve 優化效能。

3. **思考題**：為什麼 `shrink_to_fit` 被設計成「非強制性請求」而不是保證執行？（提示：考慮記憶體配置器的限制）

4. **除錯題**：以下程式碼有什麼問題？
   ```cpp
   std::vector<int> v;
   v.reserve(100);
   for (int i = 0; i < 100; ++i) {
       v[i] = i;  // 這裡有問題嗎？
   }
   ```

---

下一課我們講 **vector 元素存取：operator[]、at、front、back**，學習各種存取元素的方式及其安全性差異。

準備好繼續嗎？
*/



#include <vector>
#include <iostream>

std::vector<int> find_even_numbers(const std::vector<int>& input) {
    std::vector<int> result;
    
    // 最多有 input.size() 個偶數，所以這是合理的上界
    result.reserve(input.size());
    
    for (int x : input) {
        if (x % 2 == 0) {
            result.push_back(x);
        }
    }
    
    // 可選：釋放多餘空間
    result.shrink_to_fit();
    
    return result;
}

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto evens = find_even_numbers(nums);
    
    for (int x : evens) {
        std::cout << x << " ";  // 2 4 6 8 10
    }
    std::cout << std::endl;
    
    return 0;
}
