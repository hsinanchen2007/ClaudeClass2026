// =============================================================================
//  第三課：STL 的六大組件概覽 9  —  容器配接器：stack / queue / priority_queue
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<stack>（stack）、<queue>（queue 與 priority_queue）
//   樣板宣告：
//     template <class T, class Container = std::deque<T>>  class stack;
//     template <class T, class Container = std::deque<T>>  class queue;
//     template <class T, class Container = std::vector<T>,
//               class Compare = std::less<typename Container::value_type>>
//     class priority_queue;
//   標準版本：三者皆 C++98；emplace 系列是 C++11；
//             C++23 新增 flat_set / flat_map 等更多配接器。
//   複雜度：
//     stack / queue        push / pop / top / front 全部 O(1) 攤銷
//     priority_queue       push O(log N)、pop O(log N)、top O(1)
//   關鍵性質：**三者都沒有迭代器**，不能 begin()/end()，不能用範圍 for，
//             也不能用任何 STL 演算法。這是刻意的設計，不是遺漏。
//
// 【詳細解釋 Explanation】
//
// 【1. 「配接器（adapter）」是什麼意思】
//   配接器不自己管理記憶體，而是**包住另一個容器並限制其介面**。
//   std::stack<int> 內部真的就有一個 std::deque<int> 成員：
//       template <class T, class Container = deque<T>>
//       class stack {
//       protected:
//           Container c;                       // 這就是全部的資料
//       public:
//           void push(const T& v) { c.push_back(v); }
//           void pop()            { c.pop_back(); }
//           T&   top()            { return c.back(); }
//           bool empty()    const { return c.empty(); }
//           size_t size()   const { return c.size(); }
//       };
//   它做的事情不是「增加功能」而是「**移除**功能」——
//   把 deque 的隨機存取、中間插入、迭代器統統藏起來，只留 LIFO 的五個操作。
//
// 【2. 為什麼要刻意移除功能】
//   因為型別本身就是文件與保證。當函式參數宣告成 std::stack<Task>& 時：
//     - 讀者立刻知道這裡的存取模式是後進先出
//     - 沒有人能「不小心」從中間插入或隨機存取而破壞演算法的正確性
//     - 日後想換底層容器（deque → vector）不必改任何呼叫端程式碼
//   這是「以介面表達意圖」的典型實踐。如果傳的是 std::deque<Task>&，
//   讀者得看完整份實作才知道你到底怎麼用它。
//
// 【3. 為什麼 pop() 不回傳值 —— 例外安全的經典設計】
//   直覺上 T pop() 比較好用，但它無法做到例外安全：
//     若 pop() 要「移除元素並回傳它的副本」，回傳時的複製建構若丟出例外，
//     元素已經被移除了 → 資料永久遺失，且容器狀態已改變，無法回復。
//   標準因此拆成兩步：top() 只讀（不改狀態）、pop() 只移除（回傳 void，不會丟）。
//   使用者自己決定先複製再彈出：
//       T value = s.top();   // 複製，若丟例外容器仍完好
//       s.pop();             // 移除，不會丟
//   代價是多打一行、且對大型物件多一次複製（可用 std::move(s.top()) 緩解）。
//
// 【4. 底層容器怎麼選】
//   stack  預設 deque。可換 vector（cache 較好，但擴充時要搬移）或 list。
//          要求：back() / push_back() / pop_back()
//   queue  預設 deque。**不能換 vector**（vector 沒有 pop_front()）。可換 list。
//          要求：front() / back() / push_back() / pop_front()
//   priority_queue 預設 **vector**（而非 deque），因為堆積演算法需要隨機存取，
//          且 vector 的連續記憶體對 sift-up / sift-down 的 cache 表現最好。
//          要求：Random Access Iterator + front() / push_back() / pop_back()
//
// 【概念補充 Concept Deep Dive】
//   priority_queue 底下並不是排序好的陣列，而是一個**二元堆積（binary heap）**：
//     - 用陣列隱式表示完全二元樹：節點 i 的子節點是 2i+1 與 2i+2
//     - 只保證「父節點優先度 ≥ 子節點」，兄弟之間毫無順序可言
//   所以：
//     - top() 是 O(1)（就是 arr[0]）
//     - push 是 O(log N)（放到尾端後 sift-up）
//     - pop 是 O(log N)（把尾端搬到頂端後 sift-down）
//     - 想「依序取出全部」必須 pop N 次 → 總共 O(N log N)，這就是 heapsort
//   若一開始就有全部資料，用範圍建構子（會呼叫 make_heap）是 **O(N)**，
//   比逐一 push 的 O(N log N) 更快 —— 這是很實用但常被忽略的最佳化。
//
// 【注意事項 Pay Attention】
//   1. 三者都**沒有迭代器**：不能範圍 for、不能 std::find、不能印出全部內容
//      而不破壞它。要「看」內容只能反覆 top()+pop()（會清空容器）。
//   2. 對空的 stack/queue 呼叫 top() / front() / pop() 是**未定義行為**，
//      不會丟例外。每次操作前務必先檢查 empty()。
//   3. pop() 回傳 void，不回傳被移除的元素（例外安全設計）。
//   4. priority_queue 預設是**大頂堆**；要小頂堆須寫完整三個樣板參數：
//        std::priority_queue<int, std::vector<int>, std::greater<int>>
//   5. queue 不能用 vector 當底層容器（缺 pop_front）；priority_queue 不能用 list
//      （缺隨機存取）。
//   6. priority_queue 沒有「更新某元素優先度」的操作。真的需要 decrease-key
//      （如 Dijkstra）時，常見作法是「懶惰刪除」：直接 push 新值，
//      pop 出來時再檢查是否為過期項目並跳過。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】容器配接器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::stack::pop() 不回傳被彈出的元素？
//     答：為了例外安全。若 pop() 要回傳副本，那次回傳的複製建構若丟出例外，
//         元素已經從容器移除、卻沒送到呼叫端 → 資料永久遺失且無法回復。
//         標準把它拆成 top()（唯讀、不改狀態）與 pop()（只移除、不會丟），
//         由使用者自行決定何時複製。
//     追問：C++11 有了移動語意之後，是不是就可以安全地回傳了？
//           → 仍不行。只有在移動建構子是 noexcept 時才安全，
//             但那是型別相依的，標準不能為所有 T 提供這個保證。
//             實務上可以自己寫 `T v = std::move(s.top()); s.pop();` 取得等效效果。
//
// 🔥 Q2. stack / queue / priority_queue 的預設底層容器各是什麼？為什麼不同？
//     答：stack 與 queue 預設 std::deque；priority_queue 預設 std::vector。
//         stack/queue 只在兩端進出，deque 兩端都是 O(1) 且擴充時不必搬移既有元素。
//         priority_queue 的堆積演算法需要**隨機存取**做 sift-up/sift-down，
//         vector 的連續記憶體對此最有利，所以選 vector。
//     追問：queue 可以換成 vector 嗎？
//           → 不行。queue 需要 pop_front()，而 vector 沒有這個成員函式，
//             編譯就會失敗。可以換成 std::list。
//
// ⚠️ 陷阱. 「priority_queue 內部就是一個排好序的陣列，所以我可以直接看第二大的元素」
//     答：錯。底層是二元堆積，只保證父節點優先度不低於子節點，
//         兄弟節點之間沒有任何順序關係。而且 priority_queue **根本沒有迭代器**
//         或 operator[]，你連「看第二個」的語法都沒有。
//         要拿第二大只能 pop 掉第一個再看 top()（會破壞容器），
//         或改用 std::multiset / 自己維護排序容器。
//     為什麼會錯：把「優先佇列」想像成「自動排序的清單」。
//         實際上堆積只維護了 O(log N) 就能達成的**部分**順序 ——
//         全排序要 O(N log N)，堆積刻意不做那麼多工作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stack>
#include <queue>
#include <vector>
#include <string>
#include <functional>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//   題目：給定只含 '(' ')' '{' '}' '[' ']' 的字串，判斷括號是否正確配對巢狀。
//   為什麼用到本主題：這是 stack 的標準應用，也是「為什麼需要 LIFO」最清楚的例子：
//         最近開啟的括號必須最先被關閉 —— 這正是後進先出的定義。
//         用 vector 也能做（push_back/back/pop_back），但宣告成 stack
//         等於在型別上宣告「我只會後進先出地使用它」。
//   複雜度：時間 O(N)、空間 O(N)（最壞情況全是左括號）。
// -----------------------------------------------------------------------------
bool isValidParentheses(const std::string& s) {
    std::stack<char> stk;
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            stk.push(c);
        } else {
            if (stk.empty()) return false;          // 有右括號卻無對應左括號
            char open = stk.top();
            stk.pop();
            if ((c == ')' && open != '(') ||
                (c == ']' && open != '[') ||
                (c == '}' && open != '{')) {
                return false;                        // 類型不匹配
            }
        }
    }
    return stk.empty();                              // 還有沒關閉的左括號則失敗
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】客服工單排程（priority_queue：嚴重度優先）
//   情境：客服系統同時湧入大量工單，必須優先處理嚴重度高的；
//         同嚴重度時先進先出（用遞增的流水號當第二鍵，較小者優先）。
//   為什麼用到本主題：這正是 priority_queue 的定義用途 ——
//         只需要「每次取出目前最該處理的那一張」，不需要全排序，
//         push/pop 各 O(log N) 就夠，而且新工單可以隨時插入。
// -----------------------------------------------------------------------------
struct Ticket {
    int         severity;   // 數字越大越嚴重
    int         seq;        // 流水號：同嚴重度時越小越先進
    std::string title;
};

// 比較器語意：回傳 true 代表 a 的「優先度較低」（會沉到堆積底部）
struct TicketLower {
    bool operator()(const Ticket& a, const Ticket& b) const {
        if (a.severity != b.severity) return a.severity < b.severity;  // 嚴重度低者優先度低
        return a.seq > b.seq;                                          // 流水號大者優先度低
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】列印工作佇列（queue：先到先服務）
//   情境：辦公室共用印表機，先送出的文件先印，中途不能插隊。
//   為什麼用到本主題：FIFO 就是 queue 的定義；用 queue 宣告等於保證
//         沒有任何一行程式碼能讓某份文件插隊（型別上就辦不到）。
// -----------------------------------------------------------------------------
struct PrintJob {
    std::string owner;
    int         pages;
};

int main() {
    // stack：後進先出（LIFO）, 使用 push() 添加元素，top() 讀取頂部元素，pop() 移除頂部元素
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

    // queue：先進先出（FIFO）, 使用 push() 添加元素，front() 讀取隊首元素，pop() 移除隊首元素
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

    // priority_queue：優先佇列（預設最大值優先）, 使用 push() 添加元素，top() 讀取優先級最高的元素，pop() 移除優先級最高的元素
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

    // 大頂堆 vs 小頂堆
    std::cout << "\n=== 大頂堆 vs 小頂堆 ===" << std::endl;
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
    for (int n : {30, 10, 50, 20}) min_pq.push(n);
    std::cout << "greater<int> 小頂堆依序取出: ";
    while (!min_pq.empty()) { std::cout << min_pq.top() << " "; min_pq.pop(); }
    std::cout << std::endl;

    // 一次建堆是 O(N)，比逐一 push 的 O(N log N) 快
    std::cout << "\n=== 建堆方式 ===" << std::endl;
    std::vector<int> data = {30, 10, 50, 20, 40};
    std::priority_queue<int> bulk(data.begin(), data.end());   // 內部呼叫 make_heap，O(N)
    std::cout << "範圍建構（make_heap, O(N)）的 top() = " << bulk.top() << std::endl;
    std::cout << "（逐一 push 是 O(N log N)；資料一開始就齊全時請用範圍建構）" << std::endl;

    // 配接器沒有迭代器
    std::cout << "\n=== 配接器沒有迭代器 ===" << std::endl;
    std::cout << "  stack/queue/priority_queue 都不能 begin()/end()，" << std::endl;
    std::cout << "  不能範圍 for，也不能用 std::find —— 這是刻意的介面限制。" << std::endl;

    std::cout << "\n=== LeetCode 20. Valid Parentheses ===" << std::endl;
    // 注意：這裡用 const char* 而非 const std::string&。
    // 寫成 const std::string& 會讓每個元素都從 const char* 建構一個暫時物件再綁定，
    // g++ 的 -Wrange-loop-construct 會警告（雖然生命週期延長使它仍然安全）。
    for (const char* t : {"()", "()[]{}", "(]", "([)]", "{[]}", "("}) {
        std::cout << "  \"" << t << "\" → "
                  << (isValidParentheses(t) ? "true" : "false") << std::endl;
    }

    std::cout << "\n=== 日常實務 1：客服工單排程（嚴重度優先）===" << std::endl;
    std::priority_queue<Ticket, std::vector<Ticket>, TicketLower> tickets;
    tickets.push({1, 1, "帳單顯示格式跑掉"});
    tickets.push({3, 2, "全站無法登入"});
    tickets.push({2, 3, "付款頁面偶爾逾時"});
    tickets.push({3, 4, "資料庫連線中斷"});
    tickets.push({1, 5, "頭像上傳失敗"});

    int order = 1;
    while (!tickets.empty()) {
        const Ticket& t = tickets.top();
        std::cout << "  " << order++ << ". [嚴重度 " << t.severity
                  << " / 單號 " << t.seq << "] " << t.title << std::endl;
        tickets.pop();
    }

    std::cout << "\n=== 日常實務 2：列印工作佇列（先到先服務）===" << std::endl;
    std::queue<PrintJob> printer;
    printer.push({"alice", 12});
    printer.push({"bob",    3});
    printer.push({"carol",  47});

    int total_pages = 0;
    while (!printer.empty()) {
        const PrintJob& j = printer.front();
        total_pages += j.pages;
        std::cout << "  列印 " << j.owner << " 的文件（" << j.pages << " 頁）" << std::endl;
        printer.pop();
    }
    std::cout << "  共列印 " << total_pages << " 頁" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽9.cpp -o demo9

// === 預期輸出 ===
// stack (LIFO): 3 2 1
// queue (FIFO): 1 2 3
// priority_queue: 50 30 20 10
//
// === 大頂堆 vs 小頂堆 ===
// greater<int> 小頂堆依序取出: 10 20 30 50
//
// === 建堆方式 ===
// 範圍建構（make_heap, O(N)）的 top() = 50
// （逐一 push 是 O(N log N)；資料一開始就齊全時請用範圍建構）
//
// === 配接器沒有迭代器 ===
//   stack/queue/priority_queue 都不能 begin()/end()，
//   不能範圍 for，也不能用 std::find —— 這是刻意的介面限制。
//
// === LeetCode 20. Valid Parentheses ===
//   "()" → true
//   "()[]{}" → true
//   "(]" → false
//   "([)]" → false
//   "{[]}" → true
//   "(" → false
//
// === 日常實務 1：客服工單排程（嚴重度優先）===
//   1. [嚴重度 3 / 單號 2] 全站無法登入
//   2. [嚴重度 3 / 單號 4] 資料庫連線中斷
//   3. [嚴重度 2 / 單號 3] 付款頁面偶爾逾時
//   4. [嚴重度 1 / 單號 1] 帳單顯示格式跑掉
//   5. [嚴重度 1 / 單號 5] 頭像上傳失敗
//
// === 日常實務 2：列印工作佇列（先到先服務）===
//   列印 alice 的文件（12 頁）
//   列印 bob 的文件（3 頁）
//   列印 carol 的文件（47 頁）
//   共列印 62 頁
