/*
 * ================================================================
 * 【第 6 課：容器（Container）的概念與分類】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 容器的概念：管理一組物件的資料結構
 * 2. 序列容器（Sequence Containers）：array, vector, deque, list, forward_list
 * 3. 關聯容器（Associative Containers）：set, map, multiset, multimap
 * 4. 無序關聯容器（Unordered）：unordered_set, unordered_map
 * 5. 容器配接器（Adapters）：stack, queue, priority_queue
 * 6. 如何選擇正確的容器
 * ================================================================
 */

#include <iostream>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <string>
using namespace std;

// ================================================================
// 重點一：序列容器 —— array（固定大小陣列）
// ================================================================
// std::array：大小在編譯期固定，無法動態增減
// 優點：零額外開銷，與 C 陣列效能相同，但更安全（有 at()、size()）
// 適用：大小固定、效能敏感的場景

void demoArray() {
    cout << "\n--- std::array ---" << endl;

    array<int, 5> arr = {10, 20, 30, 40, 50};

    cout << "大小: " << arr.size() << endl;
    cout << "第一個: " << arr.front() << endl;
    cout << "最後一個: " << arr.back() << endl;
    cout << "arr[2]: " << arr[2] << endl;
    // arr.at(10);  // 會拋出 out_of_range 例外
    cout << "元素: ";
    for (int n : arr) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點二：序列容器 —— vector（動態陣列）
// ================================================================
// std::vector：最常用的容器，動態大小，連續記憶體
// 隨機存取 O(1)；尾端新增 O(1)；中間插入/刪除 O(n)

void demoVector() {
    cout << "\n--- std::vector ---" << endl;

    vector<int> v = {1, 2, 3};
    v.push_back(4);          // 尾端新增
    v.push_back(5);

    cout << "size=" << v.size() << ", capacity=" << v.capacity() << endl;
    cout << "元素: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點三：序列容器 —— deque（雙端佇列）
// ================================================================
// std::deque：頭尾都能 O(1) 新增/刪除，不保證連續記憶體
// push_front() 是 vector 沒有的功能

void demoDeque() {
    cout << "\n--- std::deque ---" << endl;

    deque<int> dq = {3, 4, 5};
    dq.push_front(2);   // 頭部插入 O(1)
    dq.push_front(1);
    dq.push_back(6);    // 尾部插入 O(1)

    cout << "元素: ";
    for (int n : dq) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：序列容器 —— list（雙向鏈結串列）
// ================================================================
// std::list：任意位置插入/刪除 O(1)，但隨機存取 O(n)
// 沒有 operator[]，只能用迭代器遍歷

void demoList() {
    cout << "\n--- std::list ---" << endl;

    list<int> lst = {1, 3, 5, 7};
    lst.push_front(0);     // 頭部 O(1)
    lst.push_back(9);      // 尾部 O(1)

    // 在迭代器位置插入
    auto it = lst.begin();
    advance(it, 2);        // 移動到第 3 個
    lst.insert(it, 100);   // 任意位置插入 O(1)

    cout << "元素: ";
    for (int n : lst) cout << n << " ";
    cout << endl;

    lst.sort();            // list 有自己的 sort（不能用 std::sort）
    cout << "排序後: ";
    for (int n : lst) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點五：關聯容器 —— set（有序不重複集合）
// ================================================================
// std::set：自動排序，不允許重複元素，用紅黑樹實作
// 查找/插入/刪除：O(log n)

void demoSet() {
    cout << "\n--- std::set ---" << endl;

    set<int> s = {5, 3, 8, 1, 3, 5};  // 重複的 3、5 只保留一個
    cout << "set（自動排序，去重）: ";
    for (int n : s) cout << n << " ";
    cout << endl;

    // 查找
    auto it = s.find(3);
    cout << "找到 3: " << (it != s.end() ? "是" : "否") << endl;

    // count：在 set 中只有 0 或 1
    cout << "8 的數量: " << s.count(8) << endl;
    cout << "9 的數量: " << s.count(9) << endl;
}

// ================================================================
// 重點六：關聯容器 —— map（有序鍵值對）
// ================================================================
// std::map：key-value 對，按 key 自動排序，key 唯一
// 存取：m[key]（可能自動插入！）；安全存取用 m.at(key)

void demoMap() {
    cout << "\n--- std::map ---" << endl;

    map<string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"] = 87;
    scores["Carol"] = 92;

    // 有序輸出（按 key 字母順序）
    cout << "成績（有序）:" << endl;
    for (const auto& [name, score] : scores) {  // C++17 結構化綁定
        cout << "  " << name << ": " << score << endl;
    }

    // find 比 [] 安全（不會自動插入不存在的 key）
    auto it = scores.find("Dave");
    cout << "找到 Dave: " << (it != scores.end() ? "是" : "否") << endl;
}

// ================================================================
// 重點七：無序關聯容器 —— unordered_map
// ================================================================
// std::unordered_map：用雜湊表實作，平均 O(1) 存取
// 不保證順序，但速度比 map 快（平均）

void demoUnorderedMap() {
    cout << "\n--- std::unordered_map ---" << endl;

    unordered_map<string, int> freq;
    vector<string> words = {"apple", "banana", "apple", "cherry", "banana", "apple"};

    for (const string& w : words) {
        freq[w]++;  // 統計詞頻
    }

    cout << "詞頻（無序）:" << endl;
    for (const auto& [word, count] : freq) {
        cout << "  " << word << ": " << count << endl;
    }
}

// ================================================================
// 重點八：容器配接器 —— stack, queue, priority_queue
// ================================================================
// 配接器不是獨立容器，而是對現有容器的包裝，限制存取方式
// stack    = LIFO（後進先出）；預設用 deque 實作
// queue    = FIFO（先進先出）；預設用 deque 實作
// priority_queue = 最大堆；預設用 vector 實作

void demoAdapters() {
    cout << "\n--- 容器配接器 ---" << endl;

    // stack（LIFO）
    stack<int> stk;
    stk.push(1); stk.push(2); stk.push(3);
    cout << "stack top: " << stk.top() << endl;  // 3
    stk.pop();
    cout << "pop 後 top: " << stk.top() << endl; // 2

    // queue（FIFO）
    queue<int> q;
    q.push(10); q.push(20); q.push(30);
    cout << "queue front: " << q.front() << endl; // 10
    q.pop();
    cout << "pop 後 front: " << q.front() << endl; // 20

    // priority_queue（最大堆）
    priority_queue<int> pq;
    pq.push(5); pq.push(1); pq.push(8); pq.push(3);
    cout << "priority_queue top: " << pq.top() << endl; // 8（最大）
}

// ================================================================
// 重點九：如何選擇容器
// ================================================================
//
// ┌─────────────────┬──────────────────────────────────────────┐
// │ 容器             │ 最佳使用場景                              │
// ├─────────────────┼──────────────────────────────────────────┤
// │ array           │ 大小固定，零開銷                          │
// │ vector          │ 一般用途，尾端增刪頻繁，需要隨機存取      │
// │ deque           │ 頭尾都需要頻繁增刪                        │
// │ list            │ 中間頻繁插入/刪除，不需要隨機存取        │
// │ set             │ 需要自動排序且不重複的集合               │
// │ map             │ 需要 key-value 對應，且有序               │
// │ unordered_map   │ key-value 對應，追求最快查找速度         │
// │ stack           │ 需要 LIFO 行為                           │
// │ queue           │ 需要 FIFO 行為                           │
// │ priority_queue  │ 需要每次取最大（或最小）元素             │
// └─────────────────┴──────────────────────────────────────────┘

int main() {
    cout << "=========================================" << endl;
    cout << "   第 6 課：STL 容器概念與分類展示" << endl;
    cout << "=========================================" << endl;

    demoArray();
    demoVector();
    demoDeque();
    demoList();
    demoSet();
    demoMap();
    demoUnorderedMap();
    demoAdapters();

    cout << "\n=========================================" << endl;
    cout << " 選容器口訣：" << endl;
    cout << " 一般用 vector；有序唯一用 set/map；" << endl;
    cout << " 快速查找用 unordered；" << endl;
    cout << " LIFO 用 stack；FIFO 用 queue" << endl;
    cout << "=========================================" << endl;

    return 0;
}
