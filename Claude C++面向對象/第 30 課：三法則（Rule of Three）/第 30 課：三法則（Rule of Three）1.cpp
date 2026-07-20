// =============================================================================
//  第 30 課 -1  —  三法則實作演練：從「必須自己寫」到「不必自己寫」
// =============================================================================
//
// 【主題資訊 Information】
//   三法則   ： ~T()、T(const T&)、T& operator=(const T&) —— 要嘛都不寫，
//              要嘛三個都要明確決定（自己實作，或明確 = delete）
//   標準版本 ： C++98 起；C++11 之後完整版是五法則（第 34 課）
//   標頭檔   ： <cstring>、<utility>、<vector>、<type_traits>
//   複雜度   ： 深拷貝 O(n)、swap O(1)
//   驗證方式 ： valgrind --leak-check=full ./lesson30
//   本檔重點 ： 除了示範三法則怎麼寫，更用「可重現的計數」量出
//              「只寫三法則、沒寫 move」在 std::vector 裡的真實代價。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個類別為什麼非寫三法則不可】
//   ManagedString 的成員是 char* m_data，指向一塊 new[] 出來的記憶體。
//   「擁有一塊需要親手釋放的資源」這件事同時決定了三件事：
//     * 解構時必須 delete[]        → 需要自訂解構函數
//     * 複製時必須配一塊新的       → 需要自訂拷貝建構（否則兩者共用一塊）
//     * 賦值時必須先處理掉舊的     → 需要自訂拷貝賦值
//   少寫任何一個，其餘兩個就會和它不一致，結果不是洩漏就是雙重釋放。
//
// 【2. 從輸出讀出「什麼時候發生了複製」】
//   本檔七個測試其實是在回答同一個問題：這一行程式碼會不會複製？
//     測試 2  ManagedString b = a;      → 拷貝建構（宣告伴隨初始化）
//     測試 3  b = a;                    → 拷貝賦值（b 已存在）
//     測試 4  a = a;                    → 仍然會複製一份（copy-and-swap 的代價）
//     測試 5  processByValue(a)         → 按值傳參，進函式前複製一份
//     測試 6  return local;             → NRVO 生效，看不到多餘複製
//     測試 7  c = b = a;                → 兩次拷貝賦值（右結合）
//   建議對照檔尾的預期輸出逐行看 —— 「哪些行有 [拷貝建構]」比任何說明都清楚。
//
// 【3. 三法則寫完了，故事還沒結束：move 不見了】
//   C++ 有一條容易被忽略的規則：只要類別「自訂了解構函數」，
//   編譯器就不會再隱式生成 move constructor 與 move assignment。
//   ManagedString 正是這種情況。後果是所有本來可以零成本搬移的場合
//   （回傳暫時物件、std::vector 擴容、std::sort 搬移元素）
//   全部靜默退化成完整的深拷貝，而且不會有任何警告。
//
//   本檔用 demoRuleOfZeroCost() 把這件事「數」出來（不是計時，是數次數，
//   所以每次執行結果都一樣）。本機實測：
//       std::is_nothrow_move_constructible<ManagedString> = false
//       std::is_nothrow_move_constructible<ZeroString>    = true
//   往 std::vector 連續 push_back 5 次（沒有先 reserve）：
//       size=1 capacity=1  累計配置 2 塊
//       size=2 capacity=2  累計配置 5 塊
//       size=3 capacity=4  累計配置 9 塊
//       size=4 capacity=4  累計配置 11 塊
//       size=5 capacity=8  累計配置 17 塊
//   總共 17 塊記憶體，而真正需要的只有 5 塊。多出來的 12 塊分別來自：
//   每次 push_back 的暫時物件、以及每次容量成長時把舊元素「複製」過去。
//   先呼叫 reserve(5) 之後，同樣的迴圈只配置 10 塊。
//   （容量成長為 1→2→4→8，也就是 2 倍成長。這是 libstdc++ 的實作選擇，
//     標準只要求 push_back 的攤還複雜度為 O(1)，並未規定倍率。）
//
// 【4. Rule of Zero：最好的三法則就是不必寫三法則】
//   把 char* 換成 std::string，整個類別一行特殊成員函式都不用寫：
//       class ZeroString { std::string m_data; ... };
//   深拷貝、賦值、釋放全部由 std::string 負責，而且它「沒有」自訂解構函數
//   的問題，所以 move 操作照樣被隱式生成 ——
//   上面那個 is_nothrow_move_constructible = true 就是證據。
//   結論很直接：正式專案的預設做法是 Rule of Zero；
//   手寫三法則只用在「真的要包裝一個 C API handle」這種場合。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼用 is_nothrow_move_constructible 而不是 is_move_constructible
//   std::is_move_constructible<ManagedString> 其實是 true —— 因為
//   拷貝建構函數 T(const T&) 也能接受右值實參，「可以用右值建構」這件事
//   成立。所以它完全無法區分「真的有 move」和「只是退化成 copy」。
//   is_nothrow_move_constructible 才有鑑別力：真正的 move 通常是 noexcept，
//   而深拷貝要配置記憶體、可能丟 std::bad_alloc，不會是 noexcept。
//   std::vector 內部正是用 std::move_if_noexcept 做這個判斷 ——
//   move 不是 noexcept 就退回 copy，以保住強例外保證。
//
// (B) 為什麼 push_back 5 次會配置到 17 塊
//   每次 push_back(ManagedString("item")) 至少 2 塊：暫時物件 1 塊、
//   複製進 vector 1 塊。再加上容量從 1→2→4→8 期間，
//   每次重新配置都要把既有元素「深拷貝」到新緩衝區。
//   把數字攤開來看，就會明白為什麼「知道大小就先 reserve」是基本功。
//
// (C) 教學用序號 vs 原始位址
//   本檔不印 static_cast<void*>(m_data)。位址受 ASLR 影響，每次執行都不同
//   （本機實測連跑三次得到三組不同位址），寫進預期輸出就是假資料。
//   改用「配置序號」：每 new 一塊就給一個遞增編號，swap 時序號跟著緩衝區走。
//   要判斷「是不是同一塊」時，直接印指標比較的布林值即可。
//
// (D) s_allocSerial 是 static 成員，不佔物件空間
//   它屬於類別而非實例，所有物件共用同一個計數器。這也是為什麼
//   allocCount() 可以拿來當「全域配置次數」的量測工具。
//
// 【注意事項 Pay Attention】
//   1. 自訂解構函數 = 放棄隱式 move。這是本課最容易被忽略的副作用，
//      代價是效能靜默退化，不會有任何警告。
//   2. 判斷「有沒有真的 move」不要用 is_move_constructible（永遠是 true），
//      要用 is_nothrow_move_constructible 或直接在 move 函式裡印一行字。
//   3. std::vector 的成長倍率是實作定義的（libstdc++ 為 2 倍），
//      標準只保證 push_back 的攤還複雜度 O(1)。
//   4. 已知元素數量就先 reserve()：本機實測可把 17 次配置降到 10 次。
//   5. 手寫三法則是為了理解機制。正式專案請優先 Rule of Zero，
//      把資源交給 std::string／std::vector／std::unique_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三法則的代價與 Rule of Zero
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 自訂了解構函數，對 move 操作有什麼影響？
//     答：編譯器不會再隱式生成 move constructor 與 move assignment。
//         所有搬移場合都會退化成拷貝，而且完全沒有警告。
//         本檔實測：is_nothrow_move_constructible<ManagedString> 是 false，
//         換成 Rule of Zero 版本（只有一個 std::string 成員）就是 true。
//     追問：那要怎麼把 move 拿回來？
//         → 明確補上 move constructor 與 move assignment（五法則，第 34 課），
//           或直接改用 Rule of Zero 讓編譯器全部自動生成。
//
// 🔥 Q2. 什麼是 Rule of Zero？它為什麼優於自己寫三法則？
//     答：讓類別完全不自訂任何特殊成員函式，把資源管理委託給已經寫對的型別
//         （std::string、std::vector、std::unique_ptr）。優點有三：
//         程式碼更少所以更不會錯、自動具備例外安全、
//         而且因為沒有自訂解構函數，move 操作會被隱式生成。
//     追問：那什麼時候非得自己寫不可？
//         → 包裝 C API 的 handle（FILE*、socket fd、HANDLE）時。
//           但即使那樣，通常也該包成 unique_ptr + 自訂 deleter，
//           或明確 = delete 拷貝操作，而不是實作深拷貝。
//
// ⚠️ 陷阱 1. 「用 std::is_move_constructible 就能檢查類別有沒有 move」——不能。
//     答：它對 ManagedString 也回傳 true，因為 T(const T&) 同樣能接受右值。
//         這個 trait 問的是「能不能用右值建構」，不是「會不會走 move」。
//         有鑑別力的是 is_nothrow_move_constructible：
//         真 move 通常 noexcept，深拷貝要配置記憶體所以不是。
//     為什麼會錯：把 trait 的名字當成語意。C++ 的 traits 問的往往是
//         「這個運算式合不合法」，而不是「它會走哪一條路」。
//
// ⚠️ 陷阱 2. 「vector 擴容只是搬指標，成本不高」——對沒有 move 的型別完全不成立。
//     答：vector 用 std::move_if_noexcept 決定要搬還是要複製。
//         move 不是 noexcept（或根本沒有 move）時，它會「複製」每一個元素
//         以維持強例外保證。本檔實測 push_back 5 次共配置 17 塊記憶體，
//         而真正需要的只有 5 塊。
//     為什麼會錯：把 vector 想成「只搬 sizeof(T) 個位元組」。
//         那只在 trivially copyable 型別上成立；擁有資源的型別每搬一次
//         就是一次完整的深拷貝。
//
// ⚠️ 陷阱 3. 「a = a 自我賦值，copy-and-swap 會直接跳過」——不會，它照樣複製。
//     答：copy-and-swap 讓自我賦值「安全」，但不讓它「免費」。參數是按值
//         傳入的，所以仍然會實際配置並複製一份，再和自己交換。
//         本檔測試 4 的輸出可以看到那次 [拷貝建構] 確實發生了。
//     為什麼會錯：把「正確」和「有最佳化」混為一談。
//         若自我賦值在熱路徑很頻繁，才值得額外加一行 if (this == &other) 短路。
// ═══════════════════════════════════════════════════════════════════════════

// lesson30_rule_of_three.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson30 lesson30_rule_of_three.cpp
// 驗證：valgrind --leak-check=full ./lesson30

#include <iostream>
#include <cstring>
#include <utility>

class ManagedString {
private:
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在前。
    std::size_t m_len;
    char* m_data;
    // 教學用：每配置一塊新緩衝區就給它一個遞增的「配置序號」。
    // 不印原始位址，是因為位址受 ASLR 影響、每次執行都不同（實測連跑
    // 三次得到三組不同位址），無法寫成可重現的預期輸出。
    unsigned m_tag;
    static unsigned s_allocSerial;
    // 計數示範時把日誌關掉，否則十幾行建構／解構訊息會蓋掉重點
    static bool s_quiet;

public:
    static unsigned allocCount() { return s_allocSerial; }
    static void setQuiet(bool q) { s_quiet = q; }

    // ──────── 建構函數 ────────
    ManagedString(const char* str = "")
        : m_len(std::strlen(str))
        , m_data(new char[m_len + 1])
        , m_tag(++s_allocSerial)
    {
        std::strcpy(m_data, str);
        if (!s_quiet)
            std::cout << "  [建構] \"" << m_data << "\" (緩衝區 #" << m_tag << ")\n";
    }

    // ════════════════════════════════════════
    // ★ Rule of Three：以下三個必須同時存在 ★
    // ════════════════════════════════════════

    // ──────── 1. 解構函數 ────────
    ~ManagedString() {
        if (!s_quiet)
            std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr")
                      << "\" (緩衝區 #" << m_tag << ")\n";
        delete[] m_data;
    }

    // ──────── 2. 拷貝建構函數 ────────
    ManagedString(const ManagedString& other)
        : m_len(other.m_len)
        , m_data(new char[m_len + 1])
        , m_tag(++s_allocSerial)
    {
        std::strcpy(m_data, other.m_data);
        if (!s_quiet)
            std::cout << "  [拷貝建構] \"" << m_data << "\" (緩衝區 #" << m_tag
                      << " ← 複製自 #" << other.m_tag << ")\n";
    }

    // ──────── 3. 拷貝賦值運算子（Copy-and-Swap）────────
    ManagedString& operator=(ManagedString other) {
        if (!s_quiet) std::cout << "  [拷貝賦值] swap\n";
        swap(other);
        return *this;
    }

    // ──────── swap（配合 Copy-and-Swap）────────
    void swap(ManagedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
        std::swap(m_tag,  other.m_tag);   // 序號跟著緩衝區走
    }

    // ──────── 存取 ────────
    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }
};

// 靜態成員定義（教學用的配置序號產生器）
unsigned ManagedString::s_allocSerial = 0;
bool     ManagedString::s_quiet       = false;

// 非成員 swap
inline void swap(ManagedString& a, ManagedString& b) noexcept {
    a.swap(b);
}

// ──────── 測試函數：按值傳入（觸發拷貝建構）────────
void processByValue(ManagedString s) {
    std::cout << "    processByValue: \"" << s.c_str() << "\"\n";
}

// ──────── 測試函數：按值返回 ────────
ManagedString createString(const char* text) {
    ManagedString local(text);
    return local;
}

#include <vector>
#include <string>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【日常實務範例】Rule of Zero：把資源交給已經寫對的型別
//   情境：同樣是「持有一段文字」的類別，一個自己管 char*（三法則），
//         一個直接用 std::string（零法則）。實務上你幾乎永遠該選後者。
//   為什麼用到本主題：Rule of Zero 不只是「少寫程式碼」——
//         因為它沒有自訂解構函數，編譯器會繼續替它生成 move 操作，
//         而手寫三法則的版本則會失去 move。下面用可重現的計數量給你看。
// -----------------------------------------------------------------------------
class ZeroString {
private:
    std::string m_data;   // 由 std::string 負責配置、複製、釋放
public:
    ZeroString(const char* s = "") : m_data(s) {}
    const char* c_str()  const { return m_data.c_str(); }
    std::size_t length() const { return m_data.size(); }
    // 解構、拷貝建構、拷貝賦值、move 建構、move 賦值 —— 一個都不用寫
};

void demoRuleOfZeroCost() {
    std::cout << std::boolalpha;

    // ── 1. 有沒有「真的 move」──────────────────────────────────
    // 注意：is_move_constructible 對兩者都是 true（拷貝建構也能接右值），
    //       所以它無法區分。要用 is_nothrow_move_constructible 才有鑑別力。
    std::cout << "  is_move_constructible<ManagedString>         = "
              << std::is_move_constructible<ManagedString>::value
              << "  ← 無鑑別力\n";
    std::cout << "  is_nothrow_move_constructible<ManagedString> = "
              << std::is_nothrow_move_constructible<ManagedString>::value
              << "  ← 自訂解構函數 → 沒有隱式 move\n";
    std::cout << "  is_nothrow_move_constructible<ZeroString>    = "
              << std::is_nothrow_move_constructible<ZeroString>::value
              << "  ← Rule of Zero → move 自動生成\n";

    // ── 2. 沒有 move 的型別放進 vector，要付多少代價 ────────────
    // 量的是「配置次數」而不是耗時：次數由程式邏輯決定，每次執行都一樣。
    ManagedString::setQuiet(true);          // 暫時關掉建構／解構日誌
    {
        unsigned before = ManagedString::allocCount();
        std::vector<ManagedString> v;
        std::cout << "\n  push_back 5 次（未 reserve）:\n";
        for (int i = 0; i < 5; ++i) {
            v.push_back(ManagedString("item"));
            std::cout << "    size=" << v.size()
                      << " capacity=" << v.capacity()
                      << " 累計配置=" << (ManagedString::allocCount() - before)
                      << " 塊\n";
        }
        unsigned noReserve = ManagedString::allocCount() - before;

        before = ManagedString::allocCount();
        std::vector<ManagedString> w;
        w.reserve(5);                        // ★ 先講好容量
        for (int i = 0; i < 5; ++i) w.push_back(ManagedString("item"));
        unsigned withReserve = ManagedString::allocCount() - before;

        std::cout << "  未 reserve 共配置 " << noReserve << " 塊；"
                  << "先 reserve(5) 只需 " << withReserve << " 塊"
                  << "（真正需要的其實只有 5 塊）\n";
    }
    ManagedString::setQuiet(false);         // 恢復日誌

    std::cout << "  註：capacity 成長為 1→2→4→8（2 倍）是 libstdc++ 的實作選擇，\n";
    std::cout << "      標準只保證 push_back 的攤還複雜度為 O(1)，未規定倍率。\n";
}
int main() {
    std::cout << "===== 測試 1：基本生命週期 =====\n";
    {
        ManagedString s("Hello");
        std::cout << "  s = \"" << s.c_str() << "\"\n";
    }  // s 離開作用域，解構
    std::cout << "\n";

    std::cout << "===== 測試 2：拷貝建構 =====\n";
    {
        ManagedString a("Dragon");
        ManagedString b = a;   // 拷貝建構
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
        // 不印原始位址（每次執行都不同、無法重現），改印「是不是同一塊」
        std::cout << "  a 與 b 共用同一塊緩衝區？ "
                  << std::boolalpha << (a.c_str() == b.c_str()) << "\n";
        std::cout << "  （false → 兩者各自持有一塊 → 深拷貝成功）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 3：拷貝賦值 =====\n";
    {
        ManagedString a("Knight");
        ManagedString b("Wizard");
        std::cout << "  賦值前：a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
        b = a;
        std::cout << "  賦值後：a=\"" << a.c_str() << "\"  b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 4：自我賦值 =====\n";
    {
        ManagedString a("Phoenix");
        a = a;  // Copy-and-Swap 自動處理
        std::cout << "  a=\"" << a.c_str() << "\"（安全）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 5：按值傳參（觸發拷貝建構）=====\n";
    {
        ManagedString a("Rogue");
        processByValue(a);  // 拷貝建構 → 函數結束時解構副本
        std::cout << "  函數返回後 a=\"" << a.c_str() << "\"（不受影響）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：按值返回 =====\n";
    {
        ManagedString result = createString("Summoned");
        std::cout << "  result=\"" << result.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 7：鏈式賦值 =====\n";
    {
        ManagedString a("A"), b("B"), c("C");
        c = b = a;
        std::cout << "  a=\"" << a.c_str() << "\"  b=\"" << b.c_str()
                  << "\"  c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 8：Rule of Zero 與「沒有 move」的代價 =====\n";
    demoRuleOfZeroCost();
    std::cout << "\n";
    std::cout << "===== 所有測試通過 =====\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 30 課：三法則（Rule of Three）1.cpp" -o lesson30

// 【本檔對原始版本做的修改，以及為什麼】
//   原版在建構／解構訊息中印出 static_cast<void*>(m_data)（原始位址）。
//   位址受 ASLR 影響，每次執行都不同（本機實測連跑三次得到三組不同位址），
//   無法寫成可重現的預期輸出。本檔改為「配置序號」（#1、#2…），
//   swap 時序號跟著緩衝區走；判斷「是否共用同一塊」則改印指標比較的布林值。
//   另外新增 setQuiet()，讓測試 8 的計數示範不被十幾行日誌淹沒。
//
// 【測試 8 的數字為何可信】
//   量的是「配置次數」而不是耗時 —— 次數由程式邏輯決定，重跑幾次都相同
//   （本機實測連跑三次輸出完全一致）。耗時則會受機器負載影響，
//   不適合寫進教材。
//   其中 capacity 1→2→4→8 的 2 倍成長是 libstdc++ 的實作選擇，
//   標準只要求 push_back 的攤還複雜度為 O(1)，並未規定倍率；
//   換一個標準函式庫實作（例如 MSVC 用 1.5 倍）數字就會不同。
//
// 【記憶體驗證（本機實測）】
//   valgrind --leak-check=full ./lesson30
//       All heap blocks were freed -- no leaks are possible
//       ERROR SUMMARY: 0 errors from 0 contexts
//
// 【為何本檔沒有 LeetCode 範例】
//   本檔聚焦在「三法則的實作與代價量測」，屬於型別設計而非演算法。
//   本課真正對應 LeetCode 的部分已放在同課的 summary.cpp
//   （LeetCode 705. Design HashSet —— 該題的 bucket 陣列與節點鏈
//   正是需要三法則的真實案例），此處不重複掛題以免製造假關聯。

// === 預期輸出 ===
// ===== 測試 1：基本生命週期 =====
//   [建構] "Hello" (緩衝區 #1)
//   s = "Hello"
//   [解構] "Hello" (緩衝區 #1)
//
// ===== 測試 2：拷貝建構 =====
//   [建構] "Dragon" (緩衝區 #2)
//   [拷貝建構] "Dragon" (緩衝區 #3 ← 複製自 #2)
//   a="Dragon"  b="Dragon"
//   a 與 b 共用同一塊緩衝區？ false
//   （false → 兩者各自持有一塊 → 深拷貝成功）
//   [解構] "Dragon" (緩衝區 #3)
//   [解構] "Dragon" (緩衝區 #2)
//
// ===== 測試 3：拷貝賦值 =====
//   [建構] "Knight" (緩衝區 #4)
//   [建構] "Wizard" (緩衝區 #5)
//   賦值前：a="Knight"  b="Wizard"
//   [拷貝建構] "Knight" (緩衝區 #6 ← 複製自 #4)
//   [拷貝賦值] swap
//   [解構] "Wizard" (緩衝區 #5)
//   賦值後：a="Knight"  b="Knight"
//   [解構] "Knight" (緩衝區 #6)
//   [解構] "Knight" (緩衝區 #4)
//
// ===== 測試 4：自我賦值 =====
//   [建構] "Phoenix" (緩衝區 #7)
//   [拷貝建構] "Phoenix" (緩衝區 #8 ← 複製自 #7)
//   [拷貝賦值] swap
//   [解構] "Phoenix" (緩衝區 #7)
//   a="Phoenix"（安全）
//   [解構] "Phoenix" (緩衝區 #8)
//
// ===== 測試 5：按值傳參（觸發拷貝建構）=====
//   [建構] "Rogue" (緩衝區 #9)
//   [拷貝建構] "Rogue" (緩衝區 #10 ← 複製自 #9)
//     processByValue: "Rogue"
//   [解構] "Rogue" (緩衝區 #10)
//   函數返回後 a="Rogue"（不受影響）
//   [解構] "Rogue" (緩衝區 #9)
//
// ===== 測試 6：按值返回 =====
//   [建構] "Summoned" (緩衝區 #11)
//   result="Summoned"
//   [解構] "Summoned" (緩衝區 #11)
//
// ===== 測試 7：鏈式賦值 =====
//   [建構] "A" (緩衝區 #12)
//   [建構] "B" (緩衝區 #13)
//   [建構] "C" (緩衝區 #14)
//   [拷貝建構] "A" (緩衝區 #15 ← 複製自 #12)
//   [拷貝賦值] swap
//   [拷貝建構] "A" (緩衝區 #16 ← 複製自 #15)
//   [拷貝賦值] swap
//   [解構] "C" (緩衝區 #14)
//   [解構] "B" (緩衝區 #13)
//   a="A"  b="A"  c="A"
//   [解構] "A" (緩衝區 #16)
//   [解構] "A" (緩衝區 #15)
//   [解構] "A" (緩衝區 #12)
//
// ===== 測試 8：Rule of Zero 與「沒有 move」的代價 =====
//   is_move_constructible<ManagedString>         = true  ← 無鑑別力
//   is_nothrow_move_constructible<ManagedString> = false  ← 自訂解構函數 → 沒有隱式 move
//   is_nothrow_move_constructible<ZeroString>    = true  ← Rule of Zero → move 自動生成
//
//   push_back 5 次（未 reserve）:
//     size=1 capacity=1 累計配置=2 塊
//     size=2 capacity=2 累計配置=5 塊
//     size=3 capacity=4 累計配置=9 塊
//     size=4 capacity=4 累計配置=11 塊
//     size=5 capacity=8 累計配置=17 塊
//   未 reserve 共配置 17 塊；先 reserve(5) 只需 10 塊（真正需要的其實只有 5 塊）
//   註：capacity 成長為 1→2→4→8（2 倍）是 libstdc++ 的實作選擇，
//       標準只保證 push_back 的攤還複雜度為 O(1)，未規定倍率。
//
// ===== 所有測試通過 =====
