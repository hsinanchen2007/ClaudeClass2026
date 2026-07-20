// 檔案：lesson_5_5_dangerous_interface.cpp
// 說明：危險的介面設計
//
// =============================================================================
//  課程 5.5：保護共享資料實作4.cpp  —  把內部資料的引用交出去 = 把鎖丟掉
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   getData() / getFirst() 回傳內部資料的引用與指標，
//   鎖在函式回傳的瞬間就釋放了，呼叫端拿到的是【毫無保護】的入口。
//   本檔的 main 為求可安全執行，只在單執行緒下示範「鎖已無效」這件事；
//   一旦有第二條執行緒同時操作，就會是 genuine data race → 【未定義行為】。
//
// 【主題資訊 Information】
//   主題：    執行緒安全類別的介面設計；為什麼「回傳引用」會摧毀同步
//   反面語法：std::vector<int>& getData() { lock_guard lk(mtx); return data; }
//   標準版本：std::mutex / lock_guard 為 C++11
//   標頭檔：  <mutex>、<vector>
//   核心原則：【不要把受保護資料的任何存取路徑（引用、指標、迭代器）交出臨界區段】
//
// 【詳細解釋 Explanation】
//
// 【1. 錯在哪：鎖的生命週期與引用的生命週期不匹配】
//       std::vector<int>& getData() {
//           std::lock_guard<std::mutex> lock(mtx);   // 建構：上鎖
//           return data;                              // 回傳引用
//       }                                             // ← lock 解構：解鎖！
//   lock_guard 是自動儲存期物件，函式一回傳它就被解構、鎖就釋放。
//   但呼叫端拿到的引用【活得比鎖久】。於是：
//       auto& v = container.getData();   // 拿到引用，但鎖已經沒了
//       v.push_back(1);                  // 完全沒有保護的寫入
//   這把鎖從頭到尾只鎖住了「回傳一個引用」這個動作 ——
//   而那個動作本身根本不需要保護。等於整個同步機制是裝飾品。
//
// 【2. 為什麼這種錯誤特別難被發現】
//   * 程式碼「看起來」是對的：每個函式都有 lock_guard，
//     code review 掃過去會覺得同步做得很完整。
//   * 單執行緒測試 100% 通過，因為根本沒有競爭者。
//   * 靜態分析工具通常也不會報 —— 它確實有鎖。
//   → 這是一種【結構性的錯誤】：問題不在某一行，而在「保護的邊界畫錯了」。
//     鎖保護的應該是「對資料的操作」，而不是「取得資料的動作」。
//
// 【3. 三種洩漏形式，一個共同本質】
//   ① 回傳引用       std::vector<int>& getData()
//   ② 回傳指標       int* getFirst()
//   ③ 回傳迭代器     auto begin() / auto end()
//   本質相同：都把「通往受保護記憶體的路徑」交到臨界區段之外。
//   ③ 尤其常見 —— 提供 begin()/end() 讓使用者可以用 range-for，
//   看起來很現代很好用，但那等於邀請使用者在沒有鎖的情況下走訪整個容器，
//   而且期間別人的 push_back 還可能觸發重新配置讓迭代器失效。
//
// 【4. getFirst() 為什麼比 getData() 更危險】
//   `return &data[0];` 交出的是指向 vector 內部緩衝區的裸指標。
//   只要另一條執行緒 push_back 導致重新配置，
//   舊緩衝區就被釋放，這個指標立刻變成【懸空指標】——
//   之後的 `*first = 99;` 是 use-after-free，不只是讀到舊值。
//   （本機 libstdc++ 的 vector 成長倍率為 2×，此為實作定義值，非標準保證。）
//
// 【5. 正確的思考方式：把「操作」搬進來，而不是把「資料」送出去】
//   遇到「我需要對內部資料做 X」時，正確的問法不是
//   「怎麼安全地把資料交出去」，而是「怎麼讓 X 在鎖內執行」。
//   四種標準做法（下一檔 5.5-5 完整示範）：
//     ① 回傳【複本】          std::vector<int> getData() const
//     ② 回傳【值】            int getAt(size_t i) const
//     ③ 提供【操作】而非存取   void add(int) / bool tryPop(int&)
//     ④ 傳入【回呼】在鎖內執行 void forEach(const std::function<void(int)>&)
//   ④ 要特別小心：使用者的回呼若反過來呼叫本物件的方法，就會重複上鎖 → UB。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼標準容器不自帶鎖
//   常有人問「為什麼 std::vector 不內建一把鎖就好」。理由正是本檔的主題：
//   【單一操作的原子性沒有用，使用者需要的原子性單位是「一段操作」】。
//   即使 push_back 與 size() 各自都是原子的，
//   `if (v.size() < 10) v.push_back(x);` 仍然是錯的。
//   內建鎖只會讓所有人付出成本，卻無法解決真正的問題，
//   還會給人「已經安全了」的錯覺。這就是 C++ 選擇不做的原因。
//   （Java 的 Vector / Hashtable 就是這個設計失誤的著名反例，
//     後來被 ArrayList / HashMap + 明確的外部同步取代。）
//
// (B) const 引用也一樣危險
//   有人會想「那我回傳 const std::vector<int>& 總可以吧，反正不能寫」。
//   不行。讀取也需要保護 —— 只要有任何一方在寫，讀就是 data race。
//   const 只限制「透過這個引用不能寫」，
//   完全擋不住「別的執行緒同時在寫同一份資料」。
//
// (C) 這個模式在其他語言也一樣
//   Rust 用型別系統從根本擋掉這件事：MutexGuard 借用了資料，
//   而借用檢查器不允許引用活得比 guard 久 —— 這段程式碼在 Rust 裡編譯不過。
//   C++ 沒有這個保護，只能靠設計紀律與 code review。
//   了解 Rust 為什麼要這樣設計，反過來會更清楚 C++ 這裡的風險在哪。
//
// 【注意事項 Pay Attention】
// 1. 回傳引用/指標/迭代器 = 把受保護資料的存取路徑交到鎖外，同步形同虛設。
// 2. const 引用同樣危險：只要有寫入者，讀取就需要保護。
// 3. 回傳裸指標更糟：push_back 觸發重新配置後會變成懸空指標 → use-after-free。
// 4. 提供 begin()/end() 等於邀請使用者無鎖走訪；執行緒安全的容器不該有它們。
// 5. 正確做法是「把操作搬進鎖內」（回傳複本/值、提供操作、傳入回呼），
//    而不是「安全地把資料交出去」——後者不存在。
// 6. 用回呼在鎖內執行時，要文件化「回呼中不可再呼叫本物件」，否則重複上鎖 → UB。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔主題是「執行緒安全類別的介面設計原則」，
//   而 LeetCode 的設計題（146/155/705/707/1603）都在單執行緒環境判題，
//   它們的介面回傳引用與否完全不影響對錯；
//   並行題（1114～1117/1195）則沒有一題在設計資料結構的對外介面。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （設定管理器的引用洩漏事故、快取 API 的引用失效）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】執行緒安全類別的介面設計
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 這個函式有加鎖，為什麼還是不安全?
//        std::vector<int>& getData() { std::lock_guard lk(mtx); return data; }
//     答：因為 lock_guard 在函式回傳時就解構、鎖就釋放了，
//         但呼叫端拿到的引用活得比鎖久。這把鎖只保護了
//         「回傳一個引用」這個動作 —— 而那個動作本身不需要保護。
//         呼叫端之後對該引用的所有讀寫，完全沒有任何同步。
//     追問：那改成回傳 const 引用呢?
//         → 一樣不安全。const 只限制「透過這個引用不能寫」，
//           擋不住別的執行緒同時在寫同一份資料；
//           一寫一讀而無同步，依然是 data race。
//
// 🔥 Q2. 執行緒安全的容器該怎麼設計介面?
//     答：原則是「把操作搬進鎖內，而不是把資料送出鎖外」。四種做法：
//         ① 回傳複本（getData() 回傳 vector 值）
//         ② 回傳值（getAt(i) 回傳 int）
//         ③ 提供操作而非存取（add / tryPop，把 check 與 act 合併）
//         ④ 傳入回呼讓它在鎖內執行（forEach）
//         絕不提供 begin()/end() —— 那等於邀請使用者無鎖走訪。
//     追問：④ 有什麼風險?
//         → 使用者的回呼若反過來呼叫本物件的方法，就會對同一把
//           std::mutex 重複上鎖 → UB。必須在文件中明確禁止，
//           或改用 recursive_mutex（但那通常代表設計該重新想）。
//
// ⚠️ 陷阱. 「這個類別每個成員函式都有 lock_guard，所以它是執行緒安全的」——錯在哪?
//     答：執行緒安全不是「每個函式都有鎖」，而是「不變量在任何可觀察時刻都成立」。
//         只要有一個函式把內部資料的引用/指標/迭代器交出去，
//         其他函式鎖得再完整都沒有意義 —— 因為外面已經有一條沒鎖的路徑了。
//         一個洞就足以讓整艘船沉沒。
//     為什麼會錯：把執行緒安全當成可以逐函式檢查的【局部性質】。
//         它其實是類別的【整體性質】：要看所有對外介面加起來，
//         有沒有留下任何繞過鎖的途徑。這也是為什麼
//         review 執行緒安全類別時，第一件事是看每個函式的【回傳型別】。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <vector>
#include <string>
#include <map>
#include <optional>
#include <memory>

class DangerousContainer {
private:
    std::mutex mtx;
    std::vector<int> data;
    
public:
    // 💀 危險！返回內部資料的引用
    std::vector<int>& getData() {
        std::lock_guard<std::mutex> lock(mtx);
        return data;  // 鎖在這裡就釋放了！
    }
    
    // 💀 危險！返回內部資料的指標
    int* getFirst() {
        std::lock_guard<std::mutex> lock(mtx);
        if (!data.empty()) {
            return &data[0];  // 返回後鎖就釋放了！
        }
        return nullptr;
    }
    
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(value);
    }
};

/*
 * 問題分析：
 * 
 * Thread A:                          Thread B:
 * auto& vec = container.getData();   
 *   // 鎖已釋放！                    container.add(100);
 * vec.push_back(1);                    // 同時修改！
 *   // 💀 競爭條件！
 */

// -----------------------------------------------------------------------------
// 【正確版】把操作搬進鎖內，不把資料送出鎖外
//   對照 DangerousContainer 的每一個成員，看修法的差異。
// -----------------------------------------------------------------------------
class SafeContainer {
private:
    mutable std::mutex mtx;
    std::vector<int> data;

public:
    // ✓ 回傳複本，不是引用
    std::vector<int> getData() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data;
    }

    // ✓ 回傳值，不是指標；用 optional 表達「可能沒有」
    std::optional<int> getFirst() const {
        std::lock_guard<std::mutex> lock(mtx);
        if (data.empty()) return std::nullopt;
        return data[0];
    }

    // ✓ 提供「操作」而非「存取」：修改必須經過本類別
    void add(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(value);
    }

    // ✓ 要修改特定位置，也提供操作，而不是交出引用
    bool setAt(size_t index, int value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (index >= data.size()) return false;
        data[index] = value;
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.size();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】設定管理器的引用洩漏事故
//   情境：設定管理器提供 getAll() 讓各模組讀取設定。
//         最初回傳 const map&「以避免複製成本」，
//         結果某個模組在啟動時走訪這份 map，
//         同時另一條執行緒熱更新設定 → map 重新雜湊 →
//         走訪中的迭代器失效 → 服務啟動時隨機崩潰，
//         而且因為與時序有關，開發環境完全重現不出來。
//   修法：回傳複本；若真的擔心成本，用 shared_ptr<const Map> 做
//         copy-on-write，讀取端只複製一個指標。
// -----------------------------------------------------------------------------
class ConfigStore {
private:
    mutable std::mutex mtx;
    std::map<std::string, std::string> values;

public:
    ConfigStore() {
        values["host"] = "api.internal";
        values["port"] = "8080";
    }

    void set(const std::string& k, const std::string& v) {
        std::lock_guard<std::mutex> lock(mtx);
        values[k] = v;
    }

    // ✓ 回傳複本：呼叫端拿到獨立快照，愛怎麼走訪都不影響線上服務
    std::map<std::string, std::string> getAll() const {
        std::lock_guard<std::mutex> lock(mtx);
        return values;
    }

    // ✓ 單一查詢也回傳值
    std::optional<std::string> get(const std::string& k) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = values.find(k);
        if (it == values.end()) return std::nullopt;
        return it->second;
    }

    size_t size() const { std::lock_guard<std::mutex> lock(mtx); return values.size(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】快取 API：為什麼不能回傳「快取內物件的引用」
//   情境：快取提供 const Entry& get(key)「避免複製」。
//         但快取會淘汰（LRU 逐出、TTL 過期），
//         一旦持有引用期間該項目被逐出，引用立刻懸空 → use-after-free。
//         這在單執行緒也會發生，多執行緒下更是必然。
//   修法：回傳複本，或回傳 shared_ptr<const Entry>
//         讓引用計數把物件的生命週期延長到最後一個使用者放手為止。
//         本例示範 shared_ptr 版本 —— 它兼顧了「不複製大物件」與「安全」。
// -----------------------------------------------------------------------------
struct CacheEntry {
    std::string key;
    std::string payload;
    long version;
};

class SafeCache {
private:
    mutable std::mutex mtx;
    std::map<std::string, std::shared_ptr<const CacheEntry>> entries;
    long nextVersion = 1;

public:
    void put(const std::string& key, const std::string& payload) {
        auto e = std::make_shared<CacheEntry>();
        e->key = key;
        e->payload = payload;
        std::lock_guard<std::mutex> lock(mtx);
        e->version = nextVersion++;
        entries[key] = e;
    }

    // ✓ 回傳 shared_ptr：即使之後被逐出，呼叫端手上的那份仍然有效
    std::shared_ptr<const CacheEntry> get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = entries.find(key);
        return it == entries.end() ? nullptr : it->second;
    }

    void evict(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        entries.erase(key);   // 快取裡移除了，但持有 shared_ptr 的人不受影響
    }

    size_t size() const { std::lock_guard<std::mutex> lock(mtx); return entries.size(); }
};

int main() {
    std::cout << "=== 錯誤示範：鎖已釋放，引用毫無保護 ===\n";
    DangerousContainer container;
    container.add(10);
    container.add(20);
    
    // 示範危險的使用方式
    auto& vec = container.getData();  // 鎖已釋放
    vec.push_back(30);                 // 可能與其他執行緒競爭
    
    int* first = container.getFirst(); // 鎖已釋放
    if (first) {
        *first = 99;                   // 可能與其他執行緒競爭
    }
    
    // 輸出結果（不保證正確性）
    for (int val : container.getData()) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    std::cout << "→ 單執行緒下「看起來正常」，但這兩行已經完全繞過了鎖：\n";
    std::cout << "  只要有第二條執行緒同時 add()，就是 data race → UB。\n";
    std::cout << "  更糟的是 getFirst() 的裸指標：對方一次 push_back 觸發重新配置，\n";
    std::cout << "  它就變成懸空指標，*first = 99 會是 use-after-free。\n";

    std::cout << "\n=== 正確版：回傳複本 / 值 / 提供操作 ===\n";
    {
        SafeContainer safe;
        safe.add(10);
        safe.add(20);
        safe.add(30);

        std::vector<int> copy = safe.getData();   // 拿到的是獨立複本
        copy.push_back(999);                      // 改複本完全不影響內部狀態

        std::cout << "內部內容: ";
        for (int v : safe.getData()) std::cout << v << " ";
        std::cout << "(仍是 10 20 30，未被複本的修改影響)\n";
        std::cout << "複本內容: ";
        for (int v : copy) std::cout << v << " ";
        std::cout << "\n";

        auto f = safe.getFirst();
        std::cout << "getFirst() = " << (f ? std::to_string(*f) : "nullopt") << "\n";
        safe.setAt(0, 99);                        // 要改內容，透過操作
        std::cout << "setAt(0, 99) 後內部內容: ";
        for (int v : safe.getData()) std::cout << v << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== 日常實務 1：設定管理器回傳複本 ===\n";
    {
        ConfigStore cfg;
        auto snapshot = cfg.getAll();      // 獨立快照
        cfg.set("port", "9090");           // 之後的修改不影響已取得的快照

        std::cout << "快照中的 port: " << snapshot.at("port") << " (仍是 8080)\n";
        std::cout << "目前的 port:   " << cfg.get("port").value_or("?") << " (已更新為 9090)\n";
        std::cout << "→ 快照是獨立的，走訪它不可能被熱更新弄壞\n";
    }

    std::cout << "\n=== 日常實務 2：快取用 shared_ptr 而非引用 ===\n";
    {
        SafeCache cache;
        cache.put("user:1001", "Alice");
        cache.put("user:1002", "Bob");

        auto held = cache.get("user:1001");     // 持有一份
        std::cout << "取得 user:1001 = " << held->payload
                  << " (version " << held->version << ")\n";

        cache.evict("user:1001");               // 從快取中逐出

        std::cout << "逐出後快取大小: " << cache.size() << "\n";
        std::cout << "快取中還查得到嗎: "
                  << (cache.get("user:1001") ? "是" : "否") << "\n";
        std::cout << "但手上那份仍然有效: " << held->payload
                  << " (shared_ptr 把生命週期延長到最後一個使用者放手為止)\n";
        std::cout << "→ 若當初回傳的是引用，這裡就是 use-after-free\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.5：保護共享資料實作4.cpp' -o dangerous_interface
//   （main 為求可安全執行，錯誤示範只在單執行緒下進行；
//     加上第二條執行緒同時 add() 即為 data race，請用 TSan 觀察）

// 註：main 中的錯誤示範刻意只在【單執行緒】下執行，
// 因此本檔實際跑起來沒有 data race，輸出為確定值（本機連續三次實測 md5 一致）。
// 那兩行之所以危險，是因為它們把存取路徑交到了鎖外 ——
// 只要加上第二條同時 add() 的執行緒就會構成 data race，請用 TSan 觀察。

// === 預期輸出 ===
// === 錯誤示範：鎖已釋放，引用毫無保護 ===
// 99 20 30 
// → 單執行緒下「看起來正常」，但這兩行已經完全繞過了鎖：
//   只要有第二條執行緒同時 add()，就是 data race → UB。
//   更糟的是 getFirst() 的裸指標：對方一次 push_back 觸發重新配置，
//   它就變成懸空指標，*first = 99 會是 use-after-free。
//
// === 正確版：回傳複本 / 值 / 提供操作 ===
// 內部內容: 10 20 30 (仍是 10 20 30，未被複本的修改影響)
// 複本內容: 10 20 30 999 
// getFirst() = 10
// setAt(0, 99) 後內部內容: 99 20 30 
//
// === 日常實務 1：設定管理器回傳複本 ===
// 快照中的 port: 8080 (仍是 8080)
// 目前的 port:   9090 (已更新為 9090)
// → 快照是獨立的，走訪它不可能被熱更新弄壞
//
// === 日常實務 2：快取用 shared_ptr 而非引用 ===
// 取得 user:1001 = Alice (version 1)
// 逐出後快取大小: 1
// 快取中還查得到嗎: 否
// 但手上那份仍然有效: Alice (shared_ptr 把生命週期延長到最後一個使用者放手為止)
// → 若當初回傳的是引用，這裡就是 use-after-free
