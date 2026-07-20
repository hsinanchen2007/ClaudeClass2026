// =============================================================================
//  第 29 課 總結  —  拷貝賦值運算子（Copy Assignment Operator）
// =============================================================================
//
// 【主題資訊 Information】
//   簽名（傳統）  ： ClassName& operator=(const ClassName& other);
//   簽名（推薦）  ： ClassName& operator=(ClassName other);   // Copy-and-Swap，傳值
//   標準版本      ： C++98（Copy-and-Swap 在 C++11 有移動語義後更划算）
//   標頭檔        ： <cstring>、<utility>（std::swap）、<algorithm>（std::copy）、
//                    <string>、<new>（placement new）
//   複雜度        ： 深拷貝 O(n)；swap O(1)
//   對照          ： 拷貝建構函數（第 28 課）、移動賦值（第 33 課）
//
// 【詳細解釋 Explanation】
//
// 【1. 拷貝建構 vs 拷貝賦值：看左邊物件存不存在】
//       MyClass b = a;   → 拷貝「建構」（b 尚未存在，這是初始化）
//       b = a;           → 拷貝「賦值」（b 已存在，這是賦值）
//   拷貝賦值比拷貝建構多了「先處理掉自己原有的資源」這件事，
//   所以它需要自我賦值防護，而拷貝建構不需要（物件還是空的）。
//
// 【2. 傳統寫法的六個步驟】
//       ① 自我賦值檢查  if (this == &other) return *this;
//       ② 釋放舊資源    delete[] m_data;
//       ③ 複製大小
//       ④ 配置新記憶體
//       ⑤ 複製資料
//       ⑥ 回傳 *this    支援鏈式賦值 a = b = c
//   本檔的 TraditionalString 就是這個寫法。它的弱點是不具備強例外保證：
//   若 ④ 的 new 拋 bad_alloc，舊資源已在 ② 被刪，物件處於壞狀態。
//   （更安全的傳統寫法是「先配置成功再釋放舊的」，見第 28 課 -3。）
//
// 【3. Copy-and-Swap 慣用法（推薦）】
//       ClassName& operator=(ClassName other) {  // ← 傳值，不是 const&
//           swap(other);
//           return *this;
//       }
//   三個好處，本檔的 SwapString / DynamicBuffer 都採用它：
//       * 強例外保證：唯一可能拋例外的「建構參數」發生在進入函式之前，
//         進到本體後只剩 noexcept 的 swap。若建構失敗，目標完全沒被碰過。
//       * 自動處理自我賦值：參數是獨立副本，a = a 也安全，不必手動防護。
//       * 程式碼簡潔：不必手動 delete/new。舊資源由參數離場時解構釋放。
//   代價：拷貝路徑一定會重新配置記憶體，無法重用既有緩衝區。
//   （完整討論見第 33 課 -2；那裡也講了它為什麼同時處理拷貝與移動。）
//
// 【4. 隱式刪除：哪些成員會讓 operator= 消失】
//   編譯器「不生成」拷貝賦值運算子的情況：
//       * const 成員     → 賦值後不能改，無法重新賦值 → 隱式刪除
//       * 引用成員       → 引用一旦綁定就不能改綁 → 隱式刪除
//   本檔的 Config 同時有這兩種成員。
//   若不自訂 operator=，Config c1; c1 = c2; 會編譯錯誤（呼叫到 deleted 函式）。
//   ★ 首選解法是「改設計」：把 const 成員改成非 const + getter，
//     把引用成員改成指標。placement new 是最後手段（見下方【概念補充 B】）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 成員宣告順序必須與初始化列表一致（-Wreorder）
//   本檔原始版本的 DynamicBuffer 宣告是 unsigned char* m_buffer; 在前、
//   std::size_t m_capacity; 在後，但初始化列表寫 : m_capacity(...), m_buffer(...)
//   （capacity 在前），兩者不一致 → 編譯時產生 -Wreorder 警告（本機實測會報）。
//   本檔已把 DynamicBuffer 的宣告順序改為 m_capacity 在前、m_buffer 在後
//   （被依賴的成員在前 —— m_buffer 的配置長度用到 m_capacity）。
//   TraditionalString / SwapString 的 m_len / m_data 原本就是正確順序。
//
// (B) ★ placement new 解法的原理與風險（Config）
//   既然「賦值」對 const／引用成員行不通，就改用「銷毀 + 重建」：
//       this->~Config();            // 手動呼叫解構函數，銷毀當前物件
//       new (this) Config(other);   // 在「同一塊記憶體」上重新建構
//   placement new 就是「在指定位址上建構物件」，不配置新記憶體。
//   兩個必須知道的風險：
//     1) 若重建（建構函數）拋例外，物件會停在「已銷毀但未重建」的狀態，
//        之後對它的任何操作（包括正常的解構）都是未定義行為。
//        本檔的 Config 成員不會拋，所以安全，但這是它成立的前提。
//     2) 自我賦值必須擋掉（if this != &other），否則會先銷毀自己、
//        再從「已銷毀的自己」複製。
//   因此結論仍是：placement new 是進階最後手段，優先改設計。
//
// (C) ★ 為什麼 Config 要顯式寫 Config(const Config&) = default;
//   原始版本沒寫拷貝建構函數，卻自訂了 operator=。
//   自 C++11 起，「有使用者提供的拷貝賦值、卻依賴隱式生成的拷貝建構」
//   這個組合已被標記為 deprecated，GCC 會發出 -Wdeprecated-copy 警告
//   （本機實測會報）。而本檔的 placement new 正好用到那個隱式拷貝建構。
//   本檔顯式加上 Config(const Config&) = default; 表明「就是要用預設的逐成員拷貝」，
//   語義完全不變，只是消除警告、也讓意圖明確。
//   這也呼應五法則的精神：特殊成員函式要嘛都不寫，要嘛把相關的一起交代清楚。
//
// (D) Copy-and-Swap 在輸出中的可觀察軌跡
//   SwapString b = a; 這一行 b = a 的輸出是：
//       [拷貝建構]（建構參數 other，深拷貝 a）
//       [Copy-and-Swap 賦值]（swap）
//       [解構]（參數 other 帶著 b 原本的舊資料離場）
//   三行連在一起，正好把「傳值 + swap + 參數解構釋放舊資源」整個機制攤開。
//
// 【注意事項 Pay Attention】
//   1. 拷貝賦值要有自我賦值防護、要釋放舊資源、要回傳 *this。
//   2. 傳統「先 delete 再 new」不具強例外保證；Copy-and-Swap 具備。
//   3. Copy-and-Swap 的參數必須「傳值」，寫成 const& 就退化成只有拷貝路徑。
//   4. const 成員與引用成員會讓 operator= 被隱式刪除；首選改設計，
//      placement new 是有風險的最後手段。
//   5. 成員宣告順序決定初始化順序，務必與初始化列表一致（用 -Wreorder 檢查）。
//   6. 自訂了 operator= 卻依賴隱式拷貝建構會觸發 -Wdeprecated-copy，
//      應顯式 = default 表明意圖（見【概念補充 C】）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】拷貝賦值運算子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Copy-and-Swap 慣用法有哪些好處？參數為什麼必須傳值？
//     答：三個好處：強例外保證（唯一可能拋的「建構參數」在進函式前發生，
//         本體只剩 noexcept 的 swap）、自動處理自我賦值（參數是獨立副本）、
//         程式碼簡潔（舊資源由參數離場時解構釋放，不必手動 delete/new）。
//         參數必須傳值，因為要靠「建構參數」這一步來做拷貝／移動；
//         寫成 const& 就沒有那份副本可 swap，整個技巧失效。
//     追問：它有什麼代價？
//         → 拷貝路徑一定會重新配置記憶體，無法像傳統寫法那樣重用既有緩衝區。
//           對反覆賦值相近長度資料的熱點，傳統寫法可能較省。
//
// 🔥 Q2. 什麼樣的成員會讓拷貝賦值運算子被「隱式刪除」？怎麼解決？
//     答：const 成員（賦值後不能改）與引用成員（綁定後不能改綁）。
//         兩者都讓「逐成員賦值」不可能，於是編譯器把 operator= 定義為 deleted。
//         首選解法是改設計：const 改非 const + getter，引用改指標。
//         最後手段是 placement new（先呼叫解構函數、再在原地重建）。
//     追問：placement new 有什麼風險？
//         → 若重建的建構函數拋例外，物件停在「已銷毀未重建」狀態，
//           之後任何操作（包括解構）都是未定義行為；
//           且自我賦值必須擋掉，否則從已銷毀的自己複製。所以它是最後手段。
//
// ⚠️ 陷阱. 「Copy-and-Swap 的參數已經是一份拷貝，所以 a = a 自我賦值時，
//           完全不需要任何防護就一定安全。」
//     答：對 Copy-and-Swap 本身而言這句話是對的 ——
//         參數 other 是從 a 拷貝出來的獨立副本，與 this 不同記憶體，
//         swap 後再讓副本解構，完全安全，確實不需要 if (this == &other)。
//         但這個「不需防護」是 Copy-and-Swap 專屬的性質，不能推廣：
//         傳統寫法（先 delete 再 new）與移動賦值（先 delete 再偷）
//         都「必須」手動防護，否則會從已釋放的自己讀取。
//     為什麼會錯：把某一種實作的性質當成所有 operator= 的通則。
//         「要不要自我賦值防護」取決於實作順序 ——
//         只有「先建立完整副本、最後才碰自己」的寫法才天生免疫。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   拷貝賦值運算子是資源管理型類別的實作細節，
//   價值在例外安全與正確性（自我賦值、隱式刪除）。
//   LeetCode 判的是演算法的輸入輸出與時間限制，
//   評測系統只會建立一個實例並呼叫指定方法，
//   從不對你的物件做賦值，也不會在賦值中途注入例外來檢驗保證強度。
//   硬掛設計類題號只會誤導，故從缺；
//   本檔以「傳統 vs Copy-and-Swap vs 隱式刪除」三種寫法完整覆蓋主題。
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <utility>   // std::swap
#include <algorithm> // std::copy
#include <string>
#include <new>       // placement new

// ============================================================
// 方法一：傳統拷貝賦值寫法
// ============================================================
class TraditionalString {
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在前。
    std::size_t m_len;
    char* m_data;
public:
    TraditionalString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    TraditionalString(const TraditionalString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // ★ 傳統拷貝賦值運算子
    TraditionalString& operator=(const TraditionalString& other) {
        std::cout << "  [傳統拷貝賦值] ";
        if (this == &other) {                       // ① 自我賦值檢查
            std::cout << "自我賦值，跳過\n";
            return *this;
        }
        delete[] m_data;                            // ② 釋放舊資源
        m_len = other.m_len;                        // ③ 複製大小
        m_data = new char[m_len + 1];               // ④ 配置新記憶體
        std::strcpy(m_data, other.m_data);          // ⑤ 複製資料
        std::cout << "\"" << m_data << "\"\n";
        return *this;                               // ⑥ 回傳 *this
    }

    ~TraditionalString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// ============================================================
// 方法二：Copy-and-Swap 慣用法（推薦）
// ============================================================
class SwapString {
    // ⚠️ 同上：m_len 必須宣告在 m_data 之前。
    std::size_t m_len;
    char* m_data;
public:
    SwapString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    SwapString(const SwapString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // swap 函數
    void swap(SwapString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ★ Copy-and-Swap：參數按值傳入（自動觸發拷貝建構）
    SwapString& operator=(SwapString other) {  // ← 注意：傳值，不是 const&
        std::cout << "  [Copy-and-Swap 賦值]\n";
        swap(other);        // 交換 this 和 other 的內容
        return *this;       // other 帶著舊資料離開作用域，自動解構
    }
    // 優點：
    //   1. 異常安全：如果 new 在拷貝建構階段失敗，原物件不受影響
    //   2. 自動處理自我賦值：a = a 時，other 是 a 的拷貝，交換後仍正確
    //   3. 程式碼簡潔：不需要手動 delete/new

    ~SwapString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr") << "\"\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data; }
};

// ============================================================
// 隱式刪除示範：含 const 成員和引用成員
// ============================================================
class Config {
    const int m_id;           // const 成員 → operator= 被隱式刪除
    std::string& m_nameRef;   // 引用成員 → operator= 被隱式刪除
public:
    Config(int id, std::string& name) : m_id(id), m_nameRef(name) {}

    // 顯式採用預設的逐成員拷貝建構：表明意圖，並消除 -Wdeprecated-copy 警告
    // （自訂了 operator= 卻依賴隱式拷貝建構，自 C++11 起已 deprecated；
    //   而下方 placement new 正好會用到這個拷貝建構，見檔頭【概念補充 C】）。
    Config(const Config& other) = default;

    // ★ 解法：使用 placement new（進階技巧，最後手段）
    // 原理：既然「賦值」對 const/引用成員行不通，改用「銷毀 + 重建」
    Config& operator=(const Config& other) {
        if (this != &other) {             // 自我賦值必須擋掉（見檔頭【概念補充 B】）
            this->~Config();              // 手動銷毀當前物件
            new (this) Config(other);     // 在同一塊記憶體上重新建構
        }
        return *this;
    }
    // ⚠️ placement new 風險：若建構拋例外，物件處於已銷毀但未重建狀態
    //    （Config 的成員不會拋，故此處安全，但這是前提）。
    // 一般建議：優先考慮將 const 改為 non-const + getter，引用改為指標。

    void print() const {
        std::cout << "  Config{ id=" << m_id << ", name=\"" << m_nameRef << "\" }\n";
    }
};

// ============================================================
// DynamicBuffer：完整的 Copy-and-Swap 拷貝賦值示範
// ============================================================
class DynamicBuffer {
    // ⚠️ 宣告順序改為 m_capacity 在前、m_buffer 在後，與初始化列表一致，
    //    避免 -Wreorder（原始版本相反，本機實測會報，見檔頭【概念補充 A】）。
    std::size_t m_capacity;
    unsigned char* m_buffer;
public:
    explicit DynamicBuffer(std::size_t cap)
        : m_capacity(cap), m_buffer(new unsigned char[cap]{}) {}

    DynamicBuffer(const DynamicBuffer& other)
        : m_capacity(other.m_capacity), m_buffer(new unsigned char[other.m_capacity]) {
        std::copy(other.m_buffer, other.m_buffer + m_capacity, m_buffer);
    }

    void swap(DynamicBuffer& other) noexcept {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_capacity, other.m_capacity);
    }

    DynamicBuffer& operator=(DynamicBuffer other) {  // Copy-and-Swap
        swap(other);
        return *this;
    }

    ~DynamicBuffer() { delete[] m_buffer; }

    unsigned char& operator[](std::size_t i) { return m_buffer[i]; }
    std::size_t capacity() const { return m_capacity; }
};

int main() {
    // ============================================================
    // 示範 1：傳統拷貝賦值
    // ============================================================
    std::cout << "=== 傳統拷貝賦值 ===\n";
    {
        TraditionalString a("Dragon");
        TraditionalString b("Knight");
        std::cout << "  b = a:\n";
        b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        std::cout << "  自我賦值：\n";
        a = a;  // 安全
    }
    std::cout << "\n";

    // ============================================================
    // 示範 2：Copy-and-Swap 慣用法
    // ============================================================
    std::cout << "=== Copy-and-Swap 賦值 ===\n";
    {
        SwapString a("Wizard");
        SwapString b("Archer");
        std::cout << "  b = a:\n";
        b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        // 用暫時物件賦值
        std::cout << "  b = SwapString(\"Phoenix\"):\n";
        b = SwapString("Phoenix");
        std::cout << "  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 示範 3：鏈式賦值
    // ============================================================
    std::cout << "=== 鏈式賦值 ===\n";
    {
        SwapString a("A"), b("B"), c("C");
        std::cout << "  c = b = a:\n";
        c = b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 示範 4：隱式刪除 + placement new 解法
    // ============================================================
    std::cout << "=== 隱式刪除（const/引用成員）===\n";
    {
        std::string name1 = "Alice", name2 = "Bob";
        Config c1(1, name1);
        Config c2(2, name2);
        std::cout << "  賦值前:\n";
        c1.print(); c2.print();

        c1 = c2;  // 使用 placement new 實現
        std::cout << "  賦值後 (c1 = c2):\n";
        c1.print(); c2.print();
    }
    std::cout << "\n";

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 重點整理 ===\n";
    std::cout << "  1. 拷貝建構：物件不存在時初始化（MyClass b = a）\n";
    std::cout << "  2. 拷貝賦值：物件已存在時賦值（b = a）\n";
    std::cout << "  3. 傳統寫法：自我檢查 → delete → new → copy → return *this\n";
    std::cout << "  4. Copy-and-Swap（推薦）：異常安全、自動處理自我賦值\n";
    std::cout << "  5. const/引用成員 → operator= 被隱式刪除\n";
    std::cout << "     解法：去 const + 改指標，或 placement new\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder summary.cpp -o summary

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：沒有位址、沒有耗時、沒有執行緒
//   （本機實測連跑 5 次逐位元組相同）。用建構／賦值／解構的呼叫順序當證據。
// * 示範 2 的 b = a 是本檔重點：連續三行
//       [拷貝建構]（建構參數 other）→ [Copy-and-Swap 賦值]（swap）→
//       [解構]（參數帶著 b 的舊資料 "Archer" 離場）
//   把 Copy-and-Swap 的完整機制攤開。
// * 示範 2 的 b = SwapString("Phoenix")：暫時物件是純右值，
//   C++17 保證省略，直接就地建構在參數的位置上，所以只看到一次 [建構]。
// * 示範 3 的 c = b = a：兩次都是 Copy-and-Swap，
//   各自 [拷貝建構] + [Copy-and-Swap 賦值]；中間的 [解構] 是兩次 swap
//   把 b、c 的舊資料 "B"、"C" 換出來後、參數離場時釋放的。
// * 示範 4 的 c1 = c2 之後兩者都是 id=2 name="Bob"：
//   placement new 用 c2 的成員在 c1 原地重建，連引用成員也重新綁到 name2。
//   注意這裡刻意不印任何位址（placement new 在原地重建，位址本就相同，
//   印出來既不穩定也沒有意義）。
// * 本檔已修正原始版本的兩個編譯警告：DynamicBuffer 的 -Wreorder
//   （成員宣告順序）與 Config 的 -Wdeprecated-copy（顯式 = default 拷貝建構）。
//   修正前後執行結果完全相同（見檔頭【概念補充 A】【概念補充 C】）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// === 傳統拷貝賦值 ===
//   [建構] "Dragon"
//   [建構] "Knight"
//   b = a:
//   [傳統拷貝賦值] "Dragon"
//   a="Dragon" b="Dragon"
//   自我賦值：
//   [傳統拷貝賦值] 自我賦值，跳過
//   [解構] "Dragon"
//   [解構] "Dragon"
//
// === Copy-and-Swap 賦值 ===
//   [建構] "Wizard"
//   [建構] "Archer"
//   b = a:
//   [拷貝建構] "Wizard"
//   [Copy-and-Swap 賦值]
//   [解構] "Archer"
//   a="Wizard" b="Wizard"
//   b = SwapString("Phoenix"):
//   [建構] "Phoenix"
//   [Copy-and-Swap 賦值]
//   [解構] "Wizard"
//   b="Phoenix"
//   [解構] "Phoenix"
//   [解構] "Wizard"
//
// === 鏈式賦值 ===
//   [建構] "A"
//   [建構] "B"
//   [建構] "C"
//   c = b = a:
//   [拷貝建構] "A"
//   [Copy-and-Swap 賦值]
//   [拷貝建構] "A"
//   [Copy-and-Swap 賦值]
//   [解構] "C"
//   [解構] "B"
//   a="A" b="A" c="A"
//   [解構] "A"
//   [解構] "A"
//   [解構] "A"
//
// === 隱式刪除（const/引用成員）===
//   賦值前:
//   Config{ id=1, name="Alice" }
//   Config{ id=2, name="Bob" }
//   賦值後 (c1 = c2):
//   Config{ id=2, name="Bob" }
//   Config{ id=2, name="Bob" }
//
// === 重點整理 ===
//   1. 拷貝建構：物件不存在時初始化（MyClass b = a）
//   2. 拷貝賦值：物件已存在時賦值（b = a）
//   3. 傳統寫法：自我檢查 → delete → new → copy → return *this
//   4. Copy-and-Swap（推薦）：異常安全、自動處理自我賦值
//   5. const/引用成員 → operator= 被隱式刪除
//      解法：去 const + 改指標，或 placement new
