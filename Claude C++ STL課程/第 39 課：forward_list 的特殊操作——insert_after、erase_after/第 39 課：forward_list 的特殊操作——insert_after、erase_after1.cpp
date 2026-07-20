// =============================================================================
//  第 39 課：forward_list 的特殊操作——insert_after、erase_after1.cpp
//    —  單向鏈結只有 next，於是整組 API 都變成「在之後」
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <forward_list>          （C++11 新增）
//   結構:   單向鏈結串列，每個節點只有 next 指標 + value
//
//     insert_after(pos, v / n,v / {init} / first,last)   回傳指向新元素的 iterator
//     emplace_after(pos, args...)                        原地建構
//     erase_after(pos)                                   刪除 pos 的**下一個**
//     erase_after(first, last)                           刪除 **(first,last) 開區間**
//     splice_after(pos, other[, ...])                    只改指標的搬移
//     before_begin()                                     「第一個元素之前」
//   單一元素操作皆 O(1)（已持有 pos 時）。
//
//   ★ **沒有 size()**（刻意設計）；要長度用 std::distance，O(n)
//   ★ **沒有 push_back / back() / rbegin()**
//   ★ 迭代器只是 forward iterator：不能 --、不能 +n
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼全部都是 _after】
// 單向鏈結的節點只有 next、沒有 prev。要在節點 X「之前」插入，必須改
// 「X 的前一個節點」的 next，但從指向 X 的 iterator **走不回前一個**，
// 只能從頭再找一遍 → O(n)。標準因此選擇**不提供做不到 O(1) 的介面**，
// 把 API 定義成「在 pos 之後」：
//     insert_after(pos,v) → 只改 pos->next                     O(1)
//     erase_after(pos)    → 把 pos->next 換成 pos->next->next  O(1)
// 對照 list：它有 prev 指標，所以 insert(pos,v)（在 pos 之前）才是 O(1)。
//
// 【2. before_begin() 的存在理由】
// 既然操作都是「在 pos 之後」，那要在**第一個元素之前**插入呢？
// 沒有任何 pos 的「之後」是第一個位置——所以標準給了一個虛擬位置：
//     fl.insert_after(fl.before_begin(), v);   // 等同 push_front
//     fl.erase_after(fl.before_begin());       // 刪掉第一個元素
// 實作上它指向容器內嵌、不存放元素的 head 節點。
// ★ **before_begin() 不可解參考**（*it 是 UB），只能當引數用。
//   std::next(before_begin()) 就是 begin()。
//
// 【3. erase_after 的區間是「開區間」】
// STL 幾乎所有區間都是半開 [first,last)，唯獨這裡是 **(first,last)**：
// first 與 last **本身都不會被刪**，刪的是它們**之間**的元素。
// 因為 first 必須是「要刪的第一個元素的前驅」（單向鏈結唯一能改接的位置），
// 而 last 要被接回 first 後面，當然不能刪。
// 記法：**兩個引數都是邊界樁，拔掉的是樁與樁之間的東西。**
//
// 【概念補充 Concept Deep Dive】
// forward_list 的設計目標是「與手寫單向鏈結串列有相同的空間與時間開銷」，
// 所以連 size() 都不提供（維護計數會讓容器變大，也會讓 splice_after
// 這類只改指標的 O(1) 操作被迫變成 O(n)）。
// 本機實測：forward_list<int> 每節點 16 bytes、list<int> 每節點 24 bytes；
// sizeof(forward_list<int>)=8（只有一根 head 指標）、sizeof(list<int>)=24。
// 省下 1/3 的節點記憶體正是它唯一的優勢。
// ★ 以上為 libstdc++ 64-bit 實測值，非標準規定。
//
// 【注意事項 Pay Attention】
//  1. before_begin() **不可解參考**；也不能拿來判斷容器是否為空（用 empty()）。
//  2. erase_after(first,last) 是 **(first,last) 開區間**，兩端都不刪。
//  3. 沒有 size()：用 std::distance(begin(),end())，O(n)。
//  4. 沒有 push_back；要尾端追加請自行維護 tail iterator（見範例 6）。
//  5. 不能用 std::sort / std::reverse（forward iterator）→ 用成員版。
//  6. 迴圈刪除要自己追蹤 prev，且刪除時 prev **不前進**。
//  7. remove/remove_if/unique 在 C++17 以前回傳 void，C++20 起回傳移除個數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】forward_list 的 _after 系列
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. forward_list 為什麼沒有 insert()，只有 insert_after()?
//        又為什麼需要 before_begin()?
//     答：節點只有 next 指標，從某個 iterator **走不回它的前一個節點**，
//         所以「在 pos 之前插入」無法做到 O(1)（得從頭找 prev）。
//         標準選擇不提供做不到 O(1) 的介面，改成「在 pos 之後」。
//         但這樣就沒辦法插到第一個元素之前了，所以額外提供 before_begin()
//         這個虛擬位置（指向不存放元素的 head 節點）來補上這個缺口。
//     追問：before_begin() 可以解參考嗎?→ **不行**，它不指向任何元素，
//         *fl.before_begin() 是 UB；它只能當 _after 系列函式的引數。
//
// ⚠️ 陷阱. fl.erase_after(first, last) 刪掉了哪些元素?
//        forward_list<int> fl = {10,20,30,40,50};
//        auto first = fl.begin();               // → 10
//        auto last  = std::next(first, 3);      // → 40
//        fl.erase_after(first, last);
//     答：刪掉 **20 和 30**，結果 {10,40,50}。這是 **(first,last) 開區間**，
//         first(10) 與 last(40) 本身都保留。
//     為什麼會錯：STL 其他地方一律是半開區間 [first,last)，直覺會以為
//         「刪 10,20,30」。但單向鏈結只能從「前一個」下手：first 必須是
//         要刪的第一個元素的**前驅**，而 last 要被接回 first 後面。
//
// ⚠️ 陷阱2. 這段邊走邊刪的程式錯在哪?
//        if (要刪) { curr = fl.erase_after(prev); prev = curr; }
//     答：錯在刪除後又把 prev 設成 curr。刪除時 prev 的下一個已經換成新元素，
//         **prev 必須保持不動**；把它前移會讓下一輪刪錯位置（跳過一個元素）。
//         正確寫法：刪除分支只做 `curr = fl.erase_after(prev);`。
//     為什麼會錯：以為「兩個游標要一起前進」。其實 prev 的語意是
//         「最後一個**確定保留**的節點」，只有不刪的時候它才推進。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <forward_list>
#include <vector>
#include <string>
#include <iterator>

template <typename T>
void print(const std::string& label, const std::forward_list<T>& fl) {
    // 沒有 size()，長度得自己算
    std::cout << "  " << label << " [" << std::distance(fl.begin(), fl.end()) << "]:";
    for (const auto& v : fl) std::cout << " " << v;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 203. Remove Linked List Elements
//   題目：刪除鏈結串列中所有等於 val 的節點。
//   為什麼用到本主題：這題最經典的技巧就是**加一個 dummy head**，
//     讓「刪除第一個節點」不必寫特例——而 forward_list 的 before_begin()
//     正是標準內建的 dummy head。手動追蹤 prev 的迴圈也正是原題
//     指標操作的直譯，能同時練到 erase_after 與 prev 不前進的要點。
// -----------------------------------------------------------------------------
std::forward_list<int> removeElements(std::forward_list<int> head, int val) {
    auto prev = head.before_begin();          // ← 標準版 dummy head
    auto curr = head.begin();
    while (curr != head.end()) {
        if (*curr == val) curr = head.erase_after(prev);   // ★ prev 不動
        else            { prev = curr; ++curr; }
    }
    return head;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】事件處理鏈（handler chain）:註冊、插隊、移除
//   情境：框架維護一串事件處理器，依序執行；模組可在某個處理器之後插入
//         自己的處理器，也可以把失效的處理器移除。
//   為什麼用 forward_list：處理鏈只會**從頭往後**依序執行，永遠不需要
//     反向走訪；註冊時我們手上本來就有「要插在誰後面」的位置，
//     與 insert_after 的形狀完全吻合；每節點還能省下一個 prev 指標。
// -----------------------------------------------------------------------------
struct Handler {
    std::string name;
    bool        enabled;
};
std::ostream& operator<<(std::ostream& os, const Handler& h) {
    return os << h.name << (h.enabled ? "" : "(停用)");
}

// 在名為 afterName 的處理器之後插入；找不到就插到最前面
void registerAfter(std::forward_list<Handler>& chain,
                   const std::string& afterName, const Handler& h) {
    auto prev = chain.before_begin();
    for (auto it = chain.begin(); it != chain.end(); ++it) {
        if (it->name == afterName) { chain.insert_after(it, h); return; }
        prev = it;
    }
    chain.insert_after(chain.before_begin(), h);   // 沒找到 → 放最前面
    (void)prev;
}

int main() {
    std::cout << "=== 1. insert_after 的各種重載 ===\n";
    {
        std::forward_list<int> fl = {10, 40, 70};
        print("初始              ", fl);
        auto it  = fl.begin();                      // → 10
        auto ret = fl.insert_after(it, 20);
        print("insert_after(10,20)", fl);
        std::cout << "  回傳→" << *ret << "\n";
        ret = fl.insert_after(ret, 2, 30);          // 插入 2 個 30
        print("insert 2×30       ", fl);
        std::cout << "  回傳→" << *ret << "（最後一個新元素）\n";
        ret = fl.insert_after(fl.before_begin(), {1, 2, 3});
        print("頭端插入{1,2,3}   ", fl);
        std::cout << "  回傳→" << *ret << "\n";
        std::vector<int> extra = {97, 98, 99};       // 迭代器範圍版
        auto tail = fl.before_begin();
        for (auto i = fl.begin(); i != fl.end(); ++i) tail = i;   // 走到尾端 O(n)
        fl.insert_after(tail, extra.begin(), extra.end());
        print("尾端插入97,98,99  ", fl);
    }

    std::cout << "\n=== 2. before_begin():在第一個之前操作的唯一辦法 ===\n";
    {
        std::forward_list<std::string> fl = {"banana", "cherry"};
        print("初始              ", fl);
        fl.insert_after(fl.before_begin(), "apple");   // 等同 push_front
        print("插到最前面        ", fl);
        fl.erase_after(fl.before_begin());             // 刪掉第一個
        print("刪掉第一個        ", fl);
        std::cout << "  ★ before_begin() **不可解參考**（*it 是 UB）\n";
        std::cout << "  ★ std::next(before_begin()) 就是 begin():"
                  << *std::next(fl.before_begin()) << " == " << *fl.begin() << "\n";
    }

    std::cout << "\n=== 3. erase_after:單一與「開區間」範圍（最易錯）===\n";
    {
        std::forward_list<int> fl = {10,20,30,40,50,60,70};
        print("初始              ", fl);
        auto ret = fl.erase_after(fl.begin());       // 刪 10 的下一個 = 20
        print("erase_after(begin)", fl);
        std::cout << "  回傳→" << *ret << "（被刪元素的下一個）\n";
        fl.erase_after(fl.before_begin());           // 刪第一個 = 10
        print("刪第一個          ", fl);
    }
    {
        std::forward_list<int> fl = {10,20,30,40,50};
        print("重新初始          ", fl);
        auto first = fl.begin();                     // → 10
        auto last  = std::next(first, 3);            // → 40
        std::cout << "  first→" << *first << "  last→" << *last << "\n";
        fl.erase_after(first, last);                 // 刪 (10,40) = 20,30
        print("erase_after(f,l)  ", fl);
        std::cout << "  ★ 刪的是 20 與 30 —— (first,last) **開區間**，\n";
        std::cout << "    first(10) 與 last(40) 本身都保留！與 STL 慣例相反\n";
    }

    std::cout << "\n=== 4. emplace_after 鏈式追加 ===\n";
    {
        std::forward_list<std::pair<std::string,int>> scores;
        auto pos = scores.before_begin();
        pos = scores.emplace_after(pos, "Alice", 95);   // 拿回傳值當下一次的 pos
        pos = scores.emplace_after(pos, "Bob", 88);
        pos = scores.emplace_after(pos, "Charlie", 92);
        for (const auto& [n, s] : scores) std::cout << "  " << n << ": " << s << "\n";
    }

    std::cout << "\n=== 5. 迴圈中安全刪除:自己追蹤 prev ===\n";
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
        fl.remove_if([](int x) { return x % 3 == 0; });
        print("刪 3 的倍數(rm_if)", fl);
        std::cout << "  ★ 結果相同；remove_if 不必手動管 prev，不易寫錯\n";
    }

    std::cout << "\n=== 6. 沒有 push_back:用 tail iterator 模擬 ===\n";
    {
        std::forward_list<int> fl;
        auto tail = fl.before_begin();
        for (int i = 1; i <= 5; ++i)
            tail = fl.insert_after(tail, i * 10);    // 每次 O(1)
        print("模擬 push_back    ", fl);
        std::cout << "  ★ 維護一個 tail iterator 就能 O(1) 尾端追加\n";
        std::cout << "  ★ 長度只能用 distance 算:"
                  << std::distance(fl.begin(), fl.end()) << "（O(n)，沒有 size()）\n";
    }

    std::cout << "\n=== 7. splice_after:只改指標的搬移 ===\n";
    {
        std::forward_list<int> A = {1,2,3}, B = {10,20,30};
        A.splice_after(A.before_begin(), B);         // 整串搬到 A 最前面
        print("整串移植後 A      ", A);
        print("整串移植後 B      ", B);
        std::cout << "  ★ O(1)，不搬元素、不配置也不釋放記憶體\n";
    }
    {
        std::forward_list<int> A = {1,2,3}, B = {10,20,30};
        A.splice_after(A.begin(), B, B.begin());     // 搬 B.begin()(=10) 的下一個
        print("單一移植後 A      ", A);
        print("單一移植後 B      ", B);
        std::cout << "  ★ 搬的是 10 的**下一個**（=20），不是 10 本身\n";
    }
    {
        std::forward_list<int> fl = {1,2,3,4,5};
        auto before4 = std::next(fl.begin(), 2);     // → 3（4 的前一個）
        fl.splice_after(fl.before_begin(), fl, before4);
        print("把 4 移到最前面   ", fl);
    }

    std::cout << "\n=== 8. splice_after 後 iterator 仍有效（歸屬改變）===\n";
    {
        std::forward_list<int> A = {1,2,3}, B = {10,20,30};
        auto it20 = std::next(B.begin());            // → 20
        std::cout << "  splice 前 *it20=" << *it20 << "（屬於 B）\n";
        A.splice_after(A.begin(), B, B.begin());     // 把 20 搬到 A
        std::cout << "  splice 後 *it20=" << *it20 << "（現在屬於 A，iterator 未失效）\n";
        print("A                 ", A);
        print("B                 ", B);
    }

    std::cout << "\n=== 9. LeetCode 203. Remove Linked List Elements ===\n";
    print("原始 val=6        ", std::forward_list<int>{1,2,6,3,4,5,6});
    print("結果              ", removeElements({1,2,6,3,4,5,6}, 6));
    print("全等 val=7        ", removeElements({7,7,7,7}, 7));
    std::cout << "  ★ before_begin() 就是標準內建的 dummy head，\n";
    std::cout << "    「刪除頭節點」完全不需要特例分支\n";

    std::cout << "\n=== 10. 實務:事件處理鏈 ===\n";
    {
        std::forward_list<Handler> chain;
        auto tail = chain.before_begin();
        for (const char* n : {"驗證身分", "解析請求", "寫入稽核log"})
            tail = chain.insert_after(tail, Handler{n, true});
        print("初始處理鏈        ", chain);

        registerAfter(chain, "解析請求", Handler{"檢查權限", true});   // 插隊
        print("插入「檢查權限」  ", chain);

        registerAfter(chain, "不存在的名字", Handler{"限流", true});   // 找不到→最前
        print("插入「限流」      ", chain);

        chain.remove_if([](const Handler& h) { return !h.enabled; });
        chain.begin()->enabled = false;                 // 停用第一個
        chain.remove_if([](const Handler& h) { return !h.enabled; });
        print("移除停用的處理器  ", chain);
        std::cout << "  ★ 處理鏈只會由前往後執行，永遠不需要反向走訪\n";
        std::cout << "    → 正是 forward_list 適用的場景\n";
    }
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 39 課：forward_list 的特殊操作——insert_after、erase_after1.cpp" -o flist_ops

// === 預期輸出 ===
// === 1. insert_after 的各種重載 ===
//   初始               [3]: 10 40 70
//   insert_after(10,20) [4]: 10 20 40 70
//   回傳→20
//   insert 2×30        [6]: 10 20 30 30 40 70
//   回傳→30（最後一個新元素）
//   頭端插入{1,2,3}    [9]: 1 2 3 10 20 30 30 40 70
//   回傳→3
//   尾端插入97,98,99   [12]: 1 2 3 10 20 30 30 40 70 97 98 99
//
// === 2. before_begin():在第一個之前操作的唯一辦法 ===
//   初始               [2]: banana cherry
//   插到最前面         [3]: apple banana cherry
//   刪掉第一個         [2]: banana cherry
//   ★ before_begin() **不可解參考**（*it 是 UB）
//   ★ std::next(before_begin()) 就是 begin():banana == banana
//
// === 3. erase_after:單一與「開區間」範圍（最易錯）===
//   初始               [7]: 10 20 30 40 50 60 70
//   erase_after(begin) [6]: 10 30 40 50 60 70
//   回傳→30（被刪元素的下一個）
//   刪第一個           [5]: 30 40 50 60 70
//   重新初始           [5]: 10 20 30 40 50
//   first→10  last→40
//   erase_after(f,l)   [3]: 10 40 50
//   ★ 刪的是 20 與 30 —— (first,last) **開區間**，
//     first(10) 與 last(40) 本身都保留！與 STL 慣例相反
//
// === 4. emplace_after 鏈式追加 ===
//   Alice: 95
//   Bob: 88
//   Charlie: 92
//
// === 5. 迴圈中安全刪除:自己追蹤 prev ===
//   原始               [10]: 1 2 3 4 5 6 7 8 9 10
//   刪 3 的倍數(手動)  [7]: 1 2 4 5 7 8 10
//   刪 3 的倍數(rm_if) [7]: 1 2 4 5 7 8 10
//   ★ 結果相同；remove_if 不必手動管 prev，不易寫錯
//
// === 6. 沒有 push_back:用 tail iterator 模擬 ===
//   模擬 push_back     [5]: 10 20 30 40 50
//   ★ 維護一個 tail iterator 就能 O(1) 尾端追加
//   ★ 長度只能用 distance 算:5（O(n)，沒有 size()）
//
// === 7. splice_after:只改指標的搬移 ===
//   整串移植後 A       [6]: 10 20 30 1 2 3
//   整串移植後 B       [0]:
//   ★ O(1)，不搬元素、不配置也不釋放記憶體
//   單一移植後 A       [4]: 1 20 2 3
//   單一移植後 B       [2]: 10 30
//   ★ 搬的是 10 的**下一個**（=20），不是 10 本身
//   把 4 移到最前面    [5]: 4 1 2 3 5
//
// === 8. splice_after 後 iterator 仍有效（歸屬改變）===
//   splice 前 *it20=20（屬於 B）
//   splice 後 *it20=20（現在屬於 A，iterator 未失效）
//   A                  [4]: 1 20 2 3
//   B                  [2]: 10 30
//
// === 9. LeetCode 203. Remove Linked List Elements ===
//   原始 val=6         [7]: 1 2 6 3 4 5 6
//   結果               [5]: 1 2 3 4 5
//   全等 val=7         [0]:
//   ★ before_begin() 就是標準內建的 dummy head，
//     「刪除頭節點」完全不需要特例分支
//
// === 10. 實務:事件處理鏈 ===
//   初始處理鏈         [3]: 驗證身分 解析請求 寫入稽核log
//   插入「檢查權限」   [4]: 驗證身分 解析請求 檢查權限 寫入稽核log
//   插入「限流」       [5]: 限流 驗證身分 解析請求 檢查權限 寫入稽核log
//   移除停用的處理器   [4]: 驗證身分 解析請求 檢查權限 寫入稽核log
//   ★ 處理鏈只會由前往後執行，永遠不需要反向走訪
//     → 正是 forward_list 適用的場景
