/*
# 第 4 課：命名空間（namespace）基礎

---

## 4.1 課程目標

當程式規模變大，或使用多個函式庫時，很容易遇到**名稱衝突**的問題——不同的函式庫可能定義了相同名稱的函數或類別。C++ 的**命名空間（namespace）**機制就是為了解決這個問題而設計的。

本課將學習命名空間的概念、語法，以及如何正確使用它。

---

## 4.2 為什麼需要命名空間？

### 4.2.1 問題情境

假設你在開發一個遊戲，使用了兩個第三方函式庫：

- **圖形庫**：提供一個 `draw()` 函數來繪製圖形
- **日誌庫**：也提供一個 `draw()` 函數來「抽取」日誌記錄

當你同時使用這兩個函式庫時，編譯器無法知道你要呼叫哪一個 `draw()`！

### 4.2.2 C 語言的解決方案（前綴命名）

在 C 語言中，通常用**前綴**來避免衝突：

```c
// 圖形庫的函數
void graphics_draw();
void graphics_clear();
void graphics_init();

// 日誌庫的函數
void log_draw();
void log_write();
void log_init();
```

這種方式雖然有效，但名稱變得很長，而且容易打錯。

### 4.2.3 C++ 的解決方案（命名空間）

C++ 使用命名空間來組織程式碼：

```cpp
namespace graphics {
    void draw();
    void clear();
    void init();
}

namespace log {
    void draw();
    void write();
    void init();
}

// 使用時明確指定
graphics::draw();  // 呼叫圖形庫的 draw
log::draw();       // 呼叫日誌庫的 draw
```

---

## 4.3 命名空間的基本語法

### 4.3.1 定義命名空間

```cpp
namespace 命名空間名稱 {
    // 變數、函數、類別等
}
```

### 4.3.2 簡單範例

```cpp
#include <iostream>

// 定義命名空間 math_utils
namespace math_utils {
    const double PI = 3.14159265358979;
    
    int add(int a, int b) {
        return a + b;
    }
    
    int square(int x) {
        return x * x;
    }
}

int main() {
    // 使用命名空間中的成員，需要加上 命名空間:: 前綴
    std::cout << "PI = " << math_utils::PI << std::endl;
    std::cout << "add(3, 5) = " << math_utils::add(3, 5) << std::endl;
    std::cout << "square(4) = " << math_utils::square(4) << std::endl;
    
    return 0;
}
```

### 輸出

```
PI = 3.14159
add(3, 5) = 8
square(4) = 16
```

---

## 4.4 存取命名空間成員的三種方式

### 4.4.1 方式一：完全限定名稱（推薦）

使用 `命名空間::成員` 的完整形式：

```cpp
std::cout << "Hello" << std::endl;
std::string name = "Alice";
std::vector<int> numbers;
```

**優點**：清楚明確，不會有歧義
**缺點**：程式碼較長

### 4.4.2 方式二：using 宣告（引入特定成員）

使用 `using` 引入特定的成員：

```cpp
#include <iostream>
#include <string>

using std::cout;    // 只引入 cout
using std::endl;    // 只引入 endl
using std::string;  // 只引入 string

int main() {
    cout << "Hello" << endl;  // 可以直接使用 cout 和 endl
    string name = "Alice";    // 可以直接使用 string
    
    // 其他成員仍需完整名稱
    std::cin >> name;
    
    return 0;
}
```

**優點**：只引入需要的成員，減少名稱衝突風險
**缺點**：需要逐一列出要引入的成員

### 4.4.3 方式三：using 指令（引入整個命名空間）

使用 `using namespace` 引入整個命名空間：

```cpp
#include <iostream>
#include <string>

using namespace std;  // 引入 std 的所有成員

int main() {
    cout << "Hello" << endl;
    string name = "Alice";
    cin >> name;
    
    return 0;
}
```

**優點**：程式碼最簡潔
**缺點**：可能導致名稱衝突，不推薦在大型專案中使用

---

## 4.5 using namespace 的風險

### 4.5.1 名稱衝突範例

```cpp
#include <iostream>
#include <algorithm>  // 包含 std::count

using namespace std;

// 自己定義的 count 函數
int count(int arr[], int size) {
    return size;
}

int main() {
    int arr[] = {1, 2, 3, 4, 5};
    
    // 這裡會產生歧義！
    // 是呼叫我們的 count，還是 std::count？
    // cout << count(arr, 5) << endl;  // 編譯錯誤
    
    return 0;
}
```

### 4.5.2 最佳實踐建議

| 情況 | 建議 |
|------|------|
| 標頭檔（.h / .hpp） | **絕對不要**使用 `using namespace` |
| 小型練習程式 | 可以使用 `using namespace std;` |
| 大型專案 | 使用完全限定名稱或 `using` 宣告 |
| 函數內部 | 可以使用 `using namespace`（作用域限制在函數內） |

### 4.5.3 安全的使用方式

```cpp
#include <iostream>
#include <vector>

// 不要在全域範圍使用 using namespace

void processData() {
    // 在函數內部使用是安全的，作用域限制在這個函數內
    using namespace std;
    
    vector<int> data = {1, 2, 3, 4, 5};
    for (int x : data) {
        cout << x << " ";
    }
    cout << endl;
}

int main() {
    // 這裡仍需使用完整名稱
    std::cout << "開始處理..." << std::endl;
    processData();
    std::cout << "處理完成" << std::endl;
    
    return 0;
}
```

---

## 4.6 標準函式庫的命名空間：std

C++ 標準函式庫的所有內容都定義在 `std` 命名空間中：

| 成員 | 說明 |
|------|------|
| `std::cout` | 標準輸出 |
| `std::cin` | 標準輸入 |
| `std::endl` | 換行並清空緩衝區 |
| `std::string` | 字串類別 |
| `std::vector` | 動態陣列類別 |
| `std::map` | 關聯容器 |
| `std::sort` | 排序演算法 |
| `std::move` | 移動語義函數 |

---

## 4.7 巢狀命名空間

命名空間可以嵌套：

### 4.7.1 傳統寫法

```cpp
namespace company {
    namespace project {
        namespace module {
            void doSomething() {
                // ...
            }
        }
    }
}

// 使用時
company::project::module::doSomething();
```

### 4.7.2 C++17 簡化寫法

```cpp
namespace company::project::module {
    void doSomething() {
        // ...
    }
}

// 使用方式相同
company::project::module::doSomething();
```

---

## 4.8 命名空間別名

當命名空間名稱很長時，可以建立別名：

```cpp
#include <iostream>

namespace very_long_namespace_name {
    void greet() {
        std::cout << "Hello from long namespace!" << std::endl;
    }
}

int main() {
    // 建立別名
    namespace vln = very_long_namespace_name;
    
    // 使用別名
    vln::greet();
    
    // 原名稱仍然有效
    very_long_namespace_name::greet();
    
    return 0;
}
```

實際應用範例：

```cpp
// 標準函式庫中常見的簡化
namespace fs = std::filesystem;  // C++17

// 使用
fs::path p = "/home/user/documents";
```

---

## 4.9 匿名命名空間

沒有名稱的命名空間稱為**匿名命名空間**，其成員只在當前檔案內可見：

```cpp
#include <iostream>

// 匿名命名空間
namespace {
    int internalCounter = 0;  // 只在這個檔案內可見
    
    void internalFunction() {
        std::cout << "這是內部函數" << std::endl;
    }
}

int main() {
    internalCounter = 10;  // 可以直接使用，不需前綴
    internalFunction();
    
    return 0;
}
```

匿名命名空間的作用類似於 C 語言中的 `static`，用於限制符號的連結範圍。

### 比較

| 方式 | C 語言 | C++ |
|------|--------|-----|
| 檔案內部函數 | `static void func()` | 放在匿名命名空間中 |
| 檔案內部變數 | `static int var` | 放在匿名命名空間中 |

---

## 4.10 擴展命名空間

同一個命名空間可以在多處定義，編譯器會將它們合併：

```cpp
#include <iostream>

// 第一部分
namespace mylib {
    void funcA() {
        std::cout << "Function A" << std::endl;
    }
}

// 第二部分（擴展同一個命名空間）
namespace mylib {
    void funcB() {
        std::cout << "Function B" << std::endl;
    }
}

// 這在多檔案專案中很常見：
// math.h 定義 namespace mylib { 數學函數 }
// string.h 定義 namespace mylib { 字串函數 }
// 它們都屬於同一個 mylib 命名空間

int main() {
    mylib::funcA();
    mylib::funcB();
    
    return 0;
}
```

---

## 4.11 完整範例程式

```cpp
#include <iostream>
#include <string>
#include <cmath>

// ============================================================
// 定義多個命名空間，模擬實際專案結構
// ============================================================

// 數學工具命名空間
namespace math_tools {
    const double PI = 3.14159265358979;
    const double E = 2.71828182845905;
    
    double circleArea(double radius) {
        return PI * radius * radius;
    }
    
    double circleCircumference(double radius) {
        return 2 * PI * radius;
    }
    
    int power(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// 字串工具命名空間
namespace string_tools {
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
        }
        return result;
    }
    
    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return result;
    }
    
    std::string repeat(const std::string& str, int times) {
        std::string result;
        for (int i = 0; i < times; i++) {
            result += str;
        }
        return result;
    }
}

// 巢狀命名空間（模擬公司專案結構）
namespace company {
    namespace graphics {
        void draw() {
            std::cout << "[Graphics] 繪製圖形" << std::endl;
        }
        
        void clear() {
            std::cout << "[Graphics] 清除畫面" << std::endl;
        }
    }
    
    namespace audio {
        void play() {
            std::cout << "[Audio] 播放音效" << std::endl;
        }
        
        void stop() {
            std::cout << "[Audio] 停止播放" << std::endl;
        }
    }
}

// 匿名命名空間（檔案內部使用）
namespace {
    int callCount = 0;
    
    void logCall(const std::string& funcName) {
        callCount++;
        std::cout << "  [Log] 第 " << callCount << " 次呼叫: " 
                  << funcName << std::endl;
    }
}

// ============================================================
// 主程式
// ============================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    命名空間（namespace）展示" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 1. 使用完全限定名稱
    // --------------------------------------------------
    std::cout << "[1] 完全限定名稱存取" << std::endl;
    std::cout << "  PI = " << math_tools::PI << std::endl;
    std::cout << "  E = " << math_tools::E << std::endl;
    std::cout << "  圓面積(r=5) = " << math_tools::circleArea(5) << std::endl;
    std::cout << "  2^10 = " << math_tools::power(2, 10) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 2. 使用 using 宣告（引入特定成員）
    // --------------------------------------------------
    std::cout << "[2] using 宣告（引入特定成員）" << std::endl;
    
    using string_tools::toUpperCase;
    using string_tools::toLowerCase;
    
    std::string text = "Hello World";
    std::cout << "  原始: " << text << std::endl;
    std::cout << "  大寫: " << toUpperCase(text) << std::endl;
    std::cout << "  小寫: " << toLowerCase(text) << std::endl;
    
    // repeat 沒有 using，仍需完整名稱
    std::cout << "  重複: " << string_tools::repeat("Hi", 3) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 3. 命名空間別名
    // --------------------------------------------------
    std::cout << "[3] 命名空間別名" << std::endl;
    
    namespace mt = math_tools;
    namespace st = string_tools;
    
    std::cout << "  mt::PI = " << mt::PI << std::endl;
    std::cout << "  st::toUpperCase(\"test\") = " 
              << st::toUpperCase("test") << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 4. 巢狀命名空間
    // --------------------------------------------------
    std::cout << "[4] 巢狀命名空間" << std::endl;
    
    company::graphics::draw();
    company::graphics::clear();
    company::audio::play();
    company::audio::stop();
    std::cout << std::endl;
    
    // 也可以使用別名簡化
    namespace gfx = company::graphics;
    namespace sfx = company::audio;
    
    std::cout << "  使用別名：" << std::endl;
    gfx::draw();
    sfx::play();
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 5. 匿名命名空間（內部使用）
    // --------------------------------------------------
    std::cout << "[5] 匿名命名空間（內部計數器）" << std::endl;
    
    logCall("function1");
    logCall("function2");
    logCall("function3");
    
    std::cout << "  總呼叫次數: " << callCount << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 6. 在區塊內使用 using namespace
    // --------------------------------------------------
    std::cout << "[6] 區塊內的 using namespace" << std::endl;
    
    {
        // 只在這個區塊內有效
        using namespace math_tools;
        using namespace string_tools;
        
        std::cout << "  在區塊內可以直接使用：" << std::endl;
        std::cout << "  PI = " << PI << std::endl;
        std::cout << "  circleArea(3) = " << circleArea(3) << std::endl;
        std::cout << "  toUpperCase(\"abc\") = " << toUpperCase("abc") << std::endl;
    }
    
    // 離開區塊後，又需要完整名稱
    std::cout << "  離開區塊後：math_tools::PI = " << math_tools::PI << std::endl;
    std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "    展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -Wall -Wextra lesson04.cpp -o lesson04
./lesson04
```

### 執行結果

```
========================================
    命名空間（namespace）展示
========================================

[1] 完全限定名稱存取
  PI = 3.14159
  E = 2.71828
  圓面積(r=5) = 78.5398
  2^10 = 1024

[2] using 宣告（引入特定成員）
  原始: Hello World
  大寫: HELLO WORLD
  小寫: hello world
  重複: HiHiHi

[3] 命名空間別名
  mt::PI = 3.14159
  st::toUpperCase("test") = TEST

[4] 巢狀命名空間
[Graphics] 繪製圖形
[Graphics] 清除畫面
[Audio] 播放音效
[Audio] 停止播放

  使用別名：
[Graphics] 繪製圖形
[Audio] 播放音效

[5] 匿名命名空間（內部計數器）
  [Log] 第 1 次呼叫: function1
  [Log] 第 2 次呼叫: function2
  [Log] 第 3 次呼叫: function3
  總呼叫次數: 3

[6] 區塊內的 using namespace
  在區塊內可以直接使用：
  PI = 3.14159
  circleArea(3) = 28.2743
  toUpperCase("abc") = ABC
  離開區塊後：math_tools::PI = 3.14159

========================================
    展示完成
========================================
```

---

## 4.12 本課重點回顧

| 概念 | 說明 |
|------|------|
| 命名空間目的 | 避免名稱衝突，組織程式碼 |
| 定義語法 | `namespace name { ... }` |
| 存取成員 | 使用 `::` 運算子，如 `std::cout` |
| using 宣告 | `using std::cout;` 引入特定成員 |
| using 指令 | `using namespace std;` 引入整個命名空間 |
| 命名空間別名 | `namespace fs = std::filesystem;` |
| 巢狀命名空間 | `namespace a::b::c { }` (C++17) |
| 匿名命名空間 | `namespace { }` 限制在檔案內部 |
| std 命名空間 | C++ 標準函式庫的命名空間 |

---

## 4.13 最佳實踐總結

1. **標頭檔中絕對不要使用 `using namespace`**
2. **優先使用完全限定名稱**（如 `std::cout`）
3. **若要簡化，使用 `using` 宣告引入特定成員**
4. **`using namespace` 只在小範圍內使用**（如函數內部）
5. **為長命名空間建立別名**
6. **使用匿名命名空間取代 `static` 限制檔案作用域**

---

## 4.14 下一課預告

下一課我們將學習 **輸入輸出流（iostream）**，深入了解 C++ 的 `cout`、`cin`、格式化輸出等功能，這是 C++ 程式設計的基本技能。

準備好進入 **第 5 課：輸入輸出流（iostream）入門** 了嗎？
*/



#include <iostream>
#include <string>
#include <cmath>

// ============================================================
// 定義多個命名空間，模擬實際專案結構
// ============================================================

// 數學工具命名空間
namespace math_tools {
    const double PI = 3.14159265358979;
    const double E = 2.71828182845905;
    
    double circleArea(double radius) {
        return PI * radius * radius;
    }
    
    double circleCircumference(double radius) {
        return 2 * PI * radius;
    }
    
    int power(int base, int exp) {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

// 字串工具命名空間
namespace string_tools {
    std::string toUpperCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
        }
        return result;
    }
    
    std::string toLowerCase(const std::string& str) {
        std::string result = str;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return result;
    }
    
    std::string repeat(const std::string& str, int times) {
        std::string result;
        for (int i = 0; i < times; i++) {
            result += str;
        }
        return result;
    }
}

// 巢狀命名空間（模擬公司專案結構）
namespace company {
    namespace graphics {
        void draw() {
            std::cout << "[Graphics] 繪製圖形" << std::endl;
        }
        
        void clear() {
            std::cout << "[Graphics] 清除畫面" << std::endl;
        }
    }
    
    namespace audio {
        void play() {
            std::cout << "[Audio] 播放音效" << std::endl;
        }
        
        void stop() {
            std::cout << "[Audio] 停止播放" << std::endl;
        }
    }
}

// 匿名命名空間（檔案內部使用）
namespace {
    int callCount = 0;
    
    void logCall(const std::string& funcName) {
        callCount++;
        std::cout << "  [Log] 第 " << callCount << " 次呼叫: " 
                  << funcName << std::endl;
    }
}

// ============================================================
// 主程式
// ============================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    命名空間（namespace）展示" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 1. 使用完全限定名稱
    // --------------------------------------------------
    std::cout << "[1] 完全限定名稱存取" << std::endl;
    std::cout << "  PI = " << math_tools::PI << std::endl;
    std::cout << "  E = " << math_tools::E << std::endl;
    std::cout << "  圓面積(r=5) = " << math_tools::circleArea(5) << std::endl;
    std::cout << "  2^10 = " << math_tools::power(2, 10) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 2. 使用 using 宣告（引入特定成員）
    // --------------------------------------------------
    std::cout << "[2] using 宣告（引入特定成員）" << std::endl;
    
    using string_tools::toUpperCase;
    using string_tools::toLowerCase;
    
    std::string text = "Hello World";
    std::cout << "  原始: " << text << std::endl;
    std::cout << "  大寫: " << toUpperCase(text) << std::endl;
    std::cout << "  小寫: " << toLowerCase(text) << std::endl;
    
    // repeat 沒有 using，仍需完整名稱
    std::cout << "  重複: " << string_tools::repeat("Hi", 3) << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 3. 命名空間別名
    // --------------------------------------------------
    std::cout << "[3] 命名空間別名" << std::endl;
    
    namespace mt = math_tools;
    namespace st = string_tools;
    
    std::cout << "  mt::PI = " << mt::PI << std::endl;
    std::cout << "  st::toUpperCase(\"test\") = " 
              << st::toUpperCase("test") << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 4. 巢狀命名空間
    // --------------------------------------------------
    std::cout << "[4] 巢狀命名空間" << std::endl;
    
    company::graphics::draw();
    company::graphics::clear();
    company::audio::play();
    company::audio::stop();
    std::cout << std::endl;
    
    // 也可以使用別名簡化
    namespace gfx = company::graphics;
    namespace sfx = company::audio;
    
    std::cout << "  使用別名：" << std::endl;
    gfx::draw();
    sfx::play();
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 5. 匿名命名空間（內部使用）
    // --------------------------------------------------
    std::cout << "[5] 匿名命名空間（內部計數器）" << std::endl;
    
    logCall("function1");
    logCall("function2");
    logCall("function3");
    
    std::cout << "  總呼叫次數: " << callCount << std::endl;
    std::cout << std::endl;
    
    // --------------------------------------------------
    // 6. 在區塊內使用 using namespace
    // --------------------------------------------------
    std::cout << "[6] 區塊內的 using namespace" << std::endl;
    
    {
        // 只在這個區塊內有效
        using namespace math_tools;
        using namespace string_tools;
        
        std::cout << "  在區塊內可以直接使用：" << std::endl;
        std::cout << "  PI = " << PI << std::endl;
        std::cout << "  circleArea(3) = " << circleArea(3) << std::endl;
        std::cout << "  toUpperCase(\"abc\") = " << toUpperCase("abc") << std::endl;
    }
    
    // 離開區塊後，又需要完整名稱
    std::cout << "  離開區塊後：math_tools::PI = " << math_tools::PI << std::endl;
    std::cout << std::endl;
    
    std::cout << "========================================" << std::endl;
    std::cout << "    展示完成" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
