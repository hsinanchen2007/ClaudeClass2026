// =============================================================================
//  課程 4.4：資料競爭範例分析3.cpp  —  Check-Then-Act：檢查與行動之間的縫隙
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   兩條執行緒對同一個 std::vector 一寫一讀，且完全沒有同步
//   → genuine data race → 【未定義行為 (UB)】。
//   本檔【不會】宣稱「一定會 crash」或「一定會超過 10 個元素」。
//
// 【主題資訊 Information】
//   主題：    Check-Then-Act 競爭模式；五大競爭模式之二
//   語法：    if (vec.size() < 10) { vec.push_back(v); }   // 檢查與行動可被拆開
//             if (!vec.empty())    { vec.back(); }         // 同樣的形狀
//   標準版本：std::vector 為 C++98；std::thread 為 C++11
//   標頭檔：  <vector>、<thread>、<mutex>
//   偵測工具：g++ -fsanitize=thread -g -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. Check-Then-Act 的通用形狀】
//   所有這類 bug 都長成同一個樣子：
//       if (某個條件成立) {      // ← check：讀取狀態做判斷
//           //  ★ 縫隙：別人可以在這裡把狀態改掉，讓上面的結論失效
//           做某件依賴該條件的事;  // ← act：基於「已經過期」的結論行動
//       }
//   一旦 check 與 act 不在同一個臨界區段內，中間就有縫隙。
//   縫隙可能只有幾奈秒，但在每秒數百萬次的系統裡，
//   「幾奈秒的縫隙」× 「巨大的次數」= 「每天都發生」。
//
// 【2. 本檔的兩個實例】
//   (a) unsafeAppend：`if (vec.size() < 10) vec.push_back(v);`
//       兩條執行緒同時看到 size()==9，於是都通過檢查，都 push_back
//       → 最後變成 11 個元素，超過了原本想強制的上限。
//       這在「容量限制」「配額」「連線池上限」的場景是嚴重的邏輯錯誤。
//   (b) unsafeAccess：`if (!vec.empty()) { int last = vec.back(); }`
//       檢查通過之後、取值之前，另一條執行緒可能 clear() 或 pop_back()
//       → back() 對空的 vector 是【未定義行為】，不是丟例外。
//       更糟的是 push_back 可能觸發重新配置，讓 back() 讀到已釋放的記憶體。
//
// 【3. 為什麼 std::vector 本身「執行緒安全」與這件事無關】
//   標準函式庫的容器提供的保證是：
//     * 多執行緒同時【讀取】同一個容器（呼叫 const 成員）→ 安全
//     * 只要有任何一方【寫入】→ 使用者必須自己同步
//   常見誤解是「STL 容器是執行緒安全的」。正確的說法是
//   「不同物件的並行存取是安全的；同一物件的並行存取，只有全部唯讀時才安全」。
//   所以本檔的問題不是「vector 不夠好」，而是「使用者沒有做該做的同步」。
//
// 【4. 為什麼「把 size() 換成 atomic」修不好】
//   即使 size 是原子讀取，check 與 act 仍是兩個獨立的動作，縫隙照樣存在。
//   這與課程 4.2-3 的結論一致：
//   【原子化每個零件，不會讓組裝過程變原子】。
//   唯一的解法是把「檢查 + 行動」整段放進同一個臨界區段。
//
// 【5. 正確的介面設計：把 check 與 act 合併成一個操作】
//   真正的修法不只是加鎖，而是【改介面】：
//       ✗ if (c.size() < 10) c.push_back(v);   // 呼叫端負責原子性 → 一定有人忘記
//       ✓ bool ok = c.tryPush(v);              // 由容器保證原子性 → 不可能用錯
//   把「檢查」與「行動」封裝成單一個成員函式，
//   呼叫端就沒有機會在中間插入東西。這是執行緒安全類別設計的核心原則，
//   課程 5.5「安全的介面設計」會完整展開。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 push_back 讓問題升級成記憶體錯誤
//   vector 的元素存在一塊連續的堆積記憶體。當 size == capacity 時，
//   push_back 會：配置更大的新緩衝區 → 搬移元素 → 釋放舊緩衝區。
//   本機 libstdc++ 的成長倍率為 2×（實作定義值，MSVC 是 1.5×）。
//   重新配置會讓所有既有的 iterator / reference / 指標【全部失效】。
//   所以 unsafeAccess 拿到的 `vec.back()` 引用可能指向已經被 free 的記憶體 ——
//   這已經不是「讀到舊值」，而是 use-after-free。
//
// (B) 為什麼本檔多數執行看起來完全正常
//   兩條執行緒各只跑 15 次迴圈，工作量極小；執行緒建立成本
//   （本機約數十 μs）通常大於整段迴圈，兩者往往根本沒有重疊執行。
//   這是「測不出來」而非「沒有錯」。
//   要穩定觀察，請用 ThreadSanitizer，它靠 happens-before 判定而非碰運氣。
//
// (C) 「先檢查再行動」在單執行緒為何完全正確
//   單執行緒下 check 與 act 之間不可能有別人插入，
//   所以這個寫法是完全正確的、也是最自然的寫法。
//   這正是它危險的原因：它是一段【在單執行緒下絕對正確】的程式碼，
//   移植到多執行緒環境時看起來毫無問題，review 也很容易放過。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定 crash」，也不可說「一定超過 10 個」。
// 2. STL 容器沒有提供「同一物件並行讀寫」的保證，同步是使用者的責任。
// 3. 把 size() 換成 atomic 修不好 check-then-act —— 縫隙在兩個動作之間。
// 4. push_back 觸發重新配置會讓所有既有 iterator / reference / 指標失效。
// 5. 真正的修法是改介面：把 check 與 act 封裝成單一原子操作，
//    而不是要求每個呼叫端自己記得加鎖。
// 6. vector 成長倍率 2× 是本機 libstdc++ 的【實作定義】值，非標準保證。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔與「課程 4.1：共享資料的問題2.cpp」是同一個 check-then-act 模式，
//   該檔已用 LeetCode 1603. Design Parking System 做過完整示範
//   （addCar 的「檢查剩餘車位 → 扣減」正是同一個形狀）。
//   在此重複同一題只會稀釋重點，故改以下方兩個真實情境
//   （有界佇列、快取的 get-or-compute）呈現同一個模式的不同面貌。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Check-Then-Act 與容器的執行緒安全
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector 是執行緒安全的嗎?
//     答：問法本身就不完整。標準的保證是：
//         【不同物件】的並行存取安全；【同一物件】只有在全部唯讀時才安全。
//         只要有任何一方寫入，同步就是使用者的責任。
//         所以本檔的問題不是 vector 不好，而是缺少該做的同步。
//     追問：那多執行緒同時對同一個 vector 的【不同元素】寫入呢?
//         → 寫 vec[0] 與 vec[1] 這種不重疊的元素本身沒有 data race，
//           但前提是期間不能有任何人改變 size / capacity；
//           一次 push_back 觸發重新配置就會讓全部指標失效。
//           另外要注意 std::vector<bool> 是特化的位元容器，
//           相鄰元素共用同一個 byte，寫不同元素也會產生 data race。
//
// 🔥 Q2. `if (!vec.empty()) x = vec.back();` 這段在多執行緒下有什麼問題?
//     答：典型的 check-then-act。empty() 回傳 false 之後、back() 執行之前，
//         另一條執行緒可能 pop_back / clear，此時 back() 就是 UB
//         （不是丟例外，是未定義行為）。
//         若對方是 push_back 觸發了重新配置，還會變成 use-after-free。
//     追問：那要怎麼修?
//         → 不是在呼叫端加鎖，而是改介面：提供
//           `std::optional<int> tryPop()` 這種把檢查與取值合併的單一操作，
//           讓呼叫端沒有機會在中間插入東西。
//
// ⚠️ 陷阱. 「我把 size 改成 std::atomic<size_t> 了，check-then-act 應該就安全了」——錯在哪?
//     答：不安全。atomic 讓「讀 size」這個動作變原子，
//         但 check 與 act 仍是兩個分開的動作，縫隙原封不動。
//         兩條執行緒仍然可以同時讀到 size==9、同時通過檢查、同時 push。
//     為什麼會錯：把 atomic 當成「加上去就執行緒安全」的萬用貼紙。
//         atomic 保證的是【單一操作】的不可分割，
//         而 check-then-act 的問題出在【兩個操作之間】。
//         判準是：你能不能把整個操作壓縮成一個原子動作？
//         不能的話就需要鎖，或用 compare_exchange 迴圈把兩步合併成一次嘗試。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <optional>
#include <map>
#include <string>
#include <atomic>

// -----------------------------------------------------------------------------
// 【錯誤示範】check-then-act：檢查與行動之間有縫隙 —— 本課主角
// -----------------------------------------------------------------------------
std::vector<int> vec;

void unsafeAppend(int value) {
    if (vec.size() < 10) {        // 檢查
        // ← 另一執行緒可能在此改變 size！
        vec.push_back(value);      // 可能超過限制
    }
}

void unsafeAccess() {
    if (!vec.empty()) {            // 檢查
        // ← 另一執行緒可能清空 vec！
        int last = vec.back();     // 可能崩潰
        (void)last;                // 刻意不印出：這個值可能來自已失效的記憶體，
                                   // 印出一個不確定的值等於在教材裡散播錯誤資訊
    }
}

// -----------------------------------------------------------------------------
// 【正確版】把「檢查 + 行動」封裝成單一原子操作
//   重點不只是加鎖，而是【改介面】：呼叫端拿不到「中間狀態」，
//   自然就不可能用錯。
// -----------------------------------------------------------------------------
class BoundedVector {
private:
    mutable std::mutex mtx;
    std::vector<int> data;
    const size_t limit;

public:
    explicit BoundedVector(size_t cap) : limit(cap) {}

    // ✓ 檢查與新增在同一個臨界區段內 → 不可能超過上限
    bool tryPush(int value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (data.size() >= limit) return false;
        data.push_back(value);
        return true;
    }

    // ✓ 檢查與取值合併成一個操作，用 optional 表達「可能沒有」
    std::optional<int> tryBack() const {
        std::lock_guard<std::mutex> lock(mtx);
        if (data.empty()) return std::nullopt;
        return data.back();       // 回傳複本，不是引用
    }

    std::optional<int> tryPop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (data.empty()) return std::nullopt;
        int v = data.back();
        data.pop_back();
        return v;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return data.size();
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】生產者-消費者的有界佇列
//   情境：日誌收集、訊息佇列、任務排程都會用「有界佇列」做背壓 (backpressure)：
//         佇列滿了就拒收或阻塞，避免記憶體被無限制的生產者撐爆。
//   天真寫法：if (q.size() < MAX) q.push(item);
//         → 多個生產者同時通過檢查，佇列長度衝破上限，
//           背壓機制形同虛設，記憶體用量失控。
//   正解：把「檢查容量 + 入列」做成單一個 tryPush()。
// -----------------------------------------------------------------------------
class BoundedQueue {
private:
    mutable std::mutex mtx;
    std::vector<std::string> items;
    const size_t capacity;
    size_t rejected = 0;

public:
    explicit BoundedQueue(size_t cap) : capacity(cap) {}

    bool tryPush(const std::string& item) {
        std::lock_guard<std::mutex> lock(mtx);
        if (items.size() >= capacity) {
            ++rejected;              // 背壓：據實拒收並計數
            return false;
        }
        items.push_back(item);
        return true;
    }

    std::optional<std::string> tryPop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (items.empty()) return std::nullopt;
        std::string v = items.back();
        items.pop_back();
        return v;
    }

    size_t size() const      { std::lock_guard<std::mutex> lock(mtx); return items.size(); }
    size_t rejectedCount() const { std::lock_guard<std::mutex> lock(mtx); return rejected; }
    size_t cap() const       { return capacity; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】快取的 get-or-compute（最經典的 check-then-act）
//   情境：查快取 → 沒有就去算 / 查資料庫 → 存回快取。
//   天真寫法：
//       if (!cache.count(key)) cache[key] = expensiveCompute(key);
//     兩條執行緒同時 miss，就會【重複計算】。若計算是「打外部 API」
//     或「建立資料庫連線」，重複的代價非常高（cache stampede，快取踩踏）。
//     更糟的是若 value 是 shared_ptr 之類的資源，還可能覆蓋掉別人剛放進去的。
//   本例用「一把鎖保護整段查詢與寫入」示範最單純的正解；
//   正式系統還會再加上 per-key 鎖或 std::shared_future 以避免鎖住整張表。
// -----------------------------------------------------------------------------
class ComputeCache {
private:
    mutable std::mutex mtx;
    std::map<int, long> cache;
    std::atomic<int> computeCalls{0};    // 統計實際算了幾次

    long expensiveCompute(int key) {
        ++computeCalls;
        long r = 0;
        for (int i = 0; i < 200; ++i) r += (key * 31 + i) % 97;
        return r;
    }

public:
    // ✓ 查詢與寫入在同一個臨界區段 → 同一個 key 只會被計算一次
    long get(int key) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;
        long v = expensiveCompute(key);
        cache[key] = v;
        return v;
    }

    int computeCount() const { return computeCalls.load(); }
    size_t entries() const { std::lock_guard<std::mutex> lock(mtx); return cache.size(); }
};

int main() {
    std::cout << "=== 錯誤示範：check-then-act（data race / UB）===\n";
    {
        std::thread t1([]{
            for (int i = 0; i < 15; ++i) {
                unsafeAppend(i);
            }
        });

        std::thread t2([]{
            for (int i = 0; i < 15; ++i) {
                unsafeAccess();
            }
        });

        t1.join();
        t2.join();

        std::cout << "vec.size() = " << vec.size()
                  << "（想限制在 10 以內，但這個數字不受任何保證）\n";
        std::cout << "註：本檔工作量極小，兩條執行緒常常沒有重疊執行，\n";
        std::cout << "    所以多數時候看起來完全正常 —— 這是測不出來，不是沒有錯。\n";
    }

    std::cout << "\n=== 正確版：把檢查與行動封裝成單一操作 ===\n";
    {
        BoundedVector bv(10);
        std::vector<std::thread> ths;
        std::atomic<int> accepted{0};

        // 8 條執行緒各嘗試塞 50 個，上限 10
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&bv, &accepted, i] {
                int local = 0;
                for (int k = 0; k < 50; ++k) {
                    if (bv.tryPush(i * 50 + k)) ++local;
                }
                accepted.fetch_add(local);
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "8 執行緒共嘗試 400 次，上限 10\n";
        std::cout << "成功次數: " << accepted.load() << " (必定 = 10)\n";
        std::cout << "最終大小: " << bv.size() << " (必定 = 10，絕不超過)\n";

        auto top = bv.tryBack();
        std::cout << "tryBack() 有值: " << std::boolalpha << top.has_value() << "\n";

        // 取空之後再取，安全地回傳 nullopt 而不是 UB
        while (bv.tryPop().has_value()) {}
        std::cout << "取空後 tryPop() 有值: " << bv.tryPop().has_value()
                  << " (回傳 nullopt，不是 UB)\n";
    }

    std::cout << "\n=== 日常實務 1：有界佇列的背壓 ===\n";
    {
        BoundedQueue q(100);
        std::vector<std::thread> producers;
        std::atomic<int> ok{0};

        for (int i = 0; i < 4; ++i) {
            producers.emplace_back([&q, &ok, i] {
                int local = 0;
                for (int k = 0; k < 200; ++k) {
                    if (q.tryPush("msg-" + std::to_string(i) + "-" + std::to_string(k)))
                        ++local;
                }
                ok.fetch_add(local);
            });
        }
        for (auto& t : producers) t.join();

        std::cout << "4 個生產者共送 800 則，佇列容量 " << q.cap() << "\n";
        std::cout << "入列成功: " << ok.load() << " (必定 = 100)\n";
        std::cout << "遭拒收數: " << q.rejectedCount() << " (必定 = 700)\n";
        std::cout << "佇列長度: " << q.size() << " (必定 = 100，背壓真的生效)\n";
    }

    std::cout << "\n=== 日常實務 2：快取 get-or-compute 不重複計算 ===\n";
    {
        ComputeCache cache;
        std::vector<std::thread> ths;

        // 8 條執行緒同時搶同一批 20 個 key
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&cache] {
                for (int k = 0; k < 20; ++k) cache.get(k);
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "8 執行緒 × 20 個 key = 160 次查詢\n";
        std::cout << "實際計算次數: " << cache.computeCount()
                  << " (必定 = 20，每個 key 只算一次)\n";
        std::cout << "快取項目數: " << cache.entries() << " (必定 = 20)\n";
        std::cout << "→ 若寫成「先檢查再計算」而不加鎖，就會發生 cache stampede\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.4：資料競爭範例分析3.cpp' -o race3
//
// 偵測資料競爭（第一段是 UB，唯一可靠的判定方式）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.4：資料競爭範例分析3.cpp' -o race3_tsan
//   ./race3_tsan  → 會報出 unsafeAppend 的 push_back 與 unsafeAccess 的讀取衝突

// ⚠️ 只有第一段的 vec.size() 【不受任何保證】——該段是 genuine data race → UB，
// 而且 unsafeAccess 可能讀到已失效的記憶體（本檔刻意【不印出】該值，
// 因為把不確定的值當成預期輸出等於在教材裡散播錯誤資訊）。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）多次實測都印出 10，
// 原因是各只跑 15 次迴圈、兩條執行緒幾乎沒有重疊 —— 這是測不出來，不是沒有錯。
// 其餘各段（BoundedVector、有界佇列、快取）皆由鎖保證，為確定值。

// === 預期輸出 ===
// === 錯誤示範：check-then-act（data race / UB）===
// vec.size() = 10（想限制在 10 以內，但這個數字不受任何保證）
// 註：本檔工作量極小，兩條執行緒常常沒有重疊執行，
//     所以多數時候看起來完全正常 —— 這是測不出來，不是沒有錯。
//
// === 正確版：把檢查與行動封裝成單一操作 ===
// 8 執行緒共嘗試 400 次，上限 10
// 成功次數: 10 (必定 = 10)
// 最終大小: 10 (必定 = 10，絕不超過)
// tryBack() 有值: true
// 取空後 tryPop() 有值: false (回傳 nullopt，不是 UB)
//
// === 日常實務 1：有界佇列的背壓 ===
// 4 個生產者共送 800 則，佇列容量 100
// 入列成功: 100 (必定 = 100)
// 遭拒收數: 700 (必定 = 700)
// 佇列長度: 100 (必定 = 100，背壓真的生效)
//
// === 日常實務 2：快取 get-or-compute 不重複計算 ===
// 8 執行緒 × 20 個 key = 160 次查詢
// 實際計算次數: 20 (必定 = 20，每個 key 只算一次)
// 快取項目數: 20 (必定 = 20)
// → 若寫成「先檢查再計算」而不加鎖，就會發生 cache stampede
