// =============================================================================
//  第 37 課 總結  —  list 迭代器失效規則：節點式容器的核心優勢
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   關鍵成員：
//     iterator insert(const_iterator pos, const T& v);      // 不使任何迭代器失效
//     iterator erase (const_iterator pos);                  // 只有被刪的失效
//     void     splice(const_iterator pos, list& other, const_iterator it);  // O(1)
//     void     sort();                                      // 歸併排序，迭代器全有效
//     void     remove(const T& v) / remove_if(pred);
//   標準版本：list 自 C++98；C++11 起 insert/erase 參數改為 const_iterator；
//             C++20 起 remove / remove_if / unique 回傳被移除的元素個數（原為 void）。
//   複雜度：insert / erase（已有迭代器）O(1)；splice 單一元素 O(1)；
//           sort O(N log N)；remove / unique / reverse O(N)。
//   關鍵保證：**除了被刪除的那個節點，list 的任何操作都不會使既有迭代器失效。**
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 list 的迭代器這麼穩定：節點永遠不搬家】
//   list 的每個元素各自被 new 出來，成為一個獨立節點：
//       struct _List_node { _List_node* prev; _List_node* next; T data; };
//   而 list::iterator 內部只存一個節點指標。
//   插入 = 配置新節點 + 改四個指標；刪除 = 改兩個指標 + 釋放該節點。
//   **既有節點的位址從頭到尾都沒有變過** —— 所以指向它們的迭代器當然一直有效。
//
//   對比 vector：元素連續存放，插入/刪除要 memmove 整段，
//   push_back 超過容量還要重新配置整塊記憶體 + 搬移全部元素，
//   於是迭代器（本質是指標）指向的位址整個作廢。
//   這不是「哪個實作比較用心」，而是資料結構本身的必然結果。
//
// 【2. sort 之後迭代器仍然有效，這件事比想像中特別】
//   std::sort 對 vector 做的是「交換元素的值」——
//   位置 0 的迭代器排序後仍指向位置 0，但那裡的**值換人了**。
//   list::sort 則是「重新串接節點指標」——
//   每個節點的位址不變、值也不變，只是 prev/next 被重接。
//   所以：
//       auto it = std::next(lst.begin());   // 指向某個節點
//       lst.sort();
//       *it                                 // 仍然是「同一個元素」，值沒變
//   這是 std::sort 給不了的保證，也是 list::sort 存在的理由之一
//   （另一個理由是 std::sort 需要 Random Access，list 根本編不過）。
//
// 【3. splice 是 list 的殺手鐧：O(1) 的元素轉移】
//   splice 把節點從一個 list「摘下來」接到另一個 list，
//   全程不配置、不釋放、不複製、不移動任何元素 —— 只改指標，所以是 O(1)。
//   而且**被搬移的迭代器依然有效**，只是它現在屬於新的 list 了。
//   任何其他容器都做不到這件事（vector 的等價操作是 O(N) 的複製或移動）。
//   這正是 LRU cache 的實作基礎：把「剛被存取的節點」搬到串列頭端，
//   在 list 上是 O(1)，用 vector 則是 O(N)。
//
// 【4. 安全刪除模式】
//     ❌ for (auto it = l.begin(); it != l.end(); ++it) l.erase(it);
//        erase 之後 it 已失效，++it 是未定義行為
//     ✅ for (auto it = l.begin(); it != l.end(); )
//            if (cond) it = l.erase(it); else ++it;
//     ✅ l.remove(val) / l.remove_if(pred);      // 更簡潔，且只失效被刪的
//     ✅ std::erase(l, val) / std::erase_if(l, pred);   // C++20
//   注意：erase-remove 慣用法（std::remove + erase）**不該用在 list 上** ——
//   它會逐一賦值搬移元素內容，比成員版慢，還會破壞其他迭代器指向的語意。
//
// 【概念補充 Concept Deep Dive】
//   「迭代器有效」與「參考／指標有效」是兩個獨立的保證，別混為一談。
//   最能說明差異的是 std::deque：
//     - 在 deque 兩端插入 → **迭代器全部失效**，但**參考與指標仍然有效**
//       （因為 deque 的元素不會被搬動，只是迭代器內部的區塊索引表可能被重建）
//     - 在 deque 中間插入 → 迭代器與參考都失效
//   而 vector 的重新配置會讓兩者同時失效（元素真的被搬到新位址）；
//   list 則是兩者都保持有效（除了被刪的節點）。
//   實務意義：如果你只是要長期持有「某個元素的參考」而不需要走訪，
//   deque 兩端操作其實是安全的 —— 但這個細節極少人知道，
//   寫進註解才不會被後人「順手改掉」。
//
// 【注意事項 Pay Attention】
//   1. erase 只讓「被刪的那一個」失效；其餘全部有效 —— 但被刪的那個絕不能再用。
//   2. clear() 會讓**全部**迭代器失效（所有節點都被釋放）。
//   3. splice 之後被搬移的迭代器仍有效，但它已經屬於**目的地 list**；
//      再拿它去對來源 list 做 erase 是未定義行為。
//   4. sort / reverse 之後迭代器有效，但它在串列中的**位置**已經改變 ——
//      「it 仍指向 30」不等於「it 仍是第二個元素」。
//   5. list 沒有 operator[]，也不能用 std::sort（需要 Random Access）。
//   6. 不要對 list 用 erase-remove 慣用法，請用成員函式 remove / remove_if。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】list 迭代器失效規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 list 的插入操作完全不會使迭代器失效，而 vector 會？
//     答：因為 list 的每個元素是獨立配置的節點，迭代器內部存的是節點指標。
//         插入只是配置一個新節點並改動四個指標，
//         **既有節點的位址從頭到尾沒有變過**，所以指向它們的迭代器仍然有效。
//         vector 的元素連續存放，push_back 超過容量就要重新配置整塊記憶體
//         並搬移全部元素 —— 舊位址整個作廢，迭代器自然全部失效。
//     追問：那 list::sort 之後迭代器還有效嗎？
//           → 有效。list::sort 是重接 prev/next 指標，節點位址與值都沒變。
//             但要注意它在串列中的**位置**變了 ——
//             「仍指向 30」不代表「仍是第二個元素」。
//
// 🔥 Q2. splice 為什麼是 O(1)？它對迭代器有什麼影響？
//     答：splice 只是把節點從來源串列摘下、接到目的串列，
//         全程不配置、不釋放、不複製、不移動任何元素，只改指標，所以 O(1)。
//         被搬移的迭代器**仍然有效**，但它現在屬於目的地 list。
//         這是任何其他標準容器都做不到的操作。
//     追問：這個特性在什麼實際設計裡是決定性的？
//           → LRU cache。每次存取要把該節點移到串列頭端，
//             list::splice 是 O(1)；改用 vector 則是 O(N) 的搬移。
//             LeetCode 146 要求 get/put 都是 O(1)，只有 list + hash map 做得到。
//
// ⚠️ 陷阱. 「list::erase 只讓被刪的迭代器失效，所以下面這段是安全的」——錯在哪？
//          for (auto it = l.begin(); it != l.end(); ++it)
//              if (*it % 2 == 0) l.erase(it);
//     答：正好相反 —— 這段一定是未定義行為。
//         「只有被刪的失效」的意思是**其他**迭代器安全，
//         但 it 自己就是那個被刪掉的，erase 之後它已經指向被釋放的節點。
//         接著 ++it 要讀取那個已釋放節點的 next 指標 → UB。
//         正解是 it = l.erase(it)（用回傳值接手），或直接用 l.remove_if(pred)。
//     為什麼會錯：把「list 的迭代器很穩定」過度延伸成「怎麼寫都安全」。
//         穩定性保護的是**旁觀者**，不是被刪除的那一個本身。
//         而且這個 UB 很可能不會當場崩潰（已釋放的節點記憶體通常還沒被覆寫），
//         測試很容易矇混過關 —— 要抓它請用 -D_GLIBCXX_DEBUG 或 ASan。
// ═══════════════════════════════════════════════════════════════════════════

// ============================================================
// 第 37 課 總結：list 迭代器失效規則
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【list 迭代器失效規則（極簡版）】
//   insert / push  → 所有迭代器有效 ✅
//   erase          → 只有被刪元素的迭代器失效 ❌ 其餘有效 ✅
//   splice         → 所有迭代器有效 ✅（被移植的也有效）
//   sort           → 所有迭代器有效 ✅
//   remove/unique  → 被刪的失效 ❌ 其餘有效 ✅
//   reverse        → 所有迭代器有效 ✅
//   merge          → 所有迭代器有效 ✅
//   clear          → 全部失效 ❌
//
// 【vs vector 的巨大差異】
//   vector insert/erase → 插入/刪除點之後的所有迭代器失效
//   list insert/erase   → 只有被刪的失效，其餘全部安全
//   → 這就是 list 最大的優勢之一：迭代器穩定性
//
// 【安全刪除模式】
//   ❌ for (...; ++it) { lst.erase(it); }      → it 已失效，++it UB
//   ✅ for (...; ) { it = lst.erase(it); }      → erase 回傳下一個
//   ✅ lst.remove(val) / lst.remove_if(pred)    → 更簡潔
// ============================================================

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <unordered_map>
#include <iterator>
using namespace std;

template <typename T>
void print(const string& label, const list<T>& lst) {
    cout << "  " << label << " [" << lst.size() << "]: ";
    for (const auto& v : lst) cout << v << " ";
    cout << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的 LRU（Least Recently Used）快取，
//         要求 get 與 put **都是 O(1)**。容量滿時淘汰最久未使用的項目。
//   為什麼用到本主題：這題是 list 迭代器穩定性的**決定性應用**，
//         也是本課全部規則的集大成：
//           (1) 用 list 維護「使用順序」，最前面是最近使用、最後面是最久未用
//           (2) 用 unordered_map<key, list::iterator> 讓 O(1) 找到節點
//           (3) 每次存取要把節點移到最前面 → list::splice，**O(1)**
//           (4) 關鍵：map 裡存的迭代器在 splice/insert 之後**依然有效**，
//               所以不必更新 map —— 這正是 list 迭代器穩定性的價值
//         若改用 vector，(3) 會變成 O(N) 的搬移，而且 (4) 的迭代器會全部失效，
//         整個設計立刻崩潰。這就是為什麼 LRU 幾乎一定用 list + hash map。
//   複雜度：get / put 皆為 O(1)；空間 O(capacity)。
// -----------------------------------------------------------------------------
class LRUCache {
    int capacity_;
    // 串列前端 = 最近使用；後端 = 最久未使用
    list<pair<int, int>> order_;                                  // (key, value)
    unordered_map<int, list<pair<int, int>>::iterator> index_;    // key → 節點迭代器

public:
    explicit LRUCache(int capacity) : capacity_(capacity) {}

    int get(int key) {
        auto found = index_.find(key);
        if (found == index_.end()) return -1;

        // 命中：把該節點搬到最前面。splice 是 O(1)，
        // 而且 found->second 這個迭代器搬移後**仍然有效**，不必更新 index_
        order_.splice(order_.begin(), order_, found->second);
        return found->second->second;
    }

    void put(int key, int value) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = value;                    // 更新值
            order_.splice(order_.begin(), order_, found->second);  // 移到最前
            return;
        }

        if (static_cast<int>(order_.size()) >= capacity_) {
            // 淘汰最久未使用（串列最後一個）
            int old_key = order_.back().first;
            index_.erase(old_key);
            order_.pop_back();
        }

        order_.emplace_front(key, value);
        index_[key] = order_.begin();                         // 存下迭代器
    }

    // 教學用：印出目前的使用順序（最近 → 最久）
    string dump() const {
        string s;
        for (const auto& kv : order_) {
            s += "(" + to_string(kv.first) + "," + to_string(kv.second) + ") ";
        }
        return s.empty() ? "(空)" : s;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】編輯器的復原堆疊 + 書籤：長期持有元素位置
//   情境：文字編輯器要同時維護
//         (1) 文件的段落串列（會頻繁在中間插入/刪除）
//         (2) 使用者設下的書籤（必須一直指向「同一個段落」，
//             即使前面插入或刪除了其他段落）
//   為什麼用到本主題：書籤本質就是「長期持有的迭代器」。
//         用 vector 的話，任何一次中間插入都可能讓所有書籤失效或錯位；
//         用 list 則除了「書籤指向的段落本身被刪除」之外，
//         書籤永遠正確指向同一個段落 —— 這是資料結構層級的保證，
//         不需要任何額外的同步程式碼。
// -----------------------------------------------------------------------------
class Document {
    list<string> paragraphs_;
public:
    using Bookmark = list<string>::iterator;

    Bookmark append(const string& text) {
        paragraphs_.push_back(text);
        return prev(paragraphs_.end());
    }

    // 在某個書籤之前插入新段落 —— 不影響任何既有書籤
    void insertBefore(Bookmark bm, const string& text) {
        paragraphs_.insert(bm, text);
    }

    // 刪除某段落。回傳下一段的迭代器（呼叫端須丟棄被刪的那個書籤）
    Bookmark remove(Bookmark bm) { return paragraphs_.erase(bm); }

    size_t size() const { return paragraphs_.size(); }

    void dump(const string& label) const {
        cout << "  " << label << " [" << paragraphs_.size() << "]: ";
        for (const auto& p : paragraphs_) cout << p << " ";
        cout << endl;
    }
};

int main() {
    // 1. insert 後全部有效
    cout << "===== insert 後迭代器有效 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it10 = lst.begin();
        auto it30 = next(it10, 2);
        auto it50 = next(it10, 4);
        lst.insert(it30, 25);
        lst.push_back(60);
        lst.push_front(5);
        cout << "  *it10=" << *it10 << " *it30=" << *it30 << " *it50=" << *it50 << " ✅\n";
        print("結果", lst);
    }

    // 2. erase 只影響被刪的
    cout << "\n===== erase 只影響被刪的 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it10=lst.begin(), it20=next(it10), it30=next(it20);
        auto it40=next(it30), it50=next(it40);
        lst.erase(it30);  // it30 失效
        cout << "  刪30後：*it10=" << *it10 << " *it20=" << *it20
             << " *it40=" << *it40 << " *it50=" << *it50 << " ✅\n";

        // 對比 vector
        cout << "  （vector erase 後，刪除點之後的迭代器全部失效！）\n";
    }

    // 3. splice 後全部有效
    cout << "\n===== splice 後迭代器有效 =====\n";
    {
        list<int> A = {1,2,3}, B = {10,20,30};
        auto it1 = A.begin();
        auto it20 = next(B.begin());
        A.splice(A.end(), B, it20);
        cout << "  *it1=" << *it1 << " *it20=" << *it20 << " ✅\n";
        cout << "  （it20 仍有效，但現在屬於 A）\n";
    }

    // 4. sort 後全部有效
    cout << "\n===== sort 後迭代器有效 =====\n";
    {
        list<int> lst = {50,30,10,40,20};
        auto it30 = next(lst.begin());
        cout << "  sort 前 *it=" << *it30 << " addr=" << &(*it30) << "\n";
        lst.sort();
        cout << "  sort 後 *it=" << *it30 << " addr=" << &(*it30) << " ✅\n";
        cout << "  （值和地址不變，在 list 中位置改變）\n";
    }

    // 5. remove 只使被刪的失效
    cout << "\n===== remove 迭代器影響 =====\n";
    {
        list<int> lst = {1,2,3,2,4,2,5};
        auto it1=lst.begin(), it3=next(it1,2), it4=next(it3,2), it5=next(it4,2);
        lst.remove(2);
        cout << "  remove(2)後：" << *it1 << " " << *it3 << " " << *it4 << " " << *it5 << " ✅\n";
    }

    // 6. reverse 後全部有效
    cout << "\n===== reverse 後迭代器有效 =====\n";
    {
        list<int> lst = {10,20,30,40,50};
        auto it_begin = lst.begin();
        lst.reverse();
        cout << "  *it_begin=" << *it_begin << "（仍是10，但現在是最後一個）\n";
        cout << "  it_begin == prev(end)? " << (it_begin==prev(lst.end())?"是":"否") << " ✅\n";
    }

    // 7. 正確 vs 錯誤的刪除模式
    cout << "\n===== 安全刪除模式 =====\n";
    {
        list<int> lst = {1,2,3,4,5,6,7,8};
        // ✅ 正確
        for (auto it = lst.begin(); it != lst.end(); ) {
            if (*it % 2 == 0) it = lst.erase(it);
            else ++it;
        }
        print("刪偶數（正確）", lst);
    }
    cout << "  ❌ 錯誤：erase(it); ++it; → it 已失效\n";
    cout << "  ✅ 正確：it = erase(it);  → erase 回傳下一個\n";

    // 8. 書籤系統
    cout << "\n===== 實戰：迭代器書籤 =====\n";
    {
        list<string> doc = {"第一章","第二章","第三章","第四章","第五章"};
        auto bm1 = next(doc.begin());    // 第二章
        auto bm3 = next(doc.begin(), 2); // 第三章
        doc.insert(bm3, "新增章節");
        doc.erase(next(doc.begin(), 4)); // 刪第四章
        doc.sort();
        cout << "  經過 insert+erase+sort 後：\n";
        cout << "  書籤1=" << *bm1 << " 書籤3=" << *bm3 << " ✅ 仍有效\n";
    }

    // 9. LeetCode 146. LRU Cache
    cout << "\n===== LeetCode 146. LRU Cache =====\n";
    {
        LRUCache cache(2);
        cout << "  容量 2 的 LRU Cache（左邊=最近使用）\n";

        cache.put(1, 1);  cout << "  put(1,1)  → " << cache.dump() << "\n";
        cache.put(2, 2);  cout << "  put(2,2)  → " << cache.dump() << "\n";
        cout << "  get(1)    = " << cache.get(1) << "   → " << cache.dump() << "\n";
        cache.put(3, 3);  cout << "  put(3,3)  → " << cache.dump()
                               << " （key 2 被淘汰）\n";
        cout << "  get(2)    = " << cache.get(2) << "  （已被淘汰，回傳 -1）\n";
        cache.put(4, 4);  cout << "  put(4,4)  → " << cache.dump()
                               << " （key 1 被淘汰）\n";
        cout << "  get(1)    = " << cache.get(1) << "  （已被淘汰）\n";
        cout << "  get(3)    = " << cache.get(3) << "   → " << cache.dump() << "\n";
        cout << "  get(4)    = " << cache.get(4) << "   → " << cache.dump() << "\n";
        cout << "  → 全程靠 list::splice 的 O(1) 搬移 +\n";
        cout << "    「splice 後迭代器仍有效」讓 hash map 不必更新\n";
    }

    // 10. 實務：編輯器書籤
    cout << "\n===== 日常實務：編輯器書籤 =====\n";
    {
        Document doc;
        doc.append("第一章 導論");
        Document::Bookmark bm_ch2 = doc.append("第二章 迭代器");
        doc.append("第三章 演算法");
        Document::Bookmark bm_ch4 = doc.append("第四章 容器");
        doc.dump("初始");
        cout << "  書籤A → " << *bm_ch2 << "\n";
        cout << "  書籤B → " << *bm_ch4 << "\n";

        // 在書籤 A 之前插入一段：任何書籤都不受影響
        doc.insertBefore(bm_ch2, "序言");
        doc.dump("插入序言後");
        cout << "  書籤A → " << *bm_ch2 << "  ✅ 仍指向同一段\n";
        cout << "  書籤B → " << *bm_ch4 << "  ✅ 仍指向同一段\n";

        // 刪除一個「不是書籤所指」的段落：書籤照樣安全
        Document::Bookmark tmp = doc.append("附錄（待刪）");
        doc.remove(tmp);
        doc.dump("刪除附錄後");
        cout << "  書籤A → " << *bm_ch2 << "  ✅\n";
        cout << "  書籤B → " << *bm_ch4 << "  ✅\n";
        cout << "  → 若改用 vector，上面每一次插入都可能讓兩個書籤失效或錯位\n";
    }

    return 0;
}

// 注意（非決定性輸出）：
//   「sort 前/後 addr=0x...」印的是節點的真實記憶體位址，
//   由配置器決定，**每次執行都不同**，下方預期輸出中的那兩個數值僅供參考。
//   真正要看的是：sort 前後兩個位址**完全相同** ——
//   這就是「list::sort 只重接指標、不搬移節點」的證據。

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== insert 後迭代器有效 =====
//   *it10=10 *it30=30 *it50=50 ✅
//   結果 [8]: 5 10 20 25 30 40 50 60
//
// ===== erase 只影響被刪的 =====
//   刪30後：*it10=10 *it20=20 *it40=40 *it50=50 ✅
//   （vector erase 後，刪除點之後的迭代器全部失效！）
//
// ===== splice 後迭代器有效 =====
//   *it1=1 *it20=20 ✅
//   （it20 仍有效，但現在屬於 A）
//
// ===== sort 後迭代器有效 =====
//   sort 前 *it=30 addr=0x603db43ed3b0
//   sort 後 *it=30 addr=0x603db43ed3b0 ✅
//   （值和地址不變，在 list 中位置改變）
//
// ===== remove 迭代器影響 =====
//   remove(2)後：1 3 4 5 ✅
//
// ===== reverse 後迭代器有效 =====
//   *it_begin=10（仍是10，但現在是最後一個）
//   it_begin == prev(end)? 是 ✅
//
// ===== 安全刪除模式 =====
//   刪偶數（正確） [4]: 1 3 5 7
//   ❌ 錯誤：erase(it); ++it; → it 已失效
//   ✅ 正確：it = erase(it);  → erase 回傳下一個
//
// ===== 實戰：迭代器書籤 =====
//   經過 insert+erase+sort 後：
//   書籤1=第二章 書籤3=第三章 ✅ 仍有效
//
// ===== LeetCode 146. LRU Cache =====
//   容量 2 的 LRU Cache（左邊=最近使用）
//   put(1,1)  → (1,1)
//   put(2,2)  → (2,2) (1,1)
//   get(1)    = 1   → (1,1) (2,2)
//   put(3,3)  → (3,3) (1,1)  （key 2 被淘汰）
//   get(2)    = -1  （已被淘汰，回傳 -1）
//   put(4,4)  → (4,4) (3,3)  （key 1 被淘汰）
//   get(1)    = -1  （已被淘汰）
//   get(3)    = 3   → (3,3) (4,4)
//   get(4)    = 4   → (4,4) (3,3)
//   → 全程靠 list::splice 的 O(1) 搬移 +
//     「splice 後迭代器仍有效」讓 hash map 不必更新
//
// ===== 日常實務：編輯器書籤 =====
//   初始 [4]: 第一章 導論 第二章 迭代器 第三章 演算法 第四章 容器
//   書籤A → 第二章 迭代器
//   書籤B → 第四章 容器
//   插入序言後 [5]: 第一章 導論 序言 第二章 迭代器 第三章 演算法 第四章 容器
//   書籤A → 第二章 迭代器  ✅ 仍指向同一段
//   書籤B → 第四章 容器  ✅ 仍指向同一段
//   刪除附錄後 [5]: 第一章 導論 序言 第二章 迭代器 第三章 演算法 第四章 容器
//   書籤A → 第二章 迭代器  ✅
//   書籤B → 第四章 容器  ✅
//   → 若改用 vector，上面每一次插入都可能讓兩個書籤失效或錯位
