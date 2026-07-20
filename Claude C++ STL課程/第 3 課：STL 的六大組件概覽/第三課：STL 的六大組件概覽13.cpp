// =============================================================================
//  第三課：STL 的六大組件概覽 13  —  本課教科書：六大組件的整合與協作
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是第三課的完整講義（下方 /* ... */ 內為全文），
//   最後附一支把六大組件全部串起來的整合範例。
//   六大組件與各自的標頭檔：
//     1. 容器 Containers    <vector> <list> <deque> <set> <map> <unordered_*> ...
//     2. 迭代器 Iterators   <iterator>（容器自帶 begin/end；配接器在此）
//     3. 演算法 Algorithms  <algorithm> <numeric>
//     4. 函數物件 Functors  <functional>（lambda 為語言特性，不需標頭檔）
//     5. 配接器 Adapters    <stack> <queue>（容器配接器）、<iterator>（迭代器配接器）
//     6. 配置器 Allocators  <memory>
//   標準版本：六大組件框架自 C++98；lambda C++11；透明比較器 C++14；
//             std::pmr C++17；ranges（迭代器模型的現代化包裝）C++20。
//
// 【詳細解釋 Explanation】
//
// 【1. 六大組件不是六個並列的工具箱，而是一個有方向的架構】
//   真正的依賴關係是：
//       配置器  →  容器  →（產生）迭代器  ←（消費）演算法  ←  函數物件
//                              ↑
//                            配接器（包裝容器或迭代器，改變其介面）
//   讀法：
//     - 配置器在最底層，決定記憶體從哪來；容器用它管理元素。
//     - 容器對外只暴露迭代器，不暴露內部結構。
//     - 演算法只認迭代器，因此完全不知道容器是誰。
//     - 函數物件是演算法的「可替換零件」，決定比較/過濾/轉換的具體行為。
//     - 配接器是黏著劑：容器配接器限制介面（stack），
//       迭代器配接器把非迭代器的東西偽裝成迭代器（back_inserter、ostream_iterator）。
//
// 【2. 這個架構解決的核心問題：M×N → M+N】
//   若沒有迭代器這層抽象，M 個容器與 N 個演算法需要 M×N 份實作。
//   有了它，容器只需實作「產生合格迭代器」，演算法只需針對迭代器類別撰寫，
//   總量降為 M+N。這也是為什麼你自己寫的類別只要提供 begin()/end()，
//   立刻能使用全部 STL 演算法與範圍 for —— 不需要繼承任何基底類別。
//
// 【3. 為什麼是編譯期多型而不是繼承】
//   Java 的做法是讓所有容器實作 Collection 介面，靠虛擬函式做執行期多型。
//   STL 選擇樣板：
//     優點 —— 零額外執行期成本（無虛擬函式表、無間接跳躍、可完全 inline）；
//             元素型別不必是 Object，int 就是 int，不需要裝箱。
//     缺點 —— 二進位檔膨脹（每個型別各一份實例化）、編譯變慢、
//             型別錯誤訊息極長（C++20 的 concepts 就是為此而生）。
//   理解這個取捨，就理解了 STL 為什麼長成這樣。
//
// 【4. 六大組件在一支程式中的實際協作】
//   本檔最後的 main() 刻意讓六者同時登場：
//       vector<int>                       ← 容器（背後有 allocator 配置記憶體）
//       numbers.begin() / end()           ← 迭代器
//       sort / copy_if / transform        ← 演算法
//       greater<int>() / lambda           ← 函數物件
//       back_inserter / ostream_iterator  ← 迭代器配接器
//   六個組件、五行程式碼，這就是 STL 的表達力。
//
// 【概念補充 Concept Deep Dive】
//   「哪個演算法需要哪一級迭代器」是理解整個架構的關鍵表格：
//       Input Iterator        find / count / accumulate / equal
//       Output Iterator       copy / transform 的**目的地**
//       Forward Iterator      replace / remove / unique / rotate
//       Bidirectional         reverse / copy_backward / next_permutation
//       Random Access         sort / nth_element / binary_search / random_shuffle
//   要求越高的演算法，能用的容器越少：
//       sort 只能用在 vector / deque / array / 原生陣列；
//       list 與 forward_list 必須改用自己的成員函式 sort()。
//   反過來說，寫泛型程式碼時應該**只要求你真正需要的最低等級**，
//   這樣才能服務最多的容器 —— 這是 STL 設計者留下的一條實用準則。
//
// 【注意事項 Pay Attention】
//   1. 演算法不會改變容器大小。std::remove 只是把元素往前搬並回傳新邏輯尾端，
//      真的刪除必須配合 erase（erase-remove 慣用法）。
//   2. 容器有同名成員函式時優先用成員版：lst.sort() / lst.remove() / s.find()
//      都比泛型版更快，且提供更強的迭代器有效性保證。
//   3. 容器配接器（stack/queue/priority_queue）**沒有迭代器**，
//      不能用範圍 for，也不能套任何 STL 演算法。
//   4. transform / copy 的目的地必須已有足夠元素，或使用 back_inserter；
//      對空 vector 傳 begin() 是未定義行為。
//   5. 修改容器可能使迭代器失效，且各容器規則不同
//      （vector 的 push_back 可能全部失效；list 的插入完全不失效）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 六大組件與整體架構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 的六大組件是哪些？它們之間的依賴關係是什麼？
//     答：容器、迭代器、演算法、函數物件、配接器、配置器。
//         關係是：配置器供給容器記憶體；容器對外只暴露迭代器；
//         演算法只透過迭代器工作，完全不認識容器；
//         函數物件是演算法的可替換零件（比較/過濾/轉換）；
//         配接器則包裝容器（stack 限制介面）或包裝其他東西成迭代器
//         （back_inserter 讓 copy 能自動 push_back）。
//     追問：這個設計最大的好處是什麼？
//           → 把 M×N 份實作降為 M+N。而且自訂類別只要提供合格的 begin()/end()，
//             不必繼承任何東西就能免費用上全部 STL 演算法。
//
// 🔥 Q2. STL 為什麼用樣板（編譯期多型）而不是繼承（執行期多型）？
//     答：為了零額外執行期成本 —— 沒有虛擬函式表、沒有間接跳躍、
//         operator() 與 ++it 都能完全 inline；而且 int 就是 int，不需要裝箱。
//         代價是二進位檔膨脹、編譯變慢、型別錯誤訊息冗長。
//         這是 C++「不為你沒用到的東西付錢」哲學的直接體現。
//     追問：那 C++20 的 concepts 在這裡解決了什麼？
//           → 把「這個樣板參數需要滿足什麼條件」寫進宣告，
//             於是錯誤在呼叫點就被攔下並給出人話訊息，
//             而不是在樣板實例化的第 20 層才爆出幾百行錯誤。
//
// ⚠️ 陷阱. 「我用 std::remove 把 vector 裡的 3 都刪掉了，為什麼 size() 沒變？」
//     答：因為 std::remove 是**演算法**，而演算法只透過迭代器工作 ——
//         它根本碰不到容器本身，自然無法改變 size。
//         它做的是把要保留的元素往前搬，回傳「新的邏輯尾端」，
//         尾端到 end() 之間的元素則處於「有效但未指定」的狀態。
//         真的要縮短容器必須自己呼叫成員函式：
//             v.erase(std::remove(v.begin(), v.end(), 3), v.end());
//     為什麼會錯：把 remove 想成容器操作。
//         這其實正好反映了「演算法與容器解耦」這個核心設計 ——
//         解耦的代價就是演算法無法做任何改變容器大小的事。
//         C++20 提供了 std::erase(v, 3) 一行完成，正是為了收掉這個長年陷阱。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第三課：STL 的六大組件概覽

---

## 3.1 STL 的整體架構

STL 由六大組件構成，它們彼此協作，形成一個強大而靈活的系統：

```
┌─────────────────────────────────────────────────────────────────┐
│                      STL 六大組件架構圖                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      ┌─────────────┐                            │
│                      │  配置器      │                            │
│                      │ (Allocator) │                            │
│                      └──────┬──────┘                            │
│                             │ 負責記憶體配置                     │
│                             ▼                                   │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│   │   演算法     │◄───│   迭代器     │───►│    容器     │        │
│   │ (Algorithm) │    │ (Iterator)  │    │ (Container) │        │
│   └──────┬──────┘    └─────────────┘    └──────┬──────┘        │
│          │                                      │               │
│          │ 使用                                 │ 可被包裝      │
│          ▼                                      ▼               │
│   ┌─────────────┐                       ┌─────────────┐        │
│   │  函數物件    │                       │   配接器    │        │
│   │ (Functor)   │                       │  (Adapter)  │        │
│   └─────────────┘                       └─────────────┘        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

讓我們逐一認識這六大組件。

---

## 3.2 組件一：容器（Containers）

**容器**是用來儲存和管理資料的類別模板。

### 容器的分類

```
┌─────────────────────────────────────────────────────────────────┐
│                        容器分類總覽                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  序列容器（Sequence Containers）                                 │
│  ├── array          固定大小陣列                                │
│  ├── vector         動態陣列（最常用）                          │
│  ├── deque          雙端佇列                                    │
│  ├── list           雙向鏈結串列                                │
│  └── forward_list   單向鏈結串列                                │
│                                                                 │
│  關聯容器（Associative Containers）                              │
│  ├── set            有序集合（不重複）                          │
│  ├── multiset       有序集合（可重複）                          │
│  ├── map            有序鍵值對（不重複鍵）                      │
│  └── multimap       有序鍵值對（可重複鍵）                      │
│                                                                 │
│  無序容器（Unordered Containers）                                │
│  ├── unordered_set       雜湊集合（不重複）                     │
│  ├── unordered_multiset  雜湊集合（可重複）                     │
│  ├── unordered_map       雜湊鍵值對（不重複鍵）                 │
│  └── unordered_multimap  雜湊鍵值對（可重複鍵）                 │
│                                                                 │
│  容器配接器（Container Adapters）                                │
│  ├── stack          後進先出（LIFO）                            │
│  ├── queue          先進先出（FIFO）                            │
│  └── priority_queue 優先佇列                                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 容器快速示範

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>

int main() {
    // 序列容器：vector
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 序列容器：list
    std::list<int> lst = {10, 20, 30};
    lst.push_front(5);  // list 可以高效地在前端插入
    std::cout << "list: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 關聯容器：set（自動排序、不重複）
    std::set<int> s = {30, 10, 20, 10, 30};  // 重複的會被忽略
    std::cout << "set: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    // 關聯容器：map（鍵值對）
    std::map<std::string, int> ages;
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    std::cout << "map: ";
    for (const auto& pair : ages) {
        std::cout << pair.first << "=" << pair.second << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
vector: 1 2 3 4 5 
list: 5 10 20 30 
set: 10 20 30 
map: Alice=25 Bob=30 Charlie=35 
```

---

## 3.3 組件二：迭代器（Iterators）

**迭代器**是連接容器與演算法的橋樑，它提供了一種統一的方式來遍歷容器中的元素。

### 迭代器的概念

你可以把迭代器想像成「泛化的指標」：

```
┌─────────────────────────────────────────────────────────────────┐
│                      迭代器概念圖解                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   容器內容：  [ 10 | 20 | 30 | 40 | 50 ]                        │
│                 ▲                    ▲                          │
│                 │                    │                          │
│              begin()              end()                         │
│              (指向第一個)        (指向最後一個的下一位)          │
│                                                                 │
│   遍歷過程：                                                    │
│                                                                 │
│   it = begin()  →  *it = 10                                    │
│   ++it          →  *it = 20                                    │
│   ++it          →  *it = 30                                    │
│   ++it          →  *it = 40                                    │
│   ++it          →  *it = 50                                    │
│   ++it          →  it == end()  (結束)                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 迭代器基本操作

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 方法一：使用迭代器遍歷
    std::cout << "使用迭代器: ";
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";  // *it 取得迭代器指向的值
    }
    std::cout << std::endl;
    
    // 方法二：使用 auto 簡化
    std::cout << "使用 auto: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 方法三：範圍 for（底層也是迭代器）
    std::cout << "範圍 for: ";
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
使用迭代器: 10 20 30 40 50 
使用 auto: 10 20 30 40 50 
範圍 for: 10 20 30 40 50 
```

### 迭代器的五種類別

```
┌─────────────────────────────────────────────────────────────────┐
│                     迭代器類別層次                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   功能最強                                                      │
│      ▲                                                          │
│      │   Random Access Iterator（隨機存取迭代器）               │
│      │   • 支援 +n, -n, [], <, >                                │
│      │   • 代表：vector, deque, array                           │
│      │                                                          │
│      │   Bidirectional Iterator（雙向迭代器）                   │
│      │   • 支援 ++, --                                          │
│      │   • 代表：list, set, map                                 │
│      │                                                          │
│      │   Forward Iterator（前向迭代器）                         │
│      │   • 只支援 ++                                            │
│      │   • 代表：forward_list, unordered_set                    │
│      │                                                          │
│      │   Input Iterator（輸入迭代器）                           │
│      │   • 只能讀取，只能前進一次                               │
│      │   • 代表：istream_iterator                               │
│      │                                                          │
│      │   Output Iterator（輸出迭代器）                          │
│      │   • 只能寫入，只能前進一次                               │
│      │   • 代表：ostream_iterator                               │
│      ▼                                                          │
│   功能最弱                                                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 不同迭代器的能力差異

```cpp
#include <iostream>
#include <vector>
#include <list>

int main() {
    // vector 有 Random Access Iterator
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto vit = vec.begin();
    
    std::cout << "vector 迭代器可以：" << std::endl;
    std::cout << "  vit[2] = " << vit[2] << std::endl;      // 隨機存取
    std::cout << "  *(vit + 3) = " << *(vit + 3) << std::endl;  // 算術運算
    
    // list 只有 Bidirectional Iterator
    std::list<int> lst = {10, 20, 30, 40, 50};
    auto lit = lst.begin();
    
    std::cout << "\nlist 迭代器可以：" << std::endl;
    ++lit;  // 前進
    std::cout << "  ++lit → *lit = " << *lit << std::endl;
    --lit;  // 後退
    std::cout << "  --lit → *lit = " << *lit << std::endl;
    
    // 但 list 迭代器不能：
    // lit[2];      // 編譯錯誤！
    // lit + 3;     // 編譯錯誤！
    
    return 0;
}
```

**輸出：**
```
vector 迭代器可以：
  vit[2] = 30
  *(vit + 3) = 40

list 迭代器可以：
  ++lit → *lit = 20
  --lit → *lit = 10
```

---

## 3.4 組件三：演算法（Algorithms）

**演算法**是一系列對容器元素進行操作的函數模板。它們透過迭代器來操作容器，與容器本身解耦。

### 演算法的分類

```
┌─────────────────────────────────────────────────────────────────┐
│                       演算法分類總覽                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  非修改序列操作                                                  │
│  ├── find, find_if        查找元素                             │
│  ├── count, count_if      計數                                 │
│  ├── for_each             對每個元素執行操作                    │
│  └── all_of, any_of       條件判斷                             │
│                                                                 │
│  修改序列操作                                                    │
│  ├── copy, copy_if        複製元素                             │
│  ├── transform            轉換元素                             │
│  ├── fill, fill_n         填充                                 │
│  ├── replace              替換                                 │
│  └── remove, remove_if    移除                                 │
│                                                                 │
│  排序相關操作                                                    │
│  ├── sort, stable_sort    排序                                 │
│  ├── partial_sort         部分排序                             │
│  ├── nth_element          找第 n 個元素                        │
│  └── binary_search        二分搜尋                             │
│                                                                 │
│  數值操作（<numeric>）                                          │
│  ├── accumulate           累加                                 │
│  ├── inner_product        內積                                 │
│  └── partial_sum          部分和                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 演算法示範

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    
    // 排序
    std::sort(vec.begin(), vec.end());
    std::cout << "排序後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 查找
    auto it = std::find(vec.begin(), vec.end(), 7);
    if (it != vec.end()) {
        std::cout << "找到 7，位置: " << (it - vec.begin()) << std::endl;
    }
    
    // 計數
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2, 5};
    int count = std::count(data.begin(), data.end(), 2);
    std::cout << "2 出現次數: " << count << std::endl;
    
    // 累加
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;
    
    // 最大最小
    auto minmax = std::minmax_element(vec.begin(), vec.end());
    std::cout << "最小值: " << *minmax.first << std::endl;
    std::cout << "最大值: " << *minmax.second << std::endl;
    
    return 0;
}
```

**輸出：**
```
排序後: 1 2 3 4 5 6 7 8 9 
找到 7，位置: 6
2 出現次數: 4
總和: 45
最小值: 1
最大值: 9
```

### 演算法與容器的獨立性

同一個演算法可以用在不同容器上：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <algorithm>

template <typename Container>
void print_container(const std::string& name, const Container& c) {
    std::cout << name << ": ";
    for (const auto& elem : c) std::cout << elem << " ";
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {3, 1, 4, 1, 5};
    std::list<int> lst = {9, 2, 6, 5, 3};
    std::deque<int> deq = {5, 8, 9, 7, 9};
    
    // 同一個 sort 演算法用在 vector 和 deque
    std::sort(vec.begin(), vec.end());
    std::sort(deq.begin(), deq.end());
    
    // list 有自己的 sort（因為 std::sort 需要 Random Access Iterator）
    lst.sort();
    
    print_container("vector", vec);
    print_container("list", lst);
    print_container("deque", deq);
    
    // 同一個 find 演算法用在所有容器
    std::cout << "\n尋找元素 5：" << std::endl;
    
    auto vit = std::find(vec.begin(), vec.end(), 5);
    std::cout << "  vector: " << (vit != vec.end() ? "找到" : "沒找到") << std::endl;
    
    auto lit = std::find(lst.begin(), lst.end(), 5);
    std::cout << "  list: " << (lit != lst.end() ? "找到" : "沒找到") << std::endl;
    
    auto dit = std::find(deq.begin(), deq.end(), 5);
    std::cout << "  deque: " << (dit != deq.end() ? "找到" : "沒找到") << std::endl;
    
    return 0;
}
```

**輸出：**
```
vector: 1 1 3 4 5 
list: 2 3 5 6 9 
deque: 5 7 8 9 9 

尋找元素 5：
  vector: 找到
  list: 找到
  deque: 找到
```

---

## 3.5 組件四：函數物件（Function Objects / Functors）

**函數物件**是重載了 `operator()` 的類別實例，它可以像函數一樣被呼叫。

### 為什麼需要函數物件？

演算法常常需要一個「策略」來決定如何操作。例如：
- `sort` 需要知道如何比較兩個元素
- `find_if` 需要知道什麼條件算「找到」
- `transform` 需要知道如何轉換元素

### 函數物件 vs 函數指標

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

// 方法一：普通函數
bool is_even_func(int n) {
    return n % 2 == 0;
}

// 方法二：函數物件
class IsEven {
public:
    bool operator()(int n) const {
        return n % 2 == 0;
    }
};

// 方法三：帶狀態的函數物件
class IsDivisibleBy {
    int divisor;
public:
    IsDivisibleBy(int d) : divisor(d) {}
    bool operator()(int n) const {
        return n % divisor == 0;
    }
};

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 使用普通函數
    int count1 = std::count_if(vec.begin(), vec.end(), is_even_func);
    std::cout << "偶數個數（函數）: " << count1 << std::endl;
    
    // 使用函數物件
    int count2 = std::count_if(vec.begin(), vec.end(), IsEven());
    std::cout << "偶數個數（函數物件）: " << count2 << std::endl;
    
    // 使用帶狀態的函數物件
    int count3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    std::cout << "3的倍數個數: " << count3 << std::endl;
    
    int count4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(5));
    std::cout << "5的倍數個數: " << count4 << std::endl;
    
    return 0;
}
```

**輸出：**
```
偶數個數（函數）: 5
偶數個數（函數物件）: 5
3的倍數個數: 3
5的倍數個數: 2
```

### STL 內建的函數物件

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>  // 內建函數物件

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    
    // 預設排序（升序）
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 使用 greater<int> 降序排序
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // 其他內建函數物件
    std::cout << "\n內建函數物件示範：" << std::endl;
    std::cout << "plus<int>()(3, 4) = " << std::plus<int>()(3, 4) << std::endl;
    std::cout << "minus<int>()(10, 3) = " << std::minus<int>()(10, 3) << std::endl;
    std::cout << "multiplies<int>()(5, 6) = " << std::multiplies<int>()(5, 6) << std::endl;
    std::cout << "logical_and<bool>()(true, false) = " 
              << std::logical_and<bool>()(true, false) << std::endl;
    
    return 0;
}
```

**輸出：**
```
升序: 1 2 5 8 9 
降序: 9 8 5 2 1 

內建函數物件示範：
plus<int>()(3, 4) = 7
minus<int>()(10, 3) = 7
multiplies<int>()(5, 6) = 30
logical_and<bool>()(true, false) = 0
```

### Lambda 表達式：現代的函數物件

C++11 引入了 Lambda，讓函數物件的撰寫更簡潔：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Lambda 表達式
    int count = std::count_if(vec.begin(), vec.end(), 
        [](int n) { return n % 2 == 0; }  // Lambda
    );
    std::cout << "偶數個數: " << count << std::endl;
    
    // 帶捕獲的 Lambda
    int divisor = 3;
    int count3 = std::count_if(vec.begin(), vec.end(),
        [divisor](int n) { return n % divisor == 0; }  // 捕獲外部變數
    );
    std::cout << "3的倍數個數: " << count3 << std::endl;
    
    // 用 Lambda 做 transform
    std::vector<int> squared;
    squared.resize(vec.size());
    std::transform(vec.begin(), vec.end(), squared.begin(),
        [](int n) { return n * n; }
    );
    std::cout << "平方: ";
    for (int n : squared) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
偶數個數: 5
3的倍數個數: 3
平方: 1 4 9 16 25 36 49 64 81 100 
```

---

## 3.6 組件五：配接器（Adapters）

**配接器**是用來修改或包裝其他組件介面的工具。STL 有三類配接器：

### 容器配接器

```cpp
#include <iostream>
#include <stack>
#include <queue>

int main() {
    // stack：後進先出（LIFO）
    std::stack<int> stk;
    stk.push(1);
    stk.push(2);
    stk.push(3);
    
    std::cout << "stack (LIFO): ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;
    
    // queue：先進先出（FIFO）
    std::queue<int> que;
    que.push(1);
    que.push(2);
    que.push(3);
    
    std::cout << "queue (FIFO): ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();
    }
    std::cout << std::endl;
    
    // priority_queue：優先佇列（預設最大值優先）
    std::priority_queue<int> pq;
    pq.push(30);
    pq.push(10);
    pq.push(50);
    pq.push(20);
    
    std::cout << "priority_queue: ";
    while (!pq.empty()) {
        std::cout << pq.top() << " ";
        pq.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
stack (LIFO): 3 2 1 
queue (FIFO): 1 2 3 
priority_queue: 50 30 20 10 
```

### 迭代器配接器

```cpp
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // reverse_iterator：反向迭代
    std::cout << "反向迭代: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;
    
    // back_inserter：自動在尾端插入
    std::vector<int> src = {10, 20, 30};
    std::vector<int> dest;
    
    std::copy(src.begin(), src.end(), std::back_inserter(dest));
    std::cout << "back_inserter 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;
    
    // ostream_iterator：輸出到串流
    std::cout << "ostream_iterator: ";
    std::copy(vec.begin(), vec.end(), 
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
反向迭代: 5 4 3 2 1 
back_inserter 結果: 10 20 30 
ostream_iterator: 1 2 3 4 5 
```

### 函數配接器（C++11 後主要用 Lambda 取代）

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // std::bind：綁定參數
    auto is_greater_than_5 = std::bind(std::greater<int>(), 
                                        std::placeholders::_1, 5);
    
    int count = std::count_if(vec.begin(), vec.end(), is_greater_than_5);
    std::cout << "大於5的個數: " << count << std::endl;
    
    // 等價的 Lambda 寫法（更推薦）
    int count2 = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n > 5; }
    );
    std::cout << "大於5的個數（Lambda）: " << count2 << std::endl;
    
    return 0;
}
```

**輸出：**
```
大於5的個數: 5
大於5的個數（Lambda）: 5
```

---

## 3.7 組件六：配置器（Allocators）

**配置器**負責容器的記憶體配置與釋放。大多數情況下，你不需要直接使用它。

### 預設配置器

```cpp
#include <iostream>
#include <vector>
#include <memory>

int main() {
    // 所有容器都有一個預設的配置器
    // vector<int> 實際上是 vector<int, allocator<int>>
    
    std::vector<int> vec1;  // 使用預設配置器
    std::vector<int, std::allocator<int>> vec2;  // 明確指定（效果相同）
    
    vec1.push_back(1);
    vec2.push_back(2);
    
    std::cout << "vec1[0] = " << vec1[0] << std::endl;
    std::cout << "vec2[0] = " << vec2[0] << std::endl;
    
    // allocator 的基本用法
    std::allocator<int> alloc;
    
    // 配置記憶體（但不建構物件）
    int* ptr = alloc.allocate(5);  // 配置 5 個 int 的空間
    
    // 建構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::construct(alloc, ptr + i, i * 10);
    }
    
    std::cout << "手動配置的陣列: ";
    for (int i = 0; i < 5; ++i) {
        std::cout << ptr[i] << " ";
    }
    std::cout << std::endl;
    
    // 解構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::destroy(alloc, ptr + i);
    }
    
    // 釋放記憶體
    alloc.deallocate(ptr, 5);
    
    return 0;
}
```

**輸出：**
```
vec1[0] = 1
vec2[0] = 2
手動配置的陣列: 0 10 20 30 40 
```

配置器在進階應用中很有用，例如：
- 記憶體池（Memory Pool）
- 追蹤記憶體使用
- 特殊的記憶體區域（如共享記憶體）

目前你只需要知道它的存在，我們在第十六階段會深入探討。

---

## 3.8 六大組件的協作範例

讓我們用一個完整的例子展示六大組件如何協同工作：

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>

int main() {
    // 【容器】儲存資料
    std::vector<int> numbers = {64, 25, 12, 22, 11, 90, 42};
    
    std::cout << "原始資料: ";
    // 【迭代器配接器】ostream_iterator 輸出
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】sort + 【函數物件】greater
    std::sort(numbers.begin(), numbers.end(), std::greater<int>());
    
    std::cout << "降序排序: ";
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】copy_if + 【Lambda（函數物件）】
    std::vector<int> even_numbers;
    std::copy_if(numbers.begin(), numbers.end(),
                 std::back_inserter(even_numbers),  // 【迭代器配接器】
                 [](int n) { return n % 2 == 0; }); // 【Lambda】
    
    std::cout << "偶數: ";
    std::copy(even_numbers.begin(), even_numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】transform + 【Lambda】
    std::vector<int> doubled;
    doubled.resize(even_numbers.size());
    std::transform(even_numbers.begin(), even_numbers.end(),
                   doubled.begin(),
                   [](int n) { return n * 2; });
    
    std::cout << "偶數加倍: ";
    std::copy(doubled.begin(), doubled.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
原始資料: 64 25 12 22 11 90 42 
降序排序: 90 64 42 25 22 12 11 
偶數: 90 64 42 22 12 
偶數加倍: 180 128 84 44 24 
```

### 組件關係圖

```
┌─────────────────────────────────────────────────────────────────┐
│                    上例中的組件協作                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   vector<int>                                                   │
│   (容器)                                                        │
│       │                                                         │
│       │ 提供                                                    │
│       ▼                                                         │
│   begin() / end()                                               │
│   (迭代器)                                                      │
│       │                                                         │
│       │ 傳給                                                    │
│       ▼                                                         │
│   sort / copy_if / transform      ◄──── greater<int> / Lambda  │
│   (演算法)                               (函數物件)              │
│       │                                                         │
│       │ 輸出到                                                  │
│       ▼                                                         │
│   back_inserter / ostream_iterator                              │
│   (迭代器配接器)                                                │
│                                                                 │
│   ※ 整個過程中，allocator 在背後默默配置記憶體                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3.9 本課重點整理

```
┌─────────────────────────────────────────────────────────────────┐
│                      第三課 重點整理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  STL 六大組件：                                                  │
│                                                                 │
│  1. 容器（Containers）                                          │
│     → 儲存資料：vector, list, set, map...                       │
│                                                                 │
│  2. 迭代器（Iterators）                                         │
│     → 遍歷容器的泛化指標                                        │
│     → 五種類別：Input, Output, Forward, Bidirectional, Random   │
│                                                                 │
│  3. 演算法（Algorithms）                                        │
│     → 操作資料：sort, find, copy, transform...                  │
│     → 透過迭代器與容器解耦                                      │
│                                                                 │
│  4. 函數物件（Functors）                                        │
│     → 可呼叫的物件：重載 operator()                             │
│     → 現代 C++ 常用 Lambda 取代                                 │
│                                                                 │
│  5. 配接器（Adapters）                                          │
│     → 容器配接器：stack, queue, priority_queue                  │
│     → 迭代器配接器：reverse_iterator, back_inserter             │
│                                                                 │
│  6. 配置器（Allocators）                                        │
│     → 管理記憶體配置與釋放                                      │
│     → 通常使用預設，進階才需自訂                                │
│                                                                 │
│  核心設計：                                                      │
│  → 容器與演算法透過迭代器連接，達到正交性                       │
│  → 函數物件讓演算法行為可客製化                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3.10 課後思考

1. **思考題**：為什麼 `std::sort` 不能直接用在 `std::list` 上，而 `list` 要有自己的 `sort` 成員函數？

2. **觀察題**：在你目前或過去的專案中，有哪些地方可以用 STL 的容器和演算法來簡化程式碼？

---

準備好進入**第四課：迭代器（Iterator）的核心概念**了嗎？下一課我們會深入探討迭代器這個「連接容器與演算法的橋樑」，理解它的設計精髓。
*/



#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 349. Intersection of Two Arrays
//   題目：回傳兩個陣列的交集（結果中每個元素只出現一次）。
//   為什麼用到本主題：這一題能在一支函式裡同時用到**五個**組件，
//         是六大組件協作的最佳縮影：
//           容器      vector
//           迭代器    begin() / end()
//           演算法    sort / set_intersection / unique
//           函數物件  std::less（set_intersection 的預設比較器）
//           配接器    back_inserter（迭代器配接器，讓演算法能自動長大目的容器）
//         （配置器在背後默默替 vector 配置記憶體。）
//   複雜度：時間 O(N log N + M log M)、空間 O(min(N, M))。
// -----------------------------------------------------------------------------
std::vector<int> intersection(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());              // 演算法：set_intersection 要求已排序
    std::sort(b.begin(), b.end());

    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(),
                          b.begin(), b.end(),
                          std::back_inserter(out));   // 配接器：自動 push_back
    // set_intersection 對重複元素保留 min(count_a, count_b) 份，本題要求唯一 → 去重
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器存取紀錄分析：找出高流量來源 IP
//   情境：維運要從一批存取紀錄中找出「請求數超過門檻」的來源 IP，
//         依請求數由多到少列出，並輸出成一行報表。
//   為什麼用到本主題：這是六大組件在真實工作中的典型組合 ——
//         用 vector 收資料、用演算法過濾與排序、用 lambda 表達條件、
//         用 back_inserter 收集結果、用 ostream_iterator 輸出。
//         整段沒有一個手寫的索引迴圈。
// -----------------------------------------------------------------------------
struct AccessCount {
    std::string ip;
    int         requests;
};

std::vector<AccessCount> findHeavyHitters(const std::vector<AccessCount>& logs,
                                          int threshold) {
    std::vector<AccessCount> heavy;

    // 演算法 copy_if + 函數物件（lambda 捕獲門檻）+ 配接器 back_inserter
    std::copy_if(logs.begin(), logs.end(), std::back_inserter(heavy),
                 [threshold](const AccessCount& a) { return a.requests > threshold; });

    // 演算法 sort + 函數物件（lambda 比較器，用 > 維持嚴格弱序）
    std::sort(heavy.begin(), heavy.end(),
              [](const AccessCount& x, const AccessCount& y) {
                  return x.requests > y.requests;
              });
    return heavy;
}

int main() {
    // 【容器】儲存資料
    std::vector<int> numbers = {64, 25, 12, 22, 11, 90, 42};
    
    std::cout << "原始資料: ";
    // 【迭代器配接器】ostream_iterator 輸出
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】sort + 【函數物件】greater
    std::sort(numbers.begin(), numbers.end(), std::greater<int>());
    
    std::cout << "降序排序: ";
    std::copy(numbers.begin(), numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】copy_if + 【Lambda（函數物件）】
    std::vector<int> even_numbers;
    std::copy_if(numbers.begin(), numbers.end(),
                 std::back_inserter(even_numbers),  // 【迭代器配接器】
                 [](int n) { return n % 2 == 0; }); // 【Lambda】
    
    std::cout << "偶數: ";
    std::copy(even_numbers.begin(), even_numbers.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
    
    // 【演算法】transform + 【Lambda】
    std::vector<int> doubled;
    doubled.resize(even_numbers.size());
    std::transform(even_numbers.begin(), even_numbers.end(),
                   doubled.begin(),
                   [](int n) { return n * 2; });
    
    std::cout << "偶數加倍: ";
    std::copy(doubled.begin(), doubled.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    // 【演算法】accumulate（<numeric>）—— 同樣只透過迭代器工作
    std::cout << "偶數總和: "
              << std::accumulate(even_numbers.begin(), even_numbers.end(), 0)
              << std::endl;

    // 六大組件在上面這段程式中的分工
    std::cout << "\n=== 這段程式用到的六大組件 ===" << std::endl;
    std::cout << "  1. 容器      vector<int> numbers / even_numbers / doubled" << std::endl;
    std::cout << "  2. 迭代器    numbers.begin() / end()" << std::endl;
    std::cout << "  3. 演算法    sort / copy_if / transform / copy / accumulate" << std::endl;
    std::cout << "  4. 函數物件  greater<int>() 與兩個 lambda" << std::endl;
    std::cout << "  5. 配接器    back_inserter / ostream_iterator（迭代器配接器）" << std::endl;
    std::cout << "  6. 配置器    allocator<int>（在背後替 vector 配置記憶體）" << std::endl;

    // 「演算法改不了容器大小」的實際演示
    std::cout << "\n=== 為什麼 std::remove 之後 size() 不變 ===" << std::endl;
    std::vector<int> r = {1, 3, 2, 3, 4, 3, 5};
    std::cout << "  原始 size = " << r.size() << std::endl;
    auto new_end = std::remove(r.begin(), r.end(), 3);
    std::cout << "  remove(3) 後 size = " << r.size()
              << "  ← 沒變！演算法碰不到容器本身" << std::endl;
    std::cout << "  但邏輯上只剩 " << (new_end - r.begin()) << " 個有效元素" << std::endl;
    r.erase(new_end, r.end());          // 這一步才真的縮短容器
    std::cout << "  erase 之後 size = " << r.size() << "，內容 = ";
    for (int n : r) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===" << std::endl;
    std::cout << "  [1,2,2,1] ∩ [2,2]     = ";
    for (int n : intersection({1, 2, 2, 1}, {2, 2})) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  [4,9,5] ∩ [9,4,9,8,4] = ";
    for (int n : intersection({4, 9, 5}, {9, 4, 9, 8, 4})) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：找出高流量來源 IP ===" << std::endl;
    std::vector<AccessCount> logs = {
        {"10.0.0.7",     142}, {"192.168.1.20",  38}, {"10.0.0.99",  2571},
        {"172.16.5.3",   890}, {"10.0.0.7",       0}, {"203.0.113.9",  17},
        {"198.51.100.4", 455},
    };
    const int threshold = 100;
    std::vector<AccessCount> heavy = findHeavyHitters(logs, threshold);

    std::cout << "  門檻 " << threshold << " 次，命中 " << heavy.size() << " 個來源：" << std::endl;
    for (const AccessCount& a : heavy) {
        std::cout << "    " << a.ip << "  " << a.requests << " 次" << std::endl;
    }

    // 用 transform + ostream_iterator 產生一行報表
    std::vector<std::string> labels;
    std::transform(heavy.begin(), heavy.end(), std::back_inserter(labels),
                   [](const AccessCount& a) {
                       return a.ip + "(" + std::to_string(a.requests) + ")";
                   });
    std::cout << "  報表: ";
    std::copy(labels.begin(), labels.end(),
              std::ostream_iterator<std::string>(std::cout, " "));
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽13.cpp -o demo13

// === 預期輸出 ===
// 原始資料: 64 25 12 22 11 90 42
// 降序排序: 90 64 42 25 22 12 11
// 偶數: 90 64 42 22 12
// 偶數加倍: 180 128 84 44 24
// 偶數總和: 230
//
// === 這段程式用到的六大組件 ===
//   1. 容器      vector<int> numbers / even_numbers / doubled
//   2. 迭代器    numbers.begin() / end()
//   3. 演算法    sort / copy_if / transform / copy / accumulate
//   4. 函數物件  greater<int>() 與兩個 lambda
//   5. 配接器    back_inserter / ostream_iterator（迭代器配接器）
//   6. 配置器    allocator<int>（在背後替 vector 配置記憶體）
//
// === 為什麼 std::remove 之後 size() 不變 ===
//   原始 size = 7
//   remove(3) 後 size = 7  ← 沒變！演算法碰不到容器本身
//   但邏輯上只剩 4 個有效元素
//   erase 之後 size = 4，內容 = 1 2 4 5
//
// === LeetCode 349. Intersection of Two Arrays ===
//   [1,2,2,1] ∩ [2,2]     = 2
//   [4,9,5] ∩ [9,4,9,8,4] = 4 9
//
// === 日常實務：找出高流量來源 IP ===
//   門檻 100 次，命中 4 個來源：
//     10.0.0.99  2571 次
//     172.16.5.3  890 次
//     198.51.100.4  455 次
//     10.0.0.7  142 次
//   報表: 10.0.0.99(2571) 172.16.5.3(890) 198.51.100.4(455) 10.0.0.7(142)
