// =============================================================================
//  summary.cpp  —  forward_list 的 _after 系列：單向鏈結如何決定整組 API
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <forward_list>          （C++11 新增的容器）
//   結構:   單向鏈結串列，每個節點只有 next 指標 + value
//
//   插入（全部都是「在 pos 之後」）:
//     insert_after(pos, v)                回傳指向**新元素**的 iterator
//     insert_after(pos, n, v)             插入 n 個；回傳最後一個新元素
//     insert_after(pos, {init})           初始化列表
//     insert_after(pos, first, last)      迭代器範圍
//     emplace_after(pos, args...)         原地建構
//   刪除:
//     erase_after(pos)                    刪除 pos 的**下一個**；回傳再下一個
//     erase_after(first, last)            刪除 **(first, last) 開區間**
//   其他:
//     before_begin() / cbefore_begin()    「第一個元素之前」的位置
//     push_front / pop_front / front      O(1)
//     remove / remove_if / unique / sort / merge / reverse / splice_after
//
//   複雜度: 上述單一元素操作皆 O(1)（已持有 pos 時）
//   ★ **沒有 size()**（刻意設計，見下）；要算長度用 std::distance，O(n)
//   ★ **沒有 push_back / back()**：沒有尾端指標，找尾端是 O(n)
//   ★ 回傳型別:remove/remove_if/unique 在 C++17 以前是 void，
//     C++20 起回傳移除個數（本檔不取回傳值，故以 -std=c++17 編譯）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼全部都是 _after?——這是本課唯一需要真正理解的事】
// 單向鏈結的節點只有 next，**沒有 prev**。要在節點 X「之前」插入新節點 N，
// 必須把「X 的前一個節點」的 next 改成指向 N：
//
//     [prev] ──next──> [X]          想變成    [prev] ──> [N] ──> [X]
//        ↑
//     但從指向 X 的 iterator，**永遠走不回 prev**
//
// 你手上只有指向 X 的 iterator，而 X 不知道誰指向自己。唯一的辦法是
// 從頭再走一遍去找 prev —— 那就是 O(n)，違背了鏈結串列 O(1) 插入的初衷。
//
// 所以標準做了一個誠實的設計決定：**不提供做不到 O(1) 的介面**。
// 既然只能改「自己的 next」，那就把 API 定義成「在 pos 之後操作」：
//     insert_after(pos, v)   →  只需改 pos->next，O(1)
//     erase_after(pos)       →  只需把 pos->next 改成 pos->next->next，O(1)
// 這與 list 提供 insert(pos, v)（在 pos 之前）形成對照——
// list 有 prev 指標，所以「在之前插入」對它才是 O(1)。
//
// 【2. before_begin() 的存在理由】
// 如果所有操作都是「在 pos 之後」，那要在**第一個元素之前**插入怎麼辦？
// 沒有任何一個 pos 的「之後」是第一個位置。
// 解法是提供一個虛擬位置 before_begin()，它代表「第一個元素之前」：
//     fl.insert_after(fl.before_begin(), v);   // 等同 push_front
//     fl.erase_after(fl.before_begin());       // 刪掉第一個元素
//
// 實作上 before_begin() 通常指向容器內嵌的「頭節點」（head，只有 next、
// 不存放元素）。這也解釋了 sizeof(std::forward_list<int>) == 8（本機實測）：
// 整個容器物件就只是那一根 head 指標，**連 size 計數都沒有**。
//
// ★ **before_begin() 不可解參考**：它不指向任何元素，*fl.before_begin()
//   是 UB。它只能當作 insert_after / erase_after / splice_after 的引數。
//   同理，你也不能用它跟 end() 比較來判斷容器是否為空（請用 empty()）。
//
// 【3. erase_after 的區間是「開區間」——最容易寫錯的地方】
// STL 幾乎所有區間都是**半開區間 [first, last)**，唯獨 forward_list 的
// erase_after / splice_after 是 **(first, last) 開區間**：
//     erase_after(first, last)  刪除的是 first 與 last **之間**的元素，
//                               **first 和 last 本身都不會被刪**。
//
//   {10,20,30,40,50,60,70}
//     first → 10
//     last  → 40
//     erase_after(first, last) 刪掉 20 與 30，結果 {10,40,50,60,70}
//   （本檔範例 3 有實際執行驗證。）
//
// 為什麼是開區間？因為 first 必須是「要刪的第一個元素的**前一個**」——
// 這是單向鏈結唯一能改接指標的位置；而 last 是「停下來的地方」，
// 它本身要被接到 first 後面，當然不能刪。
// 記法：**兩個引數都是「邊界樁」，被拔掉的是樁與樁之間的東西。**
//
// 【4. 為什麼沒有 size()——這是特意的設計】
// 維護 size 需要一個計數欄位，且每次 insert/erase/splice 都要更新它。
// forward_list 的設計目標是「**與手寫單向鏈結串列有相同的空間與時間開銷**」
// （C++ 標準的設計理由即是如此）。多一個 size_t 欄位就會讓容器物件變大，
// 也讓 splice_after 這種「只改指標」的 O(1) 操作被迫變成 O(n)（要數搬了幾個）。
// 這正是 list::splice 的實際困擾：list 為了維護 O(1) 的 size()，
// 它的「部分區間 splice」必須逐一數過。
// 所以要知道長度請自己算，而且成本是明擺著的：
//     auto n = std::distance(fl.begin(), fl.end());   // O(n)
//
// 【5. forward_list vs list:省了什麼、付出什麼】
//   省下：每個節點少一個 prev 指標。本機實測
//         forward_list<int> 每節點 16 bytes、list<int> 每節點 24 bytes
//         —— 節點數量龐大時省下 1/3 記憶體（此為 libstdc++ 64-bit 實測值）。
//         容器物件本身也從 24 bytes 降到 8 bytes。
//   付出：只能單向走訪（沒有 --it、沒有 rbegin()）；
//         沒有 size()、沒有 push_back / back()；
//         所有插刪都要「前一個位置」的 iterator，寫起來比 list 麻煩。
//   結論：只有在「節點數極多、記憶體吃緊、且只需單向走訪」時才選它。
//         絕大多數情況 list 或 vector 更合適。
//
// 【6. 迴圈中安全刪除:必須自己追蹤 prev】
// 因為 erase_after 要的是「前一個位置」，遍歷刪除的寫法與 list 不同：
//     auto prev = fl.before_begin();
//     auto curr = fl.begin();
//     while (curr != fl.end()) {
//         if (要刪) curr = fl.erase_after(prev);   // prev 不動，curr 前進
//         else      { prev = curr; ++curr; }
//     }
// 注意刪除時 **prev 保持不動**（它的下一個換成了新的元素）。
// 這段程式碼容易寫錯，所以能用 fl.remove_if(pred) 就用它。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 迭代器類別:forward iterator
//   forward_list 的迭代器只滿足 **LegacyForwardIterator**：
//   只有 ++，沒有 --，也沒有 +n / it2-it1。
//   因此 std::sort（需 random access）與 std::reverse（需 bidirectional）
//   對它都**無法編譯**；forward_list 自備成員 sort() 與 reverse()。
//   std::distance 對它會退化成逐步走訪（O(n)），這是刻意可見的成本。
//
// (B) 為什麼沒有 push_back
//   沒有尾端指標，push_back 得先 O(n) 走到尾端。標準同樣選擇「不提供
//   做不到 O(1) 的介面」。若真的需要尾端追加，請自己維護一個 tail iterator：
//       auto tail = fl.before_begin();
//       tail = fl.insert_after(tail, v);   // 每次 O(1)
//   本檔範例 6 示範這個技巧。
//
// (C) splice_after 為什麼是 forward_list 的殺手鐧
//   把節點從一個串列搬到另一個，只需改幾個 next 指標，**不搬移任何元素、
//   不配置也不釋放記憶體**，而且被搬動元素的 iterator 依然有效
//   （只是歸屬的容器變了）。整串 splice_after 是 O(1)。
//   注意區間版同樣是 **(first, last) 開區間**。
//
// (D) 失效規則
//   與 list 一樣寬鬆：insert_after 不使任何 iterator 失效；
//   erase_after 只使**被刪除元素**的 iterator/reference 失效。
//   原因相同：節點各自配置，插刪只改 next 指標，既有節點位址不變。
//
// 【注意事項 Pay Attention】
//  1. **before_begin() 不可解參考**（*fl.before_begin() 是 UB），
//     它只能當 _after 系列函式的引數。
//  2. **erase_after(first, last) 是 (first, last) 開區間**，
//     first 與 last 本身都不會被刪 —— 與 STL 其他 [first, last) 相反。
//  3. **沒有 size()**：要長度用 std::distance(begin(), end())，O(n)。
//  4. **沒有 push_back / back() / rbegin()**；要尾端追加請自行維護 tail。
//  5. 不能用 std::sort / std::reverse（forward iterator）；
//     請用成員 fl.sort() / fl.reverse()。
//  6. 迴圈刪除必須自己追蹤 prev，且刪除時 prev **不前進**；
//     能用 remove_if 就用 remove_if。
//  7. 每節點省下的一個指標（本機 16 vs 24 bytes）是它唯一的優勢；
//     若非節點數極多且記憶體吃緊，選 list 或 vector 通常更好。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】forward_list 的 _after 系列
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 forward_list 的插入刪除都叫 insert_after / erase_after，
//        而不是像 list 那樣的 insert / erase?
//     答：單向鏈結的節點只有 next、沒有 prev。要在某節點「之前」插入，
//         必須改「前一個節點的 next」，但從指向該節點的 iterator
//         **走不回前一個**，只能從頭再找一遍 → O(n)。
//         標準因此選擇不提供做不到 O(1) 的介面，把 API 定義成
//         「在 pos 之後」——只改 pos->next，保證 O(1)。
//     追問：那要在第一個元素之前插入怎麼辦?→ 用 before_begin()，
//         它是專為此存在的虛擬位置（實作上指向不存元素的 head 節點）。
//
// 🔥 Q2. before_begin() 可以解參考嗎?可以拿它跟 begin() 比較嗎?
//     答：**不可以解參考**——它不指向任何元素，*fl.before_begin() 是 UB。
//         它只能當作 insert_after / erase_after / splice_after 的引數。
//         比較上，before_begin() != begin() 恆成立（即使容器是空的），
//         所以不能用它判斷容器是否為空，請用 empty()。
//     追問：那 std::next(fl.before_begin()) 是什麼?→ 就是 begin()。
//         這也是為什麼 erase_after(before_begin()) 等同於「刪除第一個元素」。
//
// 🔥 Q3. forward_list 為什麼沒有 size()?
//     答：它的設計目標是「與手寫單向鏈結串列有相同的空間與時間開銷」。
//         維護 size 需要一個計數欄位（容器變大），而且會讓 splice_after
//         這類只改指標的 O(1) 操作被迫變成 O(n)（得數搬移了幾個節點）。
//         標準寧可不提供，讓你用 std::distance 明確付出 O(n)。
//     追問：sizeof(forward_list<int>) 是多少?→ 本機實測 **8 bytes**，
//         就只有一根 head 指標；對照 sizeof(list<int>) 是 24 bytes
//         （兩個哨兵指標 + size 計數）。此為 libstdc++ 實測值，非標準規定。
//
// ⚠️ 陷阱. fl.erase_after(first, last) 到底刪掉了哪些元素?
//        forward_list<int> fl = {10,20,30,40,50,60,70};
//        auto first = fl.begin();               // → 10
//        auto last  = std::next(first, 3);      // → 40
//        fl.erase_after(first, last);
//     答：刪掉的是 **20 和 30**，結果是 {10,40,50,60,70}。
//         這是 **(first, last) 開區間**——first 與 last 本身都不刪。
//     為什麼會錯：STL 幾乎所有區間都是半開的 [first, last)，
//         多數人直覺套用成「刪 10,20,30」。但單向鏈結只能從「前一個」
//         下手：first 必須是要刪的第一個元素的**前驅**，
//         而 last 要被接回 first 後面，當然不能刪。
//         記法：**兩個引數都是邊界樁，拔掉的是樁與樁之間的東西。**
//
// ⚠️ 陷阱2. 這段「邊走邊刪」的程式碼錯在哪?
//        auto prev = fl.before_begin(), curr = fl.begin();
//        while (curr != fl.end()) {
//            if (*curr % 2 == 0) { curr = fl.erase_after(prev); prev = curr; }
//            else                { prev = curr; ++curr; }
//        }
//     答：錯在刪除後又把 prev 設成 curr。刪除時 prev 的下一個已經換成了
//         新的元素，**prev 必須保持不動**；把 prev 前移會讓下一輪的
//         erase_after(prev) 刪錯位置（等於跳過一個元素）。
//         正確寫法是刪除分支只做 `curr = fl.erase_after(prev);`。
//     為什麼會錯：把「兩個游標要一起前進」當成通則。
//         實際上 prev 的語意是「最後一個**確定保留**的節點」，
//         只有在不刪的時候它才推進。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <forward_list>
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <cstddef>

// -----------------------------------------------------------------------------
// 工具:印出 forward_list（長度要自己算 —— 沒有 size()）
// -----------------------------------------------------------------------------
template <typename T>
void print(const std::string& label, const std::forward_list<T>& fl) {
    std::cout << "  " << label << " [" << std::distance(fl.begin(), fl.end()) << "]:";
    for (const auto& v : fl) std::cout << " " << v;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 計數配置器:實測 forward_list 每個節點的真實大小
// -----------------------------------------------------------------------------
static std::size_t g_bytes = 0;
static std::size_t g_count = 0;

template <class T>
struct CountingAlloc {
    using value_type = T;
    CountingAlloc() = default;
    template <class U> CountingAlloc(const CountingAlloc<U>&) {}
    T* allocate(std::size_t n) {
        g_bytes += n * sizeof(T); ++g_count;
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
    template <class U> bool operator==(const CountingAlloc<U>&) const { return true; }
    template <class U> bool operator!=(const CountingAlloc<U>&) const { return false; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 203. Remove Linked List Elements
//   題目：刪除鏈結串列中所有等於 val 的節點，回傳新的 head。
//   為什麼用到本主題：這題最經典的解法就是**加一個 dummy head**，
//     好讓「刪除第一個節點」不必寫特例。而 forward_list 的
//     **before_begin() 正是標準內建的 dummy head** ——
//     這題與本課的設計動機是同一件事的一體兩面。
//     手動追蹤 prev 的迴圈也正是原題指標操作的直譯。
// -----------------------------------------------------------------------------
std::forward_list<int> removeElements(std::forward_list<int> head, int val) {
    auto prev = head.before_begin();     // ← 標準版的 dummy head
    auto curr = head.begin();
    while (curr != head.end()) {
        if (*curr == val) curr = head.erase_after(prev);   // prev 不動！
        else            { prev = curr; ++curr; }
    }
    return head;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 206. Reverse Linked List
//   題目：反轉單向鏈結串列。
//   為什麼用到本主題：這題是單向鏈結的招牌題。這裡示範兩種做法：
//     (a) 成員函式 fl.reverse()：O(n) 只改 next 指標，不搬元素；
//         注意 std::reverse 演算法**無法編譯**（它要 bidirectional iterator，
//         而 forward_list 只有 forward iterator）——這正是
//         「迭代器類別決定可用演算法」的具體後果。
//     (b) 手動版：用 push_front 逐個搬到新串列前端，等價於原題的
//         三指標 prev/curr/next 迴圈。
// -----------------------------------------------------------------------------
std::forward_list<int> reverseByMember(std::forward_list<int> head) {
    head.reverse();                      // 成員版；std::reverse 不能用
    return head;
}

std::forward_list<int> reverseByPushFront(const std::forward_list<int>& head) {
    std::forward_list<int> out;
    for (int v : head) out.push_front(v);   // 每次 O(1)，天然反轉
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】記憶體池的 free list（空閒區塊清單）
//   情境：自訂配置器把固定大小的區塊串成「空閒清單」，
//         配置時從頭取一塊、釋放時再掛回頭部。
//   為什麼用 forward_list：這是 forward_list 最貼切的用途——
//     (1) 只需要「從頭拿、往頭放」，永遠不需要反向走訪；
//     (2) 節點數量可能非常多，每個節點省下的一個 prev 指標
//         （本機 16 vs 24 bytes）直接換算成可觀的記憶體節省；
//     (3) 不需要 size()（要統計時再 O(n) 算即可）。
// -----------------------------------------------------------------------------
class BlockPool {
    std::forward_list<int> free_;        // 存區塊編號
    int nextId_ = 0;
public:
    explicit BlockPool(int initial) {
        // 反向建立，讓編號由小到大排列
        for (int i = initial - 1; i >= 0; --i) free_.push_front(i);
        nextId_ = initial;
    }
    int acquire() {                      // 取一塊
        if (free_.empty()) return nextId_++;      // 空了就擴充
        int id = free_.front();
        free_.pop_front();               // O(1)
        return id;
    }
    void release(int id) { free_.push_front(id); }   // 歸還，O(1)
    std::size_t freeCount() const {                  // 沒有 size()，得自己算
        return static_cast<std::size_t>(std::distance(free_.begin(), free_.end()));
    }
    const std::forward_list<int>& freeList() const { return free_; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】依優先度插入的工作佇列（保持排序的插入）
//   情境：排程器維護一串工作，新工作要依 priority 插到正確位置。
//   為什麼用 forward_list：「有序插入」天生就需要「前一個位置」——
//     我們一路往前走，記住 prev，找到第一個 priority 比新工作低的位置，
//     然後 insert_after(prev, job)。這個模式與 _after API 完全契合，
//     反而比 list 的 insert(pos) 更自然。
// -----------------------------------------------------------------------------
struct Job {
    std::string name;
    int         priority;      // 數字越大越優先
};
std::ostream& operator<<(std::ostream& os, const Job& j) {
    return os << j.name << "(P" << j.priority << ")";
}

void insertByPriority(std::forward_list<Job>& q, const Job& job) {
    auto prev = q.before_begin();        // 從虛擬頭開始
    auto curr = q.begin();
    while (curr != q.end() && curr->priority >= job.priority) {
        prev = curr;
        ++curr;
    }
    q.insert_after(prev, job);           // 插在最後一個「優先度不低於它」的後面
}

int main() {
    // =========================================================================
    std::cout << "=== 1. insert_after 的各種重載 ===\n";
    // =========================================================================
    {
        std::forward_list<int> fl = {10, 40, 70};
        print("初始              ", fl);
        auto it  = fl.begin();                       // → 10
        auto ret = fl.insert_after(it, 20);          // 在 10 之後插入
        print("insert_after(10,20)", fl);
        std::cout << "  回傳→" << *ret << "（指向新元素）\n";
        ret = fl.insert_after(ret, 2, 30);           // 插入 2 個 30
        print("insert 2×30       ", fl);
        std::cout << "  回傳→" << *ret << "（最後一個新元素）\n";
        fl.insert_after(fl.before_begin(), {1, 2, 3});   // 插到最前面
        print("頭端插入{1,2,3}   ", fl);
    }

    // =========================================================================
    std::cout << "\n=== 2. before_begin():在第一個元素之前操作的唯一辦法 ===\n";
    // =========================================================================
    {
        std::forward_list<std::string> fl = {"banana", "cherry"};
        print("初始              ", fl);
        fl.insert_after(fl.before_begin(), "apple");   // 等同 push_front
        print("插到最前面        ", fl);
        fl.erase_after(fl.before_begin());             // 刪掉第一個
        print("刪掉第一個        ", fl);
        std::cout << "  ★ before_begin() **不可解參考**（*it 是 UB），\n";
        std::cout << "    它只能當 insert_after/erase_after/splice_after 的引數\n";
        std::cout << "  ★ std::next(before_begin()) 就是 begin()\n";
        std::cout << "    驗證:*std::next(fl.before_begin()) = "
                  << *std::next(fl.before_begin())
                  << "，*fl.begin() = " << *fl.begin() << "\n";
    }

    // =========================================================================
    std::cout << "\n=== 3. erase_after:單一與「開區間」範圍（最易錯）===\n";
    // =========================================================================
    {
        std::forward_list<int> fl = {10,20,30,40,50,60,70};
        print("初始              ", fl);
        auto ret = fl.erase_after(fl.begin());       // 刪 begin() 的下一個 = 20
        print("erase_after(begin)", fl);
        std::cout << "  回傳→" << *ret << "（被刪元素的下一個）\n";
        fl.erase_after(fl.before_begin());           // 刪第一個 = 10
        print("刪第一個          ", fl);
    }
    {
        std::forward_list<int> fl = {10,20,30,40,50,60,70};
        print("重新初始          ", fl);
        auto first = fl.begin();                     // → 10
        auto last  = std::next(first, 3);            // → 40
        std::cout << "  first→" << *first << "  last→" << *last << "\n";
        fl.erase_after(first, last);                 // 刪 (10,40) = 20,30
        print("erase_after(f,l)  ", fl);
        std::cout << "  ★ 刪掉的是 20 與 30 —— **(first,last) 開區間**，\n";
        std::cout << "    first(10) 與 last(40) 本身都保留下來了！\n";
        std::cout << "  ★ 這與 STL 其他 [first,last) 半開區間相反，務必記牢\n";
    }

    // =========================================================================
    std::cout << "\n=== 4. emplace_after 鏈式追加（模擬 push_back）===\n";
    // =========================================================================
    {
        std::forward_list<std::pair<std::string,int>> scores;
        auto pos = scores.before_begin();
        pos = scores.emplace_after(pos, "Alice", 95);   // 用回傳值當下一次的 pos
        pos = scores.emplace_after(pos, "Bob", 88);
        pos = scores.emplace_after(pos, "Charlie", 92);
        for (const auto& [n, s] : scores) std::cout << "  " << n << ":" << s << "\n";
        std::cout << "  ★ forward_list 沒有 push_back，但維護一個 tail iterator\n";
        std::cout << "    就能每次 O(1) 追加到尾端\n";
    }

    // =========================================================================
    std::cout << "\n=== 5. 迴圈中安全刪除:必須自己追蹤 prev ===\n";
    // =========================================================================
    {
        std::forward_list<int> fl = {1,2,3,4,5,6,7,8,9,10};
        print("原始              ", fl);
        auto prev = fl.before_begin();
        auto curr = fl.begin();
        while (curr != fl.end()) {
            if (*curr % 3 == 0) curr = fl.erase_after(prev);   // ★ prev 不前進
            else              { prev = curr; ++curr; }
        }
        print("刪 3 的倍數(手動) ", fl);
    }
    {
        std::forward_list<int> fl = {1,2,3,4,5,6,7,8,9,10};
        fl.remove_if([](int x) { return x % 3 == 0; });        // 推薦寫法
        print("刪 3 的倍數(rm_if)", fl);
        std::cout << "  ★ 結果相同；remove_if 不必手動管理 prev，不易寫錯\n";
    }

    // =========================================================================
    std::cout << "\n=== 6. 沒有 size():長度要自己算（O(n)）===\n";
    // =========================================================================
    {
        std::forward_list<int> fl = {5,10,15,20,25};
        std::cout << "  std::distance(begin,end) = "
                  << std::distance(fl.begin(), fl.end()) << "（O(n)）\n";
        std::cout << "  empty() = " << (fl.empty() ? "true" : "false")
                  << "（這個是 O(1)，判斷空請用它）\n";
        std::cout << "  ★ 標準刻意不提供 size():維護計數會讓容器變大，\n";
        std::cout << "    並使 splice_after 這類 O(1) 操作被迫變成 O(n)\n";
    }

    // =========================================================================
    std::cout << "\n=== 7. 記憶體比較:forward_list vs list（實測）===\n";
    // =========================================================================
    {
        std::forward_list<int, CountingAlloc<int>> fl;
        for (int i = 0; i < 10; ++i) fl.push_front(i);
        std::cout << "  forward_list<int> 每節點 = " << g_bytes / g_count
                  << " bytes（next 8 + int 4 + padding 4）\n";
        std::cout << "  list<int>         每節點 = 24 bytes"
                  << "（prev 8 + next 8 + int 4 + padding 4）\n";
        std::cout << "  容器物件本身:sizeof(forward_list<int>)="
                  << sizeof(std::forward_list<int>)
                  << "（只有一根 head 指標，連 size 都沒有）\n";
        std::cout << "  ★ 省下 1/3 的節點記憶體，就是 forward_list 唯一的優勢\n";
        std::cout << "  ★ libstdc++ 64-bit 實測值，非標準規定\n";
    }

    // =========================================================================
    std::cout << "\n=== 8. splice_after:只改指標的搬移（同樣是開區間）===\n";
    // =========================================================================
    {
        std::forward_list<int> A = {1,2,3}, B = {10,20,30};
        A.splice_after(A.before_begin(), B);          // 整個 B 移到 A 最前面
        print("整串移植後 A      ", A);
        print("整串移植後 B      ", B);
        std::cout << "  ★ O(1)，不搬移任何元素、不配置也不釋放記憶體\n";
    }
    {
        std::forward_list<int> A = {1,2,3}, B = {10,20,30};
        A.splice_after(A.begin(), B, B.begin());      // 搬 B.begin() 的下一個(20)
        print("單一移植後 A      ", A);
        print("單一移植後 B      ", B);
        std::cout << "  ★ 搬的是 B.begin()(=10) 的**下一個**元素 20\n";
    }
    {
        std::forward_list<int> fl = {1,2,3,4,5};
        auto before4 = std::next(fl.begin(), 2);      // → 3（4 的前一個）
        fl.splice_after(fl.before_begin(), fl, before4);   // 把 4 移到最前
        print("把 4 移到最前面   ", fl);
    }

    // =========================================================================
    std::cout << "\n=== 9. LeetCode 203. Remove Linked List Elements ===\n";
    // =========================================================================
    print("原始 val=6        ", std::forward_list<int>{1,2,6,3,4,5,6});
    print("結果              ", removeElements({1,2,6,3,4,5,6}, 6));
    print("全等 val=7        ", removeElements({7,7,7,7}, 7));
    std::cout << "  ★ before_begin() 就是標準內建的 dummy head，\n";
    std::cout << "    所以「刪除頭節點」完全不需要特例分支\n";

    // =========================================================================
    std::cout << "\n=== 10. LeetCode 206. Reverse Linked List ===\n";
    // =========================================================================
    print("原始              ", std::forward_list<int>{1,2,3,4,5});
    print("成員 reverse()    ", reverseByMember({1,2,3,4,5}));
    print("手動 push_front   ", reverseByPushFront({1,2,3,4,5}));
    print("單一元素          ", reverseByMember({1}));
    std::cout << "  ★ std::reverse 演算法對 forward_list **無法編譯**\n";
    std::cout << "    （它需要 bidirectional iterator）→ 必須用成員版\n";

    // =========================================================================
    std::cout << "\n=== 11. 實務:記憶體池的 free list ===\n";
    // =========================================================================
    {
        BlockPool pool(5);
        print("初始空閒區塊      ", pool.freeList());
        int a = pool.acquire();
        int b = pool.acquire();
        std::cout << "  配置兩塊 → #" << a << " #" << b << "\n";
        print("剩餘空閒          ", pool.freeList());
        pool.release(a);
        std::cout << "  歸還 #" << a << "\n";
        print("歸還後            ", pool.freeList());
        std::cout << "  空閒數量 = " << pool.freeCount() << "（沒有 size()，自己算）\n";
    }

    // =========================================================================
    std::cout << "\n=== 12. 實務:依優先度插入的工作佇列 ===\n";
    // =========================================================================
    {
        std::forward_list<Job> queue;
        insertByPriority(queue, {"備份資料庫", 3});
        insertByPriority(queue, {"寄送通知",   1});
        insertByPriority(queue, {"處理付款",   9});
        insertByPriority(queue, {"產生報表",   5});
        insertByPriority(queue, {"清理暫存",   1});
        print("依優先度排列      ", queue);
        std::cout << "  ★ 有序插入天生需要「前一個位置」，\n";
        std::cout << "    與 insert_after 的 API 形狀完全契合\n";
    }

    // =========================================================================
    std::cout << "\n=== 重點整理 ===\n";
    // =========================================================================
    std::cout << "  1. 只有 next 指標 → 走不回前一個 → 所有 API 都是 _after\n";
    std::cout << "  2. before_begin() 是「第一個之前」的虛擬位置，**不可解參考**\n";
    std::cout << "  3. erase_after(pos) 刪的是 pos 的**下一個**\n";
    std::cout << "  4. erase_after(first,last) 是 **(first,last) 開區間**，兩端都不刪\n";
    std::cout << "  5. 沒有 size()（刻意設計）；用 std::distance，O(n)\n";
    std::cout << "  6. 沒有 push_back/back()/rbegin()；不能用 std::sort/std::reverse\n";
    std::cout << "  7. 每節點省一個指標（16 vs 24 bytes），代價是只能單向走訪\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

//   ★ libstdc++ 64-bit 實測值，非標準規定

// === 預期輸出 ===
// === 1. insert_after 的各種重載 ===
//   初始               [3]: 10 40 70
//   insert_after(10,20) [4]: 10 20 40 70
//   回傳→20（指向新元素）
//   insert 2×30        [6]: 10 20 30 30 40 70
//   回傳→30（最後一個新元素）
//   頭端插入{1,2,3}    [9]: 1 2 3 10 20 30 30 40 70
//
// === 2. before_begin():在第一個元素之前操作的唯一辦法 ===
//   初始               [2]: banana cherry
//   插到最前面         [3]: apple banana cherry
//   刪掉第一個         [2]: banana cherry
//   ★ before_begin() **不可解參考**（*it 是 UB），
//     它只能當 insert_after/erase_after/splice_after 的引數
//   ★ std::next(before_begin()) 就是 begin()
//     驗證:*std::next(fl.before_begin()) = banana，*fl.begin() = banana
//
// === 3. erase_after:單一與「開區間」範圍（最易錯）===
//   初始               [7]: 10 20 30 40 50 60 70
//   erase_after(begin) [6]: 10 30 40 50 60 70
//   回傳→30（被刪元素的下一個）
//   刪第一個           [5]: 30 40 50 60 70
//   重新初始           [7]: 10 20 30 40 50 60 70
//   first→10  last→40
//   erase_after(f,l)   [5]: 10 40 50 60 70
//   ★ 刪掉的是 20 與 30 —— **(first,last) 開區間**，
//     first(10) 與 last(40) 本身都保留下來了！
//   ★ 這與 STL 其他 [first,last) 半開區間相反，務必記牢
//
// === 4. emplace_after 鏈式追加（模擬 push_back）===
//   Alice:95
//   Bob:88
//   Charlie:92
//   ★ forward_list 沒有 push_back，但維護一個 tail iterator
//     就能每次 O(1) 追加到尾端
//
// === 5. 迴圈中安全刪除:必須自己追蹤 prev ===
//   原始               [10]: 1 2 3 4 5 6 7 8 9 10
//   刪 3 的倍數(手動)  [7]: 1 2 4 5 7 8 10
//   刪 3 的倍數(rm_if) [7]: 1 2 4 5 7 8 10
//   ★ 結果相同；remove_if 不必手動管理 prev，不易寫錯
//
// === 6. 沒有 size():長度要自己算（O(n)）===
//   std::distance(begin,end) = 5（O(n)）
//   empty() = false（這個是 O(1)，判斷空請用它）
//   ★ 標準刻意不提供 size():維護計數會讓容器變大，
//     並使 splice_after 這類 O(1) 操作被迫變成 O(n)
//
// === 7. 記憶體比較:forward_list vs list（實測）===
//   forward_list<int> 每節點 = 16 bytes（next 8 + int 4 + padding 4）
//   list<int>         每節點 = 24 bytes（prev 8 + next 8 + int 4 + padding 4）
//   容器物件本身:sizeof(forward_list<int>)=8（只有一根 head 指標，連 size 都沒有）
//   ★ 省下 1/3 的節點記憶體，就是 forward_list 唯一的優勢
//
// === 8. splice_after:只改指標的搬移（同樣是開區間）===
//   整串移植後 A       [6]: 10 20 30 1 2 3
//   整串移植後 B       [0]:
//   ★ O(1)，不搬移任何元素、不配置也不釋放記憶體
//   單一移植後 A       [4]: 1 20 2 3
//   單一移植後 B       [2]: 10 30
//   ★ 搬的是 B.begin()(=10) 的**下一個**元素 20
//   把 4 移到最前面    [5]: 4 1 2 3 5
//
// === 9. LeetCode 203. Remove Linked List Elements ===
//   原始 val=6         [7]: 1 2 6 3 4 5 6
//   結果               [5]: 1 2 3 4 5
//   全等 val=7         [0]:
//   ★ before_begin() 就是標準內建的 dummy head，
//     所以「刪除頭節點」完全不需要特例分支
//
// === 10. LeetCode 206. Reverse Linked List ===
//   原始               [5]: 1 2 3 4 5
//   成員 reverse()     [5]: 5 4 3 2 1
//   手動 push_front    [5]: 5 4 3 2 1
//   單一元素           [1]: 1
//   ★ std::reverse 演算法對 forward_list **無法編譯**
//     （它需要 bidirectional iterator）→ 必須用成員版
//
// === 11. 實務:記憶體池的 free list ===
//   初始空閒區塊       [5]: 0 1 2 3 4
//   配置兩塊 → #0 #1
//   剩餘空閒           [3]: 2 3 4
//   歸還 #0
//   歸還後             [4]: 0 2 3 4
//   空閒數量 = 4（沒有 size()，自己算）
//
// === 12. 實務:依優先度插入的工作佇列 ===
//   依優先度排列       [5]: 處理付款(P9) 產生報表(P5) 備份資料庫(P3) 寄送通知(P1) 清理暫存(P1)
//   ★ 有序插入天生需要「前一個位置」，
//     與 insert_after 的 API 形狀完全契合
//
// === 重點整理 ===
//   1. 只有 next 指標 → 走不回前一個 → 所有 API 都是 _after
//   2. before_begin() 是「第一個之前」的虛擬位置，**不可解參考**
//   3. erase_after(pos) 刪的是 pos 的**下一個**
//   4. erase_after(first,last) 是 **(first,last) 開區間**，兩端都不刪
//   5. 沒有 size()（刻意設計）；用 std::distance，O(n)
//   6. 沒有 push_back/back()/rbegin()；不能用 std::sort/std::reverse
//   7. 每節點省一個指標（16 vs 24 bytes），代價是只能單向走訪
