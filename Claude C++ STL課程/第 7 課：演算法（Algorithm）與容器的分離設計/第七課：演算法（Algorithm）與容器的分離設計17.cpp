/*
# 第七課：演算法（Algorithm）與容器的分離設計

---

## 7.1 為什麼要分離？

這是 STL 最核心的設計決策之一：**演算法和容器是獨立的**。

### 如果不分離會怎樣？

```
┌─────────────────────────────────────────────────────────────────┐
│                    不分離的設計（傳統做法）                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   假設有 M 個容器和 N 個演算法：                                 │
│                                                                 │
│   容器：vector, list, deque, set, map... (M 個)                 │
│   演算法：sort, find, count, copy, reverse... (N 個)            │
│                                                                 │
│   傳統做法：每個容器都要實作每個演算法                          │
│                                                                 │
│   vector::sort()      list::sort()      deque::sort()           │
│   vector::find()      list::find()      deque::find()           │
│   vector::count()     list::count()     deque::count()          │
│   vector::copy()      list::copy()      deque::copy()           │
│   ...                 ...               ...                     │
│                                                                 │
│   需要實作的函數數量：M × N                                     │
│   假設 M=10, N=50 → 需要 500 個函數！                           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### STL 的分離設計

```
┌─────────────────────────────────────────────────────────────────┐
│                    STL 的分離設計                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│        容器 (M 個)              演算法 (N 個)                    │
│   ┌─────────────────┐      ┌─────────────────┐                 │
│   │ vector          │      │ sort            │                 │
│   │ list            │      │ find            │                 │
│   │ deque           │◄────►│ count           │                 │
│   │ set             │      │ copy            │                 │
│   │ map             │      │ reverse         │                 │
│   │ ...             │      │ ...             │                 │
│   └────────┬────────┘      └────────┬────────┘                 │
│            │                        │                           │
│            │    ┌──────────────┐    │                           │
│            └───►│   迭代器     │◄───┘                           │
│                 │  (Iterator)  │                                │
│                 └──────────────┘                                │
│                                                                 │
│   需要實作的元件：M + N                                         │
│   假設 M=10, N=50 → 只需要 60 個元件！                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

這就是**正交設計**的威力。

---

## 7.2 演算法透過迭代器操作

STL 演算法不直接操作容器，而是透過迭代器：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <algorithm>

int main() {
    std::cout << "=== 同一個 find 用在不同容器 ===" << std::endl;
    
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int> lst = {10, 20, 30, 40, 50};
    std::set<int> s = {10, 20, 30, 40, 50};
    
    // std::find 只認識迭代器，不認識容器
    auto v_it = std::find(vec.begin(), vec.end(), 30);
    auto l_it = std::find(lst.begin(), lst.end(), 30);
    auto s_it = std::find(s.begin(), s.end(), 30);
    
    std::cout << "vector: " << (v_it != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "list: " << (l_it != lst.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "set: " << (s_it != s.end() ? "找到" : "沒找到") << std::endl;
    
    // 甚至可以用在原生陣列（指標就是迭代器）
    int arr[] = {10, 20, 30, 40, 50};
    auto a_it = std::find(arr, arr + 5, 30);
    std::cout << "array: " << (a_it != arr + 5 ? "找到" : "沒找到") << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 同一個 find 用在不同容器 ===
vector: 找到
list: 找到
set: 找到
array: 找到
```

---

## 7.3 演算法的基本形式

大多數 STL 演算法遵循相似的模式：

```cpp
// 基本形式
algorithm(begin_iterator, end_iterator);

// 帶額外參數
algorithm(begin_iterator, end_iterator, value);

// 帶謂詞（predicate）
algorithm(begin_iterator, end_iterator, predicate);

// 帶輸出迭代器
algorithm(begin_iterator, end_iterator, output_iterator);

// 操作兩個範圍
algorithm(begin1, end1, begin2);
```

### 範例展示

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 形式一：只有範圍
    std::cout << "=== 只有範圍 ===" << std::endl;
    std::sort(vec.begin(), vec.end());
    std::cout << "sort 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 形式二：範圍 + 值
    std::cout << "\n=== 範圍 + 值 ===" << std::endl;
    int count = std::count(vec.begin(), vec.end(), 5);
    std::cout << "5 出現次數: " << count << std::endl;
    
    // 形式三：範圍 + 謂詞
    std::cout << "\n=== 範圍 + 謂詞 ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;
    
    // 形式四：範圍 + 輸出迭代器
    std::cout << "\n=== 範圍 + 輸出迭代器 ===" << std::endl;
    std::vector<int> dest(vec.size());
    std::copy(vec.begin(), vec.end(), dest.begin());
    std::cout << "copy 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;
    
    // 形式五：兩個範圍
    std::cout << "\n=== 兩個範圍 ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {1, 2, 3};
    bool are_equal = std::equal(v1.begin(), v1.end(), v2.begin());
    std::cout << "v1 和 v2 相等: " << (are_equal ? "是" : "否") << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 只有範圍 ===
sort 後: 1 2 3 4 5 6 7 8 9 

=== 範圍 + 值 ===
5 出現次數: 1

=== 範圍 + 謂詞 ===
偶數個數: 4

=== 範圍 + 輸出迭代器 ===
copy 結果: 1 2 3 4 5 6 7 8 9 

=== 兩個範圍 ===
v1 和 v2 相等: 是
```

---

## 7.4 演算法分類總覽

STL 演算法大致分為以下幾類：

```
┌─────────────────────────────────────────────────────────────────┐
│                      STL 演算法分類                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  非修改序列操作（不改變容器內容）                                │
│  ├── 查找：find, find_if, find_first_of, adjacent_find         │
│  ├── 計數：count, count_if                                     │
│  ├── 比較：equal, mismatch, lexicographical_compare            │
│  ├── 搜尋：search, search_n                                    │
│  └── 遍歷：for_each, all_of, any_of, none_of                   │
│                                                                 │
│  修改序列操作（改變容器內容）                                    │
│  ├── 複製：copy, copy_if, copy_n, copy_backward                │
│  ├── 移動：move, move_backward                                 │
│  ├── 填充：fill, fill_n, generate, generate_n                  │
│  ├── 轉換：transform                                           │
│  ├── 替換：replace, replace_if, replace_copy                   │
│  ├── 移除：remove, remove_if, remove_copy, unique              │
│  └── 反轉/旋轉：reverse, rotate, shuffle                       │
│                                                                 │
│  排序相關操作                                                    │
│  ├── 排序：sort, stable_sort, partial_sort                     │
│  ├── 第 n 元素：nth_element                                    │
│  ├── 二分搜尋：binary_search, lower_bound, upper_bound         │
│  └── 合併：merge, inplace_merge                                │
│                                                                 │
│  集合操作（已排序範圍）                                          │
│  ├── set_union, set_intersection                               │
│  └── set_difference, set_symmetric_difference                  │
│                                                                 │
│  堆積操作                                                        │
│  ├── make_heap, push_heap, pop_heap                            │
│  └── sort_heap, is_heap                                        │
│                                                                 │
│  最小/最大                                                       │
│  ├── min, max, minmax                                          │
│  └── min_element, max_element, minmax_element                  │
│                                                                 │
│  數值操作（<numeric>）                                          │
│  ├── accumulate, inner_product                                 │
│  ├── partial_sum, adjacent_difference                          │
│  └── iota                                                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7.5 非修改序列操作

這類演算法**只讀取**元素，不會改變容器內容。

### 7.5.1 查找類

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 30, 50, 30};
    
    // find：找第一個等於某值的元素
    std::cout << "=== find ===" << std::endl;
    auto it = std::find(vec.begin(), vec.end(), 30);
    if (it != vec.end()) {
        std::cout << "找到 30，位置: " << (it - vec.begin()) << std::endl;
    }
    
    // find_if：找第一個滿足條件的元素
    std::cout << "\n=== find_if ===" << std::endl;
    auto it2 = std::find_if(vec.begin(), vec.end(), 
        [](int n) { return n > 25; });
    if (it2 != vec.end()) {
        std::cout << "第一個大於 25 的元素: " << *it2 << std::endl;
    }
    
    // find_if_not：找第一個不滿足條件的元素
    std::cout << "\n=== find_if_not ===" << std::endl;
    auto it3 = std::find_if_not(vec.begin(), vec.end(),
        [](int n) { return n < 30; });
    if (it3 != vec.end()) {
        std::cout << "第一個不小於 30 的元素: " << *it3 << std::endl;
    }
    
    // adjacent_find：找連續相同的元素
    std::cout << "\n=== adjacent_find ===" << std::endl;
    std::vector<int> v2 = {1, 2, 3, 3, 4, 5, 5, 5};
    auto it4 = std::adjacent_find(v2.begin(), v2.end());
    if (it4 != v2.end()) {
        std::cout << "第一對相鄰相同元素: " << *it4 << std::endl;
    }
    
    return 0;
}
```

**輸出：**
```
=== find ===
找到 30，位置: 2

=== find_if ===
第一個大於 25 的元素: 30

=== find_if_not ===
第一個不小於 30 的元素: 30

=== adjacent_find ===
第一對相鄰相同元素: 3
```

### 7.5.2 計數與條件判斷

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // count：計算某值出現次數
    std::cout << "=== count ===" << std::endl;
    std::vector<int> v2 = {1, 2, 2, 3, 2, 4, 2, 5};
    std::cout << "2 出現次數: " << std::count(v2.begin(), v2.end(), 2) << std::endl;
    
    // count_if：計算滿足條件的元素數
    std::cout << "\n=== count_if ===" << std::endl;
    int even_count = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "偶數個數: " << even_count << std::endl;
    
    // all_of：是否全部滿足條件
    std::cout << "\n=== all_of ===" << std::endl;
    bool all_positive = std::all_of(vec.begin(), vec.end(),
        [](int n) { return n > 0; });
    std::cout << "全部是正數: " << (all_positive ? "是" : "否") << std::endl;
    
    // any_of：是否有任一滿足條件
    std::cout << "\n=== any_of ===" << std::endl;
    bool has_even = std::any_of(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; });
    std::cout << "有偶數: " << (has_even ? "是" : "否") << std::endl;
    
    // none_of：是否全部不滿足條件
    std::cout << "\n=== none_of ===" << std::endl;
    bool no_negative = std::none_of(vec.begin(), vec.end(),
        [](int n) { return n < 0; });
    std::cout << "沒有負數: " << (no_negative ? "是" : "否") << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== count ===
2 出現次數: 4

=== count_if ===
偶數個數: 5

=== all_of ===
全部是正數: 是

=== any_of ===
有偶數: 是

=== none_of ===
沒有負數: 是
```

### 7.5.3 for_each：對每個元素執行操作

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // for_each：對每個元素執行操作
    std::cout << "=== for_each（輸出）===" << std::endl;
    std::cout << "元素: ";
    std::for_each(vec.begin(), vec.end(), [](int n) {
        std::cout << n << " ";
    });
    std::cout << std::endl;
    
    // for_each 也可以修改元素（透過參考）
    std::cout << "\n=== for_each（修改）===" << std::endl;
    std::for_each(vec.begin(), vec.end(), [](int& n) {
        n *= 2;
    });
    std::cout << "乘以 2 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // for_each 會回傳函數物件（可以累積狀態）
    std::cout << "\n=== for_each（累積）===" << std::endl;
    struct Sum {
        int total = 0;
        void operator()(int n) { total += n; }
    };
    
    Sum result = std::for_each(vec.begin(), vec.end(), Sum());
    std::cout << "總和: " << result.total << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== for_each（輸出）===
元素: 1 2 3 4 5 

=== for_each（修改）===
乘以 2 後: 2 4 6 8 10 

=== for_each（累積）===
總和: 30
```

---

## 7.6 修改序列操作

這類演算法會**改變**元素的值或位置。

### 7.6.1 複製與移動

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

int main() {
    std::vector<int> src = {1, 2, 3, 4, 5};
    
    // copy：複製到另一個容器
    std::cout << "=== copy ===" << std::endl;
    std::vector<int> dest1(src.size());
    std::copy(src.begin(), src.end(), dest1.begin());
    std::cout << "copy 結果: ";
    for (int n : dest1) std::cout << n << " ";
    std::cout << std::endl;
    
    // copy_if：條件複製
    std::cout << "\n=== copy_if ===" << std::endl;
    std::vector<int> dest2;
    std::copy_if(src.begin(), src.end(), std::back_inserter(dest2),
        [](int n) { return n % 2 == 0; });
    std::cout << "只複製偶數: ";
    for (int n : dest2) std::cout << n << " ";
    std::cout << std::endl;
    
    // copy_n：複製前 n 個
    std::cout << "\n=== copy_n ===" << std::endl;
    std::vector<int> dest3;
    std::copy_n(src.begin(), 3, std::back_inserter(dest3));
    std::cout << "複製前 3 個: ";
    for (int n : dest3) std::cout << n << " ";
    std::cout << std::endl;
    
    // copy_backward：從後往前複製
    std::cout << "\n=== copy_backward ===" << std::endl;
    std::vector<int> dest4(7, 0);  // {0, 0, 0, 0, 0, 0, 0}
    std::copy_backward(src.begin(), src.end(), dest4.end());
    std::cout << "copy_backward: ";
    for (int n : dest4) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== copy ===
copy 結果: 1 2 3 4 5 

=== copy_if ===
只複製偶數: 2 4 

=== copy_n ===
複製前 3 個: 1 2 3 

=== copy_backward ===
copy_backward: 0 0 1 2 3 4 5 
```

### 7.6.2 transform：轉換

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>

int main() {
    // 一元 transform：對每個元素套用轉換
    std::cout << "=== 一元 transform ===" << std::endl;
    std::vector<int> src = {1, 2, 3, 4, 5};
    std::vector<int> squares(src.size());
    
    std::transform(src.begin(), src.end(), squares.begin(),
        [](int n) { return n * n; });
    
    std::cout << "平方: ";
    for (int n : squares) std::cout << n << " ";
    std::cout << std::endl;
    
    // 二元 transform：結合兩個範圍
    std::cout << "\n=== 二元 transform ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3, 4, 5};
    std::vector<int> v2 = {10, 20, 30, 40, 50};
    std::vector<int> sums(v1.size());
    
    std::transform(v1.begin(), v1.end(), v2.begin(), sums.begin(),
        [](int a, int b) { return a + b; });
    
    std::cout << "v1 + v2: ";
    for (int n : sums) std::cout << n << " ";
    std::cout << std::endl;
    
    // 實用案例：字串轉大寫
    std::cout << "\n=== 字串轉大寫 ===" << std::endl;
    std::string str = "Hello, World!";
    std::transform(str.begin(), str.end(), str.begin(),
        [](char c) { return std::toupper(c); });
    std::cout << "轉換後: " << str << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 一元 transform ===
平方: 1 4 9 16 25 

=== 二元 transform ===
v1 + v2: 11 22 33 44 55 

=== 字串轉大寫 ===
轉換後: HELLO, WORLD!
```

### 7.6.3 fill 與 generate

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // fill：用固定值填充
    std::cout << "=== fill ===" << std::endl;
    std::vector<int> vec(5);
    std::fill(vec.begin(), vec.end(), 42);
    std::cout << "fill 42: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // fill_n：填充前 n 個
    std::cout << "\n=== fill_n ===" << std::endl;
    std::fill_n(vec.begin(), 3, 100);
    std::cout << "fill_n 前 3 個為 100: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // generate：用函數生成值
    std::cout << "\n=== generate ===" << std::endl;
    int counter = 0;
    std::generate(vec.begin(), vec.end(), [&counter]() {
        return ++counter * 10;
    });
    std::cout << "generate: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // generate_n：生成前 n 個
    std::cout << "\n=== generate_n ===" << std::endl;
    std::vector<int> fibs(10);
    int a = 0, b = 1;
    std::generate_n(fibs.begin(), 10, [&a, &b]() {
        int result = a;
        int temp = a + b;
        a = b;
        b = temp;
        return result;
    });
    std::cout << "Fibonacci: ";
    for (int n : fibs) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== fill ===
fill 42: 42 42 42 42 42 

=== fill_n ===
fill_n 前 3 個為 100: 100 100 100 42 42 

=== generate ===
generate: 10 20 30 40 50 

=== generate_n ===
Fibonacci: 0 1 1 2 3 5 8 13 21 34 
```

### 7.6.4 replace 與 remove

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // replace：替換所有等於某值的元素
    std::cout << "=== replace ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 2, 4, 2, 5};
    std::replace(vec.begin(), vec.end(), 2, 99);
    std::cout << "把 2 替換為 99: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // replace_if：條件替換
    std::cout << "\n=== replace_if ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::replace_if(vec2.begin(), vec2.end(),
        [](int n) { return n % 2 == 0; }, 0);
    std::cout << "偶數替換為 0: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    
    // remove：移除所有等於某值的元素
    // 重要：remove 不會真正刪除元素，而是把要保留的移到前面！
    std::cout << "\n=== remove（重要！）===" << std::endl;
    std::vector<int> vec3 = {1, 2, 3, 2, 4, 2, 5};
    std::cout << "原始: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << "(size=" << vec3.size() << ")" << std::endl;
    
    auto new_end = std::remove(vec3.begin(), vec3.end(), 2);
    std::cout << "remove 後: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << "(size=" << vec3.size() << ")" << std::endl;
    
    std::cout << "有效範圍: ";
    for (auto it = vec3.begin(); it != new_end; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 真正刪除：erase-remove 慣用法
    std::cout << "\n=== erase-remove 慣用法 ===" << std::endl;
    std::vector<int> vec4 = {1, 2, 3, 2, 4, 2, 5};
    vec4.erase(std::remove(vec4.begin(), vec4.end(), 2), vec4.end());
    std::cout << "erase-remove 後: ";
    for (int n : vec4) std::cout << n << " ";
    std::cout << "(size=" << vec4.size() << ")" << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== replace ===
把 2 替換為 99: 1 99 3 99 4 99 5 

=== replace_if ===
偶數替換為 0: 1 0 3 0 5 0 7 0 9 0 

=== remove（重要！）===
原始: 1 2 3 2 4 2 5 (size=7)
remove 後: 1 3 4 5 4 2 5 (size=7)
有效範圍: 1 3 4 5 

=== erase-remove 慣用法 ===
erase-remove 後: 1 3 4 5 (size=4)
```

### remove 的運作原理

```
┌─────────────────────────────────────────────────────────────────┐
│                    remove 的運作原理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   原始狀態：                                                    │
│   [ 1 | 2 | 3 | 2 | 4 | 2 | 5 ]                                │
│                                                                 │
│   remove(begin, end, 2) 執行過程：                              │
│   把不等於 2 的元素往前搬                                       │
│                                                                 │
│   [ 1 | 3 | 4 | 5 | ? | ? | ? ]                                │
│                       ↑                                         │
│                    new_end                                      │
│                                                                 │
│   注意：                                                        │
│   • 容器大小沒變（還是 7）                                      │
│   • new_end 之後的元素是「不確定」的垃圾值                     │
│   • 需要配合 erase 才能真正刪除                                 │
│                                                                 │
│   vec.erase(new_end, vec.end());                                │
│   [ 1 | 3 | 4 | 5 ]                                             │
│   大小變成 4                                                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 7.6.5 reverse 與 rotate

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // reverse：反轉
    std::cout << "=== reverse ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::reverse(vec.begin(), vec.end());
    std::cout << "反轉後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // rotate：旋轉
    std::cout << "\n=== rotate ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 4, 5};
    // 把 vec2.begin() + 2 變成新的第一個元素
    std::rotate(vec2.begin(), vec2.begin() + 2, vec2.end());
    std::cout << "rotate 後（3 變成第一個）: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    
    // unique：移除連續重複
    std::cout << "\n=== unique ===" << std::endl;
    std::vector<int> vec3 = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
    auto new_end = std::unique(vec3.begin(), vec3.end());
    vec3.erase(new_end, vec3.end());
    std::cout << "unique 後: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== reverse ===
反轉後: 5 4 3 2 1 

=== rotate ===
rotate 後（3 變成第一個）: 3 4 5 1 2 

=== unique ===
unique 後: 1 2 3 4 5 
```

---

## 7.7 排序相關操作

### 7.7.1 sort 與 stable_sort

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

struct Person {
    std::string name;
    int age;
};

int main() {
    // 基本排序
    std::cout << "=== sort ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7};
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 自訂比較函數
    std::cout << "\n=== sort 自訂比較 ===" << std::endl;
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // stable_sort：保持相等元素的原始順序
    std::cout << "\n=== stable_sort ===" << std::endl;
    std::vector<Person> people = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 25},
        {"Diana", 30}
    };
    
    // 按年齡排序，相同年齡的保持原順序
    std::stable_sort(people.begin(), people.end(),
        [](const Person& a, const Person& b) {
            return a.age < b.age;
        });
    
    std::cout << "按年齡 stable_sort:" << std::endl;
    for (const auto& p : people) {
        std::cout << "  " << p.name << ": " << p.age << std::endl;
    }
    
    return 0;
}
```

**輸出：**
```
=== sort ===
升序: 1 2 3 5 7 8 9 

=== sort 自訂比較 ===
降序: 9 8 7 5 3 2 1 

=== stable_sort ===
按年齡 stable_sort:
  Alice: 25
  Charlie: 25
  Bob: 30
  Diana: 30
```

### 7.7.2 partial_sort 與 nth_element

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // partial_sort：只排序前 n 個
    std::cout << "=== partial_sort ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 只需要最小的 3 個，排好放在最前面
    std::partial_sort(vec.begin(), vec.begin() + 3, vec.end());
    std::cout << "前 3 個最小（已排序）: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // nth_element：找第 n 個元素
    std::cout << "\n=== nth_element ===" << std::endl;
    std::vector<int> vec2 = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 找中位數（第 4 個元素，索引 4）
    std::nth_element(vec2.begin(), vec2.begin() + 4, vec2.end());
    std::cout << "nth_element(4) 後: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "第 5 個元素（索引 4）: " << vec2[4] << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== partial_sort ===
前 3 個最小（已排序）: 1 2 3 9 8 5 7 4 6 

=== nth_element ===
nth_element(4) 後: 3 2 4 1 5 6 7 8 9 
第 5 個元素（索引 4）: 5
```

### 7.7.3 二分搜尋

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    // 二分搜尋要求已排序！
    
    // binary_search：是否存在
    std::cout << "=== binary_search ===" << std::endl;
    std::cout << "存在 5: " << (std::binary_search(vec.begin(), vec.end(), 5) ? "是" : "否") << std::endl;
    std::cout << "存在 11: " << (std::binary_search(vec.begin(), vec.end(), 11) ? "是" : "否") << std::endl;
    
    // lower_bound：第一個 >= 值的位置
    std::cout << "\n=== lower_bound ===" << std::endl;
    auto lb = std::lower_bound(vec.begin(), vec.end(), 5);
    std::cout << "lower_bound(5): 位置 " << (lb - vec.begin()) << ", 值 " << *lb << std::endl;
    
    // upper_bound：第一個 > 值的位置
    std::cout << "\n=== upper_bound ===" << std::endl;
    auto ub = std::upper_bound(vec.begin(), vec.end(), 5);
    std::cout << "upper_bound(5): 位置 " << (ub - vec.begin()) << ", 值 " << *ub << std::endl;
    
    // equal_range：lower_bound 和 upper_bound 的組合
    std::cout << "\n=== equal_range ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 2, 2, 3, 4, 5};
    auto range = std::equal_range(vec2.begin(), vec2.end(), 2);
    std::cout << "equal_range(2): [" << (range.first - vec2.begin()) 
              << ", " << (range.second - vec2.begin()) << ")" << std::endl;
    std::cout << "2 的數量: " << (range.second - range.first) << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== binary_search ===
存在 5: 是
存在 11: 否

=== lower_bound ===
lower_bound(5): 位置 4, 值 5

=== upper_bound ===
upper_bound(5): 位置 5, 值 6

=== equal_range ===
equal_range(2): [1, 4)
2 的數量: 3
```

---

## 7.8 數值演算法

這些演算法定義在 `<numeric>` 標頭檔。

```cpp
#include <iostream>
#include <vector>
#include <numeric>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // accumulate：累加
    std::cout << "=== accumulate ===" << std::endl;
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;
    
    // accumulate 可以自訂運算
    int product = std::accumulate(vec.begin(), vec.end(), 1, 
        std::multiplies<int>());
    std::cout << "乘積: " << product << std::endl;
    
    // iota：填充遞增序列
    std::cout << "\n=== iota ===" << std::endl;
    std::vector<int> seq(10);
    std::iota(seq.begin(), seq.end(), 1);  // 從 1 開始
    std::cout << "iota: ";
    for (int n : seq) std::cout << n << " ";
    std::cout << std::endl;
    
    // partial_sum：部分和
    std::cout << "\n=== partial_sum ===" << std::endl;
    std::vector<int> partial(vec.size());
    std::partial_sum(vec.begin(), vec.end(), partial.begin());
    std::cout << "部分和: ";
    for (int n : partial) std::cout << n << " ";
    std::cout << std::endl;
    
    // adjacent_difference：相鄰差
    std::cout << "\n=== adjacent_difference ===" << std::endl;
    std::vector<int> data = {1, 3, 6, 10, 15};
    std::vector<int> diff(data.size());
    std::adjacent_difference(data.begin(), data.end(), diff.begin());
    std::cout << "相鄰差: ";
    for (int n : diff) std::cout << n << " ";
    std::cout << std::endl;
    
    // inner_product：內積
    std::cout << "\n=== inner_product ===" << std::endl;
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {4, 5, 6};
    int dot = std::inner_product(v1.begin(), v1.end(), v2.begin(), 0);
    std::cout << "內積: " << dot << " (1*4 + 2*5 + 3*6 = 32)" << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== accumulate ===
總和: 15
乘積: 120

=== iota ===
iota: 1 2 3 4 5 6 7 8 9 10 

=== partial_sum ===
部分和: 1 3 6 10 15 

=== adjacent_difference ===
相鄰差: 1 2 3 4 5 

=== inner_product ===
內積: 32 (1*4 + 2*5 + 3*6 = 32)
```

---

## 7.9 演算法與迭代器類別的關係

不同演算法對迭代器有不同的要求：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <forward_list>
#include <algorithm>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};
    std::forward_list<int> flst = {5, 2, 8, 1, 9};
    
    // find：只需要 Input Iterator（所有容器都能用）
    std::cout << "=== find（Input Iterator）===" << std::endl;
    std::cout << "vector: " << (*std::find(vec.begin(), vec.end(), 8)) << std::endl;
    std::cout << "list: " << (*std::find(lst.begin(), lst.end(), 8)) << std::endl;
    std::cout << "forward_list: " << (*std::find(flst.begin(), flst.end(), 8)) << std::endl;
    
    // reverse：需要 Bidirectional Iterator
    std::cout << "\n=== reverse（Bidirectional Iterator）===" << std::endl;
    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    // std::reverse(flst.begin(), flst.end());  // 錯誤！forward_list 不支援
    
    std::cout << "vector reversed: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // sort：需要 Random Access Iterator
    std::cout << "\n=== sort（Random Access Iterator）===" << std::endl;
    std::sort(vec.begin(), vec.end());
    // std::sort(lst.begin(), lst.end());  // 錯誤！list 不支援
    
    // list 和 forward_list 有自己的 sort
    lst.sort();
    flst.sort();
    
    std::cout << "All sorted successfully!" << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== find（Input Iterator）===
vector: 8
list: 8
forward_list: 8

=== reverse（Bidirectional Iterator）===
vector reversed: 9 1 8 2 5 

=== sort（Random Access Iterator）===
All sorted successfully!
```

---

## 7.10 為什麼 list 和 set 有自己的成員函數？

有些容器提供與 STL 演算法同名的成員函數，原因是效能：

```cpp
#include <iostream>
#include <list>
#include <set>
#include <algorithm>

int main() {
    // list 的成員函數 vs STL 演算法
    std::cout << "=== list 的成員函數 ===" << std::endl;
    
    std::list<int> lst = {5, 2, 8, 1, 9, 2, 7, 2};
    
    // list::sort() 比 std::sort() 更適合 list
    // 因為 list::sort() 只調整指標，不需要移動元素
    lst.sort();
    std::cout << "sort 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list::remove() 比 erase-remove 慣用法更高效
    lst.remove(2);
    std::cout << "remove(2) 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list::unique() 移除連續重複
    std::list<int> lst2 = {1, 1, 2, 2, 2, 3, 3};
    lst2.unique();
    std::cout << "unique 後: ";
    for (int n : lst2) std::cout << n << " ";
    std::cout << std::endl;
    
    // set 的成員函數
    std::cout << "\n=== set 的成員函數 ===" << std::endl;
    std::set<int> s = {5, 2, 8, 1, 9};
    
    // set::find() 是 O(log n)
    // std::find() 在 set 上是 O(n)（因為它用線性搜尋）
    auto it = s.find(8);  // O(log n) - 使用紅黑樹
    // auto it2 = std::find(s.begin(), s.end(), 8);  // O(n) - 線性搜尋
    
    if (it != s.end()) {
        std::cout << "set::find(8): 找到" << std::endl;
    }
    
    return 0;
}
```

**輸出：**
```
=== list 的成員函數 ===
sort 後: 1 2 2 2 5 7 8 9 
remove(2) 後: 1 5 7 8 9 
unique 後: 1 2 3 

=== set 的成員函數 ===
set::find(8): 找到
```

### 成員函數 vs STL 演算法

```
┌─────────────────────────────────────────────────────────────────┐
│                 成員函數 vs STL 演算法                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   容器        成員函數              STL 演算法                  │
│   ──────────────────────────────────────────────────────────    │
│   list        sort()               std::sort() ✗ 不支援         │
│               remove()             std::remove() + erase()      │
│               unique()             std::unique() + erase()      │
│               merge()              std::merge()                 │
│               splice()             無對應                       │
│                                                                 │
│   set/map     find()    O(log n)   std::find()    O(n)         │
│               count()   O(log n)   std::count()   O(n)         │
│               lower_bound()        std::lower_bound()           │
│                                                                 │
│   原則：如果容器有同名成員函數，優先使用成員函數！               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7.11 完整範例：綜合應用

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

struct Student {
    std::string name;
    int score;
};

int main() {
    std::vector<Student> students = {
        {"Alice", 85},
        {"Bob", 92},
        {"Charlie", 78},
        {"Diana", 95},
        {"Eve", 88},
        {"Frank", 72},
        {"Grace", 91}
    };
    
    // 1. 按分數排序（降序）
    std::cout << "=== 按分數排序 ===" << std::endl;
    std::sort(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.score > b.score;
        });
    
    for (const auto& s : students) {
        std::cout << s.name << ": " << s.score << std::endl;
    }
    
    // 2. 找出分數 >= 90 的學生
    std::cout << "\n=== 分數 >= 90 ===" << std::endl;
    auto high_it = std::find_if(students.begin(), students.end(),
        [](const Student& s) { return s.score < 90; });
    
    std::cout << "優秀學生: ";
    for (auto it = students.begin(); it != high_it; ++it) {
        std::cout << it->name << " ";
    }
    std::cout << std::endl;
    
    // 3. 計算平均分
    std::cout << "\n=== 平均分 ===" << std::endl;
    int total = std::accumulate(students.begin(), students.end(), 0,
        [](int sum, const Student& s) { return sum + s.score; });
    double average = static_cast<double>(total) / students.size();
    std::cout << "平均分: " << average << std::endl;
    
    // 4. 統計及格（>= 80）人數
    std::cout << "\n=== 及格人數 ===" << std::endl;
    int pass_count = std::count_if(students.begin(), students.end(),
        [](const Student& s) { return s.score >= 80; });
    std::cout << "及格人數: " << pass_count << "/" << students.size() << std::endl;
    
    // 5. 提取所有分數到另一個 vector
    std::cout << "\n=== 提取分數 ===" << std::endl;
    std::vector<int> scores(students.size());
    std::transform(students.begin(), students.end(), scores.begin(),
        [](const Student& s) { return s.score; });
    
    std::cout << "所有分數: ";
    for (int score : scores) std::cout << score << " ";
    std::cout << std::endl;
    
    // 6. 找最高分和最低分
    std::cout << "\n=== 最高最低分 ===" << std::endl;
    auto minmax = std::minmax_element(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.score < b.score;
        });
    
    std::cout << "最高分: " << minmax.second->name << " (" << minmax.second->score << ")" << std::endl;
    std::cout << "最低分: " << minmax.first->name << " (" << minmax.first->score << ")" << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== 按分數排序 ===
Diana: 95
Bob: 92
Grace: 91
Eve: 88
Alice: 85
Charlie: 78
Frank: 72

=== 分數 >= 90 ===
優秀學生: Diana Bob Grace 

=== 平均分 ===
平均分: 85.8571

=== 及格人數 ===
及格人數: 5/7

=== 提取分數 ===
所有分數: 95 92 91 88 85 78 72 

=== 最高最低分 ===
最高分: Diana (95)
最低分: Frank (72)
```

---

## 7.12 本課重點整理

```
┌─────────────────────────────────────────────────────────────────┐
│                      第七課 重點整理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 分離設計的優勢                                               │
│     • M 個容器 + N 個演算法 = M + N 個元件（而非 M × N）        │
│     • 透過迭代器連接，達到正交性                                │
│                                                                 │
│  2. 演算法的基本形式                                             │
│     • algorithm(begin, end)                                     │
│     • algorithm(begin, end, value)                              │
│     • algorithm(begin, end, predicate)                          │
│     • algorithm(begin, end, output_iterator)                    │
│                                                                 │
│  3. 主要演算法類別                                               │
│     • 非修改：find, count, all_of, for_each                     │
│     • 修改：copy, transform, fill, replace, remove              │
│     • 排序：sort, stable_sort, partial_sort, nth_element        │
│     • 二分搜尋：binary_search, lower_bound, upper_bound         │
│     • 數值：accumulate, iota, partial_sum                       │
│                                                                 │
│  4. 重要觀念                                                     │
│     • remove 不會真正刪除，需配合 erase                         │
│     • 二分搜尋演算法要求已排序                                  │
│     • 不同演算法對迭代器有不同要求                              │
│                                                                 │
│  5. 成員函數 vs STL 演算法                                       │
│     • 如果容器有同名成員函數，優先使用成員函數                  │
│     • 例如：list::sort(), set::find()                           │
│                                                                 │
│  6. 數值演算法在 <numeric>                                      │
│     • accumulate, inner_product, partial_sum, iota              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7.13 課後練習

1. **思考題**：為什麼 `std::remove` 不直接刪除元素，而是需要配合 `erase` 使用？

2. **實作題**：使用 STL 演算法完成以下任務：
   - 給定一個整數 vector
   - 移除所有負數
   - 將剩餘的數字平方
   - 按降序排列
   - 計算總和

---

準備好進入**第八課：函數物件（Function Object）初探**了嗎？下一課我們會深入探討函數物件和 Lambda 表達式，了解它們如何讓演算法變得更加靈活。
*/



#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

struct Student {
    std::string name;
    int score;
};

int main() {
    std::vector<Student> students = {
        {"Alice", 85},
        {"Bob", 92},
        {"Charlie", 78},
        {"Diana", 95},
        {"Eve", 88},
        {"Frank", 72},
        {"Grace", 91}
    };
    
    // 1. 按分數排序（降序）
    std::cout << "=== 按分數排序 ===" << std::endl;
    std::sort(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.score > b.score;
        });
    
    for (const auto& s : students) {
        std::cout << s.name << ": " << s.score << std::endl;
    }
    
    // 2. 找出分數 >= 90 的學生
    std::cout << "\n=== 分數 >= 90 ===" << std::endl;
    auto high_it = std::find_if(students.begin(), students.end(),
        [](const Student& s) { return s.score < 90; });
    
    std::cout << "優秀學生: ";
    for (auto it = students.begin(); it != high_it; ++it) {
        std::cout << it->name << " ";
    }
    std::cout << std::endl;
    
    // 3. 計算平均分
    std::cout << "\n=== 平均分 ===" << std::endl;
    int total = std::accumulate(students.begin(), students.end(), 0,
        [](int sum, const Student& s) { return sum + s.score; });
    double average = static_cast<double>(total) / students.size();
    std::cout << "平均分: " << average << std::endl;
    
    // 4. 統計及格（>= 80）人數
    std::cout << "\n=== 及格人數 ===" << std::endl;
    int pass_count = std::count_if(students.begin(), students.end(),
        [](const Student& s) { return s.score >= 80; });
    std::cout << "及格人數: " << pass_count << "/" << students.size() << std::endl;
    
    // 5. 提取所有分數到另一個 vector
    std::cout << "\n=== 提取分數 ===" << std::endl;
    std::vector<int> scores(students.size());
    std::transform(students.begin(), students.end(), scores.begin(),
        [](const Student& s) { return s.score; });
    
    std::cout << "所有分數: ";
    for (int score : scores) std::cout << score << " ";
    std::cout << std::endl;
    
    // 6. 找最高分和最低分
    std::cout << "\n=== 最高最低分 ===" << std::endl;
    auto minmax = std::minmax_element(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.score < b.score;
        });
    
    std::cout << "最高分: " << minmax.second->name << " (" << minmax.second->score << ")" << std::endl;
    std::cout << "最低分: " << minmax.first->name << " (" << minmax.first->score << ")" << std::endl;
    
    return 0;
}
