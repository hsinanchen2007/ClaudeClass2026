// =============================================================================
//  第 32 課 總結  —  移動建構函數（Move Constructor）
// =============================================================================
//
// 【主題資訊 Information】
//   簽名       ： ClassName(ClassName&& other) noexcept;
//   移動三步驟 ： ① 偷取指標   m_data = other.m_data;   （不配置新記憶體）
//                 ② 歸零來源   other.m_data = nullptr;  （避免 double free）
//                 ③ 複製基本型別 m_len = other.m_len;
//   標準版本   ： C++11
//   標頭檔     ： <utility>、<vector>、<string>、<type_traits>
//   複雜度     ： 移動 O(1)（只搬指標）；拷貝 O(n)（配置 + 複製）
//   noexcept   ： 必須標，否則 vector 擴容時不會採用（見下方第 3 點）
//
// 【詳細解釋 Explanation】
//
// 【1. 移動建構函數解決的問題：來源不再需要時，別複製、直接接手】
//   拷貝建構要「配置新記憶體 + 逐位元組複製」，成本 O(n)。
//   但當來源是暫時物件、或呼叫端用 std::move 明確放棄它時，複製是浪費。
//   移動建構直接把指標接過來（O(1)），並把來源置空。
//   本檔的 SimpleString 就是完整示範：拷貝印 💰、移動印 ⚡。
//
// 【2. 移動後的來源：valid but unspecified】
//   移動後 other 處於「有效但未指定」狀態：
//       * 可以安全解構（本檔把 m_data 設 nullptr，delete[] nullptr 是空操作）
//       * 可以被重新賦值（示範 3 驗證這件事）
//       * 但不該讀它的內容 —— 標準不保證內容是什麼
//   本檔能安全印出 "(null)" 是因為那是我們「自己歸零的成員」，
//   不是去讀未指定的內容。若成員換成 std::string，被移動後就不可以直接印。
//
// 【3. noexcept 為什麼是關鍵（示範 4）】
//   std::vector 擴容時用 std::move_if_noexcept 決定怎麼搬既有元素：
//       移動建構是 noexcept → 用移動（快，且保證不會半途失敗）
//       移動建構可能拋例外 → 改用拷貝（慢，但失敗時舊資料還在、可回復）
//   所以移動建構不標 noexcept，vector 根本不會採用它，等於白寫。
//   示範 4 中 Widget 的移動建構有 noexcept，擴容時 Alpha/Beta 走的是移動。
//
// 【4. 自動生成規則（示範 5）】
//   只要自訂了「解構函數 / 拷貝建構 / 拷貝賦值」任一項，
//   編譯器就不再自動生成移動建構與移動賦值。
//   NoAutoMove 只加了一個空解構函數，移動建構就消失了 ——
//   但 is_move_constructible 仍是 true（拷貝建構可接住右值），
//   要看 is_nothrow_move_constructible 才知道它其實走的是拷貝（false）。
//   這正是需要 Rule of Five 的原因（見第 34 課）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 示範 2「從臨時物件」為什麼看不到移動建構
//   SimpleString a = SimpleString("Phoenix Staff"); 在 C++17 只印一次 [建構]。
//   因為右邊是純右值（prvalue），C++17 保證省略這種初始化 ——
//   直接就地建構在 a 的位置上，從頭到尾只有一個物件，沒有搬移。
//   本機實測對照（用等價的最小範例）：
//       C++17                            → 只有 [建構]
//       C++14 + -fno-elide-constructors  → [建構] 之後才出現 [移動建構]
//   後者才是「暫時物件先造、再搬進 a」的舊模型。
//   （對照第 28 課 -2：C++17 只保證 prvalue 初始化的省略，NRVO 仍是可選的。）
//   真正示範移動的是示範 1（具名變數 + std::move）與示範 3。
//
// (B) ★ 效能比較為什麼用「配置次數」而不是「計時」（示範 6）
//   原始版本用 std::chrono 量兩個迴圈的毫秒數並印到 stdout。
//   問題是牆鐘時間受 CPU 排程、快取、其他行程影響，每次執行都不同，
//   無法寫成可對照的預期輸出（教材的預期輸出必須可重現）。
//   本檔改量「記憶體配置次數」這個決定性指標：
//       深拷貝 N 個元素 → N 次配置（每個都要 new 一塊新記憶體）
//       移動   N 個元素 → 0 次配置（只搬指標）
//   0 vs N 是決定性的，而且直接對應 O(n) vs O(1) 的本質。
//   想看實際速度的人，本檔仍量了牆鐘時間，但把它印到 stderr
//   並註明「每次執行都不同」——這樣 stdout 保持逐位元組穩定。
//
// (C) 成員宣告順序（-Wreorder）
//   SimpleString 的 m_data 初始值用到 m_len（new char[m_len + 1]），
//   所以 m_len 宣告在前 —— 檔案中已是正確順序。
//   對調會讓 m_data 用到尚未初始化的 m_len，配置錯誤大小的緩衝區。
//
// (D) SimpleString 的賦值是統一賦值（Copy-and-Swap）
//   operator=(SimpleString other) 傳值 + swap，一個函式同時處理拷貝與移動；
//   傳左值時參數由拷貝建構、傳右值時由移動建構。細節見第 33 課 -2。
//
// 【注意事項 Pay Attention】
//   1. 移動建構函數務必標 noexcept，否則 vector 擴容時退回拷貝。
//   2. 一定要把來源置空，否則兩個物件持有同一塊記憶體 → double free。
//   3. 移動後的物件是 valid but unspecified：可重新賦值、可解構，不該讀內容。
//      能安全印 "(null)" 是因為那是自己歸零的成員；std::string 不可這樣印。
//   4. C++17 起用暫時物件初始化看不到移動建構是正常的（保證省略），見【概念補充 A】。
//   5. 教材的效能比較用「配置次數」而非計時；計時要送 stderr 並註明每次不同。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 移動建構和拷貝建構的成本差在哪？為什麼移動建構要標 noexcept？
//     答：拷貝要配置新記憶體並逐位元組複製，O(n)；
//         移動只複製指標值再把來源置空，O(1)，與資料量無關。
//         標 noexcept 是因為 std::vector 擴容時用 move_if_noexcept：
//         移動保證不拋才會被採用，否則 vector 為了強例外保證改用拷貝。
//         不標 noexcept，你的移動建構等於白寫。
//     追問：移動後來源的 size() 一定是 0 嗎？
//         → 標準沒這樣要求。本檔自己把成員歸零所以是 0，
//           但那是實作細節；標準庫型別被移動後的內容沒有保證。
//
// 🔥 Q2. 為什麼只加一個空的解構函數，移動建構就消失了？
//     答：只要自訂了解構、拷貝建構、拷貝賦值任一項，
//         編譯器就不再自動生成移動建構與移動賦值。
//         NoAutoMove 的 ~NoAutoMove() {} 雖然是空的，也算「宣告」，
//         足以讓移動建構消失，之後所有「移動」都安靜退回深拷貝。
//     追問：那 is_move_constructible 會變 false 嗎？
//         → 不會，仍是 true —— 因為 const T& 的拷貝建構可以接住右值。
//           要看 is_nothrow_move_constructible 才會露出馬腳（變 false），
//           因為它實際走的是可能拋 bad_alloc 的拷貝。
//
// ⚠️ 陷阱. 「要比較移動和拷貝的效能，就在程式裡量兩段迴圈的毫秒數印出來。」
//     答：印牆鐘時間到 stdout 是教材的大忌 —— 它每次執行都不同
//         （受 CPU 排程、快取、其他行程影響），無法寫成可重現的預期輸出。
//         正確做法是量「決定性的指標」：例如記憶體配置次數。
//         深拷貝 N 個元素配置 N 次、移動配置 0 次，這個 0 vs N 是穩定的，
//         而且直接對應 O(n) vs O(1)。真要量時間就送 stderr 並註明每次不同。
//     為什麼會錯：把「示範效能差異」等同於「印出耗時」。
//         效能差異的本質是「做了多少工作」（配置、複製的次數），
//         而不是「花了多少時間」——後者是前者加上一堆雜訊的結果。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   移動建構函數解決的是資源轉移的成本，屬於資源管理與效能議題。
//   LeetCode 只驗證演算法的輸入輸出與整體時間限制，
//   評測系統既不會拷貝也不會移動你的物件，
//   無從觀察走的是 O(1) 的移動還是 O(n) 的拷貝。
//   硬掛設計類題號只會誤導，故從缺；
//   本檔以六個示範（含 noexcept 對 vector 的影響、配置次數比較）完整覆蓋主題。
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <string>
#include <type_traits>
#include <chrono>

// ============================================================
// SimpleString：移動建構函數完整示範
// ============================================================
class SimpleString {
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在前。
    std::size_t m_len;
    char* m_data;
public:
    // 一般建構
    SimpleString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\"\n";
    }

    // 拷貝建構（深拷貝）— 昂貴 💰
    SimpleString(const SimpleString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構💰] \"" << m_data << "\"\n";
    }

    // ★ 移動建構 — 便宜 ⚡
    SimpleString(SimpleString&& other) noexcept   // ← noexcept！
        : m_len(other.m_len)                       // ③ 複製基本型別（順序依宣告順序）
        , m_data(other.m_data)                     // ① 偷取指標
    {
        other.m_data = nullptr;                    // ② 歸零源物件
        other.m_len = 0;
        std::cout << "  [移動建構⚡] \"" << m_data << "\"\n";
    }

    // 賦值（Copy-and-Swap，統一處理拷貝與移動，見第 33 課 -2）
    SimpleString& operator=(SimpleString other) {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
        return *this;
    }

    // 解構
    ~SimpleString() {
        if (m_data) std::cout << "  [解構] \"" << m_data << "\"\n";
        else        std::cout << "  [解構] (nullptr，已被移動)\n";
        delete[] m_data;  // delete[] nullptr 安全
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
    bool empty() const { return !m_data || m_len == 0; }
};

// ============================================================
// noexcept 對 vector 的影響
// ============================================================
class Widget {
    char* m_name;
public:
    Widget(const char* name) : m_name(new char[std::strlen(name) + 1]) {
        std::strcpy(m_name, name);
    }
    Widget(const Widget& other) : m_name(new char[std::strlen(other.m_name) + 1]) {
        std::strcpy(m_name, other.m_name);
        std::cout << "    [Widget 拷貝] \"" << m_name << "\"\n";
    }
    Widget(Widget&& other) noexcept : m_name(other.m_name) {
        other.m_name = nullptr;
        std::cout << "    [Widget 移動] \"" << m_name << "\"\n";
    }
    Widget& operator=(Widget other) { std::swap(m_name, other.m_name); return *this; }
    ~Widget() { delete[] m_name; }
};

// ============================================================
// 自動生成規則示範
// ============================================================
class AutoMove {
    std::string m_name;
public:
    AutoMove(std::string name) : m_name(std::move(name)) {}
    // 什麼都不寫 → 編譯器自動生成移動建構 ✅
};

class NoAutoMove {
    std::string m_name;
public:
    NoAutoMove(std::string name) : m_name(std::move(name)) {}
    ~NoAutoMove() {}  // 自訂解構 → 移動建構不自動生成 ❌
    // 仍然「可移動建構」但退回使用拷貝建構（const T&）
};

// ============================================================
// 效能比較用：計數型 Blob（拷貝配置記憶體、移動不配置）
//   用「配置次數」而非「計時」當證據，理由見檔頭【概念補充 B】。
// ============================================================
namespace perf {

long g_allocations = 0;   // 深拷貝時的 new 次數
long g_moves       = 0;   // 移動時只搬指標，不配置

class Blob {
    char* m_data;
    std::size_t m_len;
public:
    explicit Blob(std::size_t n) : m_data(new char[n]), m_len(n) {
        std::memset(m_data, 'A', n);
        ++g_allocations;   // 一般建構也配置了一次
    }
    Blob(const Blob& o) : m_data(new char[o.m_len]), m_len(o.m_len) {
        std::memcpy(m_data, o.m_data, m_len);
        ++g_allocations;   // ★ 深拷貝：每次都配置一塊新記憶體
    }
    Blob(Blob&& o) noexcept : m_data(o.m_data), m_len(o.m_len) {
        o.m_data = nullptr; o.m_len = 0;
        ++g_moves;         // ★ 移動：只搬指標，不配置
    }
    ~Blob() { delete[] m_data; }
};

}  // namespace perf

int main() {
    // ============================================================
    // 1. 基本移動建構
    // ============================================================
    std::cout << "===== 1. 基本移動建構 =====\n";
    {
        SimpleString a("Dragon Sword");
        std::cout << "  拷貝建構（左值）：\n";
        SimpleString b = a;           // a 是左值 → 拷貝建構
        (void)b;

        std::cout << "  移動建構（std::move）：\n";
        SimpleString c = std::move(a); // 右值 → 移動建構
        std::cout << "  a = \"" << a.c_str() << "\" (已被移動)\n";
        std::cout << "  c = \"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 從臨時物件初始化（C++17 保證省略，看不到移動建構）
    // ============================================================
    std::cout << "===== 2. 從臨時物件移動 =====\n";
    {
        SimpleString a = SimpleString("Phoenix Staff");   // 純右值 → 保證省略
        std::cout << "  a = \"" << a.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 3. 移動後可以重新賦值
    // ============================================================
    std::cout << "===== 3. 移動後重新賦值 =====\n";
    {
        SimpleString a("First");
        SimpleString b = std::move(a);  // a 被掏空
        (void)b;
        std::cout << "  a = \"" << a.c_str() << "\" (已被移動)\n";
        a = SimpleString("Reborn");     // 重新賦值（valid but unspecified → 賦值安全）
        std::cout << "  a = \"" << a.c_str() << "\" (重生了)\n";
    }
    std::cout << "\n";

    // ============================================================
    // 4. noexcept 對 vector 擴容的影響
    // ============================================================
    std::cout << "===== 4. noexcept 對 vector 的影響 =====\n";
    {
        std::vector<Widget> vec;
        vec.reserve(2);  // 預留 2 個空間

        std::cout << "  push_back #1:\n";
        vec.push_back(Widget("Alpha"));

        std::cout << "  push_back #2:\n";
        vec.push_back(Widget("Beta"));

        std::cout << "  push_back #3（觸發擴容，搬移既有元素）:\n";
        vec.push_back(Widget("Gamma"));
        // 因為 Widget 的移動建構有 noexcept → 用移動搬 Alpha 和 Beta
        // 若沒有 noexcept → 會用拷貝（慢）
    }
    std::cout << "\n";

    // ============================================================
    // 5. 自動生成規則
    // ============================================================
    std::cout << "===== 5. 自動生成規則 =====\n";
    std::cout << std::boolalpha;

    std::cout << "  AutoMove（什麼都沒寫）:\n";
    std::cout << "    可移動建構？ " << std::is_move_constructible_v<AutoMove> << "\n";
    std::cout << "    nothrow？   " << std::is_nothrow_move_constructible_v<AutoMove> << "\n";

    std::cout << "  NoAutoMove（自訂了解構函數）:\n";
    std::cout << "    可移動建構？ " << std::is_move_constructible_v<NoAutoMove> << "\n";
    std::cout << "    nothrow？   " << std::is_nothrow_move_constructible_v<NoAutoMove> << "\n";
    // NoAutoMove 仍「可移動建構」但退回拷貝，沒有真正的移動建構
    std::cout << "\n";

    // ============================================================
    // 6. 效能比較：用「配置次數」當決定性證據（計時另送 stderr）
    // ============================================================
    std::cout << "===== 6. 效能比較（配置次數）=====\n";
    {
        const int N = 200000;

        std::vector<perf::Blob> source;
        source.reserve(N);
        for (int i = 0; i < N; ++i) source.emplace_back(500);   // 建立 N 個 Blob

        // 拷貝：每個元素都深拷貝 → 每次都 new 一塊新記憶體
        perf::g_allocations = 0;
        perf::g_moves = 0;
        auto t1 = std::chrono::high_resolution_clock::now();
        {
            std::vector<perf::Blob> copied;
            copied.reserve(N);
            for (const auto& b : source) copied.push_back(b);   // 拷貝建構
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        long copyAllocs = perf::g_allocations;

        // 移動：只搬指標 → 0 次配置
        perf::g_allocations = 0;
        perf::g_moves = 0;
        auto t3 = std::chrono::high_resolution_clock::now();
        {
            std::vector<perf::Blob> moved;
            moved.reserve(N);
            for (auto& b : source) moved.push_back(std::move(b));  // 移動建構
        }
        auto t4 = std::chrono::high_resolution_clock::now();
        long moveAllocs = perf::g_allocations;
        long moveCount  = perf::g_moves;

        // ★ 決定性指標印到 stdout（可重現）
        std::cout << "  拷貝 " << N << " 個 Blob：配置記憶體 " << copyAllocs << " 次\n";
        std::cout << "  移動 " << N << " 個 Blob：配置記憶體 " << moveAllocs
                  << " 次（改用搬指標 " << moveCount << " 次）\n";
        std::cout << "  → 拷貝是 O(n) 配置、移動是 O(1) 搬指標，這才是速度差的根源\n";

        // 牆鐘時間只作參考，送 stderr 並註明每次執行都不同（不進 stdout）
        auto copy_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        auto move_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
        std::cerr << "  [參考|每次執行都不同] 拷貝約 " << copy_ms
                  << " ms、移動約 " << move_ms << " ms\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  移動三步驟：偷指標 → 歸零源物件 → 複製基本型別\n";
    std::cout << "  noexcept 讓 vector 擴容時用移動而非拷貝\n";
    std::cout << "  自訂解構/拷貝 → 移動不自動生成 → 需要 Rule of Five\n";
    std::cout << "  移動是 O(1)，拷貝是 O(n)，差異巨大\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder summary.cpp -o summary

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 下面貼的是 stdout 的內容，完全決定性：沒有位址、沒有耗時、沒有執行緒
//   （本機實測連跑 5 次逐位元組相同）。牆鐘時間刻意印到 stderr、不進 stdout，
//   所以「每次執行都不同」的雜訊不會污染可對照的預期輸出。
// * 示範 2 只有一次 [建構]、沒有 [移動建構]：
//   SimpleString("Phoenix Staff") 是純右值，C++17 保證省略，
//   直接就地建構在 a 的位置上（見檔頭【概念補充 A】）。
// * 示範 4 的第 3 次 push_back 觸發擴容，Alpha/Beta 走 [Widget 移動]
//   而非拷貝，正是因為 Widget 的移動建構標了 noexcept。
// * 示範 5 的 NoAutoMove「可移動建構=true、nothrow=false」不是矛盾：
//   true 是拷貝建構接住右值，false 才揭露它實際走的是拷貝（見第 32 課 -3）。
// * 示範 6 是本檔的效能證據，用「配置次數」而非計時：
//   拷貝 200000 個 Blob 配置 200000 次記憶體，移動配置 0 次（改搬指標 200000 次）。
//   0 vs 200000 是決定性的，直接對應 O(n) vs O(1)。
// * 示範 1、3 各段結束時的 [解構] 順序為區域物件宣告的反向；
//   被移動過的物件印 (nullptr，已被移動) 是安全的（自己歸零的成員）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 1. 基本移動建構 =====
//   [建構] "Dragon Sword"
//   拷貝建構（左值）：
//   [拷貝建構💰] "Dragon Sword"
//   移動建構（std::move）：
//   [移動建構⚡] "Dragon Sword"
//   a = "(null)" (已被移動)
//   c = "Dragon Sword"
//   [解構] "Dragon Sword"
//   [解構] "Dragon Sword"
//   [解構] (nullptr，已被移動)
//
// ===== 2. 從臨時物件移動 =====
//   [建構] "Phoenix Staff"
//   a = "Phoenix Staff"
//   [解構] "Phoenix Staff"
//
// ===== 3. 移動後重新賦值 =====
//   [建構] "First"
//   [移動建構⚡] "First"
//   a = "(null)" (已被移動)
//   [建構] "Reborn"
//   [解構] (nullptr，已被移動)
//   a = "Reborn" (重生了)
//   [解構] "First"
//   [解構] "Reborn"
//
// ===== 4. noexcept 對 vector 的影響 =====
//   push_back #1:
//     [Widget 移動] "Alpha"
//   push_back #2:
//     [Widget 移動] "Beta"
//   push_back #3（觸發擴容，搬移既有元素）:
//     [Widget 移動] "Gamma"
//     [Widget 移動] "Alpha"
//     [Widget 移動] "Beta"
//
// ===== 5. 自動生成規則 =====
//   AutoMove（什麼都沒寫）:
//     可移動建構？ true
//     nothrow？   true
//   NoAutoMove（自訂了解構函數）:
//     可移動建構？ true
//     nothrow？   false
//
// ===== 6. 效能比較（配置次數）=====
//   拷貝 200000 個 Blob：配置記憶體 200000 次
//   移動 200000 個 Blob：配置記憶體 0 次（改用搬指標 200000 次）
//   → 拷貝是 O(n) 配置、移動是 O(1) 搬指標，這才是速度差的根源
//
// === 重點整理 ===
//   移動三步驟：偷指標 → 歸零源物件 → 複製基本型別
//   noexcept 讓 vector 擴容時用移動而非拷貝
//   自訂解構/拷貝 → 移動不自動生成 → 需要 Rule of Five
//   移動是 O(1)，拷貝是 O(n)，差異巨大
