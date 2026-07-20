// =============================================================================
//  第六課 13 — std::queue：先進先出(FIFO)的容器配接器
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, class Container = std::deque<T>> class queue;  // <queue>,C++98 起
//
//   完整介面(和 stack 一樣少,少是刻意的):
//     bool            empty() const;            // O(1)
//     size_type       size()  const;            // O(1)
//     reference       front();                  // O(1),最早進來的那個;空的時候是 UB
//     reference       back();                   // O(1),最晚進來的那個;空的時候是 UB
//     void            push(const T&);           // O(1),從**尾端**加入
//     void            push(T&&);                // O(1),C++11
//     template<class... Args> void emplace(Args&&...);  // C++11(本機 -pedantic-errors 實測)
//     void            pop();                    // O(1),從**前端**移除;回傳 void;空的時候是 UB
//     void            swap(queue&);             // O(1),C++11(本機 -pedantic-errors 實測)
//
//   標準版本：queue 本身 C++98;emplace / push(T&&) / swap 是 C++11;
//             CTAD(std::queue q(deq) 免寫模板參數)是 C++17
//             —— 以上皆以 g++ -pedantic-errors 在本機逐一編譯驗證。
//   標頭檔：#include <queue>
//
// 【詳細解釋 Explanation】
//
// 【1. queue 同樣是配接器:它不存資料,只轉發並「刪掉」介面】
// 和 std::stack 一樣,std::queue **不是容器**,而是容器配接器(container adapter)。
// 它內部持有一個真正的容器(預設 std::deque<T>),把每個操作原封不動轉發過去:
//
//     que.push(x)   →  c.push_back(x)      // 從尾端進
//     que.front()   →  c.front()           // 看最早進來的
//     que.back()    →  c.back()            // 看最晚進來的
//     que.pop()     →  c.pop_front()       // 從前端出
//     que.size()    →  c.size()
//
// 注意這組對應:**進出在不同端**。這是 queue 和 stack 唯一的本質差別 ——
// stack 是 push_back/pop_back(同一端進出,LIFO),queue 是 push_back/pop_front
// (兩端分工,FIFO)。整個配接器的價值同樣在於「減法」:deque 本來能隨機索引、
// 能從前端插入、能被 std::sort 排序,包成 queue 之後這些全部消失,
// 只剩下佇列該有的那幾個動作。
//
// 【2. 為什麼預設底層是 deque?這次不是「比較好」,而是「非它不可」】
// 這題比 stack 那題更值得想清楚,因為兩者的答案性質完全不同。
//
// stack 只需要在單一端進出,所以 vector、deque、list **三者都能當底層**,
// 選 deque 只是因為它成長時不必搬動既有元素、比較平順 —— 那是效能取捨。
//
// queue 不一樣。它要求底層同時提供:
//     push_back()  +  pop_front()  +  front()  +  back()
// 而 **std::vector 根本沒有 pop_front()**。原因是 vector 的元素連續排列,
// 移除第一個元素必須把後面 n-1 個全部往前搬,是 O(n) —— 標準庫不願意提供
// 一個看起來 O(1) 實際 O(n) 的介面,所以乾脆不提供。
//
// 結論:
//     std::stack<int, std::vector<int>>   // 合法,而且常常更快
//     std::queue<int, std::vector<int>>   // **編譯失敗**,vector 沒有 pop_front
//     std::queue<int, std::list<int>>     // 合法,list 兩端都是 O(1)
//
// 這是型別系統在替你把關的好例子:錯誤的底層選擇不會變成執行期的效能地雷,
// 而是當場編譯不過。
//
// 【3. deque 為什麼兩端都能 O(1)?】
// deque(double-ended queue)不是一整塊連續記憶體,而是「一張指標表 + 多個
// 固定大小的資料區塊(chunk)」。前端與尾端各自記著自己的位置:
//
//     map(指標表):  [ ptr0 ][ ptr1 ][ ptr2 ][ ptr3 ]
//                       ↓       ↓       ↓       ↓
//     chunk:        [....xx][xxxxxx][xxxxxx][xx....]
//                       ↑                      ↑
//                     front                   back
//
// 從前端移除只是把 front 指標往右移一格(必要時釋放整個空掉的 chunk);
// 從尾端加入就是往右寫,寫滿了再配一個新 chunk 並把指標登記進表裡。
// 兩邊都不必搬動既有元素,所以都是 O(1)。這正是 FIFO 需要的形狀 ——
// 標準把 deque 選為 queue 的預設底層,是因為 deque 天生就是為這件事設計的。
//
// 【4. front() 與 back():最容易講反的兩個字】
// 這兩個名字取自底層容器,不是取自「佇列語意」,所以很多人第一次會弄反:
//     front() = 隊伍的**最前面** = 最早進來的 = **下一個會被 pop 掉的**
//     back()  = 隊伍的**最後面** = 最晚進來的 = 剛剛 push 進去的那個
//
// 記憶方式:想像排隊買票。front 是正在被服務的人,back 是剛走到隊尾的人。
// 實務上 99% 的程式只用 front()(取下一個待處理項目);back() 主要用於
// 「我剛丟進去的東西是什麼」這種驗證,或需要檢視最新項目的少數場合。
//
// 【5. pop() 為什麼回傳 void?—— 例外安全,和 stack 是同一個理由】
// 每個人第一次用都會想寫 int x = que.pop();。做不到,必須拆兩行:
//
//     int x = que.front();   // 先讀
//     que.pop();             // 再移除
//
// 理由是**強例外安全保證(strong exception safety)**。若 pop() 要回傳值,
// 實作必然長成:
//     T pop() { T tmp = c.front(); c.pop_front(); return tmp; }   // 假想
// 回傳時要複製/搬移建構一份給呼叫端。如果 T 的複製建構子**在這一刻丟出例外**,
// 元素已經從容器裡移除了,而值又還沒送到呼叫端手上 —— 這筆資料就永久消失,
// 而且無法復原(容器狀態已改變,回捲不了)。
//
// 拆成 front() + pop() 就沒這個問題:front() 只回傳 reference、不複製、不丟例外;
// 複製發生在使用者自己的指派式裡,失敗了元素還好端端待在佇列中,可以重試。
// 這是 C++ 標準庫「寧可介面囉嗦,也不要讓資料可能遺失」的一貫取捨。
//
// 【6. 沒有 iterator —— 一樣是刻意的】
// std::queue 沒有 begin()/end(),因此:
//   (a) 不能用 range-based for 遍歷;
//   (b) 不能餵給任何 STL 演算法(std::find、std::count、std::accumulate 全不行);
//   (c) 想看看佇列裡有什麼,只能複製一份出來一邊 front() 一邊 pop() 拆掉它。
//
// 理由和 stack 相同:開放遍歷就等於開放「不照 FIFO 順序存取」,那條約束就破了。
// 如果你發現自己很想遍歷這個 queue,那代表你要的其實不是佇列,
// 應該直接用 deque 或 list。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:配接器是真正的零成本包裝
//     libstdc++ 的 queue 簡化後就是:
//         template<class T, class Container = deque<T>>
//         class queue {
//         protected:
//             Container c;                            // 唯一的資料成員
//         public:
//             void push(const T& x) { c.push_back(x); }
//             void pop()            { c.pop_front(); }
//             T&   front()          { return c.front(); }
//             T&   back()           { return c.back(); }
//         };
//     只有一個成員。本機實測(GCC 15.2.0 / x86-64,**實作定義**):
//         sizeof(std::queue<int>) == 80 == sizeof(std::deque<int>)
//     完全相等 —— 配接器本身不佔任何額外空間。那些一行的轉發函式在 -O2 下
//     會被完全 inline,機器碼與直接呼叫 c.push_back(x) 沒有差別。
//     所以「用 queue 比較慢」是錯的:執行期成本為零,價值全部兌現在編譯期。
//
// (B) 手寫 FIFO 的傳統做法:circular buffer
//     C 語言時代要做佇列,標準做法是「固定陣列 + head/tail 兩個索引 + 取模」:
//         buf[tail++ % N] = x;    // 入列
//         x = buf[head++ % N];    // 出列
//     優點是完全不配置記憶體、cache 極友善;缺點是容量固定,滿了要嘛丟資料、
//     要嘛整個重配。std::queue over deque 等於是「容量無上限的 circular buffer」:
//     用多配一個 chunk 換取不必煩惱容量。若你的場景容量真的固定且極度在意延遲
//     (嵌入式、audio callback、lock-free ring),手寫 circular buffer 仍然更合適。
//
// (C) 沒有 clear()
//     queue 沒有 clear()。要清空只有兩招:
//         while (!q.empty()) q.pop();          // 逐一彈出,O(n)
//         q = std::queue<int>();                // 整個換掉,通常更快也更清楚
//     第二招利用 move assignment 把舊的整個丟棄,是慣用寫法。
//
// (D) 標準只規定「預設是 deque」,不規定 deque 怎麼實作
//     std::queue<T> 的預設底層由標準明文規定為 std::deque<T>,這點不是實作定義。
//     但 deque 內部的 chunk 大小(libstdc++ 對小型別是 512 bytes)、
//     sizeof(std::deque<int>)==80 這類數值,則完全屬於**實作定義**,
//     換一套標準庫(libc++、MSVC STL)就會不同,不可寫死在程式邏輯裡。
//
// 【注意事項 Pay Attention】
//  1. 對空的 queue 呼叫 front()、back() 或 pop() 是 **undefined behavior**。
//     配接器**不做任何檢查**,直接轉發給底層 deque 的 front()/pop_front(),
//     而那些同樣不檢查。行為不保證、不可預測:可能讀到殘留舊值、可能讓
//     size() 變成極大的錯誤數值、可能當場崩潰,也可能看起來正常跑完。
//     絕不能拿「測試時沒事」當作程式正確的證據 —— 正確寫法永遠是先
//     if (!q.empty())。(本檔的可執行程式碼因此不會對空 queue 做任何存取。)
//  2. front() 和 back() 別講反:front 是最早進來、下一個要出去的;
//     back 是最晚進來的。pop() 移除的永遠是 front 那一端。
//  3. **std::vector 不能當 queue 的底層**(沒有 pop_front),這是編譯期錯誤。
//     想換底層只有 std::deque(預設)和 std::list 兩個實務選項。
//  4. 沒有 iterator → 不能 range-based for、不能用 STL 演算法、也沒有 clear()。
//  5. sizeof(std::queue<int>) == 80 是本機實測值,屬**實作定義**,
//     不同標準庫/平台會不同。
//  6. **std::queue 不是 thread-safe**。這點特別重要,因為佇列最常見的用途
//     正是 producer-consumer(生產者-消費者)。多執行緒下必須自己加
//     std::mutex 保護,而且「先 empty() 再 front()」這種兩段式檢查在並行環境
//     會有 race condition —— 檢查完到取用之間,別的執行緒可能已經把它 pop 掉了。
//     整組操作必須放在同一個鎖的臨界區內。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::queue
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::queue 的預設底層容器是什麼?為什麼不能用 std::vector?
//     答：預設是 **std::deque<T>**(標準明文規定)。不能用 vector 的原因是
//         queue 需要底層同時提供 push_back() 與 **pop_front()**,而 vector
//         **沒有 pop_front()** —— 因為 vector 元素連續排列,移除第一個要把
//         後面全部往前搬,是 O(n),標準庫不願意提供假的 O(1) 介面。
//         所以 std::queue<int, std::vector<int>> 會**編譯失敗**。
//         deque 由多個 chunk 組成,前後兩端各自記位置,兩端進出都是 O(1),
//         天生就是 FIFO 要的形狀。
//     追問：那 std::stack 為什麼可以用 vector?
//         → 因為 stack 只在單一端進出,只需要 push_back/pop_back/back,
//           vector 全都有。stack 選 deque 當預設是效能取捨(成長時不搬元素),
//           queue 選 deque 則是介面上的硬需求 —— 兩者性質完全不同。
//
// 🔥 Q2. 為什麼 queue::pop() 回傳 void,而不是回傳被取出的那個元素?
//     答：為了**強例外安全保證**。若 pop() 要回傳值,就得在移除元素之後、
//         把值送回呼叫端之前做一次複製/搬移建構;若 T 的複製建構子在這一刻
//         丟出例外,元素已經從容器移除、值又沒交到呼叫端手上,資料就永久遺失
//         且無法復原。拆成 front()(只回傳 reference,不複製、不丟例外)
//         + pop()(只移除)之後,複製發生在使用者自己的指派式,失敗時元素
//         仍在佇列裡,可以安全重試。
//     追問：那 C++11 有 move semantics 了,現在可以加回傳值的 pop 嗎?
//         → 仍然不行。move 建構子雖然**通常** noexcept,但標準不保證所有型別
//           都如此;而且這組介面已穩定二十多年,改動會破壞相容性。
//           需要這個語意就自己包一個 helper 函式。
//
// ⚠️ 陷阱. BFS 走訪二元樹時,想「一層一層」分開輸出,直接 while(!q.empty())
//         逐個 pop 就好了吧?
//     答：不行,那樣所有節點會混成一串,分不出層。關鍵技巧是在每層開始前
//         先把 **size_t levelSize = q.size();** 存下來,然後只 pop 這麼多次
//         —— 這個數字就是「本層的節點數」。迴圈中新 push 進去的是下一層的
//         節點,因為 levelSize 已經先鎖定,不會被算進本層。
//     為什麼會錯：多數人腦中把 q.size() 當成一個穩定的值,忘了迴圈裡
//         **一邊 pop 一邊 push**,size() 是隨時在變的。若寫成
//         for (size_t i = 0; i < q.size(); ++i) 每次迭代都重新求值,
//         層的邊界就完全亂掉了。
//
// ⚠️ 陷阱. 多執行緒的 producer-consumer,寫成
//         if (!q.empty()) { auto x = q.front(); q.pop(); } 只要 queue 有加鎖
//         就安全了吧?
//     答：不安全。就算 empty()、front()、pop() 每一個各自都上鎖,三者之間
//         仍有空隙:執行緒 A 檢查完 !empty() 為真,還沒取值就被切換走,
//         執行緒 B 把最後一個元素 pop 掉;A 回來後對**空佇列**呼叫 front(),
//         就是 undefined behavior。這是典型的 TOCTOU(檢查與使用之間的競態)。
//     為什麼會錯：把「每個操作各自是 atomic」誤當成「整段邏輯是 atomic」。
//         正確做法是把 empty()+front()+pop() **整組**放進同一個
//         std::lock_guard 的臨界區,或直接用 condition_variable 等待非空。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <queue>
#include <deque>
#include <list>
#include <vector>
#include <string>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 102. Binary Tree Level Order Traversal
//   題目：給一棵二元樹,回傳它的**層序走訪**結果 —— 每一層的節點值各成一個
//     陣列,由上而下排列。例如 [3,9,20,null,null,15,7] → [[3],[9,20],[15,7]]。
//   為什麼用到本主題：BFS(廣度優先搜尋)的本質就是佇列。你希望「先發現的節點
//     先被展開」,這正是 FIFO 的定義 —— 換成 stack(LIFO)就會變成 DFS,
//     結果完全不同。這題是「為什麼世上需要佇列」最直觀的答案。
//   關鍵技巧：每層開始前先用 levelSize 鎖定本層節點數,才能把層與層切開
//     (見上方【陷阱】題)。
//   複雜度：時間 O(n)(每個節點進出佇列各一次),空間 O(w),w 是最大層寬。
// -----------------------------------------------------------------------------
struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    explicit TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

std::vector<std::vector<int>> levelOrder(TreeNode* root) {
    std::vector<std::vector<int>> result;
    if (root == nullptr) return result;

    std::queue<TreeNode*> q;
    q.push(root);

    while (!q.empty()) {
        // 關鍵:先把本層節點數存下來。迴圈中會 push 下一層的節點進去,
        // q.size() 會一直變 —— 先鎖定才切得出層的邊界。
        size_t levelSize = q.size();
        std::vector<int> level;
        level.reserve(levelSize);

        for (size_t i = 0; i < levelSize; ++i) {
            TreeNode* node = q.front();   // 先讀
            q.pop();                      // 再移除(pop 回傳 void)
            level.push_back(node->val);

            // 子節點排進隊尾,它們屬於「下一層」,本輪不會被處理到
            if (node->left  != nullptr) q.push(node->left);
            if (node->right != nullptr) q.push(node->right);
        }
        result.push_back(level);
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】印表機的列印工作排程器(print spooler)
//   情境：一台共用印表機接受多位同事送來的列印工作。列印必須**嚴格照送出
//     順序**執行 —— 誰先按下列印誰先印,這是使用者對公平性的基本期待,
//     也是所有作業系統 spooler 的預設行為。
//   為什麼用到本主題：這就是教科書級的 FIFO。用 std::queue 而不是 vector,
//     是因為這裡**只需要**先進先出,而型別系統會確保沒有人不小心寫出
//     jobs[2] 這種插隊存取 —— 那會破壞公平性,而且是使用者會抱怨、
//     工程師卻很難重現的 bug。
//   注意：真實的 spooler 多半還有優先權(急件先印),那就該換成
//     std::priority_queue(見下一個檔案)—— 兩者的選擇取決於「公平」
//     還是「重要」哪個優先。
// -----------------------------------------------------------------------------
class PrintSpooler {
public:
    // 送出一份列印工作,排進隊尾
    void submit(const std::string& user, const std::string& doc, int pages) {
        jobs_.push(Job{user, doc, pages});
        queuedPages_ += pages;
    }

    // 取出並「列印」下一份工作。沒有工作可印時回傳 false。
    // 注意這個 empty() 檢查是必要的,不是防禦性冗餘 ——
    // 少了它,對空佇列呼叫 front() 就是 undefined behavior。
    bool printNext(std::string& logLine) {
        if (jobs_.empty()) return false;

        const Job& j = jobs_.front();       // 先讀(reference,不複製)
        logLine = "  列印中: [" + j.user + "] " + j.doc
                + " (" + std::to_string(j.pages) + " 頁)";
        queuedPages_ -= j.pages;
        jobs_.pop();                        // 再移除
        return true;
    }

    // 只看不取:下一個會印誰的文件?(front 是最早送出的那筆)
    bool peekNext(std::string& who) const {
        if (jobs_.empty()) return false;
        who = jobs_.front().user;
        return true;
    }

    // 剛剛送進來的最後一筆是誰的?(back 是最晚送出的那筆)
    bool peekLast(std::string& who) const {
        if (jobs_.empty()) return false;
        who = jobs_.back().user;
        return true;
    }

    size_t pendingJobs()  const { return jobs_.size(); }
    int    pendingPages() const { return queuedPages_; }

private:
    struct Job {
        std::string user;
        std::string doc;
        int         pages;
    };
    std::queue<Job> jobs_;
    int             queuedPages_ = 0;
};

int main() {
    // ── 原始課堂示範:std::queue 的基本操作 ────────────────────────────────
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

    // ── 配接器是零成本包裝的實證 ──────────────────────────────────────────
    std::cout << "\n=== 配接器零成本實證 ===" << std::endl;
    std::cout << "sizeof(std::queue<int>) = " << sizeof(std::queue<int>) << std::endl;
    std::cout << "sizeof(std::deque<int>) = " << sizeof(std::deque<int>) << std::endl;
    std::cout << "兩者相等 → 配接器本身不佔額外空間(數值為實作定義)" << std::endl;
    // 預設底層容器由標準規定為 std::deque<T>
    static_assert(std::is_same<std::queue<int>::container_type, std::deque<int>>::value,
                  "queue 的預設底層容器是 std::deque");

    // ── FIFO vs LIFO:同樣的輸入,順序完全相反 ─────────────────────────────
    std::cout << "\n=== FIFO 的意義 ===" << std::endl;
    std::queue<std::string> line;
    line.push("Alice");     // 第一個來排隊
    line.push("Bob");
    line.push("Carol");     // 最後一個來排隊
    std::cout << "front()(最早到、下一個被服務): " << line.front() << std::endl;
    std::cout << "back() (最晚到、剛站到隊尾)  : " << line.back()  << std::endl;
    std::cout << "服務順序: ";
    while (!line.empty()) {
        std::cout << line.front() << " ";
        line.pop();
    }
    std::cout << "(先來先服務 —— 換成 stack 會變成 Carol Bob Alice)" << std::endl;

    // ── 更換底層容器:list 可以,vector 不行 ───────────────────────────────
    std::cout << "\n=== 更換底層容器 ===" << std::endl;
    std::queue<int, std::list<int>> lq;      // list 有 push_back/pop_front → 合法
    lq.push(1);
    lq.push(2);
    lq.push(3);
    std::cout << "std::queue<int, std::list<int>> 前端: " << lq.front()
              << ",大小: " << lq.size() << std::endl;
    std::cout << "但 std::queue<int, std::vector<int>> 無法編譯 ——" << std::endl;
    std::cout << "  vector 沒有 pop_front(),連編譯都過不了(故此處不示範)" << std::endl;

    // ── 從既有容器建構(CTAD,C++17) ─────────────────────────────────────
    std::cout << "\n=== 從既有容器建構(CTAD,C++17) ===" << std::endl;
    std::deque<int> seed{7, 8, 9};
    std::queue q2(seed);                     // C++17 才能省略模板參數
    std::cout << "std::queue q2(seed) 大小: " << q2.size()
              << ",front: " << q2.front()
              << ",back: "  << q2.back() << std::endl;

    // ── 清空 queue:沒有 clear(),只能整個換掉 ─────────────────────────────
    std::cout << "\n=== 清空 queue(沒有 clear()) ===" << std::endl;
    std::cout << "換掉前 q2.size() = " << q2.size() << std::endl;
    q2 = std::queue<int>();                  // 慣用寫法:整個丟棄重建
    std::cout << "換掉後 q2.size() = " << q2.size() << std::endl;

    // ── LeetCode 102 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 102. Binary Tree Level Order Traversal ===" << std::endl;
    // 建出範例樹 [3,9,20,null,null,15,7]:
    //         3
    //        ┌┴┐
    //        9 20
    //          ┌┴┐
    //         15 7
    TreeNode n3(3), n9(9), n20(20), n15(15), n7(7);
    n3.left  = &n9;
    n3.right = &n20;
    n20.left  = &n15;
    n20.right = &n7;

    std::vector<std::vector<int>> levels = levelOrder(&n3);
    std::cout << "層序走訪結果: [";
    for (size_t i = 0; i < levels.size(); ++i) {
        std::cout << "[";
        for (size_t j = 0; j < levels[i].size(); ++j) {
            std::cout << levels[i][j];
            if (j + 1 < levels[i].size()) std::cout << ",";
        }
        std::cout << "]";
        if (i + 1 < levels.size()) std::cout << ",";
    }
    std::cout << "]" << std::endl;
    std::cout << "共 " << levels.size() << " 層(每層靠 levelSize 鎖定節點數切開)" << std::endl;

    // ── 日常實務:印表機列印排程 ──────────────────────────────────────────
    std::cout << "\n=== 日常實務: 印表機列印排程(FIFO) ===" << std::endl;
    PrintSpooler spooler;
    spooler.submit("Alice", "季報.pdf",       12);
    spooler.submit("Bob",   "合約草稿.docx",   3);
    spooler.submit("Carol", "海報.png",        1);

    std::string who;
    if (spooler.peekNext(who)) std::cout << "下一個要印(front): " << who << std::endl;
    if (spooler.peekLast(who)) std::cout << "最後送出的(back) : " << who << std::endl;
    std::cout << "待印工作數: " << spooler.pendingJobs()
              << ",總頁數: " << spooler.pendingPages() << std::endl;

    std::cout << "開始列印:" << std::endl;
    std::string logLine;
    while (spooler.printNext(logLine)) {
        std::cout << logLine << std::endl;
    }
    std::cout << "全部印完,剩餘工作數: " << spooler.pendingJobs() << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類13.cpp" -o queue_demo

// === 預期輸出 ===
// === std::queue ===
// 前端: 10
// 後端: 30
// 依序取出: 10 20 30
//
// === 配接器零成本實證 ===
// sizeof(std::queue<int>) = 80
// sizeof(std::deque<int>) = 80
// 兩者相等 → 配接器本身不佔額外空間(數值為實作定義)
//
// === FIFO 的意義 ===
// front()(最早到、下一個被服務): Alice
// back() (最晚到、剛站到隊尾)  : Carol
// 服務順序: Alice Bob Carol (先來先服務 —— 換成 stack 會變成 Carol Bob Alice)
//
// === 更換底層容器 ===
// std::queue<int, std::list<int>> 前端: 1,大小: 3
// 但 std::queue<int, std::vector<int>> 無法編譯 ——
//   vector 沒有 pop_front(),連編譯都過不了(故此處不示範)
//
// === 從既有容器建構(CTAD,C++17) ===
// std::queue q2(seed) 大小: 3,front: 7,back: 9
//
// === 清空 queue(沒有 clear()) ===
// 換掉前 q2.size() = 3
// 換掉後 q2.size() = 0
//
// === LeetCode 102. Binary Tree Level Order Traversal ===
// 層序走訪結果: [[3],[9,20],[15,7]]
// 共 3 層(每層靠 levelSize 鎖定節點數切開)
//
// === 日常實務: 印表機列印排程(FIFO) ===
// 下一個要印(front): Alice
// 最後送出的(back) : Carol
// 待印工作數: 3,總頁數: 16
// 開始列印:
//   列印中: [Alice] 季報.pdf (12 頁)
//   列印中: [Bob] 合約草稿.docx (3 頁)
//   列印中: [Carol] 海報.png (1 頁)
// 全部印完,剩餘工作數: 0
