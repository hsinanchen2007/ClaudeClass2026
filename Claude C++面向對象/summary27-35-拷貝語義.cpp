// ============================================================
// 第 27～30 課總結：拷貝語義（Copy Semantics）
// 涵蓋：淺拷貝 vs 深拷貝、拷貝建構函數、拷貝賦值運算子、三法則
// 編譯：g++ -std=c++17 -o copy_semantics summary27-35-拷貝語義.cpp
// ============================================================

#include <iostream>
#include <cstring>    // strlen, strcpy
#include <algorithm>  // std::copy, std::swap
#include <utility>    // std::swap

using namespace std;

// =============================================================================
//
// 【主題資訊 Information】
//   範圍：  淺拷貝 vs 深拷貝（27）→ 拷貝建構函數（28）
//           → 拷貝賦值運算子（29）→ 三法則（30）
//   標準：  拷貝語義自 C++98 即有。本檔以 C++17 編譯，涉及版本的關鍵點：
//             * = default / = delete —— C++11（C++98 時代要用「宣告為 private 且不定義」）
//             * 三法則在 C++11 後擴充為五法則（加上移動建構與移動賦值）
//   標頭檔：<iostream> <cstring> <algorithm> <utility>
//   關鍵詞：shallow copy、deep copy、copy constructor、copy assignment、
//           Rule of Three、copy-and-swap、self-assignment、strong exception guarantee
//
// 【詳細解釋 Explanation】
//
// 【1. 問題的根源：編譯器產生的拷貝是「逐成員拷貝」】
// 若你不寫拷貝建構函數，編譯器會生一個，它做的是 memberwise copy
// —— 對每個成員各自呼叫其拷貝建構。對 int、double 這種型別完全正確；
// 對 char* 就出事了：**指標被複製的是「位址值」，不是它指向的內容**。
//     ShallowString a("hello");
//     ShallowString b = a;        // b.m_data == a.m_data（同一塊記憶體）
// 於是產生三個連鎖問題：
//   (a) double free：a 與 b 解構時各自 delete[] 同一個指標
//   (b) dangling pointer：a 先解構後，b.m_data 指向已釋放的記憶體
//   (c) 意外的別名：改 a 的內容，b 也跟著變（而使用者以為它們是獨立的）
// 注意這三者**全都是 undefined behavior**，標準不保證任何特定結果 ——
// 可能立刻崩潰、可能靜默毀壞堆積、也可能在測試時「看起來正常」而在上線後爆炸。
// 這正是本檔 Lesson27 只用文字說明淺拷貝後果、不實際執行的原因。
//
// 【2. 深拷貝：配置自己的記憶體，複製內容】
//     DeepString(const DeepString& o) : m_len(o.m_len), m_data(new char[o.m_len+1]) {
//         strcpy(m_data, o.m_data);
//     }
// 關鍵是「新配置 + 複製內容」，於是兩個物件各自擁有獨立資源，
// 解構互不干擾，修改也互不影響。這才符合使用者對「拷貝」的直覺。
//
// 【3. 拷貝建構 vs 拷貝賦值：初始化與覆蓋是兩件事】
// 這是第 28、29 課的分界，也是初學者最常混淆的地方：
//     DeepString b = a;    // 拷貝【建構】：b 尚不存在，從無到有
//     DeepString c("x");
//     c = a;               // 拷貝【賦值】：c 已存在，要先處理掉舊資源
// 差別在於「目標物件原本有沒有東西」：
//   * 拷貝建構：目標是空白的，直接配置並複製即可。
//   * 拷貝賦值：目標已持有資源，必須先釋放舊的，否則就是記憶體洩漏。
// 所以拷貝賦值天生比拷貝建構複雜，要處理三件事：
//     ① 自我賦值檢查   ② 釋放舊資源   ③ 配置並複製新資源，並回傳 *this
//
// 【4. 自我賦值：為什麼 a = a 會炸掉】
// 天真的拷貝賦值長這樣：
//     Buffer& operator=(const Buffer& o) {
//         delete[] m_data;                       // ① 先釋放自己的
//         m_data = new char[o.m_len + 1];        // ② 再從 o 複製
//         strcpy(m_data, o.m_data);              // ③ ← o 就是自己，已經被釋放了！
//         return *this;
//     }
// 當 &o == this 時，第 ① 步已經把 o.m_data 也釋放掉了（因為是同一塊），
// 第 ③ 步就是在讀取已釋放的記憶體 → UB。
// 兩種正解：
//   (a) 自我賦值檢查：`if (this == &other) return *this;`  —— 直觀，成本低
//   (b) copy-and-swap：先複製一份，再與自己交換 —— 更穩健，見下一點
//
// 【5. copy-and-swap：一招同時解決自我賦值與例外安全】
//     Buffer& operator=(Buffer other) {   // ← 注意：按值傳遞，複製在此發生
//         swap(*this, other);             // 交換內容
//         return *this;                   // other 帶著舊資源離開作用域並解構
//     }
// 它的三個優點：
//   * 自我賦值自動安全：複製出的是獨立副本，交換也不會出問題。
//   * **強例外保證**：所有可能失敗的操作（配置記憶體）都發生在
//     參數複製階段，此時 *this 還沒被動過。一旦進入函式本體，
//     只剩 swap（通常 noexcept），不會失敗 —— 要嘛完全成功，要嘛完全沒變。
//   * 程式碼量少，且自動同時支援拷貝賦值與移動賦值
//     （傳入右值時，參數 other 是用移動建構出來的）。
// 代價是即使自我賦值也會實際複製一次，但那是罕見情況，不值得為它優化。
//
// 【6. 三法則（Rule of Three）】
// 若你需要自訂以下三者之一，通常三者都需要：
//     ① 解構函式   ② 拷貝建構函數   ③ 拷貝賦值運算子
// 理由很直接：會需要自訂解構函式，代表這個類別**自己管理某種資源**；
// 既然自己管理資源，那麼「複製」與「賦值」時該怎麼處理那份資源，
// 編譯器產生的逐成員版本一定是錯的。
// C++11 之後擴充為**五法則**（再加移動建構、移動賦值），
// 而更現代的建議是**零法則**：用 std::string、std::vector、std::unique_ptr
// 當成員，五個特殊成員函式一個都不用寫。
//
// 【概念補充 Concept Deep Dive】
// (A) 拷貝建構函數的參數為什麼必須是 reference
//     `DeepString(DeepString other)` 按值傳參 → 傳參本身就需要拷貝
//     → 又要呼叫拷貝建構 → 無限遞迴。編譯器會直接報錯。
//     所以必須是 `const DeepString&`。加 const 則是為了能接受
//     const 物件與臨時物件作為來源。
//
// (B) 哪些時機會呼叫拷貝建構函數
//     ① 用同型別物件初始化新物件（T b = a; 或 T b(a);）
//     ② 按值傳參   ③ 按值回傳（但常被 copy elision 消掉）
//     ④ 放進容器（push_back 一個左值）
//     注意 `T b = a;` 是**拷貝建構**不是賦值 —— 有等號不代表是 operator=，
//     判準是「b 是不是剛誕生」。
//
// (C) = delete 禁止拷貝（C++11）
//     不可拷貝的型別（如互斥鎖、檔案句柄、本檔的租借憑證）應明確寫：
//         T(const T&) = delete;
//         T& operator=(const T&) = delete;
//     C++11 之前的做法是「宣告為 private 且不提供定義」，
//     錯誤訊息很難懂且要到連結期才報；= delete 在編譯期就給出清楚訊息。
//
// (D) 成員初始化順序永遠依「宣告順序」，不是初始化列表的書寫順序
//     本檔的 ManagedString 刻意把 m_len 宣告在 m_data 之前：
//         size_t m_len;      // 先宣告
//         char*  m_data;     // 後宣告
//     於是初始化列表 `: m_len(other.m_len), m_data(new char[other.m_len + 1])`
//     的書寫順序與實際執行順序一致，讀起來不會誤導。
//     ⚠️ 反例：若宣告順序相反（m_data 在前），即使初始化列表寫成
//        `m_len(...), m_data(new char[m_len + 1])`，m_data 仍會先初始化，
//        此時 m_len 尚未賦值 → 用未初始化的值配置記憶體，是 UB。
//     編譯器的 -Wreorder 專門警告「列表順序與宣告順序不一致」；
//     看到它就該把兩者對齊，而不是忽略。
//
// (E) 為什麼 operator= 要回傳 T& 而不是 void 或 T
//     回傳 T& 是為了支援連續賦值 `a = b = c;`（右結合，先算 b = c）。
//     回傳 void 會讓連續賦值編譯失敗；回傳 T（值）則每次都多複製一份，
//     且會讓 `(a = b) = c;` 這種寫法作用在臨時物件上，語意錯誤。
//     慣例是 `T& operator=(...)` 並 `return *this;`。
//
// 【注意事項 Pay Attention】
// 1. 編譯器產生的拷貝是逐成員拷貝；對指標成員只複製位址，不複製內容。
// 2. double free、dangling pointer 都是 UB —— 不可描述成「一定會崩潰」。
// 3. 拷貝賦值必須處理自我賦值，否則 a = a 會讀到已釋放的記憶體。
// 4. 拷貝賦值要先釋放舊資源，否則就是記憶體洩漏（拷貝建構則不需要）。
// 5. 拷貝建構函數的參數必須是 const T&；按值傳會造成無限遞迴。
// 6. 自訂解構、拷貝建構、拷貝賦值三者之一，通常三者都要（三法則）。
// 7. 最好的做法是零法則：改用 std::string / std::vector，一個都不用寫。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】拷貝語義與三法則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是三法則？為什麼「需要自訂解構函式」就代表另外兩個也要自訂？
//     答：三法則指解構函式、拷貝建構函數、拷貝賦值運算子，
//         需要其一通常就需要全部三個。
//         推理是：會需要自訂解構函式，幾乎必然是因為這個類別**自己管理資源**
//         （裸指標、檔案句柄、鎖）。既然如此，編譯器產生的逐成員拷貝
//         一定會做出「兩個物件共用同一份資源」的錯誤結果 → double free。
//     追問：C++11 之後呢？
//         → 擴充為五法則（加移動建構、移動賦值）。
//           更好的是零法則：用 std::vector/std::string/unique_ptr 當成員，
//           五個全部交給編譯器產生，自己一個都不寫。
//
// 🔥 Q2. 拷貝建構函數和拷貝賦值運算子的差別在哪？
//     答：拷貝建構是「從無到有」——目標物件尚未存在，直接配置並複製。
//         拷貝賦值是「覆蓋既有物件」——目標已持有資源，
//         必須先處理自我賦值、再釋放舊資源、才配置新的，最後回傳 *this。
//         判準不是有沒有等號，而是目標物件是不是剛誕生：
//         `T b = a;` 是拷貝建構；`T c("x"); c = a;` 才是拷貝賦值。
//     追問：拷貝建構的參數為什麼一定要是 reference？
//         → 按值傳參本身就需要一次拷貝，那又要呼叫拷貝建構 → 無限遞迴，
//           編譯器會直接報錯。所以必須是 const T&。
//
// 🔥 Q3. 什麼是 copy-and-swap？它解決了什麼問題？
//     答：把 operator= 的參數改成**按值**傳遞，在函式內與 *this 交換，
//         讓舊資源隨著參數物件解構而釋放：
//             T& operator=(T other) { swap(*this, other); return *this; }
//         它一次解決兩件事：自我賦值自動安全；
//         而且提供**強例外保證** —— 所有可能失敗的配置都發生在參數複製階段，
//         此時 *this 尚未被修改，失敗就等於什麼都沒做。
//     追問：有沒有缺點？
//         → 自我賦值時仍會實際複製一次（用 if 檢查的版本可以直接跳過）。
//           但自我賦值極罕見，不值得為它犧牲例外安全與程式碼簡潔。
//
// ⚠️ 陷阱 1. 這個 operator= 在 a = a 時為什麼會出事？
//         Buffer& operator=(const Buffer& o) {
//             delete[] m_data;
//             m_data = new char[o.m_len + 1];
//             strcpy(m_data, o.m_data);      // ← 這裡
//             return *this;
//         }
//     答：當 &o == this 時，第一行的 delete[] 已經把 o.m_data 指向的記憶體
//         釋放掉了（因為 m_data 與 o.m_data 是同一個指標）。
//         第三行再去讀 o.m_data 就是讀取已釋放的記憶體 —— UB，
//         標準不保證任何結果。
//     為什麼會錯：寫 operator= 時腦中預設「來源和目標是兩個不同物件」，
//         這個假設在 99% 的呼叫下成立，所以測試時測不出來。
//         但 `v[i] = v[j]`（i 恰好等於 j）、或透過引用間接賦值時就會踩到。
//         正解是加自我賦值檢查，或直接用 copy-and-swap。
//
// ⚠️ 陷阱 2. 「我的類別只有 std::string 和 int 成員，但我加了一個解構函式印 log，
//              這樣需要遵守三法則嗎？」
//     答：就「正確性」而言不需要 —— string 自己會正確拷貝，
//         編譯器產生的拷貝建構與拷貝賦值都是對的。
//         但有一個**靜默的副作用**：一旦你自訂了解構函式，
//         編譯器就**不再自動產生移動建構與移動賦值**，
//         於是這個類別的所有「移動」都會退化成拷貝，而且沒有任何警告。
//     為什麼會錯：大家把三法則記成「有解構函式就要寫另外兩個」這條表面規則，
//         卻沒注意到 C++11 之後真正的規則是**五個特殊成員函式互相連動**：
//         宣告了任一個，其他幾個的自動產生條件就會改變。
//         所以現代準則是「要嘛五個都不寫（零法則），要嘛就寫全五個」，
//         而不是只補一個解構函式就收工。
//
// 【LeetCode 實戰範例】—— 從缺（刻意不加）
//     拷貝語義是「自訂資源管理類別時怎麼寫才正確」的主題，
//     它決定你寫的 class 對不對，而不是某一題的演算法。
//     LeetCode 判題只驗證回傳值，不會測試你的類別能不能安全拷貝，
//     也沒有題目在考三法則或自我賦值。
//     （設計類題目如 146 LRU Cache、155 Min Stack 都直接用 STL 容器，
//       正好落在「零法則」那一側 —— 根本不需要自己寫拷貝。）
//     本檔改以「文字編輯器的 undo 快照」這個真實情境示範深拷貝與
//     copy-and-swap 的例外安全。硬湊一題不相關的比沒有更糟。
// ═══════════════════════════════════════════════════════════════════════════

// ============================================================
// 第 27 課：淺拷貝 vs 深拷貝（Shallow Copy vs Deep Copy）
// ============================================================
// 【核心觀念】
// 淺拷貝（Shallow Copy）：只複製指標本身，兩個物件指向同一塊記憶體
//   → 問題：其中一個物件解構後，另一個變成 dangling pointer → double free
// 深拷貝（Deep Copy）：分配新的記憶體，把資料內容完整複製過去
//   → 兩個物件各自擁有獨立的記憶體，互不影響

namespace Lesson27 {

// --- 淺拷貝問題示範 ---
class ShallowString {
    char* m_data;
    size_t m_len;
public:
    ShallowString(const char* str) {
        m_len = strlen(str);
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] 分配記憶體 @" << (void*)m_data << endl;
    }

    ~ShallowString() {
        cout << "  [解構] 釋放記憶體 @" << (void*)m_data << endl;
        delete[] m_data;  // 淺拷貝時，兩個物件都會 delete 同一塊 → crash!
    }

    // 編譯器預設產生的拷貝建構函數 = 淺拷貝
    // ShallowString(const ShallowString& other)
    //     : m_data(other.m_data), m_len(other.m_len) {}
    // ↑ 只複製指標值，不複製指標指向的資料！

    void print() const { cout << "  內容: " << m_data << endl; }
};

// --- 深拷貝解決方案 ---
class DeepString {
    char* m_data;
    size_t m_len;
public:
    DeepString(const char* str) {
        m_len = strlen(str);
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] 分配記憶體 @" << (void*)m_data << endl;
    }

    // 深拷貝建構函數：分配新記憶體 + 複製資料
    DeepString(const DeepString& other) {
        m_len = other.m_len;
        m_data = new char[m_len + 1];     // ← 分配新的記憶體
        strcpy(m_data, other.m_data);     // ← 複製資料內容
        cout << "  [深拷貝建構] 新記憶體 @" << (void*)m_data
             << " (來源 @" << (void*)other.m_data << ")" << endl;
    }

    ~DeepString() {
        cout << "  [解構] 釋放記憶體 @" << (void*)m_data << endl;
        delete[] m_data;  // 各自釋放各自的記憶體，安全！
    }

    void print() const { cout << "  內容: " << m_data << endl; }
};

void demo() {
    cout << "=== 第 27 課：淺拷貝 vs 深拷貝 ===" << endl;

    // 淺拷貝問題（僅說明，不實際執行以避免 crash）
    cout << "\n--- 淺拷貝問題（概念說明）---" << endl;
    cout << "  ShallowString a(\"hello\");" << endl;
    cout << "  ShallowString b = a;  // 淺拷貝：b.m_data == a.m_data" << endl;
    cout << "  // a 解構 → delete m_data" << endl;
    cout << "  // b 解構 → delete 同一塊 m_data → 💥 double free!" << endl;

    // 深拷貝正確示範
    cout << "\n--- 深拷貝正確示範 ---" << endl;
    {
        DeepString a("Hello");
        DeepString b = a;  // 呼叫深拷貝建構函數
        a.print();
        b.print();
        cout << "  （a 和 b 有各自獨立的記憶體，解構安全）" << endl;
    } // a, b 各自 delete 各自的記憶體 → OK
}

} // namespace Lesson27

// ============================================================
// 第 28 課：拷貝建構函數（Copy Constructor）
// ============================================================
// 【核心觀念】
// 簽名：ClassName(const ClassName& other)
// 參數必須是 const 引用（避免無限遞迴 + 接受 const/rvalue）
//
// 【六大觸發場景】
//  1. 直接初始化：MyClass b(a);
//  2. 拷貝初始化：MyClass b = a;
//  3. 函數傳值參數：void func(MyClass obj) → 傳入時拷貝
//  4. 函數回傳值：return obj; → 回傳時拷貝（可能被 RVO 省略）
//  5. 陣列初始化：MyClass arr[] = {a, b};
//  6. 容器操作：vector.push_back(obj);
//
// 【Copy Elision / RVO / NRVO】
//  RVO (Return Value Optimization)：回傳臨時物件時省略拷貝
//    → return MyClass("hi");  // 直接在呼叫者的空間建構
//  NRVO (Named RVO)：回傳具名物件時省略拷貝
//    → MyClass obj("hi"); return obj;
//  C++17 起，RVO 是強制的（mandatory），NRVO 仍是可選優化

namespace Lesson28 {

class Tracked {
    string m_name;
public:
    Tracked(const string& name) : m_name(name) {
        cout << "  [建構] " << m_name << endl;
    }
    Tracked(const Tracked& other) : m_name(other.m_name + "_copy") {
        cout << "  [拷貝建構] " << m_name << " ← " << other.m_name << endl;
    }
    ~Tracked() { cout << "  [解構] " << m_name << endl; }
    const string& name() const { return m_name; }
};

// 場景3：函數傳值 → 觸發拷貝建構
void passByValue(Tracked obj) {
    cout << "  函數內使用: " << obj.name() << endl;
}

// 場景4：函數回傳 → 可能觸發拷貝（但通常被 RVO/NRVO 省略）
Tracked createTracked() {
    Tracked local("fromFunc");
    return local;  // NRVO 可能省略拷貝
}

// --- 含指標的深拷貝建構 ---
class IntArray {
    int* m_data;
    size_t m_size;
public:
    IntArray(size_t size, int initVal = 0)
        : m_data(new int[size]), m_size(size) {
        for (size_t i = 0; i < size; ++i) m_data[i] = initVal;
    }

    // 深拷貝建構函數
    IntArray(const IntArray& other)
        : m_data(new int[other.m_size]), m_size(other.m_size) {
        // 使用 std::copy 複製陣列內容
        copy(other.m_data, other.m_data + m_size, m_data);
        cout << "  [IntArray 深拷貝] size=" << m_size << endl;
    }

    ~IntArray() { delete[] m_data; }

    void set(size_t idx, int val) { m_data[idx] = val; }
    int get(size_t idx) const { return m_data[idx]; }
    size_t size() const { return m_size; }
};

void demo() {
    cout << "\n=== 第 28 課：拷貝建構函數 ===" << endl;

    // 場景 1 & 2：直接初始化 / 拷貝初始化
    cout << "\n--- 場景 1 & 2：直接初始化 / 拷貝初始化 ---" << endl;
    Tracked a("original");
    Tracked b(a);       // 場景1：直接初始化
    Tracked c = a;      // 場景2：拷貝初始化（效果同上）

    // 場景 3：傳值參數
    cout << "\n--- 場景 3：函數傳值 ---" << endl;
    passByValue(a);     // 傳入時觸發拷貝建構

    // 場景 4：函數回傳（通常被 NRVO 省略）
    cout << "\n--- 場景 4：函數回傳（NRVO 可能省略拷貝）---" << endl;
    Tracked d = createTracked();
    cout << "  得到: " << d.name() << endl;

    // 含指標的深拷貝
    cout << "\n--- IntArray 深拷貝示範 ---" << endl;
    IntArray arr1(3, 10);   // [10, 10, 10]
    IntArray arr2 = arr1;   // 深拷貝
    arr2.set(0, 99);        // 修改 arr2 不影響 arr1
    cout << "  arr1[0]=" << arr1.get(0) << " arr2[0]=" << arr2.get(0) << endl;
    // 輸出：arr1[0]=10 arr2[0]=99（各自獨立）
}

} // namespace Lesson28

// ============================================================
// 第 29 課：拷貝賦值運算子（Copy Assignment Operator）
// ============================================================
// 【核心觀念】
// 簽名：ClassName& operator=(const ClassName& other)
// 拷貝建構 vs 拷貝賦值的區別：
//   MyClass b = a;  → 拷貝建構（物件尚未存在，初始化時呼叫）
//   b = a;          → 拷貝賦值（物件已存在，賦值時呼叫）
//
// 【自我賦值檢查】
//   if (this == &other) return *this;
//   → 防止 a = a 時先 delete 自己的資料再複製（已被刪除的）資料
//
// 【Copy-and-Swap 慣用法】推薦的安全寫法
//   ClassName& operator=(ClassName other) {  // 注意：傳值（觸發拷貝）
//       swap(*this, other);
//       return *this;
//   }
//   → 異常安全（如果 new 失敗，原物件不受影響）
//   → 自動處理自我賦值
//
// 【隱式刪除的情況】
//   類別含 const 成員或引用成員 → 編譯器不會生成 copy assignment
//   → 若需要，可用 placement new 搭配手動解構來繞過（進階技巧）

namespace Lesson29 {

class ManagedBuffer {
    int* m_data;
    size_t m_size;
public:
    ManagedBuffer(size_t size, int initVal = 0)
        : m_data(new int[size]), m_size(size) {
        for (size_t i = 0; i < size; ++i) m_data[i] = initVal;
    }

    // 深拷貝建構函數
    ManagedBuffer(const ManagedBuffer& other)
        : m_data(new int[other.m_size]), m_size(other.m_size) {
        copy(other.m_data, other.m_data + m_size, m_data);
        cout << "  [ManagedBuffer 拷貝建構]" << endl;
    }

    // 拷貝賦值運算子 — 傳統寫法
    ManagedBuffer& operator=(const ManagedBuffer& other) {
        cout << "  [ManagedBuffer 拷貝賦值]" << endl;

        if (this == &other) return *this;  // ① 自我賦值檢查

        delete[] m_data;                   // ② 釋放舊資源

        m_size = other.m_size;             // ③ 複製大小
        m_data = new int[m_size];          // ④ 分配新記憶體
        copy(other.m_data, other.m_data + m_size, m_data);  // ⑤ 複製資料

        return *this;                      // ⑥ 回傳自身引用（支援鏈式賦值）
    }

    ~ManagedBuffer() { delete[] m_data; }

    int get(size_t i) const { return m_data[i]; }
    void set(size_t i, int v) { m_data[i] = v; }
    size_t size() const { return m_size; }
};

// --- Copy-and-Swap 慣用法（推薦）---
class SwapBuffer {
    int* m_data;
    size_t m_size;
public:
    SwapBuffer(size_t size, int initVal = 0)
        : m_data(new int[size]), m_size(size) {
        for (size_t i = 0; i < size; ++i) m_data[i] = initVal;
    }

    SwapBuffer(const SwapBuffer& other)
        : m_data(new int[other.m_size]), m_size(other.m_size) {
        copy(other.m_data, other.m_data + m_size, m_data);
    }

    // 提供 swap 函數
    friend void swap(SwapBuffer& a, SwapBuffer& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_size, b.m_size);
    }

    // Copy-and-Swap：參數傳值（自動觸發拷貝建構）
    SwapBuffer& operator=(SwapBuffer other) {  // ← 傳值，不是 const&
        cout << "  [SwapBuffer Copy-and-Swap 賦值]" << endl;
        swap(*this, other);  // 交換後，other 持有舊資料
        return *this;        // other 離開作用域時自動釋放舊資料
    }
    // 優點：
    //   1. 異常安全 — new 失敗時（在拷貝建構階段），原物件不受影響
    //   2. 自動處理自我賦值 — a = a 時，other 是 a 的拷貝，交換後仍正確
    //   3. 程式碼簡潔

    ~SwapBuffer() { delete[] m_data; }

    int get(size_t i) const { return m_data[i]; }
    void set(size_t i, int v) { m_data[i] = v; }
};

// --- 隱式刪除示範 ---
class HasConst {
    const int m_id;     // const 成員 → 編譯器不生成 copy assignment
    string m_name;
public:
    HasConst(int id, const string& name) : m_id(id), m_name(name) {}
    // HasConst& operator=(const HasConst&) 被隱式刪除
    // HasConst a(1,"A"), b(2,"B"); b = a; → 編譯錯誤！
    void print() const { cout << "  id=" << m_id << " name=" << m_name << endl; }
};

class HasRef {
    int& m_ref;         // 引用成員 → 編譯器不生成 copy assignment
    string m_name;
public:
    HasRef(int& ref, const string& name) : m_ref(ref), m_name(name) {}
    // HasRef& operator=(const HasRef&) 被隱式刪除
};

void demo() {
    cout << "\n=== 第 29 課：拷貝賦值運算子 ===" << endl;

    // 拷貝建構 vs 拷貝賦值
    cout << "\n--- 拷貝建構 vs 拷貝賦值 ---" << endl;
    ManagedBuffer a(3, 10);
    ManagedBuffer b = a;   // 拷貝「建構」— b 尚未存在
    ManagedBuffer c(2, 20);
    c = a;                 // 拷貝「賦值」— c 已存在

    cout << "  a[0]=" << a.get(0) << " b[0]=" << b.get(0)
         << " c[0]=" << c.get(0) << endl;

    // 鏈式賦值
    cout << "\n--- 鏈式賦值 ---" << endl;
    ManagedBuffer x(2, 1), y(2, 2), z(2, 3);
    x = y = z;  // z 賦值給 y，y 賦值給 x
    cout << "  x[0]=" << x.get(0) << " y[0]=" << y.get(0)
         << " z[0]=" << z.get(0) << endl;  // 全部是 3

    // Copy-and-Swap 示範
    cout << "\n--- Copy-and-Swap ---" << endl;
    SwapBuffer s1(3, 100), s2(2, 200);
    s2 = s1;
    cout << "  s2[0]=" << s2.get(0) << endl;  // 100

    // 隱式刪除說明
    cout << "\n--- 隱式刪除說明 ---" << endl;
    cout << "  含 const 成員 → 拷貝賦值被隱式刪除" << endl;
    cout << "  含引用成員   → 拷貝賦值被隱式刪除" << endl;
    cout << "  （因為 const 不能重新賦值，引用不能重新綁定）" << endl;
}

} // namespace Lesson29

// ============================================================
// 第 30 課：三法則（Rule of Three）
// ============================================================
// 【核心觀念】
// 如果類別需要自訂以下任何一個，通常三個都需要自訂：
//   1. 解構函數（Destructor）
//   2. 拷貝建構函數（Copy Constructor）
//   3. 拷貝賦值運算子（Copy Assignment Operator）
//
// 為什麼？
//   → 需要自訂解構函數 = 類別管理了動態資源（如 new 出來的記憶體）
//   → 管理動態資源 = 預設的淺拷貝不安全
//   → 所以拷貝建構和拷貝賦值也必須自訂（做深拷貝）
//
// 【完整示範：ManagedString — 三法則的完整實現】

namespace Lesson30 {

class ManagedString {
    // ★ 宣告順序很重要：長度在前、指標在後。
    //   成員的初始化順序**永遠依宣告順序**，與初始化列的書寫順序無關。
    //   把 m_len 放前面，就能安全地在初始化列中用它來計算配置大小。
    //   （若順序相反，寫 : m_len(...), m_data(new char[m_len + 1]) 時
    //     m_data 會先被初始化，此時 m_len 尚未賦值 → 用未初始化的值配置記憶體。）
    size_t m_len;
    char*  m_data;

public:
    // 一般建構函數
    ManagedString(const char* str = "") {
        m_len = strlen(str);
        m_data = new char[m_len + 1];
        strcpy(m_data, str);
        cout << "  [建構] \"" << m_data << "\" @" << (void*)m_data << endl;
    }

    // ===== Rule of Three 三法則 =====

    // ① 解構函數
    ~ManagedString() {
        cout << "  [解構] \"" << m_data << "\" @" << (void*)m_data << endl;
        delete[] m_data;
    }

    // ② 拷貝建構函數（深拷貝）
    ManagedString(const ManagedString& other)
        : m_len(other.m_len), m_data(new char[other.m_len + 1]) {
        strcpy(m_data, other.m_data);
        cout << "  [拷貝建構] \"" << m_data << "\" @" << (void*)m_data
             << " ← @" << (void*)other.m_data << endl;
    }

    // ③ 拷貝賦值運算子（Copy-and-Swap）
    friend void swap(ManagedString& a, ManagedString& b) noexcept {
        using std::swap;
        swap(a.m_data, b.m_data);
        swap(a.m_len, b.m_len);
    }

    ManagedString& operator=(ManagedString other) {  // 傳值觸發拷貝
        cout << "  [拷貝賦值 Copy-and-Swap]" << endl;
        swap(*this, other);
        return *this;
    }

    // ===== 三法則結束 =====

    void print() const {
        cout << "  \"" << m_data << "\" (len=" << m_len
             << ", @" << (void*)m_data << ")" << endl;
    }
};

void demo() {
    cout << "\n=== 第 30 課：三法則（Rule of Three）===" << endl;

    cout << "\n--- 完整生命週期示範 ---" << endl;
    {
        ManagedString a("Hello");     // 建構
        ManagedString b(a);           // 拷貝建構（深拷貝）
        ManagedString c("World");     // 建構
        c = a;                        // 拷貝賦值（Copy-and-Swap）

        cout << "\n  三個物件的狀態：" << endl;
        cout << "  a: "; a.print();
        cout << "  b: "; b.print();
        cout << "  c: "; c.print();
        cout << "  （三個物件有各自獨立的記憶體）" << endl;
    }
    cout << "  （三個物件安全解構，無 double free）" << endl;

    cout << "\n--- 三法則速查 ---" << endl;
    cout << "  需要自訂解構函數？ → 也需要自訂拷貝建構 + 拷貝賦值" << endl;
    cout << "  判斷依據：類別是否管理動態資源（new/malloc/file handle...）" << endl;
    cout << "  如果只用 string, vector 等 RAII 容器 → 不需要三法則" << endl;
}

} // namespace Lesson30

// ============================================================
// main
// ============================================================
// 【日常實務範例】文字編輯器的 Undo 快照：深拷貝 + copy-and-swap
// ============================================================
//   情境：編輯器每次修改前先存一份文件快照，使用者按 Ctrl+Z 就還原。
//   為什麼一定要「深」拷貝：
//     快照必須與現行文件完全獨立。若是淺拷貝，兩者共用同一塊緩衝區，
//     使用者接著打字就會把「快照」一起改掉 —— undo 還原出來的仍是新內容。
//   為什麼用 copy-and-swap：
//     還原（restore）時若中途配置失敗，文件不能變成半毀狀態。
//     copy-and-swap 把所有可能失敗的配置放在參數複製階段，
//     進入函式本體後只剩不會失敗的 swap —— 要嘛完全還原，要嘛完全不動。
//
//   這裡刻意用手寫的 char* 緩衝區（而非 std::string），
//   目的是把三法則的每一步攤開來看。實務上請直接用 std::string，
//   那就是「零法則」——一個特殊成員函式都不用寫。
// ============================================================
namespace Practical {

class Document {
    char*  m_buf;      // 宣告在前
    size_t m_len;      // 宣告在後（初始化順序依此，與初始化列表寫法無關）

public:
    // 一般建構
    explicit Document(const char* text = "")
        : m_buf(nullptr), m_len(strlen(text)) {
        m_buf = new char[m_len + 1];
        strcpy(m_buf, text);
    }

    // ① 解構函式
    ~Document() { delete[] m_buf; }

    // ② 拷貝建構（深拷貝）：用 other.m_len，不是自己未初始化的 m_len
    Document(const Document& other)
        : m_buf(new char[other.m_len + 1]), m_len(other.m_len) {
        strcpy(m_buf, other.m_buf);
    }

    // 交換：noexcept，這是 copy-and-swap 例外安全的基礎
    friend void swap(Document& a, Document& b) noexcept {
        std::swap(a.m_buf, b.m_buf);
        std::swap(a.m_len, b.m_len);
    }

    // ③ 拷貝賦值：copy-and-swap
    //    參數按值傳遞 → 複製（可能失敗）在此完成，此時 *this 尚未被動過
    Document& operator=(Document other) {
        swap(*this, other);
        return *this;
    }   // other 帶著舊緩衝區離開作用域並解構

    void append(const char* text) {
        size_t add = strlen(text);
        char* nb = new char[m_len + add + 1];
        strcpy(nb, m_buf);
        strcpy(nb + m_len, text);
        delete[] m_buf;
        m_buf = nb;
        m_len += add;
    }

    const char* text() const { return m_buf; }
    size_t      size() const { return m_len; }
};

// 簡易編輯器：修改前存快照，可還原
class Editor {
    Document m_doc;
    Document m_snapshot;
    bool     m_hasSnapshot = false;

public:
    explicit Editor(const char* initial) : m_doc(initial), m_snapshot(initial) {}

    void type(const char* text) {
        m_snapshot = m_doc;          // 深拷貝快照（copy-and-swap 賦值）
        m_hasSnapshot = true;
        m_doc.append(text);
        cout << "    輸入 \"" << text << "\" → 目前內容：" << m_doc.text() << endl;
    }

    void undo() {
        if (!m_hasSnapshot) { cout << "    沒有可還原的快照" << endl; return; }
        m_doc = m_snapshot;          // 還原（強例外保證）
        m_hasSnapshot = false;
        cout << "    Undo → 目前內容：" << m_doc.text() << endl;
    }

    const Document& doc() const { return m_doc; }   // 唯讀視圖，不外洩可寫通道
};

void demo() {
    cout << "\n=== 日常實務：編輯器 Undo 快照（深拷貝 + copy-and-swap）===" << endl;

    cout << "\n  --- 1) 深拷貝確保快照獨立 ---" << endl;
    Editor ed("Hello");
    cout << "    初始內容：" << ed.doc().text() << endl;
    ed.type(", World");
    ed.type("!!!");
    ed.undo();
    cout << "    → 快照是獨立的記憶體，後續輸入不會污染它。" << endl;

    cout << "\n  --- 2) 自我賦值安全（copy-and-swap 天生免疫）---" << endl;
    Document d("self-assign test");
    d = d;                                   // 天真的實作會在此讀到已釋放的記憶體
    cout << "    d = d 之後：" << d.text()
         << "（長度 " << d.size() << "）" << endl;
    cout << "    → copy-and-swap 先複製再交換，自我賦值完全安全。" << endl;

    cout << "\n  --- 3) 拷貝建構 vs 拷貝賦值 ---" << endl;
    Document a("AAA");
    Document b = a;                          // 拷貝建構（b 從無到有）
    Document c("CCC");
    c = a;                                   // 拷貝賦值（c 已存在，先釋放舊的）
    cout << "    a=" << a.text() << "  b(拷貝建構)=" << b.text()
         << "  c(拷貝賦值)=" << c.text() << endl;

    b.append("-modified");                   // 改 b 不應影響 a
    cout << "    修改 b 之後： a=" << a.text() << "  b=" << b.text() << endl;
    cout << "    → 三份各自獨立，這就是深拷貝的目的。" << endl;

    cout << "\n  --- 4) 現代寫法提醒 ---" << endl;
    cout << "    上面這些手寫程式碼，換成 std::string 成員後全部可以刪除：" << endl;
    cout << "    解構／拷貝建構／拷貝賦值／移動建構／移動賦值都由編譯器正確產生。" << endl;
    cout << "    這就是零法則（Rule of Zero）—— 最好的資源管理程式碼是不用寫的那種。" << endl;
}

} // namespace Practical

// ============================================================
int main() {
    Lesson27::demo();
    Lesson28::demo();
    Lesson29::demo();
    Lesson30::demo();
    Practical::demo();

    cout << "\n========================================" << endl;
    cout << "第 27～30 課總結完畢（拷貝語義）" << endl;
    cout << "下一步：summary27-35-移動語義.cpp" << endl;
    cout << "========================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary27-35-拷貝語義.cpp -o copy_semantics
//   （本檔零警告；成員宣告順序已與初始化列表一致，見概念補充 D）

//   ⚠️ 非決定性內容：輸出中所有 @0x... 的記憶體位址（以及 &r 這類位址）
//      **每次執行都不同**（作業系統的 ASLR 會隨機化堆積與堆疊位置）。
//      下方貼的是某一次實跑的結果；請只比對「位址之間的關係」
//      （例如深拷貝的來源與目的位址不同、移動後來源變成 0 或 nullptr），
//      不要比對具體數值。
// === 預期輸出 ===
// === 第 27 課：淺拷貝 vs 深拷貝 ===
//
// --- 淺拷貝問題（概念說明）---
//   ShallowString a("hello");
//   ShallowString b = a;  // 淺拷貝：b.m_data == a.m_data
//   // a 解構 → delete m_data
//   // b 解構 → delete 同一塊 m_data → 💥 double free!
//
// --- 深拷貝正確示範 ---
//   [建構] 分配記憶體 @0x5e131ea7c020
//   [深拷貝建構] 新記憶體 @0x5e131ea7c340 (來源 @0x5e131ea7c020)
//   內容: Hello
//   內容: Hello
//   （a 和 b 有各自獨立的記憶體，解構安全）
//   [解構] 釋放記憶體 @0x5e131ea7c340
//   [解構] 釋放記憶體 @0x5e131ea7c020
//
// === 第 28 課：拷貝建構函數 ===
//
// --- 場景 1 & 2：直接初始化 / 拷貝初始化 ---
//   [建構] original
//   [拷貝建構] original_copy ← original
//   [拷貝建構] original_copy ← original
//
// --- 場景 3：函數傳值 ---
//   [拷貝建構] original_copy ← original
//   函數內使用: original_copy
//   [解構] original_copy
//
// --- 場景 4：函數回傳（NRVO 可能省略拷貝）---
//   [建構] fromFunc
//   得到: fromFunc
//
// --- IntArray 深拷貝示範 ---
//   [IntArray 深拷貝] size=3
//   arr1[0]=10 arr2[0]=99
//   [解構] fromFunc
//   [解構] original_copy
//   [解構] original_copy
//   [解構] original
//
// === 第 29 課：拷貝賦值運算子 ===
//
// --- 拷貝建構 vs 拷貝賦值 ---
//   [ManagedBuffer 拷貝建構]
//   [ManagedBuffer 拷貝賦值]
//   a[0]=10 b[0]=10 c[0]=10
//
// --- 鏈式賦值 ---
//   [ManagedBuffer 拷貝賦值]
//   [ManagedBuffer 拷貝賦值]
//   x[0]=3 y[0]=3 z[0]=3
//
// --- Copy-and-Swap ---
//   [SwapBuffer Copy-and-Swap 賦值]
//   s2[0]=100
//
// --- 隱式刪除說明 ---
//   含 const 成員 → 拷貝賦值被隱式刪除
//   含引用成員   → 拷貝賦值被隱式刪除
//   （因為 const 不能重新賦值，引用不能重新綁定）
//
// === 第 30 課：三法則（Rule of Three）===
//
// --- 完整生命週期示範 ---
//   [建構] "Hello" @0x5e131ea7c020
//   [拷貝建構] "Hello" @0x5e131ea7c340 ← @0x5e131ea7c020
//   [建構] "World" @0x5e131ea7c360
//   [拷貝建構] "Hello" @0x5e131ea7c380 ← @0x5e131ea7c020
//   [拷貝賦值 Copy-and-Swap]
//   [解構] "World" @0x5e131ea7c360
//
//   三個物件的狀態：
//   a:   "Hello" (len=5, @0x5e131ea7c020)
//   b:   "Hello" (len=5, @0x5e131ea7c340)
//   c:   "Hello" (len=5, @0x5e131ea7c380)
//   （三個物件有各自獨立的記憶體）
//   [解構] "Hello" @0x5e131ea7c380
//   [解構] "Hello" @0x5e131ea7c340
//   [解構] "Hello" @0x5e131ea7c020
//   （三個物件安全解構，無 double free）
//
// --- 三法則速查 ---
//   需要自訂解構函數？ → 也需要自訂拷貝建構 + 拷貝賦值
//   判斷依據：類別是否管理動態資源（new/malloc/file handle...）
//   如果只用 string, vector 等 RAII 容器 → 不需要三法則
//
// === 日常實務：編輯器 Undo 快照（深拷貝 + copy-and-swap）===
//
//   --- 1) 深拷貝確保快照獨立 ---
//     初始內容：Hello
//     輸入 ", World" → 目前內容：Hello, World
//     輸入 "!!!" → 目前內容：Hello, World!!!
//     Undo → 目前內容：Hello, World
//     → 快照是獨立的記憶體，後續輸入不會污染它。
//
//   --- 2) 自我賦值安全（copy-and-swap 天生免疫）---
//     d = d 之後：self-assign test（長度 16）
//     → copy-and-swap 先複製再交換，自我賦值完全安全。
//
//   --- 3) 拷貝建構 vs 拷貝賦值 ---
//     a=AAA  b(拷貝建構)=AAA  c(拷貝賦值)=AAA
//     修改 b 之後： a=AAA  b=AAA-modified
//     → 三份各自獨立，這就是深拷貝的目的。
//
//   --- 4) 現代寫法提醒 ---
//     上面這些手寫程式碼，換成 std::string 成員後全部可以刪除：
//     解構／拷貝建構／拷貝賦值／移動建構／移動賦值都由編譯器正確產生。
//     這就是零法則（Rule of Zero）—— 最好的資源管理程式碼是不用寫的那種。
//
// ========================================
// 第 27～30 課總結完畢（拷貝語義）
// 下一步：summary27-35-移動語義.cpp
// ========================================
