// ============================================================
// 第 27 課 總結：deque 作為 stack/queue 底層容器
// 編譯：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
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
//
// 【主題資訊 Information】
//   template <class T, class Container = std::deque<T>> class stack;
//   template <class T, class Container = std::deque<T>> class queue;
//   template <class T, class Container = std::vector<T>,
//             class Compare = std::less<typename Container::value_type>>
//   class priority_queue;                       // ★ 預設底層是 vector！
//
//   標頭檔：<stack>、<queue>
//   複雜度：stack/queue 的所有操作都是 O(1)（攤銷），成本完全由底層容器決定
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 配接器的本質：一個成員 + 一堆轉呼叫】
//   std::stack 與 std::queue 內部都只有一個資料成員：
//       protected: Container c;
//   它們自己不管理任何記憶體、不含任何演算法，全部委託出去：
//       stack: push→c.push_back  pop→c.pop_back   top→c.back
//       queue: push→c.push_back  pop→c.pop_front  front→c.front  back→c.back
//   兩者唯一的實質差別就是 **pop 委託給 pop_back 還是 pop_front**，
//   而這一個字的差別，決定了 LIFO 與 FIFO、也決定了誰能當底層容器。
//
// 【2. 配接器的價值在於它「拿掉」了什麼】
//   deque 本來就能當堆疊與佇列用，為什麼還要包一層？
//   因為配接器**刻意不提供** begin()/end()、operator[]、insert/erase。
//   當程式碼裡出現 stack<int>，讀者立刻知道這裡只有 LIFO 行為，
//   不必讀完整段程式確認有沒有人從中間插入。
//   這是「用型別表達意圖」，而且是編譯器強制執行的——
//   比註解或口頭約定可靠得多。
//
// 【3. 三個配接器的預設底層，以及為什麼】
//       stack          → deque   （只需要 push_back/pop_back/back）
//       queue          → deque   （需要 pop_front，vector 沒有）
//       priority_queue → vector  （heap 演算法需要 random access）
//   最常被搞混的是第三個。priority_queue 用 std::push_heap / pop_heap
//   維護二元堆積，那些演算法需要 random access iterator，
//   而且 vector 的連續記憶體讓父子索引計算（2i+1、2i+2）最有效率。
//
// 【4. 為什麼 queue 不能用 vector】
//   queue::pop() 委託給底層的 pop_front()，而 **std::vector 根本沒有這個成員**。
//   同課 2.cpp 用 detection idiom 在編譯期驗證過這件事。
//   微妙之處：只「宣告」queue<int, vector<int>> 通常能編譯通過，
//   因為模板成員函式是**延遲實例化**的——直到你呼叫 pop() 才會報錯
//   （實測錯誤訊息：'class std::vector<int>' has no member named 'pop_front'）。
//
// 【概念補充 Concept Deep Dive】
//   ● pop() 為什麼回傳 void
//     例外安全。若 pop 同時回傳值，就必須複製建構一個 T；
//     萬一該複製建構子丟出例外，元素已被移除、值卻沒交到呼叫端手上，
//     資料就永久遺失且無法復原。
//     拆成 top()/front()（看）+ pop()（移除）兩步，任一步失敗容器都還完整。
//     其他語言（Python、Java）沒有這個問題，是因為它們有 GC 與參考語意，
//     「回傳」不需要複製物件。
//
//   ● 底層容器的最低需求
//     stack：back、push_back、pop_back、empty、size  → vector/deque/list 皆可
//     queue：front、back、push_back、pop_front、empty、size → 只有 deque/list
//     priority_queue：front、push_back、pop_back + random access iterator
//                     → vector/deque 可，list 不行
//
//   ● 資料結構的選擇就是演算法的選擇
//     同課 5.cpp 示範過：一份圖走訪程式碼，把 queue 換成 stack，
//     BFS 立刻變成 DFS，走訪順序完全不同（實測 0 1 2 3 4 vs 0 2 4 1 3）。
//     容器不只是「裝東西的盒子」，它編碼了「下一個要處理誰」這個決策。
//
// 【注意事項 Pay Attention】
//   1. 對空的 stack/queue 呼叫 top()/front()/back()/pop() 是 **UB**，
//      不保證崩潰、不保證任何特定值。務必先 empty() 檢查。
//   2. pop() 回傳 void；要取值請先 top()/front()。
//   3. queue **不能**用 vector 當底層（缺 pop_front）。
//   4. priority_queue 的預設底層是 **vector**，不是 deque。
//   5. 配接器沒有虛擬解構子，不要當多型基底類別用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器配接器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是「容器配接器」？stack 和 queue 的實作差在哪一行？
//     答：配接器不是容器，它內部只有一個底層容器成員，把介面轉呼叫出去，
//         自己不管理記憶體、不含演算法。
//         stack 與 queue 的實質差別只有 pop()：
//         stack 委託給 pop_back（LIFO），queue 委託給 pop_front（FIFO）。
//         就這一個字的差別，決定了兩者的語意與可用的底層容器。
//     追問：那為什麼不直接用 deque？→ 為了「限制介面」。
//         配接器刻意拿掉 begin/end/operator[]，讓型別本身就宣告了存取模式，
//         而且由編譯器強制執行。
//
// 🔥 Q2. stack、queue、priority_queue 的預設底層各是什麼？為什麼不一樣？
//     答：stack → deque、queue → deque、**priority_queue → vector**。
//         stack/queue 選 deque 是因為 queue 需要 O(1) 的 pop_front，
//         而且 deque 擴容不必搬移既有元素。
//         priority_queue 選 vector 是因為 heap 演算法（push_heap/pop_heap）
//         需要 random access iterator，且連續記憶體讓 2i+1/2i+2 的索引計算最快。
//     追問：stack 可以用 vector 嗎？→ 可以，stack<int, vector<int>> 完全合法，
//         而且若堆疊大小可預期、事先 reserve，往往比 deque 更快。
//
// ⚠️ 陷阱. 「配接器包一層會不會有效能損失？是不是直接用 deque 比較快？」
//     答：**沒有損失**。配接器的所有成員函式都是一行的 inline 轉呼叫，
//         編譯器會完全展開，產生的機器碼與直接呼叫底層容器相同。
//         你付出的是零執行期成本，換到的是編譯期強制的介面限制。
//     為什麼會錯：把「多一層抽象」直覺地等同於「多一層執行期成本」。
//         在 C++ 裡，**抽象發生在編譯期**——模板與 inline 讓
//         「零成本抽象（zero-overhead abstraction）」成為可能。
//         這正是 C++ 與有虛擬機的語言在設計哲學上的根本差異：
//         你不會為你沒用到的東西付費，也不會為編譯期就能解決的抽象付費。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stack>
#include <queue>
#include <deque>
#include <vector>
#include <list>
#include <string>
#include <type_traits>
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
// detection idiom：編譯期詢問「型別 C 有沒有 pop_front()」
// 這正是 queue 對底層容器的關鍵要求
// ============================================================
template <typename C, typename = void>
struct HasPopFront : false_type {};

template <typename C>
struct HasPopFront<C, void_t<decltype(declval<C&>().pop_front())>> : true_type {};

// ============================================================
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計支援 push / pop / top，且能在 O(1) 取得最小值的堆疊。
//   為什麼用到本主題：這題就是在考「設計配接器」——
//     用既有容器組出一個介面受限、但多了一項保證的資料結構。
//     兩個 stack 同進同出，getMin 只要看輔助堆疊的 top()。
// ============================================================
class MinStack {
    stack<int> data_;   // 主堆疊（底層 deque）
    stack<int> mins_;   // 輔助堆疊：top() 永遠是目前最小值
public:
    void push(int val) {
        data_.push(val);
        mins_.push(mins_.empty() ? val : min(val, mins_.top()));
    }
    void pop() {
        if (data_.empty()) return;      // 先檢查，對空 stack 呼叫 pop() 是 UB
        data_.pop();
        mins_.pop();
    }
    int top() const    { return data_.top(); }
    int getMin() const { return mins_.top(); }
};

// ============================================================
// 【日常實務範例】任務排程器：queue 排隊、stack 復原
//   情境：部署腳本依序執行多個步驟。正常時照提交順序執行（FIFO）；
//         一旦中途失敗，必須**依相反順序**復原已完成的步驟（LIFO）。
//   這個系統同時需要兩種語意，剛好各用一個配接器表達，
//   而且型別本身就說明了「這裡是排隊」「那裡是回滾堆疊」。
// ============================================================
class TaskScheduler {
    queue<string> pending_;   // 待執行：FIFO
    stack<string> done_;      // 已完成：LIFO（回滾用）
public:
    void submit(const string& task) { pending_.push(task); }

    bool runNext() {
        if (pending_.empty()) return false;
        string t = pending_.front();     // 先取值
        pending_.pop();                  // 再移除（兩步式，例外安全）
        cout << "    執行：" << t << "\n";
        done_.push(t);
        return true;
    }

    bool rollbackLast() {
        if (done_.empty()) return false;
        cout << "    復原：" << done_.top() << "\n";
        done_.pop();
        return true;
    }

    size_t pending() const { return pending_.size(); }
    size_t doneCount() const { return done_.size(); }
};

// ============================================================
// 對照組：把 queue 換成 stack，BFS 立刻變成 DFS
// ============================================================
void dfsWithStack(const vector<vector<int>>& graph, int start) {
    vector<bool> visited(graph.size(), false);
    stack<int> s;                    // 與 bfs() 唯一的差別就是這一行
    visited[start] = true;
    s.push(start);

    cout << "  DFS 順序：";
    while (!s.empty()) {
        int node = s.top();
        s.pop();
        cout << node << " ";
        for (int neighbor : graph[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                s.push(neighbor);
            }
        }
    }
    cout << endl;
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
    cout << "\n";

    // ============================================================
    // 6. 三個配接器的預設底層（編譯期驗證，不是背的）
    // ============================================================
    cout << "===== 6. 三個配接器的預設底層 =====\n";
    cout << boolalpha;
    cout << "  stack<int>          底層是 deque<int>?  "
         << is_same_v<stack<int>::container_type, deque<int>> << "\n";
    cout << "  queue<int>          底層是 deque<int>?  "
         << is_same_v<queue<int>::container_type, deque<int>> << "\n";
    cout << "  priority_queue<int> 底層是 vector<int>? "
         << is_same_v<priority_queue<int>::container_type, vector<int>> << "\n";
    cout << "  → priority_queue 用 vector，因為 heap 演算法需要 random access\n\n";

    // ============================================================
    // 7. 為什麼 queue 不能用 vector（編譯期偵測）
    // ============================================================
    cout << "===== 7. queue 的底層必須有 pop_front =====\n";
    cout << "  vector<int> 有 pop_front? " << HasPopFront<vector<int>>::value
         << "  ← 所以 queue<int,vector<int>> 呼叫 pop() 會編譯失敗\n";
    cout << "  deque<int>  有 pop_front? " << HasPopFront<deque<int>>::value << "\n";
    cout << "  list<int>   有 pop_front? " << HasPopFront<list<int>>::value << "\n";
    static_assert(!HasPopFront<vector<int>>::value, "vector 不應有 pop_front");
    static_assert(HasPopFront<deque<int>>::value,  "deque 應有 pop_front");
    cout << "  ⚠️ 只「宣告」queue<int,vector<int>> 通常能編譯通過——\n";
    cout << "     模板成員函式是延遲實例化的，呼叫 pop() 時才報錯。\n\n";

    // ============================================================
    // 8. 資料結構的選擇 = 演算法的選擇
    // ============================================================
    cout << "===== 8. 只換容器，BFS 就變成 DFS =====\n";
    bfs(graph, 0);
    dfsWithStack(graph, 0);
    cout << "  → 兩者程式碼的唯一差別是 queue<int> vs stack<int>。\n";
    cout << "    FIFO 按距離分層推進（BFS）；LIFO 一路往深處走（DFS）。\n\n";

    // ============================================================
    // LeetCode 155. Min Stack
    // ============================================================
    cout << "===== LeetCode 155. Min Stack =====\n";
    MinStack ms;
    ms.push(-2);  ms.push(0);  ms.push(-3);
    cout << "  push(-2), push(0), push(-3) → getMin() = " << ms.getMin() << "\n";
    ms.pop();
    cout << "  pop() → top() = " << ms.top()
         << ", getMin() = " << ms.getMin() << "\n";
    cout << "  → 用一個輔助 stack 同步 push/pop，getMin 就是 O(1)；\n";
    cout << "    這題本身就是在考「設計一個介面受限但多一項保證的配接器」。\n\n";

    // ============================================================
    // 日常實務：任務排程器（queue 排隊 + stack 復原）
    // ============================================================
    cout << "===== 日常實務：任務排程器 =====\n";
    TaskScheduler sched;
    sched.submit("下載設定檔");
    sched.submit("驗證簽章");
    sched.submit("套用設定");
    sched.submit("重啟服務");
    cout << "  待執行 " << sched.pending() << " 個任務\n";

    sched.runNext();      // 下載設定檔
    sched.runNext();      // 驗證簽章
    sched.runNext();      // 套用設定
    cout << "  已完成 " << sched.doneCount()
         << " 個，剩餘 " << sched.pending() << " 個\n";

    cout << "  發生錯誤，開始回滾：\n";
    while (sched.rollbackLast()) { /* 依相反順序復原 */ }
    cout << "  → 排隊用 queue（FIFO，先submit先執行），\n";
    cout << "    復原用 stack（LIFO，後執行的先回滾）——\n";
    cout << "    同一個系統裡兩種語意並存，各自用對的配接器表達。\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 1. stack（LIFO）=====
//   top: 30
//   size: 3
//   pop 後 top: 20
//   逐一取出：20 10
//
// ===== 2. queue（FIFO）=====
//   front: 10
//   back:  30
//   pop 後 front: 20
//   逐一取出：20 30
//
// ===== 3. 自製配接器 & 換底層 =====
//   MyStack<int>（deque 底層）：30 20 10
//   MyStack<int, vector>：top = 200
//
// ===== 4. 括號匹配 =====
//   (a+b)*[c-d]   → 1
//   {(a+b)*[c-d]} → 1
//   (a+b]         → 0
//   ((a+b)        → 0
//
// ===== 5. BFS 圖遍歷 =====
//   BFS 順序：0 1 2 3 4
//
// ===== 6. 三個配接器的預設底層 =====
//   stack<int>          底層是 deque<int>?  true
//   queue<int>          底層是 deque<int>?  true
//   priority_queue<int> 底層是 vector<int>? true
//   → priority_queue 用 vector，因為 heap 演算法需要 random access
//
// ===== 7. queue 的底層必須有 pop_front =====
//   vector<int> 有 pop_front? false  ← 所以 queue<int,vector<int>> 呼叫 pop() 會編譯失敗
//   deque<int>  有 pop_front? true
//   list<int>   有 pop_front? true
//   ⚠️ 只「宣告」queue<int,vector<int>> 通常能編譯通過——
//      模板成員函式是延遲實例化的，呼叫 pop() 時才報錯。
//
// ===== 8. 只換容器，BFS 就變成 DFS =====
//   BFS 順序：0 1 2 3 4
//   DFS 順序：0 2 4 1 3
//   → 兩者程式碼的唯一差別是 queue<int> vs stack<int>。
//     FIFO 按距離分層推進（BFS）；LIFO 一路往深處走（DFS）。
//
// ===== LeetCode 155. Min Stack =====
//   push(-2), push(0), push(-3) → getMin() = -3
//   pop() → top() = 0, getMin() = -2
//   → 用一個輔助 stack 同步 push/pop，getMin 就是 O(1)；
//     這題本身就是在考「設計一個介面受限但多一項保證的配接器」。
//
// ===== 日常實務：任務排程器 =====
//   待執行 4 個任務
//     執行：下載設定檔
//     執行：驗證簽章
//     執行：套用設定
//   已完成 3 個，剩餘 1 個
//   發生錯誤，開始回滾：
//     復原：套用設定
//     復原：驗證簽章
//     復原：下載設定檔
//   → 排隊用 queue（FIFO，先submit先執行），
//     復原用 stack（LIFO，後執行的先回滾）——
//     同一個系統裡兩種語意並存，各自用對的配接器表達。
