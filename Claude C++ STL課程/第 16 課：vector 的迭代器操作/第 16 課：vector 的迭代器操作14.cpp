/*
信安，我們繼續課程。第 16 課的主題是 **vector 的迭代器操作**。

---

# 第 16 課：vector 的迭代器操作

## 一、迭代器是什麼？

在前面的課程中，我們已經用過 `operator[]` 和 `at()` 來存取 vector 的元素。但在 STL 的設計哲學中，**迭代器（iterator）** 才是存取容器元素的核心機制。

迭代器的概念很簡單：它就像一個**指標**，指向容器中的某個元素，你可以透過它來遍歷容器。

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 用指標的思維來理解迭代器
    // p 指向第一個元素
    std::vector<int>::iterator it = v.begin();

    std::cout << *it << std::endl;  // 10（解參考，就像指標一樣）
    ++it;
    std::cout << *it << std::endl;  // 20（往前移一個位置）

    return 0;
}
```

**輸出：**
```
10
20
```

---

## 二、vector 提供的迭代器函數

vector 提供了 **八個** 取得迭代器的成員函數，分為四組：

### 2.1 正向迭代器：`begin()` 與 `end()`

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // begin() 指向第一個元素
    // end()   指向最後一個元素的「下一個位置」（past-the-end）
    std::vector<int>::iterator first = v.begin();
    std::vector<int>::iterator last  = v.end();

    std::cout << "*first = " << *first << std::endl;
    // *last 是未定義行為！end() 不指向任何有效元素

    // 典型的遍歷方式
    for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
*first = 10
10 20 30 40 50
```

**重要觀念：** `end()` 回傳的迭代器指向「最後一個元素之後的位置」，這是一個 **哨兵（sentinel）**，不可以解參考。這個設計叫做 **半開區間 [begin, end)**，是 STL 的核心設計原則之一。

```
元素：  [10] [20] [30] [40] [50]
         ↑                        ↑
       begin()                  end()
```

### 2.2 反向迭代器：`rbegin()` 與 `rend()`

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // rbegin() 指向最後一個元素
    // rend()   指向第一個元素的「前一個位置」
    // 型別是 reverse_iterator
    std::vector<int>::reverse_iterator rit;

    std::cout << "反向遍歷：";
    for (rit = v.rbegin(); rit != v.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
反向遍歷：50 40 30 20 10
```

**注意：** 反向迭代器的 `++` 操作實際上是往**前**移動（往索引變小的方向）。這是透過運算子重載實現的，讓你用統一的 `++` 語法就能正向或反向遍歷。

```
元素：  [10] [20] [30] [40] [50]
    ↑                        ↑
  rend()                  rbegin()
```

### 2.3 常數迭代器：`cbegin()` 與 `cend()`（C++11）

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // cbegin() / cend() 回傳 const_iterator
    // 只能讀取，不能修改元素
    for (std::vector<int>::const_iterator it = v.cbegin(); it != v.cend(); ++it) {
        std::cout << *it << " ";
        // *it = 100;  // 編譯錯誤！不可透過 const_iterator 修改元素
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
10 20 30 40 50
```

**為什麼需要 `cbegin()` / `cend()`？**

在 C++11 之前，如果你想要 const_iterator，必須明確宣告型別：

```cpp
// C++11 之前，要明確寫出 const_iterator
std::vector<int>::const_iterator it = v.begin();  // begin() 回傳的會自動轉型

// 但用 auto 時就有問題了
auto it2 = v.begin();   // 推導為 iterator，不是 const_iterator！
auto it3 = v.cbegin();  // 推導為 const_iterator ✓
```

所以 `cbegin()` / `cend()` 是為了搭配 `auto` 而設計的，讓你明確表達「我只想讀取，不想修改」的意圖。

### 2.4 常數反向迭代器：`crbegin()` 與 `crend()`（C++11）

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 反向 + 唯讀
    std::cout << "常數反向遍歷：";
    for (auto rit = v.crbegin(); rit != v.crend(); ++rit) {
        std::cout << *rit << " ";
        // *rit = 100;  // 編譯錯誤！
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
常數反向遍歷：50 40 30 20 10
```

### 2.5 八個迭代器函數總覽

| 函數 | 回傳型別 | 指向位置 | 可否修改 |
|------|----------|----------|----------|
| `begin()` | `iterator` | 第一個元素 | 可 |
| `end()` | `iterator` | 最後元素之後 | — |
| `rbegin()` | `reverse_iterator` | 最後一個元素 | 可 |
| `rend()` | `reverse_iterator` | 第一元素之前 | — |
| `cbegin()` | `const_iterator` | 第一個元素 | 不可 |
| `cend()` | `const_iterator` | 最後元素之後 | — |
| `crbegin()` | `const_reverse_iterator` | 最後一個元素 | 不可 |
| `crend()` | `const_reverse_iterator` | 第一元素之前 | — |

---

## 三、vector 迭代器的性質：隨機存取迭代器

vector 的迭代器屬於 **隨機存取迭代器（Random Access Iterator）**，這是最強大的迭代器類別。它支援所有指標能做的運算：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();

    // 1. 解參考
    std::cout << "*it = " << *it << std::endl;           // 10

    // 2. 遞增 / 遞減
    ++it;
    std::cout << "++it: " << *it << std::endl;           // 20
    --it;
    std::cout << "--it: " << *it << std::endl;           // 10

    // 3. 加減整數（隨機跳躍）
    it = it + 3;
    std::cout << "it + 3: " << *it << std::endl;         // 40
    it = it - 2;
    std::cout << "it - 2: " << *it << std::endl;         // 20

    // 4. 下標運算
    std::cout << "it[2] = " << it[2] << std::endl;       // 40（等同 *(it + 2)）

    // 5. 迭代器相減（計算距離）
    auto it1 = v.begin();
    auto it2 = v.end();
    std::cout << "距離 = " << (it2 - it1) << std::endl;  // 5

    // 6. 比較運算
    std::cout << std::boolalpha;
    std::cout << "(begin < end) = " << (v.begin() < v.end()) << std::endl;  // true

    return 0;
}
```

**輸出：**
```
*it = 10
++it: 20
--it: 10
it + 3: 40
it - 2: 20
it[2] = 40
距離 = 5
(begin < end) = true
```

**隨機存取迭代器支援的完整運算列表：**

| 運算 | 說明 | 範例 |
|------|------|------|
| `*it` | 解參考 | `*it = 10` |
| `it->member` | 成員存取 | `it->size()` |
| `++it` / `it++` | 前進一步 | |
| `--it` / `it--` | 後退一步 | |
| `it + n` | 前進 n 步 | `it + 3` |
| `it - n` | 後退 n 步 | `it - 2` |
| `it += n` | 前進 n 步（原地） | `it += 3` |
| `it -= n` | 後退 n 步（原地） | `it -= 2` |
| `it[n]` | 下標存取 | `it[2]` 等同 `*(it+2)` |
| `it1 - it2` | 計算距離 | 回傳 `difference_type` |
| `it1 == it2` | 相等比較 | |
| `it1 != it2` | 不等比較 | |
| `it1 < it2` | 小於比較 | |
| `it1 > it2` / `<=` / `>=` | 其他比較 | |

> **對照：** `list` 的迭代器是雙向迭代器（Bidirectional Iterator），不支援 `+n`、`-n`、`[]`、`<` 等運算。這就是不同容器迭代器能力的差異。

---

## 四、迭代器與 `auto` 的搭配

在實務中，很少人會完整寫出 `std::vector<int>::iterator` 這種冗長型別。C++11 的 `auto` 讓程式碼簡潔許多：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 不用 auto（冗長）
    for (std::vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 使用 auto（簡潔）
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 如果只讀取，用 cbegin() / cend()
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出（三行相同）：**
```
10 20 30 40 50
10 20 30 40 50
10 20 30 40 50
```

---

## 五、範圍 for 迴圈的本質

C++11 引入的範圍 for 迴圈（range-based for loop）其實就是迭代器操作的**語法糖**：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 範圍 for 迴圈
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    // 上面等同於：
    for (auto it = v.begin(); it != v.end(); ++it) {
        int x = *it;
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
10 20 30 40 50
10 20 30 40 50
```

更精確地說，範圍 for 迴圈：

```cpp
for (declaration : expression) {
    statement;
}
```

會被編譯器展開為（概念上）：

```cpp
{
    auto __begin = expression.begin();  // 或 begin(expression)
    auto __end   = expression.end();    // 或 end(expression)
    for ( ; __begin != __end; ++__begin) {
        declaration = *__begin;
        statement;
    }
}
```

### 5.1 範圍 for 的三種接收方式

```cpp
#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};

    // 1. 拷貝（值語意）—— 不會修改原容器
    std::cout << "值拷貝：" << std::endl;
    for (std::string name : names) {
        name += "!";  // 修改的是拷貝
        std::cout << "  " << name << std::endl;
    }

    // 驗證原容器未被修改
    std::cout << "原容器：" << names[0] << std::endl;  // Alice（沒有 !）

    // 2. 引用 —— 可以修改原容器
    for (std::string& name : names) {
        name += "!";  // 修改原容器中的元素
    }
    std::cout << "修改後：" << names[0] << std::endl;   // Alice!

    // 3. const 引用 —— 不拷貝、不修改（最常用的唯讀方式）
    for (const std::string& name : names) {
        std::cout << name << " ";
        // name += "?";  // 編譯錯誤！
    }
    std::cout << std::endl;

    return 0;
}
```

**輸出：**
```
值拷貝：
  Alice!
  Bob!
  Charlie!
原容器：Alice
修改後：Alice!
Alice! Bob! Charlie!
```

**最佳實踐：**
- 需要修改元素 → `for (auto& x : v)`
- 只讀取、元素是基本型別 → `for (auto x : v)`（拷貝成本低）
- 只讀取、元素是較大物件 → `for (const auto& x : v)`（避免拷貝）

---

## 六、迭代器的實用技巧

### 6.1 用 `std::distance()` 計算距離

```cpp
#include <iostream>
#include <vector>
#include <iterator>  // std::distance

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it1 = v.begin();
    auto it2 = v.begin() + 3;

    // 方法一：直接相減（僅限隨機存取迭代器）
    std::cout << "it2 - it1 = " << (it2 - it1) << std::endl;

    // 方法二：std::distance()（適用於所有迭代器類型）
    std::cout << "distance = " << std::distance(it1, it2) << std::endl;

    return 0;
}
```

**輸出：**
```
it2 - it1 = 3
distance = 3
```

> `std::distance()` 的優點是通用性：對隨機存取迭代器，它直接算 `last - first`（O(1)）；對其他迭代器，它逐步 `++` 來計算（O(n)）。

### 6.2 用 `std::advance()` 移動迭代器

```cpp
#include <iostream>
#include <vector>
#include <iterator>  // std::advance

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();
    std::cout << "*it = " << *it << std::endl;  // 10

    // 前進 3 步
    std::advance(it, 3);
    std::cout << "advance(it, 3): " << *it << std::endl;  // 40

    // 後退 1 步
    std::advance(it, -1);
    std::cout << "advance(it, -1): " << *it << std::endl;  // 30

    return 0;
}
```

**輸出：**
```
*it = 10
advance(it, 3): 40
advance(it, -1): 30
```

> **注意：** `std::advance()` 是**原地修改**迭代器（沒有回傳值），而 `it + n` 是回傳新的迭代器。

### 6.3 用 `std::next()` 和 `std::prev()`（C++11）

```cpp
#include <iostream>
#include <vector>
#include <iterator>  // std::next, std::prev

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();

    // std::next() 回傳新迭代器，原迭代器不變
    auto it2 = std::next(it, 3);
    std::cout << "*it = " << *it << std::endl;    // 10（不變）
    std::cout << "*it2 = " << *it2 << std::endl;  // 40

    // std::prev() 回傳前一個位置
    auto it3 = std::prev(v.end());
    std::cout << "*prev(end) = " << *it3 << std::endl;  // 50

    // 也可以指定步數
    auto it4 = std::prev(v.end(), 2);
    std::cout << "*prev(end, 2) = " << *it4 << std::endl;  // 40

    return 0;
}
```

**輸出：**
```
*it = 10
*it2 = 40
*prev(end) = 50
*prev(end, 2) = 40
```

**`advance` vs `next`/`prev` 比較：**

| 特性 | `std::advance(it, n)` | `std::next(it, n)` / `std::prev(it, n)` |
|------|----------------------|------------------------------------------|
| 修改原迭代器 | 是（原地修改） | 否（回傳新迭代器） |
| 回傳值 | 無（`void`） | 新迭代器 |
| 適用場景 | 需要移動現有迭代器 | 需要保留原位置 |

---

## 七、空 vector 的迭代器

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;  // 空 vector

    std::cout << std::boolalpha;
    std::cout << "begin() == end(): " << (v.begin() == v.end()) << std::endl;

    // 空 vector 的 begin() 和 end() 相等
    // 所以 for 迴圈的條件 (it != v.end()) 一開始就不成立，不會執行
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";  // 不會執行
    }
    std::cout << "（迴圈未執行）" << std::endl;

    return 0;
}
```

**輸出：**
```
begin() == end(): true
（迴圈未執行）
```

---

## 八、迭代器失效（Iterator Invalidation）預告

這是使用 vector 迭代器時最危險的陷阱。我們會在第 17 課（vector 的記憶體重新配置機制）中深入討論，但這裡先給你一個概念：

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30};
    auto it = v.begin();  // 指向 10

    std::cout << "*it = " << *it << std::endl;  // 10

    // 這個 push_back 可能導致記憶體重新配置
    // 如果發生重新配置，it 就失效了！
    v.push_back(40);
    v.push_back(50);
    v.push_back(60);

    // 危險！it 可能已經失效
    // std::cout << *it << std::endl;  // 未定義行為！

    // 安全的做法：重新取得迭代器
    it = v.begin();
    std::cout << "*it = " << *it << std::endl;  // 10

    return 0;
}
```

**核心概念：** 任何可能導致 vector 記憶體重新配置的操作（如 `push_back`、`insert`、`resize` 等），都可能使所有現有迭代器失效。第 17 課會詳細解釋為什麼。

---

## 九、本課重點回顧

1. **八個迭代器函數**：`begin/end`、`rbegin/rend`、`cbegin/cend`、`crbegin/crend`
2. **半開區間 [begin, end)**：`end()` 指向最後元素之後，不可解參考
3. **vector 的迭代器是隨機存取迭代器**：支援 `+n`、`-n`、`[]`、比較等完整運算
4. **`auto` 搭配 `cbegin()`**：明確表達唯讀意圖
5. **範圍 for 迴圈是迭代器的語法糖**
6. **實用工具**：`distance()`、`advance()`、`next()`、`prev()`
7. **迭代器失效**：修改 vector 結構後，現有迭代器可能失效

---

## 課後練習

**練習一：基礎遍歷**
使用四種方式遍歷 `vector<int> v = {1, 2, 3, 4, 5}`：
1. 正向迭代器
2. 反向迭代器
3. 範圍 for 迴圈
4. `auto` + `cbegin()`

**練習二：尋找與距離**
給定 `vector<int> v = {5, 12, 8, 3, 17, 9, 1}`，使用迭代器找出值為 `17` 的元素，並印出它距離 `begin()` 的距離（索引位置）。提示：可以用迴圈搭配 `std::distance()`。

**練習三：反向修改**
使用反向迭代器，將 `vector<int> v = {1, 2, 3, 4, 5}` 中的每個元素乘以 10，然後正向印出結果。預期輸出：`10 20 30 40 50`。

---

準備好了就告訴我，我們可以進入第 17 課：**vector 的記憶體重新配置機制**，那會解釋為什麼迭代器會失效，以及 vector 擴容的完整機制。
*/



#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30};
    auto it = v.begin();  // 指向 10

    std::cout << "*it = " << *it << std::endl;  // 10

    // 這個 push_back 可能導致記憶體重新配置
    // 如果發生重新配置，it 就失效了！
    v.push_back(40);
    v.push_back(50);
    v.push_back(60);

    // 危險！it 可能已經失效
    // std::cout << *it << std::endl;  // 未定義行為！

    // 安全的做法：重新取得迭代器
    it = v.begin();
    std::cout << "*it = " << *it << std::endl;  // 10

    return 0;
}
