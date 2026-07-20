// =============================================================================
//  第 33 課 -1  —  移動賦值運算子：把資源搬進「已經初始化」的物件
// =============================================================================
//
// 【主題資訊 Information】
//   簽名     ： ClassName& operator=(ClassName&& other) noexcept;
//   四個步驟 ： ① 自我賦值防護（if this == &other）
//               ② 釋放自己原有的資源（delete[]）
//               ③ 偷走 other 的資源（複製指標，不配置新記憶體）
//               ④ 置空 other，回傳 *this
//   標準版本 ： C++11
//   標頭檔   ： <utility>（std::move、std::swap）
//   複雜度   ： O(1)（只搬指標）；對照拷貝賦值的 O(n)
//   noexcept ： 應該標 —— 它只搬指標、不配置記憶體，天生不會拋
//
// 【詳細解釋 Explanation】
//
// 【1. 移動賦值 vs 移動建構：差在「有沒有舊資源要先處理」】
//   移動建構函數面對的是「還沒初始化」的新物件，直接接手來源即可。
//   移動賦值運算子面對的是「已經初始化過」的物件，
//   所以比移動建構多了一步：要先把自己原本持有的資源釋放掉，
//   否則那塊記憶體就洩漏了。這就是步驟 ② delete[] m_data 的作用。
//   （對照第 32 課 -1 的移動建構，它沒有這一步。）
//
// 【2. ★ 自我賦值防護在移動賦值裡是「必要」的，不是可選的】
//   這是移動賦值與拷貝賦值最關鍵的差別。
//   本檔的拷貝賦值採「先配置新的、成功後才釋放舊的」寫法，
//   即使少了 if (this == &other)，自我賦值也頂多多做一次白工，仍然正確。
//   但移動賦值的順序是「先 delete 自己、再從 other 偷」，
//   而自我賦值時 other 就是自己：
//       delete[] m_data;          // 把自己的資源刪了
//       m_data = other.m_data;    // other 就是自己 → 這是個已釋放的懸空指標
//   於是後續使用 m_data 就是存取已釋放的記憶體 —— 未定義行為。
//   所以移動賦值的 if (this == &other) 防護不能省。
//   本檔測試 4 就是在驗證這道防護有效。
//
// 【3. 測試 5：鏈式賦值裡藏著的「移動變拷貝」】
//   c = b = std::move(a) 是本課最愛考的一行。拆開來看：
//       b = std::move(a)   → std::move(a) 是右值 → 走「移動賦值」
//       回傳的是 b 的參考（SimpleString&），是個「具名左值」
//       c = (那個左值)     → 左值 → 走「拷貝賦值」！
//   所以這一行是「一次移動 + 一次拷貝」，不是兩次移動。
//   關鍵在第 31 課反覆強調的規則：右值引用／賦值的回傳值一旦有了名字
//   （或說能被取址），它就是左值。輸出中 [移動賦值] 後面緊跟 [拷貝賦值]
//   正是這件事的證據。
//
// 【4. 測試 6：被移動過的物件可以再次被移動賦值】
//   步驟 1 把 a 掏空（a 變成 valid but unspecified）。
//   步驟 2 再對 a 做 a = std::move(c)，a 重新獲得資源。
//   這驗證了第 31 課的契約：被移動後的物件「重新賦值」是安全的。
//   移動賦值的步驟 ② 對「已經是 nullptr」的 m_data 執行 delete[]，
//   而 delete[] nullptr 是標準保證的空操作，所以完全安全。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼移動賦值應該標 noexcept
//   它只複製指標值、把來源置空，不配置任何記憶體，天生不會拋例外。
//   標了 noexcept，std::vector 這類容器在需要「移動賦值既有元素」時
//   才會採用它（機制與移動建構的 move_if_noexcept 同源，見第 32 課 -2）。
//   相對地，拷貝賦值要配置記憶體、可能拋 bad_alloc，不能標 noexcept。
//
// (B) 這個「先 delete 再偷」的寫法有一個小缺點
//   它不能像 copy-and-swap 那樣，用一份程式碼同時處理拷貝與移動；
//   而且自我賦值防護是「手動」加的，容易漏。
//   copy-and-swap（第 33 課 -2）用「傳值 + swap」一次解決兩者，
//   還天生免疫自我賦值、附帶強例外保證，缺點是拷貝路徑一定會重新配置記憶體。
//   兩種寫法各有取捨：分開寫（本檔）在拷貝時能重用緩衝區、較省；
//   統一寫（-2）較簡潔安全。實務上兩種都常見。
//
// (C) 為什麼可以安全印出被移動後的物件內容
//   c_str() 寫成 m_data ? m_data : "(null)"，而移動操作明確把來源的
//   m_data 設成 nullptr。所以印出的 "(null)" 是「本程式自己歸零的成員」，
//   不是去讀 valid but unspecified 的內容。
//   ⚠️ 若成員換成 std::string，被移動後的內容沒有任何標準保證，
//   就「不可以」直接印出來（見第 31 課 -4）。
//
// (D) 成員宣告順序（-Wreorder）
//   m_data 的初始值用到了 m_len（new char[m_len + 1]），
//   所以 m_len 必須宣告在 m_data 之前 —— 檔案中已是正確順序。
//   對調會讓 m_data 用到尚未初始化的 m_len，配置錯誤大小的緩衝區。
//
// 【注意事項 Pay Attention】
//   1. 移動賦值一定要有自我賦值防護 —— 它是「先 delete 再偷」，
//      少了防護，a = std::move(a) 會從已釋放的自己偷取（未定義行為）。
//      這與拷貝賦值不同：拷貝賦值採「先配置後釋放」時可容忍缺防護。
//   2. 移動賦值要標 noexcept；它只搬指標，天生不會拋。
//   3. 記得釋放自己原有的資源（步驟 ②），否則記憶體洩漏。
//   4. 鏈式賦值 c = b = std::move(a) 的外層是拷貝，不是移動 ——
//      因為 b = ... 回傳的是具名左值（見測試 5）。
//   5. 能安全印出被移動物件的 "(null)" 是因為那是自己歸零的成員；
//      標準庫型別（std::string）被移動後不可這樣印。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動賦值運算子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動賦值運算子和移動建構函數差在哪？多做了什麼？
//     答：移動建構面對「還沒初始化」的物件，直接接手來源即可。
//         移動賦值面對「已經初始化過」的物件，多兩件事：
//         先釋放自己原有的資源（否則洩漏），並加上自我賦值防護。
//         兩者共通的是「偷指標 + 置空來源」，成本都是 O(1)。
//     追問：移動賦值該不該標 noexcept？
//         → 該。它只搬指標、不配置記憶體，天生不會拋；
//           標了容器（如 vector）才會在移動賦值既有元素時採用它。
//
// 🔥 Q2. 為什麼移動賦值「一定」要自我賦值防護，而拷貝賦值有時可以不用？
//     答：移動賦值的順序是「先 delete 自己、再從 other 偷」。
//         自我賦值時 other 就是自己，delete 之後再從自己偷，
//         拿到的是懸空指標，後續使用是未定義行為，所以防護不能省。
//         拷貝賦值若採「先配置新的、成功後才釋放舊的」寫法，
//         自我賦值頂多多做一次白工，仍然正確，防護就不是必要的。
//     追問：那 copy-and-swap 呢？
//         → 它天生免疫自我賦值：參數是獨立副本，即使 a = a 也是
//           從 a 拷貝出來的新物件與 this 不同記憶體，swap 完全安全，
//           所以完全不需要手動防護（見第 33 課 -2）。
//
// ⚠️ 陷阱. 「c = b = std::move(a) 用了 std::move，所以兩次賦值都是移動賦值。」
//     答：不是。只有內層 b = std::move(a) 是移動賦值。
//         它回傳 b 的參考，而具名參考是左值，
//         所以外層 c = b 走的是「拷貝賦值」。
//         這一行是「一次移動 + 一次拷貝」。本檔測試 5 的輸出
//         [移動賦值] 之後緊跟 [拷貝賦值] 就是證據。
//     為什麼會錯：以為 std::move 的效力會「傳染」整條運算式。
//         實際上 std::move 只作用在它括住的那個運算式上；
//         賦值運算子回傳的具名左值，早已不帶右值屬性了
//         （第 31 課的核心規則：有名字的右值引用是左值）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   移動賦值運算子是資源管理型類別的實作細節，
//   價值在效能（O(1) 轉移）與正確性（自我賦值、例外安全）。
//   LeetCode 判的是演算法的輸入輸出與時間限制，
//   評測系統只會建立一個實例並呼叫指定方法，
//   從不對你的物件做移動賦值，也不會注入自我賦值來檢驗防護。
//   硬掛設計類題號只會誤導，故從缺；
//   本檔以測試 1~6 的六種賦值情境完整覆蓋本主題，證據比題號更直接。
//
// -----------------------------------------------------------------------------
// lesson33_move_assignment.cpp
// 驗證：valgrind --leak-check=full ./lesson33
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <utility>

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

    // ──────── 解構函數 ────────
    ~SimpleString() {
        if (m_data)
            std::cout << "  [解構] \"" << m_data << "\"\n";
        else
            std::cout << "  [解構] (nullptr)\n";
        delete[] m_data;
    }

    // ──────── 拷貝建構函數 ────────
    SimpleString(const SimpleString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
    {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // ──────── 移動建構函數 ────────
    SimpleString(SimpleString&& other) noexcept
        : m_len(other.m_len)
        , m_data(other.m_data)
    {
        other.m_data = nullptr;
        other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    // ──────── 拷貝賦值運算子（先配置後釋放，具強例外保證）────────
    SimpleString& operator=(const SimpleString& other) {
        std::cout << "  [拷貝賦值] ";
        if (this == &other) {
            std::cout << "自我賦值，跳過\n";
            return *this;
        }

        // 先配置再釋放（例外安全）：new 若失敗，自己的舊資源還完好
        char* newData = new char[other.m_len + 1];
        std::strcpy(newData, other.m_data);

        delete[] m_data;
        m_data = newData;
        m_len = other.m_len;

        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ════════════════════════════════════════════════
    // ★ 移動賦值運算子 ★
    // ════════════════════════════════════════════════
    SimpleString& operator=(SimpleString&& other) noexcept {
        std::cout << "  [移動賦值] ";
        // ① 自我賦值防護 —— 移動賦值「必須」有，否則會從已釋放的自己偷取
        if (this == &other) {
            std::cout << "自我賦值，跳過\n";
            return *this;
        }

        // ② 釋放舊資源（否則洩漏）
        delete[] m_data;

        // ③ 偷走 other 的資源（只搬指標，O(1)）
        m_data = other.m_data;
        m_len  = other.m_len;

        // ④ 置空 other（否則兩者共享 → double free）
        other.m_data = nullptr;
        other.m_len  = 0;

        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ──────── swap ────────
    void swap(SimpleString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ──────── 存取 ────────
    // 印被移動後的物件是安全的：m_data 由移動操作明確歸零，這裡印的是
    // 自己歸零的成員，不是去讀 valid but unspecified 的內容。
    const char* c_str() const { return m_data ? m_data : "(null)"; }
    std::size_t length() const { return m_len; }
};

int main() {
    std::cout << "===== 測試 1：拷貝賦值（左值）=====\n";
    {
        SimpleString a("Dragon");
        SimpleString b("Knight");
        std::cout << "  執行 b = a:\n";
        b = a;   // a 是左值 → 拷貝賦值
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 2：移動賦值（右值）=====\n";
    {
        SimpleString a("Wizard");
        SimpleString b("Archer");
        std::cout << "  執行 b = std::move(a):\n";
        b = std::move(a);   // std::move(a) 是右值 → 移動賦值
        std::cout << "  a=\"" << a.c_str() << "\" (已被移動)\n";
        std::cout << "  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 3：從暫時物件移動賦值 =====\n";
    {
        SimpleString a("Rogue");
        std::cout << "  執行 a = SimpleString(\"Phoenix\"):\n";
        a = SimpleString("Phoenix");  // 暫時物件是右值 → 移動賦值
        std::cout << "  a=\"" << a.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 4：自我移動賦值 =====\n";
    {
        SimpleString a("Paladin");
        std::cout << "  執行 a = std::move(a):\n";
        // ★ 這行故意示範「自我移動賦值」以驗證步驟 ① 的防護。
        //   GCC 的 -Wself-move 會對這種明顯的寫法發出警告（它是對的：
        //   正常程式不該這樣寫）。這裡只在這一行關掉該警告，
        //   讓整份檔案仍能以 -Wall -Wextra 乾淨編譯。
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
        a = std::move(a);   // 自我賦值檢查保護
#pragma GCC diagnostic pop
        std::cout << "  a=\"" << a.c_str() << "\" (安全)\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 5：鏈式移動賦值 =====\n";
    {
        SimpleString a("Alpha");
        SimpleString b("Beta");
        SimpleString c("Gamma");
        std::cout << "  執行 c = b = std::move(a):\n";
        c = b = std::move(a);
        // 分解：b = std::move(a) → 移動賦值，回傳 b（具名左值）
        //       c = b           → 拷貝賦值（因為 b 是左值！）
        std::cout << "  a=\"" << a.c_str() << "\"\n";
        std::cout << "  b=\"" << b.c_str() << "\"\n";
        std::cout << "  c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：移動賦值給被移動過的物件 =====\n";
    {
        SimpleString a("First");
        SimpleString b("Second");
        SimpleString c("Third");

        std::cout << "  步驟 1：b = std::move(a)\n";
        b = std::move(a);      // a 被掏空
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";

        std::cout << "  步驟 2：a = std::move(c)  (對被移動過的 a 重新移動賦值)\n";
        a = std::move(c);      // a 重新獲得資源（對已是 nullptr 的 m_data 執行 delete[] 是安全的）
        std::cout << "  a=\"" << a.c_str() << "\"  c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 所有測試完成 =====\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 33 課：移動賦值運算子（Move Assignment Operator）1.cpp" -o lesson33

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：沒有位址、沒有耗時、沒有執行緒
//   （本機實測連跑 5 次逐位元組相同）。用「建構／賦值／解構的呼叫順序」
//   當證據，而不是量時間 —— 計數可重現，計時不行。
// * 印出被移動物件的 "(null)"、"(nullptr)" 是安全的：那是移動操作
//   明確歸零的成員（m_data = nullptr），不是去讀 valid but unspecified
//   的內容。若成員換成 std::string 就不能這樣印。
// * 測試 5 是本檔的重點：c = b = std::move(a) 的輸出是
//   [移動賦值] 之後緊跟 [拷貝賦值]，證明外層是拷貝不是移動 ——
//   因為 b = ... 回傳的是具名左值。
// * 測試 3 中間那行 [解構] (nullptr) 是暫時物件 SimpleString("Phoenix")
//   被移動賦值掏空後、在該行分號處解構時印的（資源已被 a 接手）。
// * 測試 6 步驟 2 對「已被移動過（m_data 為 nullptr）」的 a 再做移動賦值，
//   步驟 ② 的 delete[] nullptr 是標準保證的空操作，所以完全安全。
// * 各測試包在獨立的 { } 區塊內，所以每段結束就會看到該段區域物件的
//   [解構]，順序是宣告的反向。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 測試 1：拷貝賦值（左值）=====
//   [建構] "Dragon"
//   [建構] "Knight"
//   執行 b = a:
//   [拷貝賦值] "Dragon"
//   a="Dragon"  b="Dragon"
//   [解構] "Dragon"
//   [解構] "Dragon"
//
// ===== 測試 2：移動賦值（右值）=====
//   [建構] "Wizard"
//   [建構] "Archer"
//   執行 b = std::move(a):
//   [移動賦值] "Wizard"
//   a="(null)" (已被移動)
//   b="Wizard"
//   [解構] "Wizard"
//   [解構] (nullptr)
//
// ===== 測試 3：從暫時物件移動賦值 =====
//   [建構] "Rogue"
//   執行 a = SimpleString("Phoenix"):
//   [建構] "Phoenix"
//   [移動賦值] "Phoenix"
//   [解構] (nullptr)
//   a="Phoenix"
//   [解構] "Phoenix"
//
// ===== 測試 4：自我移動賦值 =====
//   [建構] "Paladin"
//   執行 a = std::move(a):
//   [移動賦值] 自我賦值，跳過
//   a="Paladin" (安全)
//   [解構] "Paladin"
//
// ===== 測試 5：鏈式移動賦值 =====
//   [建構] "Alpha"
//   [建構] "Beta"
//   [建構] "Gamma"
//   執行 c = b = std::move(a):
//   [移動賦值] "Alpha"
//   [拷貝賦值] "Alpha"
//   a="(null)"
//   b="Alpha"
//   c="Alpha"
//   [解構] "Alpha"
//   [解構] "Alpha"
//   [解構] (nullptr)
//
// ===== 測試 6：移動賦值給被移動過的物件 =====
//   [建構] "First"
//   [建構] "Second"
//   [建構] "Third"
//   步驟 1：b = std::move(a)
//   [移動賦值] "First"
//   a="(null)"  b="First"
//   步驟 2：a = std::move(c)  (對被移動過的 a 重新移動賦值)
//   [移動賦值] "Third"
//   a="Third"  c="(null)"
//   [解構] (nullptr)
//   [解構] "First"
//   [解構] "Third"
//
// ===== 所有測試完成 =====
