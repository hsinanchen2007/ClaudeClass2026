/*
# 第二階段：序列容器 — vector

## 第 10 課：vector 的宣告與初始化方式

---

### 核心概念

C++ 提供了多種初始化 vector 的方式，每種都有其適用場景。理解這些方式的差異，能讓你寫出更清晰、更有效率的程式碼。

---

### 一、預設建構：空的 vector

```cpp
#include <vector>

int main() {
    std::vector<int> v1;           // 空的 vector，size=0，capacity=0
    std::vector<double> v2{};      // 同上，C++11 統一初始化語法
    std::vector<std::string> v3 = {};  // 同上
    
    return 0;
}
```

這三種寫法效果相同，建立一個不含任何元素的 vector。

---

### 二、指定大小的建構

```cpp
#include <vector>
#include <iostream>

int main() {
    // 建立含有 5 個元素的 vector，每個元素都是預設值（int 的預設值是 0）
    std::vector<int> v1(5);
    
    for (int x : v1) {
        std::cout << x << " ";  // 輸出：0 0 0 0 0
    }
    std::cout << std::endl;
    
    // 建立含有 5 個元素的 vector，每個元素都是 42
    std::vector<int> v2(5, 42);
    
    for (int x : v2) {
        std::cout << x << " ";  // 輸出：42 42 42 42 42
    }
    std::cout << std::endl;
    
    return 0;
}
```

**注意**：這裡用的是小括號 `()`，不是大括號 `{}`。這個差異很重要，稍後會詳細說明。

---

### 三、初始化串列（Initializer List）

C++11 引入的大括號初始化，可以直接列出元素：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1 = {1, 2, 3, 4, 5};    // 明確的初始化串列
    std::vector<int> v2{1, 2, 3, 4, 5};       // 同上，省略等號
    std::vector<int> v3({1, 2, 3, 4, 5});     // 同上，但較少人這樣寫
    
    for (int x : v1) {
        std::cout << x << " ";  // 輸出：1 2 3 4 5
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### 四、小括號 vs 大括號的陷阱

這是初學者最容易混淆的地方：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v1(5, 10);   // 5 個元素，每個都是 10
    std::vector<int> v2{5, 10};   // 2 個元素，分別是 5 和 10
    
    std::cout << "v1 size: " << v1.size() << std::endl;  // 輸出：5
    std::cout << "v2 size: " << v2.size() << std::endl;  // 輸出：2
    
    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";  // 輸出：10 10 10 10 10
    std::cout << std::endl;
    
    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";  // 輸出：5 10
    std::cout << std::endl;
    
    return 0;
}
```

**規則**：
- `()` 呼叫建構子，`(5, 10)` 意思是「5 個元素，值為 10」
- `{}` 優先嘗試初始化串列，`{5, 10}` 意思是「元素是 5 和 10」

---

### 五、從其他容器或範圍初始化

```cpp
#include <vector>
#include <array>
#include <iostream>

int main() {
    // 從另一個 vector 複製
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> copy1(original);           // 複製建構
    std::vector<int> copy2 = original;          // 同上
    
    // 從迭代器範圍建構
    std::vector<int> partial(original.begin() + 1, original.begin() + 4);
    for (int x : partial) {
        std::cout << x << " ";  // 輸出：2 3 4
    }
    std::cout << std::endl;
    
    // 從 C 風格陣列建構
    int arr[] = {10, 20, 30, 40};
    std::vector<int> from_array(std::begin(arr), std::end(arr));
    // 或者：std::vector<int> from_array(arr, arr + 4);
    
    // 從 std::array 建構
    std::array<int, 3> std_arr = {100, 200, 300};
    std::vector<int> from_std_array(std_arr.begin(), std_arr.end());
    
    return 0;
}
```

---

### 六、移動建構

當來源 vector 不再需要時，可以「移動」而非「複製」：

```cpp
#include <vector>
#include <iostream>
#include <utility>  // for std::move

int main() {
    std::vector<int> source = {1, 2, 3, 4, 5};
    
    std::cout << "移動前 source size: " << source.size() << std::endl;
    
    std::vector<int> dest = std::move(source);  // 移動建構
    
    std::cout << "移動後 source size: " << source.size() << std::endl;  // 通常是 0
    std::cout << "移動後 dest size: " << dest.size() << std::endl;      // 5
    
    return 0;
}
```

移動後，`source` 處於「有效但未指定」的狀態，通常是空的。移動操作只是轉移指標的所有權，不會複製元素，效率很高。

---

### 七、使用 assign 重新初始化

已存在的 vector 可以用 `assign` 重新設定內容：

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3};
    
    // 方法一：指定數量和值
    v.assign(5, 100);
    std::cout << "assign(5, 100): ";
    for (int x : v) std::cout << x << " ";  // 輸出：100 100 100 100 100
    std::cout << std::endl;
    
    // 方法二：從初始化串列
    v.assign({10, 20, 30});
    std::cout << "assign({10, 20, 30}): ";
    for (int x : v) std::cout << x << " ";  // 輸出：10 20 30
    std::cout << std::endl;
    
    // 方法三：從迭代器範圍
    std::vector<int> other = {7, 8, 9, 10, 11};
    v.assign(other.begin() + 1, other.end() - 1);
    std::cout << "從迭代器範圍: ";
    for (int x : v) std::cout << x << " ";  // 輸出：8 9 10
    std::cout << std::endl;
    
    return 0;
}
```

---

### 八、自訂類別的 vector 初始化

```cpp
#include <vector>
#include <string>
#include <iostream>

struct Person {
    std::string name;
    int age;
    
    Person(const std::string& n, int a) : name(n), age(a) {}
};

int main() {
    // 使用初始化串列（需要隱式轉換或大括號建構）
    std::vector<Person> people = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };
    
    for (const auto& p : people) {
        std::cout << p.name << " is " << p.age << " years old." << std::endl;
    }
    
    return 0;
}
```

---

### 九、C++17 類別模板引數推導（CTAD）

C++17 開始，某些情況下可以省略模板參數：

```cpp
#include <vector>
#include <iostream>

int main() {
    // C++17 之前必須寫：std::vector<int> v = {1, 2, 3};
    // C++17 可以讓編譯器推導：
    std::vector v1 = {1, 2, 3};       // 推導為 vector<int>
    std::vector v2 = {1.5, 2.5, 3.5}; // 推導為 vector<double>
    
    // 從迭代器推導
    std::vector<int> source = {10, 20, 30};
    std::vector v3(source.begin(), source.end());  // 推導為 vector<int>
    
    return 0;
}
```

**注意**：CTAD 有時會產生意外結果，建議在型別不明顯時還是明確寫出來。

---

### 各種初始化方式對照表

| 語法 | 結果 | 說明 |
|------|------|------|
| `vector<int> v;` | 空 vector | 預設建構 |
| `vector<int> v(5);` | `{0,0,0,0,0}` | 5 個預設值元素 |
| `vector<int> v(5, 42);` | `{42,42,42,42,42}` | 5 個值為 42 的元素 |
| `vector<int> v{5, 42};` | `{5, 42}` | 2 個元素：5 和 42 |
| `vector<int> v = {1,2,3};` | `{1, 2, 3}` | 初始化串列 |
| `vector<int> v(other);` | 複製 other | 複製建構 |
| `vector<int> v(std::move(other));` | 接收 other 的資源 | 移動建構 |
| `vector<int> v(it1, it2);` | 範圍 [it1, it2) | 迭代器範圍建構 |

---

### 練習題

1. **辨識題**：以下程式碼建立的 vector 各有幾個元素？值分別是什麼？
   ```cpp
   std::vector<int> a(3);
   std::vector<int> b(3, 7);
   std::vector<int> c{3};
   std::vector<int> d{3, 7};
   std::vector<int> e(3, 7, 9);  // 這行能編譯嗎？
   ```

2. **實作題**：建立一個 `vector<double>`，包含 1.0 到 10.0 的等差數列（共 10 個元素）。用兩種不同的方式完成。

3. **思考題**：假設你有一個函數回傳 `vector<string>`，在呼叫端接收時，用 `auto result = func();` 和 `auto&& result = func();` 有什麼差異？（提示：考慮 C++17 的保證複製消除）

---

下一課我們講 **vector 的容量管理：size、capacity、reserve**，這是控制 vector 記憶體行為的關鍵。

準備好繼續嗎？
*/



#include <vector>
#include <iostream>

int main() {
    // C++17 之前必須寫：std::vector<int> v = {1, 2, 3};
    // C++17 可以讓編譯器推導：
    std::vector v1 = {1, 2, 3};       // 推導為 vector<int>
    std::vector v2 = {1.5, 2.5, 3.5}; // 推導為 vector<double>
    
    // 從迭代器推導
    std::vector<int> source = {10, 20, 30};
    std::vector v3(source.begin(), source.end());  // 推導為 vector<int>
    
    return 0;
}
