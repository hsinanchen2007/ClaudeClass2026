// =============================================================================
//  第 27 課：deque 作為 stack/queue 底層容器 2  —  std::queue 基本操作
// =============================================================================
//
// 【主題資訊 Information】
//   template <class T, class Container = std::deque<T>> class queue;
//
//   void push(const T&);  void push(T&&);
//   template <class... Args> decltype(auto) emplace(Args&&...);  // C++17 起回傳 reference
//   void pop();                       // 回傳 void！
//   reference front();                // 最先進去的（下一個要出來的）
//   reference back();                 // 最後進去的
//   bool empty() const;  size_type size() const;
//
//   標頭檔：<queue>
//   複雜度：全部 O(1)（攤銷）
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. queue 的委託關係——注意與 stack 的關鍵差異】
//   queue 同樣是配接器，內部只有一個 Container c：
//       push(v)  →  c.push_back(v)
//       pop()    →  c.pop_front()      ★ 這裡與 stack 不同！
//       front()  →  c.front()
//       back()   →  c.back()
//   stack 是 push_back / pop_back（同一端進出）；
//   queue 是 push_back / **pop_front**（兩端各司其職）。
//   這個 pop_front 就是「為什麼 queue 的底層不能是 vector」的全部答案。
//
// 【2. 為什麼 vector 不能當 queue 的底層】
//   std::vector **根本沒有 pop_front() 這個成員函式**。
//   所以 queue<int, vector<int>> 一旦你呼叫 pop()，編譯就會失敗，
//   錯誤訊息大致是 "'class std::vector<int>' has no member named 'pop_front'"。
//   ⚠️ 注意這個微妙之處：只宣告 queue<int, vector<int>> 而不呼叫 pop()，
//      通常**可以編譯通過**——因為成員函式是模板，只有被使用時才實例化。
//      這是 C++ 模板「延遲實例化」的性質，本檔會用註解說明。
//   即使 vector 提供了 pop_front，那也會是 O(n)（要把全部元素往前搬一格），
//   讓 queue 的核心操作從 O(1) 退化成 O(n)。
//
// 【3. deque 為什麼剛好適合】
//   deque 的 push_back 與 pop_front 都是攤銷 O(1)，而且：
//       * 出列後，前端的空 chunk 可以被歸還或重複利用
//       * 入列時只是在尾端 chunk 加一格，或加掛新 chunk
//   所以一個長期運行的 queue（例如伺服器的請求佇列）不會無限膨脹，
//   記憶體用量與「目前佇列長度」成正比，而不是與「歷史總入列數」成正比。
//
// 【4. queue 有 front() 也有 back()】
//   這點常被忽略：queue 可以看尾端（back()），只是不能從尾端取出。
//   front() 是「下一個要出來的」，back() 是「最後排進來的」。
//   實務上 back() 很有用，例如「檢查剛加入的任務是否與前一個重複」。
//   但要移除，永遠只能從 front 那端（pop()）。
//
// 【概念補充 Concept Deep Dive】
//   ● 底層容器的最低要求
//     queue 的底層必須提供：front()、back()、push_back()、pop_front()、
//     empty()、size()。符合的標準容器只有 **deque 與 list**。
//     vector 因缺 pop_front 而不符合。
//
//   ● 為什麼不用 list
//     list 的 push_back / pop_front 也都是 O(1)，語意上完全可行
//     （queue<int, list<int>> 合法且能運作）。
//     但 list 每個元素要一次獨立的 heap 配置、每個節點多 16 位元組的指標，
//     而且節點散布記憶體各處、快取局部性差。
//     deque 一次配置一整塊 chunk（本機 512 位元組）攤提多個元素，
//     整體效率明顯較好——這就是標準選它當預設的理由。
//
//   ● pop() 同樣不回傳值
//     理由與 stack 完全相同：例外安全。
//     正確寫法是 int v = q.front(); q.pop();
//
//   ● priority_queue 是另一回事
//     同樣在 <queue> 標頭裡，但 std::priority_queue 的**預設底層是 vector**，
//     因為它需要 random access 來做 heap 演算法（push_heap / pop_heap）。
//     別把三個配接器的預設底層搞混：
//         stack          → deque
//         queue          → deque
//         priority_queue → vector
//
// 【注意事項 Pay Attention】
//   1. 對空 queue 呼叫 front() / back() / pop() 是 **UB**，務必先 empty() 檢查。
//   2. pop() 回傳 void；要取值請先 front()。
//   3. queue **不能**用 vector 當底層（缺 pop_front）；可以用 deque 或 list。
//   4. queue 沒有 begin()/end()/operator[]——刻意的介面限制。
//   5. priority_queue 的預設底層是 **vector**，不是 deque，別搞混。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::queue
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::queue 不能用 vector 當底層容器？
//     答：queue 的 pop() 委託給底層的 **pop_front()**，
//         而 std::vector 根本沒有這個成員函式，編譯會失敗。
//         即使自己補一個，vector 的前端刪除也必須把全部元素往前搬一格，
//         是 O(n)，會讓 queue 最核心的操作退化。
//         deque 的 pop_front 是攤銷 O(1)，這才是它被選為預設底層的原因。
//     追問：那 stack 為什麼可以用 vector？→ stack 只需要
//         push_back / pop_back / back，全部都是 vector 的 O(1) 操作。
//
// 🔥 Q2. stack、queue、priority_queue 的預設底層容器分別是什麼？
//     答：stack → deque，queue → deque，**priority_queue → vector**。
//         priority_queue 不同是因為它要用 std::push_heap / pop_heap 維護堆積，
//         那些演算法需要 random access iterator，
//         而且 vector 的連續記憶體讓 heap 的父子索引計算（2i+1、2i+2）最有效率。
//     追問：priority_queue 可以用 deque 嗎？→ 可以（deque 也是 random access），
//         但沒有好處：heap 演算法大量隨機存取，deque 的兩次間接定址反而更慢。
//
// ⚠️ 陷阱. 寫 std::queue<int, std::vector<int>> q; 這一行，編譯會過嗎？
//     答：**通常會過**——只要你沒有呼叫 q.pop()。
//         C++ 的類別模板成員函式是「延遲實例化」的：
//         只有被實際使用的成員函式才會被編譯。
//         一旦你寫下 q.pop()，編譯器才去實例化它、才發現 vector 沒有 pop_front，
//         這時才報錯。
//     為什麼會錯：以為「型別不合就會在宣告當下報錯」。
//         實際上模板的錯誤往往延後到「第一次使用該成員」時才浮現，
//         這也是為什麼模板的錯誤訊息常常又長又晚。
//         C++20 的 concepts 就是為了讓這類約束能在介面層就被檢查並給出清楚訊息。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <queue>
#include <stack>      // 第 2 節要比較三個配接器的預設底層
#include <deque>
#include <list>
#include <vector>
#include <string>
#include <type_traits>
using namespace std;

// -----------------------------------------------------------------------------
// detection idiom：在編譯期詢問「型別 C 有沒有 pop_front() 成員函式」。
// 這正是 queue 對底層容器的關鍵要求——vector 因為缺這一項而不能當 queue 底層。
// （C++20 可用 concepts 寫得更短，這裡用 C++17 通用的 SFINAE 寫法。）
// -----------------------------------------------------------------------------
template <typename C, typename = void>
struct HasPopFront : false_type {};

template <typename C>
struct HasPopFront<C, void_t<decltype(declval<C&>().pop_front())>> : true_type {};

// -----------------------------------------------------------------------------
// 【日常實務範例】列印工作佇列（先到先服務）
//   情境：印表機服務接收多個使用者送來的工作，必須嚴格照抵達順序處理——
//         這正是 FIFO 的定義。用 queue 表達這個語意，
//         型別本身就保證了「沒有人能插隊」（沒有 insert、沒有 operator[]）。
//   額外示範 back()：加入前先看看最後一筆，避免同一使用者連續送出重複工作。
// -----------------------------------------------------------------------------
struct PrintJob {
    string user;
    string document;
    int pages;
};

class PrintSpooler {
    queue<PrintJob> jobs_;
    int totalPagesPrinted_ = 0;
public:
    // 回傳 false 代表被視為重複送件而略過
    bool submit(const PrintJob& job) {
        // queue 可以看 back()（最後排進來的），只是不能從那端取出
        if (!jobs_.empty() &&
            jobs_.back().user == job.user &&
            jobs_.back().document == job.document) {
            return false;                       // 與上一筆重複，忽略
        }
        jobs_.push(job);
        return true;
    }

    // 處理一筆；回傳是否真的處理了
    bool processNext() {
        if (jobs_.empty()) return false;        // 先檢查！對空 queue front() 是 UB
        const PrintJob& j = jobs_.front();      // 先看
        cout << "    列印 [" << j.user << "] " << j.document
             << "（" << j.pages << " 頁）" << endl;
        totalPagesPrinted_ += j.pages;
        jobs_.pop();                            // 再移除（兩步式）
        return true;
    }

    size_t pending() const { return jobs_.size(); }
    int totalPages() const { return totalPagesPrinted_; }
    // 可以窺看下一個要處理的，但不能取出
    const PrintJob& peekNext() const { return jobs_.front(); }
};

// 註：本檔不附 LeetCode 範例。單純的 queue 基本操作沒有對應的 LeetCode 題型
//     （BFS 類題目會用到 queue，但那是同課 5.cpp 的主題，
//     在這裡重複一次只會稀釋「配接器介面」這個重點）。

int main() {
    cout << "=== 1. queue 基本操作 ===" << endl;
    queue<int> q;       // 預設底層是 deque<int>

    q.push(10);
    q.push(20);
    q.push(30);

    cout << "front: " << q.front() << endl;  // 10（最先進去的）
    cout << "back: " << q.back() << endl;    // 30（最後進去的）

    q.pop();   // 移除 10
    cout << "pop 後 front: " << q.front() << endl;  // 20

    cout << "逐一取出：";
    while (!q.empty()) {
        cout << q.front() << " ";
        q.pop();
    }
    cout << endl;
    // 輸出：20 30（先進先出）

    cout << "\n=== 2. 三個配接器的預設底層（編譯期驗證）===" << endl;
    cout << boolalpha;
    cout << "stack<int>          底層是 deque<int>?  "
         << is_same_v<stack<int>::container_type, deque<int>> << endl;
    cout << "queue<int>          底層是 deque<int>?  "
         << is_same_v<queue<int>::container_type, deque<int>> << endl;
    cout << "priority_queue<int> 底層是 vector<int>? "
         << is_same_v<priority_queue<int>::container_type, vector<int>> << endl;
    cout << "→ priority_queue 用 vector，因為 heap 演算法需要 random access" << endl;

    cout << "\n=== 3. 為什麼 vector 不能當 queue 的底層 ===" << endl;
    cout << "queue 的 pop() 委託給底層的 pop_front()，" << endl;
    cout << "而 std::vector 根本沒有 pop_front 這個成員函式。" << endl;
    cout << "用 detection idiom 在編譯期問「這個型別有沒有 pop_front」：" << endl;
    cout << "  vector<int> 有 pop_front? " << HasPopFront<vector<int>>::value
         << "  ← 缺這一項，所以 queue<int,vector<int>> 呼叫 pop() 會編譯失敗" << endl;
    cout << "  deque<int>  有 pop_front? " << HasPopFront<deque<int>>::value
         << "   ← 攤銷 O(1)，標準選它當預設" << endl;
    cout << "  list<int>   有 pop_front? " << HasPopFront<list<int>>::value
         << "   ← O(1)，但每個節點都要獨立配置記憶體" << endl;
    // 編譯期斷言：把上面的結論釘死在型別系統裡
    static_assert(!HasPopFront<vector<int>>::value, "vector 不應有 pop_front");
    static_assert(HasPopFront<deque<int>>::value,  "deque 應有 pop_front");
    static_assert(HasPopFront<list<int>>::value,   "list 應有 pop_front");
    cout << "⚠️ 微妙之處：只「宣告」queue<int, vector<int>> 通常能編譯通過，" << endl;
    cout << "   因為模板成員函式是延遲實例化的——直到你呼叫 pop() 才會報錯。" << endl;

    cout << "\n=== 4. 換底層：list 也可以（但通常不划算）===" << endl;
    queue<int, list<int>> ql;
    for (int i = 1; i <= 3; ++i) ql.push(i * 100);
    cout << "list 底層的 queue：front = " << ql.front()
         << ", back = " << ql.back() << ", size = " << ql.size() << endl;
    ql.pop();
    cout << "pop 後 front = " << ql.front() << endl;
    cout << "→ 語意完全相同；但 list 每個元素一次 heap 配置、節點多 16 位元組指標，" << endl;
    cout << "  快取局部性也差，所以標準選 deque 當預設。" << endl;

    cout << "\n=== 5. front() 與 back() 的分工 ===" << endl;
    queue<string> line;
    line.push("第一位");
    line.push("第二位");
    line.push("第三位");
    cout << "front()（下一個要服務的）= " << line.front() << endl;
    cout << "back() （最後排進來的）  = " << line.back() << endl;
    cout << "→ 可以「看」尾端，但只能從 front 那端 pop —— 這就是 FIFO 的保證" << endl;

    cout << "\n=== 日常實務：印表機工作佇列（先到先服務）===" << endl;
    PrintSpooler spooler;
    cout << "  送出工作：" << endl;
    struct { const char* u; const char* d; int p; } incoming[] = {
        {"alice", "report.pdf", 12},
        {"bob",   "slides.pptx", 30},
        {"bob",   "slides.pptx", 30},     // 與上一筆重複
        {"carol", "invoice.pdf", 2},
        {"alice", "notes.txt",   1}
    };
    for (auto& j : incoming) {
        bool accepted = spooler.submit({j.u, j.d, j.p});
        cout << "    [" << j.u << "] " << j.d
             << (accepted ? " → 已排入" : " → 與前一筆重複，略過") << endl;
    }
    cout << "  待處理 " << spooler.pending() << " 筆，下一筆是 ["
         << spooler.peekNext().user << "] " << spooler.peekNext().document << endl;

    cout << "  開始處理：" << endl;
    while (spooler.processNext()) { /* 依序處理直到清空 */ }
    cout << "  全部完成，累計列印 " << spooler.totalPages() << " 頁" << endl;
    cout << "  → 用 queue 表達「先到先服務」，型別本身就保證沒有人能插隊" << endl;
    cout << "    （沒有 insert、沒有 operator[]、不能從尾端取出）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 27 課：deque 作為 stackqueue 底層容器2.cpp" -o demo27_2

// === 預期輸出 ===
// === 1. queue 基本操作 ===
// front: 10
// back: 30
// pop 後 front: 20
// 逐一取出：20 30
//
// === 2. 三個配接器的預設底層（編譯期驗證）===
// stack<int>          底層是 deque<int>?  true
// queue<int>          底層是 deque<int>?  true
// priority_queue<int> 底層是 vector<int>? true
// → priority_queue 用 vector，因為 heap 演算法需要 random access
//
// === 3. 為什麼 vector 不能當 queue 的底層 ===
// queue 的 pop() 委託給底層的 pop_front()，
// 而 std::vector 根本沒有 pop_front 這個成員函式。
// 用 detection idiom 在編譯期問「這個型別有沒有 pop_front」：
//   vector<int> 有 pop_front? false  ← 缺這一項，所以 queue<int,vector<int>> 呼叫 pop() 會編譯失敗
//   deque<int>  有 pop_front? true   ← 攤銷 O(1)，標準選它當預設
//   list<int>   有 pop_front? true   ← O(1)，但每個節點都要獨立配置記憶體
// ⚠️ 微妙之處：只「宣告」queue<int, vector<int>> 通常能編譯通過，
//    因為模板成員函式是延遲實例化的——直到你呼叫 pop() 才會報錯。
//
// === 4. 換底層：list 也可以（但通常不划算）===
// list 底層的 queue：front = 100, back = 300, size = 3
// pop 後 front = 200
// → 語意完全相同；但 list 每個元素一次 heap 配置、節點多 16 位元組指標，
//   快取局部性也差，所以標準選 deque 當預設。
//
// === 5. front() 與 back() 的分工 ===
// front()（下一個要服務的）= 第一位
// back() （最後排進來的）  = 第三位
// → 可以「看」尾端，但只能從 front 那端 pop —— 這就是 FIFO 的保證
//
// === 日常實務：印表機工作佇列（先到先服務）===
//   送出工作：
//     [alice] report.pdf → 已排入
//     [bob] slides.pptx → 已排入
//     [bob] slides.pptx → 與前一筆重複，略過
//     [carol] invoice.pdf → 已排入
//     [alice] notes.txt → 已排入
//   待處理 4 筆，下一筆是 [alice] report.pdf
//   開始處理：
//     列印 [alice] report.pdf（12 頁）
//     列印 [bob] slides.pptx（30 頁）
//     列印 [carol] invoice.pdf（2 頁）
//     列印 [alice] notes.txt（1 頁）
//   全部完成，累計列印 45 頁
//   → 用 queue 表達「先到先服務」，型別本身就保證沒有人能插隊
//     （沒有 insert、沒有 operator[]、不能從尾端取出）
