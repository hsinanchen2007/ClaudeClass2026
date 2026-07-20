// =============================================================================
//  summary.cpp  —  第 2.8 章總結：Rule of Five / Rule of Zero 的完整規則
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<cstring>（strlen/strcpy）、<utility>（move）、<memory>（unique_ptr）
//   Rule of Five（C++11 起）——五個「特殊成員函式」：
//     1. ~ClassName()                                解構子
//     2. ClassName(const ClassName&)                 複製建構子
//     3. ClassName& operator=(const ClassName&)      複製賦值
//     4. ClassName(ClassName&&) noexcept             移動建構子
//     5. ClassName& operator=(ClassName&&) noexcept  移動賦值
//   規則內容：只要你自訂了其中任何一個，通常五個都需要自己負責。
//   Rule of Zero：改用 RAII 成員，讓這五個全部由編譯器生成——這是首選。
//
// 【詳細解釋 Explanation】
//
// 【1. 規則背後的真正理由：自訂其一，代表「這個類別擁有資源」】
//   Rule of Five 常被死記成「寫了一個就要寫五個」，但它的根據其實很簡單：
//   你會自訂解構子，唯一的原因就是「這個類別擁有某項需要手動釋放的資源」。
//   而一旦有所有權，編譯器預設的「逐成員複製」必然是錯的——
//   它只會複製指標的值，造成兩個物件同時宣稱擁有同一塊資源。
//   所以規則的實質是：**有所有權，就要親自定義所有權在複製與移動時如何轉移。**
//
// 【2. 編譯器自動生成的規則（實際比口訣複雜）】
//   * 自訂了解構子、複製建構子或複製賦值中的任何一個
//     → 移動建構子與移動賦值「不會」被自動生成
//     → 此時 std::move(obj) 會靜默退回呼叫「複製」，效能損失完全無聲
//   * 自訂了移動建構子或移動賦值
//     → 複製建構子與複製賦值被隱式 delete
//   * 自訂解構子時，複製操作仍會生成，但已被標記為「deprecated」
//   最後一點正是本章 double free 的成因：解構子有了、複製卻還在自動生成。
//
// 【3. 移動操作為什麼一定要標 noexcept】
//   std::vector 擴容時要把既有元素搬到新緩衝區。它必須保證「強例外安全」：
//   萬一搬到一半失敗，原本的資料不能損毀。
//   而「移動」會破壞來源物件——搬到一半丟例外就無法復原了。
//   因此 vector 用 std::move_if_noexcept：
//     移動建構子是 noexcept → 放心用移動（快）
//     不是 noexcept        → 退回用複製（慢，但失敗時原資料完好）
//   結果就是：**忘了寫 noexcept，vector 擴容會靜默地從移動退化成複製。**
//   這是本章與第 2.9 章的關鍵連結，第 2.9 章有實測數據。
//
// 【4. 本檔三個類別的角色】
//   String  —— 完整實作 Rule of Five 的教科書範例（手動管理 char*）
//   Broken  —— 只寫解構子的錯誤示範（本檔只描述、不實際執行，避免崩潰）
//   User    —— Rule of Zero：全部用 RAII 成員，一個特殊函式都不寫
//
// 【5. 成員初始化順序：本檔 String 的關鍵細節】
//   成員的初始化順序一律依「宣告順序」，與初始化列表的書寫順序無關。
//   String 把 size_ 宣告在 data_ 前面是必要的：
//       String(const char* s) : size_(strlen(s)), data_(new char[size_ + 1])
//   若 data_ 宣告在前，new char[size_ + 1] 會讀到尚未初始化的 size_，
//   配置出錯誤大小的緩衝區，接著 strcpy 就是堆積緩衝區溢位。
//   把初始化列表寫成與宣告順序不一致時，g++ 會以 -Wreorder 警告
//   （-Wall 已包含），這個警告一定要當成錯誤看待。
//
// 【概念補充 Concept Deep Dive】
//   (A) 複製賦值為什麼要檢查 this != &o？
//       因為 a = a 這種自我賦值下，若先 delete[] data_ 再從 o 複製，
//       來源已經被釋放了。本檔用的是「先檢查」的傳統寫法；
//       更穩健的做法是 copy-and-swap：以傳值方式接參數，再與 *this 交換，
//       一次同時取得自我賦值安全與強例外安全保證。
//   (B) 移動後的來源物件必須留在「valid but unspecified」狀態。
//       本檔把 data_ 設為 nullptr、size_ 設為 0，這讓解構子的 delete[]
//       安全（delete[] nullptr 是合法的無操作）。
//       標準只要求「可安全解構、可重新賦值」，不保證任何特定值。
//   (C) 為什麼 unique_ptr 成員會讓整個類別自動變成「只能移動」？
//       因為複製建構子的隱式生成規則是「逐成員複製」，
//       而 unique_ptr 的複製建構子是 deleted，於是外層的複製也被隱式 delete。
//       所有權語意就這樣從成員自動傳播到整個類別——不需要寫任何一行程式碼。
//
// 【注意事項 Pay Attention】
//   1. 自訂解構子會抑制移動操作的生成，std::move 靜默退化成複製。
//   2. 移動操作務必標 noexcept，否則 vector 擴容會退回複製。
//   3. 成員初始化順序看「宣告順序」，不是初始化列表的順序（-Wreorder）。
//   4. 複製賦值要處理自我賦值；copy-and-swap 是更穩健的慣用法。
//   5. 被移動後的物件處於 valid but unspecified 狀態——
//      本檔輸出中被移動的 String 顯示為空字串，是本實作把指標設 nullptr
//      的結果，不是標準對所有型別的保證。
//   6. 優先 Rule of Zero。只有在包裝 OS handle、檔案描述符這類
//      無法用現成 RAII 型別表達的低階資源時，才手寫 Rule of Five。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Rule of Five / Rule of Zero
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 Rule of Five？為什麼「寫了一個就要寫五個」？
//     答：五個特殊成員函式是解構子、複製建構子、複製賦值、移動建構子、移動賦值。
//         會自訂解構子，唯一的理由是這個類別「擁有」某項需手動釋放的資源；
//         一旦有所有權，編譯器預設的逐成員複製必然錯誤（兩個物件共用同一份資源）。
//         所以規則的實質是：有所有權，就必須親自定義所有權如何複製與移動。
//     追問：那 Rule of Zero 是什麼？
//         → 改用 RAII 成員（string/vector/unique_ptr），五個全部交給編譯器生成，
//           而且必定正確。這才是首選；Rule of Five 只用於低階資源包裝。
//
// 🔥 Q2. 移動建構子為什麼一定要加 noexcept？不加會怎樣？
//     答：vector 擴容時要保證強例外安全。移動會破壞來源，搬到一半失敗就無法復原，
//         所以 vector 用 std::move_if_noexcept：移動操作是 noexcept 才用移動，
//         否則退回複製。不加 noexcept 的後果是「vector 擴容靜默退化成複製」——
//         程式完全正確，只是慢很多，而且沒有任何警告。
//
// 🔥 Q3. 為什麼一個 std::unique_ptr 成員就能讓整個類別變成「不可複製、可移動」？
//     答：隱式複製建構子是逐成員複製，而 unique_ptr 的複製建構子是 deleted，
//         導致外層類別的複製也被隱式 delete；移動則因 unique_ptr 可移動而正常生成。
//         所有權語意從成員自動傳播到整個類別，不需要寫任何程式碼——
//         這正是 Rule of Zero 的威力：把意圖寫進型別，而不是寫進註解。
//
// ⚠️ 陷阱. 「我只加了一個解構子印 log，其他都沒動，所以行為不變」——錯在哪？
//     答：自訂解構子會「抑制移動建構子與移動賦值的自動生成」。
//         於是所有 std::move(obj) 都靜默退回呼叫複製建構子——
//         程式依然正確，vector 擴容依然正確，只是全部變慢，
//         而且沒有任何編譯器警告或執行期徵兆。
//     為什麼會錯：大家把五個特殊成員函式當成彼此獨立的東西，
//         但標準規定它們之間有連動的生成規則。
//         「加一個看似無害的解構子」是真實專案中最常見的效能退化來源之一。
//     更陰險的一點（本機實測驗證）：此時
//         std::is_move_constructible<B>::value 仍然是 1（true）！
//     因為「能不能用一個右值來建構」的答案確實是「能」——
//     只是它綁到的是 const B& 複製建構子。這個 type trait 回答的是
//     「可不可以」，不是「會不會真的移動」。想確認有沒有真的移動，
//     只能靠計數器或實際觀察建構子的呼叫。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cstring>
#include <utility>
#include <string>
#include <vector>
#include <memory>

// ============================================================
// 完整的 Rule of Five 類別
// ============================================================
class String {
    // ⚠️ 成員的初始化順序一律依「宣告順序」，與初始化列表的書寫順序無關。
    //    size_ 必須宣告在 data_ 前面，否則 new char[size_ + 1] 會讀到
    //    尚未初始化的 size_，配置出錯誤大小的緩衝區（heap-buffer-overflow）。
    size_t size_;
    char* data_;
public:
    // 建構子
    String(const char* s = "")
        : size_(std::strlen(s)), data_(new char[size_ + 1]) {
        std::strcpy(data_, s);
        std::cout << "  [建構] \"" << data_ << "\"\n";
    }

    // ════════════════════════════════════
    // ★ Rule of Five ★
    // ════════════════════════════════════

    // 1. 解構子
    ~String() {
        std::cout << "  [解構] \"" << (data_ ? data_ : "null") << "\"\n";
        delete[] data_;
    }

    // 2. 複製建構子
    String(const String& o)
        : size_(o.size_), data_(new char[o.size_ + 1]) {
        std::strcpy(data_, o.data_);
        std::cout << "  [複製建構] \"" << data_ << "\"\n";
    }

    // 3. 複製賦值
    String& operator=(const String& o) {
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            data_ = new char[size_ + 1];
            std::strcpy(data_, o.data_);
        }
        std::cout << "  [複製賦值] \"" << data_ << "\"\n";
        return *this;
    }

    // 4. 移動建構子
    String(String&& o) noexcept
        : size_(o.size_), data_(o.data_) {   // 順序須與宣告順序一致
        o.data_ = nullptr; o.size_ = 0;
        std::cout << "  [移動建構] \"" << data_ << "\"\n";
    }

    // 5. 移動賦值
    String& operator=(String&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_; size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        std::cout << "  [移動賦值] \"" << data_ << "\"\n";
        return *this;
    }

    // ════════════════════════════════════

    const char* c_str() const { return data_ ? data_ : ""; }
};

// ============================================================
// 違反 Rule of Five：只寫解構子
// ============================================================
class Broken {
    int* data_;
public:
    Broken(int val) : data_(new int(val)) {}
    ~Broken() { delete data_; }
    // ⚠️ 沒有拷貝建構、拷貝賦值、移動建構、移動賦值
    // → 預設淺拷貝 → double free！
    int value() const { return *data_; }
};

// ============================================================
// Rule of Zero：最推薦的做法
// ============================================================
class User {
    std::string name_;                  // RAII：自動管理
    std::vector<int> scores_;           // RAII：自動管理
    std::unique_ptr<int[]> buffer_;     // RAII：自動管理（不可複製）

    // 不需要寫任何特殊函式！
    // 解構子：自動呼叫每個成員的解構子
    // 移動：自動逐成員移動
    // 複製：因 unique_ptr 不可複製 → 整個類別不可複製
public:
    User(std::string name, std::vector<int> scores, size_t buf_size)
        : name_(std::move(name))
        , scores_(std::move(scores))
        , buffer_(std::make_unique<int[]>(buf_size)) {}

    const std::string& name() const { return name_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：設計連結串列，支援 get / addAtHead / addAtTail / addAtIndex / deleteAtIndex。
//   為什麼用到本主題：這是少數「LeetCode 題目本身就要求你管理原始記憶體」的題型。
//     每個節點都是 new 出來的，類別因此「擁有」整條串列——
//     一旦擁有資源，Rule of Five 就從理論變成必要條件：
//       * 沒有解構子 → 記憶體洩漏（LeetCode 評測不會抓，但真實程式會）
//       * 有解構子卻沒有複製建構子 → 複製後 double free（本章的核心錯誤）
//     本實作同時示範「手寫 Rule of Five」與「若改用 unique_ptr 會如何」。
//   複雜度：get/addAtIndex/deleteAtIndex 皆 O(n)，addAtHead O(1)。
// -----------------------------------------------------------------------------
class MyLinkedList {
    struct Node {
        int val;
        Node* next;
        Node(int v, Node* n = nullptr) : val(v), next(n) {}
    };
    Node* head_ = nullptr;
    int   size_ = 0;

    // 私有輔助：深層複製整條串列（複製建構子與複製賦值共用）
    static Node* cloneList(Node* src) {
        if (!src) return nullptr;
        Node* newHead = new Node(src->val);
        Node* tail = newHead;
        for (Node* p = src->next; p; p = p->next) {
            tail->next = new Node(p->val);
            tail = tail->next;
        }
        return newHead;
    }

public:
    MyLinkedList() = default;

    // ── Rule of Five ①：解構子（釋放所有節點）──
    ~MyLinkedList() { clear(); }

    // ── ②：複製建構子（深層複製，否則兩條串列共用節點 → double free）──
    MyLinkedList(const MyLinkedList& o)
        : head_(cloneList(o.head_)), size_(o.size_) {}

    // ── ③：複製賦值（先做出新資料再釋放舊的，兼顧自我賦值與例外安全）──
    MyLinkedList& operator=(const MyLinkedList& o) {
        if (this != &o) {
            Node* tmp = cloneList(o.head_);   // 先建好，失敗也不會破壞現況
            clear();
            head_ = tmp;
            size_ = o.size_;
        }
        return *this;
    }

    // ── ④：移動建構子（偷走指標，noexcept 讓 vector 擴容願意用它）──
    MyLinkedList(MyLinkedList&& o) noexcept
        : head_(o.head_), size_(o.size_) {
        o.head_ = nullptr;    // 來源必須留在可安全解構的狀態
        o.size_ = 0;
    }

    // ── ⑤：移動賦值 ──
    MyLinkedList& operator=(MyLinkedList&& o) noexcept {
        if (this != &o) {
            clear();
            head_ = o.head_;  size_ = o.size_;
            o.head_ = nullptr; o.size_ = 0;
        }
        return *this;
    }

    void clear() {
        while (head_) { Node* nx = head_->next; delete head_; head_ = nx; }
        size_ = 0;
    }

    int get(int index) const {
        if (index < 0 || index >= size_) return -1;
        Node* p = head_;
        for (int i = 0; i < index; ++i) p = p->next;
        return p->val;
    }

    void addAtHead(int val) { head_ = new Node(val, head_); ++size_; }

    void addAtTail(int val) {
        if (!head_) { addAtHead(val); return; }
        Node* p = head_;
        while (p->next) p = p->next;
        p->next = new Node(val);
        ++size_;
    }

    void addAtIndex(int index, int val) {
        if (index > size_) return;
        if (index <= 0) { addAtHead(val); return; }
        Node* p = head_;
        for (int i = 0; i < index - 1; ++i) p = p->next;
        p->next = new Node(val, p->next);
        ++size_;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size_) return;
        if (index == 0) {
            Node* old = head_;
            head_ = head_->next;
            delete old;
        } else {
            Node* p = head_;
            for (int i = 0; i < index - 1; ++i) p = p->next;
            Node* old = p->next;
            p->next = old->next;
            delete old;
        }
        --size_;
    }

    int size() const { return size_; }

    std::string toString() const {
        std::string s = "[";
        for (Node* p = head_; p; p = p->next) {
            s += std::to_string(p->val);
            if (p->next) s += ",";
        }
        return s + "]";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】包裝一個必須手動釋放的低階資源
//   情境：C API 的資源（檔案描述符、socket、OS handle、函式庫 context）
//     沒有現成的 RAII 型別可用，這時才是真正需要手寫 Rule of Five 的場合。
//   本例用「模擬的檔案描述符」示範：
//     * 解構子負責 close
//     * 複製會 dup 出一個新的描述符（真正的深層複製）
//     * 移動則直接接管，來源標記為 -1（無效）
//   注意這裡刻意不用真正的系統呼叫，讓範例可重現且不依賴環境。
// -----------------------------------------------------------------------------
int g_openHandles = 0;   // 模擬 OS 的已開啟描述符計數

class FileHandle {
    int fd_;
    std::string path_;

    static int sys_open(const std::string& p) {
        ++g_openHandles;
        static int nextFd = 3;      // 0,1,2 保留給 stdin/stdout/stderr
        std::cout << "    [os] open(\"" << p << "\") → fd=" << nextFd << "\n";
        return nextFd++;
    }
    static void sys_close(int fd) {
        if (fd < 0) return;
        --g_openHandles;
        std::cout << "    [os] close(fd=" << fd << ")\n";
    }

public:
    explicit FileHandle(std::string path)
        : fd_(sys_open(path)), path_(std::move(path)) {}

    // ① 解構子：釋放資源
    ~FileHandle() { sys_close(fd_); }

    // ② 複製建構子：真正複製一份資源（相當於 dup）
    FileHandle(const FileHandle& o) : fd_(sys_open(o.path_)), path_(o.path_) {
        std::cout << "    FileHandle 複製（重新開啟一份）\n";
    }

    // ③ 複製賦值
    FileHandle& operator=(const FileHandle& o) {
        if (this != &o) {
            int newFd = sys_open(o.path_);   // 先取得新資源
            sys_close(fd_);                  // 再釋放舊的
            fd_ = newFd;
            path_ = o.path_;
        }
        return *this;
    }

    // ④ 移動建構子：接管，來源設為無效
    FileHandle(FileHandle&& o) noexcept
        : fd_(o.fd_), path_(std::move(o.path_)) {
        o.fd_ = -1;                          // ★ 關鍵：來源不可再擁有它
        std::cout << "    FileHandle 移動（接管 fd，不呼叫 open）\n";
    }

    // ⑤ 移動賦值
    FileHandle& operator=(FileHandle&& o) noexcept {
        if (this != &o) {
            sys_close(fd_);
            fd_ = o.fd_;  path_ = std::move(o.path_);
            o.fd_ = -1;
        }
        return *this;
    }

    int fd() const { return fd_; }
};

int main() {
    // ============================================================
    // 1. Rule of Five 完整使用
    // ============================================================
    std::cout << "===== 1. Rule of Five =====\n";
    {
        String a("Hello");
        String b = a;              // 複製建構
        String c;
        c = a;                     // 複製賦值
        String d = std::move(a);   // 移動建構
        String e;
        e = std::move(b);          // 移動賦值

        std::cout << "  a: \"" << a.c_str() << "\" (已被移動)\n";
        std::cout << "  b: \"" << b.c_str() << "\" (已被移動)\n";
        std::cout << "  c: \"" << c.c_str() << "\"\n";
        std::cout << "  d: \"" << d.c_str() << "\"\n";
        std::cout << "  e: \"" << e.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 違反 Rule of Five 的後果
    // ============================================================
    std::cout << "===== 2. 違反 Rule of Five =====\n";
    std::cout << "  Broken 類別：只寫解構子 → double free！\n";
    std::cout << "  Broken a(42); Broken b = a;\n";
    std::cout << "  a.data_ 和 b.data_ 指向同一塊記憶體\n";
    std::cout << "  兩者解構時都 delete → 💥 crash\n";
    // 不實際執行，避免崩潰
    std::cout << "\n";

    // ============================================================
    // 3. Rule of Zero
    // ============================================================
    std::cout << "===== 3. Rule of Zero（最推薦）=====\n";
    {
        User u1("Alice", {90, 85, 95}, 100);
        // User u2 = u1;          // ❌ 因為 unique_ptr 不可複製
        User u2 = std::move(u1);  // ✅ 逐成員移動
        std::cout << "  u2.name() = " << u2.name() << "\n";
        std::cout << "  u1.name() = \"" << u1.name() << "\" (已被移動)\n";
        std::cout << "  不需要寫任何特殊函式！\n";
    }

    // ============================================================
    // 4. LeetCode 707. Design Linked List（擁有節點 → 必須 Rule of Five）
    // ============================================================
    std::cout << "\n=== LeetCode 707. Design Linked List ===\n";
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);          // 串列變成 1 -> 2 -> 3
        std::cout << "  建立後: " << list.toString() << "\n";
        std::cout << "  get(1) = " << list.get(1) << "\n";      // 2
        list.deleteAtIndex(1);          // 串列變成 1 -> 3
        std::cout << "  刪除索引 1 後: " << list.toString() << "\n";
        std::cout << "  get(1) = " << list.get(1) << "\n";      // 3

        // ★ 這裡就是 Rule of Five 的價值：複製後兩條串列各自獨立
        MyLinkedList copy = list;       // 複製建構子（深層複製）
        copy.addAtHead(99);
        std::cout << "  複製一份並在複製品前面加 99:\n";
        std::cout << "    原串列: " << list.toString() << "\n";
        std::cout << "    複製品: " << copy.toString() << "\n";
        std::cout << "  → 兩者互不影響；若少寫複製建構子，這裡會共用節點\n";
        std::cout << "     並在解構時 double free\n";

        // 移動：接管節點，來源變空
        MyLinkedList moved = std::move(copy);
        std::cout << "  移動後 moved = " << moved.toString()
                  << "，來源 copy = " << copy.toString() << "（已被移動）\n";
    }

    // ============================================================
    // 5. 日常實務：手寫 Rule of Five 包裝低階資源
    // ============================================================
    std::cout << "\n=== 日常實務：FileHandle（低階資源包裝）===\n";
    {
        std::cout << "  建立 a:\n";
        FileHandle a("/var/log/app.log");

        std::cout << "  複製成 b（資源真的被複製一份）:\n";
        FileHandle b = a;

        std::cout << "  移動成 c（接管 b 的 fd，不重新開啟）:\n";
        FileHandle c = std::move(b);

        std::cout << "  目前開啟中的描述符數量 = " << g_openHandles << "\n";
        std::cout << "  a.fd=" << a.fd() << " b.fd=" << b.fd()
                  << "（-1 表示已被移走）c.fd=" << c.fd() << "\n";
        std::cout << "  離開區塊，三個物件依序解構:\n";
    }
    std::cout << "  全部解構後，開啟中的描述符數量 = " << g_openHandles
              << "（歸零代表沒有洩漏）\n";

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  Rule of Five：解構+複製建構+複製賦值+移動建構+移動賦值\n";
    std::cout << "  只寫解構不寫其他 → 淺拷貝 → double free\n";
    std::cout << "  Rule of Zero（最佳）：只用 RAII 成員 → 什麼都不寫\n";
    std::cout << "  unique_ptr → 自動禁止複製，只能移動\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 註：被 std::move 之後的物件處於 valid but unspecified 狀態。
//     下方輸出中被移動的 String 與 User 顯示為空字串、MyLinkedList 顯示為 []，
//     是本檔各實作明確把來源置空的結果（String 設 nullptr、MyLinkedList 設
//     head_=nullptr）；對 std::string 這類標準型別，標準並不保證移動後一定為空。
//
// 註：FileHandle 的 fd 編號由本檔的 sys_open 模擬遞增產生，不是真正的系統呼叫，
//     因此每次執行都相同、可重現。

// === 預期輸出 ===
// ===== 1. Rule of Five =====
//   [建構] "Hello"
//   [複製建構] "Hello"
//   [建構] ""
//   [複製賦值] "Hello"
//   [移動建構] "Hello"
//   [建構] ""
//   [移動賦值] "Hello"
//   a: "" (已被移動)
//   b: "" (已被移動)
//   c: "Hello"
//   d: "Hello"
//   e: "Hello"
//   [解構] "Hello"
//   [解構] "Hello"
//   [解構] "Hello"
//   [解構] "null"
//   [解構] "null"
//
// ===== 2. 違反 Rule of Five =====
//   Broken 類別：只寫解構子 → double free！
//   Broken a(42); Broken b = a;
//   a.data_ 和 b.data_ 指向同一塊記憶體
//   兩者解構時都 delete → 💥 crash
//
// ===== 3. Rule of Zero（最推薦）=====
//   u2.name() = Alice
//   u1.name() = "" (已被移動)
//   不需要寫任何特殊函式！
//
// === LeetCode 707. Design Linked List ===
//   建立後: [1,2,3]
//   get(1) = 2
//   刪除索引 1 後: [1,3]
//   get(1) = 3
//   複製一份並在複製品前面加 99:
//     原串列: [1,3]
//     複製品: [99,1,3]
//   → 兩者互不影響；若少寫複製建構子，這裡會共用節點
//      並在解構時 double free
//   移動後 moved = [99,1,3]，來源 copy = []（已被移動）
//
// === 日常實務：FileHandle（低階資源包裝）===
//   建立 a:
//     [os] open("/var/log/app.log") → fd=3
//   複製成 b（資源真的被複製一份）:
//     [os] open("/var/log/app.log") → fd=4
//     FileHandle 複製（重新開啟一份）
//   移動成 c（接管 b 的 fd，不重新開啟）:
//     FileHandle 移動（接管 fd，不呼叫 open）
//   目前開啟中的描述符數量 = 2
//   a.fd=3 b.fd=-1（-1 表示已被移走）c.fd=4
//   離開區塊，三個物件依序解構:
//     [os] close(fd=4)
//     [os] close(fd=3)
//   全部解構後，開啟中的描述符數量 = 0（歸零代表沒有洩漏）
//
// === 重點整理 ===
//   Rule of Five：解構+複製建構+複製賦值+移動建構+移動賦值
//   只寫解構不寫其他 → 淺拷貝 → double free
//   Rule of Zero（最佳）：只用 RAII 成員 → 什麼都不寫
//   unique_ptr → 自動禁止複製，只能移動
