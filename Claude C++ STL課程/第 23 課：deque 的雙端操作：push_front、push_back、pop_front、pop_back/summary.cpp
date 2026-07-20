// ============================================================
// 第 23 課 總結：deque 的雙端操作
// 編譯：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
// ============================================================
// 【四大雙端操作】
//   push_back(val)   尾端插入  O(1) 攤銷
//   push_front(val)  頭端插入  O(1) 攤銷
//   pop_back()       尾端刪除  O(1)
//   pop_front()      頭端刪除  O(1)
//
// 【存取首尾元素】
//   front()  回傳第一個元素的引用
//   back()   回傳最後一個元素的引用
//
// 【典型應用】
//   佇列（FIFO）：push_back 入列 + pop_front 出列
//   堆疊（LIFO）：push_back 入棧 + pop_back 出棧
//
// 【emplace_back vs push_back】
//   push_back(string("Hello"))  → 建構臨時物件 + 移動到容器
//   emplace_back("Hello")       → 直接在容器內建構（零拷貝零移動）
// ============================================================
//
// 【主題資訊 Information】
//   void push_front(const T&);  void push_front(T&&);
//   void push_back(const T&);   void push_back(T&&);
//   void pop_front();  void pop_back();          // 皆回傳 void
//   reference front();  reference back();
//   template <class... Args> reference emplace_front(Args&&...);   // C++11/17
//   template <class... Args> reference emplace_back(Args&&...);    // C++11/17
//
//   標頭檔：<deque>
//   複雜度：四個雙端操作皆為攤銷 O(1)；front()/back()/operator[] 為 O(1)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. deque 的分段連續結構——一切的根源】
//   deque 不是一整塊連續記憶體，而是：
//       中央控制器（map）：一個指標陣列，每格指向一塊固定大小的緩衝區
//       緩衝區（chunk）  ：真正存元素的地方，容量固定
//   本機 libstdc++ 的規則是 _GLIBCXX_DEQUE_BUF_SIZE = 512 位元組：
//       sizeof(T) < 512 → 每塊放 512 / sizeof(T) 個
//       sizeof(T) ≥ 512 → 每塊放 1 個
//   所以 deque<int> 每塊 128 個元素（同課 1.cpp 已用位址不連續處實測驗證）。
//   ⚠️ 這是 libstdc++ 的實作值；MSVC 的 STL 規則不同，標準完全沒有規定。
//
//   有了這個結構，前端插入只需要「頭端 chunk 還有空位就放，沒有就掛一塊新的」，
//   完全不搬移既有元素 → 這就是 push_front 為何能 O(1)。
//
// 【2. 一個容器，兩種角色】
//   FIFO 佇列：push_back 入列 + pop_front 出列
//   LIFO 堆疊：push_back 入棧 + pop_back  出棧
//   deque 同時提供兩端操作，所以一個容器就能扮演兩種角色。
//   這也是為什麼 std::stack 與 std::queue 的**預設底層容器都是 deque**
//   （第 27 課主題）：queue 需要 pop_front，而 vector 的前端刪除是 O(n)。
//
// 【3. pop 為什麼不回傳值】
//   pop_front() / pop_back() 都回傳 void。這是刻意的**例外安全**設計：
//   若 pop 同時回傳值，就必須複製建構一個 T；萬一該複製建構子丟出例外，
//   元素已被移除、值卻沒交到呼叫端手上，資料就永久遺失。
//   拆成 front()（看）+ pop_front()（移除）兩步，任一步失敗容器都還完整。
//
// 【4. emplace 系列省下了什麼】
//   push_back(string("Hello"))：建構臨時物件 → 移動進容器 → 臨時物件解構
//   emplace_back("Hello")     ：把 const char* 轉發給 string 建構子，就地建構
//   省下的是一次移動與一次解構（同課 2.cpp 已用計數器實測：
//   5 次 push_back 產生 5 次移動 + 5 次解構，5 次 emplace_back 則是 0 + 0）。
//   但若你傳入的是**現成的 T 物件**，兩者完全相同，此時 push_back 語意更清楚。
//
// 【概念補充 Concept Deep Dive】
//   ● iterator 失效規則：deque 與 vector 的關鍵差異
//     push_front / push_back → **所有 iterator 失效，但 reference 與 pointer
//     仍然有效**（既有元素沒被搬動）。
//     pop_front / pop_back → 只有被移除元素的 iterator/reference 失效。
//     vector 則是一擴容就把元素搬到新位址，reference/pointer 一起失效。
//     這個更強的保證，是「需要長期持有元素參考」時選 deque 的理由之一。
//
//   ● deque 沒有 reserve / capacity
//     因為它不需要一整塊連續空間，成長方式是「加掛新 chunk」，
//     所以標準沒有給它 reserve()，也沒有 capacity()。
//     想預留空間只能用 resize()，但那會真的建構元素。
//
//   ● 代價：存取比 vector 慢
//     deque 的 operator[] 要先算「第幾塊、塊內第幾格」，是兩次間接定址；
//     vector 只要一次加法。加上元素分散多塊，快取局部性也較差。
//     deque 是「用存取速度換兩端插入能力」，不是全面更好。
//
// 【注意事項 Pay Attention】
//   1. 對空 deque 呼叫 front/back/pop_front/pop_back 是 **UB**——
//      不保證崩潰、不保證任何特定值。務必先 empty() 檢查。
//   2. pop_front/pop_back 不回傳值；要取值請先 front()/back()。
//   3. push_front/push_back 使**所有 iterator 失效**，但 reference/pointer 有效。
//   4. chunk 大小 512 位元組是本機 libstdc++ 的實作值，不可依賴。
//   5. deque 沒有 reserve()／capacity()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的雙端操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 為什麼兩端插入都是 O(1)？它的內部結構長什麼樣？
//     答：分段連續——中央控制器（指標陣列）+ 多塊固定大小的緩衝區。
//         前端插入時，頭端 chunk 有空位就直接放，沒有就配置新 chunk
//         並把指標掛到控制器前端，完全不搬移既有元素。
//         本機 libstdc++ 每塊 512 位元組（deque<int> 為 128 個元素）。
//     追問：那 operator[] 怎麼實作？→ 先算出「第幾塊、塊內第幾格」，
//         再兩次間接定址取值。仍是 O(1)，但常數比 vector 的一次加法大。
//
// 🔥 Q2. std::queue 的預設底層容器為什麼是 deque 而不是 vector？
//     答：queue 需要 pop()（對應底層的 pop_front()）。
//         vector 的前端刪除要把全部元素往前搬一格，是 O(n)；
//         deque 的 pop_front 是 O(1)。所以 vector 根本不適合當 queue 的底層
//         （它甚至沒有 pop_front 這個成員函式，編譯就會失敗）。
//     追問：那 stack 呢？→ stack 只需要 push_back/pop_back/back，
//         vector 完全能勝任，可以寫 stack<int, vector<int>>。
//         預設仍選 deque 是因為 deque 擴容不必搬移全部既有元素。
//
// ⚠️ 陷阱. 「deque 兩端都 O(1)、還支援 operator[]，那不是完勝 vector 嗎？」
//     答：不是。deque 付出的代價是**存取速度與記憶體效率**：
//         operator[] 要多算一次「在第幾塊」並多一次間接定址；
//         元素分散在多塊記憶體，硬體預取器效果差、快取命中率低；
//         而且中央控制器本身也是額外的記憶體與一層間接。
//         純粹順序遍歷或隨機存取密集的工作，vector 明顯較快。
//     為什麼會錯：只比較複雜度表上的 O 符號，忽略了常數與快取效應。
//         正確的選法是：**預設 vector；只有真的需要前端 O(1) 插入刪除，
//         或需要「reference 不因插入而失效」的保證時，才換成 deque。**
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <string>
#include <vector>
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//   題目：判斷只含 ()[]{} 的字串括號是否正確配對。
//   為什麼用到本主題：這是 LIFO 堆疊最經典的題目，
//     而本課的 push_back / pop_back / back() 三個操作就是堆疊的全部所需。
//     這裡刻意直接用 deque（而非 std::stack 配接器）來解，
//     好讓你看見「配接器底下其實就是這三個雙端操作」——
//     這也正是第 27 課要展開的主題。
// -----------------------------------------------------------------------------
bool isValid(const string& s) {
    deque<char> stk;                     // 當作堆疊用：尾端進、尾端出
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            stk.push_back(c);            // 左括號入棧
        } else {
            if (stk.empty()) return false;          // 先檢查，避免對空容器 back()
            char top = stk.back();
            if ((c == ')' && top != '(') ||
                (c == ']' && top != '[') ||
                (c == '}' && top != '{')) {
                return false;
            }
            stk.pop_back();              // 配對成功才彈出
        }
    }
    return stk.empty();                  // 全部配對完才算合法
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定長度的即時指標視窗（monitoring sliding window）
//   情境：監控系統每秒收到一筆延遲數值，只保留「最近 N 秒」的資料，
//         用來算即時平均與峰值。
//         需要「尾端加入最新、頭端丟棄最舊」——正是 deque 的雙端操作，
//         而且兩邊都是 O(1)。用 vector 的話 pop_front 是 O(n)，
//         在每秒都要做一次的監控迴圈裡會累積成明顯負擔。
// -----------------------------------------------------------------------------
class LatencyWindow {
    deque<int> samples_;
    size_t capacity_;
public:
    explicit LatencyWindow(size_t cap) : capacity_(cap) {}

    void add(int latencyMs) {
        samples_.push_back(latencyMs);          // 尾端加入最新
        if (samples_.size() > capacity_) {
            samples_.pop_front();               // 頭端丟棄最舊 ← deque 的價值
        }
    }
    double average() const {
        if (samples_.empty()) return 0.0;
        long long sum = 0;
        for (int v : samples_) sum += v;
        return static_cast<double>(sum) / static_cast<double>(samples_.size());
    }
    int peak() const {
        int m = 0;
        for (int v : samples_) if (v > m) m = v;
        return m;
    }
    size_t size() const { return samples_.size(); }
    int oldest() const { return samples_.front(); }
    int newest() const { return samples_.back(); }
};

void print(const string& label, const deque<int>& dq) {
    cout << "  " << label << ": ";
    if (dq.empty()) {
        cout << "(空)";
    } else {
        cout << "front=" << dq.front() << " back=" << dq.back() << " | ";
        for (int val : dq) cout << val << " ";
    }
    cout << " (size=" << dq.size() << ")" << endl;
}

int main() {
    // ============================================================
    // 1. 基本雙端操作
    // ============================================================
    cout << "===== 1. 基本雙端操作 =====\n";
    deque<int> dq;
    print("初始狀態    ", dq);

    dq.push_back(10);
    dq.push_back(20);
    dq.push_back(30);
    print("push_back ×3", dq);

    dq.push_front(5);
    dq.push_front(1);
    print("push_front×2", dq);
    // 1 5 10 20 30

    dq.pop_front();
    print("pop_front   ", dq);
    // 5 10 20 30

    dq.pop_back();
    print("pop_back    ", dq);
    // 5 10 20
    cout << "\n";

    // ============================================================
    // 2. 模擬佇列（FIFO：先進先出）
    // ============================================================
    cout << "===== 2. 模擬佇列 (FIFO) =====\n";
    deque<int> queue;

    // 入列（從尾端加入）
    for (int i = 1; i <= 5; i++) {
        queue.push_back(i * 100);
        cout << "  入列 " << i * 100 << endl;
    }
    // 佇列：100 200 300 400 500

    // 出列（從頭端取出）
    cout << "  出列順序：";
    while (!queue.empty()) {
        cout << queue.front() << " ";
        queue.pop_front();
    }
    cout << "\n\n";
    // 出列順序：100 200 300 400 500

    // ============================================================
    // 3. 模擬堆疊（LIFO：後進先出）
    // ============================================================
    cout << "===== 3. 模擬堆疊 (LIFO) =====\n";
    deque<int> stack;

    for (int i = 1; i <= 5; i++) {
        stack.push_back(i * 10);
        cout << "  push " << i * 10 << endl;
    }

    cout << "  pop 順序：";
    while (!stack.empty()) {
        cout << stack.back() << " ";
        stack.pop_back();
    }
    cout << "\n\n";
    // pop 順序：50 40 30 20 10

    // ============================================================
    // 4. emplace_back vs push_back
    // ============================================================
    cout << "===== 4. emplace_back vs push_back =====\n";
    deque<string> names;

    // push_back：先建構臨時 string，再移動到 deque
    names.push_back(string("Alice"));

    // emplace_back：直接在 deque 內部建構（更高效）
    names.emplace_back("Bob");

    for (const auto& n : names) cout << "  " << n << "\n";
    cout << "  emplace_back 省去了臨時物件的建構和移動\n";
    cout << "  （同課 2.cpp 用計數器實測：5 次 push_back 產生 5 次移動 + 5 次解構，\n";
    cout << "    5 次 emplace_back 則是 0 次移動 + 0 次解構）\n";
    cout << "\n";

    // ============================================================
    // 5. iterator 失效 vs reference 有效（deque 的獨門保證）
    // ============================================================
    cout << "===== 5. iterator 失效，但 reference 仍有效 =====\n";
    {
        deque<int> d = {100, 200, 300};
        int& ref = d[1];                  // 取得中間元素的 reference
        cout << "  push_front 前 ref = " << ref << "\n";
        d.push_front(50);                 // 頭端插入
        cout << "  push_front 後 ref = " << ref
             << " ← reference 仍然有效（deque 不搬移既有元素）\n";
        cout << "  但所有 iterator 已失效，必須重新取得\n";
        cout << "  內容：";
        for (int x : d) cout << x << " ";
        cout << "\n";
        cout << "  ⚠️ vector 沒有這個保證：它擴容時元素真的被搬到新位址\n";
    }
    cout << "\n";

    // ============================================================
    // 6. 實測 chunk 大小（分段連續的直接證據）
    // ============================================================
    cout << "===== 6. 實測 chunk 大小 =====\n";
    {
        deque<int> probe;
        for (int i = 0; i < 600; ++i) probe.push_back(i);
        int breaks = 0, firstBreak = -1;
        for (size_t i = 1; i < probe.size(); ++i) {
            if (&probe[i] != &probe[i - 1] + 1) {
                if (firstBreak < 0) firstBreak = static_cast<int>(i);
                ++breaks;
            }
        }
        cout << "  放入 600 個 int，位址不連續處共 " << breaks << " 個\n";
        cout << "  第一個斷點在索引 " << firstBreak
             << " → 每塊 chunk 放 " << firstBreak << " 個 int，即 "
             << firstBreak * static_cast<int>(sizeof(int)) << " 位元組\n";
        cout << "  與 libstdc++ 的 _GLIBCXX_DEQUE_BUF_SIZE = 512 相符\n";
        cout << "  ⚠️ 這是實作值，MSVC 規則不同，標準沒有規定\n";
    }
    cout << "\n";

    // ============================================================
    // LeetCode 20. Valid Parentheses
    // ============================================================
    cout << "===== LeetCode 20. Valid Parentheses =====\n";
    const vector<string> cases = {"()", "()[]{}", "(]", "([)]", "{[]}", ""};
    for (const string& c : cases) {
        cout << "  \"" << c << "\" → " << (isValid(c) ? "true" : "false") << "\n";
    }
    cout << "  → 只用到 push_back / back() / pop_back 三個操作，\n";
    cout << "    這正是 std::stack 配接器底下做的事（第 27 課主題）\n\n";

    // ============================================================
    // 日常實務：即時延遲監控視窗
    // ============================================================
    cout << "===== 日常實務：最近 N 秒的延遲監控視窗 =====\n";
    {
        LatencyWindow win(5);                       // 只保留最近 5 筆
        const int readings[] = {12, 15, 98, 22, 17, 31, 25, 19};
        for (int r : readings) {
            win.add(r);
            cout << "  收到 " << r << " ms → 視窗大小 " << win.size()
                 << "，平均 " << win.average()
                 << "，峰值 " << win.peak() << "\n";
        }
        cout << "  視窗內最舊 = " << win.oldest()
             << "，最新 = " << win.newest() << "\n";
        cout << "  → 尾端 push_back 加入最新、頭端 pop_front 丟棄最舊，兩者皆 O(1)；\n";
        cout << "    用 vector 的話 pop_front 是 O(n)，在每秒執行的監控迴圈裡會累積成負擔。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 1. 基本雙端操作 =====
//   初始狀態    : (空) (size=0)
//   push_back ×3: front=10 back=30 | 10 20 30  (size=3)
//   push_front×2: front=1 back=30 | 1 5 10 20 30  (size=5)
//   pop_front   : front=5 back=30 | 5 10 20 30  (size=4)
//   pop_back    : front=5 back=20 | 5 10 20  (size=3)
//
// ===== 2. 模擬佇列 (FIFO) =====
//   入列 100
//   入列 200
//   入列 300
//   入列 400
//   入列 500
//   出列順序：100 200 300 400 500
//
// ===== 3. 模擬堆疊 (LIFO) =====
//   push 10
//   push 20
//   push 30
//   push 40
//   push 50
//   pop 順序：50 40 30 20 10
//
// ===== 4. emplace_back vs push_back =====
//   Alice
//   Bob
//   emplace_back 省去了臨時物件的建構和移動
//   （同課 2.cpp 用計數器實測：5 次 push_back 產生 5 次移動 + 5 次解構，
//     5 次 emplace_back 則是 0 次移動 + 0 次解構）
//
// ===== 5. iterator 失效，但 reference 仍有效 =====
//   push_front 前 ref = 200
//   push_front 後 ref = 200 ← reference 仍然有效（deque 不搬移既有元素）
//   但所有 iterator 已失效，必須重新取得
//   內容：50 100 200 300
//   ⚠️ vector 沒有這個保證：它擴容時元素真的被搬到新位址
//
// ===== 6. 實測 chunk 大小 =====
//   放入 600 個 int，位址不連續處共 4 個
//   第一個斷點在索引 128 → 每塊 chunk 放 128 個 int，即 512 位元組
//   與 libstdc++ 的 _GLIBCXX_DEQUE_BUF_SIZE = 512 相符
//   ⚠️ 這是實作值，MSVC 規則不同，標準沒有規定
//
// ===== LeetCode 20. Valid Parentheses =====
//   "()" → true
//   "()[]{}" → true
//   "(]" → false
//   "([)]" → false
//   "{[]}" → true
//   "" → true
//   → 只用到 push_back / back() / pop_back 三個操作，
//     這正是 std::stack 配接器底下做的事（第 27 課主題）
//
// ===== 日常實務：最近 N 秒的延遲監控視窗 =====
//   收到 12 ms → 視窗大小 1，平均 12，峰值 12
//   收到 15 ms → 視窗大小 2，平均 13.5，峰值 15
//   收到 98 ms → 視窗大小 3，平均 41.6667，峰值 98
//   收到 22 ms → 視窗大小 4，平均 36.75，峰值 98
//   收到 17 ms → 視窗大小 5，平均 32.8，峰值 98
//   收到 31 ms → 視窗大小 5，平均 36.6，峰值 98
//   收到 25 ms → 視窗大小 5，平均 38.6，峰值 98
//   收到 19 ms → 視窗大小 5，平均 22.8，峰值 31
//   視窗內最舊 = 22，最新 = 19
//   → 尾端 push_back 加入最新、頭端 pop_front 丟棄最舊，兩者皆 O(1)；
//     用 vector 的話 pop_front 是 O(n)，在每秒執行的監控迴圈裡會累積成負擔。
