// =============================================================================
//  第 30 課 summary.cpp  —  三法則（Rule of Three）
// =============================================================================
//
// 【主題資訊 Information】
//   三法則   ： 只要自訂了以下任何一個，通常三個都要自訂
//                ① 解構函數        ~T()
//                ② 拷貝建構函數    T(const T&)
//                ③ 拷貝賦值運算子  T& operator=(const T&)
//   標準版本 ： C++98 起的經典守則；C++11 之後擴充為「五法則」（第 34 課）
//   相關守則 ： Rule of Zero（不自訂任何一個，把資源交給 string／vector／
//              unique_ptr 等已經寫對的型別去管）
//   標頭檔   ： <cstring>（strlen/strcpy）、<utility>（std::swap）
//   複雜度   ： 深拷貝 O(n)、swap O(1)
//   實作定義 ： 本機 g++ 15.2 / x86-64 上 sizeof(ManagedString) = 24
//              （size_t 8 + 指標 8 + 教學用序號 4 + padding 4）
//
// 【詳細解釋 Explanation】
//
// 【1. 三法則到底在說什麼】
//   它不是「規定你要寫三個函式」，而是一個推論鏈：
//       你自訂了解構函數
//         → 代表這個類別「擁有」某種需要親手釋放的資源
//           （new 的記憶體、FILE*、socket、mutex lock、GPU buffer…）
//         → 而編譯器生成的拷貝是「逐成員複製」，對指標就是複製位址
//         → 於是兩個物件會同時擁有同一份資源
//         → 解構時各釋放一次 → 雙重釋放；改動其一 → 另一個莫名其妙跟著變
//       所以：一旦需要自訂其中一個，另外兩個的預設版本幾乎必然是錯的。
//   反過來也成立：如果你沒有自訂解構函數，通常也不需要自訂另外兩個。
//
// 【2. 判斷準則：問「這個類別擁有什麼？」】
//   需要三法則： 成員裡有裸指標 new/malloc、FILE*、int fd、HANDLE、
//                或任何「需要成對釋放」的東西。
//   不需要（Rule of Zero）： 成員只有 std::string、std::vector、
//                std::unique_ptr、std::shared_ptr —— 這些型別自己已經
//                把三／五法則寫對了，你只要不去干擾它們就好。
//   實務上第一選擇永遠是 Rule of Zero：不要自己管資源，讓已經寫對的型別管。
//   本課手寫 ManagedString 是為了「看清楚裡面發生什麼事」，
//   不是建議你在正式專案裡這樣寫字串。
//
// 【3. 三個函式各自的正確寫法】
//   ① 解構函數：delete[] 對應 new[]、delete 對應 new，絕不能配錯。
//   ② 拷貝建構：配置一塊自己的資源，再把內容複製過來（深拷貝）。
//      注意它是「從無到有」，不需要處理舊資源。
//   ③ 拷貝賦值：目標已經存在，所以要先處理掉自己的舊資源。
//      推薦用 copy-and-swap（本檔採用）：參數按值傳入，
//      複製在參數初始化階段就完成，函式本體只做不會失敗的 swap。
//      這同時解決了自我賦值與例外安全（詳見第 29 課）。
//
// 【4. 三法則的第三個選項：明確禁止拷貝】
//   很多人以為三法則只有「全部自己寫」一條路。其實對某些資源而言，
//   「深拷貝」根本沒有意義 —— 複製一個已開啟的檔案 handle 要複製成什麼？
//   複製一個 socket 連線？這種情況正確答案是：
//       T(const T&)            = delete;
//       T& operator=(const T&) = delete;
//   明確宣告「這個型別不可拷貝」，讓誤用在編譯期就被擋下來，
//   而不是在執行期變成雙重關閉。本檔下方的 FileHandle 實務範例
//   示範的就是這個做法。
//
// 【5. 為什麼本檔改用「緩衝區序號」而不是印位址】
//   原始版本印的是 static_cast<void*>(m_data)，也就是原始位址。
//   問題是位址受 ASLR（位址空間配置隨機化）影響，每次執行都不同 ——
//   本機實測連跑三次得到三組完全不同的位址，輸出無法重現。
//   把「無法重現的位址」貼進教材當預期輸出，那就是假資料。
//   所以本檔改成：每配置一塊緩衝區就給一個遞增序號（#1、#2…），
//   swap 時序號跟著緩衝區走。這樣既保留了「追蹤是哪一塊記憶體」的教學價值，
//   輸出又完全可重現。要驗證「是不是同一塊」時，
//   則直接印指標比較的布林值（true/false），而不是印位址本身。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 淺拷貝造成的雙重釋放，為什麼特別難除錯
//   兩個物件共用一塊記憶體時，第一次 delete[] 通常「看起來沒事」，
//   問題會在第二次 delete[] 才爆發 —— 而那時堆疊追蹤指向的是
//   「第二個物件的解構函數」，跟真正的錯誤（少寫了拷貝建構函數）
//   隔了十萬八千里。更糟的是它可能在測試環境完全不重現。
//   這就是為什麼三法則是守則而不是建議：靠除錯抓它太貴了。
//
// (B) 按值傳參與按值回傳，會分別發生什麼
//   本檔測試 5 用 processByValue(a)：參數是按值的，所以進入函式前
//   會呼叫拷貝建構函數複製一份，函式結束時副本解構 —— 輸出裡看得到。
//   測試 6 的 createString() 按值回傳，輸出裡卻「沒有」多餘的拷貝：
//   那是因為具名回傳值最佳化（NRVO）把 local 直接建構在呼叫端的位置。
//   NRVO 在 C++17 仍屬「允許但不強制」的最佳化（真正被標準保證的是
//   回傳 prvalue 的情形）；本機 g++ 15.2 實測有生效。
//   這也說明：想靠數建構／解構次數來理解程式，一定要實際跑過。
//
// (C) sizeof 與成員排列
//   本機實測 sizeof(ManagedString) = 24：
//       std::size_t m_len   8 bytes
//       char*       m_data  8 bytes
//       unsigned    m_tag   4 bytes（教學用序號）
//       padding             4 bytes（為了讓整體對齊到 8）
//   static 成員 s_allocSerial 不佔物件空間 —— 它屬於類別，不屬於實例。
//   這些數值都是實作定義的，換平台（例如 32-bit）會不同。
//
// (D) 三法則 → 五法則
//   C++11 引入 move 語意後，完整的規則是五法則：再加上
//   move constructor 與 move assignment。而且有個關鍵副作用：
//   「只要你自訂了解構函數，編譯器就不會生成 move 操作」——
//   本檔的 ManagedString 正是如此，所有搬移場合都會靜靜退化成深拷貝，
//   不會有任何警告。第 34 課會處理這件事。
//
// 【注意事項 Pay Attention】
//   1. new[] 必須配 delete[]，new 必須配 delete。配錯是未定義行為，
//      不保證任何固定結果（可能看似正常、可能記憶體損毀）。
//   2. 拷貝賦值務必處理自我賦值。copy-and-swap 天生免疫；
//      若寫傳統版本（先 delete 再 new），少了自我賦值檢查就會讀到已釋放記憶體。
//   3. swap 要標 noexcept —— copy-and-swap 的例外安全完全建立在這個承諾上。
//   4. 自訂解構函數會讓 move 操作不被生成，導致效能靜默退化。
//      要嘛補上五法則，要嘛改用 Rule of Zero。
//   5. 不要為了「示範」而在正式專案裡手寫這種字串類別。
//      正式做法是 std::string；本課的價值在於看懂它內部在做什麼。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三法則（Rule of Three）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是三法則？為什麼這三個函式要綁在一起？
//     答：只要自訂了解構函數、拷貝建構函數、拷貝賦值運算子其中之一，
//         通常三個都要自訂。推論鏈是：需要自訂解構 → 代表類別擁有資源 →
//         編譯器的逐成員拷貝會讓兩個物件共用同一份資源 →
//         雙重釋放與意外別名。所以另外兩個的預設版本必然是錯的。
//     追問：那什麼時候三個都不用寫？
//         → Rule of Zero：成員全部是 std::string／vector／unique_ptr
//           這類已經自己管好資源的型別，此時預設版本全部正確。
//
// 🔥 Q2. 只寫解構函數、不寫拷貝建構，會發生什麼具體後果？
//     答：編譯器生成的拷貝建構會逐成員複製，對指標成員就是複製位址。
//         兩個物件指向同一塊記憶體：改一個另一個跟著變；
//         解構時各 delete[] 一次 → 雙重釋放（未定義行為）。
//         而且第一次釋放通常不會報錯，錯誤在第二次才爆，
//         堆疊指向的位置跟真正的原因毫無關係。
//     追問：如果這個資源本來就不該被複製呢？
//         → 那就把兩個拷貝操作 = delete，讓誤用在編譯期就失敗。
//           這是三法則的合法解，而且往往是最正確的解。
//
// 🔥 Q3. Rule of Three 和 Rule of Five 差在哪？只寫三法則有什麼代價？
//     答：五法則多了 move constructor 與 move assignment。代價很具體：
//         只要你「自訂了解構函數」，編譯器就不再隱式生成 move 操作，
//         所有本來可以零成本搬移的場合（回傳暫時物件、vector 擴容）
//         都會退化成完整深拷貝，而且沒有任何警告告訴你。
//     追問：怎麼確認自己的類別到底有沒有 move？
//         → static_assert(std::is_move_constructible_v<T>) 只能看「可不可以
//           用右值建構」（拷貝建構也能接右值，所以會是 true）；
//           要確認是否「真的走 move」，最直接的方式是在 move 建構函式裡
//           印一行字，或用 std::is_nothrow_move_constructible 觀察差異。
//
// ⚠️ 陷阱 1. 「我的類別只有一個 char* 成員，複製一下應該沒事」——錯在哪？
//     答：只要那塊記憶體是你 new 出來、要你 delete 的，複製指標就等於
//         製造出兩個「擁有者」。誰該負責釋放？兩個都做 → 雙重釋放；
//         都不做 → 洩漏。所有權必須是明確的，這正是三法則要處理的問題。
//     為什麼會錯：把指標成員想成「只是一個位址」，忽略它同時攜帶了
//         「所有權」這個看不見的責任。C++ 不會替你追蹤所有權，
//         那是型別設計者的工作。
//
// ⚠️ 陷阱 2. 「輸出裡印出物件位址就能看出有沒有深拷貝」——為什麼不該寫進教材？
//     答：位址受 ASLR 影響，每次執行都不同（本機實測連跑三次得到三組
//         不同位址）。拿它當「預期輸出」等於寫下永遠對不上的假資料。
//         要驗證是否共用同一塊，正確做法是印「指標是否相等」的布林值，
//         或用自己維護的、由程式邏輯決定的序號。
//     為什麼會錯：把「除錯時肉眼看位址」的習慣直接搬到「可重現的輸出」上。
//         除錯時看位址完全合理，但那是一次性的觀察，不是可寫入文件的事實。
//
// ⚠️ 陷阱 3. 「按值回傳一個大物件一定會多一次拷貝，所以要回傳指標」——過時了。
//     答：本檔測試 6 按值回傳 ManagedString，輸出裡並沒有多餘的拷貝建構，
//         因為編譯器套用了具名回傳值最佳化（NRVO）把 local 直接建構在
//         呼叫端。回傳 prvalue 的情形在 C++17 更是被標準保證免除複製。
//         為了「避免拷貝」而回傳裸指標，只會製造所有權不清的問題。
//     為什麼會錯：停留在 C++98 的心智模型。現代 C++ 的正確直覺是
//         「按值回傳，讓編譯器處理」，需要時再用 move。
// ═══════════════════════════════════════════════════════════════════════════

// ============================================================
// 第 30 課 總結：三法則（Rule of Three）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【三法則】如果類別需要自訂以下任何一個，通常三個都需要自訂：
//   1. 解構函數（Destructor）           ~ClassName()
//   2. 拷貝建構函數（Copy Constructor）  ClassName(const ClassName&)
//   3. 拷貝賦值運算子（Copy Assignment） ClassName& operator=(const ClassName&)
//
// 【為什麼？】
//   需要自訂解構函數 → 表示類別管理了動態資源（new 出來的記憶體）
//   → 管理動態資源 → 預設的淺拷貝不安全
//   → 所以拷貝建構和拷貝賦值也必須自訂（做深拷貝）
//
// 【判斷依據】
//   類別有 new/malloc/file handle → 需要三法則
//   類別只用 string/vector/unique_ptr → 不需要（Rule of Zero）
//
// 【推薦實作模式】
//   解構函數：delete 資源
//   拷貝建構：new + 複製
//   拷貝賦值：Copy-and-Swap 慣用法
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>

// ============================================================
// 完整的 Rule of Three 示範：ManagedString
// ============================================================
class ManagedString {
    // ⚠️ 成員的初始化順序由「宣告順序」決定，與初始化列表的書寫順序無關。
    //    m_data 的初始值用到了 m_len，所以 m_len 必須宣告在 m_data 之前，
    //    否則 m_data 會用到尚未初始化的 m_len（未定義行為）。
    std::size_t m_len;
    char* m_data;
    // 教學用：每配置一塊新緩衝區就給它一個遞增的「配置序號」。
    // 用序號而不是原始位址來標示身分，是為了讓輸出可重現 ——
    // 位址受 ASLR 影響，每次執行都不同（實測連跑三次得到三組不同位址），
    // 貼進教材就成了對不上的假資料。序號則完全由程式邏輯決定。
    unsigned m_tag;
    static unsigned s_allocSerial;

public:
    // 一般建構函數
    ManagedString(const char* str = "")
        : m_len(std::strlen(str)), m_data(new char[m_len + 1]),
          m_tag(++s_allocSerial) {
        std::strcpy(m_data, str);
        std::cout << "  [建構] \"" << m_data << "\" (緩衝區 #" << m_tag << ")\n";
    }

    // ═══════════════════════════════════════
    // ★ Rule of Three：以下三個必須同時存在 ★
    // ═══════════════════════════════════════

    // ① 解構函數 — 釋放動態資源
    ~ManagedString() {
        std::cout << "  [解構] \"" << (m_data ? m_data : "nullptr")
                  << "\" (緩衝區 #" << m_tag << ")\n";
        delete[] m_data;
    }

    // ② 拷貝建構函數 — 深拷貝
    ManagedString(const ManagedString& other)
        : m_len(other.m_len), m_data(new char[m_len + 1]),
          m_tag(++s_allocSerial) {
        std::strcpy(m_data, other.m_data);
        std::cout << "  [拷貝建構] \"" << m_data << "\" (緩衝區 #" << m_tag
                  << " ← 複製自 #" << other.m_tag << ")\n";
    }

    // ③ 拷貝賦值運算子 — Copy-and-Swap
    void swap(ManagedString& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_len, other.m_len);
        std::swap(m_tag,  other.m_tag);   // 序號跟著緩衝區走
    }

    ManagedString& operator=(ManagedString other) {  // 傳值
        std::cout << "  [拷貝賦值] swap\n";
        swap(other);
        return *this;
    }

    // ═══════════════════════════════════════
    // ★ Rule of Three 結束 ★
    // ═══════════════════════════════════════

    const char* c_str() const { return m_data; }
    std::size_t length() const { return m_len; }
};

// 靜態成員定義（教學用的配置序號產生器）
unsigned ManagedString::s_allocSerial = 0;

// 非成員 swap
inline void swap(ManagedString& a, ManagedString& b) noexcept { a.swap(b); }

// 按值傳參 → 觸發拷貝建構
void processByValue(ManagedString s) {
    std::cout << "    processByValue: \"" << s.c_str() << "\"\n";
}

// 按值回傳
ManagedString createString(const char* text) {
    ManagedString local(text);
    return local;
}

#include <cstdio>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，自行設計 MyHashSet，支援 add / remove / contains。
//   為什麼用到本主題：標準解法是「bucket 陣列 + 鏈結串列處理碰撞」，
//         也就是同時擁有「一個 new[] 出來的桶陣列」和「一堆 new 出來的節點」。
//         這正是三法則的教科書條件：需要自訂解構函數 → 就必須自訂拷貝建構
//         與拷貝賦值，否則兩份 MyHashSet 會共用同一批節點。
//   誠實說明：LeetCode 的評測程式只建立一個實例、從不拷貝它，
//         所以不寫三法則也能 AC。但這正是危險之處 —— 這個型別「看起來」
//         可以拷貝（編譯得過），實際上一拷貝就是雙重釋放。
//         下面補齊三法則，讓它成為真正安全的型別。
// -----------------------------------------------------------------------------
class MyHashSet {
private:
    struct Node {
        int   key;
        Node* next;
        Node(int k, Node* n) : key(k), next(n) {}
    };
    static const std::size_t kBuckets = 769;   // 質數，減少碰撞
    Node** m_buckets;

    static std::size_t hashOf(int key) {
        return static_cast<std::size_t>(key) % kBuckets;
    }

public:
    MyHashSet() : m_buckets(new Node*[kBuckets]()) {}   // () → 全部初始化為 nullptr

    // ① 解構函數：先釋放每條鏈上的節點，再釋放桶陣列本身
    ~MyHashSet() {
        for (std::size_t i = 0; i < kBuckets; ++i) {
            Node* p = m_buckets[i];
            while (p) { Node* nxt = p->next; delete p; p = nxt; }
        }
        delete[] m_buckets;
    }

    // ② 拷貝建構：逐桶、逐節點深拷貝（保持每條鏈的原始順序）
    MyHashSet(const MyHashSet& other) : m_buckets(new Node*[kBuckets]()) {
        for (std::size_t i = 0; i < kBuckets; ++i) {
            Node** tail = &m_buckets[i];
            for (Node* p = other.m_buckets[i]; p; p = p->next) {
                *tail = new Node(p->key, nullptr);
                tail = &((*tail)->next);
            }
        }
    }

    void swap(MyHashSet& other) noexcept { std::swap(m_buckets, other.m_buckets); }

    // ③ 拷貝賦值：copy-and-swap
    MyHashSet& operator=(MyHashSet other) {
        swap(other);
        return *this;
    }

    void add(int key) {
        if (contains(key)) return;
        std::size_t h = hashOf(key);
        m_buckets[h] = new Node(key, m_buckets[h]);
    }

    void remove(int key) {
        Node** cur = &m_buckets[hashOf(key)];
        while (*cur) {
            if ((*cur)->key == key) {
                Node* dead = *cur;
                *cur = dead->next;
                delete dead;
                return;
            }
            cur = &((*cur)->next);
        }
    }

    bool contains(int key) const {
        for (Node* p = m_buckets[hashOf(key)]; p; p = p->next) {
            if (p->key == key) return true;
        }
        return false;
    }
};

void leetcode705Demo() {
    MyHashSet s;
    s.add(1);
    s.add(2);
    std::cout << "  contains(1) = " << std::boolalpha << s.contains(1) << "\n";
    std::cout << "  contains(3) = " << s.contains(3) << "\n";
    s.add(2);                      // 重複加入不應改變結果
    s.remove(2);
    std::cout << "  remove(2) 後 contains(2) = " << s.contains(2) << "\n";

    // ★ 三法則的價值：拷貝一份快照，兩者從此互不影響
    s.add(100);
    MyHashSet snapshot = s;        // 拷貝建構（深拷貝整個桶陣列與所有節點）
    s.remove(100);                 // 只動原本那份
    std::cout << "  原集合 contains(100)   = " << s.contains(100) << "\n";
    std::cout << "  快照   contains(100)   = " << snapshot.contains(100)
              << "（不受影響 → 深拷貝成功）\n";

    MyHashSet restored;
    restored.add(999);
    restored = snapshot;           // 拷貝賦值（copy-and-swap）
    std::cout << "  還原後 contains(100)   = " << restored.contains(100) << "\n";
    std::cout << "  還原後 contains(999)   = " << restored.contains(999)
              << "（舊內容已被整批換掉）\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】三法則的第三個選項：明確禁止拷貝
//   情境：包裝一個 FILE*，離開作用域自動關檔（RAII）。
//   為什麼用到本主題：它有自訂解構函數（要 fclose），所以三法則被觸發。
//         但這裡「深拷貝」根本沒有意義 —— 複製一個已開啟的檔案 handle
//         要複製成什麼？重新開一次？共用同一個讀寫位置？兩者都不對。
//   正確答案：把兩個拷貝操作 = delete。三法則從來不是「一定要寫三個函式」，
//         而是「這三件事你必須做出明確決定」，而「禁止」是完全合法的決定。
//         如果放著不管，編譯器會生成逐成員拷貝 → 兩個物件持有同一個 FILE*
//         → 各自 fclose 一次 → 對同一個 handle 關閉兩次（未定義行為）。
// -----------------------------------------------------------------------------
class FileHandle {
private:
    std::FILE* m_fp;

public:
    // 用 std::tmpfile()：建立一個匿名暫存檔，關閉時由系統自動刪除，
    // 不會在專案目錄留下任何檔案，也不需要挑路徑。
    FileHandle() : m_fp(std::tmpfile()) {}

    ~FileHandle() {
        if (m_fp) std::fclose(m_fp);
    }

    // ★ 明確禁止拷貝：誤用會在編譯期就失敗，而不是執行期雙重 fclose
    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    bool valid() const { return m_fp != nullptr; }

    bool writeLine(const char* text) {
        if (!m_fp) return false;
        return std::fprintf(m_fp, "%s\n", text) >= 0;
    }

    // 回到檔頭，讀回第一行（示範這個 handle 確實可用）
    std::string readFirstLine() {
        if (!m_fp) return "";
        std::rewind(m_fp);
        char buf[128] = {0};
        if (!std::fgets(buf, sizeof(buf), m_fp)) return "";
        std::string s(buf);
        if (!s.empty() && s.back() == '\n') s.pop_back();
        return s;
    }
};

// 由編譯器背書：這個型別確實不可拷貝
static_assert(!std::is_copy_constructible<FileHandle>::value,
              "FileHandle 必須不可拷貝（handle 沒有合理的深拷貝語意）");
static_assert(!std::is_copy_assignable<FileHandle>::value,
              "FileHandle 必須不可拷貝賦值");

void fileHandleDemo() {
    FileHandle f;
    std::cout << "  暫存檔開啟成功？ " << std::boolalpha << f.valid() << "\n";
    f.writeLine("audit: user=alice action=login");
    std::cout << "  讀回內容: \"" << f.readFirstLine() << "\"\n";
    std::cout << "  is_copy_constructible<FileHandle> = "
              << std::is_copy_constructible<FileHandle>::value
              << "（= delete → 誤用在編譯期就被擋下）\n";
    // FileHandle g = f;   // ← 若取消註解，編譯期就會失敗：use of deleted function
}
int main() {
    // ============================================================
    // 測試 1：基本生命週期
    // ============================================================
    std::cout << "===== 測試 1：基本生命週期 =====\n";
    {
        ManagedString s("Hello");
        std::cout << "  s = \"" << s.c_str() << "\"\n";
    } // s 離開作用域 → 解構
    std::cout << "\n";

    // ============================================================
    // 測試 2：拷貝建構（深拷貝）
    // ============================================================
    std::cout << "===== 測試 2：拷貝建構 =====\n";
    {
        ManagedString a("Dragon");
        ManagedString b = a;  // 拷貝建構
        // 不印原始位址（每次執行都不同、無法重現），改印「是不是同一塊」
        std::cout << "  a 與 b 共用同一塊緩衝區？ "
                  << std::boolalpha << (a.c_str() == b.c_str()) << "\n";
        std::cout << "  （false → 兩者各自持有一塊 → 深拷貝成功）\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 3：拷貝賦值（Copy-and-Swap）
    // ============================================================
    std::cout << "===== 測試 3：拷貝賦值 =====\n";
    {
        ManagedString a("Knight");
        ManagedString b("Wizard");
        std::cout << "  賦值前：a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
        b = a;
        std::cout << "  賦值後：a=\"" << a.c_str() << "\" b=\"" << b.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 4：自我賦值安全性
    // ============================================================
    std::cout << "===== 測試 4：自我賦值 =====\n";
    {
        ManagedString a("Phoenix");
        a = a;  // Copy-and-Swap 自動處理
        std::cout << "  a=\"" << a.c_str() << "\"（安全）\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 5：按值傳參 & 回傳
    // ============================================================
    std::cout << "===== 測試 5：按值傳參 =====\n";
    {
        ManagedString a("Rogue");
        processByValue(a);
        std::cout << "  返回後 a=\"" << a.c_str() << "\"（不受影響）\n";
    }
    std::cout << "\n";

    std::cout << "===== 測試 6：按值回傳 =====\n";
    {
        ManagedString result = createString("Summoned");
        std::cout << "  result=\"" << result.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 測試 7：鏈式賦值
    // ============================================================
    std::cout << "===== 測試 7：鏈式賦值 =====\n";
    {
        ManagedString a("A"), b("B"), c("C");
        c = b = a;
        std::cout << "  a=\"" << a.c_str() << "\" b=\"" << b.c_str()
                  << "\" c=\"" << c.c_str() << "\"\n";
    }
    std::cout << "\n";

    // ============================================================
    // 重點整理
    // ============================================================
    // ============================================================
    // 測試 8：LeetCode 705. Design HashSet（需要三法則的真實案例）
    // ============================================================
    std::cout << "===== 測試 8：LeetCode 705. Design HashSet =====\n";
    leetcode705Demo();
    std::cout << "\n";

    // ============================================================
    // 測試 9：日常實務 — 三法則的第三個選項「明確禁止拷貝」
    // ============================================================
    std::cout << "===== 測試 9：RAII FileHandle（禁止拷貝）=====\n";
    fileHandleDemo();
    std::cout << "\n";
    std::cout << "=== 三法則速查 ===\n";
    std::cout << "  需要自訂解構函數？ → 也需要拷貝建構 + 拷貝賦值\n";
    std::cout << "  判斷：類別是否管理動態資源？\n";
    std::cout << "    是 → 實作三法則（解構 + 拷貝建構 + 拷貝賦值）\n";
    std::cout << "    否（只用 string/vector/unique_ptr）→ Rule of Zero\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 【本檔對原始版本做的一項修改，以及為什麼】
//   原版在建構／解構訊息中印出 static_cast<void*>(m_data)，也就是原始位址。
//   位址受 ASLR 影響，每次執行都不同（本機實測連跑三次得到三組不同位址），
//   輸出無法重現，貼進教材就是永遠對不上的假資料。
//   本檔改為：每配置一塊緩衝區給一個遞增序號（#1、#2…），swap 時序號跟著
//   緩衝區走；要驗證「是否共用同一塊」時改印指標比較的布林值。
//   教學價值完全保留（仍然看得出哪一塊被複製、被交換、被釋放），
//   而下方的預期輸出現在每次執行都完全相同。
//
// 【記憶體驗證（本機實測）】
//   valgrind --leak-check=full ./summary
//       in use at exit: 0 bytes in 0 blocks
//       All heap blocks were freed -- no leaks are possible
//       ERROR SUMMARY: 0 errors from 0 contexts
//   三法則寫對的直接證據：包含 MyHashSet 的 769 個桶與所有節點在內，
//   全部配置都有對應的釋放。
//
// 【關於測試 6「按值回傳」為什麼看不到拷貝】
//   createString() 回傳 local 物件，輸出裡只有一次 [建構] 與一次 [解構]，
//   沒有多餘的 [拷貝建構] —— 這是具名回傳值最佳化（NRVO）生效的結果。
//   NRVO 在 C++17 仍屬「允許但不強制」（標準保證免除複製的是回傳 prvalue
//   的情形），本機 g++ 15.2 實測有生效；換編譯器或關閉最佳化可能不同。

// === 預期輸出 ===
// ===== 測試 1：基本生命週期 =====
//   [建構] "Hello" (緩衝區 #1)
//   s = "Hello"
//   [解構] "Hello" (緩衝區 #1)
//
// ===== 測試 2：拷貝建構 =====
//   [建構] "Dragon" (緩衝區 #2)
//   [拷貝建構] "Dragon" (緩衝區 #3 ← 複製自 #2)
//   a 與 b 共用同一塊緩衝區？ false
//   （false → 兩者各自持有一塊 → 深拷貝成功）
//   [解構] "Dragon" (緩衝區 #3)
//   [解構] "Dragon" (緩衝區 #2)
//
// ===== 測試 3：拷貝賦值 =====
//   [建構] "Knight" (緩衝區 #4)
//   [建構] "Wizard" (緩衝區 #5)
//   賦值前：a="Knight" b="Wizard"
//   [拷貝建構] "Knight" (緩衝區 #6 ← 複製自 #4)
//   [拷貝賦值] swap
//   [解構] "Wizard" (緩衝區 #5)
//   賦值後：a="Knight" b="Knight"
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
// ===== 測試 5：按值傳參 =====
//   [建構] "Rogue" (緩衝區 #9)
//   [拷貝建構] "Rogue" (緩衝區 #10 ← 複製自 #9)
//     processByValue: "Rogue"
//   [解構] "Rogue" (緩衝區 #10)
//   返回後 a="Rogue"（不受影響）
//   [解構] "Rogue" (緩衝區 #9)
//
// ===== 測試 6：按值回傳 =====
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
//   a="A" b="A" c="A"
//   [解構] "A" (緩衝區 #16)
//   [解構] "A" (緩衝區 #15)
//   [解構] "A" (緩衝區 #12)
//
// ===== 測試 8：LeetCode 705. Design HashSet =====
//   contains(1) = true
//   contains(3) = false
//   remove(2) 後 contains(2) = false
//   原集合 contains(100)   = false
//   快照   contains(100)   = true（不受影響 → 深拷貝成功）
//   還原後 contains(100)   = true
//   還原後 contains(999)   = false（舊內容已被整批換掉）
//
// ===== 測試 9：RAII FileHandle（禁止拷貝）=====
//   暫存檔開啟成功？ true
//   讀回內容: "audit: user=alice action=login"
//   is_copy_constructible<FileHandle> = false（= delete → 誤用在編譯期就被擋下）
//
// === 三法則速查 ===
//   需要自訂解構函數？ → 也需要拷貝建構 + 拷貝賦值
//   判斷：類別是否管理動態資源？
//     是 → 實作三法則（解構 + 拷貝建構 + 拷貝賦值）
//     否（只用 string/vector/unique_ptr）→ Rule of Zero
