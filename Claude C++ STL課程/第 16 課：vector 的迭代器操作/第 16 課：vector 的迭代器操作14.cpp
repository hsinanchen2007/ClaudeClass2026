// =============================================================================
//  第 16 課-14：迭代器失效 —— 為什麼「拿在手上的位置」會突然作廢
// =============================================================================
//
// 【主題資訊 Information】
//   本檔上半部是第 16 課的完整講義（保留在一個大 /* */ 註解區塊中），
//   下半部是可執行的「迭代器失效」示範程式。
//   相關操作與其失效範圍（vector）：
//     push_back / emplace_back  → 若觸發重新配置：全部失效；否則只有 end() 失效
//     insert                    → 若觸發重新配置：全部失效；否則插入點之後失效
//     erase                     → 刪除點之後（含）失效；之前仍有效
//     reserve / resize / shrink_to_fit → 若容量改變：全部失效
//     clear                     → 全部失效（end() 除外仍是合法值）
//   標準版本：C++98 起即有明確規定（[vector.modifiers]）
//   標頭檔：<vector>
//
// 【詳細解釋 Explanation】
//
// 【1. 失效的物理原因只有兩個】
//   (a) 記憶體重新配置：vector 的元素必須連續，容量不夠時只能
//       另外配置一塊更大的、把元素搬過去、釋放舊的。
//       舊位址上的東西已經被 free 掉，指向那裡的迭代器就是懸空指標。
//   (b) 元素搬移：即使沒有重新配置，insert/erase 也會讓插入或刪除點
//       之後的元素整批平移。位址還在，但上面住的已經是別的元素了。
//   第一種是「指向已釋放的記憶體」，第二種是「指向了錯的元素」。
//   前者通常會崩潰，後者更陰險——程式照跑，只是答案默默錯掉。
//
// 【2. 為什麼迭代器不能「自動更新」】
//   一個很自然的疑問：vector 為什麼不維護一份「所有存活迭代器」的清單，
//   在搬移時一併更新？答案是成本。
//   vector 的迭代器在多數實作上就是一根裸指標，sizeof 只有 8 bytes、
//   零建構成本、可以直接被 CPU 當位址用。
//   若要支援自動更新，每個迭代器都得註冊到容器裡、容器要多存一份清單、
//   建構解構都要動到那份清單——vector 就不再是「零額外成本的陣列」了。
//   C++ 的一貫取捨是：不為你沒要求的功能付費，正確性交給程式設計者。
//
// 【3. 「可能失效」為什麼比「一定失效」更危險】
//   push_back 只有在 size == capacity 時才重新配置。
//   所以同一段程式碼，capacity 還夠時迭代器完好、剛好用滿時就懸空。
//   這造成一種最惡劣的 bug 型態：小資料量測試全過，
//   資料一多就在毫無關聯的地方崩潰。
//   正確心態是把「可能失效」直接當成「已經失效」——
//   標準允許失效，你就不能依賴它沒失效。
//
// 【4. end() 的特殊地位】
//   push_back 即使沒有觸發重新配置，end() 也一定失效（它必須往後挪一格）。
//   這正是「範圍 for 內 push_back 會出事」的原因之一：
//   範圍 for 在進入迴圈前就把 end() 快取起來了。
//
// 【5. 三種正確的應對策略】
//   (a) 事前 reserve：容量一次到位，之後 push_back 不再重新配置。
//       這是最乾淨的做法，前提是你事先知道大概要多少。
//   (b) 改用索引：索引不會因重新配置而失效（它是相對位置，不是位址）。
//       缺點是只適用於 random access 容器。
//   (c) 接住回傳值：insert/erase 都會回傳新的有效迭代器，
//       養成 it = v.erase(it) 的習慣。
//
// 【概念補充 Concept Deep Dive】
//   ▸ 索引為什麼不會失效
//     索引是「從 begin 算起的第幾個」，是相對量；
//     迭代器是「記憶體的哪個位址」，是絕對量。
//     重新配置改變的是絕對位址，相對位置不變，所以 v[2] 永遠是第三個元素。
//     代價是每次存取都要做一次 begin_ + i 的加法（實務上可忽略）。
//   ▸ reference 與 pointer 的失效規則完全相同
//     int& r = v[0]; 和 int* p = &v[0]; 和 auto it = v.begin();
//     三者本質上都是「記住了一個位址」，失效規則一模一樣。
//     很多人只警覺迭代器，卻把 reference 存起來，一樣會中。
//   ▸ 其他容器的失效規則差很多
//     list：只有被刪除的那個節點失效，其餘永遠有效（節點各自獨立）。
//     deque：插入頭尾使所有迭代器失效，但 reference 仍有效（分段配置的特性）。
//     map/set：只有被刪的節點失效，插入完全不影響既有迭代器。
//     「vector 的規則」不能直接套到其他容器上。
//
// 【注意事項 Pay Attention】
//   1. 使用失效迭代器是 undefined behavior，不是「值會不對」——
//      它可能崩潰、可能安靜地讀到垃圾、也可能剛好看起來正常。
//   2. 「這次沒崩潰」不代表程式碼正確，只代表這次的 capacity 剛好夠。
//   3. reference 與 pointer 的失效規則和迭代器完全相同。
//   4. 範圍 for 內不可改變容器大小（begin/end 已被快取）。
//   5. reserve 本身若造成容量改變，也會使既有迭代器全部失效。
//   6. 別把 vector 的失效規則套到 list/deque/map 上，四者規則都不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 的 push_back 一定會讓迭代器失效嗎？
//     答：不一定。只有在 size == capacity、必須重新配置時，
//         所有迭代器/指標/引用才會全部失效。容量還夠時，
//         指向既有元素的迭代器仍然有效——但 end() 一定失效。
//         由於呼叫端通常無法確定當下容量，實務上必須一律當作會失效。
//     追問：那要怎麼寫才安全？
//         → 事前 reserve 把容量一次備足；或改用索引（索引不會失效）；
//           或每次操作後重新 v.begin() 取得迭代器。
//
// 🔥 Q2. 為什麼標準不讓 vector 自動更新失效的迭代器？
//     答：成本。vector 的迭代器在多數實作上就是一根裸指標，
//         零額外空間、零建構成本。要支援自動更新，就得讓容器
//         維護一份存活迭代器清單，每個迭代器的建構、複製、解構
//         都要動到那份清單，vector 也就不再是「零開銷的動態陣列」。
//         C++ 選擇把正確性責任交給使用者，換取效能。
//     追問：有沒有語言或函式庫做了自動更新？
//         → 有，例如某些 debug 模式（libstdc++ 的 _GLIBCXX_DEBUG、
//           MSVC 的 iterator debugging）會加上檢查並在誤用時直接中止，
//           但那是開發期工具，會顯著拖慢執行速度。
//
// ⚠️ 陷阱. std::vector<int> v = {1,2,3};
//          int& r = v[0];
//          v.push_back(4);
//          r = 99;                     // ← 這行為什麼危險？
//     答：push_back 若觸發重新配置，v[0] 已經搬到新位址，
//         而 r 仍綁在被 free 掉的舊記憶體上，寫入它是 undefined behavior
//         （典型的 heap-use-after-free，ASan 會直接抓到）。
//     為什麼會錯：多數人只把「迭代器失效」記成迭代器的問題，
//         沒意識到 reference 和 pointer 本質上是同一回事——
//         三者都只是「記住了一個位址」。
//         口訣：凡是從容器裡「取出位址」的東西，失效規則完全一致。
// ═══════════════════════════════════════════════════════════════════════════

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
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】即時排行榜：一邊掃描一邊補進新條目
//   情境：遊戲伺服器維護一份玩家分數清單。結算時要走訪全部玩家，
//         發現達標者就把「獎勵條目」補進同一份清單。
//   為什麼用本主題：這是迭代器失效最常見的真實形態——
//         「邊走訪邊往同一個容器加東西」。
//         正確做法有兩種，本範例兩種都示範：
//           (A) 用索引走訪（索引不會因重新配置而失效）；
//           (B) 先收集到暫存區，走訪結束後再一次併入（語意最清晰）。
//         絕對不可以的是：在範圍 for 或迭代器迴圈裡直接對同一容器 push_back。
// -----------------------------------------------------------------------------
struct Player {
    std::string name;
    int score;
};

// 做法 A：用索引走訪，邊走邊加。索引是相對位置，重新配置不會使它失效。
// 迴圈條件每次都重新讀 v.size()，所以新加入的元素也會被走訪到
// （這裡用 name 前綴擋掉，避免獎勵條目再產生獎勵）。
void grantBonusByIndex(std::vector<Player>& players, int threshold) {
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (players[i].score >= threshold && players[i].name.rfind("BONUS:", 0) != 0) {
            players.push_back({"BONUS:" + players[i].name, 0});
            // 若改用迭代器走訪，push_back 之後迭代器就可能懸空了
        }
    }
}

// 做法 B：走訪時只讀不寫，把要新增的先放暫存區，結束後一次併入。
// 走訪期間容器完全沒被改動，不存在任何失效問題。
void grantBonusByStaging(std::vector<Player>& players, int threshold) {
    std::vector<Player> pending;
    for (const Player& p : players) {              // 唯讀走訪，安全
        if (p.score >= threshold) {
            pending.push_back({"BONUS:" + p.name, 0});
        }
    }
    players.insert(players.end(), pending.begin(), pending.end());
}

int main() {
    std::cout << "=== 一、迭代器失效的基本示範 ===" << std::endl;
    std::vector<int> v = {10, 20, 30};
    auto it = v.begin();  // 指向 10

    std::cout << "*it = " << *it << std::endl;  // 10

    // 這幾個 push_back 可能導致記憶體重新配置
    // 一旦發生重新配置，it 就失效了！
    v.push_back(40);
    v.push_back(50);
    v.push_back(60);

    // 危險！it 可能已經失效
    // std::cout << *it << std::endl;  // 未定義行為！

    // 安全的做法：重新取得迭代器
    it = v.begin();
    std::cout << "*it = " << *it << std::endl;  // 10

    std::cout << "\n=== 二、觀察「什麼時候」真的重新配置 ===" << std::endl;
    std::cout << "（不印位址本身——位址每次執行都不同；改印位址有沒有改變）" << std::endl;
    std::vector<int> w;
    w.reserve(4);                       // 先把容量固定在 4，讓行為可預期
    for (int i = 1; i <= 3; ++i) w.push_back(i);

    const int* before = w.data();
    std::size_t capBefore = w.capacity();
    w.push_back(4);                     // size 3→4，還沒超過 capacity 4
    std::cout << "size 3→4（capacity " << capBefore << "）: 重新配置? "
              << std::boolalpha << (w.data() != before) << std::endl;

    before = w.data();
    capBefore = w.capacity();
    w.push_back(5);                     // size 4→5，超過 capacity → 必定重新配置
    std::cout << "size 4→5（capacity " << capBefore << "）: 重新配置? "
              << (w.data() != before)
              << "，新 capacity = " << w.capacity() << std::endl;
    std::cout << "↑ 同一行 push_back，一次沒事一次全部失效——這就是它危險的原因" << std::endl;

    std::cout << "\n=== 三、索引不會失效，指標與迭代器會 ===" << std::endl;
    std::vector<int> u = {1, 2, 3};
    std::size_t idx = 1;                       // 索引：相對位置
    const int* p = &u[1];                      // 指標：絕對位址
    std::cout << "push_back 前 u[idx] = " << u[idx] << ", *p = " << *p << std::endl;
    u.push_back(4);
    u.push_back(5);
    u.push_back(6);                            // 期間必定發生過重新配置
    std::cout << "push_back 後 u[idx] = " << u[idx] << "（索引仍然正確）" << std::endl;
    std::cout << "（*p 不再印出——p 可能已懸空，讀它是 undefined behavior）" << std::endl;
    (void)p;

    std::cout << "\n=== 四、日常實務：邊走訪邊新增的兩種安全寫法 ===" << std::endl;
    std::vector<Player> a = {{"Alice", 95}, {"Bob", 60}, {"Carol", 88}};
    grantBonusByIndex(a, 80);
    std::cout << "做法A（索引走訪）  : ";
    for (const Player& pl : a) std::cout << pl.name << "(" << pl.score << ") ";
    std::cout << std::endl;

    std::vector<Player> b = {{"Alice", 95}, {"Bob", 60}, {"Carol", 88}};
    grantBonusByStaging(b, 80);
    std::cout << "做法B（暫存後併入）: ";
    for (const Player& pl : b) std::cout << pl.name << "(" << pl.score << ") ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：vector 的迭代器操作14.cpp" -o invalidation
//
// 【關於下方預期輸出的但書】
//   ▸ 本檔刻意不印出任何記憶體位址——位址每次執行都不同，
//     改為印出「位址有沒有改變」這個布林結果，才是可重現的觀察。
//   ▸ 第二段的 capacity 數值（4 → 8）來自 libstdc++ 的 2 倍成長策略，
//     屬實作定義（本機 GCC 15.2 實測）；MSVC 採 1.5 倍，數字會不同。
//   ▸ 第三段刻意不解參考可能懸空的指標 p，因此輸出中沒有它的值。
//
// 【本檔未附 LeetCode 範例的理由】
//   迭代器失效是「容器實作機制」層次的議題，源自 vector 必須維持
//   記憶體連續這個約束。LeetCode 的解題流程幾乎不會出現
//   「邊走訪邊改變同一容器大小」的需求，硬套一題只會讓主題失焦。
//   真正會被它咬到的是上面那種伺服器端的長期維護程式碼，
//   因此改以排行榜結算的實務範例呈現。

// === 預期輸出 ===
// === 一、迭代器失效的基本示範 ===
// *it = 10
// *it = 10
//
// === 二、觀察「什麼時候」真的重新配置 ===
// （不印位址本身——位址每次執行都不同；改印位址有沒有改變）
// size 3→4（capacity 4）: 重新配置? false
// size 4→5（capacity 4）: 重新配置? true，新 capacity = 8
// ↑ 同一行 push_back，一次沒事一次全部失效——這就是它危險的原因
//
// === 三、索引不會失效，指標與迭代器會 ===
// push_back 前 u[idx] = 2, *p = 2
// push_back 後 u[idx] = 2（索引仍然正確）
// （*p 不再印出——p 可能已懸空，讀它是 undefined behavior）
//
// === 四、日常實務：邊走訪邊新增的兩種安全寫法 ===
// 做法A（索引走訪）  : Alice(95) Bob(60) Carol(88) BONUS:Alice(0) BONUS:Carol(0)
// 做法B（暫存後併入）: Alice(95) Bob(60) Carol(88) BONUS:Alice(0) BONUS:Carol(0)
