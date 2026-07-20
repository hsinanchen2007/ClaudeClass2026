// =============================================================================
//  第 27 課：deque 作為 stack/queue 底層容器 3  —  自己實作配接器
// =============================================================================
//
// 【主題資訊 Information】
//   template <typename T, typename Container = std::deque<T>>
//   class MyStack;
//
//   標頭檔：<deque>、<vector>、<utility>（std::forward）
//   複雜度：所有操作都直接委託底層容器，因此與底層同階
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 配接器的全部祕密：一個成員 + 幾行轉呼叫】
//   看完這個 MyStack 你就完全理解 std::stack 了——它真的就這麼簡單：
//       Container c;                       // 唯一的資料成員
//       void push(const T& v) { c.push_back(v); }
//       void pop()            { c.pop_back(); }
//       T&   top()            { return c.back(); }
//   沒有任何記憶體管理、沒有任何演算法。
//   配接器的價值不在「做了什麼」，而在「**不讓你做什麼**」。
//
// 【2. 預設模板參數：Container = std::deque<T>】
//   template <typename T, typename Container = deque<T>>
//   這一行讓 MyStack<int> 自動等於 MyStack<int, deque<int>>，
//   同時保留 MyStack<int, vector<int>> 的彈性。
//   注意預設值依賴前一個模板參數 T——這在 C++ 是合法的，
//   模板參數由左至右可見，就像函式的預設參數一樣。
//
// 【3. 為什麼要有 emplace：完美轉發】
//   template <typename... Args>
//   void emplace(Args&&... args) { c.emplace_back(std::forward<Args>(args)...); }
//   三個要素缺一不可：
//       Args&&...                 → 轉發參考，能同時接住左值與右值
//       std::forward<Args>(args)  → 還原每個參數原本的值類別
//       ...                       → 參數包展開
//   若寫成 c.emplace_back(args...)（少了 forward），
//   args 在函式內部都是**具名的左值**，右值參數會退化成複製而非移動。
//   本檔第 3 節會用計數器實測這個差異。
//
// 【4. 刻意不提供的東西】
//   MyStack 沒有 begin()/end()、沒有 operator[]、沒有 insert/erase。
//   這不是偷懶，是設計。少了這些，任何讀到 MyStack 的人都能確定
//   「這個資料結構只會 LIFO 存取」，而且是**編譯器保證**的，
//   不是靠註解或口頭約定。
//
// 【概念補充 Concept Deep Dive】
//   ● const 版本的 top() 為什麼需要
//     T& top() 是非 const 成員函式，對 const MyStack 物件無法呼叫。
//     所以要成對提供 const T& top() const。
//     這是 C++ 的 const 正確性（const-correctness）基本功：
//     任何「只讀」的存取器都應該有 const 版本，否則你的類別無法用在
//     const 參考的情境（例如當成 const& 參數傳入函式）。
//
//   ● 為什麼 size() 回傳 size_t 而不是 int
//     容器的大小天生非負，用無號型別表達語意更精確，也與 STL 一致。
//     實務上建議寫成 typename Container::size_type，
//     這樣換底層容器時型別自動跟著走（本檔第 5 節示範）。
//
//   ● 真正的 std::stack 還多了什麼
//     * 六個比較運算子（C++20 起用 operator<=> 一次搞定）
//     * swap、allocator 相關建構子
//     * 成員 c 宣告為 protected，允許繼承後存取
//     * C++17 起 emplace 回傳 reference
//     核心邏輯則與本檔的 MyStack 完全相同。
//
// 【注意事項 Pay Attention】
//   1. 底層容器必須提供 push_back / pop_back / back / empty / size，
//      否則相關成員函式**在被呼叫時**才會編譯失敗（模板延遲實例化）。
//   2. emplace 一定要用 std::forward，否則右值會退化成複製。
//   3. top() 要提供 const 與非 const 兩個版本。
//   4. 對空容器呼叫 top()/pop() 是 UB——配接器不會替你檢查。
//   5. 配接器沒有虛擬解構子，不要當多型基底類別用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】自製容器配接器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請說明 std::stack 的實作原理，並手寫一個簡化版。
//     答：它是配接器，內部只有一個底層容器成員，所有操作都委託出去：
//         push→push_back、pop→pop_back、top→back、empty→empty、size→size。
//         模板宣告是 template <class T, class Container = std::deque<T>>，
//         關鍵在於**刻意不提供** begin/end/operator[]，用型別強制 LIFO 語意。
//     追問：那 stack 自己有沒有管理記憶體？→ 完全沒有。
//         它的所有成本與行為都由底層容器決定，本身只是一層 inline 的轉呼叫。
//
// 🔥 Q2. emplace 為什麼必須寫 std::forward<Args>(args)...？
//     答：Args&&... 是轉發參考，能接住左值與右值。但一旦進入函式本體，
//         args 就變成**具名的左值**——直接傳下去會全部以左值傳遞，
//         右值參數就退化成複製而非移動。
//         std::forward<Args>(args) 依照推導出的 Args 還原原本的值類別，
//         右值才會保持右值、繼續觸發移動建構。
//     追問：std::forward 和 std::move 差在哪？→ std::move 無條件轉成右值；
//         std::forward 是**有條件**的：Args 推導為左值參考時保持左值，
//         否則轉成右值。所以 forward 只能用在轉發參考的場景。
//
// ⚠️ 陷阱. 「配接器不提供 begin()/end()，那我需要遍歷時，
//         繼承它再存取 protected 的 c 就好了吧？」
//     答：技術上可行（std::stack 的 c 確實是 protected，這也是刻意的），
//         但這通常是**選錯容器**的信號。你需要遍歷，代表你要的其實不是堆疊，
//         而是一個「有時當堆疊用」的序列——那就直接用 deque 或 vector。
//         另外 std::stack 沒有虛擬解構子，繼承它做多型會出問題。
//     為什麼會錯：把「介面限制」當成需要繞過的障礙，
//         而不是當成設計決策的表達。配接器的整個價值就在那些「缺少」的函式上；
//         繞過它們等於自願放棄型別帶來的保證，卻仍付出配接器的間接成本。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <list>
#include <string>
#include <utility>
using namespace std;

// -----------------------------------------------------------------------------
// 儀器型別：觀察 emplace 有沒有真的做到完美轉發
// -----------------------------------------------------------------------------
struct Tracer {
    static int copies;
    static int moves;
    static void reset() { copies = moves = 0; }
    string name;
    explicit Tracer(string n = "") : name(std::move(n)) {}
    Tracer(const Tracer& o) : name(o.name) { ++copies; }
    Tracer(Tracer&& o) noexcept : name(std::move(o.name)) { ++moves; }
    Tracer& operator=(const Tracer& o) { name = o.name; ++copies; return *this; }
    Tracer& operator=(Tracer&& o) noexcept { name = std::move(o.name); ++moves; return *this; }
};
int Tracer::copies = 0;
int Tracer::moves = 0;

template <typename T, typename Container = deque<T>>
class MyStack {
private:
    Container c;    // 唯一的資料成員

public:
    // 讓型別別名跟著底層容器走（換底層時自動正確）
    using container_type = Container;
    using value_type     = typename Container::value_type;
    using size_type      = typename Container::size_type;
    using reference      = typename Container::reference;

    // push：委託給底層容器的 push_back
    void push(const T& value) {
        c.push_back(value);
    }
    // 右值版本：讓呼叫端能用 std::move 省下一次複製
    void push(T&& value) {
        c.push_back(std::move(value));
    }

    // emplace：委託給底層容器的 emplace_back
    // 三要素：Args&&（轉發參考）+ std::forward（還原值類別）+ ...（參數包展開）
    template <typename... Args>
    void emplace(Args&&... args) {
        c.emplace_back(std::forward<Args>(args)...);
    }

    // pop：委託給底層容器的 pop_back
    void pop() {
        c.pop_back();
    }

    // top：委託給底層容器的 back
    reference top() {
        return c.back();
    }

    // const 版本：讓 const MyStack 物件也能讀取（const 正確性）
    const T& top() const {
        return c.back();
    }

    bool empty() const { return c.empty(); }
    size_type size() const { return c.size(); }

    // 配接器不提供迭代器！
    // 不提供 begin(), end()
    // 不提供 operator[]
    // 這就是「限制介面」的意義
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計支援 push / pop / top，以及 O(1) 取最小值 getMin 的堆疊。
//   為什麼用到本主題：這題本身就是在考「設計一個配接器」——
//     用既有的容器組合出一個介面受限、但多了一項保證（O(1) getMin）的資料結構。
//     這裡刻意用我們自製的 MyStack 當底層，證明它真的能用；
//     同時示範配接器可以層層堆疊（MinStack 配接 MyStack，MyStack 配接 deque）。
// -----------------------------------------------------------------------------
class MinStack {
    MyStack<int> data_;   // 主堆疊（底層又是 deque）
    MyStack<int> mins_;   // 輔助堆疊：top() 永遠是目前最小值
public:
    void push(int val) {
        data_.push(val);
        mins_.push(mins_.empty() ? val : min(val, mins_.top()));
    }
    void pop() {
        if (data_.empty()) return;      // 先檢查，避免 UB
        data_.pop();
        mins_.pop();
    }
    int top() const    { return data_.top(); }
    int getMin() const { return mins_.top(); }
    bool empty() const { return data_.empty(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】限制介面的「操作歷史」——只能記錄與回退，不能竄改
//   情境：實作一個交易系統的操作日誌。業務規則是：
//         只能「附加新紀錄」與「回退最後一筆」，
//         **絕對不允許修改或刪除中間的任何一筆**（稽核要求）。
//   若直接用 vector，任何人都能寫 log[3] = ... 把中間那筆改掉，
//   而且 code review 不一定看得出來。
//   包成配接器之後，「不能竄改」變成**編譯器強制**的保證。
// -----------------------------------------------------------------------------
struct AuditEntry {
    string action;
    int amount;
    AuditEntry(string a, int amt) : action(std::move(a)), amount(amt) {}
};

class AuditLog {
    MyStack<AuditEntry> entries_;
    size_t totalEver_ = 0;
public:
    template <typename... Args>
    void record(Args&&... args) {           // 完美轉發到 AuditEntry 的建構子
        entries_.emplace(std::forward<Args>(args)...);
        ++totalEver_;
    }
    bool rollbackLast() {
        if (entries_.empty()) return false;
        entries_.pop();
        return true;
    }
    const AuditEntry& latest() const { return entries_.top(); }
    size_t depth() const { return entries_.size(); }
    size_t totalEver() const { return totalEver_; }
    // 刻意沒有 operator[]、沒有 begin()/end()、沒有 erase
    // → 稽核紀錄無法被竄改，這是型別層級的保證
};

int main() {
    cout << "=== 1. 自製配接器的基本運作 ===" << endl;
    // 用預設底層（deque）
    MyStack<int> s1;
    s1.push(10);
    s1.push(20);
    s1.push(30);

    cout << "逐一取出：";
    while (!s1.empty()) {
        cout << s1.top() << " ";
        s1.pop();
    }
    cout << endl;
    // 輸出：30 20 10

    cout << "\n=== 2. 換底層容器 ===" << endl;
    // 用 vector 作為底層
    MyStack<int, vector<int>> s2;
    s2.push(100);
    s2.push(200);
    cout << "vector 底層 top: " << s2.top() << endl;  // 200

    // 用 list 作為底層也可以（它同樣有 push_back/pop_back/back）
    MyStack<int, list<int>> s3;
    s3.push(1); s3.push(2); s3.push(3);
    cout << "list   底層 top: " << s3.top() << ", size = " << s3.size() << endl;
    cout << "→ 三種底層、同一套介面；差別只在效能特性" << endl;

    cout << "\n=== 3. 實測：emplace 的完美轉發真的有效 ===" << endl;
    Tracer::reset();
    {
        MyStack<Tracer> st;
        Tracer named("named-object");
        st.push(named);                      // 左值 → 應該複製
    }
    cout << "push(左值)      → 複製 " << Tracer::copies
         << " 次、移動 " << Tracer::moves << " 次" << endl;

    Tracer::reset();
    {
        MyStack<Tracer> st;
        Tracer named("named-object");
        st.push(std::move(named));           // 右值 → 應該移動
    }
    cout << "push(std::move) → 複製 " << Tracer::copies
         << " 次、移動 " << Tracer::moves << " 次" << endl;

    Tracer::reset();
    {
        MyStack<Tracer> st;
        st.emplace("built-in-place");        // 就地建構 → 零複製零移動
    }
    cout << "emplace(原料)   → 複製 " << Tracer::copies
         << " 次、移動 " << Tracer::moves << " 次" << endl;
    cout << "→ emplace 把參數直接轉發給 Tracer 的建構子，連臨時物件都沒產生" << endl;
    cout << "（這組數字每次執行完全相同，可重現）" << endl;

    cout << "\n=== 4. const 正確性：const 物件也能讀 ===" << endl;
    MyStack<string> ms;
    ms.push("hello");
    ms.push("world");
    const MyStack<string>& cref = ms;        // 綁成 const 參考
    cout << "透過 const 參考讀 top() = " << cref.top()
         << "，size = " << cref.size() << endl;
    cout << "→ 若少寫了 const 版的 top()，這一行會編譯失敗" << endl;

    cout << "\n=== 5. 型別別名跟著底層容器走 ===" << endl;
    cout << "MyStack<int> 的 size_type 是無號型別嗎？ " << boolalpha
         << is_unsigned_v<MyStack<int>::size_type> << endl;
    cout << "MyStack<int, vector<int>> 的 container_type 是 vector<int> 嗎？ "
         << is_same_v<MyStack<int, vector<int>>::container_type, vector<int>> << endl;

    cout << "\n=== 6. 配接器「拿掉」的東西（編譯期保證）===" << endl;
    cout << "以下三行若取消註解都會編譯失敗——這正是設計目的：" << endl;
    cout << "  // for (int x : s1) ...       ← 沒有 begin/end" << endl;
    cout << "  // s1[0];                     ← 沒有 operator[]" << endl;
    cout << "  // s1.erase(...);             ← 沒有 erase" << endl;

    cout << "\n=== LeetCode 155. Min Stack ===" << endl;
    MinStack mst;
    mst.push(-2);  cout << "push(-2) → getMin = " << mst.getMin() << endl;
    mst.push(0);   cout << "push(0)  → getMin = " << mst.getMin() << endl;
    mst.push(-3);  cout << "push(-3) → getMin = " << mst.getMin() << endl;
    cout << "getMin() = " << mst.getMin() << endl;
    mst.pop();
    cout << "pop() → top = " << mst.top() << ", getMin = " << mst.getMin() << endl;
    cout << "→ 配接器可以層層堆疊：MinStack 配接 MyStack，MyStack 配接 deque" << endl;

    cout << "\n=== 日常實務：不可竄改的稽核日誌 ===" << endl;
    AuditLog log;
    log.record("開戶", 0);
    log.record("存款", 50000);
    log.record("轉帳", -12000);
    log.record("提款", -3000);
    cout << "共記錄 " << log.totalEver() << " 筆，目前深度 " << log.depth() << endl;
    cout << "最新一筆：" << log.latest().action
         << " " << log.latest().amount << endl;

    log.rollbackLast();
    cout << "回退最後一筆後，最新是：" << log.latest().action
         << " " << log.latest().amount
         << "（深度 " << log.depth() << "）" << endl;
    cout << "→ 沒有 operator[]、沒有 erase、沒有迭代器：" << endl;
    cout << "  「稽核紀錄不可竄改」從口頭約定變成編譯器強制的保證。" << endl;
    cout << "  若直接用 vector，任何人都能寫 log[1].amount = 0 而不被 review 發現。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 27 課：deque 作為 stackqueue 底層容器3.cpp" -o demo27_3

// === 預期輸出 ===
// === 1. 自製配接器的基本運作 ===
// 逐一取出：30 20 10
//
// === 2. 換底層容器 ===
// vector 底層 top: 200
// list   底層 top: 3, size = 3
// → 三種底層、同一套介面；差別只在效能特性
//
// === 3. 實測：emplace 的完美轉發真的有效 ===
// push(左值)      → 複製 1 次、移動 0 次
// push(std::move) → 複製 0 次、移動 1 次
// emplace(原料)   → 複製 0 次、移動 0 次
// → emplace 把參數直接轉發給 Tracer 的建構子，連臨時物件都沒產生
// （這組數字每次執行完全相同，可重現）
//
// === 4. const 正確性：const 物件也能讀 ===
// 透過 const 參考讀 top() = world，size = 2
// → 若少寫了 const 版的 top()，這一行會編譯失敗
//
// === 5. 型別別名跟著底層容器走 ===
// MyStack<int> 的 size_type 是無號型別嗎？ true
// MyStack<int, vector<int>> 的 container_type 是 vector<int> 嗎？ true
//
// === 6. 配接器「拿掉」的東西（編譯期保證）===
// 以下三行若取消註解都會編譯失敗——這正是設計目的：
//   // for (int x : s1) ...       ← 沒有 begin/end
//   // s1[0];                     ← 沒有 operator[]
//   // s1.erase(...);             ← 沒有 erase
//
// === LeetCode 155. Min Stack ===
// push(-2) → getMin = -2
// push(0)  → getMin = -2
// push(-3) → getMin = -3
// getMin() = -3
// pop() → top = 0, getMin = -2
// → 配接器可以層層堆疊：MinStack 配接 MyStack，MyStack 配接 deque
//
// === 日常實務：不可竄改的稽核日誌 ===
// 共記錄 4 筆，目前深度 4
// 最新一筆：提款 -3000
// 回退最後一筆後，最新是：轉帳 -12000（深度 3）
// → 沒有 operator[]、沒有 erase、沒有迭代器：
//   「稽核紀錄不可竄改」從口頭約定變成編譯器強制的保證。
//   若直接用 vector，任何人都能寫 log[1].amount = 0 而不被 review 發現。
