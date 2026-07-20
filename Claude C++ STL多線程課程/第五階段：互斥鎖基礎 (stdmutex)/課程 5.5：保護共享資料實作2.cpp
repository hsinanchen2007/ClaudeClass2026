// 檔案：lesson_5_5_copyable_counter.cpp
// 說明：可複製的執行緒安全計數器（明確定義複製行為）
//
// =============================================================================
//  課程 5.5：保護共享資料實作2.cpp  —  含 mutex 的類別要怎麼複製？
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    std::mutex 不可複製、不可移動，含 mutex 的類別要自己定義複製語意
//   語法：    CopyableCounter(const CopyableCounter& other) {
//                 std::lock_guard<std::mutex> lock(other.mtx);   // 鎖來源
//                 count = other.count;                            // 只複製值
//             }
//   標準版本：std::mutex / std::lock_guard 為 C++11；std::scoped_lock 為 C++17
//   標頭檔：  <mutex>
//   關鍵事實：std::mutex 的複製建構與複製賦值都被 = delete，
//             因此含 mutex 的類別預設也不可複製 —— 這是刻意的設計。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::mutex 不可複製】
//   mutex 代表的是「一個作業系統層級的同步物件與它當下的狀態」
//   （誰持有、有誰在等待佇列上）。這些東西複製沒有任何意義：
//     * 複製一把「已被鎖住」的鎖，新的那把該是鎖住還是開著？
//     * 原本在等待佇列上的執行緒，要不要也複製一份到新的鎖上？
//     * 底層是 pthread_mutex_t，內含 futex word 與 waiter 計數，
//       逐位元組複製會產生一個狀態不一致的怪物。
//   標準的答案是：這個問題無解，所以直接禁止（= delete）。
//   連帶效果是【任何含有 mutex 成員的類別，預設也不可複製】——
//   編譯器產生的複製建構函式會因為成員不可複製而被隱式刪除。
//
// 【2. 那為什麼還要讓計數器可複製】
//   因為「鎖不可複製」是實作細節，「計數器可不可以複製」是介面設計問題。
//   使用者想要的是「複製這個計數器目前的值，得到一個獨立的新計數器」——
//   這個需求完全合理。做法是自己寫複製建構函式，
//   明確表達：【複製的是資料，不是鎖】。
//       CopyableCounter(const CopyableCounter& other) {
//           std::lock_guard<std::mutex> lock(other.mtx);  // 鎖住來源，取得一致快照
//           count = other.count;                           // 只搬資料
//           // 本物件的 mtx 由預設建構函式建立，是全新的、未鎖定的鎖
//       }
//   注意 mtx 沒有出現在初始化列表裡 —— 它被預設建構，這正是我們要的。
//
// 【3. 為什麼複製時要鎖住來源】
//   複製發生時，另一條執行緒可能正在 increment()。
//   若不鎖，讀 other.count 就與那次寫入構成 data race → UB。
//   鎖住來源之後，我們取得的是一個「一致的快照」。
//   注意鎖的是 other.mtx（來源），不是 this->mtx ——
//   本物件還在建構中，根本還沒有人能看到它，不需要保護。
//   這也是 mtx 必須宣告為 mutable 的原因之一：
//   複製建構函式的參數是 const&，要在 const 物件上鎖，鎖就必須是 mutable。
//
// 【4. 複製賦值：兩把鎖，兩個陷阱】
//   operator= 比複製建構困難，因為【兩邊都已經存在】，兩把鎖都要拿：
//     陷阱一：死結。若寫成
//         lock_guard l1(mtx); lock_guard l2(other.mtx);
//     則 `a = b` 與 `b = a` 同時發生時，兩條執行緒各拿一把、各等對方
//     → 經典的 AB-BA 死結。
//     解法是 std::scoped_lock（C++17）一次鎖多把，
//     它內部用死結避免演算法（等價於 std::lock）：反覆嘗試並在失敗時全部放開重來，
//     不依賴加鎖順序。C++11 時代要寫 std::lock(mtx, other.mtx) 再各自 adopt。
//     陷阱二：自我賦值。`a = a` 時若沒有 if (this != &other) 檢查，
//     scoped_lock 會對【同一把】mutex 鎖兩次 → 這是 UB
//     （std::mutex 不可重入）。所以那行檢查不是為了效能，是為了正確性。
//
// 【5. 複製之後兩個物件的關係】
//   複製完成後，c1 與 c2 是【完全獨立】的：各自有各自的 mutex、各自的 count。
//   對 c1 呼叫 increment() 不會影響 c2 —— 這正是「值語意 (value semantics)」。
//   如果你要的是「兩個變數指向同一個計數器」，那需要的不是複製，
//   而是 std::shared_ptr<CopyableCounter>（引用語意）。
//   設計類別時先想清楚要哪一種，這決定了整個 API 的形狀。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 mutex 要宣告成 mutable
//   get() 是 const 成員函式（邏輯上不改變物件），但它需要鎖 mutex，
//   而 lock() 會修改 mutex 的內部狀態。
//   mutable 的語意正是「這個成員的改變不算改變物件的邏輯狀態」——
//   mutex、快取、惰性計算的結果都屬於這一類。
//   若不加 mutable，const 成員函式裡就無法上鎖，
//   只能把 get() 改成非 const（污染整個介面）。
//
// (B) 移動語意：為什麼含 mutex 的類別移動也很麻煩
//   std::mutex 同樣不可移動。所以移動建構函式也必須自己寫，
//   而且要鎖住來源（因為要讀它的資料）。
//   但這裡有個微妙的問題：移動的意義是「來源之後不再被使用」，
//   若還有別的執行緒持有來源的參考並正在使用它，移動本身就是設計錯誤。
//   實務上多數「含鎖的類別」乾脆兩者都禁止，改用 shared_ptr 傳遞。
//
// (C) scoped_lock 的死結避免演算法
//   std::scoped_lock（與 std::lock）不是「照順序鎖」，
//   而是採用 try-and-back-off：鎖住第一把，對其餘的用 try_lock；
//   任何一把失敗就全部放開、重新開始（通常會換一把當起點）。
//   這樣不論呼叫端以什麼順序傳入 mutex，都不會死結。
//   代價是最壞情況下可能重試多次（live-lock 的理論風險存在但實務上極罕見）。
//
// 【注意事項 Pay Attention】
// 1. std::mutex 不可複製也不可移動；含 mutex 的類別預設不可複製。
// 2. 複製建構時要鎖【來源】；本物件還在建構中，沒有人看得到，不必鎖。
// 3. mutex 要宣告 mutable，否則 const 成員函式無法上鎖。
// 4. operator= 要鎖兩把 → 用 std::scoped_lock（C++17）避免 AB-BA 死結。
// 5. `if (this != &other)` 是【正確性】需求：對同一把 std::mutex 鎖兩次是 UB。
// 6. 複製後兩物件完全獨立（值語意）；要共用請改用 shared_ptr（引用語意）。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔主題是「含 mutex 的類別如何定義複製語意」，屬於 C++ 的
//   特殊成員函式與資源管理設計；允許使用的設計題（146/155/705/707/1603）
//   在 LeetCode 上都是單執行緒判題、且從不需要複製建構函式，
//   並行題（1114～1117/1195）則完全不涉及物件複製。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （metrics 快照、設定物件的 copy-on-write 發佈）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】含 mutex 的類別與複製語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::mutex 不可複製?這對我的類別有什麼影響?
//     答：mutex 代表「同步物件的當下狀態」（誰持有、誰在等待佇列上），
//         複製這些狀態沒有任何合理的語意，所以標準直接 = delete。
//         連帶效果是任何含 mutex 成員的類別，
//         其隱式複製建構函式會被刪除 → 預設不可複製。
//         想要可複製就必須自己寫，並明確表達「複製資料、不複製鎖」。
//     追問：那新物件的 mutex 從哪來?
//         → 由預設建構函式產生一把全新的、未鎖定的鎖。
//           所以 mtx 不會出現在初始化列表裡，這是刻意的。
//
// 🔥 Q2. 寫 operator= 時為什麼要用 std::scoped_lock 而不是兩個 lock_guard?
//     答：因為要同時持有兩把鎖。若用兩個 lock_guard 依序鎖，
//         `a = b` 與 `b = a` 在兩條執行緒上同時發生時，
//         會各拿一把、各等對方 → AB-BA 死結。
//         scoped_lock（C++17）內部用 try-and-back-off 演算法，
//         不依賴加鎖順序，因此不會死結。
//     追問：C++11 沒有 scoped_lock 怎麼辦?
//         → 用 std::lock(m1, m2) 先安全地鎖住兩把，
//           再各自用 lock_guard(m, std::adopt_lock) 接管所有權以確保釋放。
//
// ⚠️ 陷阱. operator= 裡的 `if (this != &other)` 只是效能優化嗎?
//     答：不是，它是【正確性】需求。少了它，`a = a` 會讓 scoped_lock
//         對同一把 std::mutex 鎖兩次 —— 而 std::mutex 不可重入，
//         同一執行緒重複 lock 是未定義行為。
//         所以這行檢查是必要的，不是可以省略的優化。
//     為什麼會錯：大家背過「自我賦值檢查是為了避免先釋放再複製造成自毀」，
//         於是把它歸類為「與資源釋放有關的效能/安全優化」，
//         在沒有動態記憶體的類別裡就覺得可以省。
//         但在含鎖的類別裡，它擋的是完全不同的東西：重複上鎖的 UB。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <atomic>

class CopyableCounter {
private:
    mutable std::mutex mtx;
    int count = 0;
    
public:
    CopyableCounter() = default;
    
    // 明確定義複製建構函式
    CopyableCounter(const CopyableCounter& other) {
        std::lock_guard<std::mutex> lock(other.mtx);  // 鎖定來源
        count = other.count;  // 複製值
        // 新物件有自己的 mutex（預設建構）
    }
    
    // 明確定義複製賦值運算子
    CopyableCounter& operator=(const CopyableCounter& other) {
        if (this != &other) {
            // 需要同時鎖定兩個物件，注意死結！
            std::scoped_lock lock(mtx, other.mtx);
            count = other.count;
        }
        return *this;
    }
    
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++count;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】metrics 快照：用複製取得「這一刻」的一致視圖
//   情境：監控系統每 15 秒抓一次指標。若直接把內部的 metrics 物件
//         用引用交出去，抓取端讀到一半、工作執行緒又改了值，
//         就會拿到欄位互相矛盾的資料（例如 failed > total）。
//   做法：提供一個「複製建構」，在鎖內一次把所有欄位搬進新物件，
//         交出去的是完全獨立的快照，抓取端愛怎麼讀都不影響線上服務。
//   這正是本檔複製語意的實務價值：複製 = 取得一致快照。
// -----------------------------------------------------------------------------
class RequestMetrics {
private:
    mutable std::mutex mtx;
    long total = 0;
    long failed = 0;
    long bytesOut = 0;

public:
    RequestMetrics() = default;

    // 複製建構：鎖住來源，一次搬走所有欄位 → 快照必定一致
    RequestMetrics(const RequestMetrics& other) {
        std::lock_guard<std::mutex> lock(other.mtx);
        total = other.total;
        failed = other.failed;
        bytesOut = other.bytesOut;
    }

    RequestMetrics& operator=(const RequestMetrics& other) {
        if (this != &other) {
            std::scoped_lock lock(mtx, other.mtx);
            total = other.total;
            failed = other.failed;
            bytesOut = other.bytesOut;
        }
        return *this;
    }

    void record(bool ok, long bytes) {
        std::lock_guard<std::mutex> lock(mtx);
        ++total;
        if (!ok) ++failed;
        bytesOut += bytes;
    }

    // 直接回傳複本 —— 呼叫端拿到的是獨立快照，不是內部狀態的引用
    RequestMetrics snapshot() const { return *this; }

    long totalCount() const  { std::lock_guard<std::mutex> lock(mtx); return total; }
    long failedCount() const { std::lock_guard<std::mutex> lock(mtx); return failed; }
    long bytes() const       { std::lock_guard<std::mutex> lock(mtx); return bytesOut; }

    // 不變量：失敗數不可能超過總數
    bool consistent() const {
        std::lock_guard<std::mutex> lock(mtx);
        return failed <= total;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】設定的 copy-on-write 發佈
//   情境：設定讀多寫少（每個請求都要讀，但一天才改幾次）。
//         若每次讀取都要鎖，鎖會變成整個服務的瓶頸。
//   做法：更新時【複製】一份完整的設定、在複本上修改、
//         最後只用一次鎖把指標換掉。讀取端只需要複製一份 shared_ptr，
//         之後的存取完全無鎖。
//   這是本檔「複製語意」在高效能系統中最重要的應用：
//         用複製換取「讀取端零同步」。
// -----------------------------------------------------------------------------
struct Settings {
    std::map<std::string, std::string> values;
    int version = 0;
};

class SettingsPublisher {
private:
    mutable std::mutex mtx;
    std::shared_ptr<const Settings> current;

public:
    SettingsPublisher() {
        auto s = std::make_shared<Settings>();
        s->values["log.level"] = "info";
        s->version = 1;
        current = s;
    }

    // 讀取端：只在極短的時間內持鎖（複製一個 shared_ptr），之後完全無鎖
    std::shared_ptr<const Settings> get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return current;
    }

    // 寫入端：複製 → 修改複本 → 一次換掉指標
    void update(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mtx);
        auto next = std::make_shared<Settings>(*current);   // 複製整份
        next->values[key] = value;
        next->version = current->version + 1;
        current = next;                                      // 原子性地發佈
        // 舊的那份仍被正在讀取的執行緒持有，
        // 等最後一個讀者放手時 shared_ptr 自動回收 —— 不需要等待、不需要 GC
    }
};

int main() {
    std::cout << "=== 原始示範：複製後兩個計數器完全獨立 ===\n";
    CopyableCounter c1;
    c1.increment();
    c1.increment();
    
    CopyableCounter c2 = c1;  // 複製
    
    std::cout << "c1 = " << c1.get() << std::endl;  // 2
    std::cout << "c2 = " << c2.get() << std::endl;  // 2
    
    c1.increment();
    
    std::cout << "c1 = " << c1.get() << std::endl;  // 3
    std::cout << "c2 = " << c2.get() << std::endl;  // 2（獨立）

    std::cout << "\n=== 複製賦值 + 自我賦值 ===\n";
    {
        CopyableCounter a, b;
        for (int i = 0; i < 5; ++i) a.increment();
        b = a;
        std::cout << "b = a 之後，b = " << b.get() << " (預期 5)\n";
        a = a;   // 自我賦值：靠 if (this != &other) 擋掉重複上鎖的 UB
        std::cout << "a = a 之後，a = " << a.get() << " (預期 5，且沒有卡死)\n";
    }

    std::cout << "\n=== 併發下複製：快照必定一致 ===\n";
    {
        CopyableCounter shared;
        std::vector<std::thread> ths;
        std::atomic<bool> stop{false};

        // 4 條執行緒不斷遞增
        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&shared, &stop] {
                while (!stop.load()) shared.increment();
            });
        }

        // 主執行緒不斷複製快照，檢查數值是否單調不減
        int prev = 0;
        bool monotonic = true;
        for (int i = 0; i < 2000; ++i) {
            CopyableCounter snap = shared;   // 複製建構會鎖住來源
            int v = snap.get();
            if (v < prev) monotonic = false;
            prev = v;
        }
        stop.store(true);
        for (auto& t : ths) t.join();

        std::cout << "取 2000 次快照，數值單調不減: " << std::boolalpha
                  << monotonic << " (必定為 true)\n";
        std::cout << "→ 複製建構在鎖內完成，所以每個快照都是某一瞬間的真實值\n";
    }

    std::cout << "\n=== 日常實務 1：metrics 一致性快照 ===\n";
    {
        RequestMetrics metrics;
        std::vector<std::thread> ths;
        std::atomic<int> badSnapshots{0};

        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&metrics, i] {
                for (int k = 0; k < 20000; ++k) metrics.record((k + i) % 5 != 0, 128);
            });
        }
        std::thread exporter([&metrics, &badSnapshots] {
            for (int k = 0; k < 5000; ++k) {
                RequestMetrics snap = metrics.snapshot();   // 取得獨立快照
                if (!snap.consistent()) badSnapshots.fetch_add(1);
            }
        });

        for (auto& t : ths) t.join();
        exporter.join();

        RequestMetrics fin = metrics.snapshot();
        std::cout << "總請求: " << fin.totalCount()
                  << ", 失敗: " << fin.failedCount()
                  << ", 輸出位元組: " << fin.bytes() << "\n";
        std::cout << "取 5000 次快照，不一致次數: " << badSnapshots.load()
                  << " (必定為 0)\n";
    }

    std::cout << "\n=== 日常實務 2：設定的 copy-on-write 發佈 ===\n";
    {
        SettingsPublisher pub;
        std::vector<std::thread> readers;
        std::atomic<int> inconsistent{0};
        std::atomic<bool> stop{false};

        // 讀取端：拿到 shared_ptr 之後完全無鎖地讀整份設定
        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&pub, &inconsistent, &stop] {
                while (!stop.load()) {
                    auto s = pub.get();
                    // 不變量：version 與 log.level 必定來自同一份設定
                    auto it = s->values.find("log.level");
                    if (it == s->values.end()) { inconsistent.fetch_add(1); continue; }
                    bool ok = (s->version == 1 && it->second == "info")
                           || (s->version >= 2 && it->second != "info");
                    if (!ok) inconsistent.fetch_add(1);
                }
            });
        }

        std::thread writer([&pub] {
            for (int i = 0; i < 500; ++i) pub.update("log.level", "debug");
        });
        writer.join();
        stop.store(true);
        for (auto& t : readers) t.join();

        auto fin = pub.get();
        std::cout << "最終版本: " << fin->version << " (必定為 501)\n";
        std::cout << "log.level = " << fin->values.at("log.level") << "\n";
        std::cout << "讀到不一致設定的次數: " << inconsistent.load() << " (必定為 0)\n";
        std::cout << "→ 讀取端只在複製 shared_ptr 時持鎖，之後零同步\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.5：保護共享資料實作2.cpp' -o copyable_counter
//   （std::scoped_lock 需要 C++17；C++11 請改用 std::lock + adopt_lock）

// 註：本檔所有共享狀態都受鎖保護，沒有資料競爭，
// 因此輸出為確定值 —— 每次執行完全相同（本機連續三次實測 md5 一致）。

// === 預期輸出 ===
// === 原始示範：複製後兩個計數器完全獨立 ===
// c1 = 2
// c2 = 2
// c1 = 3
// c2 = 2
//
// === 複製賦值 + 自我賦值 ===
// b = a 之後，b = 5 (預期 5)
// a = a 之後，a = 5 (預期 5，且沒有卡死)
//
// === 併發下複製：快照必定一致 ===
// 取 2000 次快照，數值單調不減: true (必定為 true)
// → 複製建構在鎖內完成，所以每個快照都是某一瞬間的真實值
//
// === 日常實務 1：metrics 一致性快照 ===
// 總請求: 80000, 失敗: 16000, 輸出位元組: 10240000
// 取 5000 次快照，不一致次數: 0 (必定為 0)
//
// === 日常實務 2：設定的 copy-on-write 發佈 ===
// 最終版本: 501 (必定為 501)
// log.level = debug
// 讀到不一致設定的次數: 0 (必定為 0)
// → 讀取端只在複製 shared_ptr 時持鎖，之後零同步
