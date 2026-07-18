// ============================================================================
//  07_default_template_arguments.cpp  ──  預設模板引數
// ============================================================================
//
//  【本篇目標】
//    Template 參數可以有「預設值」(default template argument)。這像 function
//    參數的預設值，但有一些特殊規則。
//
//  【動機】
//    讓使用者不必每次都把所有型別寫滿。例如：
//
//        std::vector<int>            v1;          // Allocator 用 std::allocator<int>
//        std::map<int,int>           m1;          // Compare 用 std::less<int>
//        std::priority_queue<int>    pq1;         // Container 用 std::vector<int>
//
//    這些 default 都是 template 預設參數做到的。
//
//  【語法】
//
//        template <typename T, typename Compare = std::less<T>>
//        class SortedSet { ... };
//
//        SortedSet<int>                   a;        // Compare = std::less<int>
//        SortedSet<int, std::greater<int>> b;       // 自訂 compare
//
//    對 function template 一樣可以 (C++11 起)：
//
//        template <typename T = int>
//        T zero() { return T{}; }
//
//        zero();         // 回傳 int 0
//        zero<double>(); // 回傳 double 0
//
//  【規則】
//    1. 預設值只能在「右邊」連續省略，不能跳。Class template 比 function
//       template 嚴一點：function template 因為有引數推導，可以從引數推
//       出後面參數，所以中間有預設值有時也可以。
//    2. 預設值可以引用前面的參數：
//          template<typename T, typename Alloc = std::allocator<T>>
//       這是 STL 容器的標準寫法。
//    3. 預設值「不能在不同 declaration 同時提供」(同一個參數)；但允許多
//       個 declaration 各補一段預設值，最終必須一致。
//    4. C++11 起 function template 也可以有預設參數 (C++03 不行)。
//
//  參考：https://en.cppreference.com/cpp/language/template_parameters
// ============================================================================

/*
補充筆記：default_template_arguments
  - default_template_arguments 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - default_template_arguments 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <deque>
#include <functional>
#include <iostream>
#include <stack>
#include <vector>

// ─── 1. 預設參數可以引用前面的參數：類似 STL 容器 ─────────────────────────
//   T 是元素型別、Container 預設成 std::vector<T>。
template <typename T, typename Container = std::vector<T>>
class MyStack {
public:
    void push(const T& v) { c_.push_back(v); }
    void pop()             { c_.pop_back();   }
    const T& top() const   { return c_.back(); }
    bool empty() const     { return c_.empty(); }
    std::size_t size() const { return c_.size(); }
private:
    Container c_;
};

// ─── 2. 預設 Compare：自製 SortedSet ──────────────────────────────────────
//   插入時保持有序；compare 預設 std::less<T> (升序)，可換成 std::greater<T>。
//   這個範例只是教學，效能不如 std::set；正式專案請用標準庫。
template <typename T, typename Compare = std::less<T>>
class SortedVec {
public:
    void insert(const T& v) {
        auto pos = data_.begin();
        while (pos != data_.end() && cmp_(*pos, v)) ++pos;   // 找插入點
        data_.insert(pos, v);
    }
    const std::vector<T>& data() const { return data_; }
private:
    std::vector<T> data_;
    Compare cmp_{};
};

// ─── 3. Leetcode 232 ── Implement Queue using Stacks (template 化) ────────
//   題目：用兩個 stack 實作一個 queue (FIFO)。push/pop/peek/empty。
//   amortized O(1)：把 in_ 倒到 out_，每元素只搬一次。
//
//   為什麼這題用 template + 預設參數？
//     原題用 int。把它做成 MyQueue<T, Container = std::stack<T>>，使用者
//     如果有自己的 stack 想替換 (例如自己實作的 lock-free stack)，可以直接
//     傳進來。
//
//   時間複雜度：push O(1)，pop/peek amortized O(1) (worst O(n) 時偶發轉移)。
//   空間複雜度：O(n)。
//   邊界：empty 時 pop / peek → 行為未定義 (可改丟例外)。
template <typename T, typename Container = std::stack<T>>
class MyQueue {
public:
    void push(const T& v) { in_.push(v); }

    T pop() {
        ensure_out_filled();
        T v = out_.top();
        out_.pop();
        return v;
    }

    const T& peek() {
        ensure_out_filled();
        return out_.top();
    }

    bool empty() const { return in_.empty() && out_.empty(); }
    std::size_t size() const { return in_.size() + out_.size(); }

private:
    Container in_;
    Container out_;

    void ensure_out_filled() {
        if (out_.empty()) {
            while (!in_.empty()) {
                out_.push(in_.top());
                in_.pop();
            }
        }
    }
};

// ─── 4. function template 也可以有預設參數 ──────────────────────────────
//   範例：建構一個 T 的零值。預設 T = int 讓 zero() 可以無參數呼叫。
template <typename T = int>
T zero() { return T{}; }

// ─── 5. Leetcode 933 ── Number of Recent Calls (預設容器 = std::deque) ───
//   難度: easy
//   題目：實作 RecentCounter，每次 ping(t) 回傳「過去 3000ms 內」(含 t)
//        共有幾次 ping。t 是嚴格遞增的時間戳。
//   範例：ping(1)=1; ping(100)=2; ping(3001)=3; ping(3002)=3
//
//   解法：用一個 queue 保留 [t-3000, t] 內的 ping，每次 pop 過期者。
//   時間：amortized O(1)；空間：O(window)。
//
//   為什麼放在這裡？
//     用 template + 預設容器：使用者可以決定底層用 std::deque、std::list 等。
template <typename T = int, typename Container = std::deque<T>>
class RecentCounter {
public:
    int ping(T t) {
        q_.push_back(t);
        while (!q_.empty() && q_.front() < t - 3000) q_.pop_front();
        return static_cast<int>(q_.size());
    }
private:
    Container q_;
};

// ─── 6. 工作實用：通用 KeyValueStore (預設 Compare、預設 Allocator) ──────
//   類似簡化版 std::map：使用者只需要寫 KeyValueStore<int, std::string>，
//   進階使用者可換 Compare 為 std::greater 把順序倒過來。
#include <map>
template <typename K, typename V,
          typename Compare = std::less<K>>
class KeyValueStore {
public:
    void set(const K& k, const V& v) { data_[k] = v; }
    V get(const K& k, V fallback = V{}) const {
        auto it = data_.find(k);
        return it == data_.end() ? fallback : it->second;
    }
    std::size_t size() const { return data_.size(); }
    auto begin() const { return data_.begin(); }
    auto end()   const { return data_.end();   }
private:
    std::map<K, V, Compare> data_;
};

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) MyStack 使用預設容器
    MyStack<int> s1;
    s1.push(1); s1.push(2); s1.push(3);
    std::cout << "MyStack default top = " << s1.top() << "\n";

    // 換成 deque 當底層容器 (deque 也支援 push_back / pop_back / back)
    MyStack<int, std::deque<int>> s2;
    s2.push(10); s2.push(20);
    std::cout << "MyStack<int, deque> top = " << s2.top() << "\n";

    // (2) SortedVec
    SortedVec<int> asc;
    for (int x : {3, 1, 4, 1, 5, 9, 2, 6}) asc.insert(x);
    std::cout << "asc: ";
    for (int x : asc.data()) std::cout << x << ' ';
    std::cout << "\n";

    SortedVec<int, std::greater<int>> desc;
    for (int x : {3, 1, 4, 1, 5, 9, 2, 6}) desc.insert(x);
    std::cout << "desc: ";
    for (int x : desc.data()) std::cout << x << ' ';
    std::cout << "\n";

    // (3) Leetcode 232
    MyQueue<int> q;
    q.push(1); q.push(2); q.push(3);
    std::cout << "Queue peek = " << q.peek() << " (expect 1)\n";
    std::cout << "Queue pop  = " << q.pop()  << " (expect 1)\n";
    std::cout << "Queue pop  = " << q.pop()  << " (expect 2)\n";
    q.push(4);
    std::cout << "Queue pop  = " << q.pop()  << " (expect 3)\n";
    std::cout << "Queue pop  = " << q.pop()  << " (expect 4)\n";
    std::cout << "Queue empty = " << std::boolalpha << q.empty() << "\n";

    // (4) function template 預設參數
    std::cout << "zero()             = " << zero()         << "\n";  // int
    std::cout << "zero<double>()     = " << zero<double>() << "\n";  // double 0.0

    // (5) Leetcode 933 RecentCounter
    RecentCounter<> rc;
    std::cout << "ping(1)    = " << rc.ping(1)    << " (expect 1)\n";
    std::cout << "ping(100)  = " << rc.ping(100)  << " (expect 2)\n";
    std::cout << "ping(3001) = " << rc.ping(3001) << " (expect 3)\n";
    std::cout << "ping(3002) = " << rc.ping(3002) << " (expect 3)\n";

    // (6) KeyValueStore 預設 Compare = std::less
    KeyValueStore<int, std::string> kv;
    kv.set(3, "three"); kv.set(1, "one"); kv.set(2, "two");
    std::cout << "kv (sorted ascending): ";
    for (auto& p : kv) std::cout << p.first << ":" << p.second << ' ';
    std::cout << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：預設模板引數的「右邊向左」規則是什麼意思？
    //    A：跟 function 預設參數一樣：一旦某個參數有預設值，它右邊的所
    //       有參數也都必須有預設值 (中間不能跳過)。例如
    //       `template<typename T = int, typename U>` 不合法，因為 U 沒
    //       預設值卻在已預設的 T 之後。但 function template 因為有引數
    //       推導，允許從引數推出後面的型別，所以中間有預設值有時 OK。
    //
    //  Q2：預設值能不能引用前面的 template 參數？
    //    A：可以而且這是 STL 容器的標準寫法，例如
    //       `template<typename T, typename Alloc = std::allocator<T>>`
    //       讓 Alloc 預設成跟 T 配對的 allocator。這個技巧在實作 policy-based
    //       design 時超實用：使用者可以只填核心型別 T，但仍保留替換策略
    //       的彈性。
    //
    //  Q3：同一個 template 在多個宣告 / 定義中可以重複指定預設值嗎？
    //    A：不能在「同一個參數位置」重複給；但允許「不同 declaration 各
    //       補一段」，最後合併起來必須一致。常見模式：header 的 forward
    //       declaration 給預設值，定義那邊就不重複寫。注意這是 class
    //       template 的特性；function template 不能在多個 declaration 累積。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. 預設模板引數是 STL 廣泛使用的工具，讓使用者寫 std::vector<int> 而
//       不必寫到 std::vector<int, std::allocator<int>>。
//    2. 預設值可以引用前面的參數，做出「派生型別」型別。
//    3. function template 從 C++11 起也支援預設參數。
//
//  【下一篇】
//    08_typename_vs_class.cpp ── typename 關鍵字與 dependent name。
// ============================================================================
