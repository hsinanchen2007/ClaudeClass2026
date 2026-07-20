// =============================================================================
//  第六課 16 — 課程講義全文：容器的概念、分類，與 STL 的三分結構
// =============================================================================
//
//  本檔前半是本課完整講義（Markdown 全文，包在區塊註解中），後半是可執行的
//  四個選容器案例。以下先補上講義本身較少展開、但決定你能不能真正用好容器的
//  底層原理與工程判準。
//
// 【主題資訊 Information】
//
//   本課涵蓋的 STL 容器（C++17 為準）：
//     序列容器      array(C++11) / vector / deque / list / forward_list(C++11)
//     有序關聯容器  set / multiset / map / multimap          （<set>, <map>）
//     無序關聯容器  unordered_set / unordered_map 及其 multi 版（C++11）
//     容器配接器    stack / queue / priority_queue           （<stack>, <queue>）
//
//   所有容器共同的介面（這組共同詞彙就是「容器」這個概念的定義）：
//     size_type size()  const;      bool empty() const;      void clear();
//     iterator  begin();            iterator end();          void swap(C&);
//     複雜度：size()/empty()/begin()/end() 皆為 O(1)（C++11 起 size() 一律 O(1)）
//
//   標頭檔：各容器一個，見上表；本檔示範用到
//     <vector> <set> <map> <unordered_map> <queue> <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 STL 要把「容器／迭代器／演算法」三者拆開】
// 這是理解整個 STL 最關鍵的一件事,也是本課通往第七課的橋樑。
//
// 假設沒有這層設計:M 種容器 × N 種演算法,你必須寫 M×N 份程式碼
// (vector 的 sort、list 的 sort、deque 的 sort、vector 的 find、list 的 find…)。
// STL 的解法是在中間插入一層**迭代器(iterator)**當作共通協議:
//
//     容器  ──提供──→  迭代器  ←──操作──  演算法
//     (M 種)          (共通協議)         (N 種)
//
// 於是變成 M+N 份程式碼。演算法不認識容器,只認識「一對迭代器」;容器不認識
// 演算法,只負責產生迭代器。這就是**關注點分離**在函式庫層次的經典實踐,
// 也是為什麼 std::find 可以同時用在 vector、list、set 上。
//
// 這個設計也直接解釋了兩個常見疑惑:
//   * 為什麼 std::sort 不能用在 list 上?
//     因為 std::sort 需要**隨機存取迭代器**(要能 it + n 跳躍),而 list 的
//     迭代器只能 ++/--(雙向迭代器)。所以 list 只好自備成員函式 sort()。
//   * 為什麼 stack/queue 不能用 range-based for?
//     因為它們**根本沒有迭代器** —— 沒有迭代器就接不上這套協議,這是配接器
//     刻意限制介面的直接後果。
//
// 【2. 迭代器的分類決定了「誰能用什麼演算法」】
// 迭代器不是只有一種,它依能力分層(每層包含上一層的能力):
//     Input/Output      → 只能單次前進讀/寫（如 istream_iterator）
//     Forward           → 可多次前進           （forward_list）
//     Bidirectional     → 可前進也可後退       （list, set, map）
//     Random Access     → 可 +n 跳躍、可相減   （vector, deque, array）
//     Contiguous(C++17) → 保證記憶體實體連續   （vector, array, string）
//
// 一個演算法要求哪一層,就決定了它能用在哪些容器上。這是「容器的能力」與
// 「演算法的需求」之間的契約 —— 而契約是由型別系統在編譯期檢查的。
//
// 【3. 複雜度表是上界,不是速度排行榜】
// 講義中的複雜度表(O(1)、O(log n)、O(n))描述的是**成長趨勢**,不是絕對速度。
// 實務上會反轉直覺的因素有三個:
//   (a) cache locality:vector 元素連續,一次 cache line 就帶進多個元素,
//       硬體 prefetcher 也能預測;list/map 的節點散落 heap,每次跳躍都可能是
//       一次 cache miss(代價可達數十至上百 CPU cycle)。
//   (b) 常數因子:O(1) 的 unordered_map 查找要先算 hash(對字串要走訪整個
//       字串)、找 bucket、再走鏈;O(log n) 的 map 查找是幾次指標跳躍。
//       n 小的時候,線性掃描一個小 vector 常常打敗兩者。
//   (c) 前提條件:list 中間插入 O(1) 的前提是「你已經持有那個位置的迭代器」。
//       若還要先找到位置,尋找本身就是 O(n)。
// 結論:複雜度用來排除**明顯錯誤**的選擇(例如在迴圈裡對 vector 做 push_front);
// 要在相近的候選之間分勝負,請實際量測,不要憑表格猜。
//
// 【4. 選容器的真正流程:問四個問題,而不是背表】
//   Q1. 要用「鍵」查找嗎?
//        否 → 序列容器,跳 Q2
//        是 → 需要有序走訪或範圍查詢(lower_bound)嗎?
//               要 → map / set        不要 → unordered_map / unordered_set
//   Q2. 大小編譯期就固定嗎?  是 → array   否 → 跳 Q3
//   Q3. 主要在哪裡增刪?
//        只在尾端 → vector;頭尾都要 → deque;
//        任意位置且已有迭代器、或需要迭代器永不失效 → list
//   Q4. 存取模式該被限制嗎?
//        LIFO → stack;FIFO → queue;每次取極值 → priority_queue
//   預設答案:**不確定就用 vector**。
//
// 【5. 容器對元素型別的要求(container requirements)】
// 容器不是什麼型別都能裝。C++11 之後規則變得寬鬆而精確:**你只需要滿足你
// 實際用到的操作**。
//   * vector<T> 若會 push_back 並可能重新配置 → T 需可複製或可搬移。
//   * 只用 emplace_back 且從不複製 → T 甚至可以是 move-only(如 unique_ptr)。
//   * set<T> / map<K,V> 的鍵需要**嚴格弱序**(strict weak ordering),
//     預設用 operator<;寫成 <= 會破壞不變式,導致行為錯誤。
//   * unordered_set<T> / unordered_map<K,V> 的鍵需要 std::hash 特化
//     **以及** operator==。std::hash 對內建型別與 std::string 有特化,
//     但**對 std::pair 沒有** —— 想用 pair 當鍵必須自己提供雜湊函式物件。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 容器物件本身 vs 它管理的元素
//     容器物件通常很小(只存幾個指標),真正的資料在 heap。本機實測
//     (GCC 15.2.0 / x86-64,皆屬**實作定義**):
//         sizeof(std::vector<int>)       = 24  （三個指標:begin/end/capacity）
//         sizeof(std::list<int>)         = 24
//         sizeof(std::forward_list<int>) = 8   （只有一個 head 指標）
//         sizeof(std::deque<int>)        = 80
//         sizeof(std::map<int,int>)      = 48
//         sizeof(std::unordered_map<int,int>) = 56
//         sizeof(std::array<int,5>)      = 20  （元素內嵌,無 heap 配置）
//     forward_list 只有 8 bytes,是因為它連元素個數都不存 —— 這正是它
//     沒有 size() 的原因,也是「零開銷」承諾的代價。
//
// (B) 講義中的「不保證連續」到底是什麼意思
//     只有 array、vector、string 保證元素實體連續(C++17 起有 ContiguousIterator
//     這個正式概念)。deque 是**分段連續**:一個指標陣列指向多個固定大小的
//     chunk。本機實測 deque<int> 每 128 個元素換一個 chunk(chunk 為 512 bytes,
//     **實作定義**)。所以 &dq[0] + 1 不保證等於 &dq[1] —— 想把資料交給
//     C API(需要一整塊連續緩衝區)時,只有 vector/array 能用 .data()。
//
// (C) 為什麼 map 的 value_type 是 pair<const Key, T>
//     鍵上的 const 不是多餘的謹慎:鍵決定了元素在紅黑樹中的位置,若允許就地
//     修改鍵,樹的排序不變式立刻被破壞,之後所有查找都會出錯。所以標準直接
//     用型別把它禁掉。C++17 之後若真要換鍵又不想重新配置節點,正確做法是
//     extract() 取出 node handle → 改鍵 → 重新 insert。
//
// (D) 這一課如何接到第七課
//     本課回答「資料放哪裡」,第七課回答「怎麼操作資料」。兩者的接點就是迭代器:
//     容器提供 begin()/end(),演算法只吃這一對。理解了這層,你就會明白為什麼
//     STL 演算法全都寫成 algo(first, last, ...) 而不是 algo(container, ...)
//     ——直到 C++20 才用 Ranges 補上更好用的容器層介面。
//
// 【注意事項 Pay Attention】
//  1. 所有標為「實作定義」的數值(sizeof、deque chunk 大小、vector 成長倍率、
//     bucket 數列)都是本機 libstdc++ 實測值,換編譯器/標準庫就可能不同,
//     不可寫進程式邏輯。
//  2. 對空容器呼叫 front()/back()/top()/pop() 是 undefined behavior。
//     行為**不保證、不可預測** —— 可能崩潰,也可能安靜地讀到垃圾值繼續跑。
//     「測起來沒事」不能當成正確性的證據。
//  3. unordered_* 的走訪順序未由標準規定;講義中案例 3 的字數統計輸出順序
//     即屬此類,任何依賴它的程式都是錯的。需要穩定輸出請複製到 vector 排序,
//     或改用 map。
//  4. map/unordered_map 的 operator[] 在鍵不存在時會**預設建構並插入**。
//     它是寫入介面而非查詢介面,也因此不能用在 const map 上。
//     純查詢請用 find()、at(),或 C++20 的 contains()。
//  5. 迭代器失效規則因容器而異,且必須連同「是否重新配置」一起判斷。
//     安全慣例:對 vector 做過插入後,一律視為先前的迭代器已失效。
//  6. std::sort 不能用在 list/set 上(迭代器能力不足);list 用成員 sort(),
//     set 本身就已有序、不需要也不應該排序。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器分類與 STL 三分結構
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 為什麼要把容器、迭代器、演算法分開?這個設計解決了什麼問題?
//     答：若不分開,M 種容器 × N 種演算法要寫 M×N 份實作。迭代器作為兩者之間
//         的共通協議後,容器只負責產生迭代器、演算法只吃一對迭代器,程式碼量
//         降為 M+N,而且新增容器或新增演算法都不必改動對方。這是關注點分離
//         在函式庫設計上的經典案例。
//     追問：那為什麼 std::sort 不能用在 std::list 上?
//         → std::sort 需要隨機存取迭代器(要能 it+n 跳躍),而 list 只提供
//           雙向迭代器。所以 list 自備成員函式 sort()(內部走 merge sort)。
//
// 🔥 Q2. 序列容器、關聯容器、無序容器的分類依據是什麼?
//     答：看「元素的位置由誰決定」。序列容器由你的插入順序決定;關聯容器與
//         無序容器由元素的值決定 —— 因為它們要靠位置來加速查找。兩者再細分:
//         靠比較(嚴格弱序)排位的是有序關聯容器,額外獲得順序與範圍查詢能力;
//         靠雜湊分桶的是無序容器,放棄順序換取平均 O(1)。
//     追問：容器配接器算第四類嗎?
//         → 它不是獨立的資料結構,而是包住既有容器並刻意限制介面的包裝層。
//           所以它沒有迭代器,也不能餵給 STL 演算法。
//
// 🔥 Q3. 為什麼 map 的元素型別是 pair<const Key, T> 而不是 pair<Key, T>?
//     答：鍵決定元素在樹中的位置。若允許就地修改鍵,排序不變式會立刻被破壞,
//         之後的查找全部失準。標準用型別層的 const 把這件事直接禁掉。
//         C++17 起若真要換鍵,正確做法是 extract() 取出 node handle、
//         修改後再 insert,如此可重用節點、不必重新配置記憶體。
//     追問：那用結構化綁定 auto& [k, v] 時,k 是什麼型別?
//         → const Key&。所以 k 不能被指派,v 可以。
//
// ⚠️ 陷阱. 「複雜度表上 list 中間插入是 O(1)、vector 是 O(n),所以頻繁在中間
//         插入時一定要選 list」——這個推論錯在哪?
//     答：錯在忽略前提與常數。list 的 O(1) 前提是「已持有該位置的迭代器」;
//         若每次都要先找到位置,尋找本身就是 O(n),總成本與 vector 同級。
//         而且 list 走訪的每一步都可能 cache miss,vector 的搬移卻是 memmove
//         這種 CPU 最擅長的連續複製。元素小、資料量中等時,vector 經常勝出。
//     為什麼會錯：把大 O 當成效能排行榜。大 O 描述成長趨勢,刻意隱藏了常數
//         因子與記憶體階層效應,而在現代 CPU 上這兩者往往才是主導因素。
//
// ⚠️ 陷阱. unordered_map 走訪的順序,在同一台機器、同一個程式裡跑兩次會一樣,
//         是不是就代表可以依賴它?
//     答：不可以。標準明文規定走訪順序未指定。它在同一份實作、同一組插入序列
//         下碰巧穩定,只是實作細節的副作用 —— 換編譯器、換標準庫版本、
//         元素數量改變觸發 rehash、甚至只是插入順序不同,順序就會改變。
//         需要穩定輸出請複製到 vector 排序,或直接改用 map。
//     為什麼會錯：用「我測過,結果一樣」來推論「行為有保證」。可重現的觀察
//         不等於標準的承諾;未指定行為隨時可以在你沒改程式碼的情況下改變。
// ═══════════════════════════════════════════════════════════════════════════

/*
# 第六課：容器（Container）的概念與分類

---

## 6.1 什麼是容器？

**容器**（Container）是用來**儲存和管理一組物件**的類別模板。

你可以把容器想像成「資料的收納盒」：

```
┌─────────────────────────────────────────────────────────────────┐
│                      容器的基本概念                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   容器負責：                                                    │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ • 儲存元素                                              │  │
│   │ • 管理記憶體（自動配置和釋放）                          │  │
│   │ • 提供存取元素的方法                                    │  │
│   │ • 提供迭代器讓演算法可以操作                            │  │
│   │ • 追蹤元素數量                                          │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
│   你只需要：                                                    │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │ • 決定要存什麼型別                                      │  │
│   │ • 選擇適合的容器                                        │  │
│   │ • 使用容器提供的介面操作                                │  │
│   └─────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.2 容器的分類總覽

STL 容器分為四大類：

```
┌─────────────────────────────────────────────────────────────────┐
│                       STL 容器分類                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  序列容器（Sequence Containers）                        │   │
│  │  • 元素按插入順序排列                                   │   │
│  │  • array, vector, deque, list, forward_list             │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  關聯容器（Associative Containers）                     │   │
│  │  • 元素按鍵值自動排序                                   │   │
│  │  • set, multiset, map, multimap                         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  無序容器（Unordered Containers）                       │   │
│  │  • 使用雜湊表，無特定順序                               │   │
│  │  • unordered_set, unordered_multiset,                   │   │
│  │    unordered_map, unordered_multimap                    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  容器配接器（Container Adapters）                       │   │
│  │  • 包裝其他容器，提供特定介面                           │   │
│  │  • stack, queue, priority_queue                         │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.3 序列容器（Sequence Containers）

序列容器的特點：**元素按照插入的順序排列**。

### 6.3.1 array — 固定大小陣列

```cpp
#include <iostream>
#include <array>

int main() {
    // array：大小在編譯期固定
    std::array<int, 5> arr = {10, 20, 30, 40, 50};
    
    std::cout << "=== std::array ===" << std::endl;
    std::cout << "大小: " << arr.size() << std::endl;
    std::cout << "第一個: " << arr.front() << std::endl;
    std::cout << "最後一個: " << arr.back() << std::endl;
    std::cout << "arr[2]: " << arr[2] << std::endl;
    std::cout << "arr.at(2): " << arr.at(2) << std::endl;  // 有邊界檢查
    
    // 遍歷
    std::cout << "所有元素: ";
    for (int n : arr) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::array ===
大小: 5
第一個: 10
最後一個: 50
arr[2]: 30
arr.at(2): 30
所有元素: 10 20 30 40 50 
```

**特性：**
- 大小固定，編譯期決定
- 連續記憶體
- 零額外開銷（跟 C 陣列一樣快）
- Random Access Iterator

### 6.3.2 vector — 動態陣列

```cpp
#include <iostream>
#include <vector>

int main() {
    std::cout << "=== std::vector ===" << std::endl;
    
    std::vector<int> vec;
    
    // 動態新增
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    std::cout << "大小: " << vec.size() << std::endl;
    std::cout << "容量: " << vec.capacity() << std::endl;
    
    // 繼續新增
    vec.push_back(40);
    vec.push_back(50);
    
    std::cout << "新增後大小: " << vec.size() << std::endl;
    std::cout << "新增後容量: " << vec.capacity() << std::endl;
    
    // 存取
    std::cout << "vec[2]: " << vec[2] << std::endl;
    std::cout << "front: " << vec.front() << std::endl;
    std::cout << "back: " << vec.back() << std::endl;
    
    // 刪除最後一個
    vec.pop_back();
    std::cout << "pop_back 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::vector ===
大小: 3
容量: 4
新增後大小: 5
新增後容量: 8
vec[2]: 30
front: 10
back: 50
pop_back 後: 10 20 30 40 
```

**特性：**
- 大小可動態變化
- 連續記憶體
- 尾端操作 O(1)
- 中間插入/刪除 O(n)
- Random Access Iterator

### 6.3.3 deque — 雙端佇列

```cpp
#include <iostream>
#include <deque>

int main() {
    std::cout << "=== std::deque ===" << std::endl;
    
    std::deque<int> deq;
    
    // 可以在兩端操作
    deq.push_back(30);
    deq.push_front(20);
    deq.push_back(40);
    deq.push_front(10);
    
    std::cout << "元素: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;
    
    // 隨機存取
    std::cout << "deq[2]: " << deq[2] << std::endl;
    
    // 兩端刪除
    deq.pop_front();
    deq.pop_back();
    
    std::cout << "刪除頭尾後: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::deque ===
元素: 10 20 30 40 
deq[2]: 30
刪除頭尾後: 20 30 
```

**特性：**
- 兩端操作都是 O(1)
- 支援隨機存取
- 分段連續記憶體（不是完全連續）
- Random Access Iterator

### 6.3.4 list — 雙向鏈結串列

```cpp
#include <iostream>
#include <list>

int main() {
    std::cout << "=== std::list ===" << std::endl;
    
    std::list<int> lst = {20, 40};
    
    // 任意位置插入都是 O(1)（前提是已有迭代器）
    lst.push_front(10);
    lst.push_back(50);
    
    // 在中間插入
    auto it = lst.begin();
    ++it;  // 指向 20
    ++it;  // 指向 40
    lst.insert(it, 30);  // 在 40 前面插入 30
    
    std::cout << "元素: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    // list 特有的操作
    std::list<int> lst2 = {100, 200, 300};
    
    // splice：將 lst2 的元素移動到 lst
    it = lst.begin();
    std::advance(it, 2);  // 指向 30
    lst.splice(it, lst2);  // 在 30 前面插入 lst2 的所有元素
    
    std::cout << "splice 後 lst: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "splice 後 lst2 大小: " << lst2.size() << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::list ===
元素: 10 20 30 40 50 
splice 後 lst: 10 20 100 200 300 30 40 50 
splice 後 lst2 大小: 0
```

**特性：**
- 任意位置插入/刪除 O(1)
- 不支援隨機存取
- 每個元素有額外的指標開銷
- Bidirectional Iterator

### 6.3.5 forward_list — 單向鏈結串列

```cpp
#include <iostream>
#include <forward_list>

int main() {
    std::cout << "=== std::forward_list ===" << std::endl;
    
    std::forward_list<int> flst = {20, 30, 40};
    
    // 只能在前端插入
    flst.push_front(10);
    
    std::cout << "元素: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 在某個位置「之後」插入
    auto it = flst.begin();  // 指向 10
    flst.insert_after(it, 15);  // 在 10 之後插入 15
    
    std::cout << "insert_after 後: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 注意：forward_list 沒有 size() 成員函數！
    // 需要用 std::distance 計算
    auto count = std::distance(flst.begin(), flst.end());
    std::cout << "元素個數: " << count << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::forward_list ===
元素: 10 20 30 40 
insert_after 後: 10 15 20 30 40 
元素個數: 5
```

**特性：**
- 最小記憶體開銷的鏈結串列
- 只能往前遍歷
- 沒有 size()、push_back()、back()
- Forward Iterator

### 序列容器比較

```
┌────────────────────────────────────────────────────────────────────────────┐
│                          序列容器比較表                                     │
├──────────────┬─────────┬─────────┬─────────┬─────────┬─────────────────────┤
│    操作      │  array  │ vector  │  deque  │  list   │    forward_list     │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 隨機存取     │  O(1)   │  O(1)   │  O(1)   │  O(n)   │       O(n)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 頭部插入     │   N/A   │  O(n)   │  O(1)   │  O(1)   │       O(1)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 尾部插入     │   N/A   │ O(1)*   │  O(1)   │  O(1)   │       O(n)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 中間插入     │   N/A   │  O(n)   │  O(n)   │  O(1)   │       O(1)          │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 記憶體連續   │    ✓    │    ✓    │   部分  │   ✗     │        ✗            │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 迭代器類別   │ Random  │ Random  │ Random  │  Bidir  │     Forward         │
├──────────────┼─────────┼─────────┼─────────┼─────────┼─────────────────────┤
│ 額外開銷     │   無    │   低    │   中    │   高    │        中           │
└──────────────┴─────────┴─────────┴─────────┴─────────┴─────────────────────┘
  * vector 尾部插入 O(1) 是均攤時間複雜度，可能觸發重新配置
```

---

## 6.4 關聯容器（Associative Containers）

關聯容器的特點：**元素按鍵值自動排序**，內部通常用紅黑樹實作。

### 6.4.1 set — 有序集合

```cpp
#include <iostream>
#include <set>

int main() {
    std::cout << "=== std::set ===" << std::endl;
    
    // 元素自動排序，不允許重複
    std::set<int> s;
    
    s.insert(30);
    s.insert(10);
    s.insert(50);
    s.insert(20);
    s.insert(40);
    s.insert(30);  // 重複，會被忽略
    
    std::cout << "元素（自動排序）: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "大小: " << s.size() << std::endl;
    
    // 查找
    auto it = s.find(30);
    if (it != s.end()) {
        std::cout << "找到 30" << std::endl;
    }
    
    // count：因為不重複，只會回傳 0 或 1
    std::cout << "30 的數量: " << s.count(30) << std::endl;
    std::cout << "100 的數量: " << s.count(100) << std::endl;
    
    // 刪除
    s.erase(30);
    std::cout << "刪除 30 後: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::set ===
元素（自動排序）: 10 20 30 40 50 
大小: 5
找到 30
30 的數量: 1
100 的數量: 0
刪除 30 後: 10 20 40 50 
```

### 6.4.2 multiset — 允許重複的有序集合

```cpp
#include <iostream>
#include <set>

int main() {
    std::cout << "=== std::multiset ===" << std::endl;
    
    // 允許重複元素
    std::multiset<int> ms;
    
    ms.insert(30);
    ms.insert(10);
    ms.insert(30);  // 重複，會被保留
    ms.insert(20);
    ms.insert(30);  // 第三個 30
    
    std::cout << "元素: ";
    for (int n : ms) std::cout << n << " ";
    std::cout << std::endl;
    
    std::cout << "30 的數量: " << ms.count(30) << std::endl;
    
    // equal_range：取得所有等於某值的元素範圍
    auto range = ms.equal_range(30);
    std::cout << "所有 30：";
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::multiset ===
元素: 10 20 30 30 30 
30 的數量: 3
所有 30：30 30 30 
```

### 6.4.3 map — 有序鍵值對

```cpp
#include <iostream>
#include <map>
#include <string>

int main() {
    std::cout << "=== std::map ===" << std::endl;
    
    std::map<std::string, int> ages;
    
    // 插入方式一：operator[]
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    
    // 插入方式二：insert
    ages.insert({"Charlie", 35});
    ages.insert(std::make_pair("Diana", 28));
    
    // 按鍵排序
    std::cout << "所有人（按名字排序）:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 查找
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }
    
    // operator[] 的陷阱：會插入預設值！
    std::cout << "Eve 的年齡: " << ages["Eve"] << std::endl;  // 插入 Eve = 0
    std::cout << "現在的大小: " << ages.size() << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::map ===
所有人（按名字排序）:
  Alice: 25
  Bob: 30
  Charlie: 35
  Diana: 28
Bob 的年齡: 30
Eve 的年齡: 0
現在的大小: 5
```

### 6.4.4 multimap — 允許重複鍵的有序鍵值對

```cpp
#include <iostream>
#include <map>
#include <string>

int main() {
    std::cout << "=== std::multimap ===" << std::endl;
    
    // 一個鍵可以對應多個值
    std::multimap<std::string, std::string> phonebook;
    
    phonebook.insert({"Alice", "0912-345-678"});
    phonebook.insert({"Alice", "02-1234-5678"});  // Alice 有兩個號碼
    phonebook.insert({"Bob", "0923-456-789"});
    phonebook.insert({"Alice", "03-9876-5432"});  // Alice 第三個號碼
    
    std::cout << "電話簿:" << std::endl;
    for (const auto& entry : phonebook) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }
    
    // 查找 Alice 的所有號碼
    std::cout << "\nAlice 的所有號碼:" << std::endl;
    auto range = phonebook.equal_range("Alice");
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << "  " << it->second << std::endl;
    }
    
    std::cout << "\nAlice 有 " << phonebook.count("Alice") << " 個號碼" << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::multimap ===
電話簿:
  Alice: 0912-345-678
  Alice: 02-1234-5678
  Alice: 03-9876-5432
  Bob: 0923-456-789

Alice 的所有號碼:
  0912-345-678
  02-1234-5678
  03-9876-5432

Alice 有 3 個號碼
```

### 關聯容器比較

```
┌────────────────────────────────────────────────────────────────────────────┐
│                          關聯容器比較表                                     │
├──────────────┬────────────────┬────────────────┬───────────────────────────┤
│    容器      │    允許重複    │   儲存內容     │        典型用途           │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│    set       │      否        │     鍵         │ 去重、有序集合            │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│  multiset    │      是        │     鍵         │ 排序並保留重複            │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│    map       │    鍵不重複    │   鍵值對       │ 字典、索引                │
├──────────────┼────────────────┼────────────────┼───────────────────────────┤
│  multimap    │    鍵可重複    │   鍵值對       │ 一對多關係                │
├──────────────┴────────────────┴────────────────┴───────────────────────────┤
│                                                                            │
│  共同特性：                                                                 │
│  • 內部使用紅黑樹                                                          │
│  • 元素按鍵自動排序                                                        │
│  • 查找、插入、刪除都是 O(log n)                                           │
│  • Bidirectional Iterator                                                  │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## 6.5 無序容器（Unordered Containers）

無序容器的特點：**使用雜湊表**，無特定順序，但查找速度更快。

### 6.5.1 unordered_set

```cpp
#include <iostream>
#include <unordered_set>

int main() {
    std::cout << "=== std::unordered_set ===" << std::endl;
    
    std::unordered_set<int> us;
    
    us.insert(30);
    us.insert(10);
    us.insert(50);
    us.insert(20);
    us.insert(40);
    
    // 順序不固定（取決於雜湊）
    std::cout << "元素（順序不固定）: ";
    for (int n : us) std::cout << n << " ";
    std::cout << std::endl;
    
    // 查找：平均 O(1)
    if (us.find(30) != us.end()) {
        std::cout << "找到 30" << std::endl;
    }
    
    // 雜湊表資訊
    std::cout << "桶的數量: " << us.bucket_count() << std::endl;
    std::cout << "負載因子: " << us.load_factor() << std::endl;
    
    return 0;
}
```

**可能的輸出：**
```
=== std::unordered_set ===
元素（順序不固定）: 40 20 50 10 30 
找到 30
桶的數量: 7
負載因子: 0.714286
```

### 6.5.2 unordered_map

```cpp
#include <iostream>
#include <unordered_map>
#include <string>

int main() {
    std::cout << "=== std::unordered_map ===" << std::endl;
    
    std::unordered_map<std::string, int> ages;
    
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 35;
    ages["Diana"] = 28;
    
    // 順序不固定
    std::cout << "所有人:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 查找：平均 O(1)
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }
    
    return 0;
}
```

**可能的輸出：**
```
=== std::unordered_map ===
所有人:
  Diana: 28
  Charlie: 35
  Bob: 30
  Alice: 25
Bob 的年齡: 30
```

### 有序 vs 無序容器

```
┌────────────────────────────────────────────────────────────────────────────┐
│                      有序 vs 無序容器比較                                   │
├──────────────────────┬─────────────────────┬───────────────────────────────┤
│        面向          │     有序（set/map）  │    無序（unordered_*）        │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 內部結構             │      紅黑樹         │         雜湊表                │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 查找時間             │     O(log n)        │      O(1) 平均               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 插入時間             │     O(log n)        │      O(1) 平均               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 刪除時間             │     O(log n)        │      O(1) 平均               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 元素順序             │      有序           │         無序                 │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 範圍查詢             │       支援          │        不支援                │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 迭代器類別           │   Bidirectional     │        Forward               │
├──────────────────────┼─────────────────────┼───────────────────────────────┤
│ 最差情況             │     O(log n)        │      O(n)*                   │
└──────────────────────┴─────────────────────┴───────────────────────────────┘
  * 雜湊碰撞嚴重時可能退化
```

---

## 6.6 容器配接器（Container Adapters）

容器配接器**不是獨立的容器**，而是包裝其他容器來提供特定介面。

### 6.6.1 stack — 後進先出（LIFO）

```cpp
#include <iostream>
#include <stack>

int main() {
    std::cout << "=== std::stack ===" << std::endl;
    
    std::stack<int> stk;
    
    // 只能從頂端操作
    stk.push(10);
    stk.push(20);
    stk.push(30);
    
    std::cout << "頂端: " << stk.top() << std::endl;
    std::cout << "大小: " << stk.size() << std::endl;
    
    // 依序取出（後進先出）
    std::cout << "依序取出: ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::stack ===
頂端: 30
大小: 3
依序取出: 30 20 10 
```

### 6.6.2 queue — 先進先出（FIFO）

```cpp
#include <iostream>
#include <queue>

int main() {
    std::cout << "=== std::queue ===" << std::endl;
    
    std::queue<int> que;
    
    // 從尾端加入
    que.push(10);
    que.push(20);
    que.push(30);
    
    std::cout << "前端: " << que.front() << std::endl;
    std::cout << "後端: " << que.back() << std::endl;
    
    // 依序取出（先進先出）
    std::cout << "依序取出: ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();  // 從前端移除
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::queue ===
前端: 10
後端: 30
依序取出: 10 20 30 
```

### 6.6.3 priority_queue — 優先佇列

```cpp
#include <iostream>
#include <queue>
#include <vector>

int main() {
    std::cout << "=== std::priority_queue ===" << std::endl;
    
    // 預設是最大堆（最大的在頂端）
    std::priority_queue<int> max_pq;
    
    max_pq.push(30);
    max_pq.push(10);
    max_pq.push(50);
    max_pq.push(20);
    max_pq.push(40);
    
    std::cout << "最大堆依序取出: ";
    while (!max_pq.empty()) {
        std::cout << max_pq.top() << " ";
        max_pq.pop();
    }
    std::cout << std::endl;
    
    // 最小堆
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
    
    min_pq.push(30);
    min_pq.push(10);
    min_pq.push(50);
    min_pq.push(20);
    min_pq.push(40);
    
    std::cout << "最小堆依序取出: ";
    while (!min_pq.empty()) {
        std::cout << min_pq.top() << " ";
        min_pq.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
```

**輸出：**
```
=== std::priority_queue ===
最大堆依序取出: 50 40 30 20 10 
最小堆依序取出: 10 20 30 40 50 
```

### 配接器與底層容器

```
┌─────────────────────────────────────────────────────────────────┐
│                   容器配接器的底層容器                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   配接器           預設底層容器      可用的底層容器             │
│   ─────────────────────────────────────────────────────────     │
│   stack            deque            vector, deque, list         │
│   queue            deque            deque, list                 │
│   priority_queue   vector           vector, deque               │
│                                                                 │
│   指定底層容器：                                                │
│   std::stack<int, std::vector<int>> stk;                        │
│   std::queue<int, std::list<int>> que;                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.7 容器的共同介面

所有容器都提供一些共同的成員函數：

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <set>

template <typename Container>
void show_container_info(const std::string& name, const Container& c) {
    std::cout << name << ":" << std::endl;
    std::cout << "  size(): " << c.size() << std::endl;
    std::cout << "  empty(): " << (c.empty() ? "true" : "false") << std::endl;
    // max_size(): 容器理論上能容納的最大元素數
    std::cout << "  max_size(): " << c.max_size() << std::endl;
    std::cout << std::endl;
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::list<int> lst = {1, 2, 3};
    std::set<int> s = {1, 2};
    std::vector<int> empty_vec;
    
    show_container_info("vector", vec);
    show_container_info("list", lst);
    show_container_info("set", s);
    show_container_info("empty_vector", empty_vec);
    
    // 其他共同操作
    std::cout << "=== 其他共同操作 ===" << std::endl;
    
    // swap
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {10, 20};
    v1.swap(v2);
    
    std::cout << "swap 後 v1: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << std::endl;
    
    // clear
    v1.clear();
    std::cout << "clear 後 v1.size(): " << v1.size() << std::endl;
    
    return 0;
}
```

**輸出：**
```
vector:
  size(): 5
  empty(): false
  max_size(): 4611686018427387903

list:
  size(): 3
  empty(): false
  max_size(): 768614336404564650

set:
  size(): 2
  empty(): false
  max_size(): 461168601842738790

empty_vector:
  size(): 0
  empty(): true
  max_size(): 4611686018427387903

=== 其他共同操作 ===
swap 後 v1: 10 20 
clear 後 v1.size(): 0
```

### 共同成員函數一覽

```
┌─────────────────────────────────────────────────────────────────┐
│                    容器共同成員函數                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  大小相關：                                                     │
│  • size()       → 元素數量                                     │
│  • empty()      → 是否為空                                     │
│  • max_size()   → 最大可容納數量                               │
│                                                                 │
│  迭代器：                                                       │
│  • begin() / end()           → 正向迭代器                      │
│  • cbegin() / cend()         → const 迭代器                    │
│  • rbegin() / rend()         → 反向迭代器（部分容器）          │
│                                                                 │
│  修改：                                                         │
│  • clear()      → 清空所有元素                                 │
│  • swap()       → 交換兩個容器的內容                           │
│                                                                 │
│  比較（同型別容器）：                                           │
│  • ==, !=       → 相等比較                                     │
│  • <, <=, >, >= → 字典序比較                                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.8 如何選擇容器？

```
┌─────────────────────────────────────────────────────────────────┐
│                      容器選擇決策流程                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 需要鍵值對嗎？                                              │
│     ├─ 是 → 需要排序嗎？                                       │
│     │       ├─ 是 → map / multimap                             │
│     │       └─ 否 → unordered_map / unordered_multimap         │
│     │                                                           │
│     └─ 否 → 繼續                                               │
│                                                                 │
│  2. 元素需要唯一嗎？                                            │
│     ├─ 是 → 需要排序嗎？                                       │
│     │       ├─ 是 → set                                        │
│     │       └─ 否 → unordered_set                              │
│     │                                                           │
│     └─ 否 → 繼續                                               │
│                                                                 │
│  3. 大小固定嗎？                                                │
│     ├─ 是 → array                                              │
│     └─ 否 → 繼續                                               │
│                                                                 │
│  4. 主要操作是什麼？                                            │
│     ├─ 隨機存取多 → vector                                     │
│     ├─ 頭尾操作多 → deque                                      │
│     ├─ 中間插刪多 → list                                       │
│     ├─ 只需前端操作 + 最小開銷 → forward_list                  │
│     └─ 後進先出 → stack                                        │
│     └─ 先進先出 → queue                                        │
│     └─ 優先順序 → priority_queue                               │
│                                                                 │
│  5. 不確定？先用 vector                                         │
│     → 它在大多數情況下效能最好                                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 實際案例

```cpp
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <string>

int main() {
    // 案例 1：儲存學生成績列表 → vector
    std::cout << "=== 案例 1：學生成績 ===" << std::endl;
    std::vector<int> scores = {85, 92, 78, 95, 88};
    std::cout << "第三個學生: " << scores[2] << std::endl;
    
    // 案例 2：去重的標籤 → set 或 unordered_set
    std::cout << "\n=== 案例 2：去重標籤 ===" << std::endl;
    std::set<std::string> tags;
    tags.insert("C++");
    tags.insert("Programming");
    tags.insert("C++");  // 重複，被忽略
    tags.insert("STL");
    for (const auto& tag : tags) std::cout << tag << " ";
    std::cout << std::endl;
    
    // 案例 3：字數統計 → map 或 unordered_map
    std::cout << "\n=== 案例 3：字數統計 ===" << std::endl;
    std::string text = "hello world hello c++ world world";
    std::unordered_map<std::string, int> word_count;
    
    // 簡化的分詞
    std::string word;
    for (char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                word_count[word]++;
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) word_count[word]++;
    
    for (const auto& pair : word_count) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    
    // 案例 4：任務排程 → priority_queue
    std::cout << "\n=== 案例 4：任務排程 ===" << std::endl;
    std::priority_queue<std::pair<int, std::string>> tasks;
    
    tasks.push({3, "一般任務"});
    tasks.push({5, "緊急任務"});
    tasks.push({1, "低優先任務"});
    tasks.push({4, "重要任務"});
    
    std::cout << "按優先順序處理:" << std::endl;
    while (!tasks.empty()) {
        auto task = tasks.top();
        std::cout << "  優先級 " << task.first << ": " << task.second << std::endl;
        tasks.pop();
    }
    
    return 0;
}
```

**輸出：**
```
=== 案例 1：學生成績 ===
第三個學生: 78

=== 案例 2：去重標籤 ===
C++ Programming STL 

=== 案例 3：字數統計 ===
c++: 1
world: 3
hello: 2

=== 案例 4：任務排程 ===
按優先順序處理:
  優先級 5: 緊急任務
  優先級 4: 重要任務
  優先級 3: 一般任務
  優先級 1: 低優先任務
```

---

## 6.9 本課重點整理

```
┌─────────────────────────────────────────────────────────────────┐
│                      第六課 重點整理                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. 容器分類                                                     │
│     • 序列容器：array, vector, deque, list, forward_list        │
│     • 關聯容器：set, multiset, map, multimap                    │
│     • 無序容器：unordered_set, unordered_map 等                 │
│     • 容器配接器：stack, queue, priority_queue                  │
│                                                                 │
│  2. 序列容器特點                                                 │
│     • 元素按插入順序排列                                        │
│     • vector 最常用，連續記憶體，尾端操作快                     │
│     • list 適合頻繁中間插刪                                     │
│                                                                 │
│  3. 關聯容器特點                                                 │
│     • 元素按鍵自動排序（紅黑樹）                                │
│     • 查找、插入、刪除都是 O(log n)                             │
│     • multi 版本允許重複鍵                                      │
│                                                                 │
│  4. 無序容器特點                                                 │
│     • 使用雜湊表，無特定順序                                    │
│     • 平均 O(1) 的查找、插入、刪除                              │
│     • 不支援範圍查詢                                            │
│                                                                 │
│  5. 容器配接器特點                                               │
│     • 包裝其他容器，提供特定介面                                │
│     • stack（LIFO）、queue（FIFO）、priority_queue              │
│                                                                 │
│  6. 選擇原則                                                     │
│     • 不確定時先用 vector                                       │
│     • 需要鍵值對用 map 系列                                     │
│     • 需要去重用 set 系列                                       │
│     • 重視查找速度用 unordered 系列                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 6.10 課後練習

1. **思考題**：為什麼 `vector` 通常比 `list` 效能更好，即使在需要中間插入的情況下？（提示：考慮快取效應）

2. **實作題**：設計一個簡單的「最近瀏覽記錄」系統，要求：
   - 保持最多 5 筆記錄
   - 新記錄加在最前面
   - 如果記錄已存在，移到最前面
   - 你會選擇哪種容器？為什麼？

---

準備好進入**第七課：演算法（Algorithm）與容器的分離設計**了嗎？下一課我們會深入探討 STL 演算法的設計哲學，以及它如何透過迭代器實現與容器的解耦。
*/



#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <queue>
#include <string>
#include <algorithm>
#include <utility>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 692. Top K Frequent Words
//   題目：給一個字串陣列，回傳出現次數前 k 高的單字；次數相同時，字典序小的排前面。
//   為什麼用到本主題：這題是下方「案例 3：字數統計」的自然延伸，而且把本課
//     兩個核心判準都逼了出來：
//       * 統計階段不需要順序 → unordered_map（平均 O(1) 累加）
//       * 取前 k 名不需要全排序 → priority_queue（min-heap 維持大小 k）
//       * 「次數相同要比字典序」讓比較器必須自訂 —— 這正好示範 priority_queue
//         第三個模板參數 Compare 的真正用途。
//   關鍵細節：min-heap 要淘汰「最差的」，所以比較器方向與直覺相反 ——
//     次數少的算「差」；次數相同時，字典序**大**的算「差」（才會被先丟掉）。
//   複雜度：時間 O(n log k)，空間 O(n)。
// -----------------------------------------------------------------------------
std::vector<std::string> topKFrequentWords(const std::vector<std::string>& words,
                                           int k) {
    std::unordered_map<std::string, int> freq;
    for (const auto& w : words) ++freq[w];

    using Item = std::pair<int, std::string>;      // (次數, 單字)
    // min-heap：堆頂是「目前最該被淘汰的那個」
    auto worse = [](const Item& a, const Item& b) {
        if (a.first != b.first) return a.first > b.first;   // 次數多的比較「好」
        return a.second < b.second;                          // 次數同：字典序小的比較「好」
    };
    std::priority_queue<Item, std::vector<Item>, decltype(worse)> heap(worse);

    for (const auto& [word, count] : freq) {
        heap.emplace(count, word);
        if (static_cast<int>(heap.size()) > k) heap.pop();    // 丟掉最差的
    }

    std::vector<std::string> result;
    while (!heap.empty()) {
        result.push_back(heap.top().second);
        heap.pop();
    }
    std::reverse(result.begin(), result.end());   // 堆由差到好彈出，反轉才是答案
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 INI 風格設定檔
//   情境：讀一份 [section] key=value 格式的設定檔（nginx、systemd、git config、
//     Python configparser 都屬這個家族），要能：
//       1. 依 section 分組
//       2. 同一 section 內的 key 按字母排序輸出（方便 diff 與人工核對）
//       3. 後出現的同名 key 覆蓋先前的值（絕大多數設定檔的實際語意）
//       4. 忽略空行與 # / ; 註解，並容忍 key/value 前後的空白
//   為什麼用到本主題：需求 1+2 直接指定了容器 ——「要用鍵查找」且「要有序輸出」
//     → std::map，而且是巢狀的 map<section, map<key, value>>。
//     需求 3 剛好就是 operator[] 的指派語意。若改用 unordered_map，
//     輸出順序就不再穩定，設定檔的 diff 會整片變動而失去可讀性。
// -----------------------------------------------------------------------------
using ConfigData = std::map<std::string, std::map<std::string, std::string>>;

static std::string trimSpaces(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

ConfigData parseIniConfig(const std::vector<std::string>& lines) {
    ConfigData config;
    std::string section = "global";        // 尚未宣告 section 前的預設分組

    for (const auto& raw : lines) {
        std::string line = trimSpaces(raw);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;  // 空行/註解

        if (line.front() == '[' && line.back() == ']') {
            section = trimSpaces(line.substr(1, line.size() - 2));
            config[section];               // 空 section 也要建立：operator[] 的插入語意
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;            // 不合法的行，跳過
        std::string key   = trimSpaces(line.substr(0, eq));
        std::string value = trimSpaces(line.substr(eq + 1));
        if (key.empty()) continue;

        config[section][key] = value;      // 後者覆蓋前者，正是 operator[] 的指派語意
    }
    return config;
}

int main() {
    // 案例 1：儲存學生成績列表 → vector
    std::cout << "=== 案例 1：學生成績 ===" << std::endl;
    std::vector<int> scores = {85, 92, 78, 95, 88};
    std::cout << "第三個學生: " << scores[2] << std::endl;
    
    // 案例 2：去重的標籤 → set 或 unordered_set
    std::cout << "\n=== 案例 2：去重標籤 ===" << std::endl;
    std::set<std::string> tags;
    tags.insert("C++");
    tags.insert("Programming");
    tags.insert("C++");  // 重複，被忽略
    tags.insert("STL");
    for (const auto& tag : tags) std::cout << tag << " ";
    std::cout << std::endl;
    
    // 案例 3：字數統計 → map 或 unordered_map
    std::cout << "\n=== 案例 3：字數統計 ===" << std::endl;
    std::string text = "hello world hello c++ world world";
    std::unordered_map<std::string, int> word_count;
    
    // 簡化的分詞
    std::string word;
    for (char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                word_count[word]++;
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) word_count[word]++;
    
    for (const auto& pair : word_count) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    
    // 案例 4：任務排程 → priority_queue
    std::cout << "\n=== 案例 4：任務排程 ===" << std::endl;
    std::priority_queue<std::pair<int, std::string>> tasks;
    
    tasks.push({3, "一般任務"});
    tasks.push({5, "緊急任務"});
    tasks.push({1, "低優先任務"});
    tasks.push({4, "重要任務"});
    
    std::cout << "按優先順序處理:" << std::endl;
    while (!tasks.empty()) {
        auto task = tasks.top();
        std::cout << "  優先級 " << task.first << ": " << task.second << std::endl;
        tasks.pop();
    }

    // ── LeetCode 692. Top K Frequent Words ──────────────────────
    std::cout << "\n=== LeetCode 692. Top K Frequent Words ===" << std::endl;
    {
        std::vector<std::string> words = {"i", "love", "leetcode", "i", "love", "coding"};
        auto top = topKFrequentWords(words, 2);
        std::cout << "  words = {i, love, leetcode, i, love, coding}, k=2 → ";
        for (const auto& w : top) std::cout << w << " ";
        std::cout << std::endl;

        std::vector<std::string> words2 = {
            "the", "day", "is", "sunny", "the", "the", "the", "sunny", "is", "is"
        };
        auto top2 = topKFrequentWords(words2, 4);
        std::cout << "  第二組, k=4 → ";
        for (const auto& w : top2) std::cout << w << " ";
        std::cout << std::endl;
    }

    // ── 日常實務：解析 INI 設定檔 ────────────────────────
    std::cout << "\n=== 日常實務: 解析 INI 設定檔 ===" << std::endl;
    {
        std::vector<std::string> configLines = {
            "# 這是註解，應被忽略",
            "app_name = MyService",
            "",
            "[server]",
            "  port   =  8080  ",
            "host=0.0.0.0",
            "; 分號也是註解",
            "port = 9090",
            "[database]",
            "url = postgres://localhost/app",
            "pool_size = 20",
            "這行沒有等號，應被跳過",
        };

        ConfigData config = parseIniConfig(configLines);
        for (const auto& [section, kvs] : config) {
            std::cout << "  [" << section << "]" << std::endl;
            for (const auto& [key, value] : kvs) {
                std::cout << "    " << key << " = " << value << std::endl;
            }
        }
        std::cout << "  （注意 port 被後者覆蓋為 9090；各 section 內的 key 已按字母排序）"
                  << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類16.cpp" -o lesson6_notes

// 注意：案例 3 的字數統計使用 std::unordered_map，其走訪順序未由標準規定，
//       不同實作／不同標準庫版本可能不同（本機 libstdc++ 在固定插入序列下可重現）。
//       需要穩定輸出時應複製到 vector 排序後再輸出，或改用 std::map。

// === 預期輸出 ===
// === 案例 1：學生成績 ===
// 第三個學生: 78
//
// === 案例 2：去重標籤 ===
// C++ Programming STL
//
// === 案例 3：字數統計 ===
// world: 3
// c++: 1
// hello: 2
//
// === 案例 4：任務排程 ===
// 按優先順序處理:
//   優先級 5: 緊急任務
//   優先級 4: 重要任務
//   優先級 3: 一般任務
//   優先級 1: 低優先任務
//
// === LeetCode 692. Top K Frequent Words ===
//   words = {i, love, leetcode, i, love, coding}, k=2 → i love
//   第二組, k=4 → the is sunny day
//
// === 日常實務: 解析 INI 設定檔 ===
//   [database]
//     pool_size = 20
//     url = postgres://localhost/app
//   [global]
//     app_name = MyService
//   [server]
//     host = 0.0.0.0
//     port = 9090
//   （注意 port 被後者覆蓋為 9090；各 section 內的 key 已按字母排序）
