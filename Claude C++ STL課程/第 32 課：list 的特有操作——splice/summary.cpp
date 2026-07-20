// =============================================================================
//  第 32 課 總結：list 的特有操作——splice  —  本課的教科書
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：#include <list>
//   splice = 「接合」。把節點從一條鏈拆下來、接到另一條鏈上。
//
//   ┌─ 三個重載與複雜度（[list.ops]）────────────────────────────────────┐
//   │  A.splice(pos, B)                    整串    O(1)                  │
//   │  A.splice(pos, B, it)                單元素  O(1)                  │
//   │  A.splice(pos, B, first, last)       範圍    &B==&A 時 O(1)        │
//   │                                              否則 O(distance)      │
//   │  （每個都有對應的 list&& 右值版本，行為相同）                      │
//   └─────────────────────────────────────────────────────────────────────┘
//
//   ★★ 核心保證（整堂課只要記住這一句）★★
//     splice **不複製、不移動、不配置、不解構**任何元素，
//     指向被搬移元素的 **iterator / reference / pointer 全部保持有效**，
//     只是「歸屬」換到了目標 list。
//
// 【詳細解釋 Explanation】
//
// 【1. splice 做了什麼：六根指標的重接】
// list 是雙向鏈結串列，每個節點有 prev / next。把節點 X 從 B 搬到 A 的 pos 前：
//
//   搬移前：  B: ... ─ P ⇄ X ⇄ Q ─ ...        A: ... ─ M ⇄ pos ─ ...
//   搬移後：  B: ... ─ P ⇄ Q ─ ...            A: ... ─ M ⇄ X ⇄ pos ─ ...
//
//   P.next=Q, Q.prev=P（從 B 摘掉）
//   M.next=X, X.prev=M, X.next=pos, pos.prev=X（接進 A）
//   **節點本身連同裡面的元素，從頭到尾沒有搬動過一個位元組。**
//
// 【2. 與「push_back + erase」的天壤之別】
//   等效但昂貴的寫法：
//       A.push_back(*it);   // 複製建構新元素（可能配置記憶體）
//       B.erase(it);        // 解構舊元素（可能釋放記憶體）
//   代價：一次複製建構 + 一次解構 + 可能的配置/釋放，而且**原 iterator 全部失效**。
//   splice 的代價：六次指標賦值。
//   本課 main() 用計數型別實測驗證：
//       splice 整串 3 個元素            → 複製 0 次、移動 0 次
//       push_back + pop_front 3 個元素  → 複製 3 次
//   對 list<string>、list<vector<int>> 這類複製昂貴的元素，差距是數量級的。
//
// 【3. ★ 為什麼範圍版本跨 list 是 O(n)：size() 是罪魁禍首 ★】
//   C++11 起標準要求 list::size() 必須是 **O(1)**，因此 list 內部要維護一個
//   size 計數器。範圍 splice 於是面臨一個問題：「我到底搬了幾個節點？」
//   指標重接本身是 O(1)，但要更新**兩邊**的 size 計數器就必須知道長度，
//   而唯一辦法是 std::distance(first, last) —— 走一遍，O(n)。
//   若 &other == this（同一條 list 內搬），總數不變、不需要知道長度 → O(1)。
//   ★ 這是「C++11 把 size() 從 O(n) 改成 O(1)」付出的代價，
//     是少數「新標準讓某個操作變慢」的例子。C++98 的範圍 splice 全是 O(1)。
//
// 【4. 同一條 list 內 splice：O(1) 的元素移位】
//   lst.splice(lst.begin(), lst, it);   // 把 it 指的元素移到最前面
//   這是 list 獨有的能力（vector / deque 都做不到 O(1) 移位）。
//   限制：pos 不可落在被搬移的範圍 [first, last) 內，否則是未定義行為
//         （鏈會接成環）。單元素版本若 pos == it 或 pos == next(it) 則是 no-op。
//
// 【5. 為什麼只有 list / forward_list 有 splice】
//   vector / deque 的元素是「值的陣列」——元素的位置由索引決定，
//   沒有「獨立節點」可以拆下來重接。要把元素從一個 vector 搬到另一個，
//   只能真的複製/移動資料。
//   list 的元素則各自住在獨立配置的節點裡，節點之間靠指標連結，
//   所以「換歸屬」只是改指標。這是節點式容器的根本優勢。
//   （同理，map/set 有 extract()/merge() —— 那是 C++17 為節點式關聯容器
//     提供的對應能力。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) ★ libstdc++ 的實作與標準保證的微妙落差（本機實測）
//   讀 /usr/include/c++/15/bits/stl_list.h 可以看到範圍 splice **無條件**
//   呼叫 std::distance（沒有先判斷 this == &__x）：
//       size_t __n = std::distance(__first, __last);
//       this->_M_inc_size(__n);  __x._M_dec_size(__n);
//   同一條 list 時這兩行加減互相抵消，但 distance 仍然被算了 ——
//   那不就違反標準的 O(1) 保證？本機用獨立小型 benchmark 實測（非本檔輸出）：
//       -O0：範圍長度 4000→2,000,000，耗時 12µs→7,346µs  → **確實是線性的**
//       -O2：範圍長度 4000→2,000,000，耗時 185ns→168ns   → **平坦，O(1)**
//   原因：-O2 內聯後編譯器看出 distance 的結果只用於一加一減、互相抵消，
//   且該迴圈是純指標走訪、無副作用，於是**整個迴圈被最佳化消除**。
//   → 教訓：標準規範的是**可觀察行為**，實作可用任何方式達成。
//     效能測試務必在開最佳化的組態下做 —— debug build 的數字可能
//     連**複雜度等級**都不同，不只是常數倍差距。
//
// (B) splice 為什麼是 noexcept
//   整串與單元素版本標記為 noexcept：它們不配置記憶體、不呼叫使用者的
//   建構子或解構子，只改指標 —— 沒有任何可能拋例外的動作。
//   這讓 splice 可以安全地用在 noexcept 的移動建構子、解構子、
//   以及例外處理的清理路徑中。
//
// (C) allocator 必須相等
//   兩個 list 的 allocator 若不相等，splice 是未定義行為。
//   原因：節點是用 B 的 allocator 配置的，掛到 A 之後，A 解構時會用 A 的
//   allocator 去釋放它 —— 配置與釋放不匹配。
//   用預設 allocator 時不必擔心（所有 std::allocator<T> 實例都相等）。
//
// (D) splice 與 merge / sort 的關係
//   list::merge 與 list::sort 內部就是靠 splice 實作的 ——
//   它們把節點在鏈之間重接，完全不搬移元素資料。
//   這正是「list 的 sort 不能用 std::sort」的原因：std::sort 需要
//   random access iterator 並且靠交換**值**來排序；list::sort 則是
//   歸併排序 + 指標重接，複雜度 O(n log n) 但完全不動元素本體。
//
// 【注意事項 Pay Attention】
// 1. **pos 不可落在 [first, last) 之內**，否則是未定義行為（鏈會接成環）。
// 2. first / last 必須真的是 other 的迭代器。傳入不屬於 other 的迭代器是
//    未定義行為，而且**沒有任何檢查或錯誤訊息** —— 這類 bug 極難追查。
// 3. splice 後兩邊的 size 都會被正確更新。
// 4. 被搬移元素的 iterator / reference / pointer **保持有效**，但歸屬已改變
//    （拿它們去呼叫 other.erase() 是未定義行為）。
// 5. 範圍版本跨 list 時是 O(n)，別在熱路徑上誤以為它是 O(1)。
//    要搬整條 list 請用整串版本，那才是真 O(1)。
// 6. vector / deque **沒有** splice。需要 O(1) 轉移節點就只能用 list。
// 7. 本課的耗時皆為**本機實測，每次執行、每台機器都不同**，只看趨勢。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list::splice
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. splice 做了什麼？為什麼能做到 O(1)？
//     答：把節點從一條鏈拆下、接到另一條鏈上，**純指標重接**（約六次指標賦值）。
//         它不複製、不移動、不配置、不解構任何元素 —— 節點連同裡面的資料
//         從頭到尾沒搬動過一個位元組，所以與元素大小、元素型別完全無關。
//     追問：那 A.push_back(*it); B.erase(it); 不是等效嗎？
//         → 語意結果相同，成本天差地遠：那是一次複製建構 + 一次解構
//           （外加可能的配置/釋放），且原本的 iterator 全部失效。
//           本課實測：splice 3 個元素複製 0 次，push_back+pop_front 複製 3 次。
//
// 🔥 Q2. splice 之後，指向被搬移元素的 iterator / reference 還有效嗎？
//     答：**完全有效**，這是 splice 最重要的性質。節點沒有被搬動，
//         iterator 內存的節點位址依然正確；改變的只有它的**歸屬**。
//     追問：這個保證有什麼實際價值？
//         → 這正是 LRU Cache 能 O(1) 的關鍵：unordered_map 裡存著 list 的
//           iterator，快取命中時用 splice 把節點移到最前面，
//           **map 裡的 iterator 完全不用更新**。
//           若改用 erase + push_front，節點被銷毀重建、iterator 失效，
//           每次都得回頭更新 map，還多一次複製建構。
//           iterator 穩定性在這裡不是加分項，而是整個設計成立的前提。
//
// 🔥 Q3. splice 三個重載的複雜度分別是什麼？為什麼範圍版本特別？
//     答：整串 O(1)、單元素 O(1)；範圍版本是 **&other == this 時 O(1)，
//         跨 list 時 O(distance(first,last))**。
//         原因是 C++11 起 size() 必須 O(1)，list 得維護 size 計數器；
//         跨 list 搬移時兩邊計數器都要更新，就必須先知道搬了幾個
//         → std::distance 走一遍 → O(n)。同一條 list 內總數不變 → O(1)。
//     追問：這代表 C++98 的 splice 比 C++11 快嗎？
//         → 在「跨 list 的範圍 splice」這一項上確實是。C++98 的 size() 允許
//           O(n)，不需要計數器，所以範圍 splice 全是 O(1)。
//           這是少數「新標準讓某操作變慢」的例子 —— 換來的是 O(1) 的 size()。
//
// 🔥 Q4. 為什麼 vector 和 deque 沒有 splice？
//     答：因為它們的元素是「值的陣列」，位置由索引決定，沒有可以拆下來重接的
//         獨立節點。要把元素從一個 vector 搬到另一個，只能真的複製/移動資料。
//         list 的元素各自住在獨立配置的節點裡、靠指標連結，
//         所以「換歸屬」只需改指標。這是節點式容器的根本優勢。
//     追問：那 map / set 有類似的能力嗎？
//         → 有。C++17 為節點式關聯容器加入了 extract() 與 merge()，
//           可以把節點從一個 map 取出（不解構、不重新配置）再插入另一個，
//           概念與 splice 完全相同。
//
// ⚠️ 陷阱 1. 「splice 是 O(1)，所以熱路徑上大量用範圍版本沒問題」——錯在哪？
//     答：跨 list 的**範圍版本是 O(n)**，不是 O(1)。只有「整串」與「單元素」
//         才無條件 O(1)。本機 -O2 實測：跨 list 搬 2,000,000 個節點約 4,845µs，
//         搬 4,000 個約 5µs —— 清楚的線性關係。
//         要搬整條 list 請用整串版本 A.splice(pos, B)，那才是真 O(1)。
//     為什麼會錯：把「splice 靠指標重接所以 O(1)」這個直覺無條件外推。
//         指標重接確實 O(1)，但**維護 size 計數器**需要知道長度，這才是瓶頸。
//
// ⚠️ 陷阱 2. 同一條 list 內做範圍 splice，標準說 O(1)，你卻在 debug build
//            量到它隨長度線性增加。是標準錯了、還是實作有 bug？
//     答：都不是。libstdc++ 的範圍 splice **無條件**呼叫 std::distance
//         （沒先判斷是否同一條 list），所以原始碼層面確實是線性的。
//         但 -O2 下編譯器看出 distance 的結果只用於一加一減、互相抵消，
//         且迴圈無副作用，於是把整個迴圈最佳化掉，回到 O(1)。
//         本機實測：-O0 時 4000→2,000,000 個節點耗時 12µs→7,346µs（線性）；
//         -O2 時同樣範圍 185ns→168ns（平坦）。
//     為什麼會錯：以為「複雜度保證」是實作必須逐行照做的規則。
//         標準規範的是**可觀察行為**，編譯器有權以任何方式達成 ——
//         包括把違反該複雜度的程式碼最佳化到符合。
//         所以效能測試一定要在開最佳化的組態下做。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <chrono>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]:";
    for (const auto& v : lst) cout << " " << v;
    if (lst.empty()) cout << " (空)";
    cout << endl;
}

// 證明「splice 零複製」的計數型別。
// 注意：必須放在檔案範圍 —— 區域類別不允許有 static 資料成員。
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
//         也是少數「非 splice 不可」的場景。
//         結構：list 維護使用順序（最前 = 最近使用）；
//               unordered_map 存 key → list 節點的 iterator。
//         命中時要把該項移到最前面：
//           * splice → **O(1)，且 map 裡的 iterator 依然有效、不必更新**
//           * erase + push_front → 節點被銷毀重建，iterator 失效，
//             每次都得回頭更新 map，還多一次複製建構
//         iterator 穩定性在這裡是整個 O(1) 設計成立的**前提**，不是附加好處。
//   複雜度：get / put 皆 O(1)（雜湊查找攤還 O(1) + splice O(1)）。
// -----------------------------------------------------------------------------
class LRUCache {
    using Entry = pair<int, int>;                    // (key, value)
    list<Entry>                               order_;   // 最前 = 最近使用
    unordered_map<int, list<Entry>::iterator> index_;   // key → 節點位置
    size_t                                    capacity_;

public:
    explicit LRUCache(int capacity)
        : capacity_(static_cast<size_t>(capacity)) {}

    int get(int key) {
        auto found = index_.find(key);
        if (found == index_.end()) return -1;
        // ★ 命中：把節點移到最前。O(1)，且 found->second 在 splice 後仍有效
        order_.splice(order_.begin(), order_, found->second);
        return found->second->second;
    }

    void put(int key, int value) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = value;
            order_.splice(order_.begin(), order_, found->second);
            return;
        }
        if (order_.size() >= capacity_) {
            index_.erase(order_.back().first);       // 淘汰最久沒用的
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        index_[key] = order_.begin();
    }

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
// 【日常實務範例 1】多優先級任務排程器：任務在佇列之間升級 / 降級
//   情境：排程器有 HIGH / MEDIUM / LOW 三條佇列。營運中任務會改變優先級 ——
//         客訴進來要把某任務升成 HIGH；系統閒置時把 LOW 整條併入 MEDIUM。
//   為什麼用 splice：
//     * 任務物件可能很大（描述、附件、上下文），複製成本高 → splice 零複製。
//     * 其他模組（進度追蹤、通知）可能持有指向任務的 pointer/iterator，
//       splice 後那些指標**依然有效**，不必通知任何人更新。
//     * 「LOW 整條併入 MEDIUM」用整串版本是真 O(1)，
//       不論裡面有 10 個還是 10 萬個任務。
// -----------------------------------------------------------------------------
struct Task {
    string name;
    int    id;
    string payload;    // 模擬較大的任務內容
};

ostream& operator<<(ostream& os, const Task& t) {
    return os << t.name << "(#" << t.id << ")";
}

class Scheduler {
public:
    list<Task> high, medium, low;

    void promote(list<Task>& from, list<Task>::iterator it) {
        high.splice(high.end(), from, it);            // O(1)，零複製
    }
    void drainLowIntoMedium() {
        medium.splice(medium.end(), low);             // 整串，真 O(1)
    }
    void dump() const {
        print("HIGH  ", high);
        print("MEDIUM", medium);
        print("LOW   ", low);
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】連線池：閒置 / 使用中兩條 list 之間搬移連線
//   情境：資料庫連線池維護「閒置」與「使用中」兩條 list。
//         取用連線時把它從 idle 搬到 busy，歸還時搬回來。
//         連線物件持有 socket fd、緩衝區、交易狀態 —— **絕對不能被複製**
//         （複製一個連線物件在語意上根本是錯的，會導致 fd 被關兩次）。
//   為什麼用 splice：
//     * 連線物件不可複製也不需要移動 —— splice 只改指標，完全不碰物件本體。
//     * 借出去的程式碼可以安全持有 Connection*，
//       即使連線在兩條 list 之間搬移，**指標依然有效**。
//     * 每次借還都是 O(1)，連線池是熱路徑，這點很關鍵。
//   注意：這裡示範 splice 最貼近生產環境的用途 —— 管理「資源的狀態轉移」。
// -----------------------------------------------------------------------------
struct Connection {
    int    fd;
    string endpoint;
    long   queriesServed = 0;
};

ostream& operator<<(ostream& os, const Connection& c) {
    return os << "fd" << c.fd << "(" << c.queriesServed << "次)";
}

class ConnectionPool {
    list<Connection> idle_, busy_;
public:
    explicit ConnectionPool(int n, const string& endpoint) {
        for (int i = 0; i < n; ++i) idle_.push_back(Connection{100 + i, endpoint, 0});
    }
    // 借出一條連線：從 idle 搬到 busy，回傳穩定的指標
    Connection* acquire() {
        if (idle_.empty()) return nullptr;
        busy_.splice(busy_.end(), idle_, idle_.begin());   // O(1)，零複製
        return &busy_.back();
    }
    // 歸還：從 busy 搬回 idle（用指標找到節點的 iterator）
    void release(Connection* conn) {
        for (auto it = busy_.begin(); it != busy_.end(); ++it) {
            if (&(*it) == conn) {
                idle_.splice(idle_.end(), busy_, it);       // O(1)
                return;
            }
        }
    }
    void dump() const {
        print("閒置", idle_);
        print("使用中", busy_);
    }
};

int main() {
    cout << "========== 一、三個重載 ==========" << endl;

    cout << "----- 1. 整串 splice（O(1））-----" << endl;
    {
        list<int> A = {1, 2, 3}, B = {10, 20, 30};
        print("A(前)", A);
        print("B(前)", B);
        auto pos = A.begin(); advance(pos, 1);
        A.splice(pos, B);
        print("A(後)", A);
        print("B(後)", B);
        cout << "  ★ B 被掏空；不論 B 有幾個元素都是 O(1)" << endl;
    }

    cout << "\n----- 2. 單一元素 splice（O(1））-----" << endl;
    {
        list<int> A = {1, 2, 3}, B = {10, 20, 30};
        auto elem = B.begin(); advance(elem, 1);   // 指向 20
        A.splice(A.end(), B, elem);
        print("A(後)", A);
        print("B(後)", B);
        cout << "  ★ LRU Cache 用的就是這個版本" << endl;
    }

    cout << "\n----- 3. 範圍 splice（跨 list 是 O(n)！）-----" << endl;
    {
        list<int> A = {1, 2, 3}, B = {10, 20, 30, 40, 50};
        auto f = B.begin(); advance(f, 1);   // 20
        auto l = B.begin(); advance(l, 4);   // 50（不含）
        A.splice(A.end(), B, f, l);
        print("A(後)", A);
        print("B(後)", B);
        cout << "  ★ 要算 distance 更新 size → O(distance)" << endl;
    }

    cout << "\n----- 4. 同一條 list 內重排（O(1））-----" << endl;
    {
        list<int> lst = {1, 2, 3, 4, 5};
        print("初始  ", lst);
        auto elem = lst.begin(); advance(elem, 3);   // 指向 4
        lst.splice(lst.begin(), lst, elem);
        print("4→最前", lst);
        elem = lst.begin(); advance(elem, 2);        // {4,1,2,3,5} 的第 3 個 = 2
        lst.splice(lst.end(), lst, elem);
        print("2→最後", lst);
        cout << "  ★ list 獨有的 O(1) 元素移位能力" << endl;
    }

    cout << "\n========== 二、iterator / reference 穩定性（核心保證）==========" << endl;
    {
        list<int> A = {1, 2, 3}, B = {10, 20, 30};
        auto it20 = B.begin(); advance(it20, 1);
        const int* addr = &(*it20);
        cout << "  splice 前 *it20=" << *it20
             << " 節點位址=" << static_cast<const void*>(addr) << endl;
        A.splice(A.end(), B, it20);
        cout << "  splice 後 *it20=" << *it20
             << " 節點位址=" << static_cast<const void*>(&(*it20)) << endl;
        cout << "  位址相同嗎? " << boolalpha << (addr == &(*it20)) << endl;
        cout << "  現在屬於 A 嗎? " << (&A.back() == addr) << endl;
        print("A", A);
        print("B", B);
        cout << "  ★ 節點從未搬動，只換了歸屬 —— LRU Cache 的基石" << endl;
    }

    cout << "\n========== 三、零複製證明 ==========" << endl;
    {
        list<Tracked> A1, B1;
        for (int i = 0; i < 3; ++i) B1.emplace_back(i);
        Tracked::copies = 0; Tracked::moves = 0;
        A1.splice(A1.end(), B1);
        cout << "  splice 整串 3 個元素      ：複製 " << Tracked::copies
             << " 次、移動 " << Tracked::moves << " 次" << endl;

        list<Tracked> A2, B2;
        for (int i = 0; i < 3; ++i) B2.emplace_back(i);
        Tracked::copies = 0; Tracked::moves = 0;
        while (!B2.empty()) { A2.push_back(B2.front()); B2.pop_front(); }
        cout << "  push_back+pop_front 3 個  ：複製 " << Tracked::copies
             << " 次、移動 " << Tracked::moves << " 次" << endl;
        cout << "  ★ splice 是真正的零複製，與元素大小/型別完全無關" << endl;
    }

    cout << "\n========== 四、複雜度實測（本機實測，每次執行都不同）==========" << endl;
    {
        using namespace chrono;
        cout << "  --- 整串 splice：O(1)，不隨 n 成長 ---" << endl;
        for (int n : {100000, 1000000, 4000000}) {
            list<int> a, b;
            for (int i = 0; i < n; ++i) b.push_back(i);
            auto t1 = high_resolution_clock::now();
            a.splice(a.begin(), b);
            auto t2 = high_resolution_clock::now();
            cout << "    n=" << n << "\t耗時 "
                 << duration_cast<nanoseconds>(t2 - t1).count() << " ns" << endl;
        }
        cout << "  --- 跨 list 範圍 splice：O(distance)，線性成長 ---" << endl;
        for (int len : {4000, 40000, 400000, 2000000}) {
            list<int> a, b;
            for (int i = 0; i < 4000000; ++i) b.push_back(i);
            auto first = next(b.begin(), 10);
            auto last  = next(first, len);
            auto t1 = high_resolution_clock::now();
            a.splice(a.begin(), b, first, last);
            auto t2 = high_resolution_clock::now();
            cout << "    範圍長度=" << len << "\t耗時 "
                 << duration_cast<nanoseconds>(t2 - t1).count() << " ns" << endl;
        }
        cout << "  ★ 熱路徑要搬整條 list 請用整串版本，別用範圍版本" << endl;
    }

    cout << "\n========== 五、LeetCode 146. LRU Cache ==========" << endl;
    {
        LRUCache cache(2);
        cout << "  容量 = 2" << endl;
        cache.put(1, 1);  cout << "  put(1,1) → " << cache.dumpOrder() << endl;
        cache.put(2, 2);  cout << "  put(2,2) → " << cache.dumpOrder() << endl;
        cout << "  get(1)   → " << cache.get(1) << "  順序: " << cache.dumpOrder()
             << "（1 被 splice 到最前）" << endl;
        cache.put(3, 3);  cout << "  put(3,3) → " << cache.dumpOrder()
                               << "（滿了，淘汰最久沒用的 2）" << endl;
        cout << "  get(2)   → " << cache.get(2) << "（已淘汰）" << endl;
        cache.put(4, 4);  cout << "  put(4,4) → " << cache.dumpOrder()
                               << "（淘汰 1）" << endl;
        cout << "  get(1)   → " << cache.get(1) << "（已淘汰）" << endl;
        cout << "  get(3)   → " << cache.get(3) << endl;
        cout << "  get(4)   → " << cache.get(4) << endl;
        cout << "  ★ 命中時 splice 移到最前，map 裡的 iterator 完全不用更新" << endl;
    }

    cout << "\n========== 六、日常實務：多優先級任務排程器 ==========" << endl;
    {
        Scheduler s;
        s.medium.push_back(Task{"產生月報表",   101, string(4096, 'x')});
        s.medium.push_back(Task{"客戶資料匯出", 102, string(4096, 'x')});
        s.low.push_back(Task{"清理暫存檔",     201, string(4096, 'x')});
        s.low.push_back(Task{"重建搜尋索引",   202, string(4096, 'x')});
        s.high.push_back(Task{"修復付款失敗",  301, string(4096, 'x')});
        s.medium.push_back(Task{"Code Review",  103, string(4096, 'x')});

        cout << "  初始：" << endl;
        s.dump();

        auto it = next(s.medium.begin());          // 「客戶資料匯出」
        const Task* watcher = &(*it);              // 外部模組持有的指標
        cout << "\n  進度追蹤器盯著: " << watcher->name
             << "，位址=" << static_cast<const void*>(watcher) << endl;

        s.promote(s.medium, it);
        cout << "  升級為 HIGH 後：" << endl;
        s.dump();
        cout << "  追蹤器指標還有效嗎? " << boolalpha
             << (watcher == &s.high.back())
             << "（位址=" << static_cast<const void*>(&s.high.back()) << "）" << endl;

        s.drainLowIntoMedium();
        cout << "\n  系統閒置，LOW 整條併入 MEDIUM（真 O(1)）：" << endl;
        s.dump();
        cout << "  ★ 零複製、指標不失效 —— 不必通知任何模組" << endl;
    }

    cout << "\n========== 七、日常實務：資料庫連線池 ==========" << endl;
    {
        ConnectionPool pool(4, "db-primary:5432");
        cout << "  初始：" << endl;
        pool.dump();

        Connection* c1 = pool.acquire();
        Connection* c2 = pool.acquire();
        cout << "\n  借出兩條連線後：" << endl;
        pool.dump();

        // 借用方持有 Connection*，在兩條 list 之間搬移後依然有效
        c1->queriesServed += 12;
        c2->queriesServed += 5;
        cout << "\n  使用中… c1=" << *c1 << "  c2=" << *c2 << endl;

        pool.release(c1);
        cout << "\n  歸還 c1 後：" << endl;
        pool.dump();
        cout << "  c1 指標仍指向同一個連線物件: " << *c1 << endl;
        cout << "  ★ Connection 含 fd/緩衝區，複製語意上就是錯的 ——" << endl;
        cout << "    splice 只改指標，物件本體從未被複製或移動" << endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -O2 -Wall -Wextra summary.cpp -o summary
//   （不加 -O2 也能編譯且零警告，但第四節的複雜度實測數字會失真）

// ※ 第四節的耗時與所有節點位址（0x...）皆為**本機實測，每次執行都不同**
// ========== 四、複雜度實測（本機實測，每次執行都不同）==========

// === 預期輸出 ===
//   （Dell Precision 7550 / Ubuntu 26.04 / g++ 15.2.0 / -O2 編譯）。
//   請只看趨勢：整串 splice 不隨 n 成長（O(1)，百餘 ns 上下跳動屬量測雜訊）；
//   跨 list 的範圍 splice 隨長度線性成長（4000→2,000,000 約 5µs→5ms）。
//   不要把任何絕對數值當成效能保證。
//
// ========== 一、三個重載 ==========
// ----- 1. 整串 splice（O(1））-----
//   A(前) [3]: 1 2 3
//   B(前) [3]: 10 20 30
//   A(後) [6]: 1 10 20 30 2 3
//   B(後) [0]: (空)
//   ★ B 被掏空；不論 B 有幾個元素都是 O(1)
//
// ----- 2. 單一元素 splice（O(1））-----
//   A(後) [4]: 1 2 3 20
//   B(後) [2]: 10 30
//   ★ LRU Cache 用的就是這個版本
//
// ----- 3. 範圍 splice（跨 list 是 O(n)！）-----
//   A(後) [6]: 1 2 3 20 30 40
//   B(後) [2]: 10 50
//   ★ 要算 distance 更新 size → O(distance)
//
// ----- 4. 同一條 list 內重排（O(1））-----
//   初始   [5]: 1 2 3 4 5
//   4→最前 [5]: 4 1 2 3 5
//   2→最後 [5]: 4 1 3 5 2
//   ★ list 獨有的 O(1) 元素移位能力
//
// ========== 二、iterator / reference 穩定性（核心保證）==========
//   splice 前 *it20=20 節點位址=0x623f50d91060
//   splice 後 *it20=20 節點位址=0x623f50d91060
//   位址相同嗎? true
//   現在屬於 A 嗎? true
//   A [4]: 1 2 3 20
//   B [2]: 10 30
//   ★ 節點從未搬動，只換了歸屬 —— LRU Cache 的基石
//
// ========== 三、零複製證明 ==========
//   splice 整串 3 個元素      ：複製 0 次、移動 0 次
//   push_back+pop_front 3 個  ：複製 3 次、移動 0 次
//   ★ splice 是真正的零複製，與元素大小/型別完全無關
//
//   --- 整串 splice：O(1)，不隨 n 成長 ---
//     n=100000	耗時 116 ns
//     n=1000000	耗時 162 ns
//     n=4000000	耗時 199 ns
//   --- 跨 list 範圍 splice：O(distance)，線性成長 ---
//     範圍長度=4000	耗時 5331 ns
//     範圍長度=40000	耗時 91749 ns
//     範圍長度=400000	耗時 973713 ns
//     範圍長度=2000000	耗時 5004497 ns
//   ★ 熱路徑要搬整條 list 請用整串版本，別用範圍版本
//
// ========== 五、LeetCode 146. LRU Cache ==========
//   容量 = 2
//   put(1,1) → 1:1
//   put(2,2) → 2:2 1:1
//   get(1)   → 1  順序: 1:1 2:2（1 被 splice 到最前）
//   put(3,3) → 3:3 1:1（滿了，淘汰最久沒用的 2）
//   get(2)   → -1（已淘汰）
//   put(4,4) → 4:4 3:3（淘汰 1）
//   get(1)   → -1（已淘汰）
//   get(3)   → 3
//   get(4)   → 4
//   ★ 命中時 splice 移到最前，map 裡的 iterator 完全不用更新
//
// ========== 六、日常實務：多優先級任務排程器 ==========
//   初始：
//   HIGH   [1]: 修復付款失敗(#301)
//   MEDIUM [3]: 產生月報表(#101) 客戶資料匯出(#102) Code Review(#103)
//   LOW    [2]: 清理暫存檔(#201) 重建搜尋索引(#202)
//
//   進度追蹤器盯著: 客戶資料匯出，位址=0x623f50d93510
//   升級為 HIGH 後：
//   HIGH   [2]: 修復付款失敗(#301) 客戶資料匯出(#102)
//   MEDIUM [2]: 產生月報表(#101) Code Review(#103)
//   LOW    [2]: 清理暫存檔(#201) 重建搜尋索引(#202)
//   追蹤器指標還有效嗎? true（位址=0x623f50d93510）
//
//   系統閒置，LOW 整條併入 MEDIUM（真 O(1)）：
//   HIGH   [2]: 修復付款失敗(#301) 客戶資料匯出(#102)
//   MEDIUM [4]: 產生月報表(#101) Code Review(#103) 清理暫存檔(#201) 重建搜尋索引(#202)
//   LOW    [0]: (空)
//   ★ 零複製、指標不失效 —— 不必通知任何模組
//
// ========== 七、日常實務：資料庫連線池 ==========
//   初始：
//   閒置 [4]: fd100(0次) fd101(0次) fd102(0次) fd103(0次)
//   使用中 [0]: (空)
//
//   借出兩條連線後：
//   閒置 [2]: fd102(0次) fd103(0次)
//   使用中 [2]: fd100(0次) fd101(0次)
//
//   使用中… c1=fd100(12次)  c2=fd101(5次)
//
//   歸還 c1 後：
//   閒置 [3]: fd102(0次) fd103(0次) fd100(12次)
//   使用中 [1]: fd101(5次)
//   c1 指標仍指向同一個連線物件: fd100(12次)
//   ★ Connection 含 fd/緩衝區，複製語意上就是錯的 ——
//     splice 只改指標，物件本體從未被複製或移動
