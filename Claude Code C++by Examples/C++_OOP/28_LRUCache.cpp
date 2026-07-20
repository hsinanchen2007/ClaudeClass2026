/*=============================================================================
 * 檔名：28_LRUCache.cpp
 * 主題：綜合應用 - Leetcode 146. LRU Cache
 * 適合：學完前 27 篇，想把所有觀念串起來實戰一次的人
 *
 * 【課題介紹】
 *   LRU = Least Recently Used (最近最少使用)。
 *   LRU Cache 是工程中極常見的快取淘汰策略：當 cache 滿了，淘汰「最久沒用」的那個。
 *   實際應用：作業系統 page cache、資料庫 buffer、瀏覽器快取、Redis 等等。
 *
 * 【Leetcode 146 題目簡述】
 *   設計一個 LRUCache(capacity)：
 *     - get(key)        若 key 在 cache 內，回傳 value，否則回傳 -1。
 *                        被 get 的 key 算「最近使用過」。
 *     - put(key, value)  插入或更新 key 的值。若插入後超過 capacity，
 *                        把「最久沒用」的那個淘汰。
 *
 *   要求：所有操作都要 O(1) 平均時間。
 *
 * 【做法 (兩種資料結構搭配)】
 *
 *   1. 雙向鏈結串列 (doubly linked list)：用 std::list<std::pair<int,int>>
 *      - list 的 front 是「最近使用」，back 是「最久沒用」。
 *      - list 的 splice() 可以 O(1) 把某個 iterator 搬到別的位置。
 *
 *   2. 雜湊表 (hash map)：std::unordered_map<int, list_iterator>
 *      - O(1) 找到 key 對應的 list 節點。
 *
 *   流程：
 *     get(key):
 *       - 找不到 → return -1
 *       - 找到   → 把對應節點搬到 list 最前面，回傳 value
 *
 *     put(key, value):
 *       - 已存在 → 更新 value，搬到最前面
 *       - 不存在 → 在最前面插入；若超出 capacity，把 list.back() 砍掉，
 *                  並從 map 中也 erase 該 key
 *
 * 【這篇用到了前面哪些觀念？】
 *   - 第 3 篇 封裝：把 list 與 map 設成 private，外部只能透過 get / put 操作。
 *   - 第 4 篇 建構子：吃 capacity 作為初始化參數，存入成員。
 *   - 第 7 篇 初始化列表：用初始化列表設定 cap_。
 *   - 第 10 篇 const 成員：size() 寫成 const。
 *   - 第 23 篇 Rule of Zero：完全交給編譯器處理 copy/move/destruct。
 *   - 第 24/25 篇 模板與 STL：std::list<std::pair<int,int>>、unordered_map<int, ...>。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/container/list   (splice 詳細說明)
 *   https://en.cppreference.com/w/cpp/container/unordered_map
 *=============================================================================*/

/*
補充筆記：LRUCache
  - LRUCache 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - LRU Cache 結合 OOP 和資料結構：class 封裝容量、查找表、使用順序，外部只需要 get/put 介面。
  - 典型實作使用 unordered_map 做 O(1) key 查找，list 做 O(1) 移動節點到最前面；兩者配合才能達到題目要求。
  - list iterator 在 splice 移動節點時仍有效，這是 LRU 常用 std::list 的原因；vector 移動元素會使位置和 iterator 容易失效。
  - unordered_map 的 value 常保存 list iterator，代表 map 找到 key 後能直接定位到 list 節點。
  - 每次 get 成功都要更新最近使用順序，不只是 put 才更新；這是初學者最常漏掉的邏輯。
  - 容量為 0、重複 put 同一 key、get 不存在 key、put 後超過容量，都是 LRU 必測邊界。
  - 封裝上應讓內部 list/map 不被外部碰到，否則外部可能破壞「map iterator 必須指向 list 中正確節點」的不變條件。
  - 工作上 cache 還要考慮 thread safety、過期時間、記憶體上限和統計命中率；Leetcode 版本先專注資料結構不變條件即可。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】LRU Cache
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 手寫 LRU Cache 的資料結構怎麼選？為什麼不用 vector？
//     答：`std::list<pair<Key,Value>>`（雙向鏈結串列，維護使用順序、最近使用放最前）
//         ＋ `std::unordered_map<Key, list::iterator>`（O(1) 定位到節點）。
//         不用 vector 是因為：list 的 splice / erase 是 O(1) 且「不使 iterator 失效」，
//         而 vector 中間刪除是 O(n)，且插入／擴容會讓既有 iterator 全部失效 ——
//         偏偏我們的 map 存的就是 iterator。
//     追問：整體複雜度是 O(1) 嗎？（unordered_map 是「平均」O(1)，最壞情況
//           （大量碰撞）會退化成 O(n)；題目要求的也是平均 O(1)）
//
// 🔥 Q2. `std::list::splice()` 在這裡為什麼是關鍵？
//     答：它把節點從一個位置搬到另一個位置時「只改指標、不搬資料、不重新配置」，
//         所以是 O(1)，而且指向該元素的 iterator 保持有效 —— 這正是本檔
//         `lru_.splice(lru_.begin(), lru_, it->second);` 之後，map 裡存的
//         iterator 還能繼續用的原因。若換成 erase + push_front 就得同步更新 map。
//
// ⚠️ 陷阱. `get()` 只是讀取，應該不用動使用順序吧？
//     答：要動。LRU 的「U」是 Used，`get` 命中就代表這個 key「剛剛被使用」，必須
//         搬到最前面；只在 `put` 時更新順序會讓淘汰對象完全錯掉（本檔 main 的
//         測資 get(1) 之後 put(3,3) 淘汰的是 2 而不是 1，正是在驗證這件事）。
//     為什麼會錯：多數人把「更新資料結構」和「寫入操作」畫上等號，於是把 get 當成
//         純唯讀。這也是為什麼本檔的 `get` 不能宣告成 const 成員函式。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <list>
#include <unordered_map>
#include <utility>     // std::pair
#include <climits>     // INT_MAX
#include <string>

class LRUCache {
private:
    // list 節點型別：pair<key, value>
    using KV       = std::pair<int, int>;
    using ListIter = std::list<KV>::iterator;

    int                                cap_;       // 容量上限
    std::list<KV>                      lru_;       // 前端=最新，後端=最久沒用
    std::unordered_map<int, ListIter>  index_;     // key → list 中該節點的 iterator

public:
    explicit LRUCache(int capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = index_.find(key);
        if (it == index_.end()) return -1;          // 找不到

        // 把這個節點搬到 list 最前面 (代表「剛被使用」)
        // splice 把 [it->second, it->second+1) 移到 lru_.begin() — O(1) 不會 invalidate
        lru_.splice(lru_.begin(), lru_, it->second);
        return it->second->second;                  // 第二個 second 是 pair 的 value
    }

    void put(int key, int value) {
        auto it = index_.find(key);
        if (it != index_.end()) {
            // 已存在 → 更新 value、搬到前面
            it->second->second = value;
            lru_.splice(lru_.begin(), lru_, it->second);
            return;
        }

        // 不存在 → 若已滿，淘汰最久沒用的 (list 最後)
        if (static_cast<int>(lru_.size()) == cap_) {
            int oldKey = lru_.back().first;
            lru_.pop_back();
            index_.erase(oldKey);
        }

        // 在 list 最前面插入新節點，並更新 map
        lru_.push_front({key, value});
        index_[key] = lru_.begin();
    }

    // 為了示範用：列出目前 cache 內容 (從新到舊)
    void debugPrint() const {
        std::cout << "  cache (新→舊): ";
        for (const auto& [k, v] : lru_) std::cout << "[" << k << "=" << v << "] ";
        std::cout << "\n";
    }

    int size() const { return static_cast<int>(lru_.size()); }
};

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 460 - LFU Cache 的簡化版  (難度: hard)
// -----------------------------------------------------------------------------
// 題目簡述：LFU = Least Frequently Used，淘汰「使用頻率最低」(同頻時淘汰最久沒用)。
// 重點：與 LRU 用同樣的「容器組合」設計手法 — 多個 std::unordered_map / std::list 配合。
// 為求精簡，本實作不追求 O(1)，只示範類別設計與不變條件的維護。
class LFUCache {
private:
    int                            cap_;
    std::unordered_map<int, int>   kv_;          // key → value
    std::unordered_map<int, int>   freq_;        // key → 累計使用次數

public:
    explicit LFUCache(int capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = kv_.find(key);
        if (it == kv_.end()) return -1;
        ++freq_[key];                            // 命中：頻率 +1
        return it->second;
    }

    void put(int key, int value) {
        if (cap_ <= 0) return;
        if (kv_.count(key)) {
            kv_[key] = value;
            ++freq_[key];
            return;
        }
        if (static_cast<int>(kv_.size()) == cap_) {
            // 找頻率最小的 key (簡化：線性掃描)
            int victim = -1, minFreq = INT_MAX;
            for (const auto& [k, f] : freq_) {
                if (f < minFreq) { minFreq = f; victim = k; }
            }
            kv_.erase(victim);
            freq_.erase(victim);
        }
        kv_[key] = value;
        freq_[key] = 1;
    }

    int size() const { return static_cast<int>(kv_.size()); }
};

// -----------------------------------------------------------------------------
// 範例 3：日常實用 - TTLCache (時效快取)
// -----------------------------------------------------------------------------
// 工作上很常見：每筆紀錄帶過期時間 (TTL)，呼叫 get 時若過期就視為不存在。
class TTLCache {
private:
    // key → (value, expireAt)
    std::unordered_map<std::string, std::pair<std::string, long>> data_;

public:
    // now 代表當前時間 (秒)；put 把值存入，並計算 expireAt = now + ttl
    void put(const std::string& key, const std::string& value, long ttl, long now) {
        data_[key] = {value, now + ttl};
    }

    // 取出時若過期則返回空字串並順便清掉
    std::string get(const std::string& key, long now) {
        auto it = data_.find(key);
        if (it == data_.end()) return "";
        if (now > it->second.second) {
            data_.erase(it);                    // 已過期 → 自動清除
            return "";
        }
        return it->second.first;
    }
};

int main() {
    // 模擬 Leetcode 範例：
    //   ["LRUCache","put","put","get","put","get","put","get","get","get"]
    //   [[2],[1,1],[2,2],[1],[3,3],[2],[4,4],[1],[3],[4]]
    //   預期輸出: [null, null, null, 1, null, -1, null, -1, 3, 4]
    LRUCache c(2);

    c.put(1, 1); c.debugPrint();    // [1=1]
    c.put(2, 2); c.debugPrint();    // [2=2] [1=1]
    std::cout << "get(1) = " << c.get(1) << std::endl;        // 1
    c.debugPrint();                                            // [1=1] [2=2]
    c.put(3, 3); c.debugPrint();    // 容量滿 → 淘汰 2 → [3=3] [1=1]
    std::cout << "get(2) = " << c.get(2) << std::endl;        // -1
    c.put(4, 4); c.debugPrint();    // 容量滿 → 淘汰 1 → [4=4] [3=3]
    std::cout << "get(1) = " << c.get(1) << std::endl;        // -1
    std::cout << "get(3) = " << c.get(3) << std::endl;        // 3
    std::cout << "get(4) = " << c.get(4) << std::endl;        // 4
    c.debugPrint();

    std::cout << "===== Leetcode 460 LFU Cache (簡化版) =====" << std::endl;
    LFUCache lfu(2);
    lfu.put(1, 1);          // freq[1]=1
    lfu.put(2, 2);          // freq[2]=1
    std::cout << "get(1) = " << lfu.get(1) << std::endl;       // 1; freq[1]=2
    lfu.put(3, 3);          // 容量滿，2 的 freq 最小 → 淘汰 2
    std::cout << "get(2) = " << lfu.get(2) << std::endl;       // -1
    std::cout << "get(3) = " << lfu.get(3) << std::endl;       // 3
    std::cout << "size   = " << lfu.size() << std::endl;       // 2

    std::cout << "===== 日常實用：TTLCache =====" << std::endl;
    TTLCache tc;
    tc.put("session-1", "Alice", /*ttl=*/10, /*now=*/100);     // 100 秒時存入，10 秒後過期
    std::cout << "t=105 get = '" << tc.get("session-1", 105) << "' (未過期)\n";
    std::cout << "t=115 get = '" << tc.get("session-1", 115) << "' (已過期)\n";
    return 0;
}

/* 預期輸出：
 *   cache (新→舊): [1=1]
 *   cache (新→舊): [2=2] [1=1]
 * get(1) = 1
 *   cache (新→舊): [1=1] [2=2]
 *   cache (新→舊): [3=3] [1=1]
 * get(2) = -1
 *   cache (新→舊): [4=4] [3=3]
 * get(1) = -1
 * get(3) = 3
 * get(4) = 4
 *   cache (新→舊): [4=4] [3=3]
 * ===== Leetcode 460 LFU Cache (簡化版) =====
 * get(1) = 1
 * get(2) = -1
 * get(3) = 3
 * size   = 2
 * ===== 日常實用：TTLCache =====
 * t=105 get = 'Alice' (未過期)
 * t=115 get = '' (已過期)
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. LRU Cache 是「list (順序) + hash map (索引)」的經典組合。
 *   2. std::list::splice() 能 O(1) 把節點搬到別的位置，且不讓 iterator 失效。
 *   3. unordered_map 提供 O(1) 平均的 key → iterator 查找。
 *   4. 整個類別符合 Rule of Zero — 因為內部用的都是 RAII 容器，
 *      不需要自己寫 copy/move/destructor。
 *
 * 【完賽小結】
 *   恭喜你！28 個檔案讀完，你已經掌握：
 *     - OOP 三大支柱：封裝、繼承、多型
 *     - 物件生命週期：建構/解構、複製/賦值、移動
 *     - 現代 C++ 利器：RAII、智慧指標、模板、move semantics
 *     - 常見設計模式：Singleton、Factory
 *     - 與 Leetcode 實戰結合
 *
 *   接下來建議的延伸方向：
 *     - 多重繼承、虛擬繼承 (進階繼承)
 *     - 模板進階：variadic template、CRTP、概念 (concepts, C++20)
 *     - C++ 標準函式庫深入：algorithm, ranges (C++20), iterator
 *     - 多執行緒：std::thread, std::mutex, std::async
 *     - 設計模式：Observer, Strategy, Decorator, MVC...
 *
 *   每天寫一點，持之以恆比一次衝刺有用得多。加油！
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 28_LRUCache.cpp -o 28_LRUCache

// === 預期輸出 ===
//   cache (新→舊): [1=1]
//   cache (新→舊): [2=2] [1=1]
// get(1) = 1
//   cache (新→舊): [1=1] [2=2]
//   cache (新→舊): [3=3] [1=1]
// get(2) = -1
//   cache (新→舊): [4=4] [3=3]
// get(1) = -1
// get(3) = 3
// get(4) = 4
//   cache (新→舊): [4=4] [3=3]
// ===== Leetcode 460 LFU Cache (簡化版) =====
// get(1) = 1
// get(2) = -1
// get(3) = 3
// size   = 2
// ===== 日常實用：TTLCache =====
// t=105 get = 'Alice' (未過期)
// t=115 get = '' (已過期)
