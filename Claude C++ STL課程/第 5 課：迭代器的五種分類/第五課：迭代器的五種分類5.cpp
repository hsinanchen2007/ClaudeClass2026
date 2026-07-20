// =============================================================================
//  第五課：迭代器的五種分類 5  —  Forward Iterator（前向迭代器）與 multi-pass 保證
// =============================================================================
//
// 【主題資訊 Information】
//   類別標籤：std::forward_iterator_tag
//   代表容器：std::forward_list、std::unordered_set / unordered_map
//   支援操作：*it（可讀可寫）、++it / it++、it == / != it2、預設可建構、可複製
//   不支援：--it（不能後退）、it + n（不能跳躍）、it1 < it2（不能比大小）
//   標頭檔：<forward_list>、<unordered_set>、<iterator>
//   標準版本：五種迭代器分類自 C++98 就存在；
//             std::forward_list 與 unordered_* 是 C++11 才加入標準。
//   移動 n 步的複雜度：O(n)（只能一步一步 ++）
//
// 【詳細解釋 Explanation】
//
// 【1. Forward Iterator 比 Input Iterator 多了什麼：multi-pass 保證】
//   這是本檔最核心、也最常被講漏的一點。
//   Input Iterator 只保證「單次通過」（single-pass）：你可以從頭掃到尾一次，
//   但標準不保證你能把迭代器存起來、之後再用它掃第二次。
//   典型例子是 std::istream_iterator——資料從輸入串流讀出來就消失了，
//   複製一份迭代器再讀，讀到的是「後面的」資料，不是同一筆。
//
//   Forward Iterator 補上的正是這個保證：
//       若 it1 == it2，則 ++it1 == ++it2，且 *it1 與 *it2 是同一個物件。
//   白話說，迭代器可以被複製、存起來、稍後再用，走出來的結果完全一樣。
//   本檔的 main 就是在示範這件事：同一個 forward_list 掃兩次結果相同，
//   而且事先存起來的 saved 迭代器在第二次遍歷之後依然有效、依然指向 20。
//
// 【2. 為什麼不能後退：資料結構決定能力，不是標準故意刁難】
//   forward_list 是「單向鏈結串列」，每個節點只有一個 next 指標：
//       [10|next] -> [20|next] -> [30|next] -> ... -> nullptr
//   節點裡根本沒有 prev 欄位，所以「後退」在物理上無從實作。
//   迭代器分類不是抽象的階級遊戲，而是誠實反映底層結構能提供什麼：
//       單向鏈結 → 只能 ++            → Forward
//       雙向鏈結（list）→ 可以 ++/--   → Bidirectional
//       連續記憶體（vector）→ 可以 +n  → Random Access
//   把 --it 寫出來會直接編譯失敗（本檔 main 內有註解示範），
//   這是好事：錯誤在編譯期就被擋下，不會變成執行期的謎題。
//
// 【3. 沒有 prev 的世界怎麼刪除元素：before_begin() + erase_after()】
//   一般容器刪除是 erase(it)，但那需要「把前一個節點的 next 接到後一個」，
//   而 forward_list 的迭代器拿不到前一個節點。
//   標準的解法是把整組操作都改成「對後一個動手」：
//       insert_after(pos, v)、erase_after(pos)、emplace_after(pos, ...)
//   並額外提供一個 before_begin()——指向「第一個元素之前」的虛擬位置，
//   讓「刪除第一個元素」也能用同一套 API 表達。
//   注意 before_begin() 回傳的迭代器 **不可解參考**（*it 是 UB），
//   它只能拿來當 xxx_after 的參數。
//   本檔的實務範例就是這個慣用法的完整示範。
//
// 【4. forward_list 為什麼連 size() 都沒有】
//   std::forward_list 刻意不提供 size()。理由是它的設計目標是
//   「與手寫單向鏈結串列一樣省」——多存一個 size 欄位就多一個 word，
//   而多數使用單向串列的場合根本不需要 size。
//   要算長度得自己 std::distance(begin(), end())，那是 O(n)。
//   這也是為什麼下面 LeetCode 707 的實作要自己維護一個 size_ 計數器。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 五種分類的「能力包含關係」
//     Input ⊂ Forward ⊂ Bidirectional ⊂ Random Access
//     Output 則是獨立的一支（只能寫不能讀）。
//     這個包含關係讓演算法可以宣告「我最少需要哪一級」：
//     std::find 只要求 Input，所以 forward_list 也能用；
//     std::reverse 要求 Bidirectional，forward_list 就不能用；
//     std::sort 要求 Random Access，連 std::list 都不能用。
//
// (B) C++20 之後分類多了一層：iterator concepts
//     C++20 引入 std::forward_iterator 等 concept，與舊的 tag 並存。
//     另外新增了 contiguous_iterator（保證元素在記憶體中連續，
//     可以安全取 &*it 當陣列指標用）——vector 與 array 屬於這一類，
//     deque 則否（deque 是分段連續，不是整體連續）。
//     舊的 std::random_access_iterator_tag 並不保證連續，
//     這是 C++20 才補上的區分。
//
// (C) 標籤（tag）是怎麼被用到的：編譯期分派
//     std::advance(it, n) 內部會看 iterator_traits<It>::iterator_category，
//     對 Random Access 直接 it += n（O(1)），
//     對 Forward/Bidirectional 只能迴圈 ++（O(n)）。
//     這個選擇發生在編譯期（tag dispatch 或 C++17 起的 if constexpr），
//     執行期沒有任何判斷成本。
//
// 【注意事項 Pay Attention】
//   1. before_begin() 回傳的迭代器不可解參考，*it 是未定義行為。
//   2. forward_list 沒有 size()、沒有 back()、沒有 push_back()。
//      需要這些就該改用 std::list 或 std::vector。
//   3. Forward Iterator 的 multi-pass 是**標準保證**，不是「碰巧可以」。
//      Input Iterator 掃第二次可能得到完全不同的結果，且不算 bug。
//   4. 對 forward_list 做 erase_after 之後，被刪節點的迭代器立即失效；
//      但其他節點的迭代器**不受影響**（鏈結串列的優點）。
//   5. unordered_set / unordered_map 的迭代器也是 Forward，
//      但 rehash（例如 insert 導致擴容）會讓所有迭代器失效。
//   6. unordered_* 的走訪順序由 hash 與桶數決定，不是插入順序，
//      也不保證跨實作／跨版本一致——不要寫死期待的順序。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Forward Iterator 與 multi-pass 保證
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Forward Iterator 比 Input Iterator 多了什麼能力？
//     答：multi-pass 保證。Input Iterator 只保證單次通過，複製一份存起來
//         之後再用，不保證能重現同樣的走訪結果（istream_iterator 就是如此，
//         資料讀走就沒了）。Forward Iterator 保證
//         「it1 == it2 ⟹ ++it1 == ++it2，且 *it1 與 *it2 是同一個物件」，
//         所以可以放心把迭代器存起來、之後再掃一次。
//     追問：那 Forward 和 Bidirectional 差在哪？
//         → 只差一個 --it。能不能後退取決於節點有沒有 prev 指標：
//           forward_list 單向鏈結沒有，list 雙向鏈結有。
//
// 🔥 Q2. forward_list 為什麼要提供 before_begin()？直接 erase(it) 不好嗎？
//     答：單向鏈結刪除節點必須改「前一個節點的 next」，但 forward_list 的
//         迭代器拿不到前一個節點。所以標準把所有修改操作改成對「後一個」
//         動手（insert_after / erase_after），再用 before_begin() 這個
//         「第一個元素之前」的虛擬位置，讓刪除頭節點也能套用同一套 API。
//     追問：before_begin() 可以解參考嗎？
//         → 不行，*before_begin() 是未定義行為。它只能當 xxx_after 的參數。
//
// ⚠️ 陷阱. std::find 可以用在 forward_list，那 std::sort 也可以吧？
//         它們不都是 <algorithm> 裡的演算法嗎？
//     答：不行，std::sort 要求 Random Access Iterator，forward_list 給不起。
//         演算法的可用性不是看「它在哪個標頭檔」，而是看它對迭代器的**最低要求**：
//         find 只要 Input（所以誰都能用）、reverse 要 Bidirectional
//         （forward_list 出局）、sort 要 Random Access（連 list 都出局）。
//         forward_list 想排序只能用成員函式 flist.sort()，
//         它走的是為鏈結串列設計的合併排序。
//     為什麼會錯：把 <algorithm> 想成「一組通用工具，對所有容器一視同仁」。
//         實際上每個演算法都對迭代器能力有明確要求，
//         這個要求就寫在標準的函式簽章裡（C++20 之後直接寫成 concept）。
//         std::sort 需要隨機跳躍做 partition，鏈結串列做不到，
//         硬要支援就會從 O(n log n) 退化。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <forward_list>
#include <unordered_set>
#include <iterator>
#include <string>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：實作一個鏈結串列，支援 get / addAtHead / addAtTail /
//         addAtIndex / deleteAtIndex。
//   為什麼用到本主題：這題要求的正是「單向鏈結串列」的語意，而
//     std::forward_list 就是標準版的單向鏈結串列。用它來解可以完整示範
//     Forward Iterator 的三個特徵：
//       (1) 只能 ++ 前進 → get(index) 只能 std::advance 一步步走，O(index)
//       (2) 沒有 prev → 插入/刪除全靠 before_begin() + insert_after/erase_after
//       (3) 沒有 size() → 必須自己維護計數器
//     這三點正好是 forward_list 與 list/vector 最實際的差別。
// -----------------------------------------------------------------------------
class MyLinkedList {
public:
    // 取得第 index 個元素；越界回傳 -1
    int get(int index) const {
        if (index < 0 || index >= size_) return -1;
        auto it = data_.begin();
        std::advance(it, index);   // Forward Iterator：只能一步步走，O(index)
        return *it;
    }

    void addAtHead(int val) {
        data_.push_front(val);     // forward_list 只有 push_front，沒有 push_back
        ++size_;
    }

    void addAtTail(int val) {
        // 沒有 back()、沒有 tail 指標，只能走到最後一個節點
        auto prev = data_.before_begin();
        for (auto it = data_.begin(); it != data_.end(); ++it) prev = it;
        data_.insert_after(prev, val);
        ++size_;
    }

    void addAtIndex(int index, int val) {
        if (index > size_) return;      // 超過長度：題目規定不插入
        if (index < 0) index = 0;
        // before_begin() 前進 index 步，剛好停在「第 index 個的前一個」
        auto prev = data_.before_begin();
        std::advance(prev, index);
        data_.insert_after(prev, val);
        ++size_;
    }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= size_) return;
        auto prev = data_.before_begin();
        std::advance(prev, index);
        data_.erase_after(prev);        // 刪的是 prev 的下一個
        --size_;
    }

    std::string dump() const {
        std::string s;
        for (int v : data_) {
            if (!s.empty()) s += " -> ";
            s += std::to_string(v);
        }
        return s.empty() ? "(空)" : s;
    }

private:
    std::forward_list<int> data_;
    int size_ = 0;      // forward_list 刻意不提供 size()，只能自己記
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用 forward_list 維護連線 session 清單，定期清掉逾時者
//   情境：伺服器保存一批 active session，每輪心跳檢查把閒置過久的踢掉。
//         session 數量不多、只需要「從頭掃到尾」、又希望每個節點的
//         記憶體開銷越小越好——這正是 forward_list 的適用場景
//         （每節點只有一個 next 指標，比 list 少一個 prev）。
//   為什麼用到本主題：刪除時無法取得「前一個節點」，
//     必須用 before_begin() + erase_after() 這個 Forward Iterator 專屬慣用法。
//     注意 erase_after 會回傳「被刪節點的下一個」，所以刪除後
//     prev 保持不動、cur 直接接上回傳值，迴圈才不會漏掃。
// -----------------------------------------------------------------------------
struct Session {
    int         id;
    std::string user;
    int         idleSeconds;
};

std::size_t dropIdleSessions(std::forward_list<Session>& sessions, int limitSec) {
    std::size_t removed = 0;
    auto prev = sessions.before_begin();   // 虛擬的「頭之前」，不可解參考
    auto cur  = sessions.begin();

    while (cur != sessions.end()) {
        if (cur->idleSeconds >= limitSec) {
            cur = sessions.erase_after(prev);   // prev 不動，cur 接上下一個
            ++removed;
        } else {
            prev = cur;                        // 手動維護「前一個」
            ++cur;
        }
    }
    return removed;
}

int main() {
    // forward_list 的迭代器是 Forward Iterator
    std::cout << "=== forward_list ===" << std::endl;
    std::forward_list<int> flist = {10, 20, 30, 40, 50};

    // 保存一個迭代器
    auto saved = flist.begin();
    ++saved;  // 指向 20

    // 先遍歷一次
    std::cout << "第一次遍歷: ";
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 可以再遍歷一次（Input Iterator 不行）
    std::cout << "第二次遍歷: ";
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 之前保存的迭代器還有效 —— 這就是 multi-pass 保證
    std::cout << "保存的迭代器指向: " << *saved << std::endl;

    // 但不能後退
    // --saved;  // 編譯錯誤！forward_list 的節點沒有 prev 指標

    // 再利用保存的迭代器繼續遍歷
    std::cout << "從保存的迭代器繼續遍歷: ";
    for (auto it = saved; it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== multi-pass 保證的精確含義 ===" << std::endl;
    {
        auto a = flist.begin();
        auto b = a;                       // 複製一份
        std::cout << "a == b ? " << std::boolalpha << (a == b) << std::endl;
        ++a; ++b;
        std::cout << "各自 ++ 之後 a == b ? " << (a == b) << std::endl;
        std::cout << "*a 與 *b 是同一個物件? " << (&*a == &*b) << std::endl;
        std::cout << "→ 這三個條件成立才叫 Forward Iterator；"
                  << "Input Iterator 不保證" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== unordered_set 的迭代器也是 Forward ===" << std::endl;
    {
        std::unordered_set<int> us = {1, 2, 3};
        auto it = us.begin();
        ++it;                 // 可以前進
        // --it;              // 編譯錯誤：Forward 不能後退
        std::cout << "unordered_set 可以 ++ 但不能 --" << std::endl;
        std::cout << "元素個數: " << us.size()
                  << "（走訪順序由 hash 決定，不保證跨實作一致，故不印內容）"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== forward_list 沒有 size()，長度要自己算 ===" << std::endl;
    {
        auto n = std::distance(flist.begin(), flist.end());   // O(n)
        std::cout << "std::distance 算出長度 = " << n
                  << "（O(n)，不是 O(1)）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 707. Design Linked List ===" << std::endl;
    {
        MyLinkedList list;
        list.addAtHead(1);
        list.addAtTail(3);
        list.addAtIndex(1, 2);        // 變成 1 -> 2 -> 3
        std::cout << "addAtHead(1), addAtTail(3), addAtIndex(1,2): "
                  << list.dump() << std::endl;
        std::cout << "get(1) = " << list.get(1) << std::endl;   // 2
        list.deleteAtIndex(1);        // 變成 1 -> 3
        std::cout << "deleteAtIndex(1) 後: " << list.dump() << std::endl;
        std::cout << "get(1) = " << list.get(1) << std::endl;   // 3
        std::cout << "get(99) = " << list.get(99)
                  << "（越界回傳 -1）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：清掉逾時的 session ===" << std::endl;
    {
        std::forward_list<Session> sessions = {
            {1001, "alice",   5},
            {1002, "bob",   310},      // 逾時
            {1003, "carol",  42},
            {1004, "dave",  600},      // 逾時
            {1005, "erin",   12},
        };

        std::cout << "清理前: ";
        for (const auto& s : sessions)
            std::cout << s.user << "(" << s.idleSeconds << "s) ";
        std::cout << std::endl;

        std::size_t n = dropIdleSessions(sessions, 300);

        std::cout << "移除 " << n << " 個逾時 session" << std::endl;
        std::cout << "清理後: ";
        for (const auto& s : sessions)
            std::cout << s.user << "(" << s.idleSeconds << "s) ";
        std::cout << std::endl;
        std::cout << "→ 刪除全靠 before_begin() + erase_after()，"
                  << "因為拿不到「前一個」節點" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類5.cpp -o demo5

// === 預期輸出 ===
// === forward_list ===
// 第一次遍歷: 10 20 30 40 50
// 第二次遍歷: 10 20 30 40 50
// 保存的迭代器指向: 20
// 從保存的迭代器繼續遍歷: 20 30 40 50
//
// === multi-pass 保證的精確含義 ===
// a == b ? true
// 各自 ++ 之後 a == b ? true
// *a 與 *b 是同一個物件? true
// → 這三個條件成立才叫 Forward Iterator；Input Iterator 不保證
//
// === unordered_set 的迭代器也是 Forward ===
// unordered_set 可以 ++ 但不能 --
// 元素個數: 3（走訪順序由 hash 決定，不保證跨實作一致，故不印內容）
//
// === forward_list 沒有 size()，長度要自己算 ===
// std::distance 算出長度 = 5（O(n)，不是 O(1)）
//
// === LeetCode 707. Design Linked List ===
// addAtHead(1), addAtTail(3), addAtIndex(1,2): 1 -> 2 -> 3
// get(1) = 2
// deleteAtIndex(1) 後: 1 -> 3
// get(1) = 3
// get(99) = -1（越界回傳 -1）
//
// === 日常實務：清掉逾時的 session ===
// 清理前: alice(5s) bob(310s) carol(42s) dave(600s) erin(12s)
// 移除 2 個逾時 session
// 清理後: alice(5s) carol(42s) erin(12s)
// → 刪除全靠 before_begin() + erase_after()，因為拿不到「前一個」節點
