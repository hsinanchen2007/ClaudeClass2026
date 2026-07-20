// =============================================================================
//  summary.cpp  —  list 的元素操作：push / pop / insert / erase
//                  與「STL 中最寬鬆的迭代器失效規則」
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <list>
//   宣告:   template<class T, class Allocator = std::allocator<T>> class list;
//   結構:   雙向鏈結串列（doubly linked list），每個節點含 prev / next / value
//
//   操作與複雜度:
//     push_back(v)  / push_front(v)          O(1)
//     emplace_back(args...) / emplace_front   O(1)（原地建構，不多一次搬移）
//     pop_back()    / pop_front()            O(1)
//     insert(pos, v)                         O(1)  ← 前提是你已經有 pos
//     insert(pos, n, v) / (pos, first, last) O(插入個數)
//     erase(pos)                             O(1)  ← 前提是你已經有 pos
//     erase(first, last)                     O(範圍長度)
//     size() / empty()                       O(1)（C++11 起要求 O(1)）
//     ★ 沒有 operator[] / at()：不支援隨機存取
//     ★ 「找到 pos」本身是 O(n)——這是最常被忽略的那一半成本
//
//   回傳值:
//     insert(pos, v) → 指向**新插入元素**的 iterator
//     erase(pos)     → 指向**被刪元素的下一個**的 iterator（迴圈刪除的關鍵）
//
// 【詳細解釋 Explanation】
//
// 【1. O(1) 的真相:「插入/刪除 O(1)」少講了前半句】
// 教科書說 list 的 insert/erase 是 O(1)，vector 是 O(n)，於是很多人得出
// 「要頻繁插刪就用 list」的結論。但完整的敘述是：
//     list 的 insert/erase 在**你已經持有該位置的 iterator 時**是 O(1)。
// 如果你只知道「值」或「第 k 個」，得先走過去：
//     auto it = lst.begin(); std::advance(it, k);   // O(k)
//     lst.insert(it, v);                            // O(1)
// 總成本仍是 O(n)。而 vector 雖然搬移是 O(n)，但搬的是連續記憶體
// （memmove，SIMD + cache 友善），常數極小。
//
// 實務結論：list 真正勝出的場景是
//   「iterator 早就在手上」+「元素本身昂貴或不可搬移」+「需要 iterator 穩定」。
// 典型代表就是 LRU cache（本檔範例 8）：hash map 存著 iterator，
// 所以永遠不需要「走過去找」，每次操作都是真正的 O(1)。
//
// 【2. 失效規則:list 是整個 STL 中最寬鬆的】
//   * insert(...)  → **不使任何 iterator / reference / pointer 失效**。
//   * erase(pos)   → **只有被刪除元素**的 iterator/reference 失效，
//                    其他全部繼續有效。
//   * push/pop/emplace 同理（它們就是 insert/erase 的包裝）。
//
// 為什麼能這麼寬鬆？因為節點各自獨立配置在堆積上，插入/刪除只是改動
// 相鄰節點的 prev/next 指標——**既有節點的位址從頭到尾沒有動過**。
// 對照組：
//     vector：擴容搬移全部元素 → iterator 與 reference 一起失效
//     deque ：頭尾插入 → iterator 全失效，但 reference 仍有效
//     list  ：什麼都不失效（除了被刪的那個）
// 這個性質是 list 存在的**主要理由**，也是面試最愛的對照題。
//
// 【3. erase 的回傳值與迴圈刪除的正確寫法】
// erase(pos) 回傳「被刪元素的下一個」iterator。標準寫法：
//     for (auto it = lst.begin(); it != lst.end(); ) {
//         if (要刪) it = lst.erase(it);   // erase 已經幫你前進了
//         else      ++it;                 // 只有不刪時才自己前進
//     }
// 常見錯誤是在 for 的第三段寫 ++it，導致刪除後又多跳一格（漏掉元素）；
// 所以上面的 for 第三段刻意留空。
//
// 【4. lst.erase(it++) 為什麼對 list 可行、對 vector 卻不行】
// `it++` 的語意是：先把 it 前進到下一個，再把**舊值**當引數傳給 erase。
//   * 對 **list**：erase 只會讓「被刪元素」的 iterator 失效，
//     而 it 早已指向下一個節點（那個節點沒被動到）→ 安全可行。
//     本檔範例 6 有實際執行驗證。
//   * 對 **vector**：erase 會讓「刪除點**之後**的所有 iterator」失效，
//     而前進後的 it 正好落在那個範圍內 → 它已經失效了 → UB。
// 雖然 erase(it++) 對 list 合法，仍建議統一寫 `it = lst.erase(it)`：
// 語意直白、對所有序列容器都正確，也不必讓讀者去推敲遞增與求值的順序。
//
// 【5. size() 的歷史:C++11 之後才保證 O(1)】
// C++11 起，標準**要求** list::size() 為常數時間，所以實作必須額外維護一個
// 計數欄位。在此之前（C++98/03）標準只要求「線性時間可接受」，
// libstdc++ 曾經真的是走一遍串列來數 —— 在迴圈裡呼叫 size() 會讓
// O(n) 的程式默默變成 O(n²)。這也是為什麼老程式碼常見
// `if (lst.empty())` 而不是 `if (lst.size() == 0)`：empty() 一直都是 O(1)。
// 今天在 C++11 以後的環境兩者都是 O(1)，但 empty() 仍是更清楚的寫法。
// （順帶一提：forward_list 為了零額外開銷，**刻意連 size() 都不提供**。）
//
// 【6. 沒有 operator[]:這是誠實，不是缺陷】
// list 不提供 operator[] / at()，因為對鏈結串列而言「第 k 個」必然是 O(k)。
// 如果標準提供了 lst[k]，使用者會很自然地寫出
//     for (size_t i = 0; i < lst.size(); ++i) use(lst[i]);   // 實際是 O(n²)
// 卻以為是 O(n)。不提供這個介面，等於用型別系統**強迫你面對真實成本**。
// 要取第 k 個請明確寫 std::advance / std::next，讓 O(k) 顯而易見。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 每個節點的記憶體代價
//   節點 = prev 指標 + next 指標 + 元素值（+ 對齊填補）。
//   本檔用一個「計數配置器」實測 std::list<int>：
//     每節點 24 bytes = 8(prev) + 8(next) + 4(int) + 4(padding)
//   也就是存 4 bytes 的資料要付 24 bytes，**overhead 500%**；
//   而 vector<int> 每個元素就是 4 bytes（攤銷後略多）。
//   再加上每個節點都是一次獨立的 heap 配置：配置器呼叫成本、
//   記憶體碎裂、節點散落各處造成的 cache miss，都是 list 的隱形成本。
//   → 小型元素（int/pointer）用 list 幾乎總是划不來。
//   ★ 24 bytes 為 libstdc++ 實測值（本機 g++ 15.2，64-bit），非標準規定。
//
// (B) 為什麼 list 不能用 std::sort
//   std::sort 要求 random access iterator（要能 it+n、it2-it1）。
//   list 只有 bidirectional iterator，所以編譯期就被擋下。
//   list 因此自備成員函式 lst.sort()：歸併排序，**只改指標不搬元素**，
//   O(n log n) 且不使任何 iterator 失效。
//
// (C) 哨兵節點（sentinel）與 end()
//   libstdc++ 的 list 內部有一個不存放元素的哨兵節點，串成環狀：
//   哨兵的 next 是第一個元素，prev 是最後一個元素。
//   end() 就是指向這個哨兵。好處是插入/刪除完全不需要處理
//   「空串列」「刪頭」「刪尾」等特例——所有情況都是一致的指標改接。
//   這也解釋了為什麼 sizeof(std::list<int>) 是 24 bytes（本機實測）：
//   哨兵的兩個指標 + size 計數欄位。
//
// (D) emplace vs push
//   push_back(x) 會把 x 複製/搬移進新節點；
//   emplace_back(args...) 直接在節點記憶體上用 args 呼叫建構子，省一次搬移。
//   對 std::string、大型 struct 差異明顯；對 int 則無實質差別。
//   注意 emplace 用的是**直接初始化**，會接受 explicit 建構子——
//   偶爾會意外通過你不想要的轉換。
//
// 【注意事項 Pay Attention】
//  1. 「insert/erase 是 O(1)」的前提是**已持有 iterator**；
//     用 advance 走過去找位置是 O(n)，別把總成本算成 O(1)。
//  2. 迴圈刪除請寫 `it = lst.erase(it)`，且 for 的第三段留空。
//  3. erase(it++) 對 list 合法、對 vector 是 UB；建議統一用 (2) 的寫法。
//  4. **沒有 operator[] / at()**；取第 k 個要用 std::advance / std::next（O(k)）。
//  5. **不能用 std::sort**（bidirectional iterator）；請用成員 lst.sort()。
//  6. 每節點 overhead 很高（本機 int 為 24 bytes/節點）且各自 heap 配置，
//     cache 不友善。小型元素、以遍歷為主的場景請用 vector。
//  7. size() 自 C++11 起保證 O(1)；判斷空請優先用 empty()。
//  8. 對空 list 呼叫 front()/back()/pop_front()/pop_back() 是 UB（不會丟例外）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list 的元素操作與迭代器失效
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list 的 insert/erase 是 O(1)，vector 是 O(n)，
//        所以「頻繁插刪就該用 list」——這個推論哪裡不完整?
//     答：少講了「前提是你已經持有那個位置的 iterator」。若只知道值或索引，
//         得先 advance 走過去，那是 O(n)，總成本並沒有比 vector 好。
//         而 vector 的 O(n) 搬移是連續 memmove，常數極小、cache 友善，
//         小型元素在實測上常常反而更快。
//     追問：那 list 什麼時候真的贏?→ iterator 早就在手上（如 LRU cache 用
//         hash map 存 iterator）、元素昂貴或不可搬移、以及需要
//         「插刪不使其他 iterator 失效」這個保證的時候。
//
// 🔥 Q2. 說明 vector / deque / list 三者的迭代器失效規則差異。
//     答：vector —— 擴容或中間插刪會使 iterator 與 reference 一起失效。
//         deque  —— 頭尾插入使**所有 iterator 失效，但 reference/pointer 仍有效**；
//                   中間插刪則全部失效。
//         list   —— 最寬鬆：insert **不使任何**東西失效；
//                   erase **只使被刪元素**的 iterator/reference 失效。
//     追問：list 為什麼能這麼寬鬆?→ 節點各自獨立配置，插刪只改相鄰節點的
//         prev/next 指標，既有節點位址從未改變。這正是 list 存在的主要理由。
//
// 🔥 Q3. 迴圈中一邊遍歷一邊刪除，正確寫法是什麼?
//     答：for (auto it = lst.begin(); it != lst.end(); ) {
//             if (cond) it = lst.erase(it);   // erase 回傳下一個，已幫你前進
//             else      ++it;
//         }
//         關鍵是 for 的第三段**留空**，否則刪除後會再多跳一格而漏掉元素。
//     追問：為什麼 erase 要回傳下一個 iterator?→ 因為被刪元素的 iterator
//         必然失效，若不回傳，呼叫端就沒有任何合法方式繼續往下走。
//
// ⚠️ 陷阱. lst.erase(it++) 這種寫法對 list 可行嗎?對 vector 呢?
//     答：對 **list 可行**：it++ 先讓 it 指向下一個節點，再把舊 iterator 傳給
//         erase；erase 只讓被刪那個失效，而 it 指的下一個節點毫髮無傷。
//         對 **vector 是 UB**：vector 的 erase 會讓刪除點**之後**的所有
//         iterator 失效，而前進後的 it 正好落在那個範圍內。
//     為什麼會錯：多數人只記「erase(it++) 是經典慣用法」或反過來記
//         「erase(it++) 一律錯」，兩者都是把不同容器的規則混為一談。
//         正解是**先問這是哪個容器的失效規則**。實務上統一寫
//         `it = lst.erase(it)`：語意清楚、對所有序列容器都對。
//
// ⚠️ 陷阱2. 「list 有 size()，所以在迴圈裡用 lst.size() 沒問題」
//     答：在 **C++11 以後**成立（標準要求 size() 為 O(1)，實作維護計數欄位）。
//         但 C++98/03 時代標準允許線性時間，libstdc++ 當年真的是走一遍串列，
//         在迴圈裡呼叫會讓 O(n) 默默變成 O(n²)。維護舊程式碼時要留意。
//     為什麼會錯：以為「有這個成員函式」就等於「它很便宜」。
//         容器介面的複雜度保證是隨標準版本演進的，
//         判斷空集合用 empty() 在任何年代都是 O(1)，是更穩妥的寫法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <memory>
#include <iterator>
#include <cstddef>

// -----------------------------------------------------------------------------
// 工具:印出 list 內容（前置空白，避免行尾多餘空白）
// -----------------------------------------------------------------------------
template <typename T>
void print(const std::string& label, const std::list<T>& lst) {
    std::cout << "  " << label << " [" << lst.size() << "]:";
    for (const auto& v : lst) std::cout << " " << v;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 計數配置器:實測「每個 list 節點真正吃掉多少 bytes」
//   list 會把配置器 rebind 成「節點型別」，所以這裡的 sizeof(T) 就是節點大小。
// -----------------------------------------------------------------------------
static std::size_t g_nodeBytes = 0;
static std::size_t g_nodeCount = 0;

template <class T>
struct CountingAlloc {
    using value_type = T;
    CountingAlloc() = default;
    template <class U> CountingAlloc(const CountingAlloc<U>&) {}
    T* allocate(std::size_t n) {
        g_nodeBytes += n * sizeof(T);
        ++g_nodeCount;
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
    template <class U> bool operator==(const CountingAlloc<U>&) const { return true; }
    template <class U> bool operator!=(const CountingAlloc<U>&) const { return false; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 146. LRU Cache
//   題目：設計 LRU 快取，get 與 put 都要 O(1)。
//   為什麼用到本主題：這題是 list 的**代表作**，用到本課三個性質：
//     (1) push_front / erase / pop_back 都是 O(1)；
//     (2) **iterator 不會因為別處的插刪而失效** —— 所以 hash map 可以
//         長期保存指向節點的 iterator；
//     (3) 因為 iterator 一直在手上，永遠不需要 O(n) 走訪去找位置。
//     少了 (2)，這個 O(1) 解法根本不成立（vector 會因擴容讓存下的位置全失效）。
//   註：把節點移到最前面，這裡用「erase + push_front」（本課教的操作）；
//       更省的做法是 splice(begin, lst, it)，它只改指標、連節點都不重新配置。
// -----------------------------------------------------------------------------
class LRUCache {
    using KV = std::pair<int, int>;                  // (key, value)
    std::size_t cap_;
    std::list<KV> order_;                            // 前=最近使用，後=最久未用
    std::unordered_map<int, std::list<KV>::iterator> idx_;   // key → 節點 iterator
public:
    explicit LRUCache(std::size_t capacity) : cap_(capacity) {}

    int get(int key) {
        auto f = idx_.find(key);
        if (f == idx_.end()) return -1;
        int value = f->second->second;
        order_.erase(f->second);                     // O(1)：iterator 在手上
        order_.push_front({key, value});             // O(1)
        f->second = order_.begin();                  // 更新為新節點的 iterator
        return value;
    }

    void put(int key, int value) {
        auto f = idx_.find(key);
        if (f != idx_.end()) {                       // 已存在 → 更新並移到最前
            order_.erase(f->second);
            order_.push_front({key, value});
            f->second = order_.begin();
            return;
        }
        if (order_.size() == cap_) {                 // 滿了 → 淘汰最久未用（尾端）
            idx_.erase(order_.back().first);
            order_.pop_back();                       // O(1)
        }
        order_.push_front({key, value});
        idx_[key] = order_.begin();
    }

    std::string snapshot() const {                   // 由新到舊列出目前內容
        std::string s;
        for (const auto& [k, v] : order_)
            s += " " + std::to_string(k) + "=" + std::to_string(v);
        return s.empty() ? " (空)" : s;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 203. Remove Linked List Elements
//   題目：刪除鏈結串列中所有等於 val 的節點，回傳新的 head。
//   為什麼用到本主題：這題的原生解法要處理「刪除頭節點」的特例（常用 dummy head）。
//     改用 std::list 時，`it = lst.erase(it)` 的慣用寫法**天然涵蓋頭節點**，
//     因為 erase 回傳下一個 iterator，不需要任何特例分支。
//     這裡刻意用 erase 迴圈而非 lst.remove(val)，以示範本課的核心慣用法。
// -----------------------------------------------------------------------------
std::list<int> removeElements(std::list<int> lst, int val) {
    for (auto it = lst.begin(); it != lst.end(); ) {
        if (*it == val) it = lst.erase(it);   // erase 已回傳下一個，不要再 ++it
        else            ++it;
    }
    return lst;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】資料庫連線池的「空閒連線」清單
//   情境：連線池維護一批連線，借出時從空閒清單移除、歸還時放回，
//         並定期把閒置過久的連線關閉回收。
//   為什麼用 list：
//     (1) 借出/歸還都在兩端，O(1)；
//     (2) 回收巡檢要「邊走邊刪」，`it = erase(it)` 一次遍歷解決；
//     (3) 連線物件昂貴且不希望被搬移，list 的節點位址永遠穩定。
// -----------------------------------------------------------------------------
struct Conn {
    int  id;
    long idleSince;      // 進入空閒清單的時間戳（秒）
};
std::ostream& operator<<(std::ostream& os, const Conn& c) {
    return os << "conn" << c.id;
}

class ConnPool {
    std::list<Conn> idle_;
public:
    void release(int id, long now) { idle_.push_back({id, now}); }   // 歸還
    int acquire() {                                                  // 借出
        if (idle_.empty()) return -1;
        int id = idle_.front().id;
        idle_.pop_front();                                           // O(1)
        return id;
    }
    // 關閉閒置超過 maxIdle 秒的連線，回傳被回收的數量
    int reap(long now, long maxIdle) {
        int n = 0;
        for (auto it = idle_.begin(); it != idle_.end(); ) {
            if (now - it->idleSince > maxIdle) { it = idle_.erase(it); ++n; }
            else                                 ++it;
        }
        return n;
    }
    const std::list<Conn>& idle() const { return idle_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】文字編輯器的行緩衝（在游標處插入/刪除整行）
//   情境：編輯器把每一行存成一個節點，書籤/游標就是一個 iterator。
//   為什麼用 list：使用者在中間插入或刪除一行時，**其他行的 iterator
//     完全不受影響**——書籤、多重游標、diff 標記等長期持有的位置都不會失效。
//     若用 vector<string>，插入一行就可能讓所有既存位置全部失效。
// -----------------------------------------------------------------------------
class LineBuffer {
    std::list<std::string> lines_;
public:
    LineBuffer(std::initializer_list<std::string> init) : lines_(init) {}
    std::list<std::string>::iterator at(std::size_t k) {   // O(k)：誠實地走過去
        auto it = lines_.begin();
        std::advance(it, k);
        return it;
    }
    void insertBefore(std::list<std::string>::iterator pos, const std::string& s) {
        lines_.insert(pos, s);          // O(1)，且不使任何 iterator 失效
    }
    void eraseAt(std::list<std::string>::iterator pos) { lines_.erase(pos); }
    const std::list<std::string>& lines() const { return lines_; }
};

int main() {
    // =========================================================================
    std::cout << "=== 1. 頭尾操作 push / pop / front / back ===\n";
    // =========================================================================
    std::list<int> lst;
    lst.push_back(10); lst.push_back(20); lst.push_back(30);
    print("push_back ×3 ", lst);
    lst.push_front(5); lst.push_front(1);
    print("push_front ×2", lst);
    lst.pop_front(); lst.pop_back();
    print("pop 首尾     ", lst);
    std::cout << "  front=" << lst.front() << " back=" << lst.back()
              << " size=" << lst.size() << "（C++11 起保證 O(1)）\n";

    // =========================================================================
    std::cout << "\n=== 2. emplace:原地建構，省一次搬移 ===\n";
    // =========================================================================
    std::list<std::pair<std::string, int>> scores;
    scores.emplace_back("Alice", 95);      // 直接在節點上建構 pair
    scores.emplace_front("Bob", 88);
    for (const auto& [n, s] : scores) std::cout << "  " << n << ":" << s << "\n";

    // =========================================================================
    std::cout << "\n=== 3. insert 的各種重載（回傳指向新元素的 iterator）===\n";
    // =========================================================================
    std::list<int> l2 = {10, 30, 50};
    print("初始         ", l2);
    auto pos = std::next(l2.begin());              // 指向 30
    auto ret = l2.insert(pos, 20);                 // 在 30 之前插入 20
    print("insert(30前,20)", l2);
    std::cout << "  回傳→" << *ret << "，原 iterator 仍指向 " << *pos
              << "（insert 不使任何 iterator 失效）\n";
    l2.insert(l2.end(), 3, 99);                    // 尾端插入 3 個 99
    print("insert 3×99  ", l2);
    l2.insert(l2.begin(), {-2, -1, 0});            // 頭端插入初始化列表
    print("insert{-2,-1,0}", l2);

    // =========================================================================
    std::cout << "\n=== 4. erase:單一元素與範圍 ===\n";
    // =========================================================================
    std::list<int> l3 = {10, 20, 30, 40, 50, 60, 70};
    print("初始         ", l3);
    auto eit = std::next(l3.begin(), 2);           // 指向 30
    auto nxt = l3.erase(eit);
    print("erase(30)    ", l3);
    std::cout << "  回傳→" << *nxt << "（被刪元素的下一個）\n";
    auto f = std::next(l3.begin(), 1);             // 指向 20
    auto l = std::next(l3.begin(), 3);             // 指向 60
    l3.erase(f, l);                                // 刪除 [20, 60) = 20,40,50
    print("erase[20,60) ", l3);

    // =========================================================================
    std::cout << "\n=== 5. 失效規則:list 最寬鬆 ===\n";
    // =========================================================================
    std::list<int> l4 = {100, 200, 300, 400, 500};
    auto it200 = std::next(l4.begin(), 1);
    auto it300 = std::next(l4.begin(), 2);
    auto it400 = std::next(l4.begin(), 3);
    l4.insert(it300, 250);                         // 在 300 之前插入
    std::cout << "  insert 後 *it200=" << *it200 << " *it300=" << *it300
              << " *it400=" << *it400 << " → 全部仍有效\n";
    l4.erase(it300);                               // 只刪 300
    std::cout << "  erase(300) 後 *it200=" << *it200 << " *it400=" << *it400
              << " → 其他 iterator 不受影響\n";
    print("目前內容     ", l4);
    std::cout << "  ★ insert 不使任何 iterator 失效；erase 只使被刪的那個失效\n";

    // =========================================================================
    std::cout << "\n=== 6. 迴圈刪除:兩種寫法都對（但建議第一種）===\n";
    // =========================================================================
    std::list<int> l5 = {1,2,3,4,5,6,7,8,9,10};
    print("刪前         ", l5);
    for (auto it = l5.begin(); it != l5.end(); ) {   // ← 第三段刻意留空
        if (*it % 2 == 0) it = l5.erase(it);
        else              ++it;
    }
    print("it=erase(it) ", l5);

    std::list<int> l5b = {1,2,3,4,5,6,7,8,9,10};
    for (auto it = l5b.begin(); it != l5b.end(); ) {
        if (*it % 2 == 0) l5b.erase(it++);           // 對 list 合法，對 vector 是 UB
        else              ++it;
    }
    print("erase(it++)  ", l5b);
    std::cout << "  ★ 兩者結果相同；erase(it++) 對 vector 會是 UB，故建議統一用前者\n";

    // =========================================================================
    std::cout << "\n=== 7. 每個節點的記憶體代價（實測）===\n";
    // =========================================================================
    {
        std::list<int, CountingAlloc<int>> counted;
        for (int i = 0; i < 10; ++i) counted.push_back(i);
        std::cout << "  存 10 個 int:配置 " << g_nodeCount << " 次，共 "
                  << g_nodeBytes << " bytes → 每節點 "
                  << g_nodeBytes / g_nodeCount << " bytes\n";
        std::cout << "  拆解:prev 8 + next 8 + int 4 + padding 4 = 24\n";
        std::cout << "  → 存 4 bytes 資料付 24 bytes，且每節點各一次 heap 配置\n";
        std::cout << "  ★ libstdc++ 64-bit 實測值，非標準規定\n";
    }

    // =========================================================================
    std::cout << "\n=== 8. LeetCode 146. LRU Cache（容量 2）===\n";
    // =========================================================================
    LRUCache cache(2);
    cache.put(1, 1);                 std::cout << "  put(1,1)  →" << cache.snapshot() << "\n";
    cache.put(2, 2);                 std::cout << "  put(2,2)  →" << cache.snapshot() << "\n";
    std::cout << "  get(1)    = " << cache.get(1) << " →" << cache.snapshot() << "\n";
    cache.put(3, 3);                 std::cout << "  put(3,3)  →" << cache.snapshot()
                                               << "（淘汰最久未用的 2）\n";
    std::cout << "  get(2)    = " << cache.get(2) << "（已被淘汰）\n";
    cache.put(4, 4);                 std::cout << "  put(4,4)  →" << cache.snapshot() << "\n";
    std::cout << "  get(1)    = " << cache.get(1) << "（已被淘汰）\n";
    std::cout << "  get(3)    = " << cache.get(3) << "\n";
    std::cout << "  get(4)    = " << cache.get(4) << "\n";

    // =========================================================================
    std::cout << "\n=== 9. LeetCode 203. Remove Linked List Elements ===\n";
    // =========================================================================
    print("原始 val=6   ", std::list<int>{1,2,6,3,4,5,6});
    print("移除後       ", removeElements({1,2,6,3,4,5,6}, 6));
    print("全部相同 val=7", removeElements({7,7,7,7}, 7));
    std::cout << "  ★ erase 迴圈天然處理「刪除頭節點」，不需要 dummy head 特例\n";

    // =========================================================================
    std::cout << "\n=== 10. 實務:連線池的空閒連線清單 ===\n";
    // =========================================================================
    ConnPool pool;
    pool.release(1, 1000);
    pool.release(2, 1050);
    pool.release(3, 1200);
    pool.release(4, 1290);
    print("空閒連線     ", pool.idle());
    std::cout << "  借出一條 → conn" << pool.acquire() << "\n";
    int reaped = pool.reap(1300, 120);          // 閒置超過 120 秒的回收掉
    std::cout << "  回收閒置>120秒:" << reaped << " 條\n";
    print("回收後       ", pool.idle());

    // =========================================================================
    std::cout << "\n=== 11. 實務:編輯器行緩衝（中間插刪不影響其他位置）===\n";
    // =========================================================================
    LineBuffer buf{"#include <iostream>", "int main() {", "    return 0;", "}"};
    auto bookmark = buf.at(3);                  // 書籤釘在 "}" 這一行
    std::cout << "  書籤原本指向: " << *bookmark << "\n";
    buf.insertBefore(buf.at(2), "    std::cout << \"hi\";");   // 在中間插入一行
    std::cout << "  插入一行後書籤仍指向: " << *bookmark << "（iterator 未失效）\n";
    buf.eraseAt(buf.at(0));                     // 刪掉第一行
    std::cout << "  刪除第一行後書籤仍指向: " << *bookmark << "\n";
    std::cout << "  目前內容:\n";
    for (const auto& s : buf.lines()) std::cout << "    | " << s << "\n";

    // =========================================================================
    std::cout << "\n=== 12. resize / clear ===\n";
    // =========================================================================
    std::list<int> l6 = {10,20,30,40,50};
    l6.resize(3);       print("resize(3)    ", l6);
    l6.resize(6, 99);   print("resize(6,99) ", l6);
    l6.clear();         print("clear        ", l6);
    std::cout << "  empty=" << (l6.empty() ? "是" : "否") << "\n";

    // =========================================================================
    std::cout << "\n=== 重點整理 ===\n";
    // =========================================================================
    std::cout << "  1. insert/erase O(1) 的前提是「已持有 iterator」，找位置仍 O(n)\n";
    std::cout << "  2. insert 不使任何 iterator 失效；erase 只使被刪的那個失效\n";
    std::cout << "  3. 迴圈刪除寫 it = lst.erase(it)，for 第三段留空\n";
    std::cout << "  4. erase(it++) 對 list 合法、對 vector 是 UB\n";
    std::cout << "  5. 沒有 operator[]；取第 k 個要 advance，成本 O(k) 且顯而易見\n";
    std::cout << "  6. 每節點 24 bytes（本機 int 實測）且各自 heap 配置，cache 不友善\n";
    std::cout << "  7. size() 自 C++11 起保證 O(1)；判斷空優先用 empty()\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

//   ★ libstdc++ 64-bit 實測值，非標準規定

// === 預期輸出 ===
// === 1. 頭尾操作 push / pop / front / back ===
//   push_back ×3  [3]: 10 20 30
//   push_front ×2 [5]: 1 5 10 20 30
//   pop 首尾      [3]: 5 10 20
//   front=5 back=20 size=3（C++11 起保證 O(1)）
//
// === 2. emplace:原地建構，省一次搬移 ===
//   Bob:88
//   Alice:95
//
// === 3. insert 的各種重載（回傳指向新元素的 iterator）===
//   初始          [3]: 10 30 50
//   insert(30前,20) [4]: 10 20 30 50
//   回傳→20，原 iterator 仍指向 30（insert 不使任何 iterator 失效）
//   insert 3×99   [7]: 10 20 30 50 99 99 99
//   insert{-2,-1,0} [10]: -2 -1 0 10 20 30 50 99 99 99
//
// === 4. erase:單一元素與範圍 ===
//   初始          [7]: 10 20 30 40 50 60 70
//   erase(30)     [6]: 10 20 40 50 60 70
//   回傳→40（被刪元素的下一個）
//   erase[20,60)  [4]: 10 50 60 70
//
// === 5. 失效規則:list 最寬鬆 ===
//   insert 後 *it200=200 *it300=300 *it400=400 → 全部仍有效
//   erase(300) 後 *it200=200 *it400=400 → 其他 iterator 不受影響
//   目前內容      [5]: 100 200 250 400 500
//   ★ insert 不使任何 iterator 失效；erase 只使被刪的那個失效
//
// === 6. 迴圈刪除:兩種寫法都對（但建議第一種）===
//   刪前          [10]: 1 2 3 4 5 6 7 8 9 10
//   it=erase(it)  [5]: 1 3 5 7 9
//   erase(it++)   [5]: 1 3 5 7 9
//   ★ 兩者結果相同；erase(it++) 對 vector 會是 UB，故建議統一用前者
//
// === 7. 每個節點的記憶體代價（實測）===
//   存 10 個 int:配置 10 次，共 240 bytes → 每節點 24 bytes
//   拆解:prev 8 + next 8 + int 4 + padding 4 = 24
//   → 存 4 bytes 資料付 24 bytes，且每節點各一次 heap 配置
//
// === 8. LeetCode 146. LRU Cache（容量 2）===
//   put(1,1)  → 1=1
//   put(2,2)  → 2=2 1=1
//   get(1)    = 1 → 1=1 2=2
//   put(3,3)  → 3=3 1=1（淘汰最久未用的 2）
//   get(2)    = -1（已被淘汰）
//   put(4,4)  → 4=4 3=3
//   get(1)    = -1（已被淘汰）
//   get(3)    = 3
//   get(4)    = 4
//
// === 9. LeetCode 203. Remove Linked List Elements ===
//   原始 val=6    [7]: 1 2 6 3 4 5 6
//   移除後        [5]: 1 2 3 4 5
//   全部相同 val=7 [0]:
//   ★ erase 迴圈天然處理「刪除頭節點」，不需要 dummy head 特例
//
// === 10. 實務:連線池的空閒連線清單 ===
//   空閒連線      [4]: conn1 conn2 conn3 conn4
//   借出一條 → conn1
//   回收閒置>120秒:1 條
//   回收後        [2]: conn3 conn4
//
// === 11. 實務:編輯器行緩衝（中間插刪不影響其他位置）===
//   書籤原本指向: }
//   插入一行後書籤仍指向: }（iterator 未失效）
//   刪除第一行後書籤仍指向: }
//   目前內容:
//     | int main() {
//     |     std::cout << "hi";
//     |     return 0;
//     | }
//
// === 12. resize / clear ===
//   resize(3)     [3]: 10 20 30
//   resize(6,99)  [6]: 10 20 30 99 99 99
//   clear         [0]:
//   empty=是
//
// === 重點整理 ===
//   1. insert/erase O(1) 的前提是「已持有 iterator」，找位置仍 O(n)
//   2. insert 不使任何 iterator 失效；erase 只使被刪的那個失效
//   3. 迴圈刪除寫 it = lst.erase(it)，for 第三段留空
//   4. erase(it++) 對 list 合法、對 vector 是 UB
//   5. 沒有 operator[]；取第 k 個要 advance，成本 O(k) 且顯而易見
//   6. 每節點 24 bytes（本機 int 實測）且各自 heap 配置，cache 不友善
//   7. size() 自 C++11 起保證 O(1)；判斷空優先用 empty()
