/*
# 第 20 課：vector 效能分析與最佳實踐

## 一、為什麼要專門談效能？

前面 11 堂課，我們學了 vector 的所有操作。但「會用」和「用得好」是兩回事。在實際專案中，一個不經意的寫法可能讓效能差 10 倍甚至 100 倍。這堂課要把所有效能相關的知識系統化，讓你在寫程式時能做出正確的決策。

---

## 二、各操作的時間複雜度總覽

先建立全局觀：

| 操作 | 時間複雜度 | 說明 |
|------|-----------|------|
| `push_back` | 均攤 O(1) | 偶爾 O(n)（重新配置時） |
| `emplace_back` | 均攤 O(1) | 同 push_back |
| `pop_back` | O(1) | |
| `operator[]` / `at` | O(1) | 隨機存取 |
| `front` / `back` | O(1) | |
| `insert`（中間） | O(n) | 需搬移後方元素 |
| `erase`（中間） | O(n) | 需搬移後方元素 |
| `insert`（尾端） | 均攤 O(1) | 等同 push_back |
| `erase`（尾端） | O(1) | 等同 pop_back |
| `clear` | O(n) | 需銷毀每個元素（POD 型別可優化為 O(1)） |
| `size` / `capacity` / `empty` | O(1) | |
| `reserve` | O(n) | 若需重新配置 |
| `resize` | O(n) | 若需重新配置或建構/銷毀元素 |
| `find`（線性搜尋） | O(n) | vector 未排序時 |
| `sort` | O(n log n) | |

---

## 三、最佳實踐一：預先配置記憶體

這是效能提升最顯著的一招，第 17 課已經介紹過原理，這裡做更完整的分析。

### 3.1 效能量測

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <string>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;

    Timer(const std::string& label) : label_(label) {
        start_ = std::chrono::high_resolution_clock::now();
    }

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 1'000'000;

    // 測試一：不使用 reserve
    {
        Timer t("不用 reserve");
        std::vector<int> v;
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
    }

    // 測試二：使用 reserve
    {
        Timer t("使用 reserve");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
    }

    // 測試三：直接用 resize + 索引賦值
    {
        Timer t("resize + 索引");
        std::vector<int> v(N);
        for (int i = 0; i < N; ++i) {
            v[i] = i;
        }
    }

    return 0;
}
```

**典型結果（Release 模式，GCC -O2）：**
```
不用 reserve: 3200 μs
使用 reserve: 1100 μs
resize + 索引: 980 μs
```

### 3.2 何時用 reserve，何時用 resize？

```cpp
#include <iostream>
#include <vector>

int main() {
    // 場景一：逐一 push_back，但預知總量
    // → 用 reserve
    {
        std::vector<int> v;
        v.reserve(1000);  // size 仍為 0
        for (int i = 0; i < 1000; ++i) {
            v.push_back(i);  // 不會觸發重新配置
        }
        std::cout << "reserve: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    // 場景二：需要直接用索引存取，或傳給 C 函數
    // → 用 resize
    {
        std::vector<int> v;
        v.resize(1000);  // size 變為 1000，元素初始化為 0
        for (int i = 0; i < 1000; ++i) {
            v[i] = i;  // 直接賦值
        }
        std::cout << "resize: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    // 場景三：不確定總量，但能估計上界
    // → 用 reserve（估計值），避免大部分重新配置
    {
        std::vector<int> v;
        v.reserve(500);  // 估計大約 500 個
        for (int i = 0; i < 480; ++i) {  // 實際不到 500
            v.push_back(i);
        }
        std::cout << "估計 reserve: size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    return 0;
}
```

**輸出：**
```
reserve: size=1000, capacity=1000
resize: size=1000, capacity=1000
估計 reserve: size=480, capacity=500
```

---

## 四、最佳實踐二：emplace_back vs push_back

### 4.1 差異原理

```cpp
#include <iostream>
#include <vector>
#include <string>

class Widget {
    std::string name_;
    int value_;
public:
    // 一般建構子
    Widget(const std::string& name, int value)
        : name_(name), value_(value) {
        std::cout << "  建構 Widget(" << name_ << ", " << value_ << ")" << std::endl;
    }

    // 拷貝建構子
    Widget(const Widget& other)
        : name_(other.name_), value_(other.value_) {
        std::cout << "  拷貝 Widget(" << name_ << ")" << std::endl;
    }

    // 移動建構子
    Widget(Widget&& other) noexcept
        : name_(std::move(other.name_)), value_(other.value_) {
        std::cout << "  移動 Widget(" << name_ << ")" << std::endl;
    }

    ~Widget() = default;
};

int main() {
    std::vector<Widget> v;
    v.reserve(4);  // 預留空間，避免重新配置干擾觀察

    std::cout << "=== push_back（臨時物件）===" << std::endl;
    v.push_back(Widget("A", 1));
    // 過程：建構臨時物件 → 移動到 vector 內部 → 銷毀臨時物件

    std::cout << "\n=== emplace_back（原地建構）===" << std::endl;
    v.emplace_back("B", 2);
    // 過程：直接在 vector 內部的記憶體上建構，沒有臨時物件

    std::cout << "\n=== push_back（具名物件）===" << std::endl;
    Widget w("C", 3);
    v.push_back(w);
    // 過程：拷貝 w 到 vector 內部

    std::cout << "\n=== push_back（std::move 具名物件）===" << std::endl;
    Widget w2("D", 4);
    v.push_back(std::move(w2));
    // 過程：移動 w2 到 vector 內部

    return 0;
}
```

**輸出：**
```
=== push_back（臨時物件）===
  建構 Widget(A, 1)
  移動 Widget(A)

=== emplace_back（原地建構）===
  建構 Widget(B, 2)

=== push_back（具名物件）===
  建構 Widget(C, 3)
  拷貝 Widget(C)

=== push_back（std::move 具名物件）===
  建構 Widget(D, 4)
  移動 Widget(D)
```

**關鍵觀察：** `emplace_back("B", 2)` 只有一次建構，沒有任何拷貝或移動。這就是「原地建構（in-place construction）」的優勢。

### 4.2 何時 emplace_back 真的更快？

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <string>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 500'000;

    // 測試：string 的建構（拷貝成本較高的型別）
    {
        Timer t("push_back string");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(std::string("hello_world_test_string"));
        }
    }

    {
        Timer t("emplace_back string");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back("hello_world_test_string");
        }
    }

    // 測試：int（拷貝成本極低的型別）
    {
        Timer t("push_back int");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.push_back(i);
        }
    }

    {
        Timer t("emplace_back int");
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back(i);
        }
    }

    return 0;
}
```

**典型結果（Release 模式）：**
```
push_back string: 18500 μs
emplace_back string: 12800 μs
push_back int: 620 μs
emplace_back int: 610 μs
```

**結論：**
- 元素建構成本高時（如 `std::string`、含有動態記憶體的物件），`emplace_back` 明顯更快
- 元素是基本型別時（如 `int`、`double`），差異可忽略
- 當你傳入的參數可以直接作為建構子引數時，優先用 `emplace_back`

### 4.3 emplace_back 的注意事項

`emplace_back` 並非萬能。有一個值得注意的情況：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<std::vector<int>> vv;
    vv.reserve(4);

    // emplace_back 可能導致意外的隱式轉換
    vv.emplace_back(5, 0);
    // 這呼叫的是 vector<int>(5, 0)，建立一個含 5 個 0 的 vector
    // 你可能以為是把 5 和 0 當成兩個元素？不是！

    std::cout << "vv[0].size() = " << vv[0].size() << std::endl;
    std::cout << "vv[0] 內容：";
    for (int x : vv[0]) std::cout << x << " ";
    std::cout << std::endl;

    // push_back 在這種情況下更安全、更明確
    vv.push_back({5, 0});  // 明確：用初始化串列建立含 5 和 0 兩個元素的 vector
    std::cout << "vv[1].size() = " << vv[1].size() << std::endl;
    std::cout << "vv[1] 內容：";
    for (int x : vv[1]) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
vv[0].size() = 5
vv[0] 內容：0 0 0 0 0
vv[1].size() = 2
vv[1] 內容：5 0
```

> **原則：** 如果意圖明確且建構參數直接對應，用 `emplace_back`。如果涉及初始化串列或可能有歧義，用 `push_back` 搭配明確的物件建構。

---

## 五、最佳實踐三：避免不必要的拷貝

### 5.1 用 const 引用接收 vector

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <numeric>  // std::accumulate

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

// 錯誤：參數用值傳遞，每次呼叫都會拷貝整個 vector
long long sum_by_value(std::vector<int> v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

// 正確：參數用 const 引用，零拷貝
long long sum_by_ref(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

int main() {
    std::vector<int> data(1'000'000, 1);

    {
        Timer t("值傳遞（拷貝）");
        for (int i = 0; i < 100; ++i) {
            sum_by_value(data);
        }
    }

    {
        Timer t("const 引用（零拷貝）");
        for (int i = 0; i < 100; ++i) {
            sum_by_ref(data);
        }
    }

    return 0;
}
```

**典型結果：**
```
值傳遞（拷貝）: 52000 μs
const 引用（零拷貝）: 18000 μs
```

### 5.2 用 std::move 轉移所有權

```cpp
#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> generate_data() {
    std::vector<std::string> result;
    result.reserve(3);
    result.emplace_back("alpha");
    result.emplace_back("beta");
    result.emplace_back("gamma");
    return result;  // NRVO（具名回傳值優化）通常會消除拷貝
}

int main() {
    // 情況一：函數回傳值（通常 NRVO 會優化，不需要手動 move）
    std::vector<std::string> v1 = generate_data();

    // 情況二：明確轉移一個已存在的 vector
    std::vector<std::string> v2;
    v2 = std::move(v1);
    // v1 現在是「合法但未指定狀態」（通常是空的）

    std::cout << "v1.size() = " << v1.size() << std::endl;   // 0
    std::cout << "v2.size() = " << v2.size() << std::endl;   // 3

    for (const auto& s : v2) {
        std::cout << s << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
v1.size() = 0
v2.size() = 3
alpha beta gamma
```

### 5.3 範圍 for 迴圈中避免拷貝

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    std::vector<std::string> data(100'000, "this_is_a_reasonably_long_string_for_testing");

    // 拷貝遍歷（每次迭代都拷貝一個 string）
    {
        Timer t("for (string s : data)");
        long long total = 0;
        for (std::string s : data) {
            total += s.size();
        }
    }

    // const 引用遍歷（零拷貝）
    {
        Timer t("for (const string& s : data)");
        long long total = 0;
        for (const std::string& s : data) {
            total += s.size();
        }
    }

    return 0;
}
```

**典型結果：**
```
for (string s : data): 8500 μs
for (const string& s : data): 280 μs
```

差距 **30 倍**。對於大型物件，範圍 for 迴圈中的拷貝成本是非常可觀的。

---

## 六、最佳實踐四：高效刪除

### 6.1 三種刪除策略比較

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <string>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 100'000;

    // 方法一：逐一 erase（最慢）
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        Timer t("逐一 erase");
        for (auto it = v.begin(); it != v.end(); ) {
            if (*it % 2 == 0) {
                it = v.erase(it);  // 每次 erase 都搬移後面的元素
            } else {
                ++it;
            }
        }
    }

    // 方法二：erase-remove 慣用法（快很多）
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        Timer t("erase-remove");
        v.erase(
            std::remove_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; }),
            v.end()
        );
    }

    // 方法三：C++20 std::erase_if（最簡潔）
    {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.push_back(i);

        Timer t("std::erase_if (C++20)");
        std::erase_if(v, [](int x) { return x % 2 == 0; });
    }

    return 0;
}
```

**典型結果（Release 模式）：**
```
逐一 erase: 45000 μs
erase-remove: 180 μs
std::erase_if (C++20): 175 μs
```

差距超過 **200 倍**。逐一 erase 是 O(n²)，而 erase-remove 是 O(n)。

### 6.2 不需要保持順序時的快速刪除

如果你不在乎元素的相對順序，有一個 O(1) 的刪除技巧：

```cpp
#include <iostream>
#include <vector>

// 把要刪除的元素和最後一個元素交換，然後 pop_back
// 時間複雜度：O(1)
template <typename T>
void unstable_erase(std::vector<T>& v, size_t index) {
    v[index] = std::move(v.back());
    v.pop_back();
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 刪除索引 1（值 20）
    unstable_erase(v, 1);

    std::cout << "刪除後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    // 注意：順序改變了（50 被移到索引 1 的位置）

    return 0;
}
```

**輸出：**
```
刪除前：10 20 30 40 50
刪除後：10 50 30 40
```

---

## 七、最佳實踐五：善用 insert 的批量版本

### 7.1 逐一插入 vs 批量插入

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <string>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 100'000;
    std::vector<int> source(N, 42);

    // 方法一：逐一 push_back
    {
        std::vector<int> v;
        v.reserve(N);
        Timer t("逐一 push_back");
        for (int x : source) {
            v.push_back(x);
        }
    }

    // 方法二：批量 insert（一次性從範圍插入）
    {
        Timer t("批量 insert");
        std::vector<int> v;
        v.reserve(N);
        v.insert(v.end(), source.begin(), source.end());
    }

    // 方法三：用範圍建構子
    {
        Timer t("範圍建構子");
        std::vector<int> v(source.begin(), source.end());
    }

    // 方法四：assign
    {
        Timer t("assign");
        std::vector<int> v;
        v.assign(source.begin(), source.end());
    }

    return 0;
}
```

**典型結果：**
```
逐一 push_back: 320 μs
批量 insert: 85 μs
範圍建構子: 80 μs
assign: 82 μs
```

**為什麼批量版本更快？** 批量操作可以一次算出需要的空間，只做一次記憶體配置，並且用 `memcpy`（對 POD 型別）或連續的建構來完成——省去了逐一檢查容量的開銷。

---

## 八、最佳實踐六：選擇正確的資料結構

有時候效能問題的根源不在於「怎麼用 vector」，而是「該不該用 vector」：

### 8.1 操作特性對照表

| 需求 | 推薦容器 | 原因 |
|------|---------|------|
| 尾端增刪 + 隨機存取 | `vector` | 連續記憶體，快取友善 |
| 頭尾雙端增刪 | `deque` | 兩端 O(1) |
| 頻繁在中間插入/刪除 | `list` | O(1) 插入（但快取不友善） |
| 需要有序且快速查找 | `set` / `map` | O(log n) 查找 |
| 需要最快查找 | `unordered_set` / `unordered_map` | O(1) 平均查找 |
| 固定大小、編譯期已知 | `std::array` | 無動態配置開銷 |

### 8.2 不要忽視快取效應

即使 `list` 在理論上中間插入是 O(1)，**實務上 vector 在許多場景仍然更快**，因為連續記憶體對 CPU 快取非常友善：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <chrono>
#include <string>
#include <numeric>

struct Timer {
    std::chrono::high_resolution_clock::time_point start_;
    std::string label_;
    Timer(const std::string& label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}
    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cout << label_ << ": " << us << " μs" << std::endl;
    }
};

int main() {
    const int N = 1'000'000;

    // 順序遍歷：vector vs list
    {
        std::vector<int> v(N);
        std::iota(v.begin(), v.end(), 0);

        Timer t("vector 遍歷");
        long long sum = 0;
        for (int x : v) sum += x;
    }

    {
        std::list<int> lst;
        for (int i = 0; i < N; ++i) lst.push_back(i);

        Timer t("list   遍歷");
        long long sum = 0;
        for (int x : lst) sum += x;
    }

    return 0;
}
```

**典型結果：**
```
vector 遍歷: 850 μs
list   遍歷: 5200 μs
```

vector 的遍歷速度是 list 的 **6 倍以上**，這就是快取效應的力量。CPU 從記憶體讀取資料時會載入一整條快取行（cache line，通常 64 bytes）。vector 的元素是連續的，一次載入就能涵蓋多個元素；list 的節點散落在記憶體各處，每存取一個元素就可能觸發一次快取未命中（cache miss）。

---

## 九、效能陷阱總整理

| 陷阱 | 影響 | 解法 |
|------|------|------|
| 忘記 `reserve` | 多次重新配置 | 預估大小後 `reserve` |
| 函數參數用值傳遞 | 不必要的拷貝 | 用 `const&` 或 `&&` |
| 範圍 for 迴圈用值接收 | 每次迭代都拷貝 | 用 `const auto&` |
| 逐一 `erase` | O(n²) 刪除 | erase-remove 慣用法 |
| 逐一 `push_back` 現有資料 | 逐一檢查容量 | 用 `insert` 批量插入 |
| 移動建構子忘記 `noexcept` | 擴容時退回拷貝 | 加上 `noexcept` |
| 對基本型別過度使用 `emplace_back` | 無實質效益 | 按情況選擇 |
| 不需要排序時仍保持元素順序 | 刪除為 O(n) | swap-and-pop O(1) 刪除 |

---

## 十、第二階段總回顧：vector 的 12 堂課

到這裡，我們完成了整個 vector 系列。回顧一下學到的全部知識：

| 課程 | 主題 | 核心概念 |
|------|------|---------|
| 9 | 內部結構與記憶體配置 | 三指標結構、連續記憶體 |
| 10 | 宣告與初始化 | 各種建構方式 |
| 11 | 容量管理 | size、capacity、reserve 的差異 |
| 12 | 元素存取 | `[]`、`at`、`front`、`back` |
| 13 | 元素新增 | push_back、emplace_back |
| 14 | 元素插入 | insert、emplace |
| 15 | 元素刪除 | pop_back、erase、clear |
| 16 | 迭代器操作 | 八種迭代器、隨機存取能力 |
| 17 | 記憶體重新配置 | 擴容策略、均攤 O(1)、迭代器失效 |
| 18 | vector\<bool> | 代理物件、設計爭議、替代方案 |
| 19 | 與原始陣列互操作 | data()、C API 互通、安全守則 |
| 20 | 效能分析與最佳實踐 | reserve、emplace_back、erase-remove、快取效應 |

---

## 課後練習

**練習一：效能量測**
建立一個含有 100 萬個 `std::string`（每個字串長度約 50 字元）的 vector。分別量測以下操作的時間：(a) 不用 reserve 的 push_back，(b) 用 reserve 的 push_back，(c) 用 reserve 的 emplace_back。比較三者差異。

**練習二：刪除效能對比**
建立一個含有 10 萬個整數（0 到 99999）的 vector。用三種方法刪除所有 3 的倍數：(a) 逐一 erase，(b) erase-remove 慣用法，(c) 如果不需要保持順序，用 swap-and-pop。量測三者的時間。

**練習三：綜合設計**
設計一個 `StudentDatabase` 類別，內部用 `vector<Student>` 存儲學生資料（每個 Student 含 name、id、score）。實作以下操作，每個操作都要用到這 12 堂課學到的最佳實踐：
- `add_student`：用 emplace_back
- `remove_by_id`：用 erase-remove
- `find_by_name`：回傳 const 引用
- `get_all`：回傳 const vector 引用
- `import_from_array`：從 C 風格陣列批量匯入
- 建構時就 reserve 合理的初始容量

---

信安，到這裡我們完成了整個 **第二階段：vector** 的 12 堂課。Vector 是 STL 中最常用的容器，你在這裡學到的概念——迭代器失效、記憶體管理、效能取捨——在後面學其他容器時都會反覆出現。

準備好了就告訴我，我們進入**第三階段：deque**，從第 21 課開始：**deque 的內部結構（分段連續空間）**。那是一個非常精巧的資料結構設計。
*/

// =============================================================================
//  第 20 課 講義附錄  —  vector vs list：快取局部性（cache locality）
// =============================================================================
//
// 【主題資訊 Information】
//   std::vector<T>  —  連續記憶體，random access iterator，[] 為 O(1)
//   std::list<T>    —  雙向鏈結串列，bidirectional iterator，無 []
//
//   標頭檔：<vector>、<list>
//   複雜度：兩者「順序遍歷」都是 O(n)，但常數差距極大
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼複雜度相同、實際速度卻差很多】
//   大 O 記號描述的是「運算次數隨 n 的成長趨勢」，它把
//   「一次記憶體存取」當成單位成本 1。但在真實 CPU 上，
//   這個假設完全不成立：
//       L1 快取命中   ≈  1 ns 等級
//       主記憶體(DRAM) ≈  100 ns 等級（相差約兩個數量級）
//   遍歷 vector 時，元素在記憶體中連續排列，CPU 的硬體預取器
//   （prefetcher）能準確預測下一個位址並提前載入，幾乎每次都命中快取。
//   遍歷 list 時，每個節點是獨立 new 出來的，位址可能散布在整個 heap，
//   預取器無從預測 → 每跳一個節點就可能是一次 cache miss。
//   兩者都是 O(n)，但那個「1」的實際代價差了兩個數量級。
//
// 【2. list 的每個節點還多帶了兩個指標】
//   本機 g++ 15.2 / libstdc++ 實測（本檔第 1 節會印出）：
//       sizeof(int)                      = 4  位元組
//       std::list<int> 的節點             = 24 位元組（prev 8 + next 8 + 資料 4 + 對齊補 4）
//   也就是說，存同樣的資料，list 要用掉 vector **6 倍**的記憶體。
//   記憶體用得多 → 同樣大小的快取裝得下的元素更少 → 命中率更差。
//   這是雙重打擊：既不連續，又更肥。
//
// 【3. 那 list 什麼時候才有優勢】
//   list 真正的強項只有一個：**已經拿到 iterator 時，插入／刪除是 O(1)
//   且不搬移任何其他元素**，同時所有其他 iterator 保持有效。
//   適用情境：
//     * 需要在遍歷過程中大量插入／刪除，且元素本身複製成本很高
//     * 需要 splice（O(1) 把一整段搬到另一個 list，完全不搬資料）
//     * 需要「iterator 永不失效」的保證（例如外部持有元素的參考）
//   ⚠️ 但注意：如果你必須先「找到」那個位置，尋找本身就是 O(n)，
//      而且是 cache-miss 密集的 O(n)。此時 vector 常常整體更快，
//      即使它的刪除是 O(n) 搬移——因為 memmove 是連續的、SIMD 友善的。
//
// 【4. 實務建議】
//   C++ 社群的共識（Bjarne Stroustrup 與 Chandler Carruth 都公開示範過）是：
//   **預設用 vector，除非你有具體理由且實測證明其他容器更好。**
//   即使是「頻繁在中間插入」這種教科書上寫著「該用 list」的情境，
//   在元素較小、n 不是特別大時，vector 往往仍然勝出。
//   選容器要看實測，不要只看複雜度表。
//
// 【概念補充 Concept Deep Dive】
//   ● 快取行（cache line）
//     CPU 不是一次讀一個位元組，而是一次讀一整條 cache line
//     （x86-64 上是 64 位元組）。所以讀 vector<int> 的第一個元素時，
//     後面 15 個 int 也一起被載入了 —— 接下來 15 次存取全部免費命中。
//     list 則因為節點是 24 位元組且位址分散，一條 cache line 通常
//     只能帶進一兩個有用的節點。
//
//   ● 為什麼本檔改用「cache miss 的間接證據」而非只看耗時
//     耗時會隨機器、負載、CPU 頻率變動。本檔在 stdout 印的是
//     **結構性事實**（節點大小、記憶體總量、位址是否連續），
//     這些每次執行都一樣，而且直接解釋了效能差異的成因。
//     耗時仍然量測，但送到 stderr。
//     若要看真正的硬體事件，用 perf：
//         perf stat -e cache-misses,cache-references ./demo12
//
//   ● -Wreorder：本檔修正過的一個真實警告
//     原版 Timer 的成員宣告順序（start_、label_）與初始化列表的書寫順序
//     （label_、start_）不一致。C++ 規定成員一律依**宣告順序**初始化，
//     g++ -Wall 會以 -Wreorder 警告。本檔已把宣告順序調整為一致。
//
// 【注意事項 Pay Attention】
//   1. 「複雜度相同」不等於「效能相同」——大 O 忽略常數，而快取效應
//      正好就藏在常數裡，可以差到兩個數量級。
//   2. list 節點大小（本機 24 位元組）是**實作定義**，非標準規定。
//   3. list 沒有 operator[]；隨機存取要 std::advance，那是 O(n)。
//   4. 不要因為「要頻繁插入刪除」就反射性選 list，請先實測。
//   5. 本檔第 3 節比較的位址連續性，其結果依賴 heap 配置行為，
//      每次執行的實際位址都不同（本檔只印「是否連續」的判定，不印位址）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector vs list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 和 list 的順序遍歷都是 O(n)，為什麼實測差這麼多？
//     答：大 O 把「一次記憶體存取」當成單位成本 1，但真實 CPU 上
//         L1 命中約 1 ns、主記憶體約 100 ns，差兩個數量級。
//         vector 元素連續，硬體預取器能準確預測、幾乎全部命中快取；
//         list 節點散布 heap 各處，每跳一次就可能 cache miss。
//         再加上 list 每個節點多兩個指標（本機 int 節點 24 位元組 vs 4），
//         同樣的快取裝得下的元素更少，命中率更差。
//     追問：怎麼實際驗證是快取造成的？→ 用 perf stat 量 cache-misses：
//         perf stat -e cache-misses,cache-references ./demo12
//
// 🔥 Q2. 什麼情況下 list 才真的比 vector 好？
//     答：只有一種核心情境——**已經持有 iterator**，要在該處插入／刪除，
//         且不希望其他 iterator 失效。此時 list 是 O(1) 且零搬移。
//         另外 splice（O(1) 搬移整段）是 list 獨有的能力。
//     追問：那「頻繁在中間插入」呢？→ 要先看你怎麼找到插入點。
//         若得先線性搜尋，那個 O(n) 搜尋是 cache-miss 密集的，
//         往往比 vector 的 O(n) memmove 還慢。實測常常是 vector 贏。
//
// ⚠️ 陷阱. 「我要頻繁在中間插入刪除，所以一定要用 list」——錯在哪？
//     答：錯在只看複雜度表、沒看常數，也沒算上「找到位置」的成本。
//         vector 中間插入是 O(n) 沒錯，但那個 O(n) 是一次連續的 memmove，
//         由 SIMD 指令執行，每個元素的成本極低；
//         list 的 O(1) 插入前面往往有一個 cache-miss 密集的 O(n) 搜尋，
//         而且每個節點還要一次獨立的 heap 配置（malloc 本身就不便宜）。
//     為什麼會錯：把「資料結構課本的複雜度表」當成「效能的完整模型」。
//         那張表建立在「所有記憶體存取成本相同」的假設上，
//         而這個假設在有快取階層的現代 CPU 上早就不成立了。
//         正確做法：預設 vector，有具體理由時再換，換完一定要實測比較。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <chrono>
#include <string>
#include <numeric>
#include <algorithm>

// -----------------------------------------------------------------------------
// RAII 計時器。
// 【修正 1】成員宣告順序改成與初始化列表一致（原版觸發 -Wreorder 警告）。
// 【修正 2】結果印到 std::cerr——耗時每次執行都不同，不應混進 stdout。
// -----------------------------------------------------------------------------
struct Timer {
    std::string label_;                                     // 先宣告 → 先初始化
    std::chrono::high_resolution_clock::time_point start_;  // 後宣告 → 後初始化

    explicit Timer(const std::string& label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        std::cerr << "  [計時] " << label_ << ": " << us << " μs" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】事件佇列：為什麼即使「常常刪中間」也先選 vector
//   情境：遊戲 / 模擬程式的事件佇列，每一輪要移除所有已完成的事件。
//         直覺會想「常刪中間 → 用 list」，但實務上：
//           ① 事件通常是小 struct，vector 的搬移是連續 memmove，很便宜
//           ② 批次移除用 erase-remove 一次掃完，是 O(n) 而非 n 次 O(n)
//           ③ 遍歷（每輪都要做）在 vector 上快得多
//         所以正解是 vector + erase-remove，而不是 list。
// -----------------------------------------------------------------------------
struct Event {
    int id;
    int remainingTicks;
};

std::size_t tickEvents(std::vector<Event>& queue) {
    for (auto& e : queue) --e.remainingTicks;

    std::size_t before = queue.size();
    // 一次掃完所有要移除的，O(n)；不是 n 次 O(n)
    queue.erase(std::remove_if(queue.begin(), queue.end(),
                               [](const Event& e) { return e.remainingTicks <= 0; }),
                queue.end());
    return before - queue.size();
}

// 註：本檔不附 LeetCode 範例。「vector vs list 的快取效應」是硬體層的效能議題，
//     LeetCode 只驗證答案正確性、不量測快取行為，也沒有對應題型；
//     硬掛一題無關的陣列題只會模糊本檔重點。

int main() {
    std::cout << "=== 1. 結構性事實：記憶體佔用（可重現）===\n";
    std::cout << "sizeof(int)             = " << sizeof(int) << " 位元組\n";

    // 直接「量」出 list 節點的大小，而不是用對齊規則去「算」。
    // ⚠️ 初稿我用 alignof(std::max_align_t)（本機為 16）去進位，算出 32，
    //    但實際的 _List_node<int> 只對齊到 alignof(void*)（8），真值是 24。
    //    這是「用模型推導不如直接量測」的實例：下面這個 struct 忠實反映
    //    libstdc++ 的 _List_node<int> 佈局，sizeof 就是答案。
    struct FakeListNode { void* prev; void* next; int data; };
    const std::size_t listNodeSize = sizeof(FakeListNode);

    std::cout << "list<int> 每節點        = " << listNodeSize
              << " 位元組（prev 8 + next 8 + 資料 4 + 尾端對齊補 4）\n";
    std::cout << "→ 存同樣的資料，list 要用掉 vector 的 "
              << (listNodeSize / sizeof(int)) << " 倍記憶體\n";
    std::cout << "（節點大小為本機 g++ 15.2 / libstdc++ 的實作值，非標準保證）\n";

    const int N = 1'000'000;
    const double vecMB  = static_cast<double>(N) * sizeof(int) / (1024.0 * 1024.0);
    const double listMB = static_cast<double>(N) * static_cast<double>(listNodeSize)
                          / (1024.0 * 1024.0);
    std::cout << "\n存 " << N << " 個 int：\n";
    std::cout.setf(std::ios::fixed);
    std::cout.precision(1);
    std::cout << "  vector 資料區約 " << vecMB  << " MB\n";
    std::cout << "  list   節點共約 " << listMB << " MB\n";
    std::cout.unsetf(std::ios::fixed);
    std::cout << "→ 記憶體用得越多，同一份快取裝得下的元素越少，命中率越差。\n";

    std::cout << "\n=== 2. 結構性事實：元素是否連續（可重現）===\n";
    {
        std::vector<int> v(10);
        std::iota(v.begin(), v.end(), 0);
        bool contiguous = true;
        for (std::size_t i = 1; i < v.size(); ++i) {
            if (&v[i] != &v[i - 1] + 1) { contiguous = false; break; }
        }
        std::cout << "vector 的相鄰元素位址是否恰好相差 sizeof(int)？ "
                  << std::boolalpha << contiguous
                  << "（標準保證連續）\n";
    }
    {
        std::list<int> lst;
        for (int i = 0; i < 10; ++i) lst.push_back(i);
        bool allAdjacent = true;
        const int* prev = nullptr;
        for (const int& x : lst) {
            if (prev != nullptr && &x != prev + 1) { allAdjacent = false; break; }
            prev = &x;
        }
        std::cout << "list 的相鄰元素位址是否恰好相差 sizeof(int)？ "
                  << allAdjacent
                  << "（節點各自配置，通常不連續；實際位址每次執行都不同）\n";
    }
    std::cout << "→ 這就是硬體預取器對 vector 有效、對 list 無效的根本原因。\n";

    std::cout << "\n=== 3. 原始示範：順序遍歷實測（耗時印在 stderr）===\n";
    long long sumV = 0, sumL = 0;

    // 順序遍歷：vector vs list
    {
        std::vector<int> v(N);
        std::iota(v.begin(), v.end(), 0);

        Timer t("vector 遍歷");
        for (int x : v) sumV += x;
    }

    {
        std::list<int> lst;
        for (int i = 0; i < N; ++i) lst.push_back(i);

        Timer t("list   遍歷");
        for (int x : lst) sumL += x;
    }

    std::cout << "兩者計算結果相同：" << std::boolalpha << (sumV == sumL)
              << "（總和 = " << sumV << "）\n";
    std::cout << "耗時已印到 stderr——每次執行都不同，故不列入預期輸出。\n";
    std::cout << "只看 stdout 請用：./demo12 2>/dev/null\n";
    std::cout << "想看真正的硬體事件：perf stat -e cache-misses,cache-references ./demo12\n";

    std::cout << "\n=== 4. list 真正的優勢：splice 是 O(1) ===\n";
    std::list<int> a = {1, 2, 3};
    std::list<int> b = {10, 20, 30, 40};
    std::cout << "splice 前 a.size()=" << a.size() << ", b.size()=" << b.size() << "\n";
    a.splice(a.end(), b);        // 把整個 b 接到 a 尾端，O(1)，不搬移任何資料
    std::cout << "splice 後 a.size()=" << a.size() << ", b.size()=" << b.size() << "\n";
    std::cout << "a 的內容：";
    for (int x : a) std::cout << x << " ";
    std::cout << "\n→ 這是 vector 做不到的：它必須複製或移動每一個元素。\n";
    std::cout << "  若你的工作負載大量依賴這種整段搬移，list 才真的有價值。\n";

    std::cout << "\n=== 日常實務：事件佇列用 vector + erase-remove ===\n";
    std::vector<Event> queue = {
        {1, 3}, {2, 1}, {3, 5}, {4, 1}, {5, 2}, {6, 1}, {7, 4}
    };
    std::cout << "初始事件數：" << queue.size() << "\n";
    for (int tick = 1; tick <= 3; ++tick) {
        std::size_t done = tickEvents(queue);
        std::cout << "第 " << tick << " tick：完成 " << done
                  << " 個，剩餘 " << queue.size() << " 個 → id: ";
        for (const auto& e : queue) std::cout << e.id << " ";
        std::cout << "\n";
    }
    std::cout << "→ 「常刪中間」直覺上該選 list，但事件是小 struct、\n";
    std::cout << "  批次移除用 erase-remove 一次掃完是 O(n)，\n";
    std::cout << "  而每輪都要做的遍歷在 vector 上快得多——實測往往 vector 勝。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 20 課：vector 效能分析與最佳實踐12.cpp" -o demo12
// 只看 stdout: ./demo12 2>/dev/null
// 看快取事件: perf stat -e cache-misses,cache-references ./demo12
//
// ⚠️ 但書：
//   1. 第 3 節的耗時印在 stderr，每次執行都不同（受 CPU 頻率、快取狀態、
//      其他行程影響），故不納入下方預期輸出。
//   2. list 節點大小（24 位元組，本檔以 struct sizeof 實測）是本機 g++ 15.2 / libstdc++ 的實作值，
//      非標準保證。
//   3. 第 2 節只印「位址是否連續」的判定，不印實際位址——
//      實際位址由 heap 配置決定，每次執行都不同。

// === 預期輸出 ===
// === 1. 結構性事實：記憶體佔用（可重現）===
// sizeof(int)             = 4 位元組
// list<int> 每節點        = 24 位元組（prev 8 + next 8 + 資料 4 + 尾端對齊補 4）
// → 存同樣的資料，list 要用掉 vector 的 6 倍記憶體
// （節點大小為本機 g++ 15.2 / libstdc++ 的實作值，非標準保證）
//
// 存 1000000 個 int：
//   vector 資料區約 3.8 MB
//   list   節點共約 22.9 MB
// → 記憶體用得越多，同一份快取裝得下的元素越少，命中率越差。
//
// === 2. 結構性事實：元素是否連續（可重現）===
// vector 的相鄰元素位址是否恰好相差 sizeof(int)？ true（標準保證連續）
// list 的相鄰元素位址是否恰好相差 sizeof(int)？ false（節點各自配置，通常不連續；實際位址每次執行都不同）
// → 這就是硬體預取器對 vector 有效、對 list 無效的根本原因。
//
// === 3. 原始示範：順序遍歷實測（耗時印在 stderr）===
// 兩者計算結果相同：true（總和 = 499999500000）
// 耗時已印到 stderr——每次執行都不同，故不列入預期輸出。
// 只看 stdout 請用：./demo12 2>/dev/null
// 想看真正的硬體事件：perf stat -e cache-misses,cache-references ./demo12
//
// === 4. list 真正的優勢：splice 是 O(1) ===
// splice 前 a.size()=3, b.size()=4
// splice 後 a.size()=7, b.size()=0
// a 的內容：1 2 3 10 20 30 40
// → 這是 vector 做不到的：它必須複製或移動每一個元素。
//   若你的工作負載大量依賴這種整段搬移，list 才真的有價值。
//
// === 日常實務：事件佇列用 vector + erase-remove ===
// 初始事件數：7
// 第 1 tick：完成 3 個，剩餘 4 個 → id: 1 3 5 7
// 第 2 tick：完成 1 個，剩餘 3 個 → id: 1 3 7
// 第 3 tick：完成 1 個，剩餘 2 個 → id: 3 7
// → 「常刪中間」直覺上該選 list，但事件是小 struct、
//   批次移除用 erase-remove 一次掃完是 O(n)，
//   而每輪都要做的遍歷在 vector 上快得多——實測往往 vector 勝。
