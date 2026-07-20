// =============================================================================
//  第 33 課 總結  —  移動賦值運算子（Move Assignment Operator）
// =============================================================================
//
// 【主題資訊 Information】
//   簽名(分開寫) : ClassName& operator=(ClassName&& other) noexcept;
//                  ClassName& operator=(const ClassName& other);
//   簽名(統一寫) : ClassName& operator=(ClassName other);        // 傳值 + swap
//   標準版本     : C++11 起（右值引用與移動語意）
//   標頭檔       : <utility>（std::move / std::swap）
//   複雜度       : 移動賦值 O(1)；拷貝賦值 O(N)（N = 持有的資源大小）
//   回傳值       : 一律回傳 *this（型別為 ClassName&），以支援鏈式賦值
//
// 【詳細解釋 Explanation】
//
// 【1. 移動賦值與移動建構差在哪？】
//   移動建構面對的是「一塊還沒有內容的記憶體」，只要把來源的資源搬進來即可。
//   移動賦值面對的是「一個已經活著、而且可能已經持有資源的物件」，所以多了
//   一個責任：先處理掉自己原有的資源，否則就洩漏。這就是為什麼移動賦值的
//   標準流程比移動建構多了「釋放舊資源」與「自我賦值檢查」兩步：
//     ① if (this == &other) return *this;   // 自我賦值防護
//     ② delete[] m_data;                    // 釋放自己的舊資源
//     ③ m_data = other.m_data;              // 偷取
//     ④ other.m_data = nullptr;             // 把來源歸零（關鍵！）
//     ⑤ return *this;
//   第 ④ 步不能省：若不歸零，來源解構時會 delete 同一塊記憶體，造成
//   double free。
//
// 【2. 為什麼自我賦值檢查在移動賦值裡特別致命？】
//   拷貝賦值若漏了自我賦值檢查，用「先配置新的、成功後再釋放舊的」寫法
//   仍然安全。但移動賦值是「先 delete 再從 other 讀」——若 this == &other，
//   delete 掉的就是等一下要讀的那塊記憶體，接著把已釋放的指標抄回自己身上。
//   結果是使用已釋放記憶體 + 解構時二次釋放，兩個都是未定義行為。
//   真實世界不會有人寫 a = std::move(a)，但 v[i] = std::move(v[j]) 在
//   i == j 時就是同一回事，而編譯器完全看不出來。
//
// 【3. 統一賦值（copy-and-swap）為什麼被推薦？】
//   ClassName& operator=(ClassName other) { swap(*this, other); return *this; }
//   一個函式同時處理兩條路徑，因為「參數 other 怎麼來的」由編譯器決定：
//     傳左值 → other 由拷貝建構產生 → 等效於拷貝賦值
//     傳右值 → other 由移動建構產生 → 等效於移動賦值
//   好處有三：(a) 少寫一個函式、不會兩份邏輯不同步；(b) 天生具備強例外保證
//   ——所有可能拋例外的動作（配置記憶體）都發生在參數建構階段，此時 *this
//   還沒被改動，一旦拋出，*this 維持原狀；(c) 自我賦值天然安全，因為
//   other 是一份獨立的副本，swap 進來也不會出事。
//   代價：傳左值時一定會產生一次拷貝，無法像手寫拷貝賦值那樣在容量足夠時
//   直接重用既有緩衝區。對配置成本敏感的類別（如經常被賦值的大型 buffer），
//   分開寫仍可能比較快。
//
// 【4. 為什麼移動賦值要標 noexcept？】
//   std::vector 擴容時必須把既有元素搬到新緩衝區。為了維持強例外保證，
//   它用 std::move_if_noexcept：只有當元素的移動操作是 noexcept 時才移動，
//   否則寧可退化成拷貝（拷貝失敗時舊緩衝區還在，可以安全回滾）。
//   所以一個沒標 noexcept 的移動操作，會讓你的類別在 vector 裡失去所有
//   移動的好處——這是「寫了移動卻沒變快」最常見的原因。
//
// 【5. 鏈式賦值 c = b = std::move(a) 到底發生什麼？】
//   賦值運算子是右結合，所以先算 b = std::move(a)，這是移動賦值。
//   但它回傳的是 ClassName&（左值引用），而具名的左值引用本身是左值，
//   所以接下來的 c = (b) 走的是「拷貝賦值」，不是移動賦值。
//   很多人以為 move 會沿著鏈條傳遞下去，這是錯的——move 的效果只作用在
//   它直接套用的那一個運算元上。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼回傳 ClassName& 而不是 ClassName 或 void：回傳參考才能支援
//     a = b = c 這種鏈式寫法，且不產生多餘副本。回傳 void 會讓類別無法
//     用在某些泛型演算法中；回傳值（非參考）則會多一次拷貝/移動。
//   * 「被移動後的物件」的合約：標準規定它處於 valid but unspecified 狀態
//     ——所有不需要前置條件的操作（解構、賦新值、clear()、size()）都合法，
//     但不該假設它的值。本檔的類別把來源設為 nullptr 並印出 "(null)"，
//     那是這個類別自己定義的行為，不是語言保證。
//   * 統一賦值的參數傳遞與 copy elision：C++17 起，用純右值（如
//     UnifiedString("Phoenix")）初始化參數是「保證的複製省略」——臨時物件
//     直接就地建構在參數位置，連移動建構都不會被呼叫。這就是為什麼下方
//     第 3 節「臨時物件路徑」只印了 [建構] 而沒有 [移動建構]。
//
// 【注意事項 Pay Attention】
//   1. 移動賦值一定要把來源歸零，否則 double free（未定義行為）。
//   2. 移動賦值一定要標 noexcept，否則 vector 擴容時會退回拷貝。
//   3. 自我賦值：分開寫法必須自己檢查；統一賦值天然安全。
//   4. 本檔中 a = std::move(a) 的直接寫法會被 GCC 的 -Wself-move 擋下
//      （那個警告是對的）。真正的危險情境是透過別名發生的自我移動，
//      編譯器看不出來——本檔第 1 節用參考別名示範這種情況。
//   5. 「被移動後印出什麼」是類別自訂行為，不是語言保證，不要當成通則。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動賦值運算子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動賦值運算子的標準步驟是什麼？哪一步漏了會 double free？
//     答：① 自我賦值檢查 ② 釋放自己的舊資源 ③ 偷取 other 的資源
//         ④ 把 other 歸零 ⑤ return *this。
//         漏掉 ④ 會 double free：來源與目標同時持有同一個指標，兩者解構時
//         各 delete 一次。漏掉 ② 則是資源洩漏（舊資源再也沒人管）。
//     追問：那移動建構為什麼不用「釋放舊資源」這一步？
//         → 因為移動建構作用在尚未建構的記憶體上，本來就沒有舊資源可釋放。
//
// 🔥 Q2. 為什麼移動賦值一定要標 noexcept？不標會怎樣？
//     答：vector 擴容時用 std::move_if_noexcept 決定要移動還是拷貝既有元素。
//         沒標 noexcept，它為了維持強例外保證會選擇拷貝——你寫了移動操作，
//         卻在最需要它的場合完全用不到。
//     追問：為什麼「可能拋例外的移動」無法維持強例外保證？
//         → 因為移動會破壞來源。搬到一半拋例外時，舊緩衝區裡的元素已經被
//           掏空、新緩衝區又不完整，沒有任何一邊是完好的，回滾不了。
//           拷貝則不破壞來源，失敗時舊緩衝區仍然完整。
//
// 🔥 Q3. c = b = std::move(a) 這一行，c 走的是移動賦值還是拷貝賦值？
//     答：拷貝賦值。b = std::move(a) 確實是移動賦值，但它回傳 ClassName&，
//         而具名的左值引用是左值，所以 c = b 走拷貝賦值。
//         下方第 2 節的實際輸出可以驗證：先 [移動賦值] 再 [拷貝賦值]。
//     追問：那要怎麼讓 c 也走移動？
//         → 拆開寫：b = std::move(a); c = std::move(b);
//           但要清楚這代表 b 也會被掏空。
//
// ⚠️ 陷阱. 「反正沒人會寫 a = std::move(a)，自我賦值檢查可以省」——錯在哪？
//     答：直接寫確實沒人會，而且 GCC 的 -Wself-move 也會警告你。但透過別名
//         就完全看不出來了：v[i] = std::move(v[j]) 在 i == j 時、或
//         *p = std::move(*q) 在 p == q 時，都是自我移動賦值。
//         這種呼叫在排序、去重、swap 實作裡非常容易出現。
//     為什麼會錯：把「這行程式碼長什麼樣」當成「執行期會發生什麼」。
//         自我賦值是執行期的指標相等關係，不是語法特徵，編譯器無法替你檢查。
// ═══════════════════════════════════════════════════════════════════════════
//
// ============================================================
// 第 33 課 總結：移動賦值運算子（Move Assignment Operator）
// 編譯：g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
// ============================================================
// 【簽名】ClassName& operator=(ClassName&& other) noexcept
//
// 【移動賦值步驟（分開寫法）】
//   1. 自我賦值檢查：if (this == &other) return *this;
//   2. 釋放自己的舊資源：delete[] m_data;
//   3. 偷取 other 的資源：m_data = other.m_data;
//   4. 歸零 other：other.m_data = nullptr;
//   5. 回傳 *this
//
// 【統一賦值運算子（推薦）】
//   ClassName& operator=(ClassName other) {  // 傳值！
//       swap(*this, other);
//       return *this;
//   }
//   一個函數同時處理拷貝賦值和移動賦值！
//   傳左值 → 觸發拷貝建構 → swap（拷貝賦值路徑）
//   傳右值 → 觸發移動建構 → swap（移動賦值路徑）
//
// 【拷貝賦值 vs 移動賦值】
//   b = a;            // a 是左值 → 拷貝賦值
//   b = std::move(a); // std::move(a) 是右值 → 移動賦值
//   b = func();       // func() 回傳臨時物件 → 移動賦值
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>

// ============================================================
// 方法一：分開寫拷貝賦值和移動賦值
// ============================================================
class SeparateString {
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在前。
    std::size_t m_len;
    char* m_data;
public:
    SeparateString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    ~SeparateString() {
        if (m_data) std::cout << "  [解構] \"" << m_data << "\"\n";
        else        std::cout << "  [解構] (nullptr)\n";
        delete[] m_data;
    }

    // 拷貝建構
    SeparateString(const SeparateString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    // 移動建構
    SeparateString(SeparateString&& other) noexcept
        : m_len(other.m_len), m_data(other.m_data) {
        other.m_data = nullptr; other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    // ★ 拷貝賦值運算子
    SeparateString& operator=(const SeparateString& other) {
        std::cout << "  [拷貝賦值] ";
        if (this == &other) { std::cout << "自我賦值\n"; return *this; }
        char* newData = new char[other.m_len + 1];  // 先配置再釋放（異常安全）
        std::strcpy(newData, other.m_data);
        delete[] m_data;
        m_data = newData;
        m_len = other.m_len;
        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    // ★ 移動賦值運算子
    SeparateString& operator=(SeparateString&& other) noexcept {
        std::cout << "  [移動賦值] ";
        if (this == &other) { std::cout << "自我賦值\n"; return *this; }
        delete[] m_data;             // 釋放舊資源
        m_data = other.m_data;       // 偷取
        m_len = other.m_len;
        other.m_data = nullptr;      // 歸零
        other.m_len = 0;
        std::cout << "\"" << m_data << "\"\n";
        return *this;
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

// ============================================================
// 方法二：統一賦值運算子（推薦）
// ============================================================
class UnifiedString {
    // ⚠️ 同上：m_len 必須宣告在 m_data 之前。
    std::size_t m_len;
    char* m_data;
public:
    UnifiedString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    ~UnifiedString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "null") << "\"\n";
        delete[] m_data;
    }

    UnifiedString(const UnifiedString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\"\n";
    }

    UnifiedString(UnifiedString&& other) noexcept
        : m_len(other.m_len), m_data(other.m_data) {
        other.m_data = nullptr; other.m_len = 0;
        std::cout << "  [移動建構] \"" << m_data << "\"\n";
    }

    void swap(UnifiedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
    }

    // ★ 統一賦值：一個函數處理拷貝和移動
    UnifiedString& operator=(UnifiedString other) {  // 傳值！
        std::cout << "  [統一賦值] swap\n";
        swap(other);        // 交換內容
        return *this;       // other 解構時釋放舊資料
    }
    // 傳左值 → other 由拷貝建構 → swap（拷貝路徑）
    // 傳右值 → other 由移動建構 → swap（移動路徑）

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

// ============================================================
// 【日常實務範例】連線設定的熱重載（hot reload）
//   情境：服務啟動時載入一份設定，之後收到 SIGHUP 就重新讀檔並「換掉」
//         目前這份設定。重點在「換掉」——舊設定要被正確釋放、新設定要
//         被搬進來，而且整個過程不能留下半新半舊的狀態。
//   為什麼用到移動賦值：loadConfig() 回傳一個暫時物件（純右值），
//         current = loadConfig(...) 這一行走的正是移動賦值，
//         舊設定在賦值過程中被釋放，新設定的資源被直接接管，零深拷貝。
//   注意：這裡刻意用統一賦值（傳值 + swap），因為它天生具備強例外保證——
//         若新設定在建構過程中拋例外，current 會維持原本的舊設定不變，
//         服務不會因為重載失敗而處於沒有設定可用的狀態。
// ============================================================
class ConnConfig {
    std::size_t m_len;
    char*       m_dsn;      // 連線字串，模擬「持有資源」
    int         m_timeout;
public:
    explicit ConnConfig(const char* dsn = "", int timeout = 0)
        : m_len(std::strlen(dsn)), m_dsn(new char[m_len + 1]), m_timeout(timeout) {
        std::strcpy(m_dsn, dsn);
    }
    ~ConnConfig() { delete[] m_dsn; }

    ConnConfig(const ConnConfig& o)
        : m_len(o.m_len), m_dsn(new char[m_len + 1]), m_timeout(o.m_timeout) {
        std::strcpy(m_dsn, o.m_dsn);
        std::cout << "    [設定-拷貝建構]\n";
    }
    ConnConfig(ConnConfig&& o) noexcept
        : m_len(o.m_len), m_dsn(o.m_dsn), m_timeout(o.m_timeout) {
        o.m_dsn = nullptr; o.m_len = 0;
        std::cout << "    [設定-移動建構]\n";
    }
    void swap(ConnConfig& o) noexcept {
        std::swap(m_len, o.m_len);
        std::swap(m_dsn, o.m_dsn);
        std::swap(m_timeout, o.m_timeout);
    }
    // 統一賦值：拷貝與移動共用一份邏輯，且具強例外保證
    ConnConfig& operator=(ConnConfig other) {
        std::cout << "    [設定-統一賦值] 舊設定將在此函式結束時釋放\n";
        swap(other);
        return *this;
    }
    const char* dsn() const { return m_dsn ? m_dsn : "(null)"; }
    int timeout() const { return m_timeout; }
};

// 模擬「從檔案讀出一份新設定」，回傳純右值
ConnConfig loadConfig(const char* dsn, int timeout) {
    return ConnConfig(dsn, timeout);
}

int main() {
    // ============================================================
    // 1. 分開寫法：拷貝賦值 vs 移動賦值
    // ============================================================
    std::cout << "===== 1. 分開寫法 =====\n";
    {
        SeparateString a("Dragon");
        SeparateString b("Knight");

        std::cout << "\n  拷貝賦值（左值）：b = a\n";
        b = a;

        std::cout << "\n  移動賦值（右值）：b = std::move(a)\n";
        b = std::move(a);
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";

        std::cout << "\n  移動賦值（臨時物件）：b = SeparateString(\"Phoenix\")\n";
        b = SeparateString("Phoenix");
        std::cout << "  b=\"" << b.c_str() << "\"\n";

        std::cout << "\n  自我移動賦值（透過別名，編譯器看不出來）：\n";
        // 直接寫 b = std::move(b) 會被 GCC 的 -Wself-move 擋下（那個警告是對的，
        // 沒有人該那樣寫）。但真正危險的是「透過別名發生的自我移動」——
        // 例如 v[i] = std::move(v[j]) 在 i == j 時，或兩個指標湊巧指向同一物件。
        // 這種情況編譯器完全看不出來，只能靠 operator= 裡的 this == &other 防護。
        SeparateString& alias = b;      // 模擬「另一條路徑拿到同一個物件」
        b = std::move(alias);           // 自我賦值檢查在此發揮作用
    }
    std::cout << "\n";

    // ============================================================
    // 2. 鏈式移動賦值
    // ============================================================
    std::cout << "===== 2. 鏈式移動賦值 =====\n";
    {
        SeparateString a("Alpha"), b("Beta"), c("Gamma");
        std::cout << "  c = b = std::move(a):\n";
        c = b = std::move(a);
        // 分解：b = std::move(a) → 移動賦值，回傳 b（左值！）
        //       c = b           → 拷貝賦值（因為 b 是左值）
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 3. 統一賦值運算子
    // ============================================================
    std::cout << "===== 3. 統一賦值（推薦）=====\n";
    {
        UnifiedString a("Dragon");
        UnifiedString b("Knight");

        std::cout << "\n  拷貝路徑：b = a（左值 → 拷貝建構 other → swap）\n";
        b = a;

        std::cout << "\n  移動路徑：b = std::move(a)（右值 → 移動建構 other → swap）\n";
        b = std::move(a);

        std::cout << "\n  臨時物件路徑：\n";
        b = UnifiedString("Phoenix");
    }
    std::cout << "\n";

    // ============================================================
    // 4. 對被移動物件重新賦值
    // ============================================================
    std::cout << "===== 4. 被移動物件可以重新賦值 =====\n";
    {
        SeparateString a("First"), b("Second"), c("Third");
        b = std::move(a);
        std::cout << "  a 被移動後：\"" << a.c_str() << "\"\n";
        a = std::move(c);  // 對被移動的 a 重新移動賦值
        std::cout << "  a 重新賦值：\"" << a.c_str() << "\"\n";
    }

    std::cout << "\n";

    // ============================================================
    // 5. 日常實務：設定熱重載
    // ============================================================
    std::cout << "===== 5. 日常實務：連線設定熱重載 =====\n";
    {
        ConnConfig current("host=db-old;port=5432", 30);
        std::cout << "  啟動時設定：dsn=" << current.dsn()
                  << " timeout=" << current.timeout() << "\n";

        std::cout << "\n  收到 SIGHUP，重新載入（移動賦值路徑）：\n";
        current = loadConfig("host=db-new;port=6543", 15);   // 純右值 → 移動路徑
        std::cout << "  重載後設定：dsn=" << current.dsn()
                  << " timeout=" << current.timeout() << "\n";

        std::cout << "\n  從既有物件複製一份設定（拷貝賦值路徑）：\n";
        ConnConfig fallback("host=db-backup;port=5432", 60);
        current = fallback;                                   // 左值 → 拷貝路徑
        std::cout << "  切換後設定：dsn=" << current.dsn()
                  << " timeout=" << current.timeout() << "\n";
        std::cout << "  fallback 仍可用：dsn=" << fallback.dsn() << "（拷貝不破壞來源）\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  分開寫：operator=(const T&) + operator=(T&&)\n";
    std::cout << "  統一寫（推薦）：operator=(T other) + swap\n";
    std::cout << "  統一寫自動根據傳入左/右值選擇拷貝/移動路徑\n";
    std::cout << "  鏈式賦值中 b = std::move(a) 回傳的 b 是左值\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder summary.cpp -o summary

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：沒有位址、沒有耗時、沒有執行緒，重跑結果一致
//   （本機實測連跑 4 次逐位元組相同）。本課用「建構／賦值／解構的呼叫次數
//   與順序」當證據，而不是量時間 —— 計數可重現，計時不行。
// * 為什麼可以安全印出被移動後的物件：兩個類別的 c_str() 都寫成
//   m_data ? m_data : "(null)"，而移動操作明確把 other.m_data 設為
//   nullptr。所以印出的 "(null)" 是「本程式自己歸零的成員」，
//   不是去讀 valid but unspecified 的內容。若成員換成 std::string，
//   就不可以這樣印 —— 標準庫被移動後的內容沒有任何保證。
// * 兩個類別的「空」字樣不同，不是筆誤：SeparateString 的解構印
//   (nullptr)，UnifiedString 的解構印 "null"，來源是各自的 dtor 寫法。
// * 第 2 段是本課最容易考的一行：c = b = std::move(a) 先走 [移動賦值]，
//   再走 [拷貝賦值]。因為 b = std::move(a) 回傳的是 SeparateString&，
//   具名參考是左值，所以外層的 c = ... 選中拷貝賦值而非移動賦值。
// * 第 3 段統一賦值（傳值 + swap）的解構行，釋放的是 swap 進 other 的
//   「舊資料」，所以印出的字串是被覆蓋掉的那一份，不是新值。
// * 第 1 段最後的自我移動賦值走的是別名（編譯器看不出來），
//   靠函式內 this == &other 的防護才安全；若拿掉防護，
//   先 delete[] 再從自己偷取就會讀到已釋放記憶體（未定義行為）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 1. 分開寫法 =====
//   [建構] "Dragon"
//   [建構] "Knight"
//
//   拷貝賦值（左值）：b = a
//   [拷貝賦值] "Dragon"
//
//   移動賦值（右值）：b = std::move(a)
//   [移動賦值] "Dragon"
//   a="(null)" b="Dragon"
//
//   移動賦值（臨時物件）：b = SeparateString("Phoenix")
//   [建構] "Phoenix"
//   [移動賦值] "Phoenix"
//   [解構] (nullptr)
//   b="Phoenix"
//
//   自我移動賦值（透過別名，編譯器看不出來）：
//   [移動賦值] 自我賦值
//   [解構] "Phoenix"
//   [解構] (nullptr)
//
// ===== 2. 鏈式移動賦值 =====
//   [建構] "Alpha"
//   [建構] "Beta"
//   [建構] "Gamma"
//   c = b = std::move(a):
//   [移動賦值] "Alpha"
//   [拷貝賦值] "Alpha"
//   a="(null)" b="Alpha" c="Alpha"
//   [解構] "Alpha"
//   [解構] "Alpha"
//   [解構] (nullptr)
//
// ===== 3. 統一賦值（推薦）=====
//   [建構] "Dragon"
//   [建構] "Knight"
//
//   拷貝路徑：b = a（左值 → 拷貝建構 other → swap）
//   [拷貝建構] "Dragon"
//   [統一賦值] swap
//   [解構] "Knight"
//
//   移動路徑：b = std::move(a)（右值 → 移動建構 other → swap）
//   [移動建構] "Dragon"
//   [統一賦值] swap
//   [解構] "Dragon"
//
//   臨時物件路徑：
//   [建構] "Phoenix"
//   [統一賦值] swap
//   [解構] "Dragon"
//   [解構] "Phoenix"
//   [解構] "null"
//
// ===== 4. 被移動物件可以重新賦值 =====
//   [建構] "First"
//   [建構] "Second"
//   [建構] "Third"
//   [移動賦值] "First"
//   a 被移動後："(null)"
//   [移動賦值] "Third"
//   a 重新賦值："Third"
//   [解構] (nullptr)
//   [解構] "First"
//   [解構] "Third"
//
// ===== 5. 日常實務：連線設定熱重載 =====
//   啟動時設定：dsn=host=db-old;port=5432 timeout=30
//
//   收到 SIGHUP，重新載入（移動賦值路徑）：
//     [設定-統一賦值] 舊設定將在此函式結束時釋放
//   重載後設定：dsn=host=db-new;port=6543 timeout=15
//
//   從既有物件複製一份設定（拷貝賦值路徑）：
//     [設定-拷貝建構]
//     [設定-統一賦值] 舊設定將在此函式結束時釋放
//   切換後設定：dsn=host=db-backup;port=5432 timeout=60
//   fallback 仍可用：dsn=host=db-backup;port=5432（拷貝不破壞來源）
//
// === 重點整理 ===
//   分開寫：operator=(const T&) + operator=(T&&)
//   統一寫（推薦）：operator=(T other) + swap
//   統一寫自動根據傳入左/右值選擇拷貝/移動路徑
//   鏈式賦值中 b = std::move(a) 回傳的 b 是左值
