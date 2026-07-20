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

// =============================================================================
//  第八課 16  —  Lambda × STL 演算法：以商品管理為例的綜合應用
// =============================================================================
//
// 【主題資訊 Information】
//   本檔用到的演算法（皆在 <algorithm>／<numeric>，皆為 C++98 起）：
//     std::sort(first, last, comp)             O(n log n)，不穩定排序
//     std::find_if(first, last, pred)          O(n)，回傳第一個符合的迭代器
//     std::count_if(first, last, pred)         O(n)
//     std::for_each(first, last, fn)           O(n)，回傳 fn 的副本
//     std::transform(first, last, out, fn)     O(n)，一對一映射
//     std::accumulate(first, last, init, op)   O(n)，<numeric>，摺疊
//   Lambda 為 C++11；本檔全部語法在 C++11 即可編譯。
//
// 【詳細解釋 Explanation】
//
// 【1. STL 的核心設計：演算法與容器分離】
//   STL 最重要的設計決策是「演算法不認識容器，只認識迭代器」。
//   std::sort 不知道自己在排 vector 還是 array，它只要求
//   RandomAccessIterator。這帶來 M×N → M+N 的組合爆炸削減：
//   M 個容器 + N 個演算法，不需要寫 M×N 份程式碼。
//   代價是「介面必須用一對迭代器表達」，這也是 C++20 Ranges 想改善的地方
//   （ranges::sort(products, comp) 直接吃容器）。
//
// 【2. 為什麼「行為」要用 lambda 傳進去】
//   演算法負責「走訪與搬移的骨架」，lambda 負責「這個問題特有的判斷」。
//   本檔六個示範全是同一個模式：
//       sort      + 「怎麼比大小」   → [](a,b){ return a.price < b.price; }
//       find_if   + 「什麼算符合」   → [t](p){ return p.price > t; }
//       count_if  + 「什麼要算進去」 → [](p){ return p.quantity < 100; }
//       for_each  + 「對每個做什麼」 → [d](Product& p){ p.price *= d; }
//       transform + 「怎麼映射」     → [](p){ return p.price; }
//       accumulate+ 「怎麼摺疊」     → [](sum,p){ return sum + p.price*p.quantity; }
//   把這六行 lambda 換掉，整個程式就變成另一個應用——這就是「把行為參數化」。
//
// 【3. accumulate 的兩個容易踩的點】
//   (a) 初始值的型別決定累加型別。寫 std::accumulate(b, e, 0) 去加 double，
//       累加器會是 int，小數全被截掉。本檔寫 0.0 是刻意的。
//   (b) 二元操作的第一個參數是「累加器」，第二個才是元素，順序不能寫反。
//
// 【4. for_each vs transform 的分工】
//   for_each 用於「就地改元素或產生副作用」（本檔打 9 折）；
//   transform 用於「產生一份新資料」（本檔抽出價格列表）。
//   若只是要改自己，用 transform 寫成 in-place 也可以，但語意上 for_each 更清楚。
//
// 【概念補充 Concept Deep Dive】
//   本檔示範二用「反覆 find_if」找出所有符合的元素，這是很常見的手法，
//   但有兩個細節值得說明：
//     * 迴圈內必須從 ++it 開始下一輪搜尋，否則會在同一個位置無限迴圈。
//     * ++it 只有在 it != end() 時才合法。本檔的 while 條件先保證了這點，
//       所以 ++it 是安全的；但若把條件寫反或漏掉，對 end() 遞增就是 UB。
//   若要「一次取出全部符合的元素」，用 std::copy_if 到另一個容器會更直接，
//   也避開這個迭代器管理問題。
//
// 【注意事項 Pay Attention】
// 1. std::sort 要求比較器滿足「嚴格弱序」（a<a 必須為 false、可傳遞）。
//    寫成 <= 會破壞這個性質，是 UB——實務上可能崩潰或無限迴圈。
// 2. std::sort 不是穩定排序；價格相同的商品相對順序未定義。
//    需要穩定請用 std::stable_sort。
// 3. std::transform 的輸出迭代器必須指向「已配置好足夠空間」的位置。
//    本檔先寫 std::vector<double> prices(products.size()) 就是為此；
//    若寫成空 vector 再傳 prices.begin() 就是越界寫入（UB）。
//    要往空容器寫請用 std::back_inserter。
// 4. 浮點數輸出：$1.5 而非 $1.50，因為預設格式會去掉尾隨 0。
//    要固定兩位小數需 std::fixed << std::setprecision(2)（<iomanip>）。
// 5. 打 9 折後價格是浮點乘法結果，可能出現 2.7 而非 2.70 這類顯示差異，
//    金額計算在正式系統應使用整數分或定點數，避免浮點誤差累積。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lambda 與 STL 演算法組合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 為什麼要把演算法和容器分開？這個設計換來什麼、又付出什麼？
//     答：演算法只依賴迭代器介面，不依賴容器型別。M 個容器 × N 個演算法
//         原本要寫 M×N 份實作，分離後只要 M+N 份。付出的代價是介面必須
//         用「一對迭代器」表達，可讀性較差、也容易傳錯配對的迭代器。
//     追問：C++20 對此做了什麼改善？→ Ranges 讓演算法可直接吃容器
//         （std::ranges::sort(v, comp)），並且可組合（views），
//         同時保留原本的迭代器版本。
//
// 🔥 Q2. std::accumulate(v.begin(), v.end(), 0) 拿來加一堆 double，會發生什麼？
//     答：初始值 0 是 int，於是累加器型別被推導成 int，每一步相加後都會
//         截斷小數，最後得到錯誤結果。必須寫 0.0（或明確指定型別）。
//     追問：這是 UB 嗎？→ 不是。這是完全定義的行為——就是整數運算，
//         只是語意上不是你要的。這類「編譯過、不崩潰、答案錯」的 bug 最難抓。
//
// ⚠️ 陷阱. 排序比較器寫成 [](const P& a, const P& b){ return a.price <= b.price; }
//          「只是想讓相等的也算小於」，為什麼是嚴重錯誤？
//     答：std::sort 要求嚴格弱序，其中一條是「非自反性」：comp(a, a) 必須為
//         false。用 <= 時 comp(a, a) 為 true，違反契約，整個 sort 的行為
//         變成 UB——實務上常見的表現是越界存取而崩潰，或在相等元素多時無限迴圈。
//     為什麼會錯：直覺上以為比較器只是「回答誰在前面」，多包含一個相等
//         情況應該無害。但 sort 的實作（introsort 的 partition）會依賴
//         「必定存在一個不滿足比較的元素」來停住掃描指標；<= 讓這個
//         哨兵條件失效，指標就一路衝出邊界。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <functional>
#include <iomanip>
#include <set>

struct Product {
    std::string name;
    double price;
    int quantity;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：對已排序陣列就地去除重複元素，回傳去重後長度，前 k 個需為結果。
//   為什麼用到本主題：這題正是「演算法 + 迭代器」的教科書案例。
//     std::unique 把相鄰重複元素往後擠、回傳新的邏輯結尾——它不縮小容器，
//     只重排元素，這正好對應題目「就地修改並回傳長度」的要求。
//     第三個參數可傳自訂的「相等判斷」lambda，與本檔傳比較器給 sort 同一個模式。
//   複雜度：O(n)。
//   注意：std::unique 只移除「相鄰」的重複，所以輸入必須先排序。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto new_end = std::unique(nums.begin(), nums.end(),
                               [](int a, int b) { return a == b; });
    return static_cast<int>(new_end - nums.begin());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 349. Intersection of Two Arrays
//   題目：回傳兩陣列的交集，結果中每個元素唯一，順序不限。
//   為什麼用到本主題：示範多個 STL 演算法「串接」成一條處理管線——
//     sort → unique+erase（去重）→ set_intersection（求交集）。
//     每一步都是現成演算法，自己只需要提供資料流向；這正是本課
//     「演算法與容器分離、以迭代器銜接」的實際威力。
//   複雜度：O(n log n + m log m)，由兩次排序主導。
//   注意：set_intersection 要求兩個輸入區間都「已排序」，這是前置條件；
//     違反時不是 UB，但結果沒有意義。
// -----------------------------------------------------------------------------
std::vector<int> intersection(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    a.erase(std::unique(a.begin(), a.end()), a.end());
    b.erase(std::unique(b.begin(), b.end()), b.end());

    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(out));
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】銷售報表：取營收前 N 名商品
//   場景：後台每日產生「營收 Top 3 商品」報表。資料量可能上萬筆，
//         但只要前 3 名。
//   為什麼用 std::partial_sort：完整排序是 O(n log n)，但我們只要前 N 名。
//     partial_sort 只保證「前 N 個位置是最小（或依比較器最前）的 N 個且已排序」，
//     其餘位置順序未指定，複雜度約 O(n log N)——資料量大、N 很小時差距顯著。
//   注意：因為「其餘位置順序未指定」，本函式只回傳前 N 筆，
//     絕不可對後面的元素順序做任何假設。
// -----------------------------------------------------------------------------
std::vector<Product> topByRevenue(std::vector<Product> items, size_t n) {
    if (n > items.size()) n = items.size();
    std::partial_sort(items.begin(), items.begin() + static_cast<long>(n), items.end(),
        [](const Product& a, const Product& b) {
            return a.price * a.quantity > b.price * b.quantity;   // 營收由大到小
        });
    items.resize(n);      // 只保留前 N 筆；後面的順序未指定，不可使用
    return items;
}

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

    // ---- LeetCode 26: std::unique ----
    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> d1 = {1, 1, 2};
    int k1 = removeDuplicates(d1);
    std::cout << "[1,1,2] -> k=" << k1 << ", 前 k 個: ";
    for (int i = 0; i < k1; ++i) std::cout << d1[i] << " ";
    std::cout << std::endl;

    std::vector<int> d2 = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k2 = removeDuplicates(d2);
    std::cout << "[0,0,1,1,1,2,2,3,3,4] -> k=" << k2 << ", 前 k 個: ";
    for (int i = 0; i < k2; ++i) std::cout << d2[i] << " ";
    std::cout << std::endl;
    // 只印前 k 個：[k, size) 是「有效但未指定」的殘值，不可讀取其內容。

    // ---- LeetCode 349: 演算法串接 ----
    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===" << std::endl;
    std::cout << "[1,2,2,1] ∩ [2,2]       = ";
    for (int n : intersection({1, 2, 2, 1}, {2, 2})) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "[4,9,5] ∩ [9,4,9,8,4]   = ";
    for (int n : intersection({4, 9, 5}, {9, 4, 9, 8, 4})) std::cout << n << " ";
    std::cout << std::endl;

    // ---- 日常實務: 營收 Top N ----
    std::cout << "\n=== 日常實務: 營收 Top 3 商品 ===" << std::endl;
    // 刻意讓五筆營收兩兩相異（300 / 180 / 160 / 150 / 112.5）。
    // partial_sort 不是穩定排序，若有兩筆營收相同，它們的先後是「未指定」的，
    // 輸出就不該被當成固定結果來驗證。
    std::vector<Product> catalog = {
        {"Apple",  1.50, 100},   // 營收 150
        {"Banana", 0.75, 150},   // 營收 112.5
        {"Orange", 2.00,  80},   // 營收 160
        {"Mango",  3.00,  60},   // 營收 180
        {"Grape",  2.50, 120}    // 營收 300
    };
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& p : topByRevenue(catalog, 3)) {
        std::cout << "  " << p.name << " 營收 $" << p.price * p.quantity << std::endl;
    }
    std::cout.unsetf(std::ios::fixed);
    std::cout << std::setprecision(6);   // 還原預設格式

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探16.cpp -o stl_lambda_product

// 【但書】浮點價格以預設格式輸出會去掉尾隨 0（$1.5 而非 $1.50）；
//   只有「營收 Top 3」那段刻意套了 std::fixed << std::setprecision(2)。
//   另外 std::partial_sort 不是穩定排序，本範例已刻意讓五筆營收互不相同，
//   若資料中出現同營收，前後順序屬「未指定」，不應視為固定輸出。

// === 預期輸出 ===
// === 按價格排序 ===
// Banana: $0.75
// Apple: $1.5
// Orange: $2
// Grape: $2.5
// Mango: $3
//
// === 價格超過 $2 ===
// Grape: $2.5
// Mango: $3
//
// === 總庫存價值 ===
// 總價值: $872.5
//
// === 低庫存產品 ===
// 低庫存產品數: 2
//
// === 打 9 折後 ===
// Banana: $0.675
// Apple: $1.35
// Orange: $1.8
// Grape: $2.25
// Mango: $2.7
//
// === 價格列表 ===
// 價格: $0.675 $1.35 $1.8 $2.25 $2.7 
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// [1,1,2] -> k=2, 前 k 個: 1 2 
// [0,0,1,1,1,2,2,3,3,4] -> k=5, 前 k 個: 0 1 2 3 4 
//
// === LeetCode 349. Intersection of Two Arrays ===
// [1,2,2,1] ∩ [2,2]       = 2 
// [4,9,5] ∩ [9,4,9,8,4]   = 4 9 
//
// === 日常實務: 營收 Top 3 商品 ===
//   Grape 營收 $300.00
//   Mango 營收 $180.00
//   Orange 營收 $160.00
