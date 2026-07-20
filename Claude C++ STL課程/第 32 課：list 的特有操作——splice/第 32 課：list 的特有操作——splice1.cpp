// =============================================================================
//  第 32 課：list 的特有操作——splice 1  —  移動節點而不移動資料
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <list>
//
//   三個重載（C++11 起參數為 const_iterator）：
//     void splice(const_iterator pos, list& other);                        // (1) 整串
//     void splice(const_iterator pos, list& other, const_iterator it);     // (2) 單一元素
//     void splice(const_iterator pos, list& other,
//                 const_iterator first, const_iterator last);              // (3) 一段範圍
//     （每個都有對應的 list&& 右值版本）
//
//   複雜度（[list.ops] 的規定）：
//     (1) 整串       O(1)
//     (2) 單一元素   O(1)
//     (3) 範圍       **&other == this 時 O(1)；否則 O(distance(first,last))**
//
//   ★ 核心保證：splice **不複製、不移動、不配置、不解構任何元素**。
//     它只是把節點從一條鏈拆下來、接到另一條鏈上 —— 純指標重接。
//     因此**指向被搬移元素的 iterator / reference / pointer 全部保持有效**，
//     只是它們的「歸屬」換到了目標 list。
//
// 【詳細解釋 Explanation】
//
// 【1. splice 到底做了什麼：四根指標的重接】
// list 是雙向鏈結串列，每個節點有 prev / next 兩根指標。
// 把節點 X 從 B 搬到 A 的 pos 之前，實際只改動六根指標：
//
//   搬移前：  B: ... ─ P ⇄ X ⇄ Q ─ ...        A: ... ─ M ⇄ pos ─ ...
//   搬移後：  B: ... ─ P ⇄ Q ─ ...            A: ... ─ M ⇄ X ⇄ pos ─ ...
//
//   具體是：P.next=Q, Q.prev=P（把 X 從 B 摘掉）
//           M.next=X, X.prev=M, X.next=pos, pos.prev=X（把 X 接進 A）
//   **節點本身（連同裡面的元素）從頭到尾沒有搬動過一個位元組。**
//   這就是為什麼指向該元素的指標依然有效 —— 它根本沒換位置。
//
// 【2. 為什麼這件事很重要：跟「erase + insert」的天壤之別】
//   等效但錯誤的寫法：
//       A.push_back(*it);   // 複製建構一個新元素（可能配置記憶體）
//       B.erase(it);        // 解構舊元素（可能釋放記憶體）
//   代價：一次複製建構 + 一次解構 + 可能的配置/釋放，而且**原本的 iterator
//         全部失效**（指向的節點被 erase 掉了）。
//   splice 的代價：六次指標賦值。就這樣。
//   對 list<string>、list<vector<int>> 這種「複製很貴」的元素型別，
//   差距是數量級的。本檔第 6 節用一個會計數的型別實測驗證：
//   splice 整串 3 個元素 → 複製 0 次、移動 0 次；
//   push_back + pop_front 同樣 3 個元素 → 複製 3 次。
//
// 【3. 三個重載的使用時機】
//   (1) A.splice(pos, B)              —— 把整條 B 併進 A，之後 B 變成空的。
//                                        典型用途：合併多個工作佇列。
//   (2) A.splice(pos, B, it)          —— 只搬一個節點。
//                                        典型用途：**LRU Cache 把命中項移到最前面**。
//   (3) A.splice(pos, B, first, last) —— 搬 [first, last) 這段。
//                                        典型用途：批次轉移、分割串列。
//
// 【4. 同一個 list 內 splice（重排元素）—— 這是合法且常用的】
//   lst.splice(lst.begin(), lst, it);   // 把 it 指的元素移到最前面
//   這是 O(1) 的「元素移位」，list 特有的能力。
//   **限制**：pos 不可以落在被搬移的範圍 [first, last) 之內，否則是未定義行為
//   （會把鏈接成一個環）。單一元素版本則要求 pos != it 且 pos != next(it)
//   —— 實際上這兩種情況是 no-op，標準允許但沒有效果。
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ 為什麼範圍版本「跨 list 時是 O(n)」—— size() 是罪魁禍首
//   C++11 起標準要求 list::size() 必須是 **O(1)**，這表示 list 內部要維護
//   一個 size 計數器。於是範圍 splice 面臨一個問題：
//       「我到底搬了幾個節點？」
//   指標重接本身是 O(1)，但要更新兩邊的 size 計數器，就必須知道範圍長度，
//   而唯一的方法是 **std::distance(first, last)** —— 走一遍，O(n)。
//   反之若 &other == this（同一個 list 內搬），總數沒變、不需要知道長度，
//   所以標準規定此時是 O(1)。
//   ★ 這是「C++11 把 size() 從 O(n) 改成 O(1)」所付出的代價，
//     是少數「新標準讓某個操作變慢」的例子。C++98 的範圍 splice 全都是 O(1)。
//
// (B) ★ 本機實測：libstdc++ 的實作與標準保證的微妙落差
//   讀 libstdc++ 15.2.0 的 /usr/include/c++/15/bits/stl_list.h 可以看到，
//   範圍 splice **無條件**呼叫 std::distance（沒有先判斷 this == &__x）：
//       size_t __n = std::distance(__first, __last);
//       this->_M_inc_size(__n);  __x._M_dec_size(__n);
//   同一個 list 時這兩行加減互相抵消，但 distance 仍然被算了。
//   那自我 splice 不就變成 O(n)、違反標準了嗎？本機實測結果很有意思
//   （以獨立的小型 benchmark 量測「同一個 list 內的範圍 splice」，非本檔輸出；
//     本檔第 7 節量的是「整串」與「跨 list 範圍」）：
//       -O0：範圍長度 4000→2,000,000，耗時 12µs→7,346µs   → **確實是線性的**
//       -O2：範圍長度 4000→2,000,000，耗時 185ns→168ns    → **平坦，O(1)**
//   原因：-O2 下編譯器內聯後看出「distance 的結果只用於一加一減、互相抵消」，
//   且該迴圈只是純指標走訪、沒有副作用，於是**整個迴圈被最佳化消除**，
//   剛好回到標準保證的 O(1)。
//   → 教訓：複雜度保證是「標準對實作的要求」，實際效能還要看最佳化等級。
//     debug build（-O0）跑得比 release 慢，有時不只是常數倍的差距，
//     而是**複雜度等級**的差距。
//
// (C) 為什麼 splice 的參數是 list& 而不是 list&&（C++98 遺產）
//   splice 早在 C++98 就存在，當時沒有右值參考。C++11 加上了 list&& 的重載，
//   但 list& 版本必須保留以維持相容。實務上兩者行為相同 ——
//   splice 從來就是「移動」語意，只是 C++98 沒有詞彙描述它。
//
// (D) allocator 必須相同
//   兩個 list 的 allocator 若不相等，splice 是未定義行為。
//   原因很直接：節點是用 B 的 allocator 配置的，直接掛到 A 上之後，
//   A 解構時會用 A 的 allocator 去釋放它 —— 配置與釋放不匹配。
//   用預設 allocator 時不必擔心（所有 std::allocator<T> 實例都相等）。
//
// 【注意事項 Pay Attention】
// 1. **pos 不可落在 [first, last) 之內**，否則是未定義行為（鏈會接成環）。
// 2. first / last 必須真的是 other 的迭代器。傳入不屬於 other 的迭代器是
//    未定義行為，而且**不會有任何檢查或錯誤訊息** —— 這類 bug 極難追。
// 3. splice 後 other 的 size 減少、目標的 size 增加，兩者都會被正確更新。
// 4. 被搬移元素的 iterator / reference / pointer **保持有效**，
//    但它們現在屬於目標 list（用它們去呼叫 other.erase() 是未定義行為）。
// 5. splice 是 noexcept 的（整串與單一元素版本）—— 它不配置記憶體，不會丟例外。
// 6. 範圍版本跨 list 時是 O(n)，別在熱路徑上誤以為它是 O(1)。
// 7. 只有 list / forward_list 有 splice。vector / deque **沒有**，
//    因為它們的元素是「值的陣列」而非「獨立節點」，無法只改指標就轉移歸屬。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::splice
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. splice 做了什麼？為什麼它能做到 O(1)？
//     答：把節點從一條鏈拆下、接到另一條鏈上，**純指標重接**（約六次指標賦值）。
//         它**不複製、不移動、不配置、不解構**任何元素 —— 節點連同裡面的資料
//         從頭到尾沒有搬動過一個位元組，所以與元素大小完全無關，恆為 O(1)。
//     追問：那 A.push_back(*it); B.erase(it); 不是等效嗎？
//         → 語意上結果一樣，成本天差地遠：那是一次複製建構 + 一次解構
//           （外加可能的配置/釋放），而且原本的 iterator 全部失效。
//           對 list<string> 這種元素，差距是數量級的。
//
// 🔥 Q2. splice 之後，指向被搬移元素的 iterator 還有效嗎？
//     答：**完全有效**，這正是 splice 最重要的性質。因為節點沒有被搬動，
//         iterator 內部存的節點位址依然正確。唯一改變的是它的**歸屬**——
//         該元素現在屬於目標 list 了。
//     追問：那這個保證有什麼實際價值？
//         → 這正是 LRU Cache 能做到 O(1) 的關鍵：
//           unordered_map 裡存著 list 的 iterator，快取命中時用 splice
//           把節點移到最前面，**map 裡的 iterator 完全不用更新**。
//           若換成 erase + push_front，每次都得回頭改 map，還會失效。
//
// 🔥 Q3. splice 的三個重載複雜度分別是什麼？為什麼範圍版本比較特別？
//     答：整串 O(1)、單一元素 O(1)；範圍版本則是
//         **&other == this 時 O(1)，跨 list 時 O(distance(first,last))**。
//         原因是 C++11 起 size() 必須是 O(1)，list 得維護 size 計數器。
//         跨 list 搬移時兩邊的計數器都要更新，就必須先知道搬了幾個
//         → 只能 std::distance 走一遍 → O(n)。
//         同一個 list 內搬則總數不變、不需要知道長度，所以是 O(1)。
//     追問：這代表 C++98 的 splice 比 C++11 快嗎？
//         → 在「跨 list 的範圍 splice」這一項上，是的。C++98 的 size() 允許
//           O(n)，所以不需要計數器，範圍 splice 全都是 O(1)。
//           這是少數「新標準讓某個操作變慢」的例子 —— 用它換來 O(1) 的 size()。
//
// ⚠️ 陷阱 1. 「splice 是 O(1)，所以我在熱路徑上大量用範圍版本沒問題」——錯在哪？
//     答：跨 list 的**範圍版本是 O(n)**，不是 O(1)。只有「整串」與「單一元素」
//         才無條件 O(1)。本機 -O2 實測：跨 list 搬 2,000,000 個節點耗時
//         約 4,855µs，搬 4,000 個約 5.6µs —— 清楚的線性關係。
//         若要搬整條 list，請用整串版本 A.splice(pos, B)，那才是真 O(1)。
//     為什麼會錯：把「splice 靠指標重接所以 O(1)」這個直覺無條件外推。
//         指標重接確實 O(1)，但**維護 size 計數器**需要知道長度，這才是瓶頸。
//
// ⚠️ 陷阱 2. 同一個 list 內做範圍 splice，標準說 O(1)，但你在 debug build
//            量到它隨長度線性增加。是標準錯了、還是實作有 bug？
//     答：都不是。libstdc++ 的範圍 splice **無條件**呼叫 std::distance
//         （沒先判斷是否同一個 list），所以原始碼層面確實是線性的。
//         但 -O2 下編譯器看出 distance 的結果只用於一加一減、互相抵消，
//         且迴圈無副作用，於是**把整個迴圈最佳化掉**，回到 O(1)。
//         本機實測：-O0 時 4000→2,000,000 個節點耗時 12µs→7,346µs（線性）；
//         -O2 時同樣範圍 185ns→168ns（平坦）。
//     為什麼會錯：以為「複雜度保證」是實作必須逐行照做的規則。
//         實際上標準規範的是**可觀察行為**，編譯器有權以任何方式達成 ——
//         包括把違反該複雜度的程式碼最佳化到符合。也因此，
//         **效能測試一定要在開最佳化的組態下做**，debug build 的數字
//         可能連複雜度等級都不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <chrono>
using namespace std;

template <typename T>
void print_list(const string& label, const list<T>& lst) {
    cout << label << " [" << lst.size() << "]:";
    for (const auto& val : lst) cout << " " << val;
    if (lst.empty()) cout << " (空)";
    cout << endl;
}

// 用來證明「splice 完全不呼叫複製/移動建構子」的計數型別。
// 注意：必須定義在檔案範圍 —— 區域類別（local class）不允許有 static 資料成員。
struct Tracked {
    int id;
    static int copies;
    static int moves;
    explicit Tracked(int i) : id(i) {}
    Tracked(const Tracked& o) : id(o.id) { ++copies; }
    Tracked(Tracked&& o) noexcept : id(o.id) { ++moves; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) noexcept = default;
};
int Tracked::copies = 0;
int Tracked::moves  = 0;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計 LRU（Least Recently Used）快取，get 與 put 都必須是 O(1)。
//   為什麼用到本主題：這是 splice 在真實工程中**最重要**的應用，
//         而且是「非 splice 不可」的少數場景之一。
//         資料結構：list 維護使用順序（前 = 最近使用），
//                   unordered_map 存 key → list 節點的 iterator。
//         快取命中時要把該項移到最前面，關鍵在於：
//           * 用 splice → **O(1)，而且 map 裡存的 iterator 依然有效不必更新**
//           * 用 erase + push_front → 節點被銷毀重建，iterator 失效，
//             每次都得回頭更新 map，還多了一次複製建構
//         iterator 的穩定性在這裡不是「錦上添花」，而是整個設計成立的前提。
//   複雜度：get / put 皆 O(1)。
// -----------------------------------------------------------------------------
class LRUCache {
    // list 存 (key, value)；最前面 = 最近使用，最後面 = 最久沒用
    using Entry = pair<int, int>;
    list<Entry>                                 order_;
    unordered_map<int, list<Entry>::iterator>   index_;
    size_t                                      capacity_;

public:
    explicit LRUCache(int capacity)
        : capacity_(static_cast<size_t>(capacity)) {}

    int get(int key) {
        auto found = index_.find(key);
        if (found == index_.end()) return -1;
        // ★ 命中：把節點移到最前面。O(1)，且 found->second 這個 iterator
        //   在 splice 之後**依然有效**，index_ 完全不需要更新。
        order_.splice(order_.begin(), order_, found->second);
        return found->second->second;
    }

    void put(int key, int value) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = value;                      // 更新值
            order_.splice(order_.begin(), order_, found->second); // 移到最前
            return;
        }
        if (order_.size() >= capacity_) {
            // 淘汰最久沒用的（最後一個）
            index_.erase(order_.back().first);
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        index_[key] = order_.begin();
    }

    // 教學用：顯示目前的使用順序
    string dumpOrder() const {
        string s;
        for (const auto& e : order_) {
            if (!s.empty()) s += " ";
            s += to_string(e.first) + ":" + to_string(e.second);
        }
        return s.empty() ? "(空)" : s;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】多優先級任務排程器：任務在佇列之間「升級 / 降級」
//   情境：排程器有 HIGH / MEDIUM / LOW 三條佇列。營運過程中任務會改變優先級 ——
//         客訴進來要把某任務升成 HIGH、系統閒置時把 LOW 全部併入 MEDIUM。
//   為什麼用 splice：
//     * 任務物件可能很大（含描述、附件、上下文），複製成本高；
//       splice 只改指標，**零複製**。
//     * 其他模組（進度追蹤、通知）可能持有指向任務的 iterator/pointer，
//       splice 後那些指標**依然有效**，不必通知所有模組更新。
//     * 「把 LOW 整條併入 MEDIUM」用整串版本是真正的 O(1)，
//       不論 LOW 裡有 10 個還是 10 萬個任務。
//   注意：跨佇列搬「一段範圍」是 O(n)，搬「單一任務」與「整條佇列」才是 O(1)。
// -----------------------------------------------------------------------------
struct Task {
    string   name;
    int      id;
    string   payload;   // 模擬較大的任務內容，凸顯「零複製」的價值
};

// 讓 Task 可以被 print_list 印出
ostream& operator<<(ostream& os, const Task& t) {
    return os << t.name << "(#" << t.id << ")";
}

class Scheduler {
public:
    list<Task> high, medium, low;

    // 把某任務升級為 HIGH：O(1)，且指向該任務的外部指標仍然有效
    void promote(list<Task>& from, list<Task>::iterator it) {
        high.splice(high.end(), from, it);
    }
    // 系統閒置：把整條 LOW 併入 MEDIUM 尾端，真 O(1)
    void drainLowIntoMedium() {
        medium.splice(medium.end(), low);
    }
    void dump() const {
        print_list("    HIGH  ", high);
        print_list("    MEDIUM", medium);
        print_list("    LOW   ", low);
    }
};

int main() {
    // ===== 1. splice 版本 1：移植整個 list =====
    cout << "===== splice 版本 1：移植整個 list（O(1)）=====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};
        print_list("A ", A);
        print_list("B ", B);

        auto pos = A.begin();
        advance(pos, 1);   // 指向 2

        A.splice(pos, B);  // 把 B 全部移到 2 前面
        print_list("splice 後 A", A);
        print_list("splice 後 B", B);
        cout << "→ B 被掏空；不論 B 原本有幾個元素，都是 O(1)" << endl;
    }

    // ===== 2. splice 版本 2：移植單一元素 =====
    cout << "\n===== splice 版本 2：移植單一元素（O(1)）=====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};
        print_list("A ", A);
        print_list("B ", B);

        auto pos  = A.end();          // 插入到 A 的尾端
        auto elem = B.begin();
        advance(elem, 1);             // 指向 B 的 20

        A.splice(pos, B, elem);       // 把 B 的 20 移到 A 的尾端
        print_list("splice 後 A", A);
        print_list("splice 後 B", B);
    }

    // ===== 3. splice 版本 3：移植一段範圍 =====
    cout << "\n===== splice 版本 3：移植範圍（跨 list 是 O(n)！）=====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30, 40, 50};
        print_list("A ", A);
        print_list("B ", B);

        auto first = B.begin();
        advance(first, 1);            // 指向 20
        auto last = B.begin();
        advance(last, 4);             // 指向 50（不含）

        A.splice(A.end(), B, first, last);  // 移植 {20, 30, 40}
        print_list("splice 後 A", A);
        print_list("splice 後 B", B);
        cout << "→ 注意：跨 list 的範圍版本要算 distance 來更新 size → O(n)" << endl;
    }

    // ===== 4. 同一 list 內 splice（重新排序元素） =====
    cout << "\n===== 同一 list 內 splice（重排，O(1)）=====" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5};
        print_list("初始  ", lst);

        // 把 4 移到最前面
        auto elem = lst.begin();
        advance(elem, 3);   // 指向 4
        lst.splice(lst.begin(), lst, elem);
        print_list("4→前面", lst);

        // 把 2 移到最後面
        elem = lst.begin();
        advance(elem, 2);   // 現在是 {4,1,2,3,5}，第 3 個是 2
        lst.splice(lst.end(), lst, elem);
        print_list("2→後面", lst);
        cout << "→ 這是 list 特有的 O(1) 元素移位能力" << endl;
    }

    // ===== 5. 迭代器/參考穩定性驗證 =====
    cout << "\n===== 迭代器與參考的穩定性（splice 的核心保證）=====" << endl;
    {
        list<int> A = {1, 2, 3};
        list<int> B = {10, 20, 30};

        auto it_20 = B.begin();
        advance(it_20, 1);     // 指向 B 的 20
        const int* addr_before = &(*it_20);

        cout << "splice 前 *it_20 = " << *it_20
             << "，節點位址 = " << static_cast<const void*>(addr_before) << endl;

        A.splice(A.end(), B, it_20);   // 把 20 從 B 移到 A

        cout << "splice 後 *it_20 = " << *it_20
             << "，節點位址 = " << static_cast<const void*>(&(*it_20)) << endl;
        cout << "位址相同嗎? " << boolalpha << (addr_before == &(*it_20)) << endl;
        cout << "它現在屬於 A 嗎? " << (&A.back() == addr_before) << endl;

        print_list("A", A);
        print_list("B", B);
        cout << "→ 節點從未搬動，只是換了歸屬 —— 這正是 LRU Cache 的基石" << endl;
    }

    // ===== 6. 零複製證明：splice vs erase+push_back =====
    cout << "\n===== 零複製證明：splice 完全不呼叫複製/移動建構子 =====" << endl;
    {
        // 用 splice 搬 3 個元素
        list<Tracked> A1, B1;
        for (int i = 0; i < 3; ++i) B1.emplace_back(i);
        Tracked::copies = 0; Tracked::moves = 0;
        A1.splice(A1.end(), B1);
        cout << "  splice 整串 3 個元素：複製 " << Tracked::copies
             << " 次、移動 " << Tracked::moves << " 次" << endl;

        // 用 push_back + erase 搬 3 個元素
        list<Tracked> A2, B2;
        for (int i = 0; i < 3; ++i) B2.emplace_back(i);
        Tracked::copies = 0; Tracked::moves = 0;
        while (!B2.empty()) {
            A2.push_back(B2.front());
            B2.pop_front();
        }
        cout << "  push_back+pop_front 3 個元素：複製 " << Tracked::copies
             << " 次、移動 " << Tracked::moves << " 次" << endl;
        cout << "  ★ splice 是真正的零複製，與元素大小、數量完全無關" << endl;
    }

    // ===== 7. 複雜度實測：整串 O(1) vs 跨 list 範圍 O(n) =====
    cout << "\n===== 複雜度實測（本機實測，每次執行都不同）=====" << endl;
    {
        using namespace chrono;
        cout << "  --- 整串 splice：標準保證 O(1) ---" << endl;
        for (int n : {100000, 1000000, 4000000}) {
            list<int> a, b;
            for (int i = 0; i < n; ++i) b.push_back(i);
            auto t1 = high_resolution_clock::now();
            a.splice(a.begin(), b);
            auto t2 = high_resolution_clock::now();
            cout << "    n = " << n << "\t耗時 "
                 << duration_cast<nanoseconds>(t2 - t1).count() << " ns" << endl;
        }
        cout << "  --- 跨 list 的範圍 splice：O(distance) ---" << endl;
        for (int len : {4000, 40000, 400000, 2000000}) {
            list<int> a, b;
            for (int i = 0; i < 4000000; ++i) b.push_back(i);
            auto first = next(b.begin(), 10);
            auto last  = next(first, len);
            auto t1 = high_resolution_clock::now();
            a.splice(a.begin(), b, first, last);
            auto t2 = high_resolution_clock::now();
            cout << "    範圍長度 = " << len << "\t耗時 "
                 << duration_cast<nanoseconds>(t2 - t1).count() << " ns" << endl;
        }
        cout << "  ★ 整串版本平坦；範圍版本隨長度線性成長 —— 熱路徑請用整串版本" << endl;
    }

    // ===== 8. LeetCode 146. LRU Cache =====
    cout << "\n===== LeetCode 146. LRU Cache =====" << endl;
    {
        LRUCache cache(2);
        cout << "  容量 = 2" << endl;
        cache.put(1, 1);   cout << "  put(1,1)  → 順序: " << cache.dumpOrder() << endl;
        cache.put(2, 2);   cout << "  put(2,2)  → 順序: " << cache.dumpOrder() << endl;
        cout << "  get(1)    → " << cache.get(1)
             << "  順序: " << cache.dumpOrder() << "（1 被 splice 到最前）" << endl;
        cache.put(3, 3);   cout << "  put(3,3)  → 順序: " << cache.dumpOrder()
                                << "（容量滿，淘汰最久沒用的 2）" << endl;
        cout << "  get(2)    → " << cache.get(2) << "（已被淘汰）" << endl;
        cache.put(4, 4);   cout << "  put(4,4)  → 順序: " << cache.dumpOrder()
                                << "（淘汰 1）" << endl;
        cout << "  get(1)    → " << cache.get(1) << "（已被淘汰）" << endl;
        cout << "  get(3)    → " << cache.get(3) << endl;
        cout << "  get(4)    → " << cache.get(4) << endl;
        cout << "  ★ 命中時 splice 移到最前，map 裡的 iterator 完全不用更新" << endl;
    }

    // ===== 9. 日常實務：多優先級任務排程器 =====
    cout << "\n===== 日常實務：多優先級任務排程器 =====" << endl;
    {
        Scheduler s;
        s.medium.push_back(Task{"產生月報表", 101, string(4096, 'x')});
        s.medium.push_back(Task{"客戶資料匯出", 102, string(4096, 'x')});
        s.low.push_back(Task{"清理暫存檔", 201, string(4096, 'x')});
        s.low.push_back(Task{"重建搜尋索引", 202, string(4096, 'x')});
        s.high.push_back(Task{"修復付款失敗", 301, string(4096, 'x')});
        s.medium.push_back(Task{"Code Review", 103, string(4096, 'x')});

        cout << "  初始狀態：" << endl;
        s.dump();

        // 外部模組持有指向某任務的指標（例如進度追蹤器）
        auto it = next(s.medium.begin());          // 指向「客戶資料匯出」
        const Task* watcher = &(*it);
        cout << "\n  進度追蹤器盯著: " << watcher->name
             << "，位址 = " << static_cast<const void*>(watcher) << endl;

        // 客訴進來，把它升級為 HIGH
        s.promote(s.medium, it);
        cout << "  「客戶資料匯出」升級為 HIGH 後：" << endl;
        s.dump();
        cout << "  追蹤器的指標還有效嗎? " << boolalpha
             << (watcher == &s.high.back())
             << "（位址 = " << static_cast<const void*>(&s.high.back()) << "）" << endl;
        cout << "  ★ 零複製、指標不失效 —— 不必通知任何模組更新" << endl;

        // 系統閒置：把整條 LOW 併入 MEDIUM
        s.drainLowIntoMedium();
        cout << "\n  系統閒置，LOW 整條併入 MEDIUM（真 O(1)）：" << endl;
        s.dump();
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra 第\ 32\ 課：list\ 的特有操作——splice1.cpp -o list_splice1
//   （不加 -O2 也能編譯且零警告，但第 7 節的複雜度實測數字會失真 ——
//     見檔頭「概念補充 (B)」：-O0 下自我 splice 會呈現線性）

// ※ 第 7 節的耗時與所有節點位址（0x...）皆為**本機實測，每次執行都不同**
// ===== 複雜度實測（本機實測，每次執行都不同）=====

// === 預期輸出 ===
//   （Dell Precision 7550 / Ubuntu 26.04 / g++ 15.2.0 / -O2 編譯）。
//   請只看趨勢：整串 splice 不隨 n 成長（O(1)，數百 ns 上下跳動屬量測雜訊）；
//   跨 list 的範圍 splice 隨長度線性成長（4000→2,000,000 約 5µs→4.8ms）。
//   不要把任何絕對數值當成效能保證。
//
// ===== splice 版本 1：移植整個 list（O(1)）=====
// A  [3]: 1 2 3
// B  [3]: 10 20 30
// splice 後 A [6]: 1 10 20 30 2 3
// splice 後 B [0]: (空)
// → B 被掏空；不論 B 原本有幾個元素，都是 O(1)
//
// ===== splice 版本 2：移植單一元素（O(1)）=====
// A  [3]: 1 2 3
// B  [3]: 10 20 30
// splice 後 A [4]: 1 2 3 20
// splice 後 B [2]: 10 30
//
// ===== splice 版本 3：移植範圍（跨 list 是 O(n)！）=====
// A  [3]: 1 2 3
// B  [5]: 10 20 30 40 50
// splice 後 A [6]: 1 2 3 20 30 40
// splice 後 B [2]: 10 50
// → 注意：跨 list 的範圍版本要算 distance 來更新 size → O(n)
//
// ===== 同一 list 內 splice（重排，O(1)）=====
// 初始   [5]: 1 2 3 4 5
// 4→前面 [5]: 4 1 2 3 5
// 2→後面 [5]: 4 1 3 5 2
// → 這是 list 特有的 O(1) 元素移位能力
//
// ===== 迭代器與參考的穩定性（splice 的核心保證）=====
// splice 前 *it_20 = 20，節點位址 = 0x5e3a21364060
// splice 後 *it_20 = 20，節點位址 = 0x5e3a21364060
// 位址相同嗎? true
// 它現在屬於 A 嗎? true
// A [4]: 1 2 3 20
// B [2]: 10 30
// → 節點從未搬動，只是換了歸屬 —— 這正是 LRU Cache 的基石
//
// ===== 零複製證明：splice 完全不呼叫複製/移動建構子 =====
//   splice 整串 3 個元素：複製 0 次、移動 0 次
//   push_back+pop_front 3 個元素：複製 3 次、移動 0 次
//   ★ splice 是真正的零複製，與元素大小、數量完全無關
//
//   --- 整串 splice：標準保證 O(1) ---
//     n = 100000	耗時 145 ns
//     n = 1000000	耗時 241 ns
//     n = 4000000	耗時 196 ns
//   --- 跨 list 的範圍 splice：O(distance) ---
//     範圍長度 = 4000	耗時 4998 ns
//     範圍長度 = 40000	耗時 71766 ns
//     範圍長度 = 400000	耗時 1036773 ns
//     範圍長度 = 2000000	耗時 5503923 ns
//   ★ 整串版本平坦；範圍版本隨長度線性成長 —— 熱路徑請用整串版本
//
// ===== LeetCode 146. LRU Cache =====
//   容量 = 2
//   put(1,1)  → 順序: 1:1
//   put(2,2)  → 順序: 2:2 1:1
//   get(1)    → 1  順序: 1:1 2:2（1 被 splice 到最前）
//   put(3,3)  → 順序: 3:3 1:1（容量滿，淘汰最久沒用的 2）
//   get(2)    → -1（已被淘汰）
//   put(4,4)  → 順序: 4:4 3:3（淘汰 1）
//   get(1)    → -1（已被淘汰）
//   get(3)    → 3
//   get(4)    → 4
//   ★ 命中時 splice 移到最前，map 裡的 iterator 完全不用更新
//
// ===== 日常實務：多優先級任務排程器 =====
//   初始狀態：
//     HIGH   [1]: 修復付款失敗(#301)
//     MEDIUM [3]: 產生月報表(#101) 客戶資料匯出(#102) Code Review(#103)
//     LOW    [2]: 清理暫存檔(#201) 重建搜尋索引(#202)
//
//   進度追蹤器盯著: 客戶資料匯出，位址 = 0x5e3a21366510
//   「客戶資料匯出」升級為 HIGH 後：
//     HIGH   [2]: 修復付款失敗(#301) 客戶資料匯出(#102)
//     MEDIUM [2]: 產生月報表(#101) Code Review(#103)
//     LOW    [2]: 清理暫存檔(#201) 重建搜尋索引(#202)
//   追蹤器的指標還有效嗎? true（位址 = 0x5e3a21366510）
//   ★ 零複製、指標不失效 —— 不必通知任何模組更新
//
//   系統閒置，LOW 整條併入 MEDIUM（真 O(1)）：
//     HIGH   [2]: 修復付款失敗(#301) 客戶資料匯出(#102)
//     MEDIUM [4]: 產生月報表(#101) Code Review(#103) 清理暫存檔(#201) 重建搜尋索引(#202)
//     LOW    [0]: (空)
