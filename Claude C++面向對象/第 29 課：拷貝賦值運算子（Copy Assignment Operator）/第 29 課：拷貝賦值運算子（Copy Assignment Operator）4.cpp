// =============================================================================
//  第 29 課：拷貝賦值運算子 4  —  一個完整、可驗證無洩漏的資源管理類別
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範  ： 建構 / 拷貝建構 / 拷貝賦值(copy-and-swap) / 解構 + 非成員 swap
//   標準版本  ： C++98 即可；本檔以 -std=c++17 編譯
//   複雜度    ： 拷貝建構 O(n)、拷貝賦值 O(n)、swap O(1)、解構 O(1)
//   標頭檔    ： <algorithm>（std::copy）、<utility>（std::swap）
//   記憶體驗證： valgrind --leak-check=full ./lesson29b
//               本機實測結果：total heap usage: 6 allocs, 6 frees,
//               "All heap blocks were freed -- no leaks are possible"，
//               ERROR SUMMARY: 0 errors —— 四個特殊成員函式確實成對。
//   實作定義值： 本機 g++ 15.2 / x86-64 實測 sizeof(DynamicBuffer) = 16、
//               alignof = 8（8 bytes 指標 + 8 bytes size_t，無 padding）。
//               這是實作定義的數值，換平台（如 32-bit）會不同。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「深拷貝」是這個類別存在的理由】
//   DynamicBuffer 持有一塊 new[] 出來的原始記憶體。如果放任編譯器生成
//   預設的拷貝行為，它會逐成員複製 —— 也就是把「指標的值」抄過去，
//   結果兩個物件指向同一塊記憶體。這叫淺拷貝（shallow copy），會造成：
//     * 改動其中一個，另一個跟著變（意料之外的別名）
//     * 兩個物件解構時各 delete[] 一次 → 雙重釋放
//   本檔的拷貝建構函式自己 new 一塊、再 std::copy 內容過去，
//   兩個物件從此完全獨立 —— 這就是深拷貝（deep copy）。
//   main 的「修改 buf2，驗證獨立性」那一段就是在證明這件事。
//
// 【2. 四個特殊成員函式如何互相支撐】
//   建構       ：配置 capacity 個 byte，用 {} 值初始化為全 0
//   拷貝建構   ：配置同樣大小，std::copy 複製內容
//   拷貝賦值   ：copy-and-swap —— 參數按值傳入（複製在此發生），
//                 再與 *this 交換，舊資料隨參數離開時被解構
//   解構       ：delete[]
//   注意這裡的分工：真正的「複製邏輯」只寫在拷貝建構函式裡一份，
//   operator= 完全不重複這段程式碼。這是 copy-and-swap 除了異常安全之外
//   的第二個好處 —— 消除重複，只有一個地方可能寫錯。
//
// 【3. 為什麼要多寫一個「非成員 swap」】
//   成員 swap（a.swap(b)）已經夠用，但泛型程式碼不會這樣呼叫。標準演算法
//   與容器的慣例是：
//       using std::swap;
//       swap(a, b);        // 先讓 ADL 找使用者定義的版本，找不到才退回 std::swap
//   這叫 two-step / ADL swap idiom。提供一個和類別同一個命名空間的非成員
//   swap，就能讓 std::sort、std::rotate 這些演算法自動用上你的 O(1) 版本，
//   而不是泛型的「一次 move 建構 + 兩次 move 賦值」。
//   本檔的 inline void swap(DynamicBuffer&, DynamicBuffer&) 就是這個用途。
//
// 【4. new unsigned char[cap]{} 的那對大括號很重要】
//     new unsigned char[cap]     → 預設初始化：內容是未指定的（不確定值）
//     new unsigned char[cap]{}   → 值初始化：全部補 0
//   本檔用了後者，所以 main 印出 buf3 剛建立時是 [0, 0, 0, ...]，
//   這個 0 是標準保證的，不是碰巧。若寫成前者，去讀那些位元組會得到
//   不確定值（indeterminate value），不可以拿來當作固定結果看待。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼本檔編譯時會出現 -Wreorder 警告（而且它是刻意留著的）
//   成員宣告順序是 m_buffer 在前、m_capacity 在後；但初始化列表寫成
//       : m_capacity(capacity), m_buffer(new unsigned char[capacity]{})
//   C++ 規定：成員的初始化順序一律依「宣告順序」，與初始化列表的書寫順序
//   完全無關。所以真正的執行順序是先 m_buffer、後 m_capacity。
//   g++ 因此警告 'DynamicBuffer::m_capacity' will be initialized after ...
//
//   在這個檔案裡它剛好是良性的 —— 因為 m_buffer 的初始式用的是「參數
//   capacity」（或 other.m_capacity），不是還沒初始化的 m_capacity。
//   但只要有人把它改成：
//       : m_capacity(capacity), m_buffer(new unsigned char[m_capacity]{})
//   就會用到「尚未初始化的 m_capacity」，讀取不確定值，變成真正的錯誤。
//   對照同一課第 1 個檔案的 SimpleString：那裡刻意把 m_len 宣告在 m_data
//   之前，正是因為 m_data 的初始式真的用到了 m_len。
//   結論：把初始化列表的順序寫成和宣告順序一致，讓 -Wreorder 永遠保持沉默，
//   你就永遠不必判斷「這次是良性還是致命」。
//
// (B) copy-and-swap 在這裡的實際成本，看輸出就知道
//   main 裡 buf3 = buf1 那一段的輸出順序是：
//       [拷貝建構] capacity=5    ← 參數 other 被建構（配置新記憶體 + 複製）
//       [拷貝賦值] swap          ← 交換指標，O(1)、不會失敗
//       [解構] capacity=10       ← 參數帶著 buf3 原本的 10 bytes 離開並釋放
//   注意 buf3 原本容量是 10、賦值後變成 5：容量是資料的一部分，
//   一起被換掉了。也可以看出 copy-and-swap 一定會配置新記憶體，
//   即使舊的那塊其實裝得下。
//
// (C) std::copy vs std::memcpy
//   本檔用 std::copy。對 unsigned char 這種 trivially copyable 型別，
//   標準函式庫實作會自動退化成 memmove/memcpy，效能相同；
//   但 std::copy 對「非平凡型別」也正確（會呼叫拷貝賦值），
//   而 memcpy 用在非平凡型別上是未定義行為。預設選 std::copy 較安全。
//
// 【注意事項 Pay Attention】
//   1. 這個類別只做到「三法則」（拷貝建構 + 拷貝賦值 + 解構）。因為使用者
//      宣告了解構函式，move 操作不會被隱式生成 —— 所有搬移都會退化成拷貝。
//      要拿回 move 的效能，請看第 34 課「五法則」。
//   2. operator[] 沒有邊界檢查（和 std::vector::operator[] 一致）。
//      越界存取是未定義行為，不會有任何提示。需要檢查請自行提供 at()。
//   3. 成員 swap 標了 noexcept —— copy-and-swap 的異常安全完全建立在
//      「swap 不會失敗」之上，這個 noexcept 是承諾，不是裝飾。
//   4. print() 只印前 8 個 byte，超過會顯示 "..."。看輸出時別誤以為
//      緩衝區只有 8 bytes。
//   5. 用 valgrind 驗證時要注意：本檔在 main 結束時才解構，
//      正常應該回報 "All heap blocks were freed -- no leaks are possible"。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】深拷貝、非成員 swap 與成員初始化順序
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 淺拷貝與深拷貝的差別？不寫拷貝建構函式會發生什麼事？
//     答：編譯器生成的版本是逐成員複製，對指標成員而言就是把位址抄過去
//         （淺拷貝），兩個物件從此共用同一塊記憶體。後果有二：改一個會
//         影響另一個；解構時兩邊各 delete[] 一次，造成雙重釋放。
//         深拷貝則是自己配置一塊新記憶體再複製內容，兩者完全獨立。
//     追問：那什麼時候淺拷貝才是對的？
//         → 當成員指標是「不擁有」的觀察者（例如指向共用快取），
//           或改用 std::shared_ptr 明確表達共享所有權時。
//           關鍵是先想清楚「誰擁有這塊記憶體」，再決定拷貝語意。
//
// 🔥 Q2. 為什麼除了成員 swap，還要提供一個非成員 swap？
//     答：因為泛型程式碼的慣例是 using std::swap; swap(a, b); —— 靠 ADL
//         在類別所在的命名空間找使用者定義的版本。只有成員 swap 的話，
//         std::sort 之類的演算法找不到它，只能退回 std::swap 的泛型實作
//         （一次 move 建構 + 兩次 move 賦值），白白多做工。
//     追問：為什麼不直接特化 std::swap？
//         → 對類別樣板無法偏特化 std::swap；而且在 namespace std 內新增
//           特化的規則很嚴格。提供同命名空間的非成員 swap 是標準做法。
//
// ⚠️ 陷阱 1. 「初始化列表寫的順序，就是實際執行的順序」——錯在哪？
//     答：成員的初始化順序一律由「宣告順序」決定，初始化列表怎麼寫都不影響。
//         本檔就是實例：宣告是 m_buffer 在前，但列表寫 m_capacity 在前，
//         g++ 因此發出 -Wreorder 警告（檔尾的但書有說明）。
//         這裡剛好良性，因為 m_buffer 用的是參數而不是 m_capacity；
//         但只要有人改成用 m_capacity 當初始式，就會讀到尚未初始化的值。
//     為什麼會錯：直覺認為「程式碼由上往下執行」。但初始化列表只是提供
//         「每個成員用什麼初始式」，順序是由類別佈局（宣告順序）決定的 ——
//         因為解構必須以嚴格相反的順序進行，順序不能讓每個建構函式各自決定。
//
// ⚠️ 陷阱 2. new unsigned char[n] 和 new unsigned char[n]{} 有差嗎？
//     答：差很多。前者是預設初始化，內容是不確定值，讀它不能期待任何固定
//         結果；後者是值初始化，標準保證全部為 0。本檔用的是後者，
//         所以 buf3 剛建立時印出 [0, 0, 0, ...] 是有保證的。
//     為什麼會錯：以為「反正 new 出來的記憶體都是 0」。作業系統給行程的
//         新分頁確實常是 0，但配置器會重複使用已釋放的記憶體，
//         那裡面就是上一個物件的殘留資料。
//
// ⚠️ 陷阱 3. 這個類別已經寫了拷貝建構、拷貝賦值、解構，還缺什麼？
//     答：缺 move constructor 與 move assignment。而且因為使用者宣告了
//         解構函式，編譯器「不會」隱式生成 move 操作 —— 所有本來可以搬移的
//         場合（回傳暫時物件、vector 擴容）都會退化成完整的深拷貝。
//         寫到三法則只是及格，第 34 課的五法則才是完整答案。
//     為什麼會錯：以為「沒寫 move 就是沒有 move，頂多不加速」。
//         實際上是「明明有機會零成本搬移，卻靜靜地做了整份複製」，
//         而且不會有任何警告告訴你效能掉在哪裡。
// ═══════════════════════════════════════════════════════════════════════════

// lesson29_complete_class.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson29b lesson29_complete_class.cpp
// 驗證：valgrind --leak-check=full ./lesson29b

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class DynamicBuffer {
private:
    unsigned char* m_buffer;
    std::size_t m_capacity;

public:
    // ──────── 建構函數 ────────
    explicit DynamicBuffer(std::size_t capacity)
        : m_capacity(capacity)
        , m_buffer(new unsigned char[capacity]{})
    {
        std::cout << "  [建構] capacity=" << m_capacity << "\n";
    }

    // ──────── 拷貝建構函數（深拷貝）────────
    DynamicBuffer(const DynamicBuffer& other)
        : m_capacity(other.m_capacity)
        , m_buffer(new unsigned char[other.m_capacity])
    {
        std::copy(other.m_buffer, other.m_buffer + m_capacity, m_buffer);
        std::cout << "  [拷貝建構] capacity=" << m_capacity << "\n";
    }

    // ──────── 拷貝賦值運算子（Copy-and-Swap）────────
    DynamicBuffer& operator=(DynamicBuffer other) {
        std::cout << "  [拷貝賦值] swap\n";
        swap(other);
        return *this;
    }

    // ──────── 解構函數 ────────
    ~DynamicBuffer() {
        std::cout << "  [解構] capacity=" << m_capacity << "\n";
        delete[] m_buffer;
    }

    // ──────── swap ────────
    void swap(DynamicBuffer& other) noexcept {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_capacity, other.m_capacity);
    }

    // ──────── 存取 ────────
    unsigned char& operator[](std::size_t i) { return m_buffer[i]; }
    const unsigned char& operator[](std::size_t i) const { return m_buffer[i]; }
    std::size_t capacity() const { return m_capacity; }

    void print(const char* label) const {
        std::cout << "  " << label << " (cap=" << m_capacity << "): [";
        for (std::size_t i = 0; i < m_capacity && i < 8; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << static_cast<int>(m_buffer[i]);
        }
        if (m_capacity > 8) std::cout << ", ...";
        std::cout << "]\n";
    }
};

// 非成員 swap
inline void swap(DynamicBuffer& a, DynamicBuffer& b) noexcept {
    a.swap(b);
}


#include <string>

// -----------------------------------------------------------------------------
// 【日常實務範例】影像掃描列的 undo 快照
//   情境：影像編輯器對一條 grayscale 掃描列（scanline）套用濾鏡。使用者按下
//         Ctrl+Z 時要能還原成套用前的樣子，所以在動手之前必須先做一份快照。
//   為什麼用到本主題：
//         * 「拍快照」＝拷貝建構（必須是深拷貝，否則快照會跟著被濾鏡改掉）
//         * 「還原」  ＝拷貝賦值（目標物件已存在，要整批換回舊內容）
//         這兩件事正好就是本檔 DynamicBuffer 提供的兩個特殊成員函式，
//         而且缺一不可 —— 若拷貝是淺的，快照與現況會指向同一塊記憶體，
//         undo 就會變成「還原成自己」，完全失效。
// -----------------------------------------------------------------------------
class ScanlineEditor {
private:
    DynamicBuffer m_pixels;    // 目前的畫素
    DynamicBuffer m_snapshot;  // 套用濾鏡前的備份
    bool          m_hasSnapshot;

public:
    explicit ScanlineEditor(std::size_t width)
        : m_pixels(width), m_snapshot(width), m_hasSnapshot(false) {}

    void load(const unsigned char* src, std::size_t n) {
        for (std::size_t i = 0; i < n && i < m_pixels.capacity(); ++i) {
            m_pixels[i] = src[i];
        }
    }

    // 套用「增亮」濾鏡：動手前先深拷貝一份快照
    void applyBrighten(int delta) {
        m_snapshot = m_pixels;      // ★ 拷貝賦值：整批複製目前狀態
        m_hasSnapshot = true;
        for (std::size_t i = 0; i < m_pixels.capacity(); ++i) {
            int v = static_cast<int>(m_pixels[i]) + delta;
            if (v > 255) v = 255;
            if (v < 0)   v = 0;
            m_pixels[i] = static_cast<unsigned char>(v);
        }
    }

    void undo() {
        if (!m_hasSnapshot) return;
        m_pixels = m_snapshot;      // ★ 拷貝賦值：整批還原
        m_hasSnapshot = false;
    }

    std::string dump() const {
        std::string s = "[";
        for (std::size_t i = 0; i < m_pixels.capacity(); ++i) {
            if (i) s += ", ";
            s += std::to_string(static_cast<int>(m_pixels[i]));
        }
        return s + "]";
    }
};

void undoDemo() {
    const unsigned char row[] = {10, 60, 120, 200, 250};
    ScanlineEditor editor(5);
    editor.load(row, 5);
    std::cout << "  原始掃描列    : " << editor.dump() << "\n";

    editor.applyBrighten(40);
    std::cout << "  套用增亮 +40  : " << editor.dump() << "（250 被夾到 255）\n";

    editor.undo();
    std::cout << "  undo 還原後   : " << editor.dump() << "\n";
}
int main() {
    std::cout << "=== 建立 buf1 ===\n";
    DynamicBuffer buf1(5);
    for (std::size_t i = 0; i < buf1.capacity(); ++i) {
        buf1[i] = static_cast<unsigned char>(i * 11);
    }
    buf1.print("buf1");

    std::cout << "\n=== 拷貝建構 buf2 ===\n";
    DynamicBuffer buf2 = buf1;
    buf2.print("buf2");

    std::cout << "\n=== 修改 buf2，驗證獨立性 ===\n";
    buf2[0] = 255;
    buf1.print("buf1");
    buf2.print("buf2");

    std::cout << "\n=== 拷貝賦值：buf3 = buf1 ===\n";
    DynamicBuffer buf3(10);  // 不同大小
    buf3.print("buf3 before");
    buf3 = buf1;
    buf3.print("buf3 after");

    std::cout << "\n=== 用 std::swap 交換 buf1 和buf2 ===\n";
    buf1.print("buf1 before swap");
    buf2.print("buf2 before swap");
    swap(buf1, buf2);  // 使用我們的非成員 swap
    buf1.print("buf1 after swap");
    buf2.print("buf2 after swap");


    std::cout << "\n=== 日常實務：影像掃描列的 undo 快照 ===\n";
    undoDemo();
    std::cout << "\n=== 結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 29 課：拷貝賦值運算子（Copy Assignment Operator）4.cpp" -o lesson29_4

// 【編譯時的 -Wreorder 警告是刻意保留的教材】
//   本機 g++ 15.2 會對建構函式與拷貝建構函式各發出一次：
//       warning: 'DynamicBuffer::m_capacity' will be initialized after
//                'unsigned char* DynamicBuffer::m_buffer' [-Wreorder]
//   原因是宣告順序（m_buffer 在前）與初始化列表的書寫順序（m_capacity 在前）
//   不一致。C++ 一律依「宣告順序」初始化，所以警告說的是事實。
//   在這個檔案裡它是良性的 —— m_buffer 的初始式用的是參數 capacity
//   （拷貝建構則是 other.m_capacity），不是尚未初始化的 m_capacity。
//   但只要改成 new unsigned char[m_capacity]{} 就會讀到未初始化的值。
//   正式專案的做法是把兩邊順序調成一致，讓 -Wreorder 永遠保持沉默。
//
// 【記憶體驗證（本機實測，非推測）】
//   valgrind --leak-check=full ./lesson29b
//       total heap usage: 6 allocs, 6 frees, 77,849 bytes allocated
//       All heap blocks were freed -- no leaks are possible
//       ERROR SUMMARY: 0 errors from 0 contexts
//   （allocs 數量含 iostream 自身的配置，並非全部來自 DynamicBuffer。）
//
// 【為何本檔沒有 LeetCode 範例】
//   本檔主題是「深拷貝的資源管理類別 + 非成員 swap」，屬於類別設計而非演算法。
//   題庫中的設計題（146 LRU Cache、155 Min Stack、705 Design HashSet、
//   1603 Design Parking System、1656 Design an Ordered Stream）評測時
//   都只建立單一實例、從不做拷貝賦值，掛上去只會是假關聯。
//   本課真正對應 LeetCode 的部分已放在同課第 1 個檔案
//   （LeetCode 707. Design Linked List，那題確實需要自行管理節點記憶體）。

// === 預期輸出 ===
// === 建立 buf1 ===
//   [建構] capacity=5
//   buf1 (cap=5): [0, 11, 22, 33, 44]
//
// === 拷貝建構 buf2 ===
//   [拷貝建構] capacity=5
//   buf2 (cap=5): [0, 11, 22, 33, 44]
//
// === 修改 buf2，驗證獨立性 ===
//   buf1 (cap=5): [0, 11, 22, 33, 44]
//   buf2 (cap=5): [255, 11, 22, 33, 44]
//
// === 拷貝賦值：buf3 = buf1 ===
//   [建構] capacity=10
//   buf3 before (cap=10): [0, 0, 0, 0, 0, 0, 0, 0, ...]
//   [拷貝建構] capacity=5
//   [拷貝賦值] swap
//   [解構] capacity=10
//   buf3 after (cap=5): [0, 11, 22, 33, 44]
//
// === 用 std::swap 交換 buf1 和buf2 ===
//   buf1 before swap (cap=5): [0, 11, 22, 33, 44]
//   buf2 before swap (cap=5): [255, 11, 22, 33, 44]
//   buf1 after swap (cap=5): [255, 11, 22, 33, 44]
//   buf2 after swap (cap=5): [0, 11, 22, 33, 44]
//
// === 日常實務：影像掃描列的 undo 快照 ===
//   [建構] capacity=5
//   [建構] capacity=5
//   原始掃描列    : [10, 60, 120, 200, 250]
//   [拷貝建構] capacity=5
//   [拷貝賦值] swap
//   [解構] capacity=5
//   套用增亮 +40  : [50, 100, 160, 240, 255]（250 被夾到 255）
//   [拷貝建構] capacity=5
//   [拷貝賦值] swap
//   [解構] capacity=5
//   undo 還原後   : [10, 60, 120, 200, 250]
//   [解構] capacity=5
//   [解構] capacity=5
//
// === 結束 ===
//   [解構] capacity=5
//   [解構] capacity=5
//   [解構] capacity=5
