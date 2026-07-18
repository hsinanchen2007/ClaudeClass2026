// ============================================================
// 第 27 課 總結：deque 作為 stack/queue 底層容器
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【容器配接器 = Container Adapter】
//   stack 和 queue 不是容器，而是「配接器」
//   它們包裝一個真正的底層容器，限制介面來實現特定語義
//
// 【stack 堆疊（LIFO 後進先出）】
//   預設底層：deque<T>
//   push(val)  → 底層 push_back(val)
//   pop()      → 底層 pop_back()
//   top()      → 底層 back()
//   不提供迭代器、不提供 operator[]
//
// 【queue 佇列（FIFO 先進先出）】
//   預設底層：deque<T>
//   push(val)  → 底層 push_back(val)
//   pop()      → 底層 pop_front()   ← 這就是為什麼預設用 deque！
//   front()    → 底層 front()
//   back()     → 底層 back()
//
// 【為什麼 stack/queue 預設用 deque 而非 vector？】
//   queue 需要 pop_front() → vector 的 pop_front 是 O(n)
//   deque 的 push_front/pop_front 都是 O(1)
//   stack 用 deque 也比 vector 好：deque 擴容不需搬移全部元素
//
// 【可以換底層容器】
//   stack<int, vector<int>> s;  ← 用 vector 當底層
//   queue<int, list<int>> q;    ← 用 list 當底層
// ============================================================

#include <iostream>
#include <stack>
#include <queue>
#include <deque>
#include <vector>
#include <string>
using namespace std;

// ============================================================
// 自製 MyStack：展示配接器的本質
// ============================================================
template <typename T, typename Container = deque<T>>
class MyStack {
    Container c;  // 唯一的資料成員：底層容器
public:
    void push(const T& val) { c.push_back(val); }
    void pop()              { c.pop_back(); }
    T& top()                { return c.back(); }
    const T& top() const    { return c.back(); }
    bool empty() const      { return c.empty(); }
    size_t size() const     { return c.size(); }
    // 不提供 begin(), end(), operator[]
    // 這就是「限制介面」的意義
};

// ============================================================
// 括號匹配（stack 經典應用）
// ============================================================
bool isBalanced(const string& expr) {
    stack<char> s;  // 底層是 deque<char>
    for (char ch : expr) {
        if (ch == '(' || ch == '[' || ch == '{') {
            s.push(ch);
        } else if (ch == ')' || ch == ']' || ch == '}') {
            if (s.empty()) return false;
            char top = s.top();
            if ((ch == ')' && top == '(') ||
                (ch == ']' && top == '[') ||
                (ch == '}' && top == '{')) {
                s.pop();
            } else {
                return false;
            }
        }
    }
    return s.empty();
}

// ============================================================
// BFS 廣度優先搜索（queue 經典應用）
// ============================================================
void bfs(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    queue<int> q;  // 底層是 deque<int>
    visited[start] = true;
    q.push(start);

    cout << "  BFS 順序：";
    while (!q.empty()) {
        int node = q.front();
        q.pop();
        cout << node << " ";
        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                q.push(neighbor);
            }
        }
    }
    cout << endl;
}

int main() {
    // ============================================================
    // 1. stack 基本操作
    // ============================================================
    cout << "===== 1. stack（LIFO）=====\n";
    {
        stack<int> s;  // 預設底層是 deque<int>
        s.push(10);
        s.push(20);
        s.push(30);
        cout << "  top: " << s.top() << endl;     // 30
        cout << "  size: " << s.size() << endl;    // 3

        s.pop();
        cout << "  pop 後 top: " << s.top() << endl;  // 20

        cout << "  逐一取出：";
        while (!s.empty()) {
            cout << s.top() << " ";
            s.pop();
        }
        cout << endl;  // 20 10
    }
    cout << "\n";

    // ============================================================
    // 2. queue 基本操作
    // ============================================================
    cout << "===== 2. queue（FIFO）=====\n";
    {
        queue<int> q;  // 預設底層是 deque<int>
        q.push(10);
        q.push(20);
        q.push(30);
        cout << "  front: " << q.front() << endl;  // 10
        cout << "  back:  " << q.back() << endl;    // 30

        q.pop();
        cout << "  pop 後 front: " << q.front() << endl;  // 20

        cout << "  逐一取出：";
        while (!q.empty()) {
            cout << q.front() << " ";
            q.pop();
        }
        cout << endl;  // 20 30
    }
    cout << "\n";

    // ============================================================
    // 3. 自製配接器 & 換底層容器
    // ============================================================
    cout << "===== 3. 自製配接器 & 換底層 =====\n";
    {
        // 用預設底層 deque
        MyStack<int> s1;
        s1.push(10); s1.push(20); s1.push(30);
        cout << "  MyStack<int>（deque 底層）：";
        while (!s1.empty()) { cout << s1.top() << " "; s1.pop(); }
        cout << endl;  // 30 20 10

        // 用 vector 當底層
        MyStack<int, vector<int>> s2;
        s2.push(100); s2.push(200);
        cout << "  MyStack<int, vector>：top = " << s2.top() << endl;  // 200
    }
    cout << "\n";

    // ============================================================
    // 4. stack 應用：括號匹配
    // ============================================================
    cout << "===== 4. 括號匹配 =====\n";
    cout << "  (a+b)*[c-d]   → " << isBalanced("(a+b)*[c-d]") << endl;     // 1
    cout << "  {(a+b)*[c-d]} → " << isBalanced("{(a+b)*[c-d]}") << endl;   // 1
    cout << "  (a+b]         → " << isBalanced("(a+b]") << endl;            // 0
    cout << "  ((a+b)        → " << isBalanced("((a+b)") << endl;           // 0
    cout << "\n";

    // ============================================================
    // 5. queue 應用：BFS
    // ============================================================
    cout << "===== 5. BFS 圖遍歷 =====\n";
    //     0 --- 1 --- 3
    //     |     |
    //     2 --- 4
    vector<vector<int>> graph = {
        {1, 2}, {0, 3, 4}, {0, 4}, {1}, {1, 2}
    };
    bfs(graph, 0);  // BFS 順序：0 1 2 3 4

    return 0;
}
