/*
# 第八課：函數物件（Function Object）初探

---

## 8.1 什麼是函數物件？

**函數物件**（Function Object），也稱為 **Functor**，是一個**重載了 `operator()` 的類別實例**。

簡單來說：它是一個「可以像函數一樣被呼叫的物件」。

```cpp
#include <iostream>

// 這是一個函數物件類別
class Adder {
public:
    int operator()(int a, int b) const {
        return a + b;
    }
};

int main() {
    Adder add;  // 建立一個物件
    
    // 像呼叫函數一樣使用物件
    int result = add(3, 5);  // 實際上是 add.operator()(3, 5)
    
    std::cout << "3 + 5 = " << result << std::endl;
    
    return 0;
}
```

**輸出：**
```
3 + 5 = 8
```

---

## 8.2 為什麼需要函數物件？

### 問題：演算法需要「行為客製化」

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3};
    
    // sort 預設是升序
    std::sort(vec.begin(), vec.end());
    
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 如果想要降序呢？
    // sort 需要一個「告訴它如何比較」的東西
    
    return 0;
}
```

我們需要一種方式告訴演算法「如何操作」。這就是函數物件的用途。

### 三種提供「行為」的方式

```
┌─────────────────────────────────────────────────────────────────┐
│                 提供行為的三種方式                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 普通函數                                                    │
│     bool compare(int a, int b) { return a > b; }                │
│     std::sort(v.begin(), v.end(), compare);                     │
│                                                                 │
│  2. 函數物件（Functor）                                         │
│     class Compare {                                             │
│         bool operator()(int a, int b) { return a > b; }         │
│     };                                                          │
│     std::sort(v.begin(), v.end(), Compare());                   │
│                                                                 │
│  3. Lambda 表達式（C++11）                                      │
│     std::sort(v.begin(), v.end(), [](int a, int b) {            │
│         return a > b;                                           │
│     });                                                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8.3 函數物件 vs 普通函數

### 普通函數的寫法

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// 普通函數
bool is_even(int n) {
    return n % 2 == 0;
}

bool greater_than(int a, int b) {
    return a > b;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 用普通函數作為謂詞
    int even_count = std::count_if(vec.begin(), vec.end(), is_even);
    std::cout << "偶數個數: " << even_count << std::endl;
    
    // 用普通函數作為比較器
    std::sort(vec.begin(), vec.end(), greater_than);
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
偶數個數: 5
降序: 10 9 8 7 6 5 4 3 2 1 
```

### 函數物件的優勢：可以攜帶狀態

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// 普通函數：無法攜帶狀態
// 如果要判斷「大於 N」，N 從哪來？
// bool greater_than_n(int x) { return x > ???; }

// 函數物件：可以攜帶狀態！
class GreaterThan {
private:
    int threshold;
    
public:
    GreaterThan(int t) : threshold(t) {}
    
    bool operator()(int x) const {
        return x > threshold;
    }
};

class IsDivisibleBy {
private:
    int divisor;
    
public:
    IsDivisibleBy(int d) : divisor(d) {}
    
    bool operator()(int x) const {
        return x % divisor == 0;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 使用帶狀態的函數物件
    int count_gt_5 = std::count_if(vec.begin(), vec.end(), GreaterThan(5));
    std::cout << "大於 5 的個數: " << count_gt_5 << std::endl;
    
    int count_gt_7 = std::count_if(vec.begin(), vec.end(), GreaterThan(7));
    std::cout << "大於 7 的個數: " << count_gt_7 << std::endl;
    
    int count_div_3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3 的倍數個數: " << count_div_3 << std::endl;
    
    int count_div_4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(4));
    std::cout << "4 的倍數個數: " << count_div_4 << std::endl;
    
    return 0;
}
```

**輸出：**
```
大於 5 的個數: 5
大於 7 的個數: 3
3 的倍數個數: 3
4 的倍數個數: 2
```

---

## 8.4 函數物件的完整結構

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

class Counter {
private:
    int count_;   // 狀態：計數器
    int target_;  // 狀態：目標值
    
public:
    // 建構子：初始化狀態
    Counter(int target) : count_(0), target_(target) {}
    
    // operator()：讓物件可以像函數一樣被呼叫
    bool operator()(int x) {
        if (x == target_) {
            ++count_;
            return true;
        }
        return false;
    }
    
    // 取得累積的狀態
    int get_count() const { return count_; }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 2, 4, 2, 5, 2, 6};
    
    // 建立函數物件
    Counter counter(2);
    
    // 傳給演算法（注意：for_each 會回傳函數物件的副本）
    Counter result = std::for_each(vec.begin(), vec.end(), counter);
    
    std::cout << "找到 2 的次數: " << result.get_count() << std::endl;
    
    return 0;
}
```

**輸出：**
```
找到 2 的次數: 4
```

### 函數物件的結構圖

```
┌─────────────────────────────────────────────────────────────────┐
│                    函數物件結構                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   class MyFunctor {                                             │
│   private:                                                      │
│       // 狀態（成員變數）                                       │
│       int state1;                                               │
│       double state2;                                            │
│                                                                 │
│   public:                                                       │
│       // 建構子：初始化狀態                                     │
│       MyFunctor(int s1, double s2) : state1(s1), state2(s2) {}  │
│                                                                 │
│       // operator()：核心功能                                   │
│       ReturnType operator()(ParamType param) const {            │
│           // 使用 state1, state2 和 param 進行運算             │
│           return result;                                        │
│       }                                                         │
│                                                                 │
│       // 可選：存取狀態的方法                                   │
│       int get_state1() const { return state1; }                 │
│   };                                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8.5 STL 內建的函數物件

STL 在 `<functional>` 標頭檔提供了許多現成的函數物件。

### 8.5.1 算術類函數物件

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

int main() {
    std::cout << "=== 算術類函數物件 ===" << std::endl;
    
    // plus<T>：加法
    std::plus<int> add;
    std::cout << "plus: 3 + 5 = " << add(3, 5) << std::endl;
    
    // minus<T>：減法
    std::minus<int> subtract;
    std::cout << "minus: 10 - 3 = " << subtract(10, 3) << std::endl;
    
    // multiplies<T>：乘法
    std::multiplies<int> multiply;
    std::cout << "multiplies: 4 * 6 = " << multiply(4, 6) << std::endl;
    
    // divides<T>：除法
    std::divides<int> divide;
    std::cout << "divides: 20 / 4 = " << divide(20, 4) << std::endl;
    
    // modulus<T>：取餘數
    std::modulus<int> mod;
    std::cout << "modulus: 17 % 5 = " << mod(17, 5) << std::endl;
    
    // negate<T>：取負值
    std::negate<int> neg;
    std::cout << "negate: -7 = " << neg(7) << std::endl;
    
    // 實際應用：計算乘積
    std::cout << "\n=== 實際應用 ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    int product = std::accumulate(vec.begin(), vec.end(), 1, 
                                   std::multiplies<int>());
    std::cout << "1 * 2 * 3 * 4 * 5 = " << product << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 算術類函數物件 ===
plus: 3 + 5 = 8
minus: 10 - 3 = 7
multiplies: 4 * 6 = 24
divides: 20 / 4 = 5
modulus: 17 % 5 = 2
negate: -7 = -7

=== 實際應用 ===
1 * 2 * 3 * 4 * 5 = 120
```

### 8.5.2 比較類函數物件

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    std::cout << "=== 比較類函數物件 ===" << std::endl;
    
    // equal_to<T>：相等
    std::equal_to<int> eq;
    std::cout << "equal_to: 5 == 5? " << (eq(5, 5) ? "true" : "false") << std::endl;
    
    // not_equal_to<T>：不相等
    std::not_equal_to<int> neq;
    std::cout << "not_equal_to: 5 != 3? " << (neq(5, 3) ? "true" : "false") << std::endl;
    
    // greater<T>：大於
    std::greater<int> gt;
    std::cout << "greater: 5 > 3? " << (gt(5, 3) ? "true" : "false") << std::endl;
    
    // less<T>：小於
    std::less<int> lt;
    std::cout << "less: 3 < 5? " << (lt(3, 5) ? "true" : "false") << std::endl;
    
    // greater_equal<T>：大於等於
    std::greater_equal<int> gte;
    std::cout << "greater_equal: 5 >= 5? " << (gte(5, 5) ? "true" : "false") << std::endl;
    
    // less_equal<T>：小於等於
    std::less_equal<int> lte;
    std::cout << "less_equal: 3 <= 5? " << (lte(3, 5) ? "true" : "false") << std::endl;
    
    // 實際應用：降序排序
    std::cout << "\n=== 實際應用：排序 ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3};
    
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    std::sort(vec.begin(), vec.end(), std::less<int>());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 比較類函數物件 ===
equal_to: 5 == 5? true
not_equal_to: 5 != 3? true
greater: 5 > 3? true
less: 3 < 5? true
greater_equal: 5 >= 5? true
less_equal: 3 <= 5? true

=== 實際應用：排序 ===
降序: 9 8 5 3 2 1 
升序: 1 2 3 5 8 9 
```

### 8.5.3 邏輯類函數物件

```cpp
#include <iostream>
#include <functional>

int main() {
    std::cout << "=== 邏輯類函數物件 ===" << std::endl;
    
    // logical_and<T>：邏輯 AND
    std::logical_and<bool> land;
    std::cout << "logical_and: true && false = " << (land(true, false) ? "true" : "false") << std::endl;
    std::cout << "logical_and: true && true = " << (land(true, true) ? "true" : "false") << std::endl;
    
    // logical_or<T>：邏輯 OR
    std::logical_or<bool> lor;
    std::cout << "logical_or: true || false = " << (lor(true, false) ? "true" : "false") << std::endl;
    std::cout << "logical_or: false || false = " << (lor(false, false) ? "true" : "false") << std::endl;
    
    // logical_not<T>：邏輯 NOT
    std::logical_not<bool> lnot;
    std::cout << "logical_not: !true = " << (lnot(true) ? "true" : "false") << std::endl;
    std::cout << "logical_not: !false = " << (lnot(false) ? "true" : "false") << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 邏輯類函數物件 ===
logical_and: true && false = false
logical_and: true && true = true
logical_or: true || false = true
logical_or: false || false = false
logical_not: !true = false
logical_not: !false = true
```

### STL 函數物件總覽

```
┌─────────────────────────────────────────────────────────────────┐
│                   STL 內建函數物件總覽                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  算術類（<functional>）                                         │
│  ├── plus<T>          x + y                                    │
│  ├── minus<T>         x - y                                    │
│  ├── multiplies<T>    x * y                                    │
│  ├── divides<T>       x / y                                    │
│  ├── modulus<T>       x % y                                    │
│  └── negate<T>        -x                                       │
│                                                                 │
│  比較類（<functional>）                                         │
│  ├── equal_to<T>      x == y                                   │
│  ├── not_equal_to<T>  x != y                                   │
│  ├── greater<T>       x > y                                    │
│  ├── less<T>          x < y                                    │
│  ├── greater_equal<T> x >= y                                   │
│  └── less_equal<T>    x <= y                                   │
│                                                                 │
│  邏輯類（<functional>）                                         │
│  ├── logical_and<T>   x && y                                   │
│  ├── logical_or<T>    x || y                                   │
│  └── logical_not<T>   !x                                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8.6 Lambda 表達式：現代的函數物件

C++11 引入了 **Lambda 表達式**，讓你可以「就地」定義函數物件，不需要寫完整的類別。

### Lambda 基本語法

```
[捕獲列表](參數列表) -> 回傳型別 { 函數主體 }

簡化形式：
[捕獲列表](參數列表) { 函數主體 }
```

### 基本範例

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 最簡單的 Lambda
    std::cout << "=== 基本 Lambda ===" << std::endl;
    auto print = [](int n) { std::cout << n << " "; };
    
    std::for_each(vec.begin(), vec.end(), print);
    std::cout << std::endl;
    
    // 直接內聯使用
    std::cout << "\n=== 內聯 Lambda ===" << std::endl;
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n * n << " ";
    });
    std::cout << std::endl;
    
    // 帶回傳值的 Lambda
    std::cout << "\n=== 帶回傳值的 Lambda ===" << std::endl;
    auto square = [](int n) { return n * n; };
    std::cout << "5 的平方: " << square(5) << std::endl;
    
    // 用在 count_if
    std::cout << "\n=== 用在 count_if ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 基本 Lambda ===
1 2 3 4 5 6 7 8 9 10 

=== 內聯 Lambda ===
1 4 9 16 25 36 49 64 81 100 

=== 帶回傳值的 Lambda ===
5 的平方: 25

=== 用在 count_if ===
偶數個數: 5
```

---

## 8.7 Lambda 的捕獲（Capture）

Lambda 的真正威力在於「捕獲」外部變數。

### 8.7.1 捕獲方式總覽

```
┌─────────────────────────────────────────────────────────────────┐
│                    Lambda 捕獲方式                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  []        不捕獲任何變數                                       │
│  [x]       以值捕獲 x（複製一份）                               │
│  [&x]      以參考捕獲 x                                         │
│  [=]       以值捕獲所有外部變數                                 │
│  [&]       以參考捕獲所有外部變數                               │
│  [=, &x]   預設以值捕獲，但 x 以參考捕獲                        │
│  [&, x]    預設以參考捕獲，但 x 以值捕獲                        │
│  [this]    捕獲 this 指標（類別成員函數中）                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 8.7.2 值捕獲 vs 參考捕獲

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    int threshold = 5;
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 值捕獲 [threshold] 或 [=]
    std::cout << "=== 值捕獲 ===" << std::endl;
    int count1 = std::count_if(vec.begin(), vec.end(),
        [threshold](int n) { return n > threshold; });
    std::cout << "大於 " << threshold << " 的個數: " << count1 << std::endl;
    
    // 值捕獲是「複製」，Lambda 內部的修改不影響外部
    auto by_value = [threshold]() mutable {  // mutable 允許修改捕獲的副本
        threshold = 100;  // 修改的是副本
        return threshold;
    };
    by_value();
    std::cout << "外部 threshold 仍然是: " << threshold << std::endl;
    
    // 參考捕獲 [&threshold] 或 [&]
    std::cout << "\n=== 參考捕獲 ===" << std::endl;
    int sum = 0;
    std::for_each(vec.begin(), vec.end(),
        [&sum](int n) { sum += n; });  // sum 以參考捕獲
    std::cout << "總和: " << sum << std::endl;
    
    // 參考捕獲可以修改外部變數
    int multiplier = 2;
    auto by_ref = [&multiplier]() {
        multiplier = 10;  // 修改外部變數
    };
    by_ref();
    std::cout << "multiplier 被修改為: " << multiplier << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 值捕獲 ===
大於 5 的個數: 5
外部 threshold 仍然是: 5

=== 參考捕獲 ===
總和: 55
multiplier 被修改為: 10
```

### 8.7.3 混合捕獲

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    int a = 10;
    int b = 20;
    int c = 30;
    
    // 預設以值捕獲，但 b 以參考捕獲
    auto lambda1 = [=, &b]() {
        // a 是副本，b 是參考，c 是副本
        b = a + c;  // 可以修改 b
        // a = 100;  // 錯誤！a 是值捕獲，不能修改（除非 mutable）
    };
    
    lambda1();
    std::cout << "a = " << a << ", b = " << b << ", c = " << c << std::endl;
    
    // 預設以參考捕獲，但 a 以值捕獲
    auto lambda2 = [&, a]() {
        // a 是副本，b 和 c 是參考
        b = 100;
        c = 200;
        // a 不能修改（值捕獲）
    };
    
    lambda2();
    std::cout << "a = " << a << ", b = " << b << ", c = " << c << std::endl;
    
    return 0;
}
```

**輸出：**
```
a = 10, b = 40, c = 30
a = 10, b = 100, c = 200
```

---

## 8.8 Lambda 與函數物件的等價性

Lambda 實際上就是編譯器自動生成的函數物件類別。

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// 手寫的函數物件
class GreaterThanFunctor {
private:
    int threshold;
public:
    GreaterThanFunctor(int t) : threshold(t) {}
    bool operator()(int n) const {
        return n > threshold;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int threshold = 5;
    
    // 方法一：手寫函數物件
    int count1 = std::count_if(vec.begin(), vec.end(), 
                               GreaterThanFunctor(threshold));
    
    // 方法二：Lambda（編譯器自動生成類似的類別）
    int count2 = std::count_if(vec.begin(), vec.end(),
        [threshold](int n) { return n > threshold; });
    
    std::cout << "函數物件結果: " << count1 << std::endl;
    std::cout << "Lambda 結果: " << count2 << std::endl;
    
    // 它們是等價的！
    
    return 0;
}
```

**輸出：**
```
函數物件結果: 5
Lambda 結果: 5
```

### Lambda 的內部實現

```
┌─────────────────────────────────────────────────────────────────┐
│              Lambda 的編譯器實現（概念上）                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   你寫的 Lambda：                                               │
│   [threshold](int n) { return n > threshold; }                  │
│                                                                 │
│   編譯器生成的類別（大致上）：                                  │
│   class __lambda_1 {                                            │
│   private:                                                      │
│       int threshold;  // 值捕獲的變數                           │
│   public:                                                       │
│       __lambda_1(int t) : threshold(t) {}                       │
│       bool operator()(int n) const {                            │
│           return n > threshold;                                 │
│       }                                                         │
│   };                                                            │
│                                                                 │
│   Lambda 表達式被替換成：                                       │
│   __lambda_1(threshold)                                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8.9 mutable Lambda

預設情況下，以值捕獲的變數在 Lambda 內部是 `const` 的。使用 `mutable` 可以修改。

```cpp
#include <iostream>

int main() {
    int counter = 0;
    
    // 錯誤：預設情況下不能修改值捕獲的變數
    // auto increment = [counter]() { return ++counter; };  // 編譯錯誤
    
    // 使用 mutable
    auto increment = [counter]() mutable {
        return ++counter;  // 修改的是 Lambda 內部的副本
    };
    
    std::cout << "increment(): " << increment() << std::endl;  // 1
    std::cout << "increment(): " << increment() << std::endl;  // 2
    std::cout << "increment(): " << increment() << std::endl;  // 3
    std::cout << "外部 counter: " << counter << std::endl;     // 0（未被修改）
    
    return 0;
}
```

**輸出：**
```
increment(): 1
increment(): 2
increment(): 3
外部 counter: 0
```

---

## 8.10 泛型 Lambda（C++14）

C++14 允許 Lambda 的參數使用 `auto`，實現泛型。

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

int main() {
    // 泛型 Lambda：參數用 auto
    auto print = [](const auto& x) {
        std::cout << x << " ";
    };
    
    std::cout << "=== 泛型 Lambda ===" << std::endl;
    
    // 用在 int
    std::vector<int> ints = {1, 2, 3, 4, 5};
    std::cout << "ints: ";
    std::for_each(ints.begin(), ints.end(), print);
    std::cout << std::endl;
    
    // 用在 double
    std::vector<double> doubles = {1.1, 2.2, 3.3};
    std::cout << "doubles: ";
    std::for_each(doubles.begin(), doubles.end(), print);
    std::cout << std::endl;
    
    // 用在 string
    std::vector<std::string> strings = {"Hello", "World"};
    std::cout << "strings: ";
    std::for_each(strings.begin(), strings.end(), print);
    std::cout << std::endl;
    
    // 泛型比較
    auto compare = [](const auto& a, const auto& b) {
        return a < b;
    };
    
    std::cout << "\n=== 泛型比較 ===" << std::endl;
    std::cout << "compare(3, 5): " << compare(3, 5) << std::endl;
    std::cout << "compare(3.14, 2.72): " << compare(3.14, 2.72) << std::endl;
    std::cout << "compare(\"apple\", \"banana\"): " 
              << compare(std::string("apple"), std::string("banana")) << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 泛型 Lambda ===
ints: 1 2 3 4 5 
doubles: 1.1 2.2 3.3 
strings: Hello World 

=== 泛型比較 ===
compare(3, 5): 1
compare(3.14, 2.72): 0
compare("apple", "banana"): 1
```

---

## 8.11 std::function：儲存可呼叫物件

`std::function` 是一個通用的可呼叫物件包裝器，可以儲存函數、函數物件、Lambda。

```cpp
#include <iostream>
#include <functional>
#include <vector>

// 普通函數
int add(int a, int b) {
    return a + b;
}

// 函數物件
class Multiplier {
public:
    int operator()(int a, int b) const {
        return a * b;
    }
};

int main() {
    // std::function 可以儲存各種可呼叫物件
    std::function<int(int, int)> func;
    
    // 儲存普通函數
    func = add;
    std::cout << "普通函數: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 儲存函數物件
    func = Multiplier();
    std::cout << "函數物件: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 儲存 Lambda
    func = [](int a, int b) { return a - b; };
    std::cout << "Lambda: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 實際應用：儲存多個操作
    std::cout << "\n=== 儲存多個操作 ===" << std::endl;
    std::vector<std::function<int(int, int)>> operations;
    
    operations.push_back(add);
    operations.push_back(Multiplier());
    operations.push_back([](int a, int b) { return a - b; });
    operations.push_back([](int a, int b) { return a / b; });
    
    int a = 20, b = 4;
    std::vector<std::string> names = {"加", "乘", "減", "除"};
    
    for (size_t i = 0; i < operations.size(); ++i) {
        std::cout << a << " " << names[i] << " " << b << " = " 
                  << operations[i](a, b) << std::endl;
    }
    
    return 0;
}
```

**輸出：**
```
普通函數: func(3, 5) = 8
函數物件: func(3, 5) = 15
Lambda: func(3, 5) = -2

=== 儲存多個操作 ===
20 加 4 = 24
20 乘 4 = 80
20 減 4 = 16
20 除 4 = 5
```

---

## 8.12 實戰範例：使用函數物件和 Lambda

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <functional>

struct Product {
    std::string name;
    double price;
    int quantity;
};

int main() {
    std::vector<Product> products = {
        {"Apple", 1.50, 100},
        {"Banana", 0.75, 150},
        {"Orange", 2.00, 80},
        {"Mango", 3.00, 50},
        {"Grape", 2.50, 120}
    };
    
    // 1. 按價格排序（使用 Lambda）
    std::cout << "=== 按價格排序 ===" << std::endl;
    std::sort(products.begin(), products.end(),
        [](const Product& a, const Product& b) {
            return a.price < b.price;
        });
    
    for (const auto& p : products) {
        std::cout << p.name << ": $" << p.price << std::endl;
    }
    
    // 2. 找出價格超過 2 元的產品
    std::cout << "\n=== 價格超過 $2 ===" << std::endl;
    double threshold = 2.0;
    
    auto it = std::find_if(products.begin(), products.end(),
        [threshold](const Product& p) { return p.price > threshold; });
    
    while (it != products.end()) {
        std::cout << it->name << ": $" << it->price << std::endl;
        it = std::find_if(++it, products.end(),
            [threshold](const Product& p) { return p.price > threshold; });
    }
    
    // 3. 計算總庫存價值
    std::cout << "\n=== 總庫存價值 ===" << std::endl;
    double total_value = std::accumulate(products.begin(), products.end(), 0.0,
        [](double sum, const Product& p) {
            return sum + p.price * p.quantity;
        });
    std::cout << "總價值: $" << total_value << std::endl;
    
    // 4. 統計低庫存產品（數量 < 100）
    std::cout << "\n=== 低庫存產品 ===" << std::endl;
    int low_stock_count = std::count_if(products.begin(), products.end(),
        [](const Product& p) { return p.quantity < 100; });
    std::cout << "低庫存產品數: " << low_stock_count << std::endl;
    
    // 5. 對所有產品打 9 折
    std::cout << "\n=== 打 9 折後 ===" << std::endl;
    double discount = 0.9;
    std::for_each(products.begin(), products.end(),
        [discount](Product& p) { p.price *= discount; });
    
    for (const auto& p : products) {
        std::cout << p.name << ": $" << p.price << std::endl;
    }
    
    // 6. 使用 transform 生成價格列表
    std::cout << "\n=== 價格列表 ===" << std::endl;
    std::vector<double> prices(products.size());
    std::transform(products.begin(), products.end(), prices.begin(),
        [](const Product& p) { return p.price; });
    
    std::cout << "價格: ";
    for (double price : prices) {
        std::cout << "$" << price << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 按價格排序 ===
Banana: $0.75
Apple: $1.5
Orange: $2
Grape: $2.5
Mango: $3

=== 價格超過 $2 ===
Grape: $2.5
Mango: $3

=== 總庫存價值 ===
總價值: $747.5

=== 低庫存產品 ===
低庫存產品數: 2

=== 打 9 折後 ===
Banana: $0.675
Apple: $1.35
Orange: $1.8
Grape: $2.25
Mango: $2.7

=== 價格列表 ===
價格: $0.675 $1.35 $1.8 $2.25 $2.7 
```

---

## 8.13 本課重點整理

```
┌─────────────────────────────────────────────────────────────────┐
│                      第八課 重點整理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 函數物件（Functor）                                         │
│     • 重載 operator() 的類別                                    │
│     • 可以像函數一樣被呼叫                                      │
│     • 優勢：可以攜帶狀態                                        │
│                                                                 │
│  2. STL 內建函數物件（<functional>）                            │
│     • 算術類：plus, minus, multiplies, divides, negate          │
│     • 比較類：greater, less, equal_to 等                        │
│     • 邏輯類：logical_and, logical_or, logical_not              │
│                                                                 │
│  3. Lambda 表達式                                               │
│     • 語法：[捕獲](參數) { 主體 }                               │
│     • 是編譯器自動生成的函數物件類別                            │
│     • 現代 C++ 推薦使用                                         │
│                                                                 │
│  4. Lambda 捕獲                                                 │
│     • [x]   值捕獲（複製）                                      │
│     • [&x]  參考捕獲                                            │
│     • [=]   全部值捕獲                                          │
│     • [&]   全部參考捕獲                                        │
│     • 可混合：[=, &x] 或 [&, x]                                 │
│                                                                 │
│  5. mutable Lambda                                              │
│     • 預設情況下值捕獲的變數是 const                            │
│     • 使用 mutable 允許修改副本                                 │
│                                                                 │
│  6. 泛型 Lambda（C++14）                                        │
│     • 參數使用 auto                                             │
│     • 可用於不同型別                                            │
│                                                                 │
│  7. std::function                                               │
│     • 通用可呼叫物件包裝器                                      │
│     • 可儲存函數、函數物件、Lambda                              │
│                                                                 │
│  8. 使用建議                                                    │
│     • 簡單情況用 Lambda                                         │
│     • 複雜狀態用自訂函數物件                                    │
│     • STL 內建函數物件適合常見操作                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8.14 課後練習

1. **思考題**：Lambda 的值捕獲和參考捕獲各有什麼優缺點？什麼時候該用哪種？

2. **實作題**：寫一個函數物件 `InRange`，用來判斷一個數字是否在指定範圍內：
   ```cpp
   InRange checker(10, 20);  // 範圍 [10, 20]
   checker(15);  // true
   checker(5);   // false
   checker(25);  // false
   ```

3. **實作題**：使用 Lambda 和 STL 演算法，對一個字串 vector：
   - 過濾出長度大於 5 的字串
   - 將它們轉成大寫
   - 按長度排序

---

這一課完成了 **第一階段：STL 基礎概念**！

我們已經學習了：
- STL 的歷史與設計哲學
- 泛型編程的概念
- STL 的六大組件
- 迭代器的核心概念和五種分類
- 容器的概念與分類
- 演算法與容器的分離設計
- 函數物件和 Lambda

現在你已經對 STL 有了完整的基礎認識，準備好進入**第二階段：序列容器 - vector**了嗎？
*/



#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <functional>

struct Product {
    std::string name;
    double price;
    int quantity;
};

int main() {
    std::vector<Product> products = {
        {"Apple", 1.50, 100},
        {"Banana", 0.75, 150},
        {"Orange", 2.00, 80},
        {"Mango", 3.00, 50},
        {"Grape", 2.50, 120}
    };
    
    // 1. 按價格排序（使用 Lambda）
    std::cout << "=== 按價格排序 ===" << std::endl;
    std::sort(products.begin(), products.end(),
        [](const Product& a, const Product& b) {
            return a.price < b.price;
        });
    
    for (const auto& p : products) {
        std::cout << p.name << ": $" << p.price << std::endl;
    }
    
    // 2. 找出價格超過 2 元的產品
    std::cout << "\n=== 價格超過 $2 ===" << std::endl;
    double threshold = 2.0;
    
    auto it = std::find_if(products.begin(), products.end(),
        [threshold](const Product& p) { return p.price > threshold; });
    
    while (it != products.end()) {
        std::cout << it->name << ": $" << it->price << std::endl;
        it = std::find_if(++it, products.end(),
            [threshold](const Product& p) { return p.price > threshold; });
    }
    
    // 3. 計算總庫存價值
    std::cout << "\n=== 總庫存價值 ===" << std::endl;
    double total_value = std::accumulate(products.begin(), products.end(), 0.0,
        [](double sum, const Product& p) {
            return sum + p.price * p.quantity;
        });
    std::cout << "總價值: $" << total_value << std::endl;
    
    // 4. 統計低庫存產品（數量 < 100）
    std::cout << "\n=== 低庫存產品 ===" << std::endl;
    int low_stock_count = std::count_if(products.begin(), products.end(),
        [](const Product& p) { return p.quantity < 100; });
    std::cout << "低庫存產品數: " << low_stock_count << std::endl;
    
    // 5. 對所有產品打 9 折
    std::cout << "\n=== 打 9 折後 ===" << std::endl;
    double discount = 0.9;
    std::for_each(products.begin(), products.end(),
        [discount](Product& p) { p.price *= discount; });
    
    for (const auto& p : products) {
        std::cout << p.name << ": $" << p.price << std::endl;
    }
    
    // 6. 使用 transform 生成價格列表
    std::cout << "\n=== 價格列表 ===" << std::endl;
    std::vector<double> prices(products.size());
    std::transform(products.begin(), products.end(), prices.begin(),
        [](const Product& p) { return p.price; });
    
    std::cout << "價格: ";
    for (double price : prices) {
        std::cout << "$" << price << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
