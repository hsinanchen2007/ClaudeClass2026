// =============================================================================
//  summary.cpp  —  std::forward_list：一個為了「零開銷」而刻意殘缺的容器
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<forward_list>    標準版本：C++11 起
//   類別：  template<class T, class Allocator = std::allocator<T>> class forward_list;
//
//   核心成員（注意全部以 _after 結尾）：
//     front()                                O(1)
//     push_front(v) / pop_front()            O(1)
//     before_begin()                         O(1)   ← forward_list 特有
//     insert_after(pos, ...)                 O(1)（單一元素）
//     erase_after(pos) / erase_after(f, l)   O(1) / O(distance)
//     sort() / reverse() / unique() / remove() / remove_if() / merge()
//     splice_after(...)                      O(1)（全 list 版本）
//
//   刻意「沒有」的成員：size()、back()、push_back()、pop_back()、
//                       operator[]、rbegin()／rend()、insert()、erase()
//
// 【詳細解釋 Explanation】
//
// 【1. forward_list 的設計目標只有一個：不比手寫 C 鏈結串列貴】
//   C++11 引入它時，委員會給的定位非常明確——
//   「a container that supports forward iteration and meets the
//     space overhead of a hand-written singly linked list」。
//   意思是：任何會讓它比「你自己用 struct Node { T v; Node* next; }」
//   多花一個位元組、多一次指令的功能，一律不做。
//   於是 size() 被砍掉（要存就得多 8 bytes 或每次 O(n) 走訪）、
//   back()/push_back() 被砍掉（要 O(1) 就得多存一根 tail 指標）、
//   反向迭代被砍掉（要 -- 就得存 prev，那就變成 list 了）。
//   本機實測：sizeof(forward_list<int>) == 8（就是一根指標），
//   每個節點 16 bytes（next 8 + int 4 + padding 4）——
//   確實與手寫版本完全相同。
//
// 【2. 為什麼所有操作都叫 _after，而不是像 list 那樣直接 insert(pos)】
//   這是單向鏈結串列的物理限制，不是 API 設計偏好。
//   要在節點 X「之前」插入，必須先改 X 的前驅節點的 next 指標；
//   但單向串列拿到 X 之後根本走不回去找前驅——只能從頭重走 O(n)。
//   所以標準乾脆把介面改成「在 pos 之後動手」：
//       insert_after(pos, v)  只需要 pos->next = new_node，O(1)
//   這是一個很好的設計課：當底層做不到某件事時，
//   誠實的做法是改變介面語意，而不是提供一個偷偷 O(n) 的假 API。
//
// 【3. before_begin() 是為了補上「頭端」這個特例】
//   既然所有操作都在「某節點之後」，那要在最前面插入怎麼辦？
//   標準給了 before_begin()——一個指向「第一個元素之前的虛擬位置」的迭代器。
//   它讓頭端不再是特例：
//       fl.insert_after(fl.before_begin(), x);   // 等價於 push_front(x)
//   關鍵性質：before_begin() 不可解參考（*it 是未定義行為），
//   它只能當作 insert_after / erase_after / splice_after 的位置參數。
//   這其實就是 C 語言鏈結串列教學裡的「dummy head」技巧被標準化了。
//
// 【4. 沒有 size() 帶來的實際影響】
//   要知道長度只能 std::distance(fl.begin(), fl.end())，那是 O(n)。
//   所以：
//     * 判斷空容器一律用 empty()（O(1)），永遠不要用 distance(...) == 0。
//     * 若你的程式碼頻繁需要長度，代表你選錯容器了——應該用 list。
//   這個「缺陷」是設計目的本身：它逼你在選型時就想清楚需不需要長度。
//
// 【5. 什麼時候真的該用它】
//   坦白說日常應用程式很少用到。真正合適的場景是：
//     * 節點數量極大、記憶體是硬限制（每節點省 8 bytes，百萬節點省 8 MB）
//     * 只需要前向走訪、只在頭端或「已知位置之後」增刪
//     * 需要 O(1) 的 splice_after 把整段串接來接去
//   最貼近日常的例子其實藏在標準函式庫內部：
//   雜湊表的 separate chaining bucket 就是單向鏈結串列，
//   libstdc++ 的 std::unordered_map 內部正是用單向串列串起同一個 bucket
//   的元素——那裡不需要反向走訪、不需要 bucket 長度、而記憶體極度敏感。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 本機實測的記憶體數字（g++ 15.2 / libstdc++ 15 / x86-64；實作定義）
//     sizeof(forward_list<int>) = 8   （只有一根 head 指標）
//     sizeof(list<int>)         = 24  （next + prev + _M_size）
//     sizeof(vector<int>)       = 24  （begin + end + cap 三根指標）
//     每節點：forward_list 16 bytes、list 24 bytes（以自訂 allocator 攔截實測）
//     10 萬個 int：forward_list ≈ 1562 KB、list ≈ 2343 KB、vector ≈ 390 KB
//     → 注意 vector 仍然壓倒性地省：鏈結串列的 next 指標本身就比 int 大。
//       forward_list 省的是「相對於 list」，不是「相對於連續容器」。
//
// (B) 迭代器分類：ForwardIterator
//     只支援 ++、解參考、多次走訪（multi-pass）。
//     沒有 --，所以 rbegin()/rend() 不存在；
//     沒有 + n，所以要跳躍得用 std::advance / std::next（O(n)）。
//
// (C) 迭代器失效規則與 list 完全一致
//     插入不使任何迭代器失效；刪除只使「被刪除元素」的迭代器失效。
//     這是節點式容器的共同保證，也是它們相對 vector 的最大優勢。
//
// (D) 為什麼 unique() 只移除「連續」重複
//     它是單次線性掃描：比較相鄰兩個節點，相同就 erase_after。
//     要真正去重必須先 sort()。這與 std::unique 演算法的語意一致，
//     也是刻意的——O(n) 的操作不該偷偷變成 O(n log n)。
//
// 【注意事項 Pay Attention】
//   1. 沒有 size()。判空用 empty()；真要長度用 distance()，但那是 O(n)。
//   2. before_begin() 回傳的迭代器不可解參考，只能當位置參數。
//   3. erase_after(pos) 刪的是 pos 的「下一個」，不是 pos 自己；
//      erase_after(first, last) 刪的是開區間 (first, last)，兩端都不刪。
//   4. 沒有 push_back()。要在尾端持續追加，必須自己維護一個 tail 迭代器
//      （本檔第 8 節示範），否則每次都要 O(n) 走到底。
//   5. unique() 只去除連續重複，未排序時不會全域去重。
//   6. merge() 要求兩邊都已排序，否則結果未定義。
//   7. 記憶體優勢是相對 list 而言；相對 vector，鏈結串列一律大輸。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::forward_list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::forward_list 沒有 size()？
//     答：它的設計目標是「空間開銷不得高於手寫單向鏈結串列」。
//         O(1) 的 size() 需要在容器內多存一個計數欄位（多 8 bytes），
//         O(n) 的 size() 又會讓使用者誤以為它便宜。
//         兩種都違反設計目標，所以標準乾脆不提供。
//         本機實測 sizeof(forward_list<int>) == 8，確實只有一根指標。
//     追問：那要怎麼知道長度？
//         → std::distance(fl.begin(), fl.end())，O(n)。
//           但判斷是否為空一定要用 empty()，那是 O(1)。
//           如果程式常常需要長度，代表該用 std::list。
//
// 🔥 Q2. 為什麼是 insert_after / erase_after，而不是 insert / erase？
//     答：單向鏈結串列從一個節點走不回它的前驅。
//         要在 pos「之前」插入，得先找到 pos 的前驅節點，那是 O(n)。
//         把語意定成「在 pos 之後」就只需要改一根 next 指標，O(1)。
//         這是誠實反映底層能力，而不是提供偷偷 O(n) 的假 O(1) API。
//     追問：那要在最前面插入呢？
//         → before_begin()，它代表「第一個元素之前」的虛擬位置。
//           fl.insert_after(fl.before_begin(), x) 等價於 push_front(x)。
//           注意它不可解參考——*fl.before_begin() 是未定義行為。
//
// ⚠️ 陷阱. erase_after(it) 刪掉的是哪個元素？
//     答：是 it 的「下一個」元素，不是 it 指向的元素。
//         同理 erase_after(first, last) 刪的是開區間 (first, last)，
//         first 和 last 指向的元素都保留。
//     為什麼會錯：所有其他 STL 容器的 erase(pos) 都是「刪 pos 自己」，
//         這個肌肉記憶帶到 forward_list 就會刪錯人，
//         而且因為程式不會崩潰、只是結果錯一格，特別難查。
//         記法：函式名叫 _after，被動的那個就在 after。
//
// ⚠️ 陷阱 2. forward_list 比 vector 省記憶體嗎？
//     答：不。存 10 萬個 int，本檔實測估算 forward_list ≈ 1562 KB，
//         vector ≈ 390 KB——forward_list 是 vector 的四倍。
//     為什麼會錯：「forward_list 是最省記憶體的容器」這句話被斷章取義了。
//         它省的是相對於 list（每節點少一根 prev 指標，24 → 16 bytes）。
//         但只要是鏈結串列，每個元素就得額外背一根 8 bytes 的指標，
//         對 4 bytes 的 int 來說，光指標就比資料本身還大。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【forward_list vs list 對照表】
//   ┌────────────────┬──────────────┬──────────────┐
//   │                │ list         │ forward_list │
//   ├────────────────┼──────────────┼──────────────┤
//   │ 指標           │ prev + next  │ next only    │
//   │ 迭代器方向     │ 雙向         │ 只能前進     │
//   │ size()         │ O(1) ✅      │ ❌ 沒有！    │
//   │ push_back      │ O(1) ✅      │ ❌ 沒有！    │
//   │ push_front     │ O(1) ✅      │ O(1) ✅      │
//   │ back()         │ O(1) ✅      │ ❌ 沒有！    │
//   │ insert         │ insert(pos)  │ insert_after │
//   │ erase          │ erase(pos)   │ erase_after  │
//   │ before_begin() │ ❌           │ ✅ 特有      │
//   │ 每節點記憶體   │ 24 bytes     │ 16 bytes     │
//   └────────────────┴──────────────┴──────────────┘
//   （記憶體數值為本機 x86-64 / libstdc++ 15 實測，屬實作定義）
//
// 【何時用 forward_list？】
//   記憶體敏感 + 只需前向遍歷 + 只需頭端（或已知位置之後）操作
//   實務上很少直接用；最常見的真實案例是雜湊表的 bucket 鏈。
// =============================================================================

#include <iostream>
#include <forward_list>
#include <list>
#include <vector>
#include <string>
#include <iterator>
using namespace std;

template <typename T>
void print(const string& label, const forward_list<T>& fl) {
    cout << "  " << label << ": ";
    for (const auto& v : fl) cout << v << " ";
    cout << "(" << distance(fl.begin(), fl.end()) << ")\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 203. Remove Linked List Elements
//   題目：移除鏈結串列中所有值等於 val 的節點。
//   為什麼用到本主題：這題的官方標準解法是「加一個 dummy head，
//         然後用 prev->next = prev->next->next 刪除」——
//         而 std::forward_list 的 before_begin() 就是被標準化的 dummy head，
//         remove(val) 就是被標準化的整個迴圈。
//   這裡同時給出 STL 一行版與手寫 erase_after 版，方便對照面試白板寫法。
// -----------------------------------------------------------------------------
forward_list<int> removeElementsSTL(forward_list<int> head, int val) {
    head.remove(val);          // 一行解，O(n)
    return head;
}

forward_list<int> removeElementsManual(forward_list<int> head, int val) {
    // before_begin() 扮演 dummy head：讓「刪除第一個節點」不再是特例
    auto prev = head.before_begin();
    auto cur  = head.begin();
    while (cur != head.end()) {
        if (*cur == val) {
            cur = head.erase_after(prev);   // 刪 prev 的下一個（就是 cur）
        } else {                            // erase_after 回傳被刪者的下一個
            prev = cur;
            ++cur;
        }
    }
    return head;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 21. Merge Two Sorted Lists
//   題目：合併兩個已排序的鏈結串列，回傳合併後仍排序的串列。
//   為什麼用到本主題：forward_list::merge 就是這題的答案。
//         它不配置任何新節點，只重接 next 指標——正是題目期待的 O(1) 空間解。
//         而且 merge 是穩定的：兩邊相等時取第一個 list 的元素。
// -----------------------------------------------------------------------------
forward_list<int> mergeTwoLists(forward_list<int> a, forward_list<int> b) {
    a.merge(b);   // 前提：a、b 各自已排序。b 之後會變空
    return a;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】雜湊表的 separate chaining bucket
//   情境：自己實作一個小型雜湊表（快取索引、符號表、路由表都會用到）。
//   每個 bucket 存放雜湊值相同的元素，需求恰好是：
//     * 只需前向走訪（找到就回傳，不需要倒著走）
//     * 只在頭端插入（新元素直接 push_front，O(1)）
//     * 不需要 bucket 的長度
//     * bucket 數量極大（百萬級），每節點省 8 bytes 就是省好幾 MB
//   這正是 forward_list 唯一真正無可取代的場景——
//   libstdc++ 的 std::unordered_map 內部用的就是單向鏈結串列。
// -----------------------------------------------------------------------------
class TinyHashMap {
public:
    explicit TinyHashMap(size_t buckets) : buckets_(buckets) {}

    void put(const string& key, int value) {
        auto& bucket = buckets_[hash_(key) % buckets_.size()];
        for (auto& kv : bucket) {          // 前向走訪找既有 key
            if (kv.first == key) { kv.second = value; return; }
        }
        bucket.emplace_front(key, value);  // 沒找到就插在頭端，O(1)
    }

    bool get(const string& key, int& out) const {
        const auto& bucket = buckets_[hash_(key) % buckets_.size()];
        for (const auto& kv : bucket) {
            if (kv.first == key) { out = kv.second; return true; }
        }
        return false;
    }

    // 印出各 bucket 的鏈長：用 distance（O(n)），因為 forward_list 沒有 size()
    void dumpLoad() const {
        for (size_t i = 0; i < buckets_.size(); ++i) {
            cout << "    bucket[" << i << "] 鏈長 = "
                 << distance(buckets_[i].begin(), buckets_[i].end()) << "\n";
        }
    }

private:
    vector<forward_list<pair<string, int>>> buckets_;
    hash<string> hash_;
};

int main() {
    // 1. 初始化
    cout << "===== 初始化 =====\n";
    forward_list<int> fl1;                         print("空      ", fl1);
    forward_list<int> fl2(5, 42);                  print("5個42   ", fl2);
    forward_list<int> fl3 = {10,20,30,40,50};      print("列表    ", fl3);
    forward_list<int> fl4(fl3);                    print("複製    ", fl4);

    // 2. 沒有 size(), back(), push_back()
    cout << "\n===== 限制：沒有 size/back/push_back =====\n";
    cout << "  front() = " << fl3.front() << "\n";
    // fl3.back();       // ❌ 編譯錯誤
    // fl3.size();       // ❌ 編譯錯誤
    // fl3.push_back(x); // ❌ 編譯錯誤
    cout << "  用 distance 算長度：" << distance(fl3.begin(), fl3.end()) << "\n";

    // 3. push_front / pop_front
    cout << "\n===== push_front / pop_front =====\n";
    forward_list<int> fl;
    fl.push_front(30); fl.push_front(20); fl.push_front(10);
    print("push_front ×3", fl);
    fl.pop_front();
    print("pop_front    ", fl);

    // 4. before_begin（forward_list 特有）
    cout << "\n===== before_begin =====\n";
    {
        forward_list<int> fl = {20,30,40};
        fl.insert_after(fl.before_begin(), 10);
        print("before_begin 插入", fl);  // 10 20 30 40
    }

    // 5. insert_after / erase_after
    cout << "\n===== insert_after / erase_after =====\n";
    {
        forward_list<int> fl = {10,30,50};
        print("初始         ", fl);
        fl.insert_after(fl.begin(), 20);
        print("insert_after ", fl);  // 10 20 30 50
        fl.erase_after(fl.begin());
        print("erase_after  ", fl);  // 10 30 50
    }

    // 6. 特有成員函數
    cout << "\n===== sort / reverse / unique / remove / merge =====\n";
    {
        forward_list<int> fl = {5,3,8,1,9,2};
        fl.sort();     print("sort     ", fl);
        fl.reverse();  print("reverse  ", fl);

        forward_list<int> fl2 = {1,1,2,3,3,4};
        fl2.unique();  print("unique   ", fl2);

        forward_list<int> fl3 = {1,2,3,2,4,2};
        fl3.remove(2); print("remove(2)", fl3);

        forward_list<int> a = {1,3,5}, b = {2,4,6};
        a.merge(b);    print("merge    ", a);
    }

    // 7. 記憶體比較
    cout << "\n===== 記憶體比較 =====\n";
    cout << "  sizeof(forward_list<int>): " << sizeof(forward_list<int>) << " bytes\n";
    cout << "  sizeof(list<int>):         " << sizeof(list<int>) << " bytes\n";
    cout << "  sizeof(vector<int>):       " << sizeof(vector<int>) << " bytes\n";
    const int N = 100000;
    cout << "  " << N << " 個 int 估算：\n";
    cout << "    forward_list: ~" << (N*16)/1024 << " KB（每節點 16B）\n";
    cout << "    list:         ~" << (N*24)/1024 << " KB（每節點 24B）\n";
    cout << "    vector:       ~" << (N*4)/1024  << " KB（連續 4B）\n";

    // 8. 模擬 push_back（維護 tail 迭代器）
    cout << "\n===== 模擬 push_back =====\n";
    {
        forward_list<int> fl;
        auto tail = fl.before_begin();
        for (int i = 1; i <= 5; i++)
            tail = fl.insert_after(tail, i * 10);   // insert_after 回傳新元素的迭代器
        print("模擬push_back", fl);
        cout << "  （自行維護 tail 才能 O(1) 追加；否則每次都要 O(n) 走到底）\n";
    }

    // 9. LeetCode 203：移除所有指定值
    cout << "\n===== LeetCode 203. Remove Linked List Elements =====\n";
    {
        forward_list<int> src = {1, 2, 6, 3, 4, 5, 6};
        print("原始         ", src);
        print("remove(6) STL", removeElementsSTL(src, 6));
        print("手寫 erase_after", removeElementsManual(src, 6));
    }

    // 10. LeetCode 21：合併兩個已排序串列
    cout << "\n===== LeetCode 21. Merge Two Sorted Lists =====\n";
    {
        forward_list<int> a = {1, 2, 4};
        forward_list<int> b = {1, 3, 4};
        print("a            ", a);
        print("b            ", b);
        print("merge 後     ", mergeTwoLists(a, b));
    }

    // 11. 實務：雜湊表的 bucket 鏈
    cout << "\n===== 實務：separate chaining 雜湊表 =====\n";
    {
        TinyHashMap m(4);
        m.put("alice", 30);
        m.put("bob", 25);
        m.put("carol", 41);
        m.put("dave", 19);
        m.put("bob", 26);          // 覆蓋既有 key

        int v = 0;
        cout << "  get(bob)   = " << (m.get("bob", v) ? to_string(v) : "not found") << "\n";
        cout << "  get(eve)   = " << (m.get("eve", v) ? to_string(v) : "not found") << "\n";
        cout << "  各 bucket 負載（鏈長需用 distance，因為沒有 size()）：\n";
        m.dumpLoad();
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 初始化 =====
//   空      : (0)
//   5個42   : 42 42 42 42 42 (5)
//   列表    : 10 20 30 40 50 (5)
//   複製    : 10 20 30 40 50 (5)
//
// ===== 限制：沒有 size/back/push_back =====
//   front() = 10
//   用 distance 算長度：5
//
// ===== push_front / pop_front =====
//   push_front ×3: 10 20 30 (3)
//   pop_front    : 20 30 (2)
//
// ===== before_begin =====
//   before_begin 插入: 10 20 30 40 (4)
//
// ===== insert_after / erase_after =====
//   初始         : 10 30 50 (3)
//   insert_after : 10 20 30 50 (4)
//   erase_after  : 10 30 50 (3)
//
// ===== sort / reverse / unique / remove / merge =====
//   sort     : 1 2 3 5 8 9 (6)
//   reverse  : 9 8 5 3 2 1 (6)
//   unique   : 1 2 3 4 (4)
//   remove(2): 1 3 4 (3)
//   merge    : 1 2 3 4 5 6 (6)
//
// ===== 記憶體比較 =====
//   sizeof(forward_list<int>): 8 bytes
//   sizeof(list<int>):         24 bytes
//   sizeof(vector<int>):       24 bytes
//   100000 個 int 估算：
//     forward_list: ~1562 KB（每節點 16B）
//     list:         ~2343 KB（每節點 24B）
//     vector:       ~390 KB（連續 4B）
//
// ===== 模擬 push_back =====
//   模擬push_back: 10 20 30 40 50 (5)
//   （自行維護 tail 才能 O(1) 追加；否則每次都要 O(n) 走到底）
//
// ===== LeetCode 203. Remove Linked List Elements =====
//   原始         : 1 2 6 3 4 5 6 (7)
//   remove(6) STL: 1 2 3 4 5 (5)
//   手寫 erase_after: 1 2 3 4 5 (5)
//
// ===== LeetCode 21. Merge Two Sorted Lists =====
//   a            : 1 2 4 (3)
//   b            : 1 3 4 (3)
//   merge 後     : 1 1 2 3 4 4 (6)
//
// ===== 實務：separate chaining 雜湊表 =====
//   get(bob)   = 26
//   get(eve)   = not found
//   各 bucket 負載（鏈長需用 distance，因為沒有 size()）：
//     bucket[0] 鏈長 = 1
//     bucket[1] 鏈長 = 1
//     bucket[2] 鏈長 = 1
//     bucket[3] 鏈長 = 1
