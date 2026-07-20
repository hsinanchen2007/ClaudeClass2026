//
// =============================================================================
//  課程 5.6：互斥鎖的效能考量3.cpp  —  一把大鎖 vs 每個桶一把鎖
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    鎖的粒度 (lock granularity)；粗粒度與細粒度的量化比較
//   語法：    std::array<std::mutex, N> mutexes;   // 每個桶一把鎖
//   標準版本：std::array 為 C++11；std::mutex 不可複製也不可移動，
//             所以 std::array<std::mutex, N> 只能預設建構、不能複製整個陣列
//   標頭檔：  <array>、<mutex>、<chrono>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//   ⚠️ 所有時間數字都是本機實測值，每次執行都不同，非標準保證。
//
// 【詳細解釋 Explanation】
//
// 【1. 粒度的定義：一把鎖保護多少東西】
//   * 粗粒度 (coarse-grained)：一把鎖保護整個資料結構。
//     優點是簡單、不可能死結、程式碼好懂。
//     缺點是所有操作互相排斥 —— 即使兩條執行緒要動的是完全不相干的資料。
//   * 細粒度 (fine-grained)：把資料切成 N 份，每份一把鎖。
//     優點是不相干的操作可以真正並行。
//     缺點是複雜、記憶體變多、而且【一旦需要同時鎖多把就有死結風險】。
//   本檔的實測顯示：執行緒越多，細粒度的優勢越明顯 ——
//   因為粗粒度的競爭隨執行緒數快速惡化，細粒度則把競爭分散開。
//
// 【2. 為什麼「每個桶一把鎖」有效】
//   本檔的 worker 讓每條執行緒存取不同的桶（`threadId * 1000 + i`）。
//   在粗粒度版本中，這些不相干的操作仍然全部排隊，因為只有一把鎖；
//   在細粒度版本中，它們落在不同的桶、拿不同的鎖，可以真正同時進行。
//   → 關鍵洞見：【鎖的粒度應該對應「資料的獨立性」】。
//     若兩份資料在邏輯上互不相干，就不該共用一把鎖。
//     這正是 Java 的 ConcurrentHashMap、Linux 核心的多種分段鎖、
//     以及各種 sharded cache 的核心設計。
//
// 【3. 細粒度不是免費的，三個代價】
//   ① 記憶體：本機 sizeof(std::mutex) = 40 bytes（實作定義值，實測見輸出）。
//      16 個桶就是 640 bytes；若切成 1024 片就是 40 KB，
//      而且這些鎖會佔用寶貴的快取空間。
//   ② 死結風險：只要有任何操作需要【同時】鎖兩個桶（例如把元素從
//      桶 A 搬到桶 B、或計算跨桶的總和），就必須用 std::scoped_lock
//      或固定加鎖順序，否則就是 AB-BA 死結。
//      粗粒度版本天生沒有這個問題。
//   ③ 一致性變難：粗粒度版本的 total() 天然是一致的快照；
//      細粒度版本要取得「所有桶在同一瞬間的總和」，
//      得鎖住所有桶（那就退化成粗粒度），或接受近似值。
//      本檔的 total() 就是【沒有加鎖的近似值】——見下方注意事項。
//
// 【4. 本檔原始碼中的兩個真實缺陷（保留並說明，不假裝沒看到）】
//   ① CoarseGrainedCounters::get() 與 total() 【沒有加鎖】。
//      increment() 有鎖，但讀取端沒有 → 一寫一讀而無同步 = data race → UB。
//      正確做法是讀取端也用同一把鎖（本檔在下方的修正版示範）。
//   ② FineGrainedCounters::total() 逐桶讀取但完全沒有鎖，
//      即使每個桶各自加鎖，「跨桶加總」仍然拿不到一致的快照。
//      這正是【3-③】說的代價：細粒度讓「全域一致的觀察」變得昂貴。
//   本檔的 main 在所有執行緒 join 之後才呼叫 total()，
//   所以實際執行時沒有並行存取、不會觸發 UB；但這個介面本身是不安全的。
//
// 【5. 粒度該選多細：沒有通則，只有量測】
//   桶數太少 → 競爭仍然嚴重；桶數太多 → 記憶體浪費、快取效率下降，
//   而且超過核心數之後幾乎沒有額外收益（同時只有這麼多執行緒在跑）。
//   常見的起點是「核心數的 2~4 倍」，然後用本檔這種掃描式量測去調。
//   ⚠️ 更重要的是：先確認鎖真的是瓶頸再來調粒度。
//     多數效能問題其實出在演算法或 I/O，不在鎖。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 std::array<std::mutex, N> 可以，std::vector<std::mutex> 卻很麻煩
//   std::mutex 不可複製也不可移動。std::array 是固定大小、
//   元素就地預設建構，完全沒問題。
//   但 std::vector 需要在重新配置時【移動】元素，
//   所以 vector<mutex> 雖然能宣告，卻不能 push_back、不能 resize、
//   也不能用初始值建構。要動態數量的鎖，
//   通常改用 std::vector<std::unique_ptr<std::mutex>>
//   或 std::deque<std::mutex>（deque 不會搬動既有元素）。
//
// (B) false sharing：細粒度也可能被快取拖累
//   16 個 mutex 排在一起時，多個 mutex 可能落在同一條 cache line
//   （本機 cache line 64 bytes，sizeof(std::mutex) = 40 bytes，
//   所以相鄰兩把鎖幾乎必定跨或共用快取行）。
//   兩條執行緒鎖「不同的鎖」，卻因為兩把鎖在同一條快取行上而互相失效 ——
//   細粒度的效益就被吃掉一部分。
//   正式做法是把每個桶（鎖 + 資料）用 alignas(64) 對齊，
//   本檔在下方的 PaddedBucket 示範這個修正。
//
// (C) 除了分片，還有哪些降低競爭的路
//   * std::shared_mutex（C++17）：讀多寫少時讓讀者並行。
//   * copy-on-write + shared_ptr：讀取端幾乎零成本（見 5.5-2）。
//   * 完全不共享：每執行緒本地累加，最後合併（見 5.6-2 / 5.6-4）。
//   → 分片鎖適合「讀寫都多、且存取分佈均勻」的情況；
//     若存取集中在少數熱點 key，分片的效果會大打折扣。
//
// 【注意事項 Pay Attention】
// 1. 鎖的粒度應對應「資料的獨立性」；不相干的資料不該共用一把鎖。
// 2. 細粒度的三個代價：記憶體、死結風險、跨分片一致性變難。
// 3. 本檔原始的 get() / total() 未加鎖 —— 這是真實缺陷，下方有修正版對照。
// 4. std::mutex 不可複製移動 → 可用 std::array，不能用會重新配置的 std::vector。
// 5. 相鄰的 mutex 可能落在同一條 cache line 造成 false sharing，
//    需要 alignas 對齊才能拿到完整效益。
// 6. sizeof(std::mutex) 是實作定義值（本機為 40 bytes，程式會印出來驗證）。
// 7. 所有時間數字每次執行都不同；先確認鎖真的是瓶頸，再來調粒度。
//
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   （實作於下方 MyHashSet 類別）
//   題目：不使用內建雜湊表，自行設計 HashSet，支援 add / remove / contains。
//   為什麼用到本主題：標準解法就是【桶陣列 + 每桶一條鏈結串列】——
//         與本檔的 FineGrainedCounters 是完全相同的結構。
//         LeetCode 是單執行緒判題，但只要把同一個 HashSet 給多條執行緒用，
//         粒度的選擇就直接決定了吞吐量：
//           * 一把大鎖保護整張表 → 所有操作排隊（粗粒度）
//           * 每個桶一把鎖       → 不同桶的操作真正並行（細粒度）
//         這正是 Java ConcurrentHashMap 早期版本的 segment lock 設計，
//         也是本檔要教的東西在真實資料結構上的樣子。
//         注意 contains() 也必須加鎖 —— 只要有任何一方在寫，讀就要保護。
// -----------------------------------------------------------------------------
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】鎖的粒度
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是鎖的粒度?粗粒度和細粒度該怎麼選?
//     答：粒度是「一把鎖保護多少資料」。粗粒度（一把鎖保護整個結構）
//         簡單、不可能死結；細粒度（每個分片一把鎖）讓不相干的操作能真正並行。
//         選擇的原則是【讓鎖的粒度對應資料的獨立性】：
//         邏輯上互不相干的資料不該共用一把鎖。
//         但要先確認鎖真的是瓶頸 —— 多數效能問題出在演算法或 I/O。
//     追問：細粒度有什麼代價?
//         → 三個：記憶體（本機每把 mutex 40 bytes）、
//           死結風險（一旦需要同時鎖兩個分片）、
//           以及跨分片的一致性快照變得昂貴（要鎖全部才拿得到）。
//
// 🔥 Q2. 為什麼不能用 std::vector<std::mutex>?
//     答：std::mutex 不可複製也不可移動，而 vector 在重新配置時
//         需要移動既有元素。所以 vector<mutex> 雖然能宣告，
//         卻不能 push_back / resize / 用初始值建構。
//         固定數量請用 std::array<std::mutex, N>；
//         需要動態數量就用 std::vector<std::unique_ptr<std::mutex>>
//         或 std::deque<std::mutex>（deque 不搬動既有元素）。
//     追問：那 std::array<std::mutex, 16> 有沒有隱藏成本?
//         → 有：相鄰的 mutex 可能落在同一條 cache line
//           （本機 line 64 bytes、mutex 40 bytes），
//           兩條執行緒鎖「不同的鎖」卻互相使快取失效（false sharing）。
//           要拿到完整效益必須用 alignas(64) 把每個分片對齊。
//
// ⚠️ 陷阱. 「我把一把鎖拆成 16 把，並行度應該就提升 16 倍了」——錯在哪?
//     答：只有在【存取分佈均勻】時才接近成立。若 90% 的請求都打在
//         同一個熱點 key 上，那 16 把鎖裡有 15 把是閒置的，
//         競爭完全沒有下降。而且分片本身還帶來記憶體、false sharing、
//         以及跨分片操作的死結風險 —— 有可能比原本更慢。
//     為什麼會錯：把「鎖的數量」當成並行度的直接指標。
//         真正決定並行度的是【實際存取的分佈】。
//         正確的做法是先量測熱點分佈，再決定要不要分片、分幾片；
//         熱點集中時該做的是快取、批次化或改演算法，而不是加鎖數量。
// ═══════════════════════════════════════════════════════════════════════════
// 檔案：lesson_5_6_lock_granularity.cpp
// 說明：比較粗粒度鎖與細粒度鎖的效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <iomanip>
#include <memory>

const int NUM_BUCKETS = 16;
const int ITERATIONS = 100000;

// 粗粒度鎖：一個鎖保護所有資料
class CoarseGrainedCounters {
private:
    std::mutex mtx;
    std::array<long long, NUM_BUCKETS> counters{};
    
public:
    void increment(int bucket) {
        std::lock_guard<std::mutex> lock(mtx);  // 鎖整個結構
        ++counters[bucket % NUM_BUCKETS];
    }
    
    long long get(int bucket) const {
        return counters[bucket % NUM_BUCKETS];
    }
    
    long long total() const {
        long long sum = 0;
        for (auto c : counters) sum += c;
        return sum;
    }
};

// 細粒度鎖：每個桶一個鎖
class FineGrainedCounters {
private:
    std::array<std::mutex, NUM_BUCKETS> mutexes;
    std::array<long long, NUM_BUCKETS> counters{};
    
public:
    void increment(int bucket) {
        int idx = bucket % NUM_BUCKETS;
        std::lock_guard<std::mutex> lock(mutexes[idx]);  // 只鎖一個桶
        ++counters[idx];
    }
    
    long long get(int bucket) const {
        return counters[bucket % NUM_BUCKETS];
    }
    
    long long total() const {
        long long sum = 0;
        for (auto c : counters) sum += c;
        return sum;
    }
};

template<typename Counter>
void worker(Counter& counter, int threadId, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        // 每個執行緒存取不同的桶（模擬真實場景）
        counter.increment(threadId * 1000 + i);
    }
}

template<typename Counter>
double runTest(int numThreads) {
    Counter counter;
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker<Counter>, std::ref(counter), i, ITERATIONS);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}


// -----------------------------------------------------------------------------
// 【修正版】把上面點出的缺陷補起來：讀取端也加鎖 + cache line 對齊
// -----------------------------------------------------------------------------
struct alignas(64) PaddedBucket {          // 對齊到 cache line，消除 false sharing
    std::mutex mtx;
    long counter = 0;
};

class CorrectFineGrained {
private:
    std::array<PaddedBucket, NUM_BUCKETS> buckets;

public:
    void increment(int bucket) {
        auto& b = buckets[static_cast<size_t>(bucket % NUM_BUCKETS)];
        std::lock_guard<std::mutex> lock(b.mtx);
        ++b.counter;
    }

    // ✓ 讀取端也加鎖（原始版本沒有 → data race）
    long get(int bucket) {
        auto& b = buckets[static_cast<size_t>(bucket % NUM_BUCKETS)];
        std::lock_guard<std::mutex> lock(b.mtx);
        return b.counter;
    }

    // ✓ 要「一致的」總和就必須鎖住所有桶 —— 這正是細粒度的代價
    long totalConsistent() {
        std::vector<std::unique_lock<std::mutex>> locks;
        locks.reserve(NUM_BUCKETS);
        for (auto& b : buckets) locks.emplace_back(b.mtx);   // 固定順序 → 不會死結
        long sum = 0;
        for (auto& b : buckets) sum += b.counter;
        return sum;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 705. Design HashSet
//   桶陣列 + 每桶一條鏈結串列（用 vector 表示）；每個桶一把鎖 → 細粒度。
//   這與本檔的 FineGrainedCounters 是同一個結構，只是桶裡放的是元素而非計數。
// -----------------------------------------------------------------------------
class MyHashSet {
private:
    static const size_t BUCKETS = 769;     // 質數，減少雜湊碰撞
    struct alignas(64) Bucket {
        std::mutex mtx;
        std::vector<int> items;
    };
    std::vector<Bucket> buckets;           // 注意：Bucket 含 mutex，
                                           // 所以只能在建構時決定大小、之後不可 resize

    static size_t hash(int key) { return static_cast<size_t>(key) % BUCKETS; }

public:
    MyHashSet() : buckets(BUCKETS) {}

    void add(int key) {
        Bucket& b = buckets[hash(key)];
        std::lock_guard<std::mutex> lock(b.mtx);       // 只鎖這一個桶
        for (int v : b.items) if (v == key) return;    // 檢查與新增在同一臨界區段
        b.items.push_back(key);
    }

    void remove(int key) {
        Bucket& b = buckets[hash(key)];
        std::lock_guard<std::mutex> lock(b.mtx);
        for (size_t i = 0; i < b.items.size(); ++i) {
            if (b.items[i] == key) {
                b.items[i] = b.items.back();
                b.items.pop_back();
                return;
            }
        }
    }

    // ✓ 讀取也要鎖：只要有任何一方在寫，讀就必須保護
    bool contains(int key) {
        Bucket& b = buckets[hash(key)];
        std::lock_guard<std::mutex> lock(b.mtx);
        for (int v : b.items) if (v == key) return true;
        return false;
    }

    size_t size() {
        size_t n = 0;
        for (auto& b : buckets) {
            std::lock_guard<std::mutex> lock(b.mtx);
            n += b.items.size();
        }
        return n;   // 註：逐桶加總，不是一致快照（同 totalConsistent 的取捨）
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】連線追蹤表：熱點分佈決定分片是否有效
//   情境：閘道器要記錄每個 client IP 的連線數。用分片鎖是標準做法。
//   但真實流量常有熱點（少數大客戶佔多數流量）。
//   本例同時量測「均勻分佈」與「熱點集中」兩種存取模式，
//   證明分片的效益【完全取決於分佈】，不是鎖多就一定快。
// -----------------------------------------------------------------------------
double runHashSetTest(int numThreads, int opsPerThread, bool hotspot) {
    MyHashSet set;
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&set, i, opsPerThread, hotspot] {
            for (int k = 0; k < opsPerThread; ++k) {
                // hotspot=true：所有執行緒都打同一個 key（全部擠進同一個桶）
                // hotspot=false：key 分散（落在不同桶，可真正並行）
                int key = hotspot ? 42 : (i * opsPerThread + k);
                set.add(key);
                set.contains(key);
            }
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    std::cout << "=== 鎖粒度效能比較 ===" << std::endl;
    std::cout << "桶數量：" << NUM_BUCKETS << std::endl;
    std::cout << "每執行緒迭代：" << ITERATIONS << std::endl << std::endl;
    
    std::cout << std::setw(10) << "執行緒數"
              << std::setw(18) << "粗粒度 (ms)"
              << std::setw(18) << "細粒度 (ms)"
              << std::setw(12) << "加速比"
              << std::endl;
    std::cout << std::string(58, '-') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    for (int numThreads : {1, 2, 4, 8, 16}) {
        double coarseTime = runTest<CoarseGrainedCounters>(numThreads);
        double fineTime = runTest<FineGrainedCounters>(numThreads);
        double speedup = coarseTime / fineTime;
        
        std::cout << std::setw(10) << numThreads
                  << std::setw(18) << coarseTime
                  << std::setw(18) << fineTime
                  << std::setw(11) << speedup << "x"
                  << std::endl;
    }
    std::cout << "→ 執行緒越多，細粒度的優勢越明顯：" << std::endl;
    std::cout << "  粗粒度的競爭隨執行緒數快速惡化，細粒度把競爭分散到 16 把鎖。" << std::endl;

    std::cout << "\n=== 細粒度的成本（實作定義值）===" << std::endl;
    std::cout << "sizeof(std::mutex)   = " << sizeof(std::mutex) << " bytes" << std::endl;
    std::cout << "16 把鎖的記憶體      = " << (sizeof(std::mutex) * NUM_BUCKETS) << " bytes" << std::endl;
    std::cout << "sizeof(PaddedBucket) = " << sizeof(PaddedBucket)
              << " bytes（已對齊到 cache line）" << std::endl;
    std::cout << "→ 對齊之後每個桶佔一整條 cache line，記憶體換來的是消除 false sharing"
              << std::endl;

    std::cout << "\n=== 修正版：讀取端也加鎖 + 一致的總和 ===" << std::endl;
    {
        CorrectFineGrained c;
        std::vector<std::thread> ths;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&c, i] {
                for (int k = 0; k < 50000; ++k) c.increment(i * 1000 + k);
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "8 執行緒 × 50000 次遞增" << std::endl;
        std::cout << "一致的總和: " << c.totalConsistent()
                  << "（必定為 400000）" << std::endl;
        std::cout << "→ totalConsistent() 必須鎖住全部 16 把鎖才拿得到一致快照，" << std::endl;
        std::cout << "  這就是細粒度的代價：全域觀察變貴了。" << std::endl;
    }

    std::cout << "\n=== LeetCode 705. Design HashSet ===" << std::endl;
    {
        MyHashSet s;
        s.add(1);
        s.add(2);
        std::cout << "contains(1) = " << std::boolalpha << s.contains(1) << "  (預期 true)"  << std::endl;
        std::cout << "contains(3) = " << s.contains(3) << "  (預期 false)" << std::endl;
        s.add(2);                     // 重複加入不應改變內容
        s.remove(2);
        std::cout << "remove(2) 後 contains(2) = " << s.contains(2) << "  (預期 false)" << std::endl;
        std::cout << "size = " << s.size() << "  (預期 1)" << std::endl;

        // 多執行緒壓力測試
        MyHashSet shared;
        std::vector<std::thread> ths;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&shared, i] {
                for (int k = 0; k < 5000; ++k) shared.add(i * 5000 + k);
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "8 執行緒各加入 5000 個相異 key，size = " << shared.size()
                  << "（必定為 40000）" << std::endl;
    }

    std::cout << "\n=== 日常實務：分片的效益取決於存取分佈 ===" << std::endl;
    {
        runHashSetTest(8, 2000, false);      // 預熱
        runHashSetTest(8, 2000, true);

        double uniform = runHashSetTest(8, 20000, false);
        double hot = runHashSetTest(8, 20000, true);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "key 均勻分佈（落在不同桶）：" << uniform << " ms" << std::endl;
        std::cout << "key 全部集中（同一個桶）  ：" << hot << " ms" << std::endl;
        std::cout << "→ 同樣是 769 個桶、同樣的操作次數，差別只在分佈。" << std::endl;
        std::cout << "  熱點集中時 768 把鎖是閒置的，分片完全沒有幫助 ——" << std::endl;
        std::cout << "  這時該做的是快取或改演算法，而不是再加鎖的數量。" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量3.cpp' -o lock_granularity

// ⚠️ 本檔的【時間數字每次執行都不同】（效能量測，受頻率調節、排程、負載影響），
// 下面貼的是本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）某一次的真實實測。
// 穩定成立的是【趨勢】：執行緒越多，細粒度相對粗粒度的優勢越大；
// 以及熱點集中時分片完全失效（該次實測反而比均勻分佈慢一倍以上）。
//
// 以下數值則是確定的：
//   * sizeof(std::mutex) = 40、sizeof(PaddedBucket) = 64
//     —— 這是本機 x86-64 / libstdc++ 的【實作定義】值，非標準保證。
//   * 一致的總和 400000、HashSet 的 size 1 與 40000 —— 正確性驗證，每次相同。

// === 預期輸出 ===
// === 鎖粒度效能比較 ===
// 桶數量：16
// 每執行緒迭代：100000
//
// 執行緒數    粗粒度 (ms)    細粒度 (ms)   加速比
// ----------------------------------------------------------
//          1              2.19              2.47       0.89x
//          2             18.73              5.63       3.33x
//          4             34.63             10.76       3.22x
//          8             94.71             21.35       4.44x
//         16            201.42             36.11       5.58x
// → 執行緒越多，細粒度的優勢越明顯：
//   粗粒度的競爭隨執行緒數快速惡化，細粒度把競爭分散到 16 把鎖。
//
// === 細粒度的成本（實作定義值）===
// sizeof(std::mutex)   = 40 bytes
// 16 把鎖的記憶體      = 640 bytes
// sizeof(PaddedBucket) = 64 bytes（已對齊到 cache line）
// → 對齊之後每個桶佔一整條 cache line，記憶體換來的是消除 false sharing
//
// === 修正版：讀取端也加鎖 + 一致的總和 ===
// 8 執行緒 × 50000 次遞增
// 一致的總和: 400000（必定為 400000）
// → totalConsistent() 必須鎖住全部 16 把鎖才拿得到一致快照，
//   這就是細粒度的代價：全域觀察變貴了。
//
// === LeetCode 705. Design HashSet ===
// contains(1) = true  (預期 true)
// contains(3) = false  (預期 false)
// remove(2) 後 contains(2) = false  (預期 false)
// size = 1  (預期 1)
// 8 執行緒各加入 5000 個相異 key，size = 40000（必定為 40000）
//
// === 日常實務：分片的效益取決於存取分佈 ===
// key 均勻分佈（落在不同桶）：19.15 ms
// key 全部集中（同一個桶）  ：43.76 ms
// → 同樣是 769 個桶、同樣的操作次數，差別只在分佈。
//   熱點集中時 768 把鎖是閒置的，分片完全沒有幫助 ——
//   這時該做的是快取或改演算法，而不是再加鎖的數量。
