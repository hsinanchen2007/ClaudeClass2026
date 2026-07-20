// =============================================================================
//  第一課：STL 的歷史與設計哲學 6  —  本課完整講義 + 收束範例
// =============================================================================
//
// 【主題資訊 Information】
//   本檔前半是第一課的完整講義（保存在下方的區塊註解裡），
//   後半是把整課收束起來的可執行範例，用到三個演算法：
//     std::sort        <algorithm>  O(N log N)，IntroSort
//     std::max_element <algorithm>  O(N)，回傳「迭代器」不是值
//     std::accumulate  <numeric>    O(N)，回傳型別由 init 參數決定 ★
//   標準版本：三者皆 C++98。注意 accumulate 在 <numeric> 不是 <algorithm>。
//
// 【詳細解釋 Explanation】
//
// 【1. max_element 為什麼回傳迭代器而不是值】
//   如果它回傳值，就必須複製一份元素出來——對 std::string 或大型結構
//   是無謂的開銷。回傳迭代器則有三個好處：
//     ① 零複製：要值就 *it，不要就不付費。
//     ② 知道「位置」：常常你要的不只是最大值，而是「誰最大」。
//        有了迭代器就能 it - v.begin() 算出索引。
//     ③ 能表達「空區間」：空的時候回傳 end()，不必發明特殊值。
//   代價是呼叫端必須先確認 v 非空，否則 *v.end() 是未定義行為。
//   本檔的範例在解參考前一律先檢查 empty()。
//
// 【2. accumulate 的回傳型別由 init 決定 —— 最容易踩的一個坑】
//   簽章是：
//       template<class InputIt, class T>
//       T accumulate(InputIt first, InputIt last, T init);
//   注意回傳型別是 T，也就是「你傳進去的 init 的型別」，
//   而不是元素的型別。這造成兩個常見災難：
//     ① std::accumulate(v.begin(), v.end(), 0)
//        對 vector<long long> 求和 → T 推導成 int → 累加過程就在 int 裡
//        進行 → 大數值直接有號溢位（未定義行為）。
//        正解：傳 0LL，或 int64_t{0}。
//     ② std::accumulate(v.begin(), v.end(), 0)
//        對 vector<double> 求和 → T 是 int → 每一步都截斷成整數！
//        正解：傳 0.0。
//   這個設計不是失誤，而是刻意的：它讓你能用「較寬的型別」累加
//   「較窄的元素」，避免中途溢位。但預設值不會替你想，你得自己指定。
//
// 【3. 為什麼 accumulate 住在 <numeric> 而不是 <algorithm>】
//   STL 把「純粹重排/查詢元素」的演算法放 <algorithm>，
//   把「對元素做數值運算」的放 <numeric>（accumulate、inner_product、
//   partial_sum、adjacent_difference、iota）。
//   這是介面分割：只做搜尋排序的程式不必為數值演算法付出編譯成本。
//   實務上這個分界常被忘記，忘了 include <numeric> 是很常見的編譯錯誤。
//
// 【4. 本檔講義中兩處日期互相矛盾，引用前請查證】
//   下方講義的時間軸寫「1994 年 7 月 STL 被正式納入 C++ 標準草案」，
//   但正文又寫「1994 年 11 月，Stepanov 在聖地牙哥向委員會展示」。
//   同一份文件裡，「展示」不可能晚於「納入」。
//   常見的說法是：Stepanov 於 1993 年 11 月的 San Jose 會議首次提案，
//   1994 年 7 月的 San Diego 會議通過納入草案。
//   本檔保留原講義文字不動，但在此標注此矛盾——教材裡的歷史細節
//   應以原始會議文件（WG21 papers）為準，不要直接引用二手整理。
//
// 【概念補充 Concept Deep Dive】
//
// (A) accumulate 其實是一個 fold（摺疊）
//     它的第四參數版本可以傳入任意二元運算：
//         std::accumulate(v.begin(), v.end(), 1LL, std::multiplies<long long>());
//     這就是函數式語言裡的 foldl。用它做「連乘」「求最大」「字串串接」
//     都成立。理解成「摺疊」而不是「加總」，才不會被名字誤導。
//
// (B) accumulate 是循序的，reduce 才可平行化（C++17）
//     accumulate 保證「由左至右、依序」計算，所以即使運算可交換也不能
//     平行。C++17 的 std::reduce 放寬了順序要求，才能配合執行策略
//     std::execution::par 平行執行。代價是：對浮點數而言，
//     不同的結合順序會產生不同的捨入結果。
//
// (C) 為什麼 max_element 對相同的最大值回傳「第一個」
//     標準明訂：若有多個元素等於最大值，回傳第一個。
//     這保證了演算法的結果是確定的（不因實作而異）。
//     對應的 min_element 也是回傳第一個；
//     而 minmax_element 的 max 部分回傳的是「最後一個」——
//     這個不對稱是為了讓 minmax_element 能用在穩定分割的場合，
//     是少數需要背下來的例外。
//
// 【注意事項 Pay Attention】
//   1. max_element 對空區間回傳 end()，解參考 end() 是未定義行為。
//      一律先 if (!v.empty()) 或比較 it != v.end()。
//   2. accumulate 的 init 決定回傳型別與運算型別。整數求和請用 0LL，
//      浮點求和請用 0.0。這是實務上最常見的溢位/截斷來源。
//   3. accumulate 在 <numeric>，不是 <algorithm>。
//   4. 講義中的歷史日期有內部矛盾（見上方【4.】），引用前請查證。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】max_element 與 accumulate
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::accumulate(v.begin(), v.end(), 0) 對一個 vector<double>
//        求和，結果會是什麼？
//     答：會得到截斷後的整數和。因為回傳型別 T 是由第三個參數推導的，
//         傳 0 讓 T = int，於是每一步累加都在 int 裡進行，
//         每個 double 都被截斷。正解是傳 0.0。
//     追問：那 vector<long long> 傳 0 會怎樣？
//         → 累加在 int 中進行，總和超過 INT_MAX 就是有號整數溢位，
//           屬未定義行為。正解是傳 0LL。
//
// 🔥 Q2. 為什麼 std::max_element 回傳迭代器而不是直接回傳最大值？
//     答：三個理由。① 避免無謂複製（大物件複製很貴）；
//         ② 呼叫端往往需要知道「位置」而不只是「值」，
//            有迭代器才能算索引或就地修改；
//         ③ 空區間有現成的表示法（回傳 end()），不必發明特殊值。
//     追問：那空 vector 呼叫 max_element 會怎樣？
//         → 回傳 end()。此時解參考是未定義行為，必須先判斷。
//
// ⚠️ 陷阱. `int sum = std::accumulate(v.begin(), v.end(), 0);`
//         v 是 vector<int>，元素都在 int 範圍內，所以這樣寫很安全——對嗎？
//     答：不對。個別元素在範圍內，不代表「總和」在範圍內。
//         一百萬筆平均值三千的訂單金額，每筆都是合法的 int，
//         總和卻是 30 億，超過 INT_MAX（2147483647）→ 有號溢位 → UB。
//         正解是 `long long sum = std::accumulate(v.begin(), v.end(), 0LL);`
//     為什麼會錯：直覺檢查的是「元素的範圍」，但溢位發生在「累加器」。
//         accumulate 的累加器型別完全由 init 決定，
//         元素型別再安全也救不了一個 int 的累加器。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第一課：STL 的歷史與設計哲學

---

## 1.1 什麼是 STL？

STL 全名是 **Standard Template Library**（標準模板庫），它是 C++ 標準函式庫中最核心的部分之一。

簡單來說，STL 提供了一套**經過高度優化、可重複使用**的程式元件，讓你不必從頭實作常見的資料結構（如動態陣列、鏈結串列、二元樹）和演算法（如排序、搜尋、複製）。

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // 不用自己寫動態陣列 - 用 vector
    std::vector<int> numbers = {5, 2, 8, 1, 9};
    
    // 不用自己寫排序 - 用 sort
    std::sort(numbers.begin(), numbers.end());
    
    // 不用自己寫搜尋 - 用 find
    auto it = std::find(numbers.begin(), numbers.end(), 8);
    
    if (it != numbers.end()) {
        std::cout << "找到了: " << *it << std::endl;
    }
    
    return 0;
}
```

**輸出：**
```
找到了: 8
```

在你過去寫 C 語言時，如果需要一個可以動態增長的陣列，你可能要自己用 `malloc`、`realloc` 來管理記憶體。有了 STL，這些底層細節都被封裝好了。

---

## 1.2 STL 的歷史

### 起源：Alexander Stepanov 的願景

STL 的創造者是 **Alexander Stepanov**（亞歷山大·斯捷潘諾夫），一位出生於蘇聯的電腦科學家。

```
時間軸：

1970年代  Stepanov 開始思考「泛型編程」的概念
    ↓
1979年    在莫斯科開始研究抽象演算法
    ↓
1985年    移居美國，在 GE 研究中心繼續研究
    ↓
1987年    在 Bell Labs 與 Andrew Koenig 合作
    ↓
1992年    加入 HP 實驗室，與 Meng Lee 合作開發 STL
    ↓
1994年    向 C++ 標準委員會提案
    ↓
1994年7月 STL 被正式納入 C++ 標準草案
    ↓
1998年    隨 C++98 標準正式發布
```

### 關鍵時刻：1994 年的提案

1994 年 11 月，Stepanov 在聖地牙哥向 C++ 標準委員會展示了 STL。這是一個關鍵時刻——委員會只花了幾天就決定將 STL 納入標準。

這個決定之所以快速，是因為 STL 展現了一種**革命性的程式設計思維**：

> 「演算法不應該依賴於特定的資料結構，資料結構也不應該綁定特定的演算法。」

---

## 1.3 STL 的設計哲學

### 核心理念：泛型編程（Generic Programming）

STL 的設計哲學可以用一句話概括：

> **「用最少的程式碼，解決最多的問題。」**

這是透過**泛型編程**達成的。讓我們看一個對比：

#### 沒有泛型的世界（C 語言風格）

```cpp
// 如果沒有泛型，你需要為每種型別寫一個排序函數
void sort_int(int* arr, int size) {
    // 排序 int 的邏輯
}

void sort_double(double* arr, int size) {
    // 排序 double 的邏輯（幾乎相同的程式碼）
}

void sort_string(char** arr, int size) {
    // 排序 string 的邏輯（又是幾乎相同的程式碼）
}
```

#### 有泛型的世界（STL 風格）

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

int main() {
    // 同一個 sort，處理所有型別
    std::vector<int> ints = {3, 1, 4, 1, 5};
    std::vector<double> doubles = {3.14, 1.41, 2.72};
    std::vector<std::string> strings = {"cherry", "apple", "banana"};
    
    std::sort(ints.begin(), ints.end());
    std::sort(doubles.begin(), doubles.end());
    std::sort(strings.begin(), strings.end());
    
    std::cout << "排序後的整數: ";
    for (int n : ints) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "排序後的浮點數: ";
    for (double d : doubles) std::cout << d << " ";
    std::cout << std::endl;
    
    std::cout << "排序後的字串: ";
    for (const auto& s : strings) std::cout << s << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
排序後的整數: 1 1 3 4 5 
排序後的浮點數: 1.41 2.72 3.14 
排序後的字串: apple banana cherry 
```

一個 `std::sort` 就能處理所有型別——這就是泛型編程的威力。

---

## 1.4 三個設計原則

Stepanov 在設計 STL 時，遵循了三個核心原則：

### 原則一：正交性（Orthogonality）

**容器**和**演算法**是獨立的兩個維度，透過**迭代器**連接。

```
┌─────────────────────────────────────────────────────────────┐
│                        STL 正交設計                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   容器 (Containers)          演算法 (Algorithms)            │
│   ┌───────────┐              ┌───────────┐                 │
│   │  vector   │              │   sort    │                 │
│   │  list     │◄────────────►│   find    │                 │
│   │  deque    │   迭代器     │   copy    │                 │
│   │  set      │  (Iterator)  │   count   │                 │
│   │  map      │              │   ...     │                 │
│   └───────────┘              └───────────┘                 │
│                                                             │
│   M 個容器 × N 個演算法 = M × N 種組合                        │
│   但只需要實作 M + N 個元件！                                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

假設有 10 個容器和 50 個演算法：
- **傳統做法**：需要實作 10 × 50 = 500 個函數
- **STL 做法**：只需要實作 10 + 50 = 60 個元件

這就是正交性帶來的效率。

### 原則二：效率優先（Efficiency）

STL 的設計目標是：

> **「泛型程式碼的效能應該與手寫特化程式碼相當。」**

這意味著使用 `std::sort` 應該和你自己寫的最佳化排序一樣快。STL 透過 **template** 在編譯期展開，避免了執行期的效能損失。

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

int main() {
    const int SIZE = 1000000;
    std::vector<int> data(SIZE);
    
    // 填充隨機數據
    for (int i = 0; i < SIZE; ++i) {
        data[i] = rand();
    }
    
    // 測量 std::sort 的時間
    auto start = std::chrono::high_resolution_clock::now();
    std::sort(data.begin(), data.end());
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "排序 " << SIZE << " 個元素耗時: " 
              << duration.count() << " 毫秒" << std::endl;
    
    return 0;
}
```

**可能的輸出：**
```
排序 1000000 個元素耗時: 62 毫秒
```

`std::sort` 內部使用 **IntroSort**（混合排序），結合了 QuickSort、HeapSort 和 InsertionSort 的優點，是目前已知最有效率的通用排序演算法之一。

### 原則三：可組合性（Composability）

STL 的元件可以像樂高積木一樣組合：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

int main() {
    std::vector<int> source = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> destination;
    
    // 組合多個 STL 元件：
    // 1. copy_if - 演算法
    // 2. back_inserter - 迭代器配接器
    // 3. lambda - 函數物件
    std::copy_if(
        source.begin(),           // 來源開始
        source.end(),             // 來源結束
        std::back_inserter(destination),  // 目標（自動擴展）
        [](int n) { return n % 2 == 0; }  // 條件：偶數
    );
    
    std::cout << "偶數: ";
    for (int n : destination) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
偶數: 2 4 6 8 10 
```

這段程式碼組合了：
- `copy_if` 演算法
- `back_inserter` 迭代器配接器
- Lambda 表達式作為判斷條件

---

## 1.5 STL 與傳統 C 風格的對比

讓我們用一個完整的例子來展示 STL 如何簡化程式碼：

### 任務：讀取數字、排序、找最大值、計算總和

#### C 風格寫法

```cpp
#include <stdio.h>
#include <stdlib.h>

// 比較函數（給 qsort 用）
int compare(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

int main() {
    int* numbers = NULL;
    int capacity = 4;
    int size = 0;
    
    // 動態配置
    numbers = (int*)malloc(capacity * sizeof(int));
    if (numbers == NULL) {
        fprintf(stderr, "記憶體配置失敗\n");
        return 1;
    }
    
    // 模擬輸入
    int inputs[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    int input_count = sizeof(inputs) / sizeof(inputs[0]);
    
    for (int i = 0; i < input_count; ++i) {
        // 需要擴展嗎？
        if (size >= capacity) {
            capacity *= 2;
            int* temp = (int*)realloc(numbers, capacity * sizeof(int));
            if (temp == NULL) {
                fprintf(stderr, "記憶體重新配置失敗\n");
                free(numbers);
                return 1;
            }
            numbers = temp;
        }
        numbers[size++] = inputs[i];
    }
    
    // 排序
    qsort(numbers, size, sizeof(int), compare);
    
    // 找最大值、計算總和
    int max = numbers[0];
    int sum = 0;
    for (int i = 0; i < size; ++i) {
        if (numbers[i] > max) max = numbers[i];
        sum += numbers[i];
    }
    
    printf("排序後: ");
    for (int i = 0; i < size; ++i) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    printf("最大值: %d\n", max);
    printf("總和: %d\n", sum);
    
    // 別忘了釋放記憶體！
    free(numbers);
    
    return 0;
}
```

#### STL 風格寫法

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 排序
    std::sort(numbers.begin(), numbers.end());
    
    // 找最大值
    int max = *std::max_element(numbers.begin(), numbers.end());
    
    // 計算總和
    int sum = std::accumulate(numbers.begin(), numbers.end(), 0);
    
    std::cout << "排序後: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    std::cout << "最大值: " << max << std::endl;
    std::cout << "總和: " << sum << std::endl;
    
    return 0;
}
```

**兩種寫法輸出相同：**
```
排序後: 1 2 3 4 5 6 7 8 9 
最大值: 9
總和: 45
```

### 對比分析

| 面向 | C 風格 | STL 風格 |
|------|--------|----------|
| 程式碼行數 | ~50 行 | ~20 行 |
| 記憶體管理 | 手動 malloc/realloc/free | 自動 |
| 記憶體洩漏風險 | 高 | 極低 |
| 可讀性 | 需要理解指標操作 | 意圖清晰 |
| 維護成本 | 高 | 低 |

---

## 1.6 本課重點整理

```
┌─────────────────────────────────────────────────────────────┐
│                     第一課 重點整理                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. STL = Standard Template Library                         │
│     → C++ 標準函式庫的核心部分                               │
│                                                             │
│  2. 創造者：Alexander Stepanov                              │
│     → 1994 年被納入 C++ 標準                                │
│                                                             │
│  3. 核心理念：泛型編程                                       │
│     → 一套程式碼，處理多種型別                               │
│                                                             │
│  4. 三大設計原則：                                           │
│     ① 正交性：容器與演算法獨立，透過迭代器連接               │
│     ② 效率優先：泛型不犧牲效能                              │
│     ③ 可組合性：元件可自由組合                              │
│                                                             │
│  5. STL 相比 C 風格：                                        │
│     → 更少程式碼、更安全、更易讀、更易維護                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 1.7 課後思考

在進入下一課之前，思考這個問題：

> 你在工作中使用 C++ 時，有沒有遇過需要自己管理動態陣列或實作排序的情況？如果有，那些程式碼是否可以用 STL 簡化？

---

準備好進入**第二課：泛型編程（Generic Programming）概念**了嗎？下一課我們會深入探討 template 如何讓 STL 的「一套程式碼處理多種型別」成為可能。
*/



#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <climits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array
//   題目：給定陣列 nums，回傳 runningSum，其中 runningSum[i] = nums[0]+…+nums[i]。
//         例如 [1,2,3,4] -> [1,3,6,10]。
//   為什麼用到本主題：這題就是 accumulate 的「保留中間結果」版本——
//     std::partial_sum，兩者同住 <numeric>、同屬「摺疊」家族。
//     accumulate 只吐最後一個值，partial_sum 把每一步都寫出來。
//     一併示範可以讓讀者看清 <numeric> 這組演算法的分工。
//   複雜度：O(N) 時間、O(1) 額外空間（就地做）。
// -----------------------------------------------------------------------------
std::vector<int> runningSum(std::vector<int> nums) {
    std::partial_sum(nums.begin(), nums.end(), nums.begin());  // 就地覆寫
    return nums;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】結算一天的訂單金額（accumulate 的溢位陷阱現場）
//   情境：電商後台每日結帳，把當天所有訂單金額加總。
//         每筆金額都是合理的 int（幾百到幾萬元），
//         但單日訂單量一大，總和就會衝破 INT_MAX。
//   為什麼用到本主題：這是 accumulate 最真實、也最常出事的用法。
//     init 傳 0 還是 0LL，決定了這支結算程式會不會在業績變好的那天爆掉。
//   下面兩個函式刻意並列，main 會實測出它們的差異。
// -----------------------------------------------------------------------------
long long totalRevenueSafe(const std::vector<int>& amounts) {
    // 正解：init 傳 0LL → 累加器是 long long，中途不會溢位
    return std::accumulate(amounts.begin(), amounts.end(), 0LL);
}

long long totalRevenueByHand(const std::vector<int>& amounts) {
    // 對照組：手動用 long long 累加，證明上面那個才是「數學上正確」的答案
    long long s = 0;
    for (int a : amounts) s += a;
    return s;
}

int main() {
    std::vector<int> numbers = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // 排序
    std::sort(numbers.begin(), numbers.end());

    // 找最大值。max_element 回傳的是「迭代器」，要解參考才拿到值。
    // 空區間會回傳 end()，解參考 end() 是 UB —— 所以先檢查 empty()。
    if (numbers.empty()) {
        std::cout << "沒有資料\n";
        return 0;
    }
    auto maxIt = std::max_element(numbers.begin(), numbers.end());
    int max = *maxIt;

    // 計算總和
    int sum = std::accumulate(numbers.begin(), numbers.end(), 0);

    std::cout << "=== 原始示範：sort + max_element + accumulate ===\n";
    std::cout << "排序後: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    std::cout << "最大值: " << max << std::endl;
    std::cout << "總和: " << sum << std::endl;

    // 迭代器比「值」多帶了位置資訊
    std::cout << "最大值所在索引: " << (maxIt - numbers.begin()) << "\n";

    std::cout << "\n=== accumulate 的回傳型別由 init 決定 ===\n";
    std::vector<double> prices = {1.5, 2.5, 3.25};
    std::cout << "vector<double> 求和，init 傳 0  : "
              << std::accumulate(prices.begin(), prices.end(), 0) << "  ← 每步都被截斷成 int\n";
    std::cout << "vector<double> 求和，init 傳 0.0: "
              << std::accumulate(prices.begin(), prices.end(), 0.0) << "  ← 正確\n";

    std::cout << "\n=== accumulate 是「摺疊」，不只會加法 ===\n";
    std::vector<int> small = {1, 2, 3, 4, 5};
    std::cout << "連乘 (init=1LL, multiplies): "
              << std::accumulate(small.begin(), small.end(), 1LL,
                                 std::multiplies<long long>()) << "\n";
    std::vector<std::string> words = {"STL", "-", "is", "-", "generic"};
    std::cout << "字串串接 (init=空字串): "
              << std::accumulate(words.begin(), words.end(), std::string{}) << "\n";

    std::cout << "\n=== LeetCode 1480 Running Sum of 1d Array ===\n";
    for (auto nums : std::vector<std::vector<int>>{{1, 2, 3, 4}, {1, 1, 1, 1, 1}, {3, 1, 2, 10, 1}}) {
        std::cout << "[";
        for (size_t i = 0; i < nums.size(); ++i) std::cout << (i ? "," : "") << nums[i];
        std::cout << "] -> [";
        auto rs = runningSum(nums);
        for (size_t i = 0; i < rs.size(); ++i) std::cout << (i ? "," : "") << rs[i];
        std::cout << "]\n";
    }

    std::cout << "\n=== 日常實務：單日訂單結算（accumulate 的 init 型別）===\n";
    {
        // 100 萬筆訂單，每筆 3000 元 —— 每筆都是合法的 int
        std::vector<int> amounts(1000000, 3000);

        long long correct = totalRevenueByHand(amounts);
        long long safe    = totalRevenueSafe(amounts);

        std::cout << "訂單筆數        : " << amounts.size() << "\n";
        std::cout << "每筆金額        : " << amounts[0] << "（遠小於 INT_MAX）\n";
        std::cout << "INT_MAX         : " << INT_MAX << "\n";
        std::cout << "手動 long long 加總 : " << correct << "\n";
        std::cout << "accumulate(…, 0LL)  : " << safe << "\n";
        std::cout << "兩者一致        : " << std::boolalpha << (correct == safe) << "\n";
        std::cout << "總和是否超過 INT_MAX: " << (correct > INT_MAX) << "\n";
        std::cout << "→ 若寫成 accumulate(…, 0)，累加器是 int，這裡就會有號溢位；\n";
        std::cout << "  那是未定義行為，不保證任何特定數值，所以本檔不示範它的「結果」。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第一課：STL 的歷史與設計哲學6.cpp -o demo6

// === 預期輸出 ===
// === 原始示範：sort + max_element + accumulate ===
// 排序後: 1 2 3 4 5 6 7 8 9
// 最大值: 9
// 總和: 45
// 最大值所在索引: 8
//
// === accumulate 的回傳型別由 init 決定 ===
// vector<double> 求和，init 傳 0  : 6  ← 每步都被截斷成 int
// vector<double> 求和，init 傳 0.0: 7.25  ← 正確
//
// === accumulate 是「摺疊」，不只會加法 ===
// 連乘 (init=1LL, multiplies): 120
// 字串串接 (init=空字串): STL-is-generic
//
// === LeetCode 1480 Running Sum of 1d Array ===
// [1,2,3,4] -> [1,3,6,10]
// [1,1,1,1,1] -> [1,2,3,4,5]
// [3,1,2,10,1] -> [3,4,6,16,17]
//
// === 日常實務：單日訂單結算（accumulate 的 init 型別）===
// 訂單筆數        : 1000000
// 每筆金額        : 3000（遠小於 INT_MAX）
// INT_MAX         : 2147483647
// 手動 long long 加總 : 3000000000
// accumulate(…, 0LL)  : 3000000000
// 兩者一致        : true
// 總和是否超過 INT_MAX: true
// → 若寫成 accumulate(…, 0)，累加器是 int，這裡就會有號溢位；
//   那是未定義行為，不保證任何特定數值，所以本檔不示範它的「結果」。
