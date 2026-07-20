// ============================================================
// 第 2.3 章 總結：移動建構子（Move Constructor）— 實作與原理
// ============================================================
//
// 【主題資訊 Information】
//   簽名    ：ClassName(ClassName&& other) noexcept;
//   標準版本：移動建構子 / 右值參考 / noexcept       C++11
//             = default / = delete                   C++11
//             保證的複製省略（影響觸發時機）           C++17
//   標頭檔  ：<utility>（std::move）、<type_traits>（is_nothrow_move_constructible）
//   複雜度  ：移動 O(1)（搬指標）；複製 O(N)（配置 + 拷貝）
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 移動建構子的三個步驟，缺一不可】
//       (a) 偷取：m_data = other.m_data;      ← 把資源的所有權接過來
//       (b) 歸零：other.m_data = nullptr;     ← 讓來源不再擁有它
//       (c) 同步基本型別：m_size = other.m_size; other.m_size = 0;
//   第 (b) 步最常被忘記，而它是最關鍵的：
//   若不歸零，兩個物件會持有同一個指標，解構時各 delete 一次 →
//   double free（未定義行為）。
//   換句話說，「移動」的完整語意是「偷走資源 + 把來源變成安全的空殼」。
//
// 【2. 為什麼移動是 O(1)】
//   複製建構子必須配置一塊新記憶體、再逐位元組拷貝內容 —— 與資料量成正比。
//   移動建構子完全不碰堆積上的資料，只是把指標抄過來、把來源清空。
//   無論資源是 1 KB 還是 1 GB，成本都一樣。
//
// 【3. noexcept 為什麼是「必須」而不是「建議」】
//   std::vector 擴容時要把舊元素搬到新緩衝區。若移動途中拋例外，
//   舊元素已被掏空、新緩衝區不完整，無法回復到原狀 ——
//   這破壞了 vector 的強例外保證。
//   因此 vector 採取 std::move_if_noexcept 策略：
//       移動操作標了 noexcept → 使用移動（快）
//       沒標                  → 退回複製（慢，但可回復）
//   結論：沒標 noexcept 的移動建構子，在 vector 擴容時等於白寫。
//   驗證方式：std::is_nothrow_move_constructible<T>::value。
//
// 【4. 成員初始化順序：宣告順序決定一切】
//   初始化列表的「書寫順序」不影響實際執行順序 ——
//   成員一律依「在類別中宣告的順序」初始化。
//   這在自管資源的類別上會造成嚴重後果：
//       char*  m_data;     // 先宣告
//       size_t m_size;     // 後宣告
//       Foo(const char* s) : m_size(strlen(s)), m_data(new char[m_size+1]) {}
//   看起來 m_size 先算好了，實際上 m_data 會「先」初始化，
//   此時 m_size 尚未初始化 → 讀到不確定的值 → 配置出錯誤大小的緩衝區。
//   本檔的 MoveDemo 刻意把 m_size 宣告在 m_data 之前，
//   讓宣告順序與依賴順序一致 —— 請勿調換這兩行。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 什麼時候編譯器「不會」自動生成移動建構子
//     只要你自訂了下列任何一項，移動建構子與移動賦值就不會被自動生成：
//       * 解構子
//       * 複製建構子 / 複製賦值運算子
//       * 另一個移動操作
//     這是刻意的保護：既然你需要手動管理資源，編譯器不敢猜你的移動語意。
//     後果是 std::move 會安靜地退回複製 —— 程式仍然正確，但完全沒加速。
//     需要移動時請明寫 = default 或自己實作（這就是 Rule of Five）。
//
// (B) is_move_constructible 為什麼會「騙人」
//     即使類別沒有移動建構子，std::is_move_constructible<T>::value 仍是 true ——
//     因為 const T& 也能接住右值（會走複製）。
//     要判斷「有沒有真正的移動」，應該看
//     std::is_nothrow_move_constructible<T>::value。
//     本章的範例檔（第 4 個）正是用這組對照來揭穿這件事。
//
// (C) 移動建構子與 RVO 的關係
//     MyClass a = func(); 未必會呼叫移動建構子 ——
//     C++17 起「保證的複製省略」規定：從 prvalue 初始化時根本不存在中間物件，
//     連移動都不會發生（成本比移動更低）。
//     所以看不到移動建構子被呼叫，不代表程式碼有問題，可能是編譯器做得更好。
//
// (D) 被移動後的物件要維持什麼狀態
//     標準要求「有效但未指定」：可安全解構、可重新賦值、
//     可呼叫沒有前提條件的操作（如 empty()）。
//     實作上最簡單的作法就是把指標設 nullptr、長度設 0，
//     並讓解構子與各個成員函式都能正確處理這個空狀態。
//
// 【注意事項 Pay Attention】
//   1. 移動建構子務必標 noexcept，否則 vector 擴容時會退回複製。
//   2. 必須把來源的指標歸零，否則會 double free（未定義行為）。
//   3. 成員初始化依「宣告順序」，與初始化列表的書寫順序無關；
//      本檔 MoveDemo 的 m_size 必須宣告在 m_data 之前，請勿調換。
//   4. 自訂解構子或複製操作會「抑制」移動操作的自動生成，此時 std::move 退回複製。
//   5. is_move_constructible 為 true 不代表真的有移動；請看 is_nothrow_move_constructible。
//   6. 被移動後的物件處於「有效但未指定」狀態，除重新賦值或銷毀外不要使用。
//   7. 本檔自管 new/delete[] 是為了教學；實務上請優先 Rule of Zero（用 string/vector）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】移動建構子
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請寫出移動建構子並說明每一步的必要性。
//     答：ClassName(ClassName&& o) noexcept
//             : m_size(o.m_size), m_data(o.m_data)     // ① 接手資源
//         { o.m_data = nullptr; o.m_size = 0; }        // ② 讓來源變空殼
//         ① 是效能所在（O(1) 搬指標）；
//         ② 是正確性所在 —— 不做的話兩個物件會 delete 同一塊記憶體（double free）。
//         noexcept 則是為了讓 vector 擴容時願意使用移動。
//     追問：初始化列表的順序重要嗎？→ 書寫順序不重要，
//         但成員的「宣告順序」決定實際初始化順序；
//         若後面的成員依賴前面的成員，宣告順序就必須正確。
//
// 🔥 Q2. 為什麼移動建構子沒標 noexcept，vector 擴容就不用它？
//     答：vector 擴容要把舊元素搬到新記憶體。若移動可能拋例外，
//         搬到一半失敗時舊元素已被掏空、新的又不完整，無法回復，
//         這會破壞 vector 的強例外保證。
//         因此 vector 用 std::move_if_noexcept：沒標 noexcept 就退回複製。
//     追問：怎麼檢查我的類別有沒有做對？→
//         static_assert(std::is_nothrow_move_constructible<T>::value, "...");
//
// 🔥 Q3. 我自訂了解構子，還會有自動生成的移動建構子嗎？
//     答：不會。自訂解構子（或任一複製操作）會抑制移動操作的自動生成。
//         此時 std::move 會安靜退回複製 —— 程式正確但毫無加速。
//         需要移動請明寫 ClassName(ClassName&&) = default; 或自己實作。
//     追問：那怎麼發現這件事？→ is_move_constructible 會騙你（仍是 true，
//         因為 const T& 接得住右值）；要用 is_nothrow_move_constructible 才看得出來。
//
// ⚠️ 陷阱. 移動建構子裡忘了把 other 的指標設成 nullptr，會怎樣？
//     答：兩個物件持有同一個指標，各自解構時都會 delete[]，
//         造成 double free —— 這是未定義行為，
//         可能當場崩潰、可能破壞堆積後在無關的地方出錯，沒有固定的表現。
//     為什麼會錯：把移動想成「只是把資料搬過來」，忽略了「所有權」的移轉。
//         正確心智模型：移動 = 轉移所有權，
//         而所有權是唯一的 —— 交出去之後，來源就必須不再持有它。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯：g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
// ============================================================
// 注意：本課原始檔案為空，以下根據課程標題提供完整重點整理
// 移動建構子的詳細內容可參考面向對象課程第 32 課
//
// 【移動建構子簽名】
//   ClassName(ClassName&& other) noexcept
//
// 【實作三步驟】
//   1. 偷取（steal）：m_data = other.m_data;
//   2. 歸零（nullify）：other.m_data = nullptr;
//   3. 複製基本型別：m_size = other.m_size; other.m_size = 0;
//
// 【vs 拷貝建構子】
//   拷貝建構：new 記憶體 + 複製資料 → O(n)
//   移動建構：偷指標 + 歸零         → O(1)
//
// 【noexcept 的重要性】
//   vector 擴容時：
//     有 noexcept → 使用移動（快）
//     無 noexcept → 退回拷貝（慢，但安全）
//
// 【觸發條件】
//   傳入 rvalue 時觸發移動建構：
//     MyClass a = std::move(b);        // 明確移動
//     MyClass a = func();              // 回傳臨時物件（可能被 RVO 省略）
//     MyClass a = MyClass("temp");     // 從臨時物件
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>
#include <vector>
#include <string>
#include <type_traits>
using namespace std;

class MoveDemo {
    // ⚠️ 成員的初始化順序一律依「宣告順序」，與初始化列表的書寫順序無關。
    //    m_size 必須宣告在 m_data 前面，否則 new char[m_size + 1] 會讀到
    //    尚未初始化的 m_size，配置出錯誤大小的緩衝區（heap-buffer-overflow）。
    size_t m_size;
    char* m_data;
public:
    // 一般建構
    MoveDemo(const char* str = "")
        : m_size(strlen(str)), m_data(new char[m_size + 1]) {
        strcpy(m_data, str);
        cout << "  [建構] \"" << m_data << "\"\n";
    }

    // 拷貝建構（深拷貝）— O(n)
    MoveDemo(const MoveDemo& other)
        : m_size(other.m_size), m_data(new char[m_size + 1]) {
        strcpy(m_data, other.m_data);
        cout << "  [拷貝建構💰] \"" << m_data << "\" — new + copy\n";
    }

    // ★ 移動建構 — O(1)
    MoveDemo(MoveDemo&& other) noexcept
        : m_size(other.m_size)          // ① 複製基本型別（順序須與宣告順序一致）
        , m_data(other.m_data)          // ② 偷取指標
    {
        other.m_data = nullptr;         // ③ 歸零源物件
        other.m_size = 0;
        cout << "  [移動建構⚡] \"" << m_data << "\" — 只搬指標\n";
    }

    // 解構
    ~MoveDemo() {
        if (m_data) cout << "  [解構] \"" << m_data << "\"\n";
        else        cout << "  [解構] (nullptr)\n";
        delete[] m_data;
    }

    const char* c_str() const { return m_data ? m_data : "(null)"; }
};

// ============================================================
// 【LeetCode 實戰範例】—— 本章刻意從缺，理由如下
// ============================================================
// 移動建構子是「資源所有權轉移」的實作技術，屬於類別設計問題。
// LeetCode 的題目不會要求你自己寫一個管理原始記憶體的類別，
// 解法用的都是 std::vector / std::string 這些「已經幫你寫好移動建構子」的型別。
// 因此本章不提供 LeetCode 範例，改以一個真實的資源管理情境示範。
// ============================================================

// ------------------------------------------------------------
// 【日常實務範例】連線池：handle 只能有一個擁有者
//   情境：資料庫/socket 連線持有作業系統的 handle（fd）。
//         這種資源「不能被複製」—— 複製一份 fd 再各自關閉會關到同一個 fd 兩次。
//   為什麼用到本主題：
//     這正是移動語意存在的理由。連線可以「移交」但不可以「複製」，
//     所以我們刪除複製操作、只提供移動操作（move-only 型別）。
//     unique_ptr、thread、fstream 都是同樣的設計。
//   ⚠️ 成員宣告順序：m_handle 在前、m_name 在後，
//      本類別的初始化不互相依賴，但仍維持「書寫順序 = 宣告順序」的好習慣。
// ------------------------------------------------------------
class Connection {
    int    m_handle;    // 模擬 OS handle，-1 代表已失效
    string m_name;

public:
    Connection(const string& name, int handle)
        : m_handle(handle), m_name(name) {
        cout << "  [開啟連線] " << m_name << " (handle=" << m_handle << ")\n";
    }

    // move-only：明確刪除複製，避免同一個 handle 被關閉兩次
    Connection(const Connection&)            = delete;
    Connection& operator=(const Connection&) = delete;

    // 移動建構子：接手 handle，並讓來源失效
    Connection(Connection&& other) noexcept
        : m_handle(other.m_handle)          // ① 接手資源
        , m_name(std::move(other.m_name)) {
        other.m_handle = -1;                // ② 來源不再擁有它（關鍵！）
        cout << "  [移交連線] handle=" << m_handle << " 已轉移\n";
    }

    ~Connection() {
        if (m_handle != -1)
            cout << "  [關閉連線] " << m_name << " (handle=" << m_handle << ")\n";
        else
            cout << "  [解構] 已移交的空殼，無需關閉\n";
    }

    string describe() const {
        return m_handle == -1 ? string("(已移交，無 handle)")
                              : m_name + "/" + std::to_string(m_handle);
    }
};

class ConnectionPool {
    vector<Connection> m_conns;
public:
    ConnectionPool() { m_conns.reserve(4); }   // 先保留，避免擴容干擾示範輸出

    void add(Connection&& c) {
        m_conns.push_back(std::move(c));       // c 有名字是 lvalue → 須再 move
    }

    void report() const {
        cout << "  連線池目前持有 " << m_conns.size() << " 條連線：\n";
        for (size_t i = 0; i < m_conns.size(); ++i)
            cout << "    " << m_conns[i].describe() << "\n";
    }
};

int main() {
    // ============================================================
    // 1. 拷貝 vs 移動
    // ============================================================
    cout << "===== 1. 拷貝 vs 移動 =====\n";
    {
        MoveDemo a("Hello World");

        cout << "\n  拷貝建構（深拷貝）：\n";
        MoveDemo b = a;           // 拷貝 — O(n)

        cout << "\n  移動建構（偷指標）：\n";
        MoveDemo c = move(a);     // 移動 — O(1)
        cout << "  a = \"" << a.c_str() << "\" (已被掏空)\n";
        cout << "  c = \"" << c.c_str() << "\"\n";
    }
    cout << "\n";

    // ============================================================
    // 2. 從臨時物件移動
    // ============================================================
    cout << "===== 2. 從臨時物件移動 =====\n";
    {
        MoveDemo d = MoveDemo("Temporary");
        cout << "  d = \"" << d.c_str() << "\"\n";
    }
    cout << "\n";

    // ============================================================
    // 3. noexcept 影響 vector 擴容
    // ============================================================
    cout << "===== 3. vector 擴容使用移動 =====\n";
    {
        vector<MoveDemo> vec;
        vec.reserve(2);

        cout << "  push #1:\n";
        vec.push_back(MoveDemo("Alpha"));

        cout << "  push #2:\n";
        vec.push_back(MoveDemo("Beta"));

        cout << "  push #3（觸發擴容）:\n";
        vec.push_back(MoveDemo("Gamma"));
        // 因為有 noexcept → 搬移 Alpha 和 Beta 用移動而非拷貝
    }

    cout << "\n=== 重點 ===\n";
    cout << "  移動建構三步驟：偷指標 → 歸零源 → 複製基本型別\n";
    cout << "  拷貝 O(n)，移動 O(1)\n";
    cout << "  noexcept 讓 vector 擴容使用移動\n";
    cout << "  移動後的源物件處於有效但未指定狀態\n";

    // ========================================================================
    // 編譯期驗證：確認我們真的做對了
    // ========================================================================
    cout << "\n=== 編譯期驗證（static_assert）===\n";
    static_assert(std::is_nothrow_move_constructible<MoveDemo>::value,
                  "MoveDemo 的移動建構子必須是 noexcept，否則 vector 擴容會退回複製");
    cout << "  is_nothrow_move_constructible<MoveDemo> = "
         << std::is_nothrow_move_constructible<MoveDemo>::value << "\n";
    cout << "  ✅ 已在編譯期確認移動建構子標了 noexcept\n";
    cout << "  （若把 noexcept 拿掉，上面的 static_assert 會直接讓編譯失敗）\n";

    // ========================================================================
    // 日常實務：連線池
    // ========================================================================
    cout << "\n=== 日常實務：連線池（移動語意的典型用途）===\n";
    {
        ConnectionPool pool;

        // 建立連線後移入池中：不複製 handle，只移交所有權
        pool.add(Connection("db-primary", 101));
        pool.add(Connection("db-replica", 102));

        Connection spare("db-backup", 103);
        pool.add(std::move(spare));       // 明確交出所有權

        cout << "  spare 交出後的狀態: " << spare.describe()
             << "（有效但未指定 —— 只能重新賦值或銷毀）\n";

        pool.report();
        cout << "  全程沒有任何一個連線 handle 被複製 ——\n";
        cout << "  這正是移動語意在資源管理上的價值：所有權只有一個，且可安全移交。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o movector_summary

// === 預期輸出 ===
// ===== 1. 拷貝 vs 移動 =====
//   [建構] "Hello World"
//
//   拷貝建構（深拷貝）：
//   [拷貝建構💰] "Hello World" — new + copy
//
//   移動建構（偷指標）：
//   [移動建構⚡] "Hello World" — 只搬指標
//   a = "(null)" (已被掏空)
//   c = "Hello World"
//   [解構] "Hello World"
//   [解構] "Hello World"
//   [解構] (nullptr)
//
// ===== 2. 從臨時物件移動 =====
//   [建構] "Temporary"
//   d = "Temporary"
//   [解構] "Temporary"
//
// ===== 3. vector 擴容使用移動 =====
//   push #1:
//   [建構] "Alpha"
//   [移動建構⚡] "Alpha" — 只搬指標
//   [解構] (nullptr)
//   push #2:
//   [建構] "Beta"
//   [移動建構⚡] "Beta" — 只搬指標
//   [解構] (nullptr)
//   push #3（觸發擴容）:
//   [建構] "Gamma"
//   [移動建構⚡] "Gamma" — 只搬指標
//   [移動建構⚡] "Alpha" — 只搬指標
//   [解構] (nullptr)
//   [移動建構⚡] "Beta" — 只搬指標
//   [解構] (nullptr)
//   [解構] (nullptr)
//   [解構] "Alpha"
//   [解構] "Beta"
//   [解構] "Gamma"
//
// === 重點 ===
//   移動建構三步驟：偷指標 → 歸零源 → 複製基本型別
//   拷貝 O(n)，移動 O(1)
//   noexcept 讓 vector 擴容使用移動
//   移動後的源物件處於有效但未指定狀態
//
// === 編譯期驗證（static_assert）===
//   is_nothrow_move_constructible<MoveDemo> = 1
//   ✅ 已在編譯期確認移動建構子標了 noexcept
//   （若把 noexcept 拿掉，上面的 static_assert 會直接讓編譯失敗）
//
// === 日常實務：連線池（移動語意的典型用途）===
//   [開啟連線] db-primary (handle=101)
//   [移交連線] handle=101 已轉移
//   [解構] 已移交的空殼，無需關閉
//   [開啟連線] db-replica (handle=102)
//   [移交連線] handle=102 已轉移
//   [解構] 已移交的空殼，無需關閉
//   [開啟連線] db-backup (handle=103)
//   [移交連線] handle=103 已轉移
//   spare 交出後的狀態: (已移交，無 handle)（有效但未指定 —— 只能重新賦值或銷毀）
//   連線池目前持有 3 條連線：
//     db-primary/101
//     db-replica/102
//     db-backup/103
//   全程沒有任何一個連線 handle 被複製 ——
//   這正是移動語意在資源管理上的價值：所有權只有一個，且可安全移交。
//   [解構] 已移交的空殼，無需關閉
//   [關閉連線] db-primary (handle=101)
//   [關閉連線] db-replica (handle=102)
//   [關閉連線] db-backup (handle=103)
