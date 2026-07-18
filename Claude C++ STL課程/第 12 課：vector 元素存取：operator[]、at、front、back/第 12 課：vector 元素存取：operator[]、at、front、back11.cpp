/*
# 第二階段：序列容器 — vector

## 第 12 課：vector 元素存取：operator[]、at、front、back

---

### 核心概念

vector 提供了多種存取元素的方式，它們在**語法**、**效能**和**安全性**上各有不同。選擇正確的存取方式，能讓程式碼既安全又高效。

---

### 一、operator[]：快速但不檢查邊界

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 讀取
    std::cout << "v[0] = " << v[0] << std::endl;  // 10
    std::cout << "v[2] = " << v[2] << std::endl;  // 30
    
    // 修改
    v[1] = 200;
    std::cout << "修改後 v[1] = " << v[1] << std::endl;  // 200
    
    return 0;
}
```

**特性**：
- 不進行邊界檢查
- 存取越界是**未定義行為**（Undefined Behavior）
- 效能最佳，與原生陣列相同

---

### 二、越界存取的危險

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 危險！越界存取
    // 編譯器不會報錯，但這是未定義行為
    std::cout << v[10] << std::endl;   // 可能印出垃圾值，可能當機
    
    v[10] = 999;  // 可能破壞其他記憶體，導致難以追蹤的 bug
    
    return 0;
}
```

未定義行為的可怕之處在於：它**可能看起來正常運作**，但在其他環境、其他時間點突然出錯。

---

### 三、at()：安全的邊界檢查

```cpp
#include <vector>
#include <iostream>
#include <stdexcept>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 正常存取
    std::cout << "v.at(0) = " << v.at(0) << std::endl;  // 10
    std::cout << "v.at(2) = " << v.at(2) << std::endl;  // 30
    
    // 越界存取會拋出例外
    try {
        std::cout << v.at(10) << std::endl;
    }
    catch (const std::out_of_range& e) {
        std::cout << "捕捉到例外: " << e.what() << std::endl;
    }
    
    return 0;
}
```

**特性**：
- 進行邊界檢查
- 越界時拋出 `std::out_of_range` 例外
- 比 `operator[]` 稍慢（多一次比較）

---

### 四、front() 與 back()：存取首尾元素

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 讀取首尾
    std::cout << "front: " << v.front() << std::endl;  // 10
    std::cout << "back:  " << v.back() << std::endl;   // 50
    
    // 等價於
    std::cout << "v[0]:          " << v[0] << std::endl;
    std::cout << "v[size()-1]:   " << v[v.size() - 1] << std::endl;
    
    // 修改首尾
    v.front() = 100;
    v.back() = 500;
    
    for (int x : v) {
        std::cout << x << " ";  // 100 20 30 40 500
    }
    std::cout << std::endl;
    
    return 0;
}
```

**注意**：對空的 vector 呼叫 `front()` 或 `back()` 是未定義行為！

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> empty_vec;
    
    // 危險！未定義行為
    // std::cout << empty_vec.front() << std::endl;
    // std::cout << empty_vec.back() << std::endl;
    
    // 安全的做法：先檢查
    if (!empty_vec.empty()) {
        std::cout << empty_vec.front() << std::endl;
    }
    
    return 0;
}
```

---

### 五、data()：取得原始指標

```cpp
#include <vector>
#include <iostream>
#include <cstring>  // for memcpy

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    // 取得指向內部陣列的指標
    int* ptr = v.data();
    
    std::cout << "ptr[0] = " << ptr[0] << std::endl;  // 10
    std::cout << "ptr[2] = " << ptr[2] << std::endl;  // 30
    
    // 用指標修改
    ptr[1] = 200;
    std::cout << "v[1] = " << v[1] << std::endl;  // 200
    
    // 實用場景：與 C 函數互動
    // 例如 memcpy、C 風格 API 等
    int dest[5];
    std::memcpy(dest, v.data(), v.size() * sizeof(int));
    
    return 0;
}
```

**注意**：空的 vector 呼叫 `data()` 可能回傳 nullptr 或有效指標（C++11 後保證合法，但不應解引用）。

---

### 六、const 正確性

當 vector 是 const 時，存取方法回傳的也是 const 參考：

```cpp
#include <vector>
#include <iostream>

void print_first(const std::vector<int>& v) {
    if (!v.empty()) {
        std::cout << v[0] << std::endl;      // OK，讀取
        std::cout << v.at(0) << std::endl;   // OK，讀取
        std::cout << v.front() << std::endl; // OK，讀取
        
        // v[0] = 100;      // 編譯錯誤！不能修改
        // v.front() = 100; // 編譯錯誤！不能修改
    }
    
    const int* ptr = v.data();  // 回傳 const int*
    // ptr[0] = 100;  // 編譯錯誤！
}

int main() {
    std::vector<int> v = {1, 2, 3};
    print_first(v);
    return 0;
}
```

---

### 七、效能比較

```cpp
#include <vector>
#include <iostream>
#include <chrono>

int main() {
    const int N = 100000000;
    std::vector<int> v(1000);
    
    // 填入資料
    for (int i = 0; i < 1000; ++i) {
        v[i] = i;
    }
    
    long long sum1 = 0, sum2 = 0;
    
    // 測試 operator[]
    auto start1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        sum1 += v[i % 1000];
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    
    // 測試 at()
    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        sum2 += v.at(i % 1000);
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    std::cout << "operator[]: " << duration1.count() << " ms" << std::endl;
    std::cout << "at():       " << duration2.count() << " ms" << std::endl;
    
    // 防止編譯器優化掉
    std::cout << "sums: " << sum1 << ", " << sum2 << std::endl;
    
    return 0;
}
```

在 Release 模式下，`at()` 通常比 `operator[]` 慢一些，但在現代編譯器優化下差異可能很小。

---

### 八、何時用哪個？

| 方法 | 適用場景 |
|------|----------|
| `operator[]` | 確定索引有效時；效能關鍵的內部迴圈 |
| `at()` | 索引來自外部輸入；需要例外處理的場合 |
| `front()` | 需要存取第一個元素，且確定非空 |
| `back()` | 需要存取最後一個元素，且確定非空 |
| `data()` | 需要與 C API 互動；需要原始指標操作 |

---

### 九、安全存取的封裝

你可以自己封裝一個更安全的存取方式：

```cpp
#include <vector>
#include <iostream>
#include <optional>

template <typename T>
std::optional<T> safe_get(const std::vector<T>& v, size_t index) {
    if (index < v.size()) {
        return v[index];
    }
    return std::nullopt;
}

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 安全存取，不會拋例外也不會未定義行為
    if (auto val = safe_get(v, 1)) {
        std::cout << "v[1] = " << *val << std::endl;  // 20
    }
    
    if (auto val = safe_get(v, 10)) {
        std::cout << "v[10] = " << *val << std::endl;
    } else {
        std::cout << "索引 10 無效" << std::endl;
    }
    
    return 0;
}
```

---

### 十、常見錯誤與陷阱

#### 陷阱一：用 size_t 迴圈的溢位問題

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 危險！當 v.size() 是 0 時，v.size() - 1 會溢位成超大數字
    // for (size_t i = v.size() - 1; i >= 0; --i) {  // 永遠為真！
    //     std::cout << v[i] << std::endl;
    // }
    
    // 安全的反向遍歷方式
    for (size_t i = v.size(); i > 0; --i) {
        std::cout << v[i - 1] << " ";  // 30 20 10
    }
    std::cout << std::endl;
    
    // 或者使用反向迭代器（第 16 課會講）
    
    return 0;
}
```

#### 陷阱二：存取後 vector 被修改

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    int& ref = v[0];          // 取得參考
    int* ptr = &v[0];         // 取得指標
    
    std::cout << "ref = " << ref << std::endl;  // 10
    
    v.push_back(40);          // 可能導致擴容！
    v.push_back(50);
    v.push_back(60);
    
    // 危險！如果發生擴容，ref 和 ptr 都已失效
    // std::cout << "ref = " << ref << std::endl;  // 未定義行為
    // std::cout << "*ptr = " << *ptr << std::endl; // 未定義行為
    
    return 0;
}
```

**規則**：任何可能導致擴容的操作都會使現有的指標、參考、迭代器失效。

---

### 存取方法一覽表

| 方法 | 回傳型別 | 邊界檢查 | 空容器行為 |
|------|----------|----------|------------|
| `v[i]` | `T&` / `const T&` | 否 | 未定義行為 |
| `v.at(i)` | `T&` / `const T&` | 是 | 拋出例外 |
| `v.front()` | `T&` / `const T&` | 否 | 未定義行為 |
| `v.back()` | `T&` / `const T&` | 否 | 未定義行為 |
| `v.data()` | `T*` / `const T*` | 否 | 回傳合法指標（不可解引用）|

---

### 練習題

1. **改錯題**：找出以下程式碼的問題並修正：
   ```cpp
   std::vector<int> v;
   for (int i = 0; i < 10; ++i) {
       v[i] = i * i;
   }
   ```

2. **實作題**：寫一個函數 `double safe_average(const std::vector<double>& v)`，計算平均值。如果 vector 為空，回傳 0.0。使用適當的存取方式。

3. **思考題**：以下兩種寫法哪個比較好？為什麼？
   ```cpp
   // 寫法 A
   if (v.size() > 0) {
       process(v[0]);
   }
   
   // 寫法 B
   if (!v.empty()) {
       process(v.front());
   }
   ```

4. **追蹤題**：以下程式碼的輸出是什麼？
   ```cpp
   std::vector<int> v = {1, 2, 3, 4, 5};
   int& a = v.front();
   int& b = v.back();
   a = 10;
   b = 50;
   v[2] = v.front() + v.back();
   for (int x : v) std::cout << x << " ";
   ```

---

下一課我們講 **vector 元素新增：push_back、emplace_back**，深入理解兩者的差異與適用場景。

準備好繼續嗎？
*/



#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    int& ref = v[0];          // 取得參考
    int* ptr = &v[0];         // 取得指標
    
    std::cout << "ref = " << ref << std::endl;  // 10
    
    v.push_back(40);          // 可能導致擴容！
    v.push_back(50);
    v.push_back(60);
    
    // 危險！如果發生擴容，ref 和 ptr 都已失效
    // std::cout << "ref = " << ref << std::endl;  // 未定義行為
    // std::cout << "*ptr = " << *ptr << std::endl; // 未定義行為
    
    return 0;
}
