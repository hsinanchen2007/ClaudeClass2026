// =============================================================================
//  第 28 課：deque 的實際應用場景 3  —  環形緩衝區（Ring Buffer）
// =============================================================================
//
// 【主題資訊 Information】
//   template <typename T> class RingBuffer;   // 以 std::deque<T> 實作
//     void push(const T&);        // 滿了就丟棄最舊的，再加入最新的
//     const T& operator[](size_t) const;   // 0 = 最舊，size()-1 = 最新
//     const T& latest() const;  const T& oldest() const;
//
//   標頭檔：<deque>、<string>
//   複雜度：push / latest / oldest / operator[] 全部 O(1)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 環形緩衝區要解決什麼問題】
//   「我只想保留最近 N 筆」——這個需求無所不在：
//       系統日誌（只留最近 1000 行）、感測器讀數、遊戲的輸入緩衝、
//       音訊處理的取樣視窗、監控指標的最近數值。
//   共同特徵是**固定上限 + 自動淘汰最舊的**。
//   若用 vector 並在滿了之後 erase(begin())，每次都是 O(n)；
//   用 deque 的 pop_front 則是 O(1)。
//
// 【2. 為什麼 deque 是這裡的正解】
//   環形緩衝區需要三件事：
//       尾端加入最新   → push_back，O(1)
//       頭端丟棄最舊   → pop_front，O(1)   ★ 只有 deque 做得到
//       隨機存取第 i 筆 → operator[]，O(1)
//   list 有前兩項但**沒有 operator[]**（它是 bidirectional iterator，
//   要取第 i 個得 std::advance，是 O(n)）；
//   vector 有第一、三項但第二項是 O(n)。
//   **只有 deque 三項全中**——這正是它在標準容器中的獨特定位。
//
// 【3. 與「傳統」環形緩衝區的差別】
//   教科書的 ring buffer 是一個固定大小的陣列 + 兩個索引（head/tail），
//   靠模數運算繞回來，完全不配置記憶體。
//   本檔用 deque 的版本更簡單、更安全（不必處理繞回與滿/空的歧義），
//   代價是 deque 內部仍會配置 chunk。
//   取捨：**極致效能（嵌入式、即時音訊）用陣列版；一般應用用 deque 版**，
//   後者少掉一整類索引運算的 bug。
//
// 【4. 為什麼 push 要先檢查再 pop_front】
//       if (buffer.size() >= maxSize) buffer.pop_front();
//       buffer.push_back(item);
//   順序不可對調。若先 push_back 再檢查，緩衝區會短暫超出上限一格；
//   在記憶體極度受限的場合這一格可能就是問題。
//   而且先丟後加語意更清楚：「騰出位子，再放新的」。
//
// 【概念補充 Concept Deep Dive】
//   ● operator[] 的索引語意
//     本實作定義 0 = 最舊、size()-1 = 最新，直接對應 deque 的自然順序。
//     另一種常見設計是 0 = 最新（像 log tail），
//     那就要寫成 buffer[size() - 1 - i]。
//     **沒有哪個比較對，但一定要在文件裡講清楚**，
//     否則呼叫端很容易搞反——這是實務上真實會發生的 bug。
//
//   ● 為什麼 latest() / oldest() 值得單獨提供
//     雖然 buffer[size()-1] 也能取最新，但 latest() 語意明確、
//     而且不會因為索引算錯而出事。
//     好的 API 應該讓「最常見的用法」最短也最難寫錯。
//
//   ● 空緩衝區的邊界
//     latest() / oldest() 對空的 deque 呼叫 back()/front() 是 **UB**。
//     本檔的原始版本沒有防護；增強版加了 empty() 檢查並回傳可判斷的結果，
//     這是把「教學範例」變成「可用程式碼」的必要一步。
//
//   ● forEach 用函式指標 vs 模板
//     原始版本用 void (*fn)(const T&) 函式指標，它**接不住有捕獲的 lambda**。
//     改成模板參數 template <typename F> void forEach(F fn) 就能接受
//     lambda、functor、函式指標——這是泛型程式設計的基本改良。
//     本檔兩種都保留，並示範差異。
//
// 【注意事項 Pay Attention】
//   1. 對空緩衝區呼叫 latest()/oldest() 是 **UB**（底層 back()/front()）；
//      本檔增強版加了檢查。
//   2. operator[] 不做邊界檢查（比照 deque 的慣例），越界是 UB。
//   3. maxSize 為 0 時每次 push 都會立刻丟棄，容器恆空——呼叫端要自行避免。
//   4. 索引語意（0 是最舊還是最新）必須在文件中明確定義。
//   5. 函式指標參數接不住有捕獲的 lambda，要用模板。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】環形緩衝區
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用 STL 容器實作「只保留最近 N 筆」的緩衝區，該選哪個容器？為什麼？
//     答：deque。這個需求要三種操作：尾端加入（O(1)）、
//         **頭端丟棄最舊**（O(1)）、以及隨機存取第 i 筆（O(1)）。
//         vector 的頭端刪除是 O(n) 且沒有 pop_front；
//         list 有 pop_front 但沒有 operator[]（取第 i 個要 O(n)）。
//         **只有 deque 三項全中。**
//     追問：那和傳統的「陣列 + head/tail 索引」版本比呢？
//         → 陣列版完全不配置記憶體、效能最好，適合嵌入式與即時音訊；
//         deque 版少掉一整類「索引繞回、滿空歧義」的 bug，適合一般應用。
//
// 🔥 Q2. push 時應該「先丟舊的再加新的」還是「先加新的再丟舊的」？
//     答：先丟再加。若先 push_back 再檢查，緩衝區會短暫超出上限一格；
//         記憶體受限時這一格可能就是問題。
//         而且「騰出位子，再放新的」語意更直觀。
//     追問：這一格差別真的有影響嗎？→ 對本例的 string 不明顯，
//         但若元素是 4KB 的影像 frame、上限是記憶體剛好裝得下的數量，
//         那多出來的一格就會觸發額外配置甚至 OOM。
//
// ⚠️ 陷阱. 「RingBuffer 的 operator[] 是 O(1)，所以拿它做迴圈遍歷
//         跟 vector 一樣快」——錯在哪？
//     答：兩者都是 O(1) 沒錯，但**常數差很多**。
//         deque 的 operator[] 要先算「在第幾塊 chunk、塊內第幾格」，
//         是兩次間接定址；vector 只要一次加法。
//         再加上 deque 的元素分散在多塊記憶體（本機每塊 512 位元組），
//         硬體預取器效果差、快取命中率低。
//         實測順序遍歷 vector 通常明顯快於 deque。
//     為什麼會錯：把「複雜度相同」當成「效能相同」。
//         大 O 只描述成長趨勢，不描述常數，而快取效應正好全藏在常數裡。
//         正確做法：需要頭端 O(1) 刪除才選 deque；
//         純粹順序存取的場合，vector 仍是預設選擇。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <ctime>
using namespace std;

template <typename T>
class RingBuffer {
    deque<T> buffer;
    size_t maxSize;

public:
    explicit RingBuffer(size_t max) : maxSize(max) {}

    void push(const T& item) {
        // 順序不可對調：先丟舊的騰出位子，再加新的。
        // 若先 push_back 再檢查，容器會短暫超出上限一格。
        if (buffer.size() >= maxSize) {
            buffer.pop_front();    // 丟棄最舊的  ← O(1)，vector 做不到
        }
        buffer.push_back(item);    // 加入最新的
    }

    // 隨機存取：0 是最舊的，size()-1 是最新的
    // 注意：與 deque::operator[] 一樣不做邊界檢查，越界是 UB
    const T& operator[](size_t i) const { return buffer[i]; }

    // 取得最新的
    const T& latest() const { return buffer.back(); }

    // 取得最舊的
    const T& oldest() const { return buffer.front(); }

    // 安全版本：空的時候不會 UB，回傳是否成功
    bool tryLatest(T& out) const {
        if (buffer.empty()) return false;
        out = buffer.back();
        return true;
    }
    bool tryOldest(T& out) const {
        if (buffer.empty()) return false;
        out = buffer.front();
        return true;
    }

    size_t size() const { return buffer.size(); }
    bool empty() const { return buffer.empty(); }
    bool full() const { return buffer.size() >= maxSize; }

    // 遍歷（從舊到新）—— 函式指標版本
    // ⚠️ 限制：接不住「有捕獲的 lambda」，因為那不能轉成函式指標
    void forEach(void (*fn)(const T&)) const {
        for (const T& item : buffer) fn(item);
    }

    // 遍歷（從舊到新）—— 模板版本
    // 可接受 lambda（含捕獲）、functor、函式指標，泛用得多
    template <typename F>
    void forEachGeneric(F fn) const {
        for (const T& item : buffer) fn(item);
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個固定容量的快取，支援 get(key) 與 put(key, value)，
//         兩者都要 O(1)。容量滿時淘汰「最久未使用」的項目。
//   為什麼用到本主題：LRU 與環形緩衝區是同一個家族的問題——
//         **固定容量 + 自動淘汰最舊的**。
//         差別在淘汰規則：ring buffer 依「加入時間」，LRU 依「最後使用時間」。
//         也因為這個差別，LRU 需要「把中間的元素搬到最新端」這個操作，
//         而 deque 做不到 O(1)（它只能在兩端操作），
//         所以標準解法改用 **list + unordered_map**：
//             list  提供 O(1) 的 splice（把任意節點搬到尾端）
//             map   提供 O(1) 的「key → list 節點位置」查找
//   這正好說明了「deque 的極限在哪裡」——它強在兩端，不強在中間。
// -----------------------------------------------------------------------------
class LRUCache {
    using Entry = pair<int, int>;                 // (key, value)
    list<Entry> items_;                            // 前端=最近使用，後端=最久未使用
    unordered_map<int, list<Entry>::iterator> pos_;  // key → 節點位置
    size_t capacity_;
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    int get(int key) {
        auto it = pos_.find(key);
        if (it == pos_.end()) return -1;           // 找不到
        // 命中 → 把該節點搬到最前面（表示「最近剛用過」）
        // splice 是 O(1) 且不使 iterator 失效，這是 list 獨有的能力
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = pos_.find(key);
        if (it != pos_.end()) {                    // 已存在 → 更新並搬到最前
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        if (items_.size() >= capacity_) {          // 容量滿 → 淘汰最後一個
            int oldKey = items_.back().first;
            items_.pop_back();
            pos_.erase(oldKey);
        }
        items_.emplace_front(key, value);
        pos_[key] = items_.begin();
    }

    // 由最近使用到最久未使用，方便觀察內部狀態
    vector<int> keysMostRecentFirst() const {
        vector<int> ks;
        ks.reserve(items_.size());
        for (const auto& e : items_) ks.push_back(e.first);
        return ks;
    }
};

int main() {
    // 最多保留 5 筆日誌
    RingBuffer<string> log(5);

    log.push("09:00 系統啟動");
    log.push("09:01 使用者登入");
    log.push("09:05 查詢資料庫");
    log.push("09:10 匯出報表");
    log.push("09:15 發送郵件");

    cout << "--- 5 筆日誌 ---" << endl;
    for (size_t i = 0; i < log.size(); i++) {
        cout << "  [" << i << "] " << log[i] << endl;
    }

    // 再加入 2 筆，最舊的 2 筆會被丟棄
    log.push("09:20 備份資料");
    log.push("09:25 系統更新");

    cout << "\n--- 新增 2 筆後 ---" << endl;
    for (size_t i = 0; i < log.size(); i++) {
        cout << "  [" << i << "] " << log[i] << endl;
    }

    cout << "\n最舊：" << log.oldest() << endl;
    cout << "最新：" << log.latest() << endl;
    cout << "已滿？ " << boolalpha << log.full()
         << "（size=" << log.size() << " / 上限 5）" << endl;

    // ========================================================================
    // 空緩衝區的安全存取
    // ========================================================================
    cout << "\n=== 空緩衝區的安全存取 ===" << endl;
    RingBuffer<string> emptyBuf(3);
    string out;
    cout << "空的 RingBuffer：empty() = " << emptyBuf.empty() << endl;
    cout << "tryLatest() 成功嗎？ " << emptyBuf.tryLatest(out)
         << "（false = 安全地回報失敗）" << endl;
    cout << "→ 直接呼叫 latest() 會對空 deque 呼叫 back()，那是 UB——" << endl;
    cout << "  不保證崩潰、也不保證任何特定值。所以要嘛先檢查 empty()，" << endl;
    cout << "  要嘛提供像 tryLatest() 這種不會 UB 的介面。" << endl;

    // ========================================================================
    // forEach：函式指標 vs 模板
    // ========================================================================
    cout << "\n=== forEach：函式指標 vs 模板 ===" << endl;
    cout << "函式指標版（只能接無捕獲的可呼叫物）：" << endl;
    log.forEach([](const string& s) { cout << "    · " << s << endl; });

    int lineNo = 0;                       // ← 要被捕獲的區域變數
    cout << "模板版（可接有捕獲的 lambda）：" << endl;
    log.forEachGeneric([&lineNo](const string& s) {
        cout << "    " << ++lineNo << ". " << s << endl;
    });
    cout << "→ 上面那個有捕獲的 lambda **無法**傳給 forEach（函式指標版），" << endl;
    cout << "  因為有捕獲的 lambda 不能轉換成函式指標。" << endl;
    cout << "  改用模板參數就能同時接受 lambda / functor / 函式指標。" << endl;

    // ========================================================================
    // LeetCode 146. LRU Cache
    // ========================================================================
    cout << "\n=== LeetCode 146. LRU Cache ===" << endl;
    LRUCache cache(2);                    // 容量 2
    cache.put(1, 1);
    cache.put(2, 2);
    cout << "put(1,1), put(2,2)  → 目前 key（最近使用優先）：";
    for (int k : cache.keysMostRecentFirst()) cout << k << " ";
    cout << endl;

    cout << "get(1) = " << cache.get(1) << "（命中，1 被搬到最前）" << endl;
    cout << "  目前 key：";
    for (int k : cache.keysMostRecentFirst()) cout << k << " ";
    cout << endl;

    cache.put(3, 3);                      // 容量滿 → 淘汰最久未使用的 key 2
    cout << "put(3,3) → 容量滿，淘汰最久未使用的 key 2" << endl;
    cout << "get(2) = " << cache.get(2) << "（-1 代表已被淘汰）" << endl;
    cout << "  目前 key：";
    for (int k : cache.keysMostRecentFirst()) cout << k << " ";
    cout << endl;

    cache.put(4, 4);                      // 再滿 → 淘汰 key 1
    cout << "put(4,4) → 淘汰 key 1" << endl;
    cout << "get(1) = " << cache.get(1) << "（已淘汰）" << endl;
    cout << "get(3) = " << cache.get(3) << endl;
    cout << "get(4) = " << cache.get(4) << endl;

    cout << "\n→ LRU 與 RingBuffer 是同一個家族：固定容量 + 自動淘汰。" << endl;
    cout << "  差別在淘汰規則：RingBuffer 依「加入時間」，LRU 依「最後使用時間」。" << endl;
    cout << "  也因為 LRU 要「把中間的元素搬到最新端」，deque 做不到 O(1)" << endl;
    cout << "  （它只強在兩端），所以標準解改用 list + unordered_map：" << endl;
    cout << "    list 的 splice 是 O(1) 且不使 iterator 失效，" << endl;
    cout << "    unordered_map 提供 O(1) 的 key → 節點位置查找。" << endl;
    cout << "  這正好標示出 deque 的能力邊界。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 28 課：deque 的實際應用場景3.cpp" -o demo28_3

// === 預期輸出 ===
// --- 5 筆日誌 ---
//   [0] 09:00 系統啟動
//   [1] 09:01 使用者登入
//   [2] 09:05 查詢資料庫
//   [3] 09:10 匯出報表
//   [4] 09:15 發送郵件
//
// --- 新增 2 筆後 ---
//   [0] 09:05 查詢資料庫
//   [1] 09:10 匯出報表
//   [2] 09:15 發送郵件
//   [3] 09:20 備份資料
//   [4] 09:25 系統更新
//
// 最舊：09:05 查詢資料庫
// 最新：09:25 系統更新
// 已滿？ true（size=5 / 上限 5）
//
// === 空緩衝區的安全存取 ===
// 空的 RingBuffer：empty() = true
// tryLatest() 成功嗎？ false（false = 安全地回報失敗）
// → 直接呼叫 latest() 會對空 deque 呼叫 back()，那是 UB——
//   不保證崩潰、也不保證任何特定值。所以要嘛先檢查 empty()，
//   要嘛提供像 tryLatest() 這種不會 UB 的介面。
//
// === forEach：函式指標 vs 模板 ===
// 函式指標版（只能接無捕獲的可呼叫物）：
//     · 09:05 查詢資料庫
//     · 09:10 匯出報表
//     · 09:15 發送郵件
//     · 09:20 備份資料
//     · 09:25 系統更新
// 模板版（可接有捕獲的 lambda）：
//     1. 09:05 查詢資料庫
//     2. 09:10 匯出報表
//     3. 09:15 發送郵件
//     4. 09:20 備份資料
//     5. 09:25 系統更新
// → 上面那個有捕獲的 lambda **無法**傳給 forEach（函式指標版），
//   因為有捕獲的 lambda 不能轉換成函式指標。
//   改用模板參數就能同時接受 lambda / functor / 函式指標。
//
// === LeetCode 146. LRU Cache ===
// put(1,1), put(2,2)  → 目前 key（最近使用優先）：2 1
// get(1) = 1（命中，1 被搬到最前）
//   目前 key：1 2
// put(3,3) → 容量滿，淘汰最久未使用的 key 2
// get(2) = -1（-1 代表已被淘汰）
//   目前 key：3 1
// put(4,4) → 淘汰 key 1
// get(1) = -1（已淘汰）
// get(3) = 3
// get(4) = 4
//
// → LRU 與 RingBuffer 是同一個家族：固定容量 + 自動淘汰。
//   差別在淘汰規則：RingBuffer 依「加入時間」，LRU 依「最後使用時間」。
//   也因為 LRU 要「把中間的元素搬到最新端」，deque 做不到 O(1)
//   （它只強在兩端），所以標準解改用 list + unordered_map：
//     list 的 splice 是 O(1) 且不使 iterator 失效，
//     unordered_map 提供 O(1) 的 key → 節點位置查找。
//   這正好標示出 deque 的能力邊界。
