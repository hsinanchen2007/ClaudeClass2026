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

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  複習完畢！六大組件：容器、迭代器、演算法、函數物件、配接器、配置器" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
