// =============================================================================
//  第 19 課：動態對象的創建與銷毀 1  —  new 與 delete 的基本語法
// =============================================================================
//
// 【主題資訊 Information】
//   語法：T* p = new T;          // 預設初始化（對 int 等純量 = 不初始化）
//         T* p = new T();        // 值初始化（純量會被歸零）
//         T* p = new T(args);    // 直接初始化
//         T* p = new T{args};    // C++11 大括號初始化
//         delete p;              // 呼叫解構函式 + 釋放記憶體
//   標準版本：new/delete 自 C++98 起；大括號初始化為 C++11。
//   標頭檔：本檔用 <iostream>、<string>；new/delete 本身是語言運算子，
//           不需要引入標頭檔（但 std::bad_alloc、std::nothrow 在 <new>）。
//   複雜度：配置/釋放通常是 O(1) 攤還，但實際成本取決於配置器；
//           它涉及系統呼叫與鎖，遠比堆疊配置昂貴。
//
// 【詳細解釋 Explanation】
//
// 【1. new 做了「兩件事」，delete 也做了「兩件事」】
//   這是理解一切的起點：
//       new T(args)  =  ① operator new(sizeof(T)) 配置原始記憶體
//                       ② 在該記憶體上呼叫 T 的建構函式
//       delete p     =  ① 呼叫 p 所指物件的解構函式
//                       ② operator delete(p) 歸還記憶體
//   注意順序是對稱的：建構在配置之後，解構在釋放之前。
//   ★ 這兩件事「可以被拆開」——這正是 placement new 與 STL 配置器的基礎：
//       void* mem = operator new(sizeof(T));   // 只配置
//       T* p = new (mem) T(args);              // 只建構（placement new）
//       p->~T();                               // 只解構（明確呼叫）
//       operator delete(mem);                  // 只釋放
//     std::vector 正是這樣運作的：它一次配置一大塊記憶體（capacity），
//     再依需要逐一建構元素（size），所以 capacity 和 size 才會不同。
//
// 【2. 三種初始化的差別（新手最常踩的坑）】
//       int* a = new int;      // 預設初始化：對 int 而言「不初始化」→ 值不確定
//       int* b = new int();    // 值初始化：int 會被歸零 → 保證是 0
//       int* c = new int(42);  // 直接初始化 → 42
//       int* d = new int{42};  // C++11 大括號 → 42，且禁止窄化轉換
//   ★ `new int` 與 `new int()` 只差一對括號，結果卻天差地遠。
//     前者讀取其值是未定義行為；後者保證為 0。
//     對「有使用者定義建構函式的類別」則沒有差別——兩者都會呼叫該建構函式。
//   ★ 本檔刻意「不印出」未初始化指標所指的值：
//     讀取不確定值是未定義行為，且每次執行結果都不同，
//     把它寫成「預期輸出」既不正確也無法驗證。
//
// 【3. 大括號初始化為什麼比較安全？】
//       int* p = new int{3.9};    // 編譯錯誤：窄化轉換（double → int）
//       int* q = new int(3.9);    // 編譯通過，安靜地截斷成 3
//   大括號禁止會遺失資訊的窄化轉換，能在編譯期抓到 bug。
//   這是 C++11 推薦「統一初始化」的主要理由之一。
//
// 【4. 堆積 vs 堆疊：什麼時候才該用 new？】
//   實務準則是「預設用堆疊，必要時才用堆積」。需要 new 的情況只有三種：
//     (a) 物件的生命週期必須超出目前的作用域
//     (b) 物件太大，放堆疊會爆（典型堆疊只有 8 MB）
//     (c) 大小/型別要到執行期才知道（多型物件、動態陣列）
//   除此之外一律用區域物件——它更快（只是移動堆疊指標）、
//   自動釋放、不會洩漏、對快取更友善。
//   ★ 而且就算真的需要堆積，現代 C++ 也建議用 std::vector 或
//     std::unique_ptr / std::make_unique，而不是裸 new（見 9.cpp）。
//
// 【概念補充 Concept Deep Dive】
//   ● delete 一個指標時，編譯器怎麼知道要釋放多大？
//     它不需要知道——配置器在配置時已把區塊大小記在使用者資料之前的
//     「標頭（header）」裡，或以大小分類的 bin 管理。這也是為什麼
//     堆積配置有額外的記憶體開銷（每次配置通常多花 8～16 bytes）。
//   ● 為什麼 delete 需要知道型別？因為它要呼叫「正確的解構函式」。
//     這就是為什麼 `delete` 一個 void* 是未定義行為——
//     編譯器不知道該呼叫誰的解構函式。
//   ● operator new 失敗時「拋出 std::bad_alloc」，不是回傳 nullptr。
//     所以 `if (p == nullptr)` 這種 C 風格的檢查對 new 是無效的
//     （除非用 new(std::nothrow)），詳見 5.cpp。
//   ● new 回傳的記憶體對齊：保證滿足該型別的對齊需求；
//     C++17 起對 over-aligned 型別（如 alignas(64)）會呼叫
//     對齊版本的 operator new。
//
// 【注意事項 Pay Attention】
//   1. `new int` 不初始化，讀取其值是未定義行為；`new int()` 才保證為 0。
//   2. new 與 delete 必須配對；new[] 必須配 delete[]（見 3.cpp、4.cpp）。
//   3. delete 之後指標成為懸空指標，應設為 nullptr（見 6.cpp）。
//   4. delete nullptr 是安全的、什麼都不做；但 double delete 是未定義行為。
//   5. 堆積配置遠比堆疊昂貴（涉及配置器邏輯與可能的鎖），不要濫用。
//   6. 現代 C++ 應優先使用 std::vector / std::unique_ptr，
//      裸 new/delete 主要出現在學習與底層實作中。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new 與 delete
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. new 和 delete 各做了哪些事？和 malloc/free 的本質差別是什麼？
//     答：new 做兩件事——配置記憶體（operator new）+ 呼叫建構函式；
//         delete 也做兩件事——呼叫解構函式 + 釋放記憶體。
//         malloc/free 只做配置與釋放，完全不碰建構/解構，
//         所以拿它們管理類別物件會導致成員未初始化、資源不被清理。
//     追問：這兩件事可以拆開嗎？
//         → 可以。operator new 只配置、placement new 只在既有記憶體上建構、
//           明確呼叫 p->~T() 只解構。std::vector 正是靠這種拆分，
//           才能讓 capacity（已配置）與 size（已建構）不同。
//
// 🔥 Q2. `new int` 和 `new int()` 有什麼差別？
//     答：差一對括號，語意完全不同。`new int` 是預設初始化，
//         對 int 這種純量型別等於「不初始化」，其值不確定，讀取是未定義行為；
//         `new int()` 是值初始化，保證被歸零。
//         但對「有使用者定義建構函式的類別」兩者沒有差別，都會呼叫該建構函式。
//     追問：那 `new int{}` 呢？
//         → 同樣是值初始化，保證為 0，而且大括號會禁止窄化轉換，
//           是 C++11 之後較推薦的寫法。
//
// ⚠️ 陷阱. delete p; 之後，p 指向的記憶體會被清空、p 會變成 nullptr 嗎？
//     答：兩個都不會。delete 只是呼叫解構函式並把記憶體歸還給配置器；
//         記憶體內容通常原封不動（配置器可能只改動它自己的管理標頭），
//         而 p 這個變數的值完全沒被碰過，仍然指著原來的位址。
//     為什麼會錯：把 delete 想成「清除」。它其實是「歸還使用權」——
//         就像退房不代表房間會被清空，只代表你不能再進去。
//         正因為資料還在，用已 delete 的指標讀取往往「看起來還正常」，
//         直到那塊記憶體被下一次配置拿去用，才安靜地讀到別人的資料。
//         這也是為什麼 delete 之後應立刻把指標設為 nullptr。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Hero {
private:
    string name;
    int level;

public:
    Hero(const string& n, int lv) : name(n), level(lv) {
        cout << "  [建構] " << name << " Lv." << level << endl;
    }
    
    ~Hero() {
        cout << "  [解構] " << name << " Lv." << level << endl;
    }
    
    void print() const {
        cout << "  " << name << " (Lv." << level << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：實作一個單向鏈結串列，支援 get / addAtHead / addAtTail /
//         addAtIndex / deleteAtIndex。
//   為什麼用到本主題：鏈結串列的每個節點都必須「獨立配置、獨立釋放」，
//         這正是 new/delete 最典型也最正當的用途——
//         節點數量在執行期才決定，且節點的生命週期必須超出建立它的函式。
//         特別注意 deleteAtIndex 與解構函式：每一個 new 出來的節點
//         都必須恰好被 delete 一次，多一次是未定義行為、少一次就是洩漏。
//   複雜度：get / addAtIndex / deleteAtIndex 為 O(n)，addAtHead 為 O(1)。
// -----------------------------------------------------------------------------
class MyLinkedList {
    struct Node {
        int   val;
        Node* next;
        Node(int v) : val(v), next(nullptr) {}
    };
    Node* head;
    int   count;

public:
    MyLinkedList() : head(nullptr), count(0) {}

    // ★ 解構函式必須逐一釋放所有節點，否則整條串列都會洩漏
    ~MyLinkedList() {
        Node* cur = head;
        while (cur != nullptr) {
            Node* next = cur->next;   // 必須先存下 next，否則 delete 後就找不到了
            delete cur;
            cur = next;
        }
    }

    // 本範例不需要複製，明確禁止以免產生淺複製導致 double delete
    MyLinkedList(const MyLinkedList&) = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    int get(int index) const {
        if (index < 0 || index >= count) return -1;
        Node* cur = head;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->val;
    }

    void addAtHead(int val) {
        Node* node = new Node(val);
        node->next = head;
        head = node;
        ++count;
    }

    void addAtTail(int val) {
        if (head == nullptr) { addAtHead(val); return; }
        Node* cur = head;
        while (cur->next != nullptr) cur = cur->next;
        cur->next = new Node(val);
        ++count;
    }

    void addAtIndex(int index, int val) {
        if (index > count) return;          // 超過長度：不插入
        if (index <= 0) { addAtHead(val); return; }
        Node* cur = head;
        for (int i = 0; i < index - 1; ++i) cur = cur->next;
        Node* node = new Node(val);
        node->next = cur->next;
        cur->next = node;
        ++count;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= count) return;
        if (index == 0) {
            Node* dead = head;
            head = head->next;
            delete dead;                    // 恰好釋放一次
            --count;
            return;
        }
        Node* cur = head;
        for (int i = 0; i < index - 1; ++i) cur = cur->next;
        Node* dead = cur->next;
        cur->next = dead->next;
        delete dead;
        --count;
    }

    string toString() const {
        string s = "[";
        for (Node* cur = head; cur != nullptr; cur = cur->next) {
            s += to_string(cur->val);
            if (cur->next != nullptr) s += ",";
        }
        return s + "]";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】依執行期大小配置緩衝區
//   情境：讀取一筆網路封包，長度直到讀到標頭才知道。
//   這是 new 的合理用途之一——大小要到執行期才確定。
//   但注意本例的正解其實是 std::vector<char>：它同樣是動態大小，
//   卻能自動釋放、支援例外安全。這裡刻意兩種都寫，以便對照。
// -----------------------------------------------------------------------------
size_t computePayloadSize(const string& header) {
    // 模擬：從 header 解析出 payload 長度
    size_t pos = header.find("len=");
    if (pos == string::npos) return 0;
    return static_cast<size_t>(stoi(header.substr(pos + 4)));
}

int main() {
    cout << "=== new 與 delete 基本用法 ===" << endl;
    
    // ====== 基本型別 ======
    cout << "\n--- 基本型別 ---" << endl;
    int* p1 = new int;          // 預設初始化：對 int 而言不初始化，值不確定
    int* p2 = new int(42);      // 分配一個 int 並初始化為 42
    int* p3 = new int{100};     // C++11 大括號初始化
    int* p4 = new int();        // 值初始化：保證為 0

    // ★ 刻意不印出 *p1：讀取未初始化的 int 是未定義行為，
    //   其值每次執行都不同，不能寫成可驗證的預期輸出。
    cout << "  *p1 = (new int 未初始化，讀取是未定義行為，故不印出)" << endl;
    *p1 = 7;                    // 先寫入，之後才能安全讀取
    cout << "  寫入 7 之後 *p1 = " << *p1 << endl;
    cout << "  *p2 = " << *p2 << endl;
    cout << "  *p3 = " << *p3 << endl;
    cout << "  *p4 = " << *p4 << " (new int() 值初始化，保證為 0)" << endl;
    
    delete p1;
    delete p2;
    delete p3;
    delete p4;
    
    // ====== 類別物件 ======
    cout << "\n--- 類別物件 ---" << endl;
    Hero* hero = new Hero("勇者", 10);   // new = 分配記憶體 + 調用建構函數, 返回指向物件的指標
    hero->print();
    delete hero;                          // delete = 調用解構函數 + 釋放記憶體, 釋放後指標變成懸空指標（dangling pointer）
    hero = nullptr;                       // 將指標設為 nullptr，避免懸空指標

    // ====== LeetCode 707 ======
    cout << "\n=== LeetCode 707. Design Linked List ===" << endl;
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);            // 變成 [1,2,3]
        cout << "  addAtHead(1), addAtTail(3), addAtIndex(1,2) -> "
             << list.toString() << endl;
        cout << "  get(1)  = " << list.get(1) << endl;
        list.deleteAtIndex(1);            // 變成 [1,3]
        cout << "  deleteAtIndex(1) -> " << list.toString() << endl;
        cout << "  get(1)  = " << list.get(1) << endl;
        cout << "  get(9)  = " << list.get(9) << " (越界回傳 -1)" << endl;
        cout << "  離開作用域，解構函式會逐一 delete 每個節點" << endl;
    }

    // ====== 實務範例 ======
    cout << "\n=== 日常實務：執行期才知道大小的緩衝區 ===" << endl;
    {
        string header = "PKT type=data len=16";
        size_t n = computePayloadSize(header);
        cout << "  從標頭解析出 payload 長度 = " << n << endl;

        // 寫法 A：裸 new[]（需要自己記得 delete[]）
        char* buf = new char[n];
        for (size_t i = 0; i < n; ++i) buf[i] = static_cast<char>('A' + (i % 26));
        cout << "  裸 new[] 填入內容: " << string(buf, n) << endl;
        delete[] buf;                     // 必須配對 delete[]，見 3.cpp

        // 寫法 B：std::vector（推薦，自動釋放且例外安全）
        vector<char> vbuf(n);
        for (size_t i = 0; i < n; ++i) vbuf[i] = static_cast<char>('a' + (i % 26));
        cout << "  vector 填入內容  : " << string(vbuf.data(), vbuf.size()) << endl;
        cout << "  → 兩者結果相同，但 vector 不需要手動釋放，也不怕中途拋例外" << endl;
    }
    
    cout << "\n--- 完成 ---" << endl;
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：動態對象的創建與銷毀（new  delete）1.cpp" -o newdel1

// 【輸出說明】本檔刻意「不印出」未初始化記憶體（new int）所指的值：
//   讀取不確定值是未定義行為，其結果每次執行都不同，
//   不能寫成可驗證的預期輸出。因此下列輸出完全確定且可重現。

// === 預期輸出 ===
// === new 與 delete 基本用法 ===
//
// --- 基本型別 ---
//   *p1 = (new int 未初始化，讀取是未定義行為，故不印出)
//   寫入 7 之後 *p1 = 7
//   *p2 = 42
//   *p3 = 100
//   *p4 = 0 (new int() 值初始化，保證為 0)
//
// --- 類別物件 ---
//   [建構] 勇者 Lv.10
//   勇者 (Lv.10)
//   [解構] 勇者 Lv.10
//
// === LeetCode 707. Design Linked List ===
//   addAtHead(1), addAtTail(3), addAtIndex(1,2) -> [1,2,3]
//   get(1)  = 2
//   deleteAtIndex(1) -> [1,3]
//   get(1)  = 3
//   get(9)  = -1 (越界回傳 -1)
//   離開作用域，解構函式會逐一 delete 每個節點
//
// === 日常實務：執行期才知道大小的緩衝區 ===
//   從標頭解析出 payload 長度 = 16
//   裸 new[] 填入內容: ABCDEFGHIJKLMNOP
//   vector 填入內容  : abcdefghijklmnop
//   → 兩者結果相同，但 vector 不需要手動釋放，也不怕中途拋例外
//
// --- 完成 ---
