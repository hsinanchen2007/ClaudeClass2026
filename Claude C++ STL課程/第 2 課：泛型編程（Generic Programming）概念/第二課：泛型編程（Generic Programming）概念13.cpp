// =============================================================================
//  第二課：泛型編程（Generic Programming）概念13.cpp
//   —  本課完整講義（全文）＋ 可執行的泛型 Stack 類別模板
// =============================================================================
//
// 【主題資訊 Information】
//
//   本檔的結構與本課其他檔案不同，請先看清楚：
//     * 檔案前段（約 786 行）是**整份第二課講義的 Markdown 全文**，
//       完整包在一個多行註解區塊內，編譯器不會處理它。
//       它涵蓋 2.1 ~ 2.10 全部小節，是本課的文字教材主體。
//     * 檔案後段才是**真正會被編譯的程式碼**：一個泛型 Stack<T> 類別模板，
//       示範同一份程式碼同時服務 int 與 std::string。
//     * 檔尾另附【面試題】、一則 LeetCode 實戰與預期輸出。
//
//   可編譯部分的重點語法：
//     template <typename T>
//     class Stack {
//         static const int MAX_SIZE = 100;   // 類別內初始化的整數常數
//         T data[MAX_SIZE];                  // 固定大小陣列，非動態配置
//         int top_index;
//     };
//
//   標準版本：C++98 起即可編譯（本檔不依賴任何新特性，以 -std=c++17 編譯）
//   標頭檔  ：<iostream>、<stdexcept>、<string>
//   複雜度  ：push / pop / top / is_empty / size 全部 O(1)
//   例外    ：滿了丟 std::overflow_error，空了丟 std::underflow_error
//
//   與標準庫的對應：std::stack<T, Container> 定義於 <stack>，
//   它是一個**容器配接器**（container adapter），預設底層用 std::deque。
//   兩者的關鍵差異見下方【注意事項】。
//
// 【講義涵蓋的內容（前段 Markdown 全文）】
//   2.1  什麼是泛型編程        2.6  類別模板
//   2.2  Template 的基石       2.7  型別約束（concepts 簡介）
//   2.3  函數模板              2.8  執行期多型 vs 編譯期多型
//   2.4  型別推導              2.9  STL 與泛型編程的關係
//   2.5  多型別參數            2.10 小結與練習
//
//   本課的 13 個 .cpp 小檔正是講義中各段程式碼的可執行版本，
//   每個檔案都另有完整的解釋、概念補充與面試題。建議搭配閱讀：
//     概念1~2  重複程式碼的問題 → 函式模板
//     概念3~5  隱含介面、推導衝突、多型別參數
//     概念6~7  類別模板、實例化即獨立型別
//     概念8~9  隱含介面不滿足時的錯誤與修法
//     概念10   C++20 concepts
//     概念11~12 執行期多型 vs 編譯期多型（本課核心對照）
//     summary  全部重點的總複習
//
// 【詳細解釋 Explanation】
//   ★ 本檔前段（多行註解區塊內）是第二課講義全文，逐節說明請直接往下閱讀。
//     此處只補三個「讀完講義要能自己講出來」的主軸：
//
//   【1. 類別模板不是一個型別，而是一份產生型別的藍圖】
//      Stack<int> 與 Stack<std::string> 是**兩個毫不相干的型別**，
//      不能互相指派、也沒有共同基底類別。編譯器對每個用到的 T 各生成一份程式碼，
//      這正是「編譯期多型」與繼承式「執行期多型」最根本的差異：
//      前者沒有 vtable、沒有間接呼叫，代價是每多一個 T 就多一份機器碼。
//
//   【2. 成員函式是 lazy instantiation（用到才實例化）】
//      類別模板的成員函式只有在**真的被呼叫**時才會被實例化。
//      所以 Stack<T> 即使有某個成員用了 T 不支援的運算，只要你從不呼叫它，
//      程式仍能編譯通過。這既方便（部分支援的型別也能用）也危險
//      （錯誤要到第一次呼叫才爆出來，而且訊息指向模板內部而非呼叫端）。
//
//   【3. 隱含介面（implicit interface）取代明確的介面宣告】
//      模板不要求 T 繼承任何東西，只要求 T「支援模板內用到的那些操作」。
//      這份要求從不寫在程式碼裡，而是散落在模板本體 —— 這就是 C++20 之前
//      模板錯誤訊息難讀的根源，也是 concepts 要解決的問題（見概念10.cpp）。
//
// 【概念補充 Concept Deep Dive】
//   編譯器怎麼處理「同一個模板在多個 .cpp 都被實例化」？
//   模板定義通常放在 header，於是每個引入它的 translation unit 都會各自生成
//   一份 Stack<int> 的程式碼。這看似違反 ODR（單一定義原則），實際上編譯器把
//   這些符號標記為 **weak / COMDAT（vague linkage）**，連結器在最後合併時
//   只保留一份、丟棄其餘副本，因此不會出現重複定義錯誤。
//   代價是編譯期成本：同一份模板被反覆解析與生成，最後又被丟掉大半 ——
//   這是 C++ 編譯速度慢的主因之一，也是 C++20 modules 想改善的問題。
//   反過來說，這也解釋了為何模板定義**不能**只放 .cpp：
//   別的 TU 看不到定義就無法實例化，會在連結期得到 undefined reference。
//
// 【注意事項 Pay Attention】
// 1. 本檔後段的 Stack<T> 用 `T data[MAX_SIZE];` 固定配置 100 個元素，
//    這代表 **Stack<T> 一建立就佔用 100 個 T 的空間**，且 T 必須可預設建構
//    （100 個元素在建構時就全部被預設建構了）。
//    Stack<std::string> 因此會在建構時就配置 100 個空字串。
//    標準庫的 std::stack 底層預設用 std::deque，按需成長，沒有這個問題。
// 2. pop() 同時「移除並回傳」元素。標準庫刻意**不**這樣設計 ——
//    std::stack 的 pop() 回傳 void，要取值得先呼叫 top()。
//    原因是例外安全：若「移除」之後、「回傳的複製」過程中拋出例外，
//    元素就永久遺失且無法復原。分成 top() + pop() 兩步才能保證強例外保證。
//    這是 STL 設計中極常被面試問到的一個取捨（見【面試題 Q2】）。
// 3. MAX_SIZE 是 `static const int` 且在類別內初始化 —— 這對整數型別是
//    C++98 就允許的特例。若換成 double 或需要取其位址，就得另外在類別外定義
//    （C++17 起可用 inline 變數或 static constexpr 直接解決）。
// 4. 本檔前段講義中的程式碼片段是**教材文字**，不會被編譯。
//    要實際執行那些片段，請看對應的概念1~12.cpp。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別模板實作與 STL 設計取捨
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 本檔的 Stack<T> 用 `T data[100];` 固定陣列，
//        這個設計有什麼問題？std::stack 怎麼做？
//     答：三個問題 ——（a）不論實際用幾個元素，一律佔 100 個 T 的空間；
//         （b）T 必須可預設建構，因為 100 個元素在 Stack 建構時就全被建構了
//         （Stack<std::string> 會立刻產生 100 個空字串）；
//         （c）超過 100 個就只能丟例外，無法成長。
//         std::stack 是**容器配接器**，底層預設用 std::deque，按需成長，
//         也不要求元素可預設建構。
//     追問：為什麼 std::stack 預設用 deque 而不是 vector？
//         → deque 在尾端成長時不需要搬動既有元素（vector 重新配置時要全部搬移），
//           且不會有「一次大量複製」的延遲尖峰。需要 vector 的連續記憶體時，
//           可以自己指定 std::stack<T, std::vector<T>>。
//
// 🔥 Q2. 本檔的 pop() 同時移除並回傳元素，std::stack 的 pop() 卻回傳 void。
//        為什麼標準庫要拆成 top() + pop() 兩步？
//     答：為了**例外安全**。若 pop() 要回傳值，就必須在移除元素後複製一份出去；
//         萬一這個複製建構拋出例外，元素已經被移除、複製又失敗，
//         那個值就永久遺失且無法復原 —— 無法提供強例外保證。
//         拆成 top()（只讀，不改變狀態）與 pop()（只移除，不回傳）之後，
//         每一步都能各自保證安全。
//     追問：C++11 之後有 move 語意了，是不是就可以合併了？
//         → 仍然不建議。只有在 T 的移動建構子是 noexcept 時才安全，
//           而這無法對所有 T 保證。標準庫選擇了對所有型別都成立的介面設計。
//
// ⚠️ 陷阱. Stack<int> 和 Stack<std::string> 共用同一份 push/pop 程式碼嗎？
//     答：不共用。它們是兩個完全獨立的類別，編譯器各生成一份成員函式程式碼
//         （Stack<int>::push 與 Stack<std::string>::push 是兩個不同的函式，
//         機器碼也不同 —— 一個搬 4 bytes 整數，一個要呼叫 string 的賦值運算子）。
//         原始碼只有一份，**產出的機器碼有兩份**。
//     為什麼會錯：把「原始碼共用」誤當成「程式碼共用」。
//         這正是 C++ template 與 Java generics 的根本差異：
//         Java 抹除型別後真的只有一份 bytecode，C++ 則是為每個型別各生成一份
//         （monomorphization）。這也是 code bloat 的來源 ——
//         詳見概念2.cpp 的本機實測數據。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第二課：泛型編程（Generic Programming）概念

---

## 2.1 什麼是泛型編程？

**泛型編程**（Generic Programming）是一種程式設計範式，核心思想是：

> **「撰寫與型別無關的程式碼，讓同一段邏輯可以處理多種不同的資料型別。」**

在 C++ 中，泛型編程主要透過 **template**（模板）來實現。

### 一個簡單的問題引入

假設你需要寫一個「交換兩個變數」的函數：

```cpp
#include <iostream>

// 交換兩個 int
void swap_int(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

// 交換兩個 double
void swap_double(double& a, double& b) {
    double temp = a;
    a = b;
    b = temp;
}

// 交換兩個 string... 還要再寫一次？

int main() {
    int x = 10, y = 20;
    swap_int(x, y);
    std::cout << "x = " << x << ", y = " << y << std::endl;
    
    double a = 3.14, b = 2.72;
    swap_double(a, b);
    std::cout << "a = " << a << ", b = " << b << std::endl;
    
    return 0;
}
```

**輸出：**
```
x = 20, y = 10
a = 2.72, b = 3.14
```

問題很明顯：**邏輯完全相同，只是型別不同**，卻要重複寫多次。這違反了 DRY 原則（Don't Repeat Yourself）。

---

## 2.2 Template：泛型編程的基石

C++ 的 **template** 讓我們可以寫出「型別參數化」的程式碼：

```cpp
#include <iostream>
#include <string>

// 泛型版本：一次搞定所有型別
template <typename T>
void my_swap(T& a, T& b) {
    T temp = a;
    a = b;
    b = temp;
}

int main() {
    // 交換 int
    int x = 10, y = 20;
    my_swap(x, y);  // 編譯器自動推導 T = int
    std::cout << "x = " << x << ", y = " << y << std::endl;
    
    // 交換 double
    double a = 3.14, b = 2.72;
    my_swap(a, b);  // 編譯器自動推導 T = double
    std::cout << "a = " << a << ", b = " << b << std::endl;
    
    // 交換 string
    std::string s1 = "Hello", s2 = "World";
    my_swap(s1, s2);  // 編譯器自動推導 T = std::string
    std::cout << "s1 = " << s1 << ", s2 = " << s2 << std::endl;
    
    return 0;
}
```

**輸出：**
```
x = 20, y = 10
a = 2.72, b = 3.14
s1 = World, s2 = Hello
```

### Template 的運作機制

```
┌─────────────────────────────────────────────────────────────┐
│                  Template 實例化過程                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   原始碼                          編譯器產生的程式碼          │
│                                                             │
│   template <typename T>                                     │
│   void my_swap(T& a, T& b)                                  │
│   { ... }                                                   │
│                                                             │
│         │                                                   │
│         ▼                                                   │
│   ┌─────────────────┐                                       │
│   │  my_swap(x, y)  │ ──► void my_swap(int& a, int& b)     │
│   │  (int, int)     │                                       │
│   └─────────────────┘                                       │
│                                                             │
│   ┌─────────────────┐                                       │
│   │  my_swap(a, b)  │ ──► void my_swap(double& a, double& b)│
│   │  (double,double)│                                       │
│   └─────────────────┘                                       │
│                                                             │
│   ┌─────────────────┐                                       │
│   │  my_swap(s1,s2) │ ──► void my_swap(string& a, string& b)│
│   │  (string,string)│                                       │
│   └─────────────────┘                                       │
│                                                             │
│   關鍵：這是「編譯期」的程式碼生成，不是「執行期」的多型！     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

Template 是**編譯期**的機制：
1. 編譯器看到 `my_swap(x, y)` 時，推導出 `T = int`
2. 編譯器根據模板**生成**一個 `void my_swap(int&, int&)` 函數
3. 這個過程叫做 **template instantiation**（模板實例化）

因為是編譯期展開，所以**沒有執行期的效能損失**。

---

## 2.3 函數模板（Function Template）

### 基本語法

```cpp
template <typename T>      // 或 template <class T>，兩者在這裡等價
返回型別 函數名稱(參數列表) {
    // 函數主體
}
```

### 範例：找最大值

```cpp
#include <iostream>
#include <string>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    std::cout << "max(10, 20) = " << find_max(10, 20) << std::endl;
    std::cout << "max(3.14, 2.72) = " << find_max(3.14, 2.72) << std::endl;
    std::cout << "max('a', 'z') = " << find_max('a', 'z') << std::endl;
    
    // 字串比較（字典序）
    std::string s1 = "apple", s2 = "banana";
    std::cout << "max(\"apple\", \"banana\") = " << find_max(s1, s2) << std::endl;
    
    return 0;
}
```

**輸出：**
```
max(10, 20) = 20
max(3.14, 2.72) = 3.14
max('a', 'z') = z
max("apple", "banana") = banana
```

### 明確指定型別

有時候編譯器無法自動推導，或你想強制使用特定型別：

```cpp
#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    // 問題：10 是 int，3.14 是 double，型別不一致
    // std::cout << find_max(10, 3.14);  // 編譯錯誤！
    
    // 解法一：明確指定型別
    std::cout << find_max<double>(10, 3.14) << std::endl;
    
    // 解法二：讓兩個參數型別一致
    std::cout << find_max(10.0, 3.14) << std::endl;
    
    return 0;
}
```

**輸出：**
```
10
10
```

### 多個型別參數

```cpp
#include <iostream>
#include <string>

template <typename T1, typename T2>
void print_pair(T1 first, T2 second) {
    std::cout << "(" << first << ", " << second << ")" << std::endl;
}

int main() {
    print_pair(1, 3.14);                    // int, double
    print_pair("Name", 42);                 // const char*, int
    print_pair(std::string("Hello"), 'W');  // string, char
    
    return 0;
}
```

**輸出：**
```
(1, 3.14)
(Name, 42)
(Hello, W)
```

---

## 2.4 類別模板（Class Template）

Template 不只能用在函數，也能用在類別。這是 STL 容器的基礎。

### 基本語法

```cpp
template <typename T>
class ClassName {
    // 類別成員可以使用 T
};
```

### 範例：簡易的泛型容器

讓我們實作一個超簡化版的「盒子」容器：

```cpp
#include <iostream>
#include <string>

template <typename T>
class Box {
private:
    T content;
    bool has_content;
    
public:
    Box() : has_content(false) {}
    
    void put(const T& item) {
        content = item;
        has_content = true;
    }
    
    T get() const {
        if (!has_content) {
            throw std::runtime_error("Box is empty!");
        }
        return content;
    }
    
    bool is_empty() const {
        return !has_content;
    }
};

int main() {
    // 裝 int 的盒子
    Box<int> int_box;
    int_box.put(42);
    std::cout << "int_box contains: " << int_box.get() << std::endl;
    
    // 裝 string 的盒子
    Box<std::string> string_box;
    string_box.put("Hello, STL!");
    std::cout << "string_box contains: " << string_box.get() << std::endl;
    
    // 裝 double 的盒子
    Box<double> double_box;
    double_box.put(3.14159);
    std::cout << "double_box contains: " << double_box.get() << std::endl;
    
    return 0;
}
```

**輸出：**
```
int_box contains: 42
string_box contains: Hello, STL!
double_box contains: 3.14159
```

### STL 的 vector 就是類別模板

當你寫 `std::vector<int>` 時，就是在**實例化**一個類別模板：

```cpp
#include <iostream>
#include <vector>
#include <string>

int main() {
    // vector<int> 是一個型別
    // vector<double> 是另一個型別
    // 它們是從同一個 template 實例化出來的不同類別
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<std::string> words = {"Hello", "World"};
    
    std::cout << "numbers[0] = " << numbers[0] << std::endl;
    std::cout << "words[0] = " << words[0] << std::endl;
    
    return 0;
}
```

**輸出：**
```
numbers[0] = 1
words[0] = Hello
```

---

## 2.5 泛型編程的核心概念：概念（Concepts）

泛型編程有一個重要的問題：**不是所有型別都適用於所有模板**。

### 問題示範

```cpp
#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;  // 使用了 > 運算子
}

// 自訂類別，沒有定義 > 運算子
class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

int main() {
    Point p1(1, 2), p2(3, 4);
    // Point max_point = find_max(p1, p2);  // 編譯錯誤！
    // 錯誤訊息：no match for 'operator>'
    
    return 0;
}
```

### 隱含的要求：Concept

`find_max` 函數對型別 `T` 有一個**隱含的要求**：`T` 必須支援 `>` 運算子。

在泛型編程中，這種「型別必須滿足的條件」稱為 **Concept**（概念）。

```
┌─────────────────────────────────────────────────────────────┐
│                    Concept 概念圖解                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   template <typename T>                                     │
│   T find_max(T a, T b) {                                    │
│       return (a > b) ? a : b;                               │
│   }                                                         │
│                                                             │
│   隱含的 Concept：                                           │
│   ┌───────────────────────────────────┐                     │
│   │  Comparable（可比較）              │                     │
│   │                                   │                     │
│   │  要求：                            │                     │
│   │  • 支援 operator>                  │                     │
│   │  • 回傳值可轉換為 bool             │                     │
│   └───────────────────────────────────┘                     │
│                                                             │
│   滿足 Comparable 的型別：                                   │
│   ✓ int, double, char                                       │
│   ✓ std::string（有定義 operator>）                         │
│   ✗ Point（沒有定義 operator>）                              │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 讓自訂型別滿足 Concept

```cpp
#include <iostream>

template <typename T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
    
    // 定義 > 運算子，以距離原點的平方和比較
    bool operator>(const Point& other) const {
        return (x*x + y*y) > (other.x*other.x + other.y*other.y);
    }
};

// 為了輸出
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

int main() {
    Point p1(1, 2), p2(3, 4);
    Point max_point = find_max(p1, p2);  // 現在可以了！
    std::cout << "max point: " << max_point << std::endl;
    
    return 0;
}
```

**輸出：**
```
max point: (3, 4)
```

---

## 2.6 STL 中的 Concept（C++20 正式支援）

在 C++20 之前，Concept 只是「文件上的約定」。C++20 正式引入了 `concept` 關鍵字：

```cpp
#include <iostream>
#include <concepts>

// 定義一個 Concept：可比較的型別
template <typename T>
concept Comparable = requires(T a, T b) {
    { a > b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
};

// 使用 Concept 約束模板
template <Comparable T>
T find_max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    std::cout << find_max(10, 20) << std::endl;      // OK
    std::cout << find_max(3.14, 2.72) << std::endl;  // OK
    
    return 0;
}
```

**輸出：**
```
20
3.14
```

現在你只需要知道 Concept 的**概念**，我們在後續課程會更深入探討。

---

## 2.7 泛型編程 vs 物件導向的多型

你可能會問：「這跟物件導向的繼承、虛擬函數有什麼不同？」

### 物件導向多型（執行期）

```cpp
#include <iostream>
#include <vector>
#include <memory>

// 基底類別
class Shape {
public:
    virtual double area() const = 0;
    virtual ~Shape() = default;
};

// 衍生類別
class Circle : public Shape {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const override {
        return width * height;
    }
};

int main() {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(5.0));
    shapes.push_back(std::make_unique<Rectangle>(4.0, 3.0));
    
    for (const auto& shape : shapes) {
        std::cout << "Area: " << shape->area() << std::endl;  // 執行期決定
    }
    
    return 0;
}
```

**輸出：**
```
Area: 78.5397
Area: 12
```

### 泛型編程多型（編譯期）

```cpp
#include <iostream>

class Circle {
    double radius;
public:
    Circle(double r) : radius(r) {}
    double area() const {
        return 3.14159 * radius * radius;
    }
};

class Rectangle {
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double area() const {
        return width * height;
    }
};

// 泛型函數：不需要繼承關係
template <typename Shape>
void print_area(const Shape& shape) {
    std::cout << "Area: " << shape.area() << std::endl;  // 編譯期決定
}

int main() {
    Circle c(5.0);
    Rectangle r(4.0, 3.0);
    
    print_area(c);  // 編譯期展開為 print_area(const Circle&)
    print_area(r);  // 編譯期展開為 print_area(const Rectangle&)
    
    return 0;
}
```

**輸出：**
```
Area: 78.5397
Area: 12
```

### 比較表

```
┌────────────────────────────────────────────────────────────────┐
│           物件導向多型 vs 泛型編程多型                          │
├──────────────────┬────────────────────┬────────────────────────┤
│      面向        │   物件導向多型      │     泛型編程多型        │
├──────────────────┼────────────────────┼────────────────────────┤
│ 決定時機         │ 執行期（Runtime）   │ 編譯期（Compile-time） │
├──────────────────┼────────────────────┼────────────────────────┤
│ 關係要求         │ 需要繼承關係        │ 不需要繼承關係          │
├──────────────────┼────────────────────┼────────────────────────┤
│ 效能             │ 虛擬函數表查詢開銷  │ 無額外開銷              │
├──────────────────┼────────────────────┼────────────────────────┤
│ 彈性             │ 可存放異質物件      │ 同一容器只能放同型別    │
├──────────────────┼────────────────────┼────────────────────────┤
│ 錯誤時機         │ 執行期才發現        │ 編譯期就能發現          │
├──────────────────┼────────────────────┼────────────────────────┤
│ 代表             │ virtual function   │ template               │
└──────────────────┴────────────────────┴────────────────────────┘
```

**STL 選擇了泛型編程**，原因是效能和編譯期錯誤檢查。

---

## 2.8 完整範例：泛型的 Stack

讓我們用所學的知識，實作一個泛型的 Stack：

```cpp
#include <iostream>
#include <stdexcept>
#include <string>

template <typename T>
class Stack {
private:
    static const int MAX_SIZE = 100;
    T data[MAX_SIZE];
    int top_index;
    
public:
    Stack() : top_index(-1) {}
    
    void push(const T& value) {
        if (top_index >= MAX_SIZE - 1) {
            throw std::overflow_error("Stack overflow");
        }
        data[++top_index] = value;
    }
    
    T pop() {
        if (is_empty()) {
            throw std::underflow_error("Stack underflow");
        }
        return data[top_index--];
    }
    
    T& top() {
        if (is_empty()) {
            throw std::underflow_error("Stack is empty");
        }
        return data[top_index];
    }
    
    bool is_empty() const {
        return top_index < 0;
    }
    
    int size() const {
        return top_index + 1;
    }
};

int main() {
    // int Stack
    Stack<int> int_stack;
    int_stack.push(10);
    int_stack.push(20);
    int_stack.push(30);
    
    std::cout << "=== Int Stack ===" << std::endl;
    std::cout << "Size: " << int_stack.size() << std::endl;
    std::cout << "Top: " << int_stack.top() << std::endl;
    
    while (!int_stack.is_empty()) {
        std::cout << "Pop: " << int_stack.pop() << std::endl;
    }
    
    // string Stack（同一份程式碼，不同型別）
    Stack<std::string> string_stack;
    string_stack.push("Hello");
    string_stack.push("World");
    string_stack.push("STL");
    
    std::cout << "\n=== String Stack ===" << std::endl;
    std::cout << "Size: " << string_stack.size() << std::endl;
    
    while (!string_stack.is_empty()) {
        std::cout << "Pop: " << string_stack.pop() << std::endl;
    }
    
    return 0;
}
```

**輸出：**
```
=== Int Stack ===
Size: 3
Top: 30
Pop: 30
Pop: 20
Pop: 10

=== String Stack ===
Size: 3
Pop: STL
Pop: World
Pop: Hello
```

---

## 2.9 本課重點整理

```
┌─────────────────────────────────────────────────────────────┐
│                     第二課 重點整理                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 泛型編程 = 撰寫與型別無關的程式碼                         │
│     → 核心工具：template                                    │
│                                                             │
│  2. 函數模板                                                 │
│     → template <typename T>                                 │
│     → 編譯器自動推導型別，或明確指定 func<Type>(...)         │
│                                                             │
│  3. 類別模板                                                 │
│     → template <typename T> class ClassName { ... };        │
│     → 使用時必須明確指定：ClassName<Type>                    │
│                                                             │
│  4. 模板實例化                                               │
│     → 編譯期展開，生成特定型別的程式碼                       │
│     → 無執行期效能損失                                       │
│                                                             │
│  5. Concept（概念）                                          │
│     → 型別必須滿足的條件                                     │
│     → 例如：支援 operator>、可複製、可比較                   │
│                                                             │
│  6. 泛型 vs 物件導向多型                                     │
│     → 泛型：編譯期、無繼承要求、零開銷                       │
│     → 物件導向：執行期、需繼承、有虛擬表開銷                 │
│                                                             │
│  7. STL 的基礎                                               │
│     → vector<T>、list<T> 都是類別模板                       │
│     → sort、find 都是函數模板                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 2.10 課後練習

試著思考或實作：

1. **思考題**：`std::vector<int>` 和 `std::vector<double>` 是同一個類別嗎？

2. **實作題**：寫一個泛型函數 `print_array`，可以印出任何型別的陣列內容。

提示：
```cpp
template <typename T>
void print_array(const T* arr, int size) {
    // 你的實作
}
```

---

準備好進入**第三課：STL 的六大組件概覽**了嗎？下一課我們會鳥瞰整個 STL 的架構，了解容器、迭代器、演算法、函數物件、配置器、配接器這六大組件如何協同工作。
*/



#include <iostream>
#include <stdexcept>
#include <string>

template <typename T>
class Stack {
private:
    static const int MAX_SIZE = 100;
    T data[MAX_SIZE];
    int top_index;
    
public:
    Stack() : top_index(-1) {}
    
    void push(const T& value) {
        if (top_index >= MAX_SIZE - 1) {
            throw std::overflow_error("Stack overflow");
        }
        data[++top_index] = value;
    }
    
    T pop() {
        if (is_empty()) {
            throw std::underflow_error("Stack underflow");
        }
        return data[top_index--];
    }
    
    T& top() {
        if (is_empty()) {
            throw std::underflow_error("Stack is empty");
        }
        return data[top_index];
    }
    
    bool is_empty() const {
        return top_index < 0;
    }
    
    int size() const {
        return top_index + 1;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 20. Valid Parentheses
//   題目：給一個只含 '(' ')' '{' '}' '[' ']' 的字串，判斷括號是否正確配對。
//   為什麼用到本主題：這是「同一個泛型 Stack，換一種元素型別」最自然的示範 ——
//         上面用了 Stack<int> 與 Stack<std::string>，這裡直接用 Stack<char>，
//         而 Stack 的原始碼一個字都不必改。
//         括號配對是堆疊的教科書級應用：遇到左括號就 push，
//         遇到右括號就檢查堆疊頂端是否為對應的左括號。
//   複雜度：時間 O(n)，空間 O(n)。
//   注意：本檔的 Stack 上限是 100，超過會丟 std::overflow_error；
//         LeetCode 原題字串長度上限為 10^4，正式提交應改用 std::stack。
// -----------------------------------------------------------------------------
bool isValidParentheses(const std::string& s) {
    Stack<char> st;
    for (char c : s) {
        if (c == '(' || c == '{' || c == '[') {
            st.push(c);
        } else {
            if (st.is_empty()) return false;   // 有右括號卻沒有待配對的左括號
            char open = st.pop();
            if ((c == ')' && open != '(') ||
                (c == '}' && open != '{') ||
                (c == ']' && open != '[')) {
                return false;                  // 括號型態不匹配
            }
        }
    }
    return st.is_empty();                      // 還有沒配對完的左括號就是無效
}

int main() {
    // int Stack
    Stack<int> int_stack;
    int_stack.push(10);
    int_stack.push(20);
    int_stack.push(30);
    
    std::cout << "=== Int Stack ===" << std::endl;
    std::cout << "Size: " << int_stack.size() << std::endl;
    std::cout << "Top: " << int_stack.top() << std::endl;
    
    while (!int_stack.is_empty()) {
        std::cout << "Pop: " << int_stack.pop() << std::endl;
    }
    
    // string Stack（同一份程式碼，不同型別）
    Stack<std::string> string_stack;
    string_stack.push("Hello");
    string_stack.push("World");
    string_stack.push("STL");
    
    std::cout << "\n=== String Stack ===" << std::endl;
    std::cout << "Size: " << string_stack.size() << std::endl;
    
    while (!string_stack.is_empty()) {
        std::cout << "Pop: " << string_stack.pop() << std::endl;
    }
    
    // 同一份 Stack 原始碼，第三種元素型別：char
    std::cout << "\n=== LeetCode 20. Valid Parentheses（Stack<char>）===" << std::endl;
    const char* cases[] = {"()", "()[]{}", "(]", "([)]", "{[]}", "(((", ""};
    for (const char* c : cases) {
        std::cout << "  \"" << c << "\" -> "
                  << (isValidParentheses(c) ? "true" : "false") << std::endl;
    }
    std::cout << "Stack 的原始碼一個字都沒改，只是換了型別引數。" << std::endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念13.cpp -o concept13

// === 預期輸出 ===
// === Int Stack ===
// Size: 3
// Top: 30
// Pop: 30
// Pop: 20
// Pop: 10
//
// === String Stack ===
// Size: 3
// Pop: STL
// Pop: World
// Pop: Hello
//
// === LeetCode 20. Valid Parentheses（Stack<char>）===
//   "()" -> true
//   "()[]{}" -> true
//   "(]" -> false
//   "([)]" -> false
//   "{[]}" -> true
//   "(((" -> false
//   "" -> true
// Stack 的原始碼一個字都沒改，只是換了型別引數。
