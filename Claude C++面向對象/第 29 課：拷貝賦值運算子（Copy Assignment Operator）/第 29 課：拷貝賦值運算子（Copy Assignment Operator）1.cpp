// =============================================================================
//  第 29 課：拷貝賦值運算子（Copy Assignment Operator）1  —  Copy-and-Swap 慣用法
// =============================================================================
//
// 【主題資訊 Information】
//   傳統簽名： ClassName& operator=(const ClassName& other);
//   C-and-S ： ClassName& operator=(ClassName other);        // ★ 本檔採用：按值傳參
//   標準版本： C++98 起即有；「傳值 + swap」在 C++11 之後才真正划算（多一條 move 路徑）
//   複雜度  ： O(n)（n = 被管理的資源大小；一定要複製一份，無法更省）
//   標頭檔  ： <utility>（std::swap、std::move）、<cstring>（strlen/strcpy）
//   回傳型別： 必須是 ClassName&（回傳 *this），才能支援鏈式賦值 a = b = c
//
// 【詳細解釋 Explanation】
//
// 【1. 拷貝賦值 vs 拷貝建構：只差一件事，卻決定了整個寫法】
//   MyClass b = a;   → 拷貝「建構」：b 這塊記憶體上還沒有物件，從零蓋起來
//   b = a;           → 拷貝「賦值」：b 已經是一個完整、活著的物件
//   差別只有「目標存不存在」，但後果很大：拷貝建構不必善後，拷貝賦值必須先
//   處理掉自己身上的舊資源，否則就是記憶體洩漏。整個 operator= 的複雜度
//   全部來自這句「要先善後」。
//
// 【2. 傳統寫法的兩個地雷】
//   典型的傳統寫法是：
//       if (this == &other) return *this;   // ① 自我賦值檢查
//       delete[] m_data;                    // ② 先釋放舊的
//       m_data = new char[...];             // ③ 再配置新的
//   地雷 A：少了 ① 就會炸。a = a 時，② 已經把資料刪掉，③ 再從「已被刪除的
//           記憶體」複製資料 —— 這是讀取已釋放記憶體，屬於 undefined behavior，
//           不保證任何固定結果（可能看似正常、可能垃圾資料、可能 crash）。
//   地雷 B：就算加了 ①，仍然不是異常安全的。② 已經 delete、③ 的 new 若丟出
//           std::bad_alloc，函式就地離開，物件留下一個懸空的 m_data，
//           解構時會二次釋放。這叫「連基本保證都沒有」。
//
// 【3. Copy-and-Swap：用一個順序調換，同時解掉上面兩顆地雷】
//       ClassName& operator=(ClassName other) {  // ← 按「值」傳入，先複製
//           swap(other);                          // ← 再交換
//           return *this;
//       }
//   關鍵在於「複製發生在最前面、而且發生在參數上」：
//     * 所有可能失敗的動作（配置記憶體、複製資料）都在進入函式本體「之前」
//       就做完了。如果失敗，例外從呼叫端拋出，*this 一個位元組都沒被動過
//       —— 這就是 strong exception guarantee（強例外保證）。
//     * 進到函式本體之後只剩 swap，而 swap 只是交換指標，是 noexcept 的，
//       不可能失敗。「先做完所有會失敗的事，再做不會失敗的事」是異常安全的通則。
//     * 自我賦值自動正確：a = a 時 other 是 a 的一份「獨立副本」，交換之後
//       a 仍持有一份完整資料，只是換了一塊記憶體。不需要 if (this == &other)。
//
// 【4. 為什麼參數要「按值」而不是 const&】
//   按值傳參等於把「要不要複製、怎麼複製」這個決定權交還給編譯器：
//     * 傳左值（b = a）→ 參數用拷貝建構初始化，該複製的複製。
//     * 傳右值（e = SimpleString("Phoenix")）→ C++17 起是保證的 copy elision，
//       暫時物件「直接就地」建構成參數，一次多餘的複製都沒有。
//       本檔測試 4 的實跑輸出可以親眼看到：只有 [建構]，沒有 [拷貝建構]。
//   若類別另外有 move constructor，傳右值時還會走 move —— 一份 operator=
//   同時涵蓋 copy 與 move 兩種語意，這是 copy-and-swap 最漂亮的地方。
//
// 【5. 為什麼一定要回傳 ClassName&】
//   為了讓 a = b = c 成立，而且語意與內建型別一致（右結合：a = (b = c)）。
//   回傳 void 會讓鏈式賦值編不過；回傳 ClassName（值）則會憑空多一次複製，
//   而且 (a = b) = c 這種寫法會改到暫時物件上，語意就錯了。
//
// 【概念補充 Concept Deep Dive】
//
// (A) swap 為什麼必須 noexcept
//   copy-and-swap 的全部安全性都押在「swap 不會失敗」這個前提上。成員若是
//   指標與整數，交換就只是搬幾個暫存器的值，天生不會丟例外，標成 noexcept
//   是誠實的宣告。另外 std::vector 之類的容器在重新配置時會用
//   std::move_if_noexcept 檢查你的 move/swap 是否 noexcept —— 不是 noexcept
//   就退回去做「複製」以保住強例外保證，效能直接掉一個檔次。
//
// (B) copy-and-swap 的代價：它不是免費的
//   傳統寫法在「舊容量夠大」時可以直接覆寫既有緩衝區，一次配置都不用；
//   copy-and-swap 則一定會配置一塊新的、再把舊的丟掉。所以 std::string、
//   std::vector 的 operator= 實作反而不用 copy-and-swap，而是自己判斷
//   capacity 夠不夠、能重用就重用。慣用法是給「寫得對比寫得快重要」的
//   一般類別用的，不是無腦最佳解。
//
// (C) 千萬不要在 operator= 裡寫 std::swap(*this, other)
//   std::swap 的泛型版本內部是「一次 move 建構 + 兩次 move 賦值」。如果你的
//   move 賦值又轉呼叫 std::swap(*this, other)，就會無限互相呼叫直到堆疊耗盡。
//   本檔的做法是呼叫「自己的成員 swap」，它只交換成員、不碰 operator=，
//   所以沒有這個遞迴問題。
//
// (D) 兩個版本不能並存
//   同時宣告 operator=(const T&) 與 operator=(T) 會讓所有賦值變成
//   ambiguous overload（本機 g++ 15.2 實測：error: ambiguous overload for
//   'operator=' ... there are 2 candidates）。要嘛傳值，要嘛傳 const 參考，
//   二選一。
//
// 【注意事項 Pay Attention】
//   1. 自我賦值在 copy-and-swap 下是「安全但不是免費」：仍會實際複製一份。
//      若 profiling 顯示自我賦值很頻繁，才值得加 if (this == &other) 短路。
//   2. 成員 swap 請標 noexcept，並額外提供非成員 swap 讓 ADL 找得到。
//   3. 有繼承時，衍生類別的 operator= 必須顯式呼叫 Base::operator=(other)，
//      否則基底部分不會被賦值（編譯器不會提醒你）。
//   4. 成員的初始化順序永遠依「宣告順序」，與初始化列表的書寫順序無關；
//      寫反了 g++ 會用 -Wreorder 警告。本檔把 m_len 宣告在 m_data 之前，
//      正是因為 m_data 的初始值用到了 m_len。
//   5. 寫了自訂的 operator= 或解構函式，就要回頭檢查三/五法則（第 30、34 課）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】拷貝賦值運算子與 Copy-and-Swap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 拷貝建構與拷貝賦值的差別是什麼？各自何時被呼叫？
//     答：差別只在「目標物件是否已經存在」。MyClass b = a; 是拷貝建構
//         （b 尚未存在，從未初始化的記憶體開始蓋）；b = a; 是拷貝賦值
//         （b 已是完整物件，必須先處理掉自己的舊資源再接手新值）。
//         也因此只有拷貝賦值需要煩惱自我賦值與舊資源釋放。
//     追問：MyClass b = a; 明明有等號，為什麼不是賦值？
//         → 有等號的是「複製初始化」語法，只要伴隨宣告就是初始化，
//           呼叫的是建構函式，不會呼叫 operator=。
//
// 🔥 Q2. Copy-and-Swap 為什麼可以省掉 if (this == &other)？
//     答：因為複製發生在「參數初始化」階段，早於任何破壞性動作。a = a 時，
//         參數 other 是 a 的一份獨立副本，函式本體只是把 *this 和這份副本
//         交換；交換後 *this 仍握有完整資料，離開時解構的是舊的那塊。
//         全程沒有「先刪掉再讀自己」的空窗，所以天然安全。
//     追問：那還需要自我賦值檢查嗎？
//         → 正確性上不需要；效能上，a = a 仍會真的配置並複製一份，
//           若這在你的熱路徑很常發生，才值得加一行短路。
//
// 🔥 Q3. 為什麼 operator= 要回傳 ClassName& 而不是 void 或 ClassName？
//     答：回傳 *this 才能支援鏈式賦值 a = b = c，並與內建型別語意一致。
//         回傳 void 會讓鏈式賦值編不過；回傳值（ClassName）則多一次複製，
//         而且 (a = b) = c 會改在暫時物件上，語意錯誤。
//     追問：a = b = c 的求值順序？→ 賦值是右結合，先算 b = c，
//           其結果（b 的參考）再賦給 a。
//
// ⚠️ 陷阱 1. 「傳統寫法只要加了自我賦值檢查就安全了」——為什麼還是錯？
//     答：自我賦值檢查只解決了 a = a，沒解決例外安全。傳統寫法是
//         「先 delete[] 舊的，再 new 新的」；一旦 new 丟出 std::bad_alloc，
//         函式當場離開，m_data 已是懸空指標，解構時會二次釋放。
//         正確做法是把順序倒過來：先配置成功、再釋放舊的（或直接用
//         copy-and-swap，讓配置全部發生在參數初始化階段）。
//     為什麼會錯：多數人把「自我賦值」和「例外安全」當成同一個問題，
//         以為擋掉 a = a 就萬事太平；但這是兩個獨立的失效模式，
//         一個來自別名（aliasing），一個來自控制流被例外中斷。
//
// ⚠️ 陷阱 2. 「兩個版本都寫上去，讓編譯器挑最好的」——會發生什麼事？
//     答：同時提供 operator=(const T&) 與 operator=(T) 會讓 a = b 變成
//         ambiguous overload，直接編譯失敗（本機 g++ 15.2 實測：
//         error: ambiguous overload for 'operator=' ... there are 2 candidates）。
//     為什麼會錯：以為 overload resolution 會「挑比較特化的那個」。
//         但對左值實參而言，兩者都是一次同等級的使用者定義轉換序列，
//         沒有任何一方更好，於是判定為模稜兩可。
//
// ⚠️ 陷阱 3. 在 operator= 內寫 std::swap(*this, other) 為什麼會爆掉？
//     答：std::swap 的泛型實作是「一次 move 建構 + 兩次 move 賦值」。若 move
//         賦值又回頭呼叫 std::swap(*this, other)，兩者互相呼叫形成無窮遞迴，
//         最終堆疊耗盡。要呼叫的是「自己的成員 swap」（只交換成員），
//         像本檔的 swap(other) 那樣。
//     為什麼會錯：把 std::swap 想成「編譯器內建的位元交換」，
//         沒意識到它本身就是用 move 建構／賦值拼出來的。
// ═══════════════════════════════════════════════════════════════════════════

// lesson29_copy_assignment.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson29 lesson29_copy_assignment.cpp

#include <iostream>
#include <cstring>
#include <utility>  // std::swap

class SimpleString {
private:
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在前。
    std::size_t m_len;
    char* m_data;

public:
    // ──────── 建構函數 ────────
    SimpleString(const char* str = "")
        : m_len(std::strlen(str))
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝建構函數 ────────
    SimpleString(const SimpleString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝賦值運算子（Copy-and-Swap）────────
    SimpleString& operator=(SimpleString other) {  // 按值傳入！
        std::cout << "  [拷貝賦值] swap with \"" << other.m_data << "\"\n";
        swap(other);       // 交換內容
        return *this;      // other 帶著舊資料離開並解構
    }

    // ──────── 解構函數 ────────
    ~SimpleString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    // ──────── swap ────────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──────── 存取 ────────
    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }
};

#include <string>
#include <stdexcept>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：自行設計單向鏈結串列，支援 get / addAtHead / addAtTail /
//         addAtIndex / deleteAtIndex。
//   為什麼用到本主題：這是少數「必須自己 new/delete 節點」的 LeetCode 題目，
//         也就是典型的資源管理類別。誠實地說：LeetCode 的評測程式從頭到尾
//         只會用到一個實例，永遠不會去拷貝它，所以不寫 operator= 也能 AC。
//         但這正是陷阱所在 —— 一旦把它當成值型別（放進 vector、當回傳值、
//         做快照備份），預設的淺拷貝會讓兩個物件共用同一批節點，
//         最後雙重釋放。下面用本課的 copy-and-swap 把它補成真正安全的類別。
// -----------------------------------------------------------------------------
class MyLinkedList {
private:
    struct Node {
        int val;
        Node* next;
        Node(int v, Node* n = nullptr) : val(v), next(n) {}
    };
    Node* m_head;
    int   m_size;

public:
    MyLinkedList() : m_head(nullptr), m_size(0) {}

    // 拷貝建構：逐節點深拷貝（若省略，兩個串列會共用節點）
    MyLinkedList(const MyLinkedList& other) : m_head(nullptr), m_size(other.m_size) {
        Node** tail = &m_head;
        for (Node* p = other.m_head; p; p = p->next) {
            *tail = new Node(p->val);
            tail = &((*tail)->next);
        }
    }

    void swap(MyLinkedList& other) noexcept {
        std::swap(m_head, other.m_head);
        std::swap(m_size, other.m_size);
    }

    // ★ 本課主題：copy-and-swap 版的拷貝賦值
    MyLinkedList& operator=(MyLinkedList other) {
        swap(other);
        return *this;
    }

    ~MyLinkedList() {
        while (m_head) {
            Node* nxt = m_head->next;
            delete m_head;
            m_head = nxt;
        }
    }

    int get(int index) const {
        if (index < 0 || index >= m_size) return -1;
        Node* p = m_head;
        for (int i = 0; i < index; ++i) p = p->next;
        return p->val;
    }

    void addAtHead(int val) { m_head = new Node(val, m_head); ++m_size; }

    void addAtIndex(int index, int val) {
        if (index > m_size) return;
        if (index < 0) index = 0;
        Node** cur = &m_head;
        for (int i = 0; i < index; ++i) cur = &((*cur)->next);
        *cur = new Node(val, *cur);
        ++m_size;
    }

    void addAtTail(int val) { addAtIndex(m_size, val); }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= m_size) return;
        Node** cur = &m_head;
        for (int i = 0; i < index; ++i) cur = &((*cur)->next);
        Node* dead = *cur;
        *cur = dead->next;
        delete dead;
        --m_size;
    }

    int size() const { return m_size; }

    std::string dump() const {
        std::string s = "[";
        for (Node* p = m_head; p; p = p->next) {
            if (p != m_head) s += ", ";
            s += std::to_string(p->val);
        }
        return s + "]";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定熱重載（hot reload）：套用失敗時線上設定必須毫髮無傷
//   情境：服務執行中收到 SIGHUP，重新讀取設定檔並套用到「已存在」的設定物件。
//   為什麼用到本主題：reload 的最後一步就是對既有物件做賦值。若 operator=
//         沒有強例外保證，解析到一半失敗會留下「半套設定」——連線字串換了、
//         逾時值還是舊的 —— 這比直接沿用整份舊設定危險得多。
//         copy-and-swap 讓結果只有兩種：整批換新，或完全不動。
// -----------------------------------------------------------------------------
SimpleString parseEndpoint(const std::string& configLine) {
    const std::string key = "endpoint=";
    std::size_t pos = configLine.find(key);
    if (pos == std::string::npos) {
        throw std::runtime_error("設定檔缺少 endpoint 欄位");
    }
    std::string value = configLine.substr(pos + key.size());
    std::size_t end = value.find_first_of(" \t;");
    if (end != std::string::npos) value = value.substr(0, end);
    if (value.empty()) {
        throw std::runtime_error("endpoint 欄位為空");
    }
    return SimpleString(value.c_str());
}

void hotReloadDemo() {
    SimpleString liveEndpoint("db.prod.internal:5432");
    std::cout << "  目前線上設定：\"" << liveEndpoint.c_str() << "\"\n";

    // 第一次 reload：設定檔合法 → 整批換新
    liveEndpoint = parseEndpoint("timeout=30; endpoint=db.replica:5433; pool=8");
    std::cout << "  reload 成功後：\"" << liveEndpoint.c_str() << "\"\n";

    // 第二次 reload：設定檔壞掉 → 例外在「參數初始化」階段就拋出，
    //                operator= 的本體從未執行，liveEndpoint 一個位元組都沒動
    try {
        liveEndpoint = parseEndpoint("timeout=30; pool=8");   // 沒有 endpoint 欄位
    } catch (const std::runtime_error& e) {
        std::cout << "  reload 失敗（" << e.what() << "）\n";
    }
    std::cout << "  失敗後線上設定：\"" << liveEndpoint.c_str() << "\"（維持不變）\n";
}

int main() {
    std::cout << "===== 測試 1：基本拷貝賦值 =====\n";
    SimpleString a("Dragon");
    SimpleString b("Knight");
    std::cout << "  執行 b = a:\n";
    b = a;
    std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n\n";

    std::cout << "===== 測試 2：自我賦值 =====\n";
    std::cout << "  執行 a = a:\n";
    a = a;
    std::cout << "  a=\"" << a.c_str() << "\"（安全！）\n\n";

    std::cout << "===== 測試 3：鏈式賦值 =====\n";
    SimpleString c("Wizard");
    SimpleString d("Archer");
    std::cout << "  執行 d = c = a:\n";
    d = c = a;
    std::cout << "  a=\"" << a.c_str() << "\"  c=\"" << c.c_str()
              << "\"  d=\"" << d.c_str() << "\"\n\n";

    std::cout << "===== 測試 4：用暫時物件賦值 =====\n";
    SimpleString e("Rogue");
    std::cout << "  執行 e = SimpleString(\"Phoenix\"):\n";
    e = SimpleString("Phoenix");
    std::cout << "  e=\"" << e.c_str() << "\"\n\n";

    std::cout << "===== 測試 5：LeetCode 707. Design Linked List =====\n";
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);      // 1 -> 2 -> 3
        std::cout << "  原始串列        : " << list.dump() << "\n";
        std::cout << "  get(1)          : " << list.get(1) << "\n";

        MyLinkedList backup = list; // 拷貝建構：深拷貝一份快照
        list.deleteAtIndex(1);      // 只動原串列
        std::cout << "  刪除 index 1 後 : " << list.dump() << "\n";
        std::cout << "  快照 backup     : " << backup.dump() << "（未受影響）\n";

        MyLinkedList restored;
        restored.addAtHead(99);
        restored = backup;          // ★ 拷貝賦值（copy-and-swap）
        std::cout << "  由快照還原      : " << restored.dump() << "\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：日常實務 — 設定熱重載 =====\n";
    hotReloadDemo();
    std::cout << "\n";

    std::cout << "===== 開始解構 =====\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 29 課：拷貝賦值運算子（Copy Assignment Operator）1.cpp" -o lesson29_1

// === 預期輸出 ===
// ===== 測試 1：基本拷貝賦值 =====
//   [建構] "Dragon"
//   [建構] "Knight"
//   執行 b = a:
//   [拷貝建構] "Dragon"
//   [拷貝賦值] swap with "Dragon"
//   [解構] "Knight"
//   a="Dragon"  b="Dragon"
//
// ===== 測試 2：自我賦值 =====
//   執行 a = a:
//   [拷貝建構] "Dragon"
//   [拷貝賦值] swap with "Dragon"
//   [解構] "Dragon"
//   a="Dragon"（安全！）
//
// ===== 測試 3：鏈式賦值 =====
//   [建構] "Wizard"
//   [建構] "Archer"
//   執行 d = c = a:
//   [拷貝建構] "Dragon"
//   [拷貝賦值] swap with "Dragon"
//   [拷貝建構] "Dragon"
//   [拷貝賦值] swap with "Dragon"
//   [解構] "Archer"
//   [解構] "Wizard"
//   a="Dragon"  c="Dragon"  d="Dragon"
//
// ===== 測試 4：用暫時物件賦值 =====
//   [建構] "Rogue"
//   執行 e = SimpleString("Phoenix"):
//   [建構] "Phoenix"
//   [拷貝賦值] swap with "Phoenix"
//   [解構] "Rogue"
//   e="Phoenix"
//
// ===== 測試 5：LeetCode 707. Design Linked List =====
//   原始串列        : [1, 2, 3]
//   get(1)          : 2
//   刪除 index 1 後 : [1, 3]
//   快照 backup     : [1, 2, 3]（未受影響）
//   由快照還原      : [1, 2, 3]
//
// ===== 測試 6：日常實務 — 設定熱重載 =====
//   [建構] "db.prod.internal:5432"
//   目前線上設定："db.prod.internal:5432"
//   [建構] "db.replica:5433"
//   [拷貝賦值] swap with "db.replica:5433"
//   [解構] "db.prod.internal:5432"
//   reload 成功後："db.replica:5433"
//   reload 失敗（設定檔缺少 endpoint 欄位）
//   失敗後線上設定："db.replica:5433"（維持不變）
//   [解構] "db.replica:5433"
//
// ===== 開始解構 =====
//   [解構] "Phoenix"
//   [解構] "Dragon"
//   [解構] "Dragon"
//   [解構] "Dragon"
//   [解構] "Dragon"
