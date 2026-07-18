/*
# 第 1 課：C++ 的歷史與設計哲學

---

## 1.1 為什麼要學習 C++ 的歷史？

在深入學習 C++ 的語法和技術之前，理解它的歷史背景和設計哲學是非常重要的。這能幫助你明白「為什麼 C++ 要這樣設計」，而不只是「C++ 能做什麼」。

當你理解了設計者的思維，很多看似複雜或奇怪的語法規則都會變得合理。

---

## 1.2 C++ 的誕生背景

### 時間線

| 年份 | 事件 |
|------|------|
| 1972 | Dennis Ritchie 在貝爾實驗室創造了 C 語言 |
| 1979 | Bjarne Stroustrup 開始開發「C with Classes」 |
| 1983 | 正式更名為 C++ |
| 1985 | 第一版《The C++ Programming Language》出版 |
| 1998 | C++98 成為第一個 ISO 標準 |
| 2011 | C++11 發布（現代 C++ 的起點） |
| 2014 | C++14 發布 |
| 2017 | C++17 發布 |
| 2020 | C++20 發布 |
| 2023 | C++23 發布 |

### 創造者：Bjarne Stroustrup

Bjarne Stroustrup 是丹麥電腦科學家，當時在貝爾實驗室工作。他在博士研究期間使用過一種叫做 **Simula** 的語言，這是世界上第一個支援「類別」和「物件」概念的語言。

Simula 的優點是程式結構清晰、易於組織大型程式，但缺點是執行效率太慢。

另一方面，C 語言執行效率極高，但缺乏組織大型程式的機制。

Stroustrup 的想法很簡單：**能不能把 Simula 的組織能力加到 C 語言上，同時保持 C 的效率？**

這就是 C++ 誕生的核心動機。

---

## 1.3 名稱的由來

最初這個語言叫做 **「C with Classes」**（帶有類別的 C）。

1983 年，Rick Mascitti 建議改名為 **C++**。

這個名稱來自 C 語言的遞增運算子 `++`，意思是「C 的下一個版本」或「C 的進化」。

```c
int c = 1;
c++;  // c 變成 2
```

有趣的是，按照 C 語言的語義，`c++` 是「先使用 c 的值，再遞增」，所以有人開玩笑說 C++ 應該叫 `++C` 才對！

---

## 1.4 C++ 的設計哲學

Bjarne Stroustrup 在設計 C++ 時，有幾個核心原則：

### 原則一：不為你不用的東西付出代價（Zero-overhead Principle）

這是 C++ 最重要的設計原則。

意思是：如果你不使用某個功能，它不應該對你的程式產生任何額外的開銷（無論是執行時間還是記憶體）。

```cpp
// 如果你只需要簡單的 C 風格程式碼
// C++ 不會強迫你付出 OOP 的開銷
int add(int a, int b) {
    return a + b;
}

// 只有當你選擇使用類別時，才會有類別的機制
class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }
};
```

### 原則二：直接映射硬體（Direct Mapping to Hardware）

C++ 的抽象機制不應該阻止程式設計師直接操作硬體。你仍然可以使用指標、直接存取記憶體、進行位元操作。

```cpp
// C++ 保留了 C 的底層能力
int value = 42;
int* ptr = &value;       // 指標操作
*ptr = 100;              // 直接修改記憶體

unsigned char flags = 0b00001111;
flags = flags << 2;      // 位元操作
```

### 原則三：C 的相容性（C Compatibility）

C++ 被設計為 C 的超集。大部分合法的 C 程式碼也是合法的 C++ 程式碼。

這個設計讓已有的大量 C 程式碼可以逐步遷移到 C++，而不是完全重寫。

```cpp
// 這段程式碼在 C 和 C++ 中都能編譯運行
#include <stdio.h>

int main() {
    printf("Hello from C style!\n");
    return 0;
}
```

### 原則四：提供高階抽象，但不強制使用

C++ 提供了類別、繼承、多型、模板等高階功能，但這些都是可選的。你可以根據需要選擇使用。

---

## 1.5 C++ 是多範式語言

C++ 支援多種程式設計範式（programming paradigms）：

| 範式 | 說明 | 範例 |
|------|------|------|
| 程序式（Procedural） | 使用函數組織程式碼，類似 C | 函數、流程控制 |
| 物件導向（Object-Oriented） | 使用類別和物件組織程式碼 | class、繼承、多型 |
| 泛型（Generic） | 使用模板寫出適用於多種類型的程式碼 | template |
| 函數式（Functional） | 使用函數作為一等公民 | lambda、std::function |

這意味著你可以根據問題的性質，選擇最適合的方式來解決問題。

---

## 1.6 完整範例程式

讓我們寫一個程式，展示 C++ 如何在一個程式中融合不同的程式設計風格：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// ========================================
// 程序式風格（Procedural Style）
// ========================================
int add(int a, int b) {
    return a + b;
}

// ========================================
// 物件導向風格（Object-Oriented Style）
// ========================================
class Calculator {
private:
    int lastResult;  // 私有成員變數
    
public:
    Calculator() : lastResult(0) {}  // 建構函數
    
    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }
    
    int getLastResult() const {
        return lastResult;
    }
};

// ========================================
// 泛型風格（Generic Style）
// ========================================
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// ========================================
// 主程式
// ========================================
int main() {
    std::cout << "=== C++ 多範式展示 ===" << std::endl;
    std::cout << std::endl;
    
    // 1. 程序式風格
    std::cout << "[程序式風格]" << std::endl;
    int sum = add(3, 5);
    std::cout << "add(3, 5) = " << sum << std::endl;
    std::cout << std::endl;
    
    // 2. 物件導向風格
    std::cout << "[物件導向風格]" << std::endl;
    Calculator calc;
    int product = calc.multiply(4, 6);
    std::cout << "calc.multiply(4, 6) = " << product << std::endl;
    std::cout << "calc.getLastResult() = " << calc.getLastResult() << std::endl;
    std::cout << std::endl;
    
    // 3. 泛型風格
    std::cout << "[泛型風格]" << std::endl;
    std::cout << "getMax(10, 20) = " << getMax(10, 20) << std::endl;
    std::cout << "getMax(3.14, 2.71) = " << getMax(3.14, 2.71) << std::endl;
    std::cout << std::endl;
    
    // 4. 函數式風格（使用 lambda）
    std::cout << "[函數式風格]" << std::endl;
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3};
    
    // 使用 lambda 表達式排序
    std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
        return a < b;  // 升序排列
    });
    
    std::cout << "排序後的數字: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++11 -o lesson01 lesson01.cpp
./lesson01
```

### 執行結果

```
=== C++ 多範式展示 ===

[程序式風格]
add(3, 5) = 8

[物件導向風格]
calc.multiply(4, 6) = 24
calc.getLastResult() = 24

[泛型風格]
getMax(10, 20) = 20
getMax(3.14, 2.71) = 3.14

[函數式風格]
排序後的數字: 1 2 3 5 8 9
```

---

## 1.7 程式碼解析

### 程序式部分

```cpp
int add(int a, int b) {
    return a + b;
}
```

這就是純 C 風格的函數，沒有任何物件導向的元素。C++ 完全支援這種寫法。

### 物件導向部分

```cpp
class Calculator {
private:
    int lastResult;
    
public:
    Calculator() : lastResult(0) {}
    
    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }
};
```

這裡展示了：
- `class` 關鍵字定義類別
- `private` 和 `public` 控制存取權限
- 建構函數初始化物件
- 成員函數操作資料

這些概念我們在後續課程會詳細講解。

### 泛型部分

```cpp
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}
```

這個函數模板可以接受任何類型（int、double、甚至自定義類別），只要該類型支援 `>` 運算子。

### 函數式部分

```cpp
std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
    return a < b;
});
```

`[](int a, int b) { return a < b; }` 是一個 **lambda 表達式**，它是一個匿名函數，可以直接傳遞給其他函數使用。

---

## 1.8 本課重點回顧

| 概念 | 說明 |
|------|------|
| C++ 創造者 | Bjarne Stroustrup（1979 年開始開發） |
| 設計動機 | 結合 C 的效率和 Simula 的組織能力 |
| 名稱由來 | `++` 是 C 的遞增運算子，表示「C 的進化」 |
| 零開銷原則 | 不使用的功能不產生開銷 |
| 多範式 | 支援程序式、物件導向、泛型、函數式 |
| C 相容性 | 大部分 C 程式碼也是合法的 C++ 程式碼 |

---

## 1.9 思考問題

1. 為什麼 Stroustrup 選擇在 C 的基礎上擴展，而不是創造一個全新的語言？
2. 「零開銷原則」對程式設計師有什麼實際意義？
3. 多範式語言有什麼優點和缺點？

---

準備好了嗎？下一課我們將深入探討 **C 與 C++ 的關鍵差異**，幫助你從 C 程式設計師順利過渡到 C++ 程式設計師。

要繼續進入 **第 2 課** 嗎？
*/



#include <iostream>
#include <vector>
#include <algorithm>

// ========================================
// 程序式風格（Procedural Style）
// ========================================
int add(int a, int b) {
    return a + b;
}

// ========================================
// 物件導向風格（Object-Oriented Style）
// ========================================
class Calculator {
private:
    int lastResult;  // 私有成員變數
    
public:
    Calculator() : lastResult(0) {}  // 建構函數
    
    int multiply(int a, int b) {
        lastResult = a * b;
        return lastResult;
    }
    
    int getLastResult() const {
        return lastResult;
    }
};

// ========================================
// 泛型風格（Generic Style）
// ========================================
template<typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// ========================================
// 主程式
// ========================================
int main() {
    std::cout << "=== C++ 多範式展示 ===" << std::endl;
    std::cout << std::endl;
    
    // 1. 程序式風格
    std::cout << "[程序式風格]" << std::endl;
    int sum = add(3, 5);
    std::cout << "add(3, 5) = " << sum << std::endl;
    std::cout << std::endl;
    
    // 2. 物件導向風格
    std::cout << "[物件導向風格]" << std::endl;
    Calculator calc;
    int product = calc.multiply(4, 6);
    std::cout << "calc.multiply(4, 6) = " << product << std::endl;
    std::cout << "calc.getLastResult() = " << calc.getLastResult() << std::endl;
    std::cout << std::endl;
    
    // 3. 泛型風格
    std::cout << "[泛型風格]" << std::endl;
    std::cout << "getMax(10, 20) = " << getMax(10, 20) << std::endl;
    std::cout << "getMax(3.14, 2.71) = " << getMax(3.14, 2.71) << std::endl;
    std::cout << std::endl;
    
    // 4. 函數式風格（使用 lambda）
    std::cout << "[函數式風格]" << std::endl;
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3};
    
    // 使用 lambda 表達式排序
    std::sort(numbers.begin(), numbers.end(), [](int a, int b) {
        return a < b;  // 升序排列
    });
    
    std::cout << "排序後的數字: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
