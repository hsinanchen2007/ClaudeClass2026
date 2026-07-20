// =============================================================================
//  第 23 課：deque 的雙端操作 1  —  push_front / push_back / pop_front / pop_back
// =============================================================================
//
// 【主題資訊 Information】
//   void push_front(const T&);  void push_front(T&&);      // 頭端插入
//   void push_back(const T&);   void push_back(T&&);       // 尾端插入
//   void pop_front();                                       // 頭端刪除
//   void pop_back();                                        // 尾端刪除
//   reference front();  reference back();                   // 存取首尾
//
//   標頭檔：<deque>
//   複雜度：四個操作皆為 **攤銷 O(1)**；front()/back() 為 O(1)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. deque 憑什麼兩端都是 O(1)】
//   關鍵在於它「不是一整塊連續記憶體」。deque（double-ended queue）採用
//   **分段連續**的結構：
//       中央控制器（map）：一個指標陣列，每個元素指向一塊固定大小的緩衝區
//       緩衝區（chunk）  ：真正存放元素的地方，每塊容量固定
//   本機 libstdc++ 的 chunk 大小規則是：
//       元素大小 < 512 位元組 → 每塊放 512 / sizeof(T) 個
//       元素大小 ≥ 512 位元組 → 每塊放 1 個
//   所以 deque<int> 每塊放 512/4 = 128 個元素。
//   ⚠️ 這個 512 是 libstdc++ 的實作值，MSVC 的 STL 用的是不同的規則
//      （小型別為 16 個元素），標準完全沒有規定。
//
//   有了這個結構，push_front 只需要：
//       ① 目前的頭端 chunk 前面還有空位 → 直接放進去
//       ② 沒空位 → 配置一塊新 chunk，把它的指標放進中央控制器的前端
//   兩種情況都不需要搬移任何既有元素 → 攤銷 O(1)。
//
// 【2. 為什麼 vector 做不到 push_front】
//   vector 保證元素連續且從索引 0 開始。要在前面插入，
//   就得把全部 n 個元素往後推一格 → O(n)。
//   而且 vector 根本沒有提供 push_front 這個成員函式，
//   你只能寫 v.insert(v.begin(), x)——這個寫法本身就在提醒你「這很貴」。
//
// 【3. 四個操作的對稱性】
//   push_back  ←→  pop_back    （尾端，堆疊 LIFO 用這一組）
//   push_front ←→  pop_front   （頭端）
//   佇列 FIFO = push_back 入列 + pop_front 出列
//   堆疊 LIFO = push_back 入棧 + pop_back  出棧
//   deque 一個容器就能同時扮演這兩種角色，這是它最實用的地方。
//
// 【4. pop 不回傳值——為什麼】
//   pop_front() / pop_back() 的回傳型別都是 void，想取值必須先 front()/back()。
//   看起來多此一舉，但這是刻意的**例外安全**設計：
//   若 pop 同時回傳值，那個回傳動作需要複製建構一個 T。
//   萬一複製建構子丟出例外，元素已經被移除、值卻沒交到你手上——資料就遺失了。
//   拆成「先看（front）、再移除（pop）」兩步，任一步失敗都不會遺失資料。
//   這是 Herb Sutter 在 Exceptional C++ 裡分析過的經典設計取捨。
//
// 【概念補充 Concept Deep Dive】
//   ● 空容器上呼叫 front() / back() / pop_front() / pop_back() 是 UB
//     標準沒有規定要檢查，也沒有規定會丟例外。
//     實務上通常「看起來」讀到垃圾值或直接崩潰，但**不保證任何特定結果**。
//     一定要先檢查 empty()。
//
//   ● iterator 失效規則與 vector 完全不同
//     push_front / push_back：**所有 iterator 失效**，
//       但**指向元素的 reference 與 pointer 仍然有效**（元素本身沒被搬動）。
//     pop_front / pop_back：只有被移除那個元素的 iterator/reference 失效。
//     這個「iterator 失效但 reference 不失效」的性質是 deque 獨有的，
//     vector 做不到（它一擴容就全部搬家）。
//
//   ● 代價：存取比 vector 慢
//     deque 的 operator[] 要先算「在第幾塊、塊內第幾格」，是兩次間接定址；
//     vector 只要一次加法。加上元素分散在多塊記憶體，快取局部性也較差。
//     所以 deque 是「用存取速度換兩端插入能力」的取捨，不是全面更好。
//
// 【注意事項 Pay Attention】
//   1. 對空 deque 呼叫 front/back/pop_front/pop_back 是 **UB**，
//      不保證崩潰、不保證任何特定值。務必先 empty() 檢查。
//   2. pop_front/pop_back **不回傳值**，要取值請先 front()/back()。
//   3. push_front/push_back 會使**所有 iterator 失效**，
//      但 reference 與 pointer 仍有效——這點與 vector 完全不同。
//   4. chunk 大小（本機 512 位元組）是實作定義，不可依賴。
//   5. deque 沒有 reserve()，也沒有 capacity()——它的成長方式與 vector 不同，
//      不需要（也無法）預留連續空間。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的雙端操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 為什麼 push_front 和 push_back 都是 O(1)，vector 卻做不到？
//     答：deque 是「分段連續」——中央控制器（指標陣列）+ 多塊固定大小的緩衝區。
//         在前端插入時，只要頭端 chunk 還有空位就直接放；沒有就配置一塊新 chunk，
//         把指標掛到控制器前端。兩種情況都不搬移既有元素。
//         vector 必須維持單一連續區塊且從索引 0 開始，前端插入要把全部元素
//         往後推一格，所以是 O(n)——它甚至沒有提供 push_front 這個函式。
//     追問：deque 的代價是什麼？→ operator[] 要算「第幾塊、塊內第幾格」，
//         是兩次間接定址；元素分散多塊，快取局部性也比 vector 差。
//
// 🔥 Q2. 為什麼 pop_front() 不回傳被移除的元素？
//     答：例外安全。若 pop 同時回傳值，就必須複製建構一個 T 當回傳值；
//         萬一複製建構子丟出例外，元素已經被移除、值卻沒交到呼叫端手上，
//         資料就永久遺失了。拆成 front()（看）+ pop_front()（移除）兩步，
//         任何一步失敗，容器狀態都還是完整的。
//     追問：那 C++ 有沒有補救？→ 有 try_pop 這類自訂做法，
//         標準庫本身則維持兩步式；C++17 的 map::extract 用 node handle
//         解決了類似問題（移動節點而非複製值）。
//
// ⚠️ 陷阱. deque 的 push_back 之後，先前取得的 iterator 和 reference
//         哪個還能用？很多人認為「跟 vector 一樣，全都失效」。
//     答：**iterator 全部失效，但 reference 和 pointer 仍然有效**。
//         因為 deque 插入時不搬移既有元素（新元素放進新的或既有的 chunk），
//         元素的實體位址沒變；但 iterator 內部記錄著「目前在哪一塊、
//         塊內位置、以及控制器位置」，控制器可能被重新配置，所以 iterator 壞了。
//     為什麼會錯：把 vector 的失效規則直接套到 deque 上。
//         vector 因為要維持單一連續區塊，擴容時元素**真的被搬到新位址**，
//         所以 reference/pointer 一起失效。deque 的分段結構讓它能給出
//         更強的保證——這也是某些場合（需要長期持有元素參考）選 deque 的理由。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <string>
#include <algorithm>
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個堆疊，支援 push / pop / top，以及「在常數時間取得最小值」getMin。
//   為什麼用到本主題：這題的核心就是本課的 push_back / pop_back / back()。
//     用兩個 deque：一個存所有元素，一個存「到目前為止的最小值」。
//     每次 push 都把 min(新值, 目前最小) 推進輔助堆疊，
//     於是 getMin 只要看 back() 就好——O(1)。
//     這裡刻意用 deque 而不是 stack 配接器，是為了直接示範本課的四個操作。
// -----------------------------------------------------------------------------
class MinStack {
    deque<int> data_;   // 主堆疊
    deque<int> mins_;   // 輔助堆疊：mins_.back() 永遠是 data_ 目前的最小值
public:
    void push(int val) {
        data_.push_back(val);
        // 新的最小值 = min(這次的值, 之前的最小值)
        mins_.push_back(mins_.empty() ? val : std::min(val, mins_.back()));
    }
    void pop() {
        if (data_.empty()) return;      // 先檢查，避免對空容器 pop（UB）
        data_.pop_back();
        mins_.pop_back();               // 兩個堆疊同進同出，保持對齊
    }
    int top() const { return data_.back(); }
    int getMin() const { return mins_.back(); }
    bool empty() const { return data_.empty(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】瀏覽器的上一頁 / 下一頁
//   情境：瀏覽器要同時支援「上一頁」「下一頁」，而且歷史紀錄有上限
//         （不能無限成長吃光記憶體）。
//         這需要三種操作：尾端推入新頁面、尾端彈出（上一頁）、
//         以及**頭端彈出**（歷史超過上限時丟掉最舊的）——
//         最後這個正是 vector 做不到而 deque 擅長的。
// -----------------------------------------------------------------------------
class BrowserHistory {
    static const size_t MAX_HISTORY = 5;
    deque<string> back_;      // 上一頁堆疊（尾端是最近的）
    deque<string> forward_;   // 下一頁堆疊
    string current_;
public:
    explicit BrowserHistory(const string& home) : current_(home) {}

    void visit(const string& url) {
        back_.push_back(current_);            // 目前這頁進入「上一頁」歷史
        if (back_.size() > MAX_HISTORY) {
            back_.pop_front();                // ← deque 的價值：丟掉最舊的，O(1)
        }
        forward_.clear();                     // 造訪新頁面就清空「下一頁」
        current_ = url;
    }

    bool goBack() {
        if (back_.empty()) return false;
        forward_.push_back(current_);
        current_ = back_.back();
        back_.pop_back();
        return true;
    }

    bool goForward() {
        if (forward_.empty()) return false;
        back_.push_back(current_);
        current_ = forward_.back();
        forward_.pop_back();
        return true;
    }

    const string& current() const { return current_; }
    size_t backDepth() const { return back_.size(); }
};

void print(const string& label, const deque<int>& dq) {
    cout << label << ": ";
    if (dq.empty()) {
        cout << "(空)";
    } else {
        cout << "front=" << dq.front() << " back=" << dq.back()
             << " | ";
        for (int val : dq) cout << val << " ";
    }
    cout << " (size=" << dq.size() << ")" << endl;
}

int main() {
    cout << "=== 1. 四個雙端操作 ===" << endl;
    deque<int> dq;
    print("初始狀態  ", dq);
    // 初始狀態  : (空) (size=0)

    // === 尾端操作 ===
    dq.push_back(10);
    print("push_back 10", dq);
    // push_back 10: front=10 back=10 | 10  (size=1)

    dq.push_back(20);
    dq.push_back(30);
    print("push_back 20,30", dq);
    // push_back 20,30: front=10 back=30 | 10 20 30  (size=3)

    // === 頭端操作 ===
    dq.push_front(5);
    print("push_front 5", dq);
    // push_front 5: front=5 back=30 | 5 10 20 30  (size=4)

    dq.push_front(1);
    print("push_front 1", dq);
    // push_front 1: front=1 back=30 | 1 5 10 20 30  (size=5)

    // === 雙端刪除 ===
    dq.pop_front();
    print("pop_front   ", dq);
    // pop_front   : front=5 back=30 | 5 10 20 30  (size=4)

    dq.pop_back();
    print("pop_back    ", dq);
    // pop_back    : front=5 back=20 | 5 10 20  (size=3)

    // === 連續操作模擬佇列行為 ===
    cout << "\n--- 模擬佇列（FIFO）---" << endl;
    deque<int> queue;

    // 入列（從尾端加入）
    for (int i = 1; i <= 5; i++) {
        queue.push_back(i * 100);
        cout << "入列 " << i * 100;
        print("  → 佇列", queue);
    }

    // 出列（從頭端取出）
    while (!queue.empty()) {
        cout << "出列 " << queue.front();
        queue.pop_front();
        print("  → 佇列", queue);
    }

    cout << "\n=== 2. 同一個 deque 也能當堆疊（LIFO）===" << endl;
    deque<int> stk;
    for (int i = 1; i <= 5; i++) stk.push_back(i * 10);
    cout << "入棧 10 20 30 40 50 後，出棧順序：";
    while (!stk.empty()) {
        cout << stk.back() << " ";
        stk.pop_back();          // 尾端進、尾端出 = LIFO
    }
    cout << "\n→ FIFO 用 push_back + pop_front；LIFO 用 push_back + pop_back" << endl;

    cout << "\n=== 3. 重要：reference 不失效，但 iterator 會 ===" << endl;
    deque<int> d = {100, 200, 300};
    int& ref = d[1];                    // 取得中間元素的 reference
    cout << "push_front 前 ref = " << ref << endl;
    d.push_front(50);                   // 頭端插入
    cout << "push_front 後 ref = " << ref
         << "（reference 仍然有效——deque 不搬移既有元素）" << endl;
    cout << "但此時所有 iterator 都已失效，必須重新取得" << endl;
    cout << "目前內容：";
    for (int x : d) cout << x << " ";
    cout << endl;

    cout << "\n=== 4. 空容器保護 ===" << endl;
    deque<int> empty_dq;
    cout << "empty() = " << boolalpha << empty_dq.empty() << endl;
    cout << "對空 deque 呼叫 front()/pop_front() 是未定義行為，" << endl;
    cout << "不保證崩潰、也不保證任何特定值——所以一定要先檢查：" << endl;
    if (!empty_dq.empty()) {
        cout << "  front = " << empty_dq.front() << endl;
    } else {
        cout << "  （容器為空，安全地略過存取）" << endl;
    }

    cout << "\n=== 5. 實測 chunk 大小（分段連續的直接證據）===" << endl;
    {
        deque<int> probe;
        for (int i = 0; i < 600; ++i) probe.push_back(i);
        // 相鄰元素位址不再相差 sizeof(int) 的地方，就是兩塊 chunk 的交界
        int breaks = 0, firstBreak = -1;
        for (size_t i = 1; i < probe.size(); ++i) {
            if (&probe[i] != &probe[i - 1] + 1) {
                if (firstBreak < 0) firstBreak = static_cast<int>(i);
                ++breaks;
            }
        }
        cout << "放入 600 個 int，位址不連續處共 " << breaks << " 個" << endl;
        cout << "第一個斷點在索引 " << firstBreak
             << " \u2192 每塊 chunk 放 " << firstBreak << " 個 int"
             << "，即 " << firstBreak * static_cast<int>(sizeof(int))
             << " 位元組" << endl;
        cout << "這與 libstdc++ 標頭中的 _GLIBCXX_DEQUE_BUF_SIZE = 512 相符。" << endl;
        cout << "\u26a0\ufe0f 512 是實作值，MSVC 的 STL 規則不同，標準完全沒有規定。" << endl;
    }

    cout << "\n=== LeetCode 155. Min Stack ===" << endl;
    MinStack ms;
    ms.push(-2);  cout << "push(-2) → getMin = " << ms.getMin() << endl;
    ms.push(0);   cout << "push(0)  → getMin = " << ms.getMin() << endl;
    ms.push(-3);  cout << "push(-3) → getMin = " << ms.getMin() << endl;
    cout << "getMin() = " << ms.getMin() << endl;   // -3
    ms.pop();     cout << "pop()    → top = " << ms.top()
                       << ", getMin = " << ms.getMin() << endl;
    cout << "→ 靠一個輔助 deque 同步 push_back/pop_back，getMin 就是 O(1)" << endl;

    cout << "\n=== 日常實務：瀏覽器上一頁 / 下一頁 ===" << endl;
    BrowserHistory bh("home.html");
    cout << "起始頁：" << bh.current() << endl;

    const char* pages[] = {"news.html", "sports.html", "weather.html",
                           "mail.html", "docs.html", "video.html"};
    for (const char* p : pages) {
        bh.visit(p);
        cout << "造訪 " << p << "（歷史深度 " << bh.backDepth() << "）" << endl;
    }
    cout << "→ 歷史上限 5，超過時用 pop_front 丟掉最舊的（vector 做不到 O(1)）"
         << endl;

    bh.goBack();     cout << "上一頁 → " << bh.current() << endl;
    bh.goBack();     cout << "上一頁 → " << bh.current() << endl;
    bh.goForward();  cout << "下一頁 → " << bh.current() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 23 課：deque 的雙端操作：push_front、push_back、pop_front、pop_back1.cpp" -o demo23_1

// === 預期輸出 ===
// === 1. 四個雙端操作 ===
// 初始狀態  : (空) (size=0)
// push_back 10: front=10 back=10 | 10  (size=1)
// push_back 20,30: front=10 back=30 | 10 20 30  (size=3)
// push_front 5: front=5 back=30 | 5 10 20 30  (size=4)
// push_front 1: front=1 back=30 | 1 5 10 20 30  (size=5)
// pop_front   : front=5 back=30 | 5 10 20 30  (size=4)
// pop_back    : front=5 back=20 | 5 10 20  (size=3)
//
// --- 模擬佇列（FIFO）---
// 入列 100  → 佇列: front=100 back=100 | 100  (size=1)
// 入列 200  → 佇列: front=100 back=200 | 100 200  (size=2)
// 入列 300  → 佇列: front=100 back=300 | 100 200 300  (size=3)
// 入列 400  → 佇列: front=100 back=400 | 100 200 300 400  (size=4)
// 入列 500  → 佇列: front=100 back=500 | 100 200 300 400 500  (size=5)
// 出列 100  → 佇列: front=200 back=500 | 200 300 400 500  (size=4)
// 出列 200  → 佇列: front=300 back=500 | 300 400 500  (size=3)
// 出列 300  → 佇列: front=400 back=500 | 400 500  (size=2)
// 出列 400  → 佇列: front=500 back=500 | 500  (size=1)
// 出列 500  → 佇列: (空) (size=0)
//
// === 2. 同一個 deque 也能當堆疊（LIFO）===
// 入棧 10 20 30 40 50 後，出棧順序：50 40 30 20 10
// → FIFO 用 push_back + pop_front；LIFO 用 push_back + pop_back
//
// === 3. 重要：reference 不失效，但 iterator 會 ===
// push_front 前 ref = 200
// push_front 後 ref = 200（reference 仍然有效——deque 不搬移既有元素）
// 但此時所有 iterator 都已失效，必須重新取得
// 目前內容：50 100 200 300
//
// === 4. 空容器保護 ===
// empty() = true
// 對空 deque 呼叫 front()/pop_front() 是未定義行為，
// 不保證崩潰、也不保證任何特定值——所以一定要先檢查：
//   （容器為空，安全地略過存取）
//
// === 5. 實測 chunk 大小（分段連續的直接證據）===
// 放入 600 個 int，位址不連續處共 4 個
// 第一個斷點在索引 128 → 每塊 chunk 放 128 個 int，即 512 位元組
// 這與 libstdc++ 標頭中的 _GLIBCXX_DEQUE_BUF_SIZE = 512 相符。
// ⚠️ 512 是實作值，MSVC 的 STL 規則不同，標準完全沒有規定。
//
// === LeetCode 155. Min Stack ===
// push(-2) → getMin = -2
// push(0)  → getMin = -2
// push(-3) → getMin = -3
// getMin() = -3
// pop()    → top = 0, getMin = -2
// → 靠一個輔助 deque 同步 push_back/pop_back，getMin 就是 O(1)
//
// === 日常實務：瀏覽器上一頁 / 下一頁 ===
// 起始頁：home.html
// 造訪 news.html（歷史深度 1）
// 造訪 sports.html（歷史深度 2）
// 造訪 weather.html（歷史深度 3）
// 造訪 mail.html（歷史深度 4）
// 造訪 docs.html（歷史深度 5）
// 造訪 video.html（歷史深度 5）
// → 歷史上限 5，超過時用 pop_front 丟掉最舊的（vector 做不到 O(1)）
// 上一頁 → docs.html
// 上一頁 → mail.html
// 下一頁 → docs.html
