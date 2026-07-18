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
