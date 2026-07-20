// =============================================================================
//  課程 4.2：不變量與競爭條件1.cpp  —  不變量可以被「暫時」破壞，只要沒人看見
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    不變量 (invariant) 的定義，以及「複合操作必然暫時破壞不變量」
//   語法：    多步驟指標改寫：newNode->next / newNode->prev / a->next / c->prev
//   標準版本：本檔語法為 C++11 起（類別內成員預設初始化 `Node* prev = nullptr;`）
//   標頭檔：  <iostream>
//   複雜度：  雙向鏈結串列中間插入 O(1)
//   本檔性質：這是【正確且安全】的程式（單執行緒），刻意與下一檔 2.cpp 對照
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼是不變量 (invariant)】
//   不變量是「這個資料結構在任何【可觀察】的時刻都必須成立的條件」。
//   對本檔的雙向鏈結串列而言，不變量至少包含：
//       對任意相鄰節點 x, y：  x->next == y  ⟺  y->prev == x
//   也就是「前後指標必須互相對應」。只要這條成立，正向走訪與反向走訪
//   會得到完全相同的節點序列；一旦破壞，反向走訪就會走丟或走進環路。
//
//   請特別注意「可觀察」三個字 —— 這是整個第四階段的鑰匙。
//   不變量【不需要】每一行程式碼執行完都成立，
//   它只需要在「其他人有機會觀察到的時刻」成立。
//
// 【2. 為什麼複合操作必然會暫時破壞不變量】
//   在 a 與 c 之間插入 b，需要改寫四個指標：
//       ① newNode->next = c;   // b 認得 c
//       ② newNode->prev = a;   // b 認得 a
//       ③ a->next = newNode;   // a 改認 b
//       ④ c->prev = newNode;   // c 改認 b
//   CPU 沒有「一次改四個指標」的指令，這四步之間必定存在中間狀態。
//   例如做完 ③ 還沒做 ④ 的瞬間：
//       a->next == b（成立），但 c->prev == a（還是舊值）
//   → 此時 b->next == c 但 c->prev != b，不變量【被破壞了】。
//   這不是寫法不好，而是【任何】多步驟修改都躲不掉的物理事實。
//
// 【3. 為什麼本檔是安全的】
//   因為只有一條執行緒。插入期間沒有任何其他執行緒能夠觀察串列，
//   等到 insertAfter() 回傳、控制權交還給呼叫端時，四步已全部完成，
//   不變量重新成立。破壞是「私下」發生的，對外不可見 →
//   從外部看來，插入就像是一個不可分割的原子動作。
//
// 【4. 這就是「臨界區段」概念的來源】
//   把「不變量被破壞的那段期間」保護起來，不讓別人觀察，
//   就是臨界區段 (critical section) 的本質。單執行緒天生免費得到這個保護；
//   多執行緒則必須自己用 mutex 買回來。
//   → 下一檔（不變量與競爭條件2.cpp）就是把同一段程式碼丟進多執行緒，
//     示範讀者剛好在 ③ 與 ④ 之間走訪會發生什麼事。
//
// 【5. 「不變量」與「前置/後置條件」的關係】
//   * 前置條件 (precondition)：函式開始執行前呼叫者必須保證的事。
//   * 後置條件 (postcondition)：函式回傳時必須成立的事。
//   * 不變量：函式【開始前】與【回傳後】都必須成立，中間允許暫時不成立。
//   insertAfter 的不變量契約就是：進來時串列是合法的，出去時也必須是合法的。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局：本檔的 Node 為什麼是 24 bytes
//   struct Node { int data; Node* prev; Node* next; };
//   在 x86-64 (LP64) 上實測：sizeof(int)=4、sizeof(Node*)=8、alignof(Node)=8。
//   因為指標要求 8-byte 對齊，int 之後會補 4 bytes padding：
//       offset 0..3   : data      (4 bytes)
//       offset 4..7   : padding   (4 bytes，編譯器插入)
//       offset 8..15  : prev      (8 bytes)
//       offset 16..23 : next      (8 bytes)
//   → sizeof(Node) == 24（本機實測值，程式會印出來驗證）。
//   把 int 換成 int64_t 不會變大，但把 data 移到最後也還是 24 —— 
//   這是「成員重排可省 padding」規則在本例剛好無效的情形。
//
// (B) 為什麼四步的「順序」在單執行緒下無所謂、多執行緒下卻是關鍵
//   單執行緒下 ①②③④ 任意順序結果都一樣。但多執行緒下，
//   「先讓新節點指向舊結構（①②），最後才把舊結構指向新節點（③④）」
//   是有意義的：這樣讀者在 ③ 之前完全看不到 b，在 ③ 之後看到的 b
//   已經是內部指標齊備的狀態。這正是 lock-free 資料結構的基本手法
//   （publish-after-initialize），但要真正正確還需要記憶體屏障
//   （release/acquire），光是調整順序並不足夠——編譯器與 CPU 都可能重排。
//
// (C) 編譯器可能重排這四行嗎
//   可以。在單執行緒語意 (as-if rule) 下，只要最終可觀察行為相同，
//   編譯器完全可以改變 ①②③④ 的實際執行順序，CPU 亂序執行也會再排一次。
//   單執行緒下這無害；多執行緒下這正是「光靠寫法順序無法保證安全」的原因。
//
// 【注意事項 Pay Attention】
// 1. 「不變量暫時被破壞」本身不是 bug，看得到才是 bug。
// 2. 判斷要不要加鎖，先問：這段期間的中間狀態有沒有第二條執行緒能觀察到？
// 3. 本檔 main 裡的 a、b、c 是自動儲存期的區域變數，
//    串列只在函式內有效，不能回傳這些節點的位址。
// 4. 不變量橫跨幾個變數，臨界區段就必須橫跨幾個變數 ——
//    只鎖其中一個變數等於沒鎖。
// 5. sizeof(Node)=24 是本機 (x86-64 LP64) 的【實作定義】結果，
//    不是標準保證；32-bit 平台上會是 12。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】不變量 (invariant)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是不變量?為什麼說它「允許被暫時破壞」?
//     答：不變量是資料結構在任何【可觀察】時刻都必須成立的條件，
//         例如雙向串列的 x->next == y ⟺ y->prev == x。
//         任何多步驟修改都必然產生中間狀態，破壞是物理上躲不掉的；
//         只要破壞期間沒有第三者能觀察到，對外就等同從未被破壞。
//     追問：那要怎麼判斷「有沒有人能觀察到」?
//         → 看這段期間有沒有第二條執行緒可以碰到同一份資料。
//           單執行緒天生沒有；多執行緒就要用 mutex 把它變成沒有。
//
// 🔥 Q2. 插入節點的四個指標改寫，順序有沒有差別?
//     答：單執行緒下沒有差別（as-if rule 下編譯器本來就可能重排）。
//         多執行緒下「先初始化新節點、最後才把它接上舊結構」比較好，
//         讀者要嘛完全看不到新節點、要嘛看到的是完整的新節點。
//     追問：那這樣就執行緒安全了嗎?
//         → 沒有。還缺記憶體順序保證（release/acquire）與對 c->prev 的保護，
//           而且編譯器/CPU 都可能重排。這是 lock-free 演算法的入門，不是終點。
//
// ⚠️ 陷阱. 「我的函式每一行結束時資料都是對的，所以它執行緒安全」——錯在哪?
//     答：一行 C++ 敘述通常不是一個機器指令。`a->next = newNode;` 只是
//         一次 store，但整個插入是四次 store；而且連 `x = y + 1;` 這種
//         單行敘述都可能被拆成多道指令。用「行」當作原子性的單位是錯的。
//     為什麼會錯：多數人腦中把「一行原始碼」等同於「一個不可分割的動作」。
//         真正的原子性單位是硬體指令，而且還要加上記憶體順序的約束；
//         唯一可靠的判準是「有沒有同步機制建立 happens-before 關係」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>

struct Node {
    int data;
    Node* prev = nullptr;
    Node* next = nullptr;
};

// -----------------------------------------------------------------------------
// 在 a 與 c 之間插入 newNode。
// 四個步驟之間不變量暫時不成立，但單執行緒下沒有人能觀察到。
// -----------------------------------------------------------------------------
void insertAfter(Node* a, Node* newNode, Node* c) {
    // 不變量暫時被破壞，但沒關係
    // 因為沒有其他人會看到中間狀態
    newNode->next = c;
    newNode->prev = a;
    a->next = newNode;
    c->prev = newNode;
    // 不變量恢復
}

// -----------------------------------------------------------------------------
// 檢查不變量是否成立：對每一對相鄰節點驗證 x->next == y ⟺ y->prev == x
// 這種「可執行的不變量檢查」在實務上非常有用（debug build 用 assert 包起來）。
// -----------------------------------------------------------------------------
bool checkInvariant(Node* head) {
    for (Node* x = head; x && x->next; x = x->next) {
        if (x->next->prev != x) return false;
    }
    return true;
}

void printForward(Node* head) {
    for (Node* p = head; p; p = p->next) std::cout << p->data << " ";
    std::cout << "\n";
}

void printBackward(Node* tail) {
    for (Node* p = tail; p; p = p->prev) std::cout << p->data << " ";
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：實作一個鏈結串列，支援 get / addAtHead / addAtTail / addAtIndex /
//         deleteAtIndex。
//   為什麼用到本主題：這題的每一個操作都是「多步驟指標改寫」，
//         中途一定會破壞「節點數 size 與實際串列長度相符」「前後指標互相對應」
//         這兩條不變量。LeetCode 是單執行緒判題，所以能安全通過；
//         但只要把同一個物件丟給兩條執行緒，中間狀態立刻變成可觀察的，
//         就成了下一檔要示範的災難。本範例用單向串列 + 哨兵節點實作，
//         並在每次操作後檢查不變量。
// -----------------------------------------------------------------------------
class MyLinkedList {
private:
    struct LNode {
        int val;
        LNode* next;
        LNode(int v) : val(v), next(nullptr) {}
    };
    // 【注意】成員初始化順序依「宣告順序」，不是初始化列表的順序。
    LNode* head;   // 哨兵節點（dummy head），簡化邊界處理
    int size;

public:
    MyLinkedList() : head(new LNode(0)), size(0) {}

    ~MyLinkedList() {
        LNode* p = head;
        while (p) {
            LNode* nxt = p->next;
            delete p;
            p = nxt;
        }
    }

    MyLinkedList(const MyLinkedList&) = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    int get(int index) const {
        if (index < 0 || index >= size) return -1;
        LNode* p = head->next;
        for (int i = 0; i < index; ++i) p = p->next;
        return p->val;
    }

    void addAtHead(int val) { addAtIndex(0, val); }
    void addAtTail(int val) { addAtIndex(size, val); }

    void addAtIndex(int index, int val) {
        if (index > size) return;
        if (index < 0) index = 0;
        LNode* prev = head;
        for (int i = 0; i < index; ++i) prev = prev->next;
        LNode* node = new LNode(val);
        // ↓ 這兩行之間，size 與實際長度不一致 → 不變量被破壞
        node->next = prev->next;
        prev->next = node;
        ++size;      // ← 不變量在這一行之後才恢復
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size) return;
        LNode* prev = head;
        for (int i = 0; i < index; ++i) prev = prev->next;
        LNode* victim = prev->next;
        prev->next = victim->next;
        delete victim;
        --size;
    }

    int length() const { return size; }

    // 可執行的不變量檢查：走訪實際長度是否等於 size
    bool invariantHolds() const {
        int n = 0;
        for (LNode* p = head->next; p; p = p->next) ++n;
        return n == size;
    }

    void print() const {
        for (LNode* p = head->next; p; p = p->next) std::cout << p->val << " ";
        std::cout << "\n";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】LRU 快取的「移到最前面」操作為何必須是一個整體
//   情境：所有正式的 LRU 快取（作業系統頁面置換、資料庫 buffer pool、
//         nginx/Redis 的熱資料淘汰）都用「雙向串列 + 雜湊表」實作。
//         每次命中都要把節點從串列中間摘下、再接到最前面 ——
//         這是本檔 insertAfter 的孿生兄弟，同樣是多步驟指標改寫。
//   不變量：① 串列前後指標互相對應
//           ② 雜湊表的每個 key 都指向串列中真實存在的節點
//           ③ 雜湊表大小 == 串列長度
//   一旦第二條執行緒在「已從串列摘下、還沒插回去」的瞬間查表，
//   就會拿到一個不在串列裡的節點 —— 這是真實系統中極難重現的線上事故。
// -----------------------------------------------------------------------------
struct LruNode {
    int key;
    int value;
    LruNode* prev = nullptr;
    LruNode* next = nullptr;
    LruNode(int k, int v) : key(k), value(v) {}
};

class TinyLru {
private:
    LruNode* head;   // 最近使用
    LruNode* tail;   // 最久未使用

    void detach(LruNode* n) {
        // ← 這兩行之間 n 已經不在串列上，但它仍被外部持有 → 不變量破壞期
        if (n->prev) n->prev->next = n->next; else head = n->next;
        if (n->next) n->next->prev = n->prev; else tail = n->prev;
        n->prev = n->next = nullptr;
    }

    void pushFront(LruNode* n) {
        n->next = head;
        n->prev = nullptr;
        if (head) head->prev = n;
        head = n;
        if (!tail) tail = n;
    }

public:
    TinyLru() : head(nullptr), tail(nullptr) {}

    ~TinyLru() {
        while (head) { LruNode* nxt = head->next; delete head; head = nxt; }
    }

    TinyLru(const TinyLru&) = delete;
    TinyLru& operator=(const TinyLru&) = delete;

    void insertFront(int key, int value) { pushFront(new LruNode(key, value)); }

    // 命中：摘下 + 插回最前面。這兩步之間不變量不成立。
    void touch(LruNode* n) {
        detach(n);
        pushFront(n);
    }

    LruNode* find(int key) {
        for (LruNode* p = head; p; p = p->next) if (p->key == key) return p;
        return nullptr;
    }

    bool invariantHolds() const {
        // 正走一遍、反走一遍，長度必須相同，且前後指標互相對應
        int fwd = 0;
        for (const LruNode* p = head; p; p = p->next) {
            if (p->next && p->next->prev != p) return false;
            ++fwd;
        }
        int bwd = 0;
        for (const LruNode* p = tail; p; p = p->prev) ++bwd;
        return fwd == bwd;
    }

    void print() const {
        for (const LruNode* p = head; p; p = p->next)
            std::cout << "(" << p->key << ":" << p->value << ") ";
        std::cout << "\n";
    }
};

int main() {
    std::cout << "=== 單執行緒插入：不變量暫時破壞、對外不可見 ===\n";
    Node a{1}, b{2}, c{3};
    a.next = &c;
    c.prev = &a;

    std::cout << "插入前 不變量成立: " << std::boolalpha << checkInvariant(&a) << "\n";
    insertAfter(&a, &b, &c);  // 安全：單執行緒
    std::cout << "插入後 不變量成立: " << checkInvariant(&a) << "\n";

    std::cout << "a.next->data = " << a.next->data << "\n";   // 2
    std::cout << "正向走訪: ";
    printForward(&a);
    std::cout << "反向走訪: ";
    printBackward(&c);

    std::cout << "\n=== Node 的記憶體佈局（本機實作定義值）===\n";
    std::cout << "sizeof(int)   = " << sizeof(int) << "\n";
    std::cout << "sizeof(Node*) = " << sizeof(Node*) << "\n";
    std::cout << "sizeof(Node)  = " << sizeof(Node) << "  (4 + 4 padding + 8 + 8)\n";
    std::cout << "alignof(Node) = " << alignof(Node) << "\n";

    std::cout << "\n=== LeetCode 707. Design Linked List ===\n";
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);      // 串列變成 1 -> 2 -> 3
        std::cout << "內容: ";
        list.print();
        std::cout << "get(1) = " << list.get(1) << "\n";      // 2
        list.deleteAtIndex(1);      // 串列變成 1 -> 3
        std::cout << "刪除 index 1 後: ";
        list.print();
        std::cout << "get(1) = " << list.get(1) << "\n";      // 3
        std::cout << "get(9) = " << list.get(9) << "  (越界回傳 -1)\n";
        std::cout << "長度 = " << list.length()
                  << ", 不變量成立: " << list.invariantHolds() << "\n";
    }

    std::cout << "\n=== 日常實務：LRU 快取的 touch 是複合操作 ===\n";
    {
        TinyLru lru;
        lru.insertFront(3, 300);
        lru.insertFront(2, 200);
        lru.insertFront(1, 100);
        std::cout << "初始（最近使用在左）: ";
        lru.print();
        std::cout << "不變量成立: " << lru.invariantHolds() << "\n";

        LruNode* hit = lru.find(3);
        std::cout << "命中 key=3，執行 touch（摘下 + 插回最前面）\n";
        lru.touch(hit);
        std::cout << "touch 後: ";
        lru.print();
        std::cout << "不變量成立: " << lru.invariantHolds() << "\n";
        std::cout << "→ detach 與 pushFront 之間，節點不在任何串列上；\n";
        std::cout << "  單執行緒安全，多執行緒下這就是可被觀察的破壞期。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra '課程 4.2：不變量與競爭條件1.cpp' -o invariant1
//   （本檔為單執行緒，不需要 -pthread）

// 註：本檔為單執行緒，輸出完全確定，每次執行都相同。
// sizeof/alignof 為本機 x86-64 (LP64) 的【實作定義】值，非標準保證。

// === 預期輸出 ===
// === 單執行緒插入：不變量暫時破壞、對外不可見 ===
// 插入前 不變量成立: true
// 插入後 不變量成立: true
// a.next->data = 2
// 正向走訪: 1 2 3 
// 反向走訪: 3 2 1 
//
// === Node 的記憶體佈局（本機實作定義值）===
// sizeof(int)   = 4
// sizeof(Node*) = 8
// sizeof(Node)  = 24  (4 + 4 padding + 8 + 8)
// alignof(Node) = 8
//
// === LeetCode 707. Design Linked List ===
// 內容: 1 2 3 
// get(1) = 2
// 刪除 index 1 後: 1 3 
// get(1) = 3
// get(9) = -1  (越界回傳 -1)
// 長度 = 2, 不變量成立: true
//
// === 日常實務：LRU 快取的 touch 是複合操作 ===
// 初始（最近使用在左）: (1:100) (2:200) (3:300) 
// 不變量成立: true
// 命中 key=3，執行 touch（摘下 + 插回最前面）
// touch 後: (3:300) (1:100) (2:200) 
// 不變量成立: true
// → detach 與 pushFront 之間，節點不在任何串列上；
//   單執行緒安全，多執行緒下這就是可被觀察的破壞期。
