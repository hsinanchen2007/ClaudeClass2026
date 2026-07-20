// =============================================================================
//  第六課 4 — std::list：雙向鏈結串列，以及 O(1) 拼接(splice)的真正價值
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, class Allocator = std::allocator<T>> class list;  // <list>,C++98
//
//   常用介面與複雜度：
//     void push_front(const T&);          // O(1)
//     void push_back(const T&);           // O(1)
//     iterator insert(const_iterator pos, const T&);   // O(1)（已持有 pos）
//     iterator erase(const_iterator pos);              // O(1)（已持有 pos）
//     void splice(const_iterator pos, list& other);              // O(1)
//     void splice(const_iterator pos, list& other, const_iterator it);        // O(1)
//     void splice(const_iterator pos, list& other,
//                 const_iterator first, const_iterator last);
//         //  同一個 list 內：O(1)；跨 list：O(distance(first,last))
//         //  （因為 C++11 要求 size() 為 O(1)，跨 list 時必須更新兩邊的計數）
//     void sort();                        // O(n log n)，成員函式，非 std::sort
//     void merge(list& other);            // O(n)，兩邊都須已排序
//     void reverse();                     // O(n)
//     size_type size() const;             // O(1)（C++11 起強制）
//
//   沒有的東西：operator[]、at()、隨機存取迭代器、data()
//   標頭檔：#include <list>
//
// 【詳細解釋 Explanation】
//
// 【1. list 存在的理由:用「不連續」換「穩定」】
// vector 把元素排在一整塊連續記憶體上,所以能 O(1) 隨機存取;代價是插入、刪除、
// 以及容量不足時的重新配置,都要**搬動元素**,於是先前取得的 iterator、指標、
// reference 全部失效。
//
// list 走的是完全相反的路線:每個元素獨立配置一個節點,節點之間用指標串接。
// 因此:
//   * 沒有位址算術可用 → **不支援隨機存取**(沒有 operator[])
//   * 但插入/刪除只是改幾個指標 → **O(1),而且不動到其他任何節點**
//   * 於是 **其他元素的 iterator、指標、reference 永遠保持有效**
//
// 這個「穩定性」才是 list 真正的賣點,而不是教科書常掛在嘴邊的「插入很快」。
// 當你需要長期持有指向某個元素的 iterator,同時又要不斷增刪其他元素時,
// list 幾乎是唯一乾淨的選擇 —— 這正是 LRU cache 的核心需求(見下方 LeetCode 146)。
//
// 【2. 節點長什麼樣:為什麼一個 int 要花掉 24 bytes】
// libstdc++ 的 list 節點簡化後是:
//     struct _List_node {
//         _List_node* _M_next;    // 8 bytes
//         _List_node* _M_prev;    // 8 bytes
//         T           _M_data;    // sizeof(T)
//     };
// 存一個 int(4 bytes)要付兩個指標(16 bytes)的代價,加上對齊後每個節點
// 24 bytes(**實作定義**),記憶體開銷是 vector 的六倍;而且每個節點是一次
// 獨立的 heap 配置,配置本身也有成本。
//
// 這解釋了一個常被誤解的現象:**list 存小型元素時非常不划算**。它適合的是
// 「元素大、複製成本高、且需要頻繁重排」的場景。
//
// 【3. splice:list 真正無可取代的操作】
// splice 把元素從一個 list **轉移**到另一個(或同一個)list 的指定位置,
// 而且**完全不複製、不搬移、不配置、不解構任何元素** —— 它只是把幾個
// next/prev 指標重新接線。
//
// 這帶來兩個 vector 永遠做不到的性質:
//   (a) 成本與元素數量無關(整份 splice 是 O(1),即使搬一百萬個元素)。
//   (b) **被搬移元素的 iterator 依然有效**,而且現在自動指向新的 list ——
//       這是標準明文保證的。
//
// 對照 vector:要把一段元素從一個 vector 移到另一個,只能逐一複製或搬移,
// O(n),而且兩邊的 iterator 全數失效。
//
// 一個容易忽略的細節是**三參數版本的複雜度**:
//     lst.splice(pos, other, first, last);
// 在同一個 list 內是 O(1);但**跨 list** 時是 O(distance(first,last)),
// 因為 C++11 要求 size() 必須是 O(1),所以兩邊的元素計數都得更新,而唯一的
// 辦法就是實際數一遍區間長度。這是「size() 從 O(n) 改成 O(1)」這個標準變更
// 帶來的副作用,面試時偶爾會被追問。
//
// 【4. 為什麼 list 有自己的 sort(),不能用 std::sort】
// std::sort 內部走 introsort(quicksort + heapsort + insertion sort),需要
// **隨機存取迭代器** —— 它得能算中位數樞紐、能 it + n 跳躍。list 的迭代器
// 只能 ++/--(雙向迭代器),型別層就對不上,寫了也編譯不過。
//
// 所以 list 自備成員函式 sort(),內部用 **merge sort** —— 這個選擇很自然,
// 因為 merge sort 只需要循序走訪,而且對鏈結串列來說「合併」不需要額外空間,
// 只要改指標即可。同理,list 也自備 merge()、reverse()、unique()、remove(),
// 都是「改指標比搬資料划算」的操作。
//
// 【5. 理論勝利,實務常常落敗:cache 的力量】
// 複雜度表說 list 中間插入 O(1)、vector O(n),於是很多人得出「頻繁中間插入
// 就該用 list」。這個推論在實務上經常是錯的,原因有三:
//
//   (a) **O(1) 有前提**。只有「已經持有該位置的 iterator」時插入才是 O(1)。
//       若每次都得先找到位置,list 的尋找是 O(n) 的逐節點走訪,總成本
//       和 vector 同級 —— 而且 list 沒有二分搜尋可用(迭代器能力不足)。
//   (b) **cache locality**。vector 元素連續,一次 cache line 就帶進多個元素,
//       硬體 prefetcher 也能正確預測;list 節點散落 heap,每次 ++it 都可能是
//       一次 cache miss,代價可達數十至上百個 CPU cycle。
//   (c) **搬移其實很便宜**。vector 的 O(n) 搬移對 trivially copyable 型別
//       就是一次 memmove —— 這是 CPU 最擅長的連續複製,吞吐極高。
//
// 結論:元素小、資料量中等時,vector 經常反而更快。這是實務上反覆被量測驗證的
// 現象;確切的交叉點取決於元素大小、資料量與硬體,**請以你自己的量測為準**,
// 不要照抄任何特定數字。選 list 的正當理由通常是「需要 iterator 穩定」或
// 「需要 O(1) splice」,而不是「插入比較快」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:環狀串列與 sentinel 節點
//     libstdc++ 的 list 內部是一個**環狀雙向串列**,並在 list 物件裡內嵌一個
//     不存資料的 sentinel(哨兵)節點。end() 就是指向這個 sentinel。
//     這個設計讓所有操作都不需要特判「空串列」或「頭尾」——
//     插入永遠是同一組指標重接動作,程式碼因此沒有任何分支。
//     本機實測 sizeof(std::list<int>) = 24(**實作定義**):
//     sentinel 的兩個指標 + C++11 起強制 O(1) 的 size 計數。
//
// (B) C++11 讓 size() 從 O(n) 變成 O(1)
//     C++03 允許 list::size() 是 O(n)(現數一遍),C++11 起強制 O(1),
//     於是實作必須額外維護一個計數欄位。這是一次真實的 ABI 破壞性變更,
//     GCC 曾為此提供過渡期。副作用就是上面提到的:跨 list 的區間 splice
//     從 O(1) 退化成 O(distance)。
//
// (C) 編譯器在 splice 時做了什麼
//     一次整份 splice 展開後大約只有六次指標指派:把 other 的頭尾接到 pos
//     前後,再把 other 自己接回空狀態,最後更新兩邊的 size 計數。
//     沒有迴圈、沒有配置、沒有呼叫任何建構子或解構子 —— 這就是為什麼它是 O(1)。
//
// (D) list vs forward_list
//     forward_list 是單向的,sizeof 只有 8(一個指標,**實作定義**),
//     連 size() 都不提供。用 list 的時機是需要反向走訪或 push_back;
//     只往前走且極度在意記憶體時才選 forward_list。
//
// 【注意事項 Pay Attention】
//  1. list 沒有 operator[] 與 at()。要取第 n 個元素只能 std::advance / std::next,
//     那是 O(n) 的走訪,不是隨機存取 —— 在迴圈裡這樣做會變成 O(n²)。
//  2. splice 之後,來源 list 的元素**已被移走**,來源會變空(整份 splice 時)。
//     指向被搬移元素的 iterator 仍然有效,但它現在屬於目的地 list。
//  3. erase 會使**被刪除的那個** iterator 失效,其他 iterator 不受影響。
//     解除參照已失效的 iterator 是 undefined behavior,行為**不保證、不可預測**。
//  4. 不能對 list 使用 std::sort(迭代器能力不足,編譯期就會失敗);
//     請用成員函式 lst.sort()。同理 std::reverse 可以用但成員 reverse() 更快。
//  5. 節點大小(本機 int 節點約 24 bytes)與 sizeof(std::list<int>) = 24 皆屬
//     **實作定義**,換標準庫可能不同。
//  6. 別因為「複雜度表比較漂亮」就選 list。先確認你的理由是 iterator 穩定性
//     或 O(1) splice;若只是「聽說插入比較快」,多半該用 vector。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::list
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. list 相對於 vector 的核心優勢是什麼?
//     答：不是「插入比較快」,而是兩件事:(1) **iterator/指標/reference 穩定** ——
//         插入或刪除其他元素完全不影響既有 iterator,因為節點從不搬動;
//         (2) **O(1) splice** —— 可以把任意多個元素在兩個 list 之間轉移,
//         不複製、不配置,成本與元素數量無關,而且被搬移元素的 iterator 依然有效。
//         這兩點 vector 都做不到。
//     追問：那 list 的代價是什麼?
//         → 沒有隨機存取;每個元素多付兩個指標(本機 int 節點約 24 bytes,
//           實作定義)與一次獨立 heap 配置;走訪時 cache 不友善。
//
// 🔥 Q2. 為什麼 std::sort 不能用在 std::list 上?
//     答：std::sort 需要隨機存取迭代器(要能 it + n 跳躍以選樞紐、做分割),
//         而 list 只提供雙向迭代器,型別層就不相容,編譯期即失敗。
//         所以 list 自備成員函式 sort(),內部使用 merge sort ——
//         merge sort 只需循序走訪,且對鏈結串列而言合併只要改指標、不需額外空間。
//     追問：那 std::find 為什麼可以用在 list 上?
//         → 因為 std::find 只需要 InputIterator(能 ++ 和解參照即可),
//           list 的雙向迭代器完全滿足。演算法要求哪一層迭代器,決定了它能用在
//           哪些容器上。
//
// ⚠️ 陷阱. 「中間插入很頻繁,所以用 list 一定比 vector 快」——錯在哪?
//     答：錯在忽略前提與常數。list 插入 O(1) 的前提是**已經持有該位置的 iterator**;
//         若每次都要先找到位置,尋找本身就是 O(n) 逐節點走訪(且無法二分搜尋),
//         總成本與 vector 同級。加上 list 走訪每步都可能 cache miss,
//         而 vector 的搬移是 memmove(CPU 最擅長的操作),
//         實測中元素小、資料量中等時 vector 經常勝出。
//     為什麼會錯：把大 O 當成效能排行榜。大 O 描述的是成長趨勢,刻意隱藏了
//         常數因子與記憶體階層效應 —— 而在現代 CPU 上這兩者往往才是主導因素。
//
// ⚠️ 陷阱. splice 都是 O(1) 嗎?
//     答：不一定。整份 splice(pos, other) 與單元素 splice(pos, other, it) 是 O(1);
//         但區間版 splice(pos, other, first, last) 只有在**同一個 list 內**才是 O(1),
//         **跨 list 時是 O(distance(first, last))**。原因是 C++11 要求 size() 為 O(1),
//         跨 list 轉移必須更新兩邊的元素計數,而唯一的辦法就是實際數一遍區間長度。
//     為什麼會錯：把「splice 只是改指標」這個正確直覺,無條件套用到所有多載上,
//         忽略了「維護 O(1) size()」這個標準要求帶來的額外義務。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <unordered_map>
#include <string>
#include <vector>
#include <iterator>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個 LRU（Least Recently Used）快取，get 與 put 都要 O(1)。
//   為什麼用到本主題：這題是 std::list 兩大賣點的教科書級應用，缺一不可 ——
//     * 需要維護「使用順序」，且每次存取都要把某個元素移到最前面
//       → list::splice 讓「移到最前面」成為 O(1) 的指標重接，完全不複製元素。
//     * 需要 O(1) 找到某個 key 對應的節點
//       → unordered_map<key, list::iterator> 存下 iterator。
//         這只有在 iterator **永遠不失效** 的容器上才成立 ——
//         若改用 vector，任何一次插入都可能讓存起來的位置全部作廢。
//   複雜度：get O(1)、put O(1)。
// -----------------------------------------------------------------------------
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;              // 未命中
        // 命中：把該節點移到串列最前面（最近使用）。
        // splice 只是改指標，O(1)，而且 it->second 這個 iterator 仍然有效。
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = index_.find(key);
        if (it != index_.end()) {                       // 已存在：更新值並移到最前
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() == cap_) {                    // 滿了：淘汰最久未使用（串列尾端）
            index_.erase(items_.back().first);
            items_.pop_back();
        }
        items_.emplace_front(key, value);
        index_[key] = items_.begin();
    }

private:
    size_t cap_;
    std::list<std::pair<int, int>> items_;                              // 前=最近使用
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> index_;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】多來源 log 批次合併（log aggregator）
//   情境：一個日誌彙整服務同時從多台機器收到已按時間排序的 log 批次，
//     要把它們合併成一條全域有序的時間線，再交給下游寫檔或送進搜尋引擎。
//   為什麼用到本主題：這是 list 少數壓倒性勝出的場景 ——
//     * list::merge() 合併兩條已排序的串列，全程只改指標，
//       **不複製也不配置任何元素**；若用 vector 就得配置一塊新緩衝區
//       並把所有元素複製過去。當單筆 log 是個大字串時，差距非常明顯。
//     * merge 之後來源會變空，所有權清楚地轉移到目標，不會留下重複資料。
//   注意：merge 要求兩邊都**已經排序**（本例的批次本來就依時間有序）；
//     若未排序，結果不會是排序好的串列。
// -----------------------------------------------------------------------------
struct LogRecord {
    long        timestamp;
    std::string message;
};

std::list<LogRecord> mergeLogBatches(std::vector<std::list<LogRecord>>& batches) {
    auto byTime = [](const LogRecord& a, const LogRecord& b) {
        return a.timestamp < b.timestamp;
    };

    std::list<LogRecord> timeline;
    for (auto& batch : batches) {
        batch.sort(byTime);           // 保險起見先確保有序（成員 sort，merge sort）
        timeline.merge(batch, byTime);  // O(n) 指標重接，batch 事後變空
    }
    return timeline;
}

int main() {
    // ── 原始課堂示範：list 的基本操作與 splice ────────────────────────────
    std::cout << "=== std::list ===" << std::endl;

    std::list<int> lst = {20, 40};

    // 任意位置插入都是 O(1)（前提是已有迭代器）
    lst.push_front(10);
    lst.push_back(50);

    // 在中間插入
    auto it = lst.begin();
    ++it;  // 指向 20
    ++it;  // 指向 40
    lst.insert(it, 30);  // 在 40 前面插入 30

    std::cout << "元素: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // list 特有的操作
    std::list<int> lst2 = {100, 200, 300};

    // splice：將 lst2 的元素移動到 lst
    it = lst.begin();
    std::advance(it, 2);  // 指向 30

    // 將 lst2 的所有元素插入到 lst 的 it 位置（30 前面）
    // splice 是 O(1) 操作，因為它只是改變指標，不需要複製元素
    // 注意：splice 後 lst2 會變空，因為元素被移動了
    lst.splice(it, lst2);  // 在 30 前面插入 lst2 的所有元素

    std::cout << "splice 後 lst: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "splice 後 lst2 大小: " << lst2.size() << std::endl;

    // ── iterator 穩定性：list 的真正賣點 ──────────────────────────────────
    std::cout << "\n=== iterator 穩定性 ===" << std::endl;
    {
        std::list<int> stable = {1, 2, 3};
        auto keep = std::next(stable.begin());        // 指向 2
        const int* addr_before = &(*keep);

        for (int i = 0; i < 1000; ++i) stable.push_back(i);   // 大量插入
        stable.push_front(-1);
        const int* addr_after = &(*keep);

        std::cout << "  插入 1001 個元素後，原 iterator 仍指向: " << *keep << std::endl;
        std::cout << "  該元素的位址是否不變: "
                  << (addr_before == addr_after ? "是（節點從不搬動）" : "否") << std::endl;
        std::cout << "  → 這是 vector 做不到的：vector 重新配置會讓全部 iterator 失效"
                  << std::endl;
    }

    // ── splice 不複製元素的證明 ───────────────────────────────────────────
    std::cout << "\n=== splice 不搬動元素 ===" << std::endl;
    {
        std::list<int> a = {1, 2, 3};
        std::list<int> b = {7, 8, 9};
        auto watch = b.begin();                 // 指向 b 的 7
        const int* addr_before = &(*watch);

        a.splice(a.end(), b);                   // 整份轉移，O(1)

        const int* addr_after = &(*watch);      // 標準保證：watch 依然有效
        std::cout << "  splice 後原 iterator 仍指向: " << *watch << std::endl;
        std::cout << "  元素位址是否不變: "
                  << (addr_before == addr_after ? "是（只改了指標）" : "否") << std::endl;
        std::cout << "  a = ";
        for (int n : a) std::cout << n << " ";
        std::cout << "，b.size() = " << b.size() << std::endl;
    }

    // ── LeetCode 146 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 146. LRU Cache ===" << std::endl;
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        std::cout << "  get(1) = " << cache.get(1) << "  (期望 1)" << std::endl;
        cache.put(3, 3);                        // 容量滿 → 淘汰最久未使用的 key 2
        std::cout << "  get(2) = " << cache.get(2) << "  (期望 -1，已被淘汰)" << std::endl;
        cache.put(4, 4);                        // 淘汰 key 1
        std::cout << "  get(1) = " << cache.get(1) << "  (期望 -1，已被淘汰)" << std::endl;
        std::cout << "  get(3) = " << cache.get(3) << "  (期望 3)" << std::endl;
        std::cout << "  get(4) = " << cache.get(4) << "  (期望 4)" << std::endl;
    }

    // ── 日常實務：多來源 log 合併 ─────────────────────────────────────────
    std::cout << "\n=== 日常實務: 多來源 log 批次合併 ===" << std::endl;
    {
        std::vector<std::list<LogRecord>> batches = {
            { {1001, "web-01 收到請求 /index"},
              {1004, "web-01 回應 200"} },
            { {1002, "db-01  執行查詢 SELECT"},
              {1003, "db-01  查詢完成 12ms"} },
            { {1000, "lb-01  轉發連線"},
              {1005, "lb-01  連線關閉"} },
        };

        std::list<LogRecord> timeline = mergeLogBatches(batches);

        std::cout << "  合併後的全域時間線:" << std::endl;
        for (const auto& rec : timeline) {
            std::cout << "    [" << rec.timestamp << "] " << rec.message << std::endl;
        }
        std::cout << "  合併後各來源批次大小: ";
        for (const auto& b : batches) std::cout << b.size() << " ";
        std::cout << "（元素已被轉移，不是被複製）" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類4.cpp" -o list_demo

// === 預期輸出 ===
// === std::list ===
// 元素: 10 20 30 40 50
// splice 後 lst: 10 20 100 200 300 30 40 50
// splice 後 lst2 大小: 0
//
// === iterator 穩定性 ===
//   插入 1001 個元素後，原 iterator 仍指向: 2
//   該元素的位址是否不變: 是（節點從不搬動）
//   → 這是 vector 做不到的：vector 重新配置會讓全部 iterator 失效
//
// === splice 不搬動元素 ===
//   splice 後原 iterator 仍指向: 7
//   元素位址是否不變: 是（只改了指標）
//   a = 1 2 3 7 8 9 ，b.size() = 0
//
// === LeetCode 146. LRU Cache ===
//   get(1) = 1  (期望 1)
//   get(2) = -1  (期望 -1，已被淘汰)
//   get(1) = -1  (期望 -1，已被淘汰)
//   get(3) = 3  (期望 3)
//   get(4) = 4  (期望 4)
//
// === 日常實務: 多來源 log 批次合併 ===
//   合併後的全域時間線:
//     [1000] lb-01  轉發連線
//     [1001] web-01 收到請求 /index
//     [1002] db-01  執行查詢 SELECT
//     [1003] db-01  查詢完成 12ms
//     [1004] web-01 回應 200
//     [1005] lb-01  連線關閉
//   合併後各來源批次大小: 0 0 0 （元素已被轉移，不是被複製）
