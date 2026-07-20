// =============================================================================
//  summary.cpp  —  第 29 課總複習：list 的雙向鏈結串列結構
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<list>
//   標準版本：std::list 為 C++98；size() 的 O(1) 保證自 C++11 起強制；
//             lst.remove_if / lst.unique 的回傳值（刪除個數）為 C++20 新增
//   迭代器類別：BidirectionalIterator
//
//   ┌──────────┬──────────┬──────────┬───────────────┐
//   │          │ vector   │  deque   │  list         │
//   ├──────────┼──────────┼──────────┼───────────────┤
//   │ 隨機存取 │ O(1)     │ O(1)     │ 不支援        │
//   │ 頭端插刪 │ O(n)     │ O(1)     │ O(1)          │
//   │ 尾端插刪 │ 攤銷O(1) │ O(1)     │ O(1)          │
//   │ 中間插刪 │ O(n)     │ O(n)     │ O(1)（持有it）│
//   │ 迭代器   │ 連續     │ 隨機存取 │ 雙向          │
//   │ 記憶體   │ 連續     │ 分段連續 │ 節點散落      │
//   │ 插入失效 │ 可能全滅 │ 迭代器失效│ 完全不失效    │
//   └──────────┴──────────┴──────────┴───────────────┘
//
// 【詳細解釋 Explanation】
//
// 【1. 節點結構：資料只佔一小部分】
//   每個 list 元素被包在獨立配置的節點裡：{ prev 指標, data, next 指標 }。
//   64-bit 平台上兩個指標就是 16 bytes；存一個 int 的節點總大小約 24 bytes
//   （含對齊填補，實作定義）。資料只佔六分之一，其餘全是結構開銷。
//   結論：list 適合「元素本身很大」的情況，不適合存大量 int 這種小型元素。
//
// 【2. O(1) 插刪的完整條件】
//   list 的 insert／erase 只需改動四個指標，與串列長度無關 → O(1)。
//   但這句話必須加上前提：**你已經持有那個位置的迭代器**。
//   如果還得先找到位置，尋找本身是 O(n)，總成本就是 O(n)。
//   真正能發揮 list 優勢的情境，是「迭代器早就在手上」——
//   例如 LRU cache 用 unordered_map 存「key → list 迭代器」，
//   查表 O(1) 拿到迭代器，再 splice O(1) 搬到最前面。
//
// 【3. 為什麼沒有 operator[]：讓昂貴的操作看起來昂貴】
//   lst[3] 技術上做得到（走三步），但那是 O(n)。若提供了這個介面，
//   使用者會寫出 for (i...) lst[i] 這種「看起來 O(n)、實際 O(n²)」的迴圈。
//   標準庫刻意不提供，強迫你寫 std::advance(it, 3)——函式名稱本身就在
//   提醒你「這要一步步走過去」。這是很值得學的 API 設計原則。
//
// 【4. 迭代器穩定性：list 最被低估的價值】
//   list 的 insert 不使任何既有迭代器、指標、參考失效；
//   erase 只使「被刪除的那一個」失效。
//   vector 一擴容就全部失效，deque 兩端插入時迭代器失效（但 pointer/reference 存活）。
//   三者規則各不相同，必須分開記，不可把 vector 的規則套用到全部容器。
//   這個穩定性讓「把迭代器存起來長期使用」成為可能，是 LRU cache 的基礎。
//
// 【5. 為什麼 list 有自己的 sort()】
//   std::sort 需要 RandomAccessIterator（要做 partition 與跳躍取樣），
//   list 只有 BidirectionalIterator，不滿足需求 → 編譯錯誤。
//   list 因此自備 lst.sort()，用歸併排序實作：只需循序走訪與指標接合。
//   複雜度 O(n log n)、穩定排序，而且**只重接指標不搬移資料**，
//   所以排序後元素位址不變、既有迭代器仍指向原來那個元素（只是順序變了）。
//
// 【概念補充 Concept Deep Dive】
//   ● 哨兵節點（sentinel node）：libstdc++ 的 list 是環狀雙向串列，
//     含一個不存資料的哨兵，end() 指向它。好處是 insert／erase 完全不必為
//     「空串列」「插在頭」「插在尾」寫特例分支——所有情況共用同一段程式碼。
//     sizeof(std::list<int>) 在 libstdc++ 為 24 bytes（實作定義）。
//   ● C++11 起 size() 必須是 O(1)，實作因此要多維護一個計數器。
//     C++98／03 時代 libstdc++ 的 size() 是 O(n)，這個舊知識現在已經過時。
//   ● pointer chasing：節點各自 new 出來，位址由配置器決定，彼此可能相距甚遠。
//     CPU 硬體預取器無從預測下一個節點在哪，遍歷時幾乎每步都是 cache miss。
//     這是現代 CPU 上最慢的存取模式之一，也是 list 遍歷慢的根本原因。
//   ● 為什麼 list 在實務上常常輸給 vector：即使複雜度較好，
//     每次插入都要一次動態配置，加上遍歷全是 cache miss。
//     元素數量少於數千個時，vector 的 O(n) 搬移常常實際更快。
//     這是「複雜度不等於效能」最經典的案例。
//
// 【注意事項 Pay Attention】
//   1. 本檔會印記憶體位址，**每次執行都不同**（配置器行為與 ASLR）。
//      重點在「vector 等距、list 不規則」的現象，不在具體數值。
//   2. 「list 插刪 O(1)」的前提是已持有迭代器；含尋找則為 O(n)。
//   3. list 不支援 operator[]、at()，也不能用 std::sort —— 皆為編譯期錯誤。
//   4. erase 之後原迭代器失效，再解參考是 UB。請接住 erase 的回傳值。
//   5. sizeof(std::list<int>)、節點大小、位址間距皆為實作定義。
//   6. 空 list 仍會配置哨兵節點，不是零成本。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list 總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list、vector、deque 的迭代器失效規則各是什麼？
//     答：vector —— 擴容時 iterator／pointer／reference 全部失效；
//         未擴容的中間插刪則插入點之後全部失效。
//         deque —— 兩端插入時 iterator 失效，但指向既有元素的 pointer 與
//         reference 仍有效（元素沒被搬動）；中間插刪則三者皆失效。
//         list —— insert 完全不失效；erase 只使被刪除的那一個失效。
//     追問：為什麼 deque 兩端插入 pointer 還活著？→ 因為它只是新增一塊 chunk
//         並更新 map，既有元素完全沒有搬家。
//
// 🔥 Q2. 什麼情境下 list 真的比 vector 好？
//     答：(1) 需要保證迭代器長期有效（例如 LRU cache 把迭代器存進 hash map）。
//         (2) 需要 splice——把一整段節點 O(1) 搬到另一個 list，不複製元素。
//         (3) 元素本身很大且不可移動，搬移成本高昂。
//         若只是「中間常插刪」但元素是小型 POD 且數量不多，vector 通常仍勝。
//     追問：LRU cache 為什麼是 list + unordered_map？→ map 查 key 得到迭代器 O(1)，
//         list 用 splice 把該節點搬到最前面 O(1)，兩者都不失效，整體 O(1)。
//
// ⚠️ 陷阱. 「list 中間插入是 O(1)，所以在 list 中間插入一定比 vector 快。」
//     答：不一定。O(1) 的前提是迭代器已在手上。若要先找到位置，那是 O(n) 遍歷，
//         而且是最慢的 pointer chasing 遍歷。加上每次插入要一次動態配置，
//         實測上元素不多時 vector 的 memmove 往往更快。
//     為什麼會錯：只比較漸進複雜度，忽略常數、記憶體配置成本與 cache 行為。
//
// ⚠️ 陷阱. 「list 排序後，我之前存的迭代器就指到別的元素了。」
//     答：不會。lst.sort() 只重新接指標，不搬移元素資料。
//         既有迭代器仍然指向**原本那個元素**，只是它在串列中的位置變了。
//     為什麼會錯：用 vector 的直覺思考——vector 排序是真的把值搬來搬去，
//         所以同一個位置換了值。list 是搬節點，不是搬值。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;

// -----------------------------------------------------------------------------
// 【日常實務範例】LRU 快取（list + unordered_map 的教科書組合）
//   情境：API 回應快取只能放 N 筆，滿了要淘汰「最久沒被存取」的那一筆。
//   為什麼非 list 不可：
//     1. 每次命中都要把該筆搬到最前面 —— list 的 splice 是 O(1)，不搬資料。
//     2. hash map 裡存的是 list 迭代器；list 的插刪不使其他迭代器失效，
//        所以這些存起來的迭代器可以一直安全使用。
//        換成 vector，任何一次擴容就會讓 map 裡全部迭代器變成懸空。
//   這正是 list 在真實系統中最站得住腳的用途。
// -----------------------------------------------------------------------------
class LruCache {
public:
    explicit LruCache(size_t capacity) : cap_(capacity) {}

    // 命中回傳 true 並把該筆移到最前面
    bool get(const string& key, string& value) {
        auto found = index_.find(key);
        if (found == index_.end()) return false;
        // splice：把節點搬到最前面，O(1)，迭代器仍然有效
        order_.splice(order_.begin(), order_, found->second);
        value = found->second->second;
        return true;
    }

    void put(const string& key, const string& value) {
        auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = value;
            order_.splice(order_.begin(), order_, found->second);
            return;
        }
        if (order_.size() >= cap_) {
            // 淘汰最久未用者（串列尾端）
            const string& victim = order_.back().first;
            cout << "    [淘汰] " << victim << "\n";
            index_.erase(victim);
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        index_[key] = order_.begin();
    }

    void dump() const {
        cout << "    快取順序(新→舊): ";
        for (const auto& kv : order_) cout << kv.first << " ";
        cout << "\n";
    }

private:
    size_t cap_;
    list<pair<string, string>> order_;                                  // 最近用的在前
    unordered_map<string, list<pair<string, string>>::iterator> index_; // key → 節點迭代器
};

int main() {
    list<int> lst = {10, 20, 30, 40, 50};

    // 1. 遍歷方式
    cout << "===== 遍歷 =====\n";
    cout << "  範圍 for：";
    for (int val : lst) cout << val << " ";
    cout << "\n  反向迭代：";
    for (auto rit = lst.rbegin(); rit != lst.rend(); ++rit)
        cout << *rit << " ";
    cout << endl;

    // 2. 不支援隨機存取 → 用 advance（注意：這是 O(n) 不是 O(1)）
    cout << "\n===== 迭代器操作 =====\n";
    auto it = lst.begin();
    ++it;  cout << "  ++it → " << *it << "\n";     // 20
    --it;  cout << "  --it → " << *it << "\n";     // 10
    // it + 3;  // ❌ 編譯錯誤！雙向迭代器不支援 + 運算
    advance(it, 3);
    cout << "  advance(it, 3) → " << *it << "\n";  // 40

    // 3. 記憶體地址不連續（位址每次執行都不同）
    cout << "\n===== 記憶體地址（list：不連續）=====\n";
    for (auto& val : lst)
        cout << "  值:" << val << "  地址:" << &val << "\n";

    cout << "\n===== vector 地址（連續）=====\n";
    vector<int> vec = {10, 20, 30, 40, 50};
    for (auto& val : vec)
        cout << "  值:" << val << "  地址:" << &val << "\n";
    cout << "  vector 相鄰元素距離(以 int 計) = " << (&vec[1] - &vec[0]) << "\n";

    // 4. O(1) 插入（前提：已持有迭代器）
    cout << "\n===== O(1) 插入 =====\n";
    auto pos = lst.begin();
    while (*pos != 30) ++pos;      // ← 這個尋找是 O(n)
    lst.insert(pos, 25);           // ← 這一步才是 O(1)
    cout << "  在 30 前插入 25：";
    for (int v : lst) cout << v << " ";
    cout << "\n  pos 仍指向：" << *pos << "（list 的插入不使任何迭代器失效）\n";

    // 5. O(1) 刪除
    cout << "\n===== O(1) 刪除 =====\n";
    auto next_pos = lst.erase(pos);   // pos 自此失效，不可再解參考
    cout << "  刪除 30 後：";
    for (int v : lst) cout << v << " ";
    cout << "\n  erase 回傳下一個：" << *next_pos << "\n";

    // 6. front / back
    cout << "\n===== front / back =====\n";
    cout << "  front=" << lst.front() << " back=" << lst.back() << "\n";

    // 7. list 專屬 sort：不能用 std::sort
    cout << "\n===== list 成員 sort（歸併排序，穩定）=====\n";
    list<int> unsorted = {5, 2, 9, 1, 7};
    // sort(unsorted.begin(), unsorted.end());  // ❌ 編譯錯誤：需要隨機存取迭代器
    auto watch = unsorted.begin();               // 指向 5
    unsorted.sort();                             // ✅ 成員函式版本
    cout << "  排序後：";
    for (int v : unsorted) cout << v << " ";
    cout << "\n  排序前存下的迭代器仍指向原元素：" << *watch << "（值不變，位置變了）\n";

    // 8. 日常實務：LRU 快取
    cout << "\n===== 日常實務：LRU 快取（容量 3）=====\n";
    LruCache cache(3);
    cache.put("/api/users", "200 OK");
    cache.put("/api/orders", "200 OK");
    cache.put("/api/items", "200 OK");
    cache.dump();

    string val;
    cout << "  存取 /api/users → " << (cache.get("/api/users", val) ? "命中" : "未命中") << "\n";
    cache.dump();

    cout << "  新增 /api/stats（容量已滿，將淘汰最久未用者）\n";
    cache.put("/api/stats", "200 OK");
    cache.dump();

    cout << "  存取 /api/orders → " << (cache.get("/api/orders", val) ? "命中" : "未命中") << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 注意：第 3 段印出的記憶體位址每次執行都不同（配置器行為 + ASLR）。
//       重點在「vector 位址等距、list 位址不規則」的對比，不在具體數值。

// === 預期輸出 ===
// ===== 遍歷 =====
//   範圍 for：10 20 30 40 50
//   反向迭代：50 40 30 20 10
//
// ===== 迭代器操作 =====
//   ++it → 20
//   --it → 10
//   advance(it, 3) → 40
//
// ===== 記憶體地址（list：不連續）=====
//   值:10  地址:0x5cafb89bf030
//   值:20  地址:0x5cafb89bf350
//   值:30  地址:0x5cafb89bf370
//   值:40  地址:0x5cafb89bf390
//   值:50  地址:0x5cafb89bf3b0
//
// ===== vector 地址（連續）=====
//   值:10  地址:0x5cafb89bf3c0
//   值:20  地址:0x5cafb89bf3c4
//   值:30  地址:0x5cafb89bf3c8
//   值:40  地址:0x5cafb89bf3cc
//   值:50  地址:0x5cafb89bf3d0
//   vector 相鄰元素距離(以 int 計) = 1
//
// ===== O(1) 插入 =====
//   在 30 前插入 25：10 20 25 30 40 50
//   pos 仍指向：30（list 的插入不使任何迭代器失效）
//
// ===== O(1) 刪除 =====
//   刪除 30 後：10 20 25 40 50
//   erase 回傳下一個：40
//
// ===== front / back =====
//   front=10 back=50
//
// ===== list 成員 sort（歸併排序，穩定）=====
//   排序後：1 2 5 7 9
//   排序前存下的迭代器仍指向原元素：5（值不變，位置變了）
//
// ===== 日常實務：LRU 快取（容量 3）=====
//     快取順序(新→舊): /api/items /api/orders /api/users
//   存取 /api/users → 命中
//     快取順序(新→舊): /api/users /api/items /api/orders
//   新增 /api/stats（容量已滿，將淘汰最久未用者）
//     [淘汰] /api/orders
//     快取順序(新→舊): /api/stats /api/users /api/items
//   存取 /api/orders → 未命中
