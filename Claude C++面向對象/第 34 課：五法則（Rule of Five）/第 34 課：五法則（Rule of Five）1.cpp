// =============================================================================
//  第 34 課 -1  —  五法則（Rule of Five）：持有資源的類別要寫齊哪五個
// =============================================================================
//
// 【主題資訊 Information】
//   五法則的五個特殊成員函式：
//       1. 解構函數           ~T();
//       2. 拷貝建構函數        T(const T&);
//       3. 拷貝賦值運算子      T& operator=(const T&);
//       4. 移動建構函數        T(T&&) noexcept;
//       5. 移動賦值運算子      T& operator=(T&&) noexcept;
//   歷史       ： C++98 只有前三個（Rule of Three）；
//                 C++11 加入移動語義後擴充為五個（Rule of Five）
//   對照       ： Rule of Zero —— 讓成員自己管資源，五個一個都不寫（見第 34 課 -2）
//   標準版本   ： C++11
//   標頭檔     ： <utility>（std::move、std::swap）、<algorithm>（std::copy）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是「五個一起」而不是「各自獨立」】
//   核心原因是它們互相牽動：
//       * 你需要寫解構函數（因為持有裸資源要手動釋放），
//         這件事本身就會「關掉」移動建構與移動賦值的自動生成（見第 32 課 -3）。
//       * 一旦移動操作消失，所有「移動」都安靜退回深拷貝，效能默默變差。
//   所以只要你需要其中一個（通常是解構函數），
//   就必須把另外四個都補齊，否則會得到一個「只能拷貝、不能移動」的半殘類別。
//   本檔的 ManagedArray 持有 int* m_data，正是這種必須寫齊五法則的類別。
//
// 【2. 八個測試涵蓋的路徑】
//   本檔用八個情境把五個函式全部觸發一遍：
//       1 一般建構    2 拷貝建構(左值)    3 移動建構(右值)
//       4 拷貝賦值    5 移動賦值          6 從函式回傳(移動或省略)
//       7 按值傳左值(拷貝)               8 按值傳右值(移動)
//   對照 3 與 5 可以看到「移動之後來源 size=0」；
//   對照 7 與 8 可以看到同一個 consumeArray，傳左值走拷貝、傳右值走移動。
//
// 【3. 拷貝賦值採「先配置後釋放」，移動賦值採「先釋放後偷」】
//   兩者的例外安全等級不同，寫法也刻意不同：
//       拷貝賦值：先 new 新的、複製成功後才 delete 舊的 ——
//                 new 失敗時自己還完好，具強例外保證。
//       移動賦值：先 delete 舊的、再從 other 偷指標 ——
//                 因為只搬指標不會失敗（noexcept），這個順序是安全的，
//                 但「自我賦值防護」變成必要（否則從已釋放的自己偷取）。
//   這一對比正是第 28、29、33 課各自的重點，在五法則裡合流。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 成員宣告順序必須與所有初始化列表一致（-Wreorder）
//   成員的初始化順序「只」由宣告順序決定，與初始化列表的書寫順序無關。
//   本檔原始版本的宣告是 int* m_data; 在前、std::size_t m_size; 在後，
//   但多個建構函數的初始化列表寫法並不一致：
//       一般建構、拷貝建構：: m_size(...), m_data(...)   （size 在前）
//       移動建構          ：: m_data(...), m_size(...)   （data 在前）
//   於是無論宣告順序怎麼排，總有某個建構函數會觸發 -Wreorder 警告
//   （本機實測：原始版本的一般建構與拷貝建構都報了）。
//   本檔的修法是「兩件事一起做」：
//       1) 把宣告順序改成 m_size 在前、m_data 在後
//          （被依賴的成員在前 —— m_data 的配置長度用到 m_size）
//       2) 把「所有」建構函數的初始化列表統一成 m_size、m_data 的順序
//   兩者一致之後警告全部消除。這不只是消警告：
//   若哪天有人把移動建構的 m_data 初始值從 other.m_data 改成依賴 m_size，
//   一致的順序能保證 m_size 已經先初始化好。
//
// (B) 移動操作為什麼都標 noexcept
//   移動建構與移動賦值只搬指標、把來源置空，不配置記憶體，天生不會拋。
//   標了 noexcept，std::vector 擴容時才會採用它們（見第 32 課 -2）。
//   本檔測試 6 的 createArray 回傳、以及把 ManagedArray 放進容器的情境，
//   都依賴這個 noexcept 才能享受 O(1) 的搬移。
//
// (C) 測試 6 為什麼可能看不到「移動建構」
//   createArray 回傳一個具名區域變數 arr，理論上要移動建構到 f，
//   但編譯器可能套用 NRVO 直接把 arr 建在 f 的位置上，連移動都省掉。
//   NRVO 是「可選」的最佳化，C++17 只保證純右值初始化的省略（見第 28 課 -2）。
//   本機實測本檔測試 6 看不到 [移動建構]（NRVO 生效）；
//   若在其他編譯器看到一次移動，也是合法的。
//
// (D) Rule of Zero 才是首選
//   ManagedArray 手寫五法則是為了「理解機制」。實務上若把 int* m_data
//   換成 std::vector<int> 或 std::unique_ptr，這五個函式一個都不用寫，
//   編譯器生成的版本全部正確、還自動推導 noexcept（見第 34 課 -2 的 MoveOnly）。
//   守則：優先用標準容器與智慧指標（Rule of Zero）；
//   只有在真的要管理裸資源時，才寫齊五法則。
//
// 【注意事項 Pay Attention】
//   1. 持有裸資源就要寫齊五個 —— 少寫移動操作會退化成只能拷貝的半殘類別。
//   2. 成員宣告順序決定初始化順序，且必須與「所有」建構函數的
//      初始化列表一致（用 -Wreorder 檢查），否則潛藏用未初始化成員的風險。
//   3. 移動操作務必標 noexcept，否則容器擴容時不會採用。
//   4. 移動賦值一定要有自我賦值防護（先 delete 再偷）；
//      拷貝賦值採先配置後釋放時可容忍缺防護。
//   5. 首選 Rule of Zero（用容器／智慧指標）；手寫五法則是最後手段。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】五法則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是五法則？為什麼不是「需要哪個寫哪個」？
//     答：五法則指持有資源的類別應同時定義解構、拷貝建構、拷貝賦值、
//         移動建構、移動賦值這五個特殊成員函式。
//         不能各自獨立寫，是因為它們互相牽動：一旦你宣告了解構函數，
//         編譯器就不再自動生成移動建構與移動賦值，
//         結果所有「移動」都安靜退回深拷貝。
//         所以需要其中一個（通常是解構），就得把五個補齊。
//     追問：有沒有「一個都不用寫」的做法？
//         → 有，Rule of Zero：把裸資源換成 std::vector / std::unique_ptr
//           這類會自我管理的成員，五個特殊成員函式全部交給編譯器生成，
//           且都正確、還自動推導 noexcept。這才是實務首選。
//
// 🔥 Q2. 拷貝賦值和移動賦值的實作順序為什麼不一樣？
//     答：拷貝賦值採「先配置新的、複製成功後才釋放舊的」，
//         這樣 new 失敗時自己還完好，具強例外保證。
//         移動賦值採「先釋放舊的、再從來源偷指標」，
//         因為搬指標是 noexcept、不會失敗，這個順序安全；
//         但代價是自我賦值防護變成必要 —— 否則會從已釋放的自己偷取。
//     追問：移動操作為什麼要標 noexcept？
//         → 讓 std::vector 擴容時願意採用它們（move_if_noexcept），
//           否則會保守地改用拷貝，你寫的移動操作等於白寫（第 32 課 -2）。
//
// ⚠️ 陷阱. 「我的類別已經寫了解構、拷貝建構、拷貝賦值（三法則），
//           放進 std::vector 應該就能享受移動語義了。」
//     答：不會。因為你宣告了解構函數（其實拷貝操作也算），
//         編譯器不會自動生成移動建構與移動賦值。
//         於是 vector 擴容時能找到的只有拷貝操作，
//         所有既有元素都是被深拷貝過去的 —— 你以為有移動，其實沒有，
//         而且沒有任何錯誤或警告提醒你。
//     為什麼會錯：把三法則當成「現代 C++ 的完整資源管理」。
//         三法則是 C++98 的產物；C++11 之後，
//         「有解構函數卻沒補移動操作」的類別就是效能上的半殘品。
//         要嘛補齊五法則，要嘛退回 Rule of Zero。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺，理由如下
//   五法則是資源管理型類別的實作規範，屬於工程與正確性議題。
//   LeetCode 判的是演算法的輸入輸出與時間限制，
//   評測系統只會建立一個實例並呼叫指定方法，
//   既不會拷貝也不會移動你的物件，更不會檢驗五個特殊成員函式是否寫齊。
//   即使是設計類題目（146. LRU Cache、707. Design Linked List），
//   實務上也會直接用 std::list / std::unordered_map 這些
//   已經遵守 Rule of Zero 的標準容器，根本不必自己寫五法則。
//   硬掛題號只會誤導，故從缺。本檔以八個涵蓋全部五個函式的情境呈現主題。
//
// -----------------------------------------------------------------------------
// lesson34_rule_of_five.cpp
// 驗證：valgrind --leak-check=full ./lesson34
// -----------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class ManagedArray {
private:
    // ⚠️ 宣告順序決定初始化順序，且必須與「所有」建構函數的初始化列表一致
    //    （見檔頭【概念補充 A】）。m_size 被依賴（配置 m_data 的長度），宣告在前。
    std::size_t m_size;
    int*        m_data;

public:
    // ════════════════════════════════════════════
    //  建構函數
    // ════════════════════════════════════════════
    explicit ManagedArray(std::size_t size = 0)
        : m_size(size)
        , m_data(size > 0 ? new int[size]{} : nullptr)   // {} 值初始化為 0
    {
        std::cout << "  [建構] size=" << m_size << "\n";
    }

    // ════════════════════════════════════════════
    //  Rule of Five：以下五個必須同時定義
    // ════════════════════════════════════════════

    // ── 1. 解構函數 ──
    ~ManagedArray() {
        std::cout << "  [解構] size=" << m_size << "\n";
        delete[] m_data;
    }

    // ── 2. 拷貝建構函數（深拷貝）──
    ManagedArray(const ManagedArray& other)
        : m_size(other.m_size)
        , m_data(other.m_size > 0 ? new int[other.m_size] : nullptr)
    {
        if (m_data) {
            std::copy(other.m_data, other.m_data + m_size, m_data);
        }
        std::cout << "  [拷貝建構] size=" << m_size << "\n";
    }

    // ── 3. 拷貝賦值運算子（先配置後釋放，具強例外保證）──
    ManagedArray& operator=(const ManagedArray& other) {
        std::cout << "  [拷貝賦值] size=" << other.m_size << "\n";
        if (this == &other) return *this;

        // 先配置再釋放（例外安全）：new 若失敗，自己的舊資源還完好
        int* newData = nullptr;
        if (other.m_size > 0) {
            newData = new int[other.m_size];
            std::copy(other.m_data, other.m_data + other.m_size, newData);
        }

        delete[] m_data;
        m_data = newData;
        m_size = other.m_size;

        return *this;
    }

    // ── 4. 移動建構函數 ──
    //    初始化列表順序統一為 m_size、m_data（與宣告順序一致，避免 -Wreorder）
    ManagedArray(ManagedArray&& other) noexcept
        : m_size(other.m_size)
        , m_data(other.m_data)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        std::cout << "  [移動建構] size=" << m_size << "\n";
    }

    // ── 5. 移動賦值運算子（先釋放後偷，自我賦值防護是必要的）──
    ManagedArray& operator=(ManagedArray&& other) noexcept {
        std::cout << "  [移動賦值] size=" << other.m_size << "\n";
        if (this == &other) return *this;

        delete[] m_data;             // 先釋放自己的舊資源

        m_data = other.m_data;       // 偷走指標（O(1)）
        m_size = other.m_size;

        other.m_data = nullptr;      // 置空來源，避免 double free
        other.m_size = 0;

        return *this;
    }

    // ════════════════════════════════════════════
    //  輔助函數
    // ════════════════════════════════════════════

    void swap(ManagedArray& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

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

// ──────── 測試用函數 ────────

// 按值返回 → 觸發移動（或拷貝省略）
ManagedArray createArray(std::size_t n) {
    ManagedArray arr(n);
    for (std::size_t i = 0; i < n; ++i) {
        arr[i] = static_cast<int>((i + 1) * 100);
    }
    return arr;
}

// 按值傳入 → 觸發拷貝建構（左值）或移動建構（右值）
void consumeArray(ManagedArray arr) {
    arr.print("consumed");
}

int main() {
    std::cout << "===== 1. 一般建構 =====\n";
    ManagedArray a(4);
    for (std::size_t i = 0; i < a.size(); ++i) a[i] = static_cast<int>(i + 1);
    a.print("a");
    std::cout << "\n";

    std::cout << "===== 2. 拷貝建構（左值）=====\n";
    ManagedArray b = a;
    b.print("b");
    std::cout << "\n";

    std::cout << "===== 3. 移動建構（右值）=====\n";
    ManagedArray c = std::move(a);
    c.print("c");
    a.print("a (moved)");    // a 被掏空，size=0（這是我們自己歸零的成員，安全）
    std::cout << "\n";

    std::cout << "===== 4. 拷貝賦值（左值）=====\n";
    ManagedArray d(2);
    d[0] = 99; d[1] = 88;
    d.print("d before");
    d = b;  // b 是左值 → 拷貝賦值
    d.print("d after");
    std::cout << "\n";

    std::cout << "===== 5. 移動賦值（右值）=====\n";
    ManagedArray e(1);
    e[0] = 77;
    e.print("e before");
    e = std::move(c);  // std::move(c) 是右值 → 移動賦值
    e.print("e after");
    c.print("c (moved)");
    std::cout << "\n";

    std::cout << "===== 6. 從函數返回值（移動或拷貝省略）=====\n";
    ManagedArray f = createArray(3);
    f.print("f");    // 本機 NRVO 生效，看不到 [移動建構]（見檔頭【概念補充 C】）
    std::cout << "\n";

    std::cout << "===== 7. 按值傳入左值（拷貝建構）=====\n";
    consumeArray(b);
    b.print("b (still alive)");
    std::cout << "\n";

    std::cout << "===== 8. 按值傳入右值（移動建構）=====\n";
    consumeArray(std::move(b));
    b.print("b (moved)");
    std::cout << "\n";

    std::cout << "===== 開始解構 =====\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -Wreorder "第 34 課：五法則（Rule of Five）1.cpp" -o lesson34

// ── 輸出說明（讀之前先看這裡）────────────────────────────────────────────
// * 本檔輸出完全決定性：只有整數運算與計數，沒有位址、沒有耗時、
//   沒有執行緒（本機實測連跑 5 次逐位元組相同）。
// * 印出被移動物件的 size=0 是安全的：那是移動操作明確歸零的成員
//   （m_size = 0），不是去讀 valid but unspecified 的內容。
// * 測試 6 看不到 [移動建構]：createArray 回傳具名區域變數 arr，
//   本機 NRVO 生效，直接把它建在 f 的位置上，連移動都省了。
//   NRVO 是「可選」的最佳化，C++17 只保證純右值初始化的省略；
//   其他編譯器在這裡多出一次 [移動建構] 也是合法的（見第 28 課 -2）。
// * 測試 7 vs 8 是同一個 consumeArray：傳左值 b 走 [拷貝建構]（b 仍存活），
//   傳 std::move(b) 走 [移動建構]（b 之後 size=0）。
//   兩段末尾各有一次 [解構]，那是參數 arr 離開函式時銷毀。
// * 本檔已修正原始版本的 -Wreorder 警告：把成員宣告順序統一成
//   m_size、m_data，並讓「所有」建構函數的初始化列表都照這個順序寫
//   （原始版本移動建構的順序與其他不一致）。修正前後執行結果完全相同，
//   因為那是尚未被觸發的潛在缺陷（見檔頭【概念補充 A】）。
// * 結尾解構順序為區域物件宣告的反向：f、e、d、c、b、a
//   （其中 a、c、b 已被移動，size=0）。
// ─────────────────────────────────────────────────────────────────────────

// === 預期輸出 ===
// ===== 1. 一般建構 =====
//   [建構] size=4
//   a (size=4): [1, 2, 3, 4]
//
// ===== 2. 拷貝建構（左值）=====
//   [拷貝建構] size=4
//   b (size=4): [1, 2, 3, 4]
//
// ===== 3. 移動建構（右值）=====
//   [移動建構] size=4
//   c (size=4): [1, 2, 3, 4]
//   a (moved) (size=0): []
//
// ===== 4. 拷貝賦值（左值）=====
//   [建構] size=2
//   d before (size=2): [99, 88]
//   [拷貝賦值] size=4
//   d after (size=4): [1, 2, 3, 4]
//
// ===== 5. 移動賦值（右值）=====
//   [建構] size=1
//   e before (size=1): [77]
//   [移動賦值] size=4
//   e after (size=4): [1, 2, 3, 4]
//   c (moved) (size=0): []
//
// ===== 6. 從函數返回值（移動或拷貝省略）=====
//   [建構] size=3
//   f (size=3): [100, 200, 300]
//
// ===== 7. 按值傳入左值（拷貝建構）=====
//   [拷貝建構] size=4
//   consumed (size=4): [1, 2, 3, 4]
//   [解構] size=4
//   b (still alive) (size=4): [1, 2, 3, 4]
//
// ===== 8. 按值傳入右值（移動建構）=====
//   [移動建構] size=4
//   consumed (size=4): [1, 2, 3, 4]
//   [解構] size=4
//   b (moved) (size=0): []
//
// ===== 開始解構 =====
//   [解構] size=3
//   [解構] size=4
//   [解構] size=4
//   [解構] size=0
//   [解構] size=0
//   [解構] size=0
