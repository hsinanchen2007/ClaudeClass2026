// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定5.cpp  —  非阻塞快取存取模式
// =============================================================================
//
// 【主題資訊 Information】
//   bool std::mutex::try_lock();                                    // C++11，<mutex>
//   本檔模式：對同一份資料同時提供【阻塞】與【非阻塞】兩種介面
//       bool tryGet(key, out);   // 拿不到鎖就放棄 → 呼叫端走替代路徑
//       T    get(key);           // 一定要拿到，會等
//   標頭檔：<mutex>、<map>、<string>
//   複雜度：std::map 查找 O(log N)；try_lock 為一次原子操作。
//
// 【詳細解釋 Explanation】
//
// 【1. 「非阻塞快取」的核心價值：快取是最佳化，不是正確性依賴】
//   快取的定位決定了它該用哪種鎖策略。
//   如果快取只是【加速】——查不到就回源、拿不到鎖就直接回源——
//   那麼「為了讀快取而阻塞」在邏輯上就很荒謬：
//   本來是為了變快才加的快取，結果卻成了讓請求排隊的瓶頸。
//   → 所以正確的介面設計是：讀快取用 try_lock，失敗就當作 miss。
//   反之，如果資料【只存在於】這份結構裡（它其實是資料本身而非快取），
//   那就必須用阻塞的 lock()，因為沒有替代路徑可走。
//
// 【2. 這個 API 有一個真實的設計缺陷：兩種失敗被混為一談】
//   原始的 tryGet 回傳 false 時，呼叫端【無法區分】兩件事：
//       (a) 鎖被別人持有 → 這次沒查成，但快取裡【可能有】這筆資料
//       (b) 鎖拿到了，但 key 不存在 → 快取裡【確定沒有】這筆資料
//   為什麼這個差別重要？因為後續動作完全不同：
//       (a) 應該「用預設值先擋著」或「稍後重試」——不該去回源，
//           因為回源是昂貴的，而資料很可能就在快取裡；
//       (b) 應該「立刻回源查資料庫並回填快取」。
//   把兩者都回傳 false，呼叫端只能亂猜，於是要嘛過度回源（打爆資料庫），
//   要嘛該回源時沒回源（永遠拿不到新資料）。
//   → 本檔保留原始的 tryGet 作為對照，另外提供改良版
//     tryGetEx() 回傳三態 enum，示範正確的 API 設計。
//
// 【3. get() 裡的 cache[key] 是個真正的地雷】
//   原始程式碼：
//       std::string get(const std::string& key) {
//           mtx.lock();
//           std::string result = cache[key];   // ⚠️ 這裡
//           mtx.unlock();
//           return result;
//       }
//   std::map::operator[] 的語意是「若 key 不存在，就【插入】一個
//   預設建構的值並回傳它的參考」。所以這個「讀取」函式其實會【修改】map：
//     * 每次查詢不存在的 key，map 就無聲地長大一筆空字串；
//     * 這是典型的記憶體洩漏來源（快取被不存在的 key 撐爆）；
//     * 若這是個 const 方法，根本無法通過編譯（operator[] 非 const）。
//   正解是用 find()：
//       auto it = cache.find(key);
//       return (it != cache.end()) ? it->second : "";
//   → 本檔已修正此問題，並保留註解說明。
//
// 【4. 為什麼 mutex 要用 mutable】
//   若把 get / tryGet 宣告成 const（它們在語意上確實不修改資料），
//   就不能對非 mutable 的成員 mutex 呼叫 lock()——因為 lock() 是非 const 的。
//   標準做法是把 mutex 宣告成 mutable：
//       mutable std::mutex mtx;
//   這正是 mutable 最正當的用途之一：
//   「這個成員的改變不影響物件對外的邏輯狀態」。
//
// 【概念補充 Concept Deep Dive】
//   * 這個模式的進階版是【讀寫鎖】std::shared_mutex（C++17）：
//       std::shared_lock lock(mtx);   // 多個讀取者可同時進入
//       std::unique_lock lock(mtx);   // 寫入者獨佔
//     快取正是「讀遠多於寫」的典型場景，shared_mutex 讓所有讀取者
//     不必互相排擠。但要注意：shared_mutex 的常數成本比 mutex 高，
//     讀取的臨界區段很短時（例如只是一次 map 查找），反而可能更慢。
//   * 再進階則是完全無鎖的做法：把整份快取放在一個 shared_ptr 裡，
//     更新時複製一份新的再原子地換掉指標（copy-on-write）。
//     讀取端完全不需要鎖，代價是每次更新都要複製整份資料。
//   * 本檔把輸出改成「統計不變量」而非逐行列印：
//     原始版本讓 3 條執行緒直接 cout，輸出順序每次都不同，無法驗證。
//     改成收集結果後統一輸出，才能寫出可比對的預期輸出。
//
// 【注意事項 Pay Attention】
//   1. std::map::operator[] 在 key 不存在時會【插入】元素，
//      不可用於唯讀查詢。查詢請用 find()。
//   2. try_lock 失敗與「查無此 key」是兩件事，API 應該分開回報。
//   3. const 方法要鎖，成員 mutex 必須宣告成 mutable。
//   4. try_lock 成功後【必須】unlock；本檔的 tryGet 有兩條 return 路徑
//      各自解鎖，這種寫法極易出錯，正式程式請用 unique_lock + try_to_lock。
//   5. 「讀多寫少」可考慮 std::shared_mutex（C++17），
//      但臨界區段極短時未必更快，要實測。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】非阻塞快取
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個唯讀的查詢函式裡寫 return cache[key]; 有什麼問題？
//     答：std::map / unordered_map 的 operator[] 在 key 不存在時會
//         【插入】一個預設建構的值。所以這個「查詢」其實會修改容器：
//         每查一次不存在的 key，容器就多一筆垃圾資料，
//         長期下來是記憶體洩漏；而且該函式無法宣告成 const。
//         正解是用 find()，或 C++20 起 map 也有 contains()。
//     追問：那 at() 呢？→ at() 不會插入，但查無 key 會丟 std::out_of_range。
//           在「查不到是正常情況」的快取場景用例外流程控制太貴，
//           仍以 find() 為佳。
//
// 🔥 Q2. tryGet() 回傳 false，呼叫端應該去查資料庫嗎？
//     答：這正是這個 API 的缺陷——無法判斷。false 同時代表
//         「鎖忙碌（資料可能就在快取裡）」與「查無此 key（確定要回源）」。
//         前者不該回源（浪費且可能打爆資料庫），後者必須回源。
//         正確設計是回傳三態：Hit / Miss / Busy，讓呼叫端能分別處理。
//     追問：Busy 時該怎麼辦？→ 短暫退避後重試，或直接使用上一次的
//           舊值（stale-while-revalidate），而不是把壓力轉嫁給資料庫。
//
// ⚠️ 陷阱. 「快取讀取用 try_lock，拿不到就回源查資料庫」——
//        這樣不是既不阻塞又保證拿到資料嗎？
//     答：不阻塞是對的，但這個策略在【高競爭】時會反過來打垮資料庫。
//         想像熱門資料被大量請求同時存取：鎖的競爭愈激烈，
//         try_lock 失敗率愈高 → 愈多請求跑去回源 → 資料庫壓力暴增
//         → 回源變慢 → 持有鎖的時間變長 → try_lock 更容易失敗……
//         形成正回饋的雪崩。
//     為什麼會錯：只看到「單次請求不會被阻塞」這個局部好處，
//         沒看到「失敗時的替代路徑成本遠高於等待成本」這個全域效果。
//         正解是區分 Busy 與 Miss：Busy 就短暫退避重試（資料很可能就在
//         快取裡），只有真正的 Miss 才回源。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   題目：不使用內建雜湊表，自行設計一個 HashSet，
//         支援 add(key) / contains(key) / remove(key)。
//   為什麼用到本主題：705 原題是單執行緒的，但「自己設計一個查找結構」
//         正是本檔快取類別在做的事。這裡實作成【執行緒安全】版本，
//         恰好示範本檔的兩個重點：
//           (1) contains() 是唯讀查詢 → 用 find() 而非 operator[]，
//               否則查詢會意外插入元素（本檔第 3 點的地雷）；
//           (2) const 方法要鎖 → 成員 mutex 必須是 mutable。
//         另外提供 tryContains() 非阻塞版本，對應本檔的主題。
//   實作：鏈結串列法（separate chaining），桶數固定 769（質數，減少碰撞）。
// -----------------------------------------------------------------------------
class MyHashSet {
private:
    static constexpr int kBuckets = 769;

    mutable std::mutex mtx_;                       // mutable：const 方法也要鎖
    std::vector<std::vector<int>> buckets_;

    static int hash(int key) { return key % kBuckets; }

public:
    MyHashSet() : buckets_(kBuckets) {}

    void add(int key) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto& bucket = buckets_[hash(key)];
        for (int k : bucket) {
            if (k == key) return;                  // 已存在，不重複加入
        }
        bucket.push_back(key);
    }

    void remove(int key) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto& bucket = buckets_[hash(key)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (*it == key) { bucket.erase(it); return; }
        }
    }

    // 唯讀查詢：不修改任何內容，故可宣告成 const
    bool contains(int key) const {
        std::lock_guard<std::mutex> lock(mtx_);    // 靠 mutable mutex 才能鎖
        const auto& bucket = buckets_[hash(key)];
        for (int k : bucket) {
            if (k == key) return true;
        }
        return false;
    }

    // 非阻塞版本：對應本檔主題。三態回報，不把 Busy 與 Miss 混為一談。
    enum class Probe { Present, Absent, Busy };

    Probe tryContains(int key) const {
        if (!mtx_.try_lock()) return Probe::Busy;

        const auto& bucket = buckets_[hash(key)];
        bool found = false;
        for (int k : bucket) {
            if (k == key) { found = true; break; }
        }
        mtx_.unlock();
        return found ? Probe::Present : Probe::Absent;
    }
};

// -----------------------------------------------------------------------------
// 課程原始類別（已修正 operator[] 的地雷，並補上三態版本）
// -----------------------------------------------------------------------------
class NonBlockingCache {
public:
    // 三態結果：把「鎖忙碌」與「查無此 key」分開回報
    enum class Result { Hit, Miss, Busy };

private:
    std::map<std::string, std::string> cache;
    mutable std::mutex mtx;

public:
    // 課程原始介面：嘗試從快取獲取值（非阻塞）
    // ⚠️ 缺陷：回傳 false 時無法區分「鎖忙」與「查無此 key」
    bool tryGet(const std::string& key, std::string& value) {
        if (mtx.try_lock()) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                value = it->second;
                mtx.unlock();
                return true;
            }
            mtx.unlock();
        }
        // 無法獲取鎖或找不到 key —— 兩種情況被混為一談
        return false;
    }

    // 改良版：三態回報，呼叫端可以做出正確決策
    Result tryGetEx(const std::string& key, std::string& value) const {
        if (!mtx.try_lock()) return Result::Busy;   // 鎖忙 → 別急著回源

        auto it = cache.find(key);
        if (it == cache.end()) {
            mtx.unlock();
            return Result::Miss;                    // 確定沒有 → 該回源
        }
        value = it->second;
        mtx.unlock();
        return Result::Hit;
    }

    // 嘗試設定值（非阻塞）
    bool trySet(const std::string& key, const std::string& value) {
        if (mtx.try_lock()) {
            cache[key] = value;                     // 這裡是「寫入」，用 [] 正確
            mtx.unlock();
            return true;
        }
        return false;
    }

    // 阻塞版本（保證成功）
    void set(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx);
        cache[key] = value;
    }

    // ⚠️ 原始版本這裡寫 cache[key]，那會在 key 不存在時【插入】一筆空字串，
    //    讓一個「唯讀查詢」無聲地把快取撐大，而且無法宣告成 const。
    //    已改用 find()。
    std::string get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = cache.find(key);
        return (it != cache.end()) ? it->second : std::string{};
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return cache.size();
    }
};

int main() {
    std::cout << "=== operator[] 的地雷: 唯讀查詢竟然會插入元素 ===" << std::endl;
    {
        std::map<std::string, std::string> m;
        m["exists"] = "value";
        std::cout << "初始筆數: " << m.size() << std::endl;

        // 用 operator[] 查詢三個不存在的 key
        for (const char* k : {"ghost1", "ghost2", "ghost3"}) {
            volatile auto len = m[k].size();   // 「只是查一下」
            (void)len;
        }
        std::cout << "用 operator[] 查了 3 個不存在的 key 之後: "
                  << m.size() << " 筆  ← 憑空多了 3 筆空字串！" << std::endl;

        // 用 find() 查詢則不會插入
        std::map<std::string, std::string> m2;
        m2["exists"] = "value";
        for (const char* k : {"ghost1", "ghost2", "ghost3"}) {
            auto it = m2.find(k);
            (void)it;
        }
        std::cout << "改用 find() 查同樣 3 個 key 之後:      "
                  << m2.size() << " 筆  ← 正確，沒有副作用" << std::endl;
    }

    std::cout << "\n=== 課程示範: 非阻塞讀取 vs 阻塞寫入 ===" << std::endl;
    {
        NonBlockingCache cache;
        cache.set("data", "initial");

        std::atomic<int> hits{0}, misses{0}, busy{0};

        // writer：持續更新快取（阻塞寫入，保證成功）
        std::thread writer([&cache]() {
            for (int i = 0; i < 50; ++i) {
                cache.set("data", "value_" + std::to_string(i));
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        });

        // 兩條 reader：非阻塞讀取，用三態結果分類統計
        std::vector<std::thread> readers;
        for (int r = 0; r < 2; ++r) {
            readers.emplace_back([&cache, &hits, &misses, &busy]() {
                std::string value;
                for (int i = 0; i < 500; ++i) {
                    switch (cache.tryGetEx("data", value)) {
                        case NonBlockingCache::Result::Hit:  hits.fetch_add(1);   break;
                        case NonBlockingCache::Result::Miss: misses.fetch_add(1); break;
                        case NonBlockingCache::Result::Busy: busy.fetch_add(1);   break;
                    }
                }
            });
        }

        writer.join();
        for (auto& t : readers) t.join();

        std::cout << "reader 總查詢次數: " << (hits + misses + busy)
                  << "  (2 條 × 500 = 1000)" << std::endl;
        std::cout << "Miss 次數: " << misses.load()
                  << "  (必須是 0——key \"data\" 從頭到尾都存在)" << std::endl;
        std::cout << "Hit + Busy = " << (hits + busy)
                  << "  (剩下的全部落在這兩類)" << std::endl;
        std::cout << "註: Hit 與 Busy 各佔多少完全取決於排程，"
                     "每次執行都不同，故不列出" << std::endl;
        std::cout << "重點: Busy 不等於 Miss——若混為一談，"
                     "呼叫端會誤以為快取沒資料而去回源" << std::endl;
    }

    std::cout << "\n=== LeetCode 705. Design HashSet ===" << std::endl;
    {
        MyHashSet set;
        set.add(1);
        set.add(2);
        std::cout << "contains(1) = " << (set.contains(1) ? "true" : "false")
                  << "  (預期 true)" << std::endl;
        std::cout << "contains(3) = " << (set.contains(3) ? "true" : "false")
                  << "  (預期 false)" << std::endl;
        set.add(2);                     // 重複加入，不應改變內容
        std::cout << "contains(2) = " << (set.contains(2) ? "true" : "false")
                  << "  (預期 true)" << std::endl;
        set.remove(2);
        std::cout << "remove(2) 後 contains(2) = " << (set.contains(2) ? "true" : "false")
                  << "  (預期 false)" << std::endl;

        // 多執行緒壓測：8 條執行緒各加入 1000 個相異的 key
        MyHashSet shared;
        std::vector<std::thread> workers;
        for (int t = 0; t < 8; ++t) {
            workers.emplace_back([&shared, t]() {
                for (int i = 0; i < 1000; ++i) {
                    shared.add(t * 1000 + i);
                }
            });
        }
        for (auto& t : workers) t.join();

        int found = 0;
        for (int t = 0; t < 8; ++t) {
            for (int i = 0; i < 1000; ++i) {
                if (shared.contains(t * 1000 + i)) ++found;
            }
        }
        std::cout << "8 執行緒各加入 1000 個 key，可查到: " << found
                  << "  (必須是 8000，一個都不能掉)" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定5.cpp' -o non_blocking_cache

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 「Hit 與 Busy 各佔多少」完全取決於排程，【每次執行都不同】，
//      故只輸出兩者的總和與 Miss 數。
//   2. Miss = 0 是【確定】的：key "data" 在讀取開始前就已寫入，
//      之後只被覆寫、從未刪除。
//   3. operator[] 的示範與 LeetCode 705 的結果都是完全確定的。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === operator[] 的地雷: 唯讀查詢竟然會插入元素 ===
// 初始筆數: 1
// 用 operator[] 查了 3 個不存在的 key 之後: 4 筆  ← 憑空多了 3 筆空字串！
// 改用 find() 查同樣 3 個 key 之後:      1 筆  ← 正確，沒有副作用
//
// === 課程示範: 非阻塞讀取 vs 阻塞寫入 ===
// reader 總查詢次數: 1000  (2 條 × 500 = 1000)
// Miss 次數: 0  (必須是 0——key "data" 從頭到尾都存在)
// Hit + Busy = 1000  (剩下的全部落在這兩類)
// 註: Hit 與 Busy 各佔多少完全取決於排程，每次執行都不同，故不列出
// 重點: Busy 不等於 Miss——若混為一談，呼叫端會誤以為快取沒資料而去回源
//
// === LeetCode 705. Design HashSet ===
// contains(1) = true  (預期 true)
// contains(3) = false  (預期 false)
// contains(2) = true  (預期 true)
// remove(2) 後 contains(2) = false  (預期 false)
// 8 執行緒各加入 1000 個 key，可查到: 8000  (必須是 8000，一個都不能掉)
