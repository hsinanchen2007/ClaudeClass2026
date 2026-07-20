/*
 * ================================================================
 * 【第3課：STL 的六大組件概覽】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 容器（Containers）：儲存資料的類別模板
 * 2. 迭代器（Iterators）：遍歷容器的「泛化指標」
 * 3. 演算法（Algorithms）：操作資料的函數模板
 * 4. 函數物件（Functors）：重載 operator() 的物件
 * 5. 配接器（Adapters）：包裝其他組件介面的工具
 * 6. 配置器（Allocators）：管理記憶體配置與釋放
 * ================================================================
 * 目錄：
 *   重點一：容器（Containers）— 序列、關聯、容器配接器
 *   重點二：迭代器（Iterators）— begin/end、五種類別
 *   重點三：演算法（Algorithms）— sort/find/count/accumulate
 *   重點四：函數物件（Functors）— 函數指標、函數物件、Lambda
 *   重點五：配接器（Adapters）— stack/queue/back_inserter/bind
 *   重點六：配置器（Allocators）— allocator 基本用法
 *   重點七：六大組件協作範例
 * ================================================================
 */

// =============================================================================
//  第 3 課 總複習  —  STL 六大組件
// =============================================================================
//
// 【主題資訊 Information】
//   六大組件與對應標頭檔：
//     容器 Containers   <vector> <list> <deque> <set> <map> <array> …
//     迭代器 Iterators  <iterator>（begin/end/advance/distance/各種 adapter）
//     演算法 Algorithms <algorithm> <numeric>（sort/find/count/accumulate …）
//     函數物件 Functors <functional>（less/greater/plus、bind、function）
//     配接器 Adapters   <stack> <queue>（容器配接器）、back_inserter（迭代器配接器）
//     配置器 Allocators <memory>（std::allocator）
//
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. STL 真正的核心思想：正交分解】
//   STL 最了不起的地方不是「提供了很多容器與演算法」，
//   而是**把「資料怎麼存」與「對資料做什麼」徹底拆開**。
//   如果沒有拆開，M 種容器 × N 種演算法就要寫 M×N 份程式碼；
//   拆開之後只要寫 M + N 份。
//   讓這件事成立的關鍵就是中間那層**迭代器**——
//   它是一個共同語言，容器只要「會說」它，演算法就能用。
//
// 【2. 迭代器：黏合劑，也是效能的分水嶺】
//   迭代器不只是「泛化的指標」，它還帶著**能力標籤**（iterator category）：
//       input / output      → 只能走一次
//       forward             → 可多次走訪
//       bidirectional       → 可 ++ 也可 --（list、set、map）
//       random access       → 可 +n、可相減（vector、deque、原始指標）
//   演算法會依這個標籤選擇不同實作：
//       std::sort 需要 random access → 所以 **list 不能用 std::sort**，
//       它有自己的成員函式 lst.sort()。
//       std::distance 對 random access 是 O(1)，對其他是 O(n)。
//   **看到某個演算法對某容器不能用，多半是 iterator category 不夠。**
//
// 【3. 函數物件為什麼比函式指標好】
//   函數物件（functor）是「重載了 operator() 的物件」。它勝過函式指標的地方：
//     (a) 可以有狀態（成員變數），本檔的 IsDivisibleBy 就帶著除數
//     (b) **可以被 inline**——函式指標要間接呼叫，編譯器通常無法內聯；
//         functor 的型別在編譯期就確定，operator() 可以完全展開
//     (c) 型別唯一，可以當模板參數（例如 set 的比較器）
//   這就是為什麼 std::sort 通常比 C 的 qsort 快——
//   ⚠️ 但這個差距**高度依賴最佳化等級**：-O0 時 qsort 甚至可能更快，
//      因為 sort 的模板層層呼叫都還沒被 inline。任何效能宣稱都必須註明 -O 等級。
//
// 【4. 配接器：改介面，不改實作】
//   STL 有三種配接器：
//       容器配接器：stack / queue / priority_queue（包容器，**限制**介面）
//       迭代器配接器：back_inserter / reverse_iterator（包迭代器，改變行為）
//       函數配接器：bind / not_fn（包可呼叫物，改變參數）
//   共同點是「不重新實作功能，只把既有的東西換一個介面呈現」。
//   back_inserter 特別值得注意：它讓「賦值」變成「push_back」，
//   於是 std::copy 這種只會寫入的演算法也能對空容器擴充。
//
// 【5. 配置器：最少被直接使用、卻無所不在的一層】
//   每個容器其實都有第二個模板參數：vector<T, Allocator<T>>。
//   allocator 把「取得記憶體」與「在上面建構物件」拆成兩步
//   （allocate/deallocate vs construct/destroy），
//   這正是 vector 能做到 reserve（有容量、沒元素）的原因。
//   一般應用幾乎不需要自訂 allocator；
//   會用到的場合是嵌入式記憶體池、遊戲引擎的 frame allocator、
//   或需要精確追蹤配置行為時（第 20 課就用自訂 allocator 數過配置次數）。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼 std::sort 不能用在 list 上
//     std::sort 內部是 introsort（quicksort + heapsort + insertion sort），
//     需要隨機跳到「中位數位置」「分割點」——這要求 random access iterator。
//     list 只有 bidirectional iterator，所以編譯就會失敗。
//     list 提供自己的 sort()，用的是不需要隨機存取的 merge sort，
//     而且只搬指標、不搬元素。
//
//   ● 演算法為什麼「不能改變容器大小」
//     演算法只拿到一對迭代器，碰不到容器物件本身，
//     所以 std::remove / std::unique 都只能「重排元素並回傳新結尾」，
//     真正的刪除要靠容器的 erase（第 20 課的 erase-remove 慣用法）。
//     這是正交分解必然的代價：換來泛用性，代價是這個兩段式慣用法。
//
//   ● 六大組件如何協作
//     一個典型的 STL 敘述會同時用到多個組件：
//         std::sort(v.begin(), v.end(), std::greater<int>());
//         └─演算法─┘ └───迭代器───┘  └──函數物件──┘
//     而 v 是容器、它內部用 allocator 配置記憶體。
//     六個組件裡有五個出現在這一行。
//
// 【注意事項 Pay Attention】
//   1. std::sort 需要 random access iterator → **list / set / map 不能用**。
//   2. 演算法不能改變容器大小；remove/unique 只重排並回傳新結尾。
//   3. set / map 的 key 是 const，不能透過迭代器修改。
//   4. map::operator[] 在 key 不存在時會**新增元素**；查詢請用 find/count。
//   5. 任何「A 比 B 快」的宣稱都必須註明**最佳化等級**——
//      -O0 與 -O2 的結論可能完全相反。
//   6. std::allocator 的介面在 C++17/20 有多次調整（如移除 construct/destroy），
//      自訂 allocator 時要注意目標標準版本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 六大組件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 為什麼要把容器與演算法分開？迭代器在中間扮演什麼角色？
//     答：為了避免組合爆炸。若不分開，M 種容器 × N 種演算法要寫 M×N 份程式碼；
//         分開之後只要 M + N 份。
//         迭代器是兩者之間的**共同語言**：容器負責提供 begin()/end()，
//         演算法只認迭代器、不認容器，於是同一個 std::find
//         可以用在 vector、list、set，甚至原始陣列上。
//     追問：這個設計有什麼代價？→ 演算法碰不到容器本身，
//         所以無法改變容器大小。std::remove 只能重排元素並回傳新結尾，
//         真正的刪除要另外呼叫容器的 erase——這就是 erase-remove 慣用法的由來。
//
// 🔥 Q2. 為什麼 std::sort 不能用在 std::list 上？
//     答：std::sort 需要 **random access iterator**（它要跳到分割點、算中位數），
//         而 list 只提供 bidirectional iterator，編譯就會失敗。
//         list 有自己的成員函式 lst.sort()，用的是不需要隨機存取的 merge sort，
//         而且只重接指標、不搬移元素。
//     追問：怎麼知道某個演算法需要哪種迭代器？→ 看 cppreference 的參數名：
//         RandomIt / BidirIt / ForwardIt / InputIt 直接標示了最低要求。
//
// ⚠️ 陷阱. 「函數物件一定比函式指標快，所以 std::sort 一定比 qsort 快」——
//         這句話什麼時候會被打臉？
//     答：**在 -O0 下就會**。functor 的優勢來自「型別在編譯期確定 → operator()
//         可以被 inline」，但未開最佳化時編譯器根本不做 inline，
//         此時 std::sort 層層模板呼叫的開銷反而可能輸給 qsort 的單層間接呼叫。
//         開了 -O2 之後 functor 被完全展開，std::sort 才明顯勝出。
//     為什麼會錯：把「這個設計有利於最佳化」當成「它在任何情況下都比較快」。
//         零成本抽象的前提是**最佳化器有在工作**。
//         這也是為什麼所有效能數據都必須註明編譯器與 -O 等級，
//         否則那個數字無法解讀，也無法重現。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <stack>
#include <queue>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

// ===== 重點一：容器（Containers） =====
// STL 容器分為三大類：
//   序列容器（Sequence Containers）：array, vector, deque, list, forward_list
//   關聯容器（Associative Containers）：set, multiset, map, multimap
//   無序容器（Unordered Containers）：unordered_set, unordered_map ...
//   容器配接器（Container Adapters）：stack, queue, priority_queue

void demo_containers() {
    std::cout << "===== 重點一：容器 =====" << std::endl;

    // --- 序列容器 ---

    // vector：動態陣列，尾端插入 O(1)，支援隨機存取
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // list：雙向鏈結串列，任意位置插入 O(1)，不支援隨機存取
    std::list<int> lst = {10, 20, 30};
    lst.push_front(5);  // list 可高效地在前端插入
    std::cout << "list(push_front 5): ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // deque：雙端佇列，兩端插入 O(1)，支援隨機存取
    std::deque<int> deq = {100, 200, 300};
    deq.push_front(50);
    deq.push_back(400);
    std::cout << "deque: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;

    // --- 關聯容器 ---

    // set：有序集合，自動排序，不允許重複
    std::set<int> s = {30, 10, 20, 10, 30};  // 重複的會被忽略
    std::cout << "set(自動排序去重): ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;

    // map：有序鍵值對，鍵自動排序，鍵不重複
    std::map<std::string, int> ages;
    ages["Alice"] = 25;
    ages["Bob"]   = 30;
    ages["Charlie"] = 35;
    std::cout << "map: ";
    for (const auto& p : ages) {
        std::cout << p.first << "=" << p.second << " ";
    }
    std::cout << std::endl;
}

// ===== 重點二：迭代器（Iterators） =====
// 迭代器是「泛化的指標」，提供統一方式遍歷任何容器。
// 每個容器都有 begin()（指向第一個元素）和 end()（指向最後元素的下一位）。
// 這形成「半開區間」[begin, end)，好處：
//   - 空容器時 begin() == end()
//   - 遍歷邏輯統一（it != end()）
//   - 支援子區間表示

void demo_iterators() {
    std::cout << "\n===== 重點二：迭代器 =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 方法一：完整型別迭代器（冗長）
    std::cout << "完整型別迭代器: ";
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";  // *it 取得值（解參考）
    }
    std::cout << std::endl;

    // 方法二：auto 簡化（推薦）
    std::cout << "auto 迭代器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 方法三：範圍 for（底層也是迭代器）
    std::cout << "範圍 for: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // const_iterator（唯讀，透過 cbegin/cend 取得）
    std::cout << "const_iterator(cbegin/cend): ";
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        // *it = 99;  // 編譯錯誤！const_iterator 不允許修改
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // reverse_iterator（反向，透過 rbegin/rend 取得）
    std::cout << "reverse_iterator: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // 迭代器類別差異：vector 支援隨機存取，list 只支援雙向
    std::cout << "vector 隨機存取: vit[2]=" << vec.begin()[2]
              << ", *(vit+3)=" << *(vec.begin() + 3) << std::endl;

    std::list<int> lst2 = {10, 20, 30, 40, 50};
    auto lit = lst2.begin();
    ++lit;  // list 只能 ++ 和 --，不能 +n 或 [n]
    --lit;
    std::cout << "list 雙向迭代器: ++後--回原值=" << *lit << std::endl;
}

// ===== 重點三：演算法（Algorithms） =====
// 演算法透過迭代器操作容器，與容器解耦。
// 包含：排序、查找、計數、轉換、複製、累加等。
// 重要：std::sort 需要 Random Access Iterator，list 要用自己的 sort()。

void demo_algorithms() {
    std::cout << "\n===== 重點三：演算法 =====" << std::endl;

    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    // sort：排序（需要 Random Access Iterator）
    std::sort(vec.begin(), vec.end());
    std::cout << "sort 升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // find：線性搜尋，找到回傳迭代器，找不到回傳 end()
    auto it = std::find(vec.begin(), vec.end(), 7);
    if (it != vec.end()) {
        std::cout << "find(7) 位置: " << (it - vec.begin()) << std::endl;
    }

    // count：計算某值出現次數
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2, 5};
    int cnt = std::count(data.begin(), data.end(), 2);
    std::cout << "count(2): " << cnt << std::endl;

    // accumulate（需要 <numeric>）：累加（第三參數為初始值）
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "accumulate: " << sum << std::endl;

    // minmax_element：同時找最小和最大
    auto mm = std::minmax_element(vec.begin(), vec.end());
    std::cout << "min=" << *mm.first << " max=" << *mm.second << std::endl;

    // 同一演算法用在不同容器（透過迭代器解耦）
    std::list<int> lst = {9, 2, 6, 5, 3};
    std::deque<int> deq = {5, 8, 9, 7, 9};

    std::sort(vec.begin(), vec.end());
    std::sort(deq.begin(), deq.end());  // deque 支援 sort
    lst.sort();  // list 用自己的 sort（std::sort 需要隨機存取）

    // find 可以用在所有容器
    auto found_v = std::find(vec.begin(), vec.end(), 5);
    auto found_l = std::find(lst.begin(), lst.end(), 5);
    auto found_d = std::find(deq.begin(), deq.end(), 5);
    std::cout << "find(5) in vector: " << (found_v != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "find(5) in list:   " << (found_l != lst.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "find(5) in deque:  " << (found_d != deq.end() ? "找到" : "沒找到") << std::endl;
}

// ===== 重點四：函數物件（Function Objects / Functors） =====
// 函數物件是重載了 operator() 的類別實例，可以像函數一樣被呼叫。
// 優點：可以攜帶狀態、可內聯、比函數指標更靈活。
// 三種實現方式：普通函數、函數物件類別、Lambda（C++11，最簡潔）

// 方法一：普通函數
bool is_even_func(int n) { return n % 2 == 0; }

// 方法二：函數物件（無狀態）
class IsEven {
public:
    bool operator()(int n) const { return n % 2 == 0; }
};

// 方法三：帶狀態的函數物件
class IsDivisibleBy {
    int divisor_;
public:
    explicit IsDivisibleBy(int d) : divisor_(d) {}
    bool operator()(int n) const { return n % divisor_ == 0; }
};

void demo_functors() {
    std::cout << "\n===== 重點四：函數物件 =====" << std::endl;

    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 使用普通函數指標
    int c1 = std::count_if(vec.begin(), vec.end(), is_even_func);
    std::cout << "偶數個數（函數指標）: " << c1 << std::endl;

    // 使用函數物件
    int c2 = std::count_if(vec.begin(), vec.end(), IsEven());
    std::cout << "偶數個數（函數物件）: " << c2 << std::endl;

    // 使用帶狀態函數物件（可以複用不同除數）
    int c3 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(3));
    int c4 = std::count_if(vec.begin(), vec.end(), IsDivisibleBy(5));
    std::cout << "3的倍數個數: " << c3 << std::endl;
    std::cout << "5的倍數個數: " << c4 << std::endl;

    // Lambda（C++11，最簡潔的寫法）
    int c5 = std::count_if(vec.begin(), vec.end(),
        [](int n) { return n % 2 == 0; }
    );
    std::cout << "偶數個數（Lambda）: " << c5 << std::endl;

    // Lambda 捕獲外部變數
    int divisor = 3;
    int c6 = std::count_if(vec.begin(), vec.end(),
        [divisor](int n) { return n % divisor == 0; }
    );
    std::cout << "3的倍數個數（Lambda 捕獲）: " << c6 << std::endl;

    // transform + Lambda：對每個元素做轉換
    std::vector<int> squared(vec.size());
    std::transform(vec.begin(), vec.end(), squared.begin(),
        [](int n) { return n * n; }
    );
    std::cout << "平方: ";
    for (int n : squared) std::cout << n << " ";
    std::cout << std::endl;

    // STL 內建函數物件（<functional>）
    std::vector<int> v2 = {5, 2, 8, 1, 9};
    std::sort(v2.begin(), v2.end());
    std::cout << "升序: ";
    for (int n : v2) std::cout << n << " ";
    std::cout << std::endl;

    std::sort(v2.begin(), v2.end(), std::greater<int>());
    std::cout << "降序(greater<int>): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << std::endl;

    // 其他內建函數物件示範
    std::cout << "plus<int>(3,4) = "       << std::plus<int>()(3, 4)       << std::endl;
    std::cout << "minus<int>(10,3) = "     << std::minus<int>()(10, 3)     << std::endl;
    std::cout << "multiplies<int>(5,6) = " << std::multiplies<int>()(5, 6) << std::endl;
}

// ===== 重點五：配接器（Adapters） =====
// 配接器分三類：
//   容器配接器：stack（LIFO）、queue（FIFO）、priority_queue（最大優先）
//   迭代器配接器：reverse_iterator、back_inserter、ostream_iterator
//   函數配接器：std::bind（C++11 後多用 Lambda 取代）

void demo_adapters() {
    std::cout << "\n===== 重點五：配接器 =====" << std::endl;

    // --- 容器配接器 ---

    // stack：後進先出（LIFO），底層預設用 deque 實作
    std::stack<int> stk;
    stk.push(1); stk.push(2); stk.push(3);
    std::cout << "stack(LIFO): ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;

    // queue：先進先出（FIFO），底層預設用 deque 實作
    std::queue<int> que;
    que.push(1); que.push(2); que.push(3);
    std::cout << "queue(FIFO): ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();
    }
    std::cout << std::endl;

    // priority_queue：優先佇列，預設最大值優先（底層用 heap）
    std::priority_queue<int> pq;
    pq.push(30); pq.push(10); pq.push(50); pq.push(20);
    std::cout << "priority_queue(最大優先): ";
    while (!pq.empty()) {
        std::cout << pq.top() << " ";
        pq.pop();
    }
    std::cout << std::endl;

    // --- 迭代器配接器 ---

    std::vector<int> vec = {1, 2, 3, 4, 5};

    // reverse_iterator：反向迭代（rbegin/rend）
    std::cout << "反向迭代(rbegin/rend): ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // back_inserter：自動在容器尾端插入（避免預先 resize）
    std::vector<int> src = {10, 20, 30};
    std::vector<int> dest;
    std::copy(src.begin(), src.end(), std::back_inserter(dest));
    std::cout << "back_inserter 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;

    // ostream_iterator：將元素輸出到串流
    std::cout << "ostream_iterator: ";
    std::copy(vec.begin(), vec.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    // --- 函數配接器 ---

    // std::bind：綁定參數，建立新的可呼叫物件
    // 等號左側的 _1 是佔位符，代表呼叫時傳入的第一個參數
    std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto is_gt5 = std::bind(std::greater<int>(), std::placeholders::_1, 5);
    int cnt_bind = std::count_if(nums.begin(), nums.end(), is_gt5);
    std::cout << "bind(大於5的個數): " << cnt_bind << std::endl;

    // Lambda 是更推薦的現代替代寫法
    int cnt_lambda = std::count_if(nums.begin(), nums.end(),
        [](int n) { return n > 5; }
    );
    std::cout << "Lambda(大於5的個數): " << cnt_lambda << std::endl;
}

// ===== 重點六：配置器（Allocators） =====
// 配置器負責容器的記憶體配置與釋放。
// 大多數情況使用預設 std::allocator，進階場景（記憶體池、追蹤）才需自訂。
// vector<int> 完整型別是 vector<int, allocator<int>>。

void demo_allocators() {
    std::cout << "\n===== 重點六：配置器 =====" << std::endl;

    // 預設配置器（兩者等價）
    std::vector<int> vec1;
    std::vector<int, std::allocator<int>> vec2;
    vec1.push_back(1);
    vec2.push_back(2);
    std::cout << "vec1[0]=" << vec1[0] << " vec2[0]=" << vec2[0] << std::endl;

    // allocator 基本用法示範（手動配置/建構/解構/釋放）
    std::allocator<int> alloc;

    // 配置記憶體空間（不建構物件）
    int* ptr = alloc.allocate(5);

    // 建構物件（placement new 語義）
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::construct(alloc, ptr + i, i * 10);
    }

    std::cout << "手動配置陣列: ";
    for (int i = 0; i < 5; ++i) std::cout << ptr[i] << " ";
    std::cout << std::endl;

    // 解構物件
    for (int i = 0; i < 5; ++i) {
        std::allocator_traits<std::allocator<int>>::destroy(alloc, ptr + i);
    }

    // 釋放記憶體（必須配對使用，且大小要一致）
    alloc.deallocate(ptr, 5);
    std::cout << "記憶體已釋放" << std::endl;
}

// ===== 重點七：六大組件協作完整範例 =====
// 本例同時展示全部六大組件的協同工作：
//   容器：vector 儲存資料
//   迭代器：begin/end 連接容器與演算法
//   演算法：sort、copy_if、transform
//   函數物件：greater<int>()、Lambda
//   配接器：back_inserter（迭代器配接器）、ostream_iterator
//   配置器：幕後自動管理 vector 的記憶體

void demo_all_components() {
    std::cout << "\n===== 重點七：六大組件協作 =====" << std::endl;

    // 【容器】儲存原始資料
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

    // 【演算法】copy_if + 【Lambda】+ 【迭代器配接器】back_inserter
    std::vector<int> evens;
    std::copy_if(numbers.begin(), numbers.end(),
                 std::back_inserter(evens),
                 [](int n) { return n % 2 == 0; });
    std::cout << "偶數: ";
    std::copy(evens.begin(), evens.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    // 【演算法】transform + 【Lambda】
    std::vector<int> doubled(evens.size());
    std::transform(evens.begin(), evens.end(), doubled.begin(),
                   [](int n) { return n * 2; });
    std::cout << "偶數加倍: ";
    std::copy(doubled.begin(), doubled.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
}

// ================================================================
// 【LeetCode 實戰範例 1】LeetCode 217. Contains Duplicate
//   題目：判斷陣列中是否存在重複元素。
//   為什麼用到本課主題：一題就同時動用三個組件——
//     **容器**（set 提供自動去重）、**迭代器**（區間建構子吃 begin/end）、
//     以及「容器與演算法解耦」的思想（同一段程式碼換成 vector 也能編譯）。
// ================================================================
bool containsDuplicate(const std::vector<int>& nums) {
    std::set<int> unique(nums.begin(), nums.end());   // 迭代器區間 + 容器去重
    return unique.size() < nums.size();
}

// ================================================================
// 【LeetCode 實戰範例 2】LeetCode 349. Intersection of Two Arrays
//   題目：回傳兩個陣列的交集，結果中每個元素只出現一次。
//   為什麼用到本課主題：這題是六大組件協作的縮影——
//     容器（set）、迭代器（begin/end）、演算法（set_intersection）、
//     迭代器配接器（back_inserter，把「賦值」變成「push_back」）。
//     用 std::set_intersection 而非手寫迴圈，正是「演算法可重用」的示範。
// ================================================================
std::vector<int> intersection(const std::vector<int>& nums1,
                              const std::vector<int>& nums2) {
    std::set<int> a(nums1.begin(), nums1.end());      // 去重 + 排序
    std::set<int> b(nums2.begin(), nums2.end());
    std::vector<int> result;
    // set_intersection 要求兩個輸入都已排序——set 天生滿足
    // back_inserter 讓演算法能對空的 result 擴充（迭代器配接器）
    std::set_intersection(a.begin(), a.end(),
                          b.begin(), b.end(),
                          std::back_inserter(result));
    return result;
}

// ================================================================
// 【LeetCode 實戰範例 3】LeetCode 1480. Running Sum of 1d Array
//   題目：回傳前綴和陣列，result[i] = nums[0] + ... + nums[i]。
//   為什麼用到本課主題：這題有一個現成的標準演算法
//     std::partial_sum（在 <numeric>），一行就解決。
//     它示範了「先問標準庫有沒有」這個習慣——
//     手寫迴圈不會比較快，但比較容易寫錯邊界。
// ================================================================
std::vector<int> runningSum(const std::vector<int>& nums) {
    std::vector<int> result(nums.size());
    std::partial_sum(nums.begin(), nums.end(), result.begin());
    return result;
}

// ================================================================
// 【日常實務範例】銷售資料分析：六大組件一次全用上
//   情境：一份銷售紀錄（產品名、金額），要產出三份報表：
//       ① 各產品總金額（依產品名排序）      → map（容器）+ operator[]
//       ② 金額前 N 大的交易                → vector + partial_sort（演算法 + 函數物件）
//       ③ 有交易的產品清單（不重複）        → set（容器自動去重）
//   這正是實務中最常見的樣貌：不是「用某個容器」，
//   而是**依每份報表的需求各挑合適的容器與演算法**。
// ================================================================
struct Sale {
    std::string product;
    int amount;
};

struct SalesReport {
    std::map<std::string, int> totalByProduct;   // 自動依產品名排序
    std::vector<Sale> topDeals;                  // 金額前 N 大
    std::set<std::string> productNames;          // 不重複產品清單
    int grandTotal = 0;
};

SalesReport analyzeSales(const std::vector<Sale>& sales, std::size_t topN) {
    SalesReport r;

    // ① 容器 map：operator[] 的「不存在就建立 0」在這裡正是我們要的
    for (const Sale& s : sales) {
        r.totalByProduct[s.product] += s.amount;
        r.productNames.insert(s.product);        // ③ set：自動去重
    }

    // 演算法 + 函數物件：accumulate 搭配 lambda 算總額
    r.grandTotal = std::accumulate(sales.begin(), sales.end(), 0,
                                   [](int acc, const Sale& s) { return acc + s.amount; });

    // ② 演算法 partial_sort：只排出前 N 大，比全排序省
    r.topDeals = sales;
    std::size_t n = std::min(topN, r.topDeals.size());
    std::partial_sort(r.topDeals.begin(), r.topDeals.begin() + static_cast<long>(n),
                      r.topDeals.end(),
                      [](const Sale& a, const Sale& b) { return a.amount > b.amount; });
    r.topDeals.resize(n);
    return r;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  第3課：STL 的六大組件概覽  總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    demo_containers();
    demo_iterators();
    demo_algorithms();
    demo_functors();
    demo_adapters();
    demo_allocators();
    demo_all_components();

    // ================================================================
    // 重點八：iterator category 決定了哪些演算法能用
    // ================================================================
    std::cout << "\n===== 重點八：iterator category 決定演算法可用性 =====" << std::endl;
    std::cout << "vector 的 iterator 是 random access？ " << std::boolalpha
              << std::is_same_v<
                     std::iterator_traits<std::vector<int>::iterator>::iterator_category,
                     std::random_access_iterator_tag> << std::endl;
    std::cout << "list 的 iterator 是 bidirectional？   "
              << std::is_same_v<
                     std::iterator_traits<std::list<int>::iterator>::iterator_category,
                     std::bidirectional_iterator_tag> << std::endl;
    std::cout << "set 的 iterator 是 bidirectional？    "
              << std::is_same_v<
                     std::iterator_traits<std::set<int>::iterator>::iterator_category,
                     std::bidirectional_iterator_tag> << std::endl;
    std::cout << "→ std::sort 需要 random access，所以只有 vector/deque 能用；" << std::endl;
    std::cout << "  list 要用自己的成員函式 lst.sort()（merge sort，只搬指標）。" << std::endl;

    std::list<int> ls = {5, 2, 9, 1, 7};
    // std::sort(ls.begin(), ls.end());   // ← 編譯失敗：iterator category 不夠
    ls.sort();                            // ✅ list 自己的 sort
    std::cout << "list 用成員函式 sort() 後：";
    for (int x : ls) std::cout << x << " ";
    std::cout << std::endl;

    // ================================================================
    // 重點九：演算法不能改變容器大小
    // ================================================================
    std::cout << "\n===== 重點九：演算法不能改變容器大小 =====" << std::endl;
    std::vector<int> rv = {1, 2, 3, 4, 5, 6};
    auto newEnd = std::remove_if(rv.begin(), rv.end(),
                                 [](int x) { return x % 2 == 0; });
    std::cout << "remove_if 之後 size = " << rv.size() << " ← 完全沒變！" << std::endl;
    std::cout << "保留區內容：";
    for (auto it = rv.begin(); it != newEnd; ++it) std::cout << *it << " ";
    std::cout << std::endl;
    rv.erase(newEnd, rv.end());          // 這一步才真的刪掉
    std::cout << "再呼叫容器的 erase 之後 size = " << rv.size() << std::endl;
    std::cout << "→ 演算法只拿到迭代器、碰不到容器本身，這是正交分解的必然代價。"
              << std::endl;

    // ================================================================
    // LeetCode 實戰
    // ================================================================
    std::cout << "\n===== LeetCode 217. Contains Duplicate =====" << std::endl;
    std::cout << "  [1,2,3,1]     → " << containsDuplicate({1, 2, 3, 1}) << std::endl;
    std::cout << "  [1,2,3,4]     → " << containsDuplicate({1, 2, 3, 4}) << std::endl;
    std::cout << "  [1,1,1,3,3,4] → " << containsDuplicate({1, 1, 1, 3, 3, 4}) << std::endl;

    std::cout << "\n===== LeetCode 349. Intersection of Two Arrays =====" << std::endl;
    auto show = [](const char* label, const std::vector<int>& r) {
        std::cout << "  " << label;
        for (std::size_t i = 0; i < r.size(); ++i)
            std::cout << r[i] << (i + 1 < r.size() ? "," : "");
        std::cout << std::endl;
    };
    show("[1,2,2,1] ∩ [2,2]     = ", intersection({1, 2, 2, 1}, {2, 2}));
    show("[4,9,5] ∩ [9,4,9,8,4] = ", intersection({4, 9, 5}, {9, 4, 9, 8, 4}));
    std::cout << "  → 用了容器(set)、迭代器、演算法(set_intersection)、"
                 "迭代器配接器(back_inserter)" << std::endl;

    std::cout << "\n===== LeetCode 1480. Running Sum of 1d Array =====" << std::endl;
    show("[1,2,3,4]     → ", runningSum({1, 2, 3, 4}));
    show("[1,1,1,1,1]   → ", runningSum({1, 1, 1, 1, 1}));
    show("[3,1,2,10,1]  → ", runningSum({3, 1, 2, 10, 1}));
    std::cout << "  → std::partial_sum 一行解決；先問標準庫有沒有，"
                 "比手寫迴圈更不容易寫錯邊界" << std::endl;

    // ================================================================
    // 日常實務：銷售資料分析
    // ================================================================
    std::cout << "\n===== 日常實務：銷售資料分析 =====" << std::endl;
    std::vector<Sale> sales = {
        {"筆電",   45000}, {"滑鼠",   800},  {"筆電",   52000},
        {"鍵盤",   2400},  {"螢幕",   9800}, {"滑鼠",   1200},
        {"筆電",   38000}, {"螢幕",   12000}
    };

    SalesReport rep = analyzeSales(sales, 3);
    std::cout << "交易筆數 " << sales.size()
              << "，總金額 " << rep.grandTotal << std::endl;

    std::cout << "① 各產品總金額（map 自動依名稱排序）：" << std::endl;
    for (const auto& [product, total] : rep.totalByProduct) {
        std::cout << "     " << product << " → " << total << std::endl;
    }

    std::cout << "② 金額前 3 大交易（partial_sort + lambda 比較器）：" << std::endl;
    for (const Sale& s : rep.topDeals) {
        std::cout << "     " << s.product << " " << s.amount << std::endl;
    }

    std::cout << "③ 有交易的產品（set 自動去重排序）共 "
              << rep.productNames.size() << " 種：";
    for (const std::string& p : rep.productNames) std::cout << p << " ";
    std::cout << std::endl;
    std::cout << "→ 一份資料、三種報表，各自挑合適的容器與演算法——" << std::endl;
    std::cout << "  這就是六大組件在實務中真正的用法。" << std::endl;

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  複習完畢！六大組件：容器、迭代器、演算法、函數物件、配接器、配置器" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ================================================================
//   第3課：STL 的六大組件概覽  總複習
// ================================================================
// ===== 重點一：容器 =====
// vector: 1 2 3 4 5
// list(push_front 5): 5 10 20 30
// deque: 50 100 200 300 400
// set(自動排序去重): 10 20 30
// map: Alice=25 Bob=30 Charlie=35
//
// ===== 重點二：迭代器 =====
// 完整型別迭代器: 10 20 30 40 50
// auto 迭代器: 10 20 30 40 50
// 範圍 for: 10 20 30 40 50
// const_iterator(cbegin/cend): 10 20 30 40 50
// reverse_iterator: 50 40 30 20 10
// vector 隨機存取: vit[2]=30, *(vit+3)=40
// list 雙向迭代器: ++後--回原值=10
//
// ===== 重點三：演算法 =====
// sort 升序: 1 2 3 4 5 6 7 8 9
// find(7) 位置: 6
// count(2): 4
// accumulate: 45
// min=1 max=9
// find(5) in vector: 找到
// find(5) in list:   找到
// find(5) in deque:  找到
//
// ===== 重點四：函數物件 =====
// 偶數個數（函數指標）: 5
// 偶數個數（函數物件）: 5
// 3的倍數個數: 3
// 5的倍數個數: 2
// 偶數個數（Lambda）: 5
// 3的倍數個數（Lambda 捕獲）: 3
// 平方: 1 4 9 16 25 36 49 64 81 100
// 升序: 1 2 5 8 9
// 降序(greater<int>): 9 8 5 2 1
// plus<int>(3,4) = 7
// minus<int>(10,3) = 7
// multiplies<int>(5,6) = 30
//
// ===== 重點五：配接器 =====
// stack(LIFO): 3 2 1
// queue(FIFO): 1 2 3
// priority_queue(最大優先): 50 30 20 10
// 反向迭代(rbegin/rend): 5 4 3 2 1
// back_inserter 結果: 10 20 30
// ostream_iterator: 1 2 3 4 5
// bind(大於5的個數): 5
// Lambda(大於5的個數): 5
//
// ===== 重點六：配置器 =====
// vec1[0]=1 vec2[0]=2
// 手動配置陣列: 0 10 20 30 40
// 記憶體已釋放
//
// ===== 重點七：六大組件協作 =====
// 原始資料: 64 25 12 22 11 90 42
// 降序排序: 90 64 42 25 22 12 11
// 偶數: 90 64 42 22 12
// 偶數加倍: 180 128 84 44 24
//
// ===== 重點八：iterator category 決定演算法可用性 =====
// vector 的 iterator 是 random access？ true
// list 的 iterator 是 bidirectional？   true
// set 的 iterator 是 bidirectional？    true
// → std::sort 需要 random access，所以只有 vector/deque 能用；
//   list 要用自己的成員函式 lst.sort()（merge sort，只搬指標）。
// list 用成員函式 sort() 後：1 2 5 7 9
//
// ===== 重點九：演算法不能改變容器大小 =====
// remove_if 之後 size = 6 ← 完全沒變！
// 保留區內容：1 3 5
// 再呼叫容器的 erase 之後 size = 3
// → 演算法只拿到迭代器、碰不到容器本身，這是正交分解的必然代價。
//
// ===== LeetCode 217. Contains Duplicate =====
//   [1,2,3,1]     → true
//   [1,2,3,4]     → false
//   [1,1,1,3,3,4] → true
//
// ===== LeetCode 349. Intersection of Two Arrays =====
//   [1,2,2,1] ∩ [2,2]     = 2
//   [4,9,5] ∩ [9,4,9,8,4] = 4,9
//   → 用了容器(set)、迭代器、演算法(set_intersection)、迭代器配接器(back_inserter)
//
// ===== LeetCode 1480. Running Sum of 1d Array =====
//   [1,2,3,4]     → 1,3,6,10
//   [1,1,1,1,1]   → 1,2,3,4,5
//   [3,1,2,10,1]  → 3,4,6,16,17
//   → std::partial_sum 一行解決；先問標準庫有沒有，比手寫迴圈更不容易寫錯邊界
//
// ===== 日常實務：銷售資料分析 =====
// 交易筆數 8，總金額 161200
// ① 各產品總金額（map 自動依名稱排序）：
//      滑鼠 → 2000
//      筆電 → 135000
//      螢幕 → 21800
//      鍵盤 → 2400
// ② 金額前 3 大交易（partial_sort + lambda 比較器）：
//      筆電 52000
//      筆電 45000
//      筆電 38000
// ③ 有交易的產品（set 自動去重排序）共 4 種：滑鼠 筆電 螢幕 鍵盤
// → 一份資料、三種報表，各自挑合適的容器與演算法——
//   這就是六大組件在實務中真正的用法。
//
// ================================================================
//   複習完畢！六大組件：容器、迭代器、演算法、函數物件、配接器、配置器
// ================================================================
