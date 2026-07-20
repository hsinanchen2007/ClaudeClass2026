// =============================================================================
//  第五課：迭代器的五種分類 6  —  Bidirectional Iterator（雙向迭代器）與 reverse_iterator
// =============================================================================
//
// 【主題資訊 Information】
//   類別標籤：std::bidirectional_iterator_tag
//   代表容器：std::list、std::set / multiset、std::map / multimap
//   支援操作：Forward 的全部，再加上 --it / it--
//   不支援：it + n（不能跳躍）、it1 < it2（不能比大小）、it[n]
//   標頭檔：<list>、<set>、<map>、<iterator>
//   標準版本：C++98 起即有；reverse_iterator 也是 C++98。
//   移動 n 步的複雜度：O(n)（--/++ 都只能一步一步走）
//
// 【詳細解釋 Explanation】
//
// 【1. Bidirectional 只比 Forward 多一件事：--】
//   能力上的差距非常小，就是多了後退。但這一個能力打開了一整批演算法：
//       std::reverse、std::reverse_copy   —— 需要從兩端往中間夾
//       std::prev、rbegin() / rend()      —— 需要往回走
//       std::next_permutation             —— 需要從尾端往前掃找轉折點
//   這些在 forward_list 上全部不能用。
//
//   為什麼 list 做得到？因為雙向鏈結串列的節點多存一個 prev 指標：
//       nullptr <- [10|prev|next] <-> [20|prev|next] <-> [30|prev|next] -> nullptr
//   代價是每個節點多一個指標的空間（在 64-bit 上是 8 bytes），
//   這正是 forward_list 存在的理由——省下那個 prev。
//
// 【2. 為什麼還是不能 it + 2：因為沒有「跳躍」的物理基礎】
//   跳躍要能 O(1) 算出目標位址。vector 的元素在記憶體中連續，
//   位址就是 base + n * sizeof(T)，一次乘法搞定。
//   鏈結串列的節點散落在 heap 各處，位址之間沒有任何算術關係，
//   要到第 n 個只能真的走 n 步。
//   標準的態度是：**做不到 O(1) 的事就不要提供 O(1) 的語法**。
//   如果 list 提供了 it + 2，使用者會理所當然以為它很便宜，
//   然後在迴圈裡寫出 O(n²) 而不自知。
//   想走 n 步請明確寫 std::advance(it, n) 或 std::next(it, n)——
//   語法上就看得出「這是一個動作」，不是一個常數時間的算式。
//
// 【3. set / map 為什麼也是 Bidirectional】
//   它們的底層通常是紅黑樹。中序走訪（in-order traversal）可以往前也可以往後，
//   所以 ++/-- 都成立；但要跳到「第 n 小的元素」需要 O(log n) 甚至 O(n)，
//   所以不提供隨機存取。
//   這也解釋了 set 為什麼「自動排序」——它不是排序過的陣列，
//   而是走訪順序天生就是由小到大。本檔 main 的 set 輸入是 {50,10,30,20,40}，
//   印出來是 10 20 30 40 50。
//
// 【4. reverse_iterator：用 -- 反向包裝出來的東西】
//   rbegin() / rend() 回傳的是 std::reverse_iterator，
//   它就是拿一個 Bidirectional Iterator 包起來、把 ++ 實作成 --。
//   所以 **rbegin() 這件事本身就要求容器至少是 Bidirectional**，
//   forward_list 根本沒有 rbegin()。
//
//   關鍵的設計細節是「位置偏移一格」：
//       reverse_iterator 內部存的是 current，
//       但 operator* 回傳的是 *(current - 1)。
//   為什麼要偏移？因為 rend() 必須是一個合法可比較的位置。
//   若不偏移，rend() 就得指向 begin() 的前一個——那個位置不存在，
//   對鏈結串列來說連取址都不合法。
//   偏移之後：rbegin().base() == end()，rend().base() == begin()，
//   兩端都落在合法範圍內。代價是每次解參考多一次 --。
//
// 【概念補充 Concept Deep Dive】
//
// (A) reverse_iterator::base() 的偏移，在 erase 時會咬人
//     常見需求：「找到反向迭代器 rit，把它指的元素刪掉」。
//     直覺寫法 lst.erase(rit.base()) 會刪錯一個——刪到它的下一個。
//     正確寫法是 lst.erase(std::next(rit).base())，
//     或 C++11 起更直觀的：lst.erase(std::prev(rit.base()))。
//     本檔 main 有實測對照。
//
// (B) list 的迭代器失效規則比 vector 寬鬆得多
//     vector 的 push_back 可能整批重新配置，讓所有迭代器失效。
//     list 只有「被刪掉的那個節點」的迭代器會失效，
//     insert 完全不影響既有迭代器。
//     這個性質是 LRU Cache 能用 list 實作的關鍵——
//     可以把 iterator 存在 hash map 裡長期保留。
//
// (C) std::list::splice：O(1) 搬節點，且迭代器保持有效
//     splice 只是把節點從一條鏈拆下來、接到另一個位置，
//     不複製也不移動元素本身。所以：
//       * 複雜度 O(1)（搬單一元素時）
//       * 指向被搬元素的迭代器**依然有效**，只是所屬容器變了
//     這兩點加起來，就是下面 LRU Cache 的核心手法。
//     vector 做不到這件事，因為元素必須實際搬動記憶體。
//
// (D) 為什麼 std::list 有自己的 sort() 成員函式
//     std::sort 要 Random Access，list 給不起。
//     list::sort 走的是為鏈結串列設計的合併排序：
//     只改指標、不搬資料，所以不需要隨機存取，也不需要額外緩衝區。
//
// 【注意事項 Pay Attention】
//   1. --lst.begin() 是未定義行為（begin 之前沒有合法位置）。
//      同理 ++lst.end() 也是 UB。
//   2. rend() 不可解參考，*rend() 是 UB——它是「begin 之前」的哨兵。
//   3. it + 2 對 list 是編譯錯誤，不是效能問題。要走請用 std::advance / std::next。
//   4. reverse_iterator 的 base() 有一格偏移，拿去 erase 前務必想清楚。
//   5. set / map 的迭代器指向的 key 是 const 的（改 key 會破壞排序不變式），
//      map 只有 value（second）可改。
//   6. std::distance(list) 是 O(n)，但 list::size() 自 C++11 起被要求是 O(1)。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Bidirectional Iterator 與 list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::list 的迭代器為什麼不支援 it + 2？是效能考量還是做不到？
//     答：兩者都是，但根本原因是「沒有 O(1) 的實作方式」。
//         跳躍需要能用算術算出目標位址，vector 的元素連續所以
//         base + n * sizeof(T) 一次乘法就到；鏈結串列的節點散在 heap 各處，
//         位址之間沒有任何關係，到第 n 個只能真的走 n 步。
//         標準的原則是「不提供會誤導使用者的廉價語法」——
//         如果寫得出 it + 2，大家就會以為它很便宜。
//     追問：那要走 n 步怎麼寫？
//         → std::advance(it, n)（原地修改）或 std::next(it, n)（回傳新的）。
//           對 list 是 O(n)，對 vector 內部會自動退化成 O(1) 的 +=。
//
// 🔥 Q2. rbegin() 回傳的 reverse_iterator，為什麼 *rit 拿到的是 base() 的前一個？
//     答：因為 rend() 必須是合法位置。若不偏移，rend() 得指向 begin() 的前一個，
//         那個位置根本不存在。偏移一格之後
//         rbegin().base() == end()、rend().base() == begin()，
//         兩端都落在合法範圍內，代價是解參考時多做一次 --。
//     追問：那要刪掉 rit 指的元素該怎麼寫？
//         → lst.erase(std::next(rit).base())，或 lst.erase(std::prev(rit.base()))。
//           直接寫 lst.erase(rit.base()) 會刪錯一個。
//
// ⚠️ 陷阱. LRU Cache 為什麼用 std::list 而不用 std::vector？
//         vector 不是快取更友善、常數更小嗎？
//     答：因為 LRU 的核心動作是「把某個既有元素搬到最前面」。
//         list 用 splice 做這件事是 O(1)，而且**指向該元素的迭代器不會失效**，
//         所以可以把 iterator 存進 unordered_map 長期保留，查詢 O(1)。
//         vector 搬元素要實際移動記憶體 O(n)，更致命的是搬完之後
//         所有存下來的索引/迭代器全部作廢，hash map 裡的東西整批失效。
//     為什麼會錯：只比較「單次存取的快取友善度」，忽略了
//         **迭代器/參考的穩定性**才是這個資料結構的選型關鍵。
//         這正是「容器選型看的是操作組合，不是單一操作的常數」的典型例子。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <set>
#include <map>
#include <string>
#include <unordered_map>
#include <iterator>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的快取，get/put 都要 O(1)；
//         容量滿時淘汰「最久沒被使用」的項目。
//   為什麼用到本主題：這題是 Bidirectional Iterator（std::list）最經典的應用。
//     用到的正是 list 的兩個獨門性質：
//       (1) splice() 把節點搬到串列頭是 O(1)，只改指標不搬資料
//       (2) 搬動之後，指向該節點的迭代器**依然有效**
//     有了 (2)，才能把 list::iterator 存進 unordered_map 裡長期保留，
//     達成 get/put 都是 O(1)。換成 vector 這兩點都不成立。
// -----------------------------------------------------------------------------
class LRUCache {
public:
    explicit LRUCache(std::size_t capacity) : cap_(capacity) {}

    int get(int key) {
        auto mit = index_.find(key);
        if (mit == index_.end()) return -1;
        // 命中：把該節點搬到最前面（最近使用）。O(1)，且 mit->second 仍有效
        items_.splice(items_.begin(), items_, mit->second);
        return mit->second->second;
    }

    void put(int key, int value) {
        auto mit = index_.find(key);
        if (mit != index_.end()) {
            mit->second->second = value;                        // 更新值
            items_.splice(items_.begin(), items_, mit->second); // 搬到最前
            return;
        }
        if (index_.size() == cap_) {
            // 淘汰最後一個（最久沒用）
            const int oldKey = items_.back().first;
            items_.pop_back();
            index_.erase(oldKey);
        }
        items_.emplace_front(key, value);
        index_[key] = items_.begin();
    }

    std::string dump() const {
        std::string s;
        for (const auto& kv : items_) {
            if (!s.empty()) s += " ";
            s += "(" + std::to_string(kv.first) + ":" + std::to_string(kv.second) + ")";
        }
        return s.empty() ? "(空)" : s;
    }

private:
    using Item = std::pair<int, int>;              // (key, value)
    std::size_t                                        cap_;
    std::list<Item>                                    items_;   // 最前 = 最近使用
    std::unordered_map<int, std::list<Item>::iterator> index_;   // 迭代器可安全長期保存
};

// -----------------------------------------------------------------------------
// 【日常實務範例】從 log 尾端往回找最近一筆 ERROR
//   情境：服務出事了，log 有上萬行，要找「最後一次」錯誤發生在哪。
//         從頭掃到尾再記住最後一筆是 O(n) 且一定要掃完；
//         從尾端往回掃通常掃幾行就中，實務上快得多。
//   為什麼用到本主題：反向走訪（rbegin/rend 或 --it）**要求 Bidirectional**。
//     同樣的程式碼放到 forward_list 上連編譯都過不了。
// -----------------------------------------------------------------------------
std::string findLastError(const std::list<std::string>& logs) {
    for (auto rit = logs.rbegin(); rit != logs.rend(); ++rit) {
        if (rit->find("[ERROR]") != std::string::npos) return *rit;
    }
    return "(整份 log 沒有 ERROR)";
}

// 進階：把最後一筆 ERROR 從 log 裡刪掉（示範 base() 的偏移陷阱）
bool eraseLastError(std::list<std::string>& logs) {
    for (auto rit = logs.rbegin(); rit != logs.rend(); ++rit) {
        if (rit->find("[ERROR]") != std::string::npos) {
            // 正解：std::next(rit).base()。寫成 rit.base() 會刪到下一筆
            logs.erase(std::next(rit).base());
            return true;
        }
    }
    return false;
}

int main() {
    // list 的迭代器是 Bidirectional Iterator
    std::cout << "=== list ===" << std::endl;
    std::list<int> lst = {10, 20, 30, 40, 50};

    auto it = lst.begin();
    std::cout << "起始: " << *it << std::endl;

    ++it;
    std::cout << "++it: " << *it << std::endl;

    ++it;
    std::cout << "++it: " << *it << std::endl;

    --it;  // Bidirectional 可以後退！
    std::cout << "--it: " << *it << std::endl;

    // 但不能跳躍
    // it + 2;  // 編譯錯誤！鏈結串列的節點位址之間沒有算術關係

    // 要走多步只能用 advance / next（對 list 是 O(n)）
    std::advance(it, 2);
    std::cout << "std::advance(it, 2): " << *it << std::endl;
    std::cout << "std::next(it) = " << *std::next(it) << std::endl;
    std::cout << "std::prev(it) = " << *std::prev(it) << std::endl;

    // set 也是 Bidirectional Iterator
    std::cout << "\n=== set ===" << std::endl;
    std::set<int> s = {50, 10, 30, 20, 40};  // 自動排序

    std::cout << "正向: ";
    for (auto sit = s.begin(); sit != s.end(); ++sit) {
        std::cout << *sit << " ";
    }
    std::cout << std::endl;

    std::cout << "反向: ";
    for (auto sit = s.rbegin(); sit != s.rend(); ++sit) {
        std::cout << *sit << " ";
    }
    std::cout << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== reverse_iterator 的一格偏移 ===" << std::endl;
    {
        std::list<int> v = {1, 2, 3, 4, 5};
        auto rit = v.rbegin();                 // 指向邏輯上的「最後一個」
        std::cout << "*rbegin()        = " << *rit << "（最後一個元素）" << std::endl;
        std::cout << "*rbegin().base() 不可解參考，因為 base() == end()" << std::endl;
        std::cout << "rbegin().base() == end() ? " << std::boolalpha
                  << (rit.base() == v.end()) << std::endl;
        std::cout << "rend().base()   == begin() ? "
                  << (v.rend().base() == v.begin()) << std::endl;
        std::cout << "→ 偏移一格是為了讓 rend() 落在合法位置上" << std::endl;

        // 偏移在 erase 時的實際後果
        std::list<int> a = {1, 2, 3, 4, 5};
        std::list<int> b = {1, 2, 3, 4, 5};
        auto ra = a.rbegin(); ++ra;            // 指向 4
        auto rb = b.rbegin(); ++rb;            // 指向 4
        std::cout << "兩份都想刪掉值 4（*rit == " << *ra << "）" << std::endl;

        a.erase(ra.base());                    // 錯誤寫法：刪到 5
        b.erase(std::next(rb).base());         // 正確寫法：刪到 4

        std::cout << "erase(rit.base())        -> ";
        for (int x : a) std::cout << x << " ";
        std::cout << "  ← 刪錯了（刪掉 5）" << std::endl;
        std::cout << "erase(next(rit).base())  -> ";
        for (int x : b) std::cout << x << " ";
        std::cout << "  ← 正確（刪掉 4）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== list 的迭代器穩定性（LRU 能成立的基礎）===" << std::endl;
    {
        std::list<int> v = {1, 2, 3};
        auto keep = std::next(v.begin());      // 指向 2
        v.push_front(0);
        v.push_back(4);
        std::cout << "前後各插入元素後，先前保存的迭代器仍指向: "
                  << *keep << std::endl;
        v.splice(v.begin(), v, keep);          // 把它搬到最前面，O(1)
        std::cout << "splice 搬到最前面後，同一個迭代器仍指向: "
                  << *keep << std::endl;
        std::cout << "目前內容: ";
        for (int x : v) std::cout << x << " ";
        std::cout << std::endl;
        std::cout << "→ vector 做同樣的事，迭代器早就全部失效了" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== map 也是 Bidirectional（key 為 const）===" << std::endl;
    {
        std::map<std::string, int> m = {{"banana", 2}, {"apple", 5}, {"cherry", 9}};
        auto mit = m.end();
        --mit;                                  // Bidirectional：可以從 end 退回來
        std::cout << "最後一個（key 最大）: " << mit->first
                  << " = " << mit->second << std::endl;
        mit->second = 99;                       // value 可以改
        // mit->first = "x";                    // 編譯錯誤：key 是 const
        std::cout << "改 value 後: " << mit->first << " = " << mit->second << std::endl;
        std::cout << "（key 是 const，改了會破壞紅黑樹的排序不變式）" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== LeetCode 146. LRU Cache ===" << std::endl;
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        std::cout << "put(1,1) put(2,2): " << cache.dump() << std::endl;
        std::cout << "get(1) = " << cache.get(1)
                  << "  -> " << cache.dump() << "（1 被搬到最前）" << std::endl;
        cache.put(3, 3);      // 容量滿，淘汰最久沒用的 2
        std::cout << "put(3,3): " << cache.dump() << std::endl;
        std::cout << "get(2) = " << cache.get(2) << "（已被淘汰，回傳 -1）" << std::endl;
        cache.put(4, 4);      // 淘汰 1
        std::cout << "put(4,4): " << cache.dump() << std::endl;
        std::cout << "get(1) = " << cache.get(1) << std::endl;
        std::cout << "get(3) = " << cache.get(3) << std::endl;
        std::cout << "get(4) = " << cache.get(4) << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：從 log 尾端往回找最近一筆 ERROR ===" << std::endl;
    {
        std::list<std::string> logs = {
            "2026-07-19 10:00:01 [INFO]  service started",
            "2026-07-19 10:00:07 [ERROR] db connect failed (attempt 1)",
            "2026-07-19 10:00:09 [WARN]  retrying",
            "2026-07-19 10:00:11 [ERROR] db connect failed (attempt 2)",
            "2026-07-19 10:00:15 [INFO]  connected",
            "2026-07-19 10:00:20 [INFO]  ready",
        };

        std::cout << "最近一筆 ERROR:\n  " << findLastError(logs) << std::endl;
        std::cout << "（從尾端往回掃，通常掃幾行就中，不必掃完整份）" << std::endl;

        eraseLastError(logs);
        std::cout << "刪掉最近一筆 ERROR 之後，剩下的 ERROR:\n  "
                  << findLastError(logs) << std::endl;

        std::list<std::string> clean = {"[INFO] ok", "[INFO] ok"};
        std::cout << "沒有 ERROR 的 log: " << findLastError(clean) << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第五課：迭代器的五種分類6.cpp -o demo6

// === 預期輸出 ===
// === list ===
// 起始: 10
// ++it: 20
// ++it: 30
// --it: 20
// std::advance(it, 2): 40
// std::next(it) = 50
// std::prev(it) = 30
//
// === set ===
// 正向: 10 20 30 40 50
// 反向: 50 40 30 20 10
//
// === reverse_iterator 的一格偏移 ===
// *rbegin()        = 5（最後一個元素）
// *rbegin().base() 不可解參考，因為 base() == end()
// rbegin().base() == end() ? true
// rend().base()   == begin() ? true
// → 偏移一格是為了讓 rend() 落在合法位置上
// 兩份都想刪掉值 4（*rit == 4）
// erase(rit.base())        -> 1 2 3 4   ← 刪錯了（刪掉 5）
// erase(next(rit).base())  -> 1 2 3 5   ← 正確（刪掉 4）
//
// === list 的迭代器穩定性（LRU 能成立的基礎）===
// 前後各插入元素後，先前保存的迭代器仍指向: 2
// splice 搬到最前面後，同一個迭代器仍指向: 2
// 目前內容: 2 0 1 3 4
// → vector 做同樣的事，迭代器早就全部失效了
//
// === map 也是 Bidirectional（key 為 const）===
// 最後一個（key 最大）: cherry = 9
// 改 value 後: cherry = 99
// （key 是 const，改了會破壞紅黑樹的排序不變式）
//
// === LeetCode 146. LRU Cache ===
// put(1,1) put(2,2): (2:2) (1:1)
// get(1) = 1  -> (1:1) (2:2)（1 被搬到最前）
// put(3,3): (3:3) (1:1)
// get(2) = -1（已被淘汰，回傳 -1）
// put(4,4): (4:4) (3:3)
// get(1) = -1
// get(3) = 3
// get(4) = 4
//
// === 日常實務：從 log 尾端往回找最近一筆 ERROR ===
// 最近一筆 ERROR:
//   2026-07-19 10:00:11 [ERROR] db connect failed (attempt 2)
// （從尾端往回掃，通常掃幾行就中，不必掃完整份）
// 刪掉最近一筆 ERROR 之後，剩下的 ERROR:
//   2026-07-19 10:00:07 [ERROR] db connect failed (attempt 1)
// 沒有 ERROR 的 log: (整份 log 沒有 ERROR)
