/*
# 第二階段：序列容器 — vector

## 第 13 課：vector 元素新增：push_back、emplace_back

---

### 核心概念

在 vector 尾端新增元素是最常見的操作。C++11 之後有兩種主要方式：`push_back` 和 `emplace_back`。理解它們的差異，能幫助你寫出更高效的程式碼。

---

### 一、push_back：複製或移動元素進入容器

```cpp
#include <vector>
#include <iostream>
#include <string>

int main() {
    std::vector<int> v;
    
    // 基本用法
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    
    for (int x : v) {
        std::cout << x << " ";  // 10 20 30
    }
    std::cout << std::endl;
    
    // 對於字串
    std::vector<std::string> words;
    
    std::string hello = "Hello";
    words.push_back(hello);           // 複製 hello 進入 vector
    words.push_back("World");         // 從字面量建立臨時物件，然後移動進入
    words.push_back(std::move(hello)); // 移動 hello 進入 vector
    
    std::cout << "hello 變成: \"" << hello << "\"" << std::endl;  // 可能是空的
    
    for (const auto& w : words) {
        std::cout << w << " ";  // Hello World Hello
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### 二、觀察 push_back 的複製與移動

```cpp
#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    
    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }
    
    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(5);  // 預留空間，避免擴容干擾觀察
    
    std::cout << "=== 從左值 push_back ===" << std::endl;
    Tracker t1(1);
    v.push_back(t1);  // 複製
    
    std::cout << "\n=== 從右值 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構臨時物件，然後移動
    
    std::cout << "\n=== 用 std::move push_back ===" << std::endl;
    Tracker t3(3);
    v.push_back(std::move(t3));  // 移動
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
```

輸出：
```
=== 從左值 push_back ===
建構 Tracker(1)
複製建構 Tracker(1)

=== 從右值 push_back ===
建構 Tracker(2)
移動建構 Tracker(2)
銷毀 Tracker(2)

=== 用 std::move push_back ===
建構 Tracker(3)
移動建構 Tracker(3)

=== 程式結束 ===
銷毀 Tracker(3)
銷毀 Tracker(1)
銷毀 Tracker(3)
銷毀 Tracker(2)
銷毀 Tracker(1)
```

---

### 三、emplace_back：就地建構

`emplace_back` 直接在 vector 的記憶體空間內建構物件，**不需要先建立再複製/移動**：

```cpp
#include <vector>
#include <iostream>

class Tracker {
public:
    int id;
    
    Tracker(int i) : id(i) {
        std::cout << "建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(const Tracker& other) : id(other.id) {
        std::cout << "複製建構 Tracker(" << id << ")" << std::endl;
    }
    
    Tracker(Tracker&& other) noexcept : id(other.id) {
        std::cout << "移動建構 Tracker(" << id << ")" << std::endl;
    }
    
    ~Tracker() {
        std::cout << "銷毀 Tracker(" << id << ")" << std::endl;
    }
};

int main() {
    std::vector<Tracker> v;
    v.reserve(3);
    
    std::cout << "=== emplace_back ===" << std::endl;
    v.emplace_back(1);  // 直接在 vector 內部建構，只有一次建構
    
    std::cout << "\n=== 對比 push_back ===" << std::endl;
    v.push_back(Tracker(2));  // 建構 + 移動 + 銷毀臨時物件
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
```

輸出：
```
=== emplace_back ===
建構 Tracker(1)

=== 對比 push_back ===
建構 Tracker(2)
移動建構 Tracker(2)
銷毀 Tracker(2)

=== 程式結束 ===
銷毀 Tracker(2)
銷毀 Tracker(1)
```

可以看到 `emplace_back` 只有一次建構，沒有額外的移動和銷毀。

---

### 四、多參數建構的優勢

`emplace_back` 可以接受任意數量的參數，直接傳給元素的建構子：

```cpp
#include <vector>
#include <iostream>
#include <string>

struct Person {
    std::string name;
    int age;
    double height;
    
    Person(const std::string& n, int a, double h)
        : name(n), age(a), height(h) {
        std::cout << "建構 Person: " << name << std::endl;
    }
    
    Person(const Person& other)
        : name(other.name), age(other.age), height(other.height) {
        std::cout << "複製 Person: " << name << std::endl;
    }
    
    Person(Person&& other) noexcept
        : name(std::move(other.name)), age(other.age), height(other.height) {
        std::cout << "移動 Person: " << name << std::endl;
    }
};

int main() {
    std::vector<Person> people;
    people.reserve(3);
    
    std::cout << "=== 使用 emplace_back ===" << std::endl;
    people.emplace_back("Alice", 30, 165.5);  // 直接建構
    
    std::cout << "\n=== 使用 push_back ===" << std::endl;
    people.push_back(Person("Bob", 25, 175.0));  // 建構 + 移動
    
    std::cout << "\n=== 使用大括號 push_back ===" << std::endl;
    people.push_back({"Charlie", 35, 180.0});  // 建構 + 移動
    
    std::cout << "\n=== 結束 ===" << std::endl;
    return 0;
}
```

---

### 五、C++17 的回傳值改進

C++17 之後，`emplace_back` 會回傳新元素的參考：

```cpp
#include <vector>
#include <iostream>
#include <string>

struct Item {
    std::string name;
    int quantity;
    
    Item(const std::string& n, int q) : name(n), quantity(q) {}
};

int main() {
    std::vector<Item> items;
    items.reserve(3);
    
    // C++17：emplace_back 回傳新元素的參考
    Item& apple = items.emplace_back("Apple", 10);
    apple.quantity += 5;  // 直接修改
    
    // 鏈式操作
    items.emplace_back("Banana", 20).quantity *= 2;
    
    for (const auto& item : items) {
        std::cout << item.name << ": " << item.quantity << std::endl;
    }
    // Apple: 15
    // Banana: 40
    
    return 0;
}
```

**注意**：`push_back` 沒有回傳值（回傳 void）。

---

### 六、什麼時候用哪個？

```cpp
#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(10);
    
    // 情況 1：已有物件，需要保留原物件
    std::string s1 = "Hello";
    v.push_back(s1);  // 複製，s1 仍然有效
    // 這種情況 emplace_back(s1) 效果相同
    
    // 情況 2：已有物件，不再需要原物件
    std::string s2 = "World";
    v.push_back(std::move(s2));  // 移動
    // 這種情況 emplace_back(std::move(s2)) 效果相同
    
    // 情況 3：從字面量或臨時值
    v.push_back("Foo");     // 建構臨時 string，然後移動
    v.emplace_back("Bar");  // 直接就地建構，較優
    
    // 情況 4：需要多個參數建構
    // push_back 需要先建構物件
    // emplace_back 可以直接傳參數
    
    return 0;
}
```

---

### 七、emplace_back 的陷阱

#### 陷阱一：隱式轉換可能不如預期

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<std::vector<int>> vv;
    
    // 想要加入一個含有 5 個元素的 vector
    // 錯誤理解：emplace_back(5) 會建立 vector<int>(5)，即 5 個 0
    vv.emplace_back(5);
    
    std::cout << "內層 size: " << vv[0].size() << std::endl;  // 5
    std::cout << "內層內容: ";
    for (int x : vv[0]) std::cout << x << " ";  // 0 0 0 0 0
    std::cout << std::endl;
    
    // 如果想要一個元素是 5 的 vector？
    vv.push_back({5});  // 這樣才對
    
    std::cout << "第二個內層 size: " << vv[1].size() << std::endl;  // 1
    std::cout << "第二個內層內容: ";
    for (int x : vv[1]) std::cout << x << " ";  // 5
    std::cout << std::endl;
    
    return 0;
}
```

#### 陷阱二：emplace_back 不一定更快

```cpp
#include <vector>
#include <string>

int main() {
    std::vector<std::string> v;
    v.reserve(10);
    
    std::string s = "Hello";
    
    // 這兩個效能幾乎相同
    v.push_back(s);     // 複製
    v.emplace_back(s);  // 也是複製！因為 s 是左值
    
    // 這兩個效能幾乎相同
    v.push_back(std::move(s));     // 移動
    v.emplace_back(std::move(s));  // 也是移動
    
    return 0;
}
```

當傳入的是**已存在的物件**時，兩者效能相同。`emplace_back` 的優勢在於可以**省略臨時物件的建構**。

---

### 八、完美轉發的運作原理（進階）

`emplace_back` 使用完美轉發（perfect forwarding）將參數傳給建構子：

```cpp
#include <vector>
#include <iostream>
#include <string>

struct Widget {
    std::string data;
    
    Widget(const std::string& s) : data(s) {
        std::cout << "從 const string& 建構" << std::endl;
    }
    
    Widget(std::string&& s) : data(std::move(s)) {
        std::cout << "從 string&& 建構" << std::endl;
    }
};

int main() {
    std::vector<Widget> v;
    v.reserve(5);
    
    std::string s = "Hello";
    
    std::cout << "emplace_back(s):" << std::endl;
    v.emplace_back(s);  // 呼叫 const string& 版本
    
    std::cout << "\nemplace_back(std::move(s)):" << std::endl;
    v.emplace_back(std::move(s));  // 呼叫 string&& 版本
    
    std::cout << "\nemplace_back(\"World\"):" << std::endl;
    v.emplace_back("World");  // 呼叫 string&& 版本（從字面量建構臨時 string）
    
    return 0;
}
```

---

### 九、例外安全性

`push_back` 和 `emplace_back` 都提供強例外保證（strong exception guarantee）：如果操作失敗，vector 保持原狀。

```cpp
#include <vector>
#include <iostream>
#include <stdexcept>

struct MayThrow {
    int value;
    static int count;
    
    MayThrow(int v) : value(v) {
        ++count;
        if (count == 3) {
            throw std::runtime_error("第三個物件建構失敗");
        }
        std::cout << "建構 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(const MayThrow& other) : value(other.value) {
        std::cout << "複製 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(MayThrow&& other) noexcept : value(other.value) {
        std::cout << "移動 MayThrow(" << value << ")" << std::endl;
    }
};

int MayThrow::count = 0;

int main() {
    std::vector<MayThrow> v;
    v.reserve(5);
    
    try {
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);  // 這裡會拋出例外
        v.emplace_back(4);
    }
    catch (const std::exception& e) {
        std::cout << "捕捉例外: " << e.what() << std::endl;
    }
    
    std::cout << "vector 大小: " << v.size() << std::endl;  // 2
    
    return 0;
}
```

---

### push_back vs emplace_back 對照表

| 特性 | push_back | emplace_back |
|------|-----------|--------------|
| 參數 | 接受物件 | 接受建構子參數 |
| 運作方式 | 複製或移動物件進入 | 就地建構物件 |
| 適用場景 | 已有物件要加入 | 需要建構新物件 |
| 回傳值 | void | T&（C++17） |
| 效能 | 左值複製，右值移動 | 避免臨時物件 |

---

### 使用建議

| 場景 | 建議 |
|------|------|
| 加入已存在的左值物件 | `push_back` 或 `emplace_back` 皆可 |
| 加入已存在物件且不再需要 | `push_back(std::move(obj))` |
| 從字面量或參數建構新物件 | `emplace_back(args...)` |
| 程式碼清晰度優先 | `push_back` 意圖更明確 |
| 效能關鍵路徑 | 優先考慮 `emplace_back` |

---

### 練習題

1. **預測題**：以下程式碼會輸出什麼？
   ```cpp
   std::vector<std::pair<int, int>> v;
   v.reserve(3);
   v.push_back({1, 2});
   v.emplace_back(3, 4);
   v.push_back(std::make_pair(5, 6));
   ```

2. **改寫題**：將以下程式碼改用 `emplace_back`，使效能最佳化：
   ```cpp
   struct Point { double x, y, z; Point(double a, double b, double c) : x(a), y(b), z(c) {} };
   std::vector<Point> points;
   points.push_back(Point(1.0, 2.0, 3.0));
   points.push_back(Point(4.0, 5.0, 6.0));
   ```

3. **思考題**：為什麼 `emplace_back` 回傳參考（C++17），而 `push_back` 回傳 void？加入回傳值有什麼潛在的風險？

4. **除錯題**：以下程式碼有什麼潛在問題？
   ```cpp
   std::vector<std::unique_ptr<int>> v;
   auto p = std::make_unique<int>(42);
   v.push_back(p);
   ```

---

下一課我們講 **vector 元素插入：insert、emplace**，學習在任意位置插入元素的方法。

準備好繼續嗎？
*/



#include <vector>
#include <iostream>
#include <stdexcept>

struct MayThrow {
    int value;
    static int count;
    
    MayThrow(int v) : value(v) {
        ++count;
        if (count == 3) {
            throw std::runtime_error("第三個物件建構失敗");
        }
        std::cout << "建構 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(const MayThrow& other) : value(other.value) {
        std::cout << "複製 MayThrow(" << value << ")" << std::endl;
    }
    
    MayThrow(MayThrow&& other) noexcept : value(other.value) {
        std::cout << "移動 MayThrow(" << value << ")" << std::endl;
    }
};

int MayThrow::count = 0;

int main() {
    std::vector<MayThrow> v;
    v.reserve(5);
    
    try {
        v.emplace_back(1);
        v.emplace_back(2);
        v.emplace_back(3);  // 這裡會拋出例外
        v.emplace_back(4);
    }
    catch (const std::exception& e) {
        std::cout << "捕捉例外: " << e.what() << std::endl;
    }
    
    std::cout << "vector 大小: " << v.size() << std::endl;  // 2
    
    return 0;
}
