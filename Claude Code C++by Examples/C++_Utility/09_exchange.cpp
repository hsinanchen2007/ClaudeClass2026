/*
================================================================================
主題:std::exchange —— 取舊值、放新值,一行搞定
標準:C++14 起
標頭:<utility>
參考:https://en.cppreference.com/w/cpp/utility/exchange
================================================================================

【一、課題介紹】
  std::exchange(obj, newValue) 的行為是:
    1. 把 newValue 賦值給 obj。
    2. 「回傳 obj 原本的舊值」。

  寫成虛擬碼:
    template <class T, class U = T>
    T exchange(T& obj, U&& newVal) {
        T old = std::move(obj);
        obj   = std::forward<U>(newVal);
        return old;       // 注意:回傳「拷貝/移動的暫存」,不是參考
    }

  為什麼需要它?
    1. 寫 move constructor / move assignment 時,要把來源置回 nullptr
       並保留舊值給自己,exchange 一行解決,不易出錯。
    2. 取代「(stash old, set new)」這種兩步寫法,讓程式更精簡。
    3. 在迴圈中「取目前值並更新」是常見場景。

【二、觀念解釋】
  1. 標頭:<utility>。
  2. 形式:T old = std::exchange(target, newValue);
  3. exchange 與 swap 的差別:
       - swap(a, b)  : a, b 兩個物件互換。
       - exchange(a, n): 把 n 放進 a,回傳 a 的舊值。可以視為「半邊 swap」。
  4. 內部使用 move,所以對「移動成本低」的型別非常便宜。

【三、常見陷阱】
  - 不要忽略它的「回傳值」—— 整個重點就在這個回傳值。
  - 對 const 物件無效(無法寫入)。
  - 不需要丟一個跟原型完全相同的型別:newValue 只要可賦值給 obj 即可。

【四、與其他 utility 的比較】
  - vs std::swap:swap 雙向、無回傳;exchange 單向、回傳舊值。
  - vs std::move:move 是型別轉換工具;exchange 是「動作」。
  - 寫 move ctor 時,exchange 比手寫「先存舊、再清空」更不易出錯。

【五、Leetcode 對應題目】
  題號:206. Reverse Linked List(反轉鏈結串列)
  難度:Easy
  連結:https://leetcode.com/problems/reverse-linked-list/
  題目大意:就地反轉一條單向鏈結串列,回傳新頭節點。
  選用理由:迴圈反轉常見的三變數寫法 (prev, curr, next),正好可用
            std::exchange 把「換手」這一步寫得更直觀。

【六、日常工作實用範例】
  情境:寫一個小型 Logger,「換出舊的緩衝區交給寫盤、自己馬上拿一個新的」,
        是 exchange 的天然場景。
================================================================================
*/

/*
補充筆記：std::exchange
  - std::exchange 屬於 utility 類工具；這些型別與函式常用來表達小型資料組合、可選值、型別安全聯集或值類別轉換。
  - pair/tuple 適合簡短聚合結果，但欄位語意複雜時應定義具名 struct，避免 first/second 或 get<0> 難讀。
  - optional 表示可能沒有值，使用前要檢查 has_value 或使用 value_or；value() 在無值時會丟例外。
  - variant 表示多選一型別，應用 visit 或 holds_alternative/get_if 安全存取目前替代項。
  - any 提供執行期任意型別保存，但取回需要知道正確型別；過度使用會失去靜態型別檢查優勢。
  - std::move/std::forward/std::exchange/as_const 都是表達意圖的工具；它們本身不一定搬移或複製資料。
  - std::exchange(obj, newValue) 會把舊值回傳，同時把 obj 改成新值。
  - exchange 很適合 move constructor 中接管指標後把來源設成 nullptr。
  - 它先 move 舊值再賦新值；若型別的 move/assignment 有成本或例外，要納入設計。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::exchange（C++14）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::exchange 做什麼？最典型的用途是什麼？
//     答：std::exchange(obj, new_val) 把 obj 設為 new_val，並回傳 obj 的舊值。最經典的
//     用途是實作移動建構／移動賦值，一行同時完成「搬走並把來源置空」：
//     Buf(Buf&& o) noexcept : ptr_(std::exchange(o.ptr_, nullptr)),
//                             size_(std::exchange(o.size_, 0)) {}
//     這樣可以直接寫在成員初始化列表裡，不必在函式本體再補一段清理來源的程式碼。
//     追問：它是哪個標準加入的？（C++14；同期加入的還有 init-capture）
//
// 🔥 Q2. exchange 和 swap 差在哪？
//     答：swap 是對稱的——交換兩個同型別物件；exchange 是非對稱的——設定新值並回傳
//     舊值，而且新值的型別不必和舊值相同（只要可賦值即可）。它對應的是 atomic 的
//     exchange 操作在單執行緒下的語意。
//
// ⚠️ 陷阱. 用 exchange 寫移動建構，就代表來源物件之後一定是「空」的嗎？
//     答：只有你自己寫的那幾個成員被重設了。exchange 不會替你處理漏掉的成員——忘了
//     exchange 的那個成員仍指向原本的資源，於是兩個物件同時持有同一份資源，解構時
//     double free。移動之後要確保來源處於「可安全解構」的狀態，這是你的責任。
//     為什麼會錯：把 exchange 當成「移動語意的魔法棒」，忽略它只是一個非常小的工具函式。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 範例 1:基本用法 —— 取舊值、放新值
// ---------------------------------------------------------------------------
void demo_basic() {
    std::cout << "[demo_basic]\n";
    int x = 100;
    int old = std::exchange(x, 200);              // 把 200 放進 x,old 拿到 100
    std::cout << "  x=" << x << ", old=" << old << "\n";

    std::string s = "hello";
    std::string before = std::exchange(s, "world");
    std::cout << "  s=" << s << ", before=" << before << "\n";
}

// ---------------------------------------------------------------------------
// 範例 2:用在 move constructor —— RAII 類別常見手法
//
// 自定義一個小 Buffer:管理一段 heap 配置的記憶體。
// move constructor 要把 other 的 data_ 設成 nullptr 並保留舊值給自己。
// 用 std::exchange 一行解決,既精簡又不易遺漏「清空 other」這一步。
// ---------------------------------------------------------------------------
class Buffer {
public:
    explicit Buffer(std::size_t n = 0)
        : data_(n ? new int[n]{} : nullptr), size_(n) {}

    ~Buffer() { delete[] data_; }

    // move ctor 經典寫法
    Buffer(Buffer&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)) {}

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    std::size_t size() const { return size_; }
    bool empty() const { return data_ == nullptr; }
private:
    int* data_;
    std::size_t size_;
};

void demo_move_ctor() {
    std::cout << "[demo_move_ctor]\n";
    Buffer a(5);
    Buffer b = std::move(a);                      // 走 move ctor
    std::cout << "  b.size=" << b.size()
              << ", a.empty()=" << std::boolalpha << a.empty() << "\n";
}

// ---------------------------------------------------------------------------
// 範例 3:Leetcode #206 Reverse Linked List
//
// 解題思路(迭代法):
//   用三個指標 prev、curr、next 走訪。每輪:
//     next = curr->next     (先記住下一個)
//     curr->next = prev     (反轉指向)
//     prev = curr           (整體往前推)
//     curr = next
//   結束時 prev 就是新頭。
//
// 用 std::exchange 把「prev = curr; curr = curr->next」這一步寫得乾淨:
//   ListNode* next = curr->next;
//   curr->next = std::exchange(prev, curr);    // prev 拿到 curr,回傳舊 prev
//   curr = next;
//
// 時間複雜度:O(n),空間複雜度:O(1)。
// ---------------------------------------------------------------------------
struct ListNode {
    int val;
    ListNode* next;
    explicit ListNode(int v) : val(v), next(nullptr) {}
};

ListNode* reverseList(ListNode* head) {
    ListNode* prev = nullptr;
    ListNode* curr = head;
    while (curr) {
        ListNode* nextNode = curr->next;
        curr->next = std::exchange(prev, curr);   // prev=curr,curr->next=舊 prev
        curr = nextNode;
    }
    return prev;
}

void printList(ListNode* h) {
    while (h) {
        std::cout << h->val;
        if (h->next) std::cout << " -> ";
        h = h->next;
    }
    std::cout << "\n";
}

void demo_leetcode_reverse_list() {
    std::cout << "[demo_leetcode_reverse_list]\n";
    // 建 1 -> 2 -> 3 -> 4
    ListNode* head = new ListNode(1);
    head->next = new ListNode(2);
    head->next->next = new ListNode(3);
    head->next->next->next = new ListNode(4);
    std::cout << "  before: ";
    printList(head);
    ListNode* r = reverseList(head);
    std::cout << "  after : ";
    printList(r);
    // 釋放
    while (r) { ListNode* t = r; r = r->next; delete t; }
}

// ---------------------------------------------------------------------------
// 範例 4:日常工作實用範例 —— Logger 雙緩衝(double buffer)
//
// 情境:Logger 內部有一個寫入緩衝 buf_;呼叫 flush() 時,要把目前的 buf_
//       「整個換出來」交給寫盤函式,然後 buf_ 立刻換成新的空緩衝。
//       std::exchange 一行就完成「拿出舊的、放進新的」。
// ---------------------------------------------------------------------------
class Logger {
public:
    void log(const std::string& msg) { buf_.push_back(msg); }

    std::vector<std::string> flush() {
        // 取出舊 buf_、buf_ 變成新的空 vector
        return std::exchange(buf_, std::vector<std::string>{});
    }
private:
    std::vector<std::string> buf_;
};

void demo_practical_logger() {
    std::cout << "[demo_practical_logger]\n";
    Logger lg;
    lg.log("start");
    lg.log("processing");
    lg.log("done");

    auto batch = lg.flush();
    std::cout << "  flushed " << batch.size() << " messages\n";
    for (const auto& m : batch) std::cout << "    - " << m << "\n";

    // 此時 lg 內部已經是空 buffer
    auto again = lg.flush();
    std::cout << "  flushed " << again.size() << " messages\n";
}

// ---------------------------------------------------------------------------
// 實用範例 (額外):計步器 step_and_get_prev
//
// 工作中常見:函式「取得當前計數值, 然後把計數 +1」是一次性操作,
// std::exchange 可以一行完成「取舊值 + 換新值」。
// ---------------------------------------------------------------------------
class Counter {
public:
    int next() {
        return std::exchange(count_, count_ + 1);     // 回傳舊值, 同時 +1
    }
private:
    int count_ = 0;
};

void demo_practical_counter() {
    std::cout << "[demo_practical_counter]\n";
    Counter c;
    for (int i = 0; i < 5; ++i) {
        std::cout << "  next() = " << c.next() << "\n";   // 0, 1, 2, 3, 4
    }
}

int main() {
    demo_basic();
    demo_move_ctor();
    demo_leetcode_reverse_list();
    demo_practical_logger();
    demo_practical_counter();
    return 0;
}

/*
================================================================================
編譯與執行:
    g++ -std=c++17 -Wall -Wextra 09_exchange.cpp -o 09_exchange && ./09_exchange
================================================================================
*/
