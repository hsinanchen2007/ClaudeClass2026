// =============================================================================
//  課程 5.1：stdmutex 基本操作3.cpp  —  用一把鎖保護「多個必須成套的操作」
// =============================================================================
//
// 【主題資訊 Information】
//   class std::mutex;                                       // C++11，<mutex>
//       void lock();                                        // 阻塞直到取得
//       bool try_lock();                                    // 非阻塞
//       void unlock();
//   標頭檔：<mutex>
//   複雜度：無競爭時是一次原子操作（本機 x86-64 上是 lock cmpxchg 等級，
//           約十幾到數十奈秒）；有競爭時可能進入 futex 系統呼叫，微秒級。
//   關鍵性質：不可複製、不可移動、【不可重入】（同一執行緒重複 lock 是 UB）。
//
// 【詳細解釋 Explanation】
//
// 【1. 臨界區段的真正單位是「不變量」，不是「一行程式碼」】
//   初學者常以為「加鎖是為了保護某個變數」。更準確的說法是：
//   加鎖是為了保護某個【不變量（invariant）】在被破壞到被修復之間不被別人看見。
//   本檔的不變量是：「印出來的 Size，必須等於印出來的 value 被加入之後的筆數」。
//   要維持它，push_back 與兩次輸出就必須是一個【不可分割的整體】：
//       data.push_back(value);                  // 破壞了「印出的內容尚未反映」
//       std::cout << "Added: " << value;
//       std::cout << ", Size: " << data.size(); // 修復：印出一致的快照
//   只要中間有任何一刻放開鎖，另一條執行緒就可能插進來 push_back，
//   於是印出的 value 與 Size 對不起來——即使每一個個別操作都有鎖保護。
//
// 【2. 為什麼「每個操作各鎖一次」不等於「整段有鎖」】
//   下面這種寫法非常常見，而且【是錯的】：
//       mtx.lock(); data.push_back(value);   mtx.unlock();
//       mtx.lock(); std::cout << data.size(); mtx.unlock();
//   每一步都有鎖，沒有 data race，程式不會壞掉——
//   但兩段之間有一個空隙，別的執行緒可以在那裡插入自己的 push_back。
//   這叫 check-then-act / TOCTOU 類的錯誤：
//   「個別操作安全」與「組合起來仍然正確」是兩件完全不同的事。
//   這也是為什麼「把每個 method 都加鎖」的 thread-safe class
//   並不會自動讓使用它的程式碼變成 thread-safe。
//
// 【3. std::cout 為什麼也要放在鎖裡】
//   std::cout << a << b << c 是三次獨立的函式呼叫。
//   C++11 之後標準保證對同步化的標準串流並行操作不會造成資料損毀
//   （不是 UB），但【完全不保證不會交錯】。
//   沒有鎖的話，很容易看到這種畫面：
//       Added: 0Added: 100, Size: , Size: 21
//   把輸出納入同一個臨界區段，才能保證「一行是一行」。
//
// 【4. lock()/unlock() 是本課的教學形式，不是推薦寫法】
//   手動配對 lock/unlock 有三個實際風險：早期 return、拋出例外、
//   以及多人維護時新增的分支忘了解鎖。任何一個都會讓鎖永遠不被釋放。
//   正式程式碼應一律使用 RAII：std::lock_guard（最輕）、
//   std::unique_lock（需要延遲/手動控制時）、std::scoped_lock（多把鎖，C++17）。
//   本檔保留手寫 lock/unlock，是為了讓「臨界區段的邊界」在視覺上明確。
//
// 【概念補充 Concept Deep Dive】
//   * 鎖保護的是「約定」，不是記憶體。編譯器與硬體不知道 mtx 跟 data 有關係；
//     是【所有存取 data 的程式碼都先取得 mtx】這個紀律建立了保護。
//     只要有一處漏掉，整個保護就破功——這是 data race 最常見的來源。
//   * mutex 提供的記憶體順序保證：unlock() 是 release 操作，
//     lock() 是 acquire 操作。因此 A 執行緒在 unlock 前做的所有寫入，
//     對之後成功 lock 到同一把鎖的 B 執行緒都可見。
//     這就是為什麼有鎖之後不需要再對 data 加 volatile 或 atomic。
//   * 臨界區段內做 I/O（本檔的 cout）在教學上沒問題，但在效能敏感的
//     程式裡是反模式：I/O 可能阻塞數毫秒，期間所有其他執行緒都卡在鎖上。
//     實務做法是「在鎖內組好字串，出鎖後再輸出」。
//
// 【注意事項 Pay Attention】
//   1. 每個 lock() 都必須有配對的 unlock()；正式程式請用 lock_guard 保證。
//   2. std::mutex 不可重入：同一執行緒對它 lock 兩次是【未定義行為】。
//   3. 對未持有的 mutex 呼叫 unlock() 也是【未定義行為】。
//   4. 「每個函式都加鎖」不保證「組合起來的操作」正確（見上面第 2 點）。
//   5. 臨界區段內避免 I/O、避免呼叫可能再次取鎖的函式、避免 sleep。
//   6. 多執行緒的執行【順序】即使有鎖也不確定；鎖保證的是互斥與可見性，
//      不是公平性，更不是先來先服務（std::mutex 明確不保證公平）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 mutex 保護複合操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個類別的每個 public method 內部都加了 lock_guard，
//        這個類別是不是就 thread-safe 了？
//     答：它保證了「單一 method 呼叫」的原子性，但【不保證】使用它的程式碼正確。
//         例如 if (!q.empty()) x = q.pop();——empty() 與 pop() 各自有鎖，
//         中間卻有空隙，另一條執行緒可能把最後一個元素取走，
//         於是 pop() 對空佇列執行。
//     追問：那要怎麼修？→ 提供組合好的原子介面（例如 bool tryPop(T& out)），
//           把「檢查」與「動作」放進同一個臨界區段，
//           而不是要求呼叫端自己在外面加鎖。
//
// 🔥 Q2. 把 lock() / unlock() 改成 std::lock_guard，除了少打字還有什麼好處？
//     答：例外安全與所有 return 路徑的正確性。函式中途 throw 或 return 時，
//         lock_guard 的解構函式仍會執行 → 保證解鎖；
//         手寫 unlock() 則會被整段跳過，鎖永遠不釋放，
//         之後所有想取這把鎖的執行緒都會永久阻塞。
//     追問：lock_guard 和 unique_lock 怎麼選？→ 預設用 lock_guard（更輕，
//           沒有記錄「是否持有」的旗標）；需要延遲鎖定、中途 unlock、
//           轉移所有權、或搭配 condition_variable 時才用 unique_lock。
//
// ⚠️ 陷阱. 兩條執行緒各自有鎖地 push_back，最後 data.size() 一定是 10。
//        那為什麼印出來的 Size 序列不是 1,2,3,...,10 的固定順序？
//     答：Size 序列【確實】是 1..10 的一個排列（每次 push_back 後印一次，
//         且都在鎖內），但哪條執行緒先搶到鎖是排程決定的，
//         所以「哪個 value 對到哪個 Size」每次執行都不同。
//     為什麼會錯：多數人把「互斥」誤解成「有順序」。
//         mutex 只保證同一時間只有一個執行緒在臨界區段內，
//         【完全不保證】誰先誰後；std::mutex 甚至不保證公平性，
//         同一條執行緒連續搶到好幾次是完全合法的。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

std::vector<int> data;
std::mutex mtx;

// 課程原始示範：三個操作必須作為一個整體執行。
// 為了讓輸出可驗證，這裡把「輸出」改成「寫進 log 向量」——
// 臨界區段的結構完全不變，只是把 std::cout 換成 pushback 到 logLines。
std::vector<std::string> logLines;

void addAndPrint(int value) {
    mtx.lock();

    // 以下三個操作必須作為一個整體執行
    data.push_back(value);                       // 操作 1：新增元素
    std::ostringstream oss;
    oss << "Added: " << value;                   // 操作 2：組出訊息
    oss << ", Size: " << data.size();            // 操作 3：讀出目前大小
    logLines.push_back(oss.str());

    mtx.unlock();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 146. LRU Cache
//   題目：設計一個容量固定的 LRU（Least Recently Used）快取，
//         get(key) 與 put(key, value) 都要 O(1)。
//   為什麼用到本主題：這題是「多個操作必須成套」的教科書案例。
//         get() 內部其實做了三件事——查表、把該節點搬到 list 最前面、回傳值；
//         put() 更是查表 + 更新 or 插入 + 可能淘汰尾端。
//         只要中間放開鎖，另一條執行緒就可能看到
//         「map 裡有這個 key，但 list 裡的節點已被淘汰」的破碎狀態。
//         所以整個 get()/put() 必須在同一個臨界區段內完成，
//         而不是對 map 和 list 各加一把鎖。
//   註：LeetCode 原題是單執行緒的，這裡刻意加上 mutex 做成 thread-safe 版本，
//       正是本課要示範的重點。
// -----------------------------------------------------------------------------
class LRUCache {
private:
    using Entry = std::pair<int, int>;                    // (key, value)

    mutable std::mutex mtx_;                              // mutable：const 方法也要鎖
    std::size_t capacity_;
    std::list<Entry> order_;                              // 前端 = 最近使用
    std::unordered_map<int, std::list<Entry>::iterator> index_;

public:
    explicit LRUCache(std::size_t capacity) : capacity_(capacity) {}

    int get(int key) {
        std::lock_guard<std::mutex> lock(mtx_);           // 整段成套，不中途放鎖

        auto it = index_.find(key);
        if (it == index_.end()) return -1;

        // 命中：把節點搬到最前面（splice 不會使 iterator 失效）
        order_.splice(order_.begin(), order_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        std::lock_guard<std::mutex> lock(mtx_);

        auto it = index_.find(key);
        if (it != index_.end()) {
            it->second->second = value;                   // 更新既有值
            order_.splice(order_.begin(), order_, it->second);
            return;
        }

        if (index_.size() == capacity_) {                 // 容量滿 → 淘汰最久未用
            const int lruKey = order_.back().first;
            order_.pop_back();
            index_.erase(lruKey);
        }

        order_.emplace_front(key, value);
        index_[key] = order_.begin();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return index_.size();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】電商下單：扣庫存 + 產生出貨單，必須是一個整體
//   情境：多個下單執行緒同時處理同一件商品。
//         「檢查庫存是否足夠」與「實際扣減並產生出貨單」若拆成兩個臨界區段，
//         就會出現超賣：兩條執行緒都看到「剩 1 件」，然後都扣，變成 -1。
//   這裡示範正確做法：檢查與動作在同一把鎖內完成，
//         並回傳明確的結果（成功 / 庫存不足），而不是讓呼叫端自己先問再做。
// -----------------------------------------------------------------------------
class InventoryService {
private:
    std::mutex mtx_;
    int stock_;
    int shipmentSeq_ = 0;
    std::vector<std::string> shipments_;

public:
    explicit InventoryService(int stock) : stock_(stock) {}

    // 回傳出貨單號；庫存不足時回傳空字串。
    // 關鍵：check（庫存夠嗎）與 act（扣庫存 + 開單）在同一個臨界區段。
    std::string placeOrder(int quantity) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (stock_ < quantity) {
            return "";                                    // 庫存不足，不做任何修改
        }

        stock_ -= quantity;                               // 動作 1：扣庫存
        ++shipmentSeq_;                                   // 動作 2：取單號
        std::ostringstream oss;
        oss << "SHIP-" << shipmentSeq_;
        shipments_.push_back(oss.str());                  // 動作 3：登記出貨單
        return oss.str();
    }

    int stock() {
        std::lock_guard<std::mutex> lock(mtx_);
        return stock_;
    }

    std::size_t shipmentCount() {
        std::lock_guard<std::mutex> lock(mtx_);
        return shipments_.size();
    }
};

int main() {
    // ── 課程原始示範：兩條執行緒各加 5 筆 ──
    std::thread t1([]() {
        for (int i = 0; i < 5; ++i) {
            addAndPrint(i);
        }
    });

    std::thread t2([]() {
        for (int i = 100; i < 105; ++i) {
            addAndPrint(i);
        }
    });

    t1.join();
    t2.join();

    std::cout << "=== 課程 5.1 示範: 保護多個相關操作 ===" << std::endl;
    std::cout << "共記錄 " << logLines.size() << " 行 log" << std::endl;

    // 驗證 1：每一行都是完整未被切斷的格式（沒有交錯混雜）
    bool allWellFormed = true;
    std::vector<std::size_t> sizes;
    for (const auto& line : logLines) {
        const auto posAdded = line.find("Added: ");
        const auto posSize  = line.find(", Size: ");
        if (posAdded != 0 || posSize == std::string::npos) {
            allWellFormed = false;
            break;
        }
        sizes.push_back(static_cast<std::size_t>(
            std::stoul(line.substr(posSize + 8))));
    }
    std::cout << "每一行都是完整的 \"Added: V, Size: N\" 格式: "
              << (allWellFormed ? "是" : "否") << std::endl;

    // 驗證 2：Size 欄位恰好是 1..10 的一個排列
    //         （每次 push_back 後在鎖內立即讀取，所以不會重複也不會跳號）
    std::sort(sizes.begin(), sizes.end());
    bool sizesArePermutation = (sizes.size() == 10);
    for (std::size_t i = 0; i < sizes.size() && sizesArePermutation; ++i) {
        if (sizes[i] != i + 1) sizesArePermutation = false;
    }
    std::cout << "Size 欄位恰好是 1..10 的一個排列: "
              << (sizesArePermutation ? "是" : "否") << std::endl;

    // 驗證 3：所有 value 都被寫入且僅一次
    std::vector<int> sortedData = data;
    std::sort(sortedData.begin(), sortedData.end());
    std::cout << "所有寫入的 value（排序後）:";
    for (int v : sortedData) std::cout << " " << v;
    std::cout << std::endl;

    // ── LeetCode 146 ──
    std::cout << "\n=== LeetCode 146. LRU Cache ===" << std::endl;
    {
        LRUCache cache(2);
        cache.put(1, 1);
        cache.put(2, 2);
        std::cout << "get(1) = " << cache.get(1) << "  (預期 1)" << std::endl;
        cache.put(3, 3);            // 容量滿 → 淘汰最久未用的 key 2
        std::cout << "get(2) = " << cache.get(2) << "  (預期 -1，已被淘汰)" << std::endl;
        cache.put(4, 4);            // 淘汰 key 1
        std::cout << "get(1) = " << cache.get(1) << "  (預期 -1，已被淘汰)" << std::endl;
        std::cout << "get(3) = " << cache.get(3) << "  (預期 3)" << std::endl;
        std::cout << "get(4) = " << cache.get(4) << "  (預期 4)" << std::endl;

        // 多執行緒壓測：容量不變是不變量，任何時刻都不該超過 capacity
        LRUCache shared(50);
        std::vector<std::thread> workers;
        for (int w = 0; w < 8; ++w) {
            workers.emplace_back([&shared, w]() {
                for (int i = 0; i < 200; ++i) {
                    shared.put(w * 1000 + i, i);
                    shared.get(w * 1000 + i / 2);
                }
            });
        }
        for (auto& t : workers) t.join();
        std::cout << "8 執行緒壓測後 size() = " << shared.size()
                  << "  (capacity 50，不變量保持)" << std::endl;
    }

    // ── 日常實務：庫存 ──
    std::cout << "\n=== 日常實務: 扣庫存 + 開出貨單（不可超賣）===" << std::endl;
    {
        InventoryService inv(100);       // 只有 100 件庫存

        std::vector<std::thread> buyers;
        std::vector<int> successCount(8, 0);
        for (int b = 0; b < 8; ++b) {
            buyers.emplace_back([&inv, &successCount, b]() {
                // 8 條執行緒各嘗試下 20 張單，每張 1 件 → 總需求 160 > 庫存 100
                for (int i = 0; i < 20; ++i) {
                    if (!inv.placeOrder(1).empty()) {
                        ++successCount[b];
                    }
                }
            });
        }
        for (auto& t : buyers) t.join();

        int totalSuccess = 0;
        for (int c : successCount) totalSuccess += c;

        std::cout << "總需求 160 件，庫存 100 件" << std::endl;
        std::cout << "成功下單數: " << totalSuccess << "  (必須恰好 = 100)" << std::endl;
        std::cout << "剩餘庫存: " << inv.stock() << "  (必須 = 0，不可為負數)" << std::endl;
        std::cout << "出貨單筆數: " << inv.shipmentCount()
                  << "  (必須與成功下單數一致)" << std::endl;
        // 註：每條執行緒各搶到幾張單是排程決定的，每次執行都不同，
        //     所以這裡只驗證「總量」這個不變量，不列出各執行緒的分佈。
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.1：stdmutex 基本操作3.cpp' -o multiple_ops

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 「哪個 value 對到哪個 Size」、「各執行緒各搶到幾張訂單」
//      都由排程決定，【每次執行都不同】，故本檔只輸出可穩定驗證的不變量，
//      不列出逐行明細。
//   2. LRU 壓測的 size() 恆為 50（8 條執行緒共寫入 1600 個相異 key，
//      遠超容量，故必然填滿）。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程 5.1 示範: 保護多個相關操作 ===
// 共記錄 10 行 log
// 每一行都是完整的 "Added: V, Size: N" 格式: 是
// Size 欄位恰好是 1..10 的一個排列: 是
// 所有寫入的 value（排序後）: 0 1 2 3 4 100 101 102 103 104
//
// === LeetCode 146. LRU Cache ===
// get(1) = 1  (預期 1)
// get(2) = -1  (預期 -1，已被淘汰)
// get(1) = -1  (預期 -1，已被淘汰)
// get(3) = 3  (預期 3)
// get(4) = 4  (預期 4)
// 8 執行緒壓測後 size() = 50  (capacity 50，不變量保持)
//
// === 日常實務: 扣庫存 + 開出貨單（不可超賣）===
// 總需求 160 件，庫存 100 件
// 成功下單數: 100  (必須恰好 = 100)
// 剩餘庫存: 0  (必須 = 0，不可為負數)
// 出貨單筆數: 100  (必須與成功下單數一致)
