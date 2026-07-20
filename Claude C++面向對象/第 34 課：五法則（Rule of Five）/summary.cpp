// =============================================================================
//  第 34 課 總結  —  五法則（Rule of Five）與 Rule of Zero
// =============================================================================
//
// 【主題資訊 Information】
//   五法則 = 三法則 + 兩個移動操作：
//       ① 解構函數        ~ClassName();
//       ② 拷貝建構函數    ClassName(const ClassName&);
//       ③ 拷貝賦值運算子  ClassName& operator=(const ClassName&);
//       ④ 移動建構函數    ClassName(ClassName&&) noexcept;
//       ⑤ 移動賦值運算子  ClassName& operator=(ClassName&&) noexcept;
//   若用統一賦值（傳值 + swap）：④⑤ 合成一個，實際只寫 4 個
//       解構 + 拷貝建構 + 移動建構 + 統一賦值
//   標準版本 ： C++11
//   標頭檔   ： <algorithm>、<utility>、<string>、<vector>、<memory>、<type_traits>
//   對照     ： Rule of Zero（最推薦，什麼都不寫）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼五個要一起想】
//   持有裸資源的類別需要自訂解構函數；而只要宣告了解構（或任一拷貝操作），
//   編譯器就「不再自動生成」移動建構與移動賦值（見第 32 課 -3）。
//   於是所有「移動」都安靜退回深拷貝，效能默默變差。
//   結論：需要其中一個（通常是解構），就得把五個補齊，否則得到半殘類別。
//   本檔的 ManagedArray 是完整示範（用統一賦值精簡成 4 個），
//   FullFive 則是「分開寫 5 個」的對照。
//
// 【2. Rule of Zero 才是首選】
//   若類別只用會自我管理資源的成員（std::string、std::vector、
//   std::unique_ptr…），就「什麼特殊成員函式都不用寫」——
//   編譯器生成的版本全部正確，還自動推導 noexcept。
//   本檔的 SafeClass 只有 string 與 vector 成員，
//   拷貝、移動、解構全部安全，一行都沒寫。這是實務上最推薦的做法。
//   MoveOnly 更進一步：一個 unique_ptr 成員就讓它自動變成 move-only
//   （拷貝被隱式刪除、移動自動生成）。
//
// 【3. ★ 統一賦值的一個真實代價：nothrow 移動賦值是 false】
//   這是本檔 type_traits 表最值得玩味的一格：
//       ManagedArray（用統一賦值）→ 可移動賦值 true，但 nothrow 移動賦值 false
//       MoveOnly / SafeClass       → nothrow 移動賦值 true
//   為什麼 ManagedArray 是 false？因為統一賦值 operator=(ManagedArray other)
//   是「傳值」的，它「不能」標 noexcept —— 建構那個值參數在傳左值時
//   要深拷貝、可能拋 bad_alloc。所以 is_nothrow_move_assignable 回 false。
//   影響：某些對「移動賦值是否 noexcept」敏感的標準庫操作，
//   對統一賦值的類別會保守一點。這是「用一個函式同時處理拷貝與移動、
//   換來簡潔與強例外保證」所付出的代價。
//   若要 nothrow 的移動賦值，就得像 FullFive 那樣「分開寫」一個
//   標了 noexcept 的 operator=(T&&)。兩種寫法各有取捨（見第 33 課）。
//
// 【4. type_traits 怎麼讀這張表】
//   * is_move_constructible 幾乎永遠是 true —— 拷貝建構可接住右值。
//     要分辨真假移動看 is_nothrow_move_constructible。
//   * MoveOnly 的「可拷貝」兩項是 false：unique_ptr 讓拷貝被隱式刪除。
//   * trait 只驗介面、不驗實作：全 true 不代表類別寫對了（見第 34 課 -2）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 成員宣告順序必須與所有初始化列表一致（-Wreorder）
//   本檔原始版本的 ManagedArray 宣告是 int* m_data; 在前、
//   std::size_t m_size; 在後，但初始化列表寫法不一致：
//       一般建構、拷貝建構：m_size(...), m_data(...)   （size 在前）
//       移動建構          ：m_data(...), m_size(...)   （data 在前）
//   於是無論宣告怎麼排都會觸發 -Wreorder（本機實測原始版本報 6 次）。
//   本檔已把宣告改為 m_size 在前、m_data 在後（被依賴的成員在前 ——
//   m_data 的配置長度用到 m_size），並把「所有」建構函數的初始化列表
//   統一成 m_size、m_data 的順序，警告全消。
//
// (B) ★ FullFive 的拷貝賦值修正（與第 34 課 -2 同一個真實缺陷）
//   原始版本的 FullFive 拷貝賦值是：
//       if (this != &o) *m_data = *o.m_data;
//   但 m_data 可能是 nullptr —— 移動操作會把來源的 m_data 設成 nullptr。
//   對「被移動過的物件」再做拷貝賦值就會解參考空指標 → 未定義行為
//   （第 34 課 -2 已用 -fsanitize=undefined 實測會報 store to null pointer）。
//   FullFive 在本檔只用於說明「分開寫 5 個」，執行期並未被賦值觸發，
//   所以這個缺陷不會出現在輸出中；但為了與第 34 課 -2 一致、也避免
//   讀者照抄一個有洞的範本，本檔已一併修正為先判斷 nullptr 再處理。
//   這也再次說明五法則要「五個一起設計」：移動留下的狀態，拷貝賦值必須接得住。
//
// (C) 統一賦值在輸出中的軌跡
//   ManagedArray d(2); d = b; 的輸出是：
//       [拷貝建構]（建構值參數 other，深拷貝 b）→ [統一賦值] swap →
//       [解構]（參數帶著 d 原本 size=2 的舊資源離場）
//   移動路徑 e = std::move(c) 則是 [移動建構] → [統一賦值] swap → [解構]。
//   對照這兩段就能看出「同一個 operator= 靠參數怎麼建構來分岔」。
//
// (D) 從函式回傳為什麼看不到移動建構
//   createArray 回傳具名區域變數，本機 NRVO 生效，直接建在 f 的位置上。
//   NRVO 是「可選」的最佳化，C++17 只保證純右值初始化的省略（見第 28 課 -2）。
//
// 【注意事項 Pay Attention】
//   1. 持有裸資源就要寫齊五法則（或用統一賦值精簡成 4 個）；少寫移動會半殘。
//   2. 首選 Rule of Zero —— 用 string/vector/unique_ptr 成員，什麼都不用寫。
//   3. 統一賦值不能標 noexcept，所以 is_nothrow_move_assignable 會是 false；
//      要 nothrow 的移動賦值請分開寫（見【詳細解釋 3】）。
//   4. 成員宣告順序決定初始化順序，且要與所有初始化列表一致（-Wreorder）。
//   5. 移動操作留下的 nullptr 狀態，拷貝賦值也必須能安全處理（見【概念補充 B】）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】五法則與 Rule of Zero
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 五法則是哪五個？用「統一賦值」為什麼可以只寫四個？
//     答：解構、拷貝建構、拷貝賦值、移動建構、移動賦值。
//         統一賦值 operator=(T other) 傳值 + swap，
//         傳左值時參數由拷貝建構、傳右值時由移動建構，
//         所以一個 operator= 就同時涵蓋了拷貝賦值與移動賦值，
//         實際只需寫：解構 + 拷貝建構 + 移動建構 + 統一賦值。
//     追問：那有沒有「一個都不用寫」的做法？
//         → Rule of Zero：把裸資源換成 string/vector/unique_ptr 這類
//           會自我管理的成員，五個特殊成員函式全部交給編譯器，且都正確。
//           這是實務首選。
//
// 🔥 Q2. 用統一賦值的類別，is_nothrow_move_assignable 會是 true 還是 false？為什麼？
//     答：false。因為統一賦值 operator=(T other) 是「傳值」的，
//         它不能標 noexcept —— 建構值參數在傳左值時要深拷貝、可能拋 bad_alloc。
//         本檔的 ManagedArray 就是這樣：可移動賦值 true，但 nothrow 移動賦值 false。
//         若需要 nothrow 的移動賦值，要像 FullFive 那樣分開寫一個
//         標了 noexcept 的 operator=(T&&)。
//     追問：這個 false 有什麼實際影響？
//         → 對「移動賦值是否 noexcept」敏感的操作會保守處理。
//           這是統一賦值用簡潔與強例外保證換來的代價，需視情境取捨。
//
// ⚠️ 陷阱. 「我的類別只用了 std::string 和 std::vector 成員，
//           但為了保險起見，還是把五個特殊成員函式都手寫一遍比較安全。」
//     答：這通常是幫倒忙。string/vector 已經正確管理自己的資源，
//         編譯器生成的五個特殊成員函式會逐成員轉呼叫它們，全部正確，
//         還會自動推導 noexcept。你手寫的版本反而容易出錯
//         （漏標 noexcept、漏處理自我賦值、拷貝賦值沒接住移動後的狀態…），
//         而且一旦寫了解構函數，還會關掉移動的自動生成、讓效能變差。
//     為什麼會錯：把「手動控制」等同於「更安全」。
//         Rule of Zero 的核心洞見正相反：把資源管理下放給已經正確的成員，
//         自己什麼都不寫，才是最不容易出錯的做法。手寫五法則是
//         「真的持有裸資源」時的最後手段，不是預設選項。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   五法則與 Rule of Zero 是資源管理型類別的設計規範，屬於工程與正確性議題。
//   LeetCode 判的是演算法的輸入輸出與時間限制，
//   評測系統只會建立一個實例並呼叫指定方法，
//   既不會拷貝也不會移動你的物件，更不會檢驗特殊成員函式是否寫齊。
//   即使是設計類題目（146. LRU Cache、707. Design Linked List），
//   實務上也會直接用 std::list / std::unordered_map 這些已遵守
//   Rule of Zero 的標準容器。硬掛題號只會誤導，故從缺。
//
// -----------------------------------------------------------------------------

#include <iostream>
#include <algorithm>  // std::copy
#include <utility>    // std::swap, std::move
#include <string>
#include <vector>
#include <memory>     // std::unique_ptr, std::make_unique
#include <type_traits>

// ============================================================
// 完整的 Rule of Five 類別：ManagedArray（用統一賦值，實際寫 4 個）
// ============================================================
class ManagedArray {
    // ⚠️ 宣告順序改為 m_size 在前、m_data 在後，與所有初始化列表一致，
    //    避免 -Wreorder（原始版本相反，本機實測報 6 次，見檔頭【概念補充 A】）。
    std::size_t m_size;
    int* m_data;

public:
    // 建構函數
    explicit ManagedArray(std::size_t size = 0)
        : m_size(size), m_data(size > 0 ? new int[size]{} : nullptr) {
        std::cout << "  [建構] size=" << m_size << "\n";
    }

    // ═══════════════════════════════════════════
    // ★ Rule of Five（使用統一賦值，實際只寫 4 個）★
    // ═══════════════════════════════════════════

    // ① 解構函數
    ~ManagedArray() {
        std::cout << "  [解構] size=" << m_size << "\n";
        delete[] m_data;
    }

    // ② 拷貝建構函數（深拷貝）
    ManagedArray(const ManagedArray& other)
        : m_size(other.m_size),
          m_data(other.m_size > 0 ? new int[other.m_size] : nullptr) {
        if (m_data) std::copy(other.m_data, other.m_data + m_size, m_data);
        std::cout << "  [拷貝建構] size=" << m_size << "\n";
    }

    // ③ 移動建構函數（偷資源）—— 初始化列表順序統一為 m_size、m_data
    ManagedArray(ManagedArray&& other) noexcept
        : m_size(other.m_size), m_data(other.m_data) {
        other.m_data = nullptr;
        other.m_size = 0;
        std::cout << "  [移動建構] size=" << m_size << "\n";
    }

    // ④⑤ 統一賦值運算子（同時處理拷貝和移動）
    //     注意：傳值 → 不能標 noexcept → is_nothrow_move_assignable 為 false
    //     （見檔頭【詳細解釋 3】，這是統一賦值的真實代價）
    void swap(ManagedArray& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

    ManagedArray& operator=(ManagedArray other) {  // 傳值
        std::cout << "  [統一賦值] swap\n";
        swap(other);
        return *this;
    }

    // ═══════════════════════════════════════════

    int& operator[](std::size_t i) { return m_data[i]; }
    const int& operator[](std::size_t i) const { return m_data[i]; }
    std::size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

    void print(const char* label) const {
        std::cout << "  " << label << " (size=" << m_size << "): [";
        for (std::size_t i = 0; i < m_size; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << m_data[i];
        }
        std::cout << "]\n";
    }
};

// ============================================================
// Rule of Five（分開寫 5 個）— 對照版本
// ============================================================
class FullFive {
    int* m_data;
public:
    FullFive() : m_data(new int(0)) {}

    ~FullFive() { delete m_data; }                                        // ①

    FullFive(const FullFive& o)                                          // ②
        : m_data(o.m_data ? new int(*o.m_data) : nullptr) {}

    // ③ 拷貝賦值：必須能處理「來源或自己曾被移動過」（m_data 為 nullptr）的情況。
    //    原始版本寫 if (this != &o) *m_data = *o.m_data;，對被移動過的物件
    //    會解參考空指標 → 未定義行為（見檔頭【概念補充 B】）。
    FullFive& operator=(const FullFive& o) {
        if (this != &o) {
            if (!o.m_data) { delete m_data; m_data = nullptr; }
            else if (m_data) { *m_data = *o.m_data; }
            else { m_data = new int(*o.m_data); }
        }
        return *this;
    }

    FullFive(FullFive&& o) noexcept : m_data(o.m_data) {                 // ④
        o.m_data = nullptr;
    }

    FullFive& operator=(FullFive&& o) noexcept {                          // ⑤
        if (this != &o) { delete m_data; m_data = o.m_data; o.m_data = nullptr; }
        return *this;
    }
};

// ============================================================
// Rule of Zero 示範
// ============================================================
struct SafeClass {
    std::string name;       // RAII 成員
    std::vector<int> data;  // RAII 成員
    // 不需要寫任何解構/拷貝/移動！編譯器預設的就是正確的
};

// ============================================================
// 只能移動的類別（MoveOnly）
// ============================================================
class MoveOnly {
    std::unique_ptr<int> m_data;
public:
    MoveOnly() : m_data(std::make_unique<int>(0)) {}
    // unique_ptr 讓拷貝被隱式刪除，移動自動生成
};

// ============================================================
// type_traits 檢查
// ============================================================
template <typename T>
void checkTraits(const char* name) {
    std::cout << "  " << name << ":\n";
    std::cout << "    可解構？           " << std::is_destructible_v<T> << "\n";
    std::cout << "    可拷貝建構？       " << std::is_copy_constructible_v<T> << "\n";
    std::cout << "    可拷貝賦值？       " << std::is_copy_assignable_v<T> << "\n";
    std::cout << "    可移動建構？       " << std::is_move_constructible_v<T> << "\n";
    std::cout << "    nothrow 移動建構？ " << std::is_nothrow_move_constructible_v<T> << "\n";
    std::cout << "    可移動賦值？       " << std::is_move_assignable_v<T> << "\n";
    std::cout << "    nothrow 移動賦值？ " << std::is_nothrow_move_assignable_v<T> << "\n\n";
}

// 按值回傳 → 移動或 NRVO
ManagedArray createArray(std::size_t n) {
    ManagedArray arr(n);
    for (std::size_t i = 0; i < n; ++i) arr[i] = static_cast<int>((i + 1) * 100);
    return arr;
}

// 按值傳入 → 拷貝（左值）或移動（右值）
void consumeArray(ManagedArray arr) {
    arr.print("consumed");
}

int main() {
    // ============================================================
    // 1. 完整 Rule of Five 使用
    // ============================================================
    std::cout << "===== 1. Rule of Five 完整使用 =====\n";
    {
        ManagedArray a(4);
        for (std::size_t i = 0; i < a.size(); ++i) a[i] = static_cast<int>(i + 1);
        a.print("a");

        std::cout << "\n  拷貝建構：\n";
        ManagedArray b = a;
        b.print("b");

        std::cout << "\n  移動建構：\n";
        ManagedArray c = std::move(a);
        c.print("c");
        a.print("a (moved)");

        std::cout << "\n  拷貝賦值（統一賦值，左值路徑）：\n";
        ManagedArray d(2);
        d = b;
        d.print("d");

        std::cout << "\n  移動賦值（統一賦值，右值路徑）：\n";
        ManagedArray e(1);
        e = std::move(c);
        e.print("e");
        c.print("c (moved)");
    }
    std::cout << "\n";

    // ============================================================
    // 2. 從函數回傳 / 傳入函數
    // ============================================================
    std::cout << "===== 2. 函數回傳與傳入 =====\n";
    {
        ManagedArray f = createArray(3);
        f.print("f (from function)");

        std::cout << "  傳左值（拷貝）：\n";
        consumeArray(f);

        std::cout << "  傳右值（移動）：\n";
        consumeArray(std::move(f));
        f.print("f (moved)");
    }
    std::cout << "\n";

    // ============================================================
    // 3. Rule of Zero
    // ============================================================
    std::cout << "===== 3. Rule of Zero =====\n";
    {
        SafeClass s1{"Hero", {1, 2, 3}};
        SafeClass s2 = s1;              // 安全拷貝
        SafeClass s3 = std::move(s1);   // 安全移動
        std::cout << "  s2.name=" << s2.name << " s3.name=" << s3.name << "\n";
        // s1.name 被移動後是 valid but unspecified，libstdc++ 實測會清空 ——
        // 這裡列印只為示意，實務上不應依賴被移動後的內容（見第 31 課 -4）。
        std::cout << "  s1.name=\"" << s1.name << "\" (已被移動)\n";
        std::cout << "  只用 RAII 成員 → 不需要寫任何特殊函數！\n";
    }
    std::cout << "\n";

    // ============================================================
    // 4. type_traits 檢查
    // ============================================================
    std::cout << "===== 4. type_traits 檢查 =====\n";
    std::cout << std::boolalpha;
    checkTraits<ManagedArray>("ManagedArray (Rule of Five)");
    checkTraits<MoveOnly>("MoveOnly (只能移動)");
    checkTraits<SafeClass>("SafeClass (Rule of Zero)");
    checkTraits<std::string>("std::string (標準庫)");

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "=== 重點整理 ===\n";
    std::cout << "  Rule of Five：解構 + 拷貝建構 + 拷貝賦值 + 移動建構 + 移動賦值\n";
    std::cout << "  統一賦值（傳值 + swap）可以把拷貝賦值和移動賦值合為一個\n";
    std::cout << "  但統一賦值不能 noexcept → nothrow 移動賦值會是 false\n";
    std::cout << "  Rule of Zero：只用 RAII 成員 → 什麼都不用寫（最佳）\n";
    std::cout << "  用 type_traits 可以在編譯期檢查類別的能力\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder summary.cpp -o summary

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：只有整數運算、字串與編譯期 trait，
//   沒有位址、沒有耗時、沒有執行緒（本機實測連跑 5 次逐位元組相同）。
// * 印出被移動物件的 size=0 是安全的：那是移動操作明確歸零的成員。
//   示範 3 的 s1.name 被移動後印空字串則是 libstdc++ 的實作行為（清空），
//   標準不保證，這裡只為示意，實務上不應依賴（見程式內註解與第 31 課 -4）。
// * 示範 1 的統一賦值兩段請對照：
//       左值路徑 d = b       → [拷貝建構] → [統一賦值] swap → [解構]
//       右值路徑 e = std::move(c) → [移動建構] → [統一賦值] swap → [解構]
//   中間的 [解構] 是值參數帶著目標的舊資源離場時印的。
// * ★ type_traits 表最值得看的一格：ManagedArray 的
//   「可移動賦值 true、nothrow 移動賦值 false」——
//   因為統一賦值傳值、不能標 noexcept。對照 MoveOnly / SafeClass 都是 true。
//   這是統一賦值的真實代價（見檔頭【詳細解釋 3】）。
// * 示範 2 的 createArray 回傳看不到 [移動建構]：本機 NRVO 生效。
// * 本檔已修正原始版本的兩處問題：ManagedArray 的 6 個 -Wreorder 警告
//   （成員宣告順序），以及 FullFive 拷貝賦值的空指標缺陷（與第 34 課 -2 一致）。
//   兩者都不改變本檔的執行輸出（FullFive 在執行期未被賦值觸發）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 1. Rule of Five 完整使用 =====
//   [建構] size=4
//   a (size=4): [1, 2, 3, 4]
//
//   拷貝建構：
//   [拷貝建構] size=4
//   b (size=4): [1, 2, 3, 4]
//
//   移動建構：
//   [移動建構] size=4
//   c (size=4): [1, 2, 3, 4]
//   a (moved) (size=0): []
//
//   拷貝賦值（統一賦值，左值路徑）：
//   [建構] size=2
//   [拷貝建構] size=4
//   [統一賦值] swap
//   [解構] size=2
//   d (size=4): [1, 2, 3, 4]
//
//   移動賦值（統一賦值，右值路徑）：
//   [建構] size=1
//   [移動建構] size=4
//   [統一賦值] swap
//   [解構] size=1
//   e (size=4): [1, 2, 3, 4]
//   c (moved) (size=0): []
//   [解構] size=4
//   [解構] size=4
//   [解構] size=0
//   [解構] size=4
//   [解構] size=0
//
// ===== 2. 函數回傳與傳入 =====
//   [建構] size=3
//   f (from function) (size=3): [100, 200, 300]
//   傳左值（拷貝）：
//   [拷貝建構] size=3
//   consumed (size=3): [100, 200, 300]
//   [解構] size=3
//   傳右值（移動）：
//   [移動建構] size=3
//   consumed (size=3): [100, 200, 300]
//   [解構] size=3
//   f (moved) (size=0): []
//   [解構] size=0
//
// ===== 3. Rule of Zero =====
//   s2.name=Hero s3.name=Hero
//   s1.name="" (已被移動)
//   只用 RAII 成員 → 不需要寫任何特殊函數！
//
// ===== 4. type_traits 檢查 =====
//   ManagedArray (Rule of Five):
//     可解構？           true
//     可拷貝建構？       true
//     可拷貝賦值？       true
//     可移動建構？       true
//     nothrow 移動建構？ true
//     可移動賦值？       true
//     nothrow 移動賦值？ false
//
//   MoveOnly (只能移動):
//     可解構？           true
//     可拷貝建構？       false
//     可拷貝賦值？       false
//     可移動建構？       true
//     nothrow 移動建構？ true
//     可移動賦值？       true
//     nothrow 移動賦值？ true
//
//   SafeClass (Rule of Zero):
//     可解構？           true
//     可拷貝建構？       true
//     可拷貝賦值？       true
//     可移動建構？       true
//     nothrow 移動建構？ true
//     可移動賦值？       true
//     nothrow 移動賦值？ true
//
//   std::string (標準庫):
//     可解構？           true
//     可拷貝建構？       true
//     可拷貝賦值？       true
//     可移動建構？       true
//     nothrow 移動建構？ true
//     可移動賦值？       true
//     nothrow 移動賦值？ true
//
// === 重點整理 ===
//   Rule of Five：解構 + 拷貝建構 + 拷貝賦值 + 移動建構 + 移動賦值
//   統一賦值（傳值 + swap）可以把拷貝賦值和移動賦值合為一個
//   但統一賦值不能 noexcept → nothrow 移動賦值會是 false
//   Rule of Zero：只用 RAII 成員 → 什麼都不用寫（最佳）
//   用 type_traits 可以在編譯期檢查類別的能力
