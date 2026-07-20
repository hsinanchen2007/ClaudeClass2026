// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 6  —  C++ 的動態記憶體：new / delete
// =============================================================================
//
// 【主題資訊 Information】
//   語法     ：T*  p   = new T;            // 配置 + 呼叫建構子
//              T*  arr = new T[n];         // 配置 n 個 + 逐一建構
//              delete   p;                 // 呼叫解構子 + 釋放
//              delete[] arr;               // 逐一解構 + 釋放
//   失敗行為 ：預設丟 std::bad_alloc（不是回傳 nullptr）
//              new (std::nothrow) T 才會改成回傳 nullptr
//   標準版本 ：C++98 起；std::nothrow 同樣是 C++98；
//              對齊版 operator new(size_t, align_val_t) 是 C++17
//   標頭檔   ：<new>（bad_alloc、nothrow、placement new）
//
// 【詳細解釋 Explanation】
//
// 【1. new 是「兩個動作」的複合體】
//   new T 實際上做兩件事，順序固定：
//       (1) 呼叫 operator new(sizeof(T)) 取得一塊夠大的原始記憶體；
//       (2) 在那塊記憶體上呼叫 T 的建構子，把「記憶體」變成「物件」。
//   delete p 則是反過來：
//       (1) 呼叫 p->~T() 解構子；
//       (2) 呼叫 operator delete(p) 把記憶體還回去。
//   這個「配置 + 建構」的組合，正是 malloc 缺少的那一半。
//   也因為建構子被呼叫了，new 出來的物件從第一刻起就處於有效狀態，
//   不像 malloc 給你一塊需要自己想辦法初始化的位元組。
//
// 【2. 為什麼 new 不需要 sizeof、不需要轉型】
//   new 是「語言的運算子」而不是函式庫函式，編譯器知道 T 是什麼型別，
//   所以大小由編譯器算、回傳型別直接就是 T*。這消滅了兩類 malloc 的經典錯誤：
//       int* p = (int*)malloc(sizeof(char) * 5);   // 大小算錯，編譯器不會攔
//       int* q = (int*)malloc(sizeof(int));        // 轉型寫錯型別也不會攔
//   new 讓這些錯誤在語法層次就不可能發生。
//
// 【3. new[] 與 delete[] 為什麼一定要配對】
//   new T[n] 要記住「n 是多少」，才知道 delete[] 時要呼叫幾次解構子。
//   常見實作會在使用者拿到的指標「前面」偷偷多配一小段，藏一個元素個數
//   （這也是為什麼 new[] 回傳的位址不一定等於實際配置的起點）。
//   於是：
//       delete   arr;   // 對 new[] 來的指標用 delete   → 未定義行為
//       delete[] p;     // 對 new   來的指標用 delete[] → 未定義行為
//   注意這裡不能說「一定會 crash」——它是未定義行為，可能看似正常執行、
//   可能破壞 heap 而在很久之後才爆，也可能立即中止。正因為不一定會立刻出事，
//   它才特別危險。對 int 這種沒有解構子的型別，某些實作看起來「好像沒事」，
//   但那只是恰好，不是保證。
//
// 【4. 配置失敗：例外而非回傳值】
//   new 失敗時丟 std::bad_alloc，而不是回傳 nullptr。所以這個檢查是無效的：
//       int* p = new int[huge];
//       if (p == nullptr) { ... }        // 永遠不會成立，失敗時已經丟例外了
//   要「失敗回傳 nullptr」的語意，得明確要求：
//       int* p = new (std::nothrow) int[huge];
//       if (p == nullptr) { ... }        // 這樣才對
//   例外的好處是：正常路徑不必到處寫檢查，錯誤會自動沿呼叫堆疊上拋，
//   而且無法被「忘記檢查回傳值」而默默忽略。
//
// 【5. 現代 C++ 的真正答案：不要手寫 new/delete】
//   本檔示範 new/delete 是為了理解機制，但實務上請優先用：
//       std::vector<int> v(5);                     // 取代 new int[5]
//       auto p = std::make_unique<Widget>();       // 取代 new Widget
//       auto s = std::make_shared<Widget>();       // 需要共享所有權時
//   理由是「例外安全」：如果 new 之後、delete 之前有任何一行丟出例外，
//   delete 就永遠不會執行，記憶體洩漏。RAII 讓解構子在 stack unwinding 時
//   自動被呼叫，這是手寫 new/delete 無論多小心都難以完全做到的。
//
// 【概念補充 Concept Deep Dive】
//   * operator new 是可以被覆寫的（全域或 class 專屬），這是實作記憶體池、
//     追蹤配置來源、對齊要求的標準機制。new 運算子本身的語意不變，
//     只是換掉底層取得記憶體的方式。
//   * placement new 只做「建構」不做「配置」：
//         void* raw = ::operator new(sizeof(T));
//         T* obj = new (raw) T(args);      // 在既有記憶體上建構
//         obj->~T();                        // 必須手動呼叫解構子
//         ::operator delete(raw);
//     這正是 std::vector 內部的做法——先配置 capacity 份原始記憶體，
//     再隨著 push_back 一個一個 placement new 建構出來。這解釋了
//     「capacity 與 size 為什麼是兩件事」：capacity 是記憶體，size 是已建構的物件數。
//   * new T 與 new T() 對 POD 不同：new int 是預設初始化（值不確定），
//     new int() 與 new int{} 是值初始化（歸零）。這個差別對 class 型別
//     不明顯（都會呼叫預設建構子），但對 int/double 是實打實的差異。
//     本檔輸出中會實際驗證 new int() 為 0。
//   * delete nullptr 是明確合法的空操作，跟 free(NULL) 一樣。
//
// 【注意事項 Pay Attention】
//   1. new/delete 與 new[]/delete[] 必須嚴格配對，錯配是未定義行為。
//   2. new 與 malloc 的記憶體不可混用：new 配的不能 free，malloc 配的不能 delete。
//   3. new 失敗丟 bad_alloc，檢查 nullptr 沒有意義；要 nullptr 語意請用
//      new (std::nothrow)。
//   4. delete 之後指標成為 dangling，應設為 nullptr；重複 delete 是 UB。
//   5. new int 不初始化、new int() 歸零，兩者不同。
//   6. 實務優先用 vector / unique_ptr / make_shared，不要手寫裸 new/delete。
//   7. 有虛擬函式而且會透過 base 指標 delete 的 class，解構子必須是 virtual，
//      否則只會呼叫到 base 的解構子（未定義行為／資源洩漏）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new / delete
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. new 和 malloc 的差別？請至少講三點。
//     答：(1) new 會呼叫建構子、delete 會呼叫解構子，malloc/free 完全不會；
//         (2) new 是運算子、型別安全，大小由編譯器算、回傳就是 T*，
//             malloc 是函式、回傳 void*，大小與轉型都要自己來；
//         (3) 失敗行為不同：new 丟 std::bad_alloc，malloc 回傳 NULL；
//         (4) new 可以被 class 層級覆寫 operator new 客製化配置策略。
//     追問：那 new 底層是不是就是呼叫 malloc？→ 常見實作的 operator new
//         確實會轉呼叫 malloc，但這是實作細節不是標準保證，
//         所以仍然不能把 new 配的記憶體拿去 free。
//
// 🔥 Q2. delete 和 delete[] 用錯會怎樣？
//     答：是未定義行為，不能斷言「一定 crash」。new T[n] 需要記錄元素個數
//         才知道要解構幾次，常見實作把它藏在使用者指標前面的一小段裡；
//         用 delete 釋放時，配置器拿到的起點與當初的不一致，
//         而且只會解構第一個元素。實際後果可能是立刻中止、heap 損壞後
//         在很久之後才爆、或表面上看似正常——正因為不一定立刻出事才危險。
//     追問：那為什麼 int 陣列用錯好像沒事？→ int 沒有解構子，某些實作對
//         trivially destructible 的型別不需要額外存元素個數，於是恰好不出事。
//         但這是實作細節，不是保證，換編譯器或換型別就會出事。
//
// ⚠️ 陷阱. 這段檢查為什麼是無效的？
//         int* p = new int[1000000000];
//         if (p == nullptr) { std::cout << "配置失敗\n"; return 1; }
//     答：new 配置失敗時丟出 std::bad_alloc，根本不會執行到 if。
//         也就是說這個 if 永遠不成立——不是因為配置永遠成功，
//         而是因為失敗時控制流已經跳走了。要讓它有意義，
//         必須改成 new (std::nothrow) int[1000000000]，
//         或改用 try/catch 捕捉 std::bad_alloc。
//     為什麼會錯：從 C 帶過來的直覺是「配置函式失敗回傳空指標」，
//         於是自然而然沿用 malloc 的檢查習慣。
//         但 C++ 把配置失敗歸類成「例外情況」，走的是完全不同的錯誤通道。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//
// 題目：自己實作一個單向鏈結串列，支援 get / addAtHead / addAtTail /
//       addAtIndex / deleteAtIndex。
// 為什麼用到本主題：這是 new/delete 少數「真的該手寫」的場景之一——
//   每個節點都要動態配置，刪除時要精準釋放，且必須在解構子裡清乾淨
//   整條串列，否則就是記憶體洩漏。它同時示範了本檔的三個重點：
//   new 會呼叫建構子（節點的 val/next 被正確初始化）、
//   delete 會呼叫解構子、以及「誰配置誰負責釋放」的所有權觀念。
// 複雜度：get/addAtIndex/deleteAtIndex 為 O(index)，addAtHead 為 O(1)。
// -----------------------------------------------------------------------------
class MyLinkedList {
public:
    MyLinkedList() : head_(nullptr), size_(0) {}

    // 解構子負責把整條串列釋放乾淨——漏掉這裡就是典型的記憶體洩漏
    ~MyLinkedList() {
        Node* cur = head_;
        while (cur != nullptr) {
            Node* next = cur->next;
            delete cur;              // 對應 addAtIndex 裡的 new Node
            cur = next;
        }
    }

    // 這個類別持有裸指標，若允許複製會造成 double free。
    // 明確刪除複製操作，是「擁有裸資源就必須處理拷貝語意」的示範。
    MyLinkedList(const MyLinkedList&)            = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    int get(int index) const {
        if (index < 0 || index >= size_) return -1;
        const Node* cur = head_;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->val;
    }

    void addAtHead(int val) { addAtIndex(0, val); }
    void addAtTail(int val) { addAtIndex(size_, val); }

    void addAtIndex(int index, int val) {
        if (index > size_) return;
        if (index < 0) index = 0;

        if (index == 0) {
            head_ = new Node(val, head_);      // new 會呼叫 Node 的建構子
        } else {
            Node* prev = head_;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            prev->next = new Node(val, prev->next);
        }
        ++size_;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size_) return;

        if (index == 0) {
            Node* victim = head_;
            head_ = head_->next;
            delete victim;                     // 精準釋放這一個節點
        } else {
            Node* prev = head_;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            Node* victim = prev->next;
            prev->next = victim->next;
            delete victim;
        }
        --size_;
    }

    std::string toString() const {
        std::string out = "[";
        for (const Node* cur = head_; cur != nullptr; cur = cur->next) {
            if (cur != head_) out += ", ";
            out += std::to_string(cur->val);
        }
        return out + "]";
    }

private:
    struct Node {
        int   val;
        Node* next;
        // 有了建構子，new Node(v, n) 出來的節點一定是初始化好的
        Node(int v, Node* n) : val(v), next(n) {}
    };

    Node* head_;
    int   size_;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用 RAII 取代手寫 new/delete：為什麼例外安全非它不可
//
// 情境：載入一份設定並建立連線物件。若中途丟出例外（設定不合法），
//   手寫 new/delete 的版本會洩漏，unique_ptr 版本不會。
// 為何用到本主題：這是「理解 new/delete 之後，為什麼實務上反而不該直接用」
//   的最有說服力示範。兩個版本都會被實際執行，輸出可直接對照。
// -----------------------------------------------------------------------------
struct Connection {
    std::string endpoint;
    explicit Connection(std::string ep) : endpoint(std::move(ep)) {
        std::cout << "    [Connection 建立] " << endpoint << "\n";
    }
    ~Connection() {
        std::cout << "    [Connection 釋放] " << endpoint << "\n";
    }
};

// 危險版本：中途丟例外就洩漏（此處僅示範控制流，不實際製造洩漏）
void loadConfigRaw(bool valid) {
    Connection* conn = new Connection("db-primary");
    try {
        if (!valid) throw std::runtime_error("設定不合法");
        std::cout << "    設定載入成功\n";
    } catch (...) {
        // 手寫版本必須在每一條錯誤路徑上都記得 delete，漏一條就洩漏
        delete conn;
        std::cout << "    (手寫版) 在 catch 裡補 delete，才沒有洩漏\n";
        return;
    }
    delete conn;
}

// 安全版本：不論從哪裡離開，解構子都會被呼叫
void loadConfigRaii(bool valid) {
    auto conn = std::make_unique<Connection>("db-replica");
    if (!valid) {
        std::cout << "    (RAII 版) 直接 return，unique_ptr 自動釋放\n";
        return;                       // 不需要任何清理程式碼
    }
    std::cout << "    設定載入成功\n";
}

int main() {
    std::cout << "=== 原始示範：配置單一變數 ===\n";
    // 配置單一變數
    int* p = new int;      // 不需要 sizeof，不需要轉型
    *p = 42;
    std::cout << "值: " << *p << std::endl;
    delete p;              // 使用 delete 而非 free
    p = nullptr;           // 避免 dangling pointer

    std::cout << "\n=== 原始示範：配置陣列 ===\n";
    // 配置陣列
    int* arr = new int[5]; // 配置陣列使用 new[]
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    std::cout << "陣列內容:";
    for (int i = 0; i < 5; i++) std::cout << " " << arr[i];
    std::cout << "\n";
    delete[] arr;          // 釋放陣列使用 delete[]
    arr = nullptr;

    std::cout << "\n=== new T 與 new T() 的差別（值初始化）===\n";
    {
        // new int 是預設初始化：對 int 而言值是不確定的，
        // 所以這裡「刻意不印出它的值」，只示範 new int() 保證歸零。
        int* a = new int();        // 值初始化 → 保證是 0
        int* b = new int{};        // 同上，C++11 的統一初始化語法
        std::cout << "new int()  = " << *a << "（保證為 0）\n";
        std::cout << "new int{}  = " << *b << "（保證為 0）\n";
        std::cout << "new int    的值是不確定的，不可依賴，故不印出\n";
        delete a;
        delete b;
    }

    std::cout << "\n=== 配置失敗的正確檢查方式 ===\n";
    {
        // 用 nothrow 版本要求「失敗回傳 nullptr」的語意。
        // 這裡刻意要求一個極大的量，讓它在多數機器上失敗。
        // 注意 huge 必須透過 volatile 讓它成為「執行期」才知道的值——
        // 若寫成編譯期常數，GCC 會在編譯階段就以
        // "size of array exceeds maximum object size" 直接拒絕編譯，
        // 我們就示範不到「執行期配置失敗」這件事了。
        volatile std::size_t hugeVolatile = static_cast<std::size_t>(1) << 60;
        const std::size_t huge = hugeVolatile;
        int* big = new (std::nothrow) int[huge];
        std::cout << "new (std::nothrow) 配置超大陣列 → "
                  << (big == nullptr ? "回傳 nullptr（如預期失敗）"
                                     : "居然成功了（機器記憶體很大）") << "\n";
        delete[] big;              // delete[] nullptr 是合法的空操作

        // 對照：預設的 new 失敗是丟例外，不是回傳 nullptr
        try {
            int* boom = new int[huge];
            std::cout << "  預設 new 居然成功了\n";
            delete[] boom;
        } catch (const std::bad_alloc& e) {
            std::cout << "  預設 new 失敗 → 捕捉到 std::bad_alloc: "
                      << e.what() << "\n";
        }
    }

    std::cout << "\n=== LeetCode 707. Design Linked List ===\n";
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);        // 變成 1 -> 2 -> 3
        std::cout << "建立後: " << list.toString() << "\n";
        std::cout << "get(1) = " << list.get(1) << "\n";
        list.deleteAtIndex(1);        // 變成 1 -> 3
        std::cout << "刪除 index 1 後: " << list.toString() << "\n";
        std::cout << "get(1) = " << list.get(1) << "\n";
        std::cout << "get(9) = " << list.get(9) << "（越界回傳 -1）\n";
        // list 離開 scope，解構子把剩下的節點全部 delete 掉
    }

    std::cout << "\n=== 日常實務：手寫 new/delete vs RAII ===\n";
    std::cout << "  [手寫版，設定不合法]\n";
    loadConfigRaw(false);
    std::cout << "  [RAII 版，設定不合法]\n";
    loadConfigRaii(false);
    std::cout << "  [RAII 版，設定合法]\n";
    loadConfigRaii(true);

    std::cout << "\n=== 現代 C++ 的建議寫法 ===\n";
    {
        std::vector<int> v(5);                        // 取代 new int[5]
        for (std::size_t i = 0; i < v.size(); ++i) v[i] = static_cast<int>(i) * 10;
        std::cout << "vector 取代 new int[5]:";
        for (int x : v) std::cout << " " << x;
        std::cout << "\n";

        auto up = std::make_unique<int>(42);          // 取代 new int
        std::cout << "make_unique<int>(42) = " << *up << "\n";
        std::cout << "兩者都不需要手寫 delete，離開 scope 自動釋放\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異6.cpp" -o demo6
// 記憶體檢查: valgrind --leak-check=full ./demo6
//
// 說明：
//   1.「配置失敗」那一段的結果與機器可用記憶體有關。本機（123 GB RAM）
//      實測兩者都如預期失敗；在記憶體極大的機器上有可能改為成功，
//      屆時會印出對應的另一句話。這不是不確定行為，只是與環境有關。
//   2. 本檔刻意不印出 new int（預設初始化）的值，因為那是不確定的值。
//   3. 本檔不印出任何指標位址——位址每次執行都不同（ASLR）。
//   4. bad_alloc 的 e.what() 文字由實作決定，libstdc++ 上是
//      "std::bad_alloc"，換標準函式庫可能不同。

// === 預期輸出 ===
// === 原始示範：配置單一變數 ===
// 值: 42
//
// === 原始示範：配置陣列 ===
// 陣列內容: 0 10 20 30 40
//
// === new T 與 new T() 的差別（值初始化）===
// new int()  = 0（保證為 0）
// new int{}  = 0（保證為 0）
// new int    的值是不確定的，不可依賴，故不印出
//
// === 配置失敗的正確檢查方式 ===
// new (std::nothrow) 配置超大陣列 → 回傳 nullptr（如預期失敗）
//   預設 new 失敗 → 捕捉到 std::bad_alloc: std::bad_alloc
//
// === LeetCode 707. Design Linked List ===
// 建立後: [1, 2, 3]
// get(1) = 2
// 刪除 index 1 後: [1, 3]
// get(1) = 3
// get(9) = -1（越界回傳 -1）
//
// === 日常實務：手寫 new/delete vs RAII ===
//   [手寫版，設定不合法]
//     [Connection 建立] db-primary
//     [Connection 釋放] db-primary
//     (手寫版) 在 catch 裡補 delete，才沒有洩漏
//   [RAII 版，設定不合法]
//     [Connection 建立] db-replica
//     (RAII 版) 直接 return，unique_ptr 自動釋放
//     [Connection 釋放] db-replica
//   [RAII 版，設定合法]
//     [Connection 建立] db-replica
//     設定載入成功
//     [Connection 釋放] db-replica
//
// === 現代 C++ 的建議寫法 ===
// vector 取代 new int[5]: 0 10 20 30 40
// make_unique<int>(42) = 42
// 兩者都不需要手寫 delete，離開 scope 自動釋放
